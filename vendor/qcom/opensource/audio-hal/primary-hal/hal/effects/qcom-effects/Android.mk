ifneq ($(AUDIO_USE_STUB_HAL), true)
CURRENT_PATH := $(call my-dir)
ifeq (0,1)
############################################
#[samsung audio feature - unused
include $(CURRENT_PATH)/offloadbundle/Android.mk
include $(CURRENT_PATH)/offloadvisualizer/Android.mk
include $(CURRENT_PATH)/voiceprocessing/Android.mk
include $(CURRENT_PATH)/volumelistener/Android.mk
#samsung audio feature - unused]
############################################
else
############################################
#[samsung audio feature - used
include $(CURRENT_PATH)/voiceprocessing/Android.mk
#samsung audio feature]
############################################
endif
#include $(call all-subdir-makefiles)
endif