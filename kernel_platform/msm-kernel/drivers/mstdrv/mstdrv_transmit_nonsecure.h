/*
 * @file mstdrv_transmit_nonsecure.h
 * @brief Header file for MST Nonsecure Transmit
 * Copyright (c) 2020, Samsung Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef MST_DRV_TRANSMIT_NONSECURE_H
#define MST_DRV_TRANSMIT_NONSECURE_H

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/device.h>

/* defines */
#define	ON				1	// On state
#define	OFF				0	// Off state
#define	TRACK1				1	// Track1 data
#define	TRACK2				2	// Track2 data

#define	BITS_PER_CHAR_TRACK1		7	// Number of bits per character for Track1
#define	BITS_PER_CHAR_TRACK2		5	// Number of bits per character Track2
#define LEADING_ZEROES			28	// Number of leading zeroes to data
#define TRAILING_ZEROES			28	// Number of trailing zeroes to data
#define LRC_CHAR_TRACK1			70	// LRC character ('&' in ISO/IEC 7811) for Track1
#define	PWR_EN_WAIT_TIME		11	// time (mS) to wait after MST+PER_EN signal sent
#define	MST_DATA_TIME_DURATION		300	// time (uS) duration for transferring MST bit

void init_spin_lock(void);
int init_mst_en_gpio(struct device *dev);
int init_mst_data_gpio(struct device *dev);
int mst_change(int mst_val, int mst_stat, int mst_data);
int transmit_mst_data(int track_num);

#endif /* MST_DRV_H */
