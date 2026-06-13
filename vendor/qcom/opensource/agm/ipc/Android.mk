LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(call my-dir)
ifeq ($(AUDIO_FEATURE_AGM_USES_SW_BINDER), true)
include $(call all-makefiles-under, $(LOCAL_PATH)/SwBinders)
else

include $(MY_LOCAL_PATH)/aidl/Android.mk
endif
