# This makefile is used to generate extra images for QTI targets
# persist, device tree & NAND images required for different QTI targets.

# These variables are required to make sure that the required
# files/targets are available before generating NAND images.
# This file is included from device/qcom/<TARGET>/AndroidBoard.mk
# and gets parsed before build/core/Makefile, which has these
# variables defined. build/core/Makefile will overwrite these
# variables again.
ifneq ($(strip $(TARGET_NO_KERNEL)),true)

ifneq ($(strip $(BOARD_KERNEL_BINARIES)),)
  BUILT_BOOTIMAGE_TARGET := $(foreach k,$(subst kernel,boot,$(BOARD_KERNEL_BINARIES)), $(PRODUCT_OUT)/$(k).img)
else
  BUILT_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img
endif

INSTALLED_BOOTIMAGE_TARGET := $(BUILT_BOOTIMAGE_TARGET)

ifeq ($(PRODUCT_BUILD_RAMDISK_IMAGE),true)
INSTALLED_RAMDISK_TARGET := $(PRODUCT_OUT)/ramdisk.img
endif
endif

ifeq ($(PRODUCT_BUILD_SYSTEM_IMAGE),true)
INSTALLED_SYSTEMIMAGE := $(PRODUCT_OUT)/system.img
endif
ifeq ($(PRODUCT_BUILD_USERDATA_IMAGE),true)
INSTALLED_USERDATAIMAGE_TARGET := $(PRODUCT_OUT)/userdata.img
endif
ifneq ($(TARGET_NO_RECOVERY), true)
INSTALLED_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/recovery.img
else
INSTALLED_RECOVERYIMAGE_TARGET :=
endif
recovery_ramdisk := $(PRODUCT_OUT)/ramdisk-recovery.img
INSTALLED_USBIMAGE_TARGET := $(PRODUCT_OUT)/usbdisk.img

#A/B builds require us to create the mount points at compile time.
#Just creating it for all cases since it does not hurt.
FIRMWARE_MOUNT_POINT := $(TARGET_ROOT_OUT)/firmware
BT_FIRMWARE_MOUNT_POINT := $(TARGET_ROOT_OUT)/bt_firmware
DSP_MOUNT_POINT := $(TARGET_ROOT_OUT)/dsp
PERSIST_MOUNT_POINT := $(TARGET_ROOT_OUT)/persist
ALL_DEFAULT_INSTALLED_MODULES += $(FIRMWARE_MOUNT_POINT) \
				 $(BT_FIRMWARE_MOUNT_POINT) \
				 $(DSP_MOUNT_POINT) \
				 $(PERSIST_MOUNT_POINT)
$(FIRMWARE_MOUNT_POINT):
	@echo "Creating $(FIRMWARE_MOUNT_POINT)"
	@mkdir -p $(TARGET_ROOT_OUT)/firmware
	@mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/firmware

$(BT_FIRMWARE_MOUNT_POINT):
	@echo "Creating $(BT_FIRMWARE_MOUNT_POINT)"
	@mkdir -p $(TARGET_ROOT_OUT)/bt_firmware
	@mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/bt_firmware

$(DSP_MOUNT_POINT):
	@echo "Creating $(DSP_MOUNT_POINT)"
	@mkdir -p $(TARGET_ROOT_OUT)/dsp
	@mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/dsp

$(PERSIST_MOUNT_POINT):
	@echo "Creating $(PERSIST_MOUNT_POINT)"
	@mkdir -p $(TARGET_ROOT_OUT)/persist
	@mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/persist

#----------------------------------------------------------------------
# Generate persist image (persist.img)
#----------------------------------------------------------------------
ifneq ($(strip $(BOARD_PERSISTIMAGE_PARTITION_SIZE)),)

TARGET_OUT_PERSIST := $(PRODUCT_OUT)/persist

INTERNAL_PERSISTIMAGE_FILES := \
	$(filter $(TARGET_OUT_PERSIST)/%,$(ALL_DEFAULT_INSTALLED_MODULES))

INSTALLED_PERSISTIMAGE_TARGET := $(PRODUCT_OUT)/persist.img

define build-persistimage-target
    $(call pretty,"Target persist fs image: $(INSTALLED_PERSISTIMAGE_TARGET)")
    @mkdir -p $(TARGET_OUT_PERSIST)
    $(hide) $(MKEXTUSERIMG) -s $(TARGET_OUT_PERSIST) $@ ext4 persist $(BOARD_PERSISTIMAGE_PARTITION_SIZE)
    $(hide) chmod a+r $@
    $(hide) $(call assert-max-image-size,$@,$(BOARD_PERSISTIMAGE_PARTITION_SIZE),yaffs)
endef

$(INSTALLED_PERSISTIMAGE_TARGET): $(MKEXTUSERIMG) $(MAKE_EXT4FS) $(INTERNAL_PERSISTIMAGE_FILES)
	$(build-persistimage-target)

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_PERSISTIMAGE_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_PERSISTIMAGE_TARGET)
endif

#----------------------------------------------------------------------
# Generate device tree overlay image (dtbo.img)
#----------------------------------------------------------------------
ifeq ($(strip $(BOARD_KERNEL_SEPARATED_DTBO)),true)

MKDTIMG := $(HOST_OUT_EXECUTABLES)/mkdtimg$(HOST_EXECUTABLE_SUFFIX)

INSTALLED_DTBOIMAGE_TARGET := $(PRODUCT_OUT)/dtbo.img

# Most specific paths must come first in possible_dtbo_dirs
possible_dtbo_dirs := $(KERNEL_OUT)/arch/$(TARGET_KERNEL_ARCH)/boot/dts $(KERNEL_OUT)/arch/arm/boot/dts
$(shell mkdir -p $(possible_dtbo_dirs))
dtbo_dir := $(firstword $(wildcard $(possible_dtbo_dirs)))
dtbo_objs := $(shell find $(dtbo_dir) -name \*.dtbo)

define build-dtboimage-target
    $(call pretty,"Target dtbo image: $(INSTALLED_DTBOIMAGE_TARGET)")
    $(hide) $(MKDTIMG) create $@ --page_size=$(BOARD_KERNEL_PAGESIZE) $(dtbo_objs)
    $(hide) chmod a+r $@
    $(hide) $(AVBTOOL) add_hash_footer \
	    --image $@ \
	    --partition_size $(BOARD_DTBOIMAGE_PARTITION_SIZE) \
	    --partition_name dtbo $(INTERNAL_AVB_SIGNING_ARGS)
endef

ifeq ($(BOARD_AVB_ENABLE),true)
$(INSTALLED_DTBOIMAGE_TARGET): $(MKDTIMG) $(INSTALLED_KERNEL_TARGET) $(AVBTOOL)
	$(build-dtboimage-target)
endif

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_DTBOIMAGE_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_DTBOIMAGE_TARGET)
endif

#----------------------------------------------------------------------
# Generate device tree image (dt.img)
#----------------------------------------------------------------------
ifneq ($(strip $(TARGET_NO_KERNEL)),true)
ifeq ($(strip $(BOARD_KERNEL_SEPARATED_DT)),true)
ifeq ($(strip $(BUILD_TINY_ANDROID)),true)
include device/qcom/common/dtbtool/Android.mk
endif

DTBTOOL := $(HOST_OUT_EXECUTABLES)/dtbTool$(HOST_EXECUTABLE_SUFFIX)

INSTALLED_DTIMAGE_TARGET := $(PRODUCT_OUT)/dt.img

# Most specific paths must come first in possible_dtb_dirs
possible_dtb_dirs := $(KERNEL_OUT)/arch/$(TARGET_KERNEL_ARCH)/boot/dts/qcom/ $(KERNEL_OUT)/arch/arm/boot/dts/qcom/ $(KERNEL_OUT)/arch/$(TARGET_KERNEL_ARCH)/boot/dts/ $(KERNEL_OUT)/arch/arm/boot/dts/ $(KERNEL_OUT)/arch/arm/boot/
dtb_dir := $(firstword $(wildcard $(possible_dtb_dirs)))

define build-dtimage-target
    $(call pretty,"Target dt image: $(INSTALLED_DTIMAGE_TARGET)")
    $(hide) $(DTBTOOL) -o $@ -s $(BOARD_KERNEL_PAGESIZE) -p $(KERNEL_OUT)/scripts/dtc/ $(dtb_dir)
    $(hide) chmod a+r $@
endef

$(INSTALLED_DTIMAGE_TARGET): $(DTBTOOL) $(INSTALLED_KERNEL_TARGET)
	$(build-dtimage-target)

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_DTIMAGE_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_DTIMAGE_TARGET)
endif
endif

#---------------------------------------------------------------------
# Generate usbdisk.img FAT32 image
# Please NOTICE: the valid max size of usbdisk.bin is 10GB
#---------------------------------------------------------------------
ifneq ($(strip $(BOARD_USBIMAGE_PARTITION_SIZE_KB)),)
define build-usbimage-target
	$(hide) mkfs.vfat -n "Internal SD" -F 32 -C $(PRODUCT_OUT)/usbdisk.tmp $(BOARD_USBIMAGE_PARTITION_SIZE_KB)
	$(hide) dd if=$(PRODUCT_OUT)/usbdisk.tmp of=$(INSTALLED_USBIMAGE_TARGET) bs=1024 count=20480
	$(hide) rm -f $(PRODUCT_OUT)/usbdisk.tmp
endef

$(INSTALLED_USBIMAGE_TARGET):
	$(build-usbimage-target)
ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_USBIMAGE_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_DTIMAGE_TARGET)
endif

#####################################################################################################
# support for small user data image

ifneq ($(strip $(BOARD_SMALL_USERDATAIMAGE_PARTITION_SIZE)),)
# Don't build userdata.img if it's extfs but no partition size
skip_userdata.img :=
ifdef INTERNAL_USERIMAGES_EXT_VARIANT
ifndef BOARD_USERDATAIMAGE_PARTITION_SIZE
skip_userdata.img := true
endif
endif

ifneq ($(skip_userdata.img),true)

INSTALLED_SMALL_USERDATAIMAGE_TARGET := $(PRODUCT_OUT)/userdata_small.img

define build-small-userdataimage
  @echo "target small userdata image"
  $(hide) mkdir -p $(1)
  $(hide) $(MKEXTUSERIMG) -s $(TARGET_OUT_DATA) $(2) ext4 data $(BOARD_SMALL_USERDATAIMAGE_PARTITION_SIZE)
  $(hide) chmod a+r $@
  $(hide) $(call assert-max-image-size,$@,$(BOARD_SMALL_USERDATAIMAGE_PARTITION_SIZE),yaffs)
endef


$(INSTALLED_SMALL_USERDATAIMAGE_TARGET): $(MKEXTUSERIMG) $(MAKE_EXT4FS) $(INSTALLED_USERDATAIMAGE_TARGET)
	$(hide) $(call build-small-userdataimage,$(PRODUCT_OUT),$(INSTALLED_SMALL_USERDATAIMAGE_TARGET))

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_SMALL_USERDATAIMAGE_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_SMALL_USERDATAIMAGE_TARGET)

endif

endif

#----------------------------------------------------------------------
# Compile (L)ittle (K)ernel bootloader and the nandwrite utility
#----------------------------------------------------------------------
ifneq ($(strip $(TARGET_NO_BOOTLOADER)),true)

# Compile
ifeq ($(strip $(TARGET_USES_UEFI)),true)
include bootable/bootloader/edk2/AndroidBoot.mk
else
include bootable/bootloader/lk/AndroidBoot.mk
endif

$(INSTALLED_BOOTLOADER_MODULE): $(TARGET_EMMC_BOOTLOADER) | $(ACP)
    $(transform-prebuilt-to-target)
$(BUILT_TARGET_FILES_PACKAGE): $(INSTALLED_BOOTLOADER_MODULE)

droidcore: $(INSTALLED_BOOTLOADER_MODULE)
droidcore-unbundled: $(INSTALLED_BOOTLOADER_MODULE)
endif

###################################################################################################

.PHONY: aboot
ifeq ($(USESECIMAGETOOL), true)
aboot: $(TARGET_SIGNED_BOOTLOADER) gensecimage_install
else
aboot: $(INSTALLED_BOOTLOADER_MODULE)
endif

.PHONY: kernel
kernel: $(INSTALLED_BOOTIMAGE_TARGET) $(INSTALLED_DTBOIMAGE_TARGET)

.PHONY: dtboimage
dtboimage: $(INSTALLED_DTBOIMAGE_TARGET)

.PHONY: recoveryimage
recoveryimage: $(INSTALLED_RECOVERYIMAGE_TARGET)

.PHONY: kernelclean
kernelclean:
	$(hide) make -C $(TARGET_KERNEL_SOURCE) O=$(KERNEL_TO_BUILD_ROOT_OFFSET)$(PRODUCT_OUT)/obj/KERNEL_OBJ/ ARCH=$(TARGET_ARCH) CROSS_COMPILE=arm-eabi-  clean
	$(hide) make -C $(TARGET_KERNEL_SOURCE) O=$(KERNEL_TO_BUILD_ROOT_OFFSET)$(PRODUCT_OUT)/obj/KERNEL_OBJ/ ARCH=$(TARGET_ARCH) CROSS_COMPILE=arm-eabi-  mrproper
	$(hide) make -C $(TARGET_KERNEL_SOURCE) O=$(KERNEL_TO_BUILD_ROOT_OFFSET)$(PRODUCT_OUT)/obj/KERNEL_OBJ/ ARCH=$(TARGET_ARCH) CROSS_COMPILE=arm-eabi-  distclean
	$(hide) if [ -f "$(INSTALLED_BOOTIMAGE_TARGET)" ]; then  rm $(INSTALLED_BOOTIMAGE_TARGET); fi
	$(hide) if [ -f "$(INSTALLED_BOOTIMAGE_TARGET).nonsecure" ]; then  rm $(INSTALLED_BOOTIMAGE_TARGET).nonsecure; fi
	$(hide) if [ -f "$(PRODUCT_OUT)/kernel" ]; then  rm $(PRODUCT_OUT)/kernel; fi
	@echo "kernel cleanup done"

# Set correct dependency for kernel modules
ifneq ($(BOARD_VENDOR_KERNEL_MODULES),)
$(BOARD_VENDOR_KERNEL_MODULES): $(INSTALLED_BOOTIMAGE_TARGET)
endif
ifneq ($(BOARD_RECOVERY_KERNEL_MODULES),)
$(BOARD_RECOVERY_KERNEL_MODULES): $(INSTALLED_BOOTIMAGE_TARGET)
endif
