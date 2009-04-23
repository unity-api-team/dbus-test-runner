
AC_INIT(dbus-test-runner, 0.0.1, ted@canonical.com)
AC_COPYRIGHT([Copyright 2009 Canonical])

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(dbus-test-runner, 0.0.1)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.35.0])
AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

###########################
# Internationalization
###########################

GETTEXT_PACKAGE=dbus-test-runner
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the default gettext domain])

AM_GLIB_GNU_GETTEXT

###########################
# Files
###########################

AC_OUTPUT([
Makefile
po/Makefile.in
src/Makefile
data/Makefile
])

###########################
# Results
###########################

AC_MSG_NOTICE([

dbus-test-runner Configuration:

	Prefix:                 $prefix
])

