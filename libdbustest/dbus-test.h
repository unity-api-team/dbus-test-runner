/*
Copyright 2012 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __DBUS_TEST_H__
#define __DBUS_TEST_H__

#ifdef __DBUS_TEST_TOP_LEVEL__
#error "Please include #include <libdbustest/dbus-test.h> only"
#endif
#define __DBUS_TEST_TOP_LEVEL__ 1


#include <libdbustest/task.h>
#include <libdbustest/service.h>
#include <libdbustest/process.h>
#include <libdbustest/bustle.h>
#include <libdbustest/dbus-mock.h>


#endif /* __DBUS_TEST_H__ */
