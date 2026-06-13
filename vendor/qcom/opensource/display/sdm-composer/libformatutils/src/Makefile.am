DISPLAY_CORE_PATH := ${WORKSPACE}/display/vendor/qcom/opensource/display-core

AM_CPPFLAGS += "-I$(DISPLAY_CORE_PATH)/sdm/include"
AM_CPPFLAGS += "-I$(DISPLAY_CORE_PATH)/include"
AM_CPPFLAGS += "-I${WORKSPACE}/display/vendor/qcom/opensource/commonsys-intf/display/include/"

c_sources = formats.cpp

lib_LTLIBRARIES = libformatutils.la
libformatutils_la_CC = @CC@
libformatutils_la_SOURCES = $(c_sources)
libformatutils_la_CFLAGS = $(COMMON_CFLAGS)
libformatutils_la_CPPFLAGS = $(AM_CPPFLAGS) -DLOG_TAG=\"FORMATUTILS\"
libformatutils_la_LDFLAGS = -shared -avoid-version
