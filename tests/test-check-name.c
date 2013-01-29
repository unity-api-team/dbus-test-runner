#include <glib.h>
#include <gio/gio.h>

int
main (int argc, char * argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif

	if (argc != 2) {
		g_error("ARG, need a single argument");
		return 1;
	}

	g_debug("Looking for name: %s", argv[1]);

	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_return_val_if_fail(bus != NULL, 1);

	GDBusProxy * proxy = g_dbus_proxy_new_sync(bus,
	                                           G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
	                                           NULL,
	                                           argv[1],
	                                           "/",
	                                           "org.freedesktop.DBus.Introspectable",
	                                           NULL, NULL); /* cancel, error */
	g_return_val_if_fail(proxy != NULL, 1);

	g_return_val_if_fail(g_dbus_proxy_get_name_owner(proxy) != NULL, 1);

	g_debug("Quitting");

	return 0;
}
