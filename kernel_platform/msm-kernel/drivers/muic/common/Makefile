#
# Makefile for muic devices
#
subdir-ccflags-y := -Wformat

obj-$(CONFIG_MUIC_NOTIFIER)	+= common_muic.o
common_muic-y	:= muic-core.o muic_notifier.o muic_param.o
common_muic-$(CONFIG_MUIC_COMMON_SYSFS) += muic_sysfs.o

ifeq ($(CONFIG_SEC_KUNIT), y)
GCOV_PROFILE_muic-core.o := $(CONFIG_SEC_KUNIT)
endif
