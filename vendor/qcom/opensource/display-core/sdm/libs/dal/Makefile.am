HEADER_PATH := ${WORKSPACE}/display/vendor/qcom/opensource/display-core/sdm/include

c_sources = hw_events_interface.cpp \
            hw_info_interface.cpp \
            hw_interface.cpp \
            hw_info_drm.cpp \
            hw_device_drm.cpp \
            hw_peripheral_drm.cpp \
            hw_tv_drm.cpp \
            hw_events_drm.cpp \
            hw_scale_drm.cpp \
            hw_virtual_drm.cpp \
            hw_color_manager_drm.cpp

dal_h_sources = $(HEADER_PATH)/core/*.h

dal_includedir = $(includedir)/sdm/core
dal_include_HEADERS = $(dal_h_sources)

private_h_sources = $(HEADER_PATH)/private/*.h

private_includedir = $(includedir)/sdm/private
private_include_HEADERS = $(private_h_sources)

utils_h_sources = $(HEADER_PATH)/utils/*.h

utils_includedir = $(includedir)/sdm/utils
utils_include_HEADERS = $(utils_h_sources)

lib_LTLIBRARIES = libsdmdal.la
libsdmdal_la_CC = @CC@
libsdmdal_la_SOURCES = $(c_sources)
libsdmdal_la_CFLAGS = $(COMMON_CFLAGS) -DLOG_TAG=\"SDM\"
libsdmdal_la_CPPFLAGS = $(AM_CPPFLAGS) -DPP_DRM_ENABLE
libsdmdal_la_LIBADD = ../utils/libsdmutils.la ../../../sde-drm/libsdedrm.la -ldl -ldisplaydebug
libsdmdal_la_LDFLAGS = -shared -avoid-version
