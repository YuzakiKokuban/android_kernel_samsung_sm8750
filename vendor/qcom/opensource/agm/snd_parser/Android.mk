LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE         := libsndcardparser
LOCAL_MODULE_OWNER   := qti
LOCAL_MODULE_TAGS    := optional
LOCAL_VENDOR_MODULE  := true

LOCAL_CFLAGS         := -Wno-unused-parameter -Wall
LOCAL_CFLAGS         += -DCARD_DEF_FILE=\"/vendor/etc/card-defs.xml\"

LOCAL_C_INCLUDES            := $(LOCAL_PATH)/inc
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc
LOCAL_SRC_FILES             := src/snd-card-parser.c

LOCAL_HEADER_LIBRARIES := libagm_headers

LOCAL_SHARED_LIBRARIES := \
    libexpat \
    libcutils

include $(BUILD_SHARED_LIBRARY)
