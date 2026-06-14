#
# Makefile for usbpd common drivers
#
subdir-ccflags-y := -Wformat
obj-$(CONFIG_PDIC_NOTIFIER)		+= pdic_notifier_module.o
pdic_notifier_module-y		:= pdic_notifier.o pdic_sysfs.o pdic_core.o pdic_misc.o pdic_param.o
pdic_notifier_module-$(CONFIG_PDIC_POLICY) += pdic_policy.o

ifeq ($(CONFIG_SEC_KUNIT), y)
obj-$(CONFIG_PDIC_MISC_TEST)    += pdic_misc.o
obj-$(CONFIG_PDIC_SYSFS_TEST)    += pdic_sysfs.o
GCOV_PROFILE_pdic_misc.o := $(CONFIG_SEC_KUNIT)
GCOV_PROFILE_pdic_sysfs.o := $(CONFIG_SEC_KUNIT)
endif
