AM_CFLAGS = \
    -DDEBUG \
    -I./ \
    $(LOCCORE_CFLAGS) \
    $(GPSUTILS_CFLAGS) \
    $(LOCAPIMSGPROTO_CFLAGS) \
     -std=c++11

if USE_SYSTEMD
AM_CFLAGS += -DINIT_SYSTEM_SYSTEMD
else
AM_CFLAGS += -DINIT_SYSTEM_SYSV
endif

if USE_FEATURE_AUTOMOTIVE
AM_CFLAGS += -DFEATURE_AUTOMOTIVE
endif

requiredlibs = \
    $(LOCATIONAPI_LIBS) \
    $(GPSUTILS_LIBS) \
    $(LOCAPIMSGPROTO_LIBS) \
    -lprotobuf-lite

h_sources = \
    LocHalDaemonClientHandler.h \
    LocationApiService.h

c_sources = \
    LocHalDaemonClientHandler.cpp \
    LocationApiService.cpp \
    main.cpp

location_hal_daemon_SOURCES = \
    $(c_sources) $(h_sources)

######################
# Build location_hal_daemon
######################

if USE_GLIB
location_hal_daemon_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
location_hal_daemon_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared
location_hal_daemon_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
location_hal_daemon_CFLAGS = $(AM_CFLAGS)
location_hal_daemon_LDFLAGS = -Wl,-z,defs -lpthread -shared
location_hal_daemon_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

location_hal_daemon_LDADD = $(requiredlibs) -lcutils -ldl

bin_PROGRAMS = location_hal_daemon

library_include_HEADERS = $(h_sources)
library_includedir = $(pkgincludedir)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = location-hal-daemon.pc
EXTRA_DIST = $(pkgconfig_DATA)

