SDM_PATH := ${WORKSPACE}/display/vendor/qcom/opensource/display-core/sdm

AM_CPPFLAGS += "-I$(SDM_PATH)/include"
AM_CPPFLAGS += "-I$(SDM_PATH)/../libdebug"
AM_CPPFLAGS += "-I$(SDM_PATH)/../include"
AM_CPPFLAGS += "-I${WORKSPACE}/display/vendor/qcom/opensource/commonsys-intf/display/include/"

c_sources = alloc_interface.cpp \
            ion_alloc_impl.cpp \
            dma_buf_alloc_impl.cpp

lib_LTLIBRARIES = liballocator.la
liballocator_la_CC = @CC@
liballocator_la_SOURCES = $(c_sources)
liballocator_la_CFLAGS = $(COMMON_CFLAGS)
liballocator_la_CPPFLAGS = $(AM_CPPFLAGS) -DLOG_TAG=\"ION_ALLOCATOR\" @LIBDMABUFHEAP_CFLAGS@ -std=c++17
liballocator_la_LIBADD = ../../libformatutils/src/libformatutils.la
liballocator_la_LDFLAGS = -shared -avoid-version @LIBDMABUFHEAP_LIBS@
