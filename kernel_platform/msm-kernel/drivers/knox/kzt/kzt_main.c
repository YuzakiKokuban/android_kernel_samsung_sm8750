// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *		http://www.samsung.com
 *
 */
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>

#include "kzt_main.h"

#define KZT_DEVICE_NAME "kzt"

#define kzt_debug_level_test(level) (kzt_debug_level >= level)

const struct file_operations kzt_dev_operations = {
		.owner = THIS_MODULE,
		.unlocked_ioctl = kzt_ioctl,
		.compat_ioctl = kzt_ioctl,
};

static struct miscdevice kzt_miscdevice = {
		.minor = MISC_DYNAMIC_MINOR,
		.name  = KZT_DEVICE_NAME,
		.fops = &kzt_dev_operations,
};

static const char *level_to_str(int level)
{
	switch (level) {
	case KZT_ERR:
		return "E";
	case KZT_INFO:
		return "I";
	case KZT_DEBUG:
		return "D";
	case KZT_VERBOSE:
		return "V";
	default:
		return "X";
	}
}

void kzt_msg(const int level, const char *fmt, ...)
{
	if (kzt_debug_level_test(level)) {
		struct va_format vaf;
		va_list args;

		va_start(args, fmt);
		vaf.fmt = fmt;
		vaf.va = &args;

		switch (level) {
		case KZT_ERR:
			pr_err("knox-zt: %pV\n", &vaf);
			break;
		case KZT_INFO:
			pr_info("knox-zt: %pV\n", &vaf);
			break;
		case KZT_DEBUG:
		default:
			pr_debug("knox-zt(%s): %pV\n", level_to_str(level), &vaf);
			break;
		}
		va_end(args);
	}
}

unsigned int kzt_debug_level = DEFAULT_KZT_DEBUG_LEVEL;
module_param_named(debug_level, kzt_debug_level, uint, 0600);
MODULE_PARM_DESC(debug_level, "kzt module's debug_level");

static int __init init_kzt(void)
{
	int err = misc_register(&kzt_miscdevice);

	if (!err)
		kzt_info("Knox Zero Trust initialized");

	return err;
}

late_initcall(init_kzt);

MODULE_DESCRIPTION("Knox Zero Trust");
MODULE_LICENSE("GPL");

