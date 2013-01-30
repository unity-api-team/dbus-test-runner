
#include <glib.h>
#include <gio/gio.h>

GMainLoop * mainloop = NULL;
gboolean error = FALSE;

void
name_acquired (GDBusConnection * connection, const gchar * name, gpointer user_data)
{
	g_main_loop_quit(mainloop);
	return;
}

void
name_lost (GDBusConnection * connection, const gchar * name, gpointer user_data)
{
	g_warning("Name lost!");
	error = TRUE;
	g_main_loop_quit(mainloop);
	return;
}

int
main (int argc, gchar * argv[])
{
	if (argc != 2) {
		g_critical("Need a name");
		return -1;
	}

#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif

	guint name = g_bus_own_name(G_BUS_TYPE_SESSION,
	                            argv[1], /* name */
	                            G_BUS_NAME_OWNER_FLAGS_NONE, /* flags */
	                            NULL, /* bus acquired */
	                            name_acquired, /* name acquired */
	                            name_lost, /* name lost */
	                            NULL, /* user data */
	                            NULL /* ud free */
	                            );

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_bus_unown_name(name);

	if (error) {
		return -1;
	} else {
		return 0;
	}
}
