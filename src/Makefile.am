
bin_PROGRAMS = dbus-test-runner

dbus_test_runner_SOURCES = dbus-test-runner.c
dbus_test_runner_CFLAGS  = $(DBUS_TEST_RUNNER_CFLAGS) -DDEFAULT_SESSION_CONF="\"$(sharedir)/dbus-test-runner/session.conf\""
dbus_test_runner_LDADD   = $(DBUS_TEST_RUNNER_LIBS)
