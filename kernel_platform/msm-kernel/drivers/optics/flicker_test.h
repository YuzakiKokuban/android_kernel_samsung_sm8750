/*
 * Copyright (C) 2021 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EOL_TEST_H
#define EOL_TEST_H

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/leds.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/version.h>

/*
 * Flicker Sensor Self test module
 *
 * Uses LEDs in the system.
 * If you want to add a new LED,
 * add a header below and define dt node name.
 *
 * You can build this with CONFIG_EOL_TEST
 */

#if IS_ENABLED(CONFIG_LEDS_S2MPB02)
#include <linux/leds-s2mpb02.h>
#elif IS_ENABLED(CONFIG_LEDS_KTD2692)
#include <linux/leds-ktd2692.h>
#elif IS_ENABLED(CONFIG_LEDS_AW36518_FLASH)
#include <linux/leds-aw36518.h>
#elif IS_ENABLED(CONFIG_LEDS_QTI_FLASH) && (IS_ENABLED(CONFIG_SENSORS_STK6D2X) || IS_ENABLED(CONFIG_SENSORS_STK6DAX) || IS_ENABLED(CONFIG_SENSORS_TSL2511))
#include <linux/leds.h>
#include <linux/leds-qti-flash.h>
DEFINE_LED_TRIGGER(torch2_trigger);
DEFINE_LED_TRIGGER(torch3_trigger);
DEFINE_LED_TRIGGER(switch3_trigger);
#endif

#define LED_DT_NODE_NAME "flicker_test"

#define DEFAULT_DUTY_50HZ		5000
#define DEFAULT_DUTY_60HZ		4166

#define MAX_TEST_RESULT			256
#if IS_ENABLED(CONFIG_SENSORS_STK6D2X) || IS_ENABLED(CONFIG_SENSORS_STK6DAX)
#define EOL_COUNT				4
#define EOL_SKIP_COUNT			4
#else
#define EOL_COUNT				5
#define EOL_SKIP_COUNT			5
#endif
#define EOL_FLICKER_SKIP_COUNT	2

static int gpio_torch;
static int led_curr;
static int led_mode;

static char result_str[MAX_TEST_RESULT];

enum TEST_STATE {
	EOL_STATE_INIT = -1,
	EOL_STATE_100,
	EOL_STATE_120,
	EOL_STATE_DONE,
};

struct test_data {
	u8 eol_enable;
	s16 eol_state;
	u32 eol_count;
	u32 eol_sum_count;
	u32 eol_awb;
	u32 eol_clear;
	u32 eol_wideband;
	u32 eol_flicker;
	u32 eol_flicker_sum;
	u32 eol_flicker_sum_count;
	u32 eol_flicker_count;
	u32 eol_flicker_skip_count;
	u32 eol_pulse_count;
	u32 eol_uv;
};

struct result_data {
	int result;
	u32 flicker[EOL_STATE_DONE];
	u32 awb[EOL_STATE_DONE];
	u32 clear[EOL_STATE_DONE];
	u32 wideband[EOL_STATE_DONE];
	u32 ratio[EOL_STATE_DONE];
	u32 uv[EOL_STATE_DONE];
};

enum GPIO_TYPE {
	EOL_FLASH,
	EOL_TORCH,
};

void als_eol_set_env(bool torch, int intensity);
struct result_data* als_eol_mode(void);

void als_eol_update_als(int awb, int clear, int wideband, int uv);
void als_eol_update_flicker(int Hz);
void als_eol_set_err_handler(void (*handler)(void));
#endif /* EOL_TEST_H */
