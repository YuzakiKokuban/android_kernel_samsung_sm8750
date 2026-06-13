AM_CFLAGS = \
    -DDEBUG \
    -I./ \
    $(GPSUTILS_CFLAGS) \
    $(LOCUTILS_CFLAGS) \
    $(LOCCLIENTAPI_CFLAGS) \
    $(LOCINTEGRATIONAPI_CFLAGS) \
    -std=c++11
# HALDAEMON is requird only for msg.h

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -std=c++11

requiredlibs = \
    $(GPSUTILS_LIBS) \
    $(LOCCLIENTAPI_LIBS) \
    $(LOCINTEGRATIONAPI_LIBS)

h_sources =

c_sources = \
    main.cpp

location_client_api_testapp_SOURCES = \
    $(c_sources) $(h_sources)

######################
# Build location_client_api_testapp
######################

if USE_GLIB
location_client_api_testapp_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
location_client_api_testapp_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
location_client_api_testapp_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
location_client_api_testapp_CFLAGS = $(AM_CFLAGS)
location_client_api_testapp_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -shared -version-info 1:0:0
location_client_api_testapp_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

location_client_api_testapp_LDADD = $(requiredlibs) -lstdc++ -lc -ldl

bin_PROGRAMS = location_client_api_testapp

