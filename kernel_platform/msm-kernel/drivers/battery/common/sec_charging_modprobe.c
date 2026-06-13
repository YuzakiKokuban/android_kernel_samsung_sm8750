/*
 *  sec_charging_modprobe.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_charging_modprobe.h"

#define MODPROBE_TIMEOUT 20000

#if IS_MODULE(CONFIG_BATTERY_SAMSUNG)
static struct dev_init_info gdev_init;

void sec_chg_init_gdev(void)
{
	gdev_init.dev = 0;
	init_waitqueue_head(&gdev_init.all_dev_wait);
	init_waitqueue_head(&gdev_init.depend_dev_wait);
}

int sec_chg_set_dev_init(unsigned int dev)
{
	gdev_init.dev |= dev;
	wake_up(&gdev_init.all_dev_wait);
	wake_up(&gdev_init.depend_dev_wait);

	return 0;
}
EXPORT_SYMBOL(sec_chg_set_dev_init);

void sec_chg_check_modprobe(void)
{
	unsigned int check_dev = 0;

	check_dev |= SC_DEV_FG | SC_DEV_MAIN_CHG;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	check_dev |= SC_DEV_MAIN_LIM | SC_DEV_SUB_LIM;
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	check_dev |= SC_DEV_DIR_CHG | SC_DEV_SEC_DIR_CHG;
#endif
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	check_dev |= SC_DEV_WRL_CHG;
#if IS_ENABLED(CONFIG_SB_MFC)
	check_dev |= SC_DEV_SB_MFC;
#endif
#endif
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	check_dev |= SC_DEV_DUAL_FG;
#endif

	if (!wait_event_timeout(gdev_init.all_dev_wait,
		gdev_init.dev == check_dev, msecs_to_jiffies(MODPROBE_TIMEOUT)))
		pr_info("%s: dev_init timeout(0x%x)\n", __func__, gdev_init.dev);
	else
		pr_info("%s: takes time to wait(0x%x)\n", __func__, gdev_init.dev);
}
EXPORT_SYMBOL(sec_chg_check_modprobe);

void sec_chg_check_dev_modprobe(unsigned int dev)
{
	if (!wait_event_timeout(gdev_init.depend_dev_wait,
		gdev_init.dev & dev, msecs_to_jiffies(MODPROBE_TIMEOUT)))
		pr_info("%s: dev_init timeout(0x%x)\n", __func__, dev);
	else
		pr_info("%s: takes time to wait(0x%x)\n", __func__, dev);
}
EXPORT_SYMBOL(sec_chg_check_dev_modprobe);
#else
void sec_chg_init_gdev(void) { }
int sec_chg_set_dev_init(unsigned int dev) { return 0; }
void sec_chg_check_modprobe(void) { }
void sec_chg_check_dev_modprobe(unsigned int dev) { }
#endif
