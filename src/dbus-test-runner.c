/*
Copyright 2010 Canonical Ltd.

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


#include <glib.h>
#include <gio/gio.h>

#include <libdbustest/dbus-test.h>

static DbusTestServiceBus bus_type = DBUS_TEST_SERVICE_BUS_SESSION;
static gint max_wait = 60;
static gboolean keep_env = FALSE;
static DbusTestProcess * last_task = NULL;
static DbusTestService * service = NULL;
static gboolean timeout = FALSE;

#define NAME_SET "dbus-test-runner-name-set"

static gboolean
option_bus_type (G_GNUC_UNUSED const gchar * arg, const gchar * value, G_GNUC_UNUSED gpointer data, GError ** error)
{
	if (bus_type != DBUS_TEST_SERVICE_BUS_SESSION) {
		g_set_error_literal(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Bus type set more than once");
		return TRUE;
	}

	if (g_strcmp0(value, "session") == 0) {
		bus_type = DBUS_TEST_SERVICE_BUS_SESSION;
	} else if (g_strcmp0(value, "system") == 0) {
		bus_type = DBUS_TEST_SERVICE_BUS_SYSTEM;
	} else if (g_strcmp0(value, "both") == 0) {
		bus_type = DBUS_TEST_SERVICE_BUS_BOTH;
	} else {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Bus type '%s' unknown", value);
	}

	return TRUE;
}

static gboolean
option_task (G_GNUC_UNUSED const gchar * arg, const gchar * value, G_GNUC_UNUSED gpointer data, G_GNUC_UNUSED GError ** error)
{
	if (last_task != NULL) {
		g_object_unref(last_task);
		last_task = NULL;
	}

	last_task = dbus_test_process_new(value);
	dbus_test_service_add_task(service, DBUS_TEST_TASK(last_task));
	return TRUE;
}

static gboolean
option_taskname (G_GNUC_UNUSED const gchar * arg, const gchar * value, G_GNUC_UNUSED gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put the name %s on.", value);
		return FALSE;
	}

	if (g_object_get_data(G_OBJECT(last_task), NAME_SET)) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task already has the name %s.  Asked to put %s on it.", dbus_test_task_get_name(DBUS_TEST_TASK(last_task)), value);
		return FALSE;
	}

	g_object_set_data(G_OBJECT(last_task), NAME_SET, GINT_TO_POINTER(TRUE));
	dbus_test_task_set_name(DBUS_TEST_TASK(last_task), value);
	return TRUE;
}

static gboolean
option_taskbus (G_GNUC_UNUSED const gchar * arg, const gchar * value, G_GNUC_UNUSED gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put the name %s on.", value);
		return FALSE;
	}

	if (g_strcmp0(value, "session") == 0) {
		dbus_test_task_set_bus(DBUS_TEST_TASK(last_task), DBUS_TEST_SERVICE_BUS_SESSION);
	} else if (g_strcmp0(value, "system") == 0) {
		dbus_test_task_set_bus(DBUS_TEST_TASK(last_task), DBUS_TEST_SERVICE_BUS_SYSTEM);
	} else if (g_strcmp0(value, "both") == 0) {
		dbus_test_task_set_bus(DBUS_TEST_TASK(last_task), DBUS_TEST_SERVICE_BUS_BOTH);
	} else {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Bus type '%s' unknown", value);
	}

	return TRUE;
}

static gboolean
option_complete (G_GNUC_UNUSED const gchar * arg, G_GNUC_UNUSED const gchar * value, G_GNUC_UNUSED gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put adjust return on.");
		return FALSE;
	}

	if (dbus_test_task_get_wait_finished(DBUS_TEST_TASK(last_task))) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task has already be setup to wait until finished.");
		return FALSE;
	}

	dbus_test_task_set_wait_finished(DBUS_TEST_TASK(last_task), TRUE);
	return TRUE;
}

static gboolean
option_noreturn (G_GNUC_UNUSED const gchar * arg, G_GNUC_UNUSED const gchar * value, G_GNUC_UNUSED gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put adjust return on.");
		return FALSE;
	}

	if (dbus_test_task_get_return(DBUS_TEST_TASK(last_task)) != DBUS_TEST_TASK_RETURN_NORMAL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task return type has already been modified.");
		return FALSE;
	}

	dbus_test_task_set_return(DBUS_TEST_TASK(last_task), DBUS_TEST_TASK_RETURN_IGNORE);
	return TRUE;
}

static gboolean
option_invert (G_GNUC_UNUSED const gchar * arg, G_GNUC_UNUSED const gchar * value, G_GNUC_UNUSED gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put adjust return on.");
		return FALSE;
	}

	if (dbus_test_task_get_return(DBUS_TEST_TASK(last_task)) != DBUS_TEST_TASK_RETURN_NORMAL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task return type has already been modified.");
		return FALSE;
	}

	dbus_test_task_set_return(DBUS_TEST_TASK(last_task), DBUS_TEST_TASK_RETURN_INVERT);
	return TRUE;
}

static gboolean
option_param (G_GNUC_UNUSED const gchar * arg, const gchar * value, G_GNUC_UNUSED gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put adjust return on.");
		return FALSE;
	}

	dbus_test_process_append_param(last_task, value);
	return TRUE;
}

static gboolean
option_wait (G_GNUC_UNUSED const gchar * arg, const gchar * value, G_GNUC_UNUSED gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to add a wait on %s for.", value);
		return FALSE;
	}

	if (dbus_test_task_get_wait_for(DBUS_TEST_TASK(last_task)) != NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task is already waiting for %s.  Asked to wait for %s", dbus_test_task_get_wait_for(DBUS_TEST_TASK(last_task)), value);
		return FALSE;
	}

	dbus_test_task_set_wait_for(DBUS_TEST_TASK(last_task), value);
	return TRUE;
}

static gboolean
max_wait_hit (G_GNUC_UNUSED gpointer user_data)
{
	g_warning("Timing out at maximum wait of %d seconds.", max_wait);
	dbus_test_service_stop(service);
	timeout = TRUE;
	return FALSE;
}

static gchar * dbus_configfile = NULL;
static gchar * dbus_daemon = NULL;
static gchar * bustle_cmd = NULL;
static gchar * bustle_datafile = NULL;

static GOptionEntry general_options[] = {
	{"dbus-daemon",  0,     0,                       G_OPTION_ARG_FILENAME,  &dbus_daemon,     "Path to the DBus deamon to use.  Defaults to 'dbus-daemon'.", "executable"},
	{"dbus-config",  'd',   0,                       G_OPTION_ARG_FILENAME,  &dbus_configfile, "Configuration file for newly created DBus server.  Defaults to '" DEFAULT_SESSION_CONF "'.", "config_file"},
	{"bustle-monitor", 0,   0,                       G_OPTION_ARG_FILENAME,  &bustle_cmd,      "Path to the Bustle DBus Monitor to use.  Defaults to 'bustle-dbus-monitor'.", "executable"},
	{"bustle-data",  'b',   0,                       G_OPTION_ARG_FILENAME,  &bustle_datafile, "A file to write out data from the bustle logger to.", "data_file"},
	{"max-wait",     'm',   0,                       G_OPTION_ARG_INT,       &max_wait,        "The maximum amount of time the test runner will wait for the test to complete.  Default is 30 seconds.", "seconds"},
	{"keep-env",     0,     0,                       G_OPTION_ARG_NONE,      &keep_env,        "Whether to propagate the execution environment to the dbus-server and all the services activated by it.  By default the environment is cleared.", NULL },
	{"bus-type",     0,     0,                       G_OPTION_ARG_CALLBACK,  option_bus_type,  "Configures which buses are represented by the tool to the tasks. Default: session", "{session|system|both}" },
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

static GOptionEntry task_options[] = {
	{"task",          't',  G_OPTION_FLAG_FILENAME,   G_OPTION_ARG_CALLBACK,  option_task,     "Defines a new task to run under our private DBus session.", "executable"},
	{"task-name",     'n',  0,                        G_OPTION_ARG_CALLBACK,  option_taskname, "A string to label output from the previously defined task.  Defaults to taskN.", "name"},
	{"task-bus",      0,    0,                        G_OPTION_ARG_CALLBACK,  option_taskbus,  "Configures which bus the task expects to connect to. Default: both", "{session|system|both}"},
	{"ignore-return", 'r',  G_OPTION_FLAG_NO_ARG,     G_OPTION_ARG_CALLBACK,  option_noreturn, "Do not use the return value of the task to calculate whether the test passes or fails.", NULL},
	{"invert-return", 'i',  G_OPTION_FLAG_NO_ARG,     G_OPTION_ARG_CALLBACK,  option_invert,   "Invert the return value of the task before calculating whether the test passes or fails.", NULL},
	{"parameter",     'p',  0,                        G_OPTION_ARG_CALLBACK,  option_param,    "Add a parameter to the call of this utility.  May be called as many times as you'd like.", NULL},
	{"wait-for",      'f',  0,                        G_OPTION_ARG_CALLBACK,  option_wait,     "A dbus-name that should appear on the bus before this task is started", "dbus-name"},
	{"wait-until-complete", 'c', G_OPTION_FLAG_NO_ARG,G_OPTION_ARG_CALLBACK,  option_complete, "Signal that we should wait until this task exits even if we don't need the return value", NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

int
main (int argc, char * argv[])
{
	GError * error = NULL;
	GOptionContext * context;

#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif

	service = dbus_test_service_new(NULL);

	context = g_option_context_new("- run multiple tasks under an independent DBus session bus");

	g_option_context_add_main_entries(context, general_options, "dbus-runner");

	GOptionGroup * taskgroup = g_option_group_new("task-control", "Task control options", "Options that are used to control how the task is handled by the test runner.", NULL, NULL);
	g_option_group_add_entries(taskgroup, task_options);
	g_option_context_add_group(context, taskgroup);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		g_error_free(error);
		return 1;
	}

	dbus_test_service_set_bus(service, bus_type);

	if (dbus_daemon != NULL) {
		dbus_test_service_set_daemon(service, dbus_daemon);
	}

	if (dbus_configfile != NULL) {
		dbus_test_service_set_conf_file(service, dbus_configfile);
	}

	if (bustle_datafile != NULL) {
		DbusTestBustle * bustler = dbus_test_bustle_new(bustle_datafile);
		/* We want to ensure that bustle captures all the data so start it first */
		dbus_test_service_add_task_with_priority(service, DBUS_TEST_TASK(bustler), DBUS_TEST_SERVICE_PRIORITY_FIRST);

		if (bustle_cmd != NULL) {
			dbus_test_bustle_set_executable(bustler, bustle_cmd);
		}

		g_object_unref(bustler);
	}

	if (max_wait > 0) {
		g_timeout_add_seconds(max_wait, max_wait_hit, NULL);
	}

	dbus_test_service_set_keep_environment(service, keep_env);

	/* These should all be in the service now */
	if (last_task != NULL) {
		g_object_unref(last_task);
		last_task = NULL;
	} else {
		g_critical("No tasks assigned");
		g_object_unref(service);
		return -1;
	}

	gint service_status = dbus_test_service_run(service);
	g_object_unref(service);

	if (timeout) {
		return -1;
	} else {
		return service_status;
	}
}
