/*
 * sb_notify.h
 * Samsung Mobile Battery Notify Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SB_NOTIFY_H
#define __SB_NOTIFY_H __FILE__

#include <linux/notifier.h>
#include <linux/battery/sb_def.h>

enum sbn_type {
	SB_NOTIFY_UNKNOWN = 0,

	SB_NOTIFY_DEV_PROBE,
	SB_NOTIFY_DEV_SHUTDOWN,
	SB_NOTIFY_DEV_LIST,

	SB_NOTIFY_EVENT_MISC,
	SB_NOTIFY_EVENT_SIOP,
	SB_NOTIFY_EVENT_SLATE_MODE,

	SB_NOTIFY_MAX,

};

enum{
	OCP_WARN_ON,
	OCP_WARN_OFF
};

struct sbn_dev_list {
	const char		**list;
	unsigned int	count;
};

struct sbn_bit_event {
	unsigned int	value;
	unsigned int	mask;
};

#define SB_NOTIFY_DISABLE	(-3661)
#if IS_ENABLED(CONFIG_SB_NOTIFY)
int sb_notify_call(enum sbn_type ntype, sb_data *ndata);
int sb_notify_register(struct notifier_block *nb,
		notifier_fn_t notifier, const char *name, enum sb_dev_type type);

int sb_notify_unregister(struct notifier_block *nb);
#else
static inline int sb_notify_call(enum sbn_type ntype, sb_data *ndata)
{ return SB_NOTIFY_DISABLE; }

static inline int sb_notify_register(struct notifier_block *nb,
		notifier_fn_t notifier, const char *name, enum sb_dev_type type)
{ return SB_NOTIFY_DISABLE; }

static inline int sb_notify_unregister(struct notifier_block *nb)
{ return SB_NOTIFY_DISABLE; }
#endif

extern int register_ocp_warn_notifier(struct notifier_block *nb);
extern int unregister_ocp_warn_notifier(struct notifier_block *nb);
extern int ocp_warn_notifier_call_chain(unsigned long val);

#endif /* __SB_NOTIFY_H */

