LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE        := libagmaidltypeconverter
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS += -v -Wall  -Wextra -Wthread-safety
LOCAL_TIDY := true

LOCAL_C_INCLUDES    := $(LOCAL_PATH)/inc
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc

LOCAL_SRC_FILES     := \
    src/AgmLegacyToAidl.cpp \
    src/AgmAidlToLegacy.cpp

LOCAL_STATIC_LIBRARIES := libaidlcommonsupport

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libbinder_ndk \
    libbase \
    libcutils \
    libutils \
    libar-gsl \
    vendor.qti.hardware.agm-V1-ndk

LOCAL_HEADER_LIBRARIES := libagm_headers

include $(BUILD_STATIC_LIBRARY)
