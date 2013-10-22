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

/**
 * dbus_test_dbus_mock_new:
 * @bus_name: The name dbus mock should get on the bus
 *
 * Creates a new dbus mock process with a given name on the bus.  This actually
 * doesn't cause the process to start until the whole DBusTest framework is told
 * to run.  But it represents one that will start when told to.
 *
 * Return value: A new dbus mock instance
 */
DbusTestDbusMock *
dbus_test_dbus_mock_new (const gchar * bus_name)
{
	g_return_val_if_fail(bus_name != NULL, NULL);

	return NULL;
}

/**
 * dbus_test_dbus_mock_get_object:
 * @mock: A #DbusTestDbusMock instance
 * @path: DBus path of the object
 * @interface: Interface on that object
 *
 * Gets a pointer to a handle for an object on the dbus mock instance.  If it
 * didn't exist previous to calling this function, it is created.  If it did,
 * this is the pointer for it.  When the dbus mock is started this object will
 * be created with the parameters and methods that are added to it.
 *
 * Return Value: (transfer none): Handle to refer to an object on the DBus Mock
 */
DbusTestDbusMockObject *
dbus_test_dbus_mock_get_object (DbusTestDbusMock * mock, const gchar * path, const gchar * interface)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), NULL);
	g_return_val_if_fail(path != NULL, NULL);
	g_return_val_if_fail(interface != NULL, NULL);


	return NULL;
}

/**
 * dbus_test_dbus_mock_object_notify:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 *
 * Signal that the object was created.
 *
 * Return value: Success on emitting the signal
 */
gboolean
dbus_test_dbus_mock_object_notify (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);

	return FALSE;
}

/**
 * dbus_test_dbus_mock_object_add_method:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @method: Name of the method
 * @inparams: Parameters going into the method as a tuple
 * @outparams: Parameters gonig out of the method as a tuple
 * @python_code: Python code to execute when the method is called
 *
 * Sets up a method on the object specified.  When the method is activated this is
 * both tracked by DBusMock and the code in @python_code is executed.  This then
 * can return a value that is the same type as @outparams.
 *
 * Return value: Whether it was registered successfully
 */
gboolean
dbus_test_dbus_mock_object_add_method (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method, GVariantType * inparams, GVariantType * outparams, const gchar * python_code)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(method != NULL, FALSE);
	g_return_val_if_fail(inparams != NULL, FALSE);
	g_return_val_if_fail(outparams != NULL, FALSE);
	g_return_val_if_fail(python_code != NULL, FALSE);


	return FALSE;
}

/**
 * dbus_test_dbus_mock_object_check_method_call:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @method: Name of the method
 * @params: (allow none): Parameters to check
 *
 * Quick function to check to see if a method was called.  If the @params value is set
 * then the parameters of the call will also be checked.  If the method was called more
 * than once this function will return FALSE.
 *
 * Return value: Whether the function was called
 */
gboolean
dbus_test_dbus_mock_object_check_method_call (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method, G_GNUC_UNUSED GVariant * params)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(method != NULL, FALSE);

	return FALSE;
}

/**
 * dbus_test_dbus_mock_object_clear_method_calls:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @method: Name of the method
 *
 * Clears the queued set of method calls for the method.
 *
 * Return value: Whether we were able to clear it
 */
gboolean
dbus_test_dbus_mock_object_clear_method_calls (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(method != NULL, FALSE);

	return FALSE;
}

/**
 * dbus_test_dbus_mock_object_get_method_calls:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @method: Name of the method
 *
 * Gets a list of all method calls for a function including the parmeters.
 *
 * Return value: (transfer full): TBD
 */
GList *
dbus_test_dbus_mock_object_get_method_calls (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(method != NULL, FALSE);


	return NULL;
}

/**
 * dbus_test_dbus_mock_object_add_property:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @name: Name of the property
 * @type: Type of the property
 * @value: Initial value of the property
 *
 * Adds a property to the object and sets its initial value.
 *
 * Return value: Whether it was added
 */
gboolean
dbus_test_dbus_mock_object_add_property (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, GVariantType * type, GVariant * value)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);
	g_return_val_if_fail(type != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);


	return FALSE;
}

/**
 * dbus_test_dbus_mock_object_update_property:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @name: Name of the property
 * @value: Initial value of the property
 * @signal: Whether to signal that the property value changed
 *
 * Changes the value of a property and will send a signal that it changed
 * depending on the value of @signal.
 *
 * Return value: Whether it was changed
 */
gboolean
dbus_test_dbus_mock_object_update_property (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, GVariant * value, G_GNUC_UNUSED gboolean signal)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);

	return FALSE;
}

/**
 * dbus_test_dbus_mock_object_emit_signal:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @name: Name of the signal
 * @params: The parameters of the signal as a tuple
 * @values: Values to emit with the signal
 *
 * Causes the object on the dbus mock to emit a signal with the @params
 * provided.
 *
 * Return value: Whether we were able to request the signal
 *   to be emitted.
 */
gboolean
dbus_test_dbus_mock_object_emit_signal (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, GVariantType * params, G_GNUC_UNUSED GVariant * values)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);
	g_return_val_if_fail(params != NULL, FALSE);

	return FALSE;
}
