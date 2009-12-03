
#include <glib.h>

static GList * tasks = NULL;
static gboolean global_success = TRUE;
static GMainLoop * global_mainloop;

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
} task_t;

static void check_task_cleanup (task_t * task, gboolean force);

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

	tasks = g_list_remove(tasks, task);
	g_free(task->executable);
	g_free(task->name);

	if (g_list_length(tasks) == 0) {
		g_main_loop_quit(global_mainloop);
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

	g_spawn_close_pid(pid);

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
	GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);

	if (status != G_IO_STATUS_NORMAL) {
		task->text_die = TRUE;
		check_task_cleanup(task, FALSE);
		return;
	}

	line[termloc] = '\0';

	g_print("%s: %s\n", task->name, line);
	g_free(line);

	return TRUE;
}

void
start_task (gpointer data, gpointer userdata)
{
	task_t * task = (task_t *)data;

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
	                         NULL); /* error */
	g_free(argv);

	GIOChannel * iochan = g_io_channel_unix_new(proc_stdout);
	g_io_add_watch(iochan,
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

	g_print("DBus address: %s\n", line);
	g_setenv("DBUS_SESSION_BUS_ADDRESS", line, TRUE);
	g_setenv("DBUS_STARTER_ADDRESS", line, TRUE);
	g_setenv("DBUS_STARTER_BUS_TYPE", "session", TRUE);
	g_free(line);

	g_list_foreach(tasks, start_task, NULL);

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

static void
length_finder (gpointer data, gpointer udata)
{
	task_t * task = (task_t *)data;
	guint * longest = (guint *)udata;

	/* 640 should be enough characters for anyone */
	guint length = g_utf8_strlen(task->name, 640);
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

	/* 640 should be enough characters for anyone */
	guint length = g_utf8_strlen(task->name, 640);
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
normalize_name_length (void)
{
	guint maxlen = 0;

	g_list_foreach(tasks, set_name, NULL);
	g_list_foreach(tasks, length_finder, &maxlen);
	g_list_foreach(tasks, normalize_name, &maxlen);

	return;
}

static gchar * dbus_configfile = NULL;

static GOptionEntry general_options[] = {
	{"dbus-config",  'd',   G_OPTION_FLAG_FILENAME,  G_OPTION_ARG_FILENAME,  &dbus_configfile, "Configuration file for newly created DBus server.  Defaults to '" DEFAULT_SESSION_CONF "'.", "config_file"},
	{NULL}
};

static GOptionEntry task_options[] = {
	{"task",          't',  G_OPTION_FLAG_FILENAME,   G_OPTION_ARG_CALLBACK,  option_task,     "Defines a new task to run under our private DBus session.", "executable"},
	{"task-name",     'n',  0,                        G_OPTION_ARG_CALLBACK,  option_taskname, "A string to label output from the previously defined task.  Defaults to taskN.", "name"},
	{"ignore-return", 'r',  G_OPTION_FLAG_NO_ARG,     G_OPTION_ARG_CALLBACK,  option_noreturn, "Do not use the return value of the task to calculate whether the test passes or fails.", NULL},
	{"invert-return", 'i',  G_OPTION_FLAG_NO_ARG,     G_OPTION_ARG_CALLBACK,  option_invert,   "Invert the return value of the task before calculating whether the test passes or fails.", NULL},
	{"parameter",     'p',  0,                        G_OPTION_ARG_CALLBACK,  option_param,    "Add a parameter to the call of this utility.  May be called as many times as you'd like.", NULL},
	{NULL}
};

int
main (int argc, char * argv[])
{
	GError * error = NULL;
	GOptionContext * context;

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

	gint dbus_stdout;
	GPid dbus;
	gchar * blank[1] = {NULL};
	gchar * dbus_startup[] = {"dbus-daemon", "--config-file", dbus_configfile, "--print-address", NULL};
	g_spawn_async_with_pipes(g_get_current_dir(),
	                         dbus_startup, /* argv */
	                         blank, /* envp */
	                         G_SPAWN_SEARCH_PATH, /* flags */
	                         NULL, /* child setup func */
	                         NULL, /* child setup data */
							 &dbus, /* PID */
	                         NULL, /* stdin */
	                         &dbus_stdout, /* stdout */
	                         NULL, /* stderr */
	                         NULL); /* error */

	GIOChannel * dbus_io = g_io_channel_unix_new(dbus_stdout);
	g_io_add_watch(dbus_io,
	               G_IO_IN | G_IO_ERR, /* conditions */
	               dbus_writes, /* func */
	               NULL); /* func data */

	global_mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(global_mainloop);

	gchar * killline = g_strdup_printf("kill -9 %d", dbus);
	g_spawn_command_line_sync(killline, NULL, NULL, NULL, NULL);
	g_free(killline);

	return !global_success;
}
