/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __LEDS_QTI_FLASH_H
#define __LEDS_QTI_FLASH_H

#include <linux/leds.h>

#define ENABLE_REGULATOR		BIT(0)
#define DISABLE_REGULATOR		BIT(1)
#define QUERY_MAX_AVAIL_CURRENT		BIT(2)

/**
 * struct flash_led_param: QTI flash LED parameter data
 * @on_time_ms	: Time to wait before strobing the switch
 * @off_time_ms	: Time to wait to turn off LED after strobing switch
 */
struct flash_led_param {
	u64 on_time_ms;
	u64 off_time_ms;
};

#if IS_ENABLED(CONFIG_LEDS_QTI_FLASH)
int qti_flash_led_prepare(struct led_trigger *trig,
			int options, int *max_current);
int qti_flash_led_set_param(struct led_trigger *trig,
			struct flash_led_param param);
#if IS_ENABLED(CONFIG_SENSORS_STK6D2X) || IS_ENABLED(CONFIG_SENSORS_TSL2511) || IS_ENABLED(CONFIG_SENSORS_STK6DAX)
	int qti_flash_led_set_strobe_sel(struct led_trigger *trig,
			int strobe_sel);
#endif
#else
static inline int qti_flash_led_prepare(struct led_trigger *trig,
					int options, int *max_current)
{
	return -EINVAL;
}

static inline int qti_flash_led_set_param(struct led_trigger *trig,
			struct flash_led_param param);
{
	return -EINVAL;
}

#if IS_ENABLED(CONFIG_SENSORS_STK6D2X) || IS_ENABLED(CONFIG_SENSORS_STK6DAX)
static inline int qti_flash_led_set_strobe_sel(struct led_trigger *trig,
			int strobe_sel);
{
	return -EINVAL;
}
#endif
#endif
#endif
