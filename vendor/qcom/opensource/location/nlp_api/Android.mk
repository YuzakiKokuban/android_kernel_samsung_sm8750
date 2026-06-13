LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libnlp_api_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc

include $(BUILD_HEADER_LIBRARY)

include $(CLEAR_VARS)
