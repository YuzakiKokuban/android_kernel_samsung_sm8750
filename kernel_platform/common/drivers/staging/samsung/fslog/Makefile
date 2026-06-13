# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux proc filesystem routines.
#

obj-$(CONFIG_PROC_FSLOG)       += fslog.o
obj-$(CONFIG_PROC_DLOG)        += dlog_hook.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

cscope:
	rm -rf cscope.files cscope.files
	find $(PWD) \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.h' -o -name '*.s' -o -name '*.S' \) -print > cscope.files
	cscope
