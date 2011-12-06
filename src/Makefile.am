
bin_PROGRAMS = dbus-test-runner

dbus_test_runner_SOURCES = dbus-test-runner.c
dbus_test_runner_CFLAGS  = $(DBUS_TEST_RUNNER_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DDEFAULT_SESSION_CONF="\"$(datadir)/dbus-test-runner/session.conf\"" \
	-Wall -Werror
dbus_test_runner_LDADD   = $(DBUS_TEST_RUNNER_LIBS)
dbus_test_runner_LDFLAGS = $(COVERAGE_LDFLAGS)
