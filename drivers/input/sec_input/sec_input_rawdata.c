// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_input.h"
#include "sec_input_rawdata.h"

static struct sec_input_rawdata_device *raw_dev;

static long sec_input_rawdata_ioctl(struct file *file, unsigned int cmd,  void __user *p, int compat_mode)
{
	static struct raw_data t;
	u8 *copier;
	int total;

	if (!raw_dev->rawdata_pool[0] || !raw_dev->rawdata_pool[1] || !raw_dev->rawdata_pool[2]) {
		input_err(true, raw_dev->fac_dev, "%s: is not allocated\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&raw_dev->lock);

	if (cmd == IOCTL_TSP_MAP_READ) {
		t.num = raw_dev->raw_write_index - raw_dev->raw_read_index;
		if (t.num == 0) {
			if (copy_to_user(p, (void *)&t, sizeof(struct raw_data))) {
				input_err(true, raw_dev->fac_dev, "%s: failed to 0 copy_to_user\n",
					__func__);
				mutex_unlock(&raw_dev->lock);
				return -EFAULT;
			}
			mutex_unlock(&raw_dev->lock);
			return 0;
		} else if (t.num < 0) {
			t.num = RAW_VEC_NUM - raw_dev->raw_read_index + raw_dev->raw_write_index;
		}

		/* rawdata_len is 4K size, and if num is over 1, u8 data[IOCTL_SIZE] is overflow
		 * will be fixed to support numbers buffer.
		 */
		if (t.num * raw_dev->rawdata_len > IOCTL_SIZE) {
			t.num = IOCTL_SIZE / raw_dev->rawdata_len;
		}

		for (total = 0; total < t.num; total++) {
			copier = raw_dev->rawdata_pool[raw_dev->raw_read_index];
			memcpy(&t.data[raw_dev->rawdata_len * total], copier, raw_dev->rawdata_len);
			raw_dev->raw_read_index++;
			if (raw_dev->raw_read_index >= 3)
				raw_dev->raw_read_index = 0;
		}

		if (copy_to_user(p, (void *)&t, sizeof(struct raw_data))) {
			input_err(true, raw_dev->fac_dev, "%s: failed to copyt_to_user\n",
				__func__);
			mutex_unlock(&raw_dev->lock);
			return -EFAULT;
		}
	} else if (cmd == IOCTL_TSP_MAP_WRITE_TEST_1) {
		if (copy_from_user((void *)&t, p, sizeof(struct raw_data))) {
			input_err(true, raw_dev->fac_dev, "%s: failed to copyt_from_user\n", __func__);
			mutex_unlock(&raw_dev->lock);
			return -EFAULT;
		}
		input_info(true, raw_dev->fac_dev, "%s: TEST_1, %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
				t.data[0], t.data[1], t.data[2], t.data[3], t.data[4], t.data[5],
				t.data[6], t.data[7], t.data[8], t.data[9], t.data[10], t.data[11]);
	}

	mutex_unlock(&raw_dev->lock);
	return 0;
}

static long sec_input_rawdata_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return sec_input_rawdata_ioctl(file, cmd, (void __user *)arg, 0);
}

static long sec_input_rawdata_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return sec_input_rawdata_ioctl(file, cmd, (void __user *)arg, 1);
}

static int sec_input_rawdata_ioctl_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sec_input_rawdata_ioctl_close(struct inode *inode, struct file *file)
{
	mutex_lock(&raw_dev->lock);
	raw_dev->raw_write_index++;
	if (raw_dev->raw_write_index >= RAW_VEC_NUM)
		raw_dev->raw_write_index = 0;
	mutex_unlock(&raw_dev->lock);

	sysfs_notify(&raw_dev->fac_dev->kobj, NULL, "raw_irq");

	return 0;
}

static const struct file_operations misc_rawdata_fos = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = sec_input_rawdata_unlocked_ioctl,
	.compat_ioctl = sec_input_rawdata_compat_ioctl,
	.open = sec_input_rawdata_ioctl_open,
	.release = sec_input_rawdata_ioctl_close,
};

static struct miscdevice misc_rawdata = {
	.fops = &misc_rawdata_fos,
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tspio",
};

MODULE_ALIAS_MISCDEV(MISC_DYNAMIC_MINOR);

static ssize_t raw_irq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int size;

	mutex_lock(&raw_dev->lock);
	size = snprintf(buf, IOCTL_SIZE, "%d,%d", raw_dev->raw_write_index, raw_dev->raw_read_index);
	mutex_unlock(&raw_dev->lock);

	return size;

}

static DEVICE_ATTR_RO(raw_irq);

static struct attribute *rawdata_attrs[] = {
	&dev_attr_raw_irq.attr,
	NULL,
};

static struct attribute_group rawdata_attr_group = {
	.attrs = rawdata_attrs,
};

void sec_input_rawdata_copy_to_user(s16 *data, int len, int press)
{
	u8 *target_mem;
	s16 *buff;
	int tlength = len;

	raw_dev->rawdata_len = len;

	mutex_lock(&raw_dev->lock);
	target_mem = raw_dev->rawdata_pool[raw_dev->raw_write_index++];
	if (raw_dev->raw_write_index >= RAW_VEC_NUM)
		raw_dev->raw_write_index = 0;
	mutex_unlock(&raw_dev->lock);

	memcpy(target_mem, data, tlength);

	buff = (s16 *)target_mem;
	buff[3] = press;

	sysfs_notify(&raw_dev->fac_dev->kobj, NULL, "raw_irq");
}
EXPORT_SYMBOL(sec_input_rawdata_copy_to_user);

int sec_input_rawdata_buffer_alloc(void)
{
	input_info(true, raw_dev->fac_dev, "%s\n", __func__);

	if (!raw_dev->rawdata_pool[0]) {
		raw_dev->rawdata_pool[0] = vmalloc(IOCTL_SIZE/*raw_dev->rawdata_len*/);
		if (!raw_dev->rawdata_pool[0])
			goto alloc_out;
	}

	if (!raw_dev->rawdata_pool[1]) {
		raw_dev->rawdata_pool[1] = vmalloc(IOCTL_SIZE/*raw_dev->rawdata_len*/);
		if (!raw_dev->rawdata_pool[1])
			goto alloc_out;
	}

	if (!raw_dev->rawdata_pool[2]) {
		raw_dev->rawdata_pool[2] = vmalloc(IOCTL_SIZE/*raw_dev->rawdata_len*/);
		if (!raw_dev->rawdata_pool[2])
			goto alloc_out;
	}

	return 0;

alloc_out:
	if (raw_dev->rawdata_pool[0])
		vfree(raw_dev->rawdata_pool[0]);
	if (raw_dev->rawdata_pool[1])
		vfree(raw_dev->rawdata_pool[1]);
	if (raw_dev->rawdata_pool[2])
		vfree(raw_dev->rawdata_pool[2]);

	raw_dev->rawdata_pool[0] = raw_dev->rawdata_pool[1] = raw_dev->rawdata_pool[2] = NULL;

	return 0;
}
EXPORT_SYMBOL(sec_input_rawdata_buffer_alloc);

int sec_input_rawdata_init(struct device *dev, struct device *fac_dev)
{
	int ret;

	input_info(true, fac_dev, "%s\n", __func__);

	raw_dev = devm_kzalloc(dev, sizeof(struct sec_input_rawdata_device), GFP_KERNEL);
	raw_dev->dev = dev;
	raw_dev->rawdata_len = IOCTL_SIZE;
	raw_dev->raw_u8 = devm_kzalloc(dev, IOCTL_SIZE, GFP_KERNEL);
	raw_dev->fac_dev = fac_dev;

	mutex_init(&raw_dev->lock);

	ret = sysfs_create_group(&fac_dev->kobj, &rawdata_attr_group);
	input_info(true, raw_dev->fac_dev, "%s: sysfs_create_group: ret: %d\n", __func__, ret);

	ret = misc_register(&misc_rawdata);
	input_info(true, raw_dev->fac_dev, "%s: misc_register: ret: %d\n", __func__, ret);
	return 0;
}
EXPORT_SYMBOL(sec_input_rawdata_init);

MODULE_DESCRIPTION("Samsung input rawdata");
MODULE_LICENSE("GPL");
