/* kunit_notifier.h
 *
 * Driver to initialize, register, unregister kunit test modules
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 * Ji-Hun Kim <ji_hun.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/notifier.h>
#include <kunit/mock.h>

extern int register_kunit_notifier(struct notifier_block *nb);
extern int unregister_kunit_notifier(struct notifier_block *nb);

extern struct kunit_suite sec_battery_test_module;
extern struct kunit_suite sec_adc_test_module;
extern struct kunit_suite sec_battery_dt_test_module;
extern struct kunit_suite sec_battery_misc__module;
extern struct kunit_suite sec_battery_sysfs_test_module;
extern struct kunit_suite sec_battery_thermal_test_module;
extern struct kunit_suite sec_battery_ttf_test_module;
extern struct kunit_suite sec_battery_vote_test_module;
extern struct kunit_suite sec_battery_wc_test_module;
extern struct kunit_suite sec_cisd_test_module;
extern struct kunit_suite sec_pd_test_module;
extern struct kunit_suite sec_step_charging_test_module;

/*
 * kunit_notifier_chain_init() - initialize kunit notifier for module built
 */
#define kunit_notifier_chain_init(module)					\
	extern struct kunit_suite module;					\
	static int kunit_run_notify_##module(struct notifier_block *self,	\
			unsigned long event, void *data)			\
	{									\
		if (kunit_run_tests((struct kunit_suite *)&module)) {		\
			pr_warn("kunit error: %s\n", module.name);		\
			return NOTIFY_BAD;					\
		}								\
		return NOTIFY_OK;						\
	}									\
	static struct notifier_block callchain_notifier_##module = {		\
		.notifier_call = kunit_run_notify_##module,			\
	};

#define kunit_notifier_chain_register(module)					\
	register_kunit_notifier(&callchain_notifier_##module);

#define kunit_notifier_chain_unregister(module)					\
	unregister_kunit_notifier(&callchain_notifier_##module);
