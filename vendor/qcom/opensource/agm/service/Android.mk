LOCAL_PATH := $(call my-dir)
# Build libagm_headers
include $(CLEAR_VARS)
LOCAL_MODULE                := libagm_headers
LOCAL_VENDOR_MODULE         := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc/public
include $(BUILD_HEADER_LIBRARY)

# Build libagm
include $(CLEAR_VARS)

LOCAL_MODULE        := libagm
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

# { SEC_AUDIO_COMMON
# if you need to print metadata, add -DAGM_DEBUG_METADATA
# LOCAL_CFLAGS        := -D_ANDROID_ -DAGM_DEBUG_METADATA
# } SEC_AUDIO_COMMON
LOCAL_CFLAGS        := -D_ANDROID_ -DAGM_USE_CUTILS
LOCAL_CFLAGS        += -Wno-tautological-compare -Wno-macro-redefined -Wall
LOCAL_CFLAGS        += -D_GNU_SOURCE -DACDB_PATH=\"/vendor/etc/acdbdata/\"
LOCAL_CFLAGS        += -DACDB_DELTA_FILE_PATH="/data/vendor/audio/acdbdata/delta"

LOCAL_C_INCLUDES    := $(LOCAL_PATH)/inc/public
LOCAL_C_INCLUDES    += $(LOCAL_PATH)/inc/private

# { SEC_AUDIO_COMMON
LOCAL_C_INCLUDES    += $(TOP)/system/media/audio/include
# } SEC_AUDIO_COMMON

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc/public

LOCAL_SRC_FILES  := \
    src/agm.c\
    src/graph.c\
    src/graph_module.c\
    src/metadata.c\
    src/session_obj.c\
    src/device.c \
    src/utils.c \
    src/device_hw_ep.c \
    src/agm_memlogger.c

LOCAL_HEADER_LIBRARIES := \
    libspf-headers \
    libutils_headers \
    libacdb_headers \
    libarmemlog_headers

LOCAL_SHARED_LIBRARIES := \
    libar-gsl \
    liblog \
    liblx-osal \
    libaudioroute \
    libats \
    libarmemlog \
    libcutils

#if android version is R, use qtitinyalsa lib otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter R 11,$(PLATFORM_VERSION)),)
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

