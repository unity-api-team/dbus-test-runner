#ifndef __DBUS_TEST_DBUS_MOCK_H__
#define __DBUS_TEST_DBUS_MOCK_H__

#include <glib-object.h>
#include "process.h"

G_BEGIN_DECLS

#define DBUS_TEST_TYPE_DBUS_MOCK            (dbus_test_dbus_mock_get_type ())
#define DBUS_TEST_DBUS_MOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUS_TEST_TYPE_DBUS_MOCK, DbusTestDbusMock))
#define DBUS_TEST_DBUS_MOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_TEST_TYPE_DBUS_MOCK, DbusTestDbusMockClass))
#define DBUS_TEST_IS_DBUS_MOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUS_TEST_TYPE_DBUS_MOCK))
#define DBUS_TEST_IS_DBUS_MOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_TEST_TYPE_DBUS_MOCK))
#define DBUS_TEST_DBUS_MOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_TEST_TYPE_DBUS_MOCK, DbusTestDbusMockClass))

typedef struct _DbusTestDbusMock         DbusTestDbusMock;
typedef struct _DbusTestDbusMockClass    DbusTestDbusMockClass;
typedef struct _DbusTestDbusMockPrivate  DbusTestDbusMockPrivate;

struct _DbusTestDbusMockClass {
	DbusTestProcessClass parent_class;
};

struct _DbusTestDbusMock {
	DbusTestProcess parent;
	DbusTestDbusMockPrivate * priv;
};

GType dbus_test_dbus_mock_get_type (void);

G_END_DECLS

#endif
