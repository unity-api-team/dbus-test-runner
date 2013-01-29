#ifndef __DBUS_TEST_WATCHDOG_H__
#define __DBUS_TEST_WATCHDOG_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DBUS_TEST_TYPE_WATCHDOG            (dbus_test_watchdog_get_type ())
#define DBUS_TEST_WATCHDOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUS_TEST_TYPE_WATCHDOG, DbusTestWatchdog))
#define DBUS_TEST_WATCHDOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_TEST_TYPE_WATCHDOG, DbusTestWatchdogClass))
#define DBUS_TEST_IS_WATCHDOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUS_TEST_TYPE_WATCHDOG))
#define DBUS_TEST_IS_WATCHDOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_TEST_TYPE_WATCHDOG))
#define DBUS_TEST_WATCHDOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_TEST_TYPE_WATCHDOG, DbusTestWatchdogClass))

typedef struct _DbusTestWatchdog         DbusTestWatchdog;
typedef struct _DbusTestWatchdogClass    DbusTestWatchdogClass;
typedef struct _DbusTestWatchdogPrivate  DbusTestWatchdogPrivate;

struct _DbusTestWatchdogClass {
	GObjectClass parent_class;
};

struct _DbusTestWatchdog {
	GObject parent;
	DbusTestWatchdogPrivate * priv;
};

GType dbus_test_watchdog_get_type        (void);
void  dbus_test_watchdog_add_pid         (DbusTestWatchdog * watchdog,
                                          GPid                pid);
void  dbus_test_watchdog_ping            (DbusTestWatchdog * watchdog);

G_END_DECLS

#endif
