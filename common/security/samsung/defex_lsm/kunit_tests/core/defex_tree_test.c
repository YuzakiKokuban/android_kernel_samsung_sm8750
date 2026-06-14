// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <kunit/test.h>
#include <kunit/mock.h>
#include "include/defex_internal.h"
#include "include/defex_tree.h"
#include "include/defex_rules.h"

#define DUMMY_DIR "/dummy"
#define INPUT_PATH "/init_path/file.txt"
#define EXPECTED_PATH "init_path/file.txt"
#define NOT_EXIST_PATH "/not_existing/not_existing_file.txt"

struct subpath_extract_ctx {
	const char *path;
};

extern void subpath_extract_init(struct subpath_extract_ctx *ctx, const char *path);
extern int subpath_extract_next(struct subpath_extract_ctx *ctx, const char **subpath_ptr);
extern unsigned short d_tree_get_version(void *data, size_t data_size);
extern int d_tree_get_item_header(struct d_tree *the_tree, unsigned int offset,
			struct d_tree_item *item);
extern int d_tree_lookup_path(struct d_tree_ctx *ctx, const char *file_path,
			struct d_tree_item *found_item);

#ifdef DEFEX_GKI
extern unsigned char *rules_primary_data;
#else
extern unsigned char rules_primary_data[];
#endif
extern unsigned char *rules_secondary_data;
extern int rules_primary_size;
extern int rules_secondary_size;

unsigned char *current_rules_data;
int current_rules_size;

char defex_tree_test_file[PATH_MAX] = {0};
int rule_lookup_performed;

static int get_first_feature(int feature_type)
{
#define TEST_FEATURE(ft) do { if (feature_type & ft) return ft; } while (0)

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

	if (strlen(defex_tree_test_file) != 0)
		return;

	feature_flags &= active_feature_mask;

	is_system = ((strncmp("/system/", current_path, 8) == 0) ||
			(strncmp("/product/", current_path, 9) == 0) ||
			(strncmp("/apex/", current_path, 6) == 0) ||
			(strncmp("/system_ext/", current_path, 12) == 0)) ? 1 : 0;

	/* feature_is_file set */
	attr = get_first_feature(feature_flags & feature_is_file);
	if (attr && !is_system) {
		file_ptr = local_fopen(current_path, O_RDONLY, 0);
		if (!IS_ERR_OR_NULL(file_ptr)) {
			if (feature_flags & feature_safeplace_path)
				if (strlen(defex_tree_test_file) == 0)
					strscpy(defex_tree_test_file, current_path,
							sizeof(defex_tree_test_file));
			filp_close(file_ptr, 0);
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
	/* Rules are not loaded --- can't find any paths */
	if (!base || !base->data_size)
		return;

	path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (!path)
		return;

	d_tree_get_header(rules_ptr, rules_size, &rules_tree);
	find_paths(&rules_tree, NULL, path, 0);

	pr_info("defex_tree_test_file: %s", defex_tree_test_file);

	kfree(path);
}

static void subpath_extract_init_test(struct kunit *test)
{
	struct subpath_extract_ctx ctx = {0};

	/* T1: Null context - no check is done */
	subpath_extract_init(NULL, INPUT_PATH);
	KUNIT_SUCCEED(test);

	/* T2: Null input - no check is done */
	subpath_extract_init(&ctx, NULL);
	KUNIT_SUCCEED(test);

	/* T3: Right parameters */
	subpath_extract_init(&ctx, INPUT_PATH);
	KUNIT_EXPECT_EQ(test, strncmp(ctx.path, EXPECTED_PATH, strlen(INPUT_PATH)), 0);
}

static void subpath_extract_next_test(struct kunit *test)
{
	struct subpath_extract_ctx ctx = {0};

	const char *out;

	/* T1: Null context - no check is done */
	subpath_extract_next(NULL, &out);
	KUNIT_SUCCEED(test);

	/* T2: Zero context*/
	KUNIT_EXPECT_EQ(test, subpath_extract_next(&ctx, &out), 0);

	/* T3: Right parameters */
	subpath_extract_init(&ctx, INPUT_PATH);
	KUNIT_EXPECT_EQ(test, subpath_extract_next(&ctx, &out), (int)strlen("init_path"));
}

static void d_tree_get_version_test(struct kunit *test)
{
	unsigned int version;

	/* If packed rules are being used, they need to be loaded before the test. */
	if (check_rules_ready() == 0) {
		kunit_info(test, "DEFEX policy not loaded: skip test.");
		return;
	}

	/* T1: Null data - no check is done */
	version = d_tree_get_version(NULL, rules_primary_size);

	/* T2: Right parameters */
	version = d_tree_get_version(rules_primary_data, rules_primary_size);
	KUNIT_EXPECT_TRUE(test, (version == 1) | (version == 2));
}

static void d_tree_get_header_test(struct kunit *test)
{
	struct d_tree rules_tree = {0};

	/* If packed rules are being used, they need to be loaded before the test. */
	if (check_rules_ready() == 0) {
		kunit_info(test, "DEFEX policy not loaded: skip test.");
		return;
	}

	/* T1: Null data */
	KUNIT_EXPECT_EQ(test, d_tree_get_header(NULL, 0, &rules_tree), -1);

	/* T2: Null tree pinter */
	KUNIT_EXPECT_EQ(test, d_tree_get_header(rules_primary_data, 0, NULL), -1);

	/* T3: Right parameters */
	KUNIT_EXPECT_EQ(test, d_tree_get_header(rules_primary_data, rules_primary_size,
		&rules_tree), 0);
}

static void d_tree_get_item_header_test(struct kunit *test)
{
	struct d_tree rules_tree = {0};
	struct d_tree_item item = {0};

	/* If packed rules are being used, they need to be loaded before the test. */
	if (check_rules_ready() == 0) {
		kunit_info(test, "DEFEX policy not loaded: skip test.");
		return;
	}

	d_tree_get_header(rules_primary_data, rules_primary_size, &rules_tree);

	/* T1: Null tree pointer */
	KUNIT_EXPECT_EQ(test, d_tree_get_item_header(NULL, 0, &item), 0);

	/* T2: Null item pointer */
	KUNIT_EXPECT_EQ(test, d_tree_get_item_header(&rules_tree, 0, NULL), 0);

	/* T3: Right parameters */
	KUNIT_EXPECT_NE(test, d_tree_get_item_header(&rules_tree, 20, &item), 0);
}

static void d_tree_lookup_path_test(struct kunit *test)
{
	struct d_tree rules_tree = {0};
	struct d_tree_item item = {0};
	struct d_tree_ctx ctx = {0};
	const char zero_file[NAME_MAX] = {0};

	/* If packed rules are being used, they need to be loaded before the test. */
	if (check_rules_ready() == 0) {
		kunit_info(test, "DEFEX policy not loaded: skip test.");
		return;
	}

	if (strlen(defex_tree_test_file) > 0) {
		/* T1: Zero ctx data */
		KUNIT_EXPECT_EQ(test, d_tree_lookup_path(&ctx, defex_tree_test_file, &item), -1);

		d_tree_get_header(current_rules_data, current_rules_size, &rules_tree);
		d_tree_lookup_init(&rules_tree, &ctx, 0, feature_safeplace_path, 0);

		/* T2: Zero file data */
		KUNIT_EXPECT_EQ(test, d_tree_lookup_path(&ctx, zero_file, &item), 0);

		/* T3: Null tree pointer */
		KUNIT_EXPECT_EQ(test, d_tree_lookup_path(NULL, defex_tree_test_file, &item), -1);

		/* T4: Null target file pointer */
		KUNIT_EXPECT_EQ(test, d_tree_lookup_path(&ctx, NULL, &item), 0);

		/* T5: Right parameters & null item pointer */
		KUNIT_EXPECT_NE(test, d_tree_lookup_path(&ctx, defex_tree_test_file, NULL), 0);

		/* T5: Right parameters & not existing path */
		KUNIT_EXPECT_EQ(test, d_tree_lookup_path(&ctx, NOT_EXIST_PATH, NULL), 0);

		/* T6: Right parameters */
		d_tree_lookup_init(&rules_tree, &ctx, 0, feature_safeplace_path, 0);
		KUNIT_EXPECT_NE(test, d_tree_lookup_path(&ctx, defex_tree_test_file, &item), 0);
	}
}

static int defex_tree_test_init(struct kunit *test)
{
	if (!rule_lookup_performed) {
		memset(defex_tree_test_file, 0, PATH_MAX);
		/* If lookup not done yet, trigger it. */
		pr_info("Secondary rules checking...");
		if (d_tree_get_version(rules_secondary_data, rules_secondary_size)
				!= D_TREE_VERSION_INVALID) {
			find_rules_for_test(rules_secondary_data, rules_secondary_size);
			current_rules_data = rules_secondary_data;
			current_rules_size = rules_secondary_size;
		}
		if (strlen(defex_tree_test_file) == 0) {
			pr_info("Primary rules checking...");
			if (d_tree_get_version(rules_primary_data, rules_primary_size)
					!= D_TREE_VERSION_INVALID) {
				find_rules_for_test(rules_primary_data, rules_primary_size);
				current_rules_data = (unsigned char *)rules_primary_data;
				current_rules_size = rules_primary_size;
			}
		}
		rule_lookup_performed = 1;
	}
	return 0;
}

static void defex_tree_test_exit(struct kunit *test)
{
}

static struct kunit_case defex_tree_test_cases[] = {
	/* TEST FUNC DEFINES */
	KUNIT_CASE(subpath_extract_init_test),
	KUNIT_CASE(subpath_extract_next_test),
	KUNIT_CASE(d_tree_get_version_test),
	KUNIT_CASE(d_tree_get_header_test),
	KUNIT_CASE(d_tree_get_item_header_test),
	KUNIT_CASE(d_tree_lookup_path_test),
	{},
};

static struct kunit_suite defex_tree_test_module = {
	.name = "defex_tree_test",
	.init = defex_tree_test_init,
	.exit = defex_tree_test_exit,
	.test_cases = defex_tree_test_cases,
};

kunit_test_suites(&defex_tree_test_module);

MODULE_LICENSE("GPL v2");
