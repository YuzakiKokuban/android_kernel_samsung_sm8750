HEADER_PATH := ${WORKSPACE}/display/vendor/qcom/opensource/display-core/sdm/include

c_sources = core_interface.cpp \
            core_impl.cpp \
            display_base.cpp \
            display_builtin.cpp \
            noise_plugin_intf_impl.cpp \
            display_pluggable.cpp \
            display_virtual.cpp \
            display_null.cpp \
            comp_manager.cpp \
            strategy.cpp \
            resource_default.cpp \
            hw_info_default.cpp \
            color_manager.cpp \
            dpu_core_factory.cpp \
            dpu_multi_core.cpp \
            dpu_single_core.cpp

core_h_sources = $(HEADER_PATH)/core/*.h

core_includedir = $(includedir)/sdm/core
core_include_HEADERS = $(core_h_sources)

private_h_sources = $(HEADER_PATH)/private/*.h

private_includedir = $(includedir)/sdm/private
private_include_HEADERS = $(private_h_sources)

utils_h_sources = $(HEADER_PATH)/utils/*.h

utils_includedir = $(includedir)/sdm/utils
utils_include_HEADERS = $(utils_h_sources)

lib_LTLIBRARIES = libsdmcore.la
libsdmcore_la_CC = @CC@
libsdmcore_la_SOURCES = $(c_sources)
libsdmcore_la_CFLAGS = $(COMMON_CFLAGS) -DLOG_TAG=\"SDM\"
libsdmcore_la_CPPFLAGS = $(AM_CPPFLAGS) -DPP_DRM_ENABLE
libsdmcore_la_LIBADD = ../utils/libsdmutils.la ../dal/libsdmdal.la -ldl -ldisplaydebug
libsdmcore_la_LDFLAGS = -shared -avoid-version
