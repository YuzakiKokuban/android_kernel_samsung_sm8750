AM_CFLAGS = \
    -DDEBUG \
    -I src/ \
    -I inc/ \
    $(QMIFW_CFLAGS) \
    $(GPSUTILS_CFLAGS) \
    -fpermissive

if USE_QSOCKET
  AM_CFLAGS += -DUSE_QSOCKET
endif

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -std=c++14

requiredlibs = \
    $(QMIFW_LIBS) \
    $(GPSUTILS_LIBS)

c_sources = \
    LocSocket.cpp

libloc_socket_la_SOURCES = \
    $(c_sources)

######################
# Build loc_socket
######################

if USE_GLIB
libloc_socket_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libloc_socket_la_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_socket_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_socket_la_CFLAGS = $(AM_CFLAGS)
libloc_socket_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -shared -version-info 1:0:0
libloc_socket_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libloc_socket_la_LIBADD = $(requiredlibs) -lstdc++

#Create and Install libraries
library_include_HEADERS =
lib_LTLIBRARIES = libloc_socket.la

library_includedir = $(pkgincludedir)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = loc-socket.pc
EXTRA_DIST = $(pkgconfig_DATA)

