
#include <glib.h>
#include <libdbustest/dbus-test.h>

void
test_env_var (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	g_unsetenv("DBUS_SESSION_BUS_ADDRESS");
	dbus_test_service_start_tasks(service);
	g_assert(g_getenv("DBUS_SESSION_BUS_ADDRESS") != NULL);

	g_object_unref(service);
	return;
}

void
test_task_start (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestTask * task = dbus_test_task_new();
	g_assert(task != NULL);

	dbus_test_service_add_task(service, task);
	dbus_test_service_start_tasks(service);

	g_assert(dbus_test_task_get_state(task) == DBUS_TEST_TASK_STATE_FINISHED);

	g_object_unref(task);
	g_object_unref(service);

	return;
}

void
test_task_wait (void)
{
	DbusTestService * service = dbus_test_service_new(NULL);
	g_assert(service != NULL);

	dbus_test_service_set_conf_file(service, SESSION_CONF);

	DbusTestTask * task = dbus_test_task_new();
	g_assert(task != NULL);

	dbus_test_task_set_wait_for(task, "org.test.name");

	dbus_test_service_add_task(service, task);

	DbusTestProcess * proc = dbus_test_process_new(GETNAME_PATH);
	g_assert(proc != NULL);
	g_assert(DBUS_TEST_IS_TASK(proc));
	dbus_test_process_append_param(proc, "org.test.name");

	dbus_test_service_add_task_with_priority(service, DBUS_TEST_TASK(proc), DBUS_TEST_SERVICE_PRIORITY_LAST);

	dbus_test_service_start_tasks(service);

	g_assert(dbus_test_task_get_state(task) == DBUS_TEST_TASK_STATE_FINISHED);

	g_object_unref(task);
	g_object_unref(service);

	return;
}

/* Build our test suite */
void
test_libdbustest_suite (void)
{
	g_test_add_func ("/libdbustest/env_var",    test_env_var);
	g_test_add_func ("/libdbustest/task_start", test_task_start);
	g_test_add_func ("/libdbustest/task_wait",  test_task_wait);

	return;
}

int
main (int argc, char ** argv)
{
#ifndef GLIB_VERSION_2_36
	g_type_init (); 
#endif

	g_test_init (&argc, &argv, NULL);

	test_libdbustest_suite();

	g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);

	return g_test_run();
}
