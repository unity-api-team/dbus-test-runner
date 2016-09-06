#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "watchdog.h"

struct _DbusTestWatchdogPrivate {
	GPid watchdog;
};

#define DBUS_TEST_WATCHDOG_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_WATCHDOG, DbusTestWatchdogPrivate))

static void dbus_test_watchdog_class_init (DbusTestWatchdogClass *klass);
static void dbus_test_watchdog_init       (DbusTestWatchdog *self);
static void dbus_test_watchdog_finalize   (GObject *object);

G_DEFINE_TYPE (DbusTestWatchdog, dbus_test_watchdog, G_TYPE_OBJECT);

/* Initialize class */
static void
dbus_test_watchdog_class_init (DbusTestWatchdogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestWatchdogPrivate));

	object_class->finalize = dbus_test_watchdog_finalize;

	return;
}

/* Initialize instance data */
static void
dbus_test_watchdog_init (G_GNUC_UNUSED DbusTestWatchdog *self)
{
	self->priv = DBUS_TEST_WATCHDOG_GET_PRIVATE(self);

	self->priv->watchdog = 0;

	return;
}

/* clean up memory */
static void
dbus_test_watchdog_finalize (GObject *object)
{
	DbusTestWatchdog * watchdog = DBUS_TEST_WATCHDOG(object);
	
	if (watchdog->priv->watchdog != 0) {
		kill(watchdog->priv->watchdog, SIGTERM);
	}

	G_OBJECT_CLASS (dbus_test_watchdog_parent_class)->finalize (object);
	return;
}

/**
 * dbus_test_watchdog_add_pid:
 * @watchdog: Instance of #DbusTestWatchdog
 * @pid: PID to kill
 *
 * Adds a PID for the watchdog to watch.
 */
void
dbus_test_watchdog_add_pid (DbusTestWatchdog * watchdog, GPid pid)
{
	g_return_if_fail(DBUS_TEST_IS_WATCHDOG(watchdog));
	g_return_if_fail(pid != 0);
	g_return_if_fail(watchdog->priv->watchdog == 0);

	/* Setting up argument vector */
	gchar * strpid = g_strdup_printf("%d", pid);
	gchar * argv[3];
	argv[0] = WATCHDOG;
	argv[1] = strpid;
	argv[2] = NULL;
	
	GError * error = NULL;

	/* Spawn the watchdog, we now have 60 secs */
	g_spawn_async (NULL, /* cwd */
	               argv,
	               NULL, /* env */
	               0, /* flags */
	               NULL, NULL, /* Setup function */
	               &watchdog->priv->watchdog,
	               &error);

	g_free(strpid);

	if (error != NULL) {
		g_warning("Unable to start watchdog");
		watchdog->priv->watchdog = 0;
		g_error_free(error);
		return;
	}

	return;
}

/**
 * dbus_test_watchdog_add_pid:
 * @watchdog: Instance of #DbusTestWatchdog
 *
 * Tell the watchdog not to kill.  For now.
 */
void
dbus_test_watchdog_ping (DbusTestWatchdog * watchdog)
{
	g_return_if_fail(DBUS_TEST_IS_WATCHDOG(watchdog));

	if (watchdog->priv->watchdog != 0) {
		kill(watchdog->priv->watchdog, SIGHUP);
	}

	return;
}
