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

#include "flicker_test.h"

static struct test_data *data = NULL;
static struct result_data *test_result = NULL;

static void (*err_handler)(void);


void als_eol_set_err_handler(void (*handler)(void)){
	err_handler = handler;
}
EXPORT_SYMBOL_GPL(als_eol_set_err_handler);

/**
 * als_eol_update_als - Record every test value
 *
 * @awb      : current 'awb' value
 *            (You can use any value as you need. Maybe Infrared value will be used in most cases)
 * @clear    : current clear value detected by sensor. It means visible light in light spectrum
 * @wideband : current Wideband value detected by sensor.
 *             wideband light include visible light and infrared.
 *
 */
void als_eol_update_als(int awb, int clear, int wideband, int uv)
{
	if (data->eol_enable && data->eol_count >= EOL_SKIP_COUNT) {
		data->eol_awb += awb;
		data->eol_clear += clear;
		data->eol_wideband += wideband;
		data->eol_uv += uv;
		data->eol_sum_count++;
	}

	if (data->eol_enable && data->eol_state < EOL_STATE_DONE) {
		switch (data->eol_state) {
			case EOL_STATE_INIT:
				memset(test_result, 0, sizeof(struct result_data));

				data->eol_count = 0;
				data->eol_sum_count = 0;
				data->eol_awb = 0;
				data->eol_clear = 0;
				data->eol_wideband = 0;
				data->eol_flicker = 0;
				data->eol_flicker_sum = 0;
				data->eol_flicker_sum_count = 0;
				data->eol_flicker_count = 0;
				data->eol_flicker_skip_count = EOL_FLICKER_SKIP_COUNT;
				data->eol_state = EOL_STATE_100;
				data->eol_pulse_count = 0;
				data->eol_uv = 0;
				break;
			default:
				data->eol_count++;
				printk(KERN_INFO"%s - eol_state:%d, eol_cnt:%d (sum_cnt:%d), flk:%d (flk_cnt:%d, sum_cnt:%d), ir:%d, clear:%d, wide:%d, uv:%d\n", __func__,
						data->eol_state, data->eol_count, data->eol_sum_count, data->eol_flicker, data->eol_flicker_count, data->eol_flicker_sum_count, awb, clear, wideband, uv);

				if ((data->eol_count >= (EOL_COUNT + EOL_SKIP_COUNT)) && (data->eol_flicker_count >= (EOL_COUNT + data->eol_flicker_skip_count))) {
					if (data->eol_flicker_sum_count) {
						test_result->flicker[data->eol_state] = data->eol_flicker_sum / data->eol_flicker_sum_count;
					} else {
						test_result->flicker[data->eol_state] = data->eol_flicker;
					}

					if (data->eol_sum_count) {
						test_result->awb[data->eol_state] = data->eol_awb / data->eol_sum_count;
						test_result->clear[data->eol_state] = data->eol_clear / data->eol_sum_count;
						test_result->wideband[data->eol_state] = data->eol_wideband / data->eol_sum_count;
						test_result->uv[data->eol_state] = data->eol_uv / data->eol_sum_count;
					} else {
						test_result->awb[data->eol_state] = awb;
						test_result->clear[data->eol_state] = clear;
						test_result->wideband[data->eol_state] = wideband;
						test_result->uv[data->eol_state] = uv;
					}

					printk(KERN_INFO"%s - eol_state = %d, pulse_count = %d, flicker_result = %d Hz (%d/%d)\n",
							__func__, data->eol_state, data->eol_pulse_count, test_result->flicker[data->eol_state], data->eol_flicker_sum, data->eol_flicker_sum_count);

					data->eol_count = 0;
					data->eol_sum_count = 0;
					data->eol_awb = 0;
					data->eol_clear = 0;
					data->eol_wideband = 0;
					data->eol_flicker = 0;
					data->eol_flicker_sum = 0;
					data->eol_flicker_sum_count = 0;
					data->eol_flicker_count = 0;
					data->eol_pulse_count = 0;
					data->eol_uv = 0;
					data->eol_state++;
				}
				break;
		}
	}
}
EXPORT_SYMBOL_GPL(als_eol_update_als);

/**
 * als_eol_update_flicker - Record every test value
 *
 * @flicker: current Flicker value detected by sensor.
 */
void als_eol_update_flicker(int Hz)
{
	data->eol_flicker_count++;
	data->eol_flicker = Hz;

	if ((data->eol_flicker_skip_count < EOL_SKIP_COUNT) && (data->eol_flicker_count >= data->eol_count)) {
		data->eol_flicker_skip_count = EOL_SKIP_COUNT;
	}

	if ((data->eol_enable && Hz != 0) && (data->eol_flicker_count > data->eol_flicker_skip_count)) {
		data->eol_flicker_sum += Hz;
		data->eol_flicker_sum_count++;
	}
}
EXPORT_SYMBOL_GPL(als_eol_update_flicker);

void set_led_mode(int led_curr)
{
#if IS_ENABLED (CONFIG_LEDS_S2MPB02)
	s2mpb02_led_en(led_mode, led_curr, S2MPB02_LED_TURN_WAY_GPIO);
#elif IS_ENABLED(CONFIG_LEDS_KTD2692)
	ktd2692_led_mode_ctrl(led_mode, led_curr);
#elif IS_ENABLED(CONFIG_LEDS_AW36518_FLASH)
	if (led_curr)
		aw36518_enable_flicker(led_curr, true);
	else
		aw36518_enable_flicker(0, false);
#elif IS_ENABLED(CONFIG_LEDS_QTI_FLASH) && (IS_ENABLED(CONFIG_SENSORS_STK6D2X) || IS_ENABLED(CONFIG_SENSORS_STK6DAX) || IS_ENABLED(CONFIG_SENSORS_TSL2511))
	if(led_curr) {
		qti_flash_led_set_strobe_sel(switch3_trigger, 1);
		led_trigger_event(torch2_trigger, led_curr/2);
		led_trigger_event(torch3_trigger, led_curr/2);
		led_trigger_event(switch3_trigger, 1);
	} else {
		qti_flash_led_set_strobe_sel(switch3_trigger, 0);
		led_trigger_event(switch3_trigger, 0);
	}
#endif
}

void als_eol_set_env(bool torch, int intensity)
{
#if IS_ENABLED(CONFIG_LEDS_S2MPB02)
	led_curr = (intensity/20);
	led_mode = S2MPB02_TORCH_LED_1;
#elif IS_ENABLED(CONFIG_LEDS_KTD2692)
	led_curr = 1400;
	led_mode = KTD2692_FLICKER_FLASH_MODE;
#elif IS_ENABLED(CONFIG_LEDS_AW36518_FLASH)
	led_curr = intensity;
#elif IS_ENABLED(CONFIG_LEDS_QTI_FLASH)
	led_curr = intensity;
#endif
	printk(KERN_INFO "%s - gpio:%d intensity:%d(%d) led_mode:%d",
		__func__, gpio_torch, intensity, led_curr, led_mode);
}
EXPORT_SYMBOL_GPL(als_eol_set_env);

/**
 * als_eol_mode - start LED flicker loop
 *
 * Return result_data
 * MUST call als_eol_update* functions to notify the sensor output!!
 */
struct result_data* als_eol_mode(void)
{
	int pulse_duty = 0;
	int curr_state = EOL_STATE_INIT;
	int ret = 0;
	u32 prev_eol_count = 0, loop_count = 0;

	set_led_mode(0);

	ret = gpio_request(gpio_torch, NULL);
	if (ret < 0)
		return NULL;

	data->eol_state = EOL_STATE_INIT;
	data->eol_enable = 1;

	printk(KERN_INFO"%s - eol_loop start", __func__);
	while (data->eol_state < EOL_STATE_DONE) {
		if (prev_eol_count == data->eol_count)
			loop_count++;
		else
			loop_count = 0;

		prev_eol_count = data->eol_count;

		switch (data->eol_state) {
			case EOL_STATE_INIT:
				pulse_duty = 1000;
				break;
			case EOL_STATE_100:
				pulse_duty = DEFAULT_DUTY_50HZ;
				break;
			case EOL_STATE_120:
				pulse_duty = DEFAULT_DUTY_60HZ;
				break;
			default:
				break;
		}

		if (data->eol_state >= EOL_STATE_100) {
			if (curr_state != data->eol_state) {
#if IS_ENABLED(CONFIG_LEDS_KTD2692) || IS_ENABLED(CONFIG_LEDS_AW36518_FLASH)
				if(ret >= 0) {
					gpio_free(gpio_torch);
				}
#endif
				set_led_mode(led_curr);
				curr_state = data->eol_state;

#if IS_ENABLED(CONFIG_LEDS_KTD2692) || IS_ENABLED(CONFIG_LEDS_AW36518_FLASH)
				ret = gpio_request(gpio_torch, NULL);
				if (ret < 0)
					break;
#endif
			} else {
				gpio_direction_output(gpio_torch, 1);
				udelay(pulse_duty);
				gpio_direction_output(gpio_torch, 0);
				data->eol_pulse_count++;
			}
		}

		if (loop_count > 1000) {
			printk(KERN_ERR "%s - ERR NO Interrupt", __func__);
			// Add Debug Code
			if (err_handler)
				err_handler();
			break;
		}

		udelay(pulse_duty);
	}
	printk(KERN_INFO"%s - eol_loop end",__func__);
	if(ret >= 0) {
		gpio_free(gpio_torch);
	}
	set_led_mode(0);

	if (data->eol_state >= EOL_STATE_DONE) {
		if(test_result->clear[EOL_STATE_100] != 0) {
			test_result->ratio[EOL_STATE_100] = test_result->awb[EOL_STATE_100] * 100 / test_result->clear[EOL_STATE_100];
		}
		if(test_result->clear[EOL_STATE_120] != 0) {
			test_result->ratio[EOL_STATE_120] = test_result->awb[EOL_STATE_120] * 100 / test_result->clear[EOL_STATE_120];
		}
	} else {
		printk(KERN_ERR "%s - abnormal termination\n", __func__);
	}
	printk(KERN_INFO "%s - RESULT: flicker:%d|%d awb:%d|%d clear:%d|%d wide:%d|%d ratio:%d|%d uv:%d|%d", __func__,
			test_result->flicker[EOL_STATE_100], test_result->flicker[EOL_STATE_120],
			test_result->awb[EOL_STATE_100], test_result->awb[EOL_STATE_120],
			test_result->clear[EOL_STATE_100], test_result->clear[EOL_STATE_120],
			test_result->wideband[EOL_STATE_100], test_result->wideband[EOL_STATE_120],
			test_result->ratio[EOL_STATE_100], test_result->ratio[EOL_STATE_120],
			test_result->uv[EOL_STATE_100], test_result->uv[EOL_STATE_120]);

	data->eol_enable = 0;

	return test_result;
}
EXPORT_SYMBOL_GPL(als_eol_mode);

int als_eol_parse_dt(void)
{
	struct device_node *np;
#if KERNEL_VERSION(6, 2, 0) > LINUX_VERSION_CODE
	enum of_gpio_flags flags;
#endif

	np = of_find_node_by_name(NULL, LED_DT_NODE_NAME);
	if (np == NULL) {
		printk(KERN_ERR "%s - Can't find node", __func__);
		return -1;
	}

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
	gpio_torch = of_get_named_gpio(np, "flicker_test,torch-gpio", 0);
#else
	gpio_torch = of_get_named_gpio_flags(np, "flicker_test,torch-gpio", 0, &flags);
#endif

	printk(KERN_INFO "%s - torch : %d", __func__, gpio_torch);

	return 0;
}

static int __init als_eol_init(void)
{
	int ret = 0;
	printk(KERN_INFO "%s - EOL_TEST Module init", __func__);

	data = (struct test_data*)kzalloc(sizeof(struct test_data), GFP_KERNEL);
	if (data == NULL) {
		printk(KERN_INFO "%s - data alloc err", __func__);
		return -1;
	}

	test_result = (struct result_data*)kzalloc(sizeof(struct result_data), GFP_KERNEL);
	if (test_result == NULL) {
		printk(KERN_INFO "%s - test_result alloc err", __func__);
		return -1;
	}

	ret = als_eol_parse_dt();
	if (ret < 0) {
		printk(KERN_ERR "%s - dt parse fail!", __func__);
		return ret;
	}

#if !IS_ENABLED(CONFIG_LEDS_S2MPB02) && !IS_ENABLED(CONFIG_LEDS_KTD2692) && IS_ENABLED(CONFIG_LEDS_QTI_FLASH) \
	&& (IS_ENABLED(CONFIG_SENSORS_STK6D2X) || IS_ENABLED(CONFIG_SENSORS_STK6DAX) || IS_ENABLED(CONFIG_SENSORS_TSL2511)) && !IS_ENABLED(CONFIG_LEDS_AW36518_FLASH)
	led_trigger_register_simple("torch2_trigger", &torch2_trigger);
	led_trigger_register_simple("torch3_trigger", &torch3_trigger);
	led_trigger_register_simple("switch3_trigger", &switch3_trigger);
#endif

	return ret;
}

static void __exit als_eol_exit(void)
{
	printk(KERN_INFO "%s - EOL_TEST Module exit\n", __func__);

#if !IS_ENABLED(CONFIG_LEDS_S2MPB02) && !IS_ENABLED(CONFIG_LEDS_KTD2692) && IS_ENABLED(CONFIG_LEDS_QTI_FLASH) \
	&& (IS_ENABLED(CONFIG_SENSORS_STK6D2X) || IS_ENABLED(CONFIG_SENSORS_STK6DAX) || IS_ENABLED(CONFIG_SENSORS_TSL2511)) && !IS_ENABLED(CONFIG_LEDS_AW36518_FLASH)
	led_trigger_unregister_simple(torch2_trigger);
	led_trigger_unregister_simple(torch3_trigger);
	led_trigger_unregister_simple(switch3_trigger);
#endif

	if(data) {
		kfree(data);
	}

	if(test_result) {
		kfree(test_result);
	}
}

module_init(als_eol_init);
module_exit(als_eol_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Flicker Sensor Test Driver");
MODULE_LICENSE("GPL");
