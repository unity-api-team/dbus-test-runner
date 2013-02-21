/*
Copyright 2012 Canonical Ltd.

Authors:
    David Barth <david.barth@canonical.com>

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

#ifndef GLIB_COMPAT_H
#define GLIB_COMPAT_H

#if !GLIB_CHECK_VERSION(2, 34, 0)
#define g_clear_pointer(pp, destroy) \
  G_STMT_START {                                                               \
    G_STATIC_ASSERT (sizeof *(pp) == sizeof (gpointer));                       \
    /* Only one access, please */                                              \
    gpointer *_pp = (gpointer *) (pp);                                         \
    gpointer _p;                                                               \
    /* This assignment is needed to avoid a gcc warning */                     \
    GDestroyNotify _destroy = (GDestroyNotify) (destroy);                      \
                                                                               \
    (void) (0 ? (gpointer) *(pp) : 0);                                         \
    do                                                                         \
      _p = g_atomic_pointer_get (_pp);                                         \
    while G_UNLIKELY (!g_atomic_pointer_compare_and_exchange (_pp, _p, NULL)); \
                                                                               \
    if (_p)                                                                    \
      _destroy (_p);                                                           \
  } G_STMT_END
#endif

#endif /* GLIB_COMPAT_H */
