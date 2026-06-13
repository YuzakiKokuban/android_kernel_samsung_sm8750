// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_input.h"

struct sec_ts_external_api_func efunc;
EXPORT_SYMBOL(efunc);

bool sec_input_cmp_ic_status(struct device *dev, int check_bit)
{
	struct sec_ts_plat_data *plat_data = dev->platform_data;

	if (MODE_TO_CHECK_BIT(atomic_read(&plat_data->power_state)) & check_bit)
		return true;

	return false;
}
EXPORT_SYMBOL(sec_input_cmp_ic_status);

bool sec_input_need_ic_off(struct sec_ts_plat_data *pdata)
{
	bool lpm = pdata->lowpower_mode || pdata->ed_enable || pdata->pocket_mode || pdata->fod_lp_mode || pdata->support_always_on;

	return (sec_input_need_fold_off(pdata->multi_dev) || !lpm);
}
EXPORT_SYMBOL(sec_input_need_ic_off);

bool sec_input_need_fw_update(struct sec_ts_plat_data *pdata)
{
	u8 *ver_ic = pdata->img_version_of_ic;
	u8 *ver_bin = pdata->img_version_of_bin;
	int i;

	input_info(true, pdata->dev,
			"%s: [BIN] %02X%02X%02X%02X | [IC] %02X%02X%02X%02X | bringup %d\n",
			__func__, ver_bin[SEC_INPUT_FW_IC_VER], ver_bin[SEC_INPUT_FW_VER_PROJECT_ID],
			ver_bin[SEC_INPUT_FW_MODULE_VER], ver_bin[SEC_INPUT_FW_VER],
			ver_ic[SEC_INPUT_FW_IC_VER], ver_ic[SEC_INPUT_FW_VER_PROJECT_ID],
			ver_ic[SEC_INPUT_FW_MODULE_VER], ver_ic[SEC_INPUT_FW_VER], pdata->bringup);

	if (pdata->bringup == BRINGUP_SKIP_FW_UPDATE_WITH_REQUEST_FW) {
		input_info(true, pdata->dev, "%s: skip update: bringup %d\n", __func__, pdata->bringup);
		return false;
	}

	if (pdata->bringup == BRINGUP_FW_UPDATE_ALWAYS) {
		input_info(true, pdata->dev, "%s: force update: bringup %d\n", __func__, pdata->bringup);
		return true;
	}

	/* check f/w version		-> when mistmatched
	 * ver[0] : IC version		-> skip update
	 * ver[1] : Project ID		-> need update
	 * ver[2] : Module version	-> skip update
	 * ver[3] : Firmware version	-> version compare
	 */
	for (i = SEC_INPUT_FW_IC_VER; i < SEC_INPUT_FW_INDEX_MAX; i++) {
		if (ver_ic[i] != ver_bin[i]) {
			if (pdata->bringup == BRINGUP_FW_UPDATE_WHEN_VERSION_MISMATCH) {
				input_info(true, pdata->dev, "%s: need update: bringup %d\n",
						__func__, pdata->bringup);
				return true;
			} else if (i == SEC_INPUT_FW_IC_VER || i == SEC_INPUT_FW_MODULE_VER) {
				input_info(true, pdata->dev, "%s: skip update: ic/module mismatched\n", __func__);
				return false;
			} else if (i == SEC_INPUT_FW_VER_PROJECT_ID) {
				input_info(true, pdata->dev, "%s: need update: project id mismatched\n", __func__);
				return true;
			} else if (ver_ic[SEC_INPUT_FW_VER] < ver_bin[SEC_INPUT_FW_VER]) {
				input_info(true, pdata->dev, "%s: need update: ic fw is not latest\n", __func__);
				return true;
			}
		}
	}

	input_info(true, pdata->dev, "%s: no need to update\n", __func__);
	return false;
}
EXPORT_SYMBOL(sec_input_need_fw_update);

void sec_input_probe_work_remove(struct sec_ts_plat_data *pdata)
{
	if (pdata == NULL)
		return;

	if (!pdata->work_queue_probe_enabled) {
		input_err(true, pdata->dev, "%s: work_queue_probe_enabled is false\n", __func__);
		return;
	}

	cancel_work_sync(&pdata->probe_work);
	flush_workqueue(pdata->probe_workqueue);
	destroy_workqueue(pdata->probe_workqueue);
}
EXPORT_SYMBOL(sec_input_probe_work_remove);

void sec_input_probe_work(struct work_struct *work)
{
	struct sec_ts_plat_data *pdata = container_of(work, struct sec_ts_plat_data, probe_work);
	int ret = 0;

	if (pdata->probe == NULL) {
		input_err(true, pdata->dev, "%s: probe function is null\n", __func__);
		return;
	}

	sec_delay(pdata->work_queue_probe_delay);

	ret = pdata->probe(pdata->dev);
	if (pdata->first_booting_disabled && ret == 0) {
		input_info(true, pdata->dev, "%s: first_booting_disabled.\n", __func__);
		pdata->disable(pdata->dev);
	}
}

ssize_t sec_input_get_common_hw_param(struct sec_ts_plat_data *pdata, char *buf)
{
	char buff[SEC_INPUT_HW_PARAM_SIZE];
	char tbuff[SEC_CMD_STR_LEN];
	char mdev[SEC_CMD_STR_LEN];

	memset(mdev, 0x00, sizeof(mdev));
	if (GET_DEV_COUNT(pdata->multi_dev) == MULTI_DEV_SUB)
		snprintf(mdev, sizeof(mdev), "%s", "2");
	else
		snprintf(mdev, sizeof(mdev), "%s", "");

	memset(buff, 0x00, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TITO%s\":\"%02X%02X%02X%02X\",",
			mdev, pdata->hw_param.ito_test[0], pdata->hw_param.ito_test[1],
			pdata->hw_param.ito_test[2], pdata->hw_param.ito_test[3]);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TWET%s\":\"%d\",", mdev, pdata->hw_param.wet_count);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TNOI%s\":\"%d\",", mdev, pdata->hw_param.noise_count);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TCOM%s\":\"%d\",", mdev, pdata->hw_param.comm_err_count);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TCHK%s\":\"%d\",", mdev, pdata->hw_param.checksum_result);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TRIC%s\":\"%d\"", mdev, pdata->hw_param.ic_reset_count);
	strlcat(buff, tbuff, sizeof(buff));

	if (GET_DEV_COUNT(pdata->multi_dev) != MULTI_DEV_SUB) {
		memset(tbuff, 0x00, sizeof(tbuff));
		snprintf(tbuff, sizeof(tbuff), ",\"TMUL\":\"%d\",", pdata->hw_param.multi_count);
		strlcat(buff, tbuff, sizeof(buff));

		memset(tbuff, 0x00, sizeof(tbuff));
		snprintf(tbuff, sizeof(tbuff), "\"TTCN\":\"%d\",\"TACN\":\"%d\",\"TSCN\":\"%d\",",
				pdata->hw_param.all_finger_count, pdata->hw_param.all_aod_tap_count,
				pdata->hw_param.all_spay_count);
		strlcat(buff, tbuff, sizeof(buff));

		memset(tbuff, 0x00, sizeof(tbuff));
		snprintf(tbuff, sizeof(tbuff), "\"TMCF\":\"%d\"", pdata->hw_param.mode_change_failed_count);
		strlcat(buff, tbuff, sizeof(buff));
	}

	return snprintf(buf, SEC_INPUT_HW_PARAM_SIZE, "%s", buff);
}
EXPORT_SYMBOL(sec_input_get_common_hw_param);

void sec_input_clear_common_hw_param(struct sec_ts_plat_data *pdata)
{
	pdata->hw_param.multi_count = 0;
	pdata->hw_param.wet_count = 0;
	pdata->hw_param.noise_count = 0;
	pdata->hw_param.comm_err_count = 0;
	pdata->hw_param.checksum_result = 0;
	pdata->hw_param.all_finger_count = 0;
	pdata->hw_param.all_aod_tap_count = 0;
	pdata->hw_param.all_spay_count = 0;
	pdata->hw_param.mode_change_failed_count = 0;
	pdata->hw_param.ic_reset_count = 0;
}
EXPORT_SYMBOL(sec_input_clear_common_hw_param);

void sec_input_print_info(struct device *dev, struct sec_tclm_data *tdata)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	unsigned int irq = gpio_to_irq(pdata->irq_gpio);
	struct irq_desc *desc = irq_to_desc(irq);
	char tclm_buff[INPUT_TCLM_LOG_BUF_SIZE] = { 0 };
	char fw_ver_prefix[7] = { 0 };
	char charger_buff[10] = { 0 };

	pdata->print_info_cnt_open++;

	if (pdata->print_info_cnt_open > 0xfff0)
		pdata->print_info_cnt_open = 0;

	if (pdata->touch_count == 0)
		pdata->print_info_cnt_release++;

#if IS_ENABLED(CONFIG_INPUT_TOUCHSCREEN_TCLMV2)
	if (tdata && (tdata->tclm_level == TCLM_LEVEL_NOT_SUPPORT))
		snprintf(tclm_buff, sizeof(tclm_buff), "");
	else if (tdata && tdata->tclm_string)
		snprintf(tclm_buff, sizeof(tclm_buff), "C%02XT%04X.%4s%s Cal_flag:%d fail_cnt:%d",
			tdata->nvdata.cal_count, tdata->nvdata.tune_fix_ver,
			tdata->tclm_string[tdata->nvdata.cal_position].f_name,
			(tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ",
			tdata->nvdata.cal_fail_flag, tdata->nvdata.cal_fail_cnt);
	else
		snprintf(tclm_buff, sizeof(tclm_buff), "TCLM data is empty");
#else
	snprintf(tclm_buff, sizeof(tclm_buff), "");
#endif

	if (pdata->ic_vendor_name[0] != 0)
		snprintf(fw_ver_prefix, sizeof(fw_ver_prefix), "%c%c%02X%02X",
				pdata->ic_vendor_name[0], pdata->ic_vendor_name[1], pdata->img_version_of_ic[0], pdata->img_version_of_ic[1]);

	if (pdata->support_vbus_notifier)
		snprintf(charger_buff, sizeof(charger_buff), "chg:%d ", pdata->charger_flag);

#if IS_ENABLED(CONFIG_INPUT_SEC_TRUSTED_TOUCH)
	if (IS_ERR_OR_NULL(pdata->pvm)) {
		input_info(true, dev, "%s: pvm is null\n", __func__);
		return;
	}
	if (atomic_read(&pdata->pvm->trusted_touch_enabled) != 1)
#endif
	input_info(true, dev,
			"tc:%d noise:%d/%d ext_n:%d wet:%d %swc:%d(f:%d) lp:%x fn:%04X/%04X irqd:%d int:%d ED:%d PK:%d LS:%d// v:%s%02X%02X %s chk:%d // tmp:%d // #%d %d\n",
			pdata->touch_count,
			atomic_read(&pdata->touch_noise_status), atomic_read(&pdata->touch_pre_noise_status),
			pdata->external_noise_mode, pdata->wet_mode,
			charger_buff, pdata->wirelesscharger_mode, pdata->force_wirelesscharger_mode,
			pdata->lowpower_mode, pdata->touch_functions, pdata->ic_status,
			desc->depth, gpio_get_value(pdata->irq_gpio),
			pdata->ed_enable, pdata->pocket_mode, pdata->low_sensitivity_mode,
			fw_ver_prefix, pdata->img_version_of_ic[2], pdata->img_version_of_ic[3],
			tclm_buff, pdata->hw_param.checksum_result,
			pdata->tsp_temperature_data,
			pdata->print_info_cnt_open, pdata->print_info_cnt_release);
}
EXPORT_SYMBOL(sec_input_print_info);

int sec_input_pinctrl_configure(struct device *dev, bool on)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	struct pinctrl_state *state;

	input_info(true, dev, "%s: %s\n", __func__, on ? "ACTIVE" : "SUSPEND");

	if (sec_check_secure_trusted_mode_status(pdata))
		return 0;

	if (on) {
		state = pinctrl_lookup_state(pdata->pinctrl, "on_state");
		if (IS_ERR(pdata->pinctrl))
			input_err(true, dev, "%s: could not get active pinstate\n", __func__);
	} else {
		state = pinctrl_lookup_state(pdata->pinctrl, "off_state");
		if (IS_ERR(pdata->pinctrl))
			input_err(true, dev, "%s: could not get suspend pinstate\n", __func__);
	}

	if (!IS_ERR_OR_NULL(state))
		return pinctrl_select_state(pdata->pinctrl, state);

	return 0;
}
EXPORT_SYMBOL(sec_input_pinctrl_configure);

bool sec_check_secure_trusted_mode_status(struct sec_ts_plat_data *pdata)
{
#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
	if (atomic_read(&pdata->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, pdata->dev, "%s: secure touch enabled\n", __func__);
		return true;
	}
#if IS_ENABLED(CONFIG_INPUT_SEC_TRUSTED_TOUCH)
	if (!IS_ERR_OR_NULL(pdata->pvm)) {
		if (atomic_read(&pdata->pvm->trusted_touch_enabled) != 0) {
			input_err(true, pdata->dev, "%s: TVM is enabled\n", __func__);
			return true;
		}
	}
#endif
#endif
	return false;
}
EXPORT_SYMBOL(sec_check_secure_trusted_mode_status);

int sec_input_power(struct device *dev, bool on)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int ret = 0;

	if (pdata->power_enabled == on) {
		input_info(true, dev, "%s: power_enabled %d\n", __func__, pdata->power_enabled);
		return ret;
	}

	if (on) {
		if (!pdata->not_support_io_ldo) {
			ret = regulator_enable(pdata->dvdd);
			if (ret) {
				input_err(true, dev, "%s: Failed to enable dvdd: %d\n", __func__, ret);
				goto out;
			}

			sec_delay(1);
		}
		ret = regulator_enable(pdata->avdd);
		if (ret) {
			input_err(true, dev, "%s: Failed to enable avdd: %d\n", __func__, ret);
			goto out;
		}
	} else {
		regulator_disable(pdata->avdd);
		if (!pdata->not_support_io_ldo) {
			sec_delay(4);
			regulator_disable(pdata->dvdd);
		}
	}

	pdata->power_enabled = on;

out:
	if (!pdata->not_support_io_ldo) {
		input_info(true, dev, "%s: %s: avdd:%s, dvdd:%s\n", __func__, on ? "on" : "off",
				regulator_is_enabled(pdata->avdd) ? "on" : "off",
				regulator_is_enabled(pdata->dvdd) ? "on" : "off");
	} else {
		input_info(true, dev, "%s: %s: avdd:%s\n", __func__, on ? "on" : "off",
				regulator_is_enabled(pdata->avdd) ? "on" : "off");
	}

	return ret;
}
EXPORT_SYMBOL(sec_input_power);

#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
int sec_input_ccic_notification(struct notifier_block *nb,
	   unsigned long action, void *data)
{
	struct sec_ts_plat_data *pdata = container_of(nb, struct sec_ts_plat_data, ccic_nb);
	PD_NOTI_USB_STATUS_TYPEDEF usb_status = *(PD_NOTI_USB_STATUS_TYPEDEF *)data;

	if (pdata->dev == NULL) {
		pr_err("%s %s: dev is null\n", SECLOG, __func__);
		return 0;
	}

	if (usb_status.dest != PDIC_NOTIFY_DEV_USB)
		return 0;

	switch (usb_status.drp) {
	case USB_STATUS_NOTIFY_ATTACH_DFP:
		pdata->otg_flag = true;
		input_info(true, pdata->dev, "%s: otg_flag %d\n", __func__, pdata->otg_flag);
		break;
	case USB_STATUS_NOTIFY_DETACH:
		pdata->otg_flag = false;
		input_info(true, pdata->dev, "%s: otg_flag %d\n", __func__, pdata->otg_flag);
		break;
	default:
		break;
	}
	return 0;
}
#endif

int sec_input_vbus_notification(struct notifier_block *nb,
		unsigned long cmd, void *data)
{
	struct sec_ts_plat_data *pdata = container_of(nb, struct sec_ts_plat_data, vbus_nb);
	vbus_status_t vbus_type = *(vbus_status_t *) data;

	if (pdata->dev == NULL) {
		pr_err("%s %s: dev is null\n", SECLOG, __func__);
		return 0;
	}

	input_info(true, pdata->dev, "%s: cmd=%lu, vbus_type=%d, otg_flag:%d\n",
			__func__, cmd, vbus_type, pdata->otg_flag);

	if (atomic_read(&pdata->shutdown_called))
		return 0;

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
		if (!pdata->otg_flag)
			pdata->charger_flag = true;
		else
			return 0;
		break;
	case STATUS_VBUS_LOW:
		pdata->charger_flag = false;
		break;
	default:
		return 0;
	}

	queue_work(pdata->vbus_notifier_workqueue, &pdata->vbus_notifier_work);

	return 0;
}

void sec_input_vbus_notification_work(struct work_struct *work)
{
	struct sec_ts_plat_data *pdata = container_of(work, struct sec_ts_plat_data, vbus_notifier_work);
	int ret = 0;

	if (pdata->dev == NULL) {
		pr_err("%s %s: dev is null\n", SECLOG, __func__);
		return;
	}

	if (pdata->set_charger_mode == NULL) {
		input_err(true, pdata->dev, "%s: set_charger_mode function is not allocated\n", __func__);
		return;
	}

	if (atomic_read(&pdata->shutdown_called))
		return;

	input_info(true, pdata->dev, "%s: charger_flag:%d\n", __func__, pdata->charger_flag);

	mutex_lock(&pdata->enable_mutex);
	ret = pdata->set_charger_mode(pdata->dev, pdata->charger_flag);
	mutex_unlock(&pdata->enable_mutex);
	if (ret < 0) {
		input_info(true, pdata->dev, "%s: failed to set charger_flag\n", __func__);
		return;
	}
}
#endif

void sec_input_register_vbus_notifier(struct device *dev)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	if (!pdata->support_vbus_notifier)
		return;

	input_info(true, dev, "%s\n", __func__);

	pdata->otg_flag = false;
	pdata->charger_flag = false;

#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	manager_notifier_register(&pdata->ccic_nb, sec_input_ccic_notification,
		MANAGER_NOTIFY_PDIC_INITIAL);
	input_info(true, dev, "%s: register ccic notification\n", __func__);
#endif
	pdata->vbus_notifier_workqueue = create_singlethread_workqueue("sec_input_vbus_noti");
	INIT_WORK(&pdata->vbus_notifier_work, sec_input_vbus_notification_work);
	vbus_notifier_register(&pdata->vbus_nb, sec_input_vbus_notification, VBUS_NOTIFY_DEV_CHARGER);
	input_info(true, dev, "%s: register vbus notification\n", __func__);
#endif
	pdata->vbus_notifier_init_flag = true;

}
EXPORT_SYMBOL(sec_input_register_vbus_notifier);

void sec_input_unregister_vbus_notifier(struct device *dev)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	if (!pdata->support_vbus_notifier)
		return;

	if (!pdata->vbus_notifier_init_flag) {
		input_info(true, dev, "%s: skip(%d)\n", __func__, pdata->vbus_notifier_init_flag);
		return;
	}

	input_info(true, dev, "%s\n", __func__);

#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	manager_notifier_unregister(&pdata->ccic_nb);
#endif
	vbus_notifier_unregister(&pdata->vbus_nb);
	cancel_work_sync(&pdata->vbus_notifier_work);
	flush_workqueue(pdata->vbus_notifier_workqueue);
	destroy_workqueue(pdata->vbus_notifier_workqueue);
#endif
}
EXPORT_SYMBOL(sec_input_unregister_vbus_notifier);

int sec_input_enable_device(struct device *dev)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int retval;

	sec_input_utc_marker(dev, __func__);

	retval = mutex_lock_interruptible(&pdata->enable_mutex);
	if (retval)
		return retval;

	if (pdata->enable)
		retval = pdata->enable(dev);

	mutex_unlock(&pdata->enable_mutex);
	return retval;
}
EXPORT_SYMBOL(sec_input_enable_device);

int sec_input_disable_device(struct device *dev)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int retval;

	sec_input_utc_marker(dev, __func__);

	retval = mutex_lock_interruptible(&pdata->enable_mutex);
	if (retval)
		return retval;

	if (pdata->disable)
		retval = pdata->disable(dev);

	mutex_unlock(&pdata->enable_mutex);
	return 0;
}
EXPORT_SYMBOL(sec_input_disable_device);

void sec_input_utc_marker(struct device *dev, const char *annotation)
{
	struct timespec64 ts;
	struct rtc_time tm;

	ktime_get_real_ts64(&ts);
	rtc_time64_to_tm(ts.tv_sec, &tm);
	input_info(true, dev, "%s %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
		annotation, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
}

void sec_delay(unsigned int ms)
{
	if (!ms)
		return;
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}
EXPORT_SYMBOL(sec_delay);

#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
static struct mutex input_log_mutex;
static struct mutex input_fail_hist_log_mutex;
static struct mutex input_raw_info_log_mutex;

void sec_input_log(bool mode, struct device *dev, char *fmt, ...)
{
	static char input_module_info_buf[INPUT_MODULE_INFO_BUF_SIZE];
	static char input_log_buf[INPUT_LOG_BUF_SIZE];
	va_list args;

	if (!mode)
		return;

	mutex_lock(&input_log_mutex);
	if (dev)
		snprintf(input_module_info_buf, INPUT_MODULE_INFO_BUF_SIZE, "%d:%s %s %s",
				current->pid, current->comm,
				dev_driver_string(dev), dev_name(dev));
	else
		snprintf(input_module_info_buf, INPUT_MODULE_INFO_BUF_SIZE, "%d:%s NULL",
				current->pid, current->comm);

	va_start(args, fmt);
	vsnprintf(input_log_buf, sizeof(input_log_buf), fmt, args);
	sec_debug_tsp_log_msg(input_module_info_buf, input_log_buf);
	va_end(args);
	mutex_unlock(&input_log_mutex);
}
EXPORT_SYMBOL(sec_input_log);

void sec_input_fail_hist_log(bool mode, struct device *dev, char *fmt, ...)
{
	static char input_module_info_buf[INPUT_MODULE_INFO_BUF_SIZE];
	static char input_log_buf[INPUT_LOG_BUF_SIZE];
	va_list args;

	if (!mode)
		return;

	mutex_lock(&input_fail_hist_log_mutex);
	if (dev)
		snprintf(input_module_info_buf, INPUT_MODULE_INFO_BUF_SIZE, "%d:%s %s %s",
				current->pid, current->comm,
				dev_driver_string(dev), dev_name(dev));
	else
		snprintf(input_module_info_buf, INPUT_MODULE_INFO_BUF_SIZE, "%d:%s NULL",
				current->pid, current->comm);

	va_start(args, fmt);
	vsnprintf(input_log_buf, sizeof(input_log_buf), fmt, args);
	sec_debug_tsp_log_msg(input_module_info_buf, input_log_buf);
	sec_debug_tsp_fail_hist(input_module_info_buf, input_log_buf);
	va_end(args);
	mutex_unlock(&input_fail_hist_log_mutex);
}
EXPORT_SYMBOL(sec_input_fail_hist_log);

void sec_input_raw_info_log(int dev_count, struct device *dev, char *fmt, ...)
{
	static char input_module_info_buf[INPUT_MODULE_INFO_BUF_SIZE];
	static char input_log_buf[INPUT_LOG_BUF_SIZE];
	va_list args;

	mutex_lock(&input_raw_info_log_mutex);
	if (dev)
		snprintf(input_module_info_buf, sizeof(input_module_info_buf), "%s",
				dev_driver_string(dev));
	else
		snprintf(input_module_info_buf, sizeof(input_module_info_buf), "NULL");

	va_start(args, fmt);
	vsnprintf(input_log_buf, sizeof(input_log_buf), fmt, args);
	sec_debug_tsp_log_msg(input_module_info_buf, input_log_buf);
	sec_debug_tsp_raw_data_msg(dev_count, input_module_info_buf, input_log_buf);
	va_end(args);
	mutex_unlock(&input_raw_info_log_mutex);
}
EXPORT_SYMBOL(sec_input_raw_info_log);
#endif

void sec_external_api_init(void)
{
	efunc.devm_input_allocate_device = devm_input_allocate_device;
	efunc.input_register_device = input_register_device;
	efunc.power_supply_get_by_name = power_supply_get_by_name;
	efunc.power_supply_get_property = power_supply_get_property;
	efunc.input_sync = input_sync;
	pr_info("%s %s done\n", SECLOG, __func__);
}

static int __init sec_input_init(void)
{
	int ret = 0;

	pr_info("%s %s: 2024: ++\n", SECLOG, __func__);

#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
	mutex_init(&input_log_mutex);
	mutex_init(&input_fail_hist_log_mutex);
	mutex_init(&input_raw_info_log_mutex);

	sec_external_api_init();

	ret = sec_tsp_log_init();
	pr_info("%s %s: sec_tsp_log_init %d\n", SECLOG, __func__, ret);
#endif
#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
	ret = sec_secure_touch_init();
	pr_info("%s %s: sec_secure_touch_init %d\n", SECLOG, __func__, ret);
#endif
#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
	ret = sec_tsp_dumpkey_init();
	pr_info("%s %s: sec_tsp_dumpkey_init %d\n", SECLOG, __func__, ret);
#endif
	pr_info("%s %s --\n", SECLOG, __func__);
	return ret;
}

static void __exit sec_input_exit(void)
{
	pr_info("%s %s ++\n", SECLOG, __func__);
#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
	sec_secure_touch_exit();
#endif
#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
	sec_tsp_dumpkey_exit();
#endif
	pr_info("%s %s --\n", SECLOG, __func__);
}

module_init(sec_input_init);
module_exit(sec_input_exit);

MODULE_DESCRIPTION("Samsung input common functions");
MODULE_LICENSE("GPL");
