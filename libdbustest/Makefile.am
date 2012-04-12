
lib_LTLIBRARIES = libdbustest.la

ABI_VERSION = 1
API_VERSION = 1

libdbustestincludedir=$(includedir)/libdbustest-$(API_VERSION)/libdbustest
libdbustestinclude_HEADERS = \
	dbus-test.h \
	service.h \
	task.h

libdbustest_la_SOURCES = \
	dbus-test.h \
	service.c \
	service.h \
	task.c \
	task.h

libdbustest_la_CFLAGS = \
	$(DBUS_TEST_RUNNER_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libdbustest\" \
	-Wall -Werror

libdbustest_la_LIBADD = \
	$(DBUS_TEST_RUNNER_LIBS)

libdbustest_la_LDFLAGS = \
	$(DBUS_TEST_RUNNER_LDFLAGS) \
	$(COVERAGE_LDFLAGS) \
	-version-info $(ABI_VERSION):0:0 \
	-no-undefined \
	-export-symbols-regex "^[^_].*"
