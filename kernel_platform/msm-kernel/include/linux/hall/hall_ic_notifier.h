/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2021 Samsung Electronics
 */

#ifndef __HALL_NOTIFIER_H__
#define __HALL_NOTIFIER_H__

#include <linux/notifier.h>

struct hall_notifier_context {
	const char *name;
	int value;
};

extern int hall_notifier_register(struct notifier_block *n);
extern int hall_notifier_unregister(struct notifier_block *nb);
extern int hall_notifier_notify(const char *hall_name, int hall_value);
#endif /* __SEC_VIB_NOTIFIER_H__ */
