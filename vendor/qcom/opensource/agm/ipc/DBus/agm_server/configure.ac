#                                               -*- Autoconf -*-
# configure.ac -- Autoconf script for halinterface
#

# Process this file with autoconf to produce a configure script.

# Requires autoconf tool later than 2.61
AC_PREREQ([2.69])
# Initialize the hal-interface package version 1.0.0
AC_INIT(halinterface,1.0.0)
# Does not strictly follow GNU Coding standards
AM_INIT_AUTOMAKE([foreign])
# Disables auto rebuilding of configure, Makefile.ins
#AM_MAINTAINER_MODE
# defines some macros variable to be included by source
AC_CONFIG_HEADERS([config.h])
# defines some macros variable to be included by source
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

AC_ARG_WITH([glib],
      AC_HELP_STRING([--with-glib],
         [enable glib, Build against glib. Use this when building for HLOS systems which use glib]))

if (test "x${with_glib}" = "xyes"); then
        PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.16, dummy=yes,
                                AC_MSG_ERROR(GThread >= 2.16 is required))
        PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16, dummy=yes,
                                AC_MSG_ERROR(GLib >= 2.16 is required))
        GLIB_CFLAGS="$GLIB_CFLAGS $GTHREAD_CFLAGS"
        GLIB_LIBS="$GLIB_LIBS $GTHREAD_LIBS"

        AC_SUBST(GLIB_CFLAGS)
        AC_SUBST(GLIB_LIBS)
fi

AM_CONDITIONAL(USE_GLIB, test "x${with_glib}" = "xyes")

AC_ARG_ENABLE([dbus],
    AS_HELP_STRING([--disable-dbus],[Disable optional D-Bus support]))

AS_IF([test "x$enable_dbus" != "xno"],
    [PKG_CHECK_MODULES(DBUS, [ dbus-1 >= 1.4.12 ], HAVE_DBUS=1, HAVE_DBUS=0)],
    HAVE_DBUS=0)

AS_IF([test "x$enable_dbus" = "xyes" && test "x$HAVE_DBUS" = "x0"],
    [AC_MSG_ERROR([*** D-Bus not available or too old version])])

AS_IF([test "x$HAVE_DBUS" = "x1"],
    [
        save_CFLAGS="$CFLAGS"; CFLAGS="$CFLAGS $DBUS_CFLAGS"
        save_LIBS="$LIBS"; LIBS="$LIBS $DBUS_LIBS"
        AC_CHECK_FUNCS(dbus_watch_get_unix_fd)
        CFLAGS="$save_CFLAGS"
        LIBS="$save_LIBS"
    ])

AC_SUBST(HAVE_DBUS)
AM_CONDITIONAL([HAVE_DBUS], [test "x$HAVE_DBUS" = x1])
AS_IF([test "x$HAVE_DBUS" = "x1"], AC_DEFINE([HAVE_DBUS], 1, [Have D-Bus.]))

AC_CONFIG_FILES([ \
        Makefile \
        agmserver.pc
        ])

AC_OUTPUT

