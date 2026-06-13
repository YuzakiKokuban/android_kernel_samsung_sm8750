pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmserver.pc
EXTRA_DIST = $(pkgconfig_DATA)

h_sources = ${top_srcdir}/inc/agm-dbus-utils.h \
            ${top_srcdir}/inc/agm_server_wrapper_dbus.h

AM_CPPFLAGS = -Wno-narrowing
AM_CPPFLAGS += -I ${top_srcdir}/inc
AM_CPPFLAGS += -I ${WORKSPACE}/vendor/qcom/opensource/agm/service/inc
AM_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
AM_CPPFLAGS += $(GLIB_CFLAGS) -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h

library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)/agm

agmserver_sources = ${top_srcdir}/src/agm-dbus-utils.cpp \
                    ${top_srcdir}/src/agm_server_wrapper_dbus.cpp

lib_LTLIBRARIES = libagmserverwrapper.la
libagmserverwrapper_la_SOURCES = $(agmserver_sources)
libagmserverwrapper_la_CPPFLAGS := $(AM_CPPFLAGS) $(DBUS_CFLAGS)
libagmserverwrapper_la_LIBADD = -lagm -laudio_log_utils -lar_osal
libagmserverwrapper_la_LDFLAGS = -ldl -shared -avoid-version -lpthread -lar_osal $(DBUS_LIBS)

bin_PROGRAMS := agm_server

agm_server_SOURCES := ./src/agm-server-daemon.cpp
agm_server_la_CPPFLAGS := $(AM_CPPFLAGS)
agm_server_LDADD = libagmserverwrapper.la -laudio_log_utils -lar_osal -llog $(GLIB_LIBS)
agm_server_la_LDFLAGS = -ldl -shared -avoid-version
