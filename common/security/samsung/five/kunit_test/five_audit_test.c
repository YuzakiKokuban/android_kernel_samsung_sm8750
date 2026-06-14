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
#include <linux/fs.h>
#include <linux/task_integrity.h>
#include "five_audit.h"
#include "test_helpers.h"

#define FILE_ADDR 0xABCE

static const uint8_t cause[] = "cause", op[] = "op";

DEFINE_FUNCTION_MOCK_VOID_RETURN(five_audit_msg, PARAMS(struct task_struct *,
		struct file *, const char *, enum task_integrity_value,
		enum task_integrity_value, const char *, int))

DEFINE_FUNCTION_MOCK_VOID_RETURN(call_five_dsms_reset_integrity,
		PARAMS(const char *, int, const char *))

static void five_audit_info_test(struct kunit *test)
{
	struct file *file;
	int result = 0xab;
	struct task_struct *task = current;

	file = (struct file *)FILE_ADDR;

	KunitReturns(KUNIT_EXPECT_CALL(five_audit_msg(ptr_eq(test, task),
	ptr_eq(test, file), streq(test, op), int_eq(test, INTEGRITY_NONE),
	int_eq(test, INTEGRITY_NONE), streq(test, cause),
	int_eq(test, result))), int_return(test, 0));

	five_audit_info(task, file,
		op, INTEGRITY_NONE, INTEGRITY_NONE, cause, result);
}

static void five_audit_err_test_1(struct kunit *test)
{
	struct file *file;
	struct task_struct *task = current;
	int result = 1;

	file = (struct file *)FILE_ADDR;
	Times(1, KunitReturns(KUNIT_EXPECT_CALL(five_audit_msg(
		ptr_eq(test, task),
		ptr_eq(test, file), streq(test, op),
		int_eq(test, INTEGRITY_NONE), int_eq(test, INTEGRITY_NONE),
		streq(test, cause), int_eq(test, result))),
		int_return(test, 0)));

	five_audit_err(task, file,
		op, INTEGRITY_NONE, INTEGRITY_NONE, cause, result);
}

static void five_audit_err_test_2(struct kunit *test)
{
	struct file *file;
	struct task_struct *task = current;
	int result = 0;

	file = (struct file *)FILE_ADDR;
	KunitReturns(KUNIT_EXPECT_CALL(five_audit_msg(ptr_eq(test, task),
		ptr_eq(test, file), streq(test, op),
		int_eq(test, INTEGRITY_NONE), int_eq(test, INTEGRITY_NONE),
		streq(test, cause), int_eq(test, result))),
		int_return(test, 0));

	five_audit_err(task, file,
		op, INTEGRITY_NONE, INTEGRITY_NONE, cause, result);
}

static int security_five_test_init(struct kunit *test)
{
	return 0;
}

static void security_five_test_exit(struct kunit *test)
{
	return;
}

static struct kunit_case security_five_test_cases[] = {
	KUNIT_CASE(five_audit_info_test),
	KUNIT_CASE(five_audit_err_test_1),
	KUNIT_CASE(five_audit_err_test_2),
	{},
};

static struct kunit_suite security_five_test_module = {
	.name = "five-audit-test",
	.init = security_five_test_init,
	.exit = security_five_test_exit,
	.test_cases = security_five_test_cases,
};

kunit_test_suites(&security_five_test_module);

MODULE_LICENSE("GPL v2");
