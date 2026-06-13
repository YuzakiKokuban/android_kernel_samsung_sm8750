# Copyright (c) 2021 The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

AC_PREREQ(2.61)

AC_INIT([libgbm],
    20.1.0,
    [],
    libgbm)

AM_INIT_AUTOMAKE([-Wall -Wno-error foreign subdir-objects])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT
AM_MAINTAINER_MODE

AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])


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

if test "x${with_glib}" = "xyes"; then
    CFLAGS="$CFLAGS -I$GLIB_CFLAGS -DUSE_GLIB"
    LDFLAGS="$LDFLAGS -lglib-2.0"
fi

AC_ARG_WITH([core_includes],
    AC_HELP_STRING([--with-core-includes=@<:@dir@:>@],
       [Specify the location of the core headers]),
    [core_incdir=$withval],
    with_core_includes=no)

if test "x$with_core_includes" != "xno"; then
   CFLAGS="${CFLAGS} -I${core_incdir}"
fi

#pkgconfigdir=${libdir}/pkgconfig
#AC_SUBST(pkgconfigdir)

AC_ARG_WITH(sanitized-headers,
   AS_HELP_STRING([--with-sanitized-headers=DIR],
       [Specify the location of the sanitized Linux headers]),
   [CPPFLAGS="$CPPFLAGS -I$withval"])

AC_ARG_ENABLE([compilewithdrm],
   AC_HELP_STRING([--enable-compilewithdrm],
      [enable compilewithdrm, build gbm with compilewithdrm]),
      [enable_compilewithdrm="${enableval}"],
      enable_compilewithdrm=no)
AM_CONDITIONAL([ENABLE_COMPILEWITHDRM], [test "x${enable_compilewithdrm}" = "xyes"])

AC_ARG_ENABLE([wayland-support],
   AC_HELP_STRING([--enable-wayland-support],
      [with wayland support, build gbm with wayland-support]),
      [enable_waylandsupport="${enableval}"],
      enable_waylandsupport=no)
AM_CONDITIONAL(BUILD_HAS_WAYLAND_SUPPORT, test "x${enable_waylandsupport}" = "xyes")
if test "x${enable_waylandsupport}" = "xyes"; then
    AC_DEFINE(BUILD_HAS_WAYLAND_SUPPORT, 1, [Enable when build has wayland support])
fi

AC_ARG_ENABLE([target-qrbx210],
	AC_HELP_STRING([--enable-target-qrbx210],
		[Enable conditional compile for target qrbx210]),
	[target_qrbx210="${enableval}"],
	target_qrbx210=no)
AM_CONDITIONAL([IS_QRBX210], [test "x$target_qrbx210" = xyes])

AC_ARG_ENABLE([target-qcs610],
	AC_HELP_STRING([--enable-target-qcs610],
		[Enable conditional compile for target qcs610]),
	[target_qcs610="${enableval}"],
	target_qcs610=no)
AM_CONDITIONAL([IS_QCS610], [test "x$target_qcs610" = xyes])

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

#PKG_CHECK_MODULES(LIBDRM, [libdrm])

ALLOCATE_SURFACE_BO_AT_CREATION=yes

AM_CONDITIONAL(ALLOCATE_SURFACE_BO_AT_CREATION, test $ALLOCATE_SURFACE_BO_AT_CREATION = yes)

if test $ALLOCATE_SURFACE_BO_AT_CREATION = yes; then
        AC_DEFINE(ALLOCATE_SURFACE_BO_AT_CREATION, 1, [Enable surface's BO creation at surface creation time])
fi


AC_CONFIG_FILES([ \
    Makefile \
    gbm.pc
    ])

AC_SUBST([CFLAGS])
AC_SUBST([CPPFLAGS])
AC_SUBST([LIBS])

AC_OUTPUT
#AC_OUTPUT([Makefile src/Makefile])
