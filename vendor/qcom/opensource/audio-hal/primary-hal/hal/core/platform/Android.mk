LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE  := libaudioplatform.qti
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES    += \
     $(LOCAL_PATH)/include \
     $(LOCAL_PATH)/../extensions/include \
     $(TOP)/system/media/audio/include \
     $(TOP)/hardware/libhardware/include

LOCAL_EXPORT_C_INCLUDE_DIRS   := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
    Platform.cpp \
    AudioUsecase.cpp \
    PlatformUtils.cpp

LOCAL_WHOLE_STATIC_LIBRARIES := libaudio_microphoneinfo_parser

LOCAL_STATIC_LIBRARIES := \
    libaudiohalutils.qti

LOCAL_SHARED_LIBRARIES := \
    libbinder_ndk \
    libbase \
    libstagefright_foundation \
    android.hardware.audio.core-V2-ndk \
    android.media.audio.common.types-V3-ndk \
    libaudioaidlcommon \
    qti-audio-types-aidl-V1-ndk \
    libaudioplatformconverter.qti \
    libar-pal

# { SEC_AUDIO_COMMON
SEC_AUDIO_VARS := vendor/samsung/variant/audio/sec_audioreach_vars.mk
include $(SEC_AUDIO_VARS)
# } SEC_AUDIO_COMMON

include $(BUILD_STATIC_LIBRARY)
