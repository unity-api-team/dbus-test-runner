#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

struct _DbusTestBustlePrivate {
	gchar * filename;
	gchar * executable;
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

	self->priv->filename = g_strconcat(g_get_current_dir(), G_DIR_SEPARATOR_S, "bustle.log", NULL);
	self->priv->executable = g_strdup("bustle-dbus-monitor");

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
