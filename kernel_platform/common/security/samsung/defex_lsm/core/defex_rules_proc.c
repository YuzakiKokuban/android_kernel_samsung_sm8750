// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/dcache.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/kobject.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/version.h>
#include "include/defex_internal.h"

#if KERNEL_VER_GTE(4, 12, 0)
#include <linux/mm.h>
#endif /* < KERNEL_VERSION(4, 12, 0) */

#include "include/defex_debug.h"
#include "include/defex_rules.h"
#include "include/defex_sign.h"
#include "include/defex_tree.h"
#ifdef DEFEX_TRUSTED_MAP_ENABLE
#include "include/defex_tailer.h"
#include "include/ptree.h"
#endif

#define LOAD_FLAG_DPOLICY		0x01
#if defined(DEFEX_SINGLE_RULES_FILE)
#define LOAD_FLAG_DPOLICY_SYSTEM	0x01
#else
#define LOAD_FLAG_DPOLICY_SYSTEM	0x02
#endif
#define LOAD_FLAG_SYSTEM_FIRST		0x04
#define LOAD_FLAG_TIMEOUT		0x08
#define LOAD_FLAG_RECOVERY		0x10

#define DEFEX_RULES_ARRAY_SIZE_MIN	64
#define DEFEX_RULES_ARRAY_SIZE_FIXED	(32 * 1024)
#define DEFEX_RULES_ARRAY_SIZE_MAX	(256 * 1024)


/*
 * Variant 1: Platform build, use static packed rules array
 */
#include "defex_packed_rules.inc"

#ifdef DEFEX_RAMDISK_ENABLE
/*
 * Variant 2: Platform build, load rules from kernel ramdisk or system partition
 */
#ifdef DEFEX_SIGN_ENABLE
#include "include/defex_sign.h"
#endif
#if (DEFEX_RULES_ARRAY_SIZE < 8)
#undef DEFEX_RULES_ARRAY_SIZE
#define DEFEX_RULES_ARRAY_SIZE	DEFEX_RULES_ARRAY_SIZE_MIN
#endif
#ifdef DEFEX_GKI
#undef DEFEX_RULES_ARRAY_SIZE
#define DEFEX_RULES_ARRAY_SIZE	DEFEX_RULES_ARRAY_SIZE_MAX
__visible_for_testing unsigned char *rules_primary_data;
#else
#if (DEFEX_RULES_ARRAY_SIZE < DEFEX_RULES_ARRAY_SIZE_FIXED)
#undef DEFEX_RULES_ARRAY_SIZE
#define DEFEX_RULES_ARRAY_SIZE	DEFEX_RULES_ARRAY_SIZE_FIXED
#endif
__visible_for_testing unsigned char rules_primary_data[DEFEX_RULES_ARRAY_SIZE] __ro_after_init;
#endif /* DEFEX_GKI */
__visible_for_testing unsigned char *rules_secondary_data;
#ifdef DEFEX_TRUSTED_MAP_ENABLE
struct PPTree dtm_tree;
#endif

struct d_tree rules_primary, rules_secondary;
#ifdef DEFEX_KUNIT_ENABLED
int rules_primary_size, rules_secondary_size;
#endif /* DEFEX_KUNIT_ENABLED */
#endif /* DEFEX_RAMDISK_ENABLE */

#ifdef DEFEX_TRUSTED_MAP_ENABLE
/* In loaded policy, title of DTM's section; set by tailer -t in
 *	buildscript/build_external/defex.
 */
#define DEFEX_DTM_SECTION_NAME "dtm_rules"
#endif

#ifdef DEFEX_INTEGRITY_ENABLE

#include <linux/fs.h>
#include <crypto/hash.h>
#include <crypto/public_key.h>
#include <crypto/internal/rsa.h>
#include "../../integrity/integrity.h"
#define SHA256_DIGEST_SIZE 32
#endif /* DEFEX_INTEGRITY_ENABLE */

struct rules_file_struct {
	char *name;
	int flags;
};

static const struct rules_file_struct rules_files[4] = {
	{ "/dpolicy",			LOAD_FLAG_DPOLICY },
	{ "/first_stage_ramdisk/dpolicy", LOAD_FLAG_DPOLICY },
	{ "/vendor/etc/dpolicy",	LOAD_FLAG_DPOLICY },
	{ "/dpolicy_system",		LOAD_FLAG_DPOLICY_SYSTEM }
};
static volatile unsigned int load_flags;
static DEFINE_SPINLOCK(rules_data_lock);

unsigned int get_load_flags(void)
{
	unsigned int data;

	spin_lock(&rules_data_lock);
	data = load_flags;
	spin_unlock(&rules_data_lock);
	return data;
}

static unsigned int update_load_flags(unsigned int new_flags)
{
	unsigned int data;

	spin_lock(&rules_data_lock);
	data = load_flags;
	data |= new_flags;
	load_flags = data;
	spin_unlock(&rules_data_lock);
	return data;
}

__visible_for_testing struct d_tree *get_rules_ptr(int is_system)
{
	struct d_tree *ptr;

	spin_lock(&rules_data_lock);
	if (load_flags & LOAD_FLAG_SYSTEM_FIRST)
		is_system = !is_system;
	ptr = is_system ? &rules_secondary : &rules_primary;
	spin_unlock(&rules_data_lock);
	return ptr;
}

__visible_for_testing int get_rules_size(int is_system)
{
	struct d_tree *rules_ptr = get_rules_ptr(is_system);

	return rules_ptr ? rules_ptr->data_size : 0;
}

int check_rules_ready(void)
{
	return (rules_primary.version == D_TREE_VERSION_INVALID) ? 0 : 1;
}

static int check_path_is_system(const char *path)
{
	int i;
	static const int path_size_list[] = { 8, 9, 6, 12, 20 };
	static const char * const path_list[] = {
		"/system/",
		"/product/",
		"/apex/",
		"/system_ext/",
		"/postinstall/system/"
	};

	for (i = 0; i < ARRAY_SIZE(path_list); i++) {
		if (!strncmp(path_list[i], path, path_size_list[i]))
			return 1;
	}
	return 0;
}

__visible_for_testing int check_system_mount(void)
{
	static int mount_system_root = -1;
	struct file *fp;

	if (mount_system_root < 0) {
		fp = local_fopen("/sbin/recovery", O_RDONLY, 0);
		if (IS_ERR(fp))
			fp = local_fopen("/system/bin/recovery", O_RDONLY, 0);

		if (!IS_ERR(fp)) {
			defex_log_crit("Recovery mode");
			filp_close(fp, NULL);
			update_load_flags(LOAD_FLAG_RECOVERY);
		} else {
			defex_log_crit("Normal mode");
		}

		mount_system_root = 0;
		fp = local_fopen("/system_root", O_DIRECTORY | O_PATH, 0);
		if (!IS_ERR(fp)) {
			filp_close(fp, NULL);
			mount_system_root = 1;
			defex_log_crit("System_root=TRUE");
		} else {
			defex_log_crit("System_root=FALSE");
		}
	}
	return (mount_system_root > 0);
}

#ifdef DEFEX_INTEGRITY_ENABLE
__visible_for_testing int defex_check_integrity(struct file *f, struct d_tree_item *item)
{
	struct crypto_shash *handle = NULL;
	struct shash_desc *shash = NULL;
	const void *hash = NULL;
	static const unsigned char buff_zero[SHA256_DIGEST_SIZE] = {0};
	unsigned char hash_sha256[SHA256_DIGEST_SIZE];
	unsigned char *buff = NULL;
	size_t buff_size = PAGE_SIZE;
	loff_t file_size = 0;
	int ret = 0, err = 0, read_size = 0, hash_size;

	if (item == NULL)
		return -1;

	if (item->features & d_tree_item_integrity)
		hash = d_tree_get_table_data(d_tree_integrity_table, item->tree,
			item->integrity_index, &hash_size);

	// A saved hash is zero, skip integrity check
	if (!hash || hash_size != INTEGRITY_LENGTH || !memcmp(buff_zero, hash, SHA256_DIGEST_SIZE))
		return ret;

	if (IS_ERR(f))
		goto hash_error;

	handle = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(handle)) {
		err = PTR_ERR(handle);
		defex_log_err("Can't alloc sha256, error : %d", err);
		return -1;
	}

#if KERNEL_VER_GTE(4, 12, 0)
	shash = kvzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(handle), GFP_KERNEL);
#else
	shash = kzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(handle), GFP_KERNEL);
#endif /* < KERNEL_VERSION(4, 12, 0) */

	if (shash == NULL)
		goto hash_error;

	shash->tfm = handle;

#if KERNEL_VER_GTE(4, 12, 0)
	buff = kvmalloc(buff_size, GFP_KERNEL);
#else
	buff = kmalloc(buff_size, GFP_KERNEL);
#endif /* < KERNEL_VERSION(4, 12, 0) */

	if (buff == NULL)
		goto hash_error;

	err = crypto_shash_init(shash);
	if (err < 0)
		goto hash_error;


	while (1) {
		read_size = local_fread(f, file_size, (char *)buff, buff_size);
		if (read_size < 0)
			goto hash_error;
		if (read_size == 0)
			break;
		file_size += read_size;
		err = crypto_shash_update(shash, buff, read_size);
		if (err < 0)
			goto hash_error;
	}

	err = crypto_shash_final(shash, hash_sha256);
	if (err < 0)
		goto hash_error;

	ret = memcmp(hash_sha256, hash, SHA256_DIGEST_SIZE);

	goto hash_exit;

hash_error:
	ret = -1;
hash_exit:
#if KERNEL_VER_GTE(4, 12, 0)
	kvfree(buff);
	kvfree(shash);
#else
	kfree(buff);
	kfree(shash);
#endif /* KERNEL_VER_GTE(4, 12, 0) */

	if (handle)
		crypto_free_shash(handle);
	return ret;

}

__visible_for_testing int defex_integrity_default(const char *file_path)
{
	static const char integrity_default[] = "/system/bin/install-recovery.sh";

	return strncmp(integrity_default, file_path, sizeof(integrity_default));
}

#endif /* DEFEX_INTEGRITY_ENABLE */

#if defined(DEFEX_RAMDISK_ENABLE)

#ifdef DEFEX_TRUSTED_MAP_ENABLE
static const unsigned char *find_policy_section(const char *name, const char *data,
			int data_size, long *section_size)
{
	return data_size > 0 ? defex_tailerp_find(data, data_size, name, section_size) : 0;
}
#endif

__visible_for_testing int load_rules_common(struct file *f, int flags)
{
	int res = -1, data_size, rules_size;
	unsigned char *data_buff = NULL;
	struct dentry *f_dentry = file_dentry(f);
	struct inode *f_inode = file_inode(f);
/* where additional features like DTM could look for policy data */
	const unsigned char *policy_data = NULL;
#ifdef DEFEX_TRUSTED_MAP_ENABLE
	const unsigned char *dtm_section;
#endif /* DEFEX_TRUSTED_MAP_ENABLE */

	if (!(f->f_mode & FMODE_READ) || !f_dentry || f_dentry->d_inode != f_inode ||
			!check_slab_ptr(f_inode) || !S_ISREG(f_inode->i_mode)) {
		defex_log_err("Failed to open the file");
		goto do_clean;
	}

	data_size = i_size_read(file_inode(f));
	if (data_size <= 0 || data_size > DEFEX_RULES_ARRAY_SIZE_MAX)
		goto do_clean;
	data_buff = vmalloc(data_size);
	if (!data_buff)
		goto do_clean;

	rules_size = local_fread(f, 0, data_buff, data_size);
	if (rules_size <= 0) {
		defex_log_err("Failed to read rules file (%d)", rules_size);
		goto do_clean;
	}
	defex_log_info("Read %d bytes", rules_size);

#ifdef DEFEX_SIGN_ENABLE
	res = defex_rules_signature_check((char *)data_buff, (unsigned int)rules_size,
		(unsigned int *)&rules_size);

	if (!res)
		defex_log_info("Rules signature verified successfully");
	else
		defex_log_err("Rules signature incorrect!!!, err = %d", res);
#else
	res = 0;
#endif

	if (!res) {
		if (!(get_load_flags() & (LOAD_FLAG_DPOLICY | LOAD_FLAG_DPOLICY_SYSTEM))) {
#ifndef DEFEX_GKI
			if (rules_size > sizeof(rules_primary_data)) {
				defex_log_err("Primary rules buffer overflow [%d > %d]",
					rules_size, (int)sizeof(rules_primary_data));
				res = -1;
				goto do_clean;
			}
#endif /* DEFEX_GKI */
			spin_lock(&rules_data_lock);
#ifdef DEFEX_GKI
			if (!rules_primary_data) {
				rules_primary_data = data_buff;
				data_buff = NULL;
			}
#else
			memcpy(rules_primary_data, data_buff, rules_size);
#endif
			res = d_tree_get_header(rules_primary_data, rules_size, &rules_primary);
			spin_unlock(&rules_data_lock);
			if (!res) {
				policy_data = rules_primary_data;
#if !defined(DEFEX_SINGLE_RULES_FILE)
			if (flags & LOAD_FLAG_DPOLICY_SYSTEM)
				update_load_flags(LOAD_FLAG_SYSTEM_FIRST);
#endif
#ifdef DEFEX_KUNIT_ENABLED
				rules_primary_size = rules_size;
#endif /* DEFEX_KUNIT_ENABLED */
				defex_log_info("Primary rules have been stored");
			}
		} else {
			if (rules_size > 0) {
				spin_lock(&rules_data_lock);
				rules_secondary_data = data_buff;
				data_buff = NULL;
				res = d_tree_get_header(rules_secondary_data, rules_size,
					&rules_secondary);
				spin_unlock(&rules_data_lock);
				if (!res) {
					policy_data = rules_secondary_data;
#ifdef DEFEX_KUNIT_ENABLED
					rules_secondary_size = rules_size;
#endif /* DEFEX_KUNIT_ENABLED */
					defex_log_info("Secondary rules have been stored");
				}
			}
		}
#ifdef DEFEX_SHOW_RULES_ENABLE
		if (policy_data)
			defex_show_structure((void *)policy_data, rules_size);
#endif /* DEFEX_SHOW_RULES_ENABLE */
#ifdef DEFEX_TRUSTED_MAP_ENABLE
		if (policy_data && !dtm_tree.data) { /* DTM not yet initialized */
			dtm_section = find_policy_section(DEFEX_DTM_SECTION_NAME, policy_data,
				rules_size, 0);
			if (dtm_section)
				pptree_set_data(&dtm_tree, dtm_section);
		}
#endif
		if (res != 0)
			goto do_clean;
		update_load_flags(flags);
		res = rules_size;
	}

do_clean:
	filp_close(f, NULL);
	vfree(data_buff);
	return res;
}

int validate_file(const char *file_path)
{
	struct path a_path;
	struct super_block *sb = NULL;
	struct dentry *root_dentry;
	int err, ret = 0;

	err = kern_path(file_path, 0, &a_path);
	if (err)
		return ret;
	sb = a_path.dentry->d_sb;
	if (!sb)
		goto do_clean;
	if (!sb->s_type)
		goto do_clean;
	if (!sb->s_type->name || !sb->s_type->name[0])
		goto do_clean;
	root_dentry = dget(sb->s_root);
	if (!root_dentry)
		goto do_clean;
	dput(root_dentry);
	ret = 1;
do_clean:
	path_put(&a_path);
	return ret;
}

int load_rules_thread(void *params)
{
	const unsigned int load_both_mask = (LOAD_FLAG_DPOLICY | LOAD_FLAG_DPOLICY_SYSTEM);
	struct file *f = NULL;
	int f_index;
	const struct rules_file_struct *item;
	unsigned long start_time, cur_time, last_time = 0;
	int load_counter = 0;
#ifdef DEFEX_LOG_FILE_ENABLE
	const bool keep_thread = true;
#else
	const bool keep_thread = false;
#endif /* DEFEX_LOG_FILE_ENABLE */

	(void)params;

	start_time = get_current_sec();
	while (!is_reboot_pending && !kthread_should_stop()) {
		cur_time = get_current_sec();
		if ((cur_time - last_time) < 5) {
			if (msleep_interruptible(1000) != 0)
				break;
			continue;
		}
		last_time = cur_time;

		if (!keep_thread && (cur_time - start_time) > 600) {
			update_load_flags(LOAD_FLAG_TIMEOUT);
			defex_log_warn("Late load timeout. Try counter = %d", load_counter);
			break;
		}
		load_counter++;
		f = NULL;
		for (f_index = 0; f_index < ARRAY_SIZE(rules_files); f_index++) {
			item = &rules_files[f_index];
			if (!(get_load_flags() & item->flags) && validate_file(item->name)) {
				f = local_fopen(item->name, O_RDONLY, 0);
				if (!IS_ERR_OR_NULL(f)) {
					defex_log_info("Late load rules file: %s", item->name);
					break;
				}
			}
		}
		if (IS_ERR(f)) {
#ifdef DEFEX_GKI
			defex_log_err("Failed to open rules file (%ld)", (long)PTR_ERR(f));
#endif /* DEFEX_GKI */
		} else {
			if (!IS_ERR_OR_NULL(f))
				load_rules_common(f, item->flags);
		}
		if (!keep_thread && (get_load_flags() & load_both_mask) == load_both_mask)
			break;
#ifdef DEFEX_LOG_FILE_ENABLE
		storage_log_process();
#endif /* DEFEX_LOG_FILE_ENABLE */
	}
	return 0;
}


int load_rules_late(void)
{
	int res = 0;
	static atomic_t load_lock = ATOMIC_INIT(1);
	static int first_entry;
	struct task_struct *thread_ptr;

	if (get_load_flags() & LOAD_FLAG_TIMEOUT)
		return -1;

	if (!atomic_dec_and_test(&load_lock)) {
		atomic_inc(&load_lock);
		return res;
	}

	/* The first try to load, initialize time values and start the kernel thread */
	if (!first_entry) {
		first_entry = 1;
		thread_ptr = kthread_create(load_rules_thread, NULL, "defex_load_thread");
		if (IS_ERR_OR_NULL(thread_ptr)) {
			res = -1;
			update_load_flags(LOAD_FLAG_TIMEOUT);
			goto do_exit;
		}
		wake_up_process(thread_ptr);
	}

do_exit:
	atomic_inc(&load_lock);
	return res;
}

int __init do_load_rules(void)
{
	struct file *f = NULL;
	int res = -1;
	unsigned int f_index = 0;
	const struct rules_file_struct *item;

	if (boot_state_recovery)
		update_load_flags(LOAD_FLAG_RECOVERY);

load_next:
	while (f_index < ARRAY_SIZE(rules_files)) {
		item = &rules_files[f_index];
		if (!(get_load_flags() & item->flags)) {
			f = local_fopen(item->name, O_RDONLY, 0);
			if (!IS_ERR_OR_NULL(f)) {
				defex_log_info("Load rules file: %s", item->name);
				break;
			}
		}
		f_index++;
	};

	if (f_index == ARRAY_SIZE(rules_files)) {
		if (get_load_flags() & (LOAD_FLAG_DPOLICY_SYSTEM | LOAD_FLAG_DPOLICY))
			return 0;
		defex_log_err("Failed to open rules file (%ld)", (long)PTR_ERR(f));

#ifdef DEFEX_GKI
		if (get_load_flags() & LOAD_FLAG_RECOVERY)
			res = 0;
#endif /* DEFEX_GKI */
		return res;
	}

	res = load_rules_common(f, item->flags);
	res = (res < 0) ? res : 0;

#ifdef DEFEX_GKI
	if ((get_load_flags() & LOAD_FLAG_RECOVERY) && res != 0) {
		res = 0;
		defex_log_info("Kernel Only & recovery mode, rules loading is passed");
	}
#endif
	if (++f_index < ARRAY_SIZE(rules_files))
		goto load_next;
	return res;
}

#endif /* DEFEX_RAMDISK_ENABLE */


__visible_for_testing int lookup_tree(const char *file_path,
					unsigned int attribute,
					struct file *f,
					struct d_tree_item *found_item, int linked_offset)
{
	struct d_tree *base, *last_base = NULL;
	int is_system, offset, iteration = 0;
	const int is_recovery = (get_load_flags() & LOAD_FLAG_RECOVERY) ? feature_for_recovery : 0;
	const unsigned int load_both_mask = (LOAD_FLAG_DPOLICY | LOAD_FLAG_DPOLICY_SYSTEM);
	struct d_tree_item local_item;
	struct d_tree_ctx ctx;
	static const unsigned int attr_def_one = (feature_ped_exception |
						  feature_safeplace_path |
						  feature_umhbin_path |
						  feature_immutable_src_exception |
						  feature_immutable_dst_exception |
						  feature_immutable_root);

	if (!found_item)
		found_item = &local_item;

	if (!file_path || *file_path != '/')
		return 0;

	is_system = check_path_is_system(file_path);

	if ((get_load_flags() & load_both_mask) != load_both_mask &&
			!(get_load_flags() & LOAD_FLAG_TIMEOUT)) {
		/* allow all requests if rules were not loaded for Recovery mode */
		if (!load_rules_late() || is_recovery) {
			if (attr_def_one & attribute)
				return 1;
			return 0;
		}
	}

try_next:

	base = get_rules_ptr(get_rules_size(is_system) ? is_system : (!is_system));
	if (base == last_base || !base || !base->data_size) {
		/* block all requests if rules were not loaded */
		return 0;
	}
	last_base = base;
	d_tree_lookup_init(base, &ctx, 0, attribute | is_recovery, 0);
	ctx.linked_offset = linked_offset;
	offset = d_tree_lookup_path(&ctx, file_path, found_item);
	if (offset > 0) {
#ifdef DEFEX_INTEGRITY_ENABLE
		/* Integrity acceptable only for files */
		if ((found_item->features & feature_integrity_check) &&
			(found_item->features & feature_is_file) && f) {
			if (defex_integrity_default(file_path)
				&& defex_check_integrity(f, found_item))
				return DEFEX_INTEGRITY_FAIL;
		}
#endif /* DEFEX_INTEGRITY_ENABLE */
		return offset;
	}

	if ((get_load_flags() & load_both_mask) == load_both_mask && ++iteration < 2) {
		is_system = !is_system;
		goto try_next;
	}
	return 0;
}

int rules_lookup(const char *target_file, int attribute, struct file *f,
		struct d_tree_item *found_item, int linked_offset)
{
	int ret = 0;
#if (defined(DEFEX_SAFEPLACE_ENABLE) || defined(DEFEX_IMMUTABLE_ENABLE) \
		|| defined(DEFEX_PED_ENABLE))
	static const char system_root_txt[] = "/system_root";

	if (check_system_mount() &&
		!strncmp(target_file, system_root_txt, sizeof(system_root_txt) - 1))
		target_file += (sizeof(system_root_txt) - 1);

	ret = lookup_tree(target_file, attribute, f, found_item, linked_offset);
#endif
	return ret;
}

void __init defex_load_rules(void)
{
#if defined(DEFEX_RAMDISK_ENABLE)
	if (!boot_state_unlocked && do_load_rules() != 0) {
#if !(defined(DEFEX_DEBUG_ENABLE) || defined(DEFEX_GKI))
		panic("[DEFEX] Signature mismatch.\n");
#endif
	}
#endif /* DEFEX_RAMDISK_ENABLE */
}
