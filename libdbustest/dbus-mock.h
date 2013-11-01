/*
Copyright 2013 Canonical Ltd.

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

#ifndef __DBUS_TEST_DBUS_MOCK_H__
#define __DBUS_TEST_DBUS_MOCK_H__

#ifndef __DBUS_TEST_TOP_LEVEL__
#error "Please include #include <libdbustest/dbus-test.h> only"
#endif

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
typedef struct _DbusTestDbusMockObject   DbusTestDbusMockObject;
typedef struct _DbusTestDbusMockCall     DbusTestDbusMockCall;

struct _DbusTestDbusMockClass {
	DbusTestProcessClass parent_class;
};

struct _DbusTestDbusMock {
	DbusTestProcess parent;
	DbusTestDbusMockPrivate * priv;
};

struct _DbusTestDbusMockCall {
	guint64 timestamp;
	const gchar * name;
	GVariant * params;
};

GType dbus_test_dbus_mock_get_type (void);

DbusTestDbusMock *          dbus_test_dbus_mock_new                       (const gchar *             bus_name);


/* Object stuff */

DbusTestDbusMockObject *    dbus_test_dbus_mock_get_object                (DbusTestDbusMock *        mock,
                                                                           const gchar *             path,
                                                                           const gchar *             interface,
                                                                           GError **                 error);

gboolean                    dbus_test_dbus_mock_object_add_method         (DbusTestDbusMock *        mock,
                                                                           DbusTestDbusMockObject *  obj,
                                                                           const gchar *             method,
                                                                           const GVariantType *      inparams,
                                                                           const GVariantType *      outparams,
                                                                           const gchar *             python_code,
                                                                           GError **                 error);

gboolean                    dbus_test_dbus_mock_object_check_method_call  (DbusTestDbusMock *        mock,
                                                                           DbusTestDbusMockObject *  obj,
                                                                           const gchar *             method,
                                                                           GVariant *                params,
                                                                           GError **                 error);

gboolean                    dbus_test_dbus_mock_object_clear_method_calls (DbusTestDbusMock *        mock,
                                                                           DbusTestDbusMockObject *  obj,
                                                                           GError **                 error);

const DbusTestDbusMockCall * dbus_test_dbus_mock_object_get_method_calls  (DbusTestDbusMock *        mock,
                                                                           DbusTestDbusMockObject *  obj,
                                                                           const gchar *             method,
                                                                           guint *                   len,
                                                                           GError **                 error);

gboolean                    dbus_test_dbus_mock_object_add_property       (DbusTestDbusMock *        mock,
                                                                           DbusTestDbusMockObject *  obj,
                                                                           const gchar *             name,
                                                                           const GVariantType *      type,
                                                                           GVariant *                value,
                                                                           GError **                 error);

gboolean                    dbus_test_dbus_mock_object_update_property    (DbusTestDbusMock *        mock,
                                                                           DbusTestDbusMockObject *  obj,
                                                                           const gchar *             name,
                                                                           GVariant *                value,
                                                                           GError **                 error);

gboolean                    dbus_test_dbus_mock_object_emit_signal        (DbusTestDbusMock *        mock,
                                                                           DbusTestDbusMockObject *  obj,
                                                                           const gchar *             name,
                                                                           const GVariantType *      params,
                                                                           GVariant *                values,
                                                                           GError **                 error);

G_END_DECLS

#endif
