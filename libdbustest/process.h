#ifndef __DBUS_TEST_PROCESS_H__
#define __DBUS_TEST_PROCESS_H__

#include <glib.h>
#include <glib-object.h>
#include "dbus-test.h"

G_BEGIN_DECLS

#define DBUS_TEST_TYPE_PROCESS            (dbus_test_process_get_type ())
#define DBUS_TEST_PROCESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUS_TEST_TYPE_PROCESS, DbusTestProcess))
#define DBUS_TEST_PROCESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_TEST_TYPE_PROCESS, DbusTestProcessClass))
#define DBUS_TEST_IS_PROCESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUS_TEST_TYPE_PROCESS))
#define DBUS_TEST_IS_PROCESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_TEST_TYPE_PROCESS))
#define DBUS_TEST_PROCESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_TEST_TYPE_PROCESS, DbusTestProcessClass))

typedef struct _DbusTestProcess         DbusTestProcess;
typedef struct _DbusTestProcessClass    DbusTestProcessClass;
typedef struct _DbusTestProcessPrivate  DbusTestProcessPrivate;

struct _DbusTestProcessClass {
	DbusTestTaskClass parent_class;
};

struct _DbusTestProcess {
	DbusTestTask parent;
	DbusTestProcessPrivate * priv;
};

GType dbus_test_process_get_type (void);

DbusTestProcess * dbus_test_process_new (const gchar * executable);
void dbus_test_process_append_param (DbusTestProcess * process, const gchar * parameter);

G_END_DECLS

#endif
