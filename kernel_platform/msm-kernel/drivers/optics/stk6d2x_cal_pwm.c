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

#define OSC_CAL_FREQ 40

static u16 cal_period = -1;

u16 ams_pwm_gpio_period(struct stk6d2x_data *alps_data);
static struct pwm_device *pwm = NULL;

int stk6d2x_setup_pwm(struct stk6d2x_data *alps_data, bool onoff)
{
	stk6d2x_wrapper *stk_wrapper = container_of(alps_data, stk6d2x_wrapper, alps_data);
	struct device_node *np = stk_wrapper->dev->of_node;
	long period = 1000000000 / OSC_CAL_FREQ;
	long duty = period >> 1;
	int ret = 0;

	if (onoff) {
		if (pwm == NULL) {
			pwm = devm_of_pwm_get(dev, np, NULL);
			if (IS_ERR(pwm)) {
				ALS_dbg("pwm_get error %d", PTR_ERR(pwm));
				return PTR_ERR(pwm);
			}
		}

		ret = pwm_config(pwm, duty, period);
		pwm_enable(pwm);
	} else {
		pwm_disable(pwm);
		pwm_put(pwm);
		pwm = NULL;
	}

	return ret;
}

ssize_t stk6d2x_osc_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	u16 ret;

	stk6d2x_osc_cal(alps_data);

	return size;
}

void stk6d2x_osc_cal(struct stk6d2x_data *alps_data)
{
	u16 ret;

	stk6d2x_setup_pwm(alps_data, true);

	ret = ams_pwm_gpio_period(alps_data);
	if (ret != 0)
		cal_period = ret;
	ALS_dbg("period : %d", ret);

	stk6d2x_setup_pwm(alps_data, false);
}

u16 tsl2585_get_pwm_calibration(void)
{
	if (cal_period == 0)
		return 0xffff;
	return cal_period;
}
