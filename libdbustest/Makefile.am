
lib_LTLIBRARIES = libdbustest.la

ABI_VERSION = 1
API_VERSION = 1

AM_CFLAGS = -Wall

libdbustestincludedir=$(includedir)/libdbustest-$(API_VERSION)/libdbustest
libdbustestinclude_HEADERS = \
	bustle.h \
	dbus-mock.h \
	dbus-test.h \
	process.h \
	service.h \
	task.h

libdbustest_la_SOURCES = \
	bustle.c \
	bustle.h \
	dbus-mock.h \
	dbus-mock.c \
	dbus-test.h \
	process.c \
	process.h \
	service.c \
	service.h \
	task.c \
	task.h \
	watchdog.c \
	watchdog.h

libdbustest_la_CFLAGS = \
	$(DBUS_TEST_RUNNER_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-I$(top_srcdir) \
	-I$(builddir) \
	-DDEFAULT_SESSION_CONF="\"$(datadir)/dbus-test-runner/session.conf\"" \
	-DDEFAULT_SYSTEM_CONF="\"$(datadir)/dbus-test-runner/system.conf\"" \
	-DBUSTLE_DUAL_MONITOR="\"$(pkgdatadir)/dbus-test-bustle-handler\"" \
	-DWATCHDOG="\"$(pkglibexecdir)/dbus-test-watchdog\"" \
	-DG_LOG_DOMAIN=\"libdbustest\" \
	-Wall -Werror -Wextra

libdbustest_la_LIBADD = \
	libdbustest-generated.la \
	$(DBUS_TEST_RUNNER_LIBS)

libdbustest_la_LDFLAGS = \
	$(DBUS_TEST_RUNNER_LDFLAGS) \
	$(COVERAGE_LDFLAGS) \
	-version-info $(ABI_VERSION):0:0 \
	-no-undefined \
	-export-symbols-regex "^[^_].*"

noinst_LTLIBRARIES = libdbustest-generated.la

libdbustest_generated_la_CFLAGS = \
	$(DBUS_TEST_RUNNER_CFLAGS) \
	-I$(builddir) \
	-DG_LOG_DOMAIN=\"libdbustest\" \
	-Wall -Werror

libdbustest_generated_la_SOURCES = \
	dbus-mock-iface.h \
	dbus-mock-iface.c

dbus-mock-iface.c: dbus-mock-iface.xml
	$(AM_V_GEN) gdbus-codegen \
		--interface-prefix org.freedesktop.DBus.Mock. \
		--generate-c-code dbus-mock-iface \
		--c-namespace _DbusMockIface \
		$^
dbus-mock-iface.h: dbus-mock-iface.c
dbus-mock.c: dbus-mock-iface.h

pkgdata_SCRIPTS = \
	dbus-test-bustle-handler

EXTRA_DIST = \
	dbus-mock-iface.xml \
	dbus-test-bustle-handler \
	dbustest.pc.in

pkgconfig_DATA = dbustest-$(API_VERSION).pc
pkgconfigdir = $(libdir)/pkgconfig

%-$(API_VERSION).pc: %.pc.in
	$(AM_V_GEN) sed \
		-e "s:\@apiversion\@:$(API_VERSION):" \
		-e "s:\@libdir\@:$(libdir):" \
		-e "s:\@includedir\@:$(includedir):" \
		-e "s:\@VERSION\@:$(VERSION):" \
		$< > $@

pkglibexec_PROGRAMS = \
	dbus-test-watchdog

dbus_test_watchdog_SOURCES = \
	leash.c
dbus_test_watchdog_LDADD = \
	$(DBUS_TEST_RUNNER_LIBS)
dbus_test_watchdog_CFLAGS = \
	$(DBUS_TEST_RUNNER_CFLAGS)

DISTCLEANFILES = \
	dbus-mock-iface.c dbus-mock-iface.h \
	dbustest-$(API_VERSION).pc
