/* SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//#ifndef _SEC_INPUT_RAWDATA_H_
//#define _SEC_INPUT_RAWDATA_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/sysfs.h>

#include <linux/miscdevice.h>
#include <linux/mutex.h>

#define IOCTL_SIZE	4096
#define POSTFIX_BYTE_LENGTH	(1 * 2)
#define PREFIX_BYTE_LENGTH	(4 * 2)
#define RAW_VEC_NUM 3

#define IOCTL_TSP_MAP_READ		_IOR(0, 0, struct raw_data)
#define IOCTL_TSP_MAP_WRITE		_IOW(0, 0, struct raw_data)
#define IOCTL_TSP_MAP_WRITE_TEST_1	_IOW('T', 1, struct raw_data)

struct raw_data {
	int num;
	u8 data[IOCTL_SIZE];
};

struct sec_input_rawdata_device {
    struct device *dev;
	struct device *fac_dev;
	int num;
	int rawdata_len;
    u8 *raw_u8;
	u8 *rawdata_pool[3];
	struct mutex lock;
	int raw_write_index;
	int raw_read_index;
};

#if IS_ENABLED(CONFIG_SEC_INPUT_RAWDATA)
void sec_input_rawdata_copy_to_user(s16 *data, int len, int press);
int sec_input_rawdata_buffer_alloc(void);
int sec_input_rawdata_init(struct device *dev, struct device *fac_dev);
#else
static inline void sec_input_rawdata_copy_to_user(s16 *data, int len, int press)
{

}
static inline int sec_input_rawdata_buffer_alloc(void)
{
	return 0;
}
static inline int sec_input_rawdata_init(struct device *dev, struct device *fac_dev)
{
	return 0;
}
#endif
//#endif
