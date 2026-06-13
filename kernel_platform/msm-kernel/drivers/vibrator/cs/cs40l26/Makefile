# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the cs40l26 drivers.
#

# Each configuration option enables a list of files.
subdir-ccflags-y := -Wformat
input-cs40l26-i2c-objs := cs40l26.o cs40l26-tables.o cs40l26-sysfs.o cs40l26-i2c.o cs40l26-debugfs.o
input-cs40l26-spi-objs := cs40l26.o cs40l26-tables.o cs40l26-sysfs.o cs40l26-spi.o cs40l26-debugfs.o

obj-$(CONFIG_INPUT_CS40L26_I2C)		+= input-cs40l26-i2c.o
obj-$(CONFIG_INPUT_CS40L26_SPI)		+= input-cs40l26-spi.o
