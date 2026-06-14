// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>

#include "sec_cmd.h"
#include "sec_input.h"

#define ENABLE_DEBUG_LOG		0

#define IOCTL_SEC_CMD_IO_READ		_IOR('R', 0, struct sec_cmd_io_data)
#define IOCTL_SEC_CMD_IO_WRITE		_IOW('W', 0, struct sec_cmd_io_data)
#define MAX_FIRMWARE_FILE_SIZE		(8 * 1024 * 1024)

enum sec_cmd_io_cmd_type {
	CMD_FIRMWARE_LOAD = 1,
};

enum sec_cmd_io_mode_type {
	MODE_FIRMWARE_INFO = 2,
	MODE_FIRMWARE_DATA,
};

enum sec_cmd_io_device_type {
	IO_DEVICE_TSP = 0,
	IO_DEVICE_TSP2,
	IO_DEVICE_WACOM,
	IO_DEVICE_MAX,
};

#define SEC_CMD_IODATA_SIZE		2048
struct sec_cmd_io_data {
	int type;
	int cmd;
	int mode;
	int number;
	int length;
	char data[SEC_CMD_IODATA_SIZE];
};

struct sec_cmd_data *g_cmd_data[IO_DEVICE_MAX];

static bool is_valid_type(int device_type)
{
	if (device_type >= IO_DEVICE_MAX) {
		pr_err("%s %s: invalid type %d", SECLOG, __func__, device_type);
		return false;
	}

	if (!g_cmd_data[device_type]) {
		pr_err("%s %s: no cmd data for type %d", SECLOG, __func__, device_type);
		return false;
	}

	if (!g_cmd_data[device_type]->dev) {
		pr_err("%s %s: no device for type %d", SECLOG, __func__, device_type);
		return false;
	}

	if (!g_cmd_data[device_type]->dev->platform_data) {
		pr_err("%s %s: no platform_data for type %d", SECLOG, __func__, device_type);
		return false;
	}

	return true;
}

static long sec_cmd_ioctl_load_firmware_info(struct sec_cmd_data *cmd_data, struct sec_cmd_io_data *io_data)
{
	struct sec_ts_plat_data *plat_data = cmd_data->dev->platform_data;

	if (io_data->length <= 0 || io_data->length > MAX_FIRMWARE_FILE_SIZE) {
		input_err(true, cmd_data->dev, "%s: invalid length\n", __func__);
		return -ENOMEM;
	}

	if (plat_data->external_firmware_data && plat_data->external_firmware_size != io_data->length) {
		vfree(plat_data->external_firmware_data);
		plat_data->external_firmware_data = NULL;
		plat_data->external_firmware_size = 0;
	}

	if (!plat_data->external_firmware_data) {
		plat_data->external_firmware_data = vzalloc(io_data->length);
		if (!plat_data->external_firmware_data)
			return -ENOMEM;

		plat_data->external_firmware_size = io_data->length;
		input_info(true, cmd_data->dev, "%s: alloc firmware data size %d\n", __func__, io_data->length);
	}

	return 0;
}

static long sec_cmd_ioctl_load_firmware_data(struct sec_cmd_data *cmd_data, struct sec_cmd_io_data *io_data)
{
	struct sec_ts_plat_data *plat_data = cmd_data->dev->platform_data;
	unsigned int pos;
	int i;

	if (!plat_data->external_firmware_data) {
		input_err(true, cmd_data->dev, "%s: not allocated\n", __func__);
		return -ENOMEM;
	}

	if (io_data->number <= 0) {
		input_err(true, cmd_data->dev, "%s: invalid number\n", __func__);
		return -EINVAL;
	}

	if (io_data->length < 0 || io_data->length > SEC_CMD_IODATA_SIZE) {
		input_err(true, cmd_data->dev, "%s: invalid length\n", __func__);
		return -EINVAL;
	}

	pos = (io_data->number - 1) * SEC_CMD_IODATA_SIZE;
	if (pos + io_data->length > MAX_FIRMWARE_FILE_SIZE) {
		input_err(true, cmd_data->dev, "%s: size is invalid\n", __func__);
		return -EINVAL;
	}

	if (pos + io_data->length > plat_data->external_firmware_size) {
		input_err(true, cmd_data->dev, "%s: over than data size\n", __func__);
		return -EINVAL;
	}

	if (ENABLE_DEBUG_LOG)
		input_info(true, cmd_data->dev,
				"%s: store: number: %d, length: %d, cur_size: %d\n",
				__func__, io_data->number, io_data->length,
				pos + io_data->length);

	for (i = 0; i < io_data->length; i++) {
		if (pos + i >= plat_data->external_firmware_size) {
			input_err(true, cmd_data->dev, "%s: position is out of data size\n", __func__);
			return -ENOMEM;
		}
		plat_data->external_firmware_data[pos + i] = io_data->data[i];
	}

	return 0;
}

static long sec_cmd_ioctl_write(struct sec_cmd_data *cmd_data, void __user *p)
{
	static struct sec_cmd_io_data io_data;
	int ret = -EINVAL;

	if (copy_from_user((void *)&io_data, p, sizeof(struct sec_cmd_io_data))) {
		input_err(true, cmd_data->dev, "%s: failed to copy_form_user\n", __func__);
		return -EFAULT;
	}

	input_info(true, cmd_data->dev, "%s: type: %d, cmd: %d, mode: %d, number: %d, length: %d\n",
			__func__, io_data.type, io_data.cmd, io_data.mode,
			io_data.number, io_data.length);

	if (ENABLE_DEBUG_LOG)
		input_err(true, cmd_data->dev, "%s: %x %x %x %x %x %x %x %x %x %x\n",
				__func__, io_data.data[0], io_data.data[1], io_data.data[2],
				io_data.data[3], io_data.data[4], io_data.data[5], io_data.data[6],
				io_data.data[7], io_data.data[8], io_data.data[9]);

	switch (io_data.cmd) {
	case CMD_FIRMWARE_LOAD:
		if (io_data.mode == MODE_FIRMWARE_INFO)
			ret = sec_cmd_ioctl_load_firmware_info(cmd_data, &io_data);
		else if (io_data.mode == MODE_FIRMWARE_DATA)
			ret = sec_cmd_ioctl_load_firmware_data(cmd_data, &io_data);
		else
			input_info(true, cmd_data->dev, "%s: unknown io_data.mode: %x\n", __func__, io_data.mode);
		break;
	default:
		input_info(true, cmd_data->dev, "%s: unknown io_data.cmd: %x\n", __func__, io_data.cmd);
		break;
	}

	return ret;
}

static long sec_cmd_ioctl(int device_type, struct file *file, unsigned int cmd, void __user *p, int compat_mode)
{
	struct sec_cmd_data *cmd_data;
	int ret = -EINVAL;

	if (!is_valid_type(device_type))
		return -ENODEV;

	cmd_data = g_cmd_data[device_type];

	mutex_lock(&cmd_data->io_lock);

	switch (cmd) {
	case IOCTL_SEC_CMD_IO_READ:
		//ret = sec_cmd_ioctl_read(cmd_data, p);
		break;
	case IOCTL_SEC_CMD_IO_WRITE:
		ret = sec_cmd_ioctl_write(cmd_data, p);
		break;
	default:
		input_err(true, cmd_data->dev, "%s: unknown cmd: %x\n", __func__, cmd);
		break;
	}

	mutex_unlock(&cmd_data->io_lock);

	if (ret < 0)
		input_info(true, cmd_data->dev, "%s: failed\n", __func__);

	return ret;
}

static int sec_cmd_ioctl_open(int device_type, struct inode *inode, struct file *file)
{
	if (!is_valid_type(device_type))
		return -ENODEV;

	input_info(true, g_cmd_data[device_type]->dev, "%s\n", __func__);
	return 0;
}

static int sec_cmd_ioctl_close(int device_type, struct inode *inode, struct file *file)
{
	if (!is_valid_type(device_type))
		return -ENODEV;

	input_info(true, g_cmd_data[device_type]->dev, "%s\n", __func__);
	return 0;
}

#define sec_cmd_io_file_operation(_name, _type)								\
static long sec_cmd_unlocked_ioctl_##_name(struct file *file, unsigned int cmd, unsigned long arg)	\
{													\
	return sec_cmd_ioctl(_type, file, cmd, (void __user *)arg, 0);					\
}													\
static long sec_cmd_compat_ioctl_##_name(struct file *file, unsigned int cmd, unsigned long arg)	\
{													\
	return sec_cmd_ioctl(_type, file, cmd, (void __user *)arg, 1);					\
}													\
static int sec_cmd_ioctl_open_##_name(struct inode *inode, struct file *file)				\
{													\
	return sec_cmd_ioctl_open(_type, inode, file);							\
}													\
static int sec_cmd_ioctl_close_##_name(struct inode *inode, struct file *file)				\
{													\
	return sec_cmd_ioctl_close(_type, inode, file);							\
}													\
static const struct file_operations io_misc_fos_##_name = {						\
	.owner = THIS_MODULE,										\
	.unlocked_ioctl = sec_cmd_unlocked_ioctl_##_name,						\
	.compat_ioctl = sec_cmd_compat_ioctl_##_name,							\
	.open = sec_cmd_ioctl_open_##_name,								\
	.release = sec_cmd_ioctl_close_##_name,								\
}

sec_cmd_io_file_operation(tsp, IO_DEVICE_TSP);
sec_cmd_io_file_operation(tsp2, IO_DEVICE_TSP2);
sec_cmd_io_file_operation(wacom, IO_DEVICE_WACOM);

static struct miscdevice io_misc_device[IO_DEVICE_MAX] = {
	{
		/* IO_DEVICE_TSP */
		.fops = &io_misc_fos_tsp,
		.minor = MISC_DYNAMIC_MINOR,
		.name = "idio1",
	},
	{
		/* IO_DEVICE_TSP2 */
		.fops = &io_misc_fos_tsp2,
		.minor = MISC_DYNAMIC_MINOR,
		.name = "idio2",
	},
	{
		/* IO_DEVICE_WACOM */
		.fops = &io_misc_fos_wacom,
		.minor = MISC_DYNAMIC_MINOR,
		.name = "idio3",
	}
};

void sec_cmd_io_clear_firmware_data(struct sec_cmd_data *data)
{
	struct device *dev = data->dev;
	struct sec_ts_plat_data *plat_data;

	if (dev == NULL || dev->platform_data == NULL) {
		input_info(true, data->dev, "%s: dev or plat_data is NULL\n", __func__);
		return;
	}

	plat_data = dev->platform_data;
	if (plat_data->external_firmware_data)
		vfree(plat_data->external_firmware_data);
	plat_data->external_firmware_data = NULL;
	plat_data->external_firmware_size = 0;
	input_info(true, dev, "%s: done\n", __func__);
}
EXPORT_SYMBOL(sec_cmd_io_clear_firmware_data);

int sec_cmd_io_init(struct sec_cmd_data *data, int devt)
{
	int ret, device_type;

	switch (devt) {
	case SEC_CLASS_DEVT_TSP:
		fallthrough;
	case SEC_CLASS_DEVT_TSP1:
		device_type = IO_DEVICE_TSP;
		break;
	case SEC_CLASS_DEVT_TSP2:
		device_type = IO_DEVICE_TSP2;
		break;
	case SEC_CLASS_DEVT_WACOM:
		device_type = IO_DEVICE_WACOM;
		break;
	default:
		input_err(true, data->dev, "%s: not support device type: %d\n", __func__, devt);
		return -ENODEV;
	}

	mutex_init(&data->io_lock);

	ret = misc_register(&io_misc_device[device_type]);
	if (ret < 0)
		return -ENODEV;

	input_info(true, data->dev, "%s: misc_register: ret: %d\n", __func__, ret);

	g_cmd_data[device_type] = data;
	return 0;
}
EXPORT_SYMBOL(sec_cmd_io_init);

MODULE_DESCRIPTION("Samsung input cmd io");
MODULE_LICENSE("GPL");
