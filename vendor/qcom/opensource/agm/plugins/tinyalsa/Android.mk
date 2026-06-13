LOCAL_PATH := $(call my-dir)
# Build libagm_pcm_plugin
include $(CLEAR_VARS)

LOCAL_MODULE        := libagm_pcm_plugin
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wall
LOCAL_SRC_FILES     := src/agm_pcm_plugin.c

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libarosal_headers

LOCAL_SHARED_LIBRARIES := \
    libsndcardparser \
    libagmclient \
    libutils \
    libcutils \
    liblog

#if android version is R, refer to qtitinyxx otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DYNAMIC_LOG)), true)
LOCAL_CFLAGS           += -DDYNAMIC_LOG_ENABLED
LOCAL_C_INCLUDES       += $(TOP)/external/expat/lib/expat.h
LOCAL_SHARED_LIBRARIES += libaudio_log_utils
LOCAL_SHARED_LIBRARIES += libexpat
LOCAL_HEADER_LIBRARIES += libaudiologutils_headers
endif

include $(BUILD_SHARED_LIBRARY)

# Build libagm_mixer_plugin
include $(CLEAR_VARS)

LOCAL_MODULE        := libagm_mixer_plugin
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES     := src/agm_mixer_plugin.c

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libarosal_headers

LOCAL_SHARED_LIBRARIES := \
    libsndcardparser \
    libagmclient \
    libcutils \
    libutils \
    liblog

#if android version is R, refer to qtitinyxx otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DYNAMIC_LOG)), true)
LOCAL_CFLAGS           += -DDYNAMIC_LOG_ENABLED
LOCAL_C_INCLUDES       += $(TOP)/external/expat/lib/expat.h
LOCAL_SHARED_LIBRARIES += libaudio_log_utils
LOCAL_SHARED_LIBRARIES += libexpat
LOCAL_HEADER_LIBRARIES += libaudiologutils_headers
endif

include $(BUILD_SHARED_LIBRARY)

# Build libagm_compress_plugin
include $(CLEAR_VARS)

LOCAL_MODULE        := libagm_compress_plugin
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES    += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SRC_FILES     := src/agm_compress_plugin.c

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libarosal_headers

LOCAL_SHARED_LIBRARIES := \
    libsndcardparser \
    libagmclient \
    libutils \
    libcutils \
    liblog

# Use flag based selection to use QTI vs open source tinycompress project

ifeq ($(TARGET_USES_QTI_TINYCOMPRESS),true)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa\
                          libqti-tinycompress
else
LOCAL_C_INCLUDES += $(TOP)/external/tinycompress/include
LOCAL_SHARED_LIBRARIES += libtinyalsa\
                          libtinycompress
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DYNAMIC_LOG)), true)
LOCAL_CFLAGS           += -DDYNAMIC_LOG_ENABLED
LOCAL_C_INCLUDES       += $(TOP)/external/expat/lib/expat.h
LOCAL_SHARED_LIBRARIES += libaudio_log_utils
LOCAL_SHARED_LIBRARIES += libexpat
LOCAL_HEADER_LIBRARIES += libaudiologutils_headers
endif

include $(BUILD_SHARED_LIBRARY)

