
lib_LTLIBRARIES = libdbustest.la

ABI_VERSION = 1
API_VERSION = 1

libdbustestincludedir=$(includedir)/libdbustest-$(API_VERSION)/libdbustest
libdbustestinclude_HEADERS = \
	bustle.h \
	dbus-test.h \
	process.h \
	service.h \
	task.h

libdbustest_la_SOURCES = \
	bustle.c \
	bustle.h \
	dbus-test.h \
	process.c \
	process.h \
	service.c \
	service.h \
	task.c \
	task.h

libdbustest_la_CFLAGS = \
	$(DBUS_TEST_RUNNER_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-I$(top_srcdir) \
	-DDEFAULT_SESSION_CONF="\"$(datadir)/dbus-test-runner/session.conf\"" \
	-DG_LOG_DOMAIN=\"libdbustest\" \
	-Wall -Werror -Wextra

libdbustest_la_LIBADD = \
	$(DBUS_TEST_RUNNER_LIBS)

libdbustest_la_LDFLAGS = \
	$(DBUS_TEST_RUNNER_LDFLAGS) \
	$(COVERAGE_LDFLAGS) \
	-version-info $(ABI_VERSION):0:0 \
	-no-undefined \
	-export-symbols-regex "^[^_].*"

EXTRA_DIST = \
	dbustest.pc.in

pkgconfig_DATA = dbustest-$(API_VERSION).pc
pkgconfigdir = $(libdir)/pkgconfig

%-$(API_VERSION).pc: %.pc.in
	sed \
		-e "s:\@apiversion\@:$(API_VERSION):" \
		-e "s:\@libdir\@:$(libdir):" \
		-e "s:\@includedir\@:$(includedir):" \
		-e "s:\@VERSION\@:$(VERSION):" \
		$< > $@

DISTCLEANFILES = \
	dbustest-$(API_VERSION).pc
