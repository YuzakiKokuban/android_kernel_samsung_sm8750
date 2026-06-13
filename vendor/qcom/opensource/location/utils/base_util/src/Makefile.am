AM_CFLAGS = \
     -I../ \
     -I../../base_util \
     -I../../ \
     -D__func__=__PRETTY_FUNCTION__ \
     -fno-short-enums

h_sources = \
        ../config_file.h \
        ../log.h \
        ../postcard.h \
        ../sync.h \
        ../queue.h \
        ../list.h \
        ../memorystream.h \
        ../nvparam_mgr.h \
        ../time_routines.h \
        ../vector.h \
        ../array.h \
        ../string_routines.h \
        ../shutdown_timer.h

libloc_base_util_la_SOURCES = \
        config_file.cpp \
        log.cpp \
        memorystream.cpp \
        nvparam_mgr.cpp \
        postcard.cpp \
        sync.cpp \
        string_routines.cpp \
        time_routines.cpp
library_includedir = $(pkgincludedir)
library_include_HEADERS = $(h_sources)

if USE_GLIB
libloc_base_util_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@ -Dstrlcat=g_strlcat -Dstrlcpy=g_strlcpy -include glib.h
libloc_base_util_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_base_util_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@ -Dstrlcat=g_strlcat -Dstrlcpy=g_strlcpy -include glib.h
else
libloc_base_util_la_CFLAGS = $(AM_CFLAGS)
libloc_base_util_la_LDFLAGS = -Wl,-z,defs -lpthread -shared -version-info 1:0:0
libloc_base_util_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libloc_base_util_la_LIBADD = -lstdc++ -ldl -lsqlite3

#Create and Install libraries
lib_LTLIBRARIES = libloc_base_util.la
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ../loc-base-util.pc
EXTRA_DIST = $(pkgconfig_DATA)
