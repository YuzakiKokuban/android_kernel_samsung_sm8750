AM_CFLAGS = \
    -DDEBUG \
    -I src/ \
    -I inc/ \
    $(GPSUTILS_CFLAGS) \
    $(LOCCLIENTAPI_CFLAGS) \
    -std=c++11

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -std=c++11

requiredlibs = \
    $(GPSUTILS_LIBS) \
    $(LOCCLIENTAPI_LIBS)

h_sources = \
    inc/qapi_location.h

c_sources = \
    src/qapi_location.cpp

liblocation_qapi_la_SOURCES = \
    $(c_sources) $(h_sources)

######################
# Build location_qapi
######################

if USE_GLIB
liblocation_qapi_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
liblocation_qapi_la_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
liblocation_qapi_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
liblocation_qapi_la_CFLAGS = $(AM_CFLAGS)
liblocation_qapi_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -shared -version-info 1:0:0
liblocation_qapi_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

liblocation_qapi_la_LIBADD = $(requiredlibs) -lstdc++

#Create and Install libraries
library_include_HEADERS = $(h_sources)
lib_LTLIBRARIES = liblocation_qapi.la

library_includedir = $(pkgincludedir)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = location-qapi.pc
EXTRA_DIST = $(pkgconfig_DATA)
