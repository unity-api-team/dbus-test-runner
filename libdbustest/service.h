/*
Copyright 2012 Canonical Ltd.

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

#ifndef __DBUS_TEST_SERVICE_H__
#define __DBUS_TEST_SERVICE_H__

#ifndef __DBUS_TEST_TOP_LEVEL__
#error "Please include #include <libdbustest/dbus-test.h> only"
#endif

#include <glib-object.h>

#include "task.h"

G_BEGIN_DECLS

#define DBUS_TEST_TYPE_SERVICE            (dbus_test_service_get_type ())
#define DBUS_TEST_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUS_TEST_TYPE_SERVICE, DbusTestService))
#define DBUS_TEST_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_TEST_TYPE_SERVICE, DbusTestServiceClass))
#define DBUS_TEST_IS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUS_TEST_TYPE_SERVICE))
#define DBUS_TEST_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_TEST_TYPE_SERVICE))
#define DBUS_TEST_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_TEST_TYPE_SERVICE, DbusTestServiceClass))

typedef struct _DbusTestService         DbusTestService;
typedef struct _DbusTestServiceClass    DbusTestServiceClass;
typedef struct _DbusTestServicePrivate  DbusTestServicePrivate;

struct _DbusTestServiceClass {
	GObjectClass parent_class;
};

struct _DbusTestService {
	GObject parent;
	DbusTestServicePrivate * priv;
};

typedef enum
{
	DBUS_TEST_SERVICE_PRIORITY_FIRST,
	DBUS_TEST_SERVICE_PRIORITY_NORMAL,
	DBUS_TEST_SERVICE_PRIORITY_LAST
} DbusTestServicePriority;

typedef enum
{
	DBUS_TEST_SERVICE_BUS_SESSION,
	DBUS_TEST_SERVICE_BUS_SYSTEM,
	DBUS_TEST_SERVICE_BUS_BOTH
} DbusTestServiceBus;

GType dbus_test_service_get_type (void);
DbusTestService * dbus_test_service_new (const gchar * address);
void dbus_test_service_start_tasks (DbusTestService * service);
int dbus_test_service_run (DbusTestService * service);
void dbus_test_service_stop (DbusTestService * service);

void dbus_test_service_add_task (DbusTestService * service, DbusTestTask * task);
void dbus_test_service_add_task_with_priority (DbusTestService * service, DbusTestTask * task, DbusTestServicePriority prio);
gboolean dbus_test_service_remove_task (DbusTestService * service, DbusTestTask * task);

void dbus_test_service_set_daemon (DbusTestService * service, const gchar * daemon);
void dbus_test_service_set_conf_file (DbusTestService * service, const gchar * conffile);
void dbus_test_service_set_keep_environment (DbusTestService * service, gboolean keep_env);
void dbus_test_service_set_bus (DbusTestService * service, DbusTestServiceBus bus);

G_END_DECLS

#endif
