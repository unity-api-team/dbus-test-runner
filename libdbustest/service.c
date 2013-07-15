/*
Copyright 2012 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#include <glib.h>
#include "glib-compat.h"

#include "dbus-test.h"
#include "watchdog.h"

typedef enum _ServiceState ServiceState;
enum _ServiceState {
	STATE_INIT,
	STATE_DAEMON_STARTING,
	STATE_DAEMON_STARTED,
	STATE_STARTING,
	STATE_STARTED,
	STATE_RUNNING,
	STATE_FINISHED
};

struct _DbusTestServicePrivate {
	GQueue tasks_first;
	GQueue tasks_normal;
	GQueue tasks_last;

	GMainLoop * mainloop;
	ServiceState state;

	gboolean daemon_crashed;

	GPid dbus;
	guint dbus_watch;
	GIOChannel * dbus_io;
	guint dbus_io_watch;
	gchar * dbus_daemon;
	gchar * dbus_configfile;

	gboolean first_time;

	DbusTestWatchdog * watchdog;
	guint watchdog_source;
};

#define SERVICE_CHANGE_HANDLER  "dbus-test-service-change-handler"

#define DBUS_TEST_SERVICE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_SERVICE, DbusTestServicePrivate))

static void dbus_test_service_class_init (DbusTestServiceClass *klass);
static void dbus_test_service_init       (DbusTestService *self);
static void dbus_test_service_dispose    (GObject *object);
static void dbus_test_service_finalize   (GObject *object);
static gboolean watchdog_ping            (gpointer user_data);

G_DEFINE_TYPE (DbusTestService, dbus_test_service, G_TYPE_OBJECT);

static void
dbus_test_service_class_init (DbusTestServiceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestServicePrivate));

	object_class->dispose = dbus_test_service_dispose;
	object_class->finalize = dbus_test_service_finalize;

	return;
}

static void
dbus_test_service_init (DbusTestService *self)
{
	self->priv = DBUS_TEST_SERVICE_GET_PRIVATE(self);

	g_queue_init(&self->priv->tasks_first);
	g_queue_init(&self->priv->tasks_normal);
	g_queue_init(&self->priv->tasks_last);

	self->priv->mainloop = g_main_loop_new(NULL, FALSE);
	self->priv->state = STATE_INIT;

	self->priv->daemon_crashed = FALSE;

	self->priv->dbus = 0;
	self->priv->dbus_watch = 0;
	self->priv->dbus_io = NULL;
	self->priv->dbus_io_watch = 0;
	self->priv->dbus_daemon = g_strdup("dbus-daemon");
	self->priv->dbus_configfile = g_strdup(DEFAULT_SESSION_CONF);

	self->priv->first_time = TRUE;

	self->priv->watchdog = g_object_new(DBUS_TEST_TYPE_WATCHDOG, NULL);
	self->priv->watchdog_source = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT,
	                                                         5,
	                                                         watchdog_ping,
	                                                         g_object_ref(self->priv->watchdog),
	                                                         g_object_unref);

	return;
}

static void
task_unref (gpointer data, G_GNUC_UNUSED gpointer user_data)
{
	DbusTestTask * task = DBUS_TEST_TASK(data);

	gulong handler = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(task), SERVICE_CHANGE_HANDLER));
	if (handler != 0) {
		g_signal_handler_disconnect(G_OBJECT(task), handler);
	}

	g_object_unref(task);
	return;
}

static void
dbus_test_service_dispose (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(object));
	DbusTestService * self = DBUS_TEST_SERVICE(object);

	if (!g_queue_is_empty(&self->priv->tasks_last)) {
		g_queue_foreach(&self->priv->tasks_last, task_unref, NULL);
		g_queue_clear(&self->priv->tasks_last);
	}

	if (!g_queue_is_empty(&self->priv->tasks_normal)) {
		g_queue_foreach(&self->priv->tasks_normal, task_unref, NULL);
		g_queue_clear(&self->priv->tasks_normal);
	}

	if (!g_queue_is_empty(&self->priv->tasks_first)) {
		g_queue_foreach(&self->priv->tasks_first, task_unref, NULL);
		g_queue_clear(&self->priv->tasks_first);
	}

	if (self->priv->dbus_watch != 0) {
		g_source_remove(self->priv->dbus_watch);
		self->priv->dbus_watch = 0;
	}

	if (self->priv->dbus_io_watch != 0) {
		g_source_remove(self->priv->dbus_io_watch);
		self->priv->dbus_io_watch = 0;
	}

	if (self->priv->dbus_io != NULL) {
		g_io_channel_shutdown(self->priv->dbus_io, TRUE, NULL);
		g_io_channel_unref(self->priv->dbus_io);
		self->priv->dbus_io = NULL;
	}

	g_print("DBus daemon: Shutdown\n");
	if (self->priv->dbus != 0) {
		gchar * cmd = g_strdup_printf("kill -9 %d", self->priv->dbus);
		g_spawn_command_line_async(cmd, NULL);
		g_free(cmd);

		g_spawn_close_pid(self->priv->dbus);
		self->priv->dbus = 0;
	}

	if (self->priv->mainloop != NULL) {
		g_main_loop_unref(self->priv->mainloop);
		self->priv->mainloop = NULL;
	}

	g_clear_object(&self->priv->watchdog);

	if (self->priv->watchdog_source != 0) {
		g_source_remove(self->priv->watchdog_source);
		self->priv->watchdog_source = 0;
	}

	G_OBJECT_CLASS (dbus_test_service_parent_class)->dispose (object);
	return;
}

static void
dbus_test_service_finalize (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(object));
	DbusTestService * self = DBUS_TEST_SERVICE(object);

	g_free(self->priv->dbus_daemon);
	self->priv->dbus_daemon = NULL;
	g_free(self->priv->dbus_configfile);
	self->priv->dbus_configfile = NULL;

	G_OBJECT_CLASS (dbus_test_service_parent_class)->finalize (object);
	return;
}

DbusTestService *
dbus_test_service_new (G_GNUC_UNUSED const gchar * address)
{
	DbusTestService * service = g_object_new(DBUS_TEST_TYPE_SERVICE,
	                                         NULL);

	/* TODO: Use the address */

	return service;
}

/* Ping the watchdog so that it knows we're still alive */
static gboolean
watchdog_ping (gpointer user_data)
{
	DbusTestWatchdog * watchdog = DBUS_TEST_WATCHDOG(user_data);

	dbus_test_watchdog_ping(watchdog);

	return TRUE;
}

static void
all_tasks_finished_helper (gpointer data, gpointer user_data)
{
	DbusTestTask * task = DBUS_TEST_TASK(data);
	gboolean * all_finished = (gboolean *)user_data;

	DbusTestTaskState state = dbus_test_task_get_state(task);
	DbusTestTaskReturn ret  = dbus_test_task_get_return(task);

	if (state != DBUS_TEST_TASK_STATE_FINISHED && (ret != DBUS_TEST_TASK_RETURN_IGNORE || dbus_test_task_get_wait_finished(task))) {
		*all_finished = FALSE;
	}

	return;
}

static void
all_tasks_started_helper (gpointer data, gpointer user_data)
{
	DbusTestTask * task = DBUS_TEST_TASK(data);
	gboolean * all_started = (gboolean *)user_data;

	DbusTestTaskState state = dbus_test_task_get_state(task);

	if (state == DBUS_TEST_TASK_STATE_INIT || state == DBUS_TEST_TASK_STATE_WAITING) {
		*all_started = FALSE;
	}

	return;
}

static gboolean
all_tasks (DbusTestService * service, GFunc helper)
{
	gboolean breaknow = TRUE;

	g_queue_foreach(&service->priv->tasks_first, helper, &breaknow);
	if (!breaknow) {
		return FALSE;
	}

	g_queue_foreach(&service->priv->tasks_normal, helper, &breaknow);
	if (!breaknow) {
		return FALSE;
	}

	g_queue_foreach(&service->priv->tasks_last, helper, &breaknow);
	if (!breaknow) {
		return FALSE;
	}

	return TRUE;
}

static void
task_set_name_length (gpointer data, gpointer user_data)
{
	DbusTestTask * task = DBUS_TEST_TASK(data);
	glong * length = (glong *)user_data;

	dbus_test_task_set_name_spacing(task, *length);
	return;
}

static void
task_get_name_length (gpointer data, gpointer user_data)
{
	DbusTestTask * task = DBUS_TEST_TASK(data);
	glong * length = (glong *)user_data;

	const gchar * name = dbus_test_task_get_name(task);
	g_return_if_fail(name != NULL);

	glong nlength = g_utf8_strlen(name, -1);
	*length = MAX(*length, nlength);

	return;
}

static void
normalize_name_lengths (DbusTestService * service)
{
	glong length = 0;

	g_queue_foreach(&service->priv->tasks_first, task_get_name_length, &length);
	g_queue_foreach(&service->priv->tasks_normal, task_get_name_length, &length);
	g_queue_foreach(&service->priv->tasks_last, task_get_name_length, &length);

	g_queue_foreach(&service->priv->tasks_first, task_set_name_length, &length);
	g_queue_foreach(&service->priv->tasks_normal, task_set_name_length, &length);
	g_queue_foreach(&service->priv->tasks_last, task_set_name_length, &length);

	return;
}

static void
task_starter (gpointer data, G_GNUC_UNUSED gpointer user_data)
{
	DbusTestTask * task = DBUS_TEST_TASK(data);

	dbus_test_task_run(task);

	return;
}

static gboolean
dbus_writes (GIOChannel * channel, GIOCondition condition, gpointer data)
{
	DbusTestService * service = DBUS_TEST_SERVICE(data);

	if (condition & G_IO_ERR) {
		g_critical("DBus writing failure!");
		return FALSE;
	}

	gchar * line;
	gsize termloc;
	GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);
	g_return_val_if_fail(status == G_IO_STATUS_NORMAL, FALSE);
	line[termloc] = '\0';

	g_print("DBus daemon: %s\n", line);

	if (service->priv->first_time) {
		service->priv->first_time = FALSE;

		g_setenv("DBUS_SESSION_BUS_ADDRESS", line, TRUE);
		g_setenv("DBUS_STARTER_ADDRESS", line, TRUE);
		g_setenv("DBUS_STARTER_BUS_TYPE", "session", TRUE);

		if (service->priv->state == STATE_DAEMON_STARTING) {
			g_main_loop_quit(service->priv->mainloop);
		}
	}

	g_free(line);

	return TRUE;
}

static void
dbus_watcher (GPid pid, G_GNUC_UNUSED gint status, gpointer data)
{
	DbusTestService * service = DBUS_TEST_SERVICE(data);
	g_critical("DBus Daemon exited abruptly!");

	service->priv->daemon_crashed = TRUE;
	g_main_loop_quit(DBUS_TEST_SERVICE(data)->priv->mainloop);

	if (pid != 0) {
		g_spawn_close_pid(pid);
	}

	return;
}

static void
dbus_child_setup ()
{
	setpgrp();
}

static void
start_daemon (DbusTestService * service)
{
	if (service->priv->dbus != 0) {
		return;
	}

	service->priv->state = STATE_DAEMON_STARTING;

	gint dbus_stdout = 0;
	GError * error = NULL;
	gchar * dbus_startup[] = {service->priv->dbus_daemon, "--config-file", service->priv->dbus_configfile, "--print-address", NULL};
	g_spawn_async_with_pipes(g_get_current_dir(),
	                         dbus_startup, /* argv */
	                         NULL, /* envp (inherit from parent) */
	                         G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, /* flags */
	                         (GSpawnChildSetupFunc) dbus_child_setup, /* child setup func */
	                         NULL, /* child setup data */
	                         &service->priv->dbus, /* PID */
	                         NULL, /* stdin */
	                         &dbus_stdout, /* stdout */
	                         NULL, /* stderr */
	                         &error); /* error */

	if (error != NULL) {
		g_critical("Unable to start dbus daemon: %s", error->message);
		service->priv->daemon_crashed = TRUE;
		return;
	}

	dbus_test_watchdog_add_pid(service->priv->watchdog, service->priv->dbus);

	service->priv->dbus_watch = g_child_watch_add(service->priv->dbus, dbus_watcher, service);

	service->priv->dbus_io = g_io_channel_unix_new(dbus_stdout);
	service->priv->dbus_io_watch = g_io_add_watch(service->priv->dbus_io,
	                                              G_IO_IN | G_IO_ERR, /* conditions */
	                                              dbus_writes, /* func */
	                                              service); /* func data */

	g_main_loop_run(service->priv->mainloop);
	service->priv->state = STATE_DAEMON_STARTED;

	return;
}

void
dbus_test_service_start_tasks (DbusTestService * service)
{
	g_return_if_fail(DBUS_TEST_SERVICE(service));

	start_daemon(service);
	g_return_if_fail(g_getenv("DBUS_SESSION_BUS_ADDRESS") != NULL);

	if (all_tasks(service, all_tasks_started_helper)) {
		/* If we have all started we can mark it as such as long
		   as we understand where we could hit this case */
		if (service->priv->state == STATE_INIT || service->priv->state == STATE_DAEMON_STARTED) {
			service->priv->state = STATE_STARTED;
		}
		return;
	}

	normalize_name_lengths(service);

	g_queue_foreach(&service->priv->tasks_first, task_starter, NULL);
	if (!g_queue_is_empty(&service->priv->tasks_first)) {
		g_usleep(100000);
	}

	g_queue_foreach(&service->priv->tasks_normal, task_starter, NULL);

	if (!g_queue_is_empty(&service->priv->tasks_last)) {
		g_usleep(100000);
	}
	g_queue_foreach(&service->priv->tasks_last, task_starter, NULL);

	if (!all_tasks(service, all_tasks_started_helper)) {
		service->priv->state = STATE_STARTING;
		g_main_loop_run(service->priv->mainloop);

		/* This should never happen, but let's be sure */
		g_return_if_fail(all_tasks(service, all_tasks_started_helper));
	}

	service->priv->state = STATE_STARTED;

	return;
}

static void
all_tasks_passed_helper (gpointer data, gpointer user_data)
{
	DbusTestTask * task = DBUS_TEST_TASK(data);
	gboolean * all_passed = (gboolean *)user_data;

	if (!dbus_test_task_passed(task)) {
		*all_passed = FALSE;
	}

	return;
}

static int
get_status (DbusTestService * service)
{
	if (service->priv->daemon_crashed) {
		return -1;
	}

	if (all_tasks(service, all_tasks_passed_helper)) {
		return 0;
	} else {
		return -1;
	}
}

int
dbus_test_service_run (DbusTestService * service)
{
	g_return_val_if_fail(DBUS_TEST_SERVICE(service), -1);

	dbus_test_service_start_tasks(service);
	g_return_val_if_fail(service->priv->state == STATE_STARTED, get_status(service));

	if (all_tasks(service, all_tasks_finished_helper)) {
		return get_status(service);
	}

	service->priv->state = STATE_RUNNING;
	g_main_loop_run(service->priv->mainloop);

	/* This should never happen, but let's be sure */
	g_return_val_if_fail(all_tasks(service, all_tasks_finished_helper), -1);
	service->priv->state = STATE_FINISHED;

	return get_status(service);
}

static void
task_state_changed (G_GNUC_UNUSED DbusTestTask * task, G_GNUC_UNUSED DbusTestTaskState state, gpointer user_data)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(user_data));
	DbusTestService * service = DBUS_TEST_SERVICE(user_data);

	if (service->priv->state == STATE_STARTING && all_tasks(service, all_tasks_started_helper)) {
		g_main_loop_quit(service->priv->mainloop);
		return;
	}

	if (service->priv->state == STATE_RUNNING && all_tasks(service, all_tasks_finished_helper)) {
		g_main_loop_quit(service->priv->mainloop);
		return;
	}

	return;
}

void
dbus_test_service_add_task (DbusTestService * service, DbusTestTask * task)
{
	return dbus_test_service_add_task_with_priority(service, task, DBUS_TEST_SERVICE_PRIORITY_NORMAL);
}

void
dbus_test_service_add_task_with_priority (DbusTestService * service, DbusTestTask * task, DbusTestServicePriority prio)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(service));
	g_return_if_fail(DBUS_TEST_IS_TASK(task));

	GQueue * queue = NULL;

	switch (prio) {
	case DBUS_TEST_SERVICE_PRIORITY_FIRST:
		queue = &service->priv->tasks_first;
		break;
	case DBUS_TEST_SERVICE_PRIORITY_NORMAL:
		queue = &service->priv->tasks_normal;
		break;
	case DBUS_TEST_SERVICE_PRIORITY_LAST:
		queue = &service->priv->tasks_last;
		break;
	default:
		g_assert_not_reached();
		break;
	}

	g_queue_push_tail(queue, g_object_ref(task));

	gulong connect = g_signal_connect(G_OBJECT(task), DBUS_TEST_TASK_SIGNAL_STATE_CHANGED, G_CALLBACK(task_state_changed), service);
	g_object_set_data(G_OBJECT(task), SERVICE_CHANGE_HANDLER, GUINT_TO_POINTER(connect));

	return;
}

void
dbus_test_service_set_daemon (DbusTestService * service, const gchar * daemon)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(service));
	g_free(service->priv->dbus_daemon);
	service->priv->dbus_daemon = g_strdup(daemon);
	return;
}

void
dbus_test_service_set_conf_file (DbusTestService * service, const gchar * conffile)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(service));
	g_free(service->priv->dbus_configfile);
	service->priv->dbus_configfile = g_strdup(conffile);
	return;
}

void
dbus_test_service_stop (DbusTestService * service)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(service));
	g_main_loop_quit(service->priv->mainloop);
	return;
}
