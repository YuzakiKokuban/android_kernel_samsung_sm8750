#ifndef __INTERNAL__SEC_CRASHKEY_LONG_H__
#define __INTERNAL__SEC_CRASHKEY_LONG_H__

#include <linux/notifier.h>
#include <linux/timer.h>

#include <linux/samsung/bsp/sec_key_notifier.h>
#include <linux/samsung/builder_pattern.h>

struct crashkey_long_keylog {
	unsigned long *bitmap_received;
	size_t sz_bitmap;
	const unsigned int *used_key;
	size_t nr_used_key;
};

struct crashkey_long_notify {
	struct timer_list tl;
	unsigned int expire_msec;
	struct raw_notifier_head list;
	struct notifier_block panic;
	const char *panic_msg;
};

struct crashkey_long_drvdata {
	struct builder bd;
	spinlock_t state_lock;	/* key_notifer and input events */
	bool nb_connected;
	struct notifier_block nb;
	struct crashkey_long_keylog keylog;
	struct crashkey_long_notify notify;
};

#endif /* __INTERNAL__SEC_CRASHKEY_LONG_H__ */
