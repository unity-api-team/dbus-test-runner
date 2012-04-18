#ifndef __DBUS_TEST_BUSTLE_H__
#define __DBUS_TEST_BUSTLE_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DBUS_TEST_TYPE_BUSTLE            (dbus_test_bustle_get_type ())
#define DBUS_TEST_BUSTLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUS_TEST_TYPE_BUSTLE, DbusTestBustle))
#define DBUS_TEST_BUSTLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_TEST_TYPE_BUSTLE, DbusTestBustleClass))
#define DBUS_TEST_IS_BUSTLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUS_TEST_TYPE_BUSTLE))
#define DBUS_TEST_IS_BUSTLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_TEST_TYPE_BUSTLE))
#define DBUS_TEST_BUSTLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_TEST_TYPE_BUSTLE, DbusTestBustleClass))

typedef struct _DbusTestBustle        DbusTestBustle;
typedef struct _DbusTestBustleClass   DbusTestBustleClass;
typedef struct _DbusTestBustlePrivate DbusTestBustlePrivate;

struct _DbusTestBustleClass {
	DbusTestTaskClass parent_class;
};

struct _DbusTestBustle {
	DbusTestTask parent;
	DbusTestBustlePrivate * priv;
};

GType dbus_test_bustle_get_type (void);

DbusTestBustle * dbus_test_bustle_new (const gchar * filename);
void dbus_test_bustle_set_executable (DbusTestBustle * bustle, const gchar * executable);

G_END_DECLS

#endif
