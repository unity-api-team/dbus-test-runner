#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

typedef enum _ServiceState ServiceState;
enum _ServiceState {
	STATE_INIT,
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
};

#define SERVICE_CHANGE_HANDLER  "dbus-test-service-change-handler"

#define DBUS_TEST_SERVICE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_SERVICE, DbusTestServicePrivate))

static void dbus_test_service_class_init (DbusTestServiceClass *klass);
static void dbus_test_service_init       (DbusTestService *self);
static void dbus_test_service_dispose    (GObject *object);
static void dbus_test_service_finalize   (GObject *object);

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

	return;
}

static void
task_unref (gpointer data, gpointer user_data)
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

	if (!g_queue_is_empty(&self->priv->tasks_first)) {
		g_queue_foreach(&self->priv->tasks_first, task_unref, NULL);
		g_queue_clear(&self->priv->tasks_first);
	}

	if (!g_queue_is_empty(&self->priv->tasks_normal)) {
		g_queue_foreach(&self->priv->tasks_normal, task_unref, NULL);
		g_queue_clear(&self->priv->tasks_normal);
	}

	if (!g_queue_is_empty(&self->priv->tasks_last)) {
		g_queue_foreach(&self->priv->tasks_last, task_unref, NULL);
		g_queue_clear(&self->priv->tasks_last);
	}

	if (self->priv->mainloop != NULL) {
		g_main_loop_unref(self->priv->mainloop);
		self->priv->mainloop = NULL;
	}

	G_OBJECT_CLASS (dbus_test_service_parent_class)->dispose (object);
	return;
}

static void
dbus_test_service_finalize (GObject *object)
{


	G_OBJECT_CLASS (dbus_test_service_parent_class)->finalize (object);
	return;
}

DbusTestService *
dbus_test_service_new (const gchar * address)
{
	DbusTestService * service = g_object_new(DBUS_TEST_TYPE_SERVICE,
	                                         NULL);

	/* TODO: Use the address */

	return service;
}

static void
all_task_started_helper (gpointer data, gpointer user_data)
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
all_tasks_started (DbusTestService * service)
{
	gboolean all_started = TRUE;

	g_queue_foreach(&service->priv->tasks_first, all_task_started_helper, &all_started);
	if (!all_started) {
		return FALSE;
	}

	g_queue_foreach(&service->priv->tasks_normal, all_task_started_helper, &all_started);
	if (!all_started) {
		return FALSE;
	}

	g_queue_foreach(&service->priv->tasks_last, all_task_started_helper, &all_started);
	if (!all_started) {
		return FALSE;
	}

	return TRUE;
}

static void
task_starter (gpointer data, gpointer user_data)
{
	DbusTestTask * task = DBUS_TEST_TASK(data);

	dbus_test_task_run(task);

	return;
}

void
dbus_test_service_start_tasks (DbusTestService * service)
{
	g_return_if_fail(DBUS_TEST_SERVICE(service));

	if (all_tasks_started(service)) {
		return;
	}

	g_queue_foreach(&service->priv->tasks_first, task_starter, NULL);
	/* TODO: Let some events through */
	g_queue_foreach(&service->priv->tasks_normal, task_starter, NULL);
	/* TODO: Let some events through */
	g_queue_foreach(&service->priv->tasks_last, task_starter, NULL);

	service->priv->state = STATE_STARTING;
	g_main_loop_run(service->priv->mainloop);

	/* This should never happen, but let's be sure */
	g_return_if_fail(all_tasks_started(service));
	service->priv->state = STATE_STARTED;

	return;
}

int
dbus_test_service_run (DbusTestService * service)
{

	return -1;
}

static void
task_state_changed (DbusTestTask * task, DbusTestTaskState state, gpointer user_data)
{

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
