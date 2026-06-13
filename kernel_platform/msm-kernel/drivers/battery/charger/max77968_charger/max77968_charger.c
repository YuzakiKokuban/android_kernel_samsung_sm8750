/*
 * Driver for the MAXIM MAX77968 battery charger.
 *
 * Copyright (C) 2023-2024 Analog Devices.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/rtc.h>
#include <linux/debugfs.h>
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#include <linux/of_gpio.h>
#include "max77968_charger.h"
#if defined(CONFIG_ADIENV)
#include <../drivers/battery/common/sec_charging_common.h>
#include <../drivers/battery/common/sec_direct_charger.h>
#else
#include "../../common/sec_charging_common.h"
#include "../../common/sec_direct_charger.h"
#endif
#include <linux/battery/sec_pd.h>
#else
#include "max77968_charger.h"
#endif
#include <linux/completion.h>
#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

#include <linux/pm_wakeup.h>

#define I2C_RETRY_CNT	3

#if defined(ISSUE_WORKAROUND)
#define I2C_ADDR_OTP			(0x60 >> 1)
#define OTP_REG_START			(0x10)
#define OTP_REG_END				(0x4F)
#define OTP_REG_NUM				(OTP_REG_END-OTP_REG_START+1)
#define OTP3_BYTE_9				0x49
#define P2_OTP_BSTUV_SCC		BIT(5)
#define P2_OTP_Vgs3Clmp			BIT(3)
#define I2C_ADDR_TEST			(0xA0 >> 1)
#define TEST_REG_END			(0x80)
#endif

enum {
	CHG_DATA,
	DC_ERR_CAUSE,
	PLATFORM_DATA,
	REV_MODE,
};

#if !IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#define is_3_1_wc_status(wc_stat) (wc_stat == SEC_BATTERY_CABLE_HV_WIRELESS_DC)
#endif

ssize_t max77968_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t max77968_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);


#define MAX77968_CHG_ATTR(_name)	\
{	\
	.attr = {.name = #_name, .mode = 0660},	\
	.show = max77968_chg_show_attrs,	\
	.store = max77968_chg_store_attrs,	\
}


static struct device_attribute max77968_chg_attrs[] = {
	MAX77968_CHG_ATTR(data),
	MAX77968_CHG_ATTR(dc_err_cause),
	MAX77968_CHG_ATTR(platform_data),
	MAX77968_CHG_ATTR(rev_mode),
};

static int max77968_read_adc(struct max77968_charger *max77968, u8 adc_ch);
static int max77968_set_new_iin(struct max77968_charger *max77968);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
static int max77968_send_pd_message(struct max77968_charger *max77968, unsigned int msg_type);
static int get_const_charge_voltage(struct max77968_charger *max77968);

/*******************************/
/* Switching charger control function */
/*******************************/
char *charging_state_str[] = {
	"NO_CHARGING", "CHECK_VBAT", "PRESET_DC", "CHECK_ACTIVE", "ADJUST_CC",
	"START_CC", "CC_MODE", "START_CV", "CV_MODE", "CHARGING_DONE",
	"ADJUST_TAVOL", "ADJUST_TACUR", "BYPASS_MODE", "DCMODE_CHANGE",
	"REVERSE_MODE", "FPDO_CV_MODE",
};

static int max77968_read_reg(struct max77968_charger *max77968, int reg, void *val)
{
	int i, ret = 0;

	mutex_lock(&max77968->i2c_lock);
	ret = regmap_read(max77968->regmap, reg, val);
	mutex_unlock(&max77968->i2c_lock);
	if (ret >= 0)
		return ret;

	mdelay(15);

	mutex_lock(&max77968->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = regmap_read(max77968->regmap, reg, val);
		if (ret >= 0)
			break;
	}
	mutex_unlock(&max77968->i2c_lock);
	if (ret < 0) {
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (max77968->valid_ic)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	}
	return ret;
}

static int max77968_bulk_read_reg(struct max77968_charger *max77968, int reg, void *val, int count)
{
	int i, ret = 0;

	mutex_lock(&max77968->i2c_lock);
	ret = regmap_bulk_read(max77968->regmap, reg, val, count);
	mutex_unlock(&max77968->i2c_lock);
	if (ret >= 0)
		return ret;

	mdelay(15);

	mutex_lock(&max77968->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = regmap_bulk_read(max77968->regmap, reg, val, count);
		if (ret >= 0)
			break;
	}
	mutex_unlock(&max77968->i2c_lock);
	if (ret < 0) {
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (max77968->valid_ic)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	}
	return ret;
}

static int max77968_write_reg(struct max77968_charger *max77968, int reg, u8 val)
{
	int i, ret = 0;

	mutex_lock(&max77968->i2c_lock);
	ret = regmap_write(max77968->regmap, reg, val);
	mutex_unlock(&max77968->i2c_lock);
	if (ret >= 0)
		return ret;

	mdelay(15);

	mutex_lock(&max77968->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = regmap_write(max77968->regmap, reg, val);
		if (ret >= 0)
			break;
	}
	mutex_unlock(&max77968->i2c_lock);
	if (ret < 0) {
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (max77968->valid_ic)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	}
	return ret;
}

static int max77968_update_reg(struct max77968_charger *max77968, int reg, u8 mask, u8 val)
{
	int i, ret = 0;

	mutex_lock(&max77968->i2c_lock);
	ret = regmap_update_bits(max77968->regmap, reg, mask, val);
	mutex_unlock(&max77968->i2c_lock);
	if (ret >= 0)
		return ret;

	mdelay(15);

	mutex_lock(&max77968->i2c_lock);
	ret = regmap_update_bits(max77968->regmap, reg, mask, val);
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = regmap_update_bits(max77968->regmap, reg, mask, val);
		if (ret >= 0)
			break;
	}
	mutex_unlock(&max77968->i2c_lock);
	if (ret < 0) {
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (max77968->valid_ic)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	}
	return ret;
}

static int max77968_set_iin_delay_timer(struct max77968_charger *max77968, int time)
{
	int ret = 0, val = 0;

	pr_info("%s: IIN_DELAY_TIMER (%d)\n", __func__, time);

	// must be set to OFF first and then set to the new value
	ret = max77968_update_reg(max77968, RT_CFG_REG, RT_CFG_IIN_DELAY_TIMER, IIN_DELAY_TIMER_OFF);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return -1;
	}

	if (time == IIN_DELAY_TIMER_OFF) {
		pr_info("%s: IIN_DELAY_TIMER_OFF\n", __func__);
		return 0;
	}

	val = time << MASK2SHIFT(RT_CFG_IIN_DELAY_TIMER);
	ret = max77968_update_reg(max77968, RT_CFG_REG, RT_CFG_IIN_DELAY_TIMER, val);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return -1;
	}

	return 0;
}

static int max77968_set_regulation_timer(struct max77968_charger *max77968, int time)
{
	int ret = 0, val = 0;

	pr_info("%s: REG_TIMER (%d)\n", __func__, time);

	// must be set to OFF first and then set to the new value
	ret = max77968_update_reg(max77968, RT_CFG_REG, RT_CFG_REG_TIMER, REG_TIMER_OFF);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return -1;
	}

	if (time == REG_TIMER_OFF) {
		pr_info("%s: REG_TIMER_OFF\n", __func__);
		return 0;
	}

	val = time << MASK2SHIFT(RT_CFG_REG_TIMER);
	ret = max77968_update_reg(max77968, RT_CFG_REG, RT_CFG_REG_TIMER, val);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return -1;
	}

	return 0;
}

#if defined(ISSUE_WORKAROUND)
static int max77968_write_test_reg(struct max77968_charger *max77968, int reg, u8 val)
{
	int i, ret = 0;

	mutex_lock(&max77968->i2c_lock);
	ret = regmap_write(max77968->tregmap, reg, val);
	mutex_unlock(&max77968->i2c_lock);
	if (ret >= 0)
		return ret;

	mdelay(15);

	mutex_lock(&max77968->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = regmap_write(max77968->tregmap, reg, val);
		if (ret >= 0)
			break;
	}
	mutex_unlock(&max77968->i2c_lock);
	if (ret < 0) {
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (max77968->valid_ic)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	}
	return ret;
}

static int max77968_read_test_reg(struct max77968_charger *max77968, int reg, void *val)
{
	int i, ret = 0;

	mutex_lock(&max77968->i2c_lock);
	ret = regmap_read(max77968->tregmap, reg, val);
	mutex_unlock(&max77968->i2c_lock);
	if (ret >= 0)
		return ret;

	mdelay(15);

	mutex_lock(&max77968->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = regmap_read(max77968->tregmap, reg, val);
		if (ret >= 0)
			break;
	}
	mutex_unlock(&max77968->i2c_lock);
	if (ret < 0) {
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (max77968->valid_ic)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	}
	return ret;
}

static int max77968_test_regmap_unlock(struct max77968_charger *max77968)
{
	int ret;

	mutex_lock(&max77968->tregmap_lock);
	ret = max77968_write_reg(max77968, 0x7F, 0xC5);
	if (ret < 0) {
		pr_info("%s: TEST MAP UNLOCK FAILED, ret(%d)\n", __func__, ret);
		mutex_unlock(&max77968->tregmap_lock);
		return ret;
	}

	pr_info("%s: TEST MAP UNLOCKED\n", __func__);
	return 0;
}

static int max77968_test_regmap_lock(struct max77968_charger *max77968)
{
	int ret;

	ret = max77968_write_reg(max77968, 0x7F, 0x00);
	if (ret < 0)
		pr_info("%s: TEST MAP LOCK FAILED, ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST MAP LOCKED\n", __func__);

	mutex_unlock(&max77968->tregmap_lock);
	return ret;
}

static void max77968_avdd_ldo_workaround(struct max77968_charger *max77968)
{
	int ret, val;

	pr_info("%s: =====START=====\n", __func__);

	// unlock test SID
	ret = max77968_test_regmap_unlock(max77968);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return;
	}

	// Force to use VOUT as AVDD LDO power source
	ret = max77968_write_test_reg(max77968, 0x0C, 0x40);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_read_test_reg(max77968, 0x0C, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x0C): 0x%x\n", __func__, val);

	// lock test SID
	ret = max77968_test_regmap_lock(max77968);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
}

static void max77968_1to1_cp_workaround_enable(struct max77968_charger *max77968)
{
	int ret, val;

	pr_info("%s: =====START=====\n", __func__);

	// unlock test SID
	ret = max77968_test_regmap_unlock(max77968);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return;
	}

	// Force QPCLK ON
	ret = max77968_write_test_reg(max77968, 0x35, 0x04);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_read_test_reg(max77968, 0x35, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x35): 0x%x\n", __func__, val);

	// Enable test mode of the charge pump
	ret = max77968_write_test_reg(max77968, 0x07, 0x03);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_read_test_reg(max77968, 0x07, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x07): 0x%x\n", __func__, val);

	// lock test SID
	ret = max77968_test_regmap_lock(max77968);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
}

static void max77968_1to1_cp_workaround_disable(struct max77968_charger *max77968)
{
	int ret, val;

	pr_info("%s: =====START=====\n", __func__);

	// unlock test SID
	ret = max77968_test_regmap_unlock(max77968);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return;
	}

	// Disable test mode of the charge pump
	ret = max77968_write_test_reg(max77968, 0x07, 0x00);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_read_test_reg(max77968, 0x07, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x07): 0x%x\n", __func__, val);

	// Disable QPCLK FORCE ON
	ret = max77968_write_test_reg(max77968, 0x35, 0x00);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_read_test_reg(max77968, 0x35, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x35): 0x%x\n", __func__, val);

	// lock test SID
	ret = max77968_test_regmap_lock(max77968);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
}

static void max77968_enable_S0A_force_on(struct max77968_charger *max77968)
{
	int ret, val;

	pr_info("%s: =====START=====\n", __func__);

	// unlock test SID
	ret = max77968_test_regmap_unlock(max77968);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return;
	}

	// Write TEST registers to enable S0A force on
	ret = max77968_write_test_reg(max77968, 0x2D, 0x04);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_write_test_reg(max77968, 0x33, 0x02);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_write_test_reg(max77968, 0x34, 0xFD);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_write_test_reg(max77968, 0x2D, 0x0C);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	// Read TEST registers for confirmation
	ret = max77968_read_test_reg(max77968, 0x2D, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x2D): 0x%x\n", __func__, val);

	ret = max77968_read_test_reg(max77968, 0x33, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x33): 0x%x\n", __func__, val);

	ret = max77968_read_test_reg(max77968, 0x34, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x34): 0x%x\n", __func__, val);

	// lock test SID
	ret = max77968_test_regmap_lock(max77968);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
}

static void max77968_disable_S0A_force_on(struct max77968_charger *max77968)
{
	int ret, val;

	pr_info("%s: =====START=====\n", __func__);

	// unlock test SID
	ret = max77968_test_regmap_unlock(max77968);
	if (ret < 0) {
		pr_info("%s: ret(%d)\n", __func__, ret);
		return;
	}

	// Write TEST registers to Disable S0A forced on
	ret = max77968_write_test_reg(max77968, 0x2D, 0x04);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_write_test_reg(max77968, 0x33, 0x00);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	ret = max77968_write_test_reg(max77968, 0x34, 0xFF);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	// Read TEST registers for confirmation
	ret = max77968_read_test_reg(max77968, 0x2D, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x2D): 0x%x\n", __func__, val);

	ret = max77968_read_test_reg(max77968, 0x33, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x33): 0x%x\n", __func__, val);

	ret = max77968_read_test_reg(max77968, 0x34, &val);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
	else
		pr_info("%s: TEST map, read reg(0x34): 0x%x\n", __func__, val);

	// lock test SID
	ret = max77968_test_regmap_lock(max77968);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);
}

static void max77968_before_scc_enable_workaround(struct max77968_charger *max77968, u8 mode)
{
	int ret, status1;

	pr_info("%s: =====START=====\n", __func__);

	ret = max77968_read_reg(max77968, STATUS1_REG, &status1);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	if (!(status1 & VOUT_UVLO_S))
		max77968_avdd_ldo_workaround(max77968);

	if ((mode == FWD_1TO1) || (mode == RVS_1TO1))
		max77968_1to1_cp_workaround_enable(max77968);
	else
		max77968_1to1_cp_workaround_disable(max77968);

	if (mode == FWD_3TO1)
		max77968_enable_S0A_force_on(max77968);
	else
		max77968_disable_S0A_force_on(max77968);
}

static void max77968_after_scc_enable_workaround(struct max77968_charger *max77968)
{
	max77968_1to1_cp_workaround_disable(max77968);
}

static void max77968_after_scc_disable_workaround(struct max77968_charger *max77968)
{
	msleep(50);
	max77968_disable_S0A_force_on(max77968);
}

static void max77968_init_workaround(struct max77968_charger *max77968)
{
	int ret, status1;

	pr_info("%s: =====START=====\n", __func__);

	ret = max77968_read_reg(max77968, STATUS1_REG, &status1);
	if (ret < 0)
		pr_info("%s: ret(%d)\n", __func__, ret);

	if (!(status1 & VOUT_UVLO_S))
		max77968_avdd_ldo_workaround(max77968);
}

static void max77968_adc_circ_buf_init(struct max77968_charger *max77968)
{
	int i;

	for (i = 0; i < NUM_ADC_BUF; i++) {
		max77968->circ_buf[i].head = 0;
		max77968->circ_buf[i].count = 0;
		mutex_init(&max77968->circ_buf[i].lock);
	}
}

static void max77968_adc_circ_buf_add(struct max77968_charger *max77968, int index, unsigned int data)
{
	if ((index < 0) || (index >= NUM_ADC_BUF)) {
		pr_info("%s: index out of bound, (%d)\n", __func__, index);
		return;
	} else if (max77968 == NULL) {
		pr_info("%s: max77968 ptr NULL\n", __func__);
		return;
	}

	mutex_lock(&max77968->circ_buf[index].lock);
	max77968->circ_buf[index].buf[max77968->circ_buf[index].head] = data;
	max77968->circ_buf[index].head = (max77968->circ_buf[index].head + 1) % ADC_BUF_SIZE;
	if (max77968->circ_buf[index].count < ADC_BUF_SIZE)
		max77968->circ_buf[index].count++;
	mutex_unlock(&max77968->circ_buf[index].lock);
}

static unsigned int max77968_get_adc_buf_median(struct max77968_charger *max77968, int index)
{
	unsigned int sorted_buffer[ADC_BUF_SIZE] = {0};
	unsigned int median, count;
	int i, j;
	unsigned int key;

	if ((index < 0) || (index >= NUM_ADC_BUF)) {
		pr_info("%s: index out of bound, (%d)\n", __func__, index);
		return 0;
	} else if (max77968 == NULL) {
		pr_info("%s: max77968 ptr NULL\n", __func__);
		return 0;
	}

	mutex_lock(&max77968->circ_buf[index].lock);
	for (i = 0; i < ADC_BUF_SIZE; i++)
		sorted_buffer[i] = max77968->circ_buf[index].buf[i];
	count = max77968->circ_buf[index].count;
	mutex_unlock(&max77968->circ_buf[index].lock);

	if (count <= 2 || count > ADC_BUF_SIZE) {
		pr_info("%s: Invalid count: %d\n", __func__, count);
		return 0;
	}

	for (i = 1; i < count; i++) {
		key = sorted_buffer[i];
		j = i - 1;
		while (j >= 0 && sorted_buffer[j] > key) {
			sorted_buffer[j + 1] = sorted_buffer[j];
			j = j - 1;
		}
		sorted_buffer[j + 1] = key;
	}

	if (count % 2 == 0)
		median = (sorted_buffer[count / 2 - 1] + sorted_buffer[count / 2]) / 2;
	else
		median = sorted_buffer[count / 2];

	pr_info("%s: median: %d\n", __func__, median);
	return median;
}

static int max77968_read_adc_fast(struct max77968_charger *max77968, u8 adc_ch)
{
	union power_supply_propval value = {0,};
	u8 reg_addr = 0;
	u8 reg_data[2];
	u16 raw_adc = 0;
	int conv_adc = 0, adc_step = 0;
	int ret;

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		if (adc_ch == ADC_CH_VBATT) {
			ret = psy_do_property(max77968->pdata->fg_name, get, POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
			if (ret < 0) {
				conv_adc = ret;
				return 0;
			}
			conv_adc = value.intval * MAX77968_SEC_DENOM_U_M;
			return conv_adc;
		}
	}

	switch (adc_ch) {
	case ADC_CH_VIN:
		adc_step = VIN_STEP;
		reg_addr = ADC_VIN_READ_REG;
		break;
	case ADC_CH_PMID:
		adc_step = PMID_STEP;
		reg_addr = ADC_PMID_READ_REG;
		break;
	case ADC_CH_VEXT1:
		adc_step = VEXT1_STEP;
		reg_addr = ADC_VEXT1_READ_REG;
		break;
	case ADC_CH_VEXT2:
		adc_step = VEXT2_STEP;
		reg_addr = ADC_VEXT2_READ_REG;
		break;
	case ADC_CH_VOUT:
		adc_step = VOUT_STEP;
		reg_addr = ADC_VOUT_READ_REG;
		break;
	case ADC_CH_VBATT:
		adc_step = VBATT_STEP;
		reg_addr = ADC_VBATT_READ_REG;
		break;
	case ADC_CH_NTC:
		adc_step = NTC_STEP;
		reg_addr = ADC_NTC_READ_REG;
		break;
	case ADC_CH_TDIE:
		adc_step = TDIE_STEP;
		reg_addr = ADC_TDIE_READ_REG;
		break;
	case ADC_CH_IIN:
		adc_step = IIN_STEP;
		reg_addr = ADC_IIN_READ_REG;
		break;
	default:
		conv_adc = 0;
	}

	ret = max77968_bulk_read_reg(max77968, reg_addr, reg_data, 2);
	if (ret < 0)
		return 0;

	raw_adc = ((reg_data[0] << 4) & ADC_VAL_HIGH_MASK) | ((reg_data[1] >> 4) & ADC_VAL_LOW_MASK);

	if (adc_ch == ADC_CH_TDIE) {
		/* Transfer to 16 bits signed value */
		if (raw_adc & 0x800)
			raw_adc |= 0xF000;
		conv_adc = (s16)raw_adc * adc_step / TDIE_DENOM;
		if (conv_adc > TDIE_MAX)
			conv_adc = TDIE_MAX;
	} else
		conv_adc = raw_adc * adc_step;

	return conv_adc;
}

static void max77968_adc_workaround(struct max77968_charger *max77968)
{
	int i, mask, val;
	static const u8 adc_ch[] = { ADC_CH_IIN };

	for (i = 0; i < NUM_ADC_BUF; i++) {
		val = max77968_read_adc_fast(max77968, adc_ch[i]);
		max77968_adc_circ_buf_add(max77968, i, val);
	}

	/* Disable the ADC channels */
	mask = (VIN_READ_EN | PMID_READ_EN | VOUT_READ_EN | TDIE_READ_EN);
	max77968_update_reg(max77968, ADC_CFG1_REG, mask, 0);
	max77968_update_reg(max77968, ADC_CFG2_REG, IIN_READ_EN, 0);

	/* Enable the ADC channels */
	max77968_update_reg(max77968, ADC_CFG1_REG, mask, mask);
	max77968_update_reg(max77968, ADC_CFG2_REG, IIN_READ_EN, IIN_READ_EN);
}

#endif

static int max77968_interrupt_disable(struct max77968_charger *max77968)
{
	int ret;
	int msk[REG_INT_MAX];

	/*
	 * Configure the Mask Register for interrupts: disable all interrupts by default.
	 */
	msk[REG_INT1] = 0xFF;
	msk[REG_INT2] = 0xFF;
	msk[REG_INT3] = 0xFF;
	msk[REG_INT4] = 0xFF;
	msk[REG_INT5] = 0xFF;

	ret = max77968_write_reg(max77968, INT_SRC1_M_REG, 0xFF);
	ret = max77968_write_reg(max77968, INT_SRC2_M_REG, 0xFF);
	ret = max77968_write_reg(max77968, INT_SRC3_M_REG, 0xFF);
	ret = max77968_write_reg(max77968, INT_SRC4_M_REG, 0xFF);
	ret = max77968_write_reg(max77968, INT_SRC5_M_REG, 0xFF);

	//ret = regmap_bulk_write(max77968->regmap, INT_SRC1_M_REG, msk, REG_INT_MAX);
	if (ret < 0)
		pr_info("%s: i2c error, ret=%d\n", __func__, ret);

	return ret;
}

static int max77968_config_init(struct max77968_charger *max77968)
{
	int ret, val;

	pr_info("%s: Write fixed SCC configurations\n", __func__);

#if defined(ISSUE_WORKAROUND)
	max77968_init_workaround(max77968);
#endif

	ret = max77968_update_reg(max77968, FSW_CFG_REG,
			FSW_FREQ, max77968->fsw_cfg);
	if (ret < 0)
		return ret;

	/* Set RVSBST VIN VALID EN bit to 1 */
	ret = max77968_update_reg(max77968, SCC_EN_REG,
					SCC_RVSBST_VIN_VALID_EN, SCC_RVSBST_VIN_VALID_EN);
	if (ret < 0)
		return ret;

	ret = max77968_update_reg(max77968, IIN_REG_TRACK_REG,
					TRACK_VBATT_REG_EN, TRACK_VBATT_REG_EN);
	if (ret < 0)
		return ret;

	/* Set Temperature Regulation */
	ret = max77968_write_reg(max77968, TEMP_REG_REG, TEMP_REG_TH_SET);
	if (ret < 0)
		return ret;

	/* Set Regulation Timer */
	ret = max77968_set_regulation_timer(max77968, REG_TIMER_OFF);
	if (ret < 0)
		return ret;

	ret = max77968_set_iin_delay_timer(max77968, IIN_DELAY_TIMER_800ms);
	if (ret < 0)
		return ret;

	/* Set VIN OVP */
	val = VIN_OVP_EN;
	val |= VIN_OVP_DEG_TIME_80us << MASK2SHIFT(VIN_OVP_DEG);
	val |= VIN_OVP_TH_SET << MASK2SHIFT(VIN_OVP_TH);
	ret = max77968_write_reg(max77968, VIN_OVP_CFG_REG, val);
	if (ret < 0)
		return ret;

	/* Set VOUT OVP */
	val = VOUT_OVP_EN;
	val |= VOUT_OVP_DEG_TIME_100us << MASK2SHIFT(VOUT_OVP_DEG);
	val |= VOUT_OVP_TH_SET << MASK2SHIFT(VOUT_OVP_TH);
	ret = max77968_update_reg(max77968, VOUT_OVP_CFG_REG,
			VOUT_OVP_EN | VOUT_OVP_DEG | VOUT_OVP_TH, val);
	if (ret < 0)
		return ret;

	/* Set VBATT OVP */
	val = VBATT_OVP_DEG_TIME_100us << MASK2SHIFT(VBATT_OVP_DEG);
	ret = max77968_update_reg(max77968, VOUT_OVP_CFG_REG, VBATT_OVP_DEG, val);
	if (ret < 0)
		return ret;

	val = VBATT_OVP_EN;
	val |= VBATT_OVP_TH_CFG(VBATT_OVP_TH_SET) << MASK2SHIFT(VBATT_OVP_TH);
	ret = max77968_write_reg(max77968, VBATT_OVP_CFG_REG, val);

	/* Set CHGR OCP */
	val = CHGR_OCP_EN;
	val |= CHGR_OCP_DEG_NO << MASK2SHIFT(CHGR_OCP_DEG);
	val |= CHGR_OCP_CFG(CHGR_OCP_TH_SET) << MASK2SHIFT(CHGR_OCP);
	ret = max77968_write_reg(max77968, CHGR_OCP_CFG_REG, val);
	if (ret < 0)
		return ret;

	/* Set RVSBST_OCP */
	val = RVSBST_OCP_EN;
	val |= RVSBST_OCP_DEG_NO << MASK2SHIFT(RVSBST_OCP_DEG);
	val |= RVSBST_OCP_CFG(RVSBST_OCP_TH_SET) << MASK2SHIFT(RVSBST_OCP);
	ret = max77968_write_reg(max77968, RVSBST_OCP_CFG_REG, val);
	if (ret < 0)
		return ret;

	/* Set CHGR_RCP */
	val = CHGR_RCP_EN;
	val |= CHGR_RCP_DEG_50ms << MASK2SHIFT(CHGR_RCP_DEG);
	val |= CHGR_RCP_CFG(CHG_RCP_TH_SET) << MASK2SHIFT(CHGR_RCP);
	ret = max77968_write_reg(max77968, CHGR_RCP_CFG_REG, val);
	if (ret < 0)
		return ret;

	/* Set IIN_OCP */
	ret = max77968_update_reg(max77968, IIN_OCP_DEG_CFG_REG,
			IIN_OCP_DEG, IIN_OCP_DEG_1ms);
	if (ret < 0)
		return ret;

	val = IIN_OCP_EN;
	val |= IIN_OCP_CFG(IIN_OCP_TH_SET) << MASK2SHIFT(IIN_OCP);
	ret = max77968_write_reg(max77968, IIN_OCP_CFG_REG, val);
	if (ret < 0)
		return ret;

	/* Set IIN_UCP */
	val = IIN_UCP_EN;
	val |= IIN_UCP_DEG_20ms << MASK2SHIFT(IIN_UCP_DEG);
	val |= IIN_UCP_CFG(IIN_UCP_TH_SET) << MASK2SHIFT(IIN_UCP);
	ret = max77968_write_reg(max77968, IIN_UCP_CFG_REG, val);
	if (ret < 0)
		return ret;

	ret = max77968_read_reg(max77968, IIN_UCP_CFG_REG, &val);
	dev_info(max77968->dev, "%s: reading IIN_UCP_CFG_REG, val=0x%x\n", __func__, val);

	/* Set VIN_SHORT */
	val = VIN_SHORT_EN;
	val |= VIN_SHORT_DEG_NO << MASK2SHIFT(VIN_SHORT_DEG);
	val |= VIN_SHORT_TH_SET << MASK2SHIFT(VIN_SHORT_TH);
	ret = max77968_write_reg(max77968, VIN_SHORT_CFG_REG, val);
	if (ret < 0)
		return ret;

	/* Set SKIP_CFG_REG */
	val = SKIP_CFG_AUDIO | SKIP_CFG_SKIP;
	val |= VSKIP_140mV << MASK2SHIFT(SKIP_CFG_VSKIP);
	val |= ISKIP_1TO3_TH << MASK2SHIFT(SKIP_CFG_ISKIP);
	ret = max77968_write_reg(max77968, SKIP_CFG_REG, val);
	if (ret < 0)
		return ret;

	/* Set the ADC channel */
#if defined(ISSUE_WORKAROUND)
	val = (VIN_READ_EN | PMID_READ_EN | VOUT_READ_EN | VBATT_READ_EN | TDIE_READ_EN);
#else
	val = (VIN_READ_EN | PMID_READ_EN | VEXT1_READ_EN | VEXT2_READ_EN |
			VOUT_READ_EN | VBATT_READ_EN | NTC_READ_EN | TDIE_READ_EN);
#endif

	ret = max77968_write_reg(max77968, ADC_CFG1_REG, val);
	if (ret < 0)
		return ret;

	val = IIN_READ_EN;
	ret = max77968_update_reg(max77968, ADC_CFG2_REG, IIN_READ_EN, val);
	if (ret < 0)
		return ret;

	/* Enable ADC */
	val = ADC_EN | ADC_EN_BATTONLY;
	ret = max77968_write_reg(max77968, ADC_EN_REG, val);
	if (ret < 0)
		return ret;

	//Set NTC voltage threshold
	val = NTC_OT_CFG(max77968->pdata->ntc_ot_th) >> 4;
	ret = max77968_update_reg(max77968, NTC_OT_TH1_REG, NTC_OT_TH_HIGH_BIT, val);
	if (ret < 0)
		return ret;

	val = NTC_OT_CFG(max77968->pdata->ntc_ot_th) & 0xF0;
	ret = max77968_update_reg(max77968, NTC_OT_TH2_REG, NTC_OT_TH_LOW_BIT, val);
	if (ret < 0)
		return ret;

#if defined(EXT_SW_CONFIG_6)
	val = 0x00 << MASK2SHIFT(EXT1_DISCHG_CTRL1);
	val |= HIGH_DRV_VOLTAGE << MASK2SHIFT(VEXT1_DRV_VOLT);
	val |= TURN_ON_SPEED_SLOW << MASK2SHIFT(EXT_SW_RVS_TURN_ON_SPEED);
	ret = max77968_update_reg(max77968, EXT1_SW_CTRL_REG,
			EXT1_DISCHG_CTRL1 | VEXT1_DRV_VOLT | EXT_SW_RVS_TURN_ON_SPEED, val);
	if (ret < 0)
		return ret;

	val = 0x00 << MASK2SHIFT(EXT2_DISCHG_CTRL1);
	val |= EXT2_GATE_VB_FET << MASK2SHIFT(EXT2_GATE_CTRL);
	val |= HIGH_DRV_VOLTAGE << MASK2SHIFT(VEXT2_DRV_VOLT);
	val |= 0x00 << MASK2SHIFT(EXT_DRV_LPM_EN);
	ret = max77968_update_reg(max77968, EXT2_SW_CTRL_REG,
			EXT2_DISCHG_CTRL1 | EXT2_GATE_CTRL | VEXT2_DRV_VOLT | EXT_DRV_LPM_EN, val);
	if (ret < 0)
		return ret;

	val = VEXT_OVP_EN;
	val |= EXT_OVP_DEG_TIME_100ms << MASK2SHIFT(VEXT_OVP_DEG);
	val |= EXT_OVP_THRES_17P5V << MASK2SHIFT(VEXT_OVP_TH);
	ret = max77968_write_reg(max77968, VEXT1_OVP_CFG_REG, val);
	if (ret < 0)
		return ret;

	ret = max77968_write_reg(max77968, VEXT2_OVP_CFG_REG, val);
	if (ret < 0)
		return ret;

	val = EXT_SW_OPEN_DET_EN;
	val |= EXT_OPEN_DEB_TIME_100ms << MASK2SHIFT(EXT_SW_OPEN_DET_DEB);
	val |= OPEN_THRES_3P0V << MASK2SHIFT(EXT_SW_OPEN_DET_TH);
	val |= 0x00 << MASK2SHIFT(EXT_SW_OPEN_DET_OFF);
	ret = max77968_write_reg(max77968, EXT1_SW_OPEN_DET_CFG_REG, val);
	if (ret < 0)
		return ret;

	ret = max77968_write_reg(max77968, EXT2_SW_OPEN_DET_CFG_REG, val);
	if (ret < 0)
		return ret;
#else
	// Disable EXT OVP for configuration 1
	val = 0x00;
	val |= EXT_OVP_DEG_TIME_100ms << MASK2SHIFT(VEXT_OVP_DEG);
	val |= EXT_OVP_THRES_17P5V << MASK2SHIFT(VEXT_OVP_TH);
	ret = max77968_write_reg(max77968, VEXT1_OVP_CFG_REG, val);
	if (ret < 0)
		return ret;

	ret = max77968_write_reg(max77968, VEXT2_OVP_CFG_REG, val);
	if (ret < 0)
		return ret;

	// Disable SW_OPEN_DET for configuration 1
	val = 0x00;
	val |= EXT_OPEN_DEB_TIME_100ms << MASK2SHIFT(EXT_SW_OPEN_DET_DEB);
	val |= OPEN_THRES_3P0V << MASK2SHIFT(EXT_SW_OPEN_DET_TH);
	val |= 0x00 << MASK2SHIFT(EXT_SW_OPEN_DET_OFF);
	ret = max77968_write_reg(max77968, EXT1_SW_OPEN_DET_CFG_REG, val);
	if (ret < 0)
		return ret;

	ret = max77968_write_reg(max77968, EXT2_SW_OPEN_DET_CFG_REG, val);
	if (ret < 0)
		return ret;
#endif

	ret = max77968_interrupt_disable(max77968);
	if (ret < 0)
		return ret;

	return 0;
}

static int max77968_set_standby_state(struct max77968_charger *max77968, bool enable)
{
	int ret = 0;

	if (enable) {
		ret = max77968_update_reg(max77968, SCC_EN_REG, SCC_STANDBY_MODE_SET, SCC_STANDBY_MODE_SET);
		pr_info("%s: Set STANDBY_MODE_SET=1\n", __func__);
	} else {
		/* Use tregmap_lock to avoid setting the SCC to  */
		/* shutdown while accessing WR-related registers */
		mutex_lock(&max77968->tregmap_lock);
		ret = max77968_update_reg(max77968, SCC_EN_REG, SCC_STANDBY_MODE_SET, 0);
		pr_info("%s: Set STANDBY_MODE_SET=0\n", __func__);
		/* Wait 150ms to ensure all registers of SCC have been reset */
		msleep(150);
		pr_info("%s: Completed waiting for all registers to reset\n", __func__);
		mutex_unlock(&max77968->tregmap_lock);
	}

	return ret;
}

static int max77968_get_standby_state(struct max77968_charger *max77968, int *val)
{
	int ret, tmp;
	ret = max77968_read_reg(max77968, SCC_EN_REG, &tmp);
	if (ret < 0)
		return ret;

	*val = (tmp & SCC_STANDBY_MODE_SET) >> MASK2SHIFT(SCC_STANDBY_MODE_SET);
	pr_info("%s: Get STANDBY_MODE_SET=%d\n", __func__, *val);
	return 0;
}

static int max77968_set_operation_mode(struct max77968_charger *max77968, unsigned int mode)
{
	if (mode >= OPERATION_MODE_MAX)
		return -EINVAL;

	return max77968_update_reg(max77968, SCC_EN_REG, SCC_OPERATION_MODE, mode);
}

static int max77968_set_vbatt_regu_enable(struct max77968_charger *max77968, bool enable)
{
	int val;

	if (enable)
		val = TRACK_VBATT_REG_EN;
	else
		val = 0;

	return max77968_update_reg(max77968, IIN_REG_TRACK_REG, TRACK_VBATT_REG_EN, val);
}

static int max77968_get_vbatt_regu_enable(struct max77968_charger *max77968, int *enable)
{
	int ret, val;

	ret = max77968_read_reg(max77968, IIN_REG_TRACK_REG, &val);
	if (ret < 0)
		return ret;

	*enable = (val & TRACK_VBATT_REG_EN) >> MASK2SHIFT(TRACK_VBATT_REG_EN);

	return 0;
}

static int max77968_set_charging_state(struct max77968_charger *max77968, unsigned int charging_state)
{
	union power_supply_propval value = {0,};
	static int prev_val = DC_STATE_NO_CHARGING;

	max77968->charging_state = charging_state;

	pr_info("%s : %s (%d)\n", __func__, charging_state_str[max77968->charging_state], max77968->charging_state);

	switch (charging_state) {
	case DC_STATE_NO_CHARGING:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_OFF;
		break;
	case DC_STATE_CHECK_VBAT:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_CHECK_VBAT;
		break;
	case DC_STATE_PRESET_DC:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_PRESET;
		break;
	case DC_STATE_CHECK_ACTIVE:
	case DC_STATE_START_CC:
	case DC_STATE_START_CV:
	case DC_STATE_ADJUST_TAVOL:
	case DC_STATE_ADJUST_TACUR:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_ON_ADJUST;
		break;
	case DC_STATE_CC_MODE:
	case DC_STATE_CV_MODE:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_ON;
		break;
	case DC_STATE_CHARGING_DONE:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_DONE;
		break;
	case DC_STATE_BYPASS_MODE:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_BYPASS;
		break;
	default:
		return -1;
	}

	if (prev_val == value.intval)
		return -1;

	prev_val = value.intval;
	psy_do_property(max77968->pdata->sec_dc_name, set,
		POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE, value);

	return 0;
}

static void max77968_init_adc_val(struct max77968_charger *max77968, int val)
{
	int i = 0;

	for (i = 0; i < ADC_READ_MAX; ++i)
		max77968->adc_val[i] = val;
}

static void max77968_test_read(struct max77968_charger *max77968)
{
	int address = 0, ret;
	unsigned int val;
	char str[1024] = { 0, };

	for (address = INT_SRC1_M_REG; address <= MAX77968_MAX_REG; address++) {
		ret = max77968_read_reg(max77968, address, &val);
		if (ret < 0)
			return;
		sprintf(str + strlen(str), "[0x%02x]0x%02x, ", address, val);
	}

	pr_info("%s : %s\n", __func__, str);
}

static void max77968_monitor_work(struct max77968_charger *max77968)
{

#if defined(ISSUE_WORKAROUND)
	int ta_vol = max77968->ta_vol / MAX77968_SEC_DENOM_U_M;
	int ta_cur = max77968->ta_cur / MAX77968_SEC_DENOM_U_M;
	int iin;

	if (max77968->charging_state == DC_STATE_NO_CHARGING)
		return;

	max77968_read_adc(max77968, ADC_CH_VIN);
	if (max77968->adc_wr_en)
		iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
	else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
	max77968_read_adc(max77968, ADC_CH_VBATT);
	max77968_read_adc(max77968, ADC_CH_TDIE);
	max77968_read_adc(max77968, ADC_CH_VOUT);
	pr_info("%s: state(%s), iin_cc(%dmA), v_float(%dmV), vbat(%dmV), vin(%dmV), vout(%dmV)", __func__,
		charging_state_str[max77968->charging_state],
		max77968->iin_cc / MAX77968_SEC_DENOM_U_M, max77968->vfloat / MAX77968_SEC_DENOM_U_M,
		max77968->adc_val[ADC_CH_VBATT], max77968->adc_val[ADC_CH_VIN], max77968->adc_val[ADC_CH_VOUT]);
	pr_info(" iin(%dmA), die_temp(%d), pps_requested(%d/%dmV/%dmA)\n",
		iin, max77968->adc_val[ADC_CH_TDIE], max77968->ta_objpos, ta_vol, ta_cur);
#else
	int ta_vol = max77968->ta_vol / MAX77968_SEC_DENOM_U_M;
	int ta_cur = max77968->ta_cur / MAX77968_SEC_DENOM_U_M;

	if (max77968->charging_state == DC_STATE_NO_CHARGING)
		return;
	/* update adc value */
	max77968_read_adc(max77968, ADC_CH_VIN);
	max77968_read_adc(max77968, ADC_CH_IIN);
	max77968_read_adc(max77968, ADC_CH_VBATT);
	max77968_read_adc(max77968, ADC_CH_TDIE);
	max77968_read_adc(max77968, ADC_CH_VOUT);
	pr_info("%s: state(%s), iin_cc(%dmA), v_float(%dmV), vbat(%dmV), vin(%dmV), vout(%dmV)", __func__,
		charging_state_str[max77968->charging_state],
		max77968->iin_cc / MAX77968_SEC_DENOM_U_M, max77968->vfloat / MAX77968_SEC_DENOM_U_M,
		max77968->adc_val[ADC_CH_VBATT], max77968->adc_val[ADC_CH_VIN], max77968->adc_val[ADC_CH_VOUT]);
	pr_info(" iin(%dmA), die_temp(%d), pps_requested(%d/%dmV/%dmA)\n",
		max77968->adc_val[ADC_CH_IIN], max77968->adc_val[ADC_CH_TDIE],
		max77968->ta_objpos, ta_vol, ta_cur);
#endif
}

/**************************************/
/* Switching charger control function */
/**************************************/
/* This function needs some modification by a customer */
static void max77968_set_wdt_enable(struct max77968_charger *max77968, int en)
{
	int ret;
	unsigned int val;

	val = en << MASK2SHIFT(WD_TIMER_EN);
	ret = max77968_update_reg(max77968, WT_CFG_REG, WD_TIMER_EN, val);

	pr_info("%s: set wdt enable = %d\n", __func__, en);
}

static void max77968_set_wdt_timer(struct max77968_charger *max77968, int time)
{
	int ret;
	unsigned int val;

	val = time << MASK2SHIFT(WD_TIMER);
	ret = max77968_update_reg(max77968, WT_CFG_REG, WD_TIMER, val);

	pr_info("%s: set wdt time = %d\n", __func__, time);
}

static void max77968_clear_wdt_timer(struct max77968_charger *max77968)
{
	int val, vout;

	val = 0x01 << MASK2SHIFT(WDTCLR);
	max77968_update_reg(max77968, WT_CFG_REG, WDTCLR, val);
	vout = max77968_read_adc(max77968, ADC_CH_VOUT);
	pr_info("%s: VOUT=%d\n", __func__, vout);
}

static void max77968_check_wdt_control(struct max77968_charger *max77968)
{
	max77968_set_wdt_enable(max77968, WDT_DISABLE);
	max77968_set_wdt_timer(max77968, WDT_8SEC);
	max77968_set_wdt_enable(max77968, WDT_ENABLE);
	atomic_set(&max77968->suspend, 0);
	schedule_delayed_work(&max77968->wdt_control_work, msecs_to_jiffies(MAX77968_BATT_WDT_CONTROL_T));
}

static void max77968_wdt_control_work(struct work_struct *work)
{
	struct max77968_charger *max77968 = container_of(work, struct max77968_charger,
						wdt_control_work.work);
#if defined(ISSUE_WORKAROUND)
	static u8 wdt_cnt = 0;

	if (atomic_read(&max77968->suspend) == 1) {
		pr_info("%s: system suspend, func return\n", __func__);
		return;
	}

	if (max77968->timer_id == TIMER_ID_NONE) {
		pr_info("%s: timer_id=TIMER_ID_NONE, WDT_DISABLE\n", __func__);
		max77968_set_wdt_enable(max77968, WDT_DISABLE);
		return;
	}

	if (max77968->adc_wr_en) {
		max77968_adc_workaround(max77968);
		if (wdt_cnt++ >= 20) {
			wdt_cnt = 0;

			if (!max77968->wdt_kick_disable)
				max77968_clear_wdt_timer(max77968);
		}
		schedule_delayed_work(&max77968->wdt_control_work,
					msecs_to_jiffies(MAX77968_ADC_WR_T));
	} else {
		if (!max77968->wdt_kick_disable)
			max77968_clear_wdt_timer(max77968);
		schedule_delayed_work(&max77968->wdt_control_work,
							msecs_to_jiffies(MAX77968_BATT_WDT_CONTROL_T));
	}
#else
	if (!max77968->wdt_kick_disable)
		max77968_clear_wdt_timer(max77968);
	schedule_delayed_work(&max77968->wdt_control_work,
						msecs_to_jiffies(MAX77968_BATT_WDT_CONTROL_T));
#endif
}

static void max77968_set_done(struct max77968_charger *max77968, bool enable)
{
	int ret = 0;
	union power_supply_propval value = {0, };

	value.intval = enable;
	psy_do_property(max77968->pdata->sec_dc_name, set,
		POWER_SUPPLY_EXT_PROP_DIRECT_DONE, value);

	if (ret < 0)
		pr_info("%s: error set_done, ret=%d\n", __func__, ret);
}

static void max77968_set_switching_charger(struct max77968_charger *max77968, bool enable)
{
	int ret = 0;
	union power_supply_propval value = {0, };

	value.intval = enable;
	psy_do_property(max77968->pdata->sec_dc_name, set,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC, value);

	if (ret < 0)
		pr_info("%s: error switching_charger, ret=%d\n", __func__, ret);
}
#else
static int max77968_set_switching_charger(bool enable,
					unsigned int input_current,
					unsigned int charging_current,
					unsigned int vfloat)
{
	int ret;
	struct power_supply *psy_swcharger;
	union power_supply_propval val;

	pr_info("%s: enable=%d, iin=%d, ichg=%d, vfloat=%d\n",
		__func__, enable, input_current, charging_current, vfloat);

	/* Insert Code */
	/* Get power supply name */
	/* Change "sw-charger" to the customer's switching charger name */
	psy_swcharger = power_supply_get_by_name("sw-charger");

	if (psy_swcharger == NULL) {
		pr_err("%s: cannot get power_supply_name-usb\n", __func__);
		ret = -ENODEV;
		goto error;
	}

	if (enable == true)	{
		/* Set Switching charger */
		/* input current */
		val.intval = input_current;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &val);
		if (ret < 0)
			goto error;
		/* charging current */
		val.intval = charging_current;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, &val);
		if (ret < 0)
			goto error;
		/* vfloat voltage */
		val.intval = vfloat;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, &val);
		if (ret < 0)
			goto error;

		/* it depends on customer's code to enable charger */
		val.intval = enable;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_ONLINE, &val);
		if (ret < 0)
			goto error;
	} else {
		/* disable charger */
		/* it depends on customer's code to disable charger */
		val.intval = enable;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_ONLINE, &val);
		if (ret < 0)
			goto error;

		/* input_current */
		val.intval = input_current;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &val);
		if (ret < 0)
			goto error;
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}
#endif

/*******************/
/* Send PD message */
/*******************/
/* Send Request message to the source */
/* This function needs some modification by a customer */
static int max77968_send_pd_message(struct max77968_charger *max77968, unsigned int msg_type)
{
	unsigned int pdo_idx, pps_vol, pps_cur;
	int ret = 0;

	/* For Efficiency Debugging */
	max77968_monitor_work(max77968);

	/* Cancel pps request timer */
	cancel_delayed_work(&max77968->pps_work);

	mutex_lock(&max77968->lock);

	if (((max77968->charging_state == DC_STATE_NO_CHARGING) &&
		(msg_type == PD_MSG_REQUEST_APDO)) ||
		(max77968->mains_online == false)) {
		/* Vbus reset happened in the previous PD communication */
		goto out;
	}

	/* Check whether requested TA voltage and current are in valid range or not */
	if ((msg_type == PD_MSG_REQUEST_APDO) &&
		(max77968->byp_mode != PTM_1TO1) &&
		((max77968->ta_vol < (TA_MIN_VOL * max77968->chg_mode)) || (max77968->ta_cur < TA_MIN_CUR))) {
		/* request TA voltage or current is less than minimum threshold */
		/* This is abnormal case, too low input voltage and current */
		/* Normally VIN_UVLO already happened */
		pr_err("%s: Abnormal low RDO, ta_vol=%d, ta_cur=%d\n", __func__, max77968->ta_vol, max77968->ta_cur);
		ret = -EINVAL;
		goto out;
	}

	pr_info("%s: msg_type=%d, ta_cur=%d, ta_vol=%d, ta_objpos=%d\n",
		__func__, msg_type, max77968->ta_cur, max77968->ta_vol, max77968->ta_objpos);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	pdo_idx = max77968->ta_objpos;
	pps_vol = max77968->ta_vol / MAX77968_SEC_DENOM_U_M;
	pps_cur = max77968->ta_cur / MAX77968_SEC_DENOM_U_M;
	pr_info("## %s: msg_type=%d, pdo_idx=%d, pps_vol=%dmV(max_vol=%dmV), pps_cur=%dmA(max_cur=%dmA)\n",
		__func__, msg_type, pdo_idx,
		pps_vol, max77968->pdo_max_voltage,
		pps_cur, max77968->pdo_max_current);
#endif

	switch (msg_type) {
	case PD_MSG_REQUEST_APDO:
		ret = sec_pd_select_pps(pdo_idx, pps_vol, pps_cur);
		if (ret == -EBUSY) {
			pr_info("%s: request again ret=%d\n", __func__, ret);
			msleep(100);
			ret = sec_pd_select_pps(pdo_idx, pps_vol, pps_cur);
		}

		/* Start pps request timer */
		if (ret == 0) {
			queue_delayed_work(max77968->dc_wq,
								&max77968->pps_work,
								msecs_to_jiffies(PPS_PERIODIC_T));
		}
		break;
	case PD_MSG_REQUEST_FIXED_PDO:
		if (max77968->ta_type == TA_TYPE_USBPD_20) {
			pr_err("%s: ta_type(%d)! skip pd_select_pps\n", __func__, max77968->ta_type);
		} else {
			ret = sec_pd_select_pps(pdo_idx, pps_vol, pps_cur);
			if (ret == -EBUSY) {
				pr_info("%s: request again ret=%d\n", __func__, ret);
				msleep(100);
				ret = sec_pd_select_pps(pdo_idx, pps_vol, pps_cur);
			}
		}
		break;
	default:
		break;
	}

out:
	if (((max77968->charging_state == DC_STATE_NO_CHARGING) &&
		(msg_type == PD_MSG_REQUEST_APDO)) ||
		(max77968->mains_online == false)) {
		/* Even though PD communication success, Vbus reset might happen */
		/* So, check the charging state again */
		ret = -EINVAL;
	}

	pr_info("%s: ret=%d\n", __func__, ret);
	mutex_unlock(&max77968->lock);
	return ret;
}

/************************/
/* Get APDO max power   */
/************************/
/* Get the max current/voltage/power of APDO from the CC/PD driver */
/* This function needs some modification by a customer */
static int max77968_get_apdo_max_power(struct max77968_charger *max77968)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	unsigned int ta_max_vol_mv = (max77968->ta_max_vol / MAX77968_SEC_DENOM_U_M);
	unsigned int ta_max_cur_ma = 0;
	unsigned int ta_max_pwr_uw = 0;
#endif

	ret = sec_pd_get_apdo_max_power(&max77968->ta_objpos,
				&ta_max_vol_mv, &ta_max_cur_ma, &ta_max_pwr_uw);
	/* mA,mV,uW --> uA,uV,uW */
	max77968->ta_max_vol = ta_max_vol_mv * MAX77968_SEC_DENOM_U_M;
	max77968->ta_max_cur = ta_max_cur_ma * MAX77968_SEC_DENOM_U_M;
	max77968->ta_max_pwr = ta_max_pwr_uw;

	pr_info("%s: ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d\n",
		__func__, max77968->ta_max_vol, max77968->ta_max_cur, max77968->ta_max_pwr);

	max77968->pdo_index = max77968->ta_objpos;
	max77968->pdo_max_voltage = ta_max_vol_mv;
	max77968->pdo_max_current = ta_max_cur_ma;

	return ret;
}


/******************/
/* Set RX voltage */
/******************/
/* Send RX voltage to RX IC */
/* This function needs some modification by a customer */
static int max77968_send_rx_voltage(struct max77968_charger *max77968, unsigned int msg_type)
{
	struct power_supply *psy;
	union power_supply_propval pro_val;
	int ret = 0;

	mutex_lock(&max77968->lock);

	if (max77968->mains_online == false) {
		/* Vbus reset happened in the previous PD communication */
		goto out;
	}

	pr_info("## %s: rx_vol=%d\n", __func__, max77968->ta_vol);

	psy = power_supply_get_by_name("wireless");
	if (!psy) {
		pr_err("Cannot find wireless power supply\n");
		ret = -ENODEV;
		goto out;
	}

	pro_val.intval = max77968->ta_vol;
	/* uA to mA */
	pro_val.intval /= 1000;
	ret = power_supply_set_property(psy,
		(enum power_supply_property)POWER_SUPPLY_EXT_PROP_WC_SELECT_PPS, &pro_val);
	power_supply_put(psy);
	if (ret < 0) {
		pr_err("Cannot set voltage\n");
		ret = -ENODEV;
		goto out;
	}

out:
	if (max77968->mains_online == false) {
		/* Even though PD communication success, Vbus reset might happen */
		/* So, check the charging state again */
		ret = -EINVAL;
	}

	pr_info("%s: ret=%d\n", __func__, ret);
	mutex_unlock(&max77968->lock);
	return ret;
}


/************************/
/* Get RX max power     */
/************************/
/* Get the max current/voltage/power of RXIC from the WCRX driver */
/* This function needs some modification by a customer */
static int max77968_get_rx_max_power(struct max77968_charger *max77968)
{
	struct power_supply *psy;
	int ret = 0;

	/* Get power supply name */
	psy = power_supply_get_by_name("wireless");
	if (!psy) {
		dev_err(max77968->dev, "Cannot find wireless power supply\n");
		ret = -ENODEV;
		return ret;
	}

	/* 15V & 1.8A & 2.7W */
	max77968->ta_max_vol = 15000000;
	max77968->ta_max_cur = 1800000;
	max77968->ta_max_pwr = 27000000;

	pr_info("%s: ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d\n",
		__func__, max77968->ta_max_vol, max77968->ta_max_cur, max77968->ta_max_pwr);

	power_supply_put(psy);
	return ret;
}

/**************************/
/* MAX77968 Local function */
/**************************/
/* ADC Read function */
static int max77968_read_adc(struct max77968_charger *max77968, u8 adc_ch)
{
	union power_supply_propval value = {0,};
	u8 reg_addr = 0;
	u8 reg_data[2];
	u16 raw_adc = 0;
	int conv_adc = 0, adc_step = 0;
	int ret;

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		if (adc_ch == ADC_CH_VBATT) {
			ret = psy_do_property(max77968->pdata->fg_name, get, POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
			if (ret < 0) {
				conv_adc = ret;
				goto error;
			}
			conv_adc = value.intval * MAX77968_SEC_DENOM_U_M;
			pr_info("%s: fuelgauge vbatt_adc, convert_val = (%d) -> (%d)\n", __func__, value.intval, conv_adc);
			goto error;
		}
	}

	switch (adc_ch) {
	case ADC_CH_VIN:
		adc_step = VIN_STEP;
		reg_addr = ADC_VIN_READ_REG;
		break;
	case ADC_CH_PMID:
		adc_step = PMID_STEP;
		reg_addr = ADC_PMID_READ_REG;
		break;
	case ADC_CH_VEXT1:
		adc_step = VEXT1_STEP;
		reg_addr = ADC_VEXT1_READ_REG;
		break;
	case ADC_CH_VEXT2:
		adc_step = VEXT2_STEP;
		reg_addr = ADC_VEXT2_READ_REG;
		break;
	case ADC_CH_VOUT:
		adc_step = VOUT_STEP;
		reg_addr = ADC_VOUT_READ_REG;
		break;
	case ADC_CH_VBATT:
		adc_step = VBATT_STEP;
		reg_addr = ADC_VBATT_READ_REG;
		break;
	case ADC_CH_NTC:
		adc_step = NTC_STEP;
		reg_addr = ADC_NTC_READ_REG;
		break;
	case ADC_CH_TDIE:
		adc_step = TDIE_STEP;
		reg_addr = ADC_TDIE_READ_REG;
		break;
	case ADC_CH_IIN:
		adc_step = IIN_STEP;
		reg_addr = ADC_IIN_READ_REG;
		break;
	default:
		conv_adc = -EINVAL;
	}

	ret = max77968_bulk_read_reg(max77968, reg_addr, reg_data, 2);
	if (ret < 0) {
		conv_adc = ret;
		goto error;
	}

	raw_adc = ((reg_data[0] << 4) & ADC_VAL_HIGH_MASK) | ((reg_data[1] >> 4) & ADC_VAL_LOW_MASK);

	if (adc_ch == ADC_CH_TDIE) {
		/* Transfer to 16 bits signed value */
		if (raw_adc & 0x800)
			raw_adc |= 0xF000;
		conv_adc = (s16)raw_adc * adc_step / TDIE_DENOM;
		if (conv_adc > TDIE_MAX)
			conv_adc = TDIE_MAX;
	} else
		conv_adc = raw_adc * adc_step;

error:
	pr_info("%s: adc_ch=%d, raw_adc=0x%x, convert_val=%d\n", __func__,
		adc_ch, raw_adc, conv_adc);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	if (adc_ch == ADC_CH_TDIE)
		max77968->adc_val[adc_ch] = conv_adc;
	else
		max77968->adc_val[adc_ch] = conv_adc / MAX77968_SEC_DENOM_U_M;
#endif
	return conv_adc;
}

static int max77968_set_vfloat(struct max77968_charger *max77968, unsigned int vfloat)
{
	int ret, val;

	pr_info("%s: vfloat=%d\n", __func__, vfloat);

	/* float voltage - battery regulation voltage */
	/* maximum value is 4550mV */
	if (vfloat > VBAT_REG_MAX)
		vfloat = VBAT_REG_MAX;
	/* minimum value is 3800mV */
	if (vfloat < VBAT_REG_MIN)
		vfloat = VBAT_REG_MIN;

	val = VBAT_REG_CFG(vfloat);
	pr_info("%s: vfloat=%d, (0x%x)\n", __func__, vfloat, val);

	ret = max77968_write_reg(max77968, VBATT_REGULATION_REG, val);

	return ret;
}

static int max77968_ensure_scc_standby(struct max77968_charger *max77968)
{
	int i, ret, val;
	int adc_val[5];

	ret = max77968_get_standby_state(max77968, &val);
	if (ret < 0)
		return ret;

	if (val == 0) {
		pr_info("%s: STANDBY_MODE_SET is 0, set STANDBY_MODE_SET to 1\n", __func__);
		ret = max77968_set_standby_state(max77968, true);
		if (ret < 0)
			return ret;

		ret = max77968_config_init(max77968);
		if (ret < 0)
			return ret;

		/* Wait 100ms for all ADCs update once */
		msleep(100);
	}

	adc_val[0] = max77968_read_adc(max77968, ADC_CH_VIN);
	adc_val[1] = max77968_read_adc(max77968, ADC_CH_PMID);
	adc_val[2] = max77968_read_adc(max77968, ADC_CH_VOUT);
	adc_val[3] = max77968_read_adc(max77968, ADC_CH_TDIE);
	adc_val[4] = max77968_read_adc(max77968, ADC_CH_IIN);
	for (i = 0; i < 5; i++) {
		if (adc_val[i] != 0) {
			pr_info("%s: ADC results are not all zero (1)\n", __func__);
			return 0;
		}
	}

	pr_info("%s: adc results are all zero\n", __func__);

	ret = max77968_get_standby_state(max77968, &val);
	if (ret < 0)
		return ret;

	if (val == 0) {
		pr_info("%s: STANDBY_MODE_SET is still 0, set STANDBY_MODE_SET to 1 again\n", __func__);
		ret = max77968_set_standby_state(max77968, true);
		if (ret < 0)
			return ret;

		ret = max77968_config_init(max77968);
		if (ret < 0)
			return ret;

		/* Wait 100ms for all ADCs update once */
		msleep(100);
	}

	adc_val[0] = max77968_read_adc(max77968, ADC_CH_VIN);
	adc_val[1] = max77968_read_adc(max77968, ADC_CH_PMID);
	adc_val[2] = max77968_read_adc(max77968, ADC_CH_VOUT);
	adc_val[3] = max77968_read_adc(max77968, ADC_CH_TDIE);
	adc_val[4] = max77968_read_adc(max77968, ADC_CH_IIN);
	for (i = 0; i < 5; i++) {
		if (adc_val[i] != 0) {
			pr_info("%s: ADC results are not all zero (2)\n", __func__);
			return 0;
		}
	}

	pr_info("%s: adc results are still all zero, SCC might shutdown becasue of ENB pin\n", __func__);
	return -EINVAL;
}

static int max77968_vbatt_regu_enable_chk(struct max77968_charger *max77968)
{
	int ret, vbatt, val;

	/* Read VBAT_ADC and regulation enable bit */
	vbatt = max77968_read_adc(max77968, ADC_CH_VBATT);
	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		return ret;

	if ((vbatt >= VBATT_REG_ENA_TH) && (val == 0)) {
		max77968->force_vbat_reg_off = false;
		/* Enable VBATT regulation threshold to default (4.45V) */
		ret = max77968_set_vfloat(max77968, VBAT_REG_DFT);
		if (ret < 0)
			return ret;
		pr_info("%s: vbatt=%d, Set VBATT_REG_EN=1\n", __func__, vbatt);
		return max77968_set_vbatt_regu_enable(max77968, true);
	}

	return 0;
}

static int max77968_set_input_current(struct max77968_charger *max77968, unsigned int iin)
{
	int ret, val, iin_val;

	pr_info("%s: iin=%d\n", __func__, iin);

	/* input current - input regulation current */
	/* round-up input current with input regulation resolution */
	if (iin % IIN_REG_STEP)
		iin = iin + IIN_REG_STEP;

	/* Add offset for input current regulation */
	/* Check TA type */
	if (max77968->ta_type == TA_TYPE_WIRELESS) {
		/* Add offset */
		if (iin > IIN_REG_RX_OFFSET4_TH)
			iin = iin + IIN_REG_RX_OFFSET4;
		else if (iin > IIN_REG_RX_OFFSET3_TH)
			iin = iin + IIN_REG_RX_OFFSET3;
		else if (iin > IIN_REG_RX_OFFSET2_TH)
			iin = iin + IIN_REG_RX_OFFSET2;
		else if (iin > IIN_REG_RX_OFFSET1_TH)
			iin = iin + IIN_REG_RX_OFFSET1;
	} else if (max77968->ta_type == TA_TYPE_USBPD_20) {
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		if (iin < max77968->pdata->fpdo_dc_iin_lowest_limit) {
			pr_info("%s: IIN LOWEST LIMIT! IIN %d -> %d\n", __func__,
					iin, max77968->pdata->fpdo_dc_iin_lowest_limit);
			iin = max77968->pdata->fpdo_dc_iin_lowest_limit;
		}
#endif
		/* Apply FPDO offset */
		iin = iin + IIN_REG_OFFSET_FPDO;
	} else {
		/* Add offset for input current regulation */
		if (iin < IIN_REG_OFFSET1_TH)
			iin = iin + IIN_REG_OFFSET1;
		else if (iin < IIN_REG_OFFSET2_TH)
			iin = iin + IIN_REG_OFFSET2;
		else if (iin < IIN_REG_OFFSET3_TH)
			iin = iin + IIN_REG_OFFSET3;
		else if (iin < IIN_REG_OFFSET4_TH)
			iin = iin + IIN_REG_OFFSET4;
		else
			iin = iin + IIN_REG_OFFSET5;
	}

	/* maximum value is 5.5A */
	if (iin > IIN_REG_MAX)
		iin = IIN_REG_MAX;
	/* minimum value is 500mA */
	if (iin < IIN_REG_MIN)
		iin = IIN_REG_MIN;

	iin_val = IIN_REG_CFG(iin);
	val = IIN_REG_EN;
	val |= iin_val << MASK2SHIFT(IIN_REG_TH);
	ret = max77968_write_reg(max77968, IIN_REGULATION_REG, val);

	pr_info("%s: real iin_cfg=%d\n", __func__, (iin_val * IIN_REG_STEP) + IIN_REG_MIN);
	return ret;
}

static int max77968_set_charging(struct max77968_charger *max77968, bool enable)
{
	int ret, val;
	u8 sc_op_reg = STANDBY_STATE;
	u8 int_reg[REG_INT_MAX];

	pr_info("%s: enable=%d, byp_mode=%d\n", __func__, enable, max77968->byp_mode);

	if (max77968->byp_mode == PTM_1TO1) {
		/* Pass through 1:1 mode */
		sc_op_reg = FWD_1TO1;
		max77968->fsw_cfg = max77968->pdata->fsw_cfg_byp;
	} else if (max77968->byp_mode == PTM_2TO1) {
		/* Pass through 2:1 mode */
		sc_op_reg = FWD_2TO1;
		max77968->fsw_cfg = max77968->pdata->fsw_cfg_2to1;
	} else if (max77968->byp_mode == PTM_3TO1) {
		/* Pass through 3:1 mode */
		sc_op_reg = FWD_3TO1;
		max77968->fsw_cfg = max77968->pdata->fsw_cfg_3to1;
	} else {
		/* Pass through mode none */
		if (max77968->chg_mode == CHG_2TO1_DC_MODE) {
			sc_op_reg = FWD_2TO1;
			max77968->fsw_cfg = max77968->pdata->fsw_cfg_2to1;
		} else {
			sc_op_reg = FWD_3TO1;
			max77968->fsw_cfg = max77968->pdata->fsw_cfg_3to1;
		}
	}

	/* Check device's current status */
	ret = max77968_read_reg(max77968, SCC_EN_REG, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: before enable SCC_EN_REG = 0x%x\n", __func__, val);

	if (enable == true) {
		/* Set NTC over temperature alert */
		val = (max77968->pdata->ntc_ot_en) << MASK2SHIFT(NTC_OT_EN);
		ret = max77968_update_reg(max77968, ADC_CFG2_REG, NTC_OT_EN, val);
		if (ret < 0)
			return ret;

#if defined(ISSUE_WORKAROUND)
		max77968_before_scc_enable_workaround(max77968, sc_op_reg);

		if ((sc_op_reg == FWD_2TO1) && (max77968->iin_cfg >= 3300000)
			&& (max77968->pass3_wr_en == true)) {
			max77968->adc_wr_en = true;
			pr_info("%s: ADC_WR_ENABLE\n", __func__);
		} else {
			max77968->adc_wr_en = false;
			pr_info("%s: ADC_WR_DISABLE\n", __func__);
		}
#endif
		// Read all interrupt flags once to clear all flags before activate state
		ret = max77968_bulk_read_reg(max77968, INT_SRC1_REG, &int_reg[REG_INT1], REG_INT_MAX);
		if (ret < 0)
			return ret;

		/* Set switching frequency before enabling SCC */
		ret = max77968_update_reg(max77968, FSW_CFG_REG, FSW_FREQ, max77968->fsw_cfg);
		if (ret < 0)
			return ret;

		/* Set SCC OP Mode*/
		ret = max77968_set_operation_mode(max77968, sc_op_reg);
		if (ret < 0)
			return ret;

		/* Set enable flag to true */
		max77968->enable = true;
	} else {
		/* Disable NTC over temperature alert */
		ret = max77968_update_reg(max77968, ADC_CFG2_REG, NTC_OT_EN, 0);

		ret = max77968_set_operation_mode(max77968, STANDBY_STATE);
		if (ret < 0)
			return ret;

		/* Set enable flag to false */
		max77968->enable = false;
	}

	/* Read SCC_EN_REG register */
	ret = max77968_read_reg(max77968, SCC_EN_REG, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: SCC_EN_REG after setting = 0x%x\n", __func__, val);

#if defined(ISSUE_WORKAROUND)
	if (enable == false)
		max77968_after_scc_disable_workaround(max77968);
#endif

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

static int max77968_set_vin_ocp(struct max77968_charger *max77968, unsigned int vin_ocp)
{
	int ret, val;

	pr_info("%s: vin_ocp=%d\n", __func__, vin_ocp);

	/* maximum value is 2A */
	if (vin_ocp > IIN_OCP_MAX)
		vin_ocp = IIN_OCP_MAX;
	/* minimum value is 500mA */
	if (vin_ocp < IIN_OCP_MIN)
		vin_ocp = IIN_OCP_MIN;

	/* Set VIN OCP current */
	val = IIN_OCP_CFG(vin_ocp) << MASK2SHIFT(IIN_OCP);
	ret = max77968_update_reg(max77968, IIN_OCP_CFG_REG, IIN_OCP, val);

	return ret;
}

static int max77968_set_reverse_mode(struct max77968_charger *max77968, bool enable)
{
	int ret, val;
	u8 sc_op_reg, iskip_cfg;
	u8 int_reg[REG_INT_MAX];

	pr_info("%s: enable=%d\n", __func__, enable);

	// POWER_SUPPLY_DC_REVERSE_1TO3 has not defined in sec_battery_common.h yet
	if (max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_1TO3) {
		/* 1:3 switching mode */
		sc_op_reg = RVS_1TO3;
		iskip_cfg = ISKIP_1TO3_TH;
		max77968->fsw_cfg = max77968->pdata->fsw_cfg_3to1;
	} else if (max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_1TO2) {
		/* 1:2 switching mode */
		sc_op_reg = RVS_1TO2;
		iskip_cfg = ISKIP_1TO2_TH;
		max77968->fsw_cfg = max77968->pdata->fsw_cfg_2to1;
	} else {
		/* Reverse 1:1 mode */
		sc_op_reg = RVS_1TO1;
		iskip_cfg = ISKIP_1TO1_TH;
		max77968->fsw_cfg = max77968->pdata->fsw_cfg_byp;
	}

	/* Check device's current status */
	ret = max77968_read_reg(max77968, SCC_EN_REG, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: SCC_EN_REG = 0x%x before setting\n", __func__, val);

	if (enable == true) {
		/* Set NTC over temperature alert */
		val = (max77968->pdata->ntc_ot_en) << MASK2SHIFT(NTC_OT_EN);
		ret = max77968_update_reg(max77968, ADC_CFG2_REG, NTC_OT_EN, val);
		if (ret < 0)
			return ret;

#if defined(ISSUE_WORKAROUND)
		max77968_before_scc_enable_workaround(max77968, sc_op_reg);
#endif
		// Read all interrupt flags once to clear all flags before activate state
		ret = max77968_bulk_read_reg(max77968, INT_SRC1_REG, &int_reg[REG_INT1], REG_INT_MAX);
		if (ret < 0)
			return ret;

		/* Set switching frequency before enabling SCC */
		ret = max77968_update_reg(max77968, FSW_CFG_REG, FSW_FREQ, max77968->fsw_cfg);
		if (ret < 0)
			return ret;

		/* Set Skip Mode Current Threshold */
		val = iskip_cfg << MASK2SHIFT(SKIP_CFG_ISKIP);
		ret = max77968_update_reg(max77968, SKIP_CFG_REG, SKIP_CFG_ISKIP, val);
		if (ret < 0)
			return ret;

		/* Set SCC OP Mode*/
		ret = max77968_set_operation_mode(max77968, sc_op_reg);
		if (ret < 0)
			return ret;

		/* Set enable flag to true */
		max77968->enable = true;
	} else {
		/* Disable NTC Protection */
		/* Set NTC over temperature alert */
		ret = max77968_update_reg(max77968, ADC_CFG2_REG, NTC_OT_EN, 0);
		if (ret < 0)
			return ret;

		/* Set enable flag to false */
		max77968->enable = false;
	}
	/* Read SCC_EN_REG register */
	ret = max77968_read_reg(max77968, SCC_EN_REG, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: SCC_EN_REG after setting = 0x%x\n", __func__, val);

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

static int max77968_softreset(struct max77968_charger *max77968)
{
	int ret;
	u8 reg_val[11]; /* Dump for control register */

	pr_info("%s: do soft reset\n", __func__);

	/* Check the current register before softreset */
	pr_info("%s: Before softreset\n", __func__);

	/* Read all status registers for debugging */
	ret = max77968_bulk_read_reg(max77968, STATUS1_REG, &reg_val[0], 3);
	if (ret < 0)
		goto error;
	pr_info("%s: status STATUS1=0x%x, STATUS2=0x%x, STATUS3=0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2]);

	/* Read all control registers for debugging */
	ret = max77968_bulk_read_reg(max77968, SCC_EN_REG, &reg_val[0], 4);
	if (ret < 0)
		goto error;
	pr_info("%s: control SCC_EN=0x%x, FSW_CFG=0x%x, SKIP_CFG=0x%x, SS_CFG==0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2], reg_val[3]);

	/* Read all regulation registers for debugging */
	ret = max77968_bulk_read_reg(max77968, IIN_REGULATION_REG, &reg_val[0], 5);
	if (ret < 0)
		goto error;
	pr_info("%s: regul IIN_REG=0x%x, IIN_REG_TRACK=0x%x, VBATT_REG=0x%x, TEMP_REG=0x%x, RT_CFG=0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2], reg_val[3], reg_val[4]);

	/* Read all protection registers for debugging */
	ret = max77968_bulk_read_reg(max77968, VIN_OVP_CFG_REG, &reg_val[0], 11);
	if (ret < 0)
		goto error;
	pr_info("%s: protect VIN_OVP=0x%x, VOUT_OVP=0x%x, VBATT_OVP=0x%x, CHGR_OCP=0x%x, RVSBST_OCP=0x%x, CHGR_RCP=0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2], reg_val[3], reg_val[4], reg_val[5]);
	pr_info("%s: protect IIN_OCP=0x%x, IIN_OCP_DEG=0x%x, IIN_UCP=0x%x, VIN_SHORT=0x%x, WT=0x%x\n",
			__func__, reg_val[6], reg_val[7], reg_val[8], reg_val[9], reg_val[10]);

	/* Do softreset */
	/* Set softreset register */
	ret = max77968_write_reg(max77968, SW_RST_REG, SW_RST_OPCODE);

	/* Wait 15ms */
	msleep(15);

	/* Reset MAX77968 and all regsiters values go to POR values */
	/* Check the current register after softreset */
	pr_info("%s: After softreset\n", __func__);

	/* Read all status registers for debugging */
	ret = max77968_bulk_read_reg(max77968, STATUS1_REG, &reg_val[0], 3);
	if (ret < 0)
		goto error;
	pr_info("%s: status STATUS1=0x%x, STATUS2=0x%x, STATUS3=0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2]);

	/* Read all control registers for debugging */
	ret = max77968_bulk_read_reg(max77968, SCC_EN_REG, &reg_val[0], 4);
	if (ret < 0)
		goto error;
	pr_info("%s: control SCC_EN=0x%x, FSW_CFG=0x%x, SKIP_CFG=0x%x, SS_CFG==0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2], reg_val[3]);

	/* Read all regulation registers for debugging */
	ret = max77968_bulk_read_reg(max77968, IIN_REGULATION_REG, &reg_val[0], 5);
	if (ret < 0)
		goto error;
	pr_info("%s: regul IIN_REG=0x%x, IIN_REG_TRACK=0x%x, VBATT_REG=0x%x, TEMP_REG=0x%x, RT_CFG=0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2], reg_val[3], reg_val[4]);

	/* Read all protection registers for debugging */
	ret = max77968_bulk_read_reg(max77968, VIN_OVP_CFG_REG, &reg_val[0], 11);
	if (ret < 0)
		goto error;
	pr_info("%s: protect VIN_OVP=0x%x, VOUT_OVP=0x%x, VBATT_OVP=0x%x, CHGR_OCP=0x%x, RVSBST_OCP=0x%x, CHGR_RCP=0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2], reg_val[3], reg_val[4], reg_val[5]);
	pr_info("%s: protect IIN_OCP=0x%x, IIN_OCP_DEG=0x%x, IIN_UCP=0x%x, VIN_SHORT=0x%x, WT=0x%x\n",
			__func__, reg_val[6], reg_val[7], reg_val[8], reg_val[9], reg_val[10]);

	ret = max77968_config_init(max77968);
	if (ret < 0)
		goto error;

	return 0;

error:
	pr_info("%s: i2c error, ret=%d\n", __func__, ret);
	return ret;
}

static void max77968_dc_error_print(struct max77968_charger *max77968, u8 *int_val)
{
	if (int_val[REG_INT1] & VIN_OVP_INT) {
		max77968->error_cause = ERR_NODE_VIN_OVP;
		goto out;
	}

	if (int_val[REG_INT1] & VIN_UVLO_INT) {
		max77968->error_cause = ERR_NODE_VIN_UVLO;
		goto out;
	}

	if (int_val[REG_INT1] & VOUT_OVP_INT) {
		max77968->error_cause = ERR_NODE_VOUT_OVP;
		goto out;
	}

	if (int_val[REG_INT1] & VOUT_UVLO_INT) {
		max77968->error_cause = ERR_NODE_VOUT_UVLO;
		goto out;
	}

	if (int_val[REG_INT1] & VBATT_OVP_INT) {
		max77968->error_cause = ERR_NODE_VBAT_OVP;
		goto out;
	}

	if (int_val[REG_INT1] & T_SHDN_INT) {
		max77968->error_cause = ERR_NODE_THM_SHUTDOWN;
		goto out;
	}

	if (int_val[REG_INT1] & T_ALARM_INT) {
		max77968->error_cause = ERR_NODE_DIE_TEMP_WARN;
		goto out;
	}

	if (int_val[REG_INT2] & CHGR_SS_FAULT_INT) {
		max77968->error_cause = ERR_NODE_SOFT_START_TIMEOUT;
		goto out;
	}

	if (int_val[REG_INT2] & CHGR_OCP_INT) {
		max77968->error_cause = ERR_NODE_CHGR_OCP;
		goto out;
	}

	if (int_val[REG_INT2] & CHGR_RCP_INT) {
		max77968->error_cause = ERR_NODE_CHGR_RCP;
		goto out;
	}

	if (int_val[REG_INT2] & RVSBST_SS_FAULT_INT) {
		max77968->error_cause = ERR_NODE_SOFT_START_TIMEOUT;
		goto out;
	}

	if (int_val[REG_INT2] & RVSBST_OCP_INT) {
		max77968->error_cause = ERR_NODE_RVSBST_OCP;
		goto out;
	}

	if (int_val[REG_INT3] & REG_TIMEOUT_INT) {
		max77968->error_cause = ERR_NODE_REGULATION_TIMEOUT;
		goto out;
	}

	if (int_val[REG_INT3] & IIN_OCP_INT) {
		max77968->error_cause = ERR_NODE_IBUS_OCP;
		goto out;
	}

	if (int_val[REG_INT3] & IIN_UCP_INT) {
		max77968->error_cause = ERR_NODE_IBUS_UCP;
		goto out;
	}

	if (int_val[REG_INT3] & VIN_SHORT_INT) {
		max77968->error_cause = ERR_NODE_VIN_SHORT;
		goto out;
	}

	if (int_val[REG_INT4] & VEXT1_OVP_INT) {
		max77968->error_cause = ERR_NODE_VEXT1_OVLO;
		goto out;
	}

	if (int_val[REG_INT4] & VEXT1_UVLO_INT) {
		max77968->error_cause = ERR_NODE_VEXT1_UVLO;
		goto out;
	}

	if (int_val[REG_INT4] & VEXT2_OVP_INT) {
		max77968->error_cause = ERR_NODE_VEXT2_OVLO;
		goto out;
	}

	if (int_val[REG_INT4] & VEXT2_UVLO_INT) {
		max77968->error_cause = ERR_NODE_VEXT2_UVLO;
		goto out;
	}

	if (int_val[REG_INT5] & PVDD_UVP_INT) {
		max77968->error_cause = ERR_NODE_PVDD_UVP;
		goto out;
	}

	if (int_val[REG_INT5] & CFLY_OPEN_INT) {
		max77968->error_cause = ERR_NODE_CFLY_OPEN_DET;
		goto out;
	}

	if (int_val[REG_INT5] & CFLY_SHORT_INT) {
		max77968->error_cause = ERR_NODE_CFLY_SHORT;
		goto out;
	}

	if (int_val[REG_INT5] & BST_UVP_INT) {
		max77968->error_cause = ERR_NODE_BST_UVP;
		goto out;
	}

	if (int_val[REG_INT5] & WT_INT) {
		max77968->error_cause = ERR_NODE_WTD_TIMER;
		goto out;
	}

	if (int_val[REG_INT5] & NTC_OT_INT) {
		max77968->error_cause = ERR_NODE_NTC_PROT;
		goto out;
	}

	return;

out:
	pr_info("%s: ERR NODE 0x%06x\n", __func__, max77968->error_cause);
	return;
}
/* Check Active status */
static int max77968_check_error(struct max77968_charger *max77968)
{
	int ret;
	unsigned int scc_en_reg, status2_reg, mode, src2_reg = 0;
	int vbatt;
	int rt, i;
	char chr_buf[200] = "\0";
	union power_supply_propval value = {0, };

	u8	val[REG_INT_MAX] = {0};

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968->chg_status = POWER_SUPPLY_STATUS_CHARGING;
	max77968->health_status = POWER_SUPPLY_HEALTH_GOOD;
#endif

	if (max77968->charging_state == DC_STATE_CHECK_ACTIVE) {
		for (i = 0; i < 50; i++) {

			ret = max77968_read_reg(max77968, INT_SRC2_REG, &src2_reg);
			if (ret < 0)
				goto error;

			/* if CHGR_SS_FAULT happened */
			if (src2_reg & CHGR_SS_FAULT_INT)
				break;

			/* if CHGR_SS_DONE detected */
			if (src2_reg & CHGR_SSDONE_INT) {
				pr_info("%s: CHGR_SS_DONE\n", __func__);
				break;
			}

			/* Delay 100ms */
			usleep_range(100000, 110000);
		}
	}

	ret = max77968_read_reg(max77968, SCC_EN_REG, &scc_en_reg);
	if (ret < 0)
		goto error;

	/* Read VBAT_ADC */
	vbatt = max77968_read_adc(max77968, ADC_CH_VBATT);

	mode = scc_en_reg & SCC_OPERATION_MODE;

	/* Check Active status */
	if ((mode == FWD_3TO1) || (mode == FWD_2TO1) || (mode == FWD_1TO1)) {
		/* max77968 is in 3:1/2:1/1:1 switching mode */
		/* Check whether the battery voltage is over the minimum voltage level or not */
		if (vbatt > DC_VBAT_MIN) {
			/* Normal charging battery level */
			/* Check temperature regulation loop */
			ret = max77968_read_reg(max77968, STATUS2_REG, &status2_reg);
			if (status2_reg & TEMP_REG_S) {
				/* Thermal regulation happened */
				pr_err("%s: Device is in temperature regulation\n", __func__);
				ret = -EINVAL;
			} else {
				/* Normal temperature */
				ret = 0;
			}
		} else {
			/* Abnormal battery level */
			pr_err("%s: Error abnormal battery voltage=%d\n", __func__, vbatt);
			ret = -EINVAL;
			goto error;
		}

		if ((max77968->byp_mode == PTM_NONE) &&
			((mode == FWD_3TO1 && max77968->chg_mode == CHG_3TO1_DC_MODE) ||
			(mode == FWD_2TO1 && max77968->chg_mode == CHG_2TO1_DC_MODE))) {
			// Do nothing, conditions are satisfied
			ret = 0;
		} else if ((mode == FWD_3TO1 && max77968->byp_mode == PTM_3TO1) ||
			(mode == FWD_2TO1 && max77968->byp_mode == PTM_2TO1) ||
			(mode == FWD_1TO1 && max77968->byp_mode == PTM_1TO1)) {
			// Do nothing, conditions are satisfied
			ret = 0;
		} else {
			pr_err("%s: SCC mode not match, SCC_OPERATION_MODE=%d, chg_mode=%d, byp_mode=%d\n",
				__func__, mode, max77968->chg_mode, max77968->byp_mode);
			ret = -EINVAL;
		}

	} else {
		/* max77968 is not in 2:1 switching mode - standby or shutdown state */
		/* Stop charging in timer_work */
		u16 adc_val[ADC_READ_MAX];

		rt = max77968_bulk_read_reg(max77968, INT_SRC1_REG, &val[REG_INT1], REG_INT_MAX);
		if (rt < 0)
			goto error;

		/* REG_INT2 has been cleared in previous reading */
		if (max77968->charging_state == DC_STATE_CHECK_ACTIVE)
			val[REG_INT2] = src2_reg;

		pr_err("%s: Error reg[0x02]=0x%x,[0x03]=0x%x,[0x04]=0x%x,[0x05]=0x%x,[0x06]=0x%x\n",
			__func__, val[0], val[1], val[2], val[3], val[4]);

		// Watch dog timer expired, SCC return to shutdown state
		if (val[REG_INT5] & WT_INT) {
			pr_err("%s: SCC Watchdog expired\n", __func__);
			ret = -ERROR_WT_EXPIRED;
			goto error;
		}

		if (val[REG_INT1] & VIN_OVP_INT) {
			strcat(chr_buf, "VIN_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & VIN_UVLO_INT) {
			strcat(chr_buf, "VIN_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & VOUT_OVP_INT) {
			strcat(chr_buf, "VOUT_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & VOUT_UVLO_INT) {
			strcat(chr_buf, "VOUT_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & RVSBST_UVLO_INT) {
			strcat(chr_buf, "RVSBST_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & VBATT_OVP_INT) {
			strcat(chr_buf, "VBAT_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & T_SHDN_INT) {
			strcat(chr_buf, "T_SHDN "); /* Thermal Shutdown */
			ret = -EINVAL;
		}

		if (val[REG_INT2] & CHGR_SS_FAULT_INT) {
			strcat(chr_buf, "CHGR_SS_FAULT ");
			ret = -EINVAL;
		}

		if (val[REG_INT2] & CHGR_OCP_INT) {
			strcat(chr_buf, "CHGR_OCP ");
			ret = -EINVAL;
		}

		if (val[REG_INT2] & CHGR_RCP_INT) {
			strcat(chr_buf, "CHGR_RCP ");
			ret = -ERROR_DCRCP;
		}

		if (val[REG_INT3] & REG_TIMEOUT_INT) {
			strcat(chr_buf, "REG_TOUT ");
			ret = -EINVAL;
		}

		if (val[REG_INT3] & IIN_OCP_INT) {
			strcat(chr_buf, "IIN_OCP ");
			ret = -EINVAL;
		}

		if (val[REG_INT3] & IIN_UCP_INT) {
			strcat(chr_buf, "IIN_UCP ");
			value.intval = true;
			psy_do_property(max77968->pdata->sec_dc_name, set,
				POWER_SUPPLY_EXT_PROP_DC_IBUSUCP, value);
			ret = -ERROR_DCUCP;
		}

		if (val[REG_INT3] & VIN_SHORT_INT) {
			strcat(chr_buf, "VIN_SHORT ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & VEXT1_OVP_INT) {
			strcat(chr_buf, "VEXT1_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & VEXT1_UVLO_INT) {
			strcat(chr_buf, "VEXT1_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & EXT1_SW_OPEN_INT) {
			strcat(chr_buf, "EXT1_SW_OPEN ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & VEXT2_OVP_INT) {
			strcat(chr_buf, "VEXT2_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & VEXT2_UVLO_INT) {
			strcat(chr_buf, "VEXT2_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & EXT2_SW_OPEN_INT) {
			strcat(chr_buf, "EXT2_SW_OPEN ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & PVDD_UVP_INT) {
			strcat(chr_buf, "PVDD_UVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & CFLY_OPEN_INT) {
			strcat(chr_buf, "CFLY_OPEN ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & CFLY_SHORT_INT) {
			strcat(chr_buf, "CFLY_SHORT ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & BST_UVP_INT) {
			strcat(chr_buf, "BST_UVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & NTC_OT_INT) {
			strcat(chr_buf, "NTC_OT ");
			ret = -EINVAL;
		}

		if ((max77968->charging_state == DC_STATE_CHECK_ACTIVE) && (!(scc_en_reg & SCC_STANDBY_MODE_SET))) {
			strcat(chr_buf, "UNKNOWN_RESET");
			/* retry */
			ret = -EINVAL;
		}

		if (ret < 0) {
			pr_err("%s: %s\n", __func__, chr_buf);
		} else {
			pr_err("%s: Power state error\n", __func__);	/* Power State error */
			/* Check charging state - Only retry in check active state */
			if (max77968->charging_state == DC_STATE_CHECK_ACTIVE)
				ret = -EINVAL;
		}

		/* Read ADC register for debugging */
		for (i = 0; i < ADC_READ_MAX; i++)
			adc_val[i] = max77968_read_adc(max77968, i);

		pr_info("%s: adc VIN=0x%x,PMID=0x%x,VEXT1=0x%x,VEXT2=0x%x,VOUT=0x%x,VBATT=0x%x\n",
				__func__, adc_val[0], adc_val[1], adc_val[2], adc_val[3], adc_val[4], adc_val[5]);
		pr_info("%s: adc NTC=0x%x,TDIE=0x%x,IIN=0x%x\n",
				__func__, adc_val[6], adc_val[7], adc_val[8]);
	}

error:
	/* Check RCP DONE case */
	if (ret == -ERROR_DCRCP) {
		/* Check DC state first */
		if ((max77968->charging_state == DC_STATE_START_CV) ||
			(max77968->charging_state == DC_STATE_CV_MODE)) {
			/* Now present state is start_cv or cv_mode */
			/* Compare VBAT_ADC with Vfloat threshold */
			if (max77968->prev_vbat > max77968->vfloat) {
				/* Keep RCP DONE error */
				pr_info("%s: Keep RCP_DONE error type(%d)\n",
						__func__, ret);
			} else {
				/* Overwrite error type to -EINVAL */
				ret = -EINVAL;
				pr_info("%s: Overwrite RCP_DONE error, prev_vbat=%duV\n",
						__func__, max77968->prev_vbat);
			}
		} else {
			/* Keep ret = -ERROR_DCRCP in DC_STATE_CHECK_ACTIVE */
			if (max77968->charging_state != DC_STATE_CHECK_ACTIVE) {
				ret = -EINVAL;
				pr_info("%s: Overwrite ERROR_DCRCP error, charging_state=%d\n",
						__func__, max77968->charging_state);
			}
		}
	} else if (ret == -ERROR_DCUCP) {
		/* Keep ret = -ERROR_DCUCP in DC_STATE_CHECK_ACTIVE */
		if (max77968->charging_state != DC_STATE_CHECK_ACTIVE) {
			ret = -EINVAL;
			pr_info("%s: Overwrite ERROR_DCUCP error, charging_state=%d\n",
					__func__, max77968->charging_state);
		}
	}
	max77968_dc_error_print(max77968, val);
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}


/* Check Reverse active status */
static int max77968_check_reverse_error(struct max77968_charger *max77968)
{
	int ret;
	unsigned int reg_val, mode;
	int rt, i;
	char chr_buf[200] = "\0";

	ret = max77968_read_reg(max77968, SCC_EN_REG, &reg_val);
	if (ret < 0)
		goto error;

	mode = reg_val & SCC_OPERATION_MODE;

	/* Check Active status */
	if ((mode == RVS_1TO1) || (mode == RVS_1TO2) || (mode == RVS_1TO3)) {
		/* MAX77968 is in 1:1/1:2/1:3 reverse mode */
		/* Check temperature regulation loop */
		ret = max77968_read_reg(max77968, STATUS2_REG, &reg_val);
		if (reg_val & TEMP_REG_S) {
			/* Thermal regulation happened */
			pr_err("%s: Device is in temperature regulation\n", __func__);
			ret = -EINVAL;
		} else {
			/* Normal temperature */
			ret = 0;
		}

		if ((mode == RVS_1TO3 && max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_1TO3) ||
			(mode == RVS_1TO2 && max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_1TO2) ||
			(mode == RVS_1TO1 && max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_BYP)) {
			// Do nothing, conditions are satisfied
			ret = 0;
		} else {
			pr_err("%s: SCC mode not match, SCC_OPERATION_MODE=%d, rev_mode=%d\n",
				__func__, mode, max77968->rev_mode);
			ret = -EINVAL;
			goto error;
		}

	} else {
		/* MAX77968 is not in 1:1/1:2/1:3 reverse modee - standby or shutdown state */
		/* Stop charging in timer_work */
		u8 val[REG_INT_MAX];
		u16 adc_val[ADC_READ_MAX];

		rt = max77968_bulk_read_reg(max77968, INT_SRC1_REG, &val[REG_INT1], REG_INT_MAX);
		if (rt < 0)
			goto error;

		pr_err("%s: Error reg[0x02]=0x%x,[0x03]=0x%x,[0x04]=0x%x,[0x05]=0x%x,[0x06]=0x%x\n",
				__func__, val[0], val[1], val[2], val[3], val[4]);

		// Watch dog timer expired, SCC return to shutdown state
		// #define ERROR_WT_EXPIRED    100
		if (val[REG_INT5] & WT_INT) {
			pr_err("%s: SCC Watchdog expired\n", __func__);
			ret = -ERROR_WT_EXPIRED;
			goto error;
		}

		if (val[REG_INT1] & VIN_OVP_INT) {
			strcat(chr_buf, "VIN_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & VIN_UVLO_INT) {
			strcat(chr_buf, "VIN_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & VOUT_OVP_INT) {
			strcat(chr_buf, "VOUT_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & VOUT_UVLO_INT) {
			strcat(chr_buf, "VOUT_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & T_SHDN_INT) {
			strcat(chr_buf, "T_SHDN ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & RVSBST_UVLO_INT) {
			strcat(chr_buf, "RVSBST_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT1] & VBATT_OVP_INT) {
			strcat(chr_buf, "VBAT_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT2] & RVSBST_SS_FAULT_INT) {
			strcat(chr_buf, "RVSBST_SS_FAULT ");
			ret = -EINVAL;
		}

		if (val[REG_INT2] & RVSBST_OCP_INT) {
			strcat(chr_buf, "RVSBST_OCP ");
			ret = -EINVAL;
		}

		if (val[REG_INT3] & IIN_OCP_INT) {
			strcat(chr_buf, "IIN_OCP ");
			ret = -EINVAL;
		}

		if (val[REG_INT3] & VIN_SHORT_INT) {
			strcat(chr_buf, "VIN_SHORT ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & VEXT1_OVP_INT) {
			strcat(chr_buf, "VEXT1_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & VEXT1_UVLO_INT) {
			strcat(chr_buf, "VEXT1_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & EXT1_SW_OPEN_INT) {
			strcat(chr_buf, "EXT1_SW_OPEN ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & VEXT2_OVP_INT) {
			strcat(chr_buf, "VEXT2_OVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & VEXT2_UVLO_INT) {
			strcat(chr_buf, "VEXT2_UVLO ");
			ret = -EINVAL;
		}

		if (val[REG_INT4] & EXT2_SW_OPEN_INT) {
			strcat(chr_buf, "EXT2_SW_OPEN ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & CFLY_OPEN_INT) {
			strcat(chr_buf, "CFLY_OPEN ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & CFLY_SHORT_INT) {
			strcat(chr_buf, "CFLY_SHORT ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & BST_UVP_INT) {
			strcat(chr_buf, "BST_UVP ");
			ret = -EINVAL;
		}

		if (val[REG_INT5] & NTC_OT_INT) {
			strcat(chr_buf, "NTC_OT ");
			ret = -EINVAL;
		}

		if (ret < 0) {
			pr_err("%s: %s\n", __func__, chr_buf);
		} else {
			pr_err("%s: Power state error\n", __func__);	/* Power State error */
			ret = -EAGAIN;	/* retry */
		}

		/* Read ADC register for debugging */
		for (i = 0; i < ADC_READ_MAX; i++)
			adc_val[i] = max77968_read_adc(max77968, i);

		pr_info("%s: adc VIN=0x%x,PMID=0x%x,VEXT1=0x%x,VEXT2=0x%x,VOUT=0x%x,VBATT=0x%x\n",
				__func__, adc_val[0], adc_val[1], adc_val[2], adc_val[3], adc_val[4], adc_val[5]);
		pr_info("%s: adc NTC=0x%x,TDIE=0x%x,IIN=0x%x\n",
				__func__, adc_val[6], adc_val[7], adc_val[8]);
	}

error:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	if (ret == -EINVAL) {
		max77968->chg_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		max77968->health_status = POWER_SUPPLY_EXT_HEALTH_DC_ERR;
	}
#endif
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}

/* Check DC Mode status */
static int max77968_check_dcmode_status(struct max77968_charger *max77968)
{
	unsigned int reg_val;
	int ret, i;
	int vbatt;

	/* Read CHARGING_STS */
	ret = max77968_read_reg(max77968, STATUS2_REG, &reg_val);
	if (ret < 0)
		goto error;

	/* Read battery voltage from fuelgauge */
	vbatt = max77968_read_adc(max77968, ADC_CH_VBATT);

	if (vbatt < 0) {
		ret = vbatt;
		goto error;
	}

	if (reg_val & TEMP_REG_S)
		pr_info("%s: Device is in temperature regulation\n", __func__);

	/* Check CHARGING_STS */
	if (vbatt > max77968->vfloat) {
		ret = DCMODE_VFLT_LOOP;
		pr_info("%s: FG Vnow=%d  > %d, FG Vfloat\n", __func__, vbatt, max77968->vfloat);
	} else if (reg_val & VBATT_REG_S) {
		pr_info("%s: DCMODE_VFLT_LOOP\n", __func__);
		ret = DCMODE_VFLT_LOOP;
	} else if (reg_val & IIN_REG_S) {
		pr_info("%s: DCMODE_IIN_LOOP\n", __func__);
		ret = DCMODE_IIN_LOOP;
		/* Check IIN_LOOP again to avoid unstable IIN_LOOP period */
		for (i = 0; i < 4; i++) {
			/* Wait 2ms */
			usleep_range(2000, 3000);
			/* Read CHARGING_STS again */
			ret = max77968_read_reg(max77968, STATUS2_REG, &reg_val);
			if (ret < 0)
				goto error;
			/* Overwrite status */
			ret = DCMODE_IIN_LOOP;
			/* Check CHARGING_STS again */
			if ((reg_val & IIN_REG_S) != IIN_REG_S) {
				/* Now max77968 is in unstable IIN_LOOP period */
				/* Ignore IIN_LOOP status */
				pr_info("%s: Unstable IIN_LOOP\n", __func__);
				ret = DCMODE_LOOP_INACTIVE;
				break;
			}
		}
	} else {
		pr_info("%s: DCMODE_LOOP_INACTIVE\n", __func__);
		ret = DCMODE_LOOP_INACTIVE;
	}

error:
	pr_info("%s: DCMODE Status=%d\n", __func__, ret);
	return ret;
}

/* Stop Charging */
static int max77968_stop_charging(struct max77968_charger *max77968)
{
	int ret = 0;

	if (max77968->enable == true)
		pr_info("%s : Enabled : %s\n", __func__, charging_state_str[max77968->charging_state]);
	else
		pr_info("%s : Disabled : %s\n", __func__, charging_state_str[max77968->charging_state]);

	/* Check the current state and max77968 enable status */
	if ((max77968->charging_state != DC_STATE_NO_CHARGING) ||
		(max77968->enable == true)) {
		/* Recover switching charger ICL */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968_set_switching_charger(max77968, true);
#else
		ret = max77968_set_switching_charger(true, SWCHG_ICL_NORMAL,
											max77968->ichg_cfg,
											max77968->vfloat);
#endif
		if (ret < 0) {
			pr_err("%s: Error-set_switching charger ICL\n", __func__);
			goto error;
		}

		/* Stop Direct charging */
		cancel_delayed_work(&max77968->timer_work);
		cancel_delayed_work(&max77968->pps_work);
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_ID_NONE;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);

		/* Clear parameter */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968_set_charging_state(max77968, DC_STATE_NO_CHARGING);
		max77968_init_adc_val(max77968, -1);
#else
		max77968->charging_state = DC_STATE_NO_CHARGING;
#endif
		max77968->ret_state = DC_STATE_NO_CHARGING;
		max77968->ta_target_vol = TA_MAX_VOL;
		max77968->prev_iin = 0;
		max77968->prev_inc = INC_NONE;
		mutex_lock(&max77968->lock);
		max77968->req_new_iin = false;
		max77968->new_iin_buf_has_data = false;
		max77968->req_new_vfloat = false;
		mutex_unlock(&max77968->lock);
		max77968->ta_ctrl = TA_CTRL_CL_MODE;

		/* Clear new BYP mode and BYP mode */
		max77968->new_byp_mode = PTM_NONE;
		max77968->byp_mode = PTM_NONE;
		max77968->req_new_byp_mode = false;

		/* Clear charge mode and new charge mode */
		max77968->new_chg_mode = CHG_NO_DC_MODE;
		max77968->chg_mode = CHG_NO_DC_MODE;
		max77968->new_chg_mode_busy_buf = CHG_NO_DC_MODE;
		mutex_lock(&max77968->lock);
		max77968->req_new_chg_mode = false;
		max77968->new_chg_mode_buf_has_data = false;
		mutex_unlock(&max77968->lock);

		/* Set vfloat decrement flag to false */
		max77968->dec_vfloat = false;

		/* Clear reverse mode */
		max77968->rev_mode = POWER_SUPPLY_DC_REVERSE_STOP;
		max77968->iin_rev = 0;

		/* Clear previous VBAT_ADC */
		max77968->prev_vbat = 0;

		/* Clear charging done counter */
		max77968->done_cnt = 0;

		/* Clear retry counters and flags */
		max77968->ss_fault_retry_cnt = 0;
		max77968->ss_fault_inc_ta_volt = false;
		max77968->preset_ta_fault_retry_cnt = 0;
		max77968->preset_ta_fault_inc_ta_volt = false;
		max77968->preset_ta_vol_dec_once = false;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		/* Set watchdog timer disable */
		max77968_set_wdt_enable(max77968, WDT_DISABLE);
		cancel_delayed_work(&max77968->wdt_control_work);
#endif
		ret = max77968_set_charging(max77968, false);
		if (ret < 0) {
			pr_err("%s: Error-set_charging(main)\n", __func__);
			goto error;
		}

		ret = max77968_set_standby_state(max77968, false);
		if (ret < 0) {
			pr_err("%s: Error-set_standby_state(false)\n", __func__);
			goto error;
		}
	}

error:
	__pm_relax(max77968->monitor_wake_lock);
	pr_info("%s: END, ret=%d\n", __func__, ret);

	return ret;
}

/* Compensate TA current for the target input current */
static int max77968_set_ta_current_comp(struct max77968_charger *max77968)
{
	int iin;

	/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
	if (max77968->adc_wr_en)
		iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
	else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
	iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif

	pr_info("%s: iin=%d\n", __func__, iin);

	/* Compare IIN ADC with target input current */
	if (iin > max77968->iin_cc) {
		/* TA current is higher than the target input current */
		/* Compare TA current with IIN_CC - LOW_OFFSET */
		if (max77968->ta_cur > max77968->iin_cc - TA_CUR_LOW_OFFSET) {
			/* TA current is higher than IIN_CC - LOW_OFFSET */
			/* Assume that TA operation mode is CL mode, so decrease TA current */
			/* Decrease TA current (50mA) */
			max77968->ta_cur = max77968->ta_cur - PD_MSG_TA_CUR_STEP;
			pr_info("%s: Comp. Cont1: ta_cur=%d\n", __func__, max77968->ta_cur);
		} else {
			/* TA current is already lower than IIN_CC - LOW_OFFSET */
			/* IIN_ADC is stiil in invalid range even though TA current is less than IIN_CC - LOW_OFFSET */
			/* TA has abnormal behavior */
			/* Decrease TA voltage (20mV) */
			max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
			pr_info("%s: Comp. Cont2: ta_vol=%d\n", __func__, max77968->ta_vol);
			/* Update TA target voltage */
			max77968->ta_target_vol = max77968->ta_vol;
		}
		/* Send PD Message */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PDMSG_SEND;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
	} else {
		if (iin < (max77968->iin_cc - IIN_CC_COMP_OFFSET)) {
			/* TA current is lower than the target input current */
			/* Compare present TA voltage with TA maximum voltage first */
			if (max77968->ta_vol == max77968->ta_max_vol) {
				/* TA voltage is already the maximum voltage */
				/* Compare present TA current with TA maximum current */
				if (max77968->ta_cur == max77968->ta_max_cur) {
					/* Both of present TA voltage and current are already maximum values */
					pr_info("%s: Comp. End1(Max value): ta_vol=%d, ta_cur=%d\n",
						__func__, max77968->ta_vol, max77968->ta_cur);
					/* Set timer */
					mutex_lock(&max77968->lock);
					max77968->timer_id = TIMER_CHECK_CCMODE;
					max77968->timer_period = CCMODE_CHECK_T;
					mutex_unlock(&max77968->lock);
				} else {
					/* TA voltage is maximum voltage, but TA current is not maximum current */
					/* Increase TA current (50mA) */
					max77968->ta_cur = max77968->ta_cur + PD_MSG_TA_CUR_STEP;
					if (max77968->ta_cur > max77968->ta_max_cur)
						max77968->ta_cur = max77968->ta_max_cur;
					pr_info("%s: Comp. Cont3: ta_cur=%d\n", __func__, max77968->ta_cur);
					/* Send PD Message */
					mutex_lock(&max77968->lock);
					max77968->timer_id = TIMER_PDMSG_SEND;
					max77968->timer_period = 0;
					mutex_unlock(&max77968->lock);

					/* Set TA increment flag */
					max77968->prev_inc = INC_TA_CUR;
				}
			} else {
				/* TA voltage is not maximum voltage */
				/* Compare IIN ADC with previous IIN ADC + 20mA */
				if (iin > (max77968->prev_iin + IIN_ADC_OFFSET)) {
					/* In this case, TA voltage is not enough to supply */
					/* the operating current of RDO. So, increase TA voltage */
					/* Increase TA voltage (20mV) */
					max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
					if (max77968->ta_vol > max77968->ta_max_vol)
						max77968->ta_vol = max77968->ta_max_vol;
					pr_info("%s: Comp. Cont4: ta_vol=%d\n",
						__func__, max77968->ta_vol);
					/* Update TA target voltage */
					max77968->ta_target_vol = max77968->ta_vol;
					/* Send PD Message */
					mutex_lock(&max77968->lock);
					max77968->timer_id = TIMER_PDMSG_SEND;
					max77968->timer_period = 0;
					mutex_unlock(&max77968->lock);

					/* Set TA increment flag */
					max77968->prev_inc = INC_TA_VOL;
				} else {
					/* Input current increment is too low */
					/* It is possible that TA is in current limit mode or has low TA voltage */
					/* Increase TA current or voltage */
					/* Check the previous TA increment */
					if (max77968->prev_inc == INC_TA_VOL) {
						/* The previous increment is TA voltage, but input current does not increase */
						/* Try to increase TA current */
						/* Compare present TA current with TA maximum current */
						if (max77968->ta_cur == max77968->ta_max_cur) {
							/* TA current is already the maximum current */

							/* Increase TA voltage (20mV) */
							max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
							if (max77968->ta_vol > max77968->ta_max_vol)
								max77968->ta_vol = max77968->ta_max_vol;
							pr_info("%s: Comp. Cont5: ta_vol=%d\n",
								__func__, max77968->ta_vol);
							/* Update TA target voltage */
							max77968->ta_target_vol = max77968->ta_vol;
							/* Send PD Message */
							mutex_lock(&max77968->lock);
							max77968->timer_id = TIMER_PDMSG_SEND;
							max77968->timer_period = 0;
							mutex_unlock(&max77968->lock);

							/* Set TA increment flag */
							max77968->prev_inc = INC_TA_VOL;
						} else {
							/* Check the present TA current */
							/* Consider tolerance offset(100mA) */
							if (max77968->ta_cur >= (max77968->iin_cc + TA_IIN_OFFSET)) {
								/* Maybe TA supply current is enough, but TA voltage is low */
								/* Increase TA voltage (20mV) */
								max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
								if (max77968->ta_vol > max77968->ta_max_vol)
									max77968->ta_vol = max77968->ta_max_vol;
								pr_info("%s: Comp. Cont6: ta_vol=%d\n",
									__func__, max77968->ta_vol);
								/* Update TA target voltage */
								max77968->ta_target_vol = max77968->ta_vol;
								/* Send PD Message */
								mutex_lock(&max77968->lock);
								max77968->timer_id = TIMER_PDMSG_SEND;
								max77968->timer_period = 0;
								mutex_unlock(&max77968->lock);

								/* Set TA increment flag */
								max77968->prev_inc = INC_TA_VOL;
							} else {
								/* It is possible that TA is in current limit mode */
								/* Increase TA current (50mA) */
								max77968->ta_cur = max77968->ta_cur + PD_MSG_TA_CUR_STEP;
								if (max77968->ta_cur > max77968->ta_max_cur)
									max77968->ta_cur = max77968->ta_max_cur;
								pr_info("%s: Comp. Cont7: ta_cur=%d\n",
									__func__, max77968->ta_cur);
								/* Send PD Message */
								mutex_lock(&max77968->lock);
								max77968->timer_id = TIMER_PDMSG_SEND;
								max77968->timer_period = 0;
								mutex_unlock(&max77968->lock);

								/* Set TA increment flag */
								max77968->prev_inc = INC_TA_CUR;
							}
						}
					} else {
						/* The previous increment is TA current, but input current does not increase */
						/* Try to increase TA voltage */
						/* Increase TA voltage (20mV) */
						max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
						if (max77968->ta_vol > max77968->ta_max_vol)
							max77968->ta_vol = max77968->ta_max_vol;
						pr_info("%s: Comp. Cont8: ta_vol=%d\n",
							__func__, max77968->ta_vol);
						/* Update TA target voltage */
						max77968->ta_target_vol = max77968->ta_vol;
						/* Send PD Message */
						mutex_lock(&max77968->lock);
						max77968->timer_id = TIMER_PDMSG_SEND;
						max77968->timer_period = 0;
						mutex_unlock(&max77968->lock);

						/* Set TA increment flag */
						max77968->prev_inc = INC_TA_VOL;
					}
				}
			}
		} else {
			/* IIN ADC is in valid range */
			/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA  */
			pr_info("%s: Comp. End2(valid): ta_vol=%d, ta_cur=%d\n",
				__func__, max77968->ta_vol, max77968->ta_cur);
			/* Clear TA increment flag */
			max77968->prev_inc = INC_NONE;
			/* Set timer */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_CCMODE;
			max77968->timer_period = CCMODE_CHECK_T;
			mutex_unlock(&max77968->lock);
		}
	}

	/* Save previous iin adc */
	max77968->prev_iin = iin;

	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));

	return 0;
}

/* Compensate TA current for constant power mode */
static int max77968_set_ta_current_comp2(struct max77968_charger *max77968)
{
	int iin;
	unsigned int val;
	unsigned int iin_apdo;

	/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
	if (max77968->adc_wr_en)
		iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
	else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
	iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif

	pr_info("%s: iin=%d\n", __func__, iin);

	/* Compare IIN ADC with target input current */
	if (iin > max77968->iin_cfg) {
		/* TA current is higher than the target input current */
		/* Compare TA current with IIN_CC - LOW_OFFSET */
		if (max77968->ta_cur > max77968->iin_cc - TA_CUR_LOW_OFFSET) {
			/* TA current is higher than IIN_CC - LOW_OFFSET */
			/* Assume that TA operation mode is CL mode, so decrease TA current */
			/* Decrease TA current (50mA) */
			max77968->ta_cur = max77968->ta_cur - PD_MSG_TA_CUR_STEP;
			pr_info("%s: Comp. Cont1: ta_cur=%d\n", __func__, max77968->ta_cur);
		} else {
			/* TA current is already lower than IIN_CC - LOW_OFFSET */
			/* IIN_ADC is stiil in invalid range even though TA current is less than IIN_CC - LOW_OFFSET */
			/* TA has abnormal behavior */
			/* Decrease TA voltage (20mV) */
			max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
			pr_info("%s: Comp. Cont2: ta_vol=%d\n", __func__, max77968->ta_vol);
			/* Update TA target voltage */
			max77968->ta_target_vol = max77968->ta_vol;
		}

		/* Send PD Message */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PDMSG_SEND;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
	} else if (iin < (max77968->iin_cc - IIN_CC_COMP_OFFSET_CP)) {
		/* TA current is lower than the target input current */
		/* IIN_ADC < IIN_CC -20mA */
		if (max77968->ta_vol == max77968->ta_max_vol) {
			/* Check IIN_ADC < IIN_CC - 50mA */
			if (iin < (max77968->iin_cc - IIN_CC_COMP_OFFSET)) {
				/* Compare the TA current with IIN_CC and maximum current of APDO */
				if ((max77968->ta_cur >= (max77968->iin_cc/max77968->chg_mode)) ||
					(max77968->ta_cur == max77968->ta_max_cur)) {
					/* TA current is higher than IIN_CC or maximum TA current */
					/* Set new IIN_CC to IIN_CC - 50mA */
					max77968->iin_cc = max77968->iin_cc - IIN_CC_DEC_STEP;
					/* Set new TA_MAX_VOL to TA_MAX_PWR/IIN_CC */
					/* Adjust new IIN_CC with APDO resolution */
					iin_apdo = max77968->iin_cc/PD_MSG_TA_CUR_STEP;
					iin_apdo = iin_apdo*PD_MSG_TA_CUR_STEP;
					val = max77968->ta_max_pwr/(iin_apdo / max77968->chg_mode / 1000);	/* mV */
					val = val*1000/PD_MSG_TA_VOL_STEP;	/* Adjust values with APDO resolution(20mV) */
					val = val*PD_MSG_TA_VOL_STEP; /* uV */
					/* Set new TA_MAX_VOL */
					max77968->ta_max_vol = MIN(val, TA_MAX_VOL * max77968->chg_mode);
					/* Increase TA voltage(40mV) */
					max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP*2;
					if (max77968->ta_vol > max77968->ta_max_vol)
						max77968->ta_vol = max77968->ta_max_vol;
					pr_info("%s: Comp. Cont2: ta_vol=%d\n", __func__, max77968->ta_vol);
					/* Update TA target voltage */
					max77968->ta_target_vol = max77968->ta_vol;

					/* Send PD Message */
					mutex_lock(&max77968->lock);
					max77968->timer_id = TIMER_PDMSG_SEND;
					max77968->timer_period = 0;
					mutex_unlock(&max77968->lock);
				} else {
					/* TA current is less than IIN_CC and not maximum current */
					/* Increase TA current (50mA) */
					max77968->ta_cur = max77968->ta_cur + PD_MSG_TA_CUR_STEP;
					if (max77968->ta_cur > max77968->ta_max_cur)
						max77968->ta_cur = max77968->ta_max_cur;
					pr_info("%s: Comp. Cont3: ta_cur=%d\n", __func__, max77968->ta_cur);

					/* Send PD Message */
					mutex_lock(&max77968->lock);
					max77968->timer_id = TIMER_PDMSG_SEND;
					max77968->timer_period = 0;
					mutex_unlock(&max77968->lock);
				}
			} else {
				/* Wait for next current step compensation */
				/* IIN_CC - 50mA < IIN ADC < IIN_CC - 20mA */
				pr_info("%s: Comp.(wait): ta_vol=%d\n", __func__, max77968->ta_vol);
				/* Set timer */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_CCMODE;
				max77968->timer_period = CCMODE_CHECK_T;
				mutex_unlock(&max77968->lock);
			}
		} else {
			/* Increase TA voltage(40mV) */
			max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP*2;
			if (max77968->ta_vol > max77968->ta_max_vol)
				max77968->ta_vol = max77968->ta_max_vol;
			pr_info("%s: Comp. Cont4: ta_vol=%d\n", __func__, max77968->ta_vol);
			/* Update TA target voltage */
			max77968->ta_target_vol = max77968->ta_vol;

			/* Send PD Message */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
		}
	} else {
		/* IIN ADC is in valid range */
		/* IIN_CC - 20mA < IIN ADC < IIN_CFG + 50mA */
		pr_info("%s: Comp. End(valid): ta_vol=%d\n", __func__, max77968->ta_vol);
		/* Set timer */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_CHECK_CCMODE;
		max77968->timer_period = CCMODE_CHECK_T;
		mutex_unlock(&max77968->lock);
	}

	/* Save previous iin adc */
	max77968->prev_iin = iin;

	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));

	return 0;
}

/* Compensate TA voltage for the target input current */
static int max77968_set_ta_voltage_comp(struct max77968_charger *max77968)
{
	int iin;

	pr_info("%s: ======START=======\n", __func__);

	/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
	if (max77968->adc_wr_en)
		iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
	else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
	iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
	pr_info("%s: iin=%d\n", __func__, iin);

	/* Compare IIN ADC with target input current */
	if (iin > max77968->iin_cc) {
		/* TA current is higher than the target input current */
		/* Decrease TA voltage (20mV) */
		max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
		pr_info("%s: Comp. Cont1: ta_vol=%d\n", __func__, max77968->ta_vol);

		/* Send PD Message */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PDMSG_SEND;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
	} else {
		if (iin < (max77968->iin_cc - IIN_CC_COMP_OFFSET)) {
			/* TA current is lower than the target input current */
			/* Compare TA max voltage */
			if (max77968->ta_vol == max77968->ta_max_vol) {
				/* TA current is already the maximum voltage */
				pr_info("%s: Comp. End1(max TA vol): ta_vol=%d\n", __func__, max77968->ta_vol);
				/* Set timer */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_CCMODE;
				max77968->timer_period = CCMODE_CHECK_T;
				mutex_unlock(&max77968->lock);
			} else {
				/* Increase TA voltage (20mV) */
				max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
				pr_info("%s: Comp. Cont2: ta_vol=%d\n", __func__, max77968->ta_vol);

				/* Send PD Message */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PDMSG_SEND;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
			}
		} else {
			/* IIN ADC is in valid range */
			/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA  */
			pr_info("%s: Comp. End(valid): ta_vol=%d\n", __func__, max77968->ta_vol);
			/* Set timer */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_CCMODE;
			max77968->timer_period = CCMODE_CHECK_T;
			mutex_unlock(&max77968->lock);
		}
	}

	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));

	return 0;
}

/* Compensate RX voltage for the target input current */
static int max77968_set_rx_voltage_comp(struct max77968_charger *max77968)
{
	int iin;

	pr_info("%s: ======START=======\n", __func__);

	/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
	if (max77968->adc_wr_en)
		iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
	else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
	iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif

	pr_info("%s: iin=%d\n", __func__, iin);

	/* Compare IIN ADC with target input current */
	if (iin > max77968->iin_cc) {
		/* RX current is higher than the target input current */
		/* Decrease RX voltage (12.5mV) */
		max77968->ta_vol = max77968->ta_vol - WCRX_VOL_STEP;
		pr_info("%s: Comp. Cont1: rx_vol=%d\n", __func__, max77968->ta_vol);

		/* Set RX Voltage */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PDMSG_SEND;
		max77968->timer_period = RXMSG_WAIT_T;
		mutex_unlock(&max77968->lock);
	} else {
		//if (iin < (max77968->iin_cc - IIN_CC_COMP_OFFSET)) {
		if (iin < (max77968->iin_cc - IIN_CC_COMP_DOWN_OFFSET)) {
			/* RX current is lower than the target input current */
			/* Compare RX max voltage */
			if (max77968->ta_vol == max77968->ta_max_vol) {
				/* TA current is already the maximum voltage */
				pr_info("%s: Comp. End1(max RX vol): rx_vol=%d\n", __func__, max77968->ta_vol);
				/* Set timer */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_CCMODE;
				max77968->timer_period = CCMODE_CHECK_T;
				mutex_unlock(&max77968->lock);
			} else {
				/* Increase RX voltage (12.5mV) */
				max77968->ta_vol = max77968->ta_vol + WCRX_VOL_STEP;
				pr_info("%s: Comp. Cont2: rx_vol=%d\n", __func__, max77968->ta_vol);

				/* Set RX Voltage */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PDMSG_SEND;
				max77968->timer_period = RXMSG_WAIT_T;
				mutex_unlock(&max77968->lock);
			}
		} else {
			/* IIN ADC is in valid range */
			/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA  */
			pr_info("%s: Comp. End(valid): rx_vol=%d\n", __func__, max77968->ta_vol);
			/* Set timer */
			/* Check the current charging state */
			/* Set timer */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_CCMODE;
			max77968->timer_period = CCMODE_CHECK_T;
			mutex_unlock(&max77968->lock);
		}
	}

	/* Set TA target voltage to TA voltage */
	max77968->ta_target_vol = max77968->ta_vol;

	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));

	return 0;
}

/* Set TA current for target current */
static int max77968_adjust_ta_current(struct max77968_charger *max77968)
{
	int ret = 0;
	int vbat;
	unsigned int val;

	pr_info("%s: ======START=======\n", __func__);

	/* Set charging state to ADJUST_TACUR */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_ADJUST_TACUR);
#else
	max77968->charging_state = DC_STATE_ADJUST_TACUR;
#endif

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		ret = max77968_vbatt_regu_enable_chk(max77968);
		if (ret < 0)
			goto error;
	}

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	/* Check whether TA current is same as IIN_CC or not */
	if (max77968->ta_cur == max77968->iin_cc) {
		/* finish sending PD message */
		/* Recover IIN_CC to the original value(new_iin) */
		max77968->iin_cc = max77968->new_iin;

		/* Update iin_cfg */
		max77968->iin_cfg = max77968->iin_cc;
		/* Set IIN_CFG to new IIN */
		ret = max77968_set_input_current(max77968, max77968->iin_cc);
		if (ret < 0)
			goto error;

		/* Clear req_new_iin */
		mutex_lock(&max77968->lock);
		max77968->req_new_iin = false;
		mutex_unlock(&max77968->lock);

		pr_info("%s: adj. End, ta_cur=%d, ta_vol=%d, iin_cc=%d, chg_mode=%d\n",
				__func__, max77968->ta_cur, max77968->ta_vol, max77968->iin_cc, max77968->chg_mode);

		/* Go to return state  */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968_set_charging_state(max77968, max77968->ret_state);
#else
		max77968->charging_state = max77968->ret_state;
#endif
		/* Set timer */
		mutex_lock(&max77968->lock);
		if (max77968->charging_state == DC_STATE_CC_MODE)
			max77968->timer_id = TIMER_CHECK_CCMODE;
		else
			max77968->timer_id = TIMER_CHECK_CVMODE;
		max77968->timer_period = 1000;	/* Wait 1s */
		mutex_unlock(&max77968->lock);
	} else {
		/* Compare new IIN with current IIN_CFG */
		if (max77968->iin_cc > max77968->iin_cfg) {
			/* New iin is higher than current iin_cc(iin_cfg) */
			/* Compare new IIN with IIN_LOW_TH */
			/* New IIN is high current */
			/* Update iin_cfg */
			max77968->iin_cfg = max77968->iin_cc;
			/* Set IIN_CFG to new IIN */
			ret = max77968_set_input_current(max77968, max77968->iin_cc);
			if (ret < 0)
				goto error;

			/* Clear Request flag */
			mutex_lock(&max77968->lock);
			max77968->req_new_iin = false;
			mutex_unlock(&max77968->lock);

			/* Set new TA voltage and current */
			/* Read VBAT ADC */
			vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

			/* Calculate new TA maximum current and voltage that used in the direct charging */
			/* Set IIN_CC to MIN[IIN, TA_MAX_CUR*chg_mode]*/
			max77968->iin_cc = MIN(max77968->iin_cfg, max77968->ta_max_cur);

			/* Set the current IIN_CC to iin_cfg for recovering it after resolution adjustment */
			max77968->iin_cfg = max77968->iin_cc;

			/* Calculate new TA max voltage */
			/* Adjust IIN_CC with APDO resolution(50mA) - It will recover to the original value after max voltage calculation */
			val = max77968->iin_cc / (PD_MSG_TA_CUR_STEP * max77968->chg_mode);
			max77968->iin_cc = val * (PD_MSG_TA_CUR_STEP * max77968->chg_mode);

			/* Set TA_MAX_VOL to MIN[TA_MAX_VOL, (TA_MAX_PWR/IIN_CC)] */
			val = max77968->ta_max_pwr / (max77968->iin_cc / max77968->chg_mode / 1000);	/* mV */
			val = val * 1000 / PD_MSG_TA_VOL_STEP;	/* Adjust values with APDO resolution(20mV) */
			val = val * PD_MSG_TA_VOL_STEP; /* uV */
			max77968->ta_max_vol = MIN(val, TA_MAX_VOL * max77968->chg_mode);

			/* Set TA voltage to MAX[TA_MIN_VOL_PRESET*chg_mode, (VBAT_ADC*chg_mode + offset)] */
			max77968->ta_vol = max(TA_MIN_VOL_PRESET * max77968->chg_mode, (vbat * max77968->chg_mode + TA_VOL_PRE_OFFSET));
			val = max77968->ta_vol / PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
			max77968->ta_vol = val * PD_MSG_TA_VOL_STEP;

			/* Set TA voltage to MIN[TA voltage, TA_MAX_VOL] */
			max77968->ta_vol = MIN(max77968->ta_vol, max77968->ta_max_vol);
			/* Set TA current to IIN_CC */
			max77968->ta_cur = max77968->iin_cc;
			/* Recover IIN_CC to the original value(iin_cfg) */
			max77968->iin_cc = max77968->iin_cfg;

			pr_info("%s: New IIN(1), ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, chg_mode=%d\n",
				__func__, max77968->ta_max_vol, max77968->ta_max_cur, max77968->ta_max_pwr, max77968->iin_cc, max77968->chg_mode);

			pr_info("%s: New IIN(1), ta_vol=%d, ta_cur=%d, sw_freq=%d\n",
				__func__, max77968->ta_vol, max77968->ta_cur, max77968->fsw_cfg);

			/* Clear previous IIN ADC */
			max77968->prev_iin = 0;
			/* Clear TA increment flag */
			max77968->prev_inc = INC_NONE;

			/* Send PD Message and go to Adjust CC mode */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_charging_state(max77968, DC_STATE_ADJUST_CC);
#else
			max77968->charging_state = DC_STATE_ADJUST_CC;
#endif
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			max77968->timer_period = IIN_CFG_WAIT_T;
			mutex_unlock(&max77968->lock);
		} else {
			/* New iin is lower than current iin_cc(iin_cfg) */
			/* Calculate new TA_MAX_VOL */
			/* Adjust IIN_CC with APDO resolution(50mA) - It will recover to the original value after max voltage calculation */
			val = max77968->iin_cc / (PD_MSG_TA_CUR_STEP);
			max77968->iin_cc = val * (PD_MSG_TA_CUR_STEP);
			/* Set TA_MAX_VOL to MIN[TA_MAX_VOL, (TA_MAX_PWR/IIN_CC)] */
			val = max77968->ta_max_pwr / (max77968->iin_cc / max77968->chg_mode / 1000); /* mV */
			val = val * 1000 / PD_MSG_TA_VOL_STEP;	/* Adjust values with APDO resolution(20mV) */
			val = val * PD_MSG_TA_VOL_STEP; /* uV */
			max77968->ta_max_vol = MIN(val, TA_MAX_VOL * max77968->chg_mode);
			/* Recover IIN_CC to the original value(new_iin) */
			max77968->iin_cc = max77968->new_iin;

			/* Set TA voltage to TA target voltage */
			max77968->ta_vol = max77968->ta_target_vol;
			/* Adjust IIN_CC with APDO resolution(50mA) - It will recover to the original value after sending PD message */
			val = max77968->iin_cc / PD_MSG_TA_CUR_STEP;
			max77968->iin_cc = val * PD_MSG_TA_CUR_STEP;
			/* Set TA current to IIN_CC */
			max77968->ta_cur = max77968->iin_cc;

			pr_info("%s: adj. cont1, ta_cur=%d, ta_vol=%d, ta_max_vol=%d, iin_cc=%d, chg_mode=%d\n",
					__func__, max77968->ta_cur, max77968->ta_vol, max77968->ta_max_vol, max77968->iin_cc, max77968->chg_mode);

			/* Send PD Message */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
		} /* if (max77968->iin_cc > max77968->iin_cfg) else */
	} /* if (max77968->ta_cur == max77968->iin_cc/max77968->chg_mode) else */
	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));

error:
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}


/* Set TA voltage for target current */
static int max77968_adjust_ta_voltage(struct max77968_charger *max77968)
{
	int iin, ret, val;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_ADJUST_TAVOL);
#else
	max77968->charging_state = DC_STATE_ADJUST_TAVOL;
#endif

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		ret = max77968_vbatt_regu_enable_chk(max77968);
		if (ret < 0)
			return ret;
	}

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		return ret;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
	if (max77968->adc_wr_en)
		iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
	else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
	iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif

	/* Compare IIN ADC with targer input current */
	if (iin > (max77968->iin_cc + PD_MSG_TA_CUR_STEP)) {
		/* TA current is higher than the target input current */
		/* Decrease TA voltage (20mV) */
		max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;

		pr_info("%s: adj. Cont1, ta_vol=%d\n", __func__, max77968->ta_vol);

		/* Send PD Message */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PDMSG_SEND;
		if (max77968->ta_type == TA_TYPE_WIRELESS)
			max77968->timer_period = RXMSG_WAIT_T;
		else
			max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
	} else {
		if (iin < (max77968->iin_cc - PD_MSG_TA_CUR_STEP)) {
			/* TA current is lower than the target input current */
			/* Compare TA max voltage */
			if (max77968->ta_vol == max77968->ta_max_vol) {
				/* TA current is already the maximum voltage */
				/* Clear req_new_iin */
				mutex_lock(&max77968->lock);
				max77968->req_new_iin = false;
				mutex_unlock(&max77968->lock);
				/* Return charging state to the previous state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
				max77968_set_charging_state(max77968, max77968->ret_state);
#else
				max77968->charging_state = max77968->ret_state;
#endif
				pr_info("%s: adj. End1, ta_cur=%d, ta_vol=%d, iin_cc=%d, chg_mode=%d\n",
						__func__, max77968->ta_cur, max77968->ta_vol, max77968->iin_cc, max77968->chg_mode);

				/* Go to return state  */
				/* Set timer */
				mutex_lock(&max77968->lock);
				if (max77968->charging_state == DC_STATE_CC_MODE)
					max77968->timer_id = TIMER_CHECK_CCMODE;
				else
					max77968->timer_id = TIMER_CHECK_CVMODE;
				max77968->timer_period = 1000;	/* Wait 1000ms */
				mutex_unlock(&max77968->lock);
			} else {
				/* Increase TA voltage (20mV) */
				if (max77968->ta_type == TA_TYPE_WIRELESS)
					max77968->ta_vol = max77968->ta_vol + WCRX_ADJUST_CC_RX_VOL_STEP;
				else
					max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;

				pr_info("%s: adj. Cont2, ta_vol=%d\n", __func__, max77968->ta_vol);

				/* Send PD Message */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PDMSG_SEND;
				if (max77968->ta_type == TA_TYPE_WIRELESS)
					max77968->timer_period = RXMSG_WAIT_T;
				else
					max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
			}
		} else {
			/* IIN ADC is in valid range */
			/* Clear req_new_iin */
			mutex_lock(&max77968->lock);
			max77968->req_new_iin = false;
			mutex_unlock(&max77968->lock);

			/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA  */
			/* Return charging state to the previous state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_charging_state(max77968, max77968->ret_state);
#else
			max77968->charging_state = max77968->ret_state;
#endif
			pr_info("%s: adj. End2, ta_cur=%d, ta_vol=%d, iin_cc=%d, chg_mode=%d\n",
					__func__, max77968->ta_cur, max77968->ta_vol, max77968->iin_cc, max77968->chg_mode);

			/* Go to return state  */
			/* Set timer */
			mutex_lock(&max77968->lock);
			if (max77968->charging_state == DC_STATE_CC_MODE)
				max77968->timer_id = TIMER_CHECK_CCMODE;
			else
				max77968->timer_id = TIMER_CHECK_CVMODE;
			max77968->timer_period = 1000;	/* Wait 1000ms */
			mutex_unlock(&max77968->lock);
		}
	}
	queue_delayed_work(max77968->dc_wq,
					&max77968->timer_work,
					msecs_to_jiffies(max77968->timer_period));

	return 0;
}


/* Set RX voltage for target current */
static int max77968_adjust_rx_voltage(struct max77968_charger *max77968)
{
	int iin, mode, ret, val;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_ADJUST_TAVOL);
#else
	max77968->charging_state = DC_STATE_ADJUST_TAVOL;
#endif

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		ret = max77968_vbatt_regu_enable_chk(max77968);
		if (ret < 0)
			return ret;
	}

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		return ret;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	/* Protect access to req_new_iin */
	mutex_lock(&max77968->lock);
	if ((max77968->req_new_iin == true) ||
		(max77968->new_iin_buf_has_data == true)) {
		mutex_unlock(&max77968->lock);
		pr_info("%s: max77968_set_new_iin\n", __func__);
		/* Return charging state to the previous state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968_set_charging_state(max77968, max77968->ret_state);
#else
		max77968->charging_state = max77968->ret_state;
#endif
		max77968_set_new_iin(max77968);
	} else {
		mutex_unlock(&max77968->lock);
		/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
		if (max77968->adc_wr_en)
			iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
		else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
		// For debug
		mode = max77968_check_dcmode_status(max77968);
		pr_info("%s: dcmode_status=%d\n", __func__, mode);

		/* Compare IIN ADC with targer input current */
		if (iin > max77968->iin_cc) {
			/* RX current is higher than the target input current */
			/* Decrease RX voltage (12.5mV) */
			max77968->ta_vol = max77968->ta_vol - WCRX_VOL_STEP;

			pr_info("%s: adj. Cont1, rx_vol=%d\n", __func__, max77968->ta_vol);

			/* Set RX voltage */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			max77968->timer_period = RXMSG_WAIT_T;
			mutex_unlock(&max77968->lock);
		} else {
			if (iin < (max77968->iin_cc - IIN_CC_COMP_OFFSET)) {
				/* RX current is lower than the target input current */
				/* Compare RX max voltage */
				if (max77968->ta_vol == max77968->ta_max_vol) {
					/* RX current is already the maximum voltage */
					/* Clear req_new_iin */
					mutex_lock(&max77968->lock);
					max77968->req_new_iin = false;
					mutex_unlock(&max77968->lock);

					pr_info("%s: adj. End1(max vol), rx_vol=%d, iin_cc=%d, chg_mode=%d\n",
						__func__, max77968->ta_vol, max77968->iin_cc, max77968->chg_mode);

					/* Return charging state to the previous state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
					max77968_set_charging_state(max77968, max77968->ret_state);
#else
					max77968->charging_state = max77968->ret_state;
#endif
					/* Go to return state  */
					/* Set timer */
					mutex_lock(&max77968->lock);
					if (max77968->charging_state == DC_STATE_CC_MODE)
						max77968->timer_id = TIMER_CHECK_CCMODE;
					else
						max77968->timer_id = TIMER_CHECK_CVMODE;
					max77968->timer_period = 1000;	/* Wait 1000ms */
					mutex_unlock(&max77968->lock);
				} else {
					/* Increase RX voltage (12.5mV) */
					max77968->ta_vol = max77968->ta_vol + WCRX_ADJUST_CC_RX_VOL_STEP;
					if (max77968->ta_vol > max77968->ta_max_vol)
						max77968->ta_vol = max77968->ta_max_vol;

					pr_info("%s: adj. Cont2, rx_vol=%d\n", __func__, max77968->ta_vol);

					/* Set RX voltage */
					mutex_lock(&max77968->lock);
					max77968->timer_id = TIMER_PDMSG_SEND;
					max77968->timer_period = RXMSG_WAIT_T;
					mutex_unlock(&max77968->lock);
				}
			} else {
				/* IIN ADC is in valid range */
				/* Clear req_new_iin */
				mutex_lock(&max77968->lock);
				max77968->req_new_iin = false;
				mutex_unlock(&max77968->lock);

				/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA	*/
				pr_info("%s: adj. End2(valid), rx_vol=%d, iin_cc=%d, chg_mode=%d\n",
						__func__, max77968->ta_vol, max77968->iin_cc, max77968->chg_mode);

				/* Return charging state to the previous state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
				max77968_set_charging_state(max77968, max77968->ret_state);
#else
				max77968->charging_state = max77968->ret_state;
#endif
				/* Go to return state  */
				/* Set timer */
				mutex_lock(&max77968->lock);
				if (max77968->charging_state == DC_STATE_CC_MODE)
					max77968->timer_id = TIMER_CHECK_CCMODE;
				else
					max77968->timer_id = TIMER_CHECK_CVMODE;
				max77968->timer_period = RXMSG_WAIT_T;	/* Wait 1000ms */
				mutex_unlock(&max77968->lock);
			}
		}
		queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));
	}

	return 0;
}

/* Set TA voltage for bypass mode */
static int max77968_set_bypass_ta_voltage_by_soc(struct max77968_charger *max77968, int delta_soc)
{
	int ret = 0;
	unsigned int prev_ta_vol = max77968->ta_vol;

	if (delta_soc < 0) { // increase soc (soc_now - ref_soc)
		max77968->ta_vol += PD_MSG_TA_VOL_STEP;
	} else if (delta_soc > 0) { // decrease soc (soc_now - ref_soc)
		max77968->ta_vol -= PD_MSG_TA_VOL_STEP;
	} else {
		pr_info("%s: abnormal delta_soc=%d\n", __func__, delta_soc);
		return -1;
	}

	pr_info("%s: delta_soc=%d, prev_ta_vol=%d, ta_vol=%d, ta_cur=%d\n",
		__func__, delta_soc, prev_ta_vol, max77968->ta_vol, max77968->ta_cur);

	/* Send PD Message */
	mutex_lock(&max77968->lock);
	max77968->timer_id = TIMER_PDMSG_SEND;
	max77968->timer_period = 0;
	mutex_unlock(&max77968->lock);
	schedule_delayed_work(&max77968->timer_work, msecs_to_jiffies(max77968->timer_period));

	return ret;
}

/* Set TA current for bypass mode */
static int max77968_set_bypass_ta_current(struct max77968_charger *max77968)
{
	int ret = 0;
	unsigned int val;

	/* Set charging state to BYPASS mode state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_BYPASS_MODE);
#else
	max77968->charging_state = DC_STATE_BYPASS_MODE;
#endif

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		ret = max77968_vbatt_regu_enable_chk(max77968);
		if (ret < 0)
			goto error;
	}

	pr_info("%s: new_iin=%d\n", __func__, max77968->new_iin);

	/* Set IIN_CFG to new_IIN */
	max77968->iin_cfg = max77968->new_iin;
	max77968->iin_cc = max77968->new_iin;
	ret = max77968_set_input_current(max77968, max77968->iin_cc);
	if (ret < 0)
		goto error;

	/* Clear Request flag */
	mutex_lock(&max77968->lock);
	max77968->req_new_iin = false;
	mutex_unlock(&max77968->lock);

#if defined(ISSUE_WORKAROUND)
	if ((max77968->byp_mode == PTM_2TO1) && (max77968->iin_cfg >= 3300000)
		&& (max77968->pass3_wr_en == true)) {
		max77968->adc_wr_en = true;
		pr_info("%s: ADC_WR_ENABLE\n", __func__);
	} else {
		max77968->adc_wr_en = false;
		pr_info("%s: ADC_WR_DISABLE\n", __func__);
	}
#endif

	/* Adjust IIN_CC with APDO resolution(50mA) - It will recover to the original value after sending PD message */
	val = max77968->iin_cc/PD_MSG_TA_CUR_STEP;
	max77968->iin_cc = val*PD_MSG_TA_CUR_STEP;
	/* Set TA current to IIN_CC */
	max77968->ta_cur = max77968->iin_cc/max77968->chg_mode;

	pr_info("%s: ta_cur=%d, ta_vol=%d\n", __func__, max77968->ta_cur, max77968->ta_vol);

	/* Recover IIN_CC to the original value(new_iin) */
	max77968->iin_cc = max77968->new_iin;

	/* Send PD Message */
	mutex_lock(&max77968->lock);
	max77968->timer_id = TIMER_PDMSG_SEND;
	max77968->timer_period = 0;
	mutex_unlock(&max77968->lock);
	queue_delayed_work(max77968->dc_wq,
				&max77968->timer_work,
				msecs_to_jiffies(max77968->timer_period));

error:
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}

/* Set TA voltage for bypass mode */
static int max77968_set_bypass_ta_voltage(struct max77968_charger *max77968)
{
	int ret = 0;
	unsigned int val;
	int vbat;

	/* Set charging state to BYPASS mode state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_BYPASS_MODE);
#else
	max77968->charging_state = DC_STATE_BYPASS_MODE;
#endif
	pr_info("%s: new_vfloat=%d\n", __func__, max77968->new_vfloat);

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		ret = max77968_vbatt_regu_enable_chk(max77968);
		if (ret < 0)
			goto error;
	}

	/* Set VFLOAT to new vfloat */
	max77968->vfloat = max77968->new_vfloat;
	ret = max77968_set_vfloat(max77968, max77968->vfloat);
	if (ret < 0)
		goto error;

	/* Clear Request flag */
	mutex_lock(&max77968->lock);
	max77968->req_new_vfloat = false;
	mutex_unlock(&max77968->lock);

	/* It needs to optimize TA voltage as calculating TA voltage with battery voltage later */
	/* Read VBAT ADC */
	vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

	/* Check BYP mode */
	if (max77968->byp_mode == PTM_1TO1) {
		/* Set TA voltage to VBAT_ADC + Offset */
		val = vbat + TA_VOL_OFFSET_1TO1_BYPASS;
	} else if (max77968->byp_mode == PTM_2TO1) {
		/* Set TA voltage to 2*VBAT_ADC + Offset */
		val = 2 * vbat + TA_VOL_OFFSET_2TO1_BYPASS;
	} else {
		/* Set TA voltage to 3*VBAT_ADC + Offset */
		val = 3 * vbat + TA_VOL_OFFSET_3TO1_BYPASS;
	}

	max77968->ta_vol = val;

	pr_info("%s: vbat=%d, ta_vol=%d, ta_cur=%d\n",
			__func__, vbat, max77968->ta_vol, max77968->ta_cur);

	/* Send PD Message */
	mutex_lock(&max77968->lock);
	max77968->timer_id = TIMER_PDMSG_SEND;
	max77968->timer_period = 0;
	mutex_unlock(&max77968->lock);
	queue_delayed_work(max77968->dc_wq,
				&max77968->timer_work,
				msecs_to_jiffies(max77968->timer_period));

error:
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}

/* Set new input current  */
static int max77968_set_new_iin(struct max77968_charger *max77968)
{
	int ret = 0;

	if (max77968->new_iin_buf_has_data == true) {
		mutex_lock(&max77968->lock);
		max77968->new_iin = max77968->new_iin_busy_buf;
		max77968->new_iin_buf_has_data = false;
		max77968->req_new_iin = true;
		mutex_unlock(&max77968->lock);
	}

	pr_info("%s: new_iin=%d\n", __func__, max77968->new_iin);

	/* Check current BYP mode */
	if ((max77968->byp_mode == PTM_1TO1) ||
		(max77968->byp_mode == PTM_2TO1) ||
		(max77968->byp_mode == PTM_3TO1)) {
		/* DC mode is 1:1, 2:1, 3:1 mode */
		/* Set new iin for bypass mode */
		pr_info("%s: max77968_set_bypass_ta_current\n", __func__);
		ret = max77968_set_bypass_ta_current(max77968);
	} else {
		/* DC mode is normal mode */
		/* Set new IIN to IIN_CC */
		max77968->iin_cc = max77968->new_iin;
		/* Save return state */
		max77968->ret_state = max77968->charging_state;

#if defined(ISSUE_WORKAROUND)
		if ((max77968->chg_mode == CHG_2TO1_DC_MODE) &&
			(max77968->iin_cc >= 3300000) &&
			(max77968->pass3_wr_en == true)) {
			max77968->adc_wr_en = true;
			pr_info("%s: ADC_WR_ENABLE\n", __func__);
		} else {
			max77968->adc_wr_en = false;
			pr_info("%s: ADC_WR_DISABLE\n", __func__);
		}
#endif

		/* Check the TA type first */
		if (max77968->ta_type == TA_TYPE_WIRELESS) {
			/* Wireless Charger is connected */
			max77968->iin_cfg = max77968->iin_cc;
			/* Set IIN_CFG to new IIN */
			max77968_set_input_current(max77968, max77968->iin_cc);
			mutex_lock(&max77968->lock);
			max77968->req_new_iin = false;
			mutex_unlock(&max77968->lock);
			ret = max77968_adjust_rx_voltage(max77968);
		} else {
			/* USBPD TA is connected */
			/* Check new IIN with the minimum TA current */
			if (max77968->iin_cc < TA_MIN_CUR) {
				/* Set the TA current to TA_MIN_CUR(1.0A) */
				max77968->ta_cur = TA_MIN_CUR;
				/* Need to control TA voltage for request current */
				ret = max77968_adjust_ta_voltage(max77968);
			} else {
				/* Need to control TA current for request current */
				ret = max77968_adjust_ta_current(max77968);
			}
		}
	}

	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}


/* Set new float voltage */
static int max77968_set_new_vfloat(struct max77968_charger *max77968)
{
	int ret = 0;
	int vbat;
	unsigned int val;

	pr_info("%s: Request New VFLOAT : %d\n", __func__, max77968->req_new_vfloat);

	/* Check current BYP mode */
	if ((max77968->byp_mode == PTM_1TO1) ||
		(max77968->byp_mode == PTM_2TO1) ||
		(max77968->byp_mode == PTM_3TO1)) {
		/* DC mode is bypass mode */
		/* Set new vfloat for bypass mode */
		ret = max77968_set_bypass_ta_voltage(max77968);
	} else {
		/* DC mode is normal mode */
		/* Read VBAT ADC */
		vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

		/* Compare the new VBAT with present vfloat */
		if (max77968->new_vfloat > max77968->vfloat) {
			/* cancel delayed_work */
			cancel_delayed_work(&max77968->timer_work);

			/* Set vfloat decrement flag to false */
			max77968->dec_vfloat = false;

			/* Set VFLOAT to new vfloat */
			max77968->vfloat = max77968->new_vfloat;
			/* Set MAX77968 VFLOAT to default value */
			ret = max77968_set_vfloat(max77968, VBAT_REG_DFT);
			if (ret < 0)
				goto error;

			pr_info("%s: before New VFLOAT, ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, ta_cur=%d, chg_mode=%d\n",
					__func__, max77968->ta_max_vol, max77968->ta_max_cur, max77968->ta_max_pwr, max77968->iin_cc, max77968->ta_cur, max77968->chg_mode);
			/* Set IIN_CFG to the current IIN_CC */
			/* save the current iin_cc in iin_cfg */
			max77968->iin_cfg = max77968->iin_cc;
			max77968->iin_cfg = MIN(max77968->iin_cfg, max77968->ta_max_cur * max77968->chg_mode);
			ret = max77968_set_input_current(max77968, max77968->iin_cfg);
			if (ret < 0)
				goto error;

			max77968->iin_cc = max77968->iin_cfg;

			/* Clear req_new_vfloat */
			mutex_lock(&max77968->lock);
			max77968->req_new_vfloat = false;
			mutex_unlock(&max77968->lock);

			/* Check the TA type */
			if (max77968->ta_type == TA_TYPE_WIRELESS) {
				/* Wireless Charger is connected */
				/* Set RX voltage to MAX[TA_MIN_VOL_PRESET*chg_mode, (VBAT_ADC*chg_mode + offset)] */
				max77968->ta_vol = max(TA_MIN_VOL_PRESET * max77968->chg_mode, (vbat*max77968->chg_mode + TA_VOL_PRE_OFFSET));
				val = max77968->ta_vol/WCRX_VOL_STEP;	/* RX voltage resolution is 12.5mV */
				max77968->ta_vol = val*WCRX_VOL_STEP;
				/* Set RX voltage to MIN[RX voltage, RX_MAX_VOL] */
				max77968->ta_vol = MIN(max77968->ta_vol, max77968->ta_max_vol);

				pr_info("%s: New VFLOAT, rx_max_vol=%d, rx_vol=%d, iin_cc=%d, chg_mode=%d\n",
					__func__, max77968->ta_max_vol, max77968->ta_vol, max77968->iin_cc, max77968->chg_mode);
			} else {
				/* USBPD TA is connected */
				/* Calculate new TA maximum voltage that used in the direct charging */
				/* Calculate new TA max voltage */
				/* Adjust IIN_CC with APDO resoultion(50mA) - It will recover to the original value after max voltage calculation */
				val = max77968->iin_cc / (PD_MSG_TA_CUR_STEP * max77968->chg_mode);
				max77968->iin_cc = val * (PD_MSG_TA_CUR_STEP * max77968->chg_mode);

				/* Set TA_MAX_VOL to MIN[TA_MAX_VOL, (TA_MAX_PWR/IIN_CC)] */
				val = max77968->ta_max_pwr / (max77968->iin_cc / max77968->chg_mode / 1000); /* mV */
				val = val * 1000 / PD_MSG_TA_VOL_STEP;	/* uV */
				val = val * PD_MSG_TA_VOL_STEP; /* Adjust values with APDO resolution(20mV) */
				max77968->ta_max_vol = MIN(val, TA_MAX_VOL * max77968->chg_mode);

				/* Set TA voltage to MAX[TA_MIN_VOL_PRESET*chg_mode, (VBAT_ADC*chg_mode + offset)] */
				max77968->ta_vol = max(TA_MIN_VOL_PRESET * max77968->chg_mode, (vbat * max77968->chg_mode + TA_VOL_PRE_OFFSET));
				val = max77968->ta_vol / PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
				max77968->ta_vol = val * PD_MSG_TA_VOL_STEP;
				/* Set TA voltage to MIN[TA voltage, TA_MAX_VOL] */
				max77968->ta_vol = MIN(max77968->ta_vol, max77968->ta_max_vol);
				/* Set TA current to IIN_CC */
				max77968->ta_cur = max77968->iin_cc;
				/* Recover IIN_CC to the original value(iin_cfg) */
				max77968->iin_cc = max77968->iin_cfg;

				pr_info("%s: New VFLOAT, ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, ta_cur=%d, chg_mode=%d\n",
					__func__, max77968->ta_max_vol, max77968->ta_max_cur, max77968->ta_max_pwr, max77968->iin_cc, max77968->ta_cur, max77968->chg_mode);
			}

			/* Clear previous IIN ADC */
			max77968->prev_iin = 0;
			/* Clear TA increment flag */
			max77968->prev_inc = INC_NONE;

			/* Send PD Message and go to Adjust CC mode */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_charging_state(max77968, DC_STATE_ADJUST_CC);
#else
			max77968->charging_state = DC_STATE_ADJUST_CC;
#endif
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		} else if (max77968->new_vfloat == max77968->vfloat) {
			/* New vfloat is sameas the present vfloat */
			/* Don't need any setting */
			/* cancel delayed_work */
			cancel_delayed_work(&max77968->timer_work);

			/* Clear req_new_vfloat */
			mutex_lock(&max77968->lock);
			max77968->req_new_vfloat = false;
			mutex_unlock(&max77968->lock);

			/* Go to the present state */
			pr_info("%s: New vfloat is same as present vfloat and go to the present state\n", __func__);

			/* Set timer */
			mutex_lock(&max77968->lock);
			if (max77968->charging_state == DC_STATE_CC_MODE)
				max77968->timer_id = TIMER_CHECK_CCMODE;
			else
				max77968->timer_id = TIMER_CHECK_CVMODE;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		} else {
			/* The new vfloat is lower than present vfloat */
			/* cancel delayed_work */
			cancel_delayed_work(&max77968->timer_work);

			/* Set vfloat decrement flag */
			max77968->dec_vfloat = true;

			/* Set VFLOAT to new vfloat */
			max77968->vfloat = max77968->new_vfloat;
			/* Set MAX77968 VFLOAT to default value */
			ret = max77968_set_vfloat(max77968, max77968->vfloat);
			if (ret < 0)
				goto error;

			/* Clear req_new_vfloat */
			mutex_lock(&max77968->lock);
			max77968->req_new_vfloat = false;
			mutex_unlock(&max77968->lock);

			pr_info("%s: New vfloat is lower than present vfloat and go to Pre-CV state\n", __func__);

			/* Go to Pre-CV mode */
			max77968->charging_state = DC_STATE_START_CV;
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_ENTER_CVMODE;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		}
	}

error:
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}

/* Set new direct charging mode*/
static int max77968_set_new_chg_mode(struct max77968_charger *max77968)
{
	int ret = 0;

	pr_info("%s: ======START=======\n", __func__);

	/* Protect access to new_chg_mode_buf_has_data */
	mutex_lock(&max77968->lock);
	if (max77968->new_chg_mode_buf_has_data == true) {
		max77968->new_chg_mode = max77968->new_chg_mode_busy_buf;
		max77968->new_chg_mode_buf_has_data = false;
	}
	mutex_unlock(&max77968->lock);

	switch (max77968->new_chg_mode) {
	case CHG_3TO1_DC_MODE:
	case CHG_2TO1_DC_MODE:
		if (max77968->chg_mode != max77968->new_chg_mode) {

			if (max77968->new_iin_buf_has_data == true) {
				max77968->iin_cfg = max77968->new_iin_busy_buf;
				mutex_lock(&max77968->lock);
				max77968->new_iin = max77968->iin_cfg;
				max77968->req_new_iin = false;
				max77968->new_iin_buf_has_data = false;
				mutex_unlock(&max77968->lock);
			}
			ret = max77968_set_charging(max77968, false);
			if (ret < 0)
				goto error;

			ret = max77968_softreset(max77968);
			if (ret < 0)
				goto error;

			/* Set SCC to standby state*/
			ret = max77968_set_standby_state(max77968, true);
			if (ret < 0)
				goto error;

			/* Set chg mode to new chg mode, normal mode */
			max77968->pdata->chg_mode = max77968->new_chg_mode;
			/* Clear request flag */
			mutex_lock(&max77968->lock);
			max77968->req_new_chg_mode = false;
			mutex_unlock(&max77968->lock);
			pr_info("%s: Set new_chg_mode=%d\n", __func__, max77968->pdata->chg_mode);

			/* Go to DC_STATE_PRESET_DC */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PRESET_DC;
			max77968->timer_period = DC_MODE_CHANGES_WAIT_T;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
				&max77968->timer_work,
				msecs_to_jiffies(max77968->timer_period));
		} else {
			/* Clear request flag */
			mutex_lock(&max77968->lock);
			max77968->req_new_chg_mode = false;
			mutex_unlock(&max77968->lock);

			pr_info("%s: chg_mode did not change\n", __func__);

			/* Set timer */
			mutex_lock(&max77968->lock);
			if (max77968->charging_state == DC_STATE_CC_MODE)
				max77968->timer_id = TIMER_CHECK_CCMODE;
			else
				max77968->timer_id = TIMER_CHECK_CVMODE;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);

			queue_delayed_work(max77968->dc_wq,
				&max77968->timer_work,
				msecs_to_jiffies(max77968->timer_period));
		}
		break;

	case CHG_NO_DC_MODE:
		/* Clear request flag */
		mutex_lock(&max77968->lock);
		max77968->req_new_chg_mode = false;
		mutex_unlock(&max77968->lock);

		pr_info("%s: new chg_mode is CHG_NO_DC_MODE, mode did not change\n", __func__);

		/* Set timer */
		mutex_lock(&max77968->lock);
		if (max77968->charging_state == DC_STATE_CC_MODE)
			max77968->timer_id = TIMER_CHECK_CCMODE;
		else
			max77968->timer_id = TIMER_CHECK_CVMODE;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);

		queue_delayed_work(max77968->dc_wq,
			&max77968->timer_work,
			msecs_to_jiffies(max77968->timer_period));
		break;

	default:
		ret = -EINVAL;
		pr_info("%s: Set new chg mode, Invalid mode=%d, %d\n",
				__func__, max77968->new_chg_mode, max77968->pdata->chg_mode);
		break;
	}

error:
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}

/* Set new bypass mode */
static int max77968_set_new_byp_mode(struct max77968_charger *max77968)
{
	int ret = 0;
	int vbat, val;

	pr_info("%s: ======START=======\n", __func__);

	/* Read VBAT ADC */
	vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

	/* Check new byp mode */
	switch (max77968->new_byp_mode) {
	case PTM_1TO1:
	case PTM_2TO1:
	case PTM_3TO1:
		/* Change normal mode to bypass mode */
		/* Check current byp mode */
		if ((max77968->byp_mode == PTM_1TO1) ||
			(max77968->byp_mode == PTM_2TO1) ||
			(max77968->byp_mode == PTM_3TO1)) {
			/* TA voltage already changed to 1:1, 2:1 or 3:1 mode */
			/* Enable reverse current detection by different threshold */
			val = CHGR_RCP_EN;
			val |= CHGR_RCP_DEG_10us << MASK2SHIFT(CHGR_RCP_DEG);
			val |= CHGR_RCP_MIN << MASK2SHIFT(CHGR_RCP);
			ret = max77968_write_reg(max77968, CHGR_RCP_CFG_REG, val);
			if (ret < 0)
				goto error;

			/* Disable IIN_UCP protection */
			ret = max77968_update_reg(max77968, IIN_UCP_CFG_REG, IIN_UCP_EN, 0);
			if (ret < 0)
				goto error;

			pr_info("%s: New BYP mode, mode=%d\n", __func__, max77968->byp_mode);

			/* Enable Charging - recover charging as bypass mode */
			ret = max77968_set_charging(max77968, true);
			if (ret < 0)
				goto error;

			/* Clear request flag */
			mutex_lock(&max77968->lock);
			max77968->req_new_byp_mode = false;
			mutex_unlock(&max77968->lock);

			pr_info("%s: New BYP mode, Normal->BYP(%d) done\n", __func__, max77968->byp_mode);

			/* Wait 500ms and go to bypass state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_charging_state(max77968, DC_STATE_BYPASS_MODE);
#else
			max77968->charging_state = DC_STATE_BYPASS_MODE;
#endif
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_BYPASSMODE;
			max77968->timer_period = BYPASS_WAIT_T;	/* 200ms */
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		} else {
			/* DC mode is normal mode */
			/* TA voltage is not changed to 1:1, 2:1 or 3:1 mode yet */
			/* Disable charging */
			ret = max77968_set_charging(max77968, false);
			if (ret < 0)
				goto error;
			/* Set byp mode to new byp mode */
			max77968->byp_mode = max77968->new_byp_mode;
			if (max77968->byp_mode == PTM_3TO1) {
				/* Set TA voltage to 3:1 bypass voltage */
				max77968->ta_vol = 3 * vbat + TA_VOL_OFFSET_3TO1_BYPASS;
				max77968->chg_mode = CHG_3TO1_DC_MODE;
				pr_info("%s: New BYP mode, Normal->3:1 BYP, ta_vol=%d, ta_cur=%d\n",
						__func__, max77968->ta_vol, max77968->ta_cur);
			} else if (max77968->byp_mode == PTM_2TO1) {
				/* Set TA voltage to 2:1 bypass voltage */
				max77968->ta_vol = 2 * vbat + TA_VOL_OFFSET_2TO1_BYPASS;
				max77968->chg_mode = CHG_2TO1_DC_MODE;
				pr_info("%s: New BYP mode, Normal->2:1 BYP, ta_vol=%d, ta_cur=%d\n",
						__func__, max77968->ta_vol, max77968->ta_cur);
			} else {
				/* Set TA voltage to 1:1 voltage */
				max77968->ta_vol = vbat + TA_VOL_OFFSET_1TO1_BYPASS;
				max77968->chg_mode = CHG_2TO1_DC_MODE;
				pr_info("%s: New BYP mode, Normal->1:1 BYP, ta_vol=%d, ta_cur=%d\n",
						__func__, max77968->ta_vol, max77968->ta_cur);
			}

			/* Send PD Message and go to dcmode change state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_charging_state(max77968, DC_STATE_DCMODE_CHANGE);
#else
			max77968->charging_state = DC_STATE_DCMODE_CHANGE;
#endif
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		}
		break;

	case PTM_NONE:
		/* Change bypass mode to normal mode */
		/* Disable charging */
		ret = max77968_set_charging(max77968, false);
		if (ret < 0)
			goto error;

		/* Set reverse current detection to original threshold*/
		val = CHGR_RCP_EN;
		val |= CHGR_RCP_DEG_50ms << MASK2SHIFT(CHGR_RCP_DEG);
		val |= CHGR_RCP_CFG(CHG_RCP_TH_SET) << MASK2SHIFT(CHGR_RCP);
		ret = max77968_write_reg(max77968, CHGR_RCP_CFG_REG, val);
		if (ret < 0)
			goto error;

		/* Enable IIN_UCP protection */
		ret = max77968_update_reg(max77968, IIN_UCP_CFG_REG, IIN_UCP_EN, IIN_UCP_EN);
		if (ret < 0)
			goto error;

		/* Set byp mode to new byp mode, normal mode */
		max77968->byp_mode = max77968->new_byp_mode;
		/* Clear request flag */
		mutex_lock(&max77968->lock);
		max77968->req_new_byp_mode = false;
		mutex_unlock(&max77968->lock);
		pr_info("%s: New DC mode, BYP->Normal\n", __func__);

		/* Go to DC_STATE_PRESET_DC */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PRESET_DC;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		break;

	default:
		ret = -EINVAL;
		pr_info("%s: New BYP mode, Invalid mode=%d\n", __func__, max77968->new_byp_mode);
		break;
	}

error:
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}

/* 2:1 Direct Charging Adjust CC MODE control */
static int max77968_charge_adjust_ccmode(struct max77968_charger *max77968)
{
	int iin, vbatt, ccmode;
	int val;
	int ret = 0;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_ADJUST_CC);
#else
	max77968->charging_state = DC_STATE_ADJUST_CC;
#endif

	/* Enable VBATT regulation threshold to default (4.45V) */
	ret = max77968_set_vfloat(max77968, VBAT_REG_DFT);
	if (ret < 0)
		goto error;

	/* Make sure VBATT Regulation enabled */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		goto error;

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	ret = max77968_check_error(max77968);
	if (ret != 0)
		goto error; // This is not active mode.
	/* Check the status */
	ccmode = max77968_check_dcmode_status(max77968);
	if (ccmode < 0) {
		ret = ccmode;
		goto error;
	}

	switch (ccmode) {
	case DCMODE_IIN_LOOP:
		/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
		if (max77968->adc_wr_en)
			iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
		else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
		/* Read VBAT ADC */
		vbatt = max77968_read_adc(max77968, ADC_CH_VBATT);

		/* Check the TA type first */
		if (max77968->ta_type == TA_TYPE_WIRELESS) {
			/* Decrease RX voltage (40mV) */
			max77968->ta_vol = max77968->ta_vol - WCRX_VOL_DOWN_STEP;
			pr_info("%s: CC adjust End(LOOP): iin=%d, vbatt=%d, rx_vol=%d\n",
					__func__, iin, vbatt, max77968->ta_vol);

			/* Set TA target voltage to TA voltage */
			max77968->ta_target_vol = max77968->ta_vol;
			/* Clear TA increment flag */
			max77968->prev_inc = INC_NONE;
			/* Send PD Message and then go to CC mode */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_charging_state(max77968, DC_STATE_CC_MODE);
#else
			max77968->charging_state = DC_STATE_CC_MODE;
#endif
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			max77968->timer_period = RXMSG_WAIT_T;
			mutex_unlock(&max77968->lock);
		} else {
			/* Check TA current */
			if ((max77968->ta_cur > TA_MIN_CUR) &&
				(max77968->ta_ctrl == TA_CTRL_CL_MODE)) {
				/* TA current is higher than 1.0A */
				/* Decrease TA current (50mA) */
				max77968->ta_cur = max77968->ta_cur - PD_MSG_TA_CUR_STEP;

				/* TA target voltage = TA voltage + (VFLOAT - VBAT_ADC)*CHG_mode + 100mV */
				val = max77968->ta_vol + (max77968->vfloat - vbatt)*max77968->chg_mode + 100000;
				val = val/PD_MSG_TA_VOL_STEP;
				max77968->ta_target_vol = val*PD_MSG_TA_VOL_STEP;
				if (max77968->ta_target_vol > max77968->ta_max_vol)
					max77968->ta_target_vol = max77968->ta_max_vol;
				pr_info("%s: CC adjust End(LOOP): iin=%d, vbatt=%d, ta_cur=%d, ta_vol=%d, ta_target_vol=%d\n",
						__func__, iin, vbatt, max77968->ta_cur, max77968->ta_vol, max77968->ta_target_vol);
				/* Clear TA increment flag */
				max77968->prev_inc = INC_NONE;
				/* Go to Start CC mode */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_ENTER_CCMODE;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
			} else {
				/* Decrease TA voltage (20mV) */
				max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;

				/* Set TA target voltage to TA voltage */
				max77968->ta_target_vol = max77968->ta_vol;
				/* Clear TA increment flag */
				max77968->prev_inc = INC_NONE;
				/* Send PD Message and then go to CC mode */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
				max77968_set_charging_state(max77968, DC_STATE_CC_MODE);
#else
				max77968->charging_state = DC_STATE_CC_MODE;
#endif
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PDMSG_SEND;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);

				pr_info("%s: CC adjust End(LOOP): iin=%d, vbatt=%d, ta_cur=%d, ta_vol=%d\n",
						__func__, iin, vbatt, max77968->ta_cur, max77968->ta_vol);
			}
		}
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		break;

	case DCMODE_VFLT_LOOP:
		/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
		if (max77968->adc_wr_en)
			iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
		else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
		/* Read VBAT ADC */
		vbatt = max77968_read_adc(max77968, ADC_CH_VBATT);

		pr_info("%s: CC adjust End(VFLOAT): vbatt=%d, iin=%d, ta_vol=%d\n",
				__func__, vbatt, iin, max77968->ta_vol);

		/* Save TA target voltage*/
		max77968->ta_target_vol = max77968->ta_vol;
		/* Clear TA increment flag */
		max77968->prev_inc = INC_NONE;
		/* Go to Pre-CV mode */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_ENTER_CVMODE;
		if (max77968->ta_type == TA_TYPE_WIRELESS)
			max77968->timer_period = RXMSG_WAIT_T;
		else
			max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		break;

	case DCMODE_LOOP_INACTIVE:
		/* Check IIN ADC with IIN */
#if defined(ISSUE_WORKAROUND)
		if (max77968->adc_wr_en)
			iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
		else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
		/* Read VBAT ADC */
		vbatt = max77968_read_adc(max77968, ADC_CH_VBATT);

		/* Check the TA type first */
		if (max77968->ta_type == TA_TYPE_WIRELESS) {
			/* IIN_ADC > IIN_CC -20mA ? */
			if (iin > (max77968->iin_cc - IIN_ADC_OFFSET)) {
				/* Input current is already over IIN_CC */
				/* End RX voltage adjustment */
				/* change charging state to CC mode */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
				max77968_set_charging_state(max77968, DC_STATE_CC_MODE);
#else
				max77968->charging_state = DC_STATE_CC_MODE;
#endif
				pr_info("%s: CC adjust End: iin=%d, vbatt=%d, rx_vol=%d\n",
						__func__, iin, vbatt, max77968->ta_vol);

				/* Save TA target voltage*/
				max77968->ta_target_vol = max77968->ta_vol;
				/* Clear TA increment flag */
				max77968->prev_inc = INC_NONE;
				/* Go to CC mode */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_CCMODE;
				max77968->timer_period = RXMSG_WAIT_T;
				mutex_unlock(&max77968->lock);
			} else {
				/* Check RX voltage */
				if (max77968->ta_vol == max77968->ta_max_vol) {
					/* RX voltage is already max value */
					pr_info("%s: CC adjust End: MAX value, iin=%d, vbatt=%d, rx_vol=%d\n",
							__func__, iin, vbatt, max77968->ta_vol);

					/* Save TA target voltage*/
					max77968->ta_target_vol = max77968->ta_vol;
					/* Clear TA increment flag */
					max77968->prev_inc = INC_NONE;
					/* Go to CC mode */
					mutex_lock(&max77968->lock);
					max77968->timer_id = TIMER_CHECK_CCMODE;
					max77968->timer_period = RXMSG_WAIT_T;
					mutex_unlock(&max77968->lock);
				} else {
					/* Try to increase RX voltage(12.5mV) */
					max77968->ta_vol = max77968->ta_vol + WCRX_ADJUST_CC_RX_VOL_STEP;
					if (max77968->ta_vol > max77968->ta_max_vol)
						max77968->ta_vol = max77968->ta_max_vol;
					pr_info("%s: CC adjust Cont: iin=%d, vbatt=%d, rx_vol=%d\n",
							__func__, iin, vbatt, max77968->ta_vol);
					/* Set RX voltage */
					mutex_lock(&max77968->lock);
					max77968->timer_id = TIMER_PDMSG_SEND;
					max77968->timer_period = RXMSG_WAIT_T;
					mutex_unlock(&max77968->lock);
				}
			}
		} else {
			/* USBPD TA is connected */
			/* IIN_ADC > IIN_CC -20mA ? */
			if (iin > (max77968->iin_cc - IIN_ADC_OFFSET)) {
				pr_info("%s: CC adjust End(Normal): iin=%d, iin_cc=%d, vbatt=%d, ta_vol=%d, ta_cur=%d\n",
					__func__, iin, max77968->iin_cc, vbatt, max77968->ta_vol, max77968->ta_cur);
				/* Save TA target voltage */
				max77968->ta_target_vol = max77968->ta_vol;
				pr_info("%s: CC adjust End(Normal): ta_ctrl=%d, ta_target_vol=%d\n",
					__func__, max77968->ta_ctrl, max77968->ta_target_vol);
				/* Clear TA increment flag */
				max77968->prev_inc = INC_NONE;
				/* Go to CC mode */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_CCMODE;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
			} else {
				/* Compare TA maximum voltage */
				if (max77968->ta_vol == max77968->ta_max_vol) {
					/* Compare TA maximum current */
					if (max77968->ta_cur == max77968->ta_max_cur) {
						/* TA voltage and current are already max value */
						pr_info("%s: CC adjust End(MAX_VOL/CUR): iin=%d, ta_vol=%d, ta_cur=%d\n",
								__func__, iin, max77968->ta_vol, max77968->ta_cur);
						/* Save TA target voltage */
						max77968->ta_target_vol = max77968->ta_vol;
						/* Clear TA increment flag */
						max77968->prev_inc = INC_NONE;
						/* Go to CC mode */
						mutex_lock(&max77968->lock);
						max77968->timer_id = TIMER_CHECK_CCMODE;
						max77968->timer_period = 0;
						mutex_unlock(&max77968->lock);
					} else {
						/* TA current is not maximum value */
						/* Increase TA current(50mA) */
						max77968->ta_cur = max77968->ta_cur + PD_MSG_TA_CUR_STEP;
						if (max77968->ta_cur > max77968->ta_max_cur)
							max77968->ta_cur = max77968->ta_max_cur;
						pr_info("%s: CC adjust Cont(1): iin=%d, ta_cur=%d\n",
								__func__, iin, max77968->ta_cur);

						/* Set TA increment flag */
						max77968->prev_inc = INC_TA_CUR;
						/* Send PD Message */
						mutex_lock(&max77968->lock);
						max77968->timer_id = TIMER_PDMSG_SEND;
						max77968->timer_period = 0;
						mutex_unlock(&max77968->lock);
					}
				} else {
					/* Check TA tolerance */
					/* The current input current compares the final input current(IIN_CC) with 100mA offset */
					/* PPS current tolerance has +/-150mA, so offset defined 100mA(tolerance +50mA) */
					if (iin < (max77968->iin_cc - TA_IIN_OFFSET)) {
						/* TA voltage too low to enter TA CC mode, so we should increae TA voltage */
#if IS_ENABLED(CONFIG_SEC_FACTORY)
						val = max77968->iin_cc - iin;
						val = (val * 4)/10;
						if (val > FACTORY_TA_VOL_STEP_ADJ_CC_MAX)
							val = FACTORY_TA_VOL_STEP_ADJ_CC_MAX;
						else if (val < TA_VOL_STEP_ADJ_CC)
							val = TA_VOL_STEP_ADJ_CC;
						max77968->ta_vol = max77968->ta_vol + (int)val;
						val = max77968->ta_vol / PD_MSG_TA_VOL_STEP;
						max77968->ta_vol = val * PD_MSG_TA_VOL_STEP;
#else
						if (vbatt >= BAT_VOL_TH_TO_ADJ_CC_MIN_STEP)
							max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
						else
							max77968->ta_vol = max77968->ta_vol + TA_VOL_STEP_ADJ_CC * max77968->chg_mode;
#endif
						if (max77968->ta_vol > max77968->ta_max_vol)
							max77968->ta_vol = max77968->ta_max_vol;
						pr_info("%s: CC adjust Cont(2): (iin = %d / %d), ta_vol=%d\n",
								__func__, iin, max77968->iin_cc - TA_IIN_OFFSET, max77968->ta_vol);

						/* Set TA increment flag */
						max77968->prev_inc = INC_TA_VOL;
						/* Send PD Message */
						mutex_lock(&max77968->lock);
						max77968->timer_id = TIMER_PDMSG_SEND;
						max77968->timer_period = 0;
						mutex_unlock(&max77968->lock);
					} else {
						/* compare IIN ADC with previous IIN ADC + 20mA */
						if (iin > (max77968->prev_iin + IIN_ADC_OFFSET)) {
							/* TA can supply more current if TA voltage is high */
							/* TA voltage too low to enter TA CC mode, so we should increae TA voltage */
#if IS_ENABLED(CONFIG_SEC_FACTORY)
							val = max77968->iin_cc - iin;
							val = (val * 4)/10;
							if (val > FACTORY_TA_VOL_STEP_ADJ_CC_MAX)
								val = FACTORY_TA_VOL_STEP_ADJ_CC_MAX;
							else if (val < TA_VOL_STEP_ADJ_CC)
								val = TA_VOL_STEP_ADJ_CC;
							max77968->ta_vol = max77968->ta_vol + (int)val;
							val = max77968->ta_vol / PD_MSG_TA_VOL_STEP;
							max77968->ta_vol = val * PD_MSG_TA_VOL_STEP;
#else
							if (vbatt >= BAT_VOL_TH_TO_ADJ_CC_MIN_STEP)
								max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
							else
								max77968->ta_vol = max77968->ta_vol + TA_VOL_STEP_ADJ_CC * max77968->chg_mode;
#endif
							if (max77968->ta_vol > max77968->ta_max_vol)
								max77968->ta_vol = max77968->ta_max_vol;
							pr_info("%s: CC adjust Cont(3): iin=%d, ta_vol=%d\n",
									__func__, iin, max77968->ta_vol);

							/* Set TA increment flag */
							max77968->prev_inc = INC_TA_VOL;
							/* Send PD Message */
							mutex_lock(&max77968->lock);
							max77968->timer_id = TIMER_PDMSG_SEND;
							max77968->timer_period = 0;
							mutex_unlock(&max77968->lock);
						} else {
							/* Check the previous increment */
							if (max77968->prev_inc == INC_TA_CUR) {
								/* The previous increment is TA current, but input current does not increase */
								/* Try to increase TA voltage(40mV) */
#if IS_ENABLED(CONFIG_SEC_FACTORY)
								val = max77968->iin_cc - iin;
								val = (val * 4)/10;
								if (val > FACTORY_TA_VOL_STEP_ADJ_CC_MAX)
									val = FACTORY_TA_VOL_STEP_ADJ_CC_MAX;
								else if (val < TA_VOL_STEP_ADJ_CC)
									val = TA_VOL_STEP_ADJ_CC;
								max77968->ta_vol = max77968->ta_vol + (int)val;
								val = max77968->ta_vol / PD_MSG_TA_VOL_STEP;
								max77968->ta_vol = val * PD_MSG_TA_VOL_STEP;
#else
								if (vbatt >= BAT_VOL_TH_TO_ADJ_CC_MIN_STEP)
									max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
								else
									max77968->ta_vol = max77968->ta_vol + TA_VOL_STEP_ADJ_CC * max77968->chg_mode;
#endif
								if (max77968->ta_vol > max77968->ta_max_vol)
									max77968->ta_vol = max77968->ta_max_vol;
								pr_info("%s: CC adjust Cont(4): iin=%d, ta_vol=%d\n",
										__func__, iin, max77968->ta_vol);

								/* Set TA increment flag */
								max77968->prev_inc = INC_TA_VOL;
								/* Send PD Message */
								mutex_lock(&max77968->lock);
								max77968->timer_id = TIMER_PDMSG_SEND;
								max77968->timer_period = 0;
								mutex_unlock(&max77968->lock);
							} else {
								/* The previous increment is TA voltage, but input current does not increase */
								/* Try to increase TA current */
								/* Check APDO max current */
								if (max77968->ta_cur == max77968->ta_max_cur) {
									if (max77968->ta_ctrl == TA_CTRL_CL_MODE) {
										/* Current TA control method is CL mode */
										/* TA current is maximum current */
										pr_info("%s: CC adjust End(MAX_CUR): iin=%d, ta_vol=%d, ta_cur=%d\n",
												__func__, iin, max77968->ta_vol, max77968->ta_cur);

										/* TA target voltage = TA voltage + (VFLOAT - VBAT_ADC)*CHG_mode + 100mV */
										if (vbatt >= BAT_VOL_TH_TO_ADJ_CC_MIN_STEP)
											val = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
										else
											val = max77968->ta_vol + (max77968->vfloat - vbatt) * max77968->chg_mode + 100000;

										val = val/PD_MSG_TA_VOL_STEP;
										max77968->ta_target_vol = val*PD_MSG_TA_VOL_STEP;
										if (max77968->ta_target_vol > max77968->ta_max_vol)
											max77968->ta_target_vol = max77968->ta_max_vol;
										pr_info("%s: CC adjust End: ta_target_vol=%d\n", __func__, max77968->ta_target_vol);

										/* Clear TA increment flag */
										max77968->prev_inc = INC_NONE;
										/* Go to Start CC mode */
										mutex_lock(&max77968->lock);
										max77968->timer_id = TIMER_ENTER_CCMODE;
										max77968->timer_period = 0;
										mutex_unlock(&max77968->lock);
									} else {
										/* Current TA control method is CV mode */
										pr_info("%s: CC adjust End(MAX_CUR,CV): iin=%d, ta_vol=%d, ta_cur=%d\n",
												__func__, iin, max77968->ta_vol, max77968->ta_cur);
										/* Save TA target voltage */
										max77968->ta_target_vol = max77968->ta_vol;
										pr_info("%s: CC adjust End(Normal): ta_ctrl=%d, ta_target_vol=%d\n",
												__func__, max77968->ta_ctrl, max77968->ta_target_vol);
										/* Clear TA increment flag */
										max77968->prev_inc = INC_NONE;
										/* Go to CC mode */
										mutex_lock(&max77968->lock);
										max77968->timer_id = TIMER_CHECK_CCMODE;
										max77968->timer_period = 0;
										mutex_unlock(&max77968->lock);
									}
								} else {
									/* Check the present TA current */
									/* Consider tolerance offset(100mA) */
									if (max77968->ta_cur >= (max77968->iin_cc + TA_IIN_OFFSET)) {
										/* TA voltage increment has high priority than TA current increment */
										/* Try to increase TA voltage(40mV) */
#if IS_ENABLED(CONFIG_SEC_FACTORY)
										val = max77968->iin_cc - iin;
										val = (val * 4)/10;
										if (val > FACTORY_TA_VOL_STEP_ADJ_CC_MAX)
											val = FACTORY_TA_VOL_STEP_ADJ_CC_MAX;
										else if (val < TA_VOL_STEP_ADJ_CC)
											val = TA_VOL_STEP_ADJ_CC;
										max77968->ta_vol = max77968->ta_vol + (int)val;
										val = max77968->ta_vol / PD_MSG_TA_VOL_STEP;
										max77968->ta_vol = val * PD_MSG_TA_VOL_STEP;
#else
										if (vbatt >= BAT_VOL_TH_TO_ADJ_CC_MIN_STEP)
											max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
										else
											max77968->ta_vol = max77968->ta_vol + TA_VOL_STEP_ADJ_CC * max77968->chg_mode;
#endif
										if (max77968->ta_vol > max77968->ta_max_vol)
											max77968->ta_vol = max77968->ta_max_vol;
										pr_info("%s: CC adjust Cont(5): iin=%d, ta_vol=%d\n",
												__func__, iin, max77968->ta_vol);

										/* Set TA increment flag */
										max77968->prev_inc = INC_TA_VOL;
										/* Send PD Message */
										mutex_lock(&max77968->lock);
										max77968->timer_id = TIMER_PDMSG_SEND;
										max77968->timer_period = 0;
										mutex_unlock(&max77968->lock);
									} else {
										/* TA has tolerance and compensate it as real current */
										/* Increase TA current(50mA) */
										max77968->ta_cur = max77968->ta_cur + PD_MSG_TA_CUR_STEP;
										if (max77968->ta_cur > max77968->ta_max_cur)
											max77968->ta_cur = max77968->ta_max_cur;
										pr_info("%s: CC adjust Cont(6): iin=%d, ta_cur=%d\n",
												__func__, iin, max77968->ta_cur);

										/* Set TA increment flag */
										max77968->prev_inc = INC_TA_CUR;
										/* Send PD Message */
										mutex_lock(&max77968->lock);
										max77968->timer_id = TIMER_PDMSG_SEND;
										max77968->timer_period = 0;
										mutex_unlock(&max77968->lock);
									}
								}
							}
						}
					}
				}
			}
		}
		/* Save previous iin adc */
		max77968->prev_iin = iin;
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		break;

	default:
		break;
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* 2:1 Direct Charging Start CC MODE control - Pre CC MODE */
/* Increase TA voltage to TA target voltage */
static int max77968_charge_start_ccmode(struct max77968_charger *max77968)
{
	int ret = 0;
	int ccmode, vbatt, val;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_START_CC);
#else
	max77968->charging_state = DC_STATE_START_CC;
#endif

	/* Enable VBATT regulation threshold to default (4.45V) */
	ret = max77968_set_vfloat(max77968, VBAT_REG_DFT);
	if (ret < 0)
		goto error;

	/* Make sure VBATT Regulation enabled */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		goto error;

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	ret = max77968_check_error(max77968);
	if (ret != 0)
		goto error; // This is not active mode.

	/* Check the status */
	ccmode = max77968_check_dcmode_status(max77968);
	if (ccmode < 0) {
		ret = ccmode;
		goto error;
	}

	vbatt = max77968_read_adc(max77968, ADC_CH_VBATT);

	/* Increase TA voltage */
	if (vbatt >= BAT_VOL_TH_TO_ADJ_CC_MIN_STEP)
		max77968->ta_vol = max77968->ta_vol + PD_MSG_TA_VOL_STEP;
	else
		max77968->ta_vol = max77968->ta_vol + TA_VOL_STEP_PRE_CC * max77968->chg_mode;

	/* Check TA target voltage */
	if (max77968->ta_vol >= max77968->ta_target_vol) {
		max77968->ta_vol = max77968->ta_target_vol;
		pr_info("%s: PreCC End: ta_vol=%d, ta_target_vol=%d\n", __func__, max77968->ta_vol, max77968->ta_target_vol);

		/* Change to DC state to CC mode */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968_set_charging_state(max77968, DC_STATE_CC_MODE);
#else
		max77968->charging_state = DC_STATE_CC_MODE;
#endif
	} else {
		pr_info("%s: PreCC Cont: ta_vol=%d\n", __func__, max77968->ta_vol);
	}

	/* Send PD Message */
	mutex_lock(&max77968->lock);
	max77968->timer_id = TIMER_PDMSG_SEND;
	max77968->timer_period = 0;
	mutex_unlock(&max77968->lock);

	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* 2:1 Direct Charging CC MODE control */
static int max77968_charge_ccmode(struct max77968_charger *max77968)
{
	int ret = 0;
	int ccmode;
	int iin, val;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_CC_MODE);
#else
	max77968->charging_state = DC_STATE_CC_MODE;
#endif

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		if (max77968->force_vbat_reg_off) {
			ret = max77968_set_vbatt_regu_enable(max77968, false);
			pr_info("%s: CC Moode, Set VBATT_REG_EN=0 once\n", __func__);
		} else
			ret = max77968_vbatt_regu_enable_chk(max77968);

		if (ret < 0)
			goto error;
	}

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	/* Check the charging type */
	ret = max77968_check_error(max77968);
	if (ret < 0) {
		if (ret == -EAGAIN) {
			/* DC error happens, but it is retry case */
			if (max77968->ta_ctrl == TA_CTRL_CL_MODE) {
				/* Current TA control method is Current Limit mode */
				/* Retry DC as Constant Voltage mode */
				pr_info("%s: Retry DC : ta_ctrl=%d\n", __func__, max77968->ta_ctrl);

				/* Disable charging */
				ret = max77968_set_charging(max77968, false);
				if (ret < 0)
					goto error;

				/* Softreset */
				ret = max77968_softreset(max77968);
				if (ret < 0)
					goto error;

				/* Set SCC to standby state*/
				ret = max77968_set_standby_state(max77968, true);
				if (ret < 0)
					goto error;

				/* Set TA control method to Constant Voltage mode */
				max77968->ta_ctrl = TA_CTRL_CV_MODE;

				/* Go to DC_STATE_PRESET_DC */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PRESET_DC;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
									&max77968->timer_work,
									msecs_to_jiffies(max77968->timer_period));
				ret = 0;
				goto error;
			} else {
				/* Current TA control method is Constant Voltage mode */
				/* Don't retry DC */
				pr_info("%s: Retry DC, but still failed - stop DC\n", __func__);
				goto error;
			}
		} else {
			/* Don't retry DC */
			goto error;
		}
	}

	/* Check new request */
	if (max77968->req_new_byp_mode == true) {
		pr_info("%s: max77968_set_new_byp_mode\n", __func__);
		ret = max77968_set_new_byp_mode(max77968);
	} else if ((max77968->req_new_chg_mode == true) ||
		(max77968->new_chg_mode_buf_has_data == true)) {
		pr_info("%s: max77968_set_new_chg_mode\n", __func__);
		ret = max77968_set_new_chg_mode(max77968);
	} else if ((max77968->req_new_iin == true) ||
		(max77968->new_iin_buf_has_data == true)) {
		pr_info("%s: max77968_set_new_iin\n", __func__);
		ret = max77968_set_new_iin(max77968);
	} else if (max77968->req_new_vfloat == true) {
		pr_info("%s: max77968_set_new_vfloat\n", __func__);
		ret = max77968_set_new_vfloat(max77968);
	} else {
		/* Check the charging type */
		ccmode = max77968_check_dcmode_status(max77968);
		if (ccmode < 0) {
			ret = ccmode;
			goto error;
		}

		switch (ccmode) {
		case DCMODE_LOOP_INACTIVE:
			/* Set input current compensation */
			/* Check the TA type */
			if (max77968->ta_type == TA_TYPE_WIRELESS) {
				/* Need RX voltage compensation */
				ret = max77968_set_rx_voltage_comp(max77968);
				pr_info("%s: CC INACTIVE: rx_vol=%d\n", __func__, max77968->ta_vol);
			} else {
				/* Check the current TA current with TA_MIN_CUR */
				if ((max77968->ta_cur <= TA_MIN_CUR) ||
					(max77968->ta_ctrl == TA_CTRL_CV_MODE)) {
					/* Need input voltage compensation */
					ret = max77968_set_ta_voltage_comp(max77968);
				} else {
					if (max77968->ta_max_vol >= TA_MAX_VOL_CP) {
						/* This TA can support the input current without power limit */
						/* Need input current compensation */
						ret = max77968_set_ta_current_comp(max77968);
					} else {
						/* This TA has the power limitation for the input current compenstaion */
						/* The input current cannot increase over the constant power */
						/* Need input current compensation in constant power mode */
						ret = max77968_set_ta_current_comp2(max77968);
					}
				}
				pr_info("%s: CC INACTIVE: ta_cur=%d, ta_vol=%d\n", __func__, max77968->ta_cur, max77968->ta_vol);
			}
			break;

		case DCMODE_VFLT_LOOP:
			/* Read IIN_ADC */
#if defined(ISSUE_WORKAROUND)
			if (max77968->adc_wr_en)
				iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
			else
				iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
			pr_info("%s: CC VFLOAT: iin=%d\n", __func__, iin);
			/* go to Pre-CV mode */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_ENTER_CVMODE;
			if (max77968->ta_type == TA_TYPE_WIRELESS)
				max77968->timer_period = RXMSG_WAIT_T;
			else
				max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			break;

		case DCMODE_IIN_LOOP:
			/* Read IIN_ADC */
#if defined(ISSUE_WORKAROUND)
			if (max77968->adc_wr_en)
				iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
			else
				iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
			/* Check the TA type */
			if (max77968->ta_type == TA_TYPE_WIRELESS) {
				/* Wireless Charger is connected */
				/* Decrease RX voltage (12.5mV) */
				max77968->ta_vol = max77968->ta_vol - WCRX_VOL_STEP;
				pr_info("%s: CC LOOP(WC):iin=%d, next_rx_vol=%d\n", __func__, iin, max77968->ta_vol);
			} else {
				/* USBPD TA is connected */

				/* Check the current TA current with TA_MIN_CUR */
				if ((max77968->ta_cur <= TA_MIN_CUR) ||
					(max77968->ta_ctrl == TA_CTRL_CV_MODE)) {
					/* Decrease TA voltage (20mV) */
					max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
					pr_info("%s: CC LOOP(1):iin=%d, next_ta_vol=%d\n", __func__, iin, max77968->ta_vol);
				} else {
					/* Check TA current and compare it with IIN_CC */
					if (max77968->ta_cur <= max77968->iin_cc - TA_CUR_LOW_OFFSET) {
						/* IIN_LOOP still happens even though TA current is less than IIN_CC - 200mA */
						/* TA has abnormal behavior */
						/* Decrease TA voltage (20mV) */
						max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
						pr_info("%s: CC LOOP(2):iin=%d, ta_cur=%d, next_ta_vol=%d\n",
								__func__, iin, max77968->ta_cur, max77968->ta_vol);
						/* Update TA target voltage */
						max77968->ta_target_vol = max77968->ta_vol;
					} else {
						/* TA current is higher than IIN_CC - 200mA */
						/* Decrease TA current first to reduce input current */
						/* Decrease TA current (50mA) */
						max77968->ta_cur = max77968->ta_cur - PD_MSG_TA_CUR_STEP;
						pr_info("%s: CC LOOP(3):iin=%d, ta_vol=%d, next_ta_cur=%d\n",
								__func__, iin, max77968->ta_vol, max77968->ta_cur);
					}
				}
			}
			/* Send PD Message */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			if (max77968->ta_type == TA_TYPE_WIRELESS)
				max77968->timer_period = RXMSG_WAIT_T;
			else
				max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			break;

		default:
			break;
		}
	}
error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}


/* 2:1 Direct Charging Start CV MODE control - Pre CV MODE */
static int max77968_charge_start_cvmode(struct max77968_charger *max77968)
{
	int ret = 0;
	int cvmode = 0;
	int iin, vbat, val;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_START_CV);
#else
	max77968->charging_state = DC_STATE_START_CV;
#endif

	/* Enable VBATT regulation threshold to default (4.45V) */
	ret = max77968_set_vfloat(max77968, VBAT_REG_DFT);
	if (ret < 0)
		goto error;

	/* Make sure VBATT Regulation enabled */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		goto error;

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	/* Check the charging type */
	ret = max77968_check_error(max77968);
	if (ret != 0) {
		/* Check error type */
		if (ret == -ERROR_DCRCP) {
			/* Set dcmode to DCMODE_CHG_DONE */
			cvmode = DCMODE_CHG_DONE;
			pr_info("%s: dcmode is DCMODE_CHG_DONE by RCP\n", __func__);
		} else {
			/* DC error */
			goto error; // This is not active mode.
		}	}
	/* Check the status */
	if (cvmode != DCMODE_CHG_DONE) {
		cvmode = max77968_check_dcmode_status(max77968);
		if (cvmode < 0) {
			ret = cvmode;
			goto error;
		}

		/* Read IIN_ADC */
#if defined(ISSUE_WORKAROUND)
		if (max77968->adc_wr_en)
			iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
		else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
		/* Read VBAT_ADC */
		vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

		/* Store VBAT_ADC to previous vbat */
		max77968->prev_vbat = vbat;

		/* Check charging done state */
		/* Compare iin with input topoff current */
		pr_info("%s: iin=%d, iin_topoff=%d\n", __func__, iin, max77968->iin_topoff);
		if ((iin < max77968->iin_topoff) && (max77968->vfloat > TOP_OFF_MINIMUM_THRESHOLD)) {
			/* Change cvmode status to charging done */
			cvmode = DCMODE_CHG_DONE;
			pr_info("%s: start CVMODE Status=%d\n", __func__, cvmode);
		}
	}

	switch (cvmode) {
	case DCMODE_CHG_DONE:
		/* Charging Done */
		/* Keep start CV mode until battery driver send stop charging */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968_set_charging_state(max77968, DC_STATE_CHARGING_DONE);
#else
		max77968->charging_state = DC_STATE_CHARGING_DONE;
#endif
		pr_info("%s: Start CV - Charging Done\n", __func__);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968_set_done(max77968, true);
#endif

		/* Check the charging status after notification function */
		if (max77968->charging_state != DC_STATE_NO_CHARGING) {
			/* Notification function does not stop timer work yet */
			/* Keep the charging done state */
			/* Set timer */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_ENTER_CVMODE;
			max77968->timer_period = CVMODE_CHECK_T;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		} else {
			/* Already called stop charging by notification function */
			pr_info("%s: Already stop DC\n", __func__);
		}
		break;

	case DCMODE_IIN_LOOP:
		/* Check the TA type */
		if (max77968->ta_type == TA_TYPE_WIRELESS) {
			/* Decrease RX voltage (12.5mV) */
			max77968->ta_vol = max77968->ta_vol - WCRX_VOL_STEP;
			pr_info("%s: PreCV Cont(IIN_LOOP): rx_vol=%d\n", __func__, max77968->ta_vol);
		} else {
			/* Check TA current */
			if (max77968->ta_cur > TA_MIN_CUR) {
				/* TA current is higher than 1.0A */
				/* Decrease TA current (50mA) */
				max77968->ta_cur = max77968->ta_cur - PD_MSG_TA_CUR_STEP;
				pr_info("%s: PreCV Cont: ta_cur=%d\n", __func__, max77968->ta_cur);
			} else {
				/* TA current is less than 1.0A */
				/* Decrease TA voltage (20mV) */
				max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
				pr_info("%s: PreCV Cont(IIN_LOOP): ta_vol=%d\n", __func__, max77968->ta_vol);
			}
		}
		/* Send PD Message */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PDMSG_SEND;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		break;

	case DCMODE_VFLT_LOOP:
		/* Check the TA type */
		if (max77968->ta_type == TA_TYPE_WIRELESS) {
			/* Decrease RX voltage (12.5mV) */
			max77968->ta_vol = max77968->ta_vol - WCRX_VOL_STEP;
			pr_info("%s: PreCV Cont: rx_vol=%d\n", __func__, max77968->ta_vol);
		} else {
			/* Decrease TA voltage (20mV)*chg_mode */
			max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP * max77968->chg_mode;
			pr_info("%s: PreCV Cont(20mV): ta_vol=%d\n", __func__, max77968->ta_vol);
		}
		/* Send PD Message */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PDMSG_SEND;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		break;

	case DCMODE_LOOP_INACTIVE:
		/* Exit Pre CV mode */
		pr_info("%s: PreCV End: ta_vol=%d, ta_cur=%d\n", __func__, max77968->ta_vol, max77968->ta_cur);

		/* Set TA target voltage to TA voltage */
		max77968->ta_target_vol = max77968->ta_vol;

		/* Need to implement notification to other driver */
		/* To do here */

		/* Go to CV mode */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_CHECK_CVMODE;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		break;

	default:
		break;
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* 2:1 Direct Charging CV MODE control */
static int max77968_charge_cvmode(struct max77968_charger *max77968)
{
	int ret = 0;
	int cvmode = 0;
	int iin, vbat, vout, val;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_CV_MODE);
#else
	max77968->charging_state = DC_STATE_CV_MODE;
#endif

	if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		if (max77968->force_vbat_reg_off) {
			ret = max77968_set_vbatt_regu_enable(max77968, false);
			pr_info("%s: CV Moode, Set VBATT_REG_EN=0\n", __func__);
		} else
			ret = max77968_vbatt_regu_enable_chk(max77968);

		if (ret < 0)
			goto error;
	}

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	ret = max77968_check_error(max77968);
	if (ret != 0) {
		/* Check error type */
		if (ret == -ERROR_DCRCP) {
			/* Set dcmode to DCMODE_CHG_DONE */
			cvmode = DCMODE_CHG_DONE;
			pr_info("%s: dcmode is DCMODE_CHG_DONE by RCP\n", __func__);
		} else {
			/* DC error */
			goto error; // This is not active mode.
		}
	}
	/* Check new request */
	if (max77968->req_new_byp_mode == true) {
		pr_info("%s: max77968_set_new_byp_mode\n", __func__);
		ret = max77968_set_new_byp_mode(max77968);
	} else if ((max77968->req_new_chg_mode == true) ||
		(max77968->new_chg_mode_buf_has_data == true)) {
		pr_info("%s: max77968_set_new_chg_mode\n", __func__);
		ret = max77968_set_new_chg_mode(max77968);
	} else if ((max77968->req_new_iin == true) ||
		(max77968->new_iin_buf_has_data == true)) {
		pr_info("%s: max77968_set_new_iin\n", __func__);
		ret = max77968_set_new_iin(max77968);
	} else if (max77968->req_new_vfloat == true) {
		pr_info("%s: max77968_set_new_vfloat\n", __func__);
		ret = max77968_set_new_vfloat(max77968);
	} else {
		/* Check the status */
		if (cvmode != DCMODE_CHG_DONE) {
			cvmode = max77968_check_dcmode_status(max77968);
			if (cvmode < 0) {
				ret = cvmode;
				goto error;
			}

			/* Read IIN_ADC */
#if defined(ISSUE_WORKAROUND)
			if (max77968->adc_wr_en)
				iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
			else
				iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
			/* Read VBAT_ADC */
			vbat = max77968_read_adc(max77968, ADC_CH_VBATT);
			/* Read VOUT_ADC */
			vout = max77968_read_adc(max77968, ADC_CH_VOUT);

			/* Store VBAT_ADC to previous vbat */
			max77968->prev_vbat = vbat;

			/* Check charging done state */
			if (cvmode == DCMODE_LOOP_INACTIVE) {
				/* Compare iin with input topoff current */
				pr_info("%s: iin=%d, iin_topoff=%d, vout=%d\n",
						__func__, iin, max77968->iin_topoff, vout);
				if (iin < max77968->iin_topoff) {
					/* Change cvmode status to charging done */
					cvmode = DCMODE_CHG_DONE;
					pr_info("%s: CVMODE Status=%d\n", __func__, cvmode);
				}
			}
		}
		switch (cvmode) {
		case DCMODE_CHG_DONE:
			/* Charging Done */
			/* Keep CV mode until battery driver send stop charging */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_charging_state(max77968, DC_STATE_CHARGING_DONE);
#else
			max77968->charging_state = DC_STATE_CHARGING_DONE;
#endif
			/* Need to implement notification function */
			/* A customer should insert code */

			pr_info("%s: CV Done\n", __func__);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_done(max77968, true);
#endif
			/* Check the charging status after notification function */
			if (max77968->charging_state != DC_STATE_NO_CHARGING) {
				/* Notification function does not stop timer work yet */
				/* Keep the charging done state */
				/* Set timer */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_CVMODE;

				/* Add to check charging step and set the polling time */
				if (max77968->vfloat < max77968->pdata->step1_vth) {
					/* Step1 charging - polling time is cv_polling */
					max77968->timer_period = max77968->pdata->cv_polling;
				} else if ((max77968->dec_vfloat == true) || (max77968->vfloat >= max77968->max_vfloat)) {
					/* present vfloat is lower than previous vfloat or
					 * present vfloat is maximum vfloat
					 * pollig time is CVMODE_CHECK2_T
					 */
					max77968->timer_period = CVMODE_CHECK2_T;
				} else {
					/* Step2 or 3 charging - polling time is CVMODE_CHECK_T */
					max77968->timer_period = CVMODE_CHECK_T;
				}
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
									&max77968->timer_work,
									msecs_to_jiffies(max77968->timer_period));
			} else {
				/* Already called stop charging by notification function */
				pr_info("%s: Already stop DC\n", __func__);
			}
			break;

		case DCMODE_IIN_LOOP:
			/* Check the TA type */
			if (max77968->ta_type == TA_TYPE_WIRELESS) {
				/* Decrease RX Voltage (12.5mV) */
				max77968->ta_vol = max77968->ta_vol - WCRX_VOL_STEP;
				pr_info("%s: CV LOOP(WC), Cont: iin=%d, rx_vol=%d, vout=%d\n",
						__func__, iin, max77968->ta_vol, vout);
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PDMSG_SEND;
				max77968->timer_period = RXMSG_WAIT_T;
				mutex_unlock(&max77968->lock);
			} else {
				/* Check TA current */
				if (max77968->ta_cur > TA_MIN_CUR) {
					/* TA current is higher than (1.0A*chg_mode) */
					/* Check TA current and compare it with IIN_CC */
					if (max77968->ta_cur <= max77968->iin_cc - TA_CUR_LOW_OFFSET) {
						/* IIN_LOOP still happens even though TA current is less than IIN_CC - 200mA */
						/* TA has abnormal behavior */
						/* Decrease TA voltage (20mV) */
						max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
						pr_info("%s: CV LOOP(1):iin=%d, ta_cur=%d, next_ta_vol=%d, vout=%d\n",
								__func__, iin, max77968->ta_cur, max77968->ta_vol, vout);
						/* Update TA target voltage */
						max77968->ta_target_vol = max77968->ta_vol;
					} else {
						/* TA current is higher than IIN_CC - 200mA */
						/* Decrease TA current first to reduce input current */
						/* Decrease TA current (50mA) */
						max77968->ta_cur = max77968->ta_cur - PD_MSG_TA_CUR_STEP;
						pr_info("%s: CV LOOP(2):iin=%d, ta_vol=%d, next_ta_cur=%d, vout=%d\n",
								__func__, iin, max77968->ta_vol, max77968->ta_cur, vout);
					}
				} else {
					/* TA current is less than (1.0A*chg_mode) */
					/* Decrease TA Voltage (20mV) */
					max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
					pr_info("%s: CV LOOP(3), Cont: iin=%d, ta_vol=%d, vout=%d\n",
							__func__, iin, max77968->ta_vol, vout);
				}
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PDMSG_SEND;
				if (max77968->ta_type == TA_TYPE_WIRELESS)
					max77968->timer_period = RXMSG_WAIT_T;
				else
					max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
			}

			/* Send PD Message */
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			break;

		case DCMODE_VFLT_LOOP:
			/* Check the TA type */
			if (max77968->ta_type == TA_TYPE_WIRELESS) {
				/* Decrease RX voltage */
				max77968->ta_vol = max77968->ta_vol - WCRX_VOL_STEP;
				pr_info("%s: CV VFLOAT, Cont: iin=%d, rx_vol=%d, vout=%d\n",
						__func__, iin, max77968->ta_vol, vout);
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PDMSG_SEND;
				max77968->timer_period = RXMSG_WAIT_T;
				mutex_unlock(&max77968->lock);
			} else {
				/* Decrease TA voltage */
				max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
				pr_info("%s: CV VFLOAT, Cont: iin=%d, ta_vol=%d, vout=%d\n",
						__func__, iin, max77968->ta_vol, vout);
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PDMSG_SEND;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
			}

			/* Set TA target voltage to TA voltage */
			max77968->ta_target_vol = max77968->ta_vol;

			/* Send PD Message */
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			break;

		case DCMODE_LOOP_INACTIVE:
			/* Set timer */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_CVMODE;
			/* Add to check charging step and set the polling time */
			if (max77968->vfloat < max77968->pdata->step1_vth) {
				/* Step1 charging - polling time is cv_polling */
				max77968->timer_period = max77968->pdata->cv_polling;
			} else if ((max77968->dec_vfloat == true) || (max77968->vfloat >= max77968->max_vfloat)) {
				/* present vfloat is lower than previous vfloat or
				 * present vfloat is maximum vfloat
				 * pollig time is CVMODE_CHECK2_T
				 */
				max77968->timer_period = CVMODE_CHECK2_T;
			} else {
				/* Step2 or 3 charging - polling time is CVMODE_CHECK_T */
				max77968->timer_period = CVMODE_CHECK_T;
			}
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			break;

		default:
			break;
		}
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* 2:1 Direct Charging FPDO CV MODE control */
static int max77968_charge_fpdo_cvmode(struct max77968_charger *max77968)
{
	int ret = 0;
	int cvmode;
	int iin, reg;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	union power_supply_propval val;
#endif

	pr_info("%s: ======START=======\n", __func__);

	max77968->charging_state = DC_STATE_FPDO_CV_MODE;

	/* Make sure VBATT Regulation enabled */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		goto error;

	ret = max77968_get_vbatt_regu_enable(max77968, &reg);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, reg);

	ret = max77968_check_error(max77968);
	if (ret != 0)
		goto error; // This is not active mode.

	/* Protect access to req_new_vfloat */
	mutex_lock(&max77968->lock);

	/* Check new request */
	if (max77968->req_new_vfloat == true) {
		/* Set VFLOAT to new vfloat */
		max77968->vfloat = max77968->new_vfloat;
		ret = max77968_set_vfloat(max77968, max77968->vfloat);
		if (ret < 0) {
			mutex_unlock(&max77968->lock);
			goto error;
		}
		/* Clear req_new_vfloat */
		max77968->req_new_vfloat = false;
		mutex_unlock(&max77968->lock);
	} else {
		mutex_unlock(&max77968->lock);
		cvmode = max77968_check_dcmode_status(max77968);
		if (cvmode < 0) {
			ret = cvmode;
			goto error;
		}

		/* Read IIN_ADC */
#if defined(ISSUE_WORKAROUND)
		if (max77968->adc_wr_en)
			iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
		else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
		/* Check charging done state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		psy_do_property("battery", get, POWER_SUPPLY_PROP_VOLTAGE_NOW, val);

		if (cvmode == DCMODE_LOOP_INACTIVE || val.intval >= max77968->fpdo_dc_vnow_topoff) {
			/* Compare iin with input topoff current */
			pr_info("%s: iin=%d, vnow=%d, fpdo_dc_iin_topoff=%d, fpdo_dc_vnow_topoff=%d\n",
					__func__, iin, val.intval,
					max77968->fpdo_dc_iin_topoff, max77968->fpdo_dc_vnow_topoff);
			if (val.intval >= max77968->fpdo_dc_vnow_topoff || iin < max77968->fpdo_dc_iin_topoff) {
#else
		if (cvmode == DCMODE_LOOP_INACTIVE) {
			/* Compare iin with input topoff current */
			pr_info("%s: iin=%d, iin_topoff=%d\n",
					__func__, iin, max77968->iin_topoff);
			if (iin < max77968->iin_topoff) {
#endif
				/* Check charging done counter */
				if (max77968->done_cnt < FPDO_DONE_CNT) {
					/* Keep cvmode status */
					pr_info("%s: Keep FPDO CVMODE Status=%d\n", __func__, cvmode);
					/* Increase charging done counter */
					max77968->done_cnt++;
				} else {
					/* Change cvmode status to charging done */
					cvmode = DCMODE_CHG_DONE;
					pr_info("%s: FPDO_CVMODE Status=%d\n", __func__, cvmode);
					/* Clear charging done counter */
					max77968->done_cnt = 0;
				}
			} else {
				/* Clear charging done counter */
				max77968->done_cnt = 0;
			}
		}

		switch (cvmode) {
		case DCMODE_CHG_DONE:
			/* Charging Done */
			/* Keep FPDO CV mode until battery driver send stop charging */
			max77968->charging_state = DC_STATE_CHARGING_DONE;
			/* Need to implement notification function */
			/* A customer should insert code */

			pr_info("%s: FPDO CV Done\n", __func__);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_done(max77968, true);
#endif

			/* Check the charging status after notification function */
			if (max77968->charging_state != DC_STATE_NO_CHARGING) {
				/* Notification function does not stop timer work yet */
				/* Keep the charging done state */
				/* Set timer */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_FPDOCVMODE;
				max77968->timer_period = CVMODE_CHECK2_T;
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));
			} else {
				/* Already called stop charging by notification function */
				pr_info("%s: Already stop DC\n", __func__);
			}
			break;

		case DCMODE_IIN_LOOP:
			/* IIN_LOOP happens */
			pr_info("%s: FPDO CV IIN_LOOP\n", __func__);
			/* Need to stop DC by battery driver */

			/* Need to implement notification function */
			/* A customer should insert code */
			/* To do here */

			/* Check the charging status after notification function */
			if (max77968->charging_state != DC_STATE_NO_CHARGING) {
				/* Notification function does not stop timer work yet */
				/* Keep the current state */
				/* Set timer - 1s */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_FPDOCVMODE;
				max77968->timer_period = CVMODE_CHECK2_T;
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));
			} else {
				/* Already called stop charging by notification function */
				pr_info("%s: Already stop DC\n", __func__);
			}
			break;

		case DCMODE_VFLT_LOOP:
			/* VFLOAT_LOOP happens */
			pr_info("%s: FPDO CV VFLOAT_LOOP\n", __func__);
			/* Need to stop DC and transit to switching charger by battery driver */

			/* Need to implement notification function */
			/* A customer should insert code */
			/* To do here */

			/* Check the charging status after notification function */
			if (max77968->charging_state != DC_STATE_NO_CHARGING) {
				/* Notification function does not stop timer work yet */
				/* Keep the current state */
				/* Set timer */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_CHECK_FPDOCVMODE;
				max77968->timer_period = CVMODE_CHECK2_T;
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));
			} else {
				/* Already called stop charging by notification function */
				pr_info("%s: Already stop DC\n", __func__);
			}
			break;

		case DCMODE_LOOP_INACTIVE:
			/* Set timer */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_FPDOCVMODE;
			max77968->timer_period = CVMODE_CHECK3_T;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
					&max77968->timer_work,
					msecs_to_jiffies(max77968->timer_period));
			break;

		default:
			break;
		}
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Direct Charging Bypass Mode Control */
static int max77968_charge_bypass_mode(struct max77968_charger *max77968)
{
	int ret = 0;
	int dc_status;
	int vbat, iin, val;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_BYPASS_MODE);
#else
	max77968->charging_state = DC_STATE_BYPASS_MODE;
#endif

	/* Make sure VBATT Regulation enabled */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		goto error;

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	ret = max77968_check_error(max77968);
	if (ret < 0)
		goto error;	// This is not active mode.

	/* Check new request */
	if (max77968->req_new_byp_mode == true) {
		pr_info("%s: max77968_set_new_byp_mode\n", __func__);
		ret = max77968_set_new_byp_mode(max77968);
	} else if ((max77968->req_new_iin == true) ||
		(max77968->new_iin_buf_has_data == true)) {
		pr_info("%s: max77968_set_new_iin\n", __func__);
		ret = max77968_set_new_iin(max77968);
	} else if (max77968->req_new_vfloat == true) {
		pr_info("%s: max77968_set_new_vfloat\n", __func__);
		ret = max77968_set_new_vfloat(max77968);
	} else {
		dc_status = max77968_check_dcmode_status(max77968);
		if (dc_status < 0) {
			ret = dc_status;
			goto error;
		}

		/* Read IIN ADC */
#if defined(ISSUE_WORKAROUND)
		if (max77968->adc_wr_en)
			iin = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
		else
			iin = max77968_read_adc(max77968, ADC_CH_IIN);
#else
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
#endif
		/* Read VBAT ADC */
		vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

		pr_info("%s: iin=%d, vbat=%d\n", __func__, iin, vbat);

		if (dc_status == DCMODE_IIN_LOOP) {
			/* Decrease input current */
			/* Check TA current and compare it with IIN_CC */
			if (max77968->ta_cur <= max77968->iin_cc - TA_CUR_LOW_OFFSET) {
				/* IIN_LOOP still happens even though TA current is less than IIN_CC - 200mA */
				/* TA has abnormal behavior */
				/* Decrease TA voltage (20mV) */
				max77968->ta_vol = max77968->ta_vol - PD_MSG_TA_VOL_STEP;
				pr_info("%s: IIN LOOP:iin=%d, ta_cur=%d, next_ta_vol=%d\n",
						__func__, iin, max77968->ta_cur, max77968->ta_vol);
			} else {
				/* TA current is higher than IIN_CC - 200mA */
				/* Decrease TA current first to reduce input current */
				/* Decrease TA current (50mA) */
				max77968->ta_cur = max77968->ta_cur - PD_MSG_TA_CUR_STEP;
				pr_info("%s: IIN LOOP:iin=%d, next_ta_cur=%d\n",
						__func__, iin, max77968->ta_cur);
			}

			/* Send PD Message */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PDMSG_SEND;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		} else {
			/* Ignore other status */
			/* Keep Bypass mode */
			pr_info("%s: Bypass mode, status=%d, ta_cur=%d, ta_vol=%d\n",
					__func__, dc_status, max77968->ta_cur, max77968->ta_vol);
			/* Set timer - 10s */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_BYPASSMODE;
			max77968->timer_period = BYPMODE_CHECK_T;	/* 10s */
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		}
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Direct Charging DC mode Change Control */
static int max77968_charge_dcmode_change(struct max77968_charger *max77968)
{
	int ret = 0, val;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_DCMODE_CHANGE);
#else
	max77968->charging_state = DC_STATE_DCMODE_CHANGE;
#endif

	/* Make sure VBATT Regulation enabled */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		return ret;

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		return ret;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	ret = max77968_set_new_byp_mode(max77968);

	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Preset TA voltage and current for Direct Charging Mode */
static int max77968_preset_dcmode(struct max77968_charger *max77968)
{
	int vbat;
	unsigned int val;
	int ret = 0;
	int chg_mode;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_PRESET_DC);
#else
	max77968->charging_state = DC_STATE_PRESET_DC;
#endif

	/* Check TA type */
	if (max77968->ta_type == TA_TYPE_USBPD_20) {

#if defined(EXT_SW_CONFIG_6)
		// EXT1 SW, auto mode
		val = 0x01 << MASK2SHIFT(EXT1_SW_CTRL1);
		val |= 0x00 << MASK2SHIFT(EXT1_SW_CTRL2);
		ret = max77968_update_reg(max77968, EXT1_SW_CTRL_REG, EXT1_SW_CTRL1 | EXT1_SW_CTRL2, val);
		if (ret < 0)
			return ret;

		// EXT2 SW, Manual control, Disable
		val = 0x00 << MASK2SHIFT(EXT2_SW_CTRL1);
		val |= 0x00 << MASK2SHIFT(EXT2_SW_CTRL2);
		ret = max77968_update_reg(max77968, EXT2_SW_CTRL_REG, EXT2_SW_CTRL1 | EXT2_SW_CTRL2, val);
		if (ret < 0)
			return ret;
#endif
		/* TA type is USBPD 2.0 and support only FPDO */
		pr_info("%s: ta type : fixed PDO\n", __func__);

		/* Set PDO object position to 9V FPDO */
		max77968->ta_objpos = 2;
		/* Set TA voltage to 9V */
		max77968->ta_vol = 9000000;
		/* Set TA maximum voltage to 9V */
		max77968->ta_max_vol = 9000000;
		/* Set IIN_CC to iin_cfg */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968->iin_cc = max77968->pdata->fpdo_dc_iin_lowest_limit;
#else
		max77968->iin_cc = max77968->iin_cfg;
#endif
		/* Set TA operating current and maximum current to iin_cc */
		max77968->ta_cur = max77968->iin_cc;
		max77968->ta_max_cur = max77968->iin_cc;
		/* Calculate TA maximum power */
		max77968->ta_max_pwr = (max77968->ta_max_vol/DENOM_U_M)*(max77968->ta_max_cur/DENOM_U_M);

		max77968->chg_mode = CHG_2TO1_DC_MODE;

		pr_info("%s: Preset DC(FPDO), ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, chg_mode=%d\n",
			__func__, max77968->ta_max_vol, max77968->ta_max_cur, max77968->ta_max_pwr, max77968->iin_cc, max77968->chg_mode);
	} else {
		/* Read VBAT ADC */
		vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

		if (vbat < 0) {
			ret = vbat;
			goto error;
		}

		/* Compare VBAT with VBAT ADC */
		if (vbat > max77968->vfloat)	{
			/* Warn "Invalid battery voltage to start direct charging" */
			pr_err("%s: Warning - vbat adc(%duV) is higher than VFLOAT(%duV)\n",
					__func__, vbat, max77968->vfloat);
		} else {
			pr_info("%s: info - vbat adc(%duV) is lower than VFLOAT(%duV)\n",
					__func__, vbat, max77968->vfloat);
		}

		/* Check minimum VBAT level */
		if (vbat <= DC_VBAT_MIN_ERR) {
			/* Invalid battery level to start direct charging */
			pr_err("%s: This vbat(%duV) will make VIN_OV_TRACKING error\n", __func__, vbat);
			ret = -EINVAL;
			goto error;
		}

		/* Apply the chg_mode and fsw_cfg settings*/
		mutex_lock(&max77968->lock);
		max77968->chg_mode = max77968->pdata->chg_mode;
		//max77968->fsw_cfg = max77968->pdata->fsw_cfg;
		mutex_unlock(&max77968->lock);

		/* Check the TA type and set the charging mode */
		if (max77968->ta_type == TA_TYPE_WIRELESS) {

#if defined(EXT_SW_CONFIG_6)
			// EXT1 SW,  Manual control, Disable
			val = 0x00 << MASK2SHIFT(EXT1_SW_CTRL1);
			val |= 0x00 << MASK2SHIFT(EXT1_SW_CTRL2);
			ret = max77968_update_reg(max77968, EXT1_SW_CTRL_REG, EXT1_SW_CTRL1 | EXT1_SW_CTRL2, val);
			if (ret < 0)
				return ret;

			// EXT2 SW, Manual control, Enable
			val = 0x00 << MASK2SHIFT(EXT2_SW_CTRL1);
			val |= 0x01 << MASK2SHIFT(EXT2_SW_CTRL2);
			ret = max77968_update_reg(max77968, EXT2_SW_CTRL_REG, EXT2_SW_CTRL1 | EXT2_SW_CTRL2, val);
			if (ret < 0)
				return ret;
#endif

			/* Wireless Charger is connected */
			/* Set the RX max current to input request current(iin_cfg) initially */
			/* to get RX maximum current from RX IC */
			max77968->ta_max_cur = max77968->iin_cfg;
			/* Set the RX max voltage to enough high value to find RX maximum voltage initially */
			//max77968->ta_max_vol = WCRX_MAX_VOL * max77968->pdata->chg_mode;
			max77968->ta_max_vol = WCRX_MAX_VOL * max77968->chg_mode;
			/* Get the RX max current/voltage(RX_MAX_CUR/VOL) */
			ret = max77968_get_rx_max_power(max77968);
			if (ret < 0) {
				/* RX IC does not have the desired maximum voltage */
				/* Check the desired mode */
				// if (max77968->pdata->chg_mode == CHG_3TO1_DC_MODE) {
				if (max77968->chg_mode == CHG_3TO1_DC_MODE) {
					/* RX IC doesn't have any maximum voltage to support 3:1 mode */
					/* Get the RX max current/voltage with 2:1 mode */
					max77968->ta_max_vol = WCRX_MAX_VOL;
					ret = max77968_get_rx_max_power(max77968);
					if (ret < 0) {
						pr_err("%s: RX IC doesn't have any RX voltage to support 2:1 or 3:1\n",
								__func__);
						max77968->chg_mode = CHG_NO_DC_MODE;
						goto error;
					} else {
						/* RX IC has the maximum RX voltage to support 2:1 mode */
						max77968->chg_mode = CHG_2TO1_DC_MODE;
					}
				} else {
					/* The desired CHG mode is 2:1 mode */
					/* RX IC doesn't have any RX voltage to support 2:1 mode*/
					pr_err("%s: RX IC doesn't have any RX voltage to support 2:1\n", __func__);
					max77968->chg_mode = CHG_NO_DC_MODE;
					goto error;
				}
			}

			chg_mode = max77968->chg_mode;

			/* Set IIN_CC to MIN[IIN, (RX_MAX_CUR by RX IC)*chg_mode]*/
			max77968->iin_cc = MIN(max77968->iin_cfg, (max77968->ta_max_cur*chg_mode));
			/* Set the current IIN_CC to iin_cfg */
			max77968->iin_cfg = max77968->iin_cc;

			/* Set RX voltage to MAX[(VBAT_ADC*chg_mode + offset), TA_MIN_VOL_PRESET*chg_mode] */
			max77968->ta_vol = max(TA_MIN_VOL_PRESET*chg_mode, (vbat*chg_mode + TA_VOL_PRE_OFFSET));

			if ((max77968->ss_fault_retry_cnt > 0) ||
				(max77968->preset_ta_fault_retry_cnt > 0)) {

				/* Soft start UCP/UCP retry, add 40mV to TA voltage setting */
				if (max77968->ss_fault_inc_ta_volt)
					max77968->ta_vol += TA_VOL_PRE_OFFSET_RETRY_INC*max77968->ss_fault_retry_cnt;

				/* Preset TA fault, TA voltage is lower than low limit */
				if (max77968->preset_ta_fault_inc_ta_volt)
					max77968->ta_vol += TA_VOL_PRE_OFFSET_RETRY_INC*max77968->preset_ta_fault_retry_cnt;

			} else if (max77968->preset_ta_vol_dec_once) {
				/* Decrease TA volt by 60mV if VIN is hiher than TA volt setting */
				max77968->ta_vol = max77968->ta_vol - TA_PRESET_DEC_STEP;
			}

			val = max77968->ta_vol/WCRX_VOL_STEP;	/* RX voltage resolution is 12.5mV */
			max77968->ta_vol = val*WCRX_VOL_STEP;

			/* Set RX voltage to MIN[RX voltage, RX_MAX_VOL*chg_mode] */
			max77968->ta_vol = MIN(max77968->ta_vol, max77968->ta_max_vol);

			pr_info("%s: Preset DC, rx_max_vol=%d, rx_max_cur=%d, rx_max_pwr=%d, iin_cc=%d, chg_mode=%d\n",
					__func__, max77968->ta_max_vol, max77968->ta_max_cur, max77968->ta_max_pwr,
					max77968->iin_cc, max77968->chg_mode);

			pr_info("%s: Preset DC, rx_vol=%d, ss_fault_retry_cnt=%d, preset_ta_fault_retry_cnt=%d\n",
				__func__, max77968->ta_vol, max77968->ss_fault_retry_cnt, max77968->preset_ta_fault_retry_cnt);

		} else {

#if defined(EXT_SW_CONFIG_6)
			// EXT1 SW, auto mode
			val = 0x01 << MASK2SHIFT(EXT1_SW_CTRL1);
			val |= 0x00 << MASK2SHIFT(EXT1_SW_CTRL2);
			ret = max77968_update_reg(max77968, EXT1_SW_CTRL_REG,
					EXT1_SW_CTRL1 | EXT1_SW_CTRL2, val);
			if (ret < 0)
				return ret;

			// EXT2 SW, Manual control, Disable
			val = 0x00 << MASK2SHIFT(EXT2_SW_CTRL1);
			val |= 0x00 << MASK2SHIFT(EXT2_SW_CTRL2);
			ret = max77968_update_reg(max77968, EXT2_SW_CTRL_REG,
					EXT2_SW_CTRL1 | EXT2_SW_CTRL2, val);
			if (ret < 0)
				return ret;
#endif

			/* USBPD TA is connected */
			/* Set the TA max current to input request current(iin_cfg) initially */
			/* to get TA maximum current from PD IC */
			max77968->ta_max_cur = max77968->iin_cfg;
			/* Set the TA max voltage to enough high value to find TA maximum voltage initially */
			//max77968->ta_max_vol = TA_MAX_VOL * max77968->pdata->chg_mode;
			max77968->ta_max_vol = TA_MAX_VOL * max77968->chg_mode;
			/* Search the proper object position of PDO */
			max77968->ta_objpos = 0;
			/* Get the APDO max current/voltage(TA_MAX_CUR/VOL) */
			ret = max77968_get_apdo_max_power(max77968);
			if (ret < 0) {
				/* TA does not have the desired APDO */
				/* Check the desired mode */
				//if (max77968->pdata->chg_mode == CHG_3TO1_DC_MODE) {
				if (max77968->chg_mode == CHG_3TO1_DC_MODE) {
					/* TA doesn't have any APDO to support 3:1 mode */
					/* Get the APDO max current/voltage with 2:1 mode */
					max77968->ta_max_vol = TA_MAX_VOL * CHG_2TO1_DC_MODE;
					max77968->ta_objpos = 0;
					ret = max77968_get_apdo_max_power(max77968);
					if (ret < 0) {
						pr_err("%s: TA doesn't have any APDO to support 2:1 or 3:1\n",
								__func__);
						max77968->chg_mode = CHG_NO_DC_MODE;
						goto error;
					} else {
						/* TA has APDO to support 2:1 mode */
						max77968->chg_mode = CHG_2TO1_DC_MODE;
					}
				} else {
					/* The desired TA mode is 2:1 mode */
					/* TA doesn't have any APDO to support 2:1 mode*/
					pr_err("%s: TA doesn't have any APDO to support 2:1\n", __func__);
					max77968->chg_mode = CHG_NO_DC_MODE;
					goto error;
				}
			}

			chg_mode = max77968->chg_mode;

			/* Calculate new TA maximum current and voltage that used in the direct charging */
			/* Set IIN_CC to MIN[IIN, (TA_MAX_CUR by APDO)*chg_mode]*/
			max77968->iin_cc = MIN(max77968->iin_cfg, (max77968->ta_max_cur));

			pr_info("%s: iin_cc=%d, iin_cfg=%d, ta_max_cur=%d\n", __func__, max77968->iin_cc, max77968->iin_cfg, max77968->ta_max_cur);

			/* Set the current IIN_CC to iin_cfg for recovering it after resolution adjustment */
			max77968->iin_cfg = max77968->iin_cc;

			/* Calculate new TA max voltage */
			/* Adjust IIN_CC with APDO resoultion(50mA) */
			/* - It will recover to the original value after max voltage calculation */
			val = max77968->iin_cc / PD_MSG_TA_CUR_STEP;
			max77968->iin_cc = val * PD_MSG_TA_CUR_STEP;

			/* Set TA_MAX_VOL to MIN[TA_MAX_VOL*chg_mode, TA_MAX_PWR/(IIN_CC/chg_mode)] */
			val = max77968->ta_max_pwr/(max77968->iin_cc / chg_mode / 1000);	/* mV */
			val = val * 1000 / PD_MSG_TA_VOL_STEP;	/* Adjust values with APDO resolution(20mV) */
			val = val * PD_MSG_TA_VOL_STEP; /* uV */
			max77968->ta_max_vol = MIN(val, TA_MAX_VOL * chg_mode);

			/* Set TA voltage to MAX[TA_MIN_VOL_PRESET*chg_mode, (VBAT_ADC*chg_mode + offset)] */
			max77968->ta_vol = max(TA_MIN_VOL_PRESET*chg_mode, (vbat*chg_mode + TA_VOL_PRE_OFFSET));

			if ((max77968->ss_fault_retry_cnt > 0) ||
				(max77968->preset_ta_fault_retry_cnt > 0)) {

				/* Soft start UCP/UCP retry, add 40mV to TA voltage setting */
				if (max77968->ss_fault_inc_ta_volt)
					max77968->ta_vol += TA_VOL_PRE_OFFSET_RETRY_INC*max77968->ss_fault_retry_cnt;

				/* Preset TA fault, TA voltage is lower than low limit */
				if (max77968->preset_ta_fault_inc_ta_volt)
					max77968->ta_vol += TA_VOL_PRE_OFFSET_RETRY_INC*max77968->preset_ta_fault_retry_cnt;

			} else if (max77968->preset_ta_vol_dec_once) {
				/* Decrease TA volt by 60mV if VIN is hiher than TA volt setting */
				max77968->ta_vol = max77968->ta_vol - TA_PRESET_DEC_STEP;
			}

			val = max77968->ta_vol / PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
			max77968->ta_vol = val * PD_MSG_TA_VOL_STEP;

			/* Set TA voltage to MIN[TA voltage, TA_MAX_VOL*chg_mode] */
			max77968->ta_vol = MIN(max77968->ta_vol, max77968->ta_max_vol);

			/* Set the initial TA current to IIN_CC/chg_mode */
			max77968->ta_cur = max77968->iin_cc;

			/* Recover IIN_CC to the original value(iin_cfg) */
			max77968->iin_cc = max77968->iin_cfg;

			pr_info("%s: Preset DC, ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, chg_mode=%d\n",
					__func__, max77968->ta_max_vol, max77968->ta_max_cur, max77968->ta_max_pwr,
					max77968->iin_cc, max77968->chg_mode);

			pr_info("%s: Preset DC, ta_vol=%d, ta_cur=%d, ss_fault_retry_cnt=%d, preset_ta_fault_retry_cnt=%d\n",
					__func__, max77968->ta_vol, max77968->ta_cur, max77968->ss_fault_retry_cnt, max77968->preset_ta_fault_retry_cnt);
		}
	}

	/* Send PD Message */
	mutex_lock(&max77968->lock);
	max77968->timer_id = TIMER_PDMSG_SEND;
	max77968->timer_period = 0;
	mutex_unlock(&max77968->lock);
	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}


/* Preset direct charging configuration */
static int max77968_preset_config(struct max77968_charger *max77968)
{
	int ret = 0, vin, vbat, pmid, val;
	int vin_min_th, vin_max_th;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_PRESET_DC);
#else
	max77968->charging_state = DC_STATE_PRESET_DC;
#endif

	ret = max77968_ensure_scc_standby(max77968);
	if (ret < 0)
		return ret;

	/* Read VBAT ADC */
	vbat = max77968_read_adc(max77968, ADC_CH_VBATT);
	/* Calculate MAX/MIN threshold of VIN*/
	if (max77968->ta_type == TA_TYPE_USBPD_20) {
		/* Calculate MAX/MIN threshold of VIN*/
		vin_max_th = 9500000;
		vin_min_th = 8500000;
		/* Read VIN ADC */
		vin = max77968_read_adc(max77968, ADC_CH_VIN);
		/* Read PMID ADC */
		pmid = max77968_read_adc(max77968, ADC_CH_PMID);
		/* Check if VIN is in proper range */
		if ((vin < vin_min_th) || (vin > vin_max_th)) {
			/* VIN is out of normal range */
			pr_err("%s: Preset TA volt fault (FPDO, VIN out of proper range)\n", __func__);
			pr_err("%s: vin=%d, vbat=%d,vin_max_th=%d, vin_min_th=%d, preset_ta_fault_retry_cnt=%d\n",
					__func__, vin, vbat, vin_max_th, vin_min_th, max77968->preset_ta_fault_retry_cnt);
			/* Check the retry counter */
			if (max77968->preset_ta_fault_retry_cnt < MAX_RETRY_CNT) {
				/* Accumulate preset_ta_fault_retry_cnt */
				max77968->preset_ta_fault_retry_cnt++;
				/* Don't increase TA volt for retry */
				max77968->preset_ta_fault_inc_ta_volt = false;
				/* Go to TIMER_PRESET_DC to set TA again */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PRESET_DC;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
									&max77968->timer_work,
									msecs_to_jiffies(max77968->timer_period));
				ret = 0;
			} else {
				/* Stop charging in timer_work */
				pr_err("%s: Preset TA volt retry fail (FPDO)\n", __func__);
				ret = -EINVAL;
			}

			goto error;

		} else if (pmid >= PMID_VOL_HIGH_TH) {
			/* Stop charging in timer_work */
			pr_err("%s: PMID too high, DC error, PMID_ADC=%d, PMID_HI_TH=%d\n", __func__, pmid, PMID_VOL_HIGH_TH);
			ret = -EINVAL;
			goto error;
		}

	} else {
		/* Calculate MAX/MIN threshold of VIN for APDO */
		vin_max_th = (vbat * max77968->chg_mode) + VIN_VOL_PRE_OFFSET;
		vin_min_th = vbat * max77968->chg_mode;
		/* Read VIN ADC */
		vin = max77968_read_adc(max77968, ADC_CH_VIN);
		/* Read PMID ADC */
		pmid = max77968_read_adc(max77968, ADC_CH_PMID);
		/* Check if VIN is in noraml range */
		if (vin > vin_max_th) {
			/* VIN is higher than high limit */
			pr_err("%s: Preset TA volt fault (over high limit)\n", __func__);
			pr_err("%s: vin=%d, vbat=%d,vin_max_th=%d, vin_min_th=%d, preset_ta_fault_retry_cnt=%d\n",
					__func__, vin, vbat, vin_max_th, vin_min_th, max77968->preset_ta_fault_retry_cnt);
			/* Check the retry counter */
			if (max77968->preset_ta_fault_retry_cnt < MAX_RETRY_CNT) {
				/* Accumulate preset_ta_fault_retry_cnt */
				max77968->preset_ta_fault_retry_cnt++;
				/* Don't increase TA volt for retry */
				max77968->preset_ta_fault_inc_ta_volt = false;
				/* Go to TIMER_PRESET_DC to set TA again */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PRESET_DC;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
									&max77968->timer_work,
									msecs_to_jiffies(max77968->timer_period));
				ret = 0;
			} else {
				/* Stop charging in timer_work */
				pr_err("%s: Preset TA volt retry fail\n", __func__);
				ret = -EINVAL;
			}

			goto error;

		} else if (vin < vin_min_th) {
			/* VIN is lower than low limit */
			pr_err("%s: Preset TA volt fault (under low limit, inc TA volt by 40mV)\n", __func__);
			pr_err("%s: vin=%d, vbat=%d,vin_max_th=%d, vin_min_th=%d, preset_ta_fault_retry_cnt=%d\n",
					__func__, vin, vbat, vin_max_th, vin_min_th, max77968->preset_ta_fault_retry_cnt);
			/* Check the retry counter */
			if (max77968->preset_ta_fault_retry_cnt < MAX_RETRY_CNT) {
				/* Accumulate preset_ta_fault_retry_cnt */
				max77968->preset_ta_fault_retry_cnt++;
				/* Increase TA volt for retry */
				max77968->preset_ta_fault_inc_ta_volt = true;
				/* Go to TIMER_PRESET_DC to set TA again */
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_PRESET_DC;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
									&max77968->timer_work,
									msecs_to_jiffies(max77968->timer_period));
				ret = 0;
			} else {
				/* Stop charging in timer_work */
				pr_err("%s: Preset TA volt retry fail\n", __func__);
				ret = -EINVAL;
			}

			goto error;

		} else if (pmid >= PMID_VOL_HIGH_TH) {
			/* Stop charging in timer_work */
			pr_err("%s: PMID too high, DC error, PMID_ADC=%d, PMID_HI_TH=%d\n", __func__, pmid, PMID_VOL_HIGH_TH);
			ret = -EINVAL;
			goto error;
		} else if ((max77968->preset_ta_vol_dec_once == false) &&
			(vin > (max77968->ta_vol - TA_PRESET_DEC_STEP))) {
			/* VIN is higher than request voltage */
			pr_err("%s: VIN is higher than request voltage, dec 60mV once, ta_vol=%d, vin=%d\n",
					__func__, max77968->ta_vol, vin);
			/* set preset_ta_vol_dec_once to true */
			max77968->preset_ta_vol_dec_once = true;
			/* Go to TIMER_PRESET_DC to set TA again */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PRESET_DC;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			ret = 0;
			goto error;
		}
	}

	/* Set IIN regulation to minimum value before enabling SCC */
	val = IIN_REG_EN;
	val |= IIN_REG_CFG(IIN_REG_MIN) << MASK2SHIFT(IIN_REG_TH);
	ret = max77968_write_reg(max77968, IIN_REGULATION_REG, val);
	if (ret < 0)
		goto error;

	/* Check whether the TA type is USB 2.0 or not */
	if (max77968->ta_type == TA_TYPE_USBPD_20) {
		/* Set VBATT regulation to vfloat */
		ret = max77968_set_vfloat(max77968, max77968->vfloat);
		if (ret < 0)
			goto error;
	} else if (max77968->pdata->vbatt_adc_from == VBATT_FROM_FG) {
		/* Set VBATT regulation to default value */
		ret = max77968_set_vfloat(max77968, VBAT_REG_DFT);
		if (ret < 0)
			goto error;
		max77968->force_vbat_reg_off = true;
	} else { /* vbatt_adc_from == VBATT_FROM_DC */
		/* Set VBATT regulation to vfloat */
		ret = max77968_set_vfloat(max77968, max77968->vfloat);
		if (ret < 0)
			goto error;
	}

	/* Make sure VBATT Regulation enabled */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		goto error;

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	/* Enable MAX77968 */
	ret = max77968_set_charging(max77968, true);
	if (ret < 0)
		goto error;

	/* Clear previous iin adc */
	max77968->prev_iin = 0;

	/* Clear TA increment flag */
	max77968->prev_inc = INC_NONE;

	/* Go to CHECK_ACTIVE state after 200ms*/
	mutex_lock(&max77968->lock);
	max77968->timer_id = TIMER_CHECK_ACTIVE;
	max77968->timer_period = ENABLE_DELAY_T;
	mutex_unlock(&max77968->lock);
	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));
	ret = 0;

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Check the charging status before entering the adjust cc mode */
static int max77968_check_active_state(struct max77968_charger *max77968)
{
	int ret = 0, val;

	pr_info("%s: ======START=======\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_CHECK_ACTIVE);
#else
	max77968->charging_state = DC_STATE_CHECK_ACTIVE;
#endif

	ret = max77968_check_error(max77968);

#if defined(ISSUE_WORKAROUND)
	max77968_after_scc_enable_workaround(max77968);
#endif
	if (ret == 0) {
		/* Recover IIN regulation after enabling SCC */
		ret = max77968_set_input_current(max77968, max77968->iin_cc);
		if (ret < 0)
			goto error;

		/* Make sure VBATT Regulation enabled because */
		ret = max77968_set_vbatt_regu_enable(max77968, true);
		if (ret < 0)
			goto error;

		ret = max77968_get_vbatt_regu_enable(max77968, &val);
		if (ret < 0)
			goto error;

		pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

		/* MAX77968 is in active state */
		/* Clear retry counter */
		max77968->ss_fault_retry_cnt = 0;
		max77968->ss_fault_inc_ta_volt = false;
		max77968->preset_ta_fault_retry_cnt = 0;
		max77968->preset_ta_fault_inc_ta_volt = false;
		max77968->preset_ta_vol_dec_once = false;
		/* Check TA type */
		if (max77968->ta_type == TA_TYPE_USBPD_20) {
			/* Go to FPDO CV mode */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_FPDOCVMODE;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
		} else {
			/* Go to Adjust CC mode */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_ADJUST_CCMODE;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
		}
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
	} else if ((ret == -ERROR_DCRCP) || (ret == -ERROR_DCUCP)) {
		/* It is the retry condition */
		/* Check the retry counter */
		if (max77968->ss_fault_retry_cnt < MAX_RETRY_CNT) {
			/* Disable charging */
			ret = max77968_set_charging(max77968, false);
			if (ret < 0)
				goto error;
			/* Softreset */
			ret = max77968_softreset(max77968);
			if (ret < 0)
				goto error;

			/* Set SCC to standby state*/
			ret = max77968_set_standby_state(max77968, true);
			if (ret < 0)
				goto error;

			/* Increase retry counter */
			max77968->ss_fault_retry_cnt++;
			/* Increase TA volt for retry */
			max77968->ss_fault_inc_ta_volt = true;
			pr_err("%s: Soft start fault retry(inc ta volt by 40mV) - ss_fault_retry_cnt=%d\n", __func__, max77968->ss_fault_retry_cnt);
			/* Go to DC_STATE_PRESET_DC */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PRESET_DC;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			ret = 0;
		} else {
			pr_err("%s: Soft start fault retry over %d times\n", __func__, MAX_RETRY_CNT);
			/* Disable charging */
			ret = max77968_set_charging(max77968, false);
			if (ret < 0)
				goto error;
			/* Softreset */
			ret = max77968_softreset(max77968);
			if (ret < 0)
				goto error;
			/* Notify maximum retry error */
			ret = -EINVAL;
			/* Stop charging in timer_work */
		}
	} else {
		/* It is the retry condition */
		/* Check the retry counter */
		if (max77968->ss_fault_retry_cnt < MAX_RETRY_CNT) {
			/* Disable charging */
			ret = max77968_set_charging(max77968, false);
			if (ret < 0)
				goto error;
			/* Softreset */
			ret = max77968_softreset(max77968);
			if (ret < 0)
				goto error;

			/* Set SCC to standby state*/
			ret = max77968_set_standby_state(max77968, true);
			if (ret < 0)
				goto error;

			/* Increase retry counter */
			max77968->ss_fault_retry_cnt++;
			/* Don't increase TA volt for retry */
			max77968->ss_fault_inc_ta_volt = false;
			pr_err("%s: Soft start fault retry - ss_fault_retry_cnt=%d\n", __func__, max77968->ss_fault_retry_cnt);
			/* Go to DC_STATE_PRESET_DC */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_PRESET_DC;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			ret = 0;
		} else {
			pr_err("%s: Soft start fault retry over %d times\n", __func__, MAX_RETRY_CNT);
			/* Disable charging */
			ret = max77968_set_charging(max77968, false);
			if (ret < 0)
				goto error;
			/* Softreset */
			ret = max77968_softreset(max77968);
			if (ret < 0)
				goto error;
			/* Notify maximum retry error */
			ret = -EINVAL;
			/* Stop charging in timer_work */
		}
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Start reverse mode setting */
static int max77968_charge_start_reverse(struct max77968_charger *max77968)
{
	int ret = 0;
	int val;

	pr_info("%s: ======START=======\n", __func__);

	ret = max77968_ensure_scc_standby(max77968);
	if (ret < 0)
		goto error;

	max77968->charging_state = DC_STATE_REVERSE_MODE;

#if defined(EXT_SW_CONFIG_6)
	// EXT1 SW,  Manual control, Enable
	val = 0x00 << MASK2SHIFT(EXT1_SW_CTRL1);
	val |= 0x00 << MASK2SHIFT(EXT1_SW_CTRL2);
	ret = max77968_update_reg(max77968, EXT1_SW_CTRL_REG, EXT1_SW_CTRL1 | EXT1_SW_CTRL2, val);
	if (ret < 0)
		return ret;

	// EXT2 SW, Manual control, Disable
	val = 0x00 << MASK2SHIFT(EXT2_SW_CTRL1);
	val |= 0x00 << MASK2SHIFT(EXT2_SW_CTRL2);
	ret = max77968_update_reg(max77968, EXT2_SW_CTRL_REG, EXT2_SW_CTRL1 | EXT2_SW_CTRL2, val);
	if (ret < 0)
		return ret;
#endif

	ret = max77968_set_reverse_mode(max77968, true);
	if (ret < 0)
		goto error;

	/* Go to reverse mode */
	mutex_lock(&max77968->lock);
	max77968->timer_id = TIMER_CHECK_REVERSE_ACTIVE;
	max77968->timer_period = REVERSE_WAIT_T; // ms unit
	mutex_unlock(&max77968->lock);
	queue_delayed_work(max77968->dc_wq,
						&max77968->timer_work,
						msecs_to_jiffies(max77968->timer_period));

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Check active status before entering reverse mode */
static int max77968_check_reverse_active_state(struct max77968_charger *max77968)
{
	int ret = 0;

	pr_info("%s: ======START=======\n", __func__);

	max77968->charging_state = DC_STATE_REVERSE_MODE;

	ret = max77968_check_reverse_error(max77968);
#if defined(ISSUE_WORKAROUND)
	max77968_after_scc_enable_workaround(max77968);
#endif

	if (ret == 0) {
		/* MAX77968 is in active state */
		/* Clear retry counter */
		max77968->ss_fault_retry_cnt = 0;
		/* Go to Reverse mode */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_CHECK_REVERSE_MODE;
		max77968->timer_period = REVERSE_CHECK_T;
		mutex_unlock(&max77968->lock);

		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
	} else if (ret == -EAGAIN) {
		/* It is the retry condition */
		/* Check the retry counter */
		if (max77968->ss_fault_retry_cnt < MAX_RETRY_CNT) {
			/* Disable reverse mode */
			ret = max77968_set_reverse_mode(max77968, false);
			if (ret < 0)
				goto error;

			/* Softreset */
			ret = max77968_softreset(max77968);
			if (ret < 0)
				goto error;

			/* Set SCC to standby state*/
			ret = max77968_set_standby_state(max77968, true);
			if (ret < 0)
				goto error;

			/* Increase retry counter */
			max77968->ss_fault_retry_cnt++;
			pr_err("%s: retry to set reverse mode - ss_fault_retry_cnt=%d\n", __func__, max77968->ss_fault_retry_cnt);
			/* Set VIN_OCP_CURRENT_12_11 again */
			ret = max77968_set_vin_ocp(max77968, max77968->iin_rev);
			if (ret < 0)
				goto error;

			/* Set reverse mode */
			ret = max77968_set_reverse_mode(max77968, true);
			if (ret < 0)
				goto error;

			/* Go to DC_STATE_REVERSE_MODE */
			mutex_lock(&max77968->lock);
			max77968->timer_id = TIMER_CHECK_REVERSE_ACTIVE;
			max77968->timer_period = REVERSE_WAIT_T;
			mutex_unlock(&max77968->lock);
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			ret = 0;
		} else {
			pr_err("%s: retry fail\n", __func__);
			/* Disable reverse mode */
			ret = max77968_set_reverse_mode(max77968, false);
			if (ret < 0)
				goto error;
			/* Softreset */
			ret = max77968_softreset(max77968);
			if (ret < 0)
				goto error;
			/* Notify maximum retry error */
			ret = -EINVAL;
			/* Stop charging in timer_work */
		}
	} else {
		pr_err("%s: reverse mode setting fail\n", __func__);
		/* Implement error handler function if it is needed */
		/* Disable reverse mode */
		ret = max77968_set_reverse_mode(max77968, false);
		if (ret < 0)
			goto error;
		/* Softreset */
		ret = max77968_softreset(max77968);
		if (ret < 0)
			goto error;
		/* Stop charging in timer_work */
		ret = -EINVAL;
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;

}

/* Check reverse mode status in polling time */
static int max77968_charge_reverse_mode(struct max77968_charger *max77968)
{
	int ret;
	int vin, iin, vbat;

	pr_info("%s: =========START=========\n", __func__);

	ret = max77968_check_reverse_error(max77968);
	if (ret < 0) {
		/* Error happens and stop reverse mode */
		pr_info("%s: Error happens in reverse mode\n", __func__);
	} else {
		/* Check reverse mode status in polling time */
		/* Read VIN_ADC, IIN_ADC, and VBAT_ADC */
		vin = max77968_read_adc(max77968, ADC_CH_VIN);
		iin = max77968_read_adc(max77968, ADC_CH_IIN);
		vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

		pr_info("%s: reverse mode, vin=%d, iin=%d, vbat=%d\n",
				__func__, vin, iin, vbat);

		/* Set timer - 5s */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_CHECK_REVERSE_MODE;
		max77968->timer_period = REVERSE_CHECK_T;	/* 5s */
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
	}

	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Enter direct charging algorithm */
static int max77968_start_direct_charging(struct max77968_charger *max77968)
{
	int ret, val;
	u8 reg_val[REG_INT_MAX];
	union power_supply_propval prop_val;

	pr_info("%s: =========START=========\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	/* Set watchdog timer enable */
	max77968_check_wdt_control(max77968);
#endif

	ret = max77968_ensure_scc_standby(max77968);
	if (ret < 0)
		return ret;

	/* Enable VBATT regulation threshold to default (4.45V) */
	ret = max77968_set_vfloat(max77968, VBAT_REG_DFT);
	if (ret < 0)
		return ret;

	/* Enable VBATT regulation */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		return ret;

	ret = max77968_get_vbatt_regu_enable(max77968, &val);
	if (ret < 0)
		return ret;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, val);

	/* Enable IIN_UCP protection */
	ret = max77968_update_reg(max77968, IIN_UCP_CFG_REG, IIN_UCP_EN, IIN_UCP_EN);
	if (ret < 0)
		return ret;

	/* Read IIN_UCP protection */
	ret = max77968_read_reg(max77968, IIN_UCP_CFG_REG, &val);
	if (ret < 0)
		return ret;

	val = val & IIN_UCP_EN;
	pr_info("%s: Read IIN_UCP_EN=%d\n", __func__, val);

	val = CHGR_RCP_EN;
	val |= CHGR_RCP_DEG_50ms << MASK2SHIFT(CHGR_RCP_DEG);
	val |= CHGR_RCP_CFG(CHG_RCP_TH_SET) << MASK2SHIFT(CHGR_RCP);
	ret = max77968_write_reg(max77968, CHGR_RCP_CFG_REG, val);
	if (ret < 0)
		return ret;

	ret = max77968_read_reg(max77968, CHGR_RCP_CFG_REG, &val);
	if (ret < 0)
		return ret;

	pr_info("%s: Read IIN_UCP_CFG_REG=%d\n", __func__, val);

	if (max77968->ta_type != TA_TYPE_WIRELESS) {
		/* Get TA type information from battery psy */
		psy_do_property("battery", get,
				POWER_SUPPLY_PROP_ONLINE, prop_val);

		if (prop_val.intval == SEC_BATTERY_CABLE_FPDO_DC) {
			/* The present power supply type is USBPD charger with only fixed PDO */
			max77968->ta_type = TA_TYPE_USBPD_20;
		} else if (prop_val.intval == SEC_BATTERY_CABLE_PDIC_APDO) {
			/* The present power supply type is USBPD with APDO */
			max77968->ta_type = TA_TYPE_USBPD;
		} else {
			/* DC cannot support the present power supply type - unknown power supply type */
			max77968->ta_type = TA_TYPE_UNKNOWN;
		}
	}
	pr_info("%s: ta_type = %d\n", __func__, max77968->ta_type);

	/* wake lock */
	__pm_stay_awake(max77968->monitor_wake_lock);

	/* Clear all interrupt registers before starting DC for debugging */
	ret = max77968_bulk_read_reg(max77968, INT_SRC1_REG, &reg_val[REG_INT1], REG_INT_MAX);
	if (ret < 0)
		return ret;
	pr_info("%s: reg[INT1]=0x%x,[INT2]=0x%x,[INT3]=0x%x,[INT4]=0x%x,[INT5]=0x%x\n",
			__func__, reg_val[0], reg_val[1], reg_val[2], reg_val[3], reg_val[4]);

	/* Preset charging configuration and TA condition */
	ret = max77968_preset_dcmode(max77968);

	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Check Vbat minimum level to start direct charging */
static int max77968_check_vbatmin(struct max77968_charger *max77968)
{
	int vbat;
	int ret;
	int reg;
	union power_supply_propval val;

	pr_info("%s: =========START=========\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_set_charging_state(max77968, DC_STATE_CHECK_VBAT);
#else
	max77968->charging_state = DC_STATE_CHECK_VBAT;
#endif

	ret = max77968_ensure_scc_standby(max77968);
	if (ret < 0)
		goto error;

	/* Enable VBATT regulation threshold to default (4.45V) */
	ret = max77968_set_vfloat(max77968, VBAT_REG_DFT);
	if (ret < 0)
		goto error;

	/* Enable VBATT regulation */
	ret = max77968_set_vbatt_regu_enable(max77968, true);
	if (ret < 0)
		goto error;

	/* Read VBATT regulation status */
	ret = max77968_get_vbatt_regu_enable(max77968, &reg);
	if (ret < 0)
		goto error;

	pr_info("%s: Read VBATT_REG_EN=%d\n", __func__, reg);

	/* Check Vbat */
	vbat = max77968_read_adc(max77968, ADC_CH_VBATT);

	if (vbat < 0)
		ret = vbat;

	/* Read switching charger status */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	ret = psy_do_property(max77968->pdata->sec_dc_name, get,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC, val);
#else
	ret = max77968_get_switching_charger_property(POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
#endif
	if (ret < 0) {
		/* Start Direct Charging again after 1sec */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_VBATMIN_CHECK;
		max77968->timer_period = VBATMIN_CHECK_T;
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		goto error;
	}

	if (val.intval == 0) {
		/* already disabled switching charger */
		/* Clear retry counter */
		max77968->ss_fault_retry_cnt = 0;
		max77968->ss_fault_inc_ta_volt = false;
		max77968->preset_ta_fault_retry_cnt = 0;
		max77968->preset_ta_fault_inc_ta_volt = false;
		max77968->preset_ta_vol_dec_once = false;
		/* Preset TA voltage and DC parameters */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_PRESET_DC;
		max77968->timer_period = 0;
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
	} else {
		/* Switching charger is enabled - 3.1V */
		if (vbat > DC_VBAT_MIN) {
			/* Start Direct Charging */
			/* now switching charger is enabled */
			/* disable switching charger first */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			max77968_set_switching_charger(max77968, false);
#else
			ret = max77968_set_switching_charger(false, 0, 0, 0);
#endif
		}

		/* Wait 1sec for stopping switching charger or Start 1sec timer for battery check */
		mutex_lock(&max77968->lock);
		max77968->timer_id = TIMER_VBATMIN_CHECK;
		max77968->timer_period = VBATMIN_CHECK_T;
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* delayed work function for charging timer */
static void max77968_timer_work(struct work_struct *work)
{
	struct max77968_charger *max77968 = container_of(work, struct max77968_charger,
						timer_work.work);
	int ret = 0;
	unsigned int val;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	union power_supply_propval value = {0,};
	int wire_status = 0, ta_alert_mode = 0, wc_dc_status = 0;

	pr_info("%s: ========== Start 1 =============\n", __func__);

	psy_do_property("battery", get,
			POWER_SUPPLY_EXT_PROP_CHARGE_COUNTER_SHADOW, value);
	wire_status = value.intval;

	psy_do_property("battery", get,
			POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT, value);
	ta_alert_mode = value.intval;

	psy_do_property("wireless", get,
		POWER_SUPPLY_EXT_PROP_WC_DC, value);
	wc_dc_status = value.intval;

	if (!wc_dc_status) {
		if ((wire_status == SEC_BATTERY_CABLE_NONE) && max77968->mains_online) {
			if (ta_alert_mode > OCP_NONE) {
				pr_info("%s: OCP Alert : %d\n", __func__, ta_alert_mode);
				goto error;
			} else {
				pr_info("%s: OCP NONE : %d\n", __func__, ta_alert_mode);
				return;
			}
		}
	}
#endif

	pr_info("%s: timer id=%d, charging_state=%d\n",
		__func__, max77968->timer_id, max77968->charging_state);

	/* Check req_enable flag */
	if (max77968->req_enable == false) {
		/* This case is when battery driver set to stop DC during timer_work is workinig */
		/* And after resuming time_work, timer_id is overwritten by max77968 function */
		/* Timer id shall be TIMER_ID_NONE */
		max77968->timer_id = TIMER_ID_NONE;
		pr_info("%s: req_enable=%d, timer id=%d, charging_state=%d\n",
			__func__, max77968->req_enable, max77968->timer_id, max77968->charging_state);
	}

	switch (max77968->timer_id) {
	case TIMER_VBATMIN_CHECK:
		ret = max77968_check_vbatmin(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_PRESET_DC:
		ret = max77968_start_direct_charging(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_PRESET_CONFIG:
		ret = max77968_preset_config(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_ACTIVE:
		ret = max77968_check_active_state(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ADJUST_CCMODE:
		ret = max77968_charge_adjust_ccmode(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ENTER_CCMODE:
		ret = max77968_charge_start_ccmode(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_CCMODE:
		ret = max77968_charge_ccmode(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ENTER_CVMODE:
		/* Enter Pre-CV mode */
		ret = max77968_charge_start_cvmode(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_CVMODE:
		ret = max77968_charge_cvmode(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_PDMSG_SEND:
		/* Adjust TA current and voltage step */
		if (max77968->ta_type == TA_TYPE_WIRELESS) {
			val = max77968->ta_vol / WCRX_VOL_STEP;		/* RX voltage resolution is 12.5mV */
			max77968->ta_vol = val * WCRX_VOL_STEP;

			/* Set RX voltage */
			ret = max77968_send_rx_voltage(max77968, WCRX_REQUEST_VOLTAGE);
		} else if (max77968->ta_type == TA_TYPE_USBPD_20) {
			/* Send PD Message */
			ret = max77968_send_pd_message(max77968, PD_MSG_REQUEST_FIXED_PDO);
		} else {
			val = max77968->ta_vol / PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
			max77968->ta_vol = val * PD_MSG_TA_VOL_STEP;
			val = max77968->ta_cur / PD_MSG_TA_CUR_STEP;	/* PPS current resolution is 50mA */
			max77968->ta_cur = val * PD_MSG_TA_CUR_STEP;
			if (max77968->ta_cur < TA_MIN_CUR)	/* PPS minimum current is 1000mA */
				max77968->ta_cur = TA_MIN_CUR;

			/* Send PD Message */
			ret = max77968_send_pd_message(max77968, PD_MSG_REQUEST_APDO);
		}
		if (ret < 0)
			goto error;

		/* Go to the next state */
		pr_info("%s : %s\n", __func__, charging_state_str[max77968->charging_state]);

		mutex_lock(&max77968->lock);
		switch (max77968->charging_state) {
		case DC_STATE_PRESET_DC:
			max77968->timer_id = TIMER_PRESET_CONFIG;
			break;
		case DC_STATE_ADJUST_CC:
			max77968->timer_id = TIMER_ADJUST_CCMODE;
			break;
		case DC_STATE_START_CC:
			max77968->timer_id = TIMER_ENTER_CCMODE;
			break;
		case DC_STATE_CC_MODE:
			max77968->timer_id = TIMER_CHECK_CCMODE;
			break;
		case DC_STATE_START_CV:
			max77968->timer_id = TIMER_ENTER_CVMODE;
			break;
		case DC_STATE_CV_MODE:
			max77968->timer_id = TIMER_CHECK_CVMODE;
			break;
		case DC_STATE_ADJUST_TAVOL:
			max77968->timer_id = TIMER_ADJUST_TAVOL;
			break;
		case DC_STATE_ADJUST_TACUR:
			max77968->timer_id = TIMER_ADJUST_TACUR;
			break;
		case DC_STATE_BYPASS_MODE:
			max77968->timer_id = TIMER_CHECK_BYPASSMODE;
			break;
		case DC_STATE_DCMODE_CHANGE:
			max77968->timer_id = TIMER_DCMODE_CHANGE;
			break;
		default:
			ret = -EINVAL;
			break;
		}
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		if (max77968->timer_id == TIMER_ADJUST_CCMODE)
			max77968->timer_period = FACTORY_ADJ_CC_PDMSG_WAIT_T;
		else
			max77968->timer_period = PDMSG_WAIT_T;
#else
		max77968->timer_period = PDMSG_WAIT_T;
#endif
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
		break;

	case TIMER_ADJUST_TAVOL:
		if (max77968->ta_type == TA_TYPE_WIRELESS)
			ret = max77968_adjust_rx_voltage(max77968);
		else
			ret = max77968_adjust_ta_voltage(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ADJUST_TACUR:
		ret = max77968_adjust_ta_current(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_BYPASSMODE:
		ret = max77968_charge_bypass_mode(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_DCMODE_CHANGE:
		ret = max77968_charge_dcmode_change(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_START_REVERSE:
		ret = max77968_charge_start_reverse(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_REVERSE_ACTIVE:
		ret = max77968_check_reverse_active_state(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_REVERSE_MODE:
		ret = max77968_charge_reverse_mode(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_FPDOCVMODE:
		ret = max77968_charge_fpdo_cvmode(max77968);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ID_NONE:
		ret = max77968_stop_charging(max77968);
		if (ret < 0)
			goto error;
		break;

	default:
		break;
	}

	/* Check the charging state again */
	if (max77968->charging_state == DC_STATE_NO_CHARGING) {
		/* Cancel work queue again */
		cancel_delayed_work(&max77968->timer_work);
		cancel_delayed_work(&max77968->pps_work);
	}
	return;

error:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968->chg_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	max77968->health_status = POWER_SUPPLY_EXT_HEALTH_DC_ERR;
#endif
	max77968_stop_charging(max77968);
}

/* delayed work function for pps periodic timer */
static void max77968_pps_request_work(struct work_struct *work)
{
	struct max77968_charger *max77968 = container_of(work, struct max77968_charger,
						pps_work.work);

	int ret = 0;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	int vin, iin;

	/* this is for wdt */
	vin = max77968_read_adc(max77968, ADC_CH_VIN);
	iin = max77968_read_adc(max77968, ADC_CH_IIN);
	pr_info("%s: pps_work_start (vin:%dmV, iin:%dmA)\n",
			__func__, vin/MAX77968_SEC_DENOM_U_M, iin/MAX77968_SEC_DENOM_U_M);
#else
	pr_info("%s: pps_work_start\n", __func__);
#endif

#if defined(CONFIG_SEND_PDMSG_IN_PPS_REQUEST_WORK)
	/* Send PD message */
	ret = max77968_send_pd_message(max77968, PD_MSG_REQUEST_APDO);
#endif
	pr_info("%s: End, ret=%d\n", __func__, ret);
}

static int max77968_hw_init(struct max77968_charger *max77968)
{
	unsigned int val, rev_id, vendor_id;
	int ret;

	pr_info("%s: =========START=========\n", __func__);

	/* Read Device info register */
	ret = max77968_read_reg(max77968, REVISION_REG, &val);
	if (ret < 0) {
		dev_err(max77968->dev, "reading DEVICE_INFO failed, val=0x%x\n", val);
		return -EINVAL;
	}

	/* Check the revision ID */
	rev_id = (val & REVISION_REVISION_ID) >> MASK2SHIFT(REVISION_REVISION_ID);

#if defined(ISSUE_WORKAROUND)
	if (rev_id <= MAX77968_PASS3)
		max77968->pass3_wr_en = true;
	else
		max77968->pass3_wr_en = false;

	max77968->adc_wr_en = false;
	pr_info("%s: ADC_WR_DISABLE\n", __func__);
#endif

	if (rev_id <= MAX77968_PASS4)
		dev_info(max77968->dev, "%s: reading REVISION, rev_id=0x%x\n", __func__, rev_id);
	else
		dev_err(max77968->dev, "%s: FAILED, REVISION isn't valid, rev_id=0x%x\n", __func__, rev_id);

	/* Check the vendor ID */
	vendor_id = val & REVISION_VENDOR_ID;
	if (vendor_id != ADI_VENDOR_ID) {
		dev_err(max77968->dev, "Vendor ID is not matched\n");
		return -EINVAL;
	}

	/*
	 * Program the platform specific configuration values to the device
	 * first.
	 */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968->chg_status = POWER_SUPPLY_STATUS_DISCHARGING;
	max77968->health_status = POWER_SUPPLY_HEALTH_GOOD;
#endif

	max77968->fsw_cfg = max77968->pdata->fsw_cfg_3to1;

	/* Initialize SCC configurations */
	ret = max77968_config_init(max77968);
	if (ret < 0)
		return ret;

	/* input current - uA*/
	ret = max77968_set_input_current(max77968, max77968->pdata->iin_cfg);
	if (ret < 0)
		return ret;

	/* float voltage */
	ret = max77968_set_vfloat(max77968, max77968->pdata->vfloat);
	if (ret < 0)
		return ret;

	/* Save initial charging parameters */
	max77968->iin_cfg = max77968->pdata->iin_cfg;
	max77968->ichg_cfg = max77968->pdata->ichg_cfg;
	max77968->vfloat = max77968->pdata->vfloat;
	max77968->max_vfloat = max77968->pdata->vfloat;
	max77968->iin_topoff = max77968->pdata->iin_topoff;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968->fpdo_dc_iin_topoff = max77968->pdata->fpdo_dc_iin_topoff;
	max77968->fpdo_dc_vnow_topoff = max77968->pdata->fpdo_dc_vnow_topoff;
#endif

	/* Clear new iin and new vfloat */
	max77968->new_iin = 0;
	max77968->new_vfloat = 0;

	/* Initial TA control method is Current Limit mode */
	max77968->ta_ctrl = TA_CTRL_CL_MODE;

	/* Set vfloat decrement flag to false by default */
	max77968->dec_vfloat = false;

	/* Clear charging done counter */
	max77968->done_cnt = 0;

	/* Clear the cause of the error */
	max77968->error_cause = ERR_NODE_NONE;
	return ret;
}

static irqreturn_t max77968_interrupt_handler(int irq, void *data)
{
	struct max77968_charger *max77968 = data;
	u8 int_reg[REG_INT_MAX], sts_reg[REG_STATUS_MAX], mask_reg[REG_INT_MAX];
	u8 masked_int[REG_INT_MAX];	/* masked int */
	bool handled = false;
	int ret, i;

	/* Read interrupt registers */
	ret = max77968_bulk_read_reg(max77968, INT_SRC1_REG, int_reg, REG_INT_MAX);
	if (ret < 0) {
		dev_err(max77968->dev, "reading Interrupt registers failed\n");
		handled = false;
		goto error;
	}
	pr_info("%s: INT reg[0x02]=0x%x,[0x03]=0x%x,[0x04]=0x%x,[0x05]=0x%x,[0x06]=0x%x\n",
			__func__, int_reg[0], int_reg[1], int_reg[2], int_reg[3], int_reg[4]);

	/* Read mask registers */
	ret = max77968_bulk_read_reg(max77968, INT_SRC1_M_REG, mask_reg, REG_INT_MAX);
	if (ret < 0) {
		dev_err(max77968->dev, "reading Mask registers failed\n");
		handled = false;
		goto error;
	}
	pr_info("%s: MASK reg[0x07]=0x%x,reg[0x08]=0x%x,[0x09]=0x%x,[0x0A]=0x%x,[0x0B]=0x%x\n",
			__func__, mask_reg[0], mask_reg[1], mask_reg[2], mask_reg[3], mask_reg[4]);

	/* Read status registers */
	ret = max77968_bulk_read_reg(max77968, STATUS1_REG, sts_reg, REG_STATUS_MAX);
	if (ret < 0) {
		dev_err(max77968->dev, "reading Status registers failed\n");
		handled = false;
		goto error;
	}
	pr_info("%s: STS reg[0x0C]=0x%x,[0x0D]=0x%x,[0x0E]=0x%x\n",
			__func__, sts_reg[0], sts_reg[1], sts_reg[2]);

	/* Check the masked interrupt */
	for (i = 0; i < REG_INT_MAX; i++)
		masked_int[i] = int_reg[i] & !mask_reg[i];

	pr_info("%s: Masked INT reg[0x01]=0x%x,[0x02]=0x%x,[0x03]=0x%x,[0x04]=0x%x,[0x05]=0x%x\n",
			__func__, masked_int[0], masked_int[1], masked_int[2], masked_int[3], masked_int[4]);

	handled = true;

	/* Should implement code by a customer if max77968 needs additional functions or actions */

error:
	return handled ? IRQ_HANDLED : IRQ_NONE;
}

static int max77968_irq_init(struct max77968_charger *max77968,
				struct i2c_client *client)
{
	const struct max77968_platform_data *pdata = max77968->pdata;
	int ret, irq;

	pr_info("%s: =========START=========\n", __func__);

	irq = gpio_to_irq(pdata->irq_gpio);

	ret = gpio_request_one(pdata->irq_gpio, GPIOF_IN, client->name);
	if (ret < 0)
		goto fail;

	ret = request_threaded_irq(irq, NULL, max77968_interrupt_handler,
					IRQF_TRIGGER_LOW | IRQF_ONESHOT,
					client->name, max77968);
	if (ret < 0)
		goto fail_gpio;

	ret = max77968_interrupt_disable(max77968);
	if (ret < 0)
		goto fail_write;

	client->irq = irq;
	return 0;

fail_write:
	free_irq(irq, max77968);
fail_gpio:
	gpio_free(pdata->irq_gpio);
fail:
	client->irq = 0;
	return ret;
}

/*
 * Returns the input current limit programmed
 * into the charger in uA.
 */
static int get_input_current_limit(struct max77968_charger *max77968)
{
	int ret, intval;
	unsigned int val;

	ret = max77968_read_reg(max77968, IIN_REGULATION_REG, &val);
	if (ret < 0)
		return ret;

	intval = (((val >> 1) & 0x7F) * IIN_REG_STEP) + IIN_REG_MIN;

	if (intval > IIN_REG_MAX)
		intval = IIN_REG_MAX;

	return intval;
}

/*
 * Returns the constant charge voltage programmed
 * into the charger in uV.
 */
static int get_const_charge_voltage(struct max77968_charger *max77968)
{
	int ret, intval;
	unsigned int val;

	ret = max77968_read_reg(max77968, VBATT_REGULATION_REG, &val);
	if (ret < 0)
		return ret;

	intval = (val * VBAT_REG_STEP) + VBAT_REG_MIN;

	if (intval > VBAT_REG_MAX)
		intval = VBAT_REG_MAX;

	return intval;
}

/*
 * Returns the enable or disable value.
 * into 1 or 0.
 */
static int get_charging_enabled(struct max77968_charger *max77968)
{
	int ret, intval;
	unsigned int val;

	ret = max77968_read_reg(max77968, SCC_EN_REG, &val);
	if (ret < 0)
		return ret;

	intval = (val & SCC_STANDBY_MODE_SET) ? 0 : 1;

	return intval;
}

static int max77968_chg_set_adc_force_mode(struct max77968_charger *max77968, u8 enable)
{
	unsigned int temp = 0;
	int ret = 0;

	if (enable) {
		/* Enable ADC regardless of VIN voltage */
		ret = max77968_update_reg(max77968, ADC_EN_REG, ADC_EN_BATTONLY, ADC_EN_BATTONLY);
		if (ret < 0)
			return ret;

		/* Wait 2ms to update ADC */
		usleep_range(2000, 3000);
	} else {
		/* Enable low power mode */
		temp = 0;
		ret = max77968_update_reg(max77968, ADC_EN_REG, ADC_EN_BATTONLY, temp);
		if (ret < 0)
			return ret;
	}

	ret = max77968_read_reg(max77968, ADC_EN_REG, &temp);
	pr_info("%s: ADC_CTRL : 0x%02x\n", __func__, temp);

	return ret;
}

static int max77968_set_mains_stat(struct max77968_charger *max77968, int mains_stat)
{
	/* Check whether max77968 is in reverse mode */
	if (max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_STOP) {
		pr_info("%s: reverse mode stop, set mains state\n", __func__);
		if (is_3_1_wc_status(mains_stat)) {
			pr_info("%s: Start Wireless Direct Charging\n", __func__);
			max77968->ta_type = TA_TYPE_WIRELESS;
			max77968->iin_topoff = 380000;
			max77968->mains_online = true;
		} else if (mains_stat == 0) {
			pr_info("%s: Stop Direct Charging\n", __func__);
			max77968->iin_topoff = 500000;
			max77968->mains_online = false;
			/* Check TA detachment and clear new_iin */
			max77968->new_iin = 0;
			/* Ensure that dc_wq still exists */
			if (max77968->dc_wq) {
				/* Cancel delayed work */
				cancel_delayed_work(&max77968->timer_work);
				cancel_delayed_work(&max77968->pps_work);
				/* Stop Direct Charging	*/
				mutex_lock(&max77968->lock);
				max77968->timer_id = TIMER_ID_NONE;
				max77968->timer_period = 0;
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq,
									&max77968->timer_work,
									msecs_to_jiffies(max77968->timer_period));
			}
			max77968->chg_status = POWER_SUPPLY_STATUS_DISCHARGING;
			max77968->health_status = POWER_SUPPLY_HEALTH_GOOD;
		} else if (mains_stat == 1) {
			/* Start Direct charging */
			pr_info("%s: Start Direct Charging\n", __func__);
			max77968->iin_topoff = 500000;
			max77968->mains_online = true;
		}
	} else {
		/* Reverse switching mode is working */
		pr_info("%s: reverse mode(%d), ignore set mains state setting\n",
			__func__, max77968->rev_mode);
	}

	return 0;
}

static int max77968_req_new_vfloat(struct max77968_charger *max77968, int new_vfloat)
{
	if (new_vfloat > VBAT_REG_MAX)
		new_vfloat = VBAT_REG_MAX;
	else if (new_vfloat < VBAT_REG_MIN)
		new_vfloat = VBAT_REG_MIN;

	pr_info("%s: new_vfloat: %d\n", __func__, new_vfloat);

	if (new_vfloat != max77968->new_vfloat) {
		/* request new float voltage */
		max77968->new_vfloat = new_vfloat;
		/* Check the charging state */
		if ((max77968->charging_state == DC_STATE_NO_CHARGING) ||
			(max77968->charging_state == DC_STATE_CHECK_VBAT)) {
			/* Apply new vfloat when the direct charging is started */
			max77968->vfloat = max77968->new_vfloat;
			pr_info("%s: Apply new vfloat when the direct charging is started\n", __func__);
		} else {

			/* Protect access to req_new_vfloat */
			mutex_lock(&max77968->lock);

			/* Check whether the previous request is done or not */
			if (max77968->req_new_vfloat == true) {
				mutex_unlock(&max77968->lock);
				/* The previous request is not done yet */
				pr_err("%s: There is the previous request for New vfloat\n", __func__);
				return -EBUSY;
			} else {
				/* Set request flag */
				max77968->req_new_vfloat = true;
				mutex_unlock(&max77968->lock);
				/* Check the charging state */
				if ((max77968->charging_state == DC_STATE_CC_MODE) ||
					(max77968->charging_state == DC_STATE_CV_MODE) ||
					(max77968->charging_state == DC_STATE_BYPASS_MODE) ||
					(max77968->charging_state == DC_STATE_CHARGING_DONE) ||
					(max77968->charging_state == DC_STATE_FPDO_CV_MODE)) {
					/* Ensure that dc_wq still exists */
					if (max77968->dc_wq) {
						/* cancel delayed_work */
						cancel_delayed_work(&max77968->timer_work);
						/* do delayed work at once */
						mutex_lock(&max77968->lock);
						max77968->timer_period = 0;	// ms unit
						mutex_unlock(&max77968->lock);
						queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
					}
				} else {
					/* Wait for next valid state - cc, cv, or bypass state */
					pr_info("%s: Not support new vfloat yet in charging state=%d\n",
							__func__, max77968->charging_state);
				}
			}
		}
	}

	return 0;
}

static int max77968_req_new_iin(struct max77968_charger *max77968, int new_iin)
{
	if ((max77968->ta_type == TA_TYPE_USBPD_20) &&
		(new_iin < max77968->pdata->fpdo_dc_iin_lowest_limit)) {
		new_iin = max77968->pdata->fpdo_dc_iin_lowest_limit;
		pr_info("%s: IIN LOWEST LIMIT! IIN %d -> %d\n", __func__,
				new_iin, max77968->pdata->fpdo_dc_iin_lowest_limit);
	} else if ((max77968->charging_state == DC_STATE_CV_MODE) &&
				(max77968->ta_type == TA_TYPE_WIRELESS) &&
				(max77968->iin_cc != IIN_TEMP_CONTROL_DFT) &&
				(new_iin > max77968->iin_cc)) {
		new_iin = max77968->iin_cc;
		pr_info("%s: WC : Not increase current limit in CV (%d, %d)\n",
				__func__, new_iin, max77968->iin_cc);
	}

	/* Compare with topoff current */
	if (new_iin < max77968->iin_topoff) {
		/* This new iin is abnormal input current */
		pr_err("%s: This new iin(%duA) is abnormal value\n",
				__func__, new_iin);
		return -EINVAL;
	}

	if ((max77968->charging_state == DC_STATE_NO_CHARGING) ||
		(max77968->charging_state == DC_STATE_CHECK_VBAT)) {
		mutex_lock(&max77968->lock);
		max77968->new_iin = new_iin;
		max77968->iin_cfg = new_iin;
		mutex_unlock(&max77968->lock);
		pr_info("%s: charging state=%d, new iin(%uA) and iin_cfg(%uA)\n",
				__func__, max77968->charging_state,
				max77968->new_iin, max77968->iin_cfg);
		return 0;
	}

	/* Check TA type */
	if (max77968->ta_type == TA_TYPE_USBPD_20) {
		/* Cannot support change input current during DC */
		/* Because FPDO cannot control input current by PD message */
		pr_err("%s: Error - FPDO cannot control input current\n", __func__);
		return -EINVAL;
	} else {
		/* Protect access to req_new_iin */
		mutex_lock(&max77968->lock);
		/* Check whether the previous request is done or not */
		if (max77968->req_new_iin == true) {
			max77968->new_iin_busy_buf = new_iin;
			max77968->new_iin_buf_has_data = true;
			mutex_unlock(&max77968->lock);
			/* The previous request is not done yet */
			pr_err("%s: There is the previous request for New iin\n", __func__);
			//return -EBUSY;
		} else {
			/* request new input current */
			max77968->new_iin = new_iin;
			max77968->req_new_iin = true;
			mutex_unlock(&max77968->lock);
			/* Check the charging state */
			if ((max77968->charging_state == DC_STATE_CC_MODE) ||
				(max77968->charging_state == DC_STATE_CV_MODE) ||
				(max77968->charging_state == DC_STATE_BYPASS_MODE) ||
				(max77968->charging_state == DC_STATE_CHARGING_DONE) ||
				(max77968->charging_state == DC_STATE_ADJUST_TAVOL)) {
				/* Ensure that dc_wq still exists */
				if (max77968->dc_wq) {
					/* cancel delayed_work */
					cancel_delayed_work(&max77968->timer_work);
					/* do delayed work at once */
					mutex_lock(&max77968->lock);
					max77968->timer_period = 0;	// ms unit
					mutex_unlock(&max77968->lock);
					queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
				}
			} else {
				/* Wait for next valid state - cc, cv, or bypass state */
				pr_info("%s: Not support new iin yet in charging state=%d\n",
						__func__, max77968->charging_state);
			}
		}
	}

	return 0;
}

static int max77968_req_new_chg_mode(struct max77968_charger *max77968, int new_chg_mode)
{
	if (new_chg_mode == CHG_2TO1_DC_MODE) {
		pr_info("%s: set CHG_2TO1_DC_MODE\n", __func__);
	} else {
		pr_info("%s: set CHG_3TO1_DC_MODE\n", __func__);
	}

	/* Protect the access to req_new_chg_mode with lock */
	mutex_lock(&max77968->lock);

	if ((max77968->charging_state == DC_STATE_NO_CHARGING) ||
		(max77968->charging_state == DC_STATE_CHECK_VBAT)    ||
		(max77968->charging_state == DC_STATE_BYPASS_MODE) ||
		(max77968->charging_state == DC_STATE_DCMODE_CHANGE) ||
		(max77968->charging_state == DC_STATE_REVERSE_MODE) ||
		(max77968->charging_state == DC_STATE_FPDO_CV_MODE)) {

		max77968->pdata->chg_mode = new_chg_mode;
		max77968->new_chg_mode = new_chg_mode;
		mutex_unlock(&max77968->lock);
		pr_info("%s: charging state=%d, new_chg_mode=%d\n",
				__func__, max77968->charging_state, new_chg_mode);
		return 0;
	}

	/* Check whether the previous request is done or not */
	if (max77968->req_new_chg_mode == true) {
		/* The previous request is not done yet */
		max77968->new_chg_mode_busy_buf = new_chg_mode;
		max77968->new_chg_mode_buf_has_data = true;
		mutex_unlock(&max77968->lock);
		pr_err("%s: There is the previous request for new chg mode, save new_chg_mode=%d in busy buffer\n",
				__func__, new_chg_mode);
		//return -EBUSY;
	} else {
		/* Check the charging state */
		if ((max77968->charging_state == DC_STATE_CC_MODE) ||
			(max77968->charging_state == DC_STATE_CV_MODE) ||
			(max77968->charging_state == DC_STATE_CHARGING_DONE)) {

			max77968->new_chg_mode = new_chg_mode;
			max77968->req_new_chg_mode = true;
			mutex_unlock(&max77968->lock);
			/* Ensure that dc_wq still exists */
			if (max77968->dc_wq) {
				/* cancel delayed_work */
				cancel_delayed_work(&max77968->timer_work);
				/* do delayed work at once */
				mutex_lock(&max77968->lock);
				max77968->timer_period = 0;	// ms unit
				mutex_unlock(&max77968->lock);
				queue_delayed_work(max77968->dc_wq, &max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
			}
			pr_info("%s: Apply new chg_mode when charging_state=%d\n",
					__func__, max77968->charging_state);
		} else {
			/* Save new charging mode and wait for the next valid state */
			max77968->new_chg_mode = new_chg_mode;
			max77968->req_new_chg_mode = true;
			mutex_unlock(&max77968->lock);

			/* Direct charging has started */
			/*
			 * Wait for next valid state to handle mode change
			 * - cc, cv, or chg_done
			 */
			pr_info("%s: Saved the new_chg_mode when charging state=%d, wait next valid state to apply\n",
				__func__, max77968->charging_state);
		}
	}

	return 0;
}

static int max77968_req_new_byp_mode(struct max77968_charger *max77968, int new_byp_mode)
{
	if (new_byp_mode != max77968->new_byp_mode) {
		/* Request new byp mode */
		max77968->new_byp_mode = new_byp_mode;
		/* Check the charging state */
		if (max77968->charging_state == DC_STATE_NO_CHARGING) {
			/* Not support state */
			pr_info("%s: Not support req new byp mode in charging state=%d\n",
					__func__, max77968->charging_state);
			return -EINVAL;
		} else {
			/* Protect the access to req_new_byp_mode with lock */
			mutex_lock(&max77968->lock);
			/* Check whether the previous request is done or not */
			if (max77968->req_new_byp_mode == true) {
				/* The previous request is not done yet */
				mutex_unlock(&max77968->lock); // Unlock before returning
				pr_err("%s: There is the previous request for new byp mode\n", __func__);
				return -EBUSY;
			} else {
				/* Set request flag */
				max77968->req_new_byp_mode = true;
				mutex_unlock(&max77968->lock);
				/* Check the charging state */
				if ((max77968->charging_state == DC_STATE_CC_MODE) ||
					(max77968->charging_state == DC_STATE_CV_MODE) ||
					(max77968->charging_state == DC_STATE_BYPASS_MODE)) {
					/* Ensure that dc_wq still exists */
					if (max77968->dc_wq) {
						/* cancel delayed_work */
						cancel_delayed_work(&max77968->timer_work);
						/* do delayed work at once */
						mutex_lock(&max77968->lock);
						max77968->timer_period = 0;	// ms unit
						mutex_unlock(&max77968->lock);
						queue_delayed_work(max77968->dc_wq,
							&max77968->timer_work,
							msecs_to_jiffies(max77968->timer_period));
					}
				} else {
					/* Wait for next valid state - cc, cv, or bypass state */
					pr_info("%s: new byp saved, wait for next valid state, currently charging_state=%d\n",
							__func__, max77968->charging_state);
				}
			}
		}
	}

	return 0;
}

static int max77968_set_charging_enabled(struct max77968_charger *max77968, int enable)
{
	int ret = 0;

	if (enable == 0) {
		/* Set req_enable flag to false */
		max77968->req_enable = false;
		/* Stop direct charging */
		pr_info("%s: Stop direct charging\n", __func__);
		ret = max77968_stop_charging(max77968);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968->chg_status = POWER_SUPPLY_STATUS_DISCHARGING;
		max77968->health_status = POWER_SUPPLY_HEALTH_GOOD;
#endif
	} else {
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		if (max77968->charging_state != DC_STATE_NO_CHARGING) {
			pr_info("## %s: duplicate charging enabled, charging_state=%d\n",
					__func__, max77968->charging_state);
			return ret;
		}
		if (!max77968->mains_online) {
			pr_info("## %s: mains_online is not attached, mains_online=%d\n",
					__func__, max77968->mains_online);
			return ret;
		}
#endif
		ret = max77968_set_standby_state(max77968, true);
		if (ret < 0)
			return ret;

		ret = max77968_config_init(max77968);
		if (ret < 0)
			return ret;

		/* Start Direct Charging */
		pr_info("%s: Start direct charging\n", __func__);
		/* Set req_enable flag to true */
		max77968->req_enable = true;
		/* Set initial wake up timeout - 10s */
		pm_wakeup_ws_event(max77968->monitor_wake_lock, INIT_WAKEUP_T, false);
		/* Start 1sec timer for battery check */
		mutex_lock(&max77968->lock);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		max77968_set_charging_state(max77968, DC_STATE_CHECK_VBAT);
#else
		max77968->charging_state = DC_STATE_CHECK_VBAT;
#endif
		max77968->timer_id = TIMER_VBATMIN_CHECK;
		max77968->timer_period = VBATMIN_CHECK_T;
		mutex_unlock(&max77968->lock);
		/* Ensure that dc_wq still exists */
		if (max77968->dc_wq) {
			queue_delayed_work(max77968->dc_wq,
								&max77968->timer_work,
								msecs_to_jiffies(max77968->timer_period));
		}
	}
	return ret;
}

static int max77968_req_rev_mode(struct max77968_charger *max77968, int rev_mode)
{
	int ret = 0;

	/* Set reverse mode */
	max77968->rev_mode = rev_mode;
	/* Set reverse mode */
	if (max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_STOP) {
		/* Set req_enable flag to false */
		max77968->req_enable = false;
		/* Cancel delayed_work */
		cancel_delayed_work(&max77968->timer_work);
		/* Stop reverse mode */
		ret = max77968_stop_charging(max77968);
	} else {
		if (max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_1TO3) {
			max77968->fsw_cfg = max77968->pdata->fsw_cfg_3to1;
			pr_info("%s: REVERSE_1TO3, fsw_cfig=%d\n", __func__, max77968->fsw_cfg);
		} else if (max77968->rev_mode == POWER_SUPPLY_DC_REVERSE_1TO2) {
			max77968->fsw_cfg = max77968->pdata->fsw_cfg_2to1;
			pr_info("%s: REVERSE_1TO2, fsw_cfig=%d\n", __func__, max77968->fsw_cfg);
		} else {
			max77968->fsw_cfg = max77968->pdata->fsw_cfg_byp;
			pr_info("%s: REVERSE_BYP, fsw_cfig=%d\n", __func__, max77968->fsw_cfg);
		}

		/* Protect access to charging_state with mutex */
		mutex_lock(&max77968->lock);
		if (max77968->charging_state == DC_STATE_NO_CHARGING) {
			/* Set req_enable flag to true */
			max77968->req_enable = true;
			/* Start Reverse Mode */
			max77968->charging_state = DC_STATE_REVERSE_MODE;
			max77968->timer_id = TIMER_START_REVERSE;
			max77968->timer_period = 0;
			mutex_unlock(&max77968->lock);

			/* Ensure that dc_wq still exists */
			if (max77968->dc_wq) {
				queue_delayed_work(max77968->dc_wq,
									&max77968->timer_work,
									msecs_to_jiffies(max77968->timer_period));
			}
		} else {
			/* Charging state does not support reverse mode */
			pr_info("%s: Not support reverse mode in charging state=%d\n",
					__func__, max77968->charging_state);
			mutex_unlock(&max77968->lock);
			ret = -EINVAL;
		}
	}

	return ret;
}

static int max77968_chg_set_property(struct power_supply *psy,
					enum power_supply_property prop,
					const union power_supply_propval *val)
{
	struct max77968_charger *max77968 = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) prop;
	unsigned int temp = 0;
	int ret = 0;

	pr_info("%s: =========START=========\n", __func__);
	pr_info("%s: prop=%d, val=%d\n", __func__, prop, val->intval);

	if (atomic_read(&max77968->shutdown) == 1) {
		pr_info("%s:System shutdown, do nothing\n", __func__);
		return -EINVAL;
	}

	switch ((int)prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s: POWER_SUPPLY_PROP_ONLINE\n", __func__);
		ret = max77968_set_mains_stat(max77968, val->intval);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		pr_info("%s: POWER_SUPPLY_PROP_PRESENT\n", __func__);
		/* Set the USBPD-TA is plugged in or out */
		max77968->mains_online = val->intval;
		break;

	case POWER_SUPPLY_PROP_TYPE:
		pr_info("%s: POWER_SUPPLY_PROP_TYPE\n", __func__);
		/* Set power supply type */
		if (val->intval == POWER_SUPPLY_TYPE_WIRELESS) {
			/* The current power supply type is wireless charger */
			max77968->ta_type = TA_TYPE_WIRELESS;
			pr_info("%s: The current power supply type is WC, ta_type=%d\n",
					__func__, max77968->ta_type);
		} else {
			/* Default TA type is USBPD TA */
			max77968->ta_type = TA_TYPE_USBPD;
			pr_info("%s: The current power supply type is USBPD, ta_type=%d\n",
					__func__, max77968->ta_type);
		}
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		pr_info("%s: POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE\n", __func__);
		max77968->float_voltage = val->intval;
		temp = max77968->float_voltage * MAX77968_SEC_DENOM_U_M;
		pr_info("%s: %s : vfloat(%d -> %d)\n", __func__,
				charging_state_str[max77968->charging_state],
				max77968->new_vfloat, temp);
		ret = max77968_req_new_vfloat(max77968, temp);
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		pr_info("%s: POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT\n", __func__);
		max77968->input_current = val->intval;
		temp = max77968->input_current * MAX77968_SEC_DENOM_U_M;
		ret = max77968_req_new_iin(max77968, temp);
		break;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		pr_info("%s: POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX\n", __func__);
		max77968->pdata->vfloat = val->intval * MAX77968_SEC_DENOM_U_M;
		if (max77968->pdata->vfloat > VBAT_REG_MAX)
			max77968->pdata->vfloat = VBAT_REG_MAX;
		else if (max77968->pdata->vfloat < VBAT_REG_MIN)
			max77968->pdata->vfloat = VBAT_REG_MIN;
		pr_info("%s: v_float(%duV)\n", __func__, max77968->pdata->vfloat);
		/* Save maximum vfloat to max_vfloat */
		max77968->max_vfloat = max77968->pdata->vfloat;
		break;
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL\n", __func__);
			if (val->intval)
				max77968->wdt_kick_disable = true;
			else
				max77968->wdt_kick_disable = false;
			pr_info("%s: wdt_kick_disable=%d\n", __func__, max77968->wdt_kick_disable);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_CURRENT_MAX:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DIRECT_CURRENT_MAX\n", __func__);
			max77968->input_current = val->intval;
			temp = max77968->input_current * MAX77968_SEC_DENOM_U_M;
			ret = max77968_req_new_iin(max77968, temp);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_ADC_CTRL:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DIRECT_ADC_CTRL\n", __func__);
			ret = max77968_chg_set_adc_force_mode(max77968, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED\n", __func__);
			ret = max77968_set_charging_enabled(max77968, val->intval);
			break;

		case POWER_SUPPLY_EXT_PROP_DC_OP_MODE:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DC_OP_MODE\n", __func__);
			if (val->intval == DC_MODE_2TO1)
				ret = max77968_req_new_chg_mode(max77968, CHG_2TO1_DC_MODE);
			else
				ret = max77968_req_new_chg_mode(max77968, CHG_3TO1_DC_MODE);
			break;

		case POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE\n", __func__);
			ret = max77968_req_new_byp_mode(max77968, val->intval);
			break;

		case POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE_TA_VOL:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE_TA_VOL\n", __func__);
			if ((max77968->charging_state == DC_STATE_BYPASS_MODE) &&
				(max77968->byp_mode != PTM_NONE)) {
				pr_info("[PASS_THROUGH_VOL] %s, bypass mode\n", __func__);
				/* Set TA voltage for bypass mode */
				max77968_set_bypass_ta_voltage_by_soc(max77968, val->intval);
			} else {
				pr_info("[PASS_THROUGH_VOL] %s, not bypass mode\n", __func__);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_DC_VIN_OVERCURRENT:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DC_VIN_OVERCURRENT\n", __func__);
			/* Set VIN OCP current */
			max77968->iin_rev = val->intval;
			ret = max77968_set_vin_ocp(max77968, max77968->iin_rev);
			break;

		case POWER_SUPPLY_EXT_PROP_DC_REVERSE_MODE:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DC_REVERSE_MODE\n", __func__);
			ret = max77968_req_rev_mode(max77968, val->intval);
			break;
		default:
			return -EINVAL;
		}
		break;
#endif
	default:
		ret = -EINVAL;
		break;
	}

	pr_info("%s: End, prop=%d, ret=%d\n", __func__, prop, ret);
	return ret;
}

static int max77968_chg_get_property(struct power_supply *psy,
					enum power_supply_property prop,
					union power_supply_propval *val)
{
	int ret = 0;
	struct max77968_charger *max77968 = power_supply_get_drvdata(psy);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) prop;
#endif

	pr_debug("%s: =========START=========\n", __func__);
	pr_debug("%s: prop=%d\n", __func__, prop);

	if (atomic_read(&max77968->shutdown) == 1) {
		pr_info("%s:System shutdown\n", __func__);
		return -EINVAL;
	}

	switch ((int)prop) {
	case POWER_SUPPLY_PROP_PRESENT:
		/* TA present */
		val->intval = max77968->mains_online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = max77968->adc_val[ADC_CH_VBATT];
		break;

	case POWER_SUPPLY_PROP_TYPE:
		if (max77968->ta_type == TA_TYPE_WIRELESS) {
			pr_info("%s: POWER_SUPPLY_PROP_TYPE : TA_TYPE_WIRELESS\n", __func__);
			val->intval = POWER_SUPPLY_TYPE_WIRELESS;
		} else {
			if (max77968->ta_type == TA_TYPE_USBPD)
				pr_info("%s: POWER_SUPPLY_PROP_TYPE : TA_TYPE_USBPD\n", __func__);
			else if (max77968->ta_type == TA_TYPE_USBPD_20)
				pr_info("%s: POWER_SUPPLY_PROP_TYPE : TA_TYPE_USBPD_20\n", __func__);
			else
				pr_info("%s: POWER_SUPPLY_PROP_TYPE : TA_TYPE_UNKNOWN\n", __func__);

			val->intval = POWER_SUPPLY_TYPE_USB_PD;
		}
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s: POWER_SUPPLY_PROP_ONLINE\n", __func__);
		val->intval = max77968->mains_online;
		break;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	case POWER_SUPPLY_PROP_STATUS:
		pr_info("%s: POWER_SUPPLY_PROP_STATUS\n", __func__);
		val->intval = max77968->chg_status;
		pr_info("%s: CHG STATUS : %d\n", __func__, max77968->chg_status);
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		pr_info("%s: POWER_SUPPLY_PROP_HEALTH\n", __func__);
		if (max77968->charging_state >= DC_STATE_CHECK_ACTIVE &&
			max77968->charging_state <= DC_STATE_CV_MODE)
			ret = max77968_check_error(max77968);

		val->intval = max77968->health_status;
		pr_info("%s: HEALTH STATUS : %d, ret = %d\n",
			__func__, max77968->health_status, ret);
		ret = 0;
		break;
#endif

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		pr_info("%s: POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE\n", __func__);
		ret = get_const_charge_voltage(max77968);
		if (ret < 0) {
			val->intval = 0;
		} else {
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			val->intval = max77968->float_voltage;
#else
			val->intval = ret;
#endif
			ret = 0;
		}
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		pr_info("%s: POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX\n", __func__);
		/* Maximum vfloat */
		val->intval = max77968->pdata->vfloat / DENOM_U_M;
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		pr_info("%s: POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT\n", __func__);
		ret = get_input_current_limit(max77968);
		if (ret < 0) {
			val->intval = 0;
		} else {
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			val->intval = ret / MAX77968_SEC_DENOM_U_M;
#else
			val->intval = ret;
#endif
			ret = 0;
		}
		break;

	case POWER_SUPPLY_PROP_TEMP:
		pr_info("%s: POWER_SUPPLY_PROP_TEMP\n", __func__);
#if defined(CONFIG_ADIENV)
		/* Has disabled NCT ADC, but we need to return fixed value for ADI test platform */
		/* Return zero make temperature management of AP works abnormally */
		val->intval = 982752;
		ret = 0;
#else
		/* return NTC voltage  - uV unit */
		ret = max77968_read_adc(max77968, ADC_CH_NTC);

		if (ret < 0) {
			val->intval = 0;
		} else {
			val->intval = ret;
			ret = 0;
		}
#endif
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		pr_info("%s: POWER_SUPPLY_PROP_CURRENT_NOW\n", __func__);
		/* return the output current - uA unit */
		/* check charging status */
		if (max77968->charging_state == DC_STATE_NO_CHARGING) {
			/* return invalid */
			val->intval = 0;
			ret = 0;
		} else {
			dev_err(max77968->dev, "Invalid IBAT ADC\n");
			val->intval = 0;
		}
		break;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL:
			val->intval = max77968->wdt_kick_disable;
			break;
		case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_MONITOR_WORK\n", __func__);
			max77968_monitor_work(max77968);
			max77968_test_read(max77968);
			break;
		case POWER_SUPPLY_EXT_PROP_MEASURE_INPUT:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_MEASURE_INPUT (%d)\n", __func__, val->intval);
			switch (val->intval) {
			case SEC_BATTERY_IIN_MA:
#if defined(ISSUE_WORKAROUND)
				if (max77968->adc_wr_en) {
					val->intval = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
					val->intval = val->intval / MAX77968_SEC_DENOM_U_M;
				} else {
					max77968_read_adc(max77968, ADC_CH_IIN);
					val->intval = max77968->adc_val[ADC_CH_IIN];
				}
#else
				max77968_read_adc(max77968, ADC_CH_IIN);
				val->intval = max77968->adc_val[ADC_CH_IIN];
#endif
				break;
			case SEC_BATTERY_IIN_UA:
#if defined(ISSUE_WORKAROUND)
				if (max77968->adc_wr_en)
					val->intval = max77968_get_adc_buf_median(max77968, ADC_BUF_IIN);
				else
					val->intval = max77968_read_adc(max77968, ADC_CH_IIN);
#else
				max77968_read_adc(max77968, ADC_CH_IIN);
				val->intval = max77968->adc_val[ADC_CH_IIN] * MAX77968_SEC_DENOM_U_M;
#endif
				break;
			case SEC_BATTERY_VIN_MA:
				max77968_read_adc(max77968, ADC_CH_VIN);
				val->intval = max77968->adc_val[ADC_CH_VIN];
				break;
			case SEC_BATTERY_VIN_UA:
				max77968_read_adc(max77968, ADC_CH_VIN);
				val->intval = max77968->adc_val[ADC_CH_VIN] * MAX77968_SEC_DENOM_U_M;
				break;
			default:
				val->intval = 0;
				break;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_MEASURE_SYS:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_MEASURE_SYS\n", __func__);
			/* get_system_current function isn't supported. Cannot get accurate value of Isys */
			val->intval = 0;
			pr_info("%s: get_system_current function isn't supported. Cannot get accurate value of Isys\n", __func__);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_CHG_STATUS:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_CHG_STATUS\n", __func__);
			val->strval = charging_state_str[max77968->charging_state];
			pr_info("%s: CHARGER_STATUS(%s)\n", __func__, val->strval);
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED\n", __func__);
			ret = get_charging_enabled(max77968);
			if (ret < 0)
				return ret;

			val->intval = ret;
			break;
		case POWER_SUPPLY_EXT_PROP_DC_OP_MODE:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DC_OP_MODE\n", __func__);
			val->intval = max77968->chg_mode;
			break;
		case POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE\n", __func__);
			break;
		case POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE_TA_VOL:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE_TA_VOL\n", __func__);
			break;
		case POWER_SUPPLY_EXT_PROP_DC_VIN_OVERCURRENT:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DC_VIN_OVERCURRENT\n", __func__);
			/* Get vin_ocp_current_12_11 */
			val->intval = max77968->iin_rev;
			break;
		case POWER_SUPPLY_EXT_PROP_DC_REVERSE_MODE:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DC_REVERSE_MODE\n", __func__);
			/* Get reverse mode */
			val->intval = max77968->rev_mode;
			break;

		case POWER_SUPPLY_EXT_PROP_DC_ERROR_CAUSE:
			/* Get dc err cause */
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DC_ERROR_CAUSE\n", __func__);
			val->intval = max77968->error_cause;
			break;
		default:
			return -EINVAL;
		}
		break;
#endif
	default:
		ret = -EINVAL;
	}

	pr_debug("%s: End, prop=%d, val=%d, ret=%d\n", __func__, prop, val->intval, ret);
	return ret;
}

static enum power_supply_property max77968_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static const struct regmap_config max77968_regmap = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= 0x80,//MAX77968_MAX_REG,
};

#if defined(ISSUE_WORKAROUND)
static const struct regmap_config max77968_otpregmap = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= OTP_REG_END,
};
static const struct regmap_config max77968_tregmap = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= TEST_REG_END,
};
#endif

static char *max77968_supplied_to[] = {
	"max77968-charger",
};

static const struct power_supply_desc max77968_mains_desc = {
	.name		= "max77968-charger",
	.type		= POWER_SUPPLY_TYPE_UNKNOWN,
	.get_property	= max77968_chg_get_property,
	.set_property	= max77968_chg_set_property,
	.properties	= max77968_charger_props,
	.num_properties	= ARRAY_SIZE(max77968_charger_props),
};

#if defined(CONFIG_OF)
static int max77968_charger_parse_dt(struct device *dev,
				struct max77968_platform_data *pdata)
{
	struct device_node *np_max77968 = dev->of_node;
	struct device_node *np;
	int ret;

	if (!np_max77968)
		return -EINVAL;

	/* input current limit */
	ret = of_property_read_u32(np_max77968, "max77968,input-current-limit",
							&pdata->iin_cfg);
	if (ret) {
		pr_info("%s: maxim,input-current-limit is Empty\n", __func__);
		pdata->iin_cfg = IIN_REG_DFT;
	}
	pr_info("%s: max77968,iin_cfg is %d\n", __func__, pdata->iin_cfg);

	/* input topoff current */
	ret = of_property_read_u32(np_max77968, "max77968,input-itopoff",
								&pdata->iin_topoff);
	if (ret) {
		pr_info("%s: max77968,input-itopoff is Empty\n", __func__);
		pdata->iin_topoff = IIN_DONE_DFT;
	}
	pr_info("%s: max77968,iin_topoff is %d\n", __func__, pdata->iin_topoff);

	/* fpdo_dc input topoff current */
	ret = of_property_read_u32(np_max77968, "max77968,fpdo_dc_input-itopoff",
								&pdata->fpdo_dc_iin_topoff);
	if (ret) {
		pr_info("%s: max77968,fpdo_dc_input-itopoff is Empty\n", __func__);
		pdata->fpdo_dc_iin_topoff = 1700000; /* 1700mA */
	}
	pr_info("%s: max77968,fpdo_dc_iin_topoff is %d\n", __func__, pdata->fpdo_dc_iin_topoff);

	/* fpdo_dc Vnow topoff condition */
	ret = of_property_read_u32(np_max77968, "max77968,fpdo_dc_vnow-topoff",
							&pdata->fpdo_dc_vnow_topoff);
	if (ret) {
		pr_info("%s: max77968,fpdo_dc_vnow-topoff is Empty\n", __func__);
		pdata->fpdo_dc_vnow_topoff = 5000000; /* Vnow 5000000uV means disable */
	}
	pr_info("%s: max77968,fpdo_dc_vnow_topoff is %d\n", __func__, pdata->fpdo_dc_vnow_topoff);

	/* switching frequency */
	ret = of_property_read_u32(np_max77968, "max77968,fsw_cfg_3to1",
								&pdata->fsw_cfg_3to1);
	if (ret) {
		pr_info("%s: max77968,fsw_cfg_3to1 is Empty\n", __func__);
		pdata->fsw_cfg_3to1 = FSW_CFG_3TO1_DFT;
	}
	pr_info("%s: max77968,fsw_cfg_3to1 is %d\n", __func__, pdata->fsw_cfg_3to1);

	ret = of_property_read_u32(np_max77968, "max77968,fsw_cfg_2to1",
								&pdata->fsw_cfg_2to1);
	if (ret) {
		pr_info("%s: max77968,fsw_cfg_2to1 is Empty\n", __func__);
		pdata->fsw_cfg_3to1 = FSW_CFG_2TO1_DFT;
	}
	pr_info("%s: max77968,fsw_cfg_2to1 is %d\n", __func__, pdata->fsw_cfg_2to1);

	/* switching frequency - bypass */
	ret = of_property_read_u32(np_max77968, "max77968,fsw_cfg_byp",
								&pdata->fsw_cfg_byp);
	if (ret) {
		pr_info("%s: max77968,fsw_cfg_byp is Empty\n", __func__);
		pdata->fsw_cfg_byp = FSW_CFG_BYP_DFT;
	}
	pr_info("%s: max77968,fsw_cfg_byp is %d\n", __func__, pdata->fsw_cfg_byp);

	/* switching frequency for fixed pdo */
	ret = of_property_read_u32(np_max77968, "max77968,fsw_cfg_fpdo",
			&pdata->fsw_cfg_fpdo);
	if (ret) {
		pr_info("%s: max77968,fsw_cfg_fpdo is Empty\n", __func__);
		pdata->fsw_cfg_fpdo = FSW_CFG_2TO1_DFT;
	}
	pr_info("%s: maxim,fsw_cfg_fpdo is %d\n", __func__, pdata->fsw_cfg_fpdo);

	/* NTC over temperature voltage threshold */
	ret = of_property_read_u32(np_max77968, "max77968,ntc-ot-th",
								&pdata->ntc_ot_th);
	if (ret) {
		pr_info("%s: max77968,ntc-ot-th is Empty\n", __func__);
		pdata->ntc_ot_th = NTC_OT_TH_DFT;
	}
	pr_info("%s: max77968,ntc-ot-th is %d\n", __func__, pdata->ntc_ot_th);

	/* NTC NTC over temperature alert enable */
	ret = of_property_read_u32(np_max77968, "max77968,ntc-ot-en",
								&pdata->ntc_ot_en);
	if (ret) {
		pr_info("%s: max77968,ntc-ot-en is Empty\n", __func__);
		pdata->ntc_ot_en = 0;	/* Disable */
	}
	pr_info("%s: max77968,ntc-ot-en is %d\n", __func__, pdata->ntc_ot_en);

	/* Charging mode */
	ret = of_property_read_u32(np_max77968, "max77968,chg-mode",
								&pdata->chg_mode);
	if (ret) {
		pr_info("%s: max77968,charging mode is Empty\n", __func__);
		pdata->chg_mode = CHG_3TO1_DC_MODE;
	}
	pr_info("%s: max77968,chg_mode is %d\n", __func__, pdata->chg_mode);

	/* cv mode polling time in step1 charging */
	ret = of_property_read_u32(np_max77968, "max77968,cv-polling",
								&pdata->cv_polling);
	if (ret) {
		pr_info("%s: max77968,cv-polling is Empty\n", __func__);
		pdata->cv_polling = CVMODE_CHECK_T;
	}
	pr_info("%s: max77968,cv polling is %d\n", __func__, pdata->cv_polling);

	/* vfloat threshold in step1 charging */
	ret = of_property_read_u32(np_max77968, "max77968,step1-vth",
								&pdata->step1_vth);
	if (ret) {
		pr_info("%s: max77968,step1-vfloat-threshold is Empty\n", __func__);
		pdata->step1_vth = STEP1_VFLOAT_THRESHOLD;
	}
	pr_info("%s: max77968,step1_vth is %d\n", __func__, pdata->step1_vth);

	/* read the vbatt value from FG or DC */
	ret = of_property_read_u32(np_max77968, "max77968,vbatt_adc_from",
								&pdata->vbatt_adc_from);
	if (ret) {
		pr_info("%s: max77968,vbatt_adc_from is Empty\n", __func__);
		pdata->vbatt_adc_from = VBATT_FROM_DC;
	}
	pr_info("%s: max77968,vbatt_adc_from is %d\n", __func__, pdata->vbatt_adc_from);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("## %s: np(battery) NULL\n", __func__);
	} else {
		ret = of_property_read_string(np, "battery,charger_name",
				(char const **)&pdata->sec_dc_name);
		if (ret) {
			pr_err("## %s: direct_charger is Empty\n", __func__);
			pdata->sec_dc_name = "sec-direct-charger";
		}
		pr_info("%s: battery,charger_name is %s\n", __func__, pdata->sec_dc_name);

		/* Fuelgauge power supply name */
		ret = of_property_read_string(np, "battery,fuelgauge_name",
				(char const **)&pdata->fg_name);
		if (ret) {
			pr_info("## %s: fuelgauge_name name is Empty\n", __func__);
			pdata->fg_name = "battery";
		}
		pr_info("%s: fuelgauge name is %s\n", __func__, pdata->fg_name);

		/* charging float voltage */
		ret = of_property_read_u32(np, "battery,chg_float_voltage",
									&pdata->vfloat);
		pdata->vfloat *= MAX77968_SEC_DENOM_U_M;
		if (ret) {
			pr_info("%s: battery,dc_float_voltage is Empty\n", __func__);
			pdata->vfloat = VBAT_REG_DFT;
		}
		pr_info("%s: battery,v_float is %d\n", __func__, pdata->vfloat);

		/* the lowest limit to FPDO DC IIN */
		ret = of_property_read_u32(np, "battery,fpdo_dc_charge_power",
									&pdata->fpdo_dc_iin_lowest_limit);
		pdata->fpdo_dc_iin_lowest_limit *= MAX77968_SEC_DENOM_U_M;
		pdata->fpdo_dc_iin_lowest_limit /= MAX77968_SEC_FPDO_DC_IV;
		if (ret) {
			pr_info("%s: battery,fpdo_dc_charge_power is Empty\n", __func__);
			pdata->fpdo_dc_iin_lowest_limit = 10000000; /* 10A */
		}
		pr_info("%s: fpdo_dc_iin_lowest_limit is %d\n", __func__, pdata->fpdo_dc_iin_lowest_limit);
	}
#endif
	return 0;
}
#else
static int max77968_charger_parse_dt(struct device *dev,
				struct max77968_platform_data *pdata)
{
	pdata->iin_cfg = IIN_REG_DFT; /* 3A */
	pdata->vfloat = VBAT_REG_DFT; /* 4.5V */
	pdata->iin_topoff = IIN_DONE_DFT; /* 500mA */
	pdata->fpdo_dc_iin_topoff = 1700000; /* 1700mA */
	pdata->fpdo_dc_vnow_topoff = 5000000; /* Vnow 5000000uV means disable */
	pdata->fsw_cfg_3to1 = FSW_CFG_3TO1_DFT; /* FSW_857kHz */
	pdata->fsw_cfg_2to1 = FSW_CFG_2TO1_DFT; /* FSW_500kHz */
	pdata->fsw_cfg_byp = FSW_CFG_BYP_DFT; /* FSW_857kHz */
	pdata->fsw_cfg_fpdo = FSW_CFG_2TO1_DFT; /* FSW_500kHz */
	pdata->ntc_ot_th = NTC_OT_TH_DFT; /* 1.11V(1110000uV) */
	pdata->ntc_ot_en = 0;	/* Disable */
	pdata->chg_mode = CHG_3TO1_DC_MODE;
	pdata->cv_polling = CVMODE_CHECK_T; /* 2000ms */
	pdata->step1_vth = STEP1_VFLOAT_THRESHOLD; /* 4200000uV - 4.2V */
	pdata->sec_dc_name = "sec-direct-charger";
	pdata->fg_name = "battery";
	pdata->vfloat = VBAT_REG_DFT; /* 4.5V*/
	pdata->fpdo_dc_iin_lowest_limit = 10000000; /* 10A */

	return 0;
}
#endif /* CONFIG_OF */

static int max77968_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < (int)ARRAY_SIZE(max77968_chg_attrs); i++) {
		rc = device_create_file(dev, &max77968_chg_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	return rc;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &max77968_chg_attrs[i]);
	return rc;
}


ssize_t max77968_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max77968_charger *max77968 = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - max77968_chg_attrs;
	int i = 0;
	u8 addr = 0;
	unsigned int data = 0;
	union power_supply_propval value = {0, };

	dev_info(max77968->dev, "%s: (%ld)\n", __func__, (long)offset);

	switch (offset) {
	case CHG_DATA:
		for (addr = 0; addr < MAX77968_MAX_REG; addr++) {
			max77968_read_reg(max77968, addr, &data);
			i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x:\t0x%04x\n", addr, data);
		}
		break;
	case DC_ERR_CAUSE:
		i = snprintf(buf, PAGE_SIZE, "ERR NODE 0x%06x\n", max77968->error_cause);
		break;
	case PLATFORM_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "irq_gpio: %d\n", max77968->pdata->irq_gpio);
		i += scnprintf(buf + i, PAGE_SIZE - i, "iin_cfg: %d\n", max77968->pdata->iin_cfg);
		i += scnprintf(buf + i, PAGE_SIZE - i, "ichg_cfg: %d\n", max77968->pdata->ichg_cfg);
		i += scnprintf(buf + i, PAGE_SIZE - i, "vfloat: %d\n", max77968->pdata->vfloat);
		i += scnprintf(buf + i, PAGE_SIZE - i, "iin_topoff: %d\n", max77968->pdata->iin_topoff);
		i += scnprintf(buf + i, PAGE_SIZE - i, "fpdo_dc_iin_topoff: %d\n", max77968->pdata->fpdo_dc_iin_topoff);
		i += scnprintf(buf + i, PAGE_SIZE - i, "fpdo_dc_vnow_topoff: %d\n", max77968->pdata->fpdo_dc_vnow_topoff);
		i += scnprintf(buf + i, PAGE_SIZE - i, "fpdo_dc_iin_lowest_limit: %d\n", max77968->pdata->fpdo_dc_iin_lowest_limit);
		i += scnprintf(buf + i, PAGE_SIZE - i, "fsw_cfg_3to1: %d\n", max77968->pdata->fsw_cfg_3to1);
		i += scnprintf(buf + i, PAGE_SIZE - i, "fsw_cfg_2to1: %d\n", max77968->pdata->fsw_cfg_2to1);
		i += scnprintf(buf + i, PAGE_SIZE - i, "fsw_cfg_byp: %d\n", max77968->pdata->fsw_cfg_byp);
		i += scnprintf(buf + i, PAGE_SIZE - i, "fsw_cfg_fpdo: %d\n", max77968->pdata->fsw_cfg_fpdo);
		i += scnprintf(buf + i, PAGE_SIZE - i, "ntc_ot_th: %d\n", max77968->pdata->ntc_ot_th);
		i += scnprintf(buf + i, PAGE_SIZE - i, "ntc_ot_en: %d\n", max77968->pdata->ntc_ot_en);
		i += scnprintf(buf + i, PAGE_SIZE - i, "chg_mode: %d\n", max77968->pdata->chg_mode);
		i += scnprintf(buf + i, PAGE_SIZE - i, "cv_polling: %d\n", max77968->pdata->cv_polling);
		i += scnprintf(buf + i, PAGE_SIZE - i, "step1_vth: %d\n", max77968->pdata->step1_vth);
		i += scnprintf(buf + i, PAGE_SIZE - i, "vbatt_adc_from: %d\n", max77968->pdata->vbatt_adc_from);
		break;
	case REV_MODE:
		psy_do_property("max77968-charger", get,
				POWER_SUPPLY_EXT_PROP_DC_REVERSE_MODE, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	default:
		return -EINVAL;
	}
	return i;
}

ssize_t max77968_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max77968_charger *max77968 = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - max77968_chg_attrs;
	int ret = 0, x, y;
	char *args[3];
	char *sep = ",\t";
	char *buff = (char *)buf;
	unsigned long val;
	union power_supply_propval value = {0, };

	dev_info(max77968->dev, "%s: (%ld)\n", __func__, (long)offset);

	switch (offset) {
	case CHG_DATA:
		if (sscanf(buf, "0x%8x 0x%8x", &x, &y) == 2) {
			u8 addr = x;
			u16 data = y;

			dev_info(max77968->dev, "%s: addr: 0x%x write 0x%x\n",	__func__, addr, data);
		}
		ret = count;
		break;

	case PLATFORM_DATA:
		args[0] = strsep(&buff, sep);
		if (args[0] == NULL)
			return -2;
		args[1] = strsep(&buff, sep);
		if (strncmp("fsw_cfg_3to1", args[0], 12) == 0) {
			ret = kstrtoul(args[1], 0, &val);
			if ((ret != 0) || (val > FSW_1500kHz))
				return -EINVAL;
			max77968->pdata->fsw_cfg_3to1 = (unsigned int)val;
			dev_info(max77968->dev, "%s: fsw_cfg_3to1: 0x%x\n",
					__func__, max77968->pdata->fsw_cfg_3to1);
			ret = count;
		} else if (strncmp("fsw_cfg_2to1", args[0], 12) == 0) {
			ret = kstrtoul(args[1], 0, &val);
			if ((ret != 0) || (val > FSW_1500kHz))
				return -EINVAL;
			max77968->pdata->fsw_cfg_2to1 = (unsigned int)val;
			dev_info(max77968->dev, "%s: fsw_cfg_2to1: 0x%x\n",
					__func__, max77968->pdata->fsw_cfg_2to1);
			ret = count;
		} else {
			pr_err("%s: command not support\n", __func__);
			return -EINVAL;
		}
		break;
	case REV_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			pr_info("%s: set rev mode(%d)\n", __func__, value.intval);
			psy_do_property("max77968-charger", set,
				POWER_SUPPLY_EXT_PROP_DC_REVERSE_MODE, value);
			ret = count;
		} else {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int read_reg(void *data, u64 *val)
{
	struct max77968_charger *max77968 = data;
	int rc;
	unsigned int temp;

	rc = max77968_read_reg(max77968, max77968->debug_address, &temp);
	if (rc) {
		pr_err("Couldn't read reg %x rc = %d\n",
			max77968->debug_address, rc);
		return -EAGAIN;
	}
	*val = temp;
	return 0;
}

static int write_reg(void *data, u64 val)
{
	struct max77968_charger *max77968 = data;
	int rc;
	u8 temp;

	temp = (u8) val;
	rc = max77968_write_reg(max77968, max77968->debug_address, temp);
	if (rc) {
		pr_err("Couldn't write 0x%02x to 0x%02x rc= %d\n",
			temp, max77968->debug_address, rc);
		return -EAGAIN;
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(register_debug_ops, read_reg, write_reg, "0x%02llx\n");

static int max77968_create_debugfs_entries(struct max77968_charger *max77968)
{
	struct dentry *ent;
	int rc = 0;

	max77968->debug_root = debugfs_create_dir("charger-max77968", NULL);
	if (!max77968->debug_root) {
		dev_err(max77968->dev, "Couldn't create debug dir\n");
		rc = -ENOENT;
	} else {
		debugfs_create_x32("address", 0644,
					max77968->debug_root, &(max77968->debug_address));

		ent = debugfs_create_file("data", 0644,
					max77968->debug_root, max77968,
					&register_debug_ops);
		if (!ent) {
			dev_err(max77968->dev,
				"Couldn't create data debug file\n");
			rc = -ENOENT;
		}
	}

	return rc;
}

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static int max77968_probe(struct i2c_client *client)
#else
static int max77968_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	struct power_supply_config mains_cfg = {};
	struct max77968_platform_data *pdata;
	struct device *dev = &client->dev;
	struct max77968_charger *max77968_chg;
	int ret;

	dev_info(&client->dev, "%s: MAX77968 Charger Driver Loading\n", __func__);

	max77968_chg = devm_kzalloc(dev, sizeof(*max77968_chg), GFP_KERNEL);
	if (!max77968_chg)
		return -ENOMEM;

#if defined(CONFIG_OF)
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(struct max77968_platform_data), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;

		ret = max77968_charger_parse_dt(&client->dev, pdata);
		if (ret < 0) {
			dev_err(&client->dev, "Failed to get device of_node\n");
			return -ENOMEM;
		}

		client->dev.platform_data = pdata;
	} else {
		dev_info(&client->dev, "%s: empty device of_node\n", __func__);
		pdata = client->dev.platform_data;
	}
#else
	pdata = dev->platform_data;
#endif
	if (!pdata)
		return -EINVAL;

	i2c_set_clientdata(client, max77968_chg);

	mutex_init(&max77968_chg->lock);
	mutex_init(&max77968_chg->i2c_lock);

	mutex_lock(&max77968_chg->lock);
	max77968_chg->timer_id = TIMER_ID_NONE;
	max77968_chg->timer_period = 0;
	mutex_unlock(&max77968_chg->lock);

	max77968_chg->dev = &client->dev;
	max77968_chg->pdata = pdata;
	max77968_chg->charging_state = DC_STATE_NO_CHARGING;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_chg->wdt_kick_disable = false;
#endif

	/* Create a work queue for the direct charger */
	max77968_chg->dc_wq = alloc_ordered_workqueue("max77968_dc_wq", WQ_MEM_RECLAIM);
	if (max77968_chg->dc_wq == NULL) {
		dev_err(max77968_chg->dev, "failed to create work queue\n");
		ret = -ENOMEM;
		goto error;
	}

	/* initialize work */
	max77968_chg->monitor_wake_lock = wakeup_source_register(&client->dev, "max77968-charger-monitor");
	INIT_DELAYED_WORK(&max77968_chg->timer_work, max77968_timer_work);
	INIT_DELAYED_WORK(&max77968_chg->pps_work, max77968_pps_request_work);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	INIT_DELAYED_WORK(&max77968_chg->wdt_control_work, max77968_wdt_control_work);
#endif

	/* initialize Charger Register Map */
	max77968_chg->regmap = devm_regmap_init_i2c(client, &max77968_regmap);
	if (IS_ERR(max77968_chg->regmap)) {
		ret = PTR_ERR(max77968_chg->regmap);
		goto error;
	}

#if defined(ISSUE_WORKAROUND)
	mutex_init(&max77968_chg->tregmap_lock);

	max77968_chg->otpid = i2c_new_dummy_device(client->adapter, I2C_ADDR_OTP);
	if (IS_ERR(max77968_chg->otpid)) {
		ret = PTR_ERR(max77968_chg->otpid);
		goto error;
	}
	i2c_set_clientdata(max77968_chg->otpid, max77968_chg);
	max77968_chg->otpregmap = devm_regmap_init_i2c(max77968_chg->otpid, &max77968_otpregmap);
	if (IS_ERR(max77968_chg->otpregmap)) {
		ret = PTR_ERR(max77968_chg->otpregmap);
		goto error;
	}
	max77968_chg->tsid = i2c_new_dummy_device(client->adapter, I2C_ADDR_TEST);
	if (IS_ERR(max77968_chg->tsid)) {
		ret = PTR_ERR(max77968_chg->tsid);
		goto error;
	}
	i2c_set_clientdata(max77968_chg->tsid, max77968_chg);
	max77968_chg->tregmap = devm_regmap_init_i2c(max77968_chg->tsid, &max77968_tregmap);
	if (IS_ERR(max77968_chg->tregmap)) {
		ret = PTR_ERR(max77968_chg->tregmap);
		goto error;
	}
	max77968_adc_circ_buf_init(max77968_chg);
#endif

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	max77968_init_adc_val(max77968_chg, -1);
#endif

	ret = max77968_hw_init(max77968_chg);
	if (ret < 0)
		goto error;

	mains_cfg.supplied_to = max77968_supplied_to;
	mains_cfg.num_supplicants = ARRAY_SIZE(max77968_supplied_to);
	mains_cfg.drv_data = max77968_chg;
	max77968_chg->mains = power_supply_register(dev, &max77968_mains_desc, &mains_cfg);
	if (IS_ERR(max77968_chg->mains)) {
		ret = PTR_ERR(max77968_chg->mains);
		goto error;
	}

	/*
	 * Interrupt pin is optional. If it is connected, we setup the
	 * interrupt support here.
	 */
	if (pdata->irq_gpio >= 0) {
		ret = max77968_irq_init(max77968_chg, client);
		if (ret < 0) {
			dev_warn(dev, "failed to initialize IRQ: %d\n", ret);
			dev_warn(dev, "disabling IRQ support\n");
		}
		/* disable interrupt */
		disable_irq(client->irq);
	}

	ret = max77968_create_debugfs_entries(max77968_chg);
	if (ret < 0)
		goto error;

	sec_chg_set_dev_init(SC_DEV_DIR_CHG);

	ret = max77968_chg_create_attrs(&max77968_chg->mains->dev);
	if (ret < 0)
		dev_err(&client->dev, "%s: failed to create_attrs\n", __func__);

	max77968_chg->valid_ic = true;
	atomic_set(&max77968_chg->shutdown, 0);
	atomic_set(&max77968_chg->suspend, 0);

	dev_info(&client->dev, "%s: MAX77968 Charger Driver Loaded, VER=%s\n",
				__func__, MAX77968_DRV_VER);
	return 0;

error:
	if (max77968_chg->dc_wq) {
		destroy_workqueue(max77968_chg->dc_wq);
		max77968_chg->dc_wq = NULL;
	}
	if (max77968_chg->monitor_wake_lock) {
		wakeup_source_unregister(max77968_chg->monitor_wake_lock);
		max77968_chg->monitor_wake_lock = NULL;
	}
	if (max77968_chg->mains) {
		power_supply_unregister(max77968_chg->mains);
		max77968_chg->mains = NULL;
	}
#if defined(ISSUE_WORKAROUND)
	if (max77968_chg->otpid) {
		i2c_unregister_device(max77968_chg->otpid);
		max77968_chg->otpid = NULL;
	}
	if (max77968_chg->tsid) {
		i2c_unregister_device(max77968_chg->tsid);
		max77968_chg->tsid = NULL;
	}
	mutex_destroy(&max77968_chg->tregmap_lock);
#endif
	mutex_destroy(&max77968_chg->lock);
	mutex_destroy(&max77968_chg->i2c_lock);
	return ret;
}

#if KERNEL_VERSION(6, 1, 0) > LINUX_VERSION_CODE
static int max77968_remove(struct i2c_client *client)
#else
static void max77968_remove(struct i2c_client *client)
#endif
{
	struct max77968_charger *max77968 = i2c_get_clientdata(client);

	pr_info("%s: ++\n", __func__);

	/* stop charging if it is active */
	max77968_stop_charging(max77968);

	if (client->irq) {
		free_irq(client->irq, max77968);
		gpio_free(max77968->pdata->irq_gpio);
	}

	/* Delete the work queue */
	if (max77968->dc_wq) {
		destroy_workqueue(max77968->dc_wq);
		max77968->dc_wq = NULL;
	}

	if (max77968->monitor_wake_lock) {
		wakeup_source_unregister(max77968->monitor_wake_lock);
		max77968->monitor_wake_lock = NULL;
	}

	if (max77968->mains) {
		power_supply_put(max77968->mains);
		power_supply_unregister(max77968->mains);
		max77968->mains = NULL;
	}

	debugfs_remove(max77968->debug_root);
	max77968->debug_root = NULL;

	mutex_destroy(&max77968->lock);
	mutex_destroy(&max77968->i2c_lock);
#if defined(ISSUE_WORKAROUND)
	mutex_destroy(&max77968->tregmap_lock);
#endif

	pr_info("%s: --\n", __func__);

#if KERNEL_VERSION(6, 1, 0) > LINUX_VERSION_CODE
	return 0;
#endif
}

static void max77968_shutdown(struct i2c_client *client)
{
	struct max77968_charger *max77968 = i2c_get_clientdata(client);

	pr_info("%s: ++\n", __func__);

	atomic_set(&max77968->shutdown, 1);

	if (max77968_set_charging(max77968, false) < 0)
		pr_info("%s: failed to disable charging\n", __func__);

	if (client->irq)
		free_irq(client->irq, max77968);

	cancel_delayed_work(&max77968->timer_work);
	cancel_delayed_work(&max77968->pps_work);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	cancel_delayed_work(&max77968->wdt_control_work);
#endif

	if (max77968->dc_wq) {
		destroy_workqueue(max77968->dc_wq);
		max77968->dc_wq = NULL;
	}

	if (max77968->monitor_wake_lock) {
		wakeup_source_unregister(max77968->monitor_wake_lock);
		max77968->monitor_wake_lock = NULL;
	}

	pr_info("%s: --\n", __func__);
}

static const struct i2c_device_id max77968_id[] = {
	{ "max77968-charger", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77968_id);

#if defined(CONFIG_OF)
static const struct of_device_id max77968_i2c_dt_ids[] = {
	{ .compatible = "maxim,max77968" },
	{ },
};
MODULE_DEVICE_TABLE(of, max77968_i2c_dt_ids);
#endif /* CONFIG_OF */

#if defined(CONFIG_PM)
static int max77968_suspend(struct device *dev)
{
	struct max77968_charger *max77968 = dev_get_drvdata(dev);

	pr_info("%s: ++\n", __func__);

	atomic_set(&max77968->suspend, 1);

	if (max77968->timer_id != TIMER_ID_NONE) {
		pr_debug("%s: cancel delayed work\n", __func__);

		/* cancel delayed_work */
		cancel_delayed_work(&max77968->timer_work);
		cancel_delayed_work(&max77968->pps_work);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		cancel_delayed_work(&max77968->wdt_control_work);
#endif
	}

	pr_info("%s: --\n", __func__);

	return 0;
}

static int max77968_resume(struct device *dev)
{
	struct max77968_charger *max77968 = dev_get_drvdata(dev);

	pr_info("%s: ++\n", __func__);

	atomic_set(&max77968->suspend, 0);

	if (max77968->timer_id != TIMER_ID_NONE) {
		pr_debug("%s: update_timer\n", __func__);

		/* Update the current timer */
		mutex_lock(&max77968->lock);
		max77968->timer_period = 0;	// ms unit
		mutex_unlock(&max77968->lock);
		queue_delayed_work(max77968->dc_wq,
				&max77968->timer_work,
				msecs_to_jiffies(max77968->timer_period));
	}

	pr_info("%s: --\n", __func__);
	return 0;
}
#else
#define max77968_suspend		NULL
#define max77968_resume		NULL
#endif

const struct dev_pm_ops max77968_pm_ops = {
	.suspend = max77968_suspend,
	.resume = max77968_resume,
};

static struct i2c_driver max77968_driver = {
	.driver = {
		.name = "max77968-charger",
#if defined(CONFIG_OF)
		.of_match_table = max77968_i2c_dt_ids,
#endif /* CONFIG_OF */
#if defined(CONFIG_PM)
		.pm = &max77968_pm_ops,
#endif
	},
	.probe        = max77968_probe,
	.remove       = max77968_remove,
	.shutdown     = max77968_shutdown,
	.id_table     = max77968_id,
};

module_i2c_driver(max77968_driver);

MODULE_DESCRIPTION("MAX77968 charger driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(MAX77968_DRV_VER);
