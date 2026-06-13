LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := fuzz-audio-hal
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
    main.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../core/include \
    $(LOCAL_PATH)/../core/extensions/include \
    $(LOCAL_PATH)/../core/module_config/include \
    $(LOCAL_PATH)/../core/platform/include \
    $(LOCAL_PATH)/../core/utils/include \
    system/media/audio/include \
    hardware/libhardware/include \
    system/core/libsystem/include

# { SEC_AUDIO_COMMON
SEC_AUDIO_VARS := vendor/samsung/variant/audio/sec_audioreach_vars.mk
include $(SEC_AUDIO_VARS)
# } SEC_AUDIO_COMMON

LOCAL_SHARED_LIBRARIES := \
    libagmipcservice \
    libaudioaidlcommon \
    libbase \
    libbinder_ndk \
    libcutils \
    libdl \
    libhidlbase \
    libhardware \
    libfmq \
    libmedia_helper \
    libstagefright_foundation \
    libutils \
    libaudioutils \
    libxml2 \
    $(LATEST_ANDROID_HARDWARE_COMMON) \
    $(LATEST_ANDROID_HARDWARE_COMMON_FMQ) \
    $(LATEST_ANDROID_MEDIA_ADUIO_COMMON_TYPES) \
    android.hardware.audio.core-V2-ndk \
    $(LATEST_ANDROID_HARDWARE_AUDIO_EFFECT) \
    android.hardware.audio.core.sounddose-V2-ndk \
    libar-pal \
    libaudioserviceexampleimpl \
    libaudioplatformconverter.qti \
    qti-audio-types-aidl-V1-ndk \
    libaudiocorehal.qti \
    libclang_rt.ubsan_standalone

LOCAL_HEADER_LIBRARIES :=  \
    libxsdc-utils \
    libaudioeffects \
    liberror_headers \
    libaudioclient_headers \
    libaudio_system_headers \
    libmedia_helper_headers

include $(BUILD_FUZZ_TEST)
