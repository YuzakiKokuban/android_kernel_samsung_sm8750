 /*
 *  sec_battery_misc.c
 *  Samsung Mobile Battery Misc Driver
 *  Author: Yeongmi Ha <yeongmi86.ha@samsung.com>
 *  Copyright (C) 2018 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/poll.h>

#include "sec_battery.h"
#include "sec_battery_misc.h"

static struct sec_bat_misc_dev *c_dev;

#define SEC_BATT_MISC_DBG 1
#define MAX_BUF 4095
#define NODE_OF_MISC "batt_misc"
#define BATT_IOCTL_SWAM _IOWR('B', 0, struct swam_data)

#define misc_log(str, ...) pr_info("%s:%s: "str, WC_AUTH_MSG, __func__, ##__VA_ARGS__)

#if SEC_BATT_MISC_DBG
static void print_message(u8* buf, int size)
{
	int start_idx = 0;

	do {
		char temp_buf[1024] = {0, };
		int size_temp = 0, str_len = 1024;
		int old_idx = start_idx;

		size_temp = ((start_idx + 0x7F) > size) ? size : (start_idx + 0x7F);
		for (; start_idx < size_temp; start_idx++) {
			snprintf(temp_buf + strlen(temp_buf), str_len, "0x%02x ", buf[start_idx]);
			str_len = 1024 - strlen(temp_buf);
		}
		misc_log("(%04d ~ %04d) %s\n", old_idx, start_idx - 1, temp_buf);
	} while (start_idx < size);
}
#endif

static inline int _lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1)
		return 0;

	atomic_dec(excl);
	return -1;
}

static inline void _unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

static int sec_bat_misc_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	misc_log("open success\n");
	if (!c_dev) {
		misc_log("error : c_dev is NULL\n");
		ret = -ENODEV;
		goto err;
	}

	if (_lock(&c_dev->open_excl)) {
		misc_log("error : device busy\n");
		ret = -EBUSY;
		goto err;
	}
	misc_log("open success\n");
	return 0;
err:
	return ret;
}

static int sec_bat_misc_close(struct inode *inode, struct file *file)
{
	if (c_dev)
		_unlock(&c_dev->open_excl);
	misc_log("close success\n");
	return 0;
}

static int send_swam_message(void *data, int size)
{
	int ret = c_dev->swam_write(data, size);

	misc_log("size : %d, ret : %d\n", size, ret);

	return ret;
}

static int receive_swam_message(void *data, int size)
{
	int ret = c_dev->swam_read(data);

	misc_log("size : %d, ret : %d\n", size, ret);

	return ret;
}

static long
sec_bat_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	void *buf = NULL;

	if (_lock(&c_dev->ioctl_excl)) {
		misc_log("error : ioctl busy - cmd : %d\n", cmd);
		return  -EBUSY;
	}

	switch (cmd) {
	case BATT_IOCTL_SWAM:
		misc_log("BATT_IOCTL_SWAM cmd\n");
		if (copy_from_user(&c_dev->u_data, (void __user *) arg,
				sizeof(struct swam_data))) {
			ret = -EIO;
			misc_log("copy_from_user error\n");
			goto err;
		}
		buf = kzalloc(MAX_BUF, GFP_KERNEL);
		if (!buf)
			return -EINVAL;
		if (c_dev->u_data.size > MAX_BUF) {
			ret = -ENOMEM;
			misc_log("user data size is %d error\n", c_dev->u_data.size);
			goto err;
		}

		if (c_dev->u_data.dir == DIR_OUT) {
			if (copy_from_user(buf, c_dev->u_data.pData, c_dev->u_data.size)) {
				ret = -EIO;
				misc_log("copy_from_user error\n");
				goto err;
			}
#if SEC_BATT_MISC_DBG
			misc_log("send_swam_message - size : %d\n", c_dev->u_data.size);
			print_message(buf, c_dev->u_data.size);
#endif
			ret = send_swam_message(buf, c_dev->u_data.size);
			if (ret < 0) {
				misc_log("send_swam_message error\n");
				ret = -EINVAL;
				goto err;
			}
		} else {
#if SEC_BATT_MISC_DBG
			misc_log("received_swam_message - size : %d\n", c_dev->u_data.size);
#endif
			ret = receive_swam_message(buf, c_dev->u_data.size);
			if (ret < 0) {
				misc_log("receive_swam_message error\n");
				ret = -EINVAL;
				goto err;
			}
#if SEC_BATT_MISC_DBG
			misc_log("received_swam_message - ret : %d\n", ret);
			print_message(buf, ret);
#endif
			if (copy_to_user((void __user *)c_dev->u_data.pData,
					 buf, ret)) {
				ret = -EIO;
				misc_log("copy_to_user error\n");
				goto err;
			}
		}
		break;

	default:
		misc_log("unknown ioctl cmd : %d\n", cmd);
		ret = -ENOIOCTLCMD;
		goto err;
	}
err:
	kfree(buf);
	_unlock(&c_dev->ioctl_excl);
	return ret;
}

#ifdef CONFIG_COMPAT
static long
sec_bat_misc_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	misc_log("cmd : %d\n", cmd);
	ret = sec_bat_misc_ioctl(file, cmd, arg);

	return ret;
}
#endif

int sec_bat_swam_out_request_message(void *data, int size)
{
	union power_supply_propval value = {0, };
	u8 *p_data;

	misc_log("auth service writes data\n");

	if (data == NULL) {
		misc_log("given data is not valid !\n");
		return -EINVAL;
	}
	misc_log("size = %d\n", size);

	/* clear received event */
	value.intval = WIRELESS_AUTH_SENT;
	psy_do_property("wireless", set,
		POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);

	p_data = (u8 *)data;

	if (size > 1 ) {
		/* set data size first */
		value.intval = size;
		psy_do_property("mfc-charger", set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE, value);

		value.strval = (u8 *)data;
		/* set data */
		psy_do_property("mfc-charger", set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA, value);
	} else if (size == 1 ) {
		if (p_data[0] == 0x1) {
			misc_log("auth has been passed\n");
			value.intval = WIRELESS_AUTH_PASS;
			psy_do_property("mfc-charger", set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
		} else if (p_data[0] == 0x2) {
			misc_log("auth has been failed\n");
			value.intval = WIRELESS_AUTH_FAIL;
			psy_do_property("mfc-charger", set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
		} else
			misc_log("invalid arg %d \n", p_data[0]);
	}
	return size;
}

void sec_bat_swam_copy_data(u8 *src, u8 *dest, int size)
{
	int i = 0;

	for (i = 0; i < size; i++)
		dest[i] = src[i];

#if SEC_BATT_MISC_DBG
	print_message(dest, size);
#endif
}

int sec_bat_swam_in_request_message(void *data)
{
	union power_supply_propval value = {0, };
	int size = 0;

	misc_log("auth service reads data\n");

	if (data == NULL) {
		misc_log("given data is not valid !\n");
		return -EINVAL;
	}
	misc_log("\n");

	/* get data size first */
	psy_do_property("mfc-charger", get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE, value);
	size = value.intval;
	/* get data */
	psy_do_property("mfc-charger", get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA, value);

	if (value.intval == 0) {
		misc_log("data hasn't been received yet!\n");
		return -EINVAL;
	}
	sec_bat_swam_copy_data((u8 *)value.strval, data, size);

	return size;
}

static const struct file_operations sec_bat_misc_fops = {
	.owner		= THIS_MODULE,
	.open		= sec_bat_misc_open,
	.release	= sec_bat_misc_close,
	.llseek		= no_llseek,
	.unlocked_ioctl = sec_bat_misc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sec_bat_misc_compat_ioctl,
#endif
};

static struct miscdevice sec_bat_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name	= NODE_OF_MISC,
	.fops	= &sec_bat_misc_fops,
};

int sec_bat_misc_init(struct sec_battery_info *battery)
{
	int ret = 0;

	ret = misc_register(&sec_bat_misc_device);
	if (ret) {
		misc_log("return error : %d\n", ret);
		goto err;
	}

	c_dev = kzalloc(sizeof(struct sec_bat_misc_dev), GFP_KERNEL);
	if (!c_dev) {
		ret = -ENOMEM;
		misc_log("kzalloc failed : %d\n", ret);
		goto err1;
	}
	atomic_set(&c_dev->open_excl, 0);
	atomic_set(&c_dev->ioctl_excl, 0);

	battery->misc_dev = c_dev;
	c_dev->swam_read = sec_bat_swam_in_request_message;
	c_dev->swam_write = sec_bat_swam_out_request_message;

	misc_log("register success\n");
	return 0;
err1:
	misc_deregister(&sec_bat_misc_device);
err:
	return ret;
}
EXPORT_SYMBOL(sec_bat_misc_init);

void sec_bat_misc_exit(void)
{
	misc_log("called\n");
	if (!c_dev)
		return;
	kfree(c_dev);
	misc_deregister(&sec_bat_misc_device);
}
EXPORT_SYMBOL(sec_bat_misc_exit);
