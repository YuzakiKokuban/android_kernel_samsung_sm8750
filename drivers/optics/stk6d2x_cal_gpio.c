/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd. All rights reserved.
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

#include <stk6d2x.h>
#include <linux/pwm.h>

#define OSC_CAL_FREQ (40)

static u16 cal_period = 0xffff;

u16 ams_pwm_gpio_period(struct stk6d2x_data *alps_data);

int stk6d2x_setup_pwm(struct stk6d2x_data *alps_data, long pwm_freq)
{
	int ret = 0;
	stk6d2x_wrapper *stk_wrapper = container_of(alps_data, stk6d2x_wrapper, alps_data);
	struct device *dev = stk_wrapper->dev;
	struct clk *pclk = NULL;

	ALS_dbg("setup gpio pwm %d Hz\n", pwm_freq);

	pclk = devm_clk_get(dev, "gpio_pwm_default-clk");
	if (NULL == pclk) {
		ALS_err("get pclk (gpio_pwm_default-clk) error. \n");
		return -ENODEV;
	}

	if (pwm_freq > 0) {
#if 0
		if (__clk_is_enabled(pclk)) {
			ALS_dbg("clk_disable_unprepare (pclk) start! (clk_is_enabled: %d, clk_get_rate: %lld)", __clk_is_enabled(pclk), clk_get_rate(pclk));
			clk_disable_unprepare(pclk);
		}

		msleep_interruptible(10);
#endif
		ret = clk_set_rate(pclk, pwm_freq);
		if (ret) {
			ALS_err("clk_set_rate (pclk) fail! (clk_is_enabled: %d, clk_get_rate: %lld)", __clk_is_enabled(pclk), clk_get_rate(pclk));
			return ret;
		} else {
			ALS_dbg("clk_set_rate (pclk) success! (clk_is_enabled: %d, clk_get_rate: %lld)", __clk_is_enabled(pclk), clk_get_rate(pclk));
		}

		if (!__clk_is_enabled(pclk)) {
			ret = clk_prepare_enable(pclk);
			if (ret) {
				ALS_err("clk_prepare_enable (pclk) fail! (clk_is_enabled: %d, clk_get_rate: %lld)", __clk_is_enabled(pclk), clk_get_rate(pclk));
				return ret;
			} else {
				ALS_dbg("clk_prepare_enable (pclk) success! (clk_is_enabled: %d, clk_get_rate: %lld)", __clk_is_enabled(pclk), clk_get_rate(pclk));
			}
		}

	} else {
		if (__clk_is_enabled(pclk)) {
			clk_disable_unprepare(pclk);
			ALS_dbg("clk_disable_unprepare (pclk) start! (clk_is_enabled: %d, clk_get_rate: %lld)", __clk_is_enabled(pclk), clk_get_rate(pclk));
		}
	}

	return ret;
}

ssize_t stk6d2x_osc_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	int pwm_Hz = 0;
	int err = 0;

	if (!data->osc_cal) {
		ALS_err("no osc calibration mode\n");
		return size;
	}

	err = kstrtoint(buf, 10, &pwm_Hz);

	if (err < 0) {
		ALS_err("kstrtoint failed.(%d)\n", err);
		return size;
	}

	if (pwm_Hz == 0) {
		ALS_dbg("pwm output stop! (%d Hz)\n", pwm_Hz);
		stk6d2x_setup_pwm(alps_data, 0);
	} else if ((pwm_Hz > 0) && (pwm_Hz <= 120)) {
		stk6d2x_pin_control(alps_data, true);

		if (pwm_Hz == 1) {
			ALS_dbg("pwm output start! (%d Hz)\n", OSC_CAL_FREQ);
			stk6d2x_osc_cal(alps_data);
		} else {
			ALS_dbg("pwm output start! (%d Hz)\n", pwm_Hz);
			stk6d2x_setup_pwm(alps_data, pwm_Hz);
		}
	} else {
		ALS_err("pwm out of range error! (%d Hz)\n", pwm_Hz);
	}

	return size;
}

void stk6d2x_osc_cal(struct stk6d2x_data *alps_data)
{
	u16 ret;

	stk6d2x_setup_pwm(alps_data, OSC_CAL_FREQ);

	ret = ams_pwm_gpio_period(alps_data);
	if (ret != 0) {
		cal_period = ret;
	}
	ALS_dbg("cal_period = %d", ret);

	stk6d2x_setup_pwm(alps_data, 0);
}

u16 stk6d2x_get_pwm_calibration(void)
{
	if (cal_period == 0)
		return 0xffff;
	return cal_period;
}
