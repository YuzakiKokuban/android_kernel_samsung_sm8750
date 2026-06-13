/*
 * SNVM Wakelock
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 */

#ifndef _SNVM_WAKELOCK_H
#define _SNVM_WAKELOCK_H

#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/version.h>

#define wake_lock_init(a, b, c)	snvm_wake_lock_init(a, c)
#define wake_lock_destroy(a)	snvm_wake_lock_destroy(a)
#define wake_lock_timeout(a, b)	snvm_wake_lock_timeout(a, b)
#define wake_lock_active(a)	snvm_wake_lock_active(a)
#define wake_lock(a)		snvm_wake_lock(a)
#define wake_unlock(a)		snvm_wake_unlock(a)

struct snvm_wake_lock {
	struct wakeup_source *ws;
};

static inline void snvm_wake_lock_init(struct snvm_wake_lock *lock, const char *name)
{
#if CONFIG_SEC_SNVM_WAKELOCK_METHOD == 2
	lock->ws = wakeup_source_register(NULL, name);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)) || CONFIG_SEC_SNVM_WAKELOCK_METHOD == 1
	wakeup_source_init(lock->ws, name); /* 4.19 R */
	if (!(lock->ws)) {
		lock->ws = wakeup_source_create(name); /* 4.19 Q */
		if (lock->ws)
			wakeup_source_add(lock->ws);
	}
#else
	lock->ws = wakeup_source_register(NULL, name); /* 5.4 R */
#endif
}

static inline void snvm_wake_lock_destroy(struct snvm_wake_lock *lock)
{
	if (lock->ws)
		wakeup_source_unregister(lock->ws);
}

static inline void snvm_wake_lock(struct snvm_wake_lock *lock)
{
	if (lock->ws)
		__pm_stay_awake(lock->ws);
}

static inline void snvm_wake_lock_timeout(struct snvm_wake_lock *lock, long timeout)
{
	if (lock->ws)
		__pm_wakeup_event(lock->ws, jiffies_to_msecs(timeout));
}

static inline void snvm_wake_unlock(struct snvm_wake_lock *lock)
{
	if (lock->ws)
		__pm_relax(lock->ws);
}

static inline int snvm_wake_lock_active(struct snvm_wake_lock *lock)
{
	return lock->ws->active;
}

#endif
