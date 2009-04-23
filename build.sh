#!/bin/sh

gcc -o dbus-test-runner `pkg-config --cflags --libs glib-2.0 gobject-2.0` dbus-test-runner.c -Wall -Werror
