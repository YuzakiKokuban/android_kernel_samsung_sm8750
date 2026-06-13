/******************************************************************************
 *
 * Copyright 2023 NXP Semiconductors
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************/
#ifndef _ESE_RESET_H_
#define _ESE_RESET_H_
#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include "p73.h"
/*!
 * \brief Delay time required for ese reset by gpio
 */
#define ESE_GPIO_RESET_WAIT_TIME_USEC (12000)

/*!
 * \brief Guard time delay to prevent back to back gpio reset requests
 */
#define ESE_GPIO_RST_GUARD_TIME_MS (50)

/*!
 * \brief Argument values to be passed to ioctl call
 */
#define ESE_SOFT_RESET (1UL)
#define ESE_HARD_RESET (2UL)
#define ESE_DOMAIN_RESET (5UL)

/*!
 * \brief To denote gpio reset currently in progress
 */
typedef struct reset_timer {
	int rst_gpio;
	bool in_progress;
	struct mutex reset_mutex;
	struct timer_list timer;
} reset_timer_t;

void ese_reset_init(void);
int perform_ese_gpio_reset(int rst_gpio);
void ese_reset_deinit(void);
int ese_reset_gpio_setup(struct p61_spi_platform_data *data);
#endif
