#ifndef __DBUS_TEST_TASK_H__
#define __DBUS_TEST_TASK_H__

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

GType dbus_test_task_get_type (void);

G_END_DECLS

#endif
