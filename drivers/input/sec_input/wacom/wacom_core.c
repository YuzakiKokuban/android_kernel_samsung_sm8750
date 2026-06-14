#include "wacom_dev.h"
#include <linux/input/spu-sign-verify.h>

// need for ble charge
struct wacom_data *g_wacom;

struct notifier_block *g_nb_wac_camera;
EXPORT_SYMBOL(g_nb_wac_camera);

void wacom_release(struct input_dev *input_dev)
{
	input_report_abs(input_dev, ABS_X, 0);
	input_report_abs(input_dev, ABS_Y, 0);
	input_report_abs(input_dev, ABS_PRESSURE, 0);
	input_report_abs(input_dev, ABS_DISTANCE, 0);

	input_report_key(input_dev, BTN_STYLUS, 0);
	input_report_key(input_dev, BTN_TOUCH, 0);
	input_report_key(input_dev, BTN_TOOL_RUBBER, 0);
	input_report_key(input_dev, BTN_TOOL_PEN, 0);

	input_sync(input_dev);
}

void wacom_forced_release(struct wacom_data *wacom)
{
#ifdef SUPPORT_STANDBY_MODE
	struct timespec64 current_time;
#endif

#if WACOM_PRODUCT_SHIP
	if (wacom->pen_pressed) {
		input_info(true, wacom->dev, "%s : [R] dd:%d,%d mc:%d & [HO] dd:%d,%d\n",
				__func__, wacom->x - wacom->pressed_x, wacom->y - wacom->pressed_y,
				wacom->mcount, wacom->x - wacom->hi_x, wacom->y - wacom->hi_y);
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_HOVER_OUT, NULL);
#endif
	} else  if (wacom->pen_prox) {
		input_info(true, wacom->dev, "%s : [HO] dd:%d,%d mc:%d\n",
				__func__, wacom->x - wacom->hi_x, wacom->y - wacom->hi_y, wacom->mcount);
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_HOVER_OUT, NULL);
#endif
	} else {
		input_info(true, wacom->dev, "%s : pen_prox(%d), pen_pressed(%d)\n",
				__func__, wacom->pen_prox, wacom->pen_pressed);
	}
#else
	if (wacom->pen_pressed) {
		input_info(true, wacom->dev, "%s : [R] lx:%d ly:%d dd:%d,%d mc:%d & [HO] dd:%d,%d\n",
				__func__, wacom->x, wacom->y,
				wacom->x - wacom->pressed_x, wacom->y - wacom->pressed_y,
				wacom->mcount, wacom->x - wacom->hi_x, wacom->y - wacom->hi_y);
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_HOVER_OUT, NULL);
#endif
	} else  if (wacom->pen_prox) {
		input_info(true, wacom->dev, "%s : [HO] lx:%d ly:%d dd:%d,%d mc:%d\n",
				__func__, wacom->x, wacom->y,
				wacom->x - wacom->hi_x, wacom->y - wacom->hi_y, wacom->mcount);
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_HOVER_OUT, NULL);
#endif
	} else {
		input_info(true, wacom->dev, "%s : pen_prox(%d), pen_pressed(%d)\n",
				__func__, wacom->pen_prox, wacom->pen_pressed);
	}
#endif

	wacom_release(wacom->input_dev);

#ifdef SUPPORT_STANDBY_MODE
	if (wacom->pen_pressed || wacom->pen_prox) {
		ktime_get_real_ts64(&current_time);
		wacom->activate_time = current_time.tv_sec;
		wacom->standby_state = WACOM_ORIGINAL_SCAN_RATE;

		input_info(true, wacom->dev, "%s: set activate time(%lld)\n",
					__func__, wacom->activate_time);
	}
#endif

	wacom->hi_x = 0;
	wacom->hi_y = 0;
	wacom->pressed_x = 0;
	wacom->pressed_y = 0;

	wacom->pen_prox = 0;
	wacom->pen_pressed = 0;
	wacom->side_pressed = 0;
	wacom->mcount = 0;
}

int wacom_start_stop_cmd(struct wacom_data *wacom, int mode)
{
	int retry;
	int ret = 0;
	char buff;

	input_info(true, wacom->dev, "%s: mode (%d)\n", __func__, mode);

reset_start:
	if (mode == WACOM_STOP_CMD) {
		buff = COM_SAMPLERATE_STOP;
	} else if (mode == WACOM_START_CMD) {
		buff = COM_SAMPLERATE_START;
	} else if (mode == WACOM_STOP_AND_START_CMD) {
		buff = COM_SAMPLERATE_STOP;
		wacom->samplerate_state = WACOM_STOP_CMD;
	} else {
		input_info(true, wacom->dev, "%s: abnormal mode (%d)\n", __func__, mode);
		return ret;
	}

	retry = WACOM_CMD_RETRY;
	do {
		ret = wacom_send(wacom, &buff, 1);
		sec_delay(50);
		if (ret < 0)
			input_err(true, wacom->dev,
					"%s: failed to send 0x%02X(%d)\n", __func__, buff, retry);
		else
			break;

	} while (--retry);

	if (mode == WACOM_STOP_AND_START_CMD) {
		mode = WACOM_START_CMD;
		goto reset_start;
	}

	if (ret == 1)
		wacom->samplerate_state = mode;

	return ret;
}

int wacom_checksum(struct wacom_data *wacom)
{
	int ret = 0, retry = 10;
	int i = 0;
	u8 buf[5] = { 0, };

	while (retry--) {
		buf[0] = COM_CHECKSUM;
		ret = wacom_send(wacom, &buf[0], 1);
		if (ret < 0) {
			input_err(true, wacom->dev, "i2c fail, retry, %d\n", __LINE__);
			continue;
		}

		sec_delay(200);

		ret = wacom_recv(wacom, buf, 5);
		if (ret < 0) {
			input_err(true, wacom->dev, "i2c fail, retry, %d\n", __LINE__);
			continue;
		}

		if (buf[0] == 0x1F)
			break;

		input_info(true, wacom->dev, "buf[0]: 0x%x, checksum retry\n", buf[0]);
	}

	if (ret >= 0) {
		input_info(true, wacom->dev, "received checksum %x, %x, %x, %x, %x\n",
				buf[0],	buf[1], buf[2], buf[3], buf[4]);
	}

	for (i = 0; i < 5; ++i) {
		if (buf[i] != wacom->fw_chksum[i]) {
			input_info(true, wacom->dev, "checksum fail %dth %x %x\n",
					i, buf[i], wacom->fw_chksum[i]);
			break;
		}
	}

	wacom->checksum_result = (i == 5);

	return wacom->checksum_result;
}

int wacom_query(struct wacom_data *wacom)
{
	u8 data[COM_QUERY_BUFFER] = { 0, };
	u8 *query = data + COM_QUERY_POS;
	int read_size = COM_QUERY_BUFFER;
	int ret;
	int i;

	for (i = 0; i < RETRY_COUNT; i++) {
		ret = wacom_recv(wacom, data, read_size);
		if (ret < 0) {
			input_err(true, wacom->dev, "%s: failed to recv\n", __func__);
			continue;
		}

		input_info(true, wacom->dev, "%s: %dth ret of wacom query=%d\n", __func__, i,	ret);

		if (read_size != ret) {
			input_err(true, wacom->dev, "%s: read size error %d of %d\n", __func__, ret, read_size);
			continue;
		}

		input_info(true, wacom->dev,
				"(retry:%d) %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X\n",
				i, query[0], query[1], query[2], query[3], query[4], query[5],
				query[6], query[7],	query[8], query[9], query[10], query[11],
				query[12], query[13], query[14], query[15]);

		if (query[EPEN_REG_HEADER] == 0x0F) {
			wacom->plat_data->img_version_of_ic[0] = query[EPEN_REG_MPUVER];
			wacom->plat_data->img_version_of_ic[1] = query[EPEN_REG_PROJ_ID];
			wacom->plat_data->img_version_of_ic[2] = query[EPEN_REG_FWVER1];
			wacom->plat_data->img_version_of_ic[3] = query[EPEN_REG_FWVER2];
			break;
		}
	}

	input_info(true, wacom->dev,
			"%X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X\n",
			query[0], query[1], query[2], query[3], query[4], query[5],
			query[6], query[7],  query[8], query[9], query[10], query[11],
			query[12], query[13], query[14], query[15]);

	if (ret < 0) {
		input_err(true, wacom->dev, "%s: failed to read query\n", __func__);
		for (i = 0; i < 4; i++)
			wacom->plat_data->img_version_of_ic[i] = 0;

		wacom->query_status = false;
		return ret;
	}

	wacom->query_status = true;

	if (wacom->use_dt_coord == false) {
		wacom->plat_data->max_x = ((u16)query[EPEN_REG_X1] << 8) + query[EPEN_REG_X2];
		wacom->plat_data->max_y = ((u16)query[EPEN_REG_Y1] << 8) + query[EPEN_REG_Y2];
	}

	wacom->max_pressure = ((u16)query[EPEN_REG_PRESSURE1] << 8) + query[EPEN_REG_PRESSURE2];
	wacom->max_x_tilt = query[EPEN_REG_TILT_X];
	wacom->max_y_tilt = query[EPEN_REG_TILT_Y];
	wacom->max_height = query[EPEN_REG_HEIGHT];
	wacom->bl_ver = query[EPEN_REG_BLVER];

	input_info(true, wacom->dev, "use_dt_coord: %d, max_x: %d max_y: %d, max_pressure: %d\n",
			wacom->use_dt_coord, wacom->plat_data->max_x, wacom->plat_data->max_y,
			wacom->max_pressure);
	input_info(true, wacom->dev, "fw_ver_ic=%02X%02X%02X%02X\n",
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3]);
	input_info(true, wacom->dev, "bl: 0x%X, tiltX: %d, tiltY: %d, maxH: %d\n",
			wacom->bl_ver, wacom->max_x_tilt, wacom->max_y_tilt, wacom->max_height);

	return 0;
}

int wacom_set_sense_mode(struct wacom_data *wacom)
{
	int retval;
	char data[4] = { 0, 0, 0, 0 };

	input_info(true, wacom->dev, "%s cmd mod(%d)\n", __func__, wacom->wcharging_mode);

	if (wacom->wcharging_mode == 1)
		data[0] = COM_LOW_SENSE_MODE;
	else if (wacom->wcharging_mode == 3)
		data[0] = COM_LOW_SENSE_MODE2;
	else {
		/* it must be 0 */
		data[0] = COM_NORMAL_SENSE_MODE;
	}

	retval = wacom_send(wacom, &data[0], 1);
	if (retval < 0) {
		input_err(true, wacom->dev, "%s: failed to send wacom i2c mode, %d\n", __func__, retval);
		return retval;
	}

	sec_delay(60);

	retval = wacom_start_stop_cmd(wacom, WACOM_STOP_CMD);
	if (retval < 0) {
		input_err(true, wacom->dev, "%s: failed to set stop cmd, %d\n",
				__func__, retval);
		return retval;
	}

	retval = wacom_start_stop_cmd(wacom, WACOM_START_CMD);
	if (retval < 0) {
		input_err(true, wacom->dev, "%s: failed to set start cmd, %d\n",
				__func__, retval);
		return retval;
	}

	return retval;
}

#ifdef SUPPORT_STANDBY_MODE
static void wacom_standby_mode_handler(struct wacom_data *wacom)
{
	struct timespec64 current_time;
	long long diff_time;
	char cmd;
	int ret = 0;

	ktime_get_real_ts64(&current_time);
	diff_time = current_time.tv_sec - wacom->activate_time;

	input_info(true, wacom->dev,
				"%s: standby_state(%d) activate_time(%lld), diff time(%lld)(%lldh %lldm %llds)\n",
					__func__, wacom->standby_state, wacom->activate_time, diff_time,
					(diff_time / 3600), ((diff_time % 3600) / 60), ((diff_time % 3600) % 60));

	if ((diff_time >= SET_STANDBY_TIME) && (wacom->standby_state == WACOM_ORIGINAL_SCAN_RATE)) {
		/*set standby mode */
		input_info(true, wacom->dev, "%s : set standby mode\n", __func__);
		wacom->activate_time = 0;

		cmd = COM_SET_STANDBY_MODE;
		ret = wacom_send(wacom, &cmd, 1);
		if (ret < 0)
			input_err(true, wacom->dev, "%s: failed to send standby mode %d\n",
						__func__, ret);

	} else if ((diff_time < SET_STANDBY_TIME) && (wacom->standby_state == WACOM_LOW_SCAN_RATE)) {
		/*set activated mode */
		input_info(true, wacom->dev, "%s : set active mode\n", __func__);
		cmd = COM_SET_ACTIVATED_MODE;
		ret = wacom_send(wacom, &cmd, 1);
		if (ret < 0)
			input_err(true, wacom->dev, "%s: failed to send activate mode %d\n", __func__, ret);

	} else {
		input_info(true, wacom->dev, "%s: skip %s mode setting\n", __func__,
					wacom->standby_state == WACOM_LOW_SCAN_RATE ? "stand by" : "normal");
	}
}
#endif

static void wacom_handler_wait_resume_pdct_work(struct work_struct *work)
{
	struct wacom_data *wacom = container_of(work, struct wacom_data, irq_work_pdct);
	struct irq_desc *desc = irq_to_desc(wacom->irq_pdct);
	int ret;

	ret = wait_for_completion_interruptible_timeout(&wacom->plat_data->resume_done,
			msecs_to_jiffies(SEC_TS_WAKE_LOCK_TIME));
	if (ret == 0) {
		input_err(true, wacom->dev, "%s: LPM: pm resume is not handled(pdct)\n", __func__);
		goto out;
	}
	if (ret < 0) {
		input_err(true, wacom->dev, "%s: LPM: -ERESTARTSYS if interrupted(pdct), %d\n", __func__, ret);
		goto out;
	}

	if (desc && desc->action && desc->action->thread_fn) {
		input_info(true, wacom->dev, "%s: run irq_pdct thread\n", __func__);
		desc->action->thread_fn(wacom->irq_pdct, desc->action->dev_id);
	}
out:
	wacom_enable_irq(wacom, true);
}

void wacom_select_survey_mode_without_pdct(struct wacom_data *wacom, bool enable)
{
	if (enable) {
		if (wacom->epen_blocked ||
				(wacom->battery_saving_mode && !(wacom->function_result & EPEN_EVENT_PEN_OUT))) {
			input_info(true, wacom->dev, "%s: %s power off\n", __func__,
					wacom->epen_blocked ? "epen blocked" : "ps on & pen in");

			if (!wacom->reset_is_on_going) {
				wacom_enable_irq(wacom, false);
				atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
				wacom_power(wacom, false);
			}

			wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
			wacom->function_result &= ~EPEN_EVENT_SURVEY;
		} else if (wacom->survey_mode) {
			input_info(true, wacom->dev, "%s: exit aop mode\n", __func__);
			wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_NONE);
#if defined(CONFIG_EPEN_WACOM_W9020)
			wacom->reset_flag = true;
			sec_delay(200);
			input_info(true, wacom->dev, "%s: reset_flag %d\n", __func__, wacom->reset_flag);
#endif
		} else {
			input_info(true, wacom->dev, "%s: power on\n", __func__);
			if (!wacom->reset_is_on_going) {
				wacom_power(wacom, true);
				atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
				sec_delay(100);
			}
#ifdef SUPPORT_STANDBY_MODE
			wacom->standby_state = WACOM_LOW_SCAN_RATE;
			wacom_standby_mode_handler(wacom);
#endif
			if (!wacom->reset_is_on_going)
				wacom_enable_irq(wacom, true);
		}
	} else {
		if (wacom->epen_blocked || (wacom->battery_saving_mode &&
				!(wacom->function_result & EPEN_EVENT_PEN_OUT))) {
			input_info(true, wacom->dev, "%s: %s power off\n",
					__func__, wacom->epen_blocked ? "epen blocked" : "ps on & pen in");
			if (!wacom->reset_is_on_going) {
				wacom_enable_irq(wacom, false);
				atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
				wacom_power(wacom, false);
			}

			wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
			wacom->function_result &= ~EPEN_EVENT_SURVEY;
		} else if (!(wacom->function_set & EPEN_SETMODE_AOP)) {
			input_info(true, wacom->dev, "%s: aop off & garage off. power off\n", __func__);
			if (!wacom->reset_is_on_going) {
				wacom_enable_irq(wacom, false);
				atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
				wacom_power(wacom, false);
			}

			wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
			wacom->function_result &= ~EPEN_EVENT_SURVEY;
		} else {
			/* aop on => (aod : screen off memo = 1:1 or 1:0 or 0:1)
			 * double tab & hover + button event will be occurred,
			 * but some of them will be skipped at reporting by mode
			 */
			input_info(true, wacom->dev, "%s: aop on. aop mode\n", __func__);
			if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF) && !wacom->reset_is_on_going) {
				input_info(true, wacom->dev, "%s: power on\n", __func__);

				wacom_power(wacom, true);
				atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
				sec_delay(100);

				wacom_enable_irq(wacom, true);
			}

			wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_AOP);
		}
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_ON_LP)) {
		input_info(true, wacom->dev, "%s: screen %s, survey mode:%d, result:%d\n",
				__func__, enable ? "on" : "off", wacom->survey_mode,
				wacom->function_result & EPEN_EVENT_SURVEY);

		if ((wacom->function_result & EPEN_EVENT_SURVEY) != wacom->survey_mode) {
			input_err(true, wacom->dev, "%s: survey mode cmd failed\n", __func__);

			wacom_set_survey_mode(wacom, wacom->survey_mode);
		}
	}
}

void wacom_select_survey_mode_with_pdct(struct wacom_data *wacom, bool enable)
{
	if (enable) {
		if (wacom->epen_blocked ||
				(wacom->battery_saving_mode && !(wacom->function_result & EPEN_EVENT_PEN_OUT))) {
			input_info(true, wacom->dev, "%s: %s & garage on. garage only mode\n",
					__func__, wacom->epen_blocked ? "epen blocked" : "ps on & pen in");
			wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_ONLY);
		} else if (wacom->survey_mode) {
			input_info(true, wacom->dev, "%s: exit aop mode\n", __func__);
			wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_NONE);
		} else {
			input_info(true, wacom->dev, "%s: power on\n", __func__);
			wacom_power(wacom, true);
			msleep(100);
#ifdef SUPPORT_STANDBY_MODE
			wacom->standby_state = WACOM_LOW_SCAN_RATE;
			wacom_standby_mode_handler(wacom);
#endif
			wacom_enable_irq(wacom, true);
		}
	} else {
		if (wacom->epen_blocked || (wacom->battery_saving_mode &&
				!(wacom->function_result & EPEN_EVENT_PEN_OUT))) {
			input_info(true, wacom->dev, "%s: %s & garage on. garage only mode\n",
				__func__, wacom->epen_blocked ? "epen blocked" : "ps on & pen in");
			wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_ONLY);
		} else if (!(wacom->function_set & EPEN_SETMODE_AOP)) {
			input_info(true, wacom->dev, "%s: aop off & garage on. garage only mode\n", __func__);
			wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_ONLY);
		} else {
			/* aop on => (aod : screen off memo = 1:1 or 1:0 or 0:1)
			 * double tab & hover + button event will be occurred,
			 * but some of them will be skipped at reporting by mode
			 */
			input_info(true, wacom->dev, "%s: aop on. aop mode\n", __func__);

			if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
				input_info(true, wacom->dev, "%s: power on\n", __func__);

				wacom_power(wacom, true);
				msleep(100);
				atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
				wacom_enable_irq(wacom, true);
			}

			wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_AOP);
		}
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_ON_LP)) {
		input_info(true, wacom->dev, "%s: screen %s, survey mode:%d, result:%d\n",
				__func__, enable ? "on" : "off", wacom->survey_mode,
				wacom->function_result & EPEN_EVENT_SURVEY);

		if ((wacom->function_result & EPEN_EVENT_SURVEY) != wacom->survey_mode) {
			input_err(true, wacom->dev, "%s: survey mode cmd failed\n", __func__);

			wacom_set_survey_mode(wacom, wacom->survey_mode);
		}
	}

}

void wacom_select_survey_mode(struct wacom_data *wacom, bool enable)
{
	mutex_lock(&wacom->mode_lock);

	if (wacom->use_garage)
		wacom_select_survey_mode_with_pdct(wacom, enable);
	else
		wacom_select_survey_mode_without_pdct(wacom, enable);

	mutex_unlock(&wacom->mode_lock);
}

int wacom_set_survey_mode(struct wacom_data *wacom, int mode)
{
	int retval;
	char data[4] = { 0, 0, 0, 0 };

	switch (mode) {
	case EPEN_SURVEY_MODE_NONE:
		data[0] = COM_SURVEY_EXIT;
		break;
	case EPEN_SURVEY_MODE_GARAGE_ONLY:
		if (wacom->use_garage) {
			data[0] = COM_SURVEY_GARAGE_ONLY;
		} else {
			input_err(true, wacom->dev, "%s: garage mode is not supported\n", __func__);
			return -EPERM;
		}
		break;
	case EPEN_SURVEY_MODE_GARAGE_AOP:
		if ((wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON) == EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON)
			data[0] = COM_SURVEY_SYNC_SCAN;
		else
			data[0] = COM_SURVEY_ASYNC_SCAN;
		break;
	default:
		input_err(true, wacom->dev, "%s: wrong param %d\n", __func__, mode);
		return -EINVAL;
	}

	wacom->survey_mode = mode;
	input_info(true, wacom->dev, "%s: ps %s & mode : %d cmd(0x%2X)\n", __func__,
			wacom->battery_saving_mode ? "on" : "off", mode, data[0]);

	retval = wacom_send(wacom, &data[0], 1);
	if (retval < 0) {
		input_err(true, wacom->dev, "%s: failed to send data(%02x %d)\n", __func__, data[0], retval);
		wacom->reset_flag = true;

		return retval;
	}

	wacom->check_survey_mode = mode;

	if (mode) {
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_LPM);
		sec_delay(300);
	} else {
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
	}

	wacom->reset_flag = false;
	wacom->function_result &= ~EPEN_EVENT_SURVEY;
	wacom->function_result |= mode;

	return 0;
}

void wacom_pdct_survey_mode(struct wacom_data *wacom)
{
	if (wacom->epen_blocked ||
				(wacom->battery_saving_mode && !(wacom->function_result & EPEN_EVENT_PEN_OUT))) {
		input_info(true, wacom->dev,
				"%s: %s & garage on. garage only mode\n", __func__,
				wacom->epen_blocked ? "epen blocked" : "ps on & pen in");

		mutex_lock(&wacom->mode_lock);
		wacom_set_survey_mode(wacom,
				EPEN_SURVEY_MODE_GARAGE_ONLY);
		mutex_unlock(&wacom->mode_lock);
	} else if (atomic_read(&wacom->screen_on) && wacom->survey_mode) {
		input_info(true, wacom->dev,
				"%s: ps %s & pen %s & lcd on. normal mode\n",
				__func__, wacom->battery_saving_mode ? "on" : "off",
				(wacom->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in");

		mutex_lock(&wacom->mode_lock);
		wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_NONE);
		mutex_unlock(&wacom->mode_lock);
	} else {
		input_info(true, wacom->dev,
				"%s: ps %s & pen %s & lcd %s. keep current mode(%s)\n",
				__func__, wacom->battery_saving_mode ? "on" : "off",
				(wacom->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
				atomic_read(&wacom->screen_on) ? "on" : "off",
				wacom->function_result & EPEN_EVENT_SURVEY ? "survey" : "normal");
	}
}

void forced_release_fullscan(struct wacom_data *wacom)
{
	input_info(true, wacom->dev, "%s: full scan OUT\n", __func__);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_RELEASE, NULL);
	wacom->is_tsp_block = false;
#endif
}

int wacom_power(struct wacom_data *wacom, bool on)
{
	return sec_input_power(wacom->dev, on);
}

void wacom_reset_hw(struct wacom_data *wacom)
{
	atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
	wacom_power(wacom, false);
	/* recommended delay in spec */
	sec_delay(100);
	wacom_power(wacom, true);
	atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);

	sec_delay(100);
}

void wacom_compulsory_flash_mode(struct wacom_data *wacom, bool enable)
{
	gpio_direction_output(wacom->fwe_gpio, enable);
	input_info(true, wacom->dev, "%s : enable(%d) fwe(%d)\n",
				__func__, enable, gpio_get_value(wacom->fwe_gpio));
}

void wacom_enable_irq(struct wacom_data *wacom, bool enable)
{
	struct irq_desc *desc = irq_to_desc(wacom->irq);

	if (desc == NULL) {
		input_err(true, wacom->dev, "%s : irq desc is NULL\n", __func__);
		return;
	}

	mutex_lock(&wacom->irq_lock);
	if (enable) {
		while (desc->depth > 0)
			enable_irq(wacom->irq);
	} else {
		disable_irq(wacom->irq);
	}

	if (gpio_is_valid(wacom->pdct_gpio)) {
		struct irq_desc *desc_pdct = irq_to_desc(wacom->irq_pdct);

		if (enable) {
			while (desc_pdct->depth > 0)
				enable_irq(wacom->irq_pdct);
		} else {
			disable_irq(wacom->irq_pdct);
		}
	}

	if (gpio_is_valid(wacom->esd_detect_gpio)) {
		struct irq_desc *desc_esd = irq_to_desc(wacom->irq_esd_detect);

		if (enable) {
			while (desc_esd->depth > 0)
				enable_irq(wacom->irq_esd_detect);
		} else {
			disable_irq(wacom->irq_esd_detect);
		}
	}
	mutex_unlock(&wacom->irq_lock);
}

static void wacom_enable_irq_wake(struct wacom_data *wacom, bool enable)
{
	struct irq_desc *desc = irq_to_desc(wacom->irq);

	if (enable) {
		while (desc->wake_depth < 1)
			enable_irq_wake(wacom->irq);
	} else {
		while (desc->wake_depth > 0)
			disable_irq_wake(wacom->irq);
	}

	if (gpio_is_valid(wacom->pdct_gpio)) {
		struct irq_desc *desc_pdct = irq_to_desc(wacom->irq_pdct);

		if (enable) {
			while (desc_pdct->wake_depth < 1)
				enable_irq_wake(wacom->irq_pdct);
		} else {
			while (desc_pdct->wake_depth > 0)
				disable_irq_wake(wacom->irq_pdct);
		}
	}

	if (gpio_is_valid(wacom->esd_detect_gpio)) {
		struct irq_desc *desc_esd = irq_to_desc(wacom->irq_esd_detect);

		if (enable) {
			while (desc_esd->wake_depth < 1)
				enable_irq_wake(wacom->irq_esd_detect);
		} else {
			while (desc_esd->wake_depth > 0)
				disable_irq_wake(wacom->irq_esd_detect);
		}
	}
}

void wacom_wakeup_sequence(struct wacom_data *wacom)
{
	int retry = 1;
	int ret;

	mutex_lock(&wacom->lock);
#if WACOM_SEC_FACTORY
	if (wacom->fac_garage_mode)
		input_info(true, wacom->dev, "%s: garage mode\n", __func__);
#endif
	if (!wacom->reset_flag && wacom->ble_disable_flag) {
		input_info(true, wacom->dev,
				"%s: ble is diabled & send enable cmd!\n", __func__);

		mutex_lock(&wacom->ble_charge_mode_lock);
		wacom_ble_charge_mode(wacom, EPEN_BLE_C_ENABLE);
		wacom->ble_disable_flag = false;
		mutex_unlock(&wacom->ble_charge_mode_lock);
	}

reset:
	if (wacom->reset_flag) {
		input_info(true, wacom->dev, "%s: IC reset start\n", __func__);

		wacom->abnormal_reset_count++;
		wacom->reset_flag = false;
		wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
		wacom->function_result &= ~EPEN_EVENT_SURVEY;

		wacom_enable_irq(wacom, false);

		wacom_reset_hw(wacom);

		if (gpio_is_valid(wacom->pdct_gpio)) {
			wacom->pen_pdct = gpio_get_value(wacom->pdct_gpio);

			input_info(true, wacom->dev, "%s: IC reset end, pdct(%d)\n", __func__, wacom->pen_pdct);

			if (wacom->use_garage) {
				if (wacom->pen_pdct)
					wacom->function_result &= ~EPEN_EVENT_PEN_OUT;
				else
					wacom->function_result |= EPEN_EVENT_PEN_OUT;
			}
		}

		input_info(true, wacom->dev, "%s: IC reset end\n", __func__);

		if (wacom->support_hall_notifier)
			wacom_swap_compensation(wacom, wacom->table_type);

		wacom_enable_irq(wacom, true);
	}

	wacom_select_survey_mode(wacom, true);

	if (wacom->reset_flag && retry--)
		goto reset;

	if (wacom->wcharging_mode)
		wacom_set_sense_mode(wacom);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	if (wacom->tsp_scan_mode < 0) {
		wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_RELEASE, NULL);
		wacom->is_tsp_block = false;
	}
#endif

	if (!gpio_get_value(wacom->plat_data->irq_gpio)) {
		u8 data[COM_COORD_NUM + 1] = { 0, };

		input_info(true, wacom->dev, "%s: irq was enabled\n", __func__);

		ret = wacom_recv(wacom, data, COM_COORD_NUM + 1);
		if (ret < 0)
			input_err(true, wacom->dev, "%s: failed to receive\n", __func__);

		input_info(true, wacom->dev,
				"%x %x %x %x %x, %x %x %x %x %x, %x %x %x\n",
				data[0], data[1], data[2], data[3], data[4], data[5],
				data[6], data[7], data[8], data[9], data[10],
				data[11], data[12]);
	}

	if (!wacom->samplerate_state) {
		char cmd = COM_SAMPLERATE_START;

		input_info(true, wacom->dev, "%s: samplerate state is %d, need to recovery\n",
				__func__, wacom->samplerate_state);

		ret = wacom_send(wacom, &cmd, 1);
		if (ret < 0)
			input_err(true, wacom->dev, "%s: failed to sned start cmd %d\n", __func__, ret);
		else
			wacom->samplerate_state = true;
	}

	if (gpio_is_valid(wacom->pdct_gpio)) {
		input_info(true, wacom->dev,
				"%s: i(%d) p(%d) f_set(0x%x) f_ret(0x%x) samplerate(%d)\n",
				__func__, gpio_get_value(wacom->plat_data->irq_gpio),
				gpio_get_value(wacom->pdct_gpio), wacom->function_set,
				wacom->function_result, wacom->samplerate_state);
	} else {
		input_info(true, wacom->dev,
				"%s: i(%d) f_set(0x%x) f_ret(0x%x) samplerate(%d)\n",
				__func__, gpio_get_value(wacom->plat_data->irq_gpio), wacom->function_set,
				wacom->function_result, wacom->samplerate_state);
	}

	wacom_enable_irq_wake(wacom, false);

	if (wacom->pdct_lock_fail) {
		wacom->pdct_lock_fail = false;
		wacom_pdct_survey_mode(wacom);
	}

	mutex_unlock(&wacom->lock);

	input_info(true, wacom->dev, "%s: end\n", __func__);
}

void wacom_sleep_sequence(struct wacom_data *wacom)
{
	int retry = 1;

	mutex_lock(&wacom->lock);
#if WACOM_SEC_FACTORY
	if (wacom->fac_garage_mode)
		input_info(true, wacom->dev, "%s: garage mode\n", __func__);

#endif

reset:
	if (wacom->reset_flag) {
		input_info(true, wacom->dev, "%s: IC reset start\n", __func__);

		wacom->abnormal_reset_count++;
		wacom->reset_flag = false;
		wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
		wacom->function_result &= ~EPEN_EVENT_SURVEY;

		wacom_enable_irq(wacom, false);

		wacom_reset_hw(wacom);

		input_info(true, wacom->dev, "%s : IC reset end\n", __func__);

		wacom_enable_irq(wacom, true);
	}

	wacom_select_survey_mode(wacom, false);

	/* release pen, if it is pressed */
	if (wacom->pen_pressed || wacom->side_pressed || wacom->pen_prox)
		wacom_forced_release(wacom);

	forced_release_fullscan(wacom);

	if (wacom->reset_flag && retry--)
		goto reset;

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_ON_LP))
		wacom_enable_irq_wake(wacom, true);

	if (wacom->pdct_lock_fail) {
		wacom_pdct_survey_mode(wacom);
		wacom->pdct_lock_fail = false;
	}

	mutex_unlock(&wacom->lock);

	input_info(true, wacom->dev, "%s end\n", __func__);
}

static void wacom_reply_handler(struct wacom_data *wacom, char *data)
{
	char pack_sub_id;
#if !WACOM_SEC_FACTORY && WACOM_PRODUCT_SHIP
	int ret;
#endif

	pack_sub_id = data[1];

	switch (pack_sub_id) {
	case SWAP_PACKET:
		input_info(true, wacom->dev, "%s: SWAP_PACKET received()\n", __func__);
		break;
	case ELEC_TEST_PACKET:
		wacom->check_elec++;

		input_info(true, wacom->dev, "%s: ELEC TEST PACKET received(%d)\n", __func__, wacom->check_elec);

#if !WACOM_SEC_FACTORY && WACOM_PRODUCT_SHIP
		ret = wacom_start_stop_cmd(wacom, WACOM_STOP_AND_START_CMD);
		if (ret < 0) {
			input_err(true, wacom->dev, "%s: failed to set stop-start cmd, %d\n",
					__func__, ret);
			return;
		}
#endif
		break;
	default:
		input_info(true, wacom->dev, "%s: unexpected packet %d\n", __func__, pack_sub_id);
		break;
	};
}

static void wacom_cmd_noti_handler(struct wacom_data *wacom, char *data)
{
#ifdef SUPPORT_STANDBY_MODE
	if (data[3] == COM_SURVEY_EXIT || data[3] == COM_SURVEY_SYNC_SCAN) {
		wacom->standby_enable = (data[4] & 0x10) >> 4;
		wacom->standby_state = data[4] & 0x0F;
		input_info(true, wacom->dev, "%s: noti ack packet(0x%X) %s state:%d(en:%d)\n",
					__func__, data[3],
					wacom->standby_state == WACOM_LOW_SCAN_RATE ? "standby" : "normal",
					wacom->standby_state, wacom->standby_enable);
	}
	/* only control standby mode in COM_SURVEY_EXIT */
	if (data[3] == COM_SURVEY_EXIT)
		wacom_standby_mode_handler(wacom);
#endif

#if defined(CONFIG_EPEN_WACOM_W9020)
	wacom->reset_flag = false;
	input_info(true, wacom->dev, "%s: reset_flag %d\n", __func__, wacom->reset_flag);
#endif
}

static void wacom_block_tsp_scan(struct wacom_data *wacom, char *data)
{
	bool tsp;
	u8 wacom_mode, scan_mode;

	wacom_mode = (data[2] & 0xF0) >> 4;
	scan_mode = data[2] & 0x0F;
	tsp = !!(data[5] & 0x80);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	if (tsp && !wacom->is_tsp_block) {
		input_info(true, wacom->dev, "%s: tsp:%d wacom mode:%d scan mode:%d\n",
				__func__, tsp, wacom_mode, scan_mode);
		wacom->is_tsp_block = true;
		wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_REQUEST, NULL);
		wacom->tsp_block_cnt++;
	} else if (!tsp && wacom->is_tsp_block) {
		input_info(true, wacom->dev, "%s: tsp:%d wacom mode:%d scan mode:%d\n",
				__func__, tsp, wacom_mode, scan_mode);
		wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_RELEASE, NULL);
		wacom->is_tsp_block = false;
	}
#endif
}

static void wacom_noti_handler(struct wacom_data *wacom, char *data)
{
	char pack_sub_id;

	pack_sub_id = data[1];

	switch (pack_sub_id) {
	case ERROR_PACKET:
		// have to check it later!
		input_err(true, wacom->dev, "%s: ERROR_PACKET\n", __func__);
		break;
	case TABLE_SWAP_PACKET:
	case EM_NOISE_PACKET:
		break;
	case TSP_STOP_PACKET:
		wacom_block_tsp_scan(wacom, data);
		break;
	case OOK_PACKET:
		if (data[4] & 0x80)
			input_err(true, wacom->dev, "%s: OOK Fail!\n", __func__);
		break;
	case CMD_PACKET:
		wacom_cmd_noti_handler(wacom, data);
		break;
	case GCF_PACKET:
		wacom->ble_charging_state = false;
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_CHARGING_FINISHED, NULL);
#endif
		break;
#ifdef SUPPORT_STANDBY_MODE
	case STANDBY_PACKET:
		//print for debugging
		/* standby_enable : support standby_enable or not
		 * standby state - 0 : original scan rate
		 *				   1 : set standby mode
		 */
		wacom->standby_enable = (data[3] & 0x10) >> 4;
		wacom->standby_state = data[3] & 0x0F;
		input_info(true, wacom->dev, "%s: stand-by noti packet - state:%d(en:%d)\n",
					__func__, wacom->standby_state, wacom->standby_enable);
		break;
#endif
	case ESD_DETECT_PACKET:
		wacom->esd_packet_count++;
		input_err(true, wacom->dev, "%s: DETECT ESD(%d)\n", __func__, wacom->esd_packet_count);
		if (wacom->reset_is_on_going) {
			input_info(true, wacom->dev, "%s: already reset on going:%d\n",
					__func__, wacom->reset_is_on_going);
		} else {
			disable_irq_nosync(wacom->irq);
			disable_irq_nosync(wacom->irq_esd_detect);
			wacom->reset_is_on_going = true;
			cancel_delayed_work_sync(&wacom->esd_reset_work);
			schedule_delayed_work(&wacom->esd_reset_work, msecs_to_jiffies(1));
		}
		break;
	default:
		input_err(true, wacom->dev, "%s: unexpected packet %d\n", __func__, pack_sub_id);
		break;
	};
}

static void wacom_aop_handler(struct wacom_data *wacom, char *data)
{
	bool stylus, prox = false;
	s16 x, y;
	u8 wacom_mode, wakeup_id;

	if (atomic_read(&wacom->screen_on) || !wacom->survey_mode ||
			!(wacom->function_set & EPEN_SETMODE_AOP)) {
		input_info(true, wacom->dev, "%s: unexpected status(%d %d %d)\n",
				__func__, atomic_read(&wacom->screen_on), wacom->survey_mode,
				wacom->function_set & EPEN_SETMODE_AOP);
		return;
	}

	prox = !!(data[0] & 0x10);
	x = ((u16)data[1] << 8) + (u16)data[2];
	y = ((u16)data[3] << 8) + (u16)data[4];
	wacom_mode = (data[6] & 0xF0) >> 4;
	wakeup_id = data[6] & 0x0F;

	stylus = !!(data[0] & 0x20);

	if (data[0] & 0x40)
		wacom->tool = BTN_TOOL_RUBBER;
	else
		wacom->tool = BTN_TOOL_PEN;

	/* [for checking */
	input_info(true, wacom->dev, "wacom mode %d, wakeup id %d\n", wacom_mode, wakeup_id);
	/* for checking] */

	if (stylus && (wakeup_id == HOVER_WAKEUP) &&
			(wacom->function_set & EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO)) {
		input_info(true, wacom->dev, "Hover & Side Button detected\n");

		input_report_key(wacom->input_dev, KEY_WAKEUP_UNLOCK, 1);
		input_sync(wacom->input_dev);

		input_report_key(wacom->input_dev, KEY_WAKEUP_UNLOCK, 0);
		input_sync(wacom->input_dev);

		wacom->survey_pos.id = EPEN_POS_ID_SCREEN_OF_MEMO;
		wacom->survey_pos.x = x;
		wacom->survey_pos.y = y;
	} else if (wakeup_id == DOUBLETAP_WAKEUP) {
		if ((wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON) == EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON) {
			input_info(true, wacom->dev, "Double Tab detected in AOD\n");

			/* make press / release event for AOP double tab gesture */
			input_report_abs(wacom->input_dev, ABS_X, x);
			input_report_abs(wacom->input_dev, ABS_Y, y);
			input_report_abs(wacom->input_dev, ABS_PRESSURE, 1);
			input_report_key(wacom->input_dev, BTN_TOUCH, 1);
			input_report_key(wacom->input_dev, wacom->tool, 1);
			input_sync(wacom->input_dev);

			input_report_abs(wacom->input_dev, ABS_PRESSURE, 0);
			input_report_key(wacom->input_dev, BTN_TOUCH, 0);
			input_report_key(wacom->input_dev, wacom->tool, 0);
			input_sync(wacom->input_dev);
#if WACOM_PRODUCT_SHIP
			input_info(true, wacom->dev, "[P/R] tool:%x\n",
					wacom->tool);
#else
			input_info(true, wacom->dev,
					"[P/R] x:%d y:%d tool:%x\n", x, y, wacom->tool);
#endif
		} else if (wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOT) {
			input_info(true, wacom->dev, "Double Tab detected\n");

			input_report_key(wacom->input_dev, KEY_HOMEPAGE, 1);
			input_sync(wacom->input_dev);
			input_report_key(wacom->input_dev, KEY_HOMEPAGE, 0);
			input_sync(wacom->input_dev);
		}
	} else {
		input_info(true, wacom->dev,
				"unexpected AOP data : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
				data[0], data[1], data[2], data[3], data[4], data[5],
				data[6], data[7], data[8], data[9], data[10], data[11],
				data[12], data[13], data[14], data[15]);
	}
}

#define EPEN_LOCATION_DETECT_SIZE	6
void epen_location_detect(struct wacom_data *wacom, char *loc, int x, int y)
{
	struct sec_ts_plat_data *plat_data = wacom->plat_data;
	int i;
	int max_x = plat_data->max_x;
	int max_y = plat_data->max_y;

	if (plat_data->area_indicator == 0) {
		/* approximately value */
		plat_data->area_indicator = max_y * 4 / 100;
		plat_data->area_navigation = max_y * 6 / 100;
		plat_data->area_edge = max_y * 3 / 100;

		input_raw_info(true, wacom->dev,
				"MAX XY(%d/%d), area_edge %d, area_indicator %d, area_navigation %d\n",
				max_x, max_y, plat_data->area_edge,
				plat_data->area_indicator, plat_data->area_navigation);
	}

	for (i = 0; i < EPEN_LOCATION_DETECT_SIZE; ++i)
		loc[i] = 0;

	if (x < plat_data->area_edge)
		strcat(loc, "E.");
	else if (x < (max_x - plat_data->area_edge))
		strcat(loc, "C.");
	else
		strcat(loc, "e.");

	if (y < plat_data->area_indicator)
		strcat(loc, "S");
	else if (y < (max_y - plat_data->area_navigation))
		strcat(loc, "C");
	else
		strcat(loc, "N");
}

static void wacom_coord_handler(struct wacom_data *wacom, char *data)
{
	bool prox = false;
	bool rdy = false;
	bool stylus;
	char location[EPEN_LOCATION_DETECT_SIZE] = { 0, };
	struct timespec64 current_time;
	struct input_dev *input = wacom->input_dev;

	ktime_get_real_ts64(&current_time);

	if (wacom->debug_flag & WACOM_DEBUG_PRINT_COORDEVENT) {
		input_info(true, wacom->dev,
				"%s : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
				__func__, data[0], data[1], data[2], data[3], data[4], data[5],
				data[6], data[7], data[8], data[9], data[10], data[11],
				data[12], data[13], data[14], data[15]);
	}

	rdy = !!(data[0] & 0x80);
	prox = !!(data[0] & 0x10);

	wacom->x = ((u16)data[1] << 8) + (u16)data[2];
	wacom->y = ((u16)data[3] << 8) + (u16)data[4];
	wacom->pressure = ((u16)(data[5] & 0x0F) << 8) + (u16)data[6];
	wacom->height = (u8)data[7];
	wacom->tilt_x = (s8)data[8];
	wacom->tilt_y = (s8)data[9];

	if (rdy) {
		int max_x = wacom->plat_data->max_x;
		int max_y = wacom->plat_data->max_y;

		/* validation check */
		if (unlikely(wacom->x > max_x || wacom->y > max_y)) {
#if WACOM_PRODUCT_SHIP
			input_info(true, wacom->dev, "%s : Abnormal raw data x & y\n", __func__);
#else
			input_info(true, wacom->dev, "%s : Abnormal raw data x=%d, y=%d\n",
					__func__, wacom->x, wacom->y);
#endif
			return;
		}

		stylus = !!(data[0] & 0x20);
		if (data[0] & 0x40)
			wacom->tool = BTN_TOOL_RUBBER;
		else
			wacom->tool = BTN_TOOL_PEN;

		if (!wacom->pen_prox) {
			wacom->pen_prox = true;

			epen_location_detect(wacom, location, wacom->x, wacom->y);
			wacom->hi_x = wacom->x;
			wacom->hi_y = wacom->y;
#if WACOM_PRODUCT_SHIP
			input_info(true, wacom->dev, "[HI] loc:%s (%s%s)\n", location,
				input == wacom->input_dev_unused ? "un_" : "",
				wacom->tool == BTN_TOOL_PEN ? "p" : "r");
#else
			input_info(true, wacom->dev, "[HI]  x:%d  y:%d loc:%s (%s%s) 0x%02x\n",
					wacom->x, wacom->y, location,
					input == wacom->input_dev_unused ? "unused_" : "",
					wacom->tool == BTN_TOOL_PEN ? "pen" : "rubber", data[0]);
#endif
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
			sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_HOVER_IN, NULL);
#endif
			return;
		}

		/* report info */
		input_report_abs(input, ABS_X, wacom->x);
		input_report_abs(input, ABS_Y, wacom->y);
		input_report_key(input, BTN_STYLUS, stylus);
		input_report_key(input, BTN_TOUCH, prox);

		input_report_abs(input, ABS_PRESSURE, wacom->pressure);
		input_report_abs(input, ABS_DISTANCE, wacom->height);
		input_report_abs(input, ABS_TILT_X, wacom->tilt_x);
		input_report_abs(input, ABS_TILT_Y, wacom->tilt_y);
		input_report_key(input, wacom->tool, 1);
		input_sync(input);

		if (wacom->debug_flag & WACOM_DEBUG_PRINT_ALLEVENT)
			input_info(true, wacom->dev, "[A] x:%d y:%d, p:%d, h:%d, tx:%d, ty:%d\n",
					wacom->x, wacom->y, wacom->pressure, wacom->height,
					wacom->tilt_x, wacom->tilt_y);

		wacom->mcount++;

		/* log */
		if (prox && !wacom->pen_pressed) {
			epen_location_detect(wacom, location, wacom->x, wacom->y);
			wacom->pressed_x = wacom->x;
			wacom->pressed_y = wacom->y;

#if WACOM_PRODUCT_SHIP
			input_info(true, wacom->dev, "[P] p:%d loc:%s tool:%x mc:%d\n",
					wacom->pressure, location, wacom->tool, wacom->mcount);
#else
			input_info(true, wacom->dev,
					"[P]  x:%d  y:%d p:%d loc:%s tool:%x mc:%d\n",
					wacom->x, wacom->y, wacom->pressure, location, wacom->tool, wacom->mcount);
#endif
		} else if (!prox && wacom->pen_pressed) {
			epen_location_detect(wacom, location, wacom->x, wacom->y);
#if WACOM_PRODUCT_SHIP
			input_info(true, wacom->dev, "[R] dd:%d,%d loc:%s tool:%x mc:%d\n",
					wacom->x - wacom->pressed_x, wacom->y - wacom->pressed_y,
					location, wacom->tool, wacom->mcount);
#else
			input_info(true, wacom->dev,
					"[R] lx:%d ly:%d dd:%d,%d loc:%s tool:%x mc:%d\n",
					wacom->x, wacom->y,
					wacom->x - wacom->pressed_x, wacom->y - wacom->pressed_y,
					location, wacom->tool, wacom->mcount);
#endif
			wacom->pressed_x = wacom->pressed_y = 0;
		}

		if (wacom->pen_pressed) {
			long msec_diff;
			int sec_diff;
			int time_diff;
			int x_diff;
			int y_diff;

			msec_diff = (current_time.tv_nsec - wacom->tv_nsec) / 1000000;
			sec_diff = current_time.tv_sec - wacom->tv_sec;
			time_diff = sec_diff * 1000 + (int)msec_diff;
			if (time_diff > TIME_MILLS_MAX)
				input_info(true, wacom->dev, "time_diff: %d\n", time_diff);

			x_diff = wacom->previous_x - wacom->x;
			y_diff = wacom->previous_y - wacom->y;
			if (x_diff < DIFF_MIN || x_diff > DIFF_MAX || y_diff < DIFF_MIN || y_diff > DIFF_MAX)
				input_info(true, wacom->dev, "x_diff: %d, y_diff: %d\n", x_diff, y_diff);
		}
		wacom->pen_pressed = prox;

		wacom->previous_x = wacom->x;
		wacom->previous_y = wacom->y;

		/* check side */
		if (stylus && !wacom->side_pressed)
			input_info(true, wacom->dev, "side on\n");
		else if (!stylus && wacom->side_pressed)
			input_info(true, wacom->dev, "side off\n");

		wacom->side_pressed = stylus;
	} else {
		if (wacom->pen_prox) {
			input_report_abs(input, ABS_PRESSURE, 0);
			input_report_key(input, BTN_STYLUS, 0);
			input_report_key(input, BTN_TOUCH, 0);
			/* prevent invalid operation of input booster */
			input_sync(input);

			input_report_abs(input, ABS_DISTANCE, 0);
			input_report_key(input, BTN_TOOL_RUBBER, 0);
			input_report_key(input, BTN_TOOL_PEN, 0);
			input_sync(input);

			epen_location_detect(wacom, location, wacom->x, wacom->y);

			if (wacom->pen_pressed) {
#if WACOM_PRODUCT_SHIP
				input_info(true, wacom->dev,
						"[R] dd:%d,%d loc:%s tool:%x mc:%d & [HO] dd:%d,%d\n",
						wacom->x - wacom->pressed_x, wacom->y - wacom->pressed_y,
						location, wacom->tool, wacom->mcount,
						wacom->x - wacom->hi_x, wacom->y - wacom->hi_y);
#else
				input_info(true, wacom->dev,
						"[R] lx:%d ly:%d dd:%d,%d loc:%s tool:%x mc:%d & [HO] dd:%d,%d\n",
						wacom->x, wacom->y,
						wacom->x - wacom->pressed_x, wacom->y - wacom->pressed_y,
						location, wacom->tool, wacom->mcount,
						wacom->x - wacom->hi_x, wacom->y - wacom->hi_y);
#endif
			} else {
#if WACOM_PRODUCT_SHIP
				input_info(true, wacom->dev, "[HO] dd:%d,%d loc:%s mc:%d\n",
						wacom->x - wacom->hi_x, wacom->y - wacom->hi_y,
						location, wacom->mcount);

#else
				input_info(true, wacom->dev, "[HO] lx:%d ly:%d dd:%d,%d loc:%s mc:%d\n",
						wacom->x, wacom->y,
						wacom->x - wacom->hi_x, wacom->y - wacom->hi_y,
						location, wacom->mcount);
#endif
				wacom_release(wacom->input_dev);
			}
#ifdef SUPPORT_STANDBY_MODE
//			ktime_get_real_ts64(&current_time);
			wacom->activate_time = current_time.tv_sec;
			wacom->standby_state = WACOM_ORIGINAL_SCAN_RATE;

			input_info(true, wacom->dev, "%s: set activate time(%lld)\n",
						__func__, wacom->activate_time);
#endif
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
			sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_HOVER_OUT, NULL);
#endif
			wacom->pressed_x = wacom->pressed_y = wacom->hi_x = wacom->hi_y = 0;

		}

		wacom->pen_prox = 0;
		wacom->pen_pressed = 0;
		wacom->side_pressed = 0;
		wacom->mcount = 0;
	}

	wacom->tv_sec = current_time.tv_sec;
	wacom->tv_nsec = current_time.tv_nsec;
}

static int wacom_event_handler(struct wacom_data *wacom)
{
	int ret;
	char pack_id;
	bool debug = true;
	char buff[COM_COORD_NUM + 1] = { 0, };

	ret = wacom_recv(wacom, buff, COM_COORD_NUM + 1);
	if (ret < 0) {
		input_err(true, wacom->dev, "%s: read failed\n", __func__);
		return ret;
	}

	pack_id = buff[0] & 0x0F;

	switch (pack_id) {
	case COORD_PACKET:
		wacom_coord_handler(wacom, buff);
		debug = false;
		break;
	case AOP_PACKET:
		wacom_aop_handler(wacom, buff);
		break;
	case NOTI_PACKET:
		wacom->report_scan_seq = (buff[2] & 0xF0) >> 4;
		wacom_noti_handler(wacom, buff);
		break;
	case REPLY_PACKET:
		wacom_reply_handler(wacom, buff);
		break;
	case SEPC_PACKET:
		break;
	default:
		input_info(true, wacom->dev, "%s: unexpected packet %d\n",
				__func__, pack_id);
		break;
	};

	if (debug) {
		input_info(true, wacom->dev,
				"%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x / s:%d\n",
				buff[0], buff[1], buff[2], buff[3], buff[4], buff[5],
				buff[6], buff[7], buff[8], buff[9], buff[10], buff[11],
				buff[12], buff[13], buff[14], buff[15],
				pack_id == NOTI_PACKET ? wacom->report_scan_seq : 0);
	}

	return ret;
}

static irqreturn_t wacom_interrupt(int irq, void *dev_id)
{
	struct wacom_data *wacom = dev_id;
	int ret = 0;

	ret = sec_input_handler_start(wacom->dev);
	if (ret < 0)
		return IRQ_HANDLED;

	ret = wacom_event_handler(wacom);
	if (ret < 0) {
		wacom_forced_release(wacom);
		forced_release_fullscan(wacom);
		wacom->reset_flag = true;
	}

	return IRQ_HANDLED;
}

static irqreturn_t wacom_interrupt_pdct(int irq, void *dev_id)
{
	struct wacom_data *wacom = dev_id;

	if (wacom->use_garage == false) {
		input_err(true, wacom->dev, "%s: not support pdct skip!\n", __func__);
		return IRQ_HANDLED;
	}

	if (wacom->query_status == false)
		return IRQ_HANDLED;

#if !WACOM_SEC_FACTORY
	if (wacom->support_garage_open_test &&
		wacom->garage_connection_check == false) {
		input_err(true, wacom->dev, "%s: garage_connection_check fail skip(%d)!!\n",
				__func__, gpio_get_value(wacom->pdct_gpio));
		return IRQ_HANDLED;
	}
#endif

	if (wacom->pm_suspend) {
		__pm_wakeup_event(wacom->wacom_ws, SEC_TS_WAKE_LOCK_TIME);

		if (!wacom->plat_data->resume_done.done) {
			if (!IS_ERR_OR_NULL(wacom->irq_workqueue_pdct)) {
				input_info(true, wacom->dev, "%s: disable pdct irq and queue waiting work\n", __func__);
				disable_irq_nosync(wacom->irq_pdct);
				queue_work(wacom->irq_workqueue_pdct, &wacom->irq_work_pdct);
			} else {
				input_err(true, wacom->dev, "%s: irq_pdct_workqueue not exist\n", __func__);
			}
			return IRQ_HANDLED;
		}
	}

	wacom->pen_pdct = gpio_get_value(wacom->pdct_gpio);
	if (wacom->pen_pdct)
		wacom->function_result &= ~EPEN_EVENT_PEN_OUT;
	else
		wacom->function_result |= EPEN_EVENT_PEN_OUT;

	input_info(true, wacom->dev, "%s: pen is %s (%d)\n",
					__func__, wacom->pen_pdct ? "IN" : "OUT",
					gpio_get_value(wacom->plat_data->irq_gpio));

	input_report_switch(wacom->input_dev, SW_PEN_INSERT, (wacom->function_result & EPEN_EVENT_PEN_OUT));
	input_sync(wacom->input_dev);

	if (wacom->function_result & EPEN_EVENT_PEN_OUT)
		wacom->pen_out_count++;

	if (!mutex_trylock(&wacom->lock)) {
		input_err(true, wacom->dev, "%s: set survey after unlock\n", __func__);
		wacom->pdct_lock_fail = true;
		goto irq_ret;
	}

	wacom_pdct_survey_mode(wacom);
	mutex_unlock(&wacom->lock);

irq_ret:
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	if (wacom->pen_pdct)
		sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_INSERT, NULL);
	else
		sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_REMOVE, NULL);
#endif

	return IRQ_HANDLED;
}


int wacom_reset_wakeup_sequence(struct wacom_data *wacom)
{
	int ret = 0;

	input_info(true, wacom->dev, "%s start\n", __func__);
#if WACOM_SEC_FACTORY
	if (wacom->fac_garage_mode)
		input_info(true, wacom->dev, "%s: garage mode\n", __func__);
#endif

	wacom_select_survey_mode(wacom, true);
	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF))
		goto wakeup_sequence_out;

	if (wacom->wcharging_mode)
		wacom_set_sense_mode(wacom);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	if (wacom->tsp_scan_mode < 0) {
		wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_RELEASE, NULL);
		wacom->is_tsp_block = false;
	}
#endif

	if (!gpio_get_value(wacom->plat_data->irq_gpio)) {
		u8 data[COM_COORD_NUM + 1] = { 0, };
		char pack_id;

		input_info(true, wacom->dev, "%s: irq was enabled\n", __func__);

		ret = wacom_recv(wacom, data, COM_COORD_NUM + 1);
		if (ret < 0)
			input_err(true, wacom->dev, "%s: failed to receive\n", __func__);

		input_info(true, wacom->dev,
				"%x %x %x %x %x, %x %x %x %x %x, %x %x %x\n",
				data[0], data[1], data[2], data[3], data[4], data[5],
				data[6], data[7], data[8], data[9], data[10],
				data[11], data[12]);
		pack_id = data[0] & 0x0F;
		if ((pack_id == NOTI_PACKET) && (data[1] == ESD_DETECT_PACKET)) {
			input_err(true, wacom->dev, "%s: ESD_DETECT_PACKET occurred during reset(%d).\n",
					__func__, wacom->esd_packet_count);
			wacom->esd_packet_count++;
			ret = -EAGAIN;
			return ret;
		}
	}

	if (!wacom->samplerate_state) {
		char cmd = COM_SAMPLERATE_START;

		input_info(true, wacom->dev, "%s: samplerate state is %d, need to recovery\n",
				__func__, wacom->samplerate_state);

		ret = wacom_send(wacom, &cmd, 1);
		if (ret < 0)
			input_err(true, wacom->dev, "%s: failed to sned start cmd %d\n", __func__, ret);
		else
			wacom->samplerate_state = true;
	}

	input_info(true, wacom->dev,
			"%s: i(%d) f_set(0x%x) f_ret(0x%x) samplerate(%d)\n",
			__func__, gpio_get_value(wacom->plat_data->irq_gpio), wacom->function_set,
			wacom->function_result, wacom->samplerate_state);

	wacom->print_info_cnt_open = 0;
	wacom->tsp_block_cnt = 0;
	schedule_work(&wacom->work_print_info.work);

	wacom_enable_irq_wake(wacom, false);

wakeup_sequence_out:
// !!!
//	atomic_set(&wacom->screen_on, 1);

	input_info(true, wacom->dev, "%s: end\n", __func__);
	return ret;
}

void wacom_reset_sleep_sequence(struct wacom_data *wacom)
{
	input_info(true, wacom->dev, "%s start\n", __func__);
#if WACOM_SEC_FACTORY
	if (wacom->fac_garage_mode)
		input_info(true, wacom->dev, "%s: garage mode\n", __func__);

#endif
	wacom_select_survey_mode(wacom, false);

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_ON_LP))
		wacom_enable_irq_wake(wacom, true);

	input_info(true, wacom->dev, "%s end\n", __func__);
}

void wacom_reset_block_sequence(struct wacom_data *wacom)
{
	input_info(true, wacom->dev, "%s start\n", __func__);
	wacom->esd_reset_blocked_enable = true;
	wacom_enable_irq_wake(wacom, false);
	cancel_delayed_work_sync(&wacom->work_print_info);
	atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
	wacom_power(wacom, false);

	input_info(true, wacom->dev, "%s end\n", __func__);
}

static void wacom_esd_reset_work(struct work_struct *work)
{
	struct wacom_data *wacom = container_of(work, struct wacom_data, esd_reset_work.work);
	int ret = 0;

	input_info(true, wacom->dev, "%s : start\n", __func__);
	mutex_lock(&wacom->lock);
	cancel_delayed_work_sync(&wacom->work_print_info);

	if (wacom->pen_pressed || wacom->side_pressed || wacom->pen_prox)
		wacom_forced_release(wacom);

	forced_release_fullscan(wacom);
	/* Reset IC */
	wacom_reset_hw(wacom);
	/* I2C Test */
	ret = wacom_query(wacom);
	if (ret < 0) {
		input_info(true, wacom->dev, "%s : failed to query to IC(%d)\n", __func__, ret);
		goto out;
	}

	input_info(true, wacom->dev, "%s: result %d\n", __func__, wacom->query_status);

	ret = wacom_reset_wakeup_sequence(wacom);
	if (ret == -EAGAIN) {
		wacom->esd_continuous_reset_count++;
		goto retry_out;
	}

	if (wacom->epen_blocked || (wacom->fold_state == FOLD_STATUS_FOLDING) ||
			sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev, "%s : power off, epen_blockecd(%d), flip(%d)\n",
			__func__, wacom->epen_blocked, wacom->fold_state);
		wacom_enable_irq_wake(wacom, false);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
		wacom_power(wacom, false);
		wacom->function_result &= ~EPEN_EVENT_SURVEY;
		wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
		wacom->reset_flag = false;

		goto out;
	}

	if (!atomic_read(&wacom->plat_data->enabled))
		wacom_reset_sleep_sequence(wacom);

out:
	if (sec_input_cmp_ic_status(wacom->dev, CHECK_ON_LP)) {
		if (!gpio_get_value(wacom->esd_detect_gpio)) {
			wacom->esd_continuous_reset_count++;
			if (wacom->esd_max_continuous_reset_count < wacom->esd_continuous_reset_count)
				wacom->esd_max_continuous_reset_count = wacom->esd_continuous_reset_count;

			input_err(true, wacom->dev,
					"%s : Esd irq gpio went down to low during the reset. Reset gain(%d)\n",
					__func__, wacom->esd_continuous_reset_count);
			if (wacom->esd_continuous_reset_count >= MAXIMUM_CONTINUOUS_ESD_RESET_COUNT) {
				input_err(true, wacom->dev,
						"%s : The esd reset is continuously operated, so the wacom is stopped until device reboot.(%d)\n",
						__func__, wacom->esd_continuous_reset_count);
				wacom_reset_block_sequence(wacom);
				goto retry_out;
			}
		}
	}
	wacom->esd_continuous_reset_count = 0;

retry_out:
	wacom->abnormal_reset_count++;
	wacom->reset_flag = false;
	wacom->reset_is_on_going = false;
	if (sec_input_cmp_ic_status(wacom->dev, CHECK_ON_LP))
		wacom_enable_irq(wacom, true);

	mutex_unlock(&wacom->lock);

	if (ret == -EAGAIN)
		schedule_delayed_work(&wacom->esd_reset_work, msecs_to_jiffies(10));

	input_info(true, wacom->dev, "%s : end(%d)\n", __func__, wacom->esd_continuous_reset_count);
}


static irqreturn_t wacom_esd_detect_interrupt(int irq, void *dev_id)
{
	struct wacom_data *wacom = dev_id;

	input_info(true, wacom->dev, "%s: normal state: gpio:%d, reset on going:%d\n",
		__func__, gpio_get_value(wacom->esd_detect_gpio), wacom->reset_is_on_going);

	if (gpio_get_value(wacom->esd_detect_gpio))
		return IRQ_HANDLED;

	wacom->esd_irq_count++;

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s : not probe done & skip!\n", __func__);
		wacom->reset_flag = true;
		wacom->esd_continuous_reset_count = 0;
		return IRQ_HANDLED;
	}

	if (wacom->fold_state == FOLD_STATUS_FOLDING) {
		input_info(true, wacom->dev, "%s: folded, skip!\n", __func__);
		wacom->reset_flag = true;
		wacom->esd_continuous_reset_count = 0;
		disable_irq_nosync(wacom->irq);
		disable_irq_nosync(wacom->irq_esd_detect);
		return IRQ_HANDLED;
	}

	__pm_wakeup_event(wacom->wacom_esd_ws, WACOM_ESD_WAKE_LOCK_TIME);
	input_info(true, wacom->dev, "%s: esd reset pm wake lock(%dms)\n",
			__func__, WACOM_ESD_WAKE_LOCK_TIME);

	if (wacom->reset_is_on_going) {
		input_info(true, wacom->dev,
				"%s: already reset on going:%d\n", __func__, wacom->reset_is_on_going);
		wacom->reset_flag = true;
	}

	disable_irq_nosync(wacom->irq);
	disable_irq_nosync(wacom->irq_esd_detect);
	wacom->reset_is_on_going = true;
	cancel_delayed_work_sync(&wacom->esd_reset_work);
	schedule_delayed_work(&wacom->esd_reset_work, msecs_to_jiffies(1));

	return IRQ_HANDLED;
}

static void open_test_work(struct work_struct *work)
{
	struct wacom_data *wacom =
		container_of(work, struct wacom_data, open_test_dwork.work);
	int ret = 0;

	input_info(true, wacom->dev, "%s : start!\n", __func__);
	mutex_lock(&wacom->table_compensation_lock);
	wacom->probe_done = false;

	if (wacom->support_garage_open_test) {
		ret = wacom_open_test(wacom, WACOM_GARAGE_TEST);
		if (ret) {
#if IS_ENABLED(CONFIG_SEC_FACTORY)
#if IS_ENABLED(CONFIG_SEC_ABC)
			sec_abc_send_event(SEC_ABC_SEND_EVENT_TYPE_WACOM_DIGITIZER_NOT_CONNECTED);
#endif
			devm_free_irq(wacom->dev, wacom->irq_pdct, wacom);
			wacom->pdct_gpio = -1;
			wacom->use_garage = false;
			input_err(true, wacom->dev, "%s : release pdct interrupt\n", __func__);
#endif
			input_err(true, wacom->dev, "grage test check failed %d\n", ret);
			wacom_reset_hw(wacom);
		}
	}

	ret = wacom_open_test(wacom, WACOM_DIGITIZER_TEST);
	if (ret) {
#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event(SEC_ABC_SEND_EVENT_TYPE_WACOM_DIGITIZER_NOT_CONNECTED);
#endif
		input_err(true, wacom->dev, "open test check failed %d\n", ret);
		wacom_reset_hw(wacom);
	}

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	if (wacom->is_tsp_block) {
		wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_RELEASE, NULL);
		wacom->is_tsp_block = false;
		input_err(true, wacom->dev, "%s : release tsp scan block\n", __func__);
	}
#endif

	if (wacom->use_garage && gpio_is_valid(wacom->pdct_gpio)) {
		wacom->pen_pdct = gpio_get_value(wacom->pdct_gpio);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		if (wacom->pen_pdct)
			sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_INSERT, NULL);
		else
			sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_REMOVE, NULL);
#endif
		input_info(true, wacom->dev, "%s: pdct(%d)\n", __func__, wacom->pen_pdct);

		if (wacom->pen_pdct)
			wacom->function_result &= ~EPEN_EVENT_PEN_OUT;
		else
			wacom->function_result |= EPEN_EVENT_PEN_OUT;

		input_report_switch(wacom->input_dev, SW_PEN_INSERT,
				(wacom->function_result & EPEN_EVENT_PEN_OUT));
		input_sync(wacom->input_dev);

		input_info(true, wacom->dev, "%s : pen is %s\n", __func__,
				(wacom->function_result & EPEN_EVENT_PEN_OUT) ? "OUT" : "IN");
	}

	wacom->probe_done = true;

	mutex_unlock(&wacom->table_compensation_lock);
	input_info(true, wacom->dev, "%s : end!\n", __func__);
}

static void probe_open_test(struct wacom_data *wacom)
{
	INIT_DELAYED_WORK(&wacom->open_test_dwork, open_test_work);

	/* update the current status */
	schedule_delayed_work(&wacom->open_test_dwork, msecs_to_jiffies(1));
}

int wacom_enable(struct device *dev)
{
	struct wacom_data *wacom = dev_get_drvdata(dev);
	int ret = 0;

	input_info(true, wacom->dev, "%s\n", __func__);

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s : not probe done & skip!\n", __func__);
		return 0;
	}

	atomic_set(&wacom->screen_on, 1);

#if WACOM_SEC_FACTORY
	if (wacom->epen_blocked) {
		input_err(true, wacom->dev, "%s : FAC epen_blocked SKIP!!\n", __func__);
		return ret;
	}
#else
	if (wacom->epen_blocked) {
		input_err(true, wacom->dev, "%s : epen_blocked\n", __func__);
		if (sec_input_cmp_ic_status(wacom->dev, CHECK_ON_LP))
			wacom_disable_mode(wacom, WACOM_DISABLE);
		return ret;
	}
#endif

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		mutex_lock(&wacom->lock);
		wacom_power(wacom, true);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
		sec_delay(100);
		wacom->reset_flag = false;
		mutex_unlock(&wacom->lock);
	}

	atomic_set(&wacom->plat_data->enabled, 1);

	wacom_wakeup_sequence(wacom);

	cancel_delayed_work(&wacom->work_print_info);
	wacom->print_info_cnt_open = 0;
	wacom->tsp_block_cnt = 0;
	schedule_work(&wacom->work_print_info.work);

	return ret;
}

static int wacom_disable(struct device *dev)
{
	struct wacom_data *wacom = dev_get_drvdata(dev);

	input_info(true, wacom->dev, "%s\n", __func__);

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s : not probe done & skip!\n", __func__);
		return 0;
	}

	atomic_set(&wacom->screen_on, 0);

	atomic_set(&wacom->plat_data->enabled, 0);

	cancel_delayed_work(&wacom->work_print_info);
	wacom_print_info(wacom);

#if WACOM_SEC_FACTORY
	if (wacom->epen_blocked) {
		input_err(true, wacom->dev, "%s : FAC epen_blocked SKIP!!\n", __func__);
		return 0;
	}
#else
	if (wacom->epen_blocked) {
		input_err(true, wacom->dev, "%s : epen_blocked\n", __func__);
		if (sec_input_cmp_ic_status(wacom->dev, CHECK_ON_LP))
			wacom_disable_mode(wacom, WACOM_DISABLE);
		return 0;
	}
#endif

	if (wacom->fold_state == FOLD_STATUS_FOLDING) {
		input_info(true, wacom->dev, "%s: folder closed, turn off\n", __func__);
		mutex_lock(&wacom->lock);
		wacom_enable_irq(wacom, false);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
		wacom_power(wacom, false);
		wacom->reset_flag = false;
		wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
		wacom->function_result &= ~EPEN_EVENT_SURVEY;
		mutex_unlock(&wacom->lock);
	} else {
		wacom_sleep_sequence(wacom);
	}

	return 0;
}

static void wacom_set_input_values(struct wacom_data *wacom,
		struct input_dev *input_dev)
{
	/* Set input values before registering input device */

	input_dev->phys = input_dev->name;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = wacom->dev;

	input_set_abs_params(input_dev, ABS_PRESSURE, 0, wacom->max_pressure, 0, 0);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, wacom->max_height, 0, 0);
	input_set_abs_params(input_dev, ABS_TILT_X, -wacom->max_x_tilt, wacom->max_x_tilt, 0, 0);
	input_set_abs_params(input_dev, ABS_TILT_Y, -wacom->max_y_tilt, wacom->max_y_tilt, 0, 0);

	input_set_abs_params(input_dev, ABS_X, 0, wacom->plat_data->max_x, 4, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, wacom->plat_data->max_y, 4, 0);

	input_set_capability(input_dev, EV_KEY, BTN_TOOL_PEN);
	input_set_capability(input_dev, EV_KEY, BTN_TOOL_RUBBER);
	input_set_capability(input_dev, EV_KEY, BTN_TOUCH);
	input_set_capability(input_dev, EV_KEY, BTN_STYLUS);

	input_set_capability(input_dev, EV_SW, SW_PEN_INSERT);

	/* AOP */
	input_set_capability(input_dev, EV_KEY, KEY_WAKEUP_UNLOCK);
	input_set_capability(input_dev, EV_KEY, KEY_HOMEPAGE);

	input_set_drvdata(input_dev, wacom);
}

void wacom_print_info(struct wacom_data *wacom)
{
	if (!wacom)
		return;

	if (!wacom->dev)
		return;

	wacom->print_info_cnt_open++;

	if (wacom->print_info_cnt_open > 0xfff0)
		wacom->print_info_cnt_open = 0;
	if (wacom->scan_info_fail_cnt > 1000)
		wacom->scan_info_fail_cnt = 1000;

	input_info(true, wacom->dev,
			"%s: ps %s, pen %s, screen_on %d, report_scan_seq %d, epen %s, stdby:%d count(%u,%u,%u), "
			"mode(%d), block_cnt(%d), check(%d), test(%d,%d), ver[%02X%02X%02X%02X] #%d\n",
			__func__, wacom->battery_saving_mode ?  "on" : "off",
			(wacom->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
			atomic_read(&wacom->screen_on),
			wacom->report_scan_seq, wacom->epen_blocked ? "blocked" : "unblocked",
#ifdef SUPPORT_STANDBY_MODE
			wacom->standby_state,
#else
			0,
#endif
			wacom->i2c_fail_count, wacom->abnormal_reset_count, wacom->scan_info_fail_cnt,
			wacom->check_survey_mode, wacom->tsp_block_cnt, wacom->check_elec,
			wacom->connection_check, wacom->garage_connection_check,
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3],
			wacom->print_info_cnt_open);
}

static void wacom_print_info_work(struct work_struct *work)
{
	struct wacom_data *wacom = container_of(work, struct wacom_data, work_print_info.work);

	wacom_print_info(wacom);
	schedule_delayed_work(&wacom->work_print_info, msecs_to_jiffies(30000));	// 30s
}

int load_fw_built_in(struct wacom_data *wacom, int fw_index)
{
	int ret = 0;
	const char *fw_load_path = NULL;
#if WACOM_SEC_FACTORY
	int index = 0;
#endif

	input_info(true, wacom->dev, "%s: fw_index (%d)\n", __func__, fw_index);

	if (wacom->fw_separate_by_camera) {
		int fw_numbers = of_property_count_strings(wacom->dev->of_node, "sec,firmware_name");

		input_err(true, wacom->dev, "%s: index:%d, fw numbers:%d\n",
						__func__, wacom->fw_bin_idx, fw_numbers);
		if (fw_numbers <= wacom->fw_bin_idx)
			return -EINVAL;

		ret = of_property_read_string_index(wacom->dev->of_node,
				"sec,firmware_name", wacom->fw_bin_idx, &wacom->plat_data->firmware_name);
		if (ret) {
			input_err(true, wacom->dev, "%s: failed to read firmware_name(%d) idx(%d)\n",
						__func__, ret, wacom->fw_bin_idx);
			wacom->plat_data->firmware_name = NULL;
		}

		fw_load_path = wacom->plat_data->firmware_name;

		input_info(true, wacom->dev, "%s: load %s firmware(%d)\n",
				__func__, fw_load_path, wacom->fw_bin_idx);
	} else {
		fw_load_path = wacom->plat_data->firmware_name;
	}

#if WACOM_SEC_FACTORY
	if (fw_index != FW_BUILT_IN) {
		if (fw_index == FW_FACTORY_GARAGE)
			index = 0;
		else if (fw_index == FW_FACTORY_UNIT)
			index = 1;

		ret = of_property_read_string_index(wacom->dev->of_node,
				"wacom,fw_fac_path", index, &wacom->fw_fac_path);
		if (ret) {
			input_err(true, wacom->dev, "%s: failed to read fw_fac_path %d\n", __func__, ret);
			wacom->fw_fac_path = NULL;
		}

		fw_load_path = wacom->fw_fac_path;

		input_info(true, wacom->dev, "%s: load %s firmware\n",
				__func__, fw_load_path);
	}
#endif

	if (fw_load_path == NULL) {
		input_err(true, wacom->dev,
				"Unable to open firmware. firmware_name is NULL\n");
		return -EINVAL;
	}

	ret = request_firmware(&wacom->firm_data, fw_load_path, wacom->dev);
	if (ret < 0) {
		input_err(true, wacom->dev, "Unable to open firmware(%s) ret %d\n", fw_load_path, ret);
		return ret;
	}

	wacom->fw_img = (struct fw_image *)wacom->firm_data->data;
	wacom->fw_size = (int)wacom->firm_data->size - wacom->fw_img->hdr_len;
	input_info(true, wacom->dev, "%s : fw load done(%s)\n", __func__, fw_load_path);

	return ret;
}

static int wacom_get_fw_size(struct wacom_data *wacom)
{
	u32 fw_size = 0;

	if (wacom->plat_data->img_version_of_ic[0] == MPU_W9020)
		fw_size = 144 * 1024;
	else if (wacom->plat_data->img_version_of_ic[0] == MPU_W9021)
		fw_size = 256 * 1024;
	else if (wacom->plat_data->img_version_of_ic[0] == MPU_WEZ01)
		fw_size = 256 * 1024;
	else if (wacom->plat_data->img_version_of_ic[0] == MPU_WEZ02)
		fw_size = 256 * 1024;
	else
		fw_size = 128 * 1024;

	input_info(true, wacom->dev, "%s: type[%d] size[0x%X]\n",
			__func__, wacom->plat_data->img_version_of_ic[0], fw_size);

	return fw_size;
}

static int load_fw_sdcard(struct wacom_data *wacom, u8 fw_index)
{
	long fsize;
	int ret = 0;
	unsigned int nSize;
	unsigned long nSize2;
	char *fw_data;

	nSize = wacom_get_fw_size(wacom);
	nSize2 = nSize + sizeof(struct fw_image);

	if (!wacom->plat_data->external_firmware_data || (wacom->plat_data->external_firmware_size <= 0)) {
		input_err(true, wacom->dev, "firmware is not available\n");
		ret = -ENOENT;
		return ret;
	}

	fw_data = wacom->plat_data->external_firmware_data;
	fsize = wacom->plat_data->external_firmware_size;
	input_info(true, wacom->dev, "size %ld Bytes\n", fsize);

	if (fw_index == FW_IN_SDCARD_SIGNED) {
		/* name 5, digest 32, signature 512 */
		long spu_ret = 0;

		fsize -= SPU_METADATA_SIZE(WACOM);
		spu_ret = spu_firmware_signature_verify("WACOM",
				wacom->plat_data->external_firmware_data,
				wacom->plat_data->external_firmware_size);
		if (spu_ret != fsize) {
			input_err(true, wacom->dev, "%s: signature verify failed, spu_ret:%ld, ori_size:%ld\n",
				__func__, spu_ret, fsize);
			ret = -EPERM;
			return ret;
		}
	}

	if ((fsize != nSize) && (fsize != nSize2)) {
		input_err(true, wacom->dev, "UMS firmware size is different\n");
		ret = -EFBIG;
		return ret;
	}

	wacom->fw_img = (struct fw_image *)fw_data;
	wacom->fw_size = wacom->plat_data->external_firmware_size - wacom->fw_img->hdr_len;
	return 0;
}

int wacom_load_fw(struct wacom_data *wacom, u8 fw_update_way)
{
	int ret = 0;
	struct fw_image *fw_img;

	switch (fw_update_way) {
	case FW_BUILT_IN:
#if WACOM_SEC_FACTORY
	case FW_FACTORY_GARAGE:
	case FW_FACTORY_UNIT:
#endif
		ret = load_fw_built_in(wacom, fw_update_way);
		break;
	case FW_IN_SDCARD:
		fallthrough;
	case FW_IN_SDCARD_SIGNED:
		ret = load_fw_sdcard(wacom, fw_update_way);
		break;
	default:
		input_info(true, wacom->dev, "unknown path(%d)\n", fw_update_way);
		goto err_load_fw;
	}

	if (ret < 0)
		goto err_load_fw;

	fw_img = wacom->fw_img;

	/* header check */
	if (fw_img->hdr_ver == 1 && fw_img->hdr_len == sizeof(struct fw_image)) {
		wacom->fw_data = (u8 *) fw_img->data;
#if WACOM_SEC_FACTORY
		if ((fw_update_way == FW_BUILT_IN) ||
				(fw_update_way == FW_FACTORY_UNIT) || (fw_update_way == FW_FACTORY_GARAGE)) {
#else
		if (fw_update_way == FW_BUILT_IN) {
#endif
			//need to check data
			wacom->plat_data->img_version_of_bin[0] = (fw_img->fw_ver1 >> 8) & 0xFF;
			wacom->plat_data->img_version_of_bin[1] = fw_img->fw_ver1 & 0xFF;
			wacom->plat_data->img_version_of_bin[2] = (fw_img->fw_ver2 >> 8) & 0xFF;
			wacom->plat_data->img_version_of_bin[3] = fw_img->fw_ver2 & 0xFF;
			memcpy(wacom->fw_chksum, fw_img->checksum, 5);
		}
	} else {
		input_err(true, wacom->dev, "no hdr\n");
		wacom->fw_data = (u8 *) fw_img;
	}

	return ret;

err_load_fw:
	wacom->fw_data = NULL;
	return ret;
}

void wacom_unload_fw(struct wacom_data *wacom)
{
	switch (wacom->fw_update_way) {
	case FW_BUILT_IN:
#if WACOM_SEC_FACTORY
	case FW_FACTORY_GARAGE:
	case FW_FACTORY_UNIT:
#endif
		release_firmware(wacom->firm_data);
		break;
	default:
		break;
	}

	wacom->fw_img = NULL;
	wacom->fw_update_way = FW_NONE;
	wacom->firm_data = NULL;
	wacom->fw_data = NULL;
}

int wacom_fw_update_on_hidden_menu(struct wacom_data *wacom, u8 fw_update_way)
{
	int ret;
	int retry = 3;
	int i;

	input_info(true, wacom->dev, "%s: update:%d\n", __func__, fw_update_way);

	mutex_lock(&wacom->update_lock);
	wacom_enable_irq(wacom, false);

	/* release pen, if it is pressed */
	if (wacom->pen_pressed || wacom->side_pressed || wacom->pen_prox)
		wacom_forced_release(wacom);

	if (wacom->is_tsp_block)
		forced_release_fullscan(wacom);

	ret = wacom_load_fw(wacom, fw_update_way);
	if (ret < 0) {
		input_info(true, wacom->dev, "failed to load fw data\n");
		wacom->update_status = FW_UPDATE_FAIL;
		goto err_update_load_fw;
	}
	wacom->fw_update_way = fw_update_way;

	/* firmware info */
	input_info(true, wacom->dev, "ic fw ver : %02X%02X%02X%02X new fw ver : %02X%02X%02X%02X\n",
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3],
			wacom->plat_data->img_version_of_bin[0], wacom->plat_data->img_version_of_bin[1],
			wacom->plat_data->img_version_of_bin[2], wacom->plat_data->img_version_of_bin[3]);

	// have to check it later
	if (wacom->fw_update_way == FW_BUILT_IN && wacom->plat_data->bringup == 1) {
		input_info(true, wacom->dev, "bringup #1. do not update\n");
		wacom->update_status = FW_UPDATE_FAIL;
		goto out_update_fw;
	}

	wacom->update_status = FW_UPDATE_RUNNING;

	while (retry--) {
		ret = wacom_flash(wacom);
		if (ret) {
			input_info(true, wacom->dev, "failed to flash fw(%d) %d\n", ret, retry);
			continue;
		}
		break;
	}

	retry = 3;
	while (retry--) {
		ret = wacom_query(wacom);
		if (ret < 0) {
			input_info(true, wacom->dev, "%s : failed to query to IC(%d) & reset, %d\n",
						__func__, ret, retry);
			wacom_compulsory_flash_mode(wacom, false);
			wacom_reset_hw(wacom);
			continue;
		} else {
			wacom->reset_flag = false;
		}
		break;
	}

	if (ret < 0) {
		wacom->update_status = FW_UPDATE_FAIL;
		for (i = 0; i < 4; i++)
			wacom->plat_data->img_version_of_ic[i] = 0;
		goto out_update_fw;
	}

	wacom->update_status = FW_UPDATE_PASS;

	ret = wacom_check_ub(wacom);
	if (ret < 0) {
		wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_AOP);
		input_info(true, wacom->dev, "Change mode for garage scan\n");
	}

	wacom_unload_fw(wacom);
	wacom_enable_irq(wacom, true);
	mutex_unlock(&wacom->update_lock);

	return 0;

out_update_fw:
	wacom_unload_fw(wacom);
err_update_load_fw:
	wacom_enable_irq(wacom, true);
	mutex_unlock(&wacom->update_lock);

	ret = wacom_check_ub(wacom);
	if (ret < 0) {
		wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_AOP);
		input_info(true, wacom->dev, "Change mode for garage scan\n");
	}

	return -1;
}

static int wacom_input_init(struct wacom_data *wacom)
{
	int ret = 0;
			/* using managed input device */
	wacom->input_dev = devm_input_allocate_device(wacom->dev);
	if (!wacom->input_dev) {
		input_err(true, wacom->dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	wacom->input_dev->name = "sec_e-pen";

	/* expand evdev buffer size */
	wacom->input_dev->hint_events_per_packet = 256;//hint 256 -> 2048 buffer size;

	wacom_set_input_values(wacom, wacom->input_dev);
	ret = input_register_device(wacom->input_dev);
	if (ret) {
		input_err(true, wacom->dev, "failed to register input_dev device\n");
		/* managed input devices need not be explicitly unregistred or freed. */
		return ret;
	}

	return ret;
}

int wacom_fw_update_on_probe(struct wacom_data *wacom)
{
	int ret;
	int retry = 3;
	bool bforced = false;
	int i;

	input_info(true, wacom->dev, "%s\n", __func__);

	if (wacom->plat_data->bringup == 1) {
		input_info(true, wacom->dev, "bringup #1. do not update\n");
		for (i = 0; i < 4; i++)
			wacom->plat_data->img_version_of_bin[i] = 0;
		goto skip_update_fw;
	}

	ret = wacom_load_fw(wacom, FW_BUILT_IN);
	if (ret < 0) {
		input_info(true, wacom->dev, "failed to load fw data (set bringup 1)\n");
		wacom->plat_data->bringup = 1;
		goto skip_update_fw;
	}

	if (wacom->plat_data->bringup == 2) {
		input_info(true, wacom->dev, "bringup #2. do not update\n");
		goto out_update_fw;
	}

	wacom->fw_update_way = FW_BUILT_IN;

	/* firmware info */
	input_info(true, wacom->dev, "ic fw ver : %02X%02X%02X%02X new fw ver : %02X%02X%02X%02X\n",
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3],
			wacom->plat_data->img_version_of_bin[0], wacom->plat_data->img_version_of_bin[1],
			wacom->plat_data->img_version_of_bin[2], wacom->plat_data->img_version_of_bin[3]);

	if (wacom->plat_data->bringup == 3) {
		input_info(true, wacom->dev, "bringup #3. force update\n");
		bforced = true;
	}

	if (bforced) {
		for (i = 0; i < 4; i++) {
			if (wacom->plat_data->img_version_of_ic[i] != wacom->plat_data->img_version_of_bin[i]) {
				input_info(true, wacom->dev, "force update : not equal, update fw\n");
				goto fw_update;
			}
		}
		input_info(true, wacom->dev, "force update : equal, skip update fw\n");
		goto out_update_fw;
	}

	if (wacom->plat_data->img_version_of_ic[2] == 0 && wacom->plat_data->img_version_of_ic[3] == 0) {
		input_info(true, wacom->dev, "fw is not written in ic. force update\n");
		goto fw_update;
	}

	for (i = 0; i < 2; i++) {
		if (wacom->plat_data->img_version_of_ic[i] != wacom->plat_data->img_version_of_bin[i]) {
			input_err(true, wacom->dev, "%s: do not matched version info\n", __func__);
			goto out_update_fw;
		}
	}

	if ((wacom->plat_data->img_version_of_ic[2] >> 4) == WACOM_FIRMWARE_VERSION_UNIT) {
		input_err(true, wacom->dev, "%s: assy fw. force update\n", __func__);
		goto fw_update;
	} else if ((wacom->plat_data->img_version_of_ic[2] >> 4) == WACOM_FIRMWARE_VERSION_SET) {
		if ((wacom->plat_data->img_version_of_ic[2] & 0x0F) !=
				(wacom->plat_data->img_version_of_bin[2] & 0x0F)) {
			input_err(true, wacom->dev, "%s: HW ID is different. force update\n", __func__);
			goto fw_update;
		}

		if (wacom->plat_data->img_version_of_ic[3] == wacom->plat_data->img_version_of_bin[3]) {
			ret = wacom_checksum(wacom);
			if (ret) {
				input_info(true, wacom->dev, "crc ok, do not update fw\n");
				goto out_update_fw;
			}
			input_info(true, wacom->dev, "crc err, do update\n");
		} else if (wacom->plat_data->img_version_of_ic[3] >= wacom->plat_data->img_version_of_bin[3]) {
			input_info(true, wacom->dev, "ic version is high, do not update fw\n");
			goto out_update_fw;
		}
	} else if ((wacom->plat_data->img_version_of_ic[2] >> 4) == WACOM_FIRMWARE_VERSION_TEST) {
		input_info(true, wacom->dev, "test fw\n");
		if (wacom->plat_data->img_version_of_ic[3] == wacom->plat_data->img_version_of_bin[3]) {
			input_info(true, wacom->dev, "fw ver is same, do not update fw\n");
			goto out_update_fw;
		}
	}

fw_update:
	while (retry--) {
		ret = wacom_flash(wacom);
		if (ret) {
			input_info(true, wacom->dev, "failed to flash fw(%d) %d\n", ret, retry);
			continue;
		}
		break;
	}

	retry = 3;
	while (retry--) {
		ret = wacom_query(wacom);
		if (ret < 0) {
			input_info(true, wacom->dev, "%s : failed to query to IC(%d) & reset, %d\n",
						__func__, ret, retry);
			wacom_compulsory_flash_mode(wacom, false);
			wacom_reset_hw(wacom);
			continue;
		} else {
			wacom->reset_flag = false;
		}
		break;
	}
	if (ret) {
		for (i = 0; i < 4; i++)
			wacom->plat_data->img_version_of_ic[i] = 0;
		goto err_update_fw;
	}

out_update_fw:
	wacom_unload_fw(wacom);
skip_update_fw:
	ret = wacom_check_ub(wacom);
	if (ret < 0) {
		wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_AOP);
		input_info(true, wacom->dev, "Change mode for garage scan\n");
	}

	return 0;

err_update_fw:
	wacom_unload_fw(wacom);
	ret = wacom_check_ub(wacom);
	if (ret < 0) {
		wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_GARAGE_AOP);
		input_info(true, wacom->dev, "Change mode for garage scan\n");
	}

	return -1;
}

/* epen_disable_mode
 * 0 : wacom ic on
 * 1 : wacom ic off
 */
void wacom_disable_mode(struct wacom_data *wacom, wacom_disable_mode_t mode)
{
	if (mode == WACOM_DISABLE) {
		input_info(true, wacom->dev, "%s: power off\n", __func__);
		wacom->epen_blocked = mode;

		wacom_enable_irq(wacom, false);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
		wacom_power(wacom, false);

		wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
		wacom->function_result &= ~EPEN_EVENT_SURVEY;

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_RELEASE, NULL);
		wacom->is_tsp_block = false;
#endif
		wacom_forced_release(wacom);
	} else if (mode == WACOM_ENABLE) {
		input_info(true, wacom->dev, "%s: power on\n", __func__);

		wacom_power(wacom, true);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
		sec_delay(100);

		wacom_enable_irq(wacom, true);
		wacom->epen_blocked = mode;

		if (atomic_read(&wacom->screen_on))
			wacom_wakeup_sequence(wacom);
		else
			wacom_sleep_sequence(wacom);
	}
	input_info(true, wacom->dev, "%s: done %d!\n", __func__, mode);
}

void wacom_swap_compensation(struct wacom_data *wacom, char table_type)
{
	char data;
	int ret;

	input_info(true, wacom->dev, "%s: %d\n", __func__, table_type);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s : reset is on going, return\n", __func__);
		return;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev,
				"%s: powered off\n", __func__);
		return;
	}

	mutex_lock(&wacom->table_compensation_lock);

	data = COM_SAMPLERATE_STOP;
	ret = wacom_send_sel(wacom, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, wacom->dev, "%s: failed to send stop cmd %d\n",
				__func__, ret);
		wacom->reset_flag = true;
		mutex_unlock(&wacom->table_compensation_lock);
		return;
	}
	wacom->samplerate_state = false;
	sec_delay(50);

	switch (table_type) {
	case WACOM_TABLE_TYPE_NORMAL:
		data = COM_NORMAL_COMPENSATION;
		break;
	case WACOM_TABLE_TYPE_SPECIAL_1:
		data = COM_SPECIAL_COMPENSATION_1;
		break;
	case WACOM_TABLE_TYPE_SPECIAL_2:
		data = COM_SPECIAL_COMPENSATION_2;
		break;
	case WACOM_TABLE_TYPE_SPECIAL_3:
		data = COM_SPECIAL_COMPENSATION_3;
		break;
	default:
		input_err(true, wacom->dev,
				  "%s: get wrong table_type %d\n",
				  __func__, table_type);
	}

	ret = wacom_send_sel(wacom, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, wacom->dev,
			  "%s: failed to send table swap cmd %d\n",
			  __func__, ret);
		wacom->reset_flag = true;
		mutex_unlock(&wacom->table_compensation_lock);
		return;
	}

	data = COM_SAMPLERATE_STOP;
	ret = wacom_send_sel(wacom, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, wacom->dev,
			  "%s: failed to send stop cmd %d\n",
			  __func__, ret);
		wacom->reset_flag = true;
		mutex_unlock(&wacom->table_compensation_lock);
		return;
	}

	sec_delay(50);

	data = COM_SAMPLERATE_START;
	ret = wacom_send_sel(wacom, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, wacom->dev,
				"%s: failed to send start cmd, %d\n",
				__func__, ret);
		wacom->reset_flag = true;
		mutex_unlock(&wacom->table_compensation_lock);
		return;
	}
	wacom->samplerate_state = true;
	mutex_unlock(&wacom->table_compensation_lock);
}

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
static int wacom_notifier_call(struct notifier_block *n, unsigned long data, void *v)
{
	struct wacom_data *wacom = container_of(n, struct wacom_data, nb);

	switch (data) {
	case NOTIFIER_SECURE_TOUCH_ENABLE:
		wacom_disable_mode(wacom, WACOM_DISABLE);
		break;
	case NOTIFIER_SECURE_TOUCH_DISABLE:
		wacom_disable_mode(wacom, WACOM_ENABLE);
		if (wacom->pdct_lock_fail) {
			wacom_pdct_survey_mode(wacom);
			wacom->pdct_lock_fail = false;
		}
		break;
	default:
		break;
	}

	return 0;
}
#endif

const char *wacom_fwe_str = "wacom_fwe";
const char *wacom_pdct_str = "wacom_pdct";
const char *wacom_esd_detect_str = "wacom_esd_detect";
static int wacom_request_gpio(struct wacom_data *wacom)
{
	int ret;

	ret = devm_gpio_request(wacom->dev, wacom->fwe_gpio, wacom_fwe_str);
	if (ret) {
		input_err(true, wacom->dev, "unable to request gpio for fwe\n");
		return ret;
	}

	if (wacom->use_garage) {
		ret = devm_gpio_request(wacom->dev, wacom->pdct_gpio, wacom_pdct_str);
		if (ret) {
			input_err(true, wacom->dev, "unable to request gpio for pdct\n");
			return ret;
		}
	}

	if (gpio_is_valid(wacom->esd_detect_gpio)) {
		ret = devm_gpio_request(wacom->dev, wacom->esd_detect_gpio, wacom_esd_detect_str);
		if (ret) {
			input_err(true, wacom->dev, "unable to request gpio for esd_detect\n");
			return ret;
		}
	}
	return 0;
}

int wacom_check_ub(struct wacom_data *wacom)
{
	int lcdtype = 0;

	lcdtype = sec_input_get_lcd_id(wacom->dev);
	if (lcdtype == 0xFFFFFF) {
		input_info(true, wacom->dev, "lcd is not attached\n");
		return -ENODEV;
	}

	input_info(true, wacom->dev, "%s: lcdtype 0x%08x\n", __func__, lcdtype);

	return lcdtype;
}

void wacom_dev_remove(struct wacom_data *wacom)
{
	g_wacom = NULL;
	wacom->probe_done = false;
	wacom->plat_data->enable = NULL;
	wacom->plat_data->disable = NULL;

	input_info(true, wacom->dev, "%s called!\n", __func__);

	if (gpio_is_valid(wacom->esd_detect_gpio))
		cancel_delayed_work_sync(&wacom->esd_reset_work);
	cancel_delayed_work_sync(&wacom->open_test_dwork);
	cancel_delayed_work_sync(&wacom->work_print_info);

	wacom_enable_irq(wacom, false);

	atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
	wacom_power(wacom, false);
	wakeup_source_unregister(wacom->wacom_esd_ws);
	wakeup_source_unregister(wacom->wacom_ws);
	wakeup_source_unregister(wacom->plat_data->sec_ws);

	mutex_destroy(&wacom->i2c_mutex);
	mutex_destroy(&wacom->irq_lock);
	mutex_destroy(&wacom->update_lock);
	mutex_destroy(&wacom->lock);
	mutex_destroy(&wacom->mode_lock);
	mutex_destroy(&wacom->ble_lock);
	mutex_destroy(&wacom->ble_charge_mode_lock);

	wacom_sec_remove(wacom);

	i2c_unregister_device(wacom->client_boot);

	input_info(true, wacom->dev, "%s Done\n", __func__);
}

static void wacom_camera_check_work(struct work_struct *work)
{
	struct wacom_data *wacom = container_of(work, struct wacom_data, work_camera_check.work);
	int tmp = 0, ret = 0;
	static int retry;

	input_info(true, wacom->dev, "%s : start!(%d/%d)\n", __func__, wacom->probe_done, retry);

	if (wacom->probe_done == false) {
		if (retry++ < 20)
			schedule_delayed_work(&wacom->work_camera_check, msecs_to_jiffies(1000));	// 1 sec
		else
			input_err(true, wacom->dev, "%s : retry(%d) over\n", __func__, retry);
		return;
	}

	wacom->probe_done = false;
	cancel_delayed_work_sync(&wacom->open_test_dwork);

	tmp = wacom->plat_data->bringup;
	wacom->plat_data->bringup = 3;

	wacom_enable_irq(wacom, false);
	ret =  wacom_fw_update_on_probe(wacom);
	if (ret < 0)
		input_err(true, wacom->dev, "%s : update fail!\n", __func__);
	sec_delay(250);
	wacom_enable_irq(wacom, true);
	sec_delay(10);

	wacom->plat_data->bringup = tmp;
	wacom->probe_done = true;

	// set default setting
	wacom_enable(wacom->plat_data->dev);

	input_info(true, wacom->dev, "%s : end!\n", __func__);
}

static int wacom_get_camera_type_notify(struct notifier_block *nb, unsigned long action, void *data)
{
	struct wacom_data *wacom = container_of(nb, struct wacom_data, nb_camera);
	u8 tele_cam, wide_cam;
	u16 result = 0;

	if (!wacom->fw_separate_by_camera) {
		input_info(true, wacom->dev, "%s : not support notify val(%ld)\n", __func__, action);
		return 0;
	}

	input_info(true, wacom->dev, "%s: idx(#%d) fw ver(%02X%02X%02X%02X) // notify val(0x%08lx)\n",
			__func__, wacom->fw_bin_idx,
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3],  action);

	if (wacom->nb_camera_call_cnt++ > 0) {
		input_info(true, wacom->dev, "%s : duplicate call & skip(%d)!\n",
					__func__, wacom->nb_camera_call_cnt);
		return 0;
	}

	tele_cam = (action >> 24) & 0xFF;
	wide_cam = action & 0xFF;

	if (wacom->plat_data->img_version_of_ic[1] == WEZ02_PA3) {
		if (wacom->slavepcb == NA_MMW || wacom->slavepcb == JPN_MMW) {
			if (tele_cam == RT_SUNNY) {
				result = FW_INDEX_0;
			} else if (tele_cam == RT_SEC) {
				result = FW_INDEX_2;
			} else {
				input_info(true, wacom->dev, "%s : not matched(tele:%d,wide:%d) skip!\n",
							__func__, tele_cam, wide_cam);
				return 0;
			}
		} else {
			if (tele_cam == RT_SUNNY) {
				result = FW_INDEX_1;
			} else if (tele_cam == RT_SEC) {
				result = FW_INDEX_3;
			} else {
				input_info(true, wacom->dev, "%s : not matched(tele:%d,wide:%d) skip!\n",
							__func__, tele_cam, wide_cam);
				return 0;
			}
		}

	} else {
		input_err(true, wacom->dev, "%s : not matched model\n", __func__);
		return 0;
	}

	if (wacom->fw_bin_idx == result) {
		input_info(true, wacom->dev, "%s : type matched & skip!\n", __func__);
		return 0;
	}

	input_info(true, wacom->dev, "%s : mismatched & call workqueue!\n", __func__);
	wacom->fw_bin_idx = result;
	schedule_delayed_work(&wacom->work_camera_check, msecs_to_jiffies(10));

	return 0;
}

#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
static void wacom_swap_work(struct work_struct *work)
{
	struct wacom_data *wacom = container_of(work, struct wacom_data, nb_swap_work);

	wacom_swap_compensation(wacom, wacom->table_type);
	input_info(true, wacom->dev, "%s: cover attached: %x\n", __func__, wacom->table_type);
}

static int wacom_hall_ic_notify(struct notifier_block *nb,
			unsigned long hall_wacom, void *v)
{
	struct hall_notifier_context *hall_notifier;
	struct wacom_data *wacom = container_of(nb, struct wacom_data, nb_h);

	hall_notifier = v;

	if (strncmp(hall_notifier->name, "back_cover", 9) != 0)
		return 0;

	if (hall_notifier->value)
		wacom->table_type = WACOM_TABLE_TYPE_MPP_COVER;
	else
		wacom->table_type = WACOM_TABLE_TYPE_NORMAL;

	input_info(true, wacom->dev, "%s: name: %s, value: %s, type: %x\n", __func__,
		hall_notifier->name, hall_notifier->value ? "CLOSE" : "OPEN", wacom->table_type);

	schedule_work(&wacom->nb_swap_work);

	return 0;
}
#endif

static int wacom_hw_init(struct wacom_data *wacom)
{
	u16 firmware_index = 0;

	/* Power on */
	if (gpio_get_value(wacom->fwe_gpio) == 1) {
		input_info(true, wacom->dev, "%s: fwe gpio is high, change low and reset device\n", __func__);
		wacom_compulsory_flash_mode(wacom, false);
		wacom_reset_hw(wacom);
		sec_delay(200);
	} else {
		wacom_compulsory_flash_mode(wacom, false);
		wacom_power(wacom, true);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
		if (!wacom->plat_data->regulator_boot_on)
			sec_delay(100);
	}

	atomic_set(&wacom->screen_on, 1);

	wacom_query(wacom);

	if (wacom->fw_separate_by_camera) {
		if (wacom->plat_data->img_version_of_ic[2] == 0 && wacom->plat_data->img_version_of_ic[3] == 0) {
			wacom->fw_bin_idx = wacom->fw_bin_default_idx;
			input_err(true, wacom->dev, "%s: ic ver(%02X%02X%02X%02X) default idx(%d)\n",
						__func__, wacom->plat_data->img_version_of_ic[0],
						wacom->plat_data->img_version_of_ic[1],
						wacom->plat_data->img_version_of_ic[2],
						wacom->plat_data->img_version_of_ic[3], wacom->fw_bin_idx);
		} else {
			firmware_index = (wacom->plat_data->img_version_of_ic[2] & 0x0F);
			switch (firmware_index) {
			case PA3_SUNNY_MMW://wez02_pa3_mmw_c0.bin 40XX
				wacom->fw_bin_idx = FW_INDEX_0;
				break;
			case PA3_SUNNY://wez02_pa3_c0.bin 41XX
				wacom->fw_bin_idx = FW_INDEX_1;
				break;
			case PA3_SEC_MMW://wez02_pa3_mmw_c1.bin 42XX
				wacom->fw_bin_idx = FW_INDEX_2;
				break;
			case PA3_SEC://wez02_pa3_c1.bin 43XX
				wacom->fw_bin_idx = FW_INDEX_3;
				break;
			default:
				wacom->fw_bin_idx = wacom->fw_bin_default_idx;
				input_err(true, wacom->dev, "%s: fw idx is not matched(0x%x) & set default\n",
						__func__, firmware_index);
			}

			input_info(true, wacom->dev, "%s: idx(#%d/0x%X) : ic ver(%02X%02X%02X%02X)\n",
					__func__, wacom->fw_bin_idx, firmware_index,
					wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
					wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3]);
		}
	}

	INIT_DELAYED_WORK(&wacom->work_camera_check, wacom_camera_check_work);
	wacom->nb_camera.notifier_call = wacom_get_camera_type_notify;
	g_nb_wac_camera = &wacom->nb_camera;

	return wacom_fw_update_on_probe(wacom);
}

int wacom_probe(struct wacom_data *wacom)
{
	int ret = 0;

	input_info(true, wacom->dev, "%s\n", __func__);

	ret = wacom_hw_init(wacom);
	if (ret < 0) {
		input_err(true, wacom->dev, "%s: fail to init hw\n", __func__);
		g_nb_wac_camera = NULL;
		wacom_dev_remove(wacom);
		return ret;
	}

	wacom_input_init(wacom);
		/*Request IRQ */
	ret = devm_request_threaded_irq(wacom->dev, wacom->irq, NULL, wacom_interrupt,
			IRQF_ONESHOT | IRQF_TRIGGER_LOW, "sec_epen_irq", wacom);
	if (ret < 0) {
		input_err(true, wacom->dev, "failed to request irq(%d) - %d\n", wacom->irq, ret);
		wacom_dev_remove(wacom);
		return ret;
	}

	if (wacom->use_garage) {
		ret = devm_request_threaded_irq(wacom->dev, wacom->irq_pdct, NULL, wacom_interrupt_pdct,
				IRQF_ONESHOT | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"sec_epen_pdct", wacom);
		if (ret < 0) {
			input_err(true, wacom->dev, "failed to request pdct irq(%d) - %d\n", wacom->irq_pdct, ret);
			wacom_dev_remove(wacom);
			return ret;
		}
		input_info(true, wacom->dev, "init pdct %d\n", wacom->irq_pdct);
	}

	if (wacom->use_garage) {
		wacom->irq_workqueue_pdct = create_singlethread_workqueue("irq_wq_pdct");
		if (!IS_ERR_OR_NULL(wacom->irq_workqueue_pdct)) {
			INIT_WORK(&wacom->irq_work_pdct, wacom_handler_wait_resume_pdct_work);
			input_info(true, wacom->dev, "%s: set wacom_handler_wait_resume_pdct_work\n", __func__);
		} else {
			input_err(true, wacom->dev, "%s: failed to create irq_workqueue_pdct, err: %ld\n",
						__func__, PTR_ERR(wacom->irq_workqueue_pdct));
		}
	}

	if (gpio_is_valid(wacom->esd_detect_gpio)) {
		INIT_DELAYED_WORK(&wacom->esd_reset_work, wacom_esd_reset_work);
		ret = devm_request_threaded_irq(wacom->dev, wacom->irq_esd_detect, NULL,
				wacom_esd_detect_interrupt, IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
				"sec_epen_esd_detectirq", wacom);
		if (ret < 0)
			input_err(true, wacom->dev, "failed to request esd_detect_irq(%d) - %d\n",
					wacom->irq_esd_detect, ret);
	}

	input_info(true, wacom->dev, "init irq %d\n", wacom->irq);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	sec_input_register_notify(&wacom->nb, wacom_notifier_call, 2);
#endif
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
	if (wacom->support_hall_notifier) {
		input_info(true, wacom->dev, "%s: support_hall_notifier: %d\n",
			__func__, wacom->support_hall_notifier);
		wacom->nb_h.priority = 1;
		wacom->nb_h.notifier_call = wacom_hall_ic_notify;
		hall_notifier_register(&wacom->nb_h);
		INIT_WORK(&wacom->nb_swap_work, wacom_swap_work);
	}
#endif
	wacom->ble_hist = devm_kzalloc(wacom->dev, WACOM_BLE_HISTORY_SIZE, GFP_KERNEL);
	if (!wacom->ble_hist)
		input_err(true, wacom->dev, "failed to history mem\n");

	wacom->ble_hist1 = devm_kzalloc(wacom->dev, WACOM_BLE_HISTORY1_SIZE, GFP_KERNEL);
	if (!wacom->ble_hist1)
		input_err(true, wacom->dev, "failed to history1 mem\n");

	probe_open_test(wacom);
	g_wacom = wacom;
	wacom->probe_done = true;

#ifdef SUPPORT_STANDBY_MODE
	// wacom ic default mode : WACOM_LOW_SCAN_RATE
	wacom->standby_state = WACOM_LOW_SCAN_RATE;
#endif

	input_info(true, wacom->dev, "probe done\n");
	input_log_fix();

	sec_cmd_send_event_to_user(&wacom->sec, NULL, "RESULT=PROBE_DONE");
	return 0;
}

static int wacom_parse_dt(struct device *dev, struct wacom_data *wacom)
{
	struct device_node *np = dev->of_node;
	u32 tmp[5] = { 0, };
	int ret = 0;

	if (!np) {
		input_err(true, dev, "%s: of_node is not exist\n", __func__);
		return -ENODEV;
	}

	wacom->fwe_gpio = of_get_named_gpio(np, "wacom,fwe-gpio", 0);
	if (!gpio_is_valid(wacom->fwe_gpio)) {
		input_err(true, dev, "failed to get fwe-gpio\n");
		return -EINVAL;
	}

	wacom->pdct_gpio = of_get_named_gpio(np, "wacom,pdct-gpio", 0);
	if (!gpio_is_valid(wacom->pdct_gpio)) {
		input_err(true, dev, "failed to get pdct-gpio, not support garage\n");
	} else {
		wacom->use_garage = true;
		ret = of_property_read_u32(np, "wacom,support_garage_open_test", &wacom->support_garage_open_test);
		if (ret) {
			input_err(true, dev, "failed to read support_garage_open_test %d\n", ret);
			wacom->support_garage_open_test = 0;
		}
	}

	wacom->esd_detect_gpio = of_get_named_gpio(np, "wacom,esd_detect_gpio", 0);
	if (!gpio_is_valid(wacom->esd_detect_gpio))
		input_err(true, dev, "failed to get esd_detect_gpio\n");
	else
		input_info(true, dev, "using esd_detect_gpio\n");

	/* get features */
	ret = of_property_read_u32(np, "wacom,boot_addr", tmp);
	if (ret < 0) {
		input_err(true, dev, "failed to read boot address %d\n", ret);
		return -EINVAL;
	}
	wacom->boot_addr = tmp[0];

	wacom->use_dt_coord = of_property_read_bool(np, "wacom,use_dt_coord");

	ret = of_property_read_u32(np, "wacom,max_pressure", tmp);
	if (ret != -EINVAL) {
		if (ret)
			input_err(true, dev, "failed to read max pressure %d\n", ret);
		else
			wacom->max_pressure = tmp[0];
	}

	ret = of_property_read_u32_array(np, "wacom,max_tilt", tmp, 2);
	if (ret != -EINVAL) {
		if (ret) {
			input_err(true, dev, "failed to read max x tilt %d\n", ret);
		} else {
			wacom->max_x_tilt = tmp[0];
			wacom->max_y_tilt = tmp[1];
		}
	}

	ret = of_property_read_u32(np, "wacom,max_height", tmp);
	if (ret != -EINVAL) {
		if (ret)
			input_err(true, dev, "failed to read max height %d\n", ret);
		else
			wacom->max_height = tmp[0];
	}

	ret = of_property_read_u32(np, "wacom,fw_separate_by_camera", &wacom->fw_separate_by_camera);
	if (ret != -EINVAL) {
		if (ret) {
			wacom->fw_separate_by_camera = 0;
			input_err(true, dev, "failed to read fw_separate_by_camera %d\n", ret);
		}
	}
	input_info(true, dev, "%s : wacom,fw_separate_by_camera(%d)\n",
			__func__, wacom->fw_separate_by_camera);

	if (wacom->fw_separate_by_camera) {
		if (wacom->slavepcb == NA_MMW || wacom->slavepcb == JPN_MMW) {
			wacom->fw_bin_default_idx = FW_INDEX_0;
		} else {
			wacom->fw_bin_default_idx = FW_INDEX_1;
		}
		input_info(true, dev, "%s : wacom,fw_default_idx(%d) & read fw name later\n",
					__func__, wacom->fw_bin_default_idx);
	}

	ret = of_property_read_u32(np, "wacom,module_ver", &wacom->module_ver);
	if (ret) {
		input_err(true, dev, "failed to read module_ver %d\n", ret);
		/* default setting to open test */
		wacom->module_ver = 1;
	}

	of_property_read_u32(np, "wacom,support_hall_notifier", &wacom->support_hall_notifier);
	of_property_read_u32(np, "wacom,support_sensor_hall", &wacom->support_sensor_hall);

	input_info(true, dev,
			"boot_addr: 0x%X, max_pressure: %d, max_height: %d, max_tilt: (%d,%d), "
			"module_ver:%d %s%s%s\n",
			wacom->boot_addr, wacom->max_pressure, wacom->max_height,
			wacom->max_x_tilt, wacom->max_y_tilt,
			wacom->module_ver, wacom->use_garage ? ", use garage" : "",
			wacom->support_garage_open_test ? ", support garage open test" : "",
			wacom->support_hall_notifier ? ", support hall noti" : "");

	return 0;
}

int wacom_init(struct wacom_data *wacom)
{
	int ret = 0;

	if (!wacom)
		return -EINVAL;

	input_info(true, wacom->dev, "%s\n", __func__);

	ret = wacom_parse_dt(wacom->dev, wacom);
	if (ret < 0) {
		input_err(true, wacom->dev, "failed to parse dt\n");
		return ret;
	}

	wacom->plat_data->dev = wacom->dev;
	wacom->plat_data->irq = wacom->irq;
	//wacom->plat_data->pinctrl_configure = sec_input_pinctrl_configure;
	//wacom->plat_data->power = sec_input_power;
	//wacom->plat_data->start_device;
	//wacom->plat_data->stop_device;
	//wacom->plat_data->init;
	//wacom->plat_data->lpmode;

	if (wacom->use_garage) {
		wacom->irq_pdct = gpio_to_irq(wacom->pdct_gpio);
		wacom->pen_pdct = PDCT_NOSIGNAL;
	}

	ret = wacom_request_gpio(wacom);
	if (ret) {
		input_err(true, wacom->dev, "failed to request gpio\n");
		return ret;
	}

	if (gpio_is_valid(wacom->esd_detect_gpio))
		wacom->irq_esd_detect = gpio_to_irq(wacom->esd_detect_gpio);

	wacom->fw_img = NULL;
	wacom->fw_update_way = FW_NONE;
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	wacom->tsp_scan_mode = DISABLE_TSP_SCAN_BLCOK;
#endif
	wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
	wacom->function_result = EPEN_EVENT_PEN_OUT;
	/* Consider about factory, it may be need to as default 1 */
#if WACOM_SEC_FACTORY
	wacom->battery_saving_mode = true;
#else
	wacom->battery_saving_mode = false;
#endif
	wacom->reset_flag = false;
	wacom->pm_suspend = false;
	wacom->samplerate_state = true;
	wacom->update_status = FW_UPDATE_PASS;
	wacom->reset_is_on_going = false;
	wacom->esd_continuous_reset_count = 0;
	wacom->esd_reset_blocked_enable = false;
	wacom->esd_packet_count = 0;
	wacom->esd_irq_count = 0;
	wacom->esd_max_continuous_reset_count = 0;

	init_completion(&wacom->i2c_done);
	complete_all(&wacom->i2c_done);
	// for irq
	init_completion(&wacom->plat_data->resume_done);
	complete_all(&wacom->plat_data->resume_done);

	/*Initializing for semaphor */
	mutex_init(&wacom->i2c_mutex);
	mutex_init(&wacom->lock);
	mutex_init(&wacom->update_lock);
	mutex_init(&wacom->irq_lock);
	mutex_init(&wacom->mode_lock);
	mutex_init(&wacom->ble_lock);
	mutex_init(&wacom->ble_charge_mode_lock);
	mutex_init(&wacom->table_compensation_lock);

	INIT_DELAYED_WORK(&wacom->work_print_info, wacom_print_info_work);

	mutex_init(&wacom->plat_data->enable_mutex);
	wacom->plat_data->enable = wacom_enable;
	wacom->plat_data->disable = wacom_disable;

	ret = wacom_sec_fn_init(wacom);
	if (ret) {
		wacom->plat_data->enable = NULL;
		wacom->plat_data->disable = NULL;
		return ret;
	}

	input_info(true, wacom->dev, "%s: fn_init\n", __func__);

	wacom->plat_data->sec_ws = wakeup_source_register(NULL, "wacom_wakelock");
	// for pdct
	wacom->wacom_ws = wakeup_source_register(NULL, "wacom_wakelock_pdct");
	wacom->wacom_esd_ws = wakeup_source_register(NULL, "wacom_esd_wakelock");

	input_info(true, wacom->dev, "%s: init resource\n", __func__);

	return 0;
}
