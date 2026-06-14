/*
 * Copyright (C) 2020 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __SEC_STAR__H
#define __SEC_STAR__H

#include <linux/types.h>

/* #define FEATURE_STAR_WAKELOCK */

#ifdef FEATURE_STAR_WAKELOCK
#include "snvm_wakelock.h"
#endif

enum sec_hal_e {
	SEC_HAL_I2C = 0,
	SEC_HAL_SPI,
};

typedef struct star_dev_s {
	const char *name;
	int hal_type;
	void *client;
	int (*power_on)(void);
	int (*power_off)(void);
	int (*reset)(void);
} star_dev_t;

typedef struct sec_star_s {
	star_dev_t *dev;
	struct mutex lock;
#ifdef FEATURE_STAR_WAKELOCK
	struct snvm_wake_lock snvm_wake_lock;
#endif
	struct miscdevice misc;
	void *hal;
	void *protocol;
	unsigned char *rsp;
	unsigned int rsp_size;
	unsigned int access;
	int direct;
} sec_star_t;

sec_star_t *star_open(star_dev_t *dev);
void star_close(sec_star_t *star);

#endif
