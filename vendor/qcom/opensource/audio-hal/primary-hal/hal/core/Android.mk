ifneq ($(AUDIO_USE_STUB_HAL), true)
LOCAL_PATH := $(call my-dir)
CURRENT_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := libaudiocorehal.qti
LOCAL_VENDOR_MODULE     := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_C_INCLUDES    :=  $(LOCAL_PATH)/include

LOCAL_CFLAGS := \
    -DBACKEND_NDK \
    -Wall \
    -Wextra \
    -Werror \
    -Wthread-safety

LOCAL_VINTF_FRAGMENTS   := \
    ../../configs/common/manifest_non_qmaa.xml

LOCAL_VINTF_FRAGMENTS += \
    ../../configs/common/manifest_non_qmaa_extn.xml

LOCAL_SRC_FILES := \
    CoreService.cpp \
    Bluetooth.cpp \
    Module.cpp \
    ModulePrimary.cpp \
    ModuleStub.cpp \
    SoundDose.cpp \
    Stream.cpp \
    StreamStub.cpp \
    Telephony.cpp \
    StreamInPrimary.cpp \
    StreamOutPrimary.cpp \
    HalOffloadEffects.cpp

# { SEC_AUDIO_COMMON
SEC_COMMON_HAL_PATH := ../../../../../../samsung/variant/audio/sec_audioreach/hal
LOCAL_SRC_FILES += \
    SecModulePrimary.cpp \
    $(SEC_COMMON_HAL_PATH)/SecFTM.cpp \
    $(SEC_COMMON_HAL_PATH)/AudioEffect.cpp \
    $(SEC_COMMON_HAL_PATH)/AudioDump.cpp
# { SEC_AUDIO_SAMSUNGRECORD
LOCAL_SRC_FILES += \
    $(SEC_COMMON_HAL_PATH)/AudioPreProcess.cpp
# } SEC_AUDIO_SAMSUNGRECORD
# } SEC_AUDIO_COMMON

LOCAL_HEADER_LIBRARIES :=  \
    libxsdc-utils \
    libaudioeffects \
    liberror_headers \
    libaudioclient_headers \
    libaudio_system_headers \
    libmedia_helper_headers

#    defaults: [
#        "latest_android_media_audio_common_types_ndk_shared",
#        "latest_android_hardware_audio_core_ndk_shared",
#    ],
# mk equivalent find a way to fix this in mk file // TODO
#    android.media.audio.common.types-V2-ndk \
#    android.hardware.audio.core-V1-ndk

LOCAL_STATIC_LIBRARIES := \
    libaudiohalutils.qti \
    libaudio_module_config.qti \
    libaudiocore.extension

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libaudioplatform.qti

LOCAL_SHARED_LIBRARIES := \
    libaudioaidlcommon \
    libbase \
    libbinder_ndk \
    libcutils \
    liblog \
    libdl \
    libhidlbase \
    libhardware \
    libfmq \
    libmedia_helper \
    libstagefright_foundation \
    libutils \
    libaudioutils \
    libxml2 \
    android.hardware.common-V2-ndk \
    android.media.audio.common.types-V3-ndk \
    android.hardware.audio.core-V2-ndk \
    $(LATEST_ANDROID_HARDWARE_AUDIO_EFFECT) \
    android.hardware.audio.core.sounddose-V1-ndk \
    libar-pal \
    libaudioserviceexampleimpl \
    libaudioplatformconverter.qti \
    qti-audio-types-aidl-V1-ndk
	
# { SEC_AUDIO_COMMON
SEC_AUDIO_VARS := vendor/samsung/variant/audio/sec_audioreach_vars.mk
include $(SEC_AUDIO_VARS)
LOCAL_SHARED_LIBRARIES += libsecaudiohalproxy_vendor
# } SEC_AUDIO_COMMON

ifneq (true,$(call spf_check,SEC_PRODUCT_FEATURE_AUDIO_CONFIG_SPEAKER_AMP,))
    LOCAL_SHARED_LIBRARIES += libspeakercalibration
endif

include $(BUILD_SHARED_LIBRARY)

include $(CURRENT_PATH)/fuzzer/Android.mk
include $(CURRENT_PATH)/extensions/Android.mk
include $(CURRENT_PATH)/platform/Android.mk
include $(CURRENT_PATH)/utils/Android.mk
endif