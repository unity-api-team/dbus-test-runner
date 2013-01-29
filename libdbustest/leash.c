#include <glib.h>

GMainLoop * mainloop = NULL;

static void
sigterm_graceful_exit (int signal)
{
	g_main_loop_quit(mainloop);
	return;
}

int
main (int argc, char * argv[])
{
	if (argc != 2) {
		g_critical("Need a PID to kill");
		return -1;
	}

	g_type_init();


	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_main_loop_unref(mainloop);

	return 0;
}

