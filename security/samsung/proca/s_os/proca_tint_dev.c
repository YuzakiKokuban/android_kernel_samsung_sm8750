// SPDX-License-Identifier: GPL-2.0-only
/*
 * PROCA task integrity
 *
 * Copyright (C) 2020 Samsung Electronics, Inc.
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 * Viacheslav Vovchenko <v.vovchenko@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/sched/task.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/compat.h>

#include "gki/task_integrity.h"
#include "proca_tint_dev.h"
#include "proca.h"

struct tint_message {
	uint32_t version;
	uint32_t param;
	uint64_t reserved;
	union __packed {
		enum task_integrity_value value;
		struct __packed {
			uint64_t i_ino;
			enum task_integrity_reset_cause cause;
			struct __packed {
				uint64_t pathname_ptr;
				uint16_t len_buf;
			} path;
		} reset_file;
		struct __packed {
			uint64_t cert_ptr;
			uint16_t cert_len;
		} cert;
	} data;
} __packed;

#define TINT_VERSION 2
#define TINT_DEV		"task_integrity"
#define TINT_IOC_MAGIC		'f'
#define TINT_IOCTL_GET_VALUE \
	_IOWR(TINT_IOC_MAGIC, 1, struct tint_message)
#define TINT_IOCTL_GET_RESET_FILE \
	_IOWR(TINT_IOC_MAGIC, 2, struct tint_message)
#define TINT_IOCTL_GET_CERTIFICATE \
	_IOWR(TINT_IOC_MAGIC, 3, struct tint_message)

static struct tint_control {
	struct device *pdev;
	struct class *driver_class;
	dev_t device_no;
	struct cdev cdev;
} dev_ctrl;

static struct task_struct *get_task_by_pid(pid_t pid)
{
	struct task_struct *task = NULL;
	struct pid *pid_data;

	pid_data = find_get_pid(pid);
	if (unlikely(!pid_data)) {
		pr_err("PROCA: Can't find PID: %u\n", pid);
		return NULL;
	}

	task = get_pid_task(pid_data, PIDTYPE_PID);
	if (unlikely(!task))
		pr_err("PROCA: Can't find task by PID: %u\n", pid);

	put_pid(pid_data);

	return task;
}

static struct task_struct *get_tint_and_task(pid_t pid)
{
	struct task_struct *task = get_task_by_pid(pid);

	if (!task)
		return NULL;

	task_integrity_get(TASK_INTEGRITY(task));

	return task;
}

static void put_tint_and_task(struct task_struct *task)
{
	if (!task)
		return;

	task_integrity_put(TASK_INTEGRITY(task));
	put_task_struct(task);
}

static long tint_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct tint_message __user *argp = (void __user *) arg;
	struct task_struct *task;
	struct tint_message msg = {0};

	if (_IOC_TYPE(cmd) != TINT_IOC_MAGIC) {
		pr_err("PROCA: IOCTL type is wrong\n");
		return -ENOTTY;
	}

	ret = copy_from_user(&msg, argp, sizeof(msg));
	if (ret) {
		pr_err("PROCA: copy_from_user failed ret: %ld\n", ret);
		return -EFAULT;
	}

	if (msg.version != TINT_VERSION) {
		pr_err("PROCA: Unsupported protocol version: %u\n", msg.version);
		return -EINVAL;
	}

	task = get_tint_and_task(msg.param);
	if (!task) {
		pr_err("PROCA: could not get task\n");
		return -ESRCH;
	}

	if (!TASK_INTEGRITY(task)) {
		pr_err("PROCA: TASK_INTEGRITY failed\n");
		ret = -ENOENT;
		goto out;
	}

	switch (cmd) {
	case TINT_IOCTL_GET_VALUE: {
		msg.data.value = task_integrity_user_read(TASK_INTEGRITY(task));

		ret = copy_to_user(
			(void __user *) &argp->data.value, &msg.data.value,
			sizeof(msg.data.value));
		if (unlikely(ret)) {
			pr_err("PROCA: copy_to_user failed: %u %ld\n",
				cmd, ret);
			ret = -EFAULT;
		}

		break;
	}
	case TINT_IOCTL_GET_RESET_FILE: {
		const struct file *reset_file;
		const struct inode *inode;
		uint16_t len_buf;
		char *buf, *pathname;

		reset_file = TASK_INTEGRITY(task)->reset_file;
		if (!reset_file) {
			ret = -ENOENT;
			break;
		}

		inode = file_inode(reset_file);
		msg.data.reset_file.i_ino = inode->i_ino;
		msg.data.reset_file.cause = TASK_INTEGRITY(task)->reset_cause;
		len_buf = msg.data.reset_file.path.len_buf;

		if (!len_buf || len_buf > PAGE_SIZE) {
			pr_err("PROCA: Bad size of user buffer: %u %u",
				cmd, len_buf);
			ret = -EINVAL;
			break;
		}

		buf = kmalloc(len_buf, GFP_KERNEL);
		if (unlikely(!buf)) {
			pr_err("PROCA: Can't allocate memory: %u %u",
				cmd, len_buf);
			ret = -ENOMEM;
			break;
		}

		pathname = d_path(&reset_file->f_path, buf, len_buf);
		if (IS_ERR(pathname)) {
			pr_err("PROCA: Can't obtain path: %u", len_buf);
			ret = -ENOMEM;
			kfree(buf);
			break;
		}

		len_buf = strnlen(pathname, len_buf) + 1;

		ret = copy_to_user(
			(void __user *) msg.data.reset_file.path.pathname_ptr,
			pathname, len_buf);
		if (unlikely(ret)) {
			pr_err("PROCA: copy_to_user failed path: %u %ld\n",
				cmd, ret);
			ret = -EFAULT;
			kfree(buf);
			break;
		}

		ret = copy_to_user((void __user *) &argp->data.reset_file,
				   &msg.data.reset_file,
				   sizeof(msg.data.reset_file));

		if (unlikely(ret)) {
			pr_err("PROCA: copy_to_user failed: %u %ld\n",
				cmd, ret);
			ret = -EFAULT;
		}

		kfree(buf);
		break;
	}
	case TINT_IOCTL_GET_CERTIFICATE: {
		const char *cert;
		size_t cert_size;

		ret = proca_get_task_cert(task, &cert, &cert_size);
		if (ret) {
			break;
		}

		if (cert_size > msg.data.cert.cert_len) {
			pr_err("PROCA: User certificate buffer too small\n");
			ret = -ENOSPC;
			break;
		}

		if (!access_ok((void __user *)msg.data.cert.cert_ptr, cert_size)) {
			pr_err("PROCA: User certificate buffer is not accessible\n");
			ret = -EACCES;
			break;
		}

		ret = copy_to_user(
			(void __user *) msg.data.cert.cert_ptr,
			cert, cert_size);
		if (unlikely(ret)) {
			pr_err("PROCA: copy_to_user cert failed: %u %ld\n",
				cmd, ret);
			ret = -EFAULT;
			break;
		}

		msg.data.cert.cert_len = cert_size;
		ret = copy_to_user((void __user *) &argp->data.cert.cert_len,
			&msg.data.cert.cert_len,
			sizeof(msg.data.cert.cert_len));
		if (unlikely(ret)) {
			pr_err("PROCA: copy_to_user failed cert_len: %u %ld\n",
				cmd, ret);
			ret = -EFAULT;
		}

		break;
	}
	default: {
		pr_err("PROCA: Invalid IOCTL command: %u\n", cmd);
		ret = -ENOIOCTLCMD;
	}
	}

out:
	put_tint_and_task(task);
	return ret;
}

#ifdef CONFIG_COMPAT
static long tint_compact_ioctl(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	return tint_ioctl(file, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static const struct file_operations tint_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = tint_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tint_compact_ioctl,
#endif
};

int __init proca_tint_init_dev(void)
{
	int rc = 0;

	rc = alloc_chrdev_region(&dev_ctrl.device_no, 0, 1, TINT_DEV);
	if (unlikely(rc < 0)) {
		pr_err("PROCA: alloc_chrdev_region failed %d\n", rc);
		return rc;
	}

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
	dev_ctrl.driver_class = class_create(TINT_DEV);
#else
	dev_ctrl.driver_class = class_create(THIS_MODULE, TINT_DEV);
#endif
	if (IS_ERR(dev_ctrl.driver_class)) {
		rc = PTR_ERR(dev_ctrl.driver_class);
		pr_err("PROCA: class_create failed %d\n", rc);
		goto exit_unreg_chrdev_region;
	}

	dev_ctrl.pdev = device_create(dev_ctrl.driver_class, NULL,
				      dev_ctrl.device_no, NULL, TINT_DEV);
	if (IS_ERR(dev_ctrl.pdev)) {
		rc = PTR_ERR(dev_ctrl.pdev);
		pr_err("PROCA: class_device_create failed %d\n", rc);
		goto exit_destroy_class;
	}

	cdev_init(&dev_ctrl.cdev, &tint_fops);
	dev_ctrl.cdev.owner = THIS_MODULE;

	rc = cdev_add(&dev_ctrl.cdev,
		      MKDEV(MAJOR(dev_ctrl.device_no), 0), 1);
	if (unlikely(rc < 0)) {
		pr_err("PROCA: cdev_add failed %d\n", rc);
		goto exit_destroy_device;
	}

	return 0;

exit_destroy_device:
	device_destroy(dev_ctrl.driver_class, dev_ctrl.device_no);
exit_destroy_class:
	class_destroy(dev_ctrl.driver_class);
exit_unreg_chrdev_region:
	unregister_chrdev_region(dev_ctrl.device_no, 1);

	return rc;
}

void __exit proca_tint_deinit_dev(void)
{
	cdev_del(&dev_ctrl.cdev);
	device_destroy(dev_ctrl.driver_class, dev_ctrl.device_no);
	class_destroy(dev_ctrl.driver_class);
	unregister_chrdev_region(dev_ctrl.device_no, 1);
}
