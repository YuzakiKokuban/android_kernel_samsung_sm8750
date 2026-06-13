AM_CFLAGS = \
    -I. \
    $(LOCPLA_CFLAGS) \
    $(GPSUTILS_CFLAGS) \
    $(LOCCORE_CFLAGS) \
    $(QMIFW_CFLAGS) \
    -fno-short-enums \
    -D__func__=__PRETTY_FUNCTION__ \
    -DTARGET_USES_QCOM_BSP

if PTP_SUPPORTED
AM_CFLAGS += -I$(WORKSPACE)/external/open-avb/lib/libgptp \
             -DPTP_SUPPORTED
endif

libloc_api_v02_la_SOURCES = \
    LocApiV02.cpp \
    loc_api_v02_log.cpp \
    loc_api_v02_client.cpp \
    loc_api_sync_req.cpp \
    location_service_v02.c

if USE_GLIB
libloc_api_v02_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libloc_api_v02_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -lcutils @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_api_v02_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_api_v02_la_CFLAGS = $(AM_CFLAGS)
libloc_api_v02_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -lcutils -shared -version-info 1:0:0
libloc_api_v02_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libloc_api_v02_la_CXXFLAGS = -std=c++0x
libloc_api_v02_la_LIBADD = -lstdc++ -ldl -lutils $(QMIFW_LIBS) $(GPSUTILS_LIBS) $(LOCCORE_LIBS)

if PTP_SUPPORTED
libloc_api_v02_la_LIBADD += -lgptp
endif

library_include_HEADERS = \
    location_service_v02.h \
    loc_api_v02_log.h \
    loc_api_v02_client.h \
    loc_api_sync_req.h \
    LocApiV02.h \
    loc_util_log.h

library_includedir = $(pkgincludedir)/loc_api_v02

#Create and Install libraries
lib_LTLIBRARIES = libloc_api_v02.la

library_includedir = $(pkgincludedir)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = loc-api-v02.pc
EXTRA_DIST = $(pkgconfig_DATA)
