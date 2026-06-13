// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2015-2023 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
 */

 /* usb notify layer v4.0 */

#define pr_fmt(fmt) "usb_notify: " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb_notify.h>
#include <linux/string.h>
#include "usb_notify_sysfs.h"
#include <linux/version.h>

#define MAX_STRING_LEN 20

#if defined(CONFIG_USB_HW_PARAM)
const char
usb_hw_param_print[USB_CCIC_HW_PARAM_MAX][MAX_HWPARAM_STRING] = {
	{"CC_WATER"},
	{"CC_DRY"},
	{"CC_I2C"},
	{"CC_OVC"},
	{"CC_OTG"},
	{"CC_DP"},
	{"CC_VR"},
	{"H_SUPER"},
	{"H_HIGH"},
	{"H_FULL"},
	{"H_LOW"},
	{"C_SUPER"},
	{"C_HIGH"},
	{"H_AUDIO"},
	{"H_AUDSS"},
	{"H_RBYPS"},
	{"H_COMM"},
	{"H_HID"},
	{"H_PHYSIC"},
	{"H_IMAGE"},
	{"H_PRINTER"},
	{"H_STORAGE"},
	{"H_STO_S"},
	{"H_STO_H"},
	{"H_STO_F"},
	{"H_HUB"},
	{"H_CDC"},
	{"H_CSCID"},
	{"H_CONTENT"},
	{"H_VIDEO"},
	{"H_WIRE"},
	{"H_MISC"},
	{"H_APP"},
	{"H_VENDOR"},
	{"CC_DEX"},
	{"CC_WTIME"},
	{"CC_WVBUS"},
	{"CC_WVTIME"},
	{"CC_WLVBS"},
	{"CC_WLVTM"},
	{"CC_CSHORT"},
	{"CC_SVSHT"},
	{"CC_SGSHT"},
	{"M_AFCNAK"},
	{"M_AFCERR"},
	{"M_DCDTMO"},
	{"F_CNT"},
	{"CC_KILLER"},
	{"CC_FWERR"},
	{"M_B12RS"},
	{"CC_PRS"},
	{"CC_DRS"},
	{"C_ARP"},
	{"CC_UMVS"},
	{"CC_STUCK"},
	{"H_SB"},
	{"H_OAD"},
	{"CC_VER"},
};
#endif /* CONFIG_USB_HW_PARAM */

struct notify_data {
	struct class *usb_notify_class;
	atomic_t device_count;
};

static struct notify_data usb_notify_data;

static int is_valid_cmd(char *cur_cmd, char *prev_cmd)
{
	unl_info("%s : current state=%s, previous state=%s\n",
		__func__, cur_cmd, prev_cmd);

	if (!strcmp(cur_cmd, "ON") ||
			!strncmp(cur_cmd, "ON_ALL_", 7)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto ignore;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto all;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto all;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto all;
		} else {
			goto invalid;
		}
	} else if (!strcmp(cur_cmd, "OFF")) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto off;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto off;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto off;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto ignore;
		} else {
			goto invalid;
		}
	} else if (!strncmp(cur_cmd, "ON_HOST_", 8)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto host;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto ignore;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto host;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto host;
		} else {
			goto invalid;
		}
	} else if (!strncmp(cur_cmd, "ON_CLIENT_", 10)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto client;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto client;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto ignore;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto client;
		} else {
			goto invalid;
		}
	} else {
		goto invalid;
	}
host:
	unl_info("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_HOST;
client:
	unl_info("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_CLIENT;
all:
	unl_info("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_ALL;
off:
	unl_info("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_NONE;
ignore:
	unl_err("%s cmd=%s is ignored but saved.\n", __func__, cur_cmd);
	return -EEXIST;
invalid:
	unl_err("%s cmd=%s is invalid.\n", __func__, cur_cmd);
	return -EINVAL;
}

static ssize_t disable_show(
	struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	unl_info("read disable_state %s\n", udev->disable_state_cmd);
	return sprintf(buf, "%s\n", udev->disable_state_cmd);
}

static ssize_t disable_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	char *disable;
	int sret, param = -EINVAL;
	size_t ret = -ENOMEM;

	if (size > MAX_DISABLE_STR_LEN) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	if (size < strlen(buf))
		goto error;
	disable = kzalloc(size+1, GFP_KERNEL);
	if (!disable)
		goto error;

	sret = sscanf(buf, "%s", disable);
	if (sret != 1)
		goto error1;

	if (udev->set_disable) {
		param = is_valid_cmd(disable, udev->disable_state_cmd);
		if (param == -EINVAL) {
			ret = param;
		} else {
			if (param != -EEXIST) {
#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
				udev->first_restrict = false;
#endif
				udev->set_disable(udev, param);
			}
			strncpy(udev->disable_state_cmd,
				disable, sizeof(udev->disable_state_cmd)-1);
			ret = size;
		}
	} else
		unl_err("set_disable func is NULL\n");
error1:
	kfree(disable);
error:
	return ret;
}

static ssize_t usb_data_enabled_show(
	struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	unl_info("read usb_data_enabled %lu\n", udev->usb_data_enabled);
	return sprintf(buf, "%lu\n", udev->usb_data_enabled);
}

static ssize_t usb_data_enabled_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	size_t ret = -ENOMEM;
	int sret = -EINVAL;
	int param = 0;
	char *usb_data_enabled;

	if (size > PAGE_SIZE) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	usb_data_enabled = kzalloc(size+1, GFP_KERNEL);
	if (!usb_data_enabled)
		goto error;

	sret = sscanf(buf, "%s", usb_data_enabled);
	if (sret != 1)
		goto error1;

	if (udev->set_disable) {
		if (strcmp(usb_data_enabled, "0") == 0) {
			param = NOTIFY_BLOCK_TYPE_ALL;
			udev->usb_data_enabled = 0;
		} else if (strcmp(usb_data_enabled, "1") == 0) {
			param = NOTIFY_BLOCK_TYPE_NONE;
			udev->usb_data_enabled = 1;
		} else {
			unl_err("%s usb_data_enabled(%s) error.\n",
				__func__, usb_data_enabled);
			goto error1;
		}
		unl_info("%s usb_data_enabled=%s\n",
			__func__, usb_data_enabled);
			udev->set_disable(udev, param);
		ret = size;
	} else {
		unl_err("%s set_disable func is NULL\n", __func__);
	}
error1:
	kfree(usb_data_enabled);
error:
	return ret;
}

static ssize_t support_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	char *support;

	if (n->unsupport_host || !IS_ENABLED(CONFIG_USB_HOST_NOTIFY))
		support = "CLIENT";
	else
		support = "ALL";

	unl_info("read support %s\n", support);
	return sprintf(buf, "%s\n", support);
}

static ssize_t otg_speed_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	int dev_max_speed = 0;
	char *speed;

	dev_max_speed = get_con_dev_max_speed(n);

	switch (dev_max_speed) {
	case USB_SPEED_SUPER_PLUS:
		speed = "SUPER PLUS";
		break;
	case USB_SPEED_SUPER:
		speed = "SUPER";
		break;
	case USB_SPEED_HIGH:
		speed = "HIGH";
		break;
	case USB_SPEED_FULL:
		speed = "FULL";
		break;
	case USB_SPEED_LOW:
		speed = "LOW";
		break;
	default:
		speed = "UNKNOWN";
		break;
	}
	unl_info("%s : read otg speed %s\n", __func__, speed);
	return sprintf(buf, "%s\n", speed);
}

static ssize_t gadget_speed_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	const char *speed;

	if (n->get_gadget_speed)
		speed = usb_speed_string(n->get_gadget_speed());
	else
		speed = "UNKNOWN";

	unl_info("%s : read gadget speed %s\n", __func__, speed);
	return snprintf(buf,  MAX_STRING_LEN, "%s\n", speed);
}

static const char *const max_speed_str[] = {
	[USB_SPEED_UNKNOWN] = "UNKNOWN",
	[USB_SPEED_LOW] = "low-speed",
	[USB_SPEED_FULL] = "full-speed",
	[USB_SPEED_HIGH] = "high-speed",
	[USB_SPEED_WIRELESS] = "wireless-usb",
	[USB_SPEED_SUPER] = "super-speed",
	[USB_SPEED_SUPER_PLUS] = "super-speed+",
};

static ssize_t usb_maximum_speed_show(
	struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	int ret = 0;

	ret = udev->control_usb_max_speed(udev, -1);

	return sprintf(buf, "%s\n", max_speed_str[ret]);
}

static ssize_t usb_maximum_speed_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	int max_speed_idx = USB_SPEED_UNKNOWN;
	char *max_speed;
	size_t ret = -ENOMEM, i, sret;

	unl_info("%s\n", __func__);

	if (size > MAX_USB_SPEED_STR_LEN) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	max_speed = kzalloc(size+1, GFP_KERNEL);
	if (!max_speed)
		goto error;

	sret = sscanf(buf, "%s", max_speed);
	if (sret != 1)
		goto error1;

	for (i = 0; i < ARRAY_SIZE(max_speed_str); i++) {
		if (strncmp(max_speed, max_speed_str[i],
				strlen(max_speed_str[i])) == 0) {
			max_speed_idx = i;
			break;
		}
	}

	if (max_speed_idx == USB_SPEED_UNKNOWN) {
		ret = -EINVAL;
		goto error1;
	} else {
		sret = udev->control_usb_max_speed(udev, max_speed_idx);
	}

	unl_info("%s req=%s now=%s\n", __func__, max_speed,
			max_speed_str[max_speed_idx]);
	ret = size;
error1:
	kfree(max_speed);
error:
	return ret;
}

#if defined(CONFIG_USB_HW_PARAM)
static unsigned long long strtoull(char *ptr, char **end, int base)
{
	unsigned long long ret = 0;

	if (base > 36)
		goto out;

	while (*ptr) {
		int digit;

		if (*ptr >= '0' && *ptr <= '9' && *ptr < '0' + base)
			digit = *ptr - '0';
		else if (*ptr >= 'A' && *ptr < 'A' + base - 10)
			digit = *ptr - 'A' + 10;
		else if (*ptr >= 'a' && *ptr < 'a' + base - 10)
			digit = *ptr - 'a' + 10;
		else
			break;

		ret *= base;
		ret += digit;
		ptr++;
	}

out:
	if (end)
		*end = (char *)ptr;

	return ret;
}

static ssize_t usb_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	int index, ret = 0;
	unsigned long long *p_param = NULL;

	if (udev->fp_hw_param_manager) {
		p_param = get_hw_param(n, USB_CCIC_WATER_INT_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_INT_COUNT);
		p_param = get_hw_param(n, USB_CCIC_DRY_INT_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_DRY_INT_COUNT);
		p_param = get_hw_param(n, USB_CLIENT_SUPER_SPEED_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CLIENT_SUPER_SPEED_COUNT);
		p_param = get_hw_param(n, USB_CLIENT_HIGH_SPEED_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CLIENT_HIGH_SPEED_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_TIME_DURATION);
		p_param = get_hw_param(n, USB_CCIC_WATER_VBUS_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_VBUS_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_LPM_VBUS_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_LPM_VBUS_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_VBUS_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_VBUS_TIME_DURATION);
		p_param = get_hw_param(n,
				USB_CCIC_WATER_LPM_VBUS_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_LPM_VBUS_TIME_DURATION);
	}
	p_param = get_hw_param(n, USB_CCIC_VERSION);
	if (p_param)
		*p_param = show_ccic_version();
	for (index = 0; index < USB_CCIC_HW_PARAM_MAX - 1; index++) {
		p_param = get_hw_param(n, index);
		if (p_param)
			ret += sprintf(buf + ret, "%llu ", *p_param);
		else
			ret += sprintf(buf + ret, "0 ");
	}
	p_param = get_hw_param(n, index);
	if (p_param)
		ret += sprintf(buf + ret, "%llu\n", *p_param);
	else
		ret += sprintf(buf + ret, "0\n");
	unl_info("%s - ret : %d\n", __func__, ret);

	return ret;
}

static ssize_t usb_hw_param_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	unsigned long long prev_hw_param[USB_CCIC_HW_PARAM_MAX] = {0, };
	unsigned long long *p_param = NULL;
	int index = 0;
	size_t ret = -ENOMEM;
	char *token, *str = (char *)buf;

	if (size > MAX_HWPARAM_STR_LEN) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}
	ret = size;
	if (size < USB_CCIC_HW_PARAM_MAX) {
		unl_err("%s efs file is not created correctly.\n", __func__);
		goto error;
	}

	for (index = 0; index < (USB_CCIC_HW_PARAM_MAX - 1); index++) {
		token = strsep(&str, " ");
		if (token)
			prev_hw_param[index] = strtoull(token, NULL, 10);

		if (!token || (prev_hw_param[index] > HWPARAM_DATA_LIMIT))
			goto error;
	}

	for (index = 0; index < (USB_CCIC_HW_PARAM_MAX - 1); index++) {
		p_param = get_hw_param(n, index);
		if (p_param)
			*p_param += prev_hw_param[index];
	}
	unl_info("%s - ret : %zu\n", __func__, ret);
error:
	return ret;
}

static int is_skip_list(struct otg_notify *n, int index)
{
	if (!n)
		return 0;

	if (n->is_skip_list)
		return n->is_skip_list(index);

	return 0;
}

static ssize_t hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	int index = 0, ret = 0;
	unsigned long long *p_param = NULL;

	if (udev->fp_hw_param_manager) {
		p_param = get_hw_param(n, USB_CCIC_WATER_INT_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_INT_COUNT);
		p_param = get_hw_param(n, USB_CCIC_DRY_INT_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_DRY_INT_COUNT);
		p_param = get_hw_param(n, USB_CLIENT_SUPER_SPEED_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CLIENT_SUPER_SPEED_COUNT);
		p_param = get_hw_param(n, USB_CLIENT_HIGH_SPEED_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CLIENT_HIGH_SPEED_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_TIME_DURATION);
		p_param = get_hw_param(n, USB_CCIC_WATER_VBUS_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_VBUS_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_LPM_VBUS_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_LPM_VBUS_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_VBUS_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_VBUS_TIME_DURATION);
		p_param = get_hw_param(n,
				USB_CCIC_WATER_LPM_VBUS_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_LPM_VBUS_TIME_DURATION);
	}
	if (!is_skip_list(n, USB_CCIC_VERSION)) {
		p_param = get_hw_param(n, USB_CCIC_VERSION);
		if (p_param)
			*p_param = show_ccic_version();
	}
	for (index = 0; index < USB_CCIC_HW_PARAM_MAX - 1; index++) {
		if (!is_skip_list(n, index)) {
			p_param = get_hw_param(n, index);
			if (p_param)
				ret += sprintf(buf + ret, "\"%s\":\"%llu\",",
					usb_hw_param_print[index], *p_param);
			else
				ret += sprintf(buf + ret, "\"%s\":\"0\",",
					usb_hw_param_print[index]);
		}
	}
	if (!is_skip_list(n, USB_CCIC_VERSION)) {
		/* CCIC FW version */
		ret += sprintf(buf + ret, "\"%s\":\"",
			usb_hw_param_print[USB_CCIC_VERSION]);
		p_param = get_hw_param(n, USB_CCIC_VERSION);
		if (p_param) {
			/* HW Version */
			ret += sprintf(buf + ret, "%02X%02X%02X%02X",
				*((unsigned char *)p_param + 3),
				*((unsigned char *)p_param + 2),
				*((unsigned char *)p_param + 1),
				*((unsigned char *)p_param));
			/* SW Main Version */
			ret += sprintf(buf + ret, "%02X%02X%02X",
				*((unsigned char *)p_param + 6),
				*((unsigned char *)p_param + 5),
				*((unsigned char *)p_param + 4));
			/* SW Boot Version */
			ret += sprintf(buf + ret, "%02X",
				*((unsigned char *)p_param + 7));
			ret += sprintf(buf + ret, "\"\n");
		} else {
			ret += sprintf(buf + ret, "0000000000000000\"\n");
		}
	} else {
		ret += sprintf(buf + ret - 1, "\n");
	}
	unl_info("%s - ret : %d\n", __func__, ret);
	return ret;
}

static ssize_t hw_param_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	int index = 0;
	size_t ret = -ENOMEM;
	char *str = (char *)buf;
	unsigned long long *p_param = NULL;

	if (size > 2) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}
	ret = size;
	unl_info("%s : %s\n", __func__, str);
	if (!strncmp(str, "c", 1))
		for (index = 0; index < USB_CCIC_HW_PARAM_MAX; index++) {
			p_param = get_hw_param(n, index);
			if (p_param)
				*p_param = 0;
		}
error:
	return ret;
}
#endif

char interface_class_name[USB_CLASS_VENDOR_SPEC][4] = {
	[U_CLASS_PER_INTERFACE]				= {"PER"},
	[U_CLASS_AUDIO]					= {"AUD"},
	[U_CLASS_COMM]					= {"COM"},
	[U_CLASS_HID]					= {"HID"},
	[U_CLASS_PHYSICAL]				= {"PHY"},
	[U_CLASS_STILL_IMAGE]				= {"STI"},
	[U_CLASS_PRINTER]				= {"PRI"},
	[U_CLASS_MASS_STORAGE]				= {"MAS"},
	[U_CLASS_HUB]					= {"HUB"},
	[U_CLASS_CDC_DATA]				= {"CDC"},
	[U_CLASS_CSCID]					= {"CSC"},
	[U_CLASS_CONTENT_SEC]				= {"CON"},
	[U_CLASS_VIDEO]					= {"VID"},
	[U_CLASS_WIRELESS_CONTROLLER]			= {"WIR"},
	[U_CLASS_MISC]					= {"MIS"},
	[U_CLASS_APP_SPEC]				= {"APP"},
	[U_CLASS_VENDOR_SPEC]				= {"VEN"}
};

void init_usb_whitelist_array(int *whitelist_array)
{
	int i;

	for (i = 1; i <= MAX_CLASS_TYPE_NUM; i++)
		whitelist_array[i] = 0;
}

void init_usb_whitelist_array_for_id(int *whitelist_array, int size)
{
	int i;

	for (i = 0; i < size; i++)
		whitelist_array[i] = 0;
}

int set_usb_allowlist_array(const char *buf, int *whitelist_array)
{
	int valid_class_count = 0;
	char *ptr = NULL;
	int i;
	char *source;

	source = (char *)buf;
	while ((ptr = strsep(&source, ":")) != NULL) {
		if (strlen(ptr) < 3)
			continue;
		unl_info("%s token = %c%c%c!\n", __func__,
			ptr[0], ptr[1], ptr[2]);
		for (i = U_CLASS_PER_INTERFACE; i <= U_CLASS_VENDOR_SPEC; i++) {
			if (!strncmp(ptr, interface_class_name[i], 3))
				whitelist_array[i] = 1;
		}
	}

	for (i = U_CLASS_PER_INTERFACE; i <= U_CLASS_VENDOR_SPEC; i++) {
		if (whitelist_array[i])
			valid_class_count++;
	}
	unl_info("%s valid_class_count = %d!\n", __func__, valid_class_count);
	return valid_class_count;
}

int set_usb_allowlist_array_for_id(const char *buf, int *whitelist_array)
{
	int valid_product_count = 0;
	int vid = 0, pid = 0, ret = 0;
	char *ptr_vid = NULL;
	char *ptr_pid = NULL;
	char *source;

	source = (char *)buf;
	while ((ptr_vid = strsep(&source, ":")) != NULL) {
		if (strlen(ptr_vid) < 4) {
			unl_err("%s short strlen(vid)\n", __func__);
			break;
		}

		ptr_pid = strsep(&source, ":");

		if (ptr_pid == NULL || strlen(ptr_pid) < 4) {
			unl_err("%s short strlen(pid)\n", __func__);
			break;
		}

		if (!ptr_vid[0] || !ptr_vid[1] || !ptr_vid[2] || !ptr_vid[3] ||
			!ptr_pid[0] || !ptr_pid[1] || !ptr_pid[2] || !ptr_pid[3])
			break;

		ret = kstrtoint(ptr_vid, 16, &vid);
		if (ret) {
			unl_err("%s ptr_vid error. ret %d\n", __func__, ret);
			break;
		}

		whitelist_array[valid_product_count] = vid;

		ret = kstrtoint(ptr_pid, 16, &pid);
		if (ret) {
			unl_err("%s ptr_pid error. ret %d\n", __func__, ret);
			break;
		}

		whitelist_array[valid_product_count+1] = pid;

		unl_info("%s : allowlist_array[%d]=%04x, allowlist_array[%d]=%04x\n",
				__func__, valid_product_count, whitelist_array[valid_product_count],
				valid_product_count+1, whitelist_array[valid_product_count+1]);

		valid_product_count += 2;
	}

	valid_product_count /= 2;

	unl_info("%s valid_product_count = %d!\n", __func__, valid_product_count);
	return valid_product_count;
}

static ssize_t whitelist_for_mdm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		return -EINVAL;
	}
	unl_info("allowlist_for_mdm read allowlist_classes %s\n",
		udev->whitelist_str);
	return sprintf(buf, "%s\n", udev->whitelist_str);
}

static ssize_t whitelist_for_mdm_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	char *disable;
	int sret;
	size_t ret = -ENOMEM;
	int mdm_disable;
	int valid_whilelist_count;

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		ret = -EINVAL;
		goto error;
	}

	if (size < 3) {
		unl_err("allowlist usage was wrong. The size(%zu) is too short.\n", size);
		goto error;
	}
	if (size < strlen(buf))
		goto error;
	
	disable = kzalloc(size+1, GFP_KERNEL);
	if (!disable)
		goto error;

	sret = sscanf(buf, "%s", disable);
	if (sret != 1)
		goto error1;

#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
	if (!strncmp(buf, "VPID:", ALLOWLIST_PREFIX_SIZE)) {

		unl_info("allowlist_for_mdm_store VID, PID buf=%s\n", disable);	

		if (size >= MAX_ALLOWLIST_BUFFER) {
			unl_err("allowlist_for_lockscreen size(%zu) is invalid.\n", size);
			goto error1;
		}

		mutex_lock(&udev->lockscreen_enabled_lock);
		init_usb_whitelist_array_for_id(udev->allowlist_array_lockscreen_enabled_id,
			MAX_ALLOWLIST_DEVICE_BUFFER_INDEX);
		
		valid_whilelist_count =	set_usb_allowlist_array_for_id
			(buf+ALLOWLIST_PREFIX_SIZE, udev->allowlist_array_lockscreen_enabled_id);	

		// for furture use ex:) show function
		strncpy(udev->allowlist_str_lockscreen_enabled_id,
			disable, sizeof(udev->allowlist_str_lockscreen_enabled_id)-1);
		mutex_unlock(&udev->lockscreen_enabled_lock);

		ret = size;

		unl_info("%s vpid allowlist update done!\n", __func__);
	} else {
#endif
		unl_info("allowlist_for_mdm_store interface buf=%s\n", disable);	

		/* To active displayport, hub class must be enabled */
		if (size > MAX_WHITELIST_STR_LEN) {
			unl_err("allowlist_for_mdm_store size(%zu) is invalid.\n", size);
			goto error1;
		}

		init_usb_whitelist_array(udev->whitelist_array_for_mdm);

		if (!strncmp(buf, "ABL", 3)) {
			udev->whitelist_array_for_mdm[U_CLASS_HUB] = 1;
			mdm_disable = NOTIFY_MDM_TYPE_ON;
		} else if (!strncmp(buf, "OFF", 3))
			mdm_disable = NOTIFY_MDM_TYPE_OFF;
		else {
			valid_whilelist_count = set_usb_allowlist_array
				(buf, udev->whitelist_array_for_mdm);
			if (valid_whilelist_count > 0) {
				udev->whitelist_array_for_mdm[U_CLASS_HUB] = 1;
				mdm_disable = NOTIFY_MDM_TYPE_ON;
			} else
				mdm_disable = NOTIFY_MDM_TYPE_OFF;
		}

		strncpy(udev->whitelist_str,
		disable, sizeof(udev->whitelist_str)-1);

		if (udev->set_mdm) {
			udev->set_mdm(udev, mdm_disable);
			ret = size;
		} else {
			unl_err("set_mdm func is NULL\n");
			ret = -EINVAL;
		}
#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
	}
#endif
error1:
	kfree(disable);
error:
	return ret;
}

static ssize_t whitelist_for_disa_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		return -EINVAL;
	}
	unl_info("%s read allowlist_classes %s\n",
		__func__, udev->whitelist_str_for_id);
	return sprintf(buf, "%s\n", udev->whitelist_str_for_id);
}

static ssize_t whitelist_for_disa_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	char *disable;
	int sret;
	size_t ret = -ENOMEM;
	int mdm_disable;
	int valid_whilelist_count;

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		ret = -EINVAL;
		goto error;
	}

	if (size > MAX_WHITELIST_STR_LEN) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	if (size < strlen(buf))
		goto error;
	disable = kzalloc(size+1, GFP_KERNEL);
	if (!disable)
		goto error;

	sret = sscanf(buf, "%s", disable);
	if (sret != 1)
		goto error1;
	unl_info("allowlist_for_disa_store buf=%s\n", disable);

	init_usb_whitelist_array_for_id(udev->whitelist_array_for_mdm_for_id, MAX_WHITELIST_STR_LEN);
	/* To active displayport, hub class must be enabled */
	if (!strncmp(buf, "OFF", 3)) {
		unl_info("%s OFF\n", __func__);
		mdm_disable = NOTIFY_MDM_TYPE_OFF;
	} else {
		unl_info("%s ALLOWLIST\n", __func__);
		valid_whilelist_count =	set_usb_allowlist_array_for_id
			(buf, udev->whitelist_array_for_mdm_for_id);
		if (valid_whilelist_count > 0)
			mdm_disable = NOTIFY_MDM_TYPE_ON;
		else
			mdm_disable = NOTIFY_MDM_TYPE_OFF;
	}

	strncpy(udev->whitelist_str_for_id,
		disable, sizeof(udev->whitelist_str_for_id)-1);

	if (udev->set_mdm_for_id) {
		udev->set_mdm_for_id(udev, mdm_disable);
		ret = size;
	} else {
		unl_err("set_mdm_for_id func is NULL\n");
		ret = -EINVAL;
	}
error1:
	kfree(disable);
error:
	return ret;
}

static ssize_t whitelist_for_serial_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		return -EINVAL;
	}
	unl_info("%s read allowlist_classes %s\n",
		__func__, udev->whitelist_array_for_mdm_for_serial);
	return sprintf(buf, "%s\n", udev->whitelist_array_for_mdm_for_serial);
}

static ssize_t whitelist_for_serial_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	char *disable;
	int sret;
	size_t ret = -ENOMEM;
	int mdm_disable;

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		ret = -EINVAL;
		goto error;
	}

	if (size > MAX_WHITELIST_STR_LEN) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	if (size < strlen(buf))
		goto error;
	disable = kzalloc(size+1, GFP_KERNEL);
	if (!disable)
		goto error;

	sret = sscanf(buf, "%s", disable);
	if (sret != 1)
		goto error1;
	unl_info("allowlist_for_serial_store buf=%s\n", disable);

	strncpy(udev->whitelist_array_for_mdm_for_serial,
		disable, sizeof(udev->whitelist_array_for_mdm_for_serial)-1);

	/* To active displayport, hub class must be enabled */
	if (!strncmp(buf, "OFF", 3)) {
		unl_info("%s OFF\n", __func__);
		mdm_disable = NOTIFY_MDM_TYPE_OFF;
	} else {
		unl_info("%s ALLOWLIST\n", __func__);
		mdm_disable = NOTIFY_MDM_TYPE_ON;
	}

	if (udev->set_mdm_for_serial) {
		udev->set_mdm_for_serial(udev, mdm_disable);
		ret = size;
	} else {
		unl_err("set_mdm_for_serial func is NULL\n");
		ret = -EINVAL;
	}
error1:
	kfree(disable);
error:
	return ret;
}

static ssize_t usb_request_action_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		return -EINVAL;
	}
	unl_info("%s request_action = %u\n",
		__func__, udev->request_action);

	return sprintf(buf, "%u\n", udev->request_action);
}

static ssize_t usb_request_action_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)

{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	unsigned int request_action = 0;
	int sret = -EINVAL;
	size_t ret = -ENOMEM;

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		return -EINVAL;
	}
	if (size > PAGE_SIZE) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	sret = sscanf(buf, "%u", &request_action);
	if (sret != 1)
		goto error;

	udev->request_action = request_action;

	unl_info("%s request_action = %d\n",
		__func__, udev->request_action);
	ret = size;

error:
	return ret;
}

static ssize_t cards_show(
	struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	char card_strings[MAX_CARD_STR_LEN] = {0,};
	char buf_card[15] = {0,};
	int i;
	int cnt = 0;

	for (i = 0; i < MAX_USB_AUDIO_CARDS; i++) {
		if (udev->usb_audio_cards[i].cards) {
			cnt += snprintf(buf_card, sizeof(buf_card),
				"<%scard%d>",
				udev->usb_audio_cards[i].bundle ? "*" : "", i);
			if (cnt < 0) {
				unl_err("%s snprintf return %d\n",
						__func__, cnt);
				continue;
			}
			if (cnt >= MAX_CARD_STR_LEN) {
				unl_err("%s overflow\n", __func__);
				goto err;
			}
			strlcat(card_strings, buf_card, sizeof(card_strings));
		}
	}
err:
	unl_info("card_strings %s\n", card_strings);
	return sprintf(buf, "%s\n", card_strings);
}

int usb_notify_dev_uevent(struct usb_notify_dev *udev, char *envp_ext[])
{
	int ret = 0;

	if (!udev || !udev->dev) {
		unl_err("%s udev or udev->dev NULL\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	if (strncmp("TYPE", envp_ext[0], 4)) {
		unl_err("%s error.first array must be filled TYPE\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	if (strncmp("STATE", envp_ext[1], 5)) {
		unl_err("%s error.second array must be filled STATE\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	kobject_uevent_env(&udev->dev->kobj, KOBJ_CHANGE, envp_ext);
	unl_info("%s\n", __func__);

err:
	return ret;
}
EXPORT_SYMBOL_GPL(usb_notify_dev_uevent);

#if defined(CONFIG_USB_LPM_CHARGING_SYNC)
static ssize_t lpm_charging_type_done_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	if (udev == NULL) {
		pr_err("udev is NULL\n");
		return -EINVAL;
	}

	return sprintf(buf, "%u\n", udev->lpm_charging_type_done);
}
#endif

#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
static const char *lock_string(enum usb_lock_state lock_state)
{
	switch (lock_state) {
	case USB_NOTIFY_INIT_STATE:
		return "init";
	case USB_NOTIFY_UNLOCK:
		return "unlock";
	case USB_NOTIFY_LOCK_USB_WORK:
		return "usb work lock";
	case USB_NOTIFY_LOCK_USB_RESTRICT:
		return "usb restrict lock";
	default:
		return "undefined";
	}
}
#endif

static ssize_t usb_sl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		return -EINVAL;
	}
	unl_info("%s secure_lock = %lu\n",
		__func__, udev->secure_lock);

	return sprintf(buf, "%lu\n", udev->secure_lock);
}

static bool valid_secure_lock_value(unsigned long secure_lock)
{
	bool ret = false;

	switch (secure_lock) {
	case USB_NOTIFY_LOCK_USB_RESTRICT:
	case USB_NOTIFY_LOCK_USB_WORK:
	case USB_NOTIFY_UNLOCK:
		ret = true;
		break;
	default:
		break;
	}

	return ret;
}

static ssize_t usb_sl_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)

{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	unsigned long secure_lock = 0;
#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
	unsigned long prev_secure_lock = 0;
#endif
	int sret = -EINVAL;
	size_t ret = -ENOMEM;

	if (udev == NULL) {
		unl_err("udev is NULL\n");
		return -EINVAL;
	}
	if (size > PAGE_SIZE) {
		unl_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
	unl_info("%s before secure_lock = %s first_restrict = %d +\n",
		__func__, lock_string(udev->secure_lock), udev->first_restrict);
#else
	unl_info("%s before secure_lock = %lu +\n",
		__func__, udev->secure_lock);
#endif

	sret = sscanf(buf, "%lu", &secure_lock);
	if (sret != 1)
		goto error;

	if (!valid_secure_lock_value(secure_lock)) {
		unl_err("%s secure_lock is invalid (%lu)\n",
			__func__, secure_lock);
		ret = -EINVAL;
		goto error;
	}

#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
	prev_secure_lock = udev->secure_lock;
#endif
	udev->secure_lock = secure_lock;
	udev->set_lock_state(udev);

#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
	if (prev_secure_lock == USB_NOTIFY_INIT_STATE
			&& secure_lock == USB_NOTIFY_LOCK_USB_RESTRICT) {
		if (udev->set_disable) {
			udev->set_disable(udev, NOTIFY_BLOCK_TYPE_ALL);
			udev->first_restrict = true;
		}
	} else if (udev->first_restrict
					&& (secure_lock == USB_NOTIFY_UNLOCK
							|| secure_lock == USB_NOTIFY_LOCK_USB_WORK)) {
		if (udev->set_disable) {
			udev->set_disable(udev, NOTIFY_BLOCK_TYPE_NONE);
			udev->first_restrict = false;
		}
	}

	unl_info("%s after secure_lock = %s -\n",
		__func__, lock_string(udev->secure_lock));
#else
	unl_info("%s after secure_lock = %lu -\n",
		__func__, udev->secure_lock);
#endif
	ret = size;

error:
	return ret;
}

static DEVICE_ATTR_RW(disable);
static DEVICE_ATTR_RW(usb_data_enabled);
static DEVICE_ATTR_RO(support);
static DEVICE_ATTR_RO(otg_speed);
static DEVICE_ATTR_RO(gadget_speed);
static DEVICE_ATTR_RW(usb_maximum_speed);
static DEVICE_ATTR_RW(whitelist_for_mdm);
static DEVICE_ATTR_RW(whitelist_for_disa);
static DEVICE_ATTR_RW(whitelist_for_serial);
static DEVICE_ATTR_RO(cards);
#if defined(CONFIG_USB_HW_PARAM)
static DEVICE_ATTR_RW(usb_hw_param);
static DEVICE_ATTR_RW(hw_param);
#endif
static DEVICE_ATTR_RW(usb_request_action);
#if defined(CONFIG_USB_LPM_CHARGING_SYNC)
static DEVICE_ATTR_RO(lpm_charging_type_done);
#endif
static DEVICE_ATTR_RW(usb_sl);

static struct attribute *usb_notify_attrs[] = {
	&dev_attr_disable.attr,
	&dev_attr_usb_data_enabled.attr,
	&dev_attr_support.attr,
	&dev_attr_otg_speed.attr,
	&dev_attr_gadget_speed.attr,
	&dev_attr_usb_maximum_speed.attr,
	&dev_attr_whitelist_for_mdm.attr,
	&dev_attr_whitelist_for_disa.attr,
	&dev_attr_whitelist_for_serial.attr,
	&dev_attr_cards.attr,
#if defined(CONFIG_USB_HW_PARAM)
	&dev_attr_usb_hw_param.attr,
	&dev_attr_hw_param.attr,
#endif
	&dev_attr_usb_request_action.attr,
#if defined(CONFIG_USB_LPM_CHARGING_SYNC)
	&dev_attr_lpm_charging_type_done.attr,
#endif
	&dev_attr_usb_sl.attr,
	NULL,
};

static struct attribute_group usb_notify_attr_grp = {
	.attrs = usb_notify_attrs,
};

static int create_usb_notify_class(void)
{
	if (!usb_notify_data.usb_notify_class) {
		usb_notify_data.usb_notify_class
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
			= class_create("usb_notify");
#else
			= class_create(THIS_MODULE, "usb_notify");
#endif
		if (IS_ERR(usb_notify_data.usb_notify_class))
			return PTR_ERR(usb_notify_data.usb_notify_class);
		atomic_set(&usb_notify_data.device_count, 0);
	}

	return 0;
}

int usb_notify_dev_register(struct usb_notify_dev *udev)
{
	int ret;

	if (!usb_notify_data.usb_notify_class) {
		ret = create_usb_notify_class();
		if (ret < 0)
			return ret;
	}

#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
	mutex_init(&udev->lockscreen_enabled_lock);
#endif

	udev->index = atomic_inc_return(&usb_notify_data.device_count);
	udev->dev = device_create(usb_notify_data.usb_notify_class, NULL,
		MKDEV(0, udev->index), NULL, "%s", udev->name);
	if (IS_ERR(udev->dev))
		return PTR_ERR(udev->dev);

	udev->disable_state = 0;
	udev->usb_data_enabled = 1;
	strncpy(udev->disable_state_cmd, "OFF",
			sizeof(udev->disable_state_cmd)-1);
	dev_set_drvdata(udev->dev, udev);

	ret = sysfs_create_group(&udev->dev->kobj, &usb_notify_attr_grp);
	if (ret < 0) {
		device_destroy(usb_notify_data.usb_notify_class,
				MKDEV(0, udev->index));
		return ret;
	}
	kobject_uevent(&udev->dev->kobj, KOBJ_CHANGE);

	return 0;
}
EXPORT_SYMBOL_GPL(usb_notify_dev_register);

void usb_notify_dev_unregister(struct usb_notify_dev *udev)
{
	sysfs_remove_group(&udev->dev->kobj, &usb_notify_attr_grp);
	device_destroy(usb_notify_data.usb_notify_class, MKDEV(0, udev->index));
	dev_set_drvdata(udev->dev, NULL);
}
EXPORT_SYMBOL_GPL(usb_notify_dev_unregister);

int usb_notify_class_init(void)
{
	return create_usb_notify_class();
}
EXPORT_SYMBOL_GPL(usb_notify_class_init);

void usb_notify_class_exit(void)
{
	class_destroy(usb_notify_data.usb_notify_class);
}
EXPORT_SYMBOL_GPL(usb_notify_class_exit);

