/*
Copyright 2013 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

