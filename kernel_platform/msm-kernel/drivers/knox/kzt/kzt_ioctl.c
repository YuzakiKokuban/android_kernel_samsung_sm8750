// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *		http://www.samsung.com
 *
 */
#include <linux/errno.h>

#include "kzt_main.h"
#include "kzt_common.h"

static long kzt_ioctl_get_version(struct file *filp, void __user *uarg)
{
	struct kzt_get_version_arg arg;

	if (copy_from_user(&arg, uarg, sizeof(arg)))
		return -EFAULT;

	arg.version = KZT_KERNEL_VERSION;

	if (copy_to_user(uarg, &arg, sizeof(arg)))
		return -EFAULT;

	return 0;
}

static long kzt_ioctl_get_offsets(struct file *filp, void __user *uarg)
{
	struct kzt_get_offsets_arg arg;
	int err;

	if (copy_from_user(&arg, uarg, sizeof(arg)))
		return -EFAULT;

	err = kzt_offset_get_offsets(&arg);

	if (!err && copy_to_user(uarg, &arg, sizeof(arg)))
		err = -EFAULT;

	return err;
}

static long __kzt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case KZT_IOCTL_GET_VERSION:
		return kzt_ioctl_get_version(filp, (void __user *)arg);
	case KZT_IOCTL_GET_OFFSETS:
		return kzt_ioctl_get_offsets(filp, (void __user *)arg);
	default:
		return -ENOTTY;
	}
}

long kzt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return __kzt_ioctl(filp, cmd, arg);
}
