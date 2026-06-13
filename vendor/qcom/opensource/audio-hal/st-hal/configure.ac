#                                               -*- Autoconf -*-
# configure.ac -- Autoconf script for sound_trigger
#

# Process this file with autoconf to produce a configure script

# Requires autoconf tool later than 2.61
AC_PREREQ(2.61)
# Initialize the sound_trigger package version 1.0.0
AC_INIT([sound_trigger],1.0.0)
# Does not strictly follow GNU Coding standards
AM_INIT_AUTOMAKE([foreign])
# Disables auto rebuilding of configure, Makefile.ins
AM_MAINTAINER_MODE
# Verifies the --srcdir is correct by checking for the path
AC_CONFIG_SRCDIR([sound_trigger_hw.c])
# defines some macros variable to be included by source
AC_CONFIG_HEADERS([config.h])
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

PKG_CHECK_MODULES([TINYALSA],[tinyalsa])
AC_SUBST([TINYALSA_CFLAGS])
PKG_CHECK_MODULES([HARDWARE],[hardware])
AC_SUBST([HARDWARE_CFLAGS])
AC_SUBST([SYSTEM_MEDIA_CFLAGS],[-I$(includedir)/])
PKG_CHECK_MODULES([AUDIO_ROUTE],[audio_route])
AC_SUBST([AUDIO_ROUTE_CFLAGS])

AC_ARG_WITH(sanitized-headers,
        AS_HELP_STRING([--with-sanitized-headers=DIR],
                [Specify the location of the sanitized Linux headers]),
        [CPPFLAGS="$CPPFLAGS -idirafter $withval"])

AM_CONDITIONAL([QSTHW_API],[test x$BOARD_SUPPORTS_QSTHW_API = xtrue])
AM_CONDITIONAL([TEST_APP],[test x$BUILD_APP = xtrue])
AM_CONDITIONAL([USE_KEEP_ALIVE],[test x$ENABLE_KEEP_ALIVE = xtrue])
AM_CONDITIONAL([SOUND_TRIGGER_CPU_AFFINITY_SET],[test x$BOARD_SUPPORTS_SOUND_TRIGGER_CPU_AFFINITY_SET = xtrue])

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

AC_CONFIG_FILES([ \
        Makefile \
        sound_trigger.pc
        ])

AC_OUTPUT
