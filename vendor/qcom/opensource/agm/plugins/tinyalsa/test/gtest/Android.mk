LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE        := gtest_agm_test
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wno-unused-parameter -Wno-unused-result
LOCAL_CFLAGS        += -DBACKEND_CONF_FILE=\"/vendor/etc/backend_conf.xml\"
LOCAL_SRC_FILES     := gtest_agm_test.cpp \
../AgmPlayer.cpp \
../RiffWaveParser.cpp \
../ChunkParser.cpp \
../PlaybackCommand.cpp \
../PlaybackCommandParser.cpp

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libacdb_headers

#if android version is R, refer to qtitinyxx otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

LOCAL_SHARED_LIBRARIES += \
    libagmmixer liblog libcutils libutils

LOCAL_STATIC_LIBRARIES := libgmock libgtest libgtest_main

include $(BUILD_EXECUTABLE)
