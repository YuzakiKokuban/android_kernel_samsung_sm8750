# This makefile is used to include
# extra product boot jars for SDK

ifneq ($(TARGET_USES_QSSI),true)
ifneq ($(call is-vendor-board-platform,QCOM),true)

#add extra jars here
PRODUCT_BOOT_JARS += tcmiface

#call dex_preopt.mk for extra jars
include $(BUILD_SYSTEM)/dex_preopt.mk

endif
endif
