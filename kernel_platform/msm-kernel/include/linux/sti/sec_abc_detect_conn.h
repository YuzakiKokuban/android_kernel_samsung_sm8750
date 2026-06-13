/*
 * include/linux/sti/sec_abc_detect_conn.h
 *
 * COPYRIGHT(C) 2017 Samsung Electronics Co., Ltd. All Right Reserved.
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

#ifndef SEC_DETECT_CONN_H
#define SEC_DETECT_CONN_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/sec_class.h>
#include <linux/version.h>
#include <linux/pinctrl/consumer.h>

#define DET_CONN_MAX_NUM_GPIOS 32
#define UEVENT_CONN_MAX_DEV_NAME 64
#define DET_CONN_GPIO_IRQ_NOT_INIT 0
#define DET_CONN_GPIO_IRQ_ENABLED 1
#define DET_CONN_GPIO_IRQ_DISABLED 2
#define DET_CONN_DEBOUNCE_TIME_MS 300

#define SEC_CONN_PRINT(format, ...) \
	pr_info("[sec_abc_detect_conn] " format, ##__VA_ARGS__)

struct sec_det_conn_p_data {
	const char *name[DET_CONN_MAX_NUM_GPIOS];
	int irq_gpio[DET_CONN_MAX_NUM_GPIOS];
	int irq_number[DET_CONN_MAX_NUM_GPIOS];
	unsigned int irq_type[DET_CONN_MAX_NUM_GPIOS];
	struct sec_det_conn_info *pinfo;
	int gpio_last_cnt;
	int gpio_total_cnt;
};

struct sec_det_conn_info {
	struct device *dev;
	int irq_enabled[DET_CONN_MAX_NUM_GPIOS];
	struct sec_det_conn_p_data *pdata;
};

static char sec_detect_available_pins_string[15 * 10] = {0,};
extern struct sec_det_conn_info *gpinfo;

void create_current_connection_state_sysnode_files(struct sec_det_conn_info *pinfo);
void create_connector_disconnected_count_sysnode_file(struct sec_det_conn_info *pinfo);
void increase_connector_disconnected_count(int index, struct sec_det_conn_info *pinfo);

#if IS_ENABLED(CONFIG_SEC_ABC_HUB)
#define ABCEVENT_CONN_MAX_DEV_STRING 120
void sec_abc_send_event(char *str);
int sec_abc_get_enabled(void);
#endif
#endif
