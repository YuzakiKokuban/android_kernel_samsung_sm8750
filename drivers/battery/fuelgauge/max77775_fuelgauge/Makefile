obj-$(CONFIG_FUELGAUGE_MAX77775)	+= max77775-fuelgauge.o
max77775-fuelgauge-$(CONFIG_FUELGAUGE_MAX77775)	+= max77775_fuelgauge.o

ifeq ($(CONFIG_SEC_KUNIT), y)
GCOV_PROFILE_max77775_fuelgauge.o := $(CONFIG_SEC_KUNIT)
endif
ccflags-y := -Wformat
