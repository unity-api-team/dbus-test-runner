#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

struct _DbusTestBustlePrivate {
	int dummy;
};

#define DBUS_TEST_BUSTLE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_BUSTLE, DbusTestBustlePrivate))

static void dbus_test_bustle_class_init (DbusTestBustleClass *klass);
static void dbus_test_bustle_init       (DbusTestBustle *self);
static void dbus_test_bustle_dispose    (GObject *object);
static void dbus_test_bustle_finalize   (GObject *object);

G_DEFINE_TYPE (DbusTestBustle, dbus_test_bustle, DBUS_TEST_TYPE_TASK);

static void
dbus_test_bustle_class_init (DbusTestBustleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestBustlePrivate));

	object_class->dispose = dbus_test_bustle_dispose;
	object_class->finalize = dbus_test_bustle_finalize;

	return;
}

static void
dbus_test_bustle_init (DbusTestBustle *self)
{
	self->priv = DBUS_TEST_BUSTLE_GET_PRIVATE(self);

	return;
}

static void
dbus_test_bustle_dispose (GObject *object)
{

	G_OBJECT_CLASS (dbus_test_bustle_parent_class)->dispose (object);
	return;
}

static void
dbus_test_bustle_finalize (GObject *object)
{

	G_OBJECT_CLASS (dbus_test_bustle_parent_class)->finalize (object);
	return;
}

DbusTestBustle *
dbus_test_bustle_new (const gchar * filename)
{

	return NULL;
}

void
dbus_test_bustle_set_executable (DbusTestBustle * bustle, const gchar * executable)
{

	return;
}
