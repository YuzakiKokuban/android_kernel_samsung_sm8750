LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE        := libpalipcservice
LOCAL_MODULE_OWNER  := qti
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES    := $(PAL_BASE_PATH)/inc \
                       $(PAL_BASE_PATH)/utils/inc
# { SEC_AUDIO_COMMON
LOCAL_C_INCLUDES    += $(TOP)/system/media/audio/include
# } SEC_AUDIO_COMMON

LOCAL_CLANG             := true
LOCAL_TIDY              := true
LOCAL_CFLAGS            += -v -Wall -Wthread-safety

LOCAL_SRC_FILES     :=  \
    Service.cpp \
    PalServerWrapper.cpp

LOCAL_STATIC_LIBRARIES := \
    libpalaidltypeconverter \
    libaidlcommonsupport

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libbinder_ndk \
    libbase \
    libcutils \
    libutils \
    libfmq \
    libar-pal \
    vendor.qti.hardware.pal-V1-ndk

LOCAL_HEADER_LIBRARIES := \
    libspf-headers \
    libarosal_headers \
    libacdb_headers

include $(BUILD_SHARED_LIBRARY)
