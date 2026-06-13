#@file Makefile
#@brief Makefile for Next Generation Knox Security Monitor driver
#Copyright (c) 2024, Samsung Electronics Corporation. All rights reserved.
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
subdir-ccflags-y := -Wformat

# ifeq ($(CONFIG_KZT), m)
#     $(warning [KZT] CONFIG_KZT Enabled)
# else
#     $(warning [KZT] CONFIG_KZT Not Enabled)
# endif

obj-$(CONFIG_KZT)       += kzt.o

kzt-objs = kzt_main.o
kzt-objs += kzt_ioctl.o
kzt-objs += kzt_offset.o

# $(warning [KZT] TARGET_BUILD_VARIANT $(TARGET_BUILD_VARIANT))
