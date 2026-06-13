LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libloc_api_v02
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libqmi_cci \
    libqmi_common_so \
    libloc_core \
    libgps.utils \
    libdl \
    liblog \
    libsemnativecarrierfeature

LOCAL_SRC_FILES := \
    LocApiV02.cpp \
    loc_api_v02_log.cpp \
    loc_api_v02_client.cpp \
    loc_api_sync_req.cpp \
    location_service_v02.c \
    qmi_secgps_clnt.c \
    qmi_secgps_api_v01.c

LOCAL_CFLAGS += \
    -fno-short-enums \
    -D_ANDROID_

## Includes
LOCAL_C_INCLUDES := bionic external/stlport/stlport
LOCAL_C_INCLUDES += vendor/samsung/configs/feature/CarrierFeature/libsemnativecarrierfeature
LOCAL_C_INCLUDES += external/zlib

LOCAL_HEADER_LIBRARIES := \
    libloc_core_headers \
    libgps.utils_headers \
    libloc_pla_headers \
    liblocation_api_headers \
    libqmi_common_headers \
    libqmi_cci_headers

LOCAL_CFLAGS += $(GNSS_CFLAGS)
include $(BUILD_SHARED_LIBRARY)
