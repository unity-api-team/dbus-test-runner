/*
Copyright 2012 Canonical Ltd.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

#include "glib-compat.h"

struct _DbusTestProcessPrivate {
	gchar * executable;
	GArray * parameters;

	GPid pid;
	guint io_watch;
	guint watcher;
	GIOChannel * io_chan;

	gboolean complete;
	gint status;
};

enum {
	PROP_0,
	PROP_EXECUTABLE,
	PROP_PARAMETERS,
	NUM_PROPS
};

#define DBUS_TEST_PROCESS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_PROCESS, DbusTestProcessPrivate))

static void dbus_test_process_class_init (DbusTestProcessClass *klass);
static void dbus_test_process_init       (DbusTestProcess *self);
static void dbus_test_process_dispose    (GObject *object);
static void dbus_test_process_finalize   (GObject *object);
static void process_run                  (DbusTestTask * task);
static DbusTestTaskState get_state       (DbusTestTask * task);
static gboolean get_passed               (DbusTestTask * task);
static void get_property                 (GObject * object,
                                          guint property_id,
                                          GValue * value,
                                          GParamSpec * pspec);
static void set_property                 (GObject * object,
                                          guint property_id,
                                          const GValue * value,
                                          GParamSpec * pspec);

G_DEFINE_TYPE (DbusTestProcess, dbus_test_process, DBUS_TEST_TYPE_TASK);

static void
dbus_test_process_class_init (DbusTestProcessClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestProcessPrivate));

	object_class->dispose = dbus_test_process_dispose;
	object_class->finalize = dbus_test_process_finalize;
	object_class->get_property = get_property;
	object_class->set_property = set_property;

	g_object_class_install_property (object_class, PROP_EXECUTABLE,
	                                 g_param_spec_string("executable",
	                                                     "Executable Name",
	                                                     "The executable being run by the process object",
	                                                     "", /* default */
	                                                     G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_PARAMETERS,
	                                 g_param_spec_boxed("parameters",
	                                                    "Parameters",
	                                                    "Parameters to pass to the executable",
	                                                    G_TYPE_ARRAY,
	                                                    G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

	DbusTestTaskClass * task_class = DBUS_TEST_TASK_CLASS(klass);

	task_class->run = process_run;
	task_class->get_state = get_state;
	task_class->get_passed = get_passed;

	return;
}

/* Small helper to free the result of the pointer */
static void
array_free_helper (gpointer data)
{
	gchar ** typed_data = (gchar **)data;
	g_free(*typed_data);
	return;
}

static void
dbus_test_process_init (DbusTestProcess *self)
{
	self->priv = DBUS_TEST_PROCESS_GET_PRIVATE(self);

	self->priv->executable = NULL;

	self->priv->parameters = g_array_new(TRUE /* zero terminated */,
	                                     TRUE /* clear */,
	                                     sizeof(gchar *));
	g_array_set_clear_func(self->priv->parameters, array_free_helper);

	self->priv->io_chan = NULL;

	return;
}

static void
dbus_test_process_dispose (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_PROCESS(object));
	DbusTestProcess * process = DBUS_TEST_PROCESS(object);

	if (process->priv->io_watch != 0) {
		g_source_remove(process->priv->io_watch);
		process->priv->io_watch = 0;
	}

	if (process->priv->watcher != 0) {
		g_source_remove(process->priv->watcher);
		process->priv->watcher = 0;
	}

	if (process->priv->pid != 0) {
		gchar * killstr = g_strdup_printf("kill -9 %d", process->priv->pid);
		g_spawn_command_line_async(killstr, NULL);
		g_free(killstr);

		g_spawn_close_pid(process->priv->pid);
		process->priv->pid = 0;
	}

	if (process->priv->io_chan != NULL) {
		GIOStatus status = G_IO_STATUS_NORMAL;

		while ((G_IO_IN & g_io_channel_get_buffer_condition(process->priv->io_chan)) && status == G_IO_STATUS_NORMAL) {
			gchar * line = NULL;
			gsize termloc;

			status = g_io_channel_read_line (process->priv->io_chan, &line, NULL, &termloc, NULL);

			if (status != G_IO_STATUS_NORMAL) {
				continue;
			}

			line[termloc] = '\0';
			dbus_test_task_print(DBUS_TEST_TASK(process), line);
			g_free(line);
		}

		g_clear_pointer(&process->priv->io_chan, g_io_channel_unref);
	}

	G_OBJECT_CLASS (dbus_test_process_parent_class)->dispose (object);
	return;
}

static void
dbus_test_process_finalize (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_PROCESS(object));
	DbusTestProcess * process = DBUS_TEST_PROCESS(object);

	g_free(process->priv->executable);
	process->priv->executable = NULL;

	g_array_free(process->priv->parameters, TRUE /* free segment */);
	process->priv->parameters = NULL;

	G_OBJECT_CLASS (dbus_test_process_parent_class)->finalize (object);
	return;
}

/* Get a property */
static void
get_property (GObject * object, guint property_id, G_GNUC_UNUSED GValue * value, GParamSpec * pspec)
{
	DbusTestProcess * self = DBUS_TEST_PROCESS(object);

	switch (property_id) {
	case PROP_EXECUTABLE:
		g_value_set_string(value, self->priv->executable);
		break;
	case PROP_PARAMETERS:
		g_value_set_boxed(value, self->priv->parameters);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

	return;
}

/* Set a property */
static void
set_property (GObject * object, guint property_id, G_GNUC_UNUSED const GValue * value, GParamSpec * pspec)
{
	if (get_state(DBUS_TEST_TASK(object)) == DBUS_TEST_TASK_STATE_RUNNING) {
		g_warning("Can't set properties on a running process");
		return;
	}

	DbusTestProcess * self = DBUS_TEST_PROCESS(object);

	switch (property_id) {
	case PROP_EXECUTABLE:
		g_free(self->priv->executable);
		self->priv->executable = g_value_dup_string(value);
		break;
	case PROP_PARAMETERS:
		g_array_free(self->priv->parameters, TRUE);
		self->priv->parameters = g_value_dup_boxed(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

	return;
}

static void
proc_watcher (GPid pid, gint status, gpointer data)
{
	gchar *message;
	g_return_if_fail(DBUS_TEST_IS_PROCESS(data));
	DbusTestProcess * process = DBUS_TEST_PROCESS(data);

	if (pid != 0) {
		g_spawn_close_pid(pid);
		process->priv->pid = 0;
	}

	process->priv->complete = TRUE;
	process->priv->status = status;
	process->priv->watcher = 0;

	if (status) {
		message = g_strdup_printf("Exited with status %d", status);
		dbus_test_task_print(DBUS_TEST_TASK(process), message);
		g_free(message);
	}

	g_signal_emit_by_name(G_OBJECT(process), DBUS_TEST_TASK_SIGNAL_STATE_CHANGED, DBUS_TEST_TASK_STATE_FINISHED, NULL);

	return;
}

static gboolean
proc_writes (GIOChannel * channel, G_GNUC_UNUSED GIOCondition condition, gpointer data)
{
	g_return_val_if_fail(DBUS_TEST_IS_PROCESS(data), FALSE);
	DbusTestProcess * process = DBUS_TEST_PROCESS(data);

	gchar * line;
	gsize termloc;
	gboolean done = FALSE;

	do {
		GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);

		if (status == G_IO_STATUS_EOF) {
			done = TRUE;
			continue;
		}

		if (status != G_IO_STATUS_NORMAL) {
			continue;
		}

		line[termloc] = '\0';
		dbus_test_task_print(DBUS_TEST_TASK(process), line);
		g_free(line);
	} while (G_IO_IN & g_io_channel_get_buffer_condition(channel));

	if (done) {
		process->priv->io_watch = 0;
		// wait for proc_watcher to switch state to FINISHED
		return FALSE;
	}

	return TRUE;
}

static void
process_run (DbusTestTask * task)
{
	g_return_if_fail(DBUS_TEST_IS_PROCESS(task));
	DbusTestProcess * process = DBUS_TEST_PROCESS(task);

	gchar * current_dir = g_get_current_dir();

	gchar ** argv;
	argv = g_new0(gchar *, process->priv->parameters->len + 2);

	argv[0] = process->priv->executable;
	guint i;
	for (i = 0; i < process->priv->parameters->len; i++) {
		argv[i + 1] = g_array_index(process->priv->parameters, gchar *, i);
	}

	GError * error = NULL;
	gint proc_stdout;
	g_spawn_async_with_pipes(current_dir,
	                         argv, /* argv */
	                         NULL, /* envp */
	                         G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, /* flags */
	                         NULL, /* child setup func */
	                         NULL, /* child setup data */
							 &(process->priv->pid), /* PID */
	                         NULL, /* stdin */
	                         &proc_stdout, /* stdout */
	                         NULL, /* stderr */
	                         &error); /* error */
	g_free(current_dir);
	g_free(argv);

	if (error != NULL) {
		g_warning("Unable to start process '%s': %s", process->priv->executable, error->message);
		process->priv->complete = TRUE;
		process->priv->status = -1;
		g_signal_emit_by_name(G_OBJECT(process), DBUS_TEST_TASK_SIGNAL_STATE_CHANGED, DBUS_TEST_TASK_STATE_FINISHED, NULL);
		return;
	}

	if (TRUE) {
		gchar * message = g_strdup_printf("Started with PID: %d", process->priv->pid);
		dbus_test_task_print(task, message);
		g_free(message);
	}

	process->priv->io_chan = g_io_channel_unix_new(proc_stdout);
	g_io_channel_set_buffer_size(process->priv->io_chan, 10 * 1024 * 1024); /* 10 MB should be enough for anyone */
	process->priv->io_watch = g_io_add_watch(process->priv->io_chan,
	                                         G_IO_IN | G_IO_HUP | G_IO_ERR, /* conditions */
	                                         proc_writes, /* func */
	                                         process); /* func data */

	process->priv->watcher = g_child_watch_add(process->priv->pid, proc_watcher, process);

	g_signal_emit_by_name(G_OBJECT(process), DBUS_TEST_TASK_SIGNAL_STATE_CHANGED, DBUS_TEST_TASK_STATE_RUNNING, NULL);

	return;
}

DbusTestProcess *
dbus_test_process_new (const gchar * executable)
{
	g_return_val_if_fail(executable != NULL, NULL);

	DbusTestProcess * proc = g_object_new(DBUS_TEST_TYPE_PROCESS,
	                                      "executable", executable,
	                                      NULL);

	return proc;
}

/**
 * dbus_test_process_get_pid:
 * @process: The #DbusTestProcess to check
 *
 * Gets the process ID of the task if it is running
 *
 * Return value: The current PID or 0 if not running
 */
GPid
dbus_test_process_get_pid (DbusTestProcess * process)
{
	g_return_val_if_fail(DBUS_TEST_IS_PROCESS(process), 0);

	if (get_state(DBUS_TEST_TASK(process)) == DBUS_TEST_TASK_STATE_RUNNING) {
		return process->priv->pid;
	}

	return 0;
}

void
dbus_test_process_append_param (DbusTestProcess * process, const gchar * parameter)
{
	g_return_if_fail(DBUS_TEST_IS_PROCESS(process));
	g_return_if_fail(parameter != NULL);

	gchar * newstr = g_strdup(parameter);
	g_array_append_val(process->priv->parameters, newstr);

	return;
}

static DbusTestTaskState
get_state (DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_PROCESS(task), DBUS_TEST_TASK_STATE_FINISHED);
	DbusTestProcess * process = DBUS_TEST_PROCESS(task);

	if (process->priv->complete) {
		return DBUS_TEST_TASK_STATE_FINISHED;
	}

	if (process->priv->pid != 0) {
		return DBUS_TEST_TASK_STATE_RUNNING;
	}

	return DBUS_TEST_TASK_STATE_INIT;
}

static gboolean
get_passed (DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_PROCESS(task), FALSE);
	DbusTestProcess * process = DBUS_TEST_PROCESS(task);

	if (!process->priv->complete) {
		return FALSE;
	}

	if (process->priv->status == 0) {
		return TRUE;
	}

	return FALSE;
}
