# configure.ac -- Autoconf script for gps location-hal-daemon
#
# Process this file with autoconf to produce a configure script

# Requires autoconf tool later than 2.61
AC_PREREQ(2.61)
# Initialize the location-hal-daemon package version 1.0.0
AC_INIT([location-hal-daemon],1.0.0)
# Does not strictly follow GNU Coding standards
AM_INIT_AUTOMAKE([foreign subdir-objects])
# Disables auto rebuilding of configure, Makefile.ins
AM_MAINTAINER_MODE
# Verifies the --srcdir is correct by checking for the path
AC_CONFIG_SRCDIR([Makefile.am])
# defines some macros variable to be included by source
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PKG_CHECK_MODULES([LOCCORE], [loc-core])
AC_SUBST([LOCCORE_CFLAGS])
AC_SUBST([LOCCORE_LIBS])

PKG_CHECK_MODULES([LOCATIONAPI], [location-api])
AC_SUBST([LOCATIONAPI_CFLAGS])
AC_SUBST([LOCATIONAPI_LIBS])

PKG_CHECK_MODULES([LOCHAL], [loc-hal])
AC_SUBST([LOCHAL_CFLAGS])
AC_SUBST([LOCHAL_LIBS])

PKG_CHECK_MODULES([GPSUTILS], [qcom-gps-utils])
AC_SUBST([GPSUTILS_CFLAGS])
AC_SUBST([GPSUTILS_LIBS])

PKG_CHECK_MODULES([LOCAPIMSGPROTO], [location-api-msg-proto])
AC_SUBST([LOCAPIMSGPROTO_CFLAGS])
AC_SUBST([LOCAPIMSGPROTO_LIBS])

AC_ARG_WITH([systemd],
      AC_HELP_STRING([--with-systemd],
         [systemd enable, building LE systems which has systemd]))

AM_CONDITIONAL(USE_SYSTEMD, test "x${with_systemd}" = "xyes")

AC_ARG_WITH([auto_feature],
    AC_HELP_STRING([--with-auto_feature=@<:@dir@:>@],
        [Using Automotive feature]),
    [],
    with_auto_feature=no)

if test "x$with_auto_feature" != "xno"; then
    CPPFLAGS="${CPPFLAGS} -DFEATURE_AUTOMOTIVE"
fi

AM_CONDITIONAL(USE_FEATURE_AUTOMOTIVE, test "x${with_auto_feature}" = "xyes")

AC_ARG_WITH([core_includes],
      AC_HELP_STRING([--with-core-includes=@<:@dir@:>@],
         [Specify the location of the core headers]),
      [core_incdir=$withval],
      with_core_includes=no)

if test "x$with_core_includes" != "xno"; then
   CPPFLAGS="${CPPFLAGS} -I${core_incdir}"
fi

AC_ARG_WITH([locpla_includes],
      AC_HELP_STRING([--with-locpla-includes=@<:@dir@:>@],
         [specify the path to locpla-includes in loc-pla_git.bb]),
      [locpla_incdir=$withval],
      with_locpla_includes=no)

if test "x$with_locpla_includes" != "xno"; then
   AC_SUBST(LOCPLA_CFLAGS, "-I${locpla_incdir}")
fi

AC_SUBST([CPPFLAGS])

AC_ARG_WITH([glib],
      AC_HELP_STRING([--with-glib],
         [enable glib, building HLOS systems which use glib]))

if (test "x${with_glib}" = "xyes"); then
        AC_DEFINE(ENABLE_USEGLIB, 1, [Define if HLOS systems uses glib])
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

AC_CONFIG_FILES([ \
        Makefile \
        location-hal-daemon.pc
        ])

AC_OUTPUT
