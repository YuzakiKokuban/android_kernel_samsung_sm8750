AM_CFLAGS = \
    -DDEBUG \
    -I src/ \
    -I inc/ \
    $(GPSUTILS_CFLAGS) \
    $(LOCAPIMSGPROTO_CFLAGS) \
    -std=c++11

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -std=c++11

requiredlibs = \
    $(GPSUTILS_LIBS) \
    $(LOCAPIMSGPROTO_LIBS) \
    -lprotobuf-lite

h_sources = \
    src/LocationIntegrationApiImpl.h \
    inc/LocationIntegrationApi.h \
    src/LocationIntegrationApiDiagLog.h

c_sources = \
    src/LocationIntegrationApiImpl.cpp \
    src/LocationIntegrationApi.cpp \
    src/LocationIntegrationApiDiagLog.cpp

liblocation_integration_api_la_SOURCES = \
    $(c_sources) $(h_sources)
liblocation_integration_api_la_h_sources = $(h_sources)

######################
# Build location_integration_api
######################

if USE_EXTERNAL_AP
AM_CFLAGS += $(LOCSOCKET_CFLAGS) -DFEATURE_EXTERNAL_AP
requiredlibs += $(LOCSOCKET_LIBS)
endif

if USE_GLIB
liblocation_integration_api_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
liblocation_integration_api_la_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
liblocation_integration_api_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
liblocation_integration_api_la_CFLAGS = $(AM_CFLAGS)
liblocation_integration_api_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -shared -version-info 1:0:0
liblocation_integration_api_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

liblocation_integration_api_la_LIBADD = $(requiredlibs) -lstdc++ -ldl

#Create and Install libraries
library_include_HEADERS = $(h_sources)
lib_LTLIBRARIES = liblocation_integration_api.la

library_includedir = $(pkgincludedir)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = location-integration-api.pc
EXTRA_DIST = $(pkgconfig_DATA)
