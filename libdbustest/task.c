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
#include <gio/gio.h>

struct _DbusTestTaskPrivate {
	DbusTestTaskReturn return_type;

	gchar * wait_for;
	guint wait_task;

	gchar * name;
	gchar * name_padded;
	glong padding_cnt;

	gboolean been_run;
};

/* Signals */
enum {
	STATE_CHANGED,
	LAST_SIGNAL /* Don't touch! */
};

#define DBUS_TEST_TASK_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_TASK, DbusTestTaskPrivate))

static void dbus_test_task_class_init (DbusTestTaskClass *klass);
static void dbus_test_task_init       (DbusTestTask *self);
static void dbus_test_task_dispose    (GObject *object);
static void dbus_test_task_finalize   (GObject *object);

G_DEFINE_TYPE (DbusTestTask, dbus_test_task, G_TYPE_OBJECT);

static guint signals[LAST_SIGNAL] = {0};

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

	signals[STATE_CHANGED]  = g_signal_new(DBUS_TEST_TASK_SIGNAL_STATE_CHANGED,
	                                       G_TYPE_FROM_CLASS (klass),
	                                       G_SIGNAL_RUN_LAST,
	                                       G_STRUCT_OFFSET (DbusTestTaskClass, state_changed),
	                                       NULL, NULL,
	                                       g_cclosure_marshal_VOID__INT,
	                                       G_TYPE_NONE, 1, G_TYPE_INT, G_TYPE_NONE);

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
dbus_test_task_set_name_spacing (DbusTestTask * task, glong chars)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(task));

	g_free(task->priv->name_padded);
	task->priv->padding_cnt = chars;

	g_return_if_fail(task->priv->padding_cnt >= g_utf8_strlen(task->priv->name, -1));

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

	g_print("%s: %s\n", name, message);

	return;
}

DbusTestTaskState
dbus_test_task_get_state (DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_TASK(task), DBUS_TEST_TASK_STATE_FINISHED);

	if (task->priv->wait_task != 0) {
		return DBUS_TEST_TASK_STATE_WAITING;
	}

	DbusTestTaskClass * klass = DBUS_TEST_TASK_GET_CLASS(task);
	if (klass->get_state != NULL) {
		return klass->get_state(task);
	}

	if (task->priv->been_run) {
		return DBUS_TEST_TASK_STATE_FINISHED;
	} else {
		return DBUS_TEST_TASK_STATE_INIT;
	}
}

DbusTestTaskReturn
dbus_test_task_get_return (DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_TASK(task), DBUS_TEST_TASK_RETURN_IGNORE);

	return task->priv->return_type;
}

static void
wait_for_found (GDBusConnection * connection, const gchar * name, const gchar * name_owner, gpointer user_data)
{
	g_return_if_fail(DBUS_TEST_IS_TASK(user_data));
	DbusTestTask * task = DBUS_TEST_TASK(user_data);

	g_bus_unwatch_name(task->priv->wait_task);
	task->priv->wait_task = 0;

	DbusTestTaskClass * klass = DBUS_TEST_TASK_GET_CLASS(task);
	if (klass->run != NULL) {
		klass->run(task);
	} else {
		task->priv->been_run = TRUE;
		g_signal_emit(G_OBJECT(task), signals[STATE_CHANGED], 0, DBUS_TEST_TASK_STATE_FINISHED, NULL);
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
		g_signal_emit(G_OBJECT(task), signals[STATE_CHANGED], 0, DBUS_TEST_TASK_STATE_WAITING, NULL);
		return;
	}

	DbusTestTaskClass * klass = DBUS_TEST_TASK_GET_CLASS(task);
	if (klass->run != NULL) {
		klass->run(task);
	} else {
		task->priv->been_run = TRUE;
		g_signal_emit(G_OBJECT(task), signals[STATE_CHANGED], 0, DBUS_TEST_TASK_STATE_FINISHED, NULL);
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

const gchar *
dbus_test_task_get_name (DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_TASK(task), NULL);

	return task->priv->name;
}

const gchar *
dbus_test_task_get_wait_for (DbusTestTask * task)
{
	g_return_val_if_fail(DBUS_TEST_IS_TASK(task), NULL);

	return task->priv->wait_for;
}
