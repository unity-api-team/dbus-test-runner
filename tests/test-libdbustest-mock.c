
#include <glib.h>
#include <libdbustest/dbus-test.h>


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

	g_assert(dbus_test_task_get_state(DBUS_TEST_TASK(mock)) == DBUS_TEST_TASK_STATE_RUNNING);

	g_object_unref(mock);
	g_object_unref(service);

	return;
}


/* Build our test suite */
void
test_libdbustest_mock_suite (void)
{
	g_test_add_func ("/libdbustest/mock/basic",    test_basic);

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

	g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);

	return g_test_run();
}
