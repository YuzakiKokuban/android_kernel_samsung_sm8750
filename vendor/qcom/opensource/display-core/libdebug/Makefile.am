h_sources = debug_handler.h

cpp_sources = debug_handler.cpp

library_includedir = $(includedir)
library_include_HEADERS = $(h_sources)

lib_LTLIBRARIES = libdisplaydebug.la
libdisplaydebug_la_CC = @CC@
libdisplaydebug_la_SOURCES = $(cpp_sources)
libdisplaydebug_la_CFLAGS = $(COMMON_CFLAGS) -DLOG_TAG=\"SDM\"
libdisplaydebug_la_CPPFLAGS = $(AM_CPPFLAGS)
libdisplaydebug_la_LIBADD = -ldl
libdisplaydebug_la_LDFLAGS = -shared -avoid-version
