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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dbus-mock.h"

struct _DbusTestDbusMockPrivate {
	int dummy;
};

#define DBUS_TEST_DBUS_MOCK_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_DBUS_MOCK, DbusTestDbusMockPrivate))

static void dbus_test_dbus_mock_class_init (DbusTestDbusMockClass *klass);
static void dbus_test_dbus_mock_init       (DbusTestDbusMock *self);
static void dbus_test_dbus_mock_dispose    (GObject *object);
static void dbus_test_dbus_mock_finalize   (GObject *object);

G_DEFINE_TYPE (DbusTestDbusMock, dbus_test_dbus_mock, DBUS_TEST_TYPE_PROCESS);

/* Initialize Class */
static void
dbus_test_dbus_mock_class_init (DbusTestDbusMockClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestDbusMockPrivate));

	object_class->dispose = dbus_test_dbus_mock_dispose;
	object_class->finalize = dbus_test_dbus_mock_finalize;

	return;
}

/* Initialize Instance */
static void
dbus_test_dbus_mock_init (G_GNUC_UNUSED DbusTestDbusMock *self)
{

	return;
}

/* Free references */
static void
dbus_test_dbus_mock_dispose (GObject *object)
{

	G_OBJECT_CLASS (dbus_test_dbus_mock_parent_class)->dispose (object);
	return;
}

/* Free memory */
static void
dbus_test_dbus_mock_finalize (GObject *object)
{

	G_OBJECT_CLASS (dbus_test_dbus_mock_parent_class)->finalize (object);
	return;
}

DbusTestDbusMock *
dbus_test_dbus_mock_new (const gchar * bus_name)
{

	return NULL;
}

DbusTestDbusMockObject *
dbus_test_dbus_mock_get_object (DbusTestDbusMock * mock, const gchar * path, const gchar * interface)
{

	return NULL;
}

gboolean
dbus_test_dbus_mock_object_notify (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj)
{

	return FALSE;
}

gboolean
dbus_test_dbus_mock_object_add_method (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method, GVariantType * inparams, GVariantType * outparams, const gchar * python_code)
{

	return FALSE;
}

gboolean
dbus_test_dbus_mock_object_check_method_call (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method, GVariant * params)
{

	return FALSE;
}

gboolean
dbus_test_dbus_mock_object_clear_method_calls (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method)
{

	return FALSE;
}

GList *
dbus_test_dbus_mock_object_get_method_calls (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method)
{

	return NULL;
}

gboolean
dbus_test_dbus_mock_object_add_property (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, GVariantType * type, GVariant * value)
{

	return FALSE;
}

gboolean
dbus_test_dbus_mock_object_update_property (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, GVariant * value, gboolean signal)
{

	return FALSE;
}

gboolean
dbus_test_dbus_mock_object_emit_signal (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, GVariantType * params, GVariant * values)
{

	return FALSE;
}
