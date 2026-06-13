// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/vibrator/sec_vibrator_inputff.h>
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
#include <linux/usb_notify.h>
#endif
#if defined(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
#else
#define __visible_for_testing static
#endif
#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif
#if IS_ENABLED(CONFIG_SEC_PARAM)
#include <linux/sec_param.h>
#endif

/* Use __VIB_TEST_STORE_LE_PARAM only for testing purpose,
 * else there will be GKI violation.
 * #define __VIB_TEST_STORE_LE_PARAM
 */

#if defined(CONFIG_VIB_STORE_LE_PARAM)
static uint32_t vib_le_est;
module_param(vib_le_est, uint, 0444);
MODULE_PARM_DESC(vib_le_est, "sec_vib_inputff_le_est value");
#endif

__visible_for_testing char *sec_vib_inputff_get_i2c_test(
	struct sec_vib_inputff_drvdata *ddata)
{
	if (ddata->vib_ops->get_i2c_test) {
		if (ddata->vib_ops->get_i2c_test(ddata->input))
			return "PASS";
		else
			return "FAIL";
	} else
		return "NONE";
}

static ssize_t i2c_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", sec_vib_inputff_get_i2c_test(ddata));
}
static DEVICE_ATTR_RO(i2c_test);

__visible_for_testing int sec_vib_inputff_get_i2s_test(
	struct sec_vib_inputff_drvdata *ddata)
{
	int ret = 0;

	if (ddata->vib_ops->get_i2s_test)
		ret = ddata->vib_ops->get_i2s_test(ddata->input);
	dev_info(ddata->dev, "%s ret : %d\n", __func__, ret);
	return ret;
}

static ssize_t i2s_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", sec_vib_inputff_get_i2s_test(ddata));
}
static DEVICE_ATTR_RO(i2s_test);

__visible_for_testing int sec_vib_inputff_load_firmware(
	struct sec_vib_inputff_drvdata *ddata, int fw_id)
{
	int ret = 0;
	int retry = 0;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif
	if (!ddata->vib_ops || !ddata->vib_ops->fw_load) {
		dev_err(ddata->dev, "%s Fail to find pointer\n", __func__);
		ret = -ENOENT;
		goto err;
	}

	dev_info(ddata->dev, "%s ret(%d), fw_id(%d) retry(%d) stat(%d)\n",
		__func__, ret, ddata->fw.id, ddata->fw.retry, ddata->fw.stat);
	mutex_lock(&ddata->fw.stat_lock);

	retry = ddata->fw.retry;
	__pm_stay_awake(&ddata->fw.ws);
	ret = ddata->vib_ops->fw_load(ddata->input, ddata->fw.id);
	__pm_relax(&ddata->fw.ws);
	if (!ret) {
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		event = NOTIFY_EXTRA_VIB_FW_LOAD_SUCCESS;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
		ddata->fw.ret[retry] = FW_LOAD_SUCCESS;
		ddata->fw.stat = FW_LOAD_SUCCESS;
		if (!ddata->fw_init_attempted)
			ddata->fw_init_attempted = true;
		if (ddata->f0_stored && ddata->fw.id == 0)
			ddata->vib_ops->set_f0_stored(ddata->input, ddata->f0_stored);
	} else {
		if (retry < FWLOAD_TRY - 1) {
			ddata->fw.ret[retry] = ret;
			queue_delayed_work(ddata->fw.fw_workqueue,
				&ddata->fw.retry_wk, msecs_to_jiffies(1000));
			ddata->fw.retry++;
		} else {
			dev_err(ddata->dev, "%s firmware load retry fail\n", __func__);
			if (!ddata->fw_init_attempted) {
				ddata->fw_init_attempted = true;
				if (ddata->vib_ops->get_ap_chipset &&
						(strcmp(ddata->vib_ops->get_ap_chipset(ddata->input), "slsi") == 0))
					goto err;
			}
#if IS_ENABLED(CONFIG_SEC_ABC)
			sec_abc_send_event("MODULE=vib@WARN=fw_load_fail");
#endif
		}
	}
err:
	mutex_unlock(&ddata->fw.stat_lock);
	dev_info(ddata->dev, "%s done\n", __func__);
	return ret;
}

static void sec_vib_inputff_load_firmware_work(struct work_struct *work)
{
	struct sec_vib_inputff_fwdata *fdata
		= container_of(work, struct sec_vib_inputff_fwdata, wk);
	struct sec_vib_inputff_drvdata *ddata
		= container_of(fdata, struct sec_vib_inputff_drvdata, fw);
	int ret = 0;

	dev_info(ddata->dev, "%s\n", __func__);
	ret = sec_vib_inputff_load_firmware(ddata, fdata->id);
	if (ret < 0)
		dev_err(ddata->dev, "%s fail(%d)\n", __func__, ret);
}

static void sec_vib_inputff_load_firmware_retry(struct work_struct *work)
{
	struct sec_vib_inputff_fwdata *fdata
		= container_of(to_delayed_work(work), struct sec_vib_inputff_fwdata, retry_wk);
	struct sec_vib_inputff_drvdata *ddata
		= container_of(fdata, struct sec_vib_inputff_drvdata, fw);
	int ret = 0;

	dev_info(ddata->dev, "%s\n", __func__);
	ret = sec_vib_inputff_load_firmware(ddata, fdata->id);
	if (ret < 0)
		dev_err(ddata->dev, "%s fail(%d)\n", __func__, ret);
}

static void firmware_load_store_work(struct work_struct *work)
{
	struct sec_vib_inputff_fwdata *fdata
		= container_of(to_delayed_work(work), struct sec_vib_inputff_fwdata, store_wk);
	struct sec_vib_inputff_drvdata *ddata
		= container_of(fdata, struct sec_vib_inputff_drvdata, fw);
	unsigned int i;

	dev_info(ddata->dev, "%s stat(%d) retry(%d)\n", __func__,
			ddata->fw.stat, ddata->fw.retry);

	cancel_work_sync(&ddata->fw.wk);
	cancel_delayed_work_sync(&ddata->fw.retry_wk);
	dev_info(ddata->dev, "%s clear retry\n", __func__);
	ddata->fw.retry = 0;
	for (i = 0; i < FWLOAD_TRY; i++)
		ddata->fw.ret[i] = FW_LOAD_STORE;
	ddata->fw.stat = FW_LOAD_STORE;

	queue_work(ddata->fw.fw_workqueue, &ddata->fw.wk);
}

static void sec_vib_inputff_start_cal_work(struct work_struct *work)
{
	struct sec_vib_inputff_drvdata *ddata
		= container_of(work, struct sec_vib_inputff_drvdata, cal_work);
	int ret;

	dev_info(ddata->dev, "%s start\n", __func__);

	ret = ddata->vib_ops->set_trigger_cal(ddata->input, ddata->trigger_calibration);
	if (ret) {
		dev_err(ddata->dev, "%s set_trigger_cal error : %d\n", __func__, ret);
		ddata->trigger_calibration = 0;
	}
}

static ssize_t firmware_load_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	bool fw_load = false;

	if (ddata->fw.stat == FW_LOAD_SUCCESS)
		fw_load = true;

	return sprintf(buf, "%d\n", fw_load);
}

static ssize_t firmware_load_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	u32 fw_id = 0;
	int ret = 0;

	if (!ddata->vib_ops->fw_load) {
		dev_info(ddata->dev, "%s fw_upload not registered.\n", __func__);

		// Mark Success for upload check
		ddata->fw.stat = FW_LOAD_SUCCESS;

		return count;
	}

	ret = kstrtou32(buf, 0, &fw_id);
	if (ret < 0) {
		dev_err(ddata->dev, "%s kstrtou32 error : %d\n", __func__, ret);
		goto err;
	}
	dev_info(ddata->dev, "%s id(%d) stat(%d) retry(%d)\n", __func__,
			fw_id, ddata->fw.stat, ddata->fw.retry);
	ddata->fw.id = fw_id;
	schedule_delayed_work(&ddata->fw.store_wk, msecs_to_jiffies(0));
	dev_info(ddata->dev, "%s done\n", __func__);
err:
	return ret ? ret : count;
}
static DEVICE_ATTR_RW(firmware_load);

int sec_vib_inputff_get_current_temp(struct sec_vib_inputff_drvdata *ddata)
{
	dev_info(ddata->dev, "%s temperature : %d\n", __func__, ddata->temperature);

	return ddata->temperature;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_get_current_temp);

static ssize_t current_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ddata->temperature);
}

static ssize_t current_temp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	int temp, ret = 0;

	ret = kstrtos32(buf, 0, &temp);
	if (ret < 0) {
		dev_err(ddata->dev, "%s kstrtos32 error : %d\n", __func__, ret);
		ddata->temperature = INT_MIN;
		goto err;
	}

	ddata->temperature = temp;
	dev_info(ddata->dev, "%s temperature : %d\n", __func__, ddata->temperature);
err:
	return ret ? ret : count;
}
static DEVICE_ATTR_RW(current_temp);

int sec_vib_inputff_get_ach_percent(struct sec_vib_inputff_drvdata *ddata)
{
	dev_info(ddata->dev, "%s ach_percent : %d\n", __func__, ddata->ach_percent);

	return ddata->ach_percent;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_get_ach_percent);

static ssize_t ach_percent_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ddata->ach_percent);
}

static ssize_t ach_percent_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	int percent, ret = 0;

	ret = kstrtos32(buf, 0, &percent);
	if (ret < 0) {
		dev_err(ddata->dev, "%s kstrtos32 error : %d\n", __func__, ret);
		ddata->ach_percent = 100;
		goto err;
	}

	if (percent >= 0 && percent <= 100)
		ddata->ach_percent = percent;
	else
		ddata->ach_percent = 100;
	dev_info(ddata->dev, "%s ach_percent : %d\n", __func__, ddata->ach_percent);
err:
	return ret ? ret : count;
}
static DEVICE_ATTR_RW(ach_percent);

static ssize_t cal_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	if (ddata->is_f0_tracking)
		return snprintf(buf, PAGE_SIZE, "f0_cal\n");
	else
		return snprintf(buf, PAGE_SIZE, "NONE\n");
}
static DEVICE_ATTR_RO(cal_type);

static ssize_t trigger_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ddata->trigger_calibration);
}

static ssize_t trigger_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	int ret;

	cancel_work_sync(&ddata->cal_work);

	ret = kstrtos32(buf, 16, &ddata->trigger_calibration);
	if (ret < 0) {
		dev_err(ddata->dev, "%s kstrtos32 error : %d\n", __func__, ret);
		return ret;
	}
	if (ddata->vib_ops->set_trigger_cal && ddata->is_f0_tracking) {
		queue_work(ddata->cal_workqueue, &ddata->cal_work);
		dev_info(ddata->dev, "%s trigger_calibration : %u\n", __func__, ddata->trigger_calibration);
	} else {
		dev_err(ddata->dev, "%s set_trigger_cal doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}
	return count;
}
static DEVICE_ATTR_RW(trigger_calibration);

static ssize_t f0_measured_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_f0_measured || !ddata->is_f0_tracking) {
		dev_err(ddata->dev, "%s get_f0_measured doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}

	return snprintf(buf, PAGE_SIZE, "%08X\n", ddata->vib_ops->get_f0_measured(ddata->input));
}
static DEVICE_ATTR_RO(f0_measured);

static ssize_t f0_offset_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	int ret;
	u32 f0_offset;

	ret = kstrtos32(buf, 16, &f0_offset);
	if (ret < 0) {
		dev_err(ddata->dev, "%s kstrtos32 error : %d\n", __func__, ret);
		return ret;
	}
	if (ddata->vib_ops->set_f0_offset && ddata->is_f0_tracking) {
		ret = ddata->vib_ops->set_f0_offset(ddata->input, f0_offset);
		if (ret) {
			dev_err(ddata->dev, "%s set_f0_offset error : %d\n", __func__, ret);
			return -EINVAL;
		}
		dev_info(ddata->dev, "%s f0_offset : %u, ret : %d\n", __func__, f0_offset, ret);
	} else {
		dev_err(ddata->dev, "%s set_f0_offset doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}
	return count;
}

static ssize_t f0_offset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_f0_offset || !ddata->is_f0_tracking) {
		dev_err(ddata->dev, "%s get_f0_offset doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}

	return snprintf(buf, PAGE_SIZE, "%08X\n", ddata->vib_ops->get_f0_offset(ddata->input));
}
static DEVICE_ATTR_RW(f0_offset);

static ssize_t f0_stored_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->set_f0_stored || !ddata->is_f0_tracking) {
		dev_err(ddata->dev, "%s f0_stored read doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}

	return sprintf(buf, "%08X\n", ddata->vib_ops->get_f0_stored ?
		ddata->vib_ops->get_f0_stored(ddata->input) : ddata->f0_stored);
}

/*
 * f0_stored_store - loads the f0 calibrated data to the driver.
 *			Its stored in efs and the f0 calibration data to be loaded
 *			on every boot.
 * sequence: (Pre-requisite)
 *	1) Load Calib firmware.
 *	2) Trigger Calibration
 *	3) Measure f0 value
 *	4) Store the f0 value to efs area.
 *	5) Load Normal firmware
 * Upon every boot, HAL reads from efs and loads to driver via this function.
 * Future Check : If any new IC, will have EEPROM, that may needs to be
 *    preferred over efs area?
 */
static ssize_t f0_stored_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	int ret;
	u32 f0_stored;

	ret = kstrtos32(buf, 16, &f0_stored);
	if (ret < 0) {
		dev_err(ddata->dev, "%s kstrtos32 error : %d\n", __func__, ret);
		return ret;
	}

	if (ddata->vib_ops->set_f0_stored && ddata->is_f0_tracking) {
		if (ddata->fw.stat == FW_LOAD_SUCCESS && ddata->fw.id == 0) {
			ret = ddata->vib_ops->set_f0_stored(ddata->input, f0_stored);
			if (ret) {
				dev_err(ddata->dev, "%s set_f0_stored error : %d\n", __func__, ret);
				return -EINVAL;
			}
		}
		ddata->f0_stored = f0_stored;
		dev_info(ddata->dev, "%s f0_stored : %u, ret : %d\n", __func__, f0_stored, ret);
	} else {
		dev_err(ddata->dev, "%s set_f0_stored doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}
	return count;
}
static DEVICE_ATTR_RW(f0_stored);

static ssize_t le_support_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", ddata->is_le_support);
}
static DEVICE_ATTR_RO(le_support);

static ssize_t le_est_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	unsigned int le = 0;
	int ret = 0;

	if (!ddata->vib_ops->get_le_est) {
		dev_err(ddata->dev, "%s le_est doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}

	ret = ddata->vib_ops->get_le_est(ddata->input, &le);
	if (ret)
		return ret;

	return snprintf(buf, PAGE_SIZE, "%u\n", le);
}
static DEVICE_ATTR_RO(le_est);

static ssize_t le_stored_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_le_stored) {
		dev_err(ddata->dev, "%s le_stored doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}

	return snprintf(buf, PAGE_SIZE, "%u\n", ddata->vib_ops->get_le_stored(ddata->input));
}

/*
 * le_stored_param_store - Stores the lra estimated data to SEC param.
 * sequence:
 *	1) Load Calib firmware.
 *	2) LRA Estimate
 *	3) Call this function to store the LRA.
 *	4) Load Normal firmware (To get the value to be effective).
 * Future Check : If any new IC, will have EEPROM, that may needs to be
 *    preferred over SEC_PARAM?
 */
static ssize_t le_stored_param_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	int ret;
	u32 le_stored;

	ret = kstrtos32(buf, 10, &le_stored);
	if (ret < 0) {
		dev_err(ddata->dev, "%s kstrtos32 error : %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s le_stored: %d\n", __func__, le_stored);

	if (ddata->vib_ops->set_le_stored) {
		if (ddata->fw.stat == FW_LOAD_SUCCESS && ddata->fw.id == 1) {
			ret = ddata->vib_ops->set_le_stored(ddata->input, le_stored);
			if (ret) {
				dev_err(ddata->dev, "%s le_stored error : %d\n", __func__, ret);
				return -EINVAL;
			}
		}
		ddata->le_stored = le_stored;
		dev_info(ddata->dev, "%s le_stored : %u, Updated\n", __func__, le_stored);
#if defined(__VIB_TEST_STORE_LE_PARAM)
		dev_err(ddata->dev, "%s Dont store sec_param from driver. Use only for testing\n", __func__);
#if IS_ENABLED(CONFIG_SEC_PARAM)
#if IS_ENABLED(CONFIG_ARCH_QCOM) && !defined(CONFIG_USB_ARCH_EXYNOS) && !defined(CONFIG_ARCH_EXYNOS)
		ret = sec_set_param(param_vib_le_est, &le_stored);
		if (ret == false) {
			dev_info(ddata->dev, "%s:set_paramset_param failed - %x(%d)\n", __func__, le_stored, ret);

			return -EIO;
		}
		dev_info(ddata->dev, "%s Store in sec param - Done\n", __func__);
#endif
#else
	/* Need to add for SLSI */
	dev_err(ddata->dev, "%s le_stored not stored in sec_param\n", __func__);
#endif
#endif // __VIB_TEST_STORE_LE_PARAM
	} else {
		dev_err(ddata->dev, "%s le_stored doesn't support\n", __func__);
		return -EOPNOTSUPP;
	}
	return count;
}
static DEVICE_ATTR_RW(le_stored_param);

static ssize_t use_sep_index_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);
	int ret = 0;
	bool use_sep_index;

	ret = kstrtobool(buf, &use_sep_index);
	if (ret < 0) {
		dev_err(ddata->dev, "%s kstrtobool error : %d\n", __func__, ret);
		goto err;
	}

	dev_info(ddata->dev, "%s use_sep_index:%d\n", __func__, use_sep_index);

	if (ddata->vib_ops->set_use_sep_index) {
		ret = ddata->vib_ops->set_use_sep_index(ddata->input, use_sep_index);
		if (ret) {
			dev_err(ddata->dev, "%s set_use_sep_index error : %d\n", __func__, ret);
			goto err;
		}
	} else {
		dev_info(ddata->dev, "%s this model doesn't need use_sep_index\n", __func__);
	}
err:
	return ret ? ret : count;
}
static DEVICE_ATTR_WO(use_sep_index);

static ssize_t lra_resistance_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_lra_resistance) {
		dev_err(ddata->dev, "%s lra_resistance isn't supported\n", __func__);
		return -EOPNOTSUPP;
	}

	return snprintf(buf, PAGE_SIZE, "%u\n", ddata->vib_ops->get_lra_resistance(ddata->input));
}
static DEVICE_ATTR_RO(lra_resistance);

static ssize_t f0_cal_way_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", ddata->pdata->f0_cal_way);
}
static DEVICE_ATTR_RO(f0_cal_way);

#if defined(CONFIG_SEC_VIB_FOLD_MODEL)
static const char sec_vib_event_cmd[EVENT_CMD_MAX][MAX_STR_LEN_EVENT_CMD] = {
	[EVENT_CMD_NONE]					= "NONE",
	[EVENT_CMD_FOLDER_CLOSE]			= "FOLDER_CLOSE",
	[EVENT_CMD_FOLDER_OPEN]				= "FOLDER_OPEN",
	[EVENT_CMD_ACCESSIBILITY_BOOST_ON]	= "ACCESSIBILITY_BOOST_ON",
	[EVENT_CMD_ACCESSIBILITY_BOOST_OFF]	= "ACCESSIBILITY_BOOST_OFF",
	[EVENT_CMD_TENT_CLOSE]				= "FOLDER_TENT_CLOSE",
	[EVENT_CMD_TENT_OPEN]				= "FOLDER_TENT_OPEN",
};

/* get_event_index_by_command() - internal function to find event command
 *		index from event command string
 */
static int get_event_index_by_command(char *cur_cmd)
{
	int cmd_idx;

	for (cmd_idx = 0; cmd_idx < EVENT_CMD_MAX; cmd_idx++) {
		if (!strcmp(cur_cmd, sec_vib_event_cmd[cmd_idx]))
			return cmd_idx;
	}
	return EVENT_CMD_NONE;
}

/*
 * event_cmd_store() - sysfs interface to inform vibrator HAL
 *		about fold states supported (such as FOLD, FOLD|TENT, etc.,).
 */
static ssize_t event_cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	pr_info("%s event: %s\n", __func__, ddata->event_cmd);
	return snprintf(buf, MAX_STR_LEN_EVENT_CMD, "%s\n", ddata->event_cmd);
}

/*
 * event_cmd_store() - sysfs interface to get the fold state from
 *		user space.
 */
static ssize_t event_cmd_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	if (count > MAX_STR_LEN_EVENT_CMD)
		pr_err("%s: size(%zu) is too long.\n", __func__, count);
	else {
		struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

		if (sscanf(buf, "%s", ddata->event_cmd) == 1) {
			ddata->event_idx = get_event_index_by_command((char *)ddata->event_cmd);

			pr_info("%s: size:%zu, event_cmd: %s, %s (idx=%d)\n",
				__func__, count, buf, ddata->event_cmd, ddata->event_idx);
		} else {
			pr_err("%s invalid param : %s\n", __func__, buf);
			return -EINVAL;
		}
	}

	return count;
}
static DEVICE_ATTR_RW(event_cmd);

/*
 * set_fold_model_ratio() - finds the gain ratio based on fold state
 * @ddata - driver data passed from the driver ic.
 */
int set_fold_model_ratio(struct sec_vib_inputff_drvdata *ddata)
{
	switch (ddata->event_idx) {
	case EVENT_CMD_FOLDER_OPEN:
		return ddata->pdata->fold_open_ratio;
	case EVENT_CMD_FOLDER_CLOSE:
		return ddata->pdata->fold_close_ratio;
	case EVENT_CMD_TENT_CLOSE:
		return ddata->pdata->tent_close_ratio;
	case EVENT_CMD_TENT_OPEN:
		return ddata->pdata->tent_open_ratio;
	case EVENT_CMD_ACCESSIBILITY_BOOST_ON:
	case EVENT_CMD_ACCESSIBILITY_BOOST_OFF:
	case EVENT_CMD_NONE:
	default:
		return ddata->pdata->normal_ratio;
	}

	return ddata->pdata->normal_ratio;
}
/*
 * sec_vib_inputff_event_cmd() - initialize event_cmd string for HAL init
 *		from dt
 * @ddata - driver data passed from the driver ic.
 */
void sec_vib_inputff_event_cmd(struct sec_vib_inputff_drvdata *ddata)
{
	if (sscanf(ddata->pdata->fold_cmd, "%s", ddata->event_cmd) == 1)
		pr_info("%s: fold_cmd: %s, vib_event_cmd: %s\n", __func__,
		ddata->pdata->fold_cmd, ddata->event_cmd);
	else
		pr_err("%s : fold cmd error: Please check dt entry\n", __func__);
}
#endif //CONFIG_SEC_VIB_FOLD_MODEL

static ssize_t owt_lib_compat_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vib_inputff_drvdata *ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_owt_lib_compat_version) {
		dev_err(ddata->dev, "%s get_owt_lib_compat_version isn't supported\n", __func__);
		return -EOPNOTSUPP;
	}
	return snprintf(buf, PAGE_SIZE, "%s\n", ddata->vib_ops->get_owt_lib_compat_version(ddata->input));
}
static DEVICE_ATTR_RO(owt_lib_compat);

static struct attribute *vib_inputff_sys_attr[] = {
	&dev_attr_i2c_test.attr,
	&dev_attr_i2s_test.attr,
	&dev_attr_firmware_load.attr,
	&dev_attr_current_temp.attr,
	&dev_attr_ach_percent.attr,
	&dev_attr_cal_type.attr,
	&dev_attr_trigger_calibration.attr,
	&dev_attr_f0_measured.attr,
	&dev_attr_f0_offset.attr,
	&dev_attr_f0_stored.attr,
	&dev_attr_le_support.attr,
	&dev_attr_le_est.attr,
	&dev_attr_le_stored_param.attr,
	&dev_attr_use_sep_index.attr,
	&dev_attr_lra_resistance.attr,
#if defined(CONFIG_SEC_VIB_FOLD_MODEL)
	&dev_attr_event_cmd.attr,
#endif
	&dev_attr_owt_lib_compat.attr,
	&dev_attr_f0_cal_way.attr,
	NULL,
};

static struct attribute_group vib_inputff_sys_attr_group = {
	.attrs = vib_inputff_sys_attr,
};


static void sec_vib_inputff_vib_param_init(struct sec_vib_inputff_drvdata *ddata)
{
#if defined(CONFIG_VIB_STORE_LE_PARAM)
	ddata->le_stored = vib_le_est;
	if (ddata->le_stored)
		ddata->vib_ops->set_le_stored(ddata->input, ddata->le_stored);
#endif
}

static void sec_vib_inputff_fw_init(struct sec_vib_inputff_drvdata *ddata)
{
	unsigned int i;

	mutex_init(&ddata->fw.stat_lock);
	for (i = 0; i < FWLOAD_TRY; i++)
		ddata->fw.ret[i] = FW_LOAD_INIT;
	ddata->fw.stat = FW_LOAD_INIT;
	ddata->fw.id = 0;
	ddata->fw.ws.name = "sec_vib_inputff";
	wakeup_source_add(&ddata->fw.ws);
	ddata->fw.fw_workqueue = alloc_ordered_workqueue("vib_workqueue",
					WQ_FREEZABLE | WQ_MEM_RECLAIM);
	INIT_WORK(&ddata->fw.wk, sec_vib_inputff_load_firmware_work);
	INIT_DELAYED_WORK(&ddata->fw.retry_wk, sec_vib_inputff_load_firmware_retry);
	INIT_DELAYED_WORK(&ddata->fw.store_wk, firmware_load_store_work);
	ddata->fw_init_attempted = false;
	queue_work(ddata->fw.fw_workqueue, &ddata->fw.wk);
	dev_info(ddata->dev, "%s done\n", __func__);
}

int sec_vib_inputff_sysfs_init(struct sec_vib_inputff_drvdata *ddata)
{
	int ret = 0;

	dev_info(ddata->dev, "%s\n", __func__);

	if (ddata->support_fw) {
		sec_vib_inputff_vib_param_init(ddata);
		sec_vib_inputff_fw_init(ddata);
	} else {
		ddata->fw.stat = FW_LOAD_SUCCESS;
	}

	ddata->cal_workqueue = alloc_ordered_workqueue("calibration_workqueue", WQ_HIGHPRI);
	INIT_WORK(&ddata->cal_work, sec_vib_inputff_start_cal_work);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
	ddata->sec_vib_inputff_class = class_create(THIS_MODULE, "sec_vib_inputff");
#else
	ddata->sec_vib_inputff_class = class_create("sec_vib_inputff");
#endif
	if (IS_ERR(ddata->sec_vib_inputff_class)) {
		ret = PTR_ERR(ddata->sec_vib_inputff_class);
		goto err1;
	}
	ddata->virtual_dev = device_create(ddata->sec_vib_inputff_class,
			NULL, MKDEV(0, 0), ddata, "control");
	if (IS_ERR(ddata->virtual_dev)) {
		ret = PTR_ERR(ddata->virtual_dev);
		goto err2;
	}

	ret = sysfs_create_group(&ddata->virtual_dev->kobj,
			&vib_inputff_sys_attr_group);
	if (ret) {
		ret = -ENODEV;
		dev_err(ddata->dev, "Failed to create sysfs %d\n", ret);
		goto err3;
	}
	dev_info(ddata->dev, "%s done\n", __func__);

	return 0;
err3:
	device_destroy(ddata->sec_vib_inputff_class, MKDEV(0, 0));
err2:
	class_destroy(ddata->sec_vib_inputff_class);
err1:
	return ret;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_sysfs_init);

void sec_vib_inputff_fw_exit(struct sec_vib_inputff_drvdata *ddata)
{
	cancel_work_sync(&ddata->fw.wk);
	cancel_delayed_work_sync(&ddata->fw.retry_wk);
	destroy_workqueue(ddata->fw.fw_workqueue);
	cancel_delayed_work_sync(&ddata->fw.store_wk);
	wakeup_source_remove(&ddata->fw.ws);
}

void sec_vib_inputff_sysfs_exit(struct sec_vib_inputff_drvdata *ddata)
{
	if (ddata->support_fw)
		sec_vib_inputff_fw_exit(ddata);
	if (ddata->cal_workqueue) {
		cancel_work_sync(&ddata->cal_work);
		destroy_workqueue(ddata->cal_workqueue);
	}
	sysfs_remove_group(&ddata->virtual_dev->kobj, &vib_inputff_sys_attr_group);
	device_destroy(ddata->sec_vib_inputff_class, MKDEV(0, 0));
	class_destroy(ddata->sec_vib_inputff_class);
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_sysfs_exit);
