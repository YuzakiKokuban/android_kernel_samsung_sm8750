# SPDX-License-Identifier: GPL-2.0
#
# Makefile for UWB devices
#

ifeq ($(CONFIG_SAMSUNG_UWB),m)
obj-$(CONFIG_SAMSUNG_UWB)	+= uwb.o
uwb-$(CONFIG_UWB_SR200)		+= sr200.o
uwb-$(CONFIG_SEC_UWB_LOGGER)	+= uwb_logger/uwb_logger.o
else
obj-$(CONFIG_UWB_SR200)		+= sr200.o
obj-$(CONFIG_SEC_UWB_LOGGER)	+= uwb_logger/uwb_logger.o
endif
