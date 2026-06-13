subdir-ccflags-y := -Wformat
obj-$(CONFIG_SEC_VIBRATOR_INPUTFF)		+= sec_vibrator_inputff_module.o
sec_vibrator_inputff_module-y		:= sec_vibrator_inputff.o sec_vibrator_inputff_sysfs.o
ifeq ($(CONFIG_SEC_KUNIT), y)
endif
