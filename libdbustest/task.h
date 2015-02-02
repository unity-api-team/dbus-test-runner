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

#ifndef __DBUS_TEST_TASK_H__
#define __DBUS_TEST_TASK_H__

#ifndef __DBUS_TEST_TOP_LEVEL__
#error "Please include #include <libdbustest/dbus-test.h> only"
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DBUS_TEST_TASK_SIGNAL_STATE_CHANGED  "state-changed"

#define DBUS_TEST_TYPE_TASK            (dbus_test_task_get_type ())
#define DBUS_TEST_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUS_TEST_TYPE_TASK, DbusTestTask))
#define DBUS_TEST_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_TEST_TYPE_TASK, DbusTestTaskClass))
#define DBUS_TEST_IS_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUS_TEST_TYPE_TASK))
#define DBUS_TEST_IS_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_TEST_TYPE_TASK))
#define DBUS_TEST_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_TEST_TYPE_TASK, DbusTestTaskClass))

typedef struct _DbusTestTask        DbusTestTask;
typedef struct _DbusTestTaskClass   DbusTestTaskClass;
typedef struct _DbusTestTaskPrivate DbusTestTaskPrivate;

typedef enum
{
	DBUS_TEST_TASK_STATE_INIT,
	DBUS_TEST_TASK_STATE_WAITING,
	DBUS_TEST_TASK_STATE_RUNNING,
	DBUS_TEST_TASK_STATE_FINISHED
} DbusTestTaskState;

typedef enum
{
	DBUS_TEST_TASK_RETURN_NORMAL,
	DBUS_TEST_TASK_RETURN_IGNORE,
	DBUS_TEST_TASK_RETURN_INVERT
} DbusTestTaskReturn;

struct _DbusTestTaskClass {
	GObjectClass parent_class;

	/* Subclassable functions */
	void               (*run)         (DbusTestTask * task);
	DbusTestTaskState  (*get_state)   (DbusTestTask * task);
	gboolean           (*get_passed)  (DbusTestTask * task);

	/* Signals */
	void               (*state_changed) (DbusTestTask * task, DbusTestTaskState new_state, gpointer user_data);
};

struct _DbusTestTask {
	GObject parent;
	DbusTestTaskPrivate * priv;
};

#include "service.h"

GType dbus_test_task_get_type (void);
DbusTestTask * dbus_test_task_new (void);

void dbus_test_task_set_name (DbusTestTask * task, const gchar * name);
void dbus_test_task_set_name_spacing (DbusTestTask * task, glong chars);
void dbus_test_task_set_wait_for (DbusTestTask * task, const gchar * dbus_name);
void dbus_test_task_set_wait_for_bus (DbusTestTask * task, const gchar * dbus_name, DbusTestServiceBus bus);
void dbus_test_task_set_return (DbusTestTask * task, DbusTestTaskReturn ret);
void dbus_test_task_set_wait_finished (DbusTestTask * task, gboolean wait_till_complete);
void dbus_test_task_set_bus (DbusTestTask * task, DbusTestServiceBus bus);

void dbus_test_task_print (DbusTestTask * task, const gchar * message);

DbusTestTaskState dbus_test_task_get_state (DbusTestTask * task);
DbusTestTaskReturn dbus_test_task_get_return (DbusTestTask * task);
const gchar * dbus_test_task_get_name (DbusTestTask * task);
const gchar * dbus_test_task_get_wait_for (DbusTestTask * task);
gboolean dbus_test_task_get_wait_finished (DbusTestTask * task);
DbusTestServiceBus dbus_test_task_get_bus (DbusTestTask * task);

void dbus_test_task_run (DbusTestTask * task);

gboolean dbus_test_task_passed (DbusTestTask * task);

G_END_DECLS

#endif
