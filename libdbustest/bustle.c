#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"

struct _DbusTestBustlePrivate {
	gchar * filename;
	gchar * executable;

	guint watch;
	GIOChannel * stdout;
	GIOChannel * stderr;
	GIOChannel * file;
	GPid pid;
};

#define DBUS_TEST_BUSTLE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_BUSTLE, DbusTestBustlePrivate))

static void dbus_test_bustle_class_init (DbusTestBustleClass *klass);
static void dbus_test_bustle_init       (DbusTestBustle *self);
static void dbus_test_bustle_dispose    (GObject *object);
static void dbus_test_bustle_finalize   (GObject *object);
static void process_run                 (DbusTestTask * task);
static DbusTestTaskState get_state      (DbusTestTask * task);
static gboolean get_passed              (DbusTestTask * task);

G_DEFINE_TYPE (DbusTestBustle, dbus_test_bustle, DBUS_TEST_TYPE_TASK);

static void
dbus_test_bustle_class_init (DbusTestBustleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestBustlePrivate));

	object_class->dispose = dbus_test_bustle_dispose;
	object_class->finalize = dbus_test_bustle_finalize;

	DbusTestTaskClass * task_class = DBUS_TEST_TASK_CLASS(klass);

	task_class->run = process_run;
	task_class->get_state = get_state;
	task_class->get_passed = get_passed;

	return;
}

static void
dbus_test_bustle_init (DbusTestBustle *self)
{
	self->priv = DBUS_TEST_BUSTLE_GET_PRIVATE(self);

	self->priv->filename = g_strconcat(g_get_current_dir(), G_DIR_SEPARATOR_S, "bustle.log", NULL);
	self->priv->executable = g_strdup("bustle-dbus-monitor");

	self->priv->watch = 0;
	self->priv->stdout = NULL;
	self->priv->stderr = NULL;
	self->priv->file = NULL;
	self->priv->pid = 0;

	return;
}

static void
dbus_test_bustle_dispose (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_BUSTLE(object));
	DbusTestBustle * bustler = DBUS_TEST_BUSTLE(object);

	if (bustler->priv->watch != 0) {
		g_source_remove(bustler->priv->watch);
		bustler->priv->watch = 0;
	}

	if (bustler->priv->pid != 0) {
		/* TODO: Send a single dbus message */

		g_spawn_close_pid(bustler->priv->pid);
	}

	if (bustler->priv->stdout != NULL) {
		g_io_channel_shutdown(bustler->priv->stdout, TRUE, NULL);
		bustler->priv->stdout = NULL;
	}

	if (bustler->priv->stderr != NULL) {
		g_io_channel_shutdown(bustler->priv->stderr, TRUE, NULL);
		bustler->priv->stderr = NULL;
	}

	if (bustler->priv->file != NULL) {
		g_io_channel_shutdown(bustler->priv->file, TRUE, NULL);
		bustler->priv->file = NULL;
	}

	G_OBJECT_CLASS (dbus_test_bustle_parent_class)->dispose (object);
	return;
}

static void
dbus_test_bustle_finalize (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_BUSTLE(object));
	DbusTestBustle * bustler = DBUS_TEST_BUSTLE(object);

	g_free(bustler->priv->filename);
	g_free(bustler->priv->executable);

	G_OBJECT_CLASS (dbus_test_bustle_parent_class)->finalize (object);
	return;
}

DbusTestBustle *
dbus_test_bustle_new (const gchar * filename)
{
	g_return_val_if_fail(filename != NULL, NULL);

	DbusTestBustle * bustler = g_object_new(DBUS_TEST_TYPE_BUSTLE,
	                                        NULL);

	g_free(bustler->priv->filename);
	bustler->priv->filename = g_strdup(filename);

	return bustler;
}

void
dbus_test_bustle_set_executable (DbusTestBustle * bustle, const gchar * executable)
{
	g_return_if_fail(DBUS_TEST_IS_BUSTLE(bustle));
	g_return_if_fail(executable != NULL);

	g_free(bustle->priv->executable);
	bustle->priv->executable = g_strdup(executable);

	return;
}

static void
process_run (DbusTestTask * task)
{

	return;
}

static DbusTestTaskState
get_state (DbusTestTask * task)
{
	/* We're always finished, but we want an error */
	g_return_val_if_fail(DBUS_TEST_IS_BUSTLE(task), DBUS_TEST_TASK_STATE_FINISHED);
	return DBUS_TEST_TASK_STATE_FINISHED;
}

static gboolean
get_passed (DbusTestTask * task)
{
	/* We can only fail if you fuck up, don't do it! */
	g_return_val_if_fail(DBUS_TEST_IS_BUSTLE(task), FALSE);
	return TRUE;
}

