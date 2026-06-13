#
# Makefile for vib_info driver
#
subdir-ccflags-y := -Wformat
obj-$(CONFIG_VIBRATOR_VIB_INFO)        += vibrator_vib_info.o
ifeq ($(CONFIG_SEC_KUNIT), y)
GCOV_PROFILE_vibrator_vib_info.o := $(CONFIG_SEC_KUNIT)
endif