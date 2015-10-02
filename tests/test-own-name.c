#include <glib.h>
#include <gio/gio.h>

void
name_aquired (GDBusConnection * connection, const gchar * name, gpointer user_data)
{
	g_debug("Got name: %s", name);
	return;
}

gboolean
end_of_line (gpointer user_data)
{
	g_main_loop_quit((GMainLoop *)user_data);
	return FALSE;
}

int
main (int argc, char * argv[])
{
	gboolean system_bus = FALSE;
	GOptionContext *options;
	GError *error = NULL;

	const GOptionEntry option_entries[] = {
		{ "system", 'y', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &system_bus, "Own the name on the system bus", NULL },
		{ NULL }
	};

#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif

	options = g_option_context_new(NULL);
	g_option_context_add_main_entries(options, option_entries, NULL);
	if (!g_option_context_parse(options, &argc, &argv, &error)) {
		g_printerr("%s", error->message);
		g_error_free(error);
		return 1;
	}

	if (argc != 2) {
		g_error("ARG, need a single argument");
		return 1;
	}

	g_debug("Trying for name: %s", argv[1]);

	g_bus_own_name(system_bus ? G_BUS_TYPE_SYSTEM : G_BUS_TYPE_SESSION,
	               argv[1],
	               G_BUS_NAME_OWNER_FLAGS_NONE,
	               NULL, /* bus aquired */
	               name_aquired,
	               NULL, /* lost */
	               NULL, /* data */
	               NULL); /* destroy */

	GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);

	g_timeout_add_seconds(2, end_of_line, mainloop);

	g_main_loop_run(mainloop);

	g_main_loop_unref(mainloop);
	g_option_context_free(options);

	g_debug("Quitting");

	return 0;
}
