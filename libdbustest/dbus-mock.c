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
#include "dbus-mock-iface.h"

typedef struct _MockObjectProperty MockObjectProperty;
typedef struct _MockObjectMethod MockObjectMethod;

struct _DbusTestDbusMockPrivate {
	gchar * name;
	DbusMockIfaceOrgFreedesktopDBusMock * proxy;
	/* Entries of DbusTestDbusMockObject */
	GArray * objects;
	GDBusConnection * bus;
	GCancellable * cancel;
};

/* Represents every object on the bus that we're mocking */
struct _DbusTestDbusMockObject {
	gchar * object_path;
	gchar * interface;
	GArray * properties;
	GArray * methods;
	DbusMockIfaceOrgFreedesktopDBusMock * proxy;
};

/* A property on an object */
struct _MockObjectProperty {
	gchar * name;
	GVariantType * type;
	GVariant * value;
};

/* A method on an object */
struct _MockObjectMethod {
	gchar * name;
	GVariantType * in;
	GVariantType * out;
	gchar * code;
	GArray * calls;
};

enum {
	PROP_0,
	PROP_DBUS_NAME,
	NUM_PROPS
};

enum {
	ERROR_METHOD_NOT_FOUND,
	NUM_ERRORS
};

#define DBUS_TEST_DBUS_MOCK_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_TEST_TYPE_DBUS_MOCK, DbusTestDbusMockPrivate))

static void dbus_test_dbus_mock_class_init (DbusTestDbusMockClass *klass);
static void dbus_test_dbus_mock_init       (DbusTestDbusMock *self);
static void constructed                    (GObject * object);
static void dbus_test_dbus_mock_dispose    (GObject *object);
static void dbus_test_dbus_mock_finalize   (GObject *object);
static void run                            (DbusTestTask * task);
static void get_property                   (GObject * object,
                                            guint property_id,
                                            GValue * value,
                                            GParamSpec * pspec);
static void set_property                   (GObject * object,
                                            guint property_id,
                                            const GValue * value,
                                            GParamSpec * pspec);
static void object_free                    (gpointer data);
static void method_free                    (gpointer data);
static void property_free                  (gpointer data);

G_DEFINE_TYPE (DbusTestDbusMock, dbus_test_dbus_mock, DBUS_TEST_TYPE_PROCESS);
G_DEFINE_QUARK("dbus-test-dbus-mock", dbus_mock);

/* Initialize Class */
static void
dbus_test_dbus_mock_class_init (DbusTestDbusMockClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusTestDbusMockPrivate));

	object_class->dispose = dbus_test_dbus_mock_dispose;
	object_class->finalize = dbus_test_dbus_mock_finalize;
	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->constructed = constructed;

	g_object_class_install_property (object_class, PROP_DBUS_NAME,
	                                 g_param_spec_string("dbus-name",
	                                                     "DBus Name",
	                                                     "The well known name for dbusmock on the session bus",
	                                                     "com.canonical.DBusTestRunner.DBusMock", /* default */
	                                                     G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	DbusTestTaskClass * tclass = DBUS_TEST_TASK_CLASS(klass);

	tclass->run = run;

	return;
}

/* Initialize Instance */
static void
dbus_test_dbus_mock_init (DbusTestDbusMock *self)
{
	self->priv = DBUS_TEST_DBUS_MOCK_GET_PRIVATE(self);

	self->priv->objects = g_array_new(FALSE, TRUE, sizeof(DbusTestDbusMockObject));
	g_array_set_clear_func(self->priv->objects, object_free);

	self->priv->cancel = g_cancellable_new();

	return;
}

/* Finish init with our properties set */
static void
constructed (GObject * object)
{
	DbusTestDbusMock * self = DBUS_TEST_DBUS_MOCK(object);
	const gchar * paramval = NULL;

	/* Execute: python3 -m dbusmock $name / com.canonical.DbusTest.DbusMock */
	g_object_set(object, "executable", "python3", NULL);

	GArray * params = g_array_new(TRUE, TRUE, sizeof(gchar *));
	/* NOTE: No free func, none of the memory is managed by the array */

	paramval = "-m"; g_array_append_val(params, paramval);
	paramval = "dbusmock"; g_array_append_val(params, paramval);
	g_array_append_val(params, self->priv->name);
	paramval = "/"; g_array_append_val(params, paramval);
	paramval = "com.canonical.DbusTest.DbusMock"; g_array_append_val(params, paramval);

	g_object_set(object, "parameters", params, NULL);
	g_array_unref(params);

	return;
}

/* Free references */
static void
dbus_test_dbus_mock_dispose (GObject *object)
{
	DbusTestDbusMock * self = DBUS_TEST_DBUS_MOCK(object);

	g_cancellable_cancel(self->priv->cancel);
	g_clear_object(&self->priv->cancel);

	g_array_set_size(self->priv->objects, 0);
	g_clear_object(&self->priv->proxy);
	g_clear_object(&self->priv->bus);

	G_OBJECT_CLASS (dbus_test_dbus_mock_parent_class)->dispose (object);
	return;
}

/* Free memory */
static void
dbus_test_dbus_mock_finalize (GObject *object)
{
	DbusTestDbusMock * self = DBUS_TEST_DBUS_MOCK(object);

	g_free(self->priv->name);
	g_array_free(self->priv->objects, TRUE);

	G_OBJECT_CLASS (dbus_test_dbus_mock_parent_class)->finalize (object);
	return;
}

/* Get a property */
static void
get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	DbusTestDbusMock * self = DBUS_TEST_DBUS_MOCK(object);

	switch (property_id) {
	case PROP_DBUS_NAME:
		g_value_set_string(value, self->priv->name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

	return;
}

/* Set a property */
static void
set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	DbusTestDbusMock * self = DBUS_TEST_DBUS_MOCK(object);

	switch (property_id) {
	case PROP_DBUS_NAME:
		g_free(self->priv->name);
		self->priv->name = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

	return;
}

/* Check to see if we're running */
static inline gboolean
is_running (DbusTestDbusMock * mock)
{
	return dbus_test_task_get_state(DBUS_TEST_TASK(mock)) == DBUS_TEST_TASK_STATE_RUNNING;
}

/* Turns a property object into the variant to represent it */
static GVariant *
property_to_variant (MockObjectProperty * prop)
{
	GVariantBuilder builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_DICT_ENTRY);

	g_variant_builder_add_value(&builder, g_variant_new_string(prop->name));
	g_variant_builder_add_value(&builder, g_variant_new_variant(prop->value));

	return g_variant_builder_end(&builder);
}

/* Turns a method into the variant to represent it */
static GVariant *
method_to_variant (MockObjectMethod * method)
{
	GVariantBuilder builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);

	g_variant_builder_add_value(&builder, g_variant_new_string(method->name));
	g_variant_builder_add_value(&builder, g_variant_new_take_string(g_variant_type_dup_string(method->in)));
	g_variant_builder_add_value(&builder, g_variant_new_take_string(g_variant_type_dup_string(method->out)));
	g_variant_builder_add_value(&builder, g_variant_new_string(method->code));

	return g_variant_builder_end(&builder);
}

/* Add an object to the DBus Mock */
static gboolean
install_object (DbusTestDbusMock * mock, DbusTestDbusMockObject * object, GError ** error)
{
	GVariant * properties = NULL;
	GVariant * methods = NULL;

	g_return_val_if_fail(mock->priv->proxy != NULL, FALSE);

	if (object->properties->len > 0) {
		GVariantBuilder property_builder;
		guint i;

		g_variant_builder_init(&property_builder, G_VARIANT_TYPE_ARRAY);

		for (i = 0; i < object->properties->len; i++) {
			MockObjectProperty * prop = &g_array_index(object->properties, MockObjectProperty, i);
			g_variant_builder_add_value(&property_builder, property_to_variant(prop));
		}

		properties = g_variant_builder_end(&property_builder);
	} else {
		properties = g_variant_new_array(G_VARIANT_TYPE("{sv}"), NULL, 0);
	}

	if (object->methods->len > 0) {
		GVariantBuilder method_builder;
		guint i;

		g_variant_builder_init(&method_builder, G_VARIANT_TYPE_ARRAY);

		for (i = 0; i < object->methods->len; i++) {
			MockObjectMethod * method = &g_array_index(object->methods, MockObjectMethod, i);
			g_variant_builder_add_value(&method_builder, method_to_variant(method));
		}

		methods = g_variant_builder_end(&method_builder);
	} else {
		methods = g_variant_new_array(G_VARIANT_TYPE("(ssss)"), NULL, 0);
	}

	gboolean add_object = dbus_mock_iface_org_freedesktop_dbus_mock_call_add_object_sync(
		mock->priv->proxy,
		object->object_path,
		object->interface,
		properties,
		methods,
		mock->priv->cancel,
		error);

	if (!add_object) {
		return add_object;
	}

	object->proxy = dbus_mock_iface_org_freedesktop_dbus_mock_proxy_new_sync(mock->priv->bus,
		G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
		mock->priv->name,
		object->object_path, /* path */
		mock->priv->cancel,
		error
	);

	return object->proxy != NULL;
}

/* Catch the mock taking too long to start */
static gboolean
mock_start_check (gpointer ploop)
{
	GMainLoop * loop = (GMainLoop *)ploop;
	g_main_loop_quit(loop);
	return G_SOURCE_REMOVE;
}

/* Called when the name owner changes, should be to get one */
static void
got_name_owner (GObject * obj, G_GNUC_UNUSED GParamSpec * pspec, gpointer ploop)
{
	gchar * owner = g_dbus_proxy_get_name_owner(G_DBUS_PROXY(obj));
	if (owner != NULL) {
		g_free(owner);
		GMainLoop * loop = (GMainLoop *)ploop;
		g_main_loop_quit(loop);
	}
	return;
}

/* Run the mock */
static void
run (DbusTestTask * task)
{
	GError * error = NULL;
	DbusTestDbusMock * self = DBUS_TEST_DBUS_MOCK(task);

	/* Grab the new bus */
	self->priv->bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	/* Use the process code to get the process running */
	DBUS_TEST_TASK_CLASS (dbus_test_dbus_mock_parent_class)->run (task);

	/**** Initialize the DBus Mock instance ****/

	/* Zero, Setup the proxy */
	self->priv->proxy = dbus_mock_iface_org_freedesktop_dbus_mock_proxy_new_sync(self->priv->bus,
		G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
		self->priv->name,
		"/", /* path */
		self->priv->cancel,
		&error
	);

	if (error != NULL) {
		g_critical("Unable to build proxy to DBusMock: %s", error->message);
		g_error_free(error);
		return;
	}

	/* First, Ensure we have a proxy */
	gchar * owner = g_dbus_proxy_get_name_owner(G_DBUS_PROXY(self->priv->proxy));
	if (owner == NULL) {
		g_debug("Waiting on name from DBusMock");
		GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);

		guint timeout_sig = g_timeout_add_seconds(3, mock_start_check, mainloop);
		gulong owner_sig = g_signal_connect(G_OBJECT(self->priv->proxy), "notify::g-name-owner", G_CALLBACK(got_name_owner), mainloop);

		g_main_loop_run(mainloop);
		g_main_loop_unref(mainloop);

		g_signal_handler_disconnect(self->priv->proxy, owner_sig);
		g_source_remove(timeout_sig);

		owner = g_dbus_proxy_get_name_owner(G_DBUS_PROXY(self->priv->proxy));
		if (owner == NULL) {
			g_critical("Unable to get DBusMock started within 3 seconds");
			return;
		}
	}
	g_free(owner);

	/* Second, Install Objects */
	guint i;
	for (i = 0; i < self->priv->objects->len; i++) {
		GError * error = NULL;

		DbusTestDbusMockObject * obj = &g_array_index(self->priv->objects, DbusTestDbusMockObject, i);
		install_object(self, obj, &error);

		if (error != NULL) {
			g_warning("Unable to install object '%s': %s", obj->object_path, error->message);
			g_error_free(error);
		}
	}

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

	DbusTestDbusMock * mock = g_object_new(DBUS_TEST_TYPE_DBUS_MOCK,
	                                       "dbus-name", bus_name,
	                                       NULL);

	return mock;
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

	/* Check to see if we have that one */
	guint i;
	for (i = 0; i < mock->priv->objects->len; i++) {
		DbusTestDbusMockObject * obj = &g_array_index(mock->priv->objects, DbusTestDbusMockObject, i);

		if (g_strcmp0(path, obj->object_path) == 0 &&
			g_strcmp0(interface, obj->interface) == 0) {
			return obj;
		}
	}

	/* K, that's cool.  We'll build it then. */
	DbusTestDbusMockObject newobj;

	newobj.object_path = g_strdup(path);
	newobj.interface = g_strdup(interface);
	newobj.properties = g_array_new(FALSE, TRUE, sizeof(MockObjectProperty));
	g_array_set_clear_func(newobj.properties, property_free);
	newobj.methods = g_array_new(FALSE, TRUE, sizeof(MockObjectMethod));
	g_array_set_clear_func(newobj.methods, method_free);
	newobj.proxy = NULL;

	g_array_append_val(mock->priv->objects, newobj);
	return &g_array_index(mock->priv->objects, DbusTestDbusMockObject, mock->priv->objects->len - 1);
}

/* Objects are initialized in dbus_test_dbus_mock_get_object() and
   they are free'd in this function */
static void
object_free (gpointer data)
{
	DbusTestDbusMockObject * obj = (DbusTestDbusMockObject *)data;

	g_free(obj->interface);
	g_free(obj->object_path);
	g_array_free(obj->properties, TRUE);
	g_array_free(obj->methods, TRUE);
	g_clear_object(&obj->proxy);

	/* NOTE: No free'ing of data */
	return;
}

/* Little helper to get a method */
static inline MockObjectMethod *
get_obj_method (DbusTestDbusMockObject * obj, const gchar * name)
{
	guint i;
	for (i = 0; i < obj->methods->len; i++) {
		MockObjectMethod * method = &g_array_index(obj->methods, MockObjectMethod, i);
		if (g_strcmp0(method->name, name) == 0) {
			return method;
		}
	}

	return NULL;
}

/* Free the resources for the call */
static void
call_free (gpointer pcall)
{
	DbusTestDbusMockCall * call = (DbusTestDbusMockCall *)pcall;

	g_free((gchar *)call->name);
	g_variant_unref(call->params);
}

/**
 * dbus_test_dbus_mock_object_add_method:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @method: Name of the method
 * @inparams: Parameters going into the method as a tuple
 * @outparams: Parameters gonig out of the method as a tuple
 * @python_code: Python code to execute when the method is called
 * @error: Possible error to return
 *
 * Sets up a method on the object specified.  When the method is activated this is
 * both tracked by DBusMock and the code in @python_code is executed.  This then
 * can return a value that is the same type as @outparams.
 *
 * Return value: Whether it was registered successfully
 */
gboolean
dbus_test_dbus_mock_object_add_method (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method, const GVariantType * inparams, const GVariantType * outparams, const gchar * python_code, G_GNUC_UNUSED GError ** error)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(method != NULL, FALSE);
	g_return_val_if_fail(inparams != NULL, FALSE);
	g_return_val_if_fail(outparams != NULL, FALSE);
	g_return_val_if_fail(python_code != NULL, FALSE);

	/* Check to make sure it doesn't already exist */
	MockObjectMethod * meth = get_obj_method(obj, method);
	g_return_val_if_fail(meth == NULL, FALSE);

	/* Build a new one */
	MockObjectMethod newmethod;
	newmethod.name = g_strdup(method);
	newmethod.in = g_variant_type_copy(inparams);
	newmethod.out = g_variant_type_copy(outparams);
	newmethod.code = g_strdup(python_code);
	newmethod.calls = g_array_new(TRUE, TRUE, sizeof(DbusTestDbusMockCall));
	g_array_set_clear_func(newmethod.calls, call_free);

	g_array_append_val(obj->methods, newmethod);

	/* If we're not running we can just leave it here */
	if (!is_running(mock)) {
		return TRUE;
	}

	/* TODO: Add to running instance */
	return FALSE;
}

/* Free the data allocated in dbus_test_dbus_mock_object_add_method() */
static void
method_free (gpointer data)
{
	MockObjectMethod * method = (MockObjectMethod *)data;

	g_free(method->name);
	g_variant_type_free(method->in);
	g_variant_type_free(method->out);
	g_free(method->code);
	g_array_free(method->calls, TRUE);

	/* NOTE: No free of 'data' */
	return;
}

/**
 * dbus_test_dbus_mock_object_check_method_call:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @method: Name of the method
 * @params: (allow none): Parameters to check
 * @error: A possible error
 *
 * Quick function to check to see if a method was called.  If the @params value is set
 * then the parameters of the call will also be checked.  If the method was called more
 * than once this function will return FALSE.
 *
 * Return value: Whether the function was called
 */
gboolean
dbus_test_dbus_mock_object_check_method_call (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method, GVariant * params, GError ** error)
{
	guint length = 0;
	guint i;
	const DbusTestDbusMockCall * calls;

	calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, method, &length, error);

	if (length == 0) {
		return FALSE;
	}

	/* Don't check params */
	if (params == NULL) {
		return TRUE;
	}

	for (i = 0; i < length; i++) {
		if (g_variant_equal(params, calls[i].params)) {
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * dbus_test_dbus_mock_object_clear_method_calls:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @method: Name of the method
 * @error: A possible error
 *
 * Clears the queued set of method calls for the method.
 *
 * Return value: Whether we were able to clear it
 */
gboolean
dbus_test_dbus_mock_object_clear_method_calls (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method, GError ** error)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(method != NULL, FALSE);

	if (!is_running(mock)) {
		return FALSE;
	}

	g_return_val_if_fail(obj->proxy != NULL, FALSE);

	return dbus_mock_iface_org_freedesktop_dbus_mock_call_clear_calls_sync(
		obj->proxy,
		mock->priv->cancel,
		error
	);
}

/* We get back an av from DBusMock but everyone else uses
   a tuple.  Let's use that */
GVariant *
variant_array_to_tuple (GVariant * array_in)
{
	if (g_variant_n_children(array_in) == 0) {
		return g_variant_new_tuple(NULL, 0);
	}

	GVariantIter iter;
	g_variant_iter_init(&iter, array_in);

	GVariantBuilder builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);

	GVariant * item = NULL;
	while (g_variant_iter_loop(&iter, "v", &item)) {
		g_variant_builder_add_value(&builder, item);
	}

	return g_variant_builder_end(&builder);
}

/**
 * dbus_test_dbus_mock_object_get_method_calls:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @method: Name of the method
 * @length: (out) (allow-none): Name of the method
 * @error: A possible error 
 *
 * Gets a list of all method calls for a function including the parmeters.
 *
 * Return value: (transfer none): An array of calls with the last item
 *   having a timestamp of 0.  Also length in the optional @len param.
 */
const DbusTestDbusMockCall *
dbus_test_dbus_mock_object_get_method_calls (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * method, guint * length, GError ** error)
{
	/* Default state */
	if (length != NULL) {
		*length = 0;
	}

	/* Check our params */
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), NULL);
	g_return_val_if_fail(obj != NULL, NULL);
	g_return_val_if_fail(method != NULL, NULL);

	if (!is_running(mock)) {
		return NULL;
	}

	g_return_val_if_fail(obj->proxy != NULL, NULL);

	/* Find our method */
	MockObjectMethod * meth = get_obj_method(obj, method);
	if (meth == NULL) {
		g_set_error(error, dbus_mock_quark(), ERROR_METHOD_NOT_FOUND, "Method '%s' not found on object '%s'", method, obj->object_path);
		return NULL;
	}

	/* Clear the current list of calls */
	g_array_set_size(meth->calls, 0);

	GVariant * call_list = NULL;
	dbus_mock_iface_org_freedesktop_dbus_mock_call_get_calls_sync(
		obj->proxy,
		&call_list,
		mock->priv->cancel,
		error);

	if (call_list == NULL) {
		return NULL;
	}

	GVariantIter call_list_itr;
	g_variant_iter_init(&call_list_itr, call_list);

	guint64 timestamp = 0;
	const gchar * name = NULL;
	GVariant * params = NULL;

	while (g_variant_iter_loop(&call_list_itr, "(t&s@av)", &timestamp, &name, &params)) {
		if (g_strcmp0(method, name) != 0) {
			continue;
		}

		DbusTestDbusMockCall callsig = {
			.timestamp = timestamp,
			.name = g_strdup(name),
			.params = g_variant_ref_sink(variant_array_to_tuple(params))
		};

		g_array_append_val(meth->calls, callsig);
	}

	g_variant_unref(call_list);

	if (length != NULL) {
		*length = meth->calls->len;
	}

	return (const DbusTestDbusMockCall *)meth->calls->data;
}

/* Quick helper to get an object property */
static inline MockObjectProperty *
get_obj_property (DbusTestDbusMockObject * obj, const gchar * name)
{
	guint i;
	for (i = 0; i < obj->properties->len; i++) {
		MockObjectProperty * prop = &g_array_index(obj->properties, MockObjectProperty, i);
		if (g_strcmp0(prop->name, name) == 0) {
			return prop;
		}
	}

	return NULL;
}

/**
 * dbus_test_dbus_mock_object_add_property:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @name: Name of the property
 * @type: Type of the property
 * @value: Initial value of the property
 * @error: A possible error
 *
 * Adds a property to the object and sets its initial value.
 *
 * Return value: Whether it was added
 */
gboolean
dbus_test_dbus_mock_object_add_property (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, const GVariantType * type, GVariant * value, G_GNUC_UNUSED GError ** error)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);
	g_return_val_if_fail(type != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);
	g_return_val_if_fail(g_variant_is_of_type(value, type), FALSE);

	/* Check to see if we have the property */
	MockObjectProperty * prop = get_obj_property(obj, name);
	g_return_val_if_fail(prop == NULL, FALSE);

	/* Build a new one */
	MockObjectProperty newprop;
	newprop.name = g_strdup(name);
	newprop.type = g_variant_type_copy(type);
	newprop.value = g_variant_ref_sink(value);

	g_array_append_val(obj->properties, newprop);

	/* If we're not running we can just leave it here */
	if (!is_running(mock)) {
		return TRUE;
	}

	/* TODO: Add to running instance */
	return FALSE;
}

/* Free the data allocated in dbus_test_dbus_mock_object_add_property() */
static void
property_free (gpointer data)
{
	MockObjectProperty * property = (MockObjectProperty *)data;

	g_free(property->name);
	g_variant_type_free(property->type);
	g_variant_unref(property->value);

	/* NOTE: No free of 'data' */
	return;
}


/**
 * dbus_test_dbus_mock_object_update_property:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @name: Name of the property
 * @value: Initial value of the property
 * @error: A possible error
 *
 * Changes the value of a property and will send a signal that it changed
 * depending on the value of @signal.
 *
 * Return value: Whether it was changed
 */
gboolean
dbus_test_dbus_mock_object_update_property (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, GVariant * value, GError ** error)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);

	MockObjectProperty * prop = get_obj_property(obj, name);
	g_return_val_if_fail(prop != NULL, FALSE);

	/* Grab a ref, we'll have to start managing this */
	g_variant_ref_sink(value);
	if (!g_variant_is_of_type(value, prop->type)) {
		g_critical("Property '%s' is not of same value in dbus_test_dbus_mock_object_update_property()", name);
		g_variant_unref(value);
		return FALSE;
	}

	/* Send the update to Dbusmock */
	if (is_running(mock)) {
		GError * local_error = NULL;
		g_dbus_connection_call_sync(mock->priv->bus,
			mock->priv->name,
			obj->object_path,
			"org.freedesktop.DBus.Properties",
			"Set",
			g_variant_new("(ssv)",
				obj->interface,
				name,
				value),
			NULL, /* return */
			G_DBUS_CALL_FLAGS_NO_AUTO_START,
			-1, /* timeout */
			mock->priv->cancel,
			&local_error);

		if (local_error != NULL) {
			g_warning("Unable to update property: %s", local_error->message);
			if (error != NULL) {
				*error = local_error;
			} else {
				g_error_free(local_error);
			}
			g_variant_unref(value);
			return FALSE;
		}
	}

	/* It's updated, let's cache */
	g_variant_unref(prop->value);
	prop->value = value;

	return TRUE;
}

/**
 * dbus_test_dbus_mock_object_emit_signal:
 * @mock: A #DbusTestDbusMock instance
 * @obj: A handle to an object on the mock interface
 * @name: Name of the signal
 * @params: The parameters of the signal as a tuple
 * @values: Values to emit with the signal
 * @error: A possible error
 *
 * Causes the object on the dbus mock to emit a signal with the @params
 * provided.
 *
 * Return value: Whether we were able to request the signal
 *   to be emitted.
 */
gboolean
dbus_test_dbus_mock_object_emit_signal (DbusTestDbusMock * mock, DbusTestDbusMockObject * obj, const gchar * name, const GVariantType * params, GVariant * values, GError ** error)
{
	g_return_val_if_fail(DBUS_TEST_IS_DBUS_MOCK(mock), FALSE);
	g_return_val_if_fail(obj != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);
	if (params == NULL) {
		g_return_val_if_fail(values == NULL, FALSE);
	} else {
		g_return_val_if_fail(values != NULL, FALSE);
	}

	if (!is_running(mock)) {
		return FALSE;
	}

	GVariant * sig_params = NULL;
	if (params == NULL) {
		sig_params = g_variant_new_array(G_VARIANT_TYPE_VARIANT, NULL, 0);
	} else {
		sig_params = values;
	}

	return dbus_mock_iface_org_freedesktop_dbus_mock_call_emit_signal_sync(
		obj->proxy,
		obj->interface,
		name,
		params != NULL ? g_variant_type_peek_string(params) : "",
		sig_params,
		mock->priv->cancel,
		error
	);
}
