LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS += -Wall
LOCAL_SRC_FILES := ipc_proxy_server.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbinder \
    libutils

LOCAL_MODULE := libagmproxy
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE := agmserver
LOCAL_SRC_FILES := \
    agm_server_daemon.cpp \
    agm_server_wrapper.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbinder \
    libutils \
    libagmproxy

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DYNAMIC_LOG)), true)
LOCAL_CFLAGS           += -DDYNAMIC_LOG_ENABLED
LOCAL_C_INCLUDES       += $(TOP)/external/expat/lib/expat.h
LOCAL_SHARED_LIBRARIES += libaudio_log_utils
LOCAL_SHARED_LIBRARIES += libexpat
LOCAL_HEADER_LIBRARIES += libaudiologutils_headers
endif

include $(BUILD_EXECUTABLE)
