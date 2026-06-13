MY_LOCAL_PATH := $(call my-dir)
include $(MY_LOCAL_PATH)/tinyalsa/Android.mk
include $(MY_LOCAL_PATH)/tinyalsa/test/Android.mk
ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
include $(MY_LOCAL_PATH)/tinyalsa/test/gtest/Android.mk
endif