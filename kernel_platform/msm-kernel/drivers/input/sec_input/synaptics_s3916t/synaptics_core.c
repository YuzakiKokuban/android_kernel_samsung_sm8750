// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "synaptics_dev.h"
#include "synaptics_reg.h"

#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
irqreturn_t synaptics_secure_filter_interrupt(struct synaptics_ts_data *ts)
{
	if (atomic_read(&ts->plat_data->secure_enabled) == SECURE_TOUCH_ENABLE) {
		if (atomic_cmpxchg(&ts->plat_data->secure_pending_irqs, 0, 1) == 0) {
			sysfs_notify(&ts->plat_data->input_dev->dev.kobj, NULL, "secure_touch");

		} else {
			input_info(true, ts->dev, "%s: pending irq:%d\n",
					__func__, (int)atomic_read(&ts->plat_data->secure_pending_irqs));
		}

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

/**
 * Sysfs attr group for secure touch & interrupt handler for Secure world.
 * @atomic : syncronization for secure_enabled
 * @pm_runtime : set rpm_resume or rpm_ilde
 */
static ssize_t secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d", atomic_read(&ts->plat_data->secure_enabled));
}

static ssize_t secure_touch_enable_store(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	int ret;
	unsigned long data;

	if (count > 2) {
		input_err(true, ts->dev,
				"%s: cmd length is over (%s,%d)!!\n",
				__func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	ret = kstrtoul(buf, 10, &data);
	if (ret != 0) {
		input_err(true, ts->dev, "%s: failed to read:%d\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data == 1) {
		if (atomic_read(&ts->reset_is_on_going)) {
			input_err(true, ts->dev, "%s: reset is on going because i2c fail\n", __func__);
			return -EBUSY;
		}

		/* Enable Secure World */
		if (atomic_read(&ts->plat_data->secure_enabled) == SECURE_TOUCH_ENABLE) {
			input_err(true, ts->dev, "%s: already enabled\n", __func__);
			return -EBUSY;
		}

		sec_delay(200);

		/* synchronize_irq -> disable_irq + enable_irq
		 * concern about timing issue.
		 */
		sec_input_irq_disable(ts->plat_data);

		/* Release All Finger */
		synaptics_ts_release_all_finger(ts);

		if (pm_runtime_get_sync(ts->plat_data->bus_master->parent) < 0) {
			sec_input_irq_enable(ts->plat_data);
			input_err(true, ts->dev, "%s: failed to get pm_runtime\n", __func__);
			return -EIO;
		}

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		sec_input_notify(&ts->synaptics_input_nb, NOTIFIER_SECURE_TOUCH_ENABLE, NULL);
#endif
		reinit_completion(&ts->plat_data->secure_powerdown);
		reinit_completion(&ts->plat_data->secure_interrupt);

		atomic_set(&ts->plat_data->secure_enabled, 1);
		atomic_set(&ts->plat_data->secure_pending_irqs, 0);

		sec_input_irq_enable(ts->plat_data);

		input_info(true, ts->dev, "%s: secure touch enable\n", __func__);
	} else if (data == 0) {
		/* Disable Secure World */
		if (atomic_read(&ts->plat_data->secure_enabled) == SECURE_TOUCH_DISABLE) {
			input_err(true, ts->dev, "%s: already disabled\n", __func__);
			return count;
		}

		sec_delay(200);

		pm_runtime_put_sync(ts->plat_data->bus_master->parent);
		atomic_set(&ts->plat_data->secure_enabled, 0);

		sysfs_notify(&ts->plat_data->input_dev->dev.kobj, NULL, "secure_touch");

		sec_delay(10);

		synaptics_ts_irq_thread(ts->irq, ts);
		complete(&ts->plat_data->secure_interrupt);
		complete(&ts->plat_data->secure_powerdown);

		input_info(true, ts->dev, "%s: secure touch disable\n", __func__);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		sec_input_notify(&ts->synaptics_input_nb, NOTIFIER_SECURE_TOUCH_DISABLE, NULL);
#endif
	} else {
		input_err(true, ts->dev, "%s: unsupport value:%ld\n", __func__, data);
		return -EINVAL;
	}

	return count;
}

static ssize_t secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&ts->plat_data->secure_enabled) == SECURE_TOUCH_DISABLE) {
		input_err(true, ts->dev, "%s: disabled\n", __func__);
		return -EBADF;
	}

	if (atomic_cmpxchg(&ts->plat_data->secure_pending_irqs, -1, 0) == -1) {
		input_err(true, ts->dev, "%s: pending irq -1\n", __func__);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&ts->plat_data->secure_pending_irqs, 1, 0) == 1) {
		val = 1;
		input_err(true, ts->dev, "%s: pending irq is %d\n",
				__func__, atomic_read(&ts->plat_data->secure_pending_irqs));
	}

	complete(&ts->plat_data->secure_interrupt);

	return snprintf(buf, PAGE_SIZE, "%u", val);
}

static ssize_t secure_ownership_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "1");
}

static int secure_touch_init(struct synaptics_ts_data *ts)
{
	input_info(true, ts->dev, "%s\n", __func__);

	init_completion(&ts->plat_data->secure_interrupt);
	init_completion(&ts->plat_data->secure_powerdown);

	return 0;
}

static void secure_touch_stop(struct synaptics_ts_data *ts, bool stop)
{
	if (atomic_read(&ts->plat_data->secure_enabled)) {
		atomic_set(&ts->plat_data->secure_pending_irqs, -1);

		sysfs_notify(&ts->plat_data->input_dev->dev.kobj, NULL, "secure_touch");

		if (stop)
			wait_for_completion_interruptible(&ts->plat_data->secure_powerdown);

		input_info(true, ts->dev, "%s: %d\n", __func__, stop);
	}
}

static DEVICE_ATTR_RW(secure_touch_enable);
static DEVICE_ATTR_RO(secure_touch);
static DEVICE_ATTR_RO(secure_ownership);
static struct attribute *secure_attr[] = {
	&dev_attr_secure_touch_enable.attr,
	&dev_attr_secure_touch.attr,
	&dev_attr_secure_ownership.attr,
	NULL,
};

static struct attribute_group secure_attr_group = {
	.attrs = secure_attr,
};
#endif

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
static int synaptics_touch_notify_call(struct notifier_block *n, unsigned long data, void *v)
{
	struct synaptics_ts_data *ts = container_of(n, struct synaptics_ts_data, synaptics_input_nb);

	if (!ts->info_work_done) {
		input_info(true, ts->dev, "%s: info work is not done. skip\n", __func__);
		return NOTIFY_DONE;
	}

	if (atomic_read(&ts->plat_data->shutdown_called))
		return NOTIFY_DONE;

	switch (data) {
	case NOTIFIER_TSP_BLOCKING_REQUEST:
		/* just notify scan block uevent for fingerprint */
		input_info(false, ts->dev, "%s: tsp block(fake)\n", __func__);
		sec_cmd_send_gesture_uevent(&ts->sec, SPONGE_EVENT_TYPE_TSP_SCAN_BLOCK, 0, 0);
		break;
	case NOTIFIER_TSP_BLOCKING_RELEASE:
		/* just notify scan unblock uevent for fingerprint */
		input_info(false, ts->dev, "%s: tsp unblock(fake)\n", __func__);
		sec_cmd_send_gesture_uevent(&ts->sec, SPONGE_EVENT_TYPE_TSP_SCAN_UNBLOCK, 0, 0);
		break;
	case NOTIFIER_WACOM_PEN_HOVER_IN:
		ts->pen_hover_in = 1;
		schedule_work(&ts->work_notify_pen_hover.work);
		input_info(true, ts->dev, "%s: pen hover in detect\n", __func__);
		break;
	case NOTIFIER_WACOM_PEN_HOVER_OUT:
		ts->pen_hover_in = 0;
		schedule_work(&ts->work_notify_pen_hover.work);
		input_info(true, ts->dev, "%s: pen hover out detect\n", __func__);
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}
#endif

void synaptics_ts_reinit(void *data)
{
	struct synaptics_ts_data *ts = (struct synaptics_ts_data *)data;
	int ret = 0, retry = 0;

	if (ts->raw_mode)
		synaptics_ts_set_up_rawdata_report_type(ts);

	synaptics_ts_rezero(ts);

	input_info(true, ts->dev,
		"%s: charger=0x%x, touch_functions=0x%x, Power mode=0x%x ic mode = %d\n",
		__func__, ts->plat_data->charger_flag, ts->plat_data->touch_functions, atomic_read(&ts->plat_data->power_state), ts->dev_mode);

	atomic_set(&ts->plat_data->touch_noise_status, 0);
	atomic_set(&ts->plat_data->touch_pre_noise_status, 0);
	ts->plat_data->wet_mode = 0;
	/* buffer clear asking */
	synaptics_ts_release_all_finger(ts);

	for (retry = 0; retry < SYNAPTICS_TS_TIMEOUT_RETRY_CNT; retry++) {
		ret = synaptics_ts_set_custom_library(ts);
		if (ret == -ETIMEDOUT) {
#ifdef SYNAPTICS_TS_RESET_ON_REINIT_TIMEOUT
			ts->plat_data->hw_param.ic_reset_count++;
			input_fail_hist(true, ts->dev, "%s: timeout error. do hw reset\n", __func__);
			sec_input_irq_disable(ts->plat_data);
			synaptics_ts_hw_reset(ts);
			synaptics_ts_setup(ts);
			sec_input_irq_enable(ts->plat_data);

			/* reinit again*/
			if (ts->raw_mode)
				synaptics_ts_set_up_rawdata_report_type(ts);
			synaptics_ts_rezero(ts);
#else
			input_fail_hist(true, ts->dev, "%s: timeout error.\n", __func__);
			break;
#endif
		} else {
			break;
		}
	}

	if (retry >= SYNAPTICS_TS_TIMEOUT_RETRY_CNT) {
		input_fail_hist(true, ts->dev, "%s: failed to recover ic from timeout error\n", __func__);
		return;
	}

	synaptics_ts_set_press_property(ts);

	if (ts->cover_closed)
		synaptics_ts_set_cover_type(ts, ts->cover_closed);

	if (ts->plat_data->support_fod && ts->plat_data->fod_data.set_val)
		synaptics_ts_set_fod_rect(ts);

	/* Power mode */
	if (sec_input_cmp_ic_status(ts->dev, CHECK_LPMODE)) {
		ts->plat_data->lpmode(ts, TO_LOWPOWER_MODE);
		sec_delay(50);
		if (ts->plat_data->lowpower_mode & SEC_TS_MODE_SPONGE_AOD)
			synaptics_ts_set_aod_rect(ts);
	} else {
		sec_input_set_grip_type(ts->dev, GRIP_ALL_DATA);
	}

	input_info(true, ts->dev, "%s: [mode recovery] ed:%d, pocket:%d, low_sen:%d, charger:%d, glove:%d, "
			"dead_zone:%d, game:%d, fast_resp:%d, fix_active:%d, scan_rate:%d, refresh_rate:%d, sip:%d, "
			"wireless_charger:%d, note:%d\n",
			__func__, ts->plat_data->ed_enable, ts->plat_data->pocket_mode,
			ts->plat_data->low_sensitivity_mode, ts->plat_data->charger_flag, ts->glove_mode,
			ts->dead_zone, ts->game_mode, ts->fast_response, ts->fix_active_mode, ts->scan_rate, ts->refresh_rate,
			ts->sip_mode, ts->plat_data->wirelesscharger_mode, ts->note_mode);

	if (ts->plat_data->ed_enable)
		synaptics_ts_ear_detect_enable(ts, ts->plat_data->ed_enable);
	if (ts->plat_data->pocket_mode)
		synaptics_ts_pocket_mode_enable(ts, ts->plat_data->pocket_mode);
	if (ts->plat_data->low_sensitivity_mode)
		synaptics_ts_low_sensitivity_mode_enable(ts, ts->plat_data->low_sensitivity_mode);
	if (ts->plat_data->charger_flag)
		synaptics_ts_set_charger_mode(ts->dev, ts->plat_data->charger_flag);
	if (ts->glove_mode)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_HIGHSENSMODE, ts->glove_mode);
	if (ts->dead_zone)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_DEADZONE, ts->dead_zone);
	if (ts->game_mode)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_GAMEMODE, ts->game_mode);
	if (ts->fast_response)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_FAST_RESPONSE, ts->fast_response);
	if (ts->fix_active_mode)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_DISABLE_DOZE, ts->fix_active_mode);
	if (ts->scan_rate)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_SET_SCANRATE, ts->scan_rate);
	if (ts->refresh_rate)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_SET_SCANRATE, ts->refresh_rate);
	if (ts->sip_mode)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_SIPMODE, ts->sip_mode);
	if (ts->plat_data->wirelesscharger_mode != TYPE_WIRELESS_CHARGER_NONE)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_WIRELESS_CHARGER,
			ts->plat_data->wirelesscharger_mode);
	if (ts->note_mode)
		synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_NOTEMODE, ts->note_mode);
}

/**
 * synaptics_ts_init_message_wrap()
 *
 * Initialize internal buffers and related structures for command processing.
 * The function must be called to prepare all essential structures for
 * command wrapper.
 *
 * @param
 *    [in] tcm_msg: message wrapper structure
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_init_message_wrap(struct synaptics_ts_data *ts, struct synaptics_ts_message_data_blob *tcm_msg)
{
	/* initialize internal buffers */
	synaptics_ts_buf_init(&tcm_msg->in);
	synaptics_ts_buf_init(&tcm_msg->out);
	synaptics_ts_buf_init(&tcm_msg->temp);

	/* allocate the completion event for command processing */
	if (synaptics_ts_pal_completion_alloc(&tcm_msg->cmd_completion) < 0) {
		input_err(true, ts->dev, "%s:Fail to allocate cmd completion event\n", __func__);
		return -EINVAL;
	}

	/* allocate the cmd_mutex for command protection */
	if (synaptics_ts_pal_mutex_alloc(&tcm_msg->cmd_mutex) < 0) {
		input_err(true, ts->dev, "%s:Fail to allocate cmd_mutex\n", __func__);
		return -EINVAL;
	}

	/* allocate the rw_mutex for rw protection */
	if (synaptics_ts_pal_mutex_alloc(&tcm_msg->rw_mutex) < 0) {
		input_err(true, ts->dev, "%s:Fail to allocate rw_mutex\n", __func__);
		return -EINVAL;
	}

	/* set default state of command_status  */
	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);

	/* allocate the internal buffer.in at first */
	synaptics_ts_buf_lock(&tcm_msg->in);

	if (synaptics_ts_buf_alloc(&tcm_msg->in, SYNAPTICS_TS_MESSAGE_HEADER_SIZE) < 0) {
		input_err(true, ts->dev, "%s:Fail to allocate memory for buf.in (size = %d)\n", __func__,
			SYNAPTICS_TS_MESSAGE_HEADER_SIZE);
		tcm_msg->in.buf_size = 0;
		tcm_msg->in.data_length = 0;
		synaptics_ts_buf_unlock(&tcm_msg->in);
		return -EINVAL;
	}
	tcm_msg->in.buf_size = SYNAPTICS_TS_MESSAGE_HEADER_SIZE;

	synaptics_ts_buf_unlock(&tcm_msg->in);

	return 0;
}

/**
 * synaptics_ts_del_message_wrap()
 *
 * Remove message wrapper interface and internal buffers.
 * Call the function once the message wrapper is no longer needed.
 *
 * @param
 *    [in] tcm_msg: message wrapper structure
 *
 * @return
 *    none.
 */
static void synaptics_ts_del_message_wrap(struct synaptics_ts_message_data_blob *tcm_msg)
{
	/* release the mutex */
	synaptics_ts_pal_mutex_free(&tcm_msg->rw_mutex);
	synaptics_ts_pal_mutex_free(&tcm_msg->cmd_mutex);

	/* release the completion event */
	synaptics_ts_pal_completion_free(&tcm_msg->cmd_completion);

	/* release internal buffers  */
	synaptics_ts_buf_release(&tcm_msg->temp);
	synaptics_ts_buf_release(&tcm_msg->out);
	synaptics_ts_buf_release(&tcm_msg->in);
}

/**
 * synaptics_ts_allocate_device()
 *
 * Create the TouchCom core device handle.
 * This function must be called in order to allocate the main device handle,
 * structure synaptics_ts_dev, which will be passed to all other operations and
 * functions within the entire source code.
 *
 * Meanwhile, caller has to prepare specific synaptics_ts_hw_interface structure,
 * so that all the implemented functions can access hardware components
 * through synaptics_ts_hw_interface.
 *
 * @param
 *    [out] ptcm_dev_ptr: a pointer to the device handle returned
 *    [ in] hw_if:        hardware-specific data on target platform
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_allocate_device(struct synaptics_ts_data *ts)
{
	int retval = 0;

	ts->write_message = NULL;
	ts->read_message = NULL;
	ts->write_immediate_message = NULL;

	/* allocate internal buffers */
	synaptics_ts_buf_init(&ts->report_buf);
	synaptics_ts_buf_init(&ts->resp_buf);
	synaptics_ts_buf_init(&ts->external_buf);
	synaptics_ts_buf_init(&ts->touch_config);
	synaptics_ts_buf_init(&ts->event_data);

	/* initialize the command wrapper interface */
	retval = synaptics_ts_init_message_wrap(ts, &ts->msg_data);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to initialize command interface\n", __func__);
		goto err_init_message_wrap;
	}

	input_err(true, ts->dev, "%s:TouchCom core module created, ver.: %d.%02d\n", __func__,
		(unsigned char)(SYNA_TCM_CORE_LIB_VERSION >> 8),
		(unsigned char)SYNA_TCM_CORE_LIB_VERSION & 0xff);

	input_err(true, ts->dev, "%s:Capability: wr_chunk(%d), rd_chunk(%d)\n", __func__,
		ts->max_wr_size, ts->max_rd_size);

	return 0;

err_init_message_wrap:
	synaptics_ts_buf_release(&ts->touch_config);
	synaptics_ts_buf_release(&ts->external_buf);
	synaptics_ts_buf_release(&ts->report_buf);
	synaptics_ts_buf_release(&ts->resp_buf);
	synaptics_ts_buf_release(&ts->event_data);

	return retval;
}

/**
 * synaptics_ts_remove_device()
 *
 * Remove the TouchCom core device handler.
 * This function must be invoked when the device is no longer needed.
 *
 * @param
 *    [ in] tcm_dev: the device handle
 *
 * @return
 *    none.
 */
void synaptics_ts_remove_device(struct synaptics_ts_data *ts)
{
	if (!ts) {
		pr_err("%s%s: Invalid tcm device handle\n", SECLOG, __func__);
		return;
	}

	/* release the command interface */
	synaptics_ts_del_message_wrap(&ts->msg_data);

	/* release buffers */
	synaptics_ts_buf_release(&ts->touch_config);
	synaptics_ts_buf_release(&ts->external_buf);
	synaptics_ts_buf_release(&ts->report_buf);
	synaptics_ts_buf_release(&ts->resp_buf);
	synaptics_ts_buf_release(&ts->event_data);

	input_err(true, ts->dev, "%s: Failtcm device handle removed\n", __func__);
}


/*
 * don't need it in interrupt handler in reality, but, need it in vendor IC for requesting vendor IC.
 * If you are requested additional i2c protocol in interrupt handler by vendor.
 * please add it in synaptics_ts_external_func.
 */
void synaptics_ts_external_func(struct synaptics_ts_data *ts)
{
	if (ts->support_immediate_cmd && ts->plat_data->touch_count == 0)
		sec_input_set_temperature(ts->dev, SEC_INPUT_SET_TEMPERATURE_IN_IRQ);
}

int synaptics_ts_enable(struct device *dev)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	int ret;

	if (!ts->probe_done) {
		input_err(true, ts->dev, "%s: probe is not done yet\n", __func__);
		ts->plat_data->first_booting_disabled = false;
		return 0;
	}

	cancel_delayed_work_sync(&ts->work_read_info);

	atomic_set(&ts->plat_data->enabled, 1);
	ts->plat_data->prox_power_off = 0;

#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
	secure_touch_stop(ts, 0);
#endif

	if (sec_input_cmp_ic_status(ts->dev, CHECK_LPMODE)) {
		ts->plat_data->lpmode(ts, TO_TOUCH_MODE);
		sec_input_set_grip_type(ts->dev, ONLY_EDGE_HANDLER);
		if (ts->plat_data->low_sensitivity_mode)
			synaptics_ts_low_sensitivity_mode_enable(ts, ts->plat_data->low_sensitivity_mode);
	} else {
		ret = ts->plat_data->start_device(ts);
		if (ret < 0)
			input_err(true, ts->dev, "%s: Failed to start device\n", __func__);
	}

	sec_input_set_temperature(ts->dev, SEC_INPUT_SET_TEMPERATURE_FORCE);

	cancel_delayed_work(&ts->work_print_info);
	ts->plat_data->print_info_cnt_open = 0;
	ts->plat_data->print_info_cnt_release = 0;
	if (!atomic_read(&ts->plat_data->shutdown_called))
		schedule_work(&ts->work_print_info.work);

	ts->irq_empty_count = 0;

	if (atomic_read(&ts->plat_data->power_state) != SEC_INPUT_STATE_POWER_OFF)
		sec_input_irq_enable(ts->plat_data);
	return 0;
}

#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH) && IS_ENABLED(CONFIG_INPUT_SEC_TRUSTED_TOUCH)
static int synaptics_trusted_touch_pre_func(struct device *dev, int value)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	struct sec_cmd_data *sec = &ts->sec;

	if (!ts->plat_data->support_rawdata)
		return 0;

	mutex_lock(&sec->cmd_lock);
	if (value != 0) {
		if (ts->raw_mode)
			synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_EXTENDED_TOUCH_REPORT_CONTROL, 0x00);
	}
	mutex_unlock(&sec->cmd_lock);

	return 0;
}

static int synaptics_trusted_touch_post_func(struct device *dev, int value)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	struct sec_cmd_data *sec = &ts->sec;

	if (!ts->plat_data->support_rawdata)
		return 0;

	mutex_lock(&sec->cmd_lock);
	if (value == 0) {
		if (ts->raw_mode) {
			if (ts->game_mode)
				input_info(true, ts->dev, "%s: game mode is ongoing\n", __func__);
			else
				synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_EXTENDED_TOUCH_REPORT_CONTROL, 0x01);
		}
	}
	mutex_unlock(&sec->cmd_lock);

	return 0;
}
#endif

int synaptics_ts_disable(struct device *dev)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);

	if (!ts->probe_done) {
		ts->plat_data->first_booting_disabled = true;
		input_err(true, ts->dev, "%s: probe is not done yet\n", __func__);
		return 0;
	}

	cancel_delayed_work_sync(&ts->work_read_info);
	cancel_delayed_work_sync(&ts->work_print_info);
	cancel_delayed_work_sync(&ts->set_temperature_work);

	if (atomic_read(&ts->plat_data->shutdown_called)) {
		input_err(true, ts->dev, "%s shutdown was called\n", __func__);
		return 0;
	}

	atomic_set(&ts->plat_data->enabled, 0);

#ifdef TCLM_CONCEPT
	sec_tclm_debug_info(ts->tdata);
#endif
	sec_input_print_info(ts->dev, ts->tdata);
#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
	secure_touch_stop(ts, 1);
#if IS_ENABLED(CONFIG_INPUT_SEC_TRUSTED_TOUCH)
	if (atomic_read(&ts->plat_data->pvm->trusted_touch_enabled)) {
		input_err(true, ts->dev, "%s wait for disabling trusted touch\n", __func__);
		wait_for_completion_interruptible(&ts->plat_data->pvm->trusted_touch_powerdown);
	}
#endif
#endif
#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	stui_cancel_session();
#endif
	cancel_delayed_work(&ts->reset_work);

	if (sec_input_need_ic_off(ts->plat_data))
		ts->plat_data->stop_device(ts);
	else
		ts->plat_data->lpmode(ts, TO_LOWPOWER_MODE);

	return 0;
}

int synaptics_ts_stop_device(void *data)
{
	struct synaptics_ts_data *ts = (struct synaptics_ts_data *)data;
	struct irq_desc *desc = irq_to_desc(ts->irq);

	input_info(true, ts->dev, "%s\n", __func__);

	cancel_delayed_work_sync(&ts->work_check_mode);
	if (ts->mode)
		memset(ts->mode, 0x00, SYNAPTICS_TS_MODE_STATE_SIZE);

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	if (ts->panel_attached == SYNAPTICS_PANEL_DETACHED) {
		input_err(true, ts->dev, "%s: panel detached(%d) skip!\n", __func__, ts->panel_attached);
		return 0;
	}
#endif

	if (ts->sec.fac_dev)
		get_lp_dump_show(ts->sec.fac_dev, NULL, NULL);

	mutex_lock(&ts->device_mutex);

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: already power off\n", __func__);
		goto out;
	}

	sec_input_irq_disable(ts->plat_data);

	while (desc->wake_depth > 0)
		disable_irq_wake(ts->irq);

	atomic_set(&ts->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);

	synaptics_ts_locked_release_all_finger(ts);

	ts->plat_data->power(ts->dev, false);
	ts->plat_data->pinctrl_configure(ts->dev, false);

out:
	mutex_unlock(&ts->device_mutex);
	return 0;
}

int synaptics_ts_start_device(void *data)
{
	struct synaptics_ts_data *ts = (struct synaptics_ts_data *)data;
	int ret = 0;

	input_info(true, ts->dev, "%s\n", __func__);

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	if (ts->panel_attached == SYNAPTICS_PANEL_DETACHED) {
		input_err(true, ts->dev, "%s: panel detached(%d) skip!\n", __func__, ts->panel_attached);
		return 0;
	}
#endif

	ts->plat_data->pinctrl_configure(ts->dev, true);

	mutex_lock(&ts->device_mutex);

	if (atomic_read(&ts->plat_data->power_state) == SEC_INPUT_STATE_POWER_ON) {
		input_err(true, ts->dev, "%s: already power on\n", __func__);
		goto out;
	}

	synaptics_ts_locked_release_all_finger(ts);

	ts->plat_data->power(ts->dev, true);

	sec_delay(ts->power_on_delay);

	atomic_set(&ts->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
	atomic_set(&ts->plat_data->touch_noise_status, 0);

	ret = synaptics_ts_detect_protocol(ts);
	if (ret < 0) {
		input_fail_hist(true, ts->dev, "%s: fail to detect protocol, ret=%d\n", __func__, ret);
		goto out;
	}

	if (atomic_read(&ts->plat_data->power_state) != SEC_INPUT_STATE_POWER_OFF)
		sec_input_irq_enable(ts->plat_data);

	ts->plat_data->init(ts);
out:
	if (IS_BOOTLOADER_MODE(ts->dev_mode)) {
		ts->plat_data->hw_param.checksum_result |= SYNAPTICS_TS_CHK_BLMODE_ON_START;
		input_err(true, ts->dev, "%s: device is in bootloader mode. count:%d\n",
				__func__, ts->plat_data->hw_param.checksum_result);
	}
	mutex_unlock(&ts->device_mutex);
	return ret;
}

static int synaptics_ts_hw_init(struct synaptics_ts_data *ts)
{
	int ret = 0;

	ret = synaptics_ts_allocate_device(ts);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to allocate TouchCom device handle\n", __func__);
		goto err_allocate_cdev;
	}

	ts->plat_data->pinctrl_configure(ts->dev, true);

	ts->plat_data->power(ts->dev, true);
	if (gpio_is_valid(ts->plat_data->gpio_spi_cs))
		gpio_direction_output(ts->plat_data->gpio_spi_cs, 1);

	if (!ts->plat_data->regulator_boot_on)
		sec_delay(ts->power_on_delay);

	atomic_set(&ts->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);

	ret = synaptics_ts_setup(ts);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: failed to synaptics_ts_setup\n", __func__);
		goto err_hw_init;
	}

	if (IS_BOOTLOADER_MODE(ts->dev_mode)) {
		ts->plat_data->hw_param.checksum_result |= SYNAPTICS_TS_CHK_BLMODE_ON_PROBE;
		input_fail_hist(true, ts->dev, "%s: device is in bootloader mode\n", __func__);
	}

	input_info(true, ts->dev, "%s: request_irq = %d (gpio:%d)\n",
			__func__, ts->irq, ts->plat_data->irq_gpio);
	ret = devm_request_threaded_irq(ts->dev, ts->irq, NULL, synaptics_ts_irq_thread,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT, dev_driver_string(ts->dev), ts);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Unable to request threaded irq\n", __func__);
		return ret;
	}

	__pm_stay_awake(ts->plat_data->sec_ws);
	ret = synaptics_ts_fw_update_on_probe(ts);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: failed to synaptics_ts_fw_update_on_probe\n", __func__);
		__pm_relax(ts->plat_data->sec_ws);
		goto err_hw_init;
	}
	__pm_relax(ts->plat_data->sec_ws);

	if (ts->dev_mode != SYNAPTICS_TS_MODE_APPLICATION_FIRMWARE) {
		input_err(true, ts->dev, "%s: Not app mode mode: 0x%02x\n", __func__,
			ts->dev_mode);
		goto err_hw_init;
	}

	if (ts->rows && ts->cols) {
		ts->pFrame = devm_kzalloc(ts->dev, ts->rows * ts->cols * sizeof(int), GFP_KERNEL);
		if (!ts->pFrame)
			return -ENOMEM;

//		slsi_ts_init_proc(ts);
	} else {
		input_err(true, ts->dev, "%s: fail to alloc pFrame nTX:%d, nRX:%d\n",
			__func__, ts->rows, ts->cols);
		return -ENOMEM;
	}

err_hw_init:
err_allocate_cdev:
	return ret;

}

static void synaptics_ts_parse_dt(struct device *dev, struct synaptics_ts_data *ts)
{
	struct device_node *np = dev->of_node;
	int temp[2];

	if (!np) {
		input_err(true, dev, "%s: of_node is not exist\n", __func__);
		return;
	}

	ts->support_immediate_cmd = of_property_read_bool(np, "synaptics,support_immediate_cmd");
	ts->support_miscal_wet = of_property_read_bool(np, "synaptics,support_miscal_wet");

	/* specific tcm drv/dev name is only need when device has more than 2 synaptics device */
	if (of_property_read_string(np, "synaptics,tcm_drv_name", &ts->tcm_drv_name))
		ts->tcm_drv_name = SYNAPTICS_TCM_DRV_DEFAULT;
	if (of_property_read_string(np, "synaptics,tcm_dev_name", &ts->tcm_dev_name))
		ts->tcm_dev_name = SYNAPTICS_TCM_DEV_DEFAULT;

	if (of_property_read_u32_array(np, "synaptics,fw_delay", temp, 2)) {
		/*
		 * fw erase delay need to be
		 *	800ms : when it use S3908 & S3907 IC
		 *	2000ms : when it use S3916A IC
		 * fw write block delay need to be
		 *	50ms : when it use I2C transfer
		 *	20ms : when it use SPI transfer
		 *
		 * default value(800ms/50ms) meets for S3908 & S3907 I2C driver
		 */
		ts->fw_erase_delay = ERASE_DELAY;
		ts->fw_write_block_delay = WRITE_FLASH_DELAY;
	} else {
		ts->fw_erase_delay = temp[0];
		ts->fw_write_block_delay = temp[1];
	}

	if (of_property_read_u32(np, "synaptics,power_on_delay", &ts->power_on_delay))
		ts->power_on_delay = TOUCH_POWER_ON_DWORK_TIME;

	input_info(true, dev, "%s: support_immediate_cmd:%d, support_miscal_wet:%d, tcm_name:%s/%s,"
			" fw_erase_delay:%d, fw_write_block_delay:%d, power_on_delay:%d\n",
			__func__, ts->support_immediate_cmd, ts->support_miscal_wet,
			ts->tcm_drv_name, ts->tcm_dev_name,
			ts->fw_erase_delay, ts->fw_write_block_delay, ts->power_on_delay);
}

int synaptics_ts_init(struct synaptics_ts_data *ts)
{
	int ret = 0;

	if (!ts)
		return -EINVAL;

	input_info(true, ts->dev, "%s\n", __func__);

	synaptics_ts_parse_dt(ts->dev, ts);

	ts->mode = devm_kzalloc(ts->dev, SYNAPTICS_TS_MODE_STATE_SIZE, GFP_KERNEL);
	if (!ts->mode)
		return -ENOMEM;

	sec_input_multi_device_create(ts->dev);
	ts->multi_dev = ts->plat_data->multi_dev;

	if (GET_DEV_COUNT(ts->multi_dev) != MULTI_DEV_SUB)
		ptsp = ts->dev;

	ts->synaptics_ts_read_sponge = synaptics_ts_read_from_sponge;
	ts->synaptics_ts_write_sponge = synaptics_ts_write_to_sponge;

	ts->plat_data->dev =  ts->dev;
	ts->plat_data->irq = ts->irq;
	ts->plat_data->probe = synaptics_ts_probe;
	ts->plat_data->pinctrl_configure = sec_input_pinctrl_configure;
	ts->plat_data->power = sec_input_power;
	ts->plat_data->start_device = synaptics_ts_start_device;
	ts->plat_data->stop_device = synaptics_ts_stop_device;
	ts->plat_data->init = synaptics_ts_reinit;
	ts->plat_data->lpmode = synaptics_ts_set_lowpowermode;
	ts->plat_data->set_grip_data = synaptics_set_grip_data_to_ic;
	if (!ts->plat_data->not_support_temp_noti)
		ts->plat_data->set_temperature = synaptics_ts_set_temperature;
	if (ts->plat_data->support_vbus_notifier)
		ts->plat_data->set_charger_mode = synaptics_ts_set_charger_mode;

#ifdef TCLM_CONCEPT
	sec_tclm_initialize(ts->tdata);
	ts->tdata->dev = ts->dev;
	ts->tdata->tclm_read = synaptics_ts_tclm_read;
	ts->tdata->tclm_write = synaptics_ts_tclm_write;
	ts->tdata->tclm_execute_force_calibration = synaptics_ts_tclm_execute_force_calibration;
	ts->tdata->tclm_parse_dt = sec_tclm_parse_dt;
#endif

	INIT_DELAYED_WORK(&ts->reset_work, synaptics_ts_reset_work);
	INIT_DELAYED_WORK(&ts->work_read_info, synaptics_ts_read_info_work);
	INIT_DELAYED_WORK(&ts->work_print_info, synaptics_ts_print_info_work);
	INIT_DELAYED_WORK(&ts->work_notify_pen_hover, synaptics_ts_notify_pen_hover_work);
	INIT_DELAYED_WORK(&ts->work_read_functions, synaptics_ts_get_touch_function);
	INIT_DELAYED_WORK(&ts->set_temperature_work, synaptics_ts_external_func_work);
	INIT_DELAYED_WORK(&ts->work_check_mode, synatpics_ts_check_mode_work);
	mutex_init(&ts->device_mutex);
	mutex_init(&ts->eventlock);
	mutex_init(&ts->sponge_mutex);
	mutex_init(&ts->fn_mutex);

	init_completion(&ts->plat_data->resume_done);
	complete_all(&ts->plat_data->resume_done);

	ret = sec_input_device_register(ts->dev, ts);
	if (ret) {
		input_err(true, ts->dev, "failed to register input device, %d\n", ret);
		goto err_register_input_device;
	}

	mutex_init(&ts->plat_data->enable_mutex);
	ts->plat_data->enable = synaptics_ts_enable;
	ts->plat_data->disable = synaptics_ts_disable;

	if (IS_FOLD_DEV(ts->multi_dev)) {
		char dev_name[16];

		snprintf(dev_name, sizeof(dev_name), "TSP-%s: ", ts->multi_dev->name);
		ts->plat_data->sec_ws = wakeup_source_register(NULL, dev_name);
	} else {
		ts->plat_data->sec_ws = wakeup_source_register(NULL, "TSP");
	}

#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	/* for vendor */
	/* create the device file and register to char device classes */
	ret = syna_cdev_create_sysfs(ts);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to create the device sysfs\n");
		goto error_vendor_data;
	}
#endif

#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
	if (sysfs_create_group(&ts->plat_data->input_dev->dev.kobj, &secure_attr_group) < 0)
		input_err(true, ts->dev, "%s: do not make secure group\n", __func__);
	else
		secure_touch_init(ts);

#if IS_ENABLED(CONFIG_INPUT_SEC_TRUSTED_TOUCH)
	ret = sec_trusted_touch_init(ts->dev);
	if (ret < 0)
		input_err(true, ts->dev, "%s: Failed to init trusted touch\n", __func__);
	if (ts->plat_data->pvm) {
		ts->plat_data->pvm->pre_func = synaptics_trusted_touch_pre_func;
		ts->plat_data->pvm->post_func = synaptics_trusted_touch_post_func;
	}
#endif

	sec_secure_touch_register(ts, ts->dev, ts->plat_data->ss_touch_num, &ts->plat_data->input_dev->dev.kobj);
#endif
#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
	sec_input_dumpkey_register(GET_DEV_COUNT(ts->multi_dev), synaptics_ts_dump_tsp_log, ts->dev);
	INIT_DELAYED_WORK(&ts->check_rawdata, synaptics_ts_check_rawdata);
#endif

	input_info(true, ts->dev, "%s: init resource\n", __func__);

	return 0;

#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
error_vendor_data:
	wakeup_source_unregister(ts->plat_data->sec_ws);
#endif
	ts->plat_data->enable = NULL;
	ts->plat_data->disable = NULL;
err_register_input_device:
	return ret;
}

static void synaptics_ts_cancel_work_sync(struct synaptics_ts_data *ts)
{
	sec_input_multi_device_remove(ts->multi_dev);

	cancel_delayed_work_sync(&ts->work_check_mode);
	cancel_delayed_work_sync(&ts->work_read_info);
	cancel_delayed_work_sync(&ts->work_print_info);
	cancel_delayed_work_sync(&ts->work_read_functions);
	cancel_delayed_work_sync(&ts->reset_work);
	cancel_delayed_work_sync(&ts->set_temperature_work);
	cancel_delayed_work_sync(&ts->work_notify_pen_hover);
	flush_delayed_work(&ts->reset_work);
#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
	cancel_delayed_work_sync(&ts->check_rawdata);
#endif
}

static void synaptics_ts_release(struct synaptics_ts_data *ts)
{
	input_info(true, ts->dev, "%s\n", __func__);

	ts->plat_data->enable = NULL;
	ts->plat_data->disable = NULL;

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	panel_notifier_unregister(&ts->lcd_nb);
#endif

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	sec_input_unregister_notify(&ts->synaptics_input_nb);
#endif

#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
	sec_secure_touch_unregister(ts->plat_data->ss_touch_num);
#endif
	sec_input_multi_device_remove(ts->multi_dev);
	synaptics_ts_cancel_work_sync(ts);
#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
	sec_input_dumpkey_unregister(GET_DEV_COUNT(ts->multi_dev));
#endif
	wakeup_source_unregister(ts->plat_data->sec_ws);
#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	syna_cdev_remove_sysfs(ts);
#endif

	ts->plat_data->lowpower_mode = false;
	ts->probe_done = false;

	ts->plat_data->power(ts->dev, false);
}

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
static int synaptics_notifier_call(struct notifier_block *n, unsigned long event, void *data)
{
	struct synaptics_ts_data *ts = container_of(n, struct synaptics_ts_data, lcd_nb);
	struct panel_notifier_event_data *evtdata = data;

	input_dbg(false, ts->dev, "%s: called! event = %ld,  ev_data->state = %d\n", __func__, event, evtdata->state);

	if (event == PANEL_EVENT_UB_CON_STATE_CHANGED) {
		input_info(false, ts->dev, "%s: event = %ld, ev_data->state = %d\n", __func__, event, evtdata->state);

		if (evtdata->state == PANEL_EVENT_UB_CON_STATE_DISCONNECTED) {
			input_info(true, ts->dev, "%s: UB_CON_DISCONNECTED : synaptics_ts_stop_device\n", __func__);
			synaptics_ts_stop_device(ts);
			ts->panel_attached = SYNAPTICS_PANEL_DETACHED;
		} else if(evtdata->state == PANEL_EVENT_UB_CON_STATE_CONNECTED && ts->panel_attached != SYNAPTICS_PANEL_ATTACHED) {
			input_info(true, ts->dev, "%s: UB_CON_CONNECTED : panel attached!\n", __func__);
			ts->panel_attached = SYNAPTICS_PANEL_ATTACHED;
		}
	}

	return 0;
}
#endif

int synaptics_ts_probe(struct device *dev)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	int ret = 0;

	input_info(true, ts->dev, "%s\n", __func__);

	ret = synaptics_ts_hw_init(ts);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: fail to init hw\n", __func__);
		synaptics_ts_cancel_work_sync(ts);
		sec_input_irq_disable(ts->plat_data);
		synaptics_ts_release(ts);
		return ret;
	}

	synaptics_ts_get_custom_library(ts);
	synaptics_ts_set_custom_library(ts);

	sec_input_register_vbus_notifier(ts->dev);

	atomic_set(&ts->plat_data->enabled, 1);
	ret = synaptics_ts_fn_init(ts);
	if (ret) {
		input_err(true, ts->dev, "%s: fail to init fn\n", __func__);
		atomic_set(&ts->plat_data->enabled, 0);
		synaptics_ts_cancel_work_sync(ts);
		sec_input_irq_disable(ts->plat_data);
		synaptics_ts_release(ts);
		return ret;
	}

	ts->probe_done = true;

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	sec_input_register_notify(&ts->synaptics_input_nb, synaptics_touch_notify_call, 1);
#endif

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	ts->lcd_nb.priority = 1;
	ts->lcd_nb.notifier_call = synaptics_notifier_call;
	ts->panel_attached = SYNAPTICS_PANEL_ATTACHED;
	panel_notifier_register(&ts->lcd_nb);
#endif

	input_err(true, ts->dev, "%s: done\n", __func__);

	input_log_fix();
#if IS_ENABLED(CONFIG_SEC_INPUT_RAWDATA)
	sec_input_rawdata_init(ts->plat_data->dev, ts->sec.fac_dev);
#endif
	if (!atomic_read(&ts->plat_data->shutdown_called))
		schedule_delayed_work(&ts->work_read_info, msecs_to_jiffies(50));

	return 0;
}

int synaptics_ts_remove(struct synaptics_ts_data *ts)
{
	if (!ts->probe_done) {
		input_info(true, ts->dev, "%s don't success probe yet\n", __func__);
		return 0;
	}

	input_info(true, ts->dev, "%s\n", __func__);

	synaptics_ts_cancel_work_sync(ts);
	mutex_lock(&ts->plat_data->enable_mutex);
	atomic_set(&ts->plat_data->shutdown_called, 1);
	mutex_unlock(&ts->plat_data->enable_mutex);

	sec_input_irq_disable(ts->plat_data);

	sec_input_probe_work_remove(ts->plat_data);

	if (!ts->probe_done) {
		input_err(true, ts->dev, "%s: probe is not done yet\n", __func__);
		return 0;
	}

	sec_input_unregister_vbus_notifier(ts->dev);

	synaptics_ts_release(ts);
	synaptics_ts_fn_remove(ts);
	synaptics_ts_remove_device(ts);

	return 0;
}

void synaptics_ts_shutdown(struct synaptics_ts_data *ts)
{
	input_info(true, ts->dev, "%s\n", __func__);

	synaptics_ts_remove(ts);
}


#if IS_ENABLED(CONFIG_PM)
int synaptics_ts_pm_suspend(struct device *dev)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);

	reinit_completion(&ts->plat_data->resume_done);

	return 0;
}

int synaptics_ts_pm_resume(struct device *dev)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);

	complete_all(&ts->plat_data->resume_done);

	return 0;
}
#endif
