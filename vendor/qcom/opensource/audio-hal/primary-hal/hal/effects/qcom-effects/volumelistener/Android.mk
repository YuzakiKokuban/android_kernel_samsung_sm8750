LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libvolumelistener
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := soundfx
LOCAL_MODULE_OWNER := qti

LOCAL_CLANG             := true
LOCAL_TIDY              := true
LOCAL_CFLAGS            += -v -Wall -Wthread-safety

LOCAL_SRC_FILES:= \
        VolumeListener.cpp \
        VolumeListenerContext.cpp \
        GlobalVolumeListenerSession.cpp

LOCAL_STATIC_LIBRARIES := libaudioeffecthal_base_impl_static

LOCAL_SHARED_LIBRARIES:= \
    $(EFFECTS_DEFAULTS_SHARED_LIBRARIES) \
    libar-pal

LOCAL_HEADER_LIBRARIES:= $(EFFECTS_DEFAULTS_HEADERS_LIBRARIES)

include $(BUILD_SHARED_LIBRARY)
