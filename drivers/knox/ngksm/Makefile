#@file Makefile
#@brief Makefile for Next Generation Knox Security Monitor driver
#Copyright (c) 2019, Samsung Electronics Corporation. All rights reserved.
#
#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License version 2 and
#only version 2 as published by the Free Software Foundation.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.


ccflags-y := -Wformat
ccflags-y += -I$(srctree)/$(src)

ifeq ($(CONFIG_NGKSM), m)
    $(warning [NGKSM] CONFIG_NGKSM Enabled)
else
    $(warning [NGKSM] CONFIG_NGKSM Not Enabled)
endif

ifeq ($(CONFIG_NGKPAD), m)
    $(warning [NGKSM] CONFIG_NGKPAD Enabled)
else
    $(warning [NGKSM] CONFIG_NGKPAD Not Enabled)
endif

obj-$(CONFIG_NGKPAD)		+= ngksm.o

ngksm-objs = ngksm_main.o
ngksm-objs += ngksm_kernel_api.o
ngksm-objs += ngksm_rate_limit.o
ngksm-objs += ngksm_netlink.o

ifeq ($(CONFIG_NGKPAD),m)
    ccflags-y += -DNGK_PAD
    ngksm-objs += ngk_hypervisor_detector.o
    ifeq ($(CONFIG_UH), y)
    	ngksm-objs += uh_entry.o
    endif
endif

$(warning [NGKSM] TARGET_BUILD_VARIANT $(TARGET_BUILD_VARIANT))

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
    $(warning [NGKSM] NGK_DEBUG)
    ccflags-y += -DNGK_DEBUG
endif

ifeq ($(CONFIG_UH), y)
    $(warning [NGKSM] CONFIG_UH Enabled)
    ccflags-y += -DCONFIG_NGKSM_UH=true
endif
