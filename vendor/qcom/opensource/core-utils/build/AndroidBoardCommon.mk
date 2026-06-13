# This file is included from device/qcom/<TARGET>/AndroidBoard.mk	1
# This makefile is used to add make rules which need to be included
# before build/core/Makefile is included.

LOCAL_PATH := $(call my-dir)

#A/B builds require us to create the mount points at compile time.
#Just creating it for all cases since it does not hurt.
FIRMWARE_MOUNT_POINT := $(TARGET_OUT_VENDOR)/firmware_mnt
BT_FIRMWARE_MOUNT_POINT := $(TARGET_OUT_VENDOR)/bt_firmware
DSP_MOUNT_POINT := $(TARGET_OUT_VENDOR)/dsp
PERSIST_MOUNT_POINT := $(TARGET_ROOT_OUT)/persist

ifeq ($(TARGET_BOARD_PLATFORM),qssi)
 ifeq ($(TARGET_BOARD_SUFFIX),_64)
  VM_SYSTEM_MOUNT_POINT := $(TARGET_OUT_PRODUCT)/vm-system
  ALL_DEFAULT_INSTALLED_MODULES += $(VM_SYSTEM_MOUNT_POINT)
  $(VM_SYSTEM_MOUNT_POINT):
	@echo "Creating $(VM_SYSTEM_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_PRODUCT)/vm-system
 endif
endif

# create symlink system/lib/modules -> system_dlkm/lib/modules
ifeq ($(CREATE_SYSTEM_DLKM_SYMLINK),true)
$(call symlink-file,,/system_dlkm/lib/modules,$(TARGET_OUT)/lib/modules)
ALL_DEFAULT_INSTALLED_MODULES += $(TARGET_OUT)/lib/modules
endif

ALL_DEFAULT_INSTALLED_MODULES += $(FIRMWARE_MOUNT_POINT) \
				 $(BT_FIRMWARE_MOUNT_POINT) \
				 $(DSP_MOUNT_POINT)

$(FIRMWARE_MOUNT_POINT):
	@echo "Creating $(FIRMWARE_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/firmware_mnt
	@mkdir -p $(TARGET_OUT_VENDOR)/firmware-modem
ifneq ($(TARGET_MOUNT_POINTS_SYMLINKS),false)
	@ln -sf /vendor/firmware_mnt $(TARGET_ROOT_OUT)/firmware
	@ln -sf /vendor/firmware-modem $(TARGET_ROOT_OUT)/firmware-modem
endif

$(BT_FIRMWARE_MOUNT_POINT):
	@echo "Creating $(BT_FIRMWARE_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/bt_firmware
ifneq ($(TARGET_MOUNT_POINTS_SYMLINKS),false)
	@ln -sf /vendor/bt_firmware $(TARGET_ROOT_OUT)/bt_firmware
endif

$(DSP_MOUNT_POINT):
	@echo "Creating $(DSP_MOUNT_POINT)"
	@mkdir -p $(TARGET_OUT_VENDOR)/dsp
ifneq ($(TARGET_MOUNT_POINTS_SYMLINKS),false)
	@ln -sf /vendor/dsp $(TARGET_ROOT_OUT)/dsp
endif

$(PERSIST_MOUNT_POINT):
	@echo "Creating $(PERSIST_MOUNT_POINT)"
ifneq ($(TARGET_MOUNT_POINTS_SYMLINKS),false)
	@ln -sf /mnt/vendor/persist $(TARGET_ROOT_OUT)/persist
endif

# Defining BOARD_PREBUILT_DTBOIMAGE here as AndroidBoardCommon.mk
# is included before build/core/Makefile, where it is required to
# set the dependencies on prebuilt_dtbo.img based on definition of
# BOARD_PREBUILT_DTBOIMAGE
ifneq ($(strip $(BOARD_KERNEL_SEPARATED_DTBO)),)
ifndef BOARD_PREBUILT_DTBOIMAGE
BOARD_PREBUILT_DTBOIMAGE := $(PRODUCT_OUT)/prebuilt_dtbo.img
endif
endif

LIBION_HEADER_PATH_WRAPPER := $(LOCAL_PATH)/libion_header_paths/libion_path.mk

# Dump the status of various feature enforcements into a single file.
include $(LOCAL_PATH)/configs_enforcement.mk
include $(LOCAL_PATH)/makefile_violation_config.mk
# Check if enforcement override exists and include it
-include device/qcom/$(TARGET_PRODUCT)/enforcement_override.mk
FEATURE_ENFORCEMENT_STATUS := $(PRODUCT_OUT)/configs/enforcement_status.txt
$(FEATURE_ENFORCEMENT_STATUS):
	rm -rf $@
	@echo "Creating $@"
	mkdir -p $(dir $@)

ifeq ($(CLEAN_UP_JAVA_IN_VENDOR),enforcing)
	echo "DISALLOW_JAVA_SRC_COMPILE_IN_VENDOR=enforcing" >> $@
else ifeq ($(CLEAN_UP_JAVA_IN_VENDOR),warning)
	echo "DISALLOW_JAVA_SRC_COMPILE_IN_VENDOR=warning" >> $@
else
	echo "DISALLOW_JAVA_SRC_COMPILE_IN_VENDOR=disabled" >> $@
endif

ifeq ($(BUILD_BROKEN_PREBUILT_ELF_FILES),true)
	echo "PREBUILT_ELF_FILES_DEPENDENCY_ENFORCED=false" >> $@
else
	echo "PREBUILT_ELF_FILES_DEPENDENCY_ENFORCED=true" >> $@
endif
ifeq ($(BUILD_BROKEN_USES_LOCAL_COPY_HEADERS),true)
	echo "BUILD_COPY_HEADERS_ENFORCED=false" >> $@
else
	echo "BUILD_COPY_HEADERS_ENFORCED=true" >> $@
endif
ifeq ($(BUILD_BROKEN_USES_SHELL),true)
	echo "SHELL_USAGE_ENFORCED=false" >> $@
else
	echo "SHELL_USAGE_ENFORCED=true" >> $@
endif
ifeq ($(BUILD_BROKEN_USES_RECURSIVE_VARS),true)
	echo "RECURSIVE_VAR_USAGE_ENFORCED=false" >> $@
else
	echo "RECURSIVE_VAR_USAGE_ENFORCED=true" >> $@
endif
ifeq ($(BUILD_BROKEN_USES_RM_OUT),true)
	echo "RM_OUT_ENFORCED=false" >> $@
else
	echo "RM_OUT_ENFORCED=true" >> $@
endif
ifeq ($(BUILD_BROKEN_USES_DATETIME),true)
	echo "DATETIME_USAGE_ENFORCED=false" >> $@
else
	echo "DATETIME_USAGE_ENFORCED=true" >> $@
endif
ifeq ($(BUILD_BROKEN_USES_TARGET_PRODUCT),true)
	echo "TARGET_PRODUCT_USAGE_ENFORCED=false" >> $@
else
	echo "TARGET_PRODUCT_USAGE_ENFORCED=true" >> $@
endif
ifeq ($(PRODUCT_SET_DEBUGFS_RESTRICTIONS),true)
	echo "PRODUCT_SET_DEBUGFS_RESTRICTIONS=true" >> $@
else
	echo "PRODUCT_SET_DEBUGFS_RESTRICTIONS=false" >> $@
endif
	echo "PRODUCT_ENFORCE_COMMONSYSINTF_CHECKER=$(PRODUCT_ENFORCE_COMMONSYSINTF_CHECKER)" >> $@
ALL_DEFAULT_INSTALLED_MODULES += $(FEATURE_ENFORCEMENT_STATUS)
droidcore: $(FEATURE_ENFORCEMENT_STATUS)
droidcore-unbundled: $(FEATURE_ENFORCEMENT_STATUS)
