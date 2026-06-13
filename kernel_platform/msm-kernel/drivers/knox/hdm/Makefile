#@file Makefile
#@brief Makefile for HDM driver
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

obj-$(CONFIG_HDM)		+= hdm.o

ifeq ($(CONFIG_UH), y)
    $(warning [HDM] CONFIG_UH Enabled)
    hdm-objs = main.o uh_entry.o
    ccflags-y += -DCONFIG_HDM_UH=true
else
    hdm-objs = main.o
endif

