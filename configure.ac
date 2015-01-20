
AC_INIT(dbus-test-runner, 15.04.0, ted@canonical.com)
AC_COPYRIGHT([Copyright 2009-2013 Canonical])

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(dbus-test-runner, 15.04.0)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.35.0])
AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

###########################
# Dependencies 
###########################

GLIB_REQUIRED_VERSION=2.34
DBUS_REQUIRED_VERSION=0.76
GIO_REQUIRED_VERSION=2.30

PKG_CHECK_MODULES(DBUS_TEST_RUNNER, glib-2.0 >= $GLIB_REQUIRED_VERSION
                                    gio-2.0 >= $GIO_REQUIRED_VERSION
                                    gio-unix-2.0 >= $GIO_REQUIRED_VERSION
                                    dbus-glib-1 >= $DBUS_REQUIRED_VERSION)
AC_SUBST(DBUS_TEST_RUNNER_CFLAGS)
AC_SUBST(DBUS_TEST_RUNNER_LIBS)


###########################
# Internationalization
###########################

GETTEXT_PACKAGE=dbus-test-runner
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the default gettext domain])

AM_GLIB_GNU_GETTEXT

###########################
# gcov coverage reporting
###########################

m4_include([m4/gcov.m4])
AC_TDD_GCOV
AM_CONDITIONAL([HAVE_GCOV], [test "x$ac_cv_check_gcov" = xyes])
AM_CONDITIONAL([HAVE_LCOV], [test "x$ac_cv_check_lcov" = xyes])
AM_CONDITIONAL([HAVE_GCOVR], [test "x$ac_cv_check_gcovr" = xyes])
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_CXXFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

###########################
# Enable bustle tests
###########################

AC_CHECK_PROG([have_bustle], [bustle], [yes],,,)
AM_CONDITIONAL([TEST_BUSTLE], [test "x$ac_cv_prog_have_bustle" = xyes])

###########################
# Files
###########################

AC_OUTPUT([
Makefile
po/Makefile.in
libdbustest/Makefile
src/Makefile
data/Makefile
tests/Makefile
])

###########################
# Results
###########################

AC_MSG_NOTICE([

dbus-test-runner Configuration:

	Prefix:                 $prefix

	Coverage reporting:     $use_gcov
])

