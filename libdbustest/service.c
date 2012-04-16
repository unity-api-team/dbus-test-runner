#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

struct _DbusTestServicePrivate {
	GQueue tasks_first;
	GQueue tasks_normal;
	GQueue tasks_last;
};

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

	return;
}

static void
dbus_test_service_dispose (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_SERVICE(object));
	DbusTestService * self = DBUS_TEST_SERVICE(object);

	if (!g_queue_is_empty(&self->priv->tasks_first)) {
		g_queue_foreach(&self->priv->tasks_first, (GFunc)g_object_unref, NULL);
		g_queue_clear(&self->priv->tasks_first);
	}

	if (!g_queue_is_empty(&self->priv->tasks_normal)) {
		g_queue_foreach(&self->priv->tasks_normal, (GFunc)g_object_unref, NULL);
		g_queue_clear(&self->priv->tasks_normal);
	}

	if (!g_queue_is_empty(&self->priv->tasks_last)) {
		g_queue_foreach(&self->priv->tasks_last, (GFunc)g_object_unref, NULL);
		g_queue_clear(&self->priv->tasks_last);
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

void
dbus_test_service_start_tasks (DbusTestService * service)
{

	return;
}

int
dbus_test_service_run (DbusTestService * service)
{

	return -1;
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

	return;
}
