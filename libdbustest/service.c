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
#include <string.h>

#include <glib.h>
#include <gio/gio.h>
#include "glib-compat.h"

#include "dbus-test.h"

enum {
	PROP_0,
	PROP_ADDRESS,
	PROP_TEST_DBUS,
	PROP_LAST
};

static GParamSpec * properties[PROP_LAST];

typedef enum _ServiceState {
	STATE_INIT,
	STATE_BUS_STARTED,
	STATE_STARTING,
	STATE_STARTED,
	STATE_RUNNING,
	STATE_FINISHED
}
ServiceState;

struct _DbusTestServicePrivate {
	GQueue tasks_first;
	GQueue tasks_normal;
	GQueue tasks_last;

	GMainLoop * mainloop;
	ServiceState state;

	gchar * external_bus_address;

	GTestDBus * test_dbus;
	gboolean test_dbus_started_here;

	DbusTestServiceBus bus_type;
};

#define SERVICE_CHANGE_HANDLER  "dbus-test-service-change-handler"

#define DBUS_TEST_SERVICE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_SERVICE, DbusTestServicePrivate))

static void dbus_test_service_class_init (DbusTestServiceClass *klass);
static void dbus_test_service_init       (DbusTestService *self);
static void dbus_test_service_dispose    (GObject *object);
static void dbus_test_service_finalize   (GObject *object);
static void dbus_test_service_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void dbus_test_service_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

G_DEFINE_TYPE (DbusTestService, dbus_test_service, G_TYPE_OBJECT)

static void
dbus_test_service_class_init (DbusTestServiceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestServicePrivate));

	object_class->dispose = dbus_test_service_dispose;
	object_class->finalize = dbus_test_service_finalize;
	object_class->get_property = dbus_test_service_get_property;
	object_class->set_property = dbus_test_service_set_property;

	properties[PROP_0] = NULL;

	properties[PROP_TEST_DBUS] = g_param_spec_object (
		"test-dbus",
		"Test DBus",
		"Externally-owned test dbus. (Optional)",
		G_TYPE_TEST_DBUS,
		(GParamFlags)(G_PARAM_READWRITE |
		              G_PARAM_CONSTRUCT_ONLY |
		              G_PARAM_STATIC_STRINGS));

	properties[PROP_ADDRESS] = g_param_spec_string (
		"external-bus-address",
		"External Bus Address",
		"Address of externally-managed DBus for service to run on. (Optional)",
		NULL,
		(GParamFlags)(G_PARAM_READWRITE |
		              G_PARAM_CONSTRUCT_ONLY |
		              G_PARAM_STATIC_STRINGS));

	g_object_class_install_properties (object_class, PROP_LAST, properties);

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

	self->priv->bus_type = DBUS_TEST_SERVICE_BUS_SESSION;

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

#if 0
static void
stop_bus (DbusTestService * self)
{
	const GBusType type = service->priv->bus_type == DBUS_TEST_SERVICE_BUS_SYSTEM
		? G_BUS_TYPE_SYSTEM
		: G_BUS_TYPE_SESSION;
	GDBusConnection *bus = g_bus_get_sync(bus_type, NULL, NULL);
	GWeakRef wref;

	if (bus != NULL) {
	g_weak_ref_init (&wref, bus);
	g_object_unref(bus);
	g_test_dbus_stop (self->priv->test_dbus);

	unsigned int cleartry = 0;
	for(;;) {
		GObject *o = g_weak_ref_get(&wref);
		const gboolean done = (o == NULL) || (cleartry >= 100);
		if (!done) {
			g_print("%d", (int)G_OBJECT(o)->ref_count);
			g_usleep(G_USEC_PER_SEC/10);
			while (g_main_pending())
				g_main_iteration(TRUE);
			++cleartry;
		}
		g_clear_object(&o);
		if (done) {
			break;
		}
	}
}
#endif


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

	if (self->priv->test_dbus_started_here) {
		self->priv->test_dbus_started_here = FALSE;
		g_test_dbus_down (self->priv->test_dbus);
	}

	g_clear_object(&self->priv->test_dbus);

	g_clear_pointer(&self->priv->mainloop, g_main_loop_unref);

	G_OBJECT_CLASS (dbus_test_service_parent_class)->dispose (object);
	return;
}

static void
dbus_test_service_finalize (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(object));
	DbusTestService * self = DBUS_TEST_SERVICE(object);

	g_clear_pointer(&self->priv->external_bus_address, g_free);
}

static void
dbus_test_service_get_property (GObject *o, guint property_id, GValue *value, GParamSpec *pspec)
{
	DbusTestService * self = DBUS_TEST_SERVICE (o);

	switch (property_id) {
	case PROP_ADDRESS:
		g_value_set_string (value, self->priv->external_bus_address);
		break;

	case PROP_TEST_DBUS:
		g_value_set_object (value, self->priv->test_dbus);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (o, property_id, pspec);
	}
}

static void
dbus_test_service_set_property (GObject *o, guint property_id, const GValue *value, GParamSpec *pspec)
{
	DbusTestService * self = DBUS_TEST_SERVICE (o);

	switch (property_id) {
	case PROP_ADDRESS:
		g_free(self->priv->external_bus_address);
		self->priv->external_bus_address = g_value_dup_string(value);
		break;

	case PROP_TEST_DBUS:
		g_warn_if_fail(self->priv->test_dbus == NULL);
		self->priv->test_dbus = g_value_dup_object(value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (o, property_id, pspec);
	}
}

/**
 * dbus_test_service_new:
 * @mock: A #DbusTestDbusMock instance
 * @external_bus_address: Address of an externally-managed bus, or NULL
 *
 * Creates a new DbusTestService.
 *
 * Specify an external addr when you want to integrate libdbustest
 * with an already-running bus. See also dbus_test_service_new_with_test_bus()
 * for GTestDBus integration.
 *
 * If external_bus_address is NULL, DbusTestService will create and manage
 * the life cycle of its own test dbus.
 *
 * Return Value: (transfer full): A new DbusTestService
 */
DbusTestService *
dbus_test_service_new (const gchar* external_bus_address)
{
	DbusTestService * service;

	if (external_bus_address && *external_bus_address)
		service = g_object_new(DBUS_TEST_TYPE_SERVICE, "external-bus-address", external_bus_address, NULL);
	else
		service = g_object_new(DBUS_TEST_TYPE_SERVICE, NULL);

	return service;
}

/**
 * dbus_test_service_new_test_bus:
 * @mock: A #DbusTestDbusMock instance
 * @test_bus: An externally-owned GTestDBus
 *
 * Creates a new DbusTestService. Instead of creating its own bus,
 * the specified test_bus will be used.
 *
 * Return Value: (transfer full): A new DbusTestService
 */
DbusTestService *
dbus_test_service_new_with_test_bus (GTestDBus * test_dbus)
{
	g_return_val_if_fail(test_dbus != NULL, NULL);
	g_return_val_if_fail(G_IS_TEST_DBUS(test_dbus), NULL);

	DbusTestService * service = g_object_new(DBUS_TEST_TYPE_SERVICE, "test-dbus", test_dbus, NULL);

	return service;
}

/***
****
***/

static gboolean
all_tasks_finished_helper (G_GNUC_UNUSED DbusTestService * service, DbusTestTask * task, G_GNUC_UNUSED gpointer user_data)
{
	DbusTestTaskState state = dbus_test_task_get_state(task);
	DbusTestTaskReturn ret  = dbus_test_task_get_return(task);

	if (state != DBUS_TEST_TASK_STATE_FINISHED && (ret != DBUS_TEST_TASK_RETURN_IGNORE || dbus_test_task_get_wait_finished(task))) {
		return FALSE;
	}

	return TRUE;
}

static gboolean
all_tasks_started_helper (G_GNUC_UNUSED DbusTestService * service, DbusTestTask * task, G_GNUC_UNUSED gpointer user_data)
{
	DbusTestTaskState state = dbus_test_task_get_state(task);

	if (state == DBUS_TEST_TASK_STATE_INIT || state == DBUS_TEST_TASK_STATE_WAITING) {
		return FALSE;
	}

	return TRUE;
}

static gboolean
all_tasks_bus_match (DbusTestService * service, DbusTestTask * task, G_GNUC_UNUSED gpointer user_data)
{
	return service->priv->bus_type == DBUS_TEST_SERVICE_BUS_BOTH ||
		dbus_test_task_get_bus(task) == DBUS_TEST_SERVICE_BUS_BOTH ||
		dbus_test_task_get_bus(task) == service->priv->bus_type;
}

typedef struct {
	DbusTestService * service;
	gboolean passing;
	gpointer user_data;
	gboolean (*func) (DbusTestService * service, DbusTestTask * task, gpointer data);
} all_tasks_helper_data_t;

static void
all_tasks_helper (gpointer taskp, gpointer datap)
{
	all_tasks_helper_data_t * data = (all_tasks_helper_data_t *)datap;

	if (!data->passing) {
		/* Quick exit */
		return;
	}

	data->passing = data->func(data->service, DBUS_TEST_TASK(taskp), data->user_data);
}

static gboolean
all_tasks (DbusTestService * service, gboolean (*helper) (DbusTestService * service, DbusTestTask * task, gpointer user_data), gpointer user_data)
{
	all_tasks_helper_data_t data = {
		.passing = TRUE,
		.service = service,
		.func = helper,
		.user_data = user_data
	};

	g_queue_foreach(&service->priv->tasks_first, all_tasks_helper, &data);
	if (!data.passing) {
		return FALSE;
	}

	g_queue_foreach(&service->priv->tasks_normal, all_tasks_helper, &data);
	if (!data.passing) {
		return FALSE;
	}

	g_queue_foreach(&service->priv->tasks_last, all_tasks_helper, &data);
	if (!data.passing) {
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

static void
ensure_bus_is_up (DbusTestService * service)
{
	GTestDBus* test_dbus = service->priv->test_dbus;
	const gchar* address = NULL;

	if (service->priv->external_bus_address != NULL) {
		/* if the client specified an already-running bus,
		   we don't have to do any process management */
		address = service->priv->external_bus_address;
	} else {
		/* if the user didn't provide a GTestDBus, create our own */
		if (service->priv->test_dbus == NULL)
			service->priv->test_dbus = g_test_dbus_new (G_TEST_DBUS_NONE);

		/* start the test dbus */
		const gboolean bus_is_up = g_test_dbus_get_bus_address(service->priv->test_dbus) != NULL;
		if (!bus_is_up) {
			g_test_dbus_up (service->priv->test_dbus);
			service->priv->test_dbus_started_here = TRUE;
		}

		address = g_test_dbus_get_bus_address(test_dbus);
	}

	/* set the environment variables */
	g_setenv("DBUS_STARTER_ADDRESS", address, TRUE);
	switch (service->priv->bus_type) {
		case DBUS_TEST_SERVICE_BUS_SESSION:
			g_setenv("DBUS_SESSION_BUS_ADDRESS", address, TRUE);
			g_setenv("DBUS_STARTER_BUS_TYPE", "session", TRUE);
			break;
		case DBUS_TEST_SERVICE_BUS_SYSTEM:
			g_setenv("DBUS_SYSTEM_BUS_ADDRESS", address, TRUE);
			g_setenv("DBUS_STARTER_BUS_TYPE", "system", TRUE);
			break;
		case DBUS_TEST_SERVICE_BUS_BOTH:
			g_setenv("DBUS_SESSION_BUS_ADDRESS", address, TRUE);
			g_setenv("DBUS_SYSTEM_BUS_ADDRESS", address, TRUE);
			g_setenv("DBUS_STARTER_BUS_TYPE", "session", TRUE);
			break;
	}

	service->priv->state = STATE_BUS_STARTED;

	return;
}

void
dbus_test_service_start_tasks (DbusTestService * service)
{
	g_return_if_fail(DBUS_TEST_SERVICE(service));
	g_return_if_fail(all_tasks(service, all_tasks_bus_match, NULL));

	ensure_bus_is_up(service);
	g_return_if_fail(g_getenv("DBUS_STARTER_ADDRESS") != NULL);

	if (all_tasks(service, all_tasks_started_helper, NULL)) {
		/* If we have all started we can mark it as such as long
		   as we understand where we could hit this case */
		if (service->priv->state == STATE_INIT || service->priv->state == STATE_BUS_STARTED) {
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

	if (!all_tasks(service, all_tasks_started_helper, NULL)) {
		service->priv->state = STATE_STARTING;
		g_main_loop_run(service->priv->mainloop);

		/* This should never happen, but let's be sure */
		g_return_if_fail(all_tasks(service, all_tasks_started_helper, NULL));
	}

	service->priv->state = STATE_STARTED;

	return;
}

static gboolean
all_tasks_passed_helper (G_GNUC_UNUSED DbusTestService * service, DbusTestTask * task, G_GNUC_UNUSED gpointer user_data)
{
	return dbus_test_task_passed(task);
}

static int
get_status (DbusTestService * service)
{
	if (all_tasks(service, all_tasks_passed_helper, NULL)) {
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

	if (all_tasks(service, all_tasks_finished_helper, NULL)) {
		return get_status(service);
	}

	service->priv->state = STATE_RUNNING;
	g_main_loop_run(service->priv->mainloop);

	/* This should never happen, but let's be sure */
	g_return_val_if_fail(all_tasks(service, all_tasks_finished_helper, NULL), -1);
	service->priv->state = STATE_FINISHED;

	return get_status(service);
}

static void
task_state_changed (G_GNUC_UNUSED DbusTestTask * task, G_GNUC_UNUSED DbusTestTaskState state, gpointer user_data)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(user_data));
	DbusTestService * service = DBUS_TEST_SERVICE(user_data);

	if (service->priv->state == STATE_STARTING && all_tasks(service, all_tasks_started_helper, NULL)) {
		g_main_loop_quit(service->priv->mainloop);
		return;
	}

	if (service->priv->state == STATE_RUNNING && all_tasks(service, all_tasks_finished_helper, NULL)) {
		g_main_loop_quit(service->priv->mainloop);
		return;
	}

	return;
}

void
dbus_test_service_add_task (DbusTestService * service, DbusTestTask * task)
{
	dbus_test_service_add_task_with_priority(service, task, DBUS_TEST_SERVICE_PRIORITY_NORMAL);
}

void
dbus_test_service_add_task_with_priority (DbusTestService * service, DbusTestTask * task, DbusTestServicePriority prio)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(service));
	g_return_if_fail(DBUS_TEST_IS_TASK(task));

	/* Check to ensure that the task and the service match in thier
	   goals for busness. Fail early. */
	g_return_if_fail(all_tasks_bus_match(service, task, NULL));

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

/**
 * @service: A #DbusTestService
 * @task: Task to remove
 *
 * Removes a task from those managed by the service, it won't
 * be checked for status or managed anymore by the service.
 *
 * Return Value: Whether the task was found and removed, FALSE if not found
 */
gboolean
dbus_test_service_remove_task (DbusTestService * service, DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_SERVICE(service), FALSE);
	g_return_val_if_fail(DBUS_TEST_IS_TASK(task), FALSE);

	guint count = 0;
	count += g_queue_remove_all(&service->priv->tasks_first, task);
	count += g_queue_remove_all(&service->priv->tasks_normal, task);
	count += g_queue_remove_all(&service->priv->tasks_last, task);

	/* Checking the count here so that we can generate a warning. Guessing that
	   this actually never happens, but it's easy to check */
	if (count > 1) {
		g_warning("Task '%s' was added to the service %d times!", dbus_test_task_get_name(task), count);
	}

	/* We're going to disconnect here even if count is zero because, well, it
	   shouldn't hurt in that case and might be good for us. */
	g_signal_handlers_disconnect_by_data(task, service);

	/* If we've added it multiple times, we made multiple references, fix it. */
	guint i;
	for (i = 0; i < count; i++) {
		g_object_unref(task);
	}

	return count > 0;
}

void
dbus_test_service_stop (DbusTestService * service)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(service));
	g_main_loop_quit(service->priv->mainloop);
	return;
}

void dbus_test_service_set_bus (DbusTestService * service, DbusTestServiceBus bus)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(service));
	g_return_if_fail(service->priv->state < STATE_BUS_STARTED); /* we can't change after the bus is started */

	if (bus == DBUS_TEST_SERVICE_BUS_BOTH) {
		g_warning("Setting bus to BOTH, which is typically only used as a default value.");
	}

	service->priv->bus_type = bus;
	g_warn_if_fail(all_tasks(service, all_tasks_bus_match, NULL));
}
