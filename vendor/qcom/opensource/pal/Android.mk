ifneq ($(AUDIO_USE_STUB_HAL), true)

LOCAL_PATH := $(call my-dir)
PAL_BASE_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE        := libar-pal
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        := -D_ANDROID_
LOCAL_CFLAGS        += -Wno-macro-redefined
LOCAL_CFLAGS        += -Wall -Werror -Wno-unused-variable -Wno-unused-parameter
LOCAL_CFLAGS        += -DCONFIG_GSL
LOCAL_CFLAGS        += -D_GNU_SOURCE
LOCAL_CFLAGS        += -DADSP_SLEEP_MONITOR
LOCAL_CFLAGS        += -DPAL_SP_TEMP_PATH=\"/data/vendor/audio/audio.cal\"
LOCAL_CFLAGS        += -DACD_SM_FILEPATH=\"/vendor/etc/models/acd/\"
ifeq ($(call is-board-platform-in-list,kalama pineapple sun), true)
LOCAL_CFLAGS        += -DSOC_PERIPHERAL_PROT
endif
LOCAL_CPPFLAGS      += -fexceptions -frtti

ifneq ($(TARGET_BOARD_PLATFORM), anorak)
LOCAL_CFLAGS        += -DA2DP_SINK_SUPPORTED
endif

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/stream/inc \
    $(LOCAL_PATH)/device/inc \
    $(LOCAL_PATH)/session/inc \
    $(LOCAL_PATH)/resource_manager/inc \
    $(LOCAL_PATH)/context_manager/inc \
    $(LOCAL_PATH)/utils/inc \
    $(LOCAL_PATH)/plugins/codecs \
    $(TOP)/system/media/audio_route/include \
    $(TOP)/system/media/audio/include

ifneq ($(TARGET_KERNEL_VERSION), 3.18)
ifneq ($(TARGET_KERNEL_VERSION), 4.14)
ifneq ($(TARGET_KERNEL_VERSION), 4.19)
ifneq ($(TARGET_KERNEL_VERSION), 4.4)
ifneq ($(TARGET_KERNEL_VERSION), 4.9)
ifneq ($(TARGET_KERNEL_VERSION), 5.4)
LOCAL_C_INCLUDES += $(TOP)/kernel_platform/msm-kernel/include/uapi/misc
endif
endif
endif
endif
endif
endif

LOCAL_C_INCLUDES              += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_C_INCLUDES              += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/techpack/audio/include
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_EXPORT_C_INCLUDE_DIRS   := $(LOCAL_PATH)/inc

LOCAL_SRC_FILES := \
    Pal.cpp \
    stream/src/Stream.cpp \
    stream/src/StreamCompress.cpp \
    stream/src/StreamPCM.cpp \
    stream/src/StreamACDB.cpp \
    stream/src/StreamInCall.cpp \
    stream/src/StreamNonTunnel.cpp \
    stream/src/StreamSoundTrigger.cpp \
    stream/src/StreamACD.cpp \
    stream/src/StreamCommon.cpp \
    stream/src/StreamContextProxy.cpp \
    stream/src/StreamCommonProxy.cpp \
    stream/src/StreamUltraSound.cpp \
    stream/src/StreamSensorPCMData.cpp\
    stream/src/StreamHaptics.cpp \
    stream/src/StreamSensorRenderer.cpp \
    stream/src/StreamASR.cpp \
    device/src/Headphone.cpp \
    device/src/USBAudio.cpp \
    device/src/Device.cpp \
    device/src/Speaker.cpp \
    device/src/Bluetooth.cpp \
    device/src/SpeakerMic.cpp \
    device/src/HeadsetMic.cpp \
    device/src/HandsetMic.cpp \
    device/src/Handset.cpp \
    device/src/HandsetVaMic.cpp \
    device/src/DisplayPort.cpp \
    device/src/HeadsetVaMic.cpp \
    device/src/RTProxy.cpp \
    device/src/SpeakerProtection.cpp \
    device/src/SpeakerProtectionTI.cpp \
    device/src/FMDevice.cpp \
    device/src/ExtEC.cpp \
    device/src/HapticsDev.cpp \
    device/src/UltrasoundDevice.cpp \
    device/src/ECRefDevice.cpp \
    device/src/DummyDev.cpp \
    device/src/HapticsDevProtection.cpp \
    session/src/Session.cpp \
    session/src/PayloadBuilder.cpp \
    session/src/SessionAlsaPcm.cpp \
    session/src/SessionAgm.cpp \
    session/src/SessionAlsaUtils.cpp \
    session/src/SessionAlsaCompress.cpp \
    session/src/SessionAlsaVoice.cpp \
    session/src/SoundTriggerEngine.cpp \
    session/src/SoundTriggerEngineCapi.cpp \
    session/src/SoundTriggerEngineGsl.cpp \
    session/src/ContextDetectionEngine.cpp \
    session/src/ASREngine.cpp \
    context_manager/src/ContextManager.cpp \
    session/src/ACDEngine.cpp \
    resource_manager/src/ResourceManager.cpp \
    resource_manager/src/SndCardMonitor.cpp \
    utils/src/SoundTriggerPlatformInfo.cpp \
    utils/src/ACDPlatformInfo.cpp \
    utils/src/VoiceUIPlatformInfo.cpp \
    utils/src/ASRPlatformInfo.cpp \
    utils/src/PalRingBuffer.cpp \
    utils/src/SignalHandler.cpp \
    utils/src/AudioHapticsInterface.cpp \
    utils/src/MetadataParser.cpp \
    utils/src/MemLogBuilder.cpp \
    utils/src/PerfLock.cpp

# { ENABLE_TFA98XX_SUPPORT
LOCAL_SRC_FILES += device/src/SpeakerProtectionTFA.cpp
# } ENABLE_TFA98XX_SUPPORT

# { SEC_AUDIO_CALL_SATELLITE
LOCAL_SRC_FILES += device/src/ExtModem.cpp \
                   device/src/ExtModemMic.cpp
# } SEC_AUDIO_CALL_SATELLITE

LOCAL_HEADER_LIBRARIES := \
    libarpal_headers \
    libspf-headers \
    libcapiv2_headers \
    libagm_headers \
    libacdb_headers \
    liblisten_headers \
    libarosal_headers \
    libvui_dmgr_headers \
    libaudiofeaturestats_headers \
    libarvui_intf_headers \
    libarmemlog_headers

LOCAL_SHARED_LIBRARIES := \
    libar-gsl\
    liblog\
    libexpat\
    liblx-osal\
    libaudioroute\
    libcutils \
    libutilscallstack \
    libagmclient \
    libvui_intf \
    libarmemlog \
    libhidlbase

ifeq ($(call is-board-platform-in-list,kalama pineapple sun), true)
LOCAL_SHARED_LIBRARIES += libPeripheralStateUtils
LOCAL_HEADER_LIBRARIES += peripheralstate_headers \
    vendor_common_inc\
    mink_headers
endif

# { SEC_AUDIO_MUTE_DETECTION
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libsecaudiohalproxy_vendor
# } SEC_AUDIO_MUTE_DETECTION

# Use flag based selection to use QTI vs open source tinycompress project

ifeq ($(TARGET_USES_QTI_TINYCOMPRESS),true)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa libqti-tinycompress
else
LOCAL_C_INCLUDES       += $(TOP)/external/tinycompress/include
LOCAL_SHARED_LIBRARIES += libtinyalsa libtinycompress
endif

# { SEC_AUDIO_COMMON
SEC_AUDIO_VARS := vendor/samsung/variant/audio/sec_audioreach_vars.mk
include $(SEC_AUDIO_VARS)

SEC_COMMON_PAL_PATH := ../../../samsung/variant/audio/sec_audioreach/pal
LOCAL_SRC_FILES += $(SEC_COMMON_PAL_PATH)/SecPal.cpp
# } SEC_AUDIO_COMMON

include $(BUILD_SHARED_LIBRARY)

#-------------------------------------------
#            Build CHARGER_LISTENER LIB
#-------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := libaudiochargerlistener
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES:= utils/src/ChargerListener.cpp

LOCAL_CFLAGS += -Wall -Werror -Wno-unused-function -Wno-unused-variable

LOCAL_SHARED_LIBRARIES += libcutils liblog

LOCAL_C_INCLUDES := $(LOCAL_PATH)/utils/inc

include $(BUILD_SHARED_LIBRARY)
ifeq (0,1)
############################################
#[samsung audio feature - unused
include $(CLEAR_VARS)
LOCAL_USE_VNDK := true

LOCAL_CFLAGS += -Wno-tautological-compare
LOCAL_CFLAGS += -Wno-macro-redefined

LOCAL_SRC_FILES  := test/PalUsecaseTest.c \
                    test/PalTest_main.c

LOCAL_MODULE               := PalTest
LOCAL_MODULE_OWNER         := qti
LOCAL_MODULE_TAGS          := optional

LOCAL_HEADER_LIBRARIES := \
    libarpal_headers

LOCAL_SHARED_LIBRARIES := \
                          libpalclient
LOCAL_VENDOR_MODULE := true

include $(BUILD_EXECUTABLE)

#samsung audio feature - unused]
############################################
endif
include $(CLEAR_VARS)

include $(PAL_BASE_PATH)/plugins/Android.mk
include $(PAL_BASE_PATH)/ipc/aidl/Android.mk

endif #AUDIO_USE_STUB_HAL
