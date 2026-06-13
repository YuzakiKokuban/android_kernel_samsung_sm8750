LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Recommend to have only utils for Audio types
# defined by AOSP

LOCAL_MODULE  := libaudiohalutils.qti
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
    Utils.cpp

LOCAL_HEADER_LIBRARIES := \
    libaudio_system_headers

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libutils \
    libaudioutils \
    android.media.audio.common.types-V3-ndk \
    android.hardware.audio.core-V2-ndk \
    qti-audio-types-aidl-V1-ndk

LOCAL_CFLAGS := \
    -DBACKEND_NDK \
    -Wall \
    -Wextra \
    -Werror \
    -Wthread-safety

include $(BUILD_STATIC_LIBRARY)
