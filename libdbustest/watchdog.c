#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "watchdog.h"

struct _DbusTestWatchdogPrivate {
	int dummy;
};

#define DBUS_TEST_WATCHDOG_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_WATCHDOG_TYPE, DbusTestWatchdogPrivate))

static void dbus_test_watchdog_class_init (DbusTestWatchdogClass *klass);
static void dbus_test_watchdog_init       (DbusTestWatchdog *self);
static void dbus_test_watchdog_dispose    (GObject *object);
static void dbus_test_watchdog_finalize   (GObject *object);

G_DEFINE_TYPE (DbusTestWatchdog, dbus_test_watchdog, G_TYPE_OBJECT);

/* Initialize class */
static void
dbus_test_watchdog_class_init (DbusTestWatchdogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestWatchdogPrivate));

	object_class->dispose = dbus_test_watchdog_dispose;
	object_class->finalize = dbus_test_watchdog_finalize;

	return;
}

/* Initialize instance data */
static void
dbus_test_watchdog_init (G_GNUC_UNUSED DbusTestWatchdog *self)
{

	return;
}

/* Drop refs */
static void
dbus_test_watchdog_dispose (GObject *object)
{

	G_OBJECT_CLASS (dbus_test_watchdog_parent_class)->dispose (object);
	return;
}

/* clean up memory */
static void
dbus_test_watchdog_finalize (GObject *object)
{

	G_OBJECT_CLASS (dbus_test_watchdog_parent_class)->finalize (object);
	return;
}
