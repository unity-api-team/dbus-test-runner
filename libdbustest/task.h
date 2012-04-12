#ifndef __DBUS_TEST_TASK_H__
#define __DBUS_TEST_TASK_H__

#ifndef __DBUS_TEST_TOP_LEVEL__
#error "Please include #include <libdbustest/dbus-test.h> only"
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DBUS_TEST_TASK_TYPE            (dbus_test_task_get_type ())
#define DBUS_TEST_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUS_TEST_TASK_TYPE, DbusTestTask))
#define DBUS_TEST_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_TEST_TASK_TYPE, DbusTestTaskClass))
#define DBUS_TEST_IS_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUS_TEST_TASK_TYPE))
#define DBUS_TEST_IS_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_TEST_TASK_TYPE))
#define DBUS_TEST_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_TEST_TASK_TYPE, DbusTestTaskClass))

typedef struct _DbusTestTask        DbusTestTask;
typedef struct _DbusTestTaskClass   DbusTestTaskClass;
typedef struct _DbusTestTaskPrivate DbusTestTaskPrivate;

struct _DbusTestTaskClass {
	GObjectClass parent_class;
};

struct _DbusTestTask {
	GObject parent;
	DbusTestTaskPrivate * priv;
};

typedef enum _DbusTestTaskState DbusTestTaskState;
enum _DbusTestTaskState {
	DBUS_TEST_TASK_STATE_INIT,
	DBUS_TEST_TASK_STATE_WAITING,
	DBUS_TEST_TASK_STATE_RUNNING,
	DBUS_TEST_TASK_STATE_FINISHED
};

typedef enum _DbusTestTaskReturn DbusTestTaskReturn;
enum _DbusTestTaskReturn {
	DBUS_TEST_TASK_RETURN_NORMAL,
	DBUS_TEST_TASK_RETURN_IGNORE,
	DBUS_TEST_TASK_RETURN_INVERT
};

GType dbus_test_task_get_type (void);
DbusTestTask * dbus_test_task_new (void);

void dbus_test_task_set_name (DbusTestTask * task, const gchar * name);
void dbus_test_task_set_name_spacing (DbusTestTask * task, guint chars);
void dbus_test_task_set_wait_for (DbusTestTask * task, const gchar * dbus_name);
void dbus_test_task_set_return (DbusTestTask * task, DbusTestTaskReturn ret);

void dbus_test_task_print (DbusTestTask * task, const gchar * message);

DbusTestTaskState dbus_test_task_get_state (DbusTestTask * task);

void dbus_test_task_run (DbusTestTask * task);

gboolean dbus_test_task_passed (DbusTestTask * task);

G_END_DECLS

#endif
