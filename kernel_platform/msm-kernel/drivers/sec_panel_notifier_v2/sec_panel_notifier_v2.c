// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sec_panel_notifier_v2.h>

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)

static BLOCKING_NOTIFIER_HEAD(panel_notifier_list);

/**
 *	panel_notifier_register - register a client notifier
 *	@nb: notifier block to callback on events
 */
int panel_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&panel_notifier_list, nb);
}
EXPORT_SYMBOL(panel_notifier_register);

/**
 *	panel_notifier_unregister - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int panel_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&panel_notifier_list, nb);
}
EXPORT_SYMBOL(panel_notifier_unregister);

/**
 *	valid_panel_notifier_event - validate panel notifier event
 *	@event: panel notifier event
 *	@ev_data : panel notifier event data
 */
static bool valid_panel_notifier_event(unsigned long event,
		struct panel_notifier_event_data *ev_data)
{
	if (event >= MAX_PANEL_EVENT) {
		pr_err("invalid panel event(event:%lu)\n", event);
		return false;
	}

	if (!ev_data) {
		pr_err("panel event data is null(event:%lu)\n", event);
		return false;
	}

	if (event == PANEL_EVENT_PANEL_STATE_CHANGED ||
		event == PANEL_EVENT_UB_CON_STATE_CHANGED ||
		event == PANEL_EVENT_TEST_MODE_STATE_CHANGED) {
		if (ev_data->state == PANEL_EVENT_STATE_NONE) {
			pr_err("panel event state is none(event:%lu)\n", event);
			return false;
		}
	}

	return true;
}

/**
 * panel_notifier_call_chain - notify clients
 *
 */
int panel_notifier_call_chain(unsigned long val, void *v)
{
	if (!valid_panel_notifier_event(val, v))
		return -EINVAL;

	return blocking_notifier_call_chain(&panel_notifier_list, val, v);
}
EXPORT_SYMBOL(panel_notifier_call_chain);

#endif

MODULE_DESCRIPTION("Samsung panel notifier");
MODULE_LICENSE("GPL");
