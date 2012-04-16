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
typedef enum   _DbusTestServicePriority DbusTestServicePriority;

struct _DbusTestServiceClass {
	GObjectClass parent_class;
};

struct _DbusTestService {
	GObject parent;
	DbusTestServicePrivate * priv;
};

enum _DbusTestServicePriority {
	DBUS_TEST_SERVICE_PRIORITY_FIRST,
	DBUS_TEST_SERVICE_PRIORITY_NORMAL,
	DBUS_TEST_SERVICE_PRIORITY_LAST
};

GType dbus_test_service_get_type (void);
DbusTestService * dbus_test_service_new (const gchar * address);
void dbus_test_service_start_tasks (DbusTestService * service);
int dbus_test_service_run (DbusTestService * service);

void dbus_test_service_add_task (DbusTestService * service, DbusTestTask * task);
void dbus_test_service_add_task_with_priority (DbusTestService * service, DbusTestTask * task, DbusTestServicePriority prio);

G_END_DECLS

#endif
