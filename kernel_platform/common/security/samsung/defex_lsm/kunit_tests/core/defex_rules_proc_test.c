// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <kunit/test.h>
#include <kunit/mock.h>
#include <linux/delay.h>
#include <linux/limits.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include "include/defex_internal.h"
#include "include/defex_rules.h"
#include "include/defex_tree.h"

#define DEFEX_RULES_FILE "/dpolicy"
#define DUMMY_DIR "/dummy"
#define ROOT_PATH "/"
#define INTEGRITY_DEFAULT "/system/bin/install-recovery.sh"
#define NOT_A_PATH "not_a_path"
#define SYSTEM_ROOT "/system_root"

#ifdef DEFEX_USE_PACKED_RULES
extern int lookup_tree(const char *file_path,
					unsigned int attribute,
					struct file *f,
					struct d_tree_item *found_item,
					int linked_offset);
extern struct d_tree *get_rules_ptr(int is_system);
#ifdef DEFEX_RAMDISK_ENABLE
#ifdef DEFEX_GKI
extern unsigned char *rules_primary_data;
#else
extern unsigned char rules_primary_data[];
#endif
extern unsigned char *rules_secondary_data;
extern int rules_primary_size;
extern int rules_secondary_size;

#endif /* DEFEX_RAMDISK_ENABLE */
#endif /* DEFEX_USE_PACKED_RULES */

#if defined(DEFEX_RAMDISK_ENABLE) && defined(DEFEX_GKI)
extern int load_rules_late(void);
#endif /* DEFEX_RAMDISK_ENABLE && DEFEX_GKI */

#ifdef DEFEX_INTEGRITY_ENABLE
#define SHA256_DIGEST_SIZE 32
extern int defex_check_integrity(struct file *f, struct d_tree_item *item);
extern int defex_integrity_default(const char *file_path);
#endif /* DEFEX_INTEGRITY_ENABLE */

extern int check_system_mount(void);

/* --------------------------------------------------------------------------*/
/* Auxiliary functions to find possible examples in the policy.              */
/* --------------------------------------------------------------------------*/

#if defined(DEFEX_USE_PACKED_RULES) && defined(DEFEX_RAMDISK_ENABLE)
static int rule_lookup_performed;

char first_file[PATH_MAX];
int second_file_attr;
char second_file[PATH_MAX];
char existing_directory_no_features[PATH_MAX];
char existing_directory_path_open[PATH_MAX];
char existing_directory_path_write[PATH_MAX];

/* get_first_feature() - Get the first feature from an integer */
int get_first_feature(int feature_type)
{
#define TEST_FEATURE(ft) do {		\
	if (feature_type & (ft))	\
		return ft;		\
	} while (0)
	TEST_FEATURE(feature_is_file);
	TEST_FEATURE(feature_for_recovery);
	TEST_FEATURE(feature_ped_path);
	TEST_FEATURE(feature_ped_exception);
	TEST_FEATURE(feature_ped_status);
	TEST_FEATURE(feature_safeplace_path);
	TEST_FEATURE(feature_safeplace_status);
	TEST_FEATURE(feature_immutable_path_open);
	TEST_FEATURE(feature_immutable_path_write);
	TEST_FEATURE(feature_immutable_src_exception);
	TEST_FEATURE(feature_immutable_status);
	TEST_FEATURE(feature_umhbin_path);

	return 0;
}

/*
 * check_current_path() - analyze the current path and store in static variables
 */
static void check_current_path(char *current_path,
			unsigned int feature_flags, size_t path_len)
{
	int attr;
	struct file *file_ptr;
	unsigned int is_system;

	feature_flags &= active_feature_mask;

	is_system = ((strncmp("/system/", current_path, 8) == 0) ||
			(strncmp("/product/", current_path, 9) == 0) ||
			(strncmp("/apex/", current_path, 6) == 0) ||
			(strncmp("/system_ext/", current_path, 12) == 0)) ? 1 : 0;

	if (!(feature_flags & feature_is_file)) {
		if (strlen(existing_directory_path_open) == 0 &&
			feature_flags & feature_immutable_path_open) {
			strscpy(existing_directory_path_open, current_path, path_len + 1);
		}
		if (strlen(existing_directory_path_write) == 0 &&
			feature_flags & feature_immutable_path_write) {
			strscpy(existing_directory_path_write, current_path, path_len + 1);
		}
		if (strlen(existing_directory_no_features) == 0 &&
			feature_flags == 0) {
			strscpy(existing_directory_no_features, current_path, path_len + 1);
		}
	} else {
		/* feature_is_file set */
		attr = get_first_feature(feature_flags & feature_is_file);
		if (attr && !is_system) {
			file_ptr = local_fopen(current_path, O_RDONLY, 0);
			if (!IS_ERR_OR_NULL(file_ptr)) {
				if (feature_flags & feature_integrity_check) {
					if (strlen(first_file) == 0)
						strscpy(first_file, current_path, sizeof(first_file));
				} else if (strlen(second_file) == 0) {
					strscpy(second_file, current_path, sizeof(second_file));
					second_file_attr = attr;
				}
				filp_close(file_ptr, 0);
			}
		}
	}
}

/**
 * find_paths() - Find example paths to be used in the test.
 * @node: The rule tree node being analyzed.
 * @current_path: The walked path so far.
 * @path_len: The path size so far.
 *
 * The method reads the packed_rules_primary policy array and find path and file
 * examples that are in the policy so the test can be performed correctly. The
 * lookup is done recursively, first horizontally and then vertically. This tree
 * walking strategy is done to make path string construction easier.
 *
 * The method finds and stores in static variables:
 * - Two different files that can be opened by the kunit without erros;
 * - A directory with no features;
 * - A directory with feature_immutable_path_open set;
 * - A directory with feature_immutable_path_write set.
 */

static void find_paths(struct d_tree *the_tree, struct d_tree_item *parent_node,
			char *current_path, size_t path_len)
{
	struct d_tree_item local_node, tmp_item, *child_node;
	const char *node_name;
	unsigned int name_size;

	if (!parent_node) {
		if (!d_tree_get_item_header(the_tree, 0, &local_node))
			return;
		parent_node = &local_node;
	}

	child_node = d_tree_lookup_dir_init(parent_node, &tmp_item);

	while (child_node) {
		node_name = d_tree_get_subpath(child_node, &name_size);
		/* If no more space in current_path is available, stop looking here. */
		if (((PATH_MAX - path_len) > (name_size + 1))
				 && (name_size != 3 || memcmp(node_name, "tmp", name_size))) {
			/* Append name to path */
			memset(current_path + path_len, 0, PATH_MAX - path_len);
			current_path[path_len] = '/';
			memcpy(current_path + path_len + 1, node_name, name_size);

			check_current_path(current_path, child_node->features, path_len + name_size + 1);
			find_paths(the_tree, child_node, current_path, path_len + name_size + 1);
			current_path[path_len] = 0;
		}
		child_node = d_tree_next_child(parent_node, child_node);
	}
}

/* Triggers the lookup process if DEFEX policy is loaded. */
static void find_rules_for_test(char *rules_ptr, unsigned int rules_size)
{
	struct d_tree rules_tree = {0};
	struct rule_item_struct *base;
	char *path;

	base = (struct rule_item_struct *)rules_ptr;
	if (!base || !base->data_size)
		/* Rules are not loaded --- can't find any paths */
		return;

	path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (!path)
		return;

	d_tree_get_header(rules_ptr, rules_size, &rules_tree);
	find_paths(&rules_tree, NULL, path, 0);

	pr_info("kunit defex_rules_proc_test: Path results:");
	pr_info("kunit defex_rules_proc_test: first_file: %s", first_file);
	pr_info("kunit defex_rules_proc_test: second_file: %s", second_file);
	pr_info("kunit defex_rules_proc_test: existing_directory_no_features: %s",
		existing_directory_no_features);
	pr_info("kunit defex_rules_proc_test: existing_directory_path_open: %s",
		existing_directory_path_open);
	pr_info("kunit defex_rules_proc_test: existing_directory_path_write: %s",
		existing_directory_path_write);

	kfree(path);
}
#endif /* DEFEX_USE_PACKED_RULES && DEFEX_RAMDISK_ENABLE */

static void rules_lookup_test(struct kunit *test)
{
#if (defined(DEFEX_SAFEPLACE_ENABLE) || defined(DEFEX_IMMUTABLE_ENABLE) \
	|| defined(DEFEX_PED_ENABLE))

#if defined(DEFEX_USE_PACKED_RULES) && defined(DEFEX_RAMDISK_ENABLE)
	/* If packed rules are being used, they need to be loaded before the test. */
	if (check_rules_ready() == 0) {
		kunit_info(test, "DEFEX policy not loaded: skip test.");
		return;
	}

	if (check_system_mount() == 1)
		KUNIT_EXPECT_EQ(test, 0, rules_lookup(SYSTEM_ROOT, 0, NULL, NULL, 0));
	else
		KUNIT_EXPECT_EQ(test, 0, rules_lookup(NULL, 0, NULL, NULL, 0));
#else
	/* Not able to build without packed rules --- Nothing to test for now. */
#endif /* DEFEX_USE_PACKED_RULES && DEFEX_RAMDISK_ENABLE */
#endif /* DEFEX_SAFEPLACE_ENABLE || DEFEX_IMMUTABLE_ENABLE || DEFEX_PED_ENABLE */
	KUNIT_SUCCEED(test);
}


static void lookup_tree_test(struct kunit *test)
{
#if defined(DEFEX_USE_PACKED_RULES) && defined(DEFEX_RAMDISK_ENABLE)
	struct file *file_one, *file_two;

	/* If packed rules are being used, they need to be loaded before the test. */
	if (check_rules_ready() == 0) {
		kunit_info(test, "DEFEX policy not loaded: skip test.");
		return;
	}

	/* T1: file_path = NULL or file_path[0] != '/' */
	KUNIT_EXPECT_EQ(test, 0, lookup_tree(NULL, 0, NULL, NULL, 0));
	KUNIT_EXPECT_EQ(test, 0, lookup_tree(NOT_A_PATH, 0, NULL, NULL, 0));

	if (strlen(first_file) > 0 && strlen(second_file) > 0) {
		/* Policy lookup fond examples. */
		file_one = local_fopen(first_file, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(file_one))
			goto test_four;
		file_two = local_fopen(second_file, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(file_two)) {
			filp_close(file_one, 0);
			goto test_four;
		}

		/* T2: file with attribute other than feature_is_file */
		KUNIT_EXPECT_EQ(test, 1,
			(lookup_tree(second_file, second_file_attr, file_one, NULL, 0)
				> DEFEX_INTEGRITY_FAIL) ? 1 : 0);

		/* T3: file with different contents and without check integrity flag */
		KUNIT_EXPECT_EQ(test, 1,
			(lookup_tree(second_file, second_file_attr, file_two, NULL, 0)
				> DEFEX_INTEGRITY_FAIL) ? 1 : 0);

		filp_close(file_one, 0);
		filp_close(file_two, 0);
	}
test_four:
	/* T4: Root path -> Does not look into the tree. */
	KUNIT_EXPECT_EQ(test, 0, lookup_tree(ROOT_PATH, 0, NULL, NULL, 0));

	if (strlen(existing_directory_path_open) > 0) {
		/* T5: Path with feature_immutable_path_open */
		KUNIT_EXPECT_EQ(test, 0,
			lookup_tree(existing_directory_path_open, feature_immutable_path_open,
				NULL, NULL, 0));

		/* T6: with other separator */
		existing_directory_path_open[strlen(existing_directory_path_open)] = '/';
		KUNIT_EXPECT_EQ(test, 0,
			lookup_tree(existing_directory_path_open, feature_immutable_path_open,
				NULL, NULL, 0));
		existing_directory_path_open[strlen(existing_directory_path_open) - 1] = '\0';
	}

	if (strlen(existing_directory_path_write) > 0) {
		/* T7: Path with feature_immutable_path_write */
		KUNIT_EXPECT_EQ(test, 0,
			lookup_tree(existing_directory_path_write, feature_immutable_path_write,
				NULL, NULL, 0));

		/* T8: with other separator */
		existing_directory_path_write[strlen(existing_directory_path_write)] = '/';
		KUNIT_EXPECT_EQ(test, 0,
			lookup_tree(existing_directory_path_write, feature_immutable_path_write,
				NULL, NULL, 0));
		existing_directory_path_write[strlen(existing_directory_path_write) - 1] = '\0';
	}

	/* T9: Path not present in policy */
	KUNIT_EXPECT_EQ(test, 0, lookup_tree(DUMMY_DIR, feature_immutable_path_open,
		NULL, NULL, 0));

#endif /* DEFEX_USE_PACKED_RULES && DEFEX_RAMDISK_ENABLE*/
	KUNIT_SUCCEED(test);
}


static void load_rules_late_test(struct kunit *test)
{
#if defined(DEFEX_RAMDISK_ENABLE) && defined(DEFEX_USE_PACKED_RULES) && defined(DEFEX_GKI)

	/* The test cannot try to load the policy by its own,
	 * since it can compromise the system.
	 */

#endif /* DEFEX_RAMDISK_ENABLE && DEFEX_USE_PACKED_RULES && DEFEX_GKI */
	KUNIT_SUCCEED(test);
}


static void do_load_rules_test(struct kunit *test)
{
	/* __init function */
	KUNIT_SUCCEED(test);
}


static void defex_load_rules_test(struct kunit *test)
{
	/* __init function */
	KUNIT_SUCCEED(test);
}


static void defex_integrity_default_test(struct kunit *test)
{
#ifdef DEFEX_INTEGRITY_ENABLE
	KUNIT_EXPECT_EQ(test, 0, defex_integrity_default(INTEGRITY_DEFAULT));
	KUNIT_EXPECT_NE(test, 0, defex_integrity_default(DUMMY_DIR));
#endif
	KUNIT_SUCCEED(test);
}


static void defex_init_rules_proc_test(struct kunit *test)
{
	/* __init function */
	KUNIT_SUCCEED(test);
}


static void defex_check_integrity_test(struct kunit *test)
{
#ifdef DEFEX_INTEGRITY_ENABLE
	struct d_tree_item item = {0};
#if defined(DEFEX_USE_PACKED_RULES) && defined(DEFEX_RAMDISK_ENABLE)
	struct d_tree *base;
	struct d_tree_ctx ctx;
	struct file *test_file;
	int offset;
#endif
	/* T1: Null input - no check is done */
	KUNIT_EXPECT_EQ(test, -1, defex_check_integrity(NULL, NULL));

	/* T2: Zero item - no check is done */
	KUNIT_EXPECT_EQ(test, 0, defex_check_integrity(NULL, &item));

#if defined(DEFEX_USE_PACKED_RULES) && defined(DEFEX_RAMDISK_ENABLE)
	/* T3: Right parameters */
	if (strlen(first_file) > 0) {
		base = get_rules_ptr(0);
		d_tree_lookup_init(base, &ctx, 0, feature_integrity_check, 0);
		offset = d_tree_lookup_path(&ctx, first_file, &item);
		if (offset) {
			test_file = local_fopen(first_file, O_RDONLY, 0);
			if ((item.features & feature_is_file) && test_file)
				KUNIT_EXPECT_EQ(test, 0, defex_check_integrity(test_file, &item));
			filp_close(test_file, NULL);
		} else
			KUNIT_FAIL(test,
				"Error in d_tree_lookup_path: target file not exist in rules");

		/* T4: file pointer is error */
		KUNIT_EXPECT_EQ(test, -1, defex_check_integrity(ERR_PTR(-1), &item));
	}
#else
	/* Not able to build without packed rules --- Nothing to test for now. */
#endif /* DEFEX_USE_PACKED_RULES && DEFEX_RAMDISK_ENABLE*/
#endif /* DEFEX_INTEGRITY_ENABLE */
	KUNIT_SUCCEED(test);
}


static void check_system_mount_test(struct kunit *test)
{
	struct file *fp;

	fp = local_fopen(SYSTEM_ROOT, O_DIRECTORY | O_PATH, 0);
	if (!IS_ERR(fp)) {
		filp_close(fp, NULL);
		KUNIT_EXPECT_EQ(test, check_system_mount(), 1);
	} else {
		KUNIT_EXPECT_EQ(test, check_system_mount(), 0);
	}
	KUNIT_SUCCEED(test);
}


static void check_rules_ready_test(struct kunit *test)
{
#if defined(DEFEX_USE_PACKED_RULES) && defined(DEFEX_RAMDISK_ENABLE)
	struct rule_item_struct *base_struct = (struct rule_item_struct *)rules_primary_data;

	if (!base_struct || !base_struct->data_size)
		KUNIT_EXPECT_EQ(test, 0, check_rules_ready());
	else
		KUNIT_EXPECT_EQ(test, 1, check_rules_ready());

#endif /* DEFEX_USE_PACKED_RULES && DEFEX_RAMDISK_ENABLE*/
	KUNIT_SUCCEED(test);
}


static void bootmode_setup_test(struct kunit *test)
{
	/* __init function */
	KUNIT_SUCCEED(test);
}


static int defex_rules_proc_test_init(struct kunit *test)
{
#if defined(DEFEX_USE_PACKED_RULES) && defined(DEFEX_RAMDISK_ENABLE)
	if (!rule_lookup_performed) {
		memset(first_file, 0, PATH_MAX);
		memset(second_file, 0, PATH_MAX);
		memset(existing_directory_no_features, 0, PATH_MAX);
		memset(existing_directory_path_open, 0, PATH_MAX);
		memset(existing_directory_path_write, 0, PATH_MAX);

		/* If lookup not done yet, trigger it. */
		pr_info("Secondary rules checking...");
		if (d_tree_get_version(rules_secondary_data, rules_secondary_size)
				!= D_TREE_VERSION_INVALID)
			find_rules_for_test(rules_secondary_data, rules_secondary_size);
		pr_info("Primary rules checking...");
		if (d_tree_get_version(rules_primary_data, rules_primary_size)
				!= D_TREE_VERSION_INVALID)
			find_rules_for_test(rules_primary_data, rules_primary_size);
		rule_lookup_performed = 1;
	}
#endif /* DEFEX_USE_PACKED_RULES && DEFEX_RAMDISK_ENABLE */
	return 0;
}

static void defex_rules_proc_test_exit(struct kunit *test)
{
}

static struct kunit_case defex_rules_proc_test_cases[] = {
	/* TEST FUNC DEFINES */
	KUNIT_CASE(rules_lookup_test),
	KUNIT_CASE(lookup_tree_test),
	KUNIT_CASE(load_rules_late_test),
	KUNIT_CASE(do_load_rules_test),
	KUNIT_CASE(defex_load_rules_test),
	KUNIT_CASE(defex_integrity_default_test),
	KUNIT_CASE(defex_init_rules_proc_test),
	KUNIT_CASE(defex_check_integrity_test),
	KUNIT_CASE(check_system_mount_test),
	KUNIT_CASE(check_rules_ready_test),
	KUNIT_CASE(bootmode_setup_test),
	{},
};

static struct kunit_suite defex_rules_proc_test_module = {
	.name = "defex_rules_proc_test",
	.init = defex_rules_proc_test_init,
	.exit = defex_rules_proc_test_exit,
	.test_cases = defex_rules_proc_test_cases,
};
kunit_test_suites(&defex_rules_proc_test_module);

MODULE_LICENSE("GPL v2");
