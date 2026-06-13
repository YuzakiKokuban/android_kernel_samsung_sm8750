// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <kunit/test.h>
#include <kunit/mock.h>
#include "include/defex_internal.h"

/* Helper methods for testing */

int get_current_ped_features(void)
{
	int ped_features = 0;
#if defined(DEFEX_PED_ENABLE) && defined(DEFEX_PERMISSIVE_PED)
	if (global_privesc_status != 0)
		ped_features |= FEATURE_CHECK_CREDS;
	if (global_privesc_status == 2)
		ped_features |= FEATURE_CHECK_CREDS_SOFT;
#elif defined(DEFEX_PED_ENABLE)
	ped_features |= GLOBAL_PED_STATUS;
#endif /* DEFEX_PERMISSIVE_PED */
	return ped_features;
}

int get_current_safeplace_features(void)
{
	int safeplace_features = 0;
#if defined(DEFEX_SAFEPLACE_ENABLE) && defined(DEFEX_PERMISSIVE_SP)
	if (global_safeplace_status != 0)
		safeplace_features |= FEATURE_SAFEPLACE;
	if (global_safeplace_status == 2)
		safeplace_features |= FEATURE_SAFEPLACE_SOFT;
#elif defined(DEFEX_SAFEPLACE_ENABLE)
	safeplace_features |= GLOBAL_SAFEPLACE_STATUS;
#endif
	return safeplace_features;
}

int get_current_immutable_features(void)
{
	int immutable_features = 0;
#if defined(DEFEX_IMMUTABLE_ENABLE) && defined(DEFEX_PERMISSIVE_IM)
	if (global_immutable_status != 0)
		immutable_features |= FEATURE_IMMUTABLE;
	if (global_immutable_status == 2)
		immutable_features |= FEATURE_IMMUTABLE_SOFT;
#elif defined(DEFEX_IMMUTABLE_ENABLE)
	immutable_features |= GLOBAL_IMMUTABLE_STATUS;
#endif
	return immutable_features;
}

int get_current_immutable_root_features(void)
{
	int immutable_root_features = 0;
#if defined(DEFEX_IMMUTABLE_ROOT_ENABLE) && defined(DEFEX_PERMISSIVE_IMR)
	if (global_immutable_root_status != 0)
		immutable_root_features |= FEATURE_IMMUTABLE_ROOT;
	if (global_immutable_status == 2)
		immutable_root_features |= FEATURE_IMMUTABLE_ROOT_SOFT;
#elif defined(DEFEX_IMMUTABLE_ROOT_ENABLE)
	immutable_root_features |= GLOBAL_IMMUTABLE_ROOT_STATUS;
#endif
	return immutable_root_features;
}

int get_current_integrity_features(void)
{
	int integrity_features = 0;
#if defined(DEFEX_INTEGRITY_ENABLE) && defined(DEFEX_PERMISSIVE_INT)
	if (global_integrity_status != 0)
		integrity_features |= FEATURE_INTEGRITY;
	if (global_integrity_status == 2)
		integrity_features |= FEATURE_INTEGRITY_SOFT;
#elif defined(DEFEX_INTEGRITY_ENABLE)
	integrity_features |= GLOBAL_INTEGRITY_STATUS;
#endif
	return integrity_features;
}

int get_current_trusted_map_features(void)
{
	int trusted_map_features = 0;
#if defined(DEFEX_TRUSTED_MAP_ENABLE) && defined(DEFEX_PERMISSIVE_TM)
	if (global_trusted_map_status != 0)
		trusted_map_features |= FEATURE_TRUSTED_MAP;
	if (global_trusted_map_status & DEFEX_TM_PERMISSIVE_MODE)
		trusted_map_features |= FEATURE_TRUSTED_MAP_SOFT;
#elif defined(DEFEX_TRUSTED_MAP_ENABLE)
	trusted_map_features |= GLOBAL_TRUSTED_MAP_STATUS;
#endif
	return trusted_map_features;
}

static void defex_get_mode_test(struct kunit *test)
{
	int expected_features;
#if defined(DEFEX_PED_ENABLE) && defined(DEFEX_PERMISSIVE_PED)
	unsigned int ped_status_backup;
#endif
#if defined(DEFEX_SAFEPLACE_ENABLE) && defined(DEFEX_PERMISSIVE_SP)
	unsigned int safeplace_status_backup;
#endif
#if defined(DEFEX_IMMUTABLE_ENABLE) && defined(DEFEX_PERMISSIVE_IM)
	unsigned int immutable_status_backup;
#endif
#if defined(DEFEX_IMMUTABLE_ROOT_ENABLE) && defined(DEFEX_PERMISSIVE_IMR)
	unsigned int immutable_root_status_backup;
#endif
#if defined(DEFEX_INTEGRITY_ENABLE) && defined(DEFEX_PERMISSIVE_INT)
	unsigned int integrity_status_backup;
#endif
#if defined(DEFEX_TRUSTED_MAP_ENABLE) && defined(DEFEX_PERMISSIVE_TM)
	unsigned int trusted_map_status_backup;
#endif

#ifdef DEFEX_PED_ENABLE
	expected_features = 0;
	expected_features |= get_current_safeplace_features();
	expected_features |= get_current_immutable_features();
	expected_features |= get_current_integrity_features();
	expected_features |= get_current_trusted_map_features();
	expected_features |= get_current_immutable_root_features();

#ifdef DEFEX_PERMISSIVE_PED
	ped_status_backup = global_privesc_status;

	global_privesc_status = 1;
	expected_features |= FEATURE_CHECK_CREDS;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_privesc_status = 2;
	expected_features |= FEATURE_CHECK_CREDS_SOFT;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_privesc_status = ped_status_backup;

#else
	expected_features |= GLOBAL_PED_STATUS;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

#endif /* DEFEX_PERMISSIVE_PED */
#endif /* DEFEX_PED_ENABLE */
/*-------------------------------------------------------------------*/
#ifdef DEFEX_SAFEPLACE_ENABLE
	expected_features = 0;
	expected_features |= get_current_ped_features();
	expected_features |= get_current_immutable_features();
	expected_features |= get_current_integrity_features();
	expected_features |= get_current_trusted_map_features();
	expected_features |= get_current_immutable_root_features();

#ifdef DEFEX_PERMISSIVE_SP
	safeplace_status_backup = global_safeplace_status;

	global_safeplace_status = 1;
	expected_features |= FEATURE_SAFEPLACE;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_safeplace_status = 2;
	expected_features |= FEATURE_SAFEPLACE_SOFT;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_safeplace_status = safeplace_status_backup;
#else
	expected_features |= GLOBAL_SAFEPLACE_STATUS;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);
#endif /* DEFEX_PERMISSIVE_SP */
#endif /* DEFEX_SAFEPLACE_ENABLE */
/*-------------------------------------------------------------------*/
#ifdef DEFEX_IMMUTABLE_ENABLE
	expected_features = 0;
	expected_features |= get_current_ped_features();
	expected_features |= get_current_safeplace_features();
	expected_features |= get_current_integrity_features();
	expected_features |= get_current_trusted_map_features();
	expected_features |= get_current_immutable_root_features();

#ifdef DEFEX_PERMISSIVE_IM
	immutable_status_backup = global_immutable_status;

	global_immutable_status = 1;
	expected_features |= FEATURE_IMMUTABLE;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_immutable_status = 2;
	expected_features |= FEATURE_IMMUTABLE_SOFT;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_immutable_status = immutable_status_backup;
#else
	expected_features |= GLOBAL_IMMUTABLE_STATUS;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);
#endif /* DEFEX_PERMISSIVE_IM */
#endif /* DEFEX_IMMUTABLE_ENABLE */
/*-------------------------------------------------------------------*/
#ifdef DEFEX_INTEGRITY_ENABLE
	expected_features = 0;
	expected_features |= get_current_ped_features();
	expected_features |= get_current_safeplace_features();
	expected_features |= get_current_immutable_features();
	expected_features |= get_current_trusted_map_features();
	expected_features |= get_current_immutable_root_features();

#ifdef DEFEX_PERMISSIVE_INT
	integrity_status_backup = global_integrity_status;

	global_integrity_status = 1;
	expected_features |= FEATURE_INTEGRITY;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_integrity_status = 2;
	expected_features |= FEATURE_INTEGRITY_SOFT;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_integrity_status = integrity_status_backup;
#else
	expected_features |= GLOBAL_INTEGRITY_STATUS;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);
#endif /* DEFEX_PERMISSIVE_INT */
#endif /* DEFEX_INTEGRITY_ENABLE */
	/*-------------------------------------------------------------------*/
#ifdef DEFEX_TRUSTED_MAP_ENABLE
	expected_features = 0;
	expected_features |= get_current_ped_features();
	expected_features |= get_current_safeplace_features();
	expected_features |= get_current_immutable_features();
	expected_features |= get_current_integrity_features();
	expected_features |= get_current_immutable_root_features();

#ifdef DEFEX_PERMISSIVE_TM
	trusted_map_status_backup = global_trusted_map_status;

	global_trusted_map_status = DEFEX_TM_ENFORCING_MODE;
	global_trusted_map_status |= DEFEX_TM_DEBUG_VIOLATIONS;
	expected_features |= FEATURE_TRUSTED_MAP;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_trusted_map_status |= DEFEX_TM_PERMISSIVE_MODE;
	expected_features |= FEATURE_TRUSTED_MAP_SOFT;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);

	global_trusted_map_status = trusted_map_status_backup;
#else
	expected_features |= GLOBAL_TRUSTED_MAP_STATUS;
	KUNIT_EXPECT_EQ(test, defex_get_features(), expected_features);
#endif /* DEFEX_PERMISSIVE_TM */
#endif /* DEFEX_TRUSTED_MAP_ENABLE */
	KUNIT_SUCCEED(test);
}

static int defex_get_mode_test_init(struct kunit *test)
{
	return 0;
}

static void defex_get_mode_test_exit(struct kunit *test)
{
}

static struct kunit_case defex_get_mode_test_cases[] = {
	/* TEST FUNC DEFINES */
	KUNIT_CASE(defex_get_mode_test),
	{},
};

static struct kunit_suite defex_get_mode_test_module = {
	.name = "defex_get_mode_test",
	.init = defex_get_mode_test_init,
	.exit = defex_get_mode_test_exit,
	.test_cases = defex_get_mode_test_cases,
};
kunit_test_suites(&defex_get_mode_test_module);

MODULE_LICENSE("GPL v2");
