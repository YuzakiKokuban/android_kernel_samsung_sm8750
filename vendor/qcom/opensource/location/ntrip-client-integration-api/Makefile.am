AM_CFLAGS = \
     $(GPSUTILS_CFLAGS) \
     -D__STDC_FORMAT_MACROS \
     -Dstrlcpy=g_strlcpy \
     -Dstrlcat=g_strlcat \
     -std=c++11

ACLOCAL_AMFLAGS = -I m4


c_sources = \
    ntrip-client-integration-api-testlib.cpp

libntripclient_integration_api_test_la_SOURCES = \
    $(c_sources)

if USE_GLIB
libntripclient_integration_api_test_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@ -include glib.h
libntripclient_integration_api_test_la_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libntripclient_integration_api_test_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@ -include glib.h
else
libntripclient_integration_api_test_la_CFLAGS = $(AM_CFLAGS)
libntripclient_integration_api_test_la_LDFLAGS = -Wl,-z,defs -lpthread -shared -version-info 1:0:0
libntripclient_integration_api_test_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libntripclient_integration_api_test_la_LIBADD = -ldl -lstdc++ $(GPSUTILS_LIBS)

#Create and Install libraries
library_include_HEADERS = $(h_sources)
lib_LTLIBRARIES = libntripclient_integration_api_test.la

library_includedir = $(pkgincludedir)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ntrip-client-integration-api-testlib.pc
EXTRA_DIST = $(pkgconfig_DATA)