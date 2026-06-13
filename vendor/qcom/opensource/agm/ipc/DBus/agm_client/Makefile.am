pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmclient.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CPPFLAGS = -Wno-narrowing
AM_CPPFLAGS += -I ${WORKSPACE}/vendor/qcom/opensource/agm/service/inc
AM_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
AM_CPPFLAGS += $(GLIB_CFLAGS) -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
AM_CPPFLAGS += $(GIO_CFLAGS)

agmclient_sources = ${top_srcdir}/src/agm_client_wrapper_dbus.cpp

lib_LTLIBRARIES = libagmclientwrapper.la
libagmclientwrapper_la_SOURCES = $(agmclient_sources)
libagmclientwrapper_la_CPPFLAGS := $(AM_CPPFLAGS) $(DBUS_CFLAGS)
libagmclientwrapper_la_LIBADD = -lglib-2.0 -lgobject-2.0 -lgio-2.0 -lar_osal
libagmclientwrapper_la_LDFLAGS = -ldl -shared -avoid-version -lpthread -lrt $(DBUS_LIBS)
