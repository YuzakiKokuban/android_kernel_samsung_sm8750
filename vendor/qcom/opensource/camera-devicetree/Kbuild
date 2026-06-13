# Use current $(MSM_ARCH) to set config/ makefile path
CAMERA_TARGET_MKFILE_PATH := $(CAMERA_DEVICETREE_ROOT)/config/$(MSM_ARCH).mk
# Check to see if current target makefile exists
CAMERA_TARGET_EXISTS := $(or $(and $(wildcard $(CAMERA_TARGET_MKFILE_PATH)),y),n)

# Since Kernel SI can support multiple ARCH's this allows only the current selected target ARCH
# to compile.
ifeq ($(CAMERA_TARGET_EXISTS), y)
include $(CAMERA_TARGET_MKFILE_PATH)
else
# Print a warning but do not throw an error to allow bring-up of new targets!
$(warning [$(MODNAME)] $(MSM_ARCH) is not a valid target, make sure config\ folder contains a makefile named $(MSM_ARCH).mk)
$(warning [$(MODNAME)] driver is NOT being enabled!)
endif

always-y	:= $(dtbo-y) $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
