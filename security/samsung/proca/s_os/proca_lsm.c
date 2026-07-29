// SPDX-License-Identifier: GPL-2.0
/*
 * PROCA LSM module
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Ivan Vorobiov, <i.vorobiov@samsung.com>
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
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#if defined(CONFIG_PROCA_GKI_20)
#include <trace/hooks/security.h>
#include "gki/task_integrity.h"
#include "proca.h"
#else
#include <linux/proca.h>
#include <linux/task_integrity.h>
#endif

#include "five_hooks.h"
#include "proca_audit.h"
#include "proca_identity.h"
#include "proca_certificate.h"
#include "proca_task_descr.h"
#include "proca_table.h"
#include "proca_log.h"
#include "proca_config.h"
#include "proca_porting.h"
#include "proca_storage.h"
#include "proca_tint_dev.h"
#include "gaf/proca_gaf.h"

#define PROCA_DEV_NAME "proca_config"

static void proca_task_free_hook(struct task_struct *task);
const char *task_integrity_reset_str(enum task_integrity_reset_cause cause);
#if defined(LINUX_LSM_SUPPORTED) && !defined(CONFIG_PROCA_GKI_20)
static struct security_hook_list proca_ops[] = {
	LSM_HOOK_INIT(task_free, proca_task_free_hook),
};
#endif

static void proca_hook_task_forked(struct task_struct *parent,
				enum task_integrity_value parent_tint_value,
				struct task_struct *child,
				enum task_integrity_value child_tint_value);

static void proca_hook_file_processed(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file, void *cert,
				size_t cert_size, int result);

static void proca_hook_file_signed(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file, void *cert,
				size_t cert_size, int result);

static void proca_hook_file_skipped(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file);

static void proca_hook_reset_integrity(struct task_struct *task,
				struct file *file,
				enum task_integrity_reset_cause cause);

static struct five_hook_list five_ops[] = {
	FIVE_HOOK_INIT(task_forked, proca_hook_task_forked),
	FIVE_HOOK_INIT(file_processed, proca_hook_file_processed),
	FIVE_HOOK_INIT(file_signed, proca_hook_file_signed),
	FIVE_HOOK_INIT(file_skipped, proca_hook_file_skipped),
	FIVE_HOOK_INIT(integrity_reset2, proca_hook_reset_integrity),
};
static struct proca_table *g_proca_table;
struct proca_config *g_proca_config;
static dev_t proca_dev;
static struct cdev proca_cdev;
static struct class *proca_class;

static int g_proca_inited;

static const char __UNIQUE_ID(built_with)[] __used __section(".modinfo") __aligned(1) = "built_with=DDK";

static struct proca_task_descr *prepare_proca_task_descr(
				struct task_struct *task, struct file *file,
				const enum task_integrity_value tint_value)
{
	struct proca_certificate parsed_cert;
	struct proca_identity ident;
	char *cert_buff = NULL;
	int cert_size;
	struct proca_task_descr *task_descr = NULL;

	cert_size = proca_get_certificate(file, &cert_buff);
	if (!cert_buff)
		return NULL;

	if (parse_proca_certificate(cert_buff, cert_size,
				    &parsed_cert))
		goto cert_buff_cleanup;

	if (!is_certificate_relevant_to_task(&parsed_cert, task)) {
		PROCA_DEBUG_LOG(
			"Certificate %s doesn't relate to task.\n",
			parsed_cert.app_name);
		goto proca_cert_cleanup;
	}

	PROCA_DEBUG_LOG("PROCA certificate was found for task %d\n",
			task->pid);

	if (!is_certificate_relevant_to_file(&parsed_cert, file)) {
		PROCA_DEBUG_LOG(
			"Certificate %s doesn't relate to file.\n",
			parsed_cert.app_name);
		goto proca_cert_cleanup;
	}

	if (init_proca_identity(&ident, file,
			&cert_buff, cert_size,
			&parsed_cert))
		goto proca_cert_cleanup;

	task_descr = create_proca_task_descr(task, &ident);
	if (!task_descr)
		goto proca_identity_cleanup;

	return task_descr;

proca_identity_cleanup:;
	deinit_proca_identity(&ident);

proca_cert_cleanup:;
	deinit_proca_certificate(&parsed_cert);

cert_buff_cleanup:;
	kfree(cert_buff);

	return NULL;
}

static bool is_bprm(struct task_struct *task, struct file *old_file,
				struct file *new_file)
{
	struct file *exe;
	bool res;

	exe = get_task_exe_file(task);
	if (!exe)
		return false;

	res = locks_inode(exe) == locks_inode(new_file) &&
		locks_inode(old_file) != locks_inode(new_file);

	fput(exe);
	return res;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
static struct file *get_real_file(struct file *file)
{
	if (locks_inode(file)->i_sb->s_magic == OVERLAYFS_SUPER_MAGIC &&
					file->private_data)
		file = (struct file *)file->private_data;

	return file;
}
#endif

static void proca_hook_file_processed(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file, void *cert,
				size_t cert_size, int result)
{
	struct proca_task_descr *target_task_descr = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
	file = get_real_file(file);
	if (!file)
		return;
#endif

	if (task->flags & PF_KTHREAD)
		return;

	target_task_descr = proca_table_get_by_task(g_proca_table, task);
	if (target_task_descr &&
		is_bprm(task, target_task_descr->proca_identity.file, file)) {
		PROCA_DEBUG_LOG(
			"Task descr for task %d already exists before exec\n",
			task->pid);

		proca_table_remove_task_descr(g_proca_table,
					      target_task_descr);
		destroy_proca_task_descr(target_task_descr);
		target_task_descr = NULL;
	}

	if (!target_task_descr) {
		target_task_descr = prepare_proca_task_descr(
						task, file, tint_value);
		if (target_task_descr)
			if (proca_table_add_task_descr(g_proca_table,
						target_task_descr))
				destroy_proca_task_descr(target_task_descr);
	}
}

static void proca_hook_file_signed(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file, void *cert,
				size_t cert_size, int result)
{
	return;
}

static void proca_hook_file_skipped(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file)
{
	if (!task || !file)
		return;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
	file = get_real_file(file);
	if (!file)
		return;
#endif

	if (proca_is_certificate_present(file)) {

		// Workaround for Android applications.
		// If file has user.pa - check it.
		five_file_verify(task, file);
	}
}

static void proca_hook_task_forked(struct task_struct *parent,
			enum task_integrity_value parent_tint_value,
			struct task_struct *child,
			enum task_integrity_value child_tint_value)
{
	struct proca_task_descr *target_task_descr = NULL;
	struct proca_identity ident;

	if (!parent || !child)
		return;

	target_task_descr = proca_table_get_by_task(g_proca_table, parent);
	if (!target_task_descr)
		return;

	PROCA_DEBUG_LOG("Going to clone proca identity from task %d to %d\n",
			parent->pid, child->pid);

	if (proca_identity_copy(&ident, &target_task_descr->proca_identity))
		return;

	target_task_descr = create_proca_task_descr(child, &ident);
	if (!target_task_descr) {
		deinit_proca_identity(&ident);
		return;
	}

	if (proca_table_add_task_descr(g_proca_table, target_task_descr))
		destroy_proca_task_descr(target_task_descr);
}

static void proca_task_free_hook(struct task_struct *task)
{
	struct proca_task_descr *target_task_descr = NULL;

	target_task_descr = proca_table_remove_by_task(g_proca_table, task);

	destroy_proca_task_descr(target_task_descr);
}

static void proca_hook_reset_integrity(struct task_struct *task,
			struct file *file,
			enum task_integrity_reset_cause cause)
{
	if (proca_table_get_by_task(g_proca_table, task))
		proca_audit_err(task, file, "proca_reset_integrity",
				task_integrity_reset_str(cause));
}

#ifndef LINUX_LSM_SUPPORTED
void proca_compat_task_free_hook(struct task_struct *task)
{
	if (unlikely(!g_proca_inited))
		return;

	proca_task_free_hook(task);
}
#endif

int proca_get_task_cert(const struct task_struct *task,
			const char **cert, size_t *cert_size)
{
	struct proca_task_descr *task_descr = NULL;

	PROCA_BUG_ON(!task || !cert || !cert_size);

	task_descr = proca_table_get_by_task(g_proca_table, task);
	if (!task_descr)
		return -ESRCH;

	*cert = task_descr->proca_identity.certificate;
	*cert_size = task_descr->proca_identity.certificate_size;
	return 0;
}

#if defined(CONFIG_PROCA_GKI_20)
static void task_free_wrapper(void *data, struct task_struct *task)
{
	proca_task_free_hook(task);
}

static int __init register_vendor_hooks(void)
{
	int error = 0;

	error = register_trace_android_rvh_security_task_free(
						task_free_wrapper, NULL);
	return error;
}

#define proca_security_add_hooks(NOT_USED1, NOT_USED2, NOT_USED3)
#else
static inline int __init register_vendor_hooks(void)
{
	return 0;
}

#define proca_security_add_hooks(HOOKS, COUNT, LSM_ID) security_add_hooks(HOOKS, COUNT, LSM_ID)

#endif

static ssize_t proca_cdev_read(struct file *filp, char __user *buf, size_t count,
			loff_t *f_pos)
{
	phys_addr_t p = virt_to_phys(g_proca_config);

	if (!proca_table_get_by_task(g_proca_table, current) ||
		!task_integrity_user_read(TASK_INTEGRITY(current))) {
		PROCA_ERROR_LOG("Access to config is restricted.\n");
		return -EPERM;
	}

	return simple_read_from_buffer(buf, count, f_pos, &p, sizeof(p));
}

static const struct file_operations proca_cdev_fops = {
	.owner = THIS_MODULE,
	.read = proca_cdev_read,
};

static int init_proca_config_device(void)
{
	if ((alloc_chrdev_region(&proca_dev, 0, 1, PROCA_DEV_NAME)) < 0) {
		PROCA_ERROR_LOG("Cannot allocate major number\n");
		return -1;
	}

	proca_class = class_create(THIS_MODULE, PROCA_DEV_NAME);
	if (IS_ERR(proca_class)) {
		PROCA_ERROR_LOG("Cannot create class\n");
		goto region_cleanup;
	}

	cdev_init(&proca_cdev, &proca_cdev_fops);
	if ((cdev_add(&proca_cdev, proca_dev, 1)) < 0) {
		PROCA_ERROR_LOG("Cannot add the device to the system\n");
		goto class_cleanup;
	}

	if (!device_create(proca_class, NULL, proca_dev, NULL, PROCA_DEV_NAME)) {
		PROCA_ERROR_LOG("Cannot create device\n");
		goto device_cleanup;
	}

	PROCA_INFO_LOG("Config driver is inited.\n");
	return 0;

device_cleanup:
	cdev_del(&proca_cdev);

class_cleanup:
	class_destroy(proca_class);

region_cleanup:
	unregister_chrdev_region(proca_dev, 1);
	return -1;
}

static __init int proca_module_init(void)
{
	int ret;
	struct page *pages = NULL;

	ret = proca_init_gaf();
	if (ret)
		return ret;

	pages = alloc_pages(GFP_KERNEL | __GFP_ZERO, get_order(PAGE_SIZE * 2));
	if (!pages) {
		PROCA_ERROR_LOG("Failed to allocate memory for g_proca_config\n");
		return -ENOMEM;
	}

	g_proca_config = page_address(pages);

	g_proca_table = kzalloc(sizeof(*g_proca_table), GFP_KERNEL);
	if (unlikely(!g_proca_table)) {
		__free_pages(pages, get_order(PAGE_SIZE * 2));
		PROCA_ERROR_LOG("Cannot allocate table\n");
		return -ENOMEM;
	}

	ret = init_proca_config(g_proca_config, g_proca_table);
	if (ret)
		goto out;

	ret = init_proca_config_device();
	if (ret)
		goto out;

	ret = init_certificate_validation_hash();
	if (ret)
		goto out;

	ret = proca_table_init(g_proca_table);
	if (ret)
		goto out;

	ret = init_proca_storage();
	if (ret)
		goto out;

	ret = register_vendor_hooks();
	if (ret)
		return ret;

	proca_security_add_hooks(proca_ops, ARRAY_SIZE(proca_ops), proca_lsmid);
	five_add_hooks(five_ops, ARRAY_SIZE(five_ops));

	proca_task_descr_debugfs_init();

#ifdef CONFIG_PROCA_GKI_20
	ret = proca_tint_init_dev();
	if (ret)
		return ret;
#endif

	PROCA_INFO_LOG("LSM module was initialized\n");
	g_proca_inited = 1;

	return 0;

out:
	kfree(g_proca_table);
	__free_pages(pages, get_order(PAGE_SIZE * 2));
	return ret;
}
late_initcall(proca_module_init);

MODULE_DESCRIPTION("PROCA LSM module");
MODULE_LICENSE("GPL");

#if IS_MODULE(CONFIG_PROCA)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
