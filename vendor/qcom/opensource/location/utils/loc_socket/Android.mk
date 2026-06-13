LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libloc_socket
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    liblog \
    libgps.utils

LOCAL_SRC_FILES := \
    LocSocket.cpp

LOCAL_CFLAGS := \
    -fno-short-enums \
    -D_ANDROID_

LOCAL_HEADER_LIBRARIES := \
    libqmi_common_headers \
    libloc_core_headers \
    libgps.utils_headers \
    libloc_pla_headers \
    liblocation_api_headers
LOCAL_CFLAGS += $(GNSS_CFLAGS)

ifeq ($(TARGET_KERNEL_VERSION),$(filter $(TARGET_KERNEL_VERSION),3.18 4.4 4.9))
LOCAL_CFLAGS += -DUSE_QSOCKET
LOCAL_HEADER_LIBRARIES += libqsocket_headers
LOCAL_SHARED_LIBRARIES += libqsocket
endif

include $(BUILD_SHARED_LIBRARY)
