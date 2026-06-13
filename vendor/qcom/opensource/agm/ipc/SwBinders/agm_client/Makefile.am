
AM_CPPFLAGS := -I $(top_srcdir) -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/agm/
AM_CPPFLAGS += -DDYNAMIC_LOG_ENABLED
lib_LTLIBRARIES      = libagmclientwrapper.la
libagmclientwrapper_ladir = $(libdir)
libagmclientwrapper_la_SOURCES   = src/agm_client_wrapper.cpp
libagmclientwrapper_la_CPPFLAGS := $(AM_CPPFLAGS)
libagmclientwrapper_la_LDFLAGS   = -lcutils -llog -ldl -lbinder -shared -avoid-version -lutils -lrt
libagmclientwrapper_la_LIBADD     =  -lagmproxy -laudio_log_utils
