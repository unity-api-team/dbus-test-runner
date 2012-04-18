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

static gint max_wait = 30;
static DbusTestProcess * last_task = NULL;
static DbusTestService * service = NULL;


static gboolean
option_task (const gchar * arg, const gchar * value, gpointer data, GError ** error)
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
option_taskname (const gchar * arg, const gchar * value, gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put the name %s on.", value);
		return FALSE;
	}

	if (dbus_test_task_get_name(DBUS_TEST_TASK(last_task)) != NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Task already has the name %s.  Asked to put %s on it.", dbus_test_task_get_name(DBUS_TEST_TASK(last_task)), value);
		return FALSE;
	}

	dbus_test_task_set_name(DBUS_TEST_TASK(last_task), value);
	return TRUE;
}

static gboolean
option_noreturn (const gchar * arg, const gchar * value, gpointer data, GError ** error)
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
option_invert (const gchar * arg, const gchar * value, gpointer data, GError ** error)
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
option_param (const gchar * arg, const gchar * value, gpointer data, GError ** error)
{
	if (last_task == NULL) {
		g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "No task to put adjust return on.");
		return FALSE;
	}

	dbus_test_process_append_param(last_task, value);
	return TRUE;
}

static gboolean
option_wait (const gchar * arg, const gchar * value, gpointer data, GError ** error)
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
max_wait_hit (gpointer user_data)
{
	g_warning("Timing out at maximum wait of %d seconds.", max_wait);
	//g_main_loop_quit(global_mainloop);	
	//global_success = FALSE;
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
	{NULL}
};

static GOptionEntry task_options[] = {
	{"task",          't',  G_OPTION_FLAG_FILENAME,   G_OPTION_ARG_CALLBACK,  option_task,     "Defines a new task to run under our private DBus session.", "executable"},
	{"task-name",     'n',  0,                        G_OPTION_ARG_CALLBACK,  option_taskname, "A string to label output from the previously defined task.  Defaults to taskN.", "name"},
	{"ignore-return", 'r',  G_OPTION_FLAG_NO_ARG,     G_OPTION_ARG_CALLBACK,  option_noreturn, "Do not use the return value of the task to calculate whether the test passes or fails.", NULL},
	{"invert-return", 'i',  G_OPTION_FLAG_NO_ARG,     G_OPTION_ARG_CALLBACK,  option_invert,   "Invert the return value of the task before calculating whether the test passes or fails.", NULL},
	{"parameter",     'p',  0,                        G_OPTION_ARG_CALLBACK,  option_param,    "Add a parameter to the call of this utility.  May be called as many times as you'd like.", NULL},
	{"wait-for",      'f',  0,                        G_OPTION_ARG_CALLBACK,  option_wait,     "A dbus-name that should appear on the bus before this task is started", "dbus-name"},
	{NULL}
};

int
main (int argc, char * argv[])
{
	GError * error = NULL;
	GOptionContext * context;

	g_type_init();

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

	if (dbus_daemon != NULL) {
		dbus_test_service_set_daemon(service, dbus_daemon);
	}

	if (dbus_configfile != NULL) {
		dbus_test_service_set_conf_file(service, dbus_configfile);
	}

	if (max_wait > 0) {
		g_timeout_add_seconds(max_wait, max_wait_hit, NULL);
	}

	/* These should all be in the service now */
	if (last_task != NULL) {
		g_object_unref(last_task);
		last_task = NULL;
	}

	return dbus_test_service_run(service);
}
