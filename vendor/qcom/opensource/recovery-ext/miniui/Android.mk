ifeq ($(TARGET_RECOVERY_UI_LIB),librecovery_ui_msm)
ifneq ($(TARGET_SIMULATOR),true)
ifneq ($(filter arm arm64, $(TARGET_ARCH)),)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := librecovery_ui_msm

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += bootable/recovery/recovery_ui/include

LOCAL_SRC_FILES += msm_recovery_ui.cpp

include $(BUILD_STATIC_LIBRARY)
endif   # TARGET_ARCH == arm
endif   # !TARGET_SIMULATOR
endif
