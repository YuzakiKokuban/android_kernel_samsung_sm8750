#ifndef __INTERNAL__SEC_CRASHKEY_H__
#define __INTERNAL__SEC_CRASHKEY_H__

#include <linux/list.h>
#include <linux/notifier.h>
#include <linux/ratelimit.h>

#include <linux/samsung/bsp/sec_key_notifier.h>
#include <linux/samsung/builder_pattern.h>

struct crashkey_kelog {
	const struct sec_key_notifier_param *desired;
	size_t nr_pattern;
	size_t sequence;
	unsigned int *used_key;
	size_t nr_used_key;
};

struct crashkey_timer {
	struct ratelimit_state rs;
	int interval;
};

struct crashkey_notify {
	struct raw_notifier_head list;
	struct notifier_block panic;
	const char *panic_msg;
};

struct crashkey_drvdata {
	struct builder bd;
	struct list_head list;
	struct notifier_block nb;
	const char *name;
	struct crashkey_kelog keylog;
	struct crashkey_timer timer;
	struct crashkey_notify notify;
};

#endif /* __INTERNAL__SEC_CRASHKEY_H__ */
