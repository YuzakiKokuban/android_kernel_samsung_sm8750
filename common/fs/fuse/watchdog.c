// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2024 Yoonho Shin <yoonho.shin@samsung.com>
 */

#include "fuse_i.h"

#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/freezer.h>

#ifndef ST_LOG
#define ST_LOG(fmt, ...) pr_err(fmt, ##__VA_ARGS__)
#endif

#define MAX_WAIT_FUSE_ABORT 2

struct fuse_daemon_watchdog_data {
	struct fuse_conn *fc;
	struct task_struct *group_leader_daemon;
};

static int watch_daemon_status(void *data)
{
	struct fuse_daemon_watchdog_data *fwd = data;
	struct fuse_conn *fc = fwd->fc;
	struct task_struct *daemon = fwd->group_leader_daemon;
	int cnt = 0;
	DECLARE_WAIT_QUEUE_HEAD_ONSTACK(wq);

	ST_LOG("<%s> dev = %u:%u  fuse watchdog start\n",
			__func__, MAJOR(fc->dev), MINOR(fc->dev));

	set_freezable();

	while (cnt < MAX_WAIT_FUSE_ABORT) {
		/*
		 * Use conditional wait to prevent race between system freezing
		 * request and sleep.
		 */
		wait_event_interruptible_timeout(wq,
				kthread_should_stop() || freezing(current) ||
				daemon->exit_state == EXIT_DEAD,
				msecs_to_jiffies(10 * MSEC_PER_SEC));

		if (kthread_should_stop()) {
			ST_LOG("<%s> dev = %u:%u  kthread_should_stop\n",
					__func__, MAJOR(fc->dev), MINOR(fc->dev));
			break;
		}

		/*
		 * The fuse daemon process have exited normally.
		 */
		if (daemon->exit_state == EXIT_DEAD) {
			ST_LOG("<%s> dev = %u:%u  EXIT_DEAD\n",
					__func__, MAJOR(fc->dev), MINOR(fc->dev));
			break;
		}

		if (try_to_freeze())
			continue;

		/*
		 * P231024-04181
		 * : The group_leader is a zombie, and other threads have exited.
		 *
		 * The group leader has completed do_exit(). However, the binder
		 * thread must call do_notify_parent() so that the parent process
		 * can clean up the MediaProvider process.
		 *
		 * do_exit()
		 *   exit_notify()
		 *     do_notify_parent()
		 */
		if (daemon->exit_state == EXIT_ZOMBIE) {
			ST_LOG("<%s> dev = %u:%u  EXIT_ZOMBIE\n",
					__func__, MAJOR(fc->dev), MINOR(fc->dev));
			cnt++;
			continue;
		}

		/*
		 * P231128-07435
		 * : Waiting for the binder thread to finish core dump.
		 *
		 * all threads wait at
		 *   do_exit()
		 *     coredump_task_exit()
		 */
		if (daemon->flags & PF_POSTCOREDUMP) {
			ST_LOG("<%s> dev = %u:%u  PF_POSTCOREDUMP\n",
					__func__, MAJOR(fc->dev), MINOR(fc->dev));
			cnt++;
			continue;
		}

	}

	if (cnt >= MAX_WAIT_FUSE_ABORT) {
		WARN_ON(cnt > MAX_WAIT_FUSE_ABORT);
		ST_LOG("<%s> dev = %u:%u  fuse watchdog abort_fuse_conn\n",
				__func__, MAJOR(fc->dev), MINOR(fc->dev));
		fuse_abort_conn(fc);
	}

	ST_LOG("<%s> dev = %u:%u  fuse watchdog end\n",
			__func__, MAJOR(fc->dev), MINOR(fc->dev));

	put_task_struct(daemon);
	kfree(fwd);
	return 0;
}

// TODO: Temp code. Need to change to mount option in the future.
static bool am_i_mp(void)
{
	struct task_struct *gl = get_task_struct(current->group_leader);
	bool ret = false;

	if (!strncmp(gl->comm, "rs.media.module", strlen("rs.media.module")))
		ret = true;

	put_task_struct(gl);

	return ret;
}

void fuse_daemon_watchdog_start(struct fuse_conn *fc)
{
	struct fuse_daemon_watchdog_data *fwd;
	struct task_struct *watchdog_thread;

	if (!am_i_mp())
		return;

	fwd = kmalloc(sizeof(struct fuse_daemon_watchdog_data), GFP_KERNEL);
	if (!fwd) {
		ST_LOG("<%s> dev = %u:%u  failed to alloc watchdog data\n",
				__func__, MAJOR(fc->dev), MINOR(fc->dev));
		return;
	}

	fwd->fc = fc;
	fwd->group_leader_daemon = get_task_struct(current->group_leader);

	watchdog_thread = kthread_run(watch_daemon_status, fwd,
			"fuse-wd%u:%u", MAJOR(fc->dev), MINOR(fc->dev));

	if (IS_ERR(watchdog_thread)) {
		ST_LOG("<%s> dev = %u:%u  failed to start watchdog thread\n",
				__func__, MAJOR(fc->dev), MINOR(fc->dev));
		put_task_struct(fwd->group_leader_daemon);
		kfree(fwd);
		return;
	}

	fc->watchdog_thread = get_task_struct(watchdog_thread);
}

void fuse_daemon_watchdog_stop(struct fuse_conn *fc)
{
	if (fc->watchdog_thread) {
		kthread_stop(fc->watchdog_thread);
		put_task_struct(fc->watchdog_thread);
	}
}
