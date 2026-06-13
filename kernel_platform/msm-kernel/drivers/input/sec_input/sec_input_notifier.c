// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>

static BLOCKING_NOTIFIER_HEAD(sec_input_notifier_list);

/*
 * sec_input_register_notify
 * @nb: pointer of blocking notifier chain structure
 * @notifier_fn_t: register notifier callback function
 *
 * register universal notifier for any development issue.
 * ex) folder open/close, seucre touch enable/disable ...
 */
void sec_input_register_notify(struct notifier_block *nb, notifier_fn_t notifier_call, int priority)
{
	nb->notifier_call = notifier_call;
	nb->priority = priority;
	blocking_notifier_chain_register(&sec_input_notifier_list, nb);
}
EXPORT_SYMBOL(sec_input_register_notify);

/*
 * sec_input_unregister_notify
 * @nb: pointer of blocking notifier chain structure
 *
 * unregister notifier
 */
void sec_input_unregister_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_unregister(&sec_input_notifier_list, nb);
}
EXPORT_SYMBOL(sec_input_unregister_notify);

/*
 * sec_input_notify
 * @nb: pointer of blocking notifier chain structure
 * data: notifier type is defined in sec_input.h(enum sec_input_notify_t)
 * v: structure data
 *
 * notifier call function
 */
int sec_input_notify(struct notifier_block *nb, unsigned long noti, void *v)
{
	return blocking_notifier_call_chain(&sec_input_notifier_list, noti, v);
}
EXPORT_SYMBOL(sec_input_notify);

/*
 * sec_input_self_request_notify
 * @nb: pointer of blocking notifier chain structure
 *
 * only test
 */
int sec_input_self_request_notify(struct notifier_block *nb)
{
	return nb->notifier_call(nb, 0, NULL);
}
EXPORT_SYMBOL(sec_input_self_request_notify);

MODULE_DESCRIPTION("Samsung input notifier");
MODULE_LICENSE("GPL");

