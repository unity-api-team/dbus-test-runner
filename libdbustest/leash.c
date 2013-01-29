#include <glib.h>

GMainLoop * mainloop = NULL;
gulong timer = 0;
pid_t victim = 0;

static void sigterm_graceful_exit (int signal);

static gboolean
destroy_everyone (gpointer user_data)
{
	if (victim != 0) {
		kill(victim, SIGTERM);
	}

	sigterm_graceful_exit(0);
	return;
}

static void
restart_handler (void)
{
	if (timer != 0) {
		g_source_remove(timer);
	}

	timer = g_timeout_add_seconds(30, destroy_everyone, NULL);

	return;
}

static void
sighup_dont_die (int signal)
{
	restart_handler();
	return;
}

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

	signal(SIGTERM, sigterm_graceful_exit);
	signal(SIGHUP, sighup_dont_die);

	victim = atoi(argv[1]);

	restart_handler();

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_main_loop_unref(mainloop);

	return 0;
}

