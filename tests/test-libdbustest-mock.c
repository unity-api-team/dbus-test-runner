
#include <glib.h>
#include <gio/gio.h>
#include <libdbustest/dbus-test.h>

/* Timeout on our loop */
static gboolean
timeout_quit_func (gpointer user_data)
{
	GMainLoop * loop = (GMainLoop *)user_data;
	g_main_loop_quit(loop);
	return FALSE;
}


static void
process_mainloop (const guint delay)
{
	GMainLoop * temploop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (delay, timeout_quit_func, temploop);
	g_main_loop_run (temploop);
	g_main_loop_unref (temploop);
}

#define SESSION_MAX_WAIT 100

/*
* Waiting until the session bus shuts down
*/
GDBusConnection * wait_for_close_ptr = NULL;
static void
wait_for_connection_close (GDBusConnection *connection)
{
	wait_for_close_ptr = connection;
	g_object_add_weak_pointer(G_OBJECT(connection), (gpointer) &wait_for_close_ptr);

	g_object_unref (connection);

	int wait_count;
	for (wait_count = 0; wait_for_close_ptr != NULL && wait_count < SESSION_MAX_WAIT; wait_count++)
	{
		process_mainloop(200);
	}

	g_assert(wait_count != SESSION_MAX_WAIT);
}

static void
signal_emitted (GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * signal, GVariant * params, gpointer user_data)
{
	guint * count = (guint *)user_data;
	(*count)++;
}

void
test_basic (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestDbusMock * mock = dbus_test_dbus_mock_new("foo.test");
	g_assert(mock != NULL);

	dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
	dbus_test_service_start_tasks(service);

	gchar * dbusname = NULL;
	g_object_get(mock, "dbus-name", &dbusname, NULL);
	g_assert(g_strcmp0(dbusname, "foo.test") == 0);
	g_free(dbusname);

	gchar * exec = NULL;
	g_object_get(mock, "executable", &exec, NULL);
	g_assert(g_strcmp0(exec, "python3") == 0);
	g_free(exec);

	g_assert(dbus_test_task_get_state(DBUS_TEST_TASK(mock)) == DBUS_TEST_TASK_STATE_RUNNING);

	/* check setup */
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_dbus_connection_set_exit_on_close(bus, FALSE);

	/* Ensure we can get an object */
	DbusTestDbusMockObject * obj = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.interface", NULL);
	g_assert(obj != NULL);

	DbusTestDbusMockObject * newobj = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.interface", NULL);
	g_assert(obj == newobj);

	g_object_unref(mock);
	g_object_unref(service);

	wait_for_connection_close(bus);

	return;
}

void
test_properties (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestDbusMock * mock = dbus_test_dbus_mock_new("foo.test");
	g_assert(mock != NULL);

	DbusTestDbusMockObject * obj = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.interface", NULL);
	/* String property */
	g_assert(dbus_test_dbus_mock_object_add_property(mock, obj, "prop1", G_VARIANT_TYPE_STRING, g_variant_new_string("test"), NULL));
	/* Invalid type */
	g_assert(!dbus_test_dbus_mock_object_add_property(mock, obj, "prop2", G_VARIANT_TYPE_STRING, g_variant_new_uint32(5), NULL));
	/* Complex type */
	g_assert(dbus_test_dbus_mock_object_add_property(mock, obj, "prop3", G_VARIANT_TYPE("(sssss)"), g_variant_new("(sssss)", "a", "b", "c", "d", "e"), NULL));

	dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
	dbus_test_service_start_tasks(service);

	g_assert(dbus_test_task_get_state(DBUS_TEST_TASK(mock)) == DBUS_TEST_TASK_STATE_RUNNING);

	/* check setup */
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_dbus_connection_set_exit_on_close(bus, FALSE);

	GVariant * propret = NULL;
	GVariant * testvar = NULL;
	GError * error = NULL;

	/* Check prop1 */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"org.freedesktop.DBus.Properties",
		"Get",
		g_variant_new("(ss)", "foo.test.interface", "prop1"),
		G_VARIANT_TYPE("(v)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to get property: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	testvar = g_variant_new_variant(g_variant_new_string("test"));
	testvar = g_variant_new_tuple(&testvar, 1);
	g_variant_ref_sink(testvar);
	g_assert(g_variant_equal(propret, testvar));
	g_variant_unref(testvar);

	g_variant_unref(propret);

	/* Check lack of prop2 */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"org.freedesktop.DBus.Properties",
		"Get",
		g_variant_new("(ss)", "foo.test.interface", "prop2"),
		G_VARIANT_TYPE("(v)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	g_assert(error != NULL);
	g_error_free(error); error = NULL;
	g_assert(propret == NULL);

	/* Check prop3 */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"org.freedesktop.DBus.Properties",
		"Get",
		g_variant_new("(ss)", "foo.test.interface", "prop3"),
		G_VARIANT_TYPE("(v)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to get property: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	testvar = g_variant_new_variant(g_variant_new("(sssss)", "a", "b", "c", "d", "e"));
	testvar = g_variant_new_tuple(&testvar, 1);
	g_variant_ref_sink(testvar);
	g_assert(g_variant_equal(propret, testvar));
	g_variant_unref(testvar);

	g_variant_unref(propret);

	/* Update the properties */
	g_assert(dbus_test_dbus_mock_object_update_property(mock, obj, "prop1", g_variant_new_string("test-update"), NULL));

	/* Check prop1 again */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"org.freedesktop.DBus.Properties",
		"Get",
		g_variant_new("(ss)", "foo.test.interface", "prop1"),
		G_VARIANT_TYPE("(v)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to get property: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	testvar = g_variant_new_variant(g_variant_new_string("test-update"));
	testvar = g_variant_new_tuple(&testvar, 1);
	g_variant_ref_sink(testvar);
	g_assert(g_variant_equal(propret, testvar));
	g_variant_unref(testvar);

	g_variant_unref(propret);

	/* Update the property wrong */
	g_assert(!dbus_test_dbus_mock_object_update_property(mock, obj, "prop1", g_variant_new_uint32(5), NULL));

	/* Check prop1 again */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"org.freedesktop.DBus.Properties",
		"Get",
		g_variant_new("(ss)", "foo.test.interface", "prop1"),
		G_VARIANT_TYPE("(v)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to get property: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	testvar = g_variant_new_variant(g_variant_new_string("test-update"));
	testvar = g_variant_new_tuple(&testvar, 1);
	g_variant_ref_sink(testvar);
	g_assert(g_variant_equal(propret, testvar));
	g_variant_unref(testvar);

	g_variant_unref(propret);

	/* Clean up */
	g_object_unref(mock);
	g_object_unref(service);

	wait_for_connection_close(bus);

	return;
}

void
test_methods (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestDbusMock * mock = dbus_test_dbus_mock_new("foo.test");
	g_assert(mock != NULL);

	DbusTestDbusMockObject * obj = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.interface", NULL);
	dbus_test_dbus_mock_object_add_method(mock, obj,
		"method1",
		G_VARIANT_TYPE("(ss)"),
		G_VARIANT_TYPE("s"),
		"ret = 'test'",
		NULL);

	dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
	dbus_test_service_start_tasks(service);

	g_assert(dbus_test_task_get_state(DBUS_TEST_TASK(mock)) == DBUS_TEST_TASK_STATE_RUNNING);

	/* Check 'em */
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_dbus_connection_set_exit_on_close(bus, FALSE);

	GVariant * propret = NULL;
	GVariant * testvar = NULL;
	GError * error = NULL;

	/* Check prop1 */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"foo.test.interface",
		"method1",
		g_variant_new("(ss)", "testin", "moretest"),
		G_VARIANT_TYPE("(s)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to call method1: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	testvar = g_variant_new_string("test");
	testvar = g_variant_new_tuple(&testvar, 1);
	g_variant_ref_sink(testvar);
	g_assert(g_variant_equal(propret, testvar));
	g_variant_unref(testvar);

	g_variant_unref(propret);

	/* Ask DBusMock if it got called */
	g_assert(dbus_test_dbus_mock_object_check_method_call(mock, obj, "method1", NULL, NULL));
	g_assert(dbus_test_dbus_mock_object_check_method_call(mock, obj, "method1", g_variant_new("(ss)", "testin", "moretest"), NULL));

	g_assert(dbus_test_dbus_mock_object_clear_method_calls(mock, obj, NULL));
	g_assert(!dbus_test_dbus_mock_object_check_method_call(mock, obj, "method1", NULL, NULL));

	/* Clean up */
	g_object_unref(mock);
	g_object_unref(service);

	wait_for_connection_close(bus);

	return;
}

static void
signal_abc_emitted (GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * signal, GVariant * params, gpointer user_data)
{
	if (g_variant_equal(params, g_variant_new_parsed("('a', 'b', 'c')"))) {
		guint * count = (guint *)user_data;
		(*count)++;
	}
}

void
test_signals (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestDbusMock * mock = dbus_test_dbus_mock_new("foo.test");
	g_assert(mock != NULL);

	DbusTestDbusMockObject * obj = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.interface", NULL);

	dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
	dbus_test_service_start_tasks(service);

	g_assert(dbus_test_task_get_state(DBUS_TEST_TASK(mock)) == DBUS_TEST_TASK_STATE_RUNNING);

	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_dbus_connection_set_exit_on_close(bus, FALSE);

	guint signal_count = 0;
	g_dbus_connection_signal_subscribe(bus,
		NULL, /* sender */
		"foo.test.interface",
		"testsig",
		"/test",
		NULL, /* arg0 */
		G_DBUS_SIGNAL_FLAGS_NONE,
		signal_emitted,
		&signal_count,
		NULL); /* user data cleanup */

	g_assert(dbus_test_dbus_mock_object_emit_signal(mock, obj, "testsig", NULL, NULL, NULL));

	g_usleep(100000);
	while (g_main_pending())
		g_main_iteration(TRUE);

	g_assert(signal_count == 1);

	/* Signal with params */
	guint signal_abc_count = 0;
	g_dbus_connection_signal_subscribe(bus,
		NULL, /* sender */
		"foo.test.interface",
		"testsig_abc",
		"/test",
		NULL, /* arg0 */
		G_DBUS_SIGNAL_FLAGS_NONE,
		signal_abc_emitted,
		&signal_abc_count,
		NULL); /* user data cleanup */

	g_assert(dbus_test_dbus_mock_object_emit_signal(mock, obj, "testsig_abc", G_VARIANT_TYPE("(sss)"), g_variant_new_parsed("('a', 'b', 'c')"), NULL));

	g_usleep(100000);
	while (g_main_pending())
		g_main_iteration(TRUE);

	g_assert(signal_abc_count == 1);

	/* Clean up */
	g_object_unref(mock);
	g_object_unref(service);

	wait_for_connection_close(bus);

	return;
}

void
test_running (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestDbusMock * mock = dbus_test_dbus_mock_new("foo.test");
	g_assert(mock != NULL);

	/* Startup the mock */
	dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
	dbus_test_service_start_tasks(service);

	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_dbus_connection_set_exit_on_close(bus, FALSE);

	/* Add the object */
	DbusTestDbusMockObject * obj = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.interface", NULL);
	g_assert(obj != NULL);

	g_assert(dbus_test_dbus_mock_object_add_method(mock, obj,
		"method1",
		G_VARIANT_TYPE("s"),
		G_VARIANT_TYPE("s"),
		"ret = 'test'",
		NULL));

	GVariant * propret = NULL;
	GVariant * testvar = NULL;
	GError * error = NULL;

	/* Check method */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"foo.test.interface",
		"method1",
		g_variant_new("(s)", "testin"),
		G_VARIANT_TYPE("(s)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to call method1: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	testvar = g_variant_new_string("test");
	testvar = g_variant_new_tuple(&testvar, 1);
	g_variant_ref_sink(testvar);
	g_assert(g_variant_equal(propret, testvar));
	g_variant_unref(testvar);

	g_variant_unref(propret);

	/* Add a property */
	g_assert(dbus_test_dbus_mock_object_add_property(mock, obj, "prop1", G_VARIANT_TYPE_STRING, g_variant_new_string("test"), NULL));

	/* Check prop1 */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"org.freedesktop.DBus.Properties",
		"Get",
		g_variant_new("(ss)", "foo.test.interface", "prop1"),
		G_VARIANT_TYPE("(v)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to get property: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	testvar = g_variant_new_variant(g_variant_new_string("test"));
	testvar = g_variant_new_tuple(&testvar, 1);
	g_variant_ref_sink(testvar);
	g_assert(g_variant_equal(propret, testvar));
	g_variant_unref(testvar);

	g_variant_unref(propret);

	/* Clean up */
	g_object_unref(mock);
	g_object_unref(service);

	wait_for_connection_close(bus);

	return;
}

void
test_running_system (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);
	dbus_test_service_set_bus(service, DBUS_TEST_SERVICE_BUS_SYSTEM);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestDbusMock * mock = dbus_test_dbus_mock_new("foo.test");
	g_assert(mock != NULL);
	dbus_test_task_set_bus(DBUS_TEST_TASK(mock), DBUS_TEST_SERVICE_BUS_SYSTEM);

	/* Startup the mock */
	dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
	dbus_test_service_start_tasks(service);

	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
	g_dbus_connection_set_exit_on_close(bus, FALSE);

	/* Add the object */
	DbusTestDbusMockObject * obj = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.interface", NULL);
	g_assert(obj != NULL);

	g_assert(dbus_test_dbus_mock_object_add_method(mock, obj,
		"method1",
		G_VARIANT_TYPE("s"),
		G_VARIANT_TYPE("s"),
		"ret = 'test'",
		NULL));

	GVariant * propret = NULL;
	GVariant * testvar = NULL;
	GError * error = NULL;

	/* Check method */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"foo.test.interface",
		"method1",
		g_variant_new("(s)", "testin"),
		G_VARIANT_TYPE("(s)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to call method1: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	testvar = g_variant_new_string("test");
	testvar = g_variant_new_tuple(&testvar, 1);
	g_variant_ref_sink(testvar);
	g_assert(g_variant_equal(propret, testvar));
	g_variant_unref(testvar);

	g_variant_unref(propret);

	/* Clean up */
	g_object_unref(mock);
	g_object_unref(service);

	wait_for_connection_close(bus);
}

void
test_interfaces (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestDbusMock * mock = dbus_test_dbus_mock_new("foo.test");
	g_assert(mock != NULL);

	/* Add the first object */
	DbusTestDbusMockObject * obj1 = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.interface", NULL);
	g_assert(obj1 != NULL);

	g_assert(dbus_test_dbus_mock_object_add_method(mock, obj1,
		"testmethod",
		G_VARIANT_TYPE("s"),
		G_VARIANT_TYPE("s"),
		"ret = 'test'",
		NULL));

	/* Add the second object */
	DbusTestDbusMockObject * obj2 = dbus_test_dbus_mock_get_object(mock, "/test", "foo.test.otherinterface", NULL);
	g_assert(obj2 != NULL);

	g_assert(dbus_test_dbus_mock_object_add_method(mock, obj2,
		"testothermethod",
		G_VARIANT_TYPE("s"),
		G_VARIANT_TYPE("s"),
		"ret = 'test'",
		NULL));

	/* Startup the mock */
	dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
	dbus_test_service_start_tasks(service);

	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_dbus_connection_set_exit_on_close(bus, FALSE);

	/* Vars */
	GVariant * propret = NULL;
	GError * error = NULL;
	guint len = 0;


	/* Check method */
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"foo.test.interface",
		"testmethod",
		g_variant_new("(s)", "testin"),
		G_VARIANT_TYPE("(s)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to call testmethod: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	g_variant_unref(propret);

	g_assert(dbus_test_dbus_mock_object_get_method_calls(mock, obj1, "testmethod", &len, NULL) != NULL);
	g_assert(len == 1);


	/* Check method */
	len = 0;
	propret = g_dbus_connection_call_sync(bus,
		"foo.test",
		"/test",
		"foo.test.otherinterface",
		"testothermethod",
		g_variant_new("(s)", "testin"),
		G_VARIANT_TYPE("(s)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (error != NULL) {
		g_error("Unable to call testothermethod: %s", error->message);
		g_error_free(error);
	}

	g_assert(propret != NULL);
	g_variant_unref(propret);

	g_assert(dbus_test_dbus_mock_object_get_method_calls(mock, obj2, "testothermethod", &len, NULL) != NULL);
	g_assert(len == 1);


	/* Clean up */
	g_object_unref(mock);
	g_object_unref(service);

	wait_for_connection_close(bus);

	return;
}


/* Build our test suite */
void
test_libdbustest_mock_suite (void)
{
	g_test_add_func ("/libdbustest/mock/basic",        test_basic);
	g_test_add_func ("/libdbustest/mock/properties",   test_properties);
	g_test_add_func ("/libdbustest/mock/methods",      test_methods);
	g_test_add_func ("/libdbustest/mock/signals",      test_signals);
	g_test_add_func ("/libdbustest/mock/running",      test_running);
	g_test_add_func ("/libdbustest/mock/running-system", test_running_system);
	g_test_add_func ("/libdbustest/mock/interfaces",   test_interfaces);

	return;
}

int
main (int argc, char ** argv)
{
#ifndef GLIB_VERSION_2_36
	g_type_init (); 
#endif

	g_test_init (&argc, &argv, NULL);

	test_libdbustest_mock_suite();

	g_log_set_always_fatal(G_LOG_LEVEL_ERROR);

	return g_test_run();
}
