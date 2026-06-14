/*
 *  common_timer.c - Linux kernel modules for sensortek stk6d2x
 *  ambient light sensor (Common function)
 *
 *  Copyright (C) 2019 Bk, sensortek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/input.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/pm_wakeup.h>
#include <common_define.h>

typedef struct timer_manager timer_manager;

struct timer_manager
{
	struct work_struct          stk_work;
	struct hrtimer              stk_hrtimer;
	struct workqueue_struct     *stk_wq;
	ktime_t                     timer_interval;

	stk_timer_info              *timer_info;
} timer_mgr_default = {.timer_info = 0};

#define MAX_LINUX_TIMER_MANAGER_NUM     5

timer_manager linux_timer_mgr[MAX_LINUX_TIMER_MANAGER_NUM];

static timer_manager* parser_timer(struct hrtimer *timer)
{
	int timer_idx = 0;

	if (timer == NULL)
	{
		return NULL;
	}

	for (timer_idx = 0; timer_idx < MAX_LINUX_TIMER_MANAGER_NUM; timer_idx ++)
	{
		if (&linux_timer_mgr[timer_idx].stk_hrtimer == timer)
		{
			return &linux_timer_mgr[timer_idx];
		}
	}

	return NULL;
}

static enum hrtimer_restart timer_func(struct hrtimer *timer)
{
	timer_manager *timer_mgr = parser_timer(timer);

	if (timer_mgr == NULL)
	{
		printk(KERN_ERR "%s: timer_mgr is NULL\n", __func__);
		return HRTIMER_NORESTART;
	}

	if (timer_mgr->stk_wq == NULL)
	{
		printk(KERN_ERR "%s: timer_mgr->stk_wq is NULL\n", __func__);
		return HRTIMER_NORESTART;
	}

	if (timer_mgr->timer_info == NULL)
	{
		printk(KERN_ERR "%s: timer_mgr->timer_info is NULL\n", __func__);
		return HRTIMER_NORESTART;
	}

	queue_work(timer_mgr->stk_wq, &timer_mgr->stk_work);
	hrtimer_forward_now(&timer_mgr->stk_hrtimer, timer_mgr->timer_interval);
	return HRTIMER_RESTART;
}

static timer_manager* parser_work(struct work_struct *work)
{
	int timer_idx = 0;

	if (work == NULL)
	{
		return NULL;
	}

	for (timer_idx = 0; timer_idx < MAX_LINUX_TIMER_MANAGER_NUM; timer_idx ++)
	{
		if (&linux_timer_mgr[timer_idx].stk_work == work)
		{
			return &linux_timer_mgr[timer_idx];
		}
	}

	return NULL;
}

static void timer_callback(struct work_struct *work)
{
	timer_manager *timer_mgr = parser_work(work);

	if (timer_mgr == NULL)
	{
		return;
	}

	timer_mgr->timer_info->timer_cb(timer_mgr->timer_info);
}

int register_timer(stk_timer_info *t_info)
{
	int timer_idx = 0;

	if (t_info == NULL)
	{
		return -1;
	}

	for (timer_idx = 0; timer_idx < MAX_LINUX_TIMER_MANAGER_NUM; timer_idx ++)
	{
		if (!linux_timer_mgr[timer_idx].timer_info)
		{
			linux_timer_mgr[timer_idx].timer_info = t_info;
			break;
		}
		else
		{
			if (linux_timer_mgr[timer_idx].timer_info == t_info)
			{
				//already register
				if (linux_timer_mgr[timer_idx].timer_info->change_interval_time)
				{
					linux_timer_mgr[timer_idx].timer_info->change_interval_time = 0;
					printk(KERN_ERR "%s: chang interval time\n", __func__);
					switch (linux_timer_mgr[timer_idx].timer_info->timer_unit)
					{
						case N_SECOND:
							linux_timer_mgr[timer_idx].timer_interval = ns_to_ktime(linux_timer_mgr[timer_idx].timer_info->interval_time);
							break;

						case U_SECOND:
							linux_timer_mgr[timer_idx].timer_interval = ns_to_ktime(linux_timer_mgr[timer_idx].timer_info->interval_time * NSEC_PER_USEC);
							break;

						case M_SECOND:
							linux_timer_mgr[timer_idx].timer_interval = ns_to_ktime(linux_timer_mgr[timer_idx].timer_info->interval_time * NSEC_PER_MSEC);
							break;

						case SECOND:
							break;
					}
					return 0;
				}
				printk(KERN_ERR "%s: this timer is registered\n", __func__);
				return -1;
			}
		}
	}

	// if search/register timer manager not successfully
	if (timer_idx == MAX_LINUX_TIMER_MANAGER_NUM)
	{
		printk(KERN_ERR "%s: timer_idx out of range %d\n", __func__, timer_idx);
		return -1;
	}

	printk(KERN_ERR "%s: register timer name %s\n", __func__, linux_timer_mgr[timer_idx].timer_info->wq_name);
	linux_timer_mgr[timer_idx].stk_wq = create_singlethread_workqueue(linux_timer_mgr[timer_idx].timer_info->wq_name);
	if (linux_timer_mgr[timer_idx].stk_wq == NULL)
	{
		printk(KERN_ERR "%s: create single thread workqueue fail\n", __func__);
		linux_timer_mgr[timer_idx].timer_info = 0;
		return -1;
	}

	INIT_WORK(&linux_timer_mgr[timer_idx].stk_work, timer_callback);
	hrtimer_init(&linux_timer_mgr[timer_idx].stk_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	switch (linux_timer_mgr[timer_idx].timer_info->timer_unit)
	{
		case N_SECOND:
			linux_timer_mgr[timer_idx].timer_interval = ns_to_ktime(linux_timer_mgr[timer_idx].timer_info->interval_time);
			break;

		case U_SECOND:
			linux_timer_mgr[timer_idx].timer_interval = ns_to_ktime(linux_timer_mgr[timer_idx].timer_info->interval_time * NSEC_PER_USEC);
			break;

		case M_SECOND:
			linux_timer_mgr[timer_idx].timer_interval = ns_to_ktime(linux_timer_mgr[timer_idx].timer_info->interval_time * NSEC_PER_MSEC);
			break;

		case SECOND:
			break;
	}

	linux_timer_mgr[timer_idx].stk_hrtimer.function = timer_func;
	linux_timer_mgr[timer_idx].timer_info->is_exist = true;
	return 0;
}

int start_timer(stk_timer_info *t_info)
{
	int timer_idx = 0;

	for (timer_idx = 0; timer_idx < MAX_LINUX_TIMER_MANAGER_NUM; timer_idx ++)
	{
		if (linux_timer_mgr[timer_idx].timer_info == t_info)
		{
			if (linux_timer_mgr[timer_idx].timer_info->is_exist)
			{
				if (!linux_timer_mgr[timer_idx].timer_info->is_active)
				{
					hrtimer_start(&linux_timer_mgr[timer_idx].stk_hrtimer, linux_timer_mgr[timer_idx].timer_interval, HRTIMER_MODE_REL);
					linux_timer_mgr[timer_idx].timer_info->is_active = true;
					printk(KERN_ERR "%s: start timer name %s\n", __func__, linux_timer_mgr[timer_idx].timer_info->wq_name);
				}
				else
				{
					printk(KERN_INFO "%s: %s was already running\n", __func__, linux_timer_mgr[timer_idx].timer_info->wq_name);
				}
			}

			return 0;
		}
	}

	return -1;
}

int stop_timer(stk_timer_info *t_info)
{
	int timer_idx = 0;

	for (timer_idx = 0; timer_idx < MAX_LINUX_TIMER_MANAGER_NUM; timer_idx ++)
	{
		if (linux_timer_mgr[timer_idx].timer_info == t_info)
		{
			if (linux_timer_mgr[timer_idx].timer_info->is_exist)
			{
				if (linux_timer_mgr[timer_idx].timer_info->is_active)
				{
					hrtimer_cancel(&linux_timer_mgr[timer_idx].stk_hrtimer);
					drain_workqueue(linux_timer_mgr[timer_idx].stk_wq);
					linux_timer_mgr[timer_idx].timer_info->is_active = false;
					printk(KERN_ERR "%s: stop timer name %s\n", __func__, linux_timer_mgr[timer_idx].timer_info->wq_name);
				}
				else
				{
					printk(KERN_ERR "%s: %s stop already stop\n", __func__, linux_timer_mgr[timer_idx].timer_info->wq_name);
				}
			}

			return 0;
		}
	}

	return -1;
}

int remove_timer(stk_timer_info *t_info)
{
	int timer_idx = 0;

	for (timer_idx = 0; timer_idx < MAX_LINUX_TIMER_MANAGER_NUM; timer_idx ++)
	{
		if (linux_timer_mgr[timer_idx].timer_info == t_info)
		{
			if (linux_timer_mgr[timer_idx].timer_info->is_exist)
			{
				if (linux_timer_mgr[timer_idx].timer_info->is_active)
				{
					hrtimer_try_to_cancel(&linux_timer_mgr[timer_idx].stk_hrtimer);
					destroy_workqueue(linux_timer_mgr[timer_idx].stk_wq);
					cancel_work_sync(&linux_timer_mgr[timer_idx].stk_work);
					linux_timer_mgr[timer_idx].timer_info->is_active = false;
					linux_timer_mgr[timer_idx].timer_info->is_exist = false;
					linux_timer_mgr[timer_idx].timer_info = 0;
				}
			}

			return 0;
		}
	}

	return -1;
}

void busy_wait(unsigned long min, unsigned long max, BUSY_WAIT_TYPE mode)
{
	if ((!min) || (!max) || (max < min))
	{
		return;
	}

	if (mode == US_RANGE_DELAY)
	{
		usleep_range(min, max);
	}

	if (mode == MS_DELAY)
	{
		msleep(max);
	}
}
const struct stk_timer_ops stk_t_ops =
{
	.register_timer         = register_timer,
	.start_timer            = start_timer,
	.stop_timer             = stop_timer,
	.remove                 = remove_timer,
	.busy_wait              = busy_wait,
};
