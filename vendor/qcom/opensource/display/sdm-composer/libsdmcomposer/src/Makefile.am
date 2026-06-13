SDM_PATH := ${WORKSPACE}/display/vendor/qcom/opensource/display-core/sdm
SDM_COMP_PATH := ${WORKSPACE}/display/vendor/qcom/opensource/display/sdm-composer

AM_CPPFLAGS += "-I$(SDM_PATH)/include"
AM_CPPFLAGS += "-I$(SDM_PATH)/../libdebug"
AM_CPPFLAGS += "-I$(SDM_PATH)/../include"
AM_CPPFLAGS += "-I${WORKSPACE}/display/vendor/qcom/opensource/commonsys-intf/display/include/"
AM_CPPFLAGS += "-I$(SDM_PATH)/../libmemutils"
AM_CPPFLAGS += "-I${PKG_CONFIG_SYSROOT_DIR}/${includedir}/qmi-framework"
AM_CPPFLAGS += "-I$(SDM_COMP_PATH)/include"


c_sources = sdm_comp_buffer_allocator.cpp \
            sdm_comp_buffer_sync_handler.cpp \
            sdm_comp_debugger.cpp \
            sdm_comp_display_builtin.cpp \
            sdm_comp_impl.cpp \
            sdm_comp_interface.cpp \
            sdm_comp_service.cpp \
            sdm_comp_service_intf.cpp \
            sdm_comp_service_client.cpp \
            property_parser_interface.cpp \
            property_parser_impl.cpp

lib_LTLIBRARIES = libsdmcomposer.la
libsdmcomposer_la_CC = @CC@
libsdmcomposer_la_SOURCES = $(c_sources)
libsdmcomposer_la_CFLAGS = $(COMMON_CFLAGS)
libsdmcomposer_la_CPPFLAGS = $(AM_CPPFLAGS) -DLOG_TAG=\"SDM\"
libsdmcomposer_la_LIBADD := ../../libformatutils/src/libformatutils.la \
                            ../../liballocator/src/liballocator.la
libsdmcomposer_la_LDFLAGS := -shared -avoid-version -lsdmcore -lsdmutils -lsdmdal -ldl \
                             -lpthread -lmemutils  -lsystemdq
