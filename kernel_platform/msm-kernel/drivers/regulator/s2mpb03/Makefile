# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_REGULATOR_S2MPB03) += s2mpb03.o


ccflags-$(CONFIG_REGULATOR_DEBUG) += -DDEBUG

ifeq ($(CONFIG_SEC_KUNIT), y)
GCOV_PROFILE_s2mpb03.o		:= $(CONFIG_SEC_KUNIT)
endif
