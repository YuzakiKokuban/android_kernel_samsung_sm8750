/* abc_hub.h
 *
 * Abnormal Behavior Catcher Hub Driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Sangsu Ha <sangsu.ha@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef SEC_ABC_HUB_H
#define SEC_ABC_HUB_H

#include <linux/sti/abc_common.h>

/******************************************/
/****************** Data ******************/
/******************************************/

/*********** sub module : cond ************/
#if IS_ENABLED(CONFIG_SEC_ABC_HUB_COND)

struct sub_cond_pdata {
	// common
	int init;
	int enabled;
};
#endif
/*********** sub module : bootc ************/
#if IS_ENABLED(CONFIG_SEC_ABC_HUB_BOOTC)
#define BOOTC_OFFSET_DATA_CNT 1
#define BOOTC_OFFSET_STR_MAX 100

struct abc_hub_bootc_offset_data {
	char module[BOOTC_OFFSET_STR_MAX];
	int offset;
};

struct sub_bootc_pdata {
	// common
	int init;
	int enabled;
	int bootc_time;
	// custom
	int time_spec;
	int time_spec_offset;
	struct abc_hub_bootc_offset_data offset_data[BOOTC_OFFSET_DATA_CNT];
	struct workqueue_struct *workqueue;
	struct delayed_work bootc_work;
};
#endif

/****************** Common *****************/

/* It will be added if sub module is added. */
enum {
	ABC_HUB_DISABLED,
	ABC_HUB_ENABLED,
};

struct abc_hub_platform_data {
	unsigned int nSub;
#if IS_ENABLED(CONFIG_SEC_ABC_HUB_COND)
	struct sub_cond_pdata cond;
#endif
#if IS_ENABLED(CONFIG_SEC_ABC_HUB_BOOTC)
	struct sub_bootc_pdata bootc_pdata;
#endif
};

struct abc_hub_info {
	struct device *dev;
	struct abc_hub_platform_data *pdata;
	int enabled;
};

/******************************************/
/**************** Function ****************/
/******************************************/

/*********** sub module : cond ************/
#if IS_ENABLED(CONFIG_SEC_ABC_HUB_COND)
void abc_hub_cond_enable(struct device *dev, int enable);
#endif

/*********** sub module : bootc ************/
#if IS_ENABLED(CONFIG_SEC_ABC_HUB_BOOTC)
int parse_bootc_data(struct device *dev,
		     struct abc_hub_platform_data *pdata,
		     struct device_node *np);
int abc_hub_bootc_init(struct device *dev);
void abc_hub_bootc_enable(struct device *dev, int enable);
#endif

extern struct abc_hub_info *abc_hub_pinfo;
/****************** Common *****************/
void abc_hub_send_event(char *str);
int abc_hub_get_enabled(void);

#endif
