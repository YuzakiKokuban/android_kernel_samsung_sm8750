# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the fat_common routines.
#

obj-$(CONFIG_FAT_COMMON)	+= fat_common.o

fat_common-objs			:= main.o bs_info.o
obj-$(CONFIG_FS_RO_UEVENT)	+= ro_uevent.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

cscope:
	rm -rf cscope.files cscope.files
	find $(PWD) \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.h' -o -name '*.s' -o -name '*.S' \) -print > cscope.files
	cscope

