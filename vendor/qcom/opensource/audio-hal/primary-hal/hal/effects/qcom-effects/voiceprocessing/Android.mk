LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libqcomvoiceprocessing
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := soundfx
LOCAL_MODULE_OWNER := qti

LOCAL_SRC_FILES:= \
        VoiceProcessing.cpp \
        VoiceProcessingContext.cpp

LOCAL_STATIC_LIBRARIES := libaudioeffecthal_base_impl_static

LOCAL_SHARED_LIBRARIES:= \
    $(EFFECTS_DEFAULTS_SHARED_LIBRARIES)

LOCAL_HEADER_LIBRARIES:= $(EFFECTS_DEFAULTS_HEADERS_LIBRARIES)

include $(BUILD_SHARED_LIBRARY)
