#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-test.h"
#include <gio/gio.h>

struct _DbusTestTaskPrivate {
	DbusTestTaskReturn return_type;

	gchar * wait_for;
	guint wait_task;

	gchar * name;
	gchar * name_padded;
	guint padding_cnt;

	gboolean been_run;
};

#define DBUS_TEST_TASK_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_TASK, DbusTestTaskPrivate))

static void dbus_test_task_class_init (DbusTestTaskClass *klass);
static void dbus_test_task_init       (DbusTestTask *self);
static void dbus_test_task_dispose    (GObject *object);
static void dbus_test_task_finalize   (GObject *object);

G_DEFINE_TYPE (DbusTestTask, dbus_test_task, G_TYPE_OBJECT);

static void
dbus_test_task_class_init (DbusTestTaskClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestTaskPrivate));

	object_class->dispose = dbus_test_task_dispose;
	object_class->finalize = dbus_test_task_finalize;

	klass->run = NULL;
	klass->get_state = NULL;
	klass->get_passed = NULL;

	return;
}

static void
dbus_test_task_init (DbusTestTask *self)
{
	static gint task_count = 0;

	self->priv = DBUS_TEST_TASK_GET_PRIVATE(self);

	self->priv->return_type = DBUS_TEST_TASK_RETURN_NORMAL;

	self->priv->wait_for = NULL;
	self->priv->wait_task = 0;

	self->priv->name = g_strdup_printf("task-%d", task_count++);
	self->priv->name_padded = NULL;
	self->priv->padding_cnt = 0;

	self->priv->been_run = FALSE;

	return;
}

static void
dbus_test_task_dispose (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(object));
	DbusTestTask * self = DBUS_TEST_TASK(object);

	if (self->priv->wait_task != 0) {
		g_bus_unwatch_name(self->priv->wait_task);
		self->priv->wait_task = 0;
	}

	G_OBJECT_CLASS (dbus_test_task_parent_class)->dispose (object);
	return;
}

static void
dbus_test_task_finalize (GObject *object)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(object));
	DbusTestTask * self = DBUS_TEST_TASK(object);

	g_free(self->priv->name);
	g_free(self->priv->name_padded);
	g_free(self->priv->wait_for);

	G_OBJECT_CLASS (dbus_test_task_parent_class)->finalize (object);
	return;
}

DbusTestTask *
dbus_test_task_new (void)
{
	DbusTestTask * task = g_object_new(DBUS_TEST_TYPE_TASK,
	                                   NULL);

	return task;
}

void
dbus_test_task_set_name (DbusTestTask * task, const gchar * name)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(task));

	g_free(task->priv->name);
	g_free(task->priv->name_padded);

	task->priv->name = g_strdup(name);
	if (task->priv->padding_cnt != 0 && task->priv->name != NULL) {
		gchar * fillstr = g_strnfill(task->priv->padding_cnt - g_utf8_strlen(task->priv->name, -1), ' ');
		task->priv->name_padded = g_strconcat(task->priv->name, fillstr, NULL);
		g_free(fillstr);
	} else {
		task->priv->name_padded = NULL;
	}

	return;
}

void
dbus_test_task_set_name_spacing (DbusTestTask * task, guint chars)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(task));

	g_free(task->priv->name_padded);

	if (chars != 0 && task->priv->name != NULL) {
		gchar * fillstr = g_strnfill(task->priv->padding_cnt - g_utf8_strlen(task->priv->name, -1), ' ');
		task->priv->name_padded = g_strconcat(task->priv->name, fillstr, NULL);
		g_free(fillstr);
	} else {
		task->priv->name_padded = NULL;
	}

	return;
}

void
dbus_test_task_set_wait_for (DbusTestTask * task, const gchar * dbus_name)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(task));

	if (task->priv->wait_for != NULL) {
		g_free(task->priv->wait_for);
		task->priv->wait_for = NULL;
	}

	if (dbus_name == NULL) {
		return;
	}

	task->priv->wait_for = g_strdup(dbus_name);

	return;
}

void
dbus_test_task_set_return (DbusTestTask * task, DbusTestTaskReturn ret)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(task));

	if (ret != task->priv->return_type && dbus_test_task_get_state(task) == DBUS_TEST_TASK_STATE_FINISHED) {
		g_warning("Changing return type after the task has finished");
	}

	task->priv->return_type = ret;
	return;
}

void
dbus_test_task_print (DbusTestTask * task, const gchar * message)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(task));
	g_return_if_fail(message != NULL);

	gchar * name = task->priv->name;
	if (task->priv->name_padded != NULL) {
		name = task->priv->name_padded;
	}

	g_print("%s: %s", name, message);

	return;
}

DbusTestTaskState
dbus_test_task_get_state (DbusTestTask * task)
{

	return DBUS_TEST_TASK_STATE_FINISHED;
}

static void
wait_for_found (GDBusConnection * connection, const gchar * name, const gchar * name_owner, gpointer user_data)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(user_data));
	DbusTestTask * task = DBUS_TEST_TASK(user_data);

	task->priv->wait_task = 0;

	DbusTestTaskClass * klass = DBUS_TEST_TASK_GET_CLASS(task);
	if (klass->run != NULL) {
		klass->run(task);
	} else {
		task->priv->been_run = TRUE;
		/* TODO: signal finished */
	}

	return;
}

void
dbus_test_task_run (DbusTestTask * task)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(task));

	/* We're going to process the waiting at this level if we've been
	   asked to do so */
	if (task->priv->wait_for != NULL) {
		task->priv->wait_task = g_bus_watch_name(G_BUS_TYPE_SESSION,
		                                         task->priv->wait_for,
		                                         G_BUS_NAME_WATCHER_FLAGS_NONE,
		                                         wait_for_found,
		                                         NULL,
		                                         task,
		                                         NULL);
		return;
	}

	DbusTestTaskClass * klass = DBUS_TEST_TASK_GET_CLASS(task);
	if (klass->run != NULL) {
		klass->run(task);
	} else {
		task->priv->been_run = TRUE;
		/* TODO: signal finished */
	}

	return;
}

gboolean
dbus_test_task_passed (DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_TASK(task), FALSE);

	/* If we don't care, we always pass */
	if (task->priv->return_type == DBUS_TEST_TASK_RETURN_IGNORE) {
		return TRUE;
	}

	DbusTestTaskClass * klass = DBUS_TEST_TASK_GET_CLASS(task);
	if (klass->get_passed == NULL) {
		return FALSE;
	}

	gboolean subret = klass->get_passed(task);

	if (task->priv->return_type == DBUS_TEST_TASK_RETURN_INVERT) {
		return !subret;
	}

	return subret;
}
