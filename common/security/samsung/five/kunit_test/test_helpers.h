/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef __LINUX_TEST_HELPERS_H
#define __LINUX_TEST_HELPERS_H

#include <kunit/test.h>
#include <kunit/mock.h>

#define DECLARE_NEW(test, type, ptr) \
	type *ptr = (type *)kunit_kzalloc(test, sizeof(type), GFP_KERNEL)

#define NEW(test, type) \
	((type *)kunit_kzalloc(test, sizeof(type), GFP_KERNEL))

// Create MOCK action object, initialize with action function
static inline struct mock_action *new_mock_action(
	struct kunit *test, void *action_func_ptr)
{
	DECLARE_NEW(test, struct mock_action, action_ptr);

	action_ptr->do_action = action_func_ptr;
	return action_ptr;
}

static inline struct mock_expectation *KunitReturns(
	struct mock_expectation *expectation,
	struct mock_action *return_action)
{
	return ActionOnMatch(expectation, return_action);
}

#endif // __LINUX_TEST_HELPERS_H
