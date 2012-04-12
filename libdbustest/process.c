#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

struct _DbusTestProcessPrivate {
	int dummy;
};

#define DBUS_TEST_PROCESS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_PROCESS_TYPE, DbusTestProcessPrivate))

static void dbus_test_process_class_init (DbusTestProcessClass *klass);
static void dbus_test_process_init       (DbusTestProcess *self);
static void dbus_test_process_dispose    (GObject *object);
static void dbus_test_process_finalize   (GObject *object);

G_DEFINE_TYPE (DbusTestProcess, dbus_test_process, DBUS_TEST_TYPE_TASK);

static void
dbus_test_process_class_init (DbusTestProcessClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestProcessPrivate));

	object_class->dispose = dbus_test_process_dispose;
	object_class->finalize = dbus_test_process_finalize;

	return;
}

static void
dbus_test_process_init (DbusTestProcess *self)
{

	return;
}

static void
dbus_test_process_dispose (GObject *object)
{


	G_OBJECT_CLASS (dbus_test_process_parent_class)->dispose (object);
	return;
}

static void
dbus_test_process_finalize (GObject *object)
{


	G_OBJECT_CLASS (dbus_test_process_parent_class)->finalize (object);
	return;
}
