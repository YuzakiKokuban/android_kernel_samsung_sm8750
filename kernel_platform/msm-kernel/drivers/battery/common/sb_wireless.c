/*
 *  sb_wireless.c
 *  Samsung Mobile Battery Wireless Module
 *
 *  Copyright (C) 2023 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>

#include <linux/battery/sb_sysfs.h>
#include <linux/battery/sb_notify.h>
#include <linux/battery/sb_wireless.h>

#define sbw_log(str, ...) pr_info("[SB-WIRELESS]:%s: "str, __func__, ##__VA_ARGS__)
#define SBW_MODULE_NAME	"sb-wireless"

const char *sb_wrl_op_mode_str(int op_mode)
{
	switch (op_mode) {
	case WPC_OP_MODE_PPDE:
		return "PPDE";
	case WPC_OP_MODE_EPP:
		return "EPP";
	case WPC_OP_MODE_MPP:
		return "MPP";
	case WPC_OP_MODE_BPP:
		return "BPP";
	}

	return "Unknown";
}
EXPORT_SYMBOL(sb_wrl_op_mode_str);

struct sb_wireless {
	struct notifier_block nb;

	const struct sb_wireless_op *op;
	void *pdata;
};

static struct sb_wireless *get_inst(void)
{
	static struct sb_wireless *sbw;

	if (sbw)
		return sbw;

	sbw = kzalloc(sizeof(struct sb_wireless), GFP_KERNEL);
	return sbw;
}

static bool check_valid_op(const struct sb_wireless_op *op)
{
	return (op != NULL) &&
		(op->get_op_mode != NULL) &&
		(op->get_qi_ver != NULL) &&
		(op->get_auth_mode != NULL);
}

static int get_op_mode(void)
{
	struct sb_wireless *sbw = get_inst();

	if (!sbw)
		return -ENOMEM;

	if (!check_valid_op(sbw->op))
		return -ENODEV;

	return sbw->op->get_op_mode(sbw->pdata);
}

static int get_qi_ver(void)
{
	struct sb_wireless *sbw = get_inst();

	if (!sbw)
		return -ENOMEM;

	if (!check_valid_op(sbw->op))
		return -ENODEV;

	return sbw->op->get_qi_ver(sbw->pdata);
}

static int get_auth_mode(void)
{
	struct sb_wireless *sbw = get_inst();

	if (!sbw)
		return -ENOMEM;

	if (!check_valid_op(sbw->op))
		return -ENODEV;

	return sbw->op->get_auth_mode(sbw->pdata);
}

int sb_wireless_set_op(void *pdata, const struct sb_wireless_op *op)
{
	struct sb_wireless *sbw = get_inst();

	if (!sbw)
		return -ENOMEM;

	if (check_valid_op(sbw->op))
		return -EBUSY;

	if (!pdata || !check_valid_op(op))
		return -EINVAL;

	sbw->pdata = pdata;
	sbw->op = op;
	return 0;
}
EXPORT_SYMBOL(sb_wireless_set_op);

static ssize_t
sb_wireless_show_attrs(struct device *, struct device_attribute *, char *);
static ssize_t
sb_wireless_store_attrs(struct device *, struct device_attribute *, const char *, size_t);

#define SB_WIRELESS_ATTR(_name)	\
{	\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sb_wireless_show_attrs,	\
	.store = sb_wireless_store_attrs,	\
}

enum {
	WPC_OP_MODE = 0,
	WPC_QI_VER,
	WPC_AUTH_MODE,
};

static struct device_attribute sb_wireless_attrs[] = {
	SB_WIRELESS_ATTR(wpc_op_mode),
	SB_WIRELESS_ATTR(wpc_qi_ver),
	SB_WIRELESS_ATTR(wpc_auth_mode),
};

static ssize_t sb_wireless_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - sb_wireless_attrs;
	int i = 0;

	switch (offset) {
	case WPC_OP_MODE:
		i += scnprintf(buf, PAGE_SIZE, "%d\n", get_op_mode());
		break;
	case WPC_QI_VER:
		i += scnprintf(buf, PAGE_SIZE, "%d\n", get_qi_ver());
		break;
	case WPC_AUTH_MODE:
		i += scnprintf(buf, PAGE_SIZE, "%d\n", get_auth_mode());
		break;
	default:
		return -EINVAL;
	}

	return i;
}

static ssize_t sb_wireless_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - sb_wireless_attrs;

	switch (offset) {
	case WPC_OP_MODE:
		break;
	case WPC_QI_VER:
		break;
	case WPC_AUTH_MODE:
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static int sb_noti_handler(struct notifier_block *nb, unsigned long action, void *data)
{
	return 0;
}

static int __init sb_wireless_init(void)
{
	struct sb_wireless *sbw = get_inst();
	int ret = 0;

	if (!sbw)
		return -ENOMEM;

	ret = sb_sysfs_add_attrs(SBW_MODULE_NAME, sbw, sb_wireless_attrs, ARRAY_SIZE(sb_wireless_attrs));
	sbw_log("sb_sysfs_add_attrs ret = %s\n", (ret) ? "fail" : "success");

	ret = sb_notify_register(&sbw->nb, sb_noti_handler, SBW_MODULE_NAME, SB_DEV_MODULE);
	sbw_log("sb_notify_register ret = %s\n", (ret) ? "fail" : "success");

	return ret;
}
module_init(sb_wireless_init);

MODULE_DESCRIPTION("Samsung Battery Wireless");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");