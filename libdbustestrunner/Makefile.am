
lib_LTLIBRARIES = libdbustestrunner.la

ABI_VERSION = 1
API_VERSION = 1

libdbustestrunnerincludedir=$(includedir)/libdbustestrunner-$(API_VERSION)/libdbustestrunner
libdbustestrunnerinclude_HEADERS = \
	dbus-task.h

libdbustestrunner_la_SOURCES = \
	dbus-task.c \
	dbus-task.h

libdbustestrunner_la_CFLAGS = \
	$(DBUS_TEST_RUNNER_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DG_LOG_DOMAIN=\"libdbustestrunner\" \
	-Wall -Werror

libdbustestrunner_la_LIBADD = \
	$(DBUS_TEST_RUNNER_LIBS)

libdbustestrunner_la_LDFLAGS = \
	$(DBUS_TEST_RUNNER_LDFLAGS) \
	$(COVERAGE_LDFLAGS) \
	-version-info $(ABI_VERSION):0:0 \
	-no-undefined \
	-export-symbols-regex "^[^_].*"
