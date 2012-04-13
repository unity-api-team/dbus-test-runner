#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

struct _DbusTestProcessPrivate {
	gchar * executable;
	GList * parameters;

	GPid pid;
	guint io_watch;
	guint watcher;

	gboolean complete;
	gint status;
};

#define DBUS_TEST_PROCESS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_PROCESS, DbusTestProcessPrivate))

static void dbus_test_process_class_init (DbusTestProcessClass *klass);
static void dbus_test_process_init       (DbusTestProcess *self);
static void dbus_test_process_dispose    (GObject *object);
static void dbus_test_process_finalize   (GObject *object);
static void process_run                  (DbusTestTask * task);

G_DEFINE_TYPE (DbusTestProcess, dbus_test_process, DBUS_TEST_TYPE_TASK);

static void
dbus_test_process_class_init (DbusTestProcessClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestProcessPrivate));

	object_class->dispose = dbus_test_process_dispose;
	object_class->finalize = dbus_test_process_finalize;

	DbusTestTaskClass * task_class = DBUS_TEST_TASK_CLASS(klass);

	task_class->run = process_run;

	return;
}

static void
dbus_test_process_init (DbusTestProcess *self)
{
	self->priv = DBUS_TEST_PROCESS_GET_PRIVATE(self);

	self->priv->executable = NULL;
	self->priv->parameters = NULL;

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

	g_list_free_full(process->priv->parameters, g_free);
	process->priv->parameters = NULL;

	G_OBJECT_CLASS (dbus_test_process_parent_class)->finalize (object);
	return;
}

static void
proc_watcher (GPid pid, gint status, gpointer data)
{
	g_return_if_fail(DBUS_TEST_IS_PROCESS(data));
	DbusTestProcess * process = DBUS_TEST_PROCESS(data);

	if (pid != 0) {
		g_spawn_close_pid(pid);
		process->priv->pid = 0;
	}

	process->priv->complete = TRUE;
	process->priv->status = status;

	/* TODO: Signal finished */

	return;
}

gboolean
proc_writes (GIOChannel * channel, GIOCondition condition, gpointer data)
{
	g_return_val_if_fail(DBUS_TEST_IS_PROCESS(data), FALSE);
	DbusTestProcess * process = DBUS_TEST_PROCESS(data);

	gchar * line;
	gsize termloc;

	do {
		GIOStatus status = g_io_channel_read_line (channel, &line, NULL, &termloc, NULL);

		if (status == G_IO_STATUS_EOF) {
			/* TODO, end this task */
			// task->text_die = TRUE;
			//check_task_cleanup(task, FALSE);
			continue;
		}

		if (status != G_IO_STATUS_NORMAL) {
			continue;
		}

		line[termloc] = '\0';

		dbus_test_task_print(DBUS_TEST_TASK(process), line);
		g_free(line);
	} while (G_IO_IN & g_io_channel_get_buffer_condition(channel));

	return TRUE;
}

void
process_run (DbusTestTask * task)
{
	g_return_if_fail(DBUS_TEST_IS_PROCESS(task));
	DbusTestProcess * process = DBUS_TEST_PROCESS(task);

	gchar ** argv;
	argv = g_new0(gchar *, g_list_length(process->priv->parameters) + 2);

	argv[0] = process->priv->executable;
	int i;
	for (i = 0; i < g_list_length(process->priv->parameters); i++) {
		argv[i + 1] = (gchar *)g_list_nth(process->priv->parameters, i)->data;
	}

	GError * error = NULL;
	gint proc_stdout;
	g_spawn_async_with_pipes(g_get_current_dir(),
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
	g_free(argv);

	if (error != NULL) {
		g_warning("Unable to start process '%s': %s", process->priv->executable, error->message);
		/* TODO: Signal finished */
		return;
	}

	if (TRUE) {
		gchar * message = g_strdup_printf("Started with PID: %d", process->priv->pid);
		dbus_test_task_print(task, message);
		g_free(message);
	}

	GIOChannel * iochan = g_io_channel_unix_new(proc_stdout);
	g_io_channel_set_buffer_size(iochan, 10 * 1024 * 1024); /* 10 MB should be enough for anyone */
	process->priv->io_watch = g_io_add_watch(iochan,
	                                         G_IO_IN, /* conditions */
	                                         proc_writes, /* func */
	                                         process); /* func data */

	process->priv->watcher = g_child_watch_add(process->priv->pid, proc_watcher, process);

	return;
}
