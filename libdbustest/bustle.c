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

#include <glib.h>
#include "dbus-test.h"

struct _DbusTestBustlePrivate {
	gchar * filename;
	gchar * executable;

	guint watch;
	GIOChannel * stdout;
	GIOChannel * stderr;
	GIOChannel * file;
	GPid pid;

	gboolean crashed;
};

#define DBUS_TEST_BUSTLE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_BUSTLE, DbusTestBustlePrivate))

static void dbus_test_bustle_class_init (DbusTestBustleClass *klass);
static void dbus_test_bustle_init       (DbusTestBustle *self);
static void dbus_test_bustle_dispose    (GObject *object);
static void dbus_test_bustle_finalize   (GObject *object);
static void process_run                 (DbusTestTask * task);
static DbusTestTaskState get_state      (DbusTestTask * task);
static gboolean get_passed              (DbusTestTask * task);
static gboolean bustle_writes           (GIOChannel *          channel,
                                         GIOCondition          condition,
                                         gpointer              data);
static gboolean bustle_write_error      (GIOChannel *          channel,
                                         GIOCondition          condition,
                                         gpointer              data);

G_DEFINE_TYPE (DbusTestBustle, dbus_test_bustle, DBUS_TEST_TYPE_TASK);

static void
dbus_test_bustle_class_init (DbusTestBustleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestBustlePrivate));

	object_class->dispose = dbus_test_bustle_dispose;
	object_class->finalize = dbus_test_bustle_finalize;

	DbusTestTaskClass * task_class = DBUS_TEST_TASK_CLASS(klass);

	task_class->run = process_run;
	task_class->get_state = get_state;
	task_class->get_passed = get_passed;

	return;
}

static void
dbus_test_bustle_init (DbusTestBustle *self)
{
	self->priv = DBUS_TEST_BUSTLE_GET_PRIVATE(self);

	self->priv->filename = g_strconcat(g_get_current_dir(), G_DIR_SEPARATOR_S, "bustle.log", NULL);
	self->priv->executable = g_strdup(BUSTLE_DUAL_MONITOR);

	self->priv->watch = 0;
	self->priv->stdout = NULL;
	self->priv->stderr = NULL;
	self->priv->file = NULL;
	self->priv->pid = 0;

	self->priv->crashed = FALSE;

	return;
}

static void
dbus_test_bustle_dispose (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_BUSTLE(object));
	DbusTestBustle * bustler = DBUS_TEST_BUSTLE(object);

	if (bustler->priv->watch != 0) {
		g_source_remove(bustler->priv->watch);
		bustler->priv->watch = 0;
	}

	if (bustler->priv->pid != 0) {
		/* TODO: Send a single dbus message */

		g_spawn_close_pid(bustler->priv->pid);
	}

	if (bustler->priv->stdout != NULL) {
		while (G_IO_IN & g_io_channel_get_buffer_condition(bustler->priv->stdout)) {
			bustle_writes(bustler->priv->stdout, 0 /* unused */, bustler->priv->file);
		}

		g_clear_pointer(&bustler->priv->stdout, g_io_channel_unref);
	}

	if (bustler->priv->stderr != NULL) {
		while (G_IO_IN & g_io_channel_get_buffer_condition(bustler->priv->stderr)) {
			bustle_write_error(bustler->priv->stderr, 0 /* unused */, bustler);
		}

		g_clear_pointer(&bustler->priv->stderr, g_io_channel_unref);
	}

	if (bustler->priv->file != NULL) {
		g_io_channel_shutdown(bustler->priv->file, TRUE, NULL);
		g_clear_pointer(&bustler->priv->file, g_io_channel_unref);
	}

	G_OBJECT_CLASS (dbus_test_bustle_parent_class)->dispose (object);
	return;
}

static void
dbus_test_bustle_finalize (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_BUSTLE(object));
	DbusTestBustle * bustler = DBUS_TEST_BUSTLE(object);

	g_free(bustler->priv->filename);
	g_free(bustler->priv->executable);

	G_OBJECT_CLASS (dbus_test_bustle_parent_class)->finalize (object);
	return;
}

DbusTestBustle *
dbus_test_bustle_new (const gchar * filename)
{
	g_return_val_if_fail(filename != NULL, NULL);

	DbusTestBustle * bustler = g_object_new(DBUS_TEST_TYPE_BUSTLE,
	                                        NULL);

	g_free(bustler->priv->filename);
	bustler->priv->filename = g_strdup(filename);

	dbus_test_task_set_name(DBUS_TEST_TASK(bustler), "Bustle");

	return bustler;
}

void
dbus_test_bustle_set_executable (DbusTestBustle * bustle, const gchar * executable)
{
	g_return_if_fail(DBUS_TEST_IS_BUSTLE(bustle));
	g_return_if_fail(executable != NULL);

	g_free(bustle->priv->executable);
	bustle->priv->executable = g_strdup(executable);

	return;
}

static void
bustle_watcher (GPid pid, G_GNUC_UNUSED gint status, gpointer data)
{
	g_critical("Bustle Monitor exited abruptly!");
	DbusTestBustle * bustler = DBUS_TEST_BUSTLE(data);

	if (bustler->priv->pid != 0) {
		g_spawn_close_pid(pid);
		bustler->priv->pid = 0;
	}

	bustler->priv->crashed = TRUE;
	g_signal_emit_by_name(G_OBJECT(bustler), DBUS_TEST_TASK_SIGNAL_STATE_CHANGED, DBUS_TEST_TASK_STATE_FINISHED, NULL);

	return;
}

static gboolean
bustle_write_error (GIOChannel * channel, G_GNUC_UNUSED GIOCondition condition, gpointer data)
{
	gchar * line;
	gsize termloc;

	do {
		GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);

		if (status == G_IO_STATUS_EOF) {
			return FALSE;
		}

		if (status != G_IO_STATUS_NORMAL) {
			continue;
		}

		line[termloc] = '\0';

		dbus_test_task_print(DBUS_TEST_TASK(data), line);
		g_free(line);
	} while (G_IO_IN & g_io_channel_get_buffer_condition(channel));

	return TRUE;
}

static gboolean
bustle_writes (GIOChannel * channel, G_GNUC_UNUSED GIOCondition condition, gpointer data)
{
	gchar * line;
	gsize termloc;

	GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);

	if (status != G_IO_STATUS_NORMAL) {
		return FALSE;
	}

	g_io_channel_write_chars((GIOChannel *)data,
							 line,
							 termloc,
							 NULL,
							 NULL);
	g_io_channel_write_chars((GIOChannel *)data,
							 "\n",
							 1,
							 NULL,
							 NULL);

	g_free(line);

	return TRUE;
}

static void
process_run (DbusTestTask * task)
{
	g_return_if_fail(DBUS_TEST_IS_BUSTLE(task));
	DbusTestBustle * bustler = DBUS_TEST_BUSTLE(task);

	if (bustler->priv->pid != 0) {
		return;
	}
	
	GError * error = NULL;

	bustler->priv->file = g_io_channel_new_file(bustler->priv->filename, "w", &error);

	if (error != NULL) {
		g_critical("Unable to open bustle file '%s': %s", bustler->priv->filename, error->message);
		g_error_free(error);

		bustler->priv->crashed = TRUE;
		g_signal_emit_by_name(G_OBJECT(bustler), DBUS_TEST_TASK_SIGNAL_STATE_CHANGED, DBUS_TEST_TASK_STATE_FINISHED, NULL);
		return;
	}

	gint bustle_stdout_num;
	gint bustle_stderr_num;
	
	gchar ** bustle_monitor = g_new0(gchar *, 2);
	bustle_monitor[0] = (gchar *)bustler->priv->executable;

	g_spawn_async_with_pipes(g_get_current_dir(),
	                         bustle_monitor, /* argv */
	                         NULL, /* envp */
	                         /* G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL, */ /* flags */
	                         G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, /* flags */
	                         NULL, /* child setup func */
	                         NULL, /* child setup data */
	                         &bustler->priv->pid, /* PID */
	                         NULL, /* stdin */
	                         &bustle_stdout_num, /* stdout */
	                         &bustle_stderr_num, /* stderr */
	                         &error); /* error */

	if (error != NULL) {
		g_critical("Unable to start bustling data: %s", error->message);
		g_error_free(error);

		bustler->priv->pid = 0; /* ensure this */
		bustler->priv->crashed = TRUE;
		g_signal_emit_by_name(G_OBJECT(bustler), DBUS_TEST_TASK_SIGNAL_STATE_CHANGED, DBUS_TEST_TASK_STATE_FINISHED, NULL);
		return;
	}

	if (TRUE) {
		gchar * start = g_strdup_printf("Starting bustle monitor.  PID: %d", bustler->priv->pid);
		dbus_test_task_print(DBUS_TEST_TASK(bustler), start);
		g_free(start);
	}
	bustler->priv->watch = g_child_watch_add(bustler->priv->pid, bustle_watcher, bustler);

	bustler->priv->stdout = g_io_channel_unix_new(bustle_stdout_num);
	g_io_add_watch(bustler->priv->stdout,
	               G_IO_IN | G_IO_PRI, /* conditions */
	               bustle_writes, /* func */
	               bustler->priv->file); /* func data */

	bustler->priv->stderr = g_io_channel_unix_new(bustle_stderr_num);
	g_io_add_watch(bustler->priv->stderr,
	               G_IO_IN, /* conditions */
	               bustle_write_error, /* func */
	               bustler); /* func data */

	return;
}

static DbusTestTaskState
get_state (DbusTestTask * task)
{
	/* We're always finished, but we want an error */
	g_return_val_if_fail(DBUS_TEST_IS_BUSTLE(task), DBUS_TEST_TASK_STATE_FINISHED);
	return DBUS_TEST_TASK_STATE_FINISHED;
}

static gboolean
get_passed (DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_BUSTLE(task), FALSE);
	DbusTestBustle * bustler = DBUS_TEST_BUSTLE(task);

	if (bustler->priv->crashed) {
		return FALSE;
	} else {
		return TRUE;
	}
}

