/******************************************************************************
 *
 *  Copyright 2023 NXP
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include "common_ese.h"
#include "ese_reset.h"

static reset_timer_t sResetTimer;

/*!
 * \ingroup		spi_driver
 * \brief		Function getting invoked after eSE reset guard time
 * \n			expiry
 * \param[in]	struct timer_list *
 */
static void gpio_reset_guard_timer_callback(struct timer_list *t)
{
	struct reset_timer *sResetTimer = from_timer(sResetTimer, t, timer);

	NFC_LOG_INFO("%s: entry\n", __func__);
	sResetTimer->in_progress = false;
	NFC_LOG_INFO("%s: exit with in_progress set to false\n", __func__);
}

/*!
 * \ingroup		spi_driver
 * \brief		Initialising mutex
 */
void ese_reset_init(void)
{
	mutex_init(&sResetTimer.reset_mutex);
	timer_setup(&sResetTimer.timer, gpio_reset_guard_timer_callback, 0);
}

/*!
 * \ingroup		spi_driver
 * \brief		Deinitialising mutex
 */
void ese_reset_deinit(void)
{
	mutex_destroy(&sResetTimer.reset_mutex);
	del_timer(&sResetTimer.timer);
}

/*!
 * \ingroup		spi_driver
 * \brief		Setting the timer on
 * \return		long 0 for inactive timer 1 for active timer
 */
static long start_gpio_reset_guard_timer(void)
{
	long ret;

	NFC_LOG_INFO("%s: entry\n", __func__);
	ret = mod_timer(&sResetTimer.timer,
			jiffies + msecs_to_jiffies(ESE_GPIO_RST_GUARD_TIME_MS));
	if (!ret)
		sResetTimer.in_progress = true;
	else
		NFC_LOG_ERR("%s: Error in mod_timer, returned:'%ld'\n", __func__, ret);
	NFC_LOG_INFO("%s: exit\n", __func__);
	return ret;
}

/*!
 * \ingroup	spi_driver
 * \brief	Reset the gpio by toggling low and high state
 * \param	struct p61_dev
 */
int perform_ese_gpio_reset(int rst_gpio)
{
	int ret = 0;

	if (gpio_is_valid(rst_gpio)) {
		NFC_LOG_INFO("%s: entry\n", __func__);
		mutex_lock(&sResetTimer.reset_mutex);
		if (sResetTimer.in_progress) {
			NFC_LOG_ERR("%s: gpio reset already in progress\n", __func__);
			ret = -EBUSY;
			mutex_unlock(&sResetTimer.reset_mutex);
			return ret;
		}
		NFC_LOG_INFO("%s: entering gpio ese reset case\n", __func__);
		ret = start_gpio_reset_guard_timer();
		if (ret) {
			mutex_unlock(&sResetTimer.reset_mutex);
			NFC_LOG_ERR("%s: error in mod_timer\n", __func__);
			ret = -EINVAL;
			return ret;
		}
		NFC_LOG_INFO(" eSE Domain Reset");

		gpio_set_value(rst_gpio, 0);
		usleep_range(ESE_GPIO_RESET_WAIT_TIME_USEC,
					 ESE_GPIO_RESET_WAIT_TIME_USEC + 100);
		gpio_set_value(rst_gpio, 1);
		NFC_LOG_INFO("%s: exit\n", __func__);
		mutex_unlock(&sResetTimer.reset_mutex);
	} else {
		NFC_LOG_ERR("%s not entering GPIO Reset, gpio value invalid : %x\n",
					__func__, rst_gpio);
		return -EPERM;
	}
	return ret;
}

/*!
 * \ingroup		spi_driver
 * \brief		setup ese reset gpio pin as an output pin
 * \param		struct p61_spi_platform_data *
 * \return		Return 0 in case of success, else an error code
 */
int ese_reset_gpio_setup(struct p61_spi_platform_data *platform_data)
{
	int ret = -1;

	if (gpio_is_valid(platform_data->rst_gpio)) {
		ret = gpio_request(platform_data->rst_gpio, "p61 reset");
		if (ret < 0) {
			NFC_LOG_ERR("reset gpio 0x%x request failed\n", platform_data->rst_gpio);
			return ret;
		}

		/* reset gpio is set to default high */
		ret = gpio_direction_output(platform_data->rst_gpio, 1);
		if (ret < 0) {
			NFC_LOG_ERR("Failed to set the direction of reset gpio=0x%x\n",
						platform_data->rst_gpio);
			goto fail_gpio;
		}
		NFC_LOG_INFO("Exit : %s\n", __func__);
	} else {
		NFC_LOG_INFO("%s, gpio value invalid : %x\n", __func__,
					platform_data->rst_gpio);
	}
	return ret;
fail_gpio:
	if (gpio_is_valid(platform_data->rst_gpio))
		gpio_free(platform_data->rst_gpio);

	return ret;
}
