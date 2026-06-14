/*
 *  stk6d2x_sec.c - Linux kernel modules for sensortek stk6d2x
 *  ambient light sensor (sec)
 *
 *  Copyright (C) 2012~2018 Bk, sensortek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/pm.h>
//#include <linux/wakelock.h>
#include <linux/interrupt.h>
//#include <linux/sensors.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_OF
	#include <linux/of_gpio.h>
#endif
#include "stk6d2x.h"
#include "stk6d2x_sec.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

char driver_ver[20] = STR(STK6D2X_MAJOR) "." STR(STK6D2X_MINOR) "." STR(STK6D2X_REV);

int als_debug = 1;
int als_info = 0;
static int probe_error;

static int flicker_param_lpcharge = 0;
module_param(flicker_param_lpcharge, int, 0440);

module_param(als_debug, int, S_IRUGO | S_IWUSR);
module_param(als_info, int, S_IRUGO | S_IWUSR);

/****************************************************************************************************
* Declaration function
****************************************************************************************************/
#ifdef STK_ALS_CALI
static ssize_t stk6d2x_cali_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	unsigned int data;
	int error;
	error = kstrtouint(buf, 10, &data);

	if (error)
	{
		dev_err(&stk_wrapper->i2c_mgr.client->dev, "%s: kstrtoul failed, error=%d\n",
				__func__, error);
		return error;
	}

	dev_err(&stk_wrapper->i2c_mgr.client->dev, "%s: Enable ALS calibration: %d\n", __func__, data);

	if (0x1 == data)
	{
		stk6d2x_cali_als(alps_data);
	}

	return size;
}
#endif

/****************************************************************************************************
* ALS control API
****************************************************************************************************/
static void stk_als_report(struct stk6d2x_data *alps_data, uint32_t *als, uint32_t num)
{
	//stk6d2x_wrapper *stk_wrapper = container_of(alps_data, stk6d2x_wrapper, alps_data);
	int i = 0;
	//input_report_abs(stk_wrapper->als_input_dev, ABS_MISC, *als);
	//input_sync(stk_wrapper->als_input_dev);

	if (!num)
		return;

	for (i=0; i < num; i ++)
	{
		//APS_ERR("als[%d] input event %d", i, *(uint32_t *)(als + i));
	}
}

void stk_sec_report(struct stk6d2x_data *alps_data)
{
	stk6d2x_wrapper *stk_wrapper = container_of(alps_data, stk6d2x_wrapper, alps_data);
	static int cnt=0;
	uint32_t temp_clear = (uint32_t)(((uint64_t)(alps_data->clear_local_average)) * 35LL / 100LL);
	uint32_t temp_ir = alps_data->ir_local_average;
	uint32_t temp_uv = (uint32_t)(((uint64_t)(alps_data->uv_local_average)) * 79LL / 100LL);

	if (cnt++ >= 10) {
		ALS_info("IR:%llu Clear:%llu UV:%llu  ir_gain:%d clear_gain:%d uv_gain:%d| Flicker:%dHz\n",
			alps_data->ir_local_average, alps_data->clear_local_average, alps_data->uv_local_average,
			alps_data->ir_gain, alps_data->clear_gain, alps_data->uv_gain, alps_data->flicker);
		ALS_info("AWB: IR*:%d Clear*:%d UV*:%d\n", temp_ir, temp_clear, temp_uv);
		cnt = 0;
	}

	input_report_rel(stk_wrapper->als_input_dev, REL_X, temp_ir + 1);
	input_report_rel(stk_wrapper->als_input_dev, REL_RX, temp_uv + 1);
	input_report_rel(stk_wrapper->als_input_dev, REL_RY, temp_clear + 1);
	input_report_abs(stk_wrapper->als_input_dev, ABS_Y, alps_data->clear_gain + 1);
	input_report_abs(stk_wrapper->als_input_dev, ABS_Z, alps_data->ir_gain + 1);
	input_report_abs(stk_wrapper->als_input_dev, ABS_RX, alps_data->uv_gain + 1);
	input_sync(stk_wrapper->als_input_dev);

#if IS_ENABLED(CONFIG_SENSORS_FLICKER_SELF_TEST)
	als_eol_update_als(temp_ir, temp_clear, temp_ir, temp_uv);
#endif
}

int32_t stk_power_ctrl(struct stk6d2x_data *alps_data, bool en)
{
	int rc = 0;

	ALS_info("enable = %s state : %d\n", en?"ON":"OFF", alps_data->regulator_state);

	if (en) {
		if (alps_data->regulator_state != 0) {
			alps_data->regulator_state++;
			ALS_dbg("duplicate regulator (increase state : %d)\n", alps_data->regulator_state);
			return 0;
		}

		if (alps_data->regulator_vbus_1p8 != NULL) {
			if (!alps_data->vbus_1p8_enable) {
				rc = regulator_enable(alps_data->regulator_vbus_1p8);
				if (rc) {
					ALS_err("enable vbus_1p8 failed, rc=%d\n", rc);
					goto enable_vbus_1p8_failed;
				} else {
					alps_data->vbus_1p8_enable = true;
					ALS_dbg("enable vbus_1p8 done, rc=%d\n", rc);
				}
			} else {
				ALS_dbg("vbus_1p8 already enabled, en=%d\n", alps_data->vbus_1p8_enable);
			}
		}

		if (alps_data->regulator_vdd_1p8 != NULL) {
			if (!alps_data->vdd_1p8_enable) {
				rc = regulator_enable(alps_data->regulator_vdd_1p8);
				if (rc) {
					ALS_err("enable vdd_1p8 failed, rc=%d\n", rc);
					goto enable_vdd_1p8_failed;
				} else {
					alps_data->vdd_1p8_enable = true;
					ALS_info("enable vdd_1p8 done, (state : %d), rc=%d\n", (alps_data->regulator_state + 1), rc);
				}
			} else {
				ALS_dbg("vdd_1p8 already enabled, en=%d\n", alps_data->vdd_1p8_enable);
			}
		}

		alps_data->regulator_state++;
		alps_data->pm_state = PM_RESUME;
	} else {

		if (alps_data->regulator_state == 0) {
			ALS_dbg("already off the regulator\n");
			return 0;
		} else if (alps_data->regulator_state != 1) {
			alps_data->regulator_state--;
			ALS_dbg("duplicate regulator (decrease state : %d)\n", alps_data->regulator_state);
			return 0;
		}
		alps_data->regulator_state--;

		if (alps_data->regulator_vdd_1p8 != NULL) {
			if (alps_data->vdd_1p8_enable) {
				rc = regulator_disable(alps_data->regulator_vdd_1p8);
				if (rc) {
					ALS_err("disable vdd_1p8 failed, rc=%d\n", rc);
				} else {
					alps_data->vdd_1p8_enable = false;
					ALS_dbg("disable vdd_1p8 done, (state : %d), rc=%d\n", alps_data->regulator_state, rc);
				}
			} else {
				ALS_dbg("vdd_1p8 already disabled, en=%d\n", alps_data->vdd_1p8_enable);
			}
		}

		if (alps_data->regulator_vbus_1p8 != NULL) {
			if (alps_data->vbus_1p8_enable) {
				rc = regulator_disable(alps_data->regulator_vbus_1p8);
				if (rc) {
					ALS_err("disable vbus_1p8 failed, rc=%d\n", rc);
				} else {
					alps_data->vbus_1p8_enable = false;
					ALS_dbg("disable vbus_1p8 done, rc=%d\n", rc);
				}
			} else {
				ALS_dbg("vbus_1p8 already disabled, en=%d\n", alps_data->vbus_1p8_enable);
			}
		}
	}

	goto done;

enable_vdd_1p8_failed:
	if (alps_data->regulator_vbus_1p8 != NULL) {
		if (alps_data->vbus_1p8_enable) {
			rc = regulator_disable(alps_data->regulator_vbus_1p8);
			if (rc) {
				ALS_err("disable vbus_1p8 failed, rc=%d\n", rc);
			} else {
				alps_data->vbus_1p8_enable = false;
				ALS_dbg("disable vbus_1p8 done, rc=%d\n", rc);
			}
		} else {
			ALS_dbg("vbus_1p8 already disabled, en=%d\n", alps_data->vbus_1p8_enable);
		}
	}

done:
enable_vbus_1p8_failed:
	return rc;
}

static ssize_t stk_als_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	return scnprintf(buf, PAGE_SIZE, "%u\n", (uint32_t)alps_data->als_info.last_raw_data[0]);
}

#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
static ssize_t stk_rear_als_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;
	bool value;

	mutex_lock(&data->enable_lock);

	if (strtobool(buf, &value)) {
		mutex_unlock(&data->enable_lock);
		return -EINVAL;
	}

	ALS_info("en : %d, c : %d\n", value, data->als_info.enable);
	if (data->als_flag == value) {
		mutex_unlock(&data->enable_lock);
		return size;
	}

	data->als_flag = value;

	if (value)
		stk_als_start(data);
	else
		stk_als_stop(data);

	mutex_unlock(&data->enable_lock);

	return size;
}

static ssize_t stk_rear_als_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;

	return snprintf(buf, PAGE_SIZE, "%u\n", data->als_flag);
}

static ssize_t stk_rear_als_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	uint32_t rawdata[3];
	int ret = 0;

	mutex_lock(&alps_data->enable_lock);
	if (alps_data->als_info.enable && alps_data->als_flag) {
		if (!alps_data->flicker_flag) {
			uint8_t flag_value;

			if (STK6D2X_REG_READ(alps_data, STK6D2X_REG_FLAG, &flag_value) < 0) {
				ret = snprintf(buf, PAGE_SIZE, "-6, -6\n");
				goto err;
			}
			else if (flag_value & STK6D2X_FLG_ALSSAT_MASK) {
				ret = snprintf(buf, PAGE_SIZE, "-2, -2\n");
				goto err;
			}

			if (stk6d2x_als_get_data(alps_data, 0) < 0) {
				ret = snprintf(buf, PAGE_SIZE, "-6, -6\n");
				goto err;
			}

			stk6d2x_get_curGain(alps_data);
			stk6d2x_get_als_ratio(alps_data);

			rawdata[0] = alps_data->als_info.last_raw_data[0] * alps_data->als_info.als_cur_ratio[0];
			rawdata[1] = alps_data->als_info.last_raw_data[1] * alps_data->als_info.als_cur_ratio[1];
			rawdata[2] = alps_data->als_info.last_raw_data[2] * alps_data->als_info.als_cur_ratio[2];
		} else {
			rawdata[0] = (uint32_t)alps_data->clear_local_average * 110; /* 40ms / 360 us */
			rawdata[1] = (uint32_t)alps_data->uv_local_average * 110;
			rawdata[2] = (uint32_t)alps_data->ir_local_average * 55;
		}
		ret = snprintf(buf, PAGE_SIZE, "%u, %u\n", rawdata[0], rawdata[2]);
	} else {
		ret = snprintf(buf, PAGE_SIZE, "-1, -1\n");
	}
err:
	mutex_unlock(&alps_data->enable_lock);
	return  ret;
}
#endif /* CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS */

static ssize_t stk_als_enable_show(struct device *dev,
								   struct device_attribute *attr,
								   char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	int32_t ret;
	uint8_t data;
	ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_STATE, &data);

	if (ret < 0)
		return ret;

	data = (data & STK6D2X_STATE_EN_ALS_MASK) ? 1 : 0;
	return scnprintf(buf, PAGE_SIZE, "%d\n", data);
}

static ssize_t stk_als_enable_store(struct device *dev,
									struct device_attribute *attr,
									const char *buf,
									size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	unsigned int data;
	int error;

	error = kstrtouint(buf, 10, &data);
	if (error) {
		dev_err(&stk_wrapper->i2c_mgr.client->dev, "%s: kstrtoul failed, error=%d\n",
				__func__, error);
		return error;
	}

#if IS_ENABLED(CONFIG_SENSORS_FLICKER_SELF_TEST)
	if (alps_data->eol_enabled) {
		dev_err(&stk_wrapper->i2c_mgr.client->dev, "%s: TEST RUNNING. recover %d after finish test", __func__, data);
		alps_data->recover_state = data;
		return size;
	}
#endif

	dev_info(&stk_wrapper->i2c_mgr.client->dev, "%s: Enable ALS : %d\n", __func__, data);

	if ((1 == data) || (0 == data))	{
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
		mutex_lock(&alps_data->enable_lock);

		alps_data->flicker_flag = (bool)data;

		if (alps_data->flicker_flag && alps_data->als_flag) {
			stk6d2x_enable_als(alps_data, false);
			stk6d2x_enable_fifo(alps_data, false);
		}
#endif
		stk6d2x_alps_set_config(alps_data, data);
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
		if (!alps_data->flicker_flag && alps_data->als_flag)
			stk_als_start(alps_data);

		mutex_unlock(&alps_data->enable_lock);
#endif
	}

	return size;
}

static ssize_t stk_als_lux_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	return scnprintf(buf, PAGE_SIZE, "%llu lux\n", alps_data->als_info.last_raw_data[0] * alps_data->als_info.scale / 1000);
}

#if 0
static ssize_t stk_als_lux_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;

	unsigned long value = 0;
	int ret;
	ret = kstrtoul(buf, 16, &value);

	if (ret < 0)
	{
		ALS_err("kstrtoul failed, ret=0x%x\n", ret);
		return ret;
	}

	//stk_als_report(alps_data, value);
	STK6D2X_ALS_REPORT(alps_data, value, 3);
	return size;
}
#endif

static ssize_t stk_als_transmittance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	int32_t transmittance;
	transmittance = alps_data->als_info.scale;
	return scnprintf(buf, PAGE_SIZE, "%d\n", transmittance);
}

static ssize_t stk_als_transmittance_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	unsigned long value = 0;
	int ret;
	ret = kstrtoul(buf, 10, &value);

	if (ret < 0)
	{
		ALS_err("kstrtoul failed, ret=0x%x\n", ret);
		return ret;
	}

	alps_data->als_info.scale = value;
	return size;
}

static ssize_t stk6d2x_als_registry_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	stk6d2x_update_registry(alps_data);
	return scnprintf(buf, PAGE_SIZE, "als scale = %d (Ver. %d)\n",
					 alps_data->cali_info.cali_para.als_scale,
					 alps_data->cali_info.cali_para.als_version);
}

static ssize_t stk6d2x_registry_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	unsigned long value = 0;
	int ret;
	ret = kstrtoul(buf, 10, &value);

	if (ret < 0) {
		ALS_err("kstrtoul failed, ret=0x%x\n", ret);
		return ret;
	}

	ret = stk6d2x_update_registry(alps_data);

	if (ret < 0)
		ALS_err("update registry fail!\n");
	else
		ALS_err("update registry success!\n");

	return size;
}

#ifdef STK_ALS_CALI
static ssize_t stk6d2x_als_cali_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	stk6d2x_request_registry(alps_data);
	return scnprintf(buf, PAGE_SIZE, "als scale = %d (Ver. %d)\n",
					 alps_data->cali_info.cali_para.als_scale,
					 alps_data->cali_info.cali_para.als_version);
}
#endif

static ssize_t stk_recv_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&stk_wrapper->recv_reg));
}

static ssize_t stk_recv_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long value = 0;
	int ret;
	uint8_t recv_data;
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;

	if ((ret = kstrtoul(buf, 16, &value)) < 0) {
		ALS_err("kstrtoul failed, ret=0x%x\n", ret);
		return ret;
	}

	atomic_set(&stk_wrapper->recv_reg, 0);
	STK6D2X_REG_READ(alps_data, value, &recv_data);
	atomic_set(&stk_wrapper->recv_reg, recv_data);
	return size;
}

static ssize_t stk_send_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t stk_send_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned char addr, cmd;
	unsigned long temp_addr, temp_cmd;
	int32_t ret, i;
	char *token[10];
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;

	for (i = 0; i < 2; i++)
		token[i] = strsep((char **)&buf, " ");

	if ((ret = kstrtoul(token[0], 16, &temp_addr)) < 0) {
		ALS_err("kstrtoul failed, ret=0x%x\n", ret);
		return ret;
	}

	if ((ret = kstrtoul(token[1], 16, &temp_cmd)) < 0) {
		ALS_err("kstrtoul failed, ret=0x%x\n", ret);
		return ret;
	}

	addr = temp_addr & 0xFF;
	cmd = temp_cmd & 0xFF;
	ALS_info("write reg 0x%x=0x%x\n", addr, cmd);
	ret = STK6D2X_REG_WRITE(alps_data, addr, cmd);

	if (0 != ret) {
		ALS_err("stk6d2x_i2c_smbus_write_byte_data fail\n");
		return ret;
	}

	return size;
}

static ssize_t stk_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", STK6D2X_DEV_NAME);
}

static ssize_t stk_flush_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;
	int ret = 0;
	u8 handle = 0;

	ret = kstrtou8(buf, 10, &handle);
	if (ret < 0) {
		ALS_err("kstrtou8 failed.(%d)\n", ret);
		return ret;
	}
	input_report_rel(stk_wrapper->als_input_dev, REL_MISC, handle);
	ALS_info("flush done\n");

	return size;
}

static ssize_t stk_nothing_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t stk_nothing_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static ssize_t stk_factory_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;

	ALS_dbg("isTrimmed = %d\n", data->isTrimmed);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->isTrimmed);
}

static ssize_t als_ir_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;
	int waiting_cnt = 0;

	while (!data->is_local_avg_update && waiting_cnt < 100) {
		waiting_cnt++;
		msleep_interruptible(10);
	}

	ALS_dbg("read als_ir = %llu (is_local_avg_update = %d, waiting_cnt = %d)\n",
		data->ir_local_average, data->is_local_avg_update, waiting_cnt);
	return snprintf(buf, PAGE_SIZE, "%llu\n", data->ir_local_average);
}

static ssize_t als_clear_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;
	uint32_t temp_clear;
	int waiting_cnt = 0;

	while (!data->is_local_avg_update && waiting_cnt < 100) {
		waiting_cnt++;
		msleep_interruptible(10);
	}

	temp_clear = (uint32_t)(((uint64_t)(data->clear_local_average)) * 35LL / 100LL);

	ALS_dbg("read als_clear = %d (is_local_avg_update = %d, waiting_cnt = %d)\n",
		temp_clear, data->is_local_avg_update, waiting_cnt);
	return snprintf(buf, PAGE_SIZE, "%d\n", temp_clear);
}

static ssize_t als_wideband_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;
	int waiting_cnt = 0;

	while (!data->is_local_avg_update && waiting_cnt < 100) {
		waiting_cnt++;
		msleep_interruptible(10);
	}

	ALS_dbg("read als_wideband = %llu (is_local_avg_update = %d, waiting_cnt = %d)\n",
		data->ir_local_average, data->is_local_avg_update, waiting_cnt);
	return snprintf(buf, PAGE_SIZE, "%llu\n", data->ir_local_average);
}

static ssize_t als_uv_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;
	uint32_t temp_uv;
	int waiting_cnt = 0;

	while (!data->is_local_avg_update && waiting_cnt < 100) {
		waiting_cnt++;
		msleep_interruptible(10);
	}

	temp_uv = (uint32_t)(((uint64_t)(data->uv_local_average)) * 79LL / 100LL);

	ALS_dbg("read als_uv = %d (is_local_avg_update = %d, waiting_cnt = %d)\n",
		temp_uv, data->is_local_avg_update, waiting_cnt);
	return snprintf(buf, PAGE_SIZE, "%d\n", temp_uv);
}

static ssize_t als_raw_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;
	uint32_t temp_clear;
	uint32_t temp_ir;
	uint32_t temp_uv;
	int waiting_cnt = 0;

	while (!data->is_local_avg_update && waiting_cnt < 100) {
		waiting_cnt++;
		msleep_interruptible(10);
	}

	temp_clear = (uint32_t)(((uint64_t)(data->clear_local_average)) * 35LL / 100LL);
	temp_ir = data->ir_local_average;
	temp_uv = (uint32_t)(((uint64_t)(data->uv_local_average)) * 79LL / 100LL);

	ALS_dbg("read als_clear = %d als_ir = %d als_uv = %d (is_local_avg_update = %d, waiting_cnt = %d)\n",\
		temp_clear, temp_ir, temp_uv, data->is_local_avg_update, waiting_cnt);
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", temp_clear, temp_ir, temp_uv);
};

static ssize_t flicker_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *data = &stk_wrapper->alps_data;
	int waiting_cnt = 0;

	while (!data->is_local_avg_update && waiting_cnt < 100) {
		waiting_cnt++;
		msleep_interruptible(10);
	}

	ALS_dbg("read flicker_data = %d (is_local_avg_update = %d, waiting_cnt = %d)\n",
		data->flicker, data->is_local_avg_update, waiting_cnt);
	return snprintf(buf, PAGE_SIZE, "%d\n", data->flicker);
}

#if IS_ENABLED(CONFIG_SENSORS_FLICKER_SELF_TEST)
#define FREQ_SPEC_MARGIN        10
#define FREQ100_SPEC_IN(X)      (((X > (100 - FREQ_SPEC_MARGIN)) && (X < (100 + FREQ_SPEC_MARGIN)))?"PASS":"FAIL")
#define FREQ120_SPEC_IN(X)      (((X > (120 - FREQ_SPEC_MARGIN)) && (X < (120 + FREQ_SPEC_MARGIN)))?"PASS":"FAIL")

#define WIDE_CLEAR_SPEC_MIN     0
#define WIDE_CLEAR_SPEC_MAX     5000000
#define WIDE_SPEC_IN(X)         ((X >= WIDE_CLEAR_SPEC_MIN && X <= WIDE_CLEAR_SPEC_MAX)?"PASS":"FAIL")
#define CLEAR_SPEC_IN(X)        ((X >= WIDE_CLEAR_SPEC_MIN && X <= WIDE_CLEAR_SPEC_MAX)?"PASS":"FAIL")
#define UV_SPEC_IN(X)           ((X >= WIDE_CLEAR_SPEC_MIN && X <= WIDE_CLEAR_SPEC_MAX)?"PASS":"FAIL")
#define ICRATIO_SPEC_IN(X)      "PASS"

static struct result_data *eol_result = NULL;
static ssize_t stk_eol_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;

	if (alps_data->eol_enabled) {
		snprintf(buf, MAX_TEST_RESULT, "EOL_RUNNING");
	} else if (eol_result == NULL) {
		snprintf(buf, MAX_TEST_RESULT, "NO_EOL_TEST");
	} else {
		snprintf(buf, MAX_TEST_RESULT, "%d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s\n",
				eol_result->flicker[EOL_STATE_100], FREQ100_SPEC_IN(eol_result->flicker[EOL_STATE_100]),
				eol_result->flicker[EOL_STATE_120], FREQ120_SPEC_IN(eol_result->flicker[EOL_STATE_120]),
				eol_result->wideband[EOL_STATE_100], WIDE_SPEC_IN(eol_result->wideband[EOL_STATE_100]),
				eol_result->wideband[EOL_STATE_120], WIDE_SPEC_IN(eol_result->wideband[EOL_STATE_120]),
				eol_result->clear[EOL_STATE_100], CLEAR_SPEC_IN(eol_result->clear[EOL_STATE_100]),
				eol_result->clear[EOL_STATE_120], CLEAR_SPEC_IN(eol_result->clear[EOL_STATE_120]),
				eol_result->ratio[EOL_STATE_100], ICRATIO_SPEC_IN(eol_result->ratio[EOL_STATE_100]),
				eol_result->ratio[EOL_STATE_120], ICRATIO_SPEC_IN(eol_result->ratio[EOL_STATE_120]),
				eol_result->uv[EOL_STATE_100], UV_SPEC_IN(eol_result->uv[EOL_STATE_100]),
				eol_result->uv[EOL_STATE_120], UV_SPEC_IN(eol_result->uv[EOL_STATE_120]));

		eol_result = NULL;
	}
	ALS_info("%s\n", buf);

	return MAX_TEST_RESULT;
}

static ssize_t stk_eol_test_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;

	ALS_dbg("eol_test start\n");

#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	bool flicker_flag = alps_data->flicker_flag;

	if (probe_error)
		return 0;

	if (!flicker_flag) {
		alps_data->flicker_flag = true;

		if (alps_data->als_flag) {
			stk6d2x_enable_als(alps_data, false);
			stk6d2x_enable_fifo(alps_data, false);
		}
	}
#endif
	//Start
	alps_data->recover_state = alps_data->als_info.enable;

	ALS_dbg("eol_test set config(%d)...\n", alps_data->recover_state);

	if (!alps_data->recover_state)
		stk6d2x_alps_set_config(alps_data, 1);

	alps_data->eol_enabled = true;

	als_eol_set_env(true, 80);
	eol_result = als_eol_mode();

	alps_data->eol_enabled = false;

	//Stop
	stk6d2x_alps_set_config(alps_data, alps_data->recover_state);
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	if (!flicker_flag) {
		alps_data->flicker_flag = false;

		if (alps_data->als_flag) {
			stk6d2x_alps_set_config(alps_data, true);
			stk_als_init(alps_data);
		}
	}
#endif
	return size;
}

#endif
/****************************************************************************************************
* ALS ATTR List
****************************************************************************************************/
static DEVICE_ATTR(enable,			0664,	stk_als_enable_show,		stk_als_enable_store);
static DEVICE_ATTR(lux,				0664,	stk_als_lux_show,			NULL);
static DEVICE_ATTR(code,			0444,	stk_als_code_show,			NULL);
static DEVICE_ATTR(transmittance,	0664,	stk_als_transmittance_show,	stk_als_transmittance_store);
static DEVICE_ATTR(updateregistry,	0644,	stk6d2x_als_registry_show,	stk6d2x_registry_store);
static DEVICE_ATTR(recv,			0664,	stk_recv_show,				stk_recv_store);
static DEVICE_ATTR(send,			0664,	stk_send_show,				stk_send_store);
static DEVICE_ATTR(poll_delay,		0664,	stk_nothing_show,			stk_nothing_store);
#ifdef STK_ALS_FIR
	static DEVICE_ATTR(firlen,		0644,	stk_als_firlen_show,		stk_als_firlen_store);
#endif
#ifdef STK_ALS_CALI
	static DEVICE_ATTR(cali,		0644,	stk6d2x_als_cali_show,		stk6d2x_cali_store);
#endif

static DEVICE_ATTR(name,			0444,	stk_name_show,				NULL);
static DEVICE_ATTR(als_flush,		0664,	stk_nothing_show,			stk_flush_store);
static DEVICE_ATTR(als_factory_cmd,	0444,	stk_factory_cmd_show,		NULL);
static DEVICE_ATTR(als_ir,			0444,	als_ir_show,				NULL);
static DEVICE_ATTR(als_clear,		0444,	als_clear_show,				NULL);
static DEVICE_ATTR(als_wideband,	0444,	als_wideband_show,			NULL);
static DEVICE_ATTR(als_uv,			0444,	als_uv_show,				NULL);
static DEVICE_ATTR(als_raw_data,	0444,	als_raw_data_show,			NULL);
static DEVICE_ATTR(flicker_data,	0444,	flicker_data_show,			NULL);
#if IS_ENABLED(CONFIG_SENSORS_FLICKER_SELF_TEST)
static DEVICE_ATTR(eol_mode,		0664,	stk_eol_test_show,			stk_eol_test_store);
#endif
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
static DEVICE_ATTR(als_enable,		0664,	stk_rear_als_enable_show,	stk_rear_als_enable_store);
static DEVICE_ATTR(als_data,		0444,	stk_rear_als_data_show,		NULL);
#endif

static DEVICE_ATTR(factory_enable,          0664,	stk_als_enable_show,		stk_als_enable_store);
static DEVICE_ATTR(factory_poll_delay,      0664,	stk_nothing_show,			stk_nothing_store);

static struct attribute *stk_als_attrs [] =
{
	&dev_attr_enable.attr,
	&dev_attr_lux.attr,
	&dev_attr_code.attr,
	&dev_attr_transmittance.attr,
	&dev_attr_updateregistry.attr,
	&dev_attr_recv.attr,
	&dev_attr_send.attr,
#ifdef STK_ALS_FIR
	&dev_attr_firlen.attr,
#endif
#ifdef STK_ALS_CALI
	&dev_attr_cali.attr,
#endif
	&dev_attr_poll_delay.attr,
	NULL
};
static struct attribute_group stk_als_attribute_group =
{
	//.name = "driver",
	.attrs = stk_als_attrs,
};

static struct device_attribute *stk_sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_als_flush,
	&dev_attr_als_factory_cmd,
	&dev_attr_als_ir,
	&dev_attr_als_clear,
	&dev_attr_als_wideband,
	&dev_attr_als_uv,
	&dev_attr_als_raw_data,
	&dev_attr_flicker_data,
#if IS_ENABLED(CONFIG_SENSORS_FLICKER_SELF_TEST)
	&dev_attr_eol_mode,
#endif
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	&dev_attr_als_enable,
	&dev_attr_als_data,
#endif
	&dev_attr_factory_enable,
	&dev_attr_factory_poll_delay,
	NULL
};

#ifdef SUPPORT_SENSOR_CLASS
/****************************************************************************************************
* Sensor class API
****************************************************************************************************/
static int stk6d2x_cdev_enable_als(struct sensors_classdev *sensors_cdev,
								   unsigned int enable)
{
	struct stk6d2x_wrapper *alps_wrapper = container_of(sensors_cdev,
										   stk6d2x_wrapper, als_cdev);
	stk6d2x_alps_set_config(&alps_wrapper->alps_data, enable);
	return 0;
}

static int stk6d2x_cdev_set_als_calibration(struct sensors_classdev *sensors_cdev, int axis, int apply_now)
{
	struct stk6d2x_wrapper *alps_wrapper = container_of(sensors_cdev,
										   stk6d2x_wrapper, als_cdev);
	stk6d2x_cali_als(&alps_wrapper->alps_data);
	return 0;
}

static struct sensors_classdev als_cdev =
{
	.name = "stk6d2x-light",
	.vendor = "sensortek",
	.version = 1,
	.handle = SENSORS_LIGHT_HANDLE,
	.type = SENSOR_TYPE_LIGHT,
	.max_range = "65536",
	.resolution = "1.0",
	.sensor_power = "0.25",
	.min_delay = 50000,
	.max_delay = 2000,
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.flags = 2,
	.enabled = 0,
	.delay_msec = 50,
	.sensors_enable = NULL,
	.sensors_calibrate = NULL,
};

#endif


void stk6d2x_pin_control(struct stk6d2x_data *alps_data, bool pin_set)
{
	int status = 0;

	if (!alps_data->als_pinctrl) {
		ALS_info("als_pinctrl is null\n");
		return;
	}

	if (pin_set) {
		if (!IS_ERR_OR_NULL(alps_data->pins_active)) {
			status = pinctrl_select_state(alps_data->als_pinctrl, alps_data->pins_active);
			if (status)
				ALS_err("can't set pin active state\n");
			else
				ALS_info("set active state\n");
		}
	} else {
		if (!IS_ERR_OR_NULL(alps_data->pins_sleep)) {
			status = pinctrl_select_state(alps_data->als_pinctrl, alps_data->pins_sleep);
			if (status)
				ALS_err("can't set pin sleep state\n");
			else
				ALS_info("set sleep state\n");
		}
	}
}

int stk6d2x_suspend(struct device *dev)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;

	if(probe_error)
		return 0;

	mutex_lock(&alps_data->enable_lock);
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	if (alps_data->als_flag)
		stk_als_stop(alps_data);
#endif

	if (alps_data->als_info.enable) {
		ALS_dbg("Disable ALS\n");
		stk6d2x_alps_set_config(alps_data, 0);
	}
	mutex_unlock(&alps_data->enable_lock);

	stk6d2x_pin_control(alps_data, false);
	stk_power_ctrl(alps_data, false);

	return 0;
}

int stk6d2x_resume(struct device *dev)
{
	stk6d2x_wrapper *stk_wrapper = dev_get_drvdata(dev);
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;

	if(probe_error)
		return 0;

	stk_power_ctrl(alps_data, true);

	mutex_lock(&alps_data->enable_lock);
	if (alps_data->als_info.enable) {
		ALS_dbg("Enable ALS\n");
		msleep_interruptible(40);
		stk6d2x_alps_set_config(alps_data, 1);
	}
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	else if (alps_data->als_flag) {
		msleep_interruptible(40);
		stk_als_start(alps_data);
	}
#endif
	mutex_unlock(&alps_data->enable_lock);

	stk6d2x_pin_control(alps_data, true);

	return 0;
}

static int stk6d2x_parse_dt(struct stk6d2x_wrapper *stk_wrapper,
							struct stk6d2x_platform_data *pdata)
{
	int rc;
	struct device *dev = stk_wrapper->dev;
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	struct device_node *np = dev->of_node;
	struct device_node *vbus_of_node = NULL;
	struct device_node *vdd_of_node = NULL;
	u32 temp_val;

	ALS_info("parse dt\n");

	if (of_get_property(np, "als_rear,use_ext_clk", NULL)) {
		alps_data->use_ext_clk = TRUE;
	}

	if (alps_data->use_ext_clk) {
#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
		alps_data->ext_clk_gpio = of_get_named_gpio(np, "stk,ext-clk-gpio", 0);
#else
		alps_data->ext_clk_gpio = of_get_named_gpio_flags(np, "stk,ext-clk-gpio",
					0, &pdata->int_flags);
#endif

		if (alps_data->ext_clk_gpio < 0)
			ALS_err("Unable to read ext_clk_gpio\n");
		else
			ALS_info("alps_data->ext_clk_gpio = %d\n", alps_data->ext_clk_gpio);
	}

	vbus_of_node = of_parse_phandle(np, "vbus_1p8-supply", 0);
	if (vbus_of_node) {
		alps_data->regulator_vbus_1p8 = regulator_get(dev, "vbus_1p8");
		if (IS_ERR(alps_data->regulator_vbus_1p8) || alps_data->regulator_vbus_1p8 == NULL) {
			ALS_err("get vbus_1p8 regulator failed\n");
			alps_data->regulator_vbus_1p8 = NULL;
		} else {
			ALS_dbg("get vbus_1p8 regulator = %p done \n", alps_data->regulator_vbus_1p8);
		}
	} else {
		ALS_err("get vbus_1p8 regulator failed\n");
		alps_data->regulator_vbus_1p8 = NULL;
	}

	vdd_of_node = of_parse_phandle(np, "vdd_1p8-supply", 0);
	if (vdd_of_node) {
		alps_data->regulator_vdd_1p8 = regulator_get(dev, "vdd_1p8");
		if (IS_ERR(alps_data->regulator_vdd_1p8) || alps_data->regulator_vdd_1p8 == NULL) {
			ALS_err("get vdd_1p8 regulator failed\n");
			alps_data->regulator_vdd_1p8 = NULL;
			return -ENODEV;
		} else {
			ALS_dbg("get vdd_1p8 regulator = %p done \n", alps_data->regulator_vdd_1p8);
		}
	} else {
		ALS_err("get vdd_1p8 regulator failed\n");
		alps_data->regulator_vdd_1p8 = NULL;
	}

	alps_data->als_pinctrl = devm_pinctrl_get(dev);

	if (IS_ERR_OR_NULL(alps_data->als_pinctrl)) {
		ALS_err("get pinctrl(%li) error\n", PTR_ERR(alps_data->als_pinctrl));
		alps_data->als_pinctrl = NULL;
	} else {
		alps_data->pins_sleep = pinctrl_lookup_state(alps_data->als_pinctrl, "sleep");
		if (IS_ERR_OR_NULL(alps_data->pins_sleep)) {
			ALS_err("get pins_sleep(%li) error\n", PTR_ERR(alps_data->pins_sleep));
			alps_data->pins_sleep = NULL;
		}

		alps_data->pins_active = pinctrl_lookup_state(alps_data->als_pinctrl, "active");
		if (IS_ERR_OR_NULL(alps_data->pins_active)) {
			ALS_err("get pins_active(%li) error\n", PTR_ERR(alps_data->pins_active));
			alps_data->pins_active = NULL;
		}
	}

	rc = of_property_read_u32(np, "stk,als_scale", &temp_val);

	if (!rc) {
		pdata->als_scale = temp_val;
		ALS_info("stk-als_scale = %d\n", temp_val);
	} else {
		ALS_err("Unable to read als_scale\n");
	}

	return 0;
}

static int stk6d2x_set_input_devices(struct stk6d2x_wrapper *stk_wrapper)
{
	int err;
	/****** Create ALS ATTR ******/
	stk6d2x_data *alps_data = &stk_wrapper->alps_data;
	stk_wrapper->als_input_dev = input_allocate_device();

	ALS_dbg("input_devices start\n");

	if (!stk_wrapper->als_input_dev) {
		ALS_err("could not allocate als device\n");
		err = -ENOMEM;
		return err;
	}

	stk_wrapper->als_input_dev->name = MODULE_NAME_ALS;
	stk_wrapper->als_input_dev->id.bustype = BUS_I2C;
	input_set_drvdata(stk_wrapper->als_input_dev, alps_data);
	//set_bit(EV_ABS, alps_data->als_input_dev->evbit);
	//input_set_abs_params(alps_data->als_input_dev, ABS_MISC, 0, ((1 << 16) - 1), 0, 0);
	input_set_capability(stk_wrapper->als_input_dev, EV_REL, REL_X);	/* ir */
	input_set_capability(stk_wrapper->als_input_dev, EV_REL, REL_RX);	/* UV */
	input_set_capability(stk_wrapper->als_input_dev, EV_REL, REL_RY);	/* clear */
	input_set_capability(stk_wrapper->als_input_dev, EV_REL, REL_RZ);	/* flicker */
	input_set_capability(stk_wrapper->als_input_dev, EV_REL, REL_MISC);	/* flush */
	input_set_capability(stk_wrapper->als_input_dev, EV_ABS, ABS_Y);	/* gain_clear */
	input_set_capability(stk_wrapper->als_input_dev, EV_ABS, ABS_Z);	/* gain_ir */
	input_set_capability(stk_wrapper->als_input_dev, EV_ABS, ABS_RX);	/* gain_uv */
	input_set_capability(stk_wrapper->als_input_dev, EV_ABS, ABS_RY);	/* flicker2 */

	err = input_register_device(stk_wrapper->als_input_dev);

	if (err) {
		ALS_err("can not register als input device\n");
		goto err_als_input_register;
	}

	ALS_dbg("sensors_create_symlink start\n");

#if IS_ENABLED(CONFIG_ARCH_EXYNOS)
	err = sensors_create_symlink(stk_wrapper->als_input_dev);
#else
	err = sensors_create_symlink(&stk_wrapper->als_input_dev->dev.kobj, stk_wrapper->als_input_dev->name);
#endif
	if (err < 0) {
		ALS_err("%s - could not create_symlink\n", __func__);
		goto err_sensors_create_symlink;
	}

	ALS_dbg("sysfs_create_group start\n");

	err = sysfs_create_group(&stk_wrapper->als_input_dev->dev.kobj, &stk_als_attribute_group);
	if (err < 0) {
		ALS_err("could not create sysfs group for als\n");
		goto err_sysfs_create_group;
	}

	input_set_drvdata(stk_wrapper->als_input_dev, stk_wrapper);
#ifdef SUPPORT_SENSOR_CLASS
	stk_wrapper->als_cdev = als_cdev;
	stk_wrapper->als_cdev.sensors_enable = stk6d2x_cdev_enable_als;
	stk_wrapper->als_cdev.sensors_calibrate = stk6d2x_cdev_set_als_calibration;
	err = sensors_classdev_register(&stk_wrapper->als_input_dev->dev, &stk_wrapper->als_cdev);

	if (err) {
		ALS_err("ALS sensors class register failed.\n");
		goto err_register_als_cdev;
	}

#endif

	ALS_dbg("sensors_register start\n");

	err = sensors_register(&stk_wrapper->sensor_dev, stk_wrapper, stk_sensor_attrs, MODULE_NAME_ALS);
	if (err) {
		ALS_err("%s - cound not register als_sensor(%d).\n", __func__, err);
		goto als_sensor_register_failed;
	}

	ALS_info("done\n");
	return 0;

als_sensor_register_failed:
#ifdef SUPPORT_SENSOR_CLASS
	sensors_classdev_unregister(&stk_wrapper->als_cdev);
err_register_als_cdev:
#endif
	ALS_err("sysfs_remove_group start\n");
	sysfs_remove_group(&stk_wrapper->als_input_dev->dev.kobj, &stk_als_attribute_group);
err_sysfs_create_group:
	ALS_err("sensors_remove_symlink start\n");
#if IS_ENABLED(CONFIG_ARCH_EXYNOS)
	sensors_remove_symlink(stk_wrapper->als_input_dev);
#else
	sensors_remove_symlink(&stk_wrapper->als_input_dev->dev.kobj, stk_wrapper->als_input_dev->name);
#endif
err_sensors_create_symlink:
	ALS_err("input_unregister_device start\n");
	input_unregister_device(stk_wrapper->als_input_dev);
err_als_input_register:
	return err;
}

int stk6d2x_probe(struct i2c_client *client,
				  struct common_function *common_fn)
{
	int err = -ENODEV;
	stk6d2x_wrapper *stk_wrapper;
	stk6d2x_data *alps_data;
	struct stk6d2x_platform_data *plat_data;

	ALS_dbg("stk6d2x_version : %d.%d.%d\n", STK6D2X_MAJOR,
		   STK6D2X_MINOR,
		   STK6D2X_REV);

	if (!common_fn) {
		dev_err(&client->dev, "%s: Operation not Supported\n", __func__);
		return -EPERM;
	}

	err = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);

	if (!err) {
		dev_err(&client->dev, "%s: SMBUS Byte Data not Supported\n", __func__);
		return -EIO;
	}

	stk_wrapper = kzalloc(sizeof(stk6d2x_wrapper), GFP_KERNEL);

	if (!stk_wrapper) {
		ALS_err("failed to allocate stk6d2x_wrapper\n");
		return -ENOMEM;
	}

	alps_data = &stk_wrapper->alps_data;

	if (!alps_data) {
		ALS_err("failed to allocate stk6d2x_data\n");
		return -ENOMEM;
	}

	stk_wrapper->i2c_mgr.client = client;
	stk_wrapper->i2c_mgr.addr_type = ADDR_8BIT;
	stk_wrapper->dev  = &client->dev;
	alps_data->bops   = common_fn->bops;
	alps_data->tops   = common_fn->tops;
	alps_data->gops   = common_fn->gops;
	i2c_set_clientdata(client, stk_wrapper);
	mutex_init(&stk_wrapper->i2c_mgr.lock);
	mutex_init(&alps_data->config_lock);
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	mutex_init(&alps_data->enable_lock);
#endif
	alps_data->bus_idx = alps_data->bops->init(&stk_wrapper->i2c_mgr);

	if (alps_data->bus_idx < 0) {
		goto err_free_mem;
	}

	// Parsing device tree
	if (client->dev.of_node) {
		ALS_dbg("probe with device tree\n");
		plat_data = devm_kzalloc(&client->dev,
								 sizeof(struct stk6d2x_platform_data), GFP_KERNEL);

		if (!plat_data) {
			dev_err(&client->dev, "%s: Failed to allocate memory\n", __func__);
			return -ENOMEM;
		}

		err = stk6d2x_parse_dt(stk_wrapper, plat_data);

		if (err) {
			dev_err(&client->dev,
					"%s: stk6d2x_parse_dt ret=%d\n", __func__, err);
			goto err_parse_dt;
		}
	} else {
		ALS_err("probe with platform data\n");
		plat_data = client->dev.platform_data;
	}

	if (!plat_data) {
		dev_err(&client->dev,
				"%s: no stk6d2x platform data!\n", __func__);
		goto err_als_input_allocate;
	}

	if (plat_data->als_scale == 0) {
		dev_err(&client->dev,
				"%s: Please set als_scale = %d\n", __func__, plat_data->als_scale);
		goto err_als_input_allocate;
	}

	stk_power_ctrl(alps_data, true);
	msleep_interruptible(40);

	stk6d2x_pin_control(alps_data, true);

	// Register device
	err = stk6d2x_set_input_devices(stk_wrapper);

	if (err < 0)
		goto err_setup_input_device;

	alps_data->pdata  = plat_data;

	err = stk6d2x_init_all_setting(alps_data);
	dev_err(&client->dev,
			"%s: err = %d\n", __func__, err);

	if (err < 0)
		goto err_setup_init_reg;

	ALS_dbg("probe successfully\n");

	stk6d2x_pin_control(alps_data, false);
	probe_error = 0;
	return 0;

err_setup_init_reg:
	ALS_err("err_setup_init_reg start\n");
	stk6d2x_pin_control(alps_data, false);
	sensors_unregister(stk_wrapper->sensor_dev, stk_sensor_attrs);
#ifdef SUPPORT_SENSOR_CLASS
	sensors_classdev_unregister(&stk_wrapper->als_cdev);
#endif
	sysfs_remove_group(&stk_wrapper->als_input_dev->dev.kobj, &stk_als_attribute_group);
#if IS_ENABLED(CONFIG_ARCH_EXYNOS)
	sensors_remove_symlink(stk_wrapper->als_input_dev);
#else
	sensors_remove_symlink(&stk_wrapper->als_input_dev->dev.kobj, stk_wrapper->als_input_dev->name);
#endif
	input_unregister_device(stk_wrapper->als_input_dev);
err_setup_input_device:
	stk_power_ctrl(alps_data, false);
err_als_input_allocate:
err_parse_dt:

	if (alps_data->als_pinctrl) {
		devm_pinctrl_put(alps_data->als_pinctrl);
		alps_data->als_pinctrl = NULL;
	}
	if (alps_data->pins_active)
		alps_data->pins_active = NULL;
	if (alps_data->pins_sleep)
		alps_data->pins_sleep = NULL;

	if (alps_data->pclk)
		alps_data->pclk = NULL;

err_free_mem:
#ifdef STK_FIFO_ENABLE
	STK6D2X_TIMER_REMOVE(alps_data, &alps_data->fifo_release_timer_info);
#endif
	alps_data->bops->remove(&stk_wrapper->i2c_mgr);
	mutex_destroy(&stk_wrapper->i2c_mgr.lock);
	mutex_destroy(&alps_data->config_lock);
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	mutex_destroy(&alps_data->enable_lock);
#endif
	kfree(stk_wrapper);
	probe_error = err;
	return err;
}

int stk6d2x_remove(struct i2c_client *client)
{
	stk6d2x_wrapper *stk_wrapper = i2c_get_clientdata(client);
	struct stk6d2x_data *alps_data = &stk_wrapper->alps_data;

	ALS_dbg("stk6d2x_remove start\n");
	stk_power_ctrl(alps_data, false);
	if (alps_data->regulator_vbus_1p8) {
		if (alps_data->vbus_1p8_enable) {
			regulator_disable(alps_data->regulator_vbus_1p8);
		}
		alps_data->regulator_vbus_1p8 = NULL;
	}

	if (alps_data->regulator_vdd_1p8) {
		if (alps_data->vdd_1p8_enable) {
			regulator_disable(alps_data->regulator_vdd_1p8);
		}
		ALS_dbg("put vdd_1p8 regulator = %p done (en = %d)\n",
			alps_data->regulator_vdd_1p8, alps_data->vdd_1p8_enable);
		regulator_put(alps_data->regulator_vdd_1p8);
		alps_data->regulator_vdd_1p8 = NULL;
	}

	if (alps_data->reg) {
		if (alps_data->reg_enable) {
			regulator_disable(alps_data->reg);
		}
		ALS_dbg("put gdscr regulator = %p done (en = %d)\n",
			alps_data->reg, alps_data->reg_enable);
		alps_data->reg = NULL;
	}

	if (alps_data->pclk)
		alps_data->pclk = NULL;

	if (alps_data->als_pinctrl) {
		devm_pinctrl_put(alps_data->als_pinctrl);
		alps_data->als_pinctrl = NULL;
	}
	if (alps_data->pins_active)
		alps_data->pins_active = NULL;
	if (alps_data->pins_sleep)
		alps_data->pins_sleep = NULL;

	device_init_wakeup(&client->dev, false);
	STK6D2X_GPIO_IRQ_REMOVE(alps_data, &alps_data->gpio_info);
	STK6D2X_TIMER_REMOVE(alps_data, &alps_data->alps_timer_info);
	sensors_unregister(stk_wrapper->sensor_dev, stk_sensor_attrs);
	sysfs_remove_group(&stk_wrapper->als_input_dev->dev.kobj, &stk_als_attribute_group);
#if IS_ENABLED(CONFIG_ARCH_EXYNOS)
	sensors_remove_symlink(stk_wrapper->als_input_dev);
#else
	sensors_remove_symlink(&stk_wrapper->als_input_dev->dev.kobj, stk_wrapper->als_input_dev->name);
#endif
	input_unregister_device(stk_wrapper->als_input_dev);
#ifdef STK_FIFO_ENABLE
	STK6D2X_TIMER_REMOVE(alps_data, &alps_data->fifo_release_timer_info);
#endif
	alps_data->bops->remove(&stk_wrapper->i2c_mgr);
	mutex_destroy(&stk_wrapper->i2c_mgr.lock);
	mutex_destroy(&alps_data->config_lock);
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	mutex_destroy(&alps_data->enable_lock);
#endif
	kfree(stk_wrapper);
	return 0;
}

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
static int stk6d2x_i2c_probe(struct i2c_client *client)
{
	struct common_function common_fn =
	{
		.bops = &stk_i2c_bops,
		.tops = &stk_t_ops,
		.gops = &stk_g_ops,
	};
	return stk6d2x_probe(client, &common_fn);
}
#else
static int stk6d2x_i2c_probe(struct i2c_client *client,
							 const struct i2c_device_id *id)
{
	struct common_function common_fn =
	{
		.bops = &stk_i2c_bops,
		.tops = &stk_t_ops,
		.gops = &stk_g_ops,
	};
	return stk6d2x_probe(client, &common_fn);
}
#endif

static int stk6d2x_i2c_remove(struct i2c_client *client)
{
	return stk6d2x_remove(client);
}

int stk6d2x_i2c_suspend(struct device *dev)
{
	stk6d2x_suspend(dev);
	return 0;
}

int stk6d2x_i2c_resume(struct device *dev)
{
	stk6d2x_resume(dev);
	return 0;
}

static const struct dev_pm_ops stk6d2x_pm_ops =
{
	SET_SYSTEM_SLEEP_PM_OPS(stk6d2x_i2c_suspend, stk6d2x_i2c_resume)
};

static const struct i2c_device_id stk_als_id[] =
{
	{ "stk_als", 0},
	{}
};

static struct of_device_id stk_match_table[] =
{
	{ .compatible = "stk,stk6d2x", },
	{ },
};

static struct i2c_driver stk_als_driver =
{
	.driver = {
		.name = STK6D2X_DEV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = stk_match_table,
#endif
		.pm = &stk6d2x_pm_ops,
	},
	.probe      = stk6d2x_i2c_probe,
	.remove     = stk6d2x_i2c_remove,
	.id_table   = stk_als_id,
};

static int __init stk6d2x_init(void)
{
	int ret = 0;

	if (flicker_param_lpcharge == 1)
		return ret;

	ALS_dbg("start\n");
	ret = i2c_add_driver(&stk_als_driver);
	ALS_dbg("Add driver ret = %d\n", ret);

	/**
	 * i2c_add_driver doesn't return the return value of stk6d2x_probe.
	 * it doesn't stop with a single probe error to keep trying to probe remaining i2c slave devices.
	 *
	 * so, check probe_error and call i2c_del_driver to remove i2c device explicitly.
	 * without i2c_del_driver, the remaining pm operation cause kernel panic
	 * __init return should be okay even if probe failure.
	 *  @ref __driver_attach
	 */

	if (probe_error)
		i2c_del_driver(&stk_als_driver);

	return ret;
}
static void __exit stk6d2x_exit(void)
{
	i2c_del_driver(&stk_als_driver);
}

module_init(stk6d2x_init);
module_exit(stk6d2x_exit);
MODULE_DEVICE_TABLE(i2c, stk_als_id);
MODULE_SOFTDEP("pre: sensors_core");
MODULE_AUTHOR("Taka Chiu <taka_chiu@sensortek.com.tw>");
MODULE_DESCRIPTION("Sensortek stk6d2x ambient Light Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
