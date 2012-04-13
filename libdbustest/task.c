#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

struct _DbusTestTaskPrivate {
	int dummy;
};

#define DBUS_TEST_TASK_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TASK_TYPE, DbusTestTaskPrivate))

static void dbus_test_task_class_init (DbusTestTaskClass *klass);
static void dbus_test_task_init       (DbusTestTask *self);
static void dbus_test_task_dispose    (GObject *object);
static void dbus_test_task_finalize   (GObject *object);

G_DEFINE_TYPE (DbusTestTask, dbus_test_task, G_TYPE_OBJECT);

static void
dbus_test_task_class_init (DbusTestTaskClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestTaskPrivate));

	object_class->dispose = dbus_test_task_dispose;
	object_class->finalize = dbus_test_task_finalize;

	klass->run = NULL;
	klass->get_state = NULL;
	klass->get_passed = NULL;

	return;
}

static void
dbus_test_task_init (DbusTestTask *self)
{

	return;
}

static void
dbus_test_task_dispose (GObject *object)
{

	G_OBJECT_CLASS (dbus_test_task_parent_class)->dispose (object);
	return;
}

static void
dbus_test_task_finalize (GObject *object)
{

	G_OBJECT_CLASS (dbus_test_task_parent_class)->finalize (object);
	return;
}

DbusTestTask *
dbus_test_task_new (void)
{

	return NULL;
}

void
dbus_test_task_set_name (DbusTestTask * task, const gchar * name)
{

	return;
}

void
dbus_test_task_set_name_spacing (DbusTestTask * task, guint chars)
{

	return;
}

void
dbus_test_task_set_wait_for (DbusTestTask * task, const gchar * dbus_name)
{

	return;
}

void
dbus_test_task_set_return (DbusTestTask * task, DbusTestTaskReturn ret)
{

	return;
}

void
dbus_test_task_print (DbusTestTask * task, const gchar * message)
{

	return;
}

DbusTestTaskState
dbus_test_task_get_state (DbusTestTask * task)
{

	return DBUS_TEST_TASK_STATE_FINISHED;
}

void
dbus_test_task_run (DbusTestTask * task)
{

	return;
}

gboolean
dbus_test_task_passed (DbusTestTask * task)
{

	return FALSE;
}
