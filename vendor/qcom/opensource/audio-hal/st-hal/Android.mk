ifneq ($(AUDIO_USE_STUB_HAL), true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE               := libsoundtriggerhal.qti
LOCAL_VENDOR_MODULE        := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_C_INCLUDES            := $(LOCAL_PATH)/inc

# { SEC_AUDIO_COMMON
LOCAL_C_INCLUDES    += $(TOP)/system/media/audio/include
# } SEC_AUDIO_COMMON

LOCAL_VINTF_FRAGMENTS      := configs/soundtrigger.qti.xml

LOCAL_SRC_FILES := \
    src/soundtriggerhw/Service.cpp \
    src/soundtriggerhw/SoundTriggerHw.cpp \
    src/soundtriggerhw/SoundTriggerSession.cpp \
    src/utils/AidlToPalConverter.cpp \
    src/utils/PalToAidlConverter.cpp \
    src/utils/CoreUtils.cpp \
    src/utils/SharedMemoryWrapper.cpp

LOCAL_SHARED_LIBRARIES := \
    libbase \
    liblog \
    libutils \
    libcutils \
    libbinder_ndk \
    android.hardware.soundtrigger3-V1-ndk \
    android.media.audio.common.types-V2-ndk \
    libar-pal

include $(BUILD_SHARED_LIBRARY)
endif #AUDIO_USE_STUB_HAL
