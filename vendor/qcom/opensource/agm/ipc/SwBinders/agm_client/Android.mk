LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := agmclient
LOCAL_SRC_FILES := agm_client_wrapper.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbinder \
    libutils \
    libagmproxy

LOCAL_CFLAGS += -Wall

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DYNAMIC_LOG)), true)
LOCAL_CFLAGS           += -DDYNAMIC_LOG_ENABLED
LOCAL_C_INCLUDES       += $(TOP)/external/expat/lib/expat.h
LOCAL_SHARED_LIBRARIES += libaudio_log_utils
LOCAL_SHARED_LIBRARIES += libexpat
LOCAL_HEADER_LIBRARIES += libaudiologutils_headers
endif

include $(BUILD_EXECUTABLE)
