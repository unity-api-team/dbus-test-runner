/*
Copyright 2010 Canonical Ltd.

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
#include <gio/gio.h>

static GList * tasks = NULL;
static gboolean global_success = TRUE;
static GMainLoop * global_mainloop;
static gint max_wait = 30;

typedef enum {
	TASK_RETURN_NORMAL = 0,
	TASK_RETURN_IGNORE,
	TASK_RETURN_INVERT
} task_return_t;

typedef struct {
	gchar * executable;
	gchar * name;
	task_return_t returntype;
	GPid pid;
	guint watcher;
	guint number;
	GList * parameters;
	gboolean task_die;
	gboolean text_die;
	guint idle;
	guint io_watch;
	gchar * wait_for;
} task_t;

static void check_task_cleanup (task_t * task, gboolean force);
static void start_task (gpointer data, gpointer userdata);

static gchar * bustle_datafile = NULL;
static GIOChannel * bustle_stdout = NULL;
static GIOChannel * bustle_stderr = NULL;
static GIOChannel * bustle_file = NULL;
static GPid bustle_pid = 0;
static gboolean any_waitfors = FALSE;

#define BUSTLE_ERROR_DEFAULT  "Bustle"
static gchar * bustle_error = BUSTLE_ERROR_DEFAULT;

static gboolean
bustle_write_error (GIOChannel * channel, GIOCondition condition, gpointer data)
{
	gchar * line;
	gsize termloc;

	do {
		GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);

		if (status == G_IO_STATUS_EOF) {
			return FALSE;
		}

		if (status != G_IO_STATUS_NORMAL) {
			continue;
		}

		line[termloc] = '\0';

		g_print("%s: %s\n", bustle_error, line);
		g_free(line);
	} while (G_IO_IN & g_io_channel_get_buffer_condition(channel));

	return TRUE;
}

static gboolean
bustle_writes (GIOChannel * channel, GIOCondition condition, gpointer data)
{
	gchar * line;
	gsize termloc;

	GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);

	if (status != G_IO_STATUS_NORMAL) {
		return FALSE;
	}

	g_io_channel_write_chars((GIOChannel *)data,
							 line,
							 termloc,
							 NULL,
							 NULL);
	g_io_channel_write_chars((GIOChannel *)data,
							 "\n",
							 1,
							 NULL,
							 NULL);

	g_free(line);

	return TRUE;
}

static void
start_bustling (void)
{
	if (bustle_datafile == NULL) {
		return;
	}

	GError * error = NULL;

	bustle_file = g_io_channel_new_file(bustle_datafile, "w", &error);

	if (error != NULL) {
		g_warning("Unable to open bustle file '%s': %s", bustle_datafile, error->message);
		g_error_free(error);
		g_free(bustle_datafile);
		bustle_datafile = NULL;
		global_success = FALSE;
		g_main_loop_quit(global_mainloop);
		return;
	}

	gint bustle_stdout_num;
	gint bustle_stderr_num;
	
	gchar ** bustle_monitor = g_new0(gchar *, 3);
	bustle_monitor[0] = "bustle-dbus-monitor";
	bustle_monitor[1] = "--session";

	g_spawn_async_with_pipes(g_get_current_dir(),
	                         bustle_monitor, /* argv */
	                         NULL, /* envp */
	                         /* G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL, */ /* flags */
	                         G_SPAWN_SEARCH_PATH, /* flags */
	                         NULL, /* child setup func */
	                         NULL, /* child setup data */
	                         &bustle_pid, /* PID */
	                         NULL, /* stdin */
	                         &bustle_stdout_num, /* stdout */
	                         &bustle_stderr_num, /* stderr */
	                         &error); /* error */

	if (error != NULL) {
		g_warning("Unable to start bustling data: %s", error->message);
		return;
	}

	g_debug("Starting bustle monitor.  PID: %d", bustle_pid);

	bustle_stdout = g_io_channel_unix_new(bustle_stdout_num);
	g_io_add_watch(bustle_stdout,
	               G_IO_IN | G_IO_PRI, /* conditions */
	               bustle_writes, /* func */
	               bustle_file); /* func data */

	bustle_stderr = g_io_channel_unix_new(bustle_stderr_num);
	g_io_add_watch(bustle_stderr,
	               G_IO_IN, /* conditions */
	               bustle_write_error, /* func */
	               NULL); /* func data */

	return;
}

static void
stop_bustling (void)
{
	if (bustle_datafile == NULL) {
		return;
	}

	gchar * killline = g_strdup_printf("kill -INT %d", bustle_pid);
	g_spawn_command_line_sync(killline, NULL, NULL, NULL, NULL);
	g_free(killline);

	gchar * send_stdout = NULL;
	gchar * send_stderr = NULL;
	g_spawn_command_line_sync("dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.ListNames", &send_stdout, &send_stderr, NULL, NULL);
	if (send_stdout != NULL) {
		g_free(send_stdout);
	}
	if (send_stderr != NULL) {
		g_free(send_stderr);
	}

	gchar * line;
	gsize termloc;

	while (!((G_IO_ERR | G_IO_HUP | G_IO_NVAL) & g_io_channel_get_buffer_condition(bustle_stdout))) {
		GIOStatus status = g_io_channel_read_line (bustle_stdout, &line, NULL, &termloc, NULL);

		if (status == G_IO_STATUS_EOF) {
			break;
		}

		if (status != G_IO_STATUS_NORMAL) {
			continue;
		}

		g_io_channel_write_chars((GIOChannel *)bustle_file,
		                         line,
		                         termloc,
		                         NULL,
		                         NULL);
		g_io_channel_write_chars((GIOChannel *)bustle_file,
		                         "\n",
		                         1,
		                         NULL,
		                         NULL);

		g_free(line);
	}

	g_io_channel_shutdown(bustle_stdout, TRUE, NULL);
	g_io_channel_shutdown(bustle_stderr, TRUE, NULL);
	g_io_channel_shutdown(bustle_file, TRUE, NULL);

	return;
}

static gboolean
force_kill_in_a_bit (gpointer data)
{
	g_debug("Forcing cleanup of: %s", ((task_t *)data)->name);
	check_task_cleanup((task_t *)data, TRUE);
	return FALSE;
}

static void
check_task_cleanup (task_t * task, gboolean force)
{
	if (!force) {
		if (task->task_die) {
			if (!task->text_die) {
				task->idle = g_idle_add(force_kill_in_a_bit, task);
			}
		} else  {
			return;
		}
	}

	if (task->idle) {
		g_source_remove(task->idle);
	}
	if (task->io_watch) {
		g_source_remove(task->io_watch);
	}
	if (task->watcher) {
		g_source_remove(task->watcher);
	}

	tasks = g_list_remove(tasks, task);
	g_free(task->wait_for);
	g_free(task->executable);
	g_free(task->name);
	g_free(task);

	if (g_list_length(tasks) == 0) {
		g_main_loop_quit(global_mainloop);
	}

	return;
}

static void
dbus_watcher (GPid pid, gint status, gpointer data)
{
	g_error("DBus Daemon exited abruptly!");

	global_success = FALSE;
	g_main_loop_quit(global_mainloop);

	if (pid != 0) {
		g_spawn_close_pid(pid);
	}

	return;
}

static void
proc_watcher (GPid pid, gint status, gpointer data)
{
	task_t * task = (task_t *)data;

	if (task->returntype == TASK_RETURN_INVERT) {
		status = !status;
	}

	if (status && task->returntype != TASK_RETURN_IGNORE) {
		global_success = FALSE;
	}

	if (pid != 0) {
		g_spawn_close_pid(pid);
	}

	task->task_die = TRUE;

	check_task_cleanup(task, FALSE);

	return;
}

gboolean
proc_writes (GIOChannel * channel, GIOCondition condition, gpointer data)
{
	task_t * task = (task_t *)data;
	gchar * line;
	gsize termloc;

	do {
		GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);

		if (status == G_IO_STATUS_EOF) {
			task->text_die = TRUE;
			check_task_cleanup(task, FALSE);
			continue;
		}

		if (status != G_IO_STATUS_NORMAL) {
			continue;
		}

		line[termloc] = '\0';

		g_print("%s: %s\n", task->name, line);
		g_free(line);
	} while (G_IO_IN & g_io_channel_get_buffer_condition(channel));

	return TRUE;
}

static void
wait_for_found (GDBusConnection * connection, const gchar * name, const gchar * name_owner, gpointer user_data)
{
	return start_task(user_data, GINT_TO_POINTER(TRUE));
}

static void
start_task (gpointer data, gpointer userdata)
{
	gboolean ignore_waitfor = GPOINTER_TO_INT(userdata);
	GError * error = NULL;
	task_t * task = (task_t *)data;

	if (!ignore_waitfor && task->wait_for != NULL) {
		g_bus_watch_name(G_BUS_TYPE_SESSION,
		                 task->wait_for,
		                 G_BUS_NAME_WATCHER_FLAGS_NONE,
		                 wait_for_found,
		                 NULL,
		                 task,
		                 NULL);
		return;
	}

	gchar ** argv;
	argv = g_new0(gchar *, g_list_length(task->parameters) + 2);

	argv[0] = task->executable;
	int i;
	for (i = 0; i < g_list_length(task->parameters); i++) {
		argv[i + 1] = (gchar *)g_list_nth(task->parameters, i)->data;
	}

	gint proc_stdout;
	g_spawn_async_with_pipes(g_get_current_dir(),
	                         argv, /* argv */
	                         NULL, /* envp */
	                         G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, /* flags */
	                         NULL, /* child setup func */
	                         NULL, /* child setup data */
							 &(task->pid), /* PID */
	                         NULL, /* stdin */
	                         &proc_stdout, /* stdout */
	                         NULL, /* stderr */
	                         &error); /* error */
	g_free(argv);

	if (error != NULL) {
		g_warning("Unable to start task '%s': %s", task->name, error->message);
		proc_watcher(0, 1, task);
		return;
	}

	g_debug("Started task '%s' PID: %d", task->name, task->pid);

	GIOChannel * iochan = g_io_channel_unix_new(proc_stdout);
	g_io_channel_set_buffer_size(iochan, 10 * 1024 * 1024); /* 10 MB should be enough for anyone */
	task->io_watch = g_io_add_watch(iochan,
	                                G_IO_IN, /* conditions */
	                                proc_writes, /* func */
	                                task); /* func data */

	task->watcher = g_child_watch_add(task->pid, proc_watcher, task);

	return;
}

gboolean
dbus_writes (GIOChannel * channel, GIOCondition condition, gpointer data)
{
	if (condition & G_IO_ERR) {
		g_error("DBus writing failure!");
		return FALSE;
	}

	gchar * line;
	gsize termloc;
	GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);
	g_return_val_if_fail(status == G_IO_STATUS_NORMAL, FALSE);
	line[termloc] = '\0';

	static gboolean first_time = TRUE;
	g_print("DBus daemon: %s\n", line);

	if (first_time) {
		first_time = FALSE;

		g_setenv("DBUS_SESSION_BUS_ADDRESS", line, TRUE);
		g_setenv("DBUS_STARTER_ADDRESS", line, TRUE);
		g_setenv("DBUS_STARTER_BUS_TYPE", "session", TRUE);

		if (tasks != NULL) {
			start_bustling();

			g_list_foreach(tasks, start_task, GINT_TO_POINTER(FALSE));
		} else {
			g_print("No tasks!\n");
			global_success = FALSE;
			g_main_loop_quit(global_mainloop);
		}
	}

	g_free(line);

	return TRUE;
}

static gboolean
option_task (const gchar * arg, const gchar * value, gpointer data, GError ** error)
{
	task_t * task = g_new0(task_t, 1);
	task->executable = g_strdup(value);
	task->number = g_list_length(tasks);
	task->parameters = NULL;
	task->task_die = FALSE;
	task->text_die = FALSE;
	tasks = g_list_prepend(tasks, task);
	task->wait_for = NULL;
	return TRUE;
}

static gboolean
option_taskname (const gchar * arg, const gchar * value, gpointer data, GError ** error)
{
	if (tasks == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put the name %s on.", value);
		return FALSE;
	}

	task_t * task = (task_t *)tasks->data;
	if (task->name != NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task already has the name %s.  Asked to put %s on it.", task->name, value);
		return FALSE;
	}

	task->name = g_strdup(value);
	return TRUE;
}

static gboolean
option_noreturn (const gchar * arg, const gchar * value, gpointer data, GError ** error)
{
	if (tasks == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put adjust return on.");
		return FALSE;
	}

	task_t * task = (task_t *)tasks->data;
	if (task->returntype != TASK_RETURN_NORMAL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task return type has already been modified.");
		return FALSE;
	}

	task->returntype = TASK_RETURN_IGNORE;
	return TRUE;
}

static gboolean
option_invert (const gchar * arg, const gchar * value, gpointer data, GError ** error)
{
	if (tasks == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put adjust return on.");
		return FALSE;
	}

	task_t * task = (task_t *)tasks->data;
	if (task->returntype != TASK_RETURN_NORMAL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task return type has already been modified.");
		return FALSE;
	}

	task->returntype = TASK_RETURN_INVERT;
	return TRUE;
}

static gboolean
option_param (const gchar * arg, const gchar * value, gpointer data, GError ** error)
{
	if (tasks == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put adjust return on.");
		return FALSE;
	}

	task_t * task = (task_t *)tasks->data;
	task->parameters = g_list_append(task->parameters, g_strdup(value));

	return TRUE;
}

static gboolean
option_wait (const gchar * arg, const gchar * value, gpointer data, GError ** error)
{
	if (tasks == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to add a wait on %s for.", value);
		return FALSE;
	}

	task_t * task = (task_t *)tasks->data;
	if (task->wait_for != NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task is already waiting for %s.  Asked to wait for %s", task->wait_for, value);
		return FALSE;
	}

	task->wait_for = g_strdup(value);
	any_waitfors = TRUE;
	return TRUE;
}

static void
length_finder (gpointer data, gpointer udata)
{
	task_t * task = (task_t *)data;
	guint * longest = (guint *)udata;

	guint length = g_utf8_strlen(task->name, -1);
	if (length > *longest) {
		*longest = length;
	}

	return;
}

static void
set_name (gpointer data, gpointer udata)
{
	task_t * task = (task_t *)data;

	if (task->name == NULL) {
		task->name = g_strdup_printf("task-%d", task->number);
	}

	return;
}

static void
normalize_name (gpointer data, gpointer udata)
{
	task_t * task = (task_t *)data;
	guint * target = (guint *)udata;

	guint length = g_utf8_strlen(task->name, -1);
	if (length != *target) {
		gchar * fillstr = g_strnfill(*target - length, ' ');
		gchar * newname = g_strconcat(task->name, fillstr, NULL);
		g_free(task->name);
		g_free(fillstr);
		task->name = newname;
	}

	return;
}

static void
normalize_bustle (guint target)
{
	if (bustle_datafile == NULL) {
		return;
	}

	guint length = g_utf8_strlen(BUSTLE_ERROR_DEFAULT, -1);
	if (length != target) {
		gchar * fillstr = g_strnfill(target - length, ' ');
		bustle_error = g_strconcat(BUSTLE_ERROR_DEFAULT, fillstr, NULL);
		g_free(fillstr);
	}

	return;
}

static void
normalize_name_length (void)
{
	guint maxlen = 0;
	
	if (bustle_datafile != NULL) {
		maxlen = g_utf8_strlen(BUSTLE_ERROR_DEFAULT, -1);
	}

	g_list_foreach(tasks, set_name, NULL);
	g_list_foreach(tasks, length_finder, &maxlen);
	g_list_foreach(tasks, normalize_name, &maxlen);

	normalize_bustle(maxlen);

	return;
}

static gboolean
max_wait_hit (gpointer user_data)
{
	g_warning("Timing out at maximum wait of %d seconds.", max_wait);
	g_main_loop_quit(global_mainloop);	
	global_success = FALSE;
	return FALSE;
}

static gchar * dbus_configfile = NULL;
static gchar * dbus_daemon = NULL;

static GOptionEntry general_options[] = {
	{"dbus-daemon",  0,     0,                       G_OPTION_ARG_FILENAME,  &dbus_daemon,     "Path to the DBus deamon to use.  Defaults to 'dbus-daemon'.", "executable"},
	{"dbus-config",  'd',   0,                       G_OPTION_ARG_FILENAME,  &dbus_configfile, "Configuration file for newly created DBus server.  Defaults to '" DEFAULT_SESSION_CONF "'.", "config_file"},
	{"bustle-data",  'b',   0,                       G_OPTION_ARG_FILENAME,  &bustle_datafile, "A file to write out data from the bustle logger to.", "data_file"},
	{"max-wait",     'm',   0,                       G_OPTION_ARG_INT,       &max_wait,        "The maximum amount of time the test runner will wait for the test to complete.  Default is 30 seconds.", "seconds"},
	{NULL}
};

static GOptionEntry task_options[] = {
	{"task",          't',  G_OPTION_FLAG_FILENAME,   G_OPTION_ARG_CALLBACK,  option_task,     "Defines a new task to run under our private DBus session.", "executable"},
	{"task-name",     'n',  0,                        G_OPTION_ARG_CALLBACK,  option_taskname, "A string to label output from the previously defined task.  Defaults to taskN.", "name"},
	{"ignore-return", 'r',  G_OPTION_FLAG_NO_ARG,     G_OPTION_ARG_CALLBACK,  option_noreturn, "Do not use the return value of the task to calculate whether the test passes or fails.", NULL},
	{"invert-return", 'i',  G_OPTION_FLAG_NO_ARG,     G_OPTION_ARG_CALLBACK,  option_invert,   "Invert the return value of the task before calculating whether the test passes or fails.", NULL},
	{"parameter",     'p',  0,                        G_OPTION_ARG_CALLBACK,  option_param,    "Add a parameter to the call of this utility.  May be called as many times as you'd like.", NULL},
	{"wait-for",      'f',  0,                        G_OPTION_ARG_CALLBACK,  option_wait,     "A dbus-name that should appear on the bus before this task is started", "dbus-name"},
	{NULL}
};

int
main (int argc, char * argv[])
{
	GError * error = NULL;
	GOptionContext * context;

	g_type_init();

	context = g_option_context_new("- run multiple tasks under an independent DBus session bus");

	g_option_context_add_main_entries(context, general_options, "dbus-runner");

	GOptionGroup * taskgroup = g_option_group_new("task-control", "Task control options", "Options that are used to control how the task is handled by the test runner.", NULL, NULL);
	g_option_group_add_entries(taskgroup, task_options);
	g_option_context_add_group(context, taskgroup);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		g_error_free(error);
		return 1;
	}

	normalize_name_length();

	if (dbus_configfile == NULL) {
		dbus_configfile = DEFAULT_SESSION_CONF;
	}

	if (dbus_daemon == NULL) {
		dbus_daemon = "dbus-daemon";
	}

	gint dbus_stdout;
	GPid dbus;
	gchar * blank[1] = {NULL};
	gchar * dbus_startup[] = {dbus_daemon, "--config-file", dbus_configfile, "--print-address", NULL};
	g_spawn_async_with_pipes(g_get_current_dir(),
	                         dbus_startup, /* argv */
	                         blank, /* envp */
	                         G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, /* flags */
	                         NULL, /* child setup func */
	                         NULL, /* child setup data */
							 &dbus, /* PID */
	                         NULL, /* stdin */
	                         &dbus_stdout, /* stdout */
	                         NULL, /* stderr */
	                         &error); /* error */

	if (error != NULL) {
		g_error("Unable to start dbus daemon: %s", error->message);
		return 1;
	}

	guint dbus_watch = g_child_watch_add(dbus, dbus_watcher, NULL);

	GIOChannel * dbus_io = g_io_channel_unix_new(dbus_stdout);
	g_io_add_watch(dbus_io,
	               G_IO_IN | G_IO_ERR, /* conditions */
	               dbus_writes, /* func */
	               NULL); /* func data */

	if (max_wait > 0) {
		g_timeout_add_seconds(max_wait, max_wait_hit, NULL);
	}

	global_mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(global_mainloop);

	stop_bustling();

	g_source_remove(dbus_watch); /* Let's not error when we want to kill it */
	gchar * killline = g_strdup_printf("kill -9 %d", dbus);
	g_spawn_command_line_sync(killline, NULL, NULL, NULL, NULL);
	g_free(killline);

	return !global_success;
}
