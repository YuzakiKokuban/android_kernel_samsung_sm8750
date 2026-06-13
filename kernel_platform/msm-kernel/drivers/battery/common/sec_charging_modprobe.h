/*
 *  sec_charging_modprobe.h
 *  Samsung Mobile Battery Header
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SEC_CHARGING_MODPROBE_H
#define __SEC_CHARGING_MODPROBE_H __FILE__

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/wait.h>

enum sec_chg_dev_info {
	SC_DEV_FG = 0x1,
	SC_DEV_MAIN_CHG = 0x2,
	SC_DEV_DIR_CHG = 0x4,
	SC_DEV_SEC_DIR_CHG = 0x8,
	SC_DEV_WRL_CHG = 0x10,
	SC_DEV_SB_MFC = 0x20,
	SC_DEV_MAIN_LIM = 0x40,
	SC_DEV_SUB_LIM = 0x80,
	SC_DEV_DUAL_FG = 0x100,
};

struct dev_init_info {
	wait_queue_head_t all_dev_wait;
	wait_queue_head_t depend_dev_wait;
	unsigned int dev;
};

extern int sec_chg_set_dev_init(unsigned int dev);
extern void sec_chg_check_modprobe(void);
extern void sec_chg_check_dev_modprobe(unsigned int dev);
extern void sec_chg_init_gdev(void);

#endif /* __SEC_CHARGING_MODPROBE_H */
