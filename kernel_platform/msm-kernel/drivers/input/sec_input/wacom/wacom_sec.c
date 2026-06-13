/*
 *  wacom_func.c - Wacom G5 Digitizer Controller (I2C bus)
 *
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

#include "wacom_dev.h"

static inline int power(int num)
{
	int i, ret;

	for (i = 0, ret = 1; i < num; i++)
		ret = ret * 10;

	return ret;
}

#if WACOM_SEC_FACTORY
static ssize_t epen_fac_select_firmware_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	u8 fw_update_way = FW_NONE;
	int ret = 0;

	input_info(true, wacom->dev, "%s\n", __func__);
	mutex_lock(&wacom->lock);
	switch (*buf) {
	case 'k':
	case 'K':
		fw_update_way = FW_BUILT_IN;
		break;
	case 't':
	case 'T':
		fw_update_way = FW_FACTORY_GARAGE;
		break;
	case 'u':
	case 'U':
		fw_update_way = FW_FACTORY_UNIT;
		break;
	default:
		input_err(true, wacom->dev, "wrong parameter\n");
		goto out;
	}
	ret = wacom_load_fw(wacom, fw_update_way);
	if (ret < 0) {
		input_info(true, wacom->dev, "failed to load fw data\n");
		goto out;
	}

	wacom_unload_fw(wacom);
out:
	mutex_unlock(&wacom->lock);
	return count;
}
#endif

static ssize_t epen_firm_update_status_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int status = wacom->update_status;
	int ret = 0;

	input_info(true, wacom->dev, "%s:(%d)\n", __func__, status);

	if (status == FW_UPDATE_PASS)
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	else if (status == FW_UPDATE_RUNNING)
		ret = snprintf(buf, PAGE_SIZE, "DOWNLOADING\n");
	else if (status == FW_UPDATE_FAIL)
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	else
		ret = 0;

	return ret;
}

static ssize_t epen_firm_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);

	input_info(true, wacom->dev, "%s: %02X%02X%02X%02X|%02X%02X%02X%02X\n", __func__,
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3],
			wacom->plat_data->img_version_of_bin[0], wacom->plat_data->img_version_of_bin[1],
			wacom->plat_data->img_version_of_bin[2], wacom->plat_data->img_version_of_bin[3]);

	return snprintf(buf, PAGE_SIZE, "%02X%02X%02X%02X\t%02X%02X%02X%02X\n",
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3],
			wacom->plat_data->img_version_of_bin[0], wacom->plat_data->img_version_of_bin[1],
			wacom->plat_data->img_version_of_bin[2], wacom->plat_data->img_version_of_bin[3]);
}

static ssize_t epen_firm_update_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	u8 fw_update_way = FW_NONE;

	input_info(true, wacom->dev, "%s\n", __func__);

	mutex_lock(&wacom->lock);

	switch (*buf) {
	case 'i':
	case 'I':
#if WACOM_PRODUCT_SHIP
		fw_update_way = FW_IN_SDCARD_SIGNED;
#else
		fw_update_way = FW_IN_SDCARD;
#endif
		break;
	case 'k':
	case 'K':
		fw_update_way = FW_BUILT_IN;
		break;
#if WACOM_SEC_FACTORY
	case 't':
	case 'T':
		fw_update_way = FW_FACTORY_GARAGE;
		break;
	case 'u':
	case 'U':
		fw_update_way = FW_FACTORY_UNIT;
		break;
#endif
	default:
		input_err(true, wacom->dev, "wrong parameter\n");
		mutex_unlock(&wacom->lock);
		return count;
	}

	wacom_fw_update_on_hidden_menu(wacom, fw_update_way);
	mutex_unlock(&wacom->lock);

	return count;
}

static ssize_t ver_of_ic_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);

	input_info(true, wacom->dev, "%s: %02X%02X%02X%02X\n", __func__,
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3]);

	return snprintf(buf, PAGE_SIZE, "%02X%02X%02X%02X",
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3]);
}

static ssize_t ver_of_bin_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);

	input_info(true, wacom->dev, "%s: %02X%02X%02X%02X\n", __func__,
			wacom->plat_data->img_version_of_bin[0], wacom->plat_data->img_version_of_bin[1],
			wacom->plat_data->img_version_of_bin[2], wacom->plat_data->img_version_of_bin[3]);

	return snprintf(buf, PAGE_SIZE, "%02X%02X%02X%02X",
			wacom->plat_data->img_version_of_bin[0], wacom->plat_data->img_version_of_bin[1],
			wacom->plat_data->img_version_of_bin[2], wacom->plat_data->img_version_of_bin[3]);
}


static ssize_t epen_reset_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int val;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

	val = !(!val);
	if (!val)
		goto out;

	wacom_enable_irq(wacom, false);

	wacom->function_result &= ~EPEN_EVENT_SURVEY;
	wacom->survey_mode = EPEN_SURVEY_MODE_NONE;

	/* Reset IC */
	wacom_reset_hw(wacom);
	/* I2C Test */
	wacom_query(wacom);

	wacom_enable_irq(wacom, true);

	input_info(true, wacom->dev,
			"%s: result %d\n", __func__, wacom->query_status);
	return count;

out:
	input_err(true, wacom->dev, "%s: invalid value %d\n", __func__, val);

	return count;
}

static ssize_t epen_reset_result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int ret = 0;

	if (wacom->query_status) {
		input_info(true, wacom->dev, "%s: PASS\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	} else {
		input_err(true, wacom->dev, "%s: FAIL\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	}

	return ret;
}

static ssize_t epen_checksum_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int val;

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev, "%s: power off state, skip!\n",
				__func__);
		return count;
	}

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe_done yet, skip!\n", __func__);
		return count;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		return count;
	}

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

	val = !(!val);
	if (!val)
		goto out;

	wacom_enable_irq(wacom, false);

	wacom_checksum(wacom);

	wacom_enable_irq(wacom, true);

	input_info(true, wacom->dev,
			"%s: result %d\n", __func__, wacom->checksum_result);

	return count;

out:
	input_err(true, wacom->dev, "%s: invalid value %d\n", __func__, val);

	return count;
}

static ssize_t epen_checksum_result_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int ret;

	if (wacom->checksum_result) {
		input_info(true, wacom->dev, "checksum, PASS\n");
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	} else {
		input_err(true, wacom->dev, "checksum, FAIL\n");
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	}

	return ret;
}

int wacom_open_test(struct wacom_data *wacom, int test_mode)
{
	u8 cmd = 0;
	u8 buf[COM_COORD_NUM + 1] = { 0, };
	int ret = 0, retry = 0, retval = 0;
	int retry_int = 30;

	input_info(true, wacom->dev, "%s : start (%d)\n", __func__, test_mode);

	if (test_mode == WACOM_GARAGE_TEST) {
		wacom->garage_connection_check = false;
		wacom->garage_fail_channel = 0;
		wacom->garage_min_adc_val = 0;
		wacom->garage_error_cal = 0;
		wacom->garage_min_cal_val = 0;
	} else if (test_mode == WACOM_DIGITIZER_TEST) {
		wacom->connection_check = false;
		wacom->fail_channel = 0;
		wacom->min_adc_val = 0;
		wacom->error_cal = 0;
		wacom->min_cal_val = 0;
	}

	if (!wacom->support_garage_open_test && test_mode == WACOM_GARAGE_TEST) {
		input_err(true, wacom->dev, "%s: not support garage open test", __func__);
		retval = EPEN_OPEN_TEST_NOTSUPPORT;
		goto err1;
	}

	/* change normal mode for open test */
	mutex_lock(&wacom->mode_lock);
	wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_NONE);
	// prevent I2C fail
	sec_delay(300);
	mutex_unlock(&wacom->mode_lock);

	ret = wacom_start_stop_cmd(wacom, WACOM_STOP_CMD);
	if (ret != 1) {
		input_err(true, wacom->dev, "%s: failed to set stop cmd, %d\n",
				__func__, ret);
		retval = EPEN_OPEN_TEST_EIO;
		goto err1;
	}

	sec_delay(50);

	wacom_enable_irq(wacom, false);

	retry = 3;
	do {
		if (gpio_get_value(wacom->plat_data->irq_gpio))
			break;

		input_info(true, wacom->dev, "%s : irq check, retry %d\n", __func__, retry);
		ret = wacom_recv(wacom, buf, COM_COORD_NUM);
		if (ret != COM_COORD_NUM)
			input_err(true, wacom->dev, "%s: failed to recv status data\n", __func__);
		sec_delay(50);
	} while (retry--);

	if (test_mode == WACOM_GARAGE_TEST)
		cmd = COM_GARAGE_TEST_MODE;
	else if (test_mode == WACOM_DIGITIZER_TEST)
		cmd = COM_DIGITIZER_TEST_MODE;

	if (wacom->support_garage_open_test) {
		input_info(true, wacom->dev, "%s : send data(%02x)\n", __func__, cmd);

		ret = wacom_send(wacom, &cmd, 1);
		if (ret != 1) {
			input_err(true, wacom->dev, "%s : failed to send data(%02x)\n", __func__, cmd);
			retval = EPEN_OPEN_TEST_EIO;
			goto err2;
		}
		sec_delay(50);
	}

	cmd = COM_OPEN_CHECK_START;
	ret = wacom_send(wacom, &cmd, 1);
	if (ret != 1) {
		input_err(true, wacom->dev, "%s : failed to send data(%02x)\n", __func__, cmd);
		retval = EPEN_OPEN_TEST_EIO;
		goto err2;
	}

	sec_delay(100);

	retry = 5;
	cmd = COM_OPEN_CHECK_STATUS;
	do {
		input_info(true, wacom->dev, "%s : read status, retry %d\n", __func__, retry);
		ret = wacom_send(wacom, &cmd, 1);
		if (ret != 1) {
			input_err(true, wacom->dev,
					"%s : failed to send data(%02x)\n", __func__, cmd);
			sec_delay(6);
			continue;
		}

		retry_int = 30;
		while (retry_int--) {
			if (!gpio_get_value(wacom->plat_data->irq_gpio))
				break;
			sec_delay(20);
		}
		input_info(true, wacom->dev, "%s : retry_int[%d]\n", __func__, retry_int);

		ret = wacom_recv(wacom, buf, COM_COORD_NUM);
		if (ret != COM_COORD_NUM) {
			input_err(true, wacom->dev, "%s : failed to recv\n", __func__);
			sec_delay(6);
			continue;
		}

		if (buf[0] != 0x0E && buf[1] != 0x01) {
			input_err(true, wacom->dev,
					"%s : invalid packet(%02x %02x %02x)\n",
					__func__, buf[0], buf[1], buf[3]);
			continue;
		}
		/*
		 * status value
		 * 0 : data is not ready
		 * 1 : PASS
		 * 2 : Fail (coil function error)
		 * 3 : Fail (All coil function error)
		 */
		if (buf[2] == 1) {
			input_info(true, wacom->dev, "%s : Open check Pass\n", __func__);
			break;
		}
	} while (retry--);

	if (test_mode == WACOM_GARAGE_TEST) {
		if (ret == COM_COORD_NUM) {
			if (wacom->module_ver == 0x2)
				wacom->garage_connection_check = ((buf[2] == 1) && !buf[6]);
			else
				wacom->garage_connection_check = (buf[2] == 1);
		} else {
			wacom->garage_connection_check = false;
			retval = EPEN_OPEN_TEST_EIO;
			goto err2;
		}

		wacom->garage_fail_channel = buf[3];
		wacom->garage_min_adc_val = buf[4] << 8 | buf[5];
		wacom->garage_error_cal = buf[6];
		wacom->garage_min_cal_val = buf[7] << 8 | buf[8];

		input_info(true, wacom->dev,
				"%s: garage %s buf[3]:%d, buf[4]:%d, buf[5]:%d, buf[6]:%d, buf[7]:%d, buf[8]:%d\n",
				__func__, wacom->garage_connection_check ? "Pass" : "Fail",
				buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);

		if (!wacom->garage_connection_check)
			retval = EPEN_OPEN_TEST_FAIL;

	} else if (test_mode == WACOM_DIGITIZER_TEST) {
		if (ret == COM_COORD_NUM) {
			if (wacom->module_ver == 0x2)
				wacom->connection_check = ((buf[2] == 1) && !buf[6]);
			else
				wacom->connection_check = (buf[2] == 1);
		} else {
			wacom->connection_check = false;
			retval = EPEN_OPEN_TEST_EIO;
			goto err2;
		}

		wacom->fail_channel = buf[3];
		wacom->min_adc_val = buf[4] << 8 | buf[5];
		wacom->error_cal = buf[6];
		wacom->min_cal_val = buf[7] << 8 | buf[8];

		input_info(true, wacom->dev,
				"%s: digitizer %s buf[3]:%d, buf[4]:%d, buf[5]:%d, buf[6]:%d, buf[7]:%d, buf[8]:%d\n",
				__func__, wacom->connection_check ? "Pass" : "Fail",
				buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);

		if (!wacom->connection_check)
			retval = EPEN_OPEN_TEST_FAIL;
	}

err2:
	wacom_enable_irq(wacom, true);

err1:
	ret = wacom_start_stop_cmd(wacom, WACOM_STOP_AND_START_CMD);
	if (ret != 1) {
		input_err(true, wacom->dev, "%s: failed to set stop-start cmd, %d\n",
				__func__, ret);
		retval = EPEN_OPEN_TEST_EMODECHG;
		goto out;
	}

out:
	return retval;
}

static int get_connection_test(struct wacom_data *wacom, int test_mode)
{
	int retry = 2;
	int ret;

	if (wacom->reset_is_on_going) {
		input_info(true, wacom->dev, "%s : reset is on going!\n", __func__);
		return -EIO;
	}

	mutex_lock(&wacom->lock);

	input_info(true, wacom->dev, "%s : start(%d)\n", __func__, test_mode);

	wacom->is_open_test = true;

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	if (wacom->is_tsp_block) {
		input_info(true, wacom->dev, "%s: full scan OUT\n", __func__);
		wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_RELEASE, NULL);
		wacom->is_tsp_block = false;
	}
#endif

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_info(true, wacom->dev, "%s : force power on\n", __func__);
		wacom_power(wacom, true);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
		sec_delay(100);
		wacom_enable_irq(wacom, true);
	}

	while (retry--) {
		ret = wacom_open_test(wacom, test_mode);
		if (ret == EPEN_OPEN_TEST_PASS || ret == EPEN_OPEN_TEST_NOTSUPPORT)
			break;

		input_err(true, wacom->dev, "failed(%d). retry %d\n", ret, retry);

		wacom_enable_irq(wacom, false);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
		wacom_power(wacom, false);
		wacom->function_result &= ~EPEN_EVENT_SURVEY;
		wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
		/* recommended delay in spec */
		sec_delay(100);
		wacom_power(wacom, true);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
		sec_delay(100);
		wacom_enable_irq(wacom, true);
		if (ret == EPEN_OPEN_TEST_EIO || ret == EPEN_OPEN_TEST_FAIL)
			continue;
		else if (ret == EPEN_OPEN_TEST_EMODECHG)
			break;
	}

	if (!wacom->samplerate_state) {
		input_info(true, wacom->dev, "%s: samplerate state is %d, need to recovery\n",
				__func__, wacom->samplerate_state);

		ret = wacom_start_stop_cmd(wacom, WACOM_START_CMD);
		if (ret != 1) {
			input_err(true, wacom->dev, "%s: failed to set start cmd, %d\n",
					__func__, ret);
		}
	}

	wacom->is_open_test = false;

	mutex_unlock(&wacom->lock);

	input_info(true, wacom->dev, "connection_check : %s\n",
			wacom->connection_check ? "OK" : "NG");

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	if (wacom->is_tsp_block) {
		wacom->tsp_scan_mode = sec_input_notify(&wacom->nb, NOTIFIER_TSP_BLOCKING_RELEASE, NULL);
		wacom->is_tsp_block = false;
		input_err(true, wacom->dev, "%s : release tsp scan block\n", __func__);
	}
#endif
	return ret;
}

static ssize_t epen_connection_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int ret;

	ret = wacom_check_ub(wacom);
	if (ret < 0) {
		input_info(true, wacom->dev, "%s: digitizer is not attached\n", __func__);
		wacom->connection_check = false;
		goto out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		wacom->connection_check = false;
		goto out;
	}

	get_connection_test(wacom, WACOM_DIGITIZER_TEST);

out:
	if (wacom->module_ver == 0x2) {
		return snprintf(buf, PAGE_SIZE, "%s %d %d %d %s %d\n",
				wacom->connection_check ? "OK" : "NG",
				wacom->module_ver, wacom->fail_channel,
				wacom->min_adc_val, wacom->error_cal ? "NG" : "OK",
				wacom->min_cal_val);
	} else {
		return snprintf(buf, PAGE_SIZE, "%s %d %d %d\n",
				wacom->connection_check ? "OK" : "NG",
				wacom->module_ver, wacom->fail_channel,
				wacom->min_adc_val);
	}
}

static ssize_t epen_saving_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	static u64 call_count;
	int val;

	call_count++;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

	wacom->battery_saving_mode = !(!val);

	input_info(true, wacom->dev, "%s: ps %s & pen %s [%llu]\n",
			__func__, wacom->battery_saving_mode ? "on" : "off",
			(wacom->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
			call_count);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s : reset is on going, data saved and skip!\n", __func__);
		return count;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF) && wacom->battery_saving_mode) {
		input_err(true, wacom->dev,
				"%s: already power off, save & return\n", __func__);
		return count;
	}

	if (!(wacom->function_result & EPEN_EVENT_PEN_OUT)) {
		wacom_select_survey_mode(wacom, atomic_read(&wacom->screen_on));

		if (wacom->battery_saving_mode)
			forced_release_fullscan(wacom);
	}

	return count;
}

static ssize_t epen_insert_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int pen_state = (wacom->function_result & EPEN_EVENT_PEN_OUT);

	input_info(true, wacom->dev, "%s : pen is %s\n", __func__,
			pen_state ? "OUT" : "IN");

	return snprintf(buf, PAGE_SIZE, "%d\n", pen_state ? 0 : 1);
}

static ssize_t screen_off_memo_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

#if WACOM_SEC_FACTORY
	input_info(true, wacom->dev,
			"%s : Not support screen off memo mode(%d) in Factory Bin\n",
			__func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wacom->function_set |= EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO;
	else
		wacom->function_set &= (~EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO);

	input_info(true, wacom->dev,
			"%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
			wacom->battery_saving_mode ? "on" : "off",
			(wacom->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
			wacom->function_set, wacom->function_result);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, data saved and skip!\n",
				__func__);
		return count;
	}

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe is not done\n", __func__);
		return count;
	}

	if (!atomic_read(&wacom->screen_on))
		wacom_select_survey_mode(wacom, false);

	return count;
}

static ssize_t aod_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

#if WACOM_SEC_FACTORY
	input_info(true, wacom->dev,
			"%s : Not support aod mode(%d) in Factory Bin\n",
			__func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wacom->function_set |= EPEN_SETMODE_AOP_OPTION_AOD_LCD_OFF;
	else
		wacom->function_set &= ~EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON;

	input_info(true, wacom->dev,
			"%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
			wacom->battery_saving_mode ? "on" : "off",
			(wacom->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
			wacom->function_set, wacom->function_result);

	if (!atomic_read(&wacom->screen_on))
		wacom_select_survey_mode(wacom, false);

	return count;
}

static ssize_t aod_lcd_onoff_status_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

#if WACOM_SEC_FACTORY
	input_info(true, wacom->dev,
			"%s : Not support aod mode(%d) in Factory Bin\n",
			__func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wacom->function_set |= EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS;
	else
		wacom->function_set &= ~EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS;

	if (!(wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOD) || atomic_read(&wacom->screen_on))
		goto out;

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF) || (wacom->reset_is_on_going == true))
		goto out;

	if (wacom->survey_mode == EPEN_SURVEY_MODE_GARAGE_AOP) {
		if ((wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON)
				== EPEN_SETMODE_AOP_OPTION_AOD_LCD_OFF) {
			forced_release_fullscan(wacom);
		}

		mutex_lock(&wacom->mode_lock);
		wacom_set_survey_mode(wacom, wacom->survey_mode);
		mutex_unlock(&wacom->mode_lock);
	}

out:
	input_info(true, wacom->dev, "%s: screen %s, survey mode:0x%x, set:0x%x\n",
			__func__, atomic_read(&wacom->screen_on) ? "on" : "off",
			wacom->survey_mode, wacom->function_set);

	return count;
}

static ssize_t aot_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

#if WACOM_SEC_FACTORY
	input_info(true, wacom->dev,
			"%s : Not support aot mode(%d) in Factory Bin\n",
			__func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wacom->function_set |= EPEN_SETMODE_AOP_OPTION_AOT;
	else
		wacom->function_set &= (~EPEN_SETMODE_AOP_OPTION_AOT);

	input_info(true, wacom->dev,
			"%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
			wacom->battery_saving_mode ? "on" : "off",
			(wacom->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
			wacom->function_set, wacom->function_result);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, data saved and skip!\n",
				__func__);
		return count;
	}

	if (!atomic_read(&wacom->screen_on))
		wacom_select_survey_mode(wacom, false);

	return count;
}

static ssize_t epen_wcharging_mode_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);

	input_info(true, wacom->dev, "%s: %s\n", __func__,
			!wacom->wcharging_mode ? "NORMAL" : "LOWSENSE");

	return snprintf(buf, PAGE_SIZE, "%s\n",
			!wacom->wcharging_mode ? "NORMAL" : "LOWSENSE");
}

static ssize_t epen_wcharging_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int retval = 0;

	if (kstrtoint(buf, 0, &retval)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

	wacom->wcharging_mode = retval;

	input_info(true, wacom->dev, "%s: %d\n", __func__,
			wacom->wcharging_mode);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s : reset is on going, data saved and skip!\n", __func__);
		return count;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev,
				"%s: power off, save & return\n", __func__);
		return count;
	}

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe is not done\n", __func__);
		return count;
	}

	retval = wacom_set_sense_mode(wacom);
	if (retval < 0) {
		input_err(true, wacom->dev,
				"%s: do not set sensitivity mode, %d\n", __func__,
				retval);
	}

	return count;

}

char ble_charge_command[] = {
	COM_BLE_C_DISABLE,	/* Disable charge (charging mode enable) */
	COM_BLE_C_ENABLE,	/* Enable charge (charging mode disable) */
	COM_BLE_C_RESET,	/* Reset (make reset pattern + 1m charge) */
	COM_BLE_C_START,	/* Start (make start patter + 1m charge) */
	COM_BLE_C_KEEP_ON,	/* Keep on charge (you need send this cmd within a minute after Start cmd) */
	COM_BLE_C_KEEP_OFF,	/* Keep off charge */
	COM_BLE_C_MODE_RETURN,	/* Request charging mode */
	COM_BLE_C_FORCE_RESET,	/* DSP force reset */
	COM_BLE_C_FULL,		/* Full charge (depend on fw) */
};

int wacom_ble_charge_mode(struct wacom_data *wacom, int mode)
{
	int ret = 0;
	char data = -1;
	ktime_t current_time;

	if (wacom->is_open_test) {
		input_err(true, wacom->dev, "%s: other cmd is working\n",
				__func__);
		return -EPERM;
	}

	if (wacom->update_status == FW_UPDATE_RUNNING) {
		input_err(true, wacom->dev, "%s: fw update running\n",
				__func__);
		return -EPERM;
	}

	if (wacom->ble_block_flag && (mode != EPEN_BLE_C_DISABLE)) {
		input_err(true, wacom->dev, "%s: operation not permitted\n",
				__func__);
		return -EPERM;
	}

	if (mode >= EPEN_BLE_C_MAX || mode < EPEN_BLE_C_DISABLE) {
		input_err(true, wacom->dev, "%s: wrong mode, %d\n", __func__, mode);
		return -EINVAL;
	}

	mutex_lock(&wacom->ble_lock);

	/* need to check mode */
	if (ble_charge_command[mode] == COM_BLE_C_RESET
			|| ble_charge_command[mode] == COM_BLE_C_START) {
		current_time = ktime_get();
		wacom->chg_time_stamp = ktime_to_ms(current_time);
		input_err(true, wacom->dev, "%s: chg_time_stamp(%ld)\n",
				__func__, wacom->chg_time_stamp);
	} else {
		wacom->chg_time_stamp = 0;
	}

	if (mode == EPEN_BLE_C_DSPX) {
		/* add abnormal ble reset case (while pen in status, do not ble charge) */
		mutex_lock(&wacom->mode_lock);
		ret = wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_NONE);
		mutex_unlock(&wacom->mode_lock);
		if (ret < 0)
			goto out_save_result;

		sec_delay(250);

		data = ble_charge_command[mode];
		ret = wacom_send(wacom, &data, 1);
		if (ret < 0) {
			input_err(true, wacom->dev, "%s: failed to send data(%02x %d)\n",
					__func__, data, ret);
			wacom_select_survey_mode(wacom, atomic_read(&wacom->screen_on));
			goto out_save_result;
		}

		sec_delay(130);

		wacom_select_survey_mode(wacom, atomic_read(&wacom->screen_on));
	} else {
		data = ble_charge_command[mode];
		ret = wacom_send(wacom, &data, 1);
		if (ret < 0) {
			input_err(true, wacom->dev, "%s: failed to send data(%02x %d)\n",
					__func__, data, ret);
			goto out_save_result;
		}

		switch (data) {
		case COM_BLE_C_RESET:
		case COM_BLE_C_START:
		case COM_BLE_C_KEEP_ON:
		case COM_BLE_C_FULL:
			wacom->ble_mode_change = true;
			wacom->ble_charging_state = true;
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
			sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_CHARGING_STARTED, NULL);
#endif
			break;
		case COM_BLE_C_DISABLE:
		case COM_BLE_C_KEEP_OFF:
			wacom->ble_mode_change = false;
			wacom->ble_charging_state = false;
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
			sec_input_notify(&wacom->nb, NOTIFIER_WACOM_PEN_CHARGING_FINISHED, NULL);
#endif
			break;
		}
	}

	input_info(true, wacom->dev, "%s: now %02X, prev %02X\n", __func__, data,
			wacom->ble_mode ? wacom->ble_mode : 0);

	wacom->ble_mode = ble_charge_command[mode];

out_save_result:
	if (wacom->ble_hist) {
		unsigned long long ksec;
		unsigned long nanosec;
		char buffer[40];
		int len;

		ksec = local_clock();
		nanosec = do_div(ksec, 1000000000);

		memset(buffer, 0x00, sizeof(buffer));
		len = snprintf(buffer, sizeof(buffer), "%5lu.%06lu:%02X,%02X,%02d\n",
				(unsigned long)ksec, nanosec / 1000, mode, data, ret);
		if (len < 0)
			goto out_ble_charging;

		if (wacom->ble_hist_index + len >= WACOM_BLE_HISTORY_SIZE) {
			memcpy(&wacom->ble_hist[0], buffer, len);
			wacom->ble_hist_index = 0;
		} else {
			memcpy(&wacom->ble_hist[wacom->ble_hist_index], buffer, len);
			wacom->ble_hist_index += len;
		}

		if (mode == EPEN_BLE_C_DISABLE || mode == EPEN_BLE_C_ENABLE) {
			if (!wacom->ble_hist1)
				goto out_ble_charging;

			memset(wacom->ble_hist1, 0x00, WACOM_BLE_HISTORY1_SIZE);
			memcpy(wacom->ble_hist1, buffer, WACOM_BLE_HISTORY1_SIZE);
		}
	}

out_ble_charging:
	mutex_unlock(&wacom->ble_lock);
	return ret;
}

int get_wacom_scan_info(bool mode)
{
	struct wacom_data *wacom = g_wacom;
	ktime_t current_time;
	long diff_time;

	if (!wacom)
		return -ENODEV;

	current_time = ktime_get();
	diff_time = ktime_to_ms(current_time) - wacom->chg_time_stamp;
	input_info(true, wacom->dev, "%s: START mode[%d] & ble[%x] & diff time[%ld]\n",
			__func__, mode, wacom->ble_mode, diff_time);

	if (!wacom->probe_done || sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF) || wacom->is_open_test) {
		input_err(true, wacom->dev, "%s: wacom is not ready\n", __func__);
		return EPEN_CHARGE_ON;
	}

	/* clear ble_mode_change or check ble_mode_change value */
	if (mode) {
		wacom->ble_mode_change = false;
		return EPEN_NONE;
	} else if (!mode && wacom->ble_mode_change) {
		input_info(true, wacom->dev, "%s : charge happened\n", __func__);
		return EPEN_CHARGE_HAPPENED;
	}

	if (!wacom->pen_pdct) {
		input_err(true, wacom->dev, "%s: pen out & charge off\n", __func__);
		return EPEN_CHARGE_OFF;
	}

	if (wacom->ble_mode == COM_BLE_C_KEEP_ON || wacom->ble_mode == COM_BLE_C_FULL) {
		input_info(true, wacom->dev, "%s: charging [%x]\n", __func__, wacom->ble_mode);
		return EPEN_CHARGE_ON;
	/* check charge time */
	} else if (wacom->ble_mode == COM_BLE_C_RESET || wacom->ble_mode == COM_BLE_C_START) {

		if (diff_time > EPEN_CHARGER_OFF_TIME) {
			input_info(true, wacom->dev, "%s: charge off time BLE chg[%x], diff[%ld]\n",
						__func__, wacom->ble_mode, diff_time);
			return EPEN_CHARGE_OFF;
		} else {
			input_info(true, wacom->dev, "%s: charge on time BLE chg[%x], diff[%ld]\n",
						__func__, wacom->ble_mode, diff_time);
			return EPEN_CHARGE_ON;
		}
		return EPEN_NONE;

	}

	input_err(true, wacom->dev, "%s: charge off mode[%d] & ble[%x]\n",
			__func__, mode, wacom->ble_mode);
	return EPEN_CHARGE_OFF;
}
EXPORT_SYMBOL(get_wacom_scan_info);

int set_wacom_ble_charge_mode(bool mode)
{
	struct wacom_data *wacom = g_wacom;
	int ret = 0;
	int ret_val = 0;	/* 0:pass, etc:fail */

	if (g_wacom == NULL) {
		pr_info("%s: %s: g_wacom is NULL(%d)\n", SECLOG, __func__, mode);
		return 0;
	}

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe_done yet, skip!\n", __func__);
		return 0;
	}

	mutex_lock(&wacom->ble_charge_mode_lock);
	input_info(true, wacom->dev, "%s start(%d)\n", __func__, mode);

	if (!mode) {
		ret = wacom_ble_charge_mode(wacom, EPEN_BLE_C_KEEP_OFF);
		if (ret > 0) {
			wacom->ble_block_flag = true;
		} else {
			input_err(true, wacom->dev, "%s Fail to keep off\n", __func__);
			ret_val = 1;
		}
	} else {
#ifdef CONFIG_SEC_FACTORY
		ret = wacom_ble_charge_mode(wacom, EPEN_BLE_C_ENABLE);
		if (ret > 0) {
			wacom->ble_block_flag = false;
		} else {
			input_err(true, wacom->dev, "%s Fail to enable in fac\n", __func__);
			ret_val = 1;
		}
#else
		wacom->ble_block_flag = false;
#endif
	}

	input_info(true, wacom->dev, "%s done(%d)\n", __func__, mode);
	mutex_unlock(&wacom->ble_charge_mode_lock);

	return ret_val;
}
EXPORT_SYMBOL(set_wacom_ble_charge_mode);

static ssize_t epen_ble_charging_mode_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);

	if (wacom->ble_charging_state == false)
		return snprintf(buf, PAGE_SIZE, "DISCHARGE\n");

	return snprintf(buf, PAGE_SIZE, "CHARGE\n");
}

static ssize_t epen_ble_charging_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int retval = 0;

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe_done yet, skip!\n", __func__);
		return count;
	}

	if (kstrtoint(buf, 0, &retval)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev,
				"%s: power off return\n", __func__);
		return count;
	}

#if !WACOM_SEC_FACTORY
	if (retval == EPEN_BLE_C_DISABLE) {
		input_info(true, wacom->dev, "%s: use keep off insted of disable\n", __func__);
		retval = EPEN_BLE_C_KEEP_OFF;
	}
#endif

	mutex_lock(&wacom->ble_charge_mode_lock);
	wacom_ble_charge_mode(wacom, retval);
	mutex_unlock(&wacom->ble_charge_mode_lock);

	return count;
}

static ssize_t epen_ble_hist_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int size, size1;

	if (!wacom->ble_hist)
		return -ENODEV;

	if (!wacom->ble_hist1)
		return -ENODEV;

	size1 = strnlen(wacom->ble_hist1, WACOM_BLE_HISTORY1_SIZE);
	memcpy(buf, wacom->ble_hist1, size1);

	size = strnlen(wacom->ble_hist, WACOM_BLE_HISTORY_SIZE);
	memcpy(buf + size1, wacom->ble_hist, size);

	return size + size1;
}

static ssize_t epen_disable_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, wacom->dev, "%s: failed to get param\n",
				__func__);
		return count;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		return count;
	}

	if (val)
		wacom_disable_mode(wacom, WACOM_DISABLE);
	else
		wacom_disable_mode(wacom, WACOM_ENABLE);

	input_info(true, wacom->dev, "%s: %d\n", __func__, val);

	return count;
}

static ssize_t hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[516];
	char tbuff[128];

	memset(buff, 0x00, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"RESET\":\"%d\",", wacom->abnormal_reset_count);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"I2C_FAIL\":\"%d\",", wacom->i2c_fail_count);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"PEN_OUT\":\"%d\",", wacom->pen_out_count);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"SDCONN\":\"%d\",", wacom->connection_check);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"SECCNT\":\"%d\",", wacom->fail_channel);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"SADCVAL\":\"%d\",", wacom->min_adc_val);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"SECLVL\":\"%d\",", wacom->error_cal);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	/* remove last comma (csv) */
	snprintf(tbuff, sizeof(tbuff), "\"SCALLVL\":\"%d\",", wacom->min_cal_val);
	strlcat(buff, tbuff, sizeof(buff));
	/*ESD PACKET COUNT*/
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"ESDPCNT\":\"%d\",", wacom->esd_packet_count);
	strlcat(buff, tbuff, sizeof(buff));
	/*ESD INTERRUPT COUNT*/
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"ESDICNT\":\"%d\",", wacom->esd_irq_count);
	strlcat(buff, tbuff, sizeof(buff));
	/*ESD MAX CONTINUOUS COUNT*/
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"ESDMCCNT\":\"%d\"", wacom->esd_max_continuous_reset_count);
	strlcat(buff, tbuff, sizeof(buff));

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, PAGE_SIZE, "%s", buff);
}

static ssize_t hw_param_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);

	wacom->abnormal_reset_count = 0;
	wacom->i2c_fail_count = 0;
	wacom->pen_out_count = 0;
	wacom->esd_packet_count = 0;
	wacom->esd_irq_count = 0;

	input_info(true, wacom->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t epen_connection_check_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);

	input_info(true, wacom->dev, "%s: SDCONN:%d,SECCNT:%d,SADCVAL:%d,SECLVL:%d,SCALLVL:%d\n",
			__func__, wacom->connection_check,
			wacom->fail_channel, wacom->min_adc_val,
			wacom->error_cal, wacom->min_cal_val);

	return snprintf(buf, PAGE_SIZE,
			"\"SDCONN\":\"%d\",\"SECCNT\":\"%d\",\"SADCVAL\":\"%d\",\"SECLVL\":\"%d\",\"SCALLVL\":\"%d\"",
			wacom->connection_check, wacom->fail_channel,
			wacom->min_adc_val, wacom->error_cal,
			wacom->min_cal_val);
}

static ssize_t get_epen_pos_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	int max_x, max_y;

	max_x = wacom->plat_data->max_x;
	max_y = wacom->plat_data->max_y;

	input_info(true, wacom->dev,
			"%s: id(%d), x(%d), y(%d), max_x(%d), max_y(%d)\n", __func__,
			wacom->survey_pos.id, wacom->survey_pos.x, wacom->survey_pos.y,
			max_x, max_y);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d\n",
			wacom->survey_pos.id, wacom->survey_pos.x, wacom->survey_pos.y,
			max_x, max_y);
}

static ssize_t debug_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[INPUT_DEBUG_INFO_SIZE];
	char tbuff[256];

	if (sizeof(tbuff) > INPUT_DEBUG_INFO_SIZE)
		input_info(true, wacom->dev, "%s: buff_size[%d], tbuff_size[%ld]\n",
				__func__, INPUT_DEBUG_INFO_SIZE, sizeof(tbuff));

	memset(buff, 0x00, sizeof(buff));
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "FW: (I)%02X%02X%02X%02X / (B)%02X%02X%02X%02X\n",
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3],
			wacom->plat_data->img_version_of_bin[0], wacom->plat_data->img_version_of_bin[1],
			wacom->plat_data->img_version_of_bin[2], wacom->plat_data->img_version_of_bin[3]);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "ps:%s pen:%s %s aop: lcd(%s), aod(%s), screenmemo(%s), aot(%s)\n",
			wacom->battery_saving_mode ?  "on" : "off",
			(wacom->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
			(wacom->function_result & EPEN_EVENT_SURVEY) ? "survey" : "normal",
			(wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS) ? "true" : "false",
			(wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOD) ? "true" : "false",
			(wacom->function_set & EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO) ? "true" : "false",
			(wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOT) ? "true" : "false");
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "screen_on %d, report_scan_seq %d, epen %s, stdby:%d\n",
			atomic_read(&wacom->screen_on),
			wacom->report_scan_seq, wacom->epen_blocked ? "blocked" : "unblocked",
#ifdef SUPPORT_STANDBY_MODE
			wacom->standby_state
#else
			0
#endif
		);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "count(%u,%u,%u), mode(%d), block_cnt(%d), check(%d), test(%d,%d)\n",
			wacom->i2c_fail_count, wacom->abnormal_reset_count, wacom->scan_info_fail_cnt,
			wacom->check_survey_mode, wacom->tsp_block_cnt, wacom->check_elec,
			wacom->connection_check, wacom->garage_connection_check);
	strlcat(buff, tbuff, sizeof(buff));

	input_info(true, wacom->dev, "%s: size: %ld\n", __func__, strlen(buff));

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

/* firmware update */
static DEVICE_ATTR_WO(epen_firm_update);
/* return firmware update status */
static DEVICE_ATTR_RO(epen_firm_update_status);
/* return firmware version */
static DEVICE_ATTR_RO(epen_firm_version);
static DEVICE_ATTR_RO(ver_of_ic);
static DEVICE_ATTR_RO(ver_of_bin);

/* For SMD Test */
static DEVICE_ATTR_WO(epen_reset);
static DEVICE_ATTR_RO(epen_reset_result);
/* For SMD Test. Check checksum */
static DEVICE_ATTR_WO(epen_checksum);
static DEVICE_ATTR_RO(epen_checksum_result);
static DEVICE_ATTR_RO(epen_connection);
static DEVICE_ATTR_WO(epen_saving_mode);
static DEVICE_ATTR_RW(epen_wcharging_mode);
static DEVICE_ATTR_WO(epen_disable_mode);
static DEVICE_ATTR_WO(screen_off_memo_enable);
static DEVICE_ATTR_RO(get_epen_pos);
static DEVICE_ATTR_WO(aod_enable);
static DEVICE_ATTR_WO(aod_lcd_onoff_status);
static DEVICE_ATTR_WO(aot_enable);
static DEVICE_ATTR_RW(hw_param);
static DEVICE_ATTR_RO(epen_connection_check);

static DEVICE_ATTR_RO(epen_insert);
static DEVICE_ATTR_RW(epen_ble_charging_mode);
static DEVICE_ATTR_RO(epen_ble_hist);

#if WACOM_SEC_FACTORY
static DEVICE_ATTR_WO(epen_fac_select_firmware);
#endif

static DEVICE_ATTR_RO(debug_info);

static struct attribute *epen_attributes[] = {
	&dev_attr_epen_firm_update.attr,
	&dev_attr_epen_firm_update_status.attr,
	&dev_attr_epen_firm_version.attr,
	&dev_attr_ver_of_ic.attr,
	&dev_attr_ver_of_bin.attr,
	&dev_attr_epen_reset.attr,
	&dev_attr_epen_reset_result.attr,
	&dev_attr_epen_checksum.attr,
	&dev_attr_epen_checksum_result.attr,
	&dev_attr_epen_connection.attr,
	&dev_attr_epen_saving_mode.attr,
	&dev_attr_epen_wcharging_mode.attr,
	&dev_attr_epen_disable_mode.attr,
	&dev_attr_screen_off_memo_enable.attr,
	&dev_attr_get_epen_pos.attr,
	&dev_attr_aod_enable.attr,
	&dev_attr_aod_lcd_onoff_status.attr,
	&dev_attr_aot_enable.attr,
	&dev_attr_hw_param.attr,
	&dev_attr_epen_connection_check.attr,

	&dev_attr_epen_insert.attr,
	&dev_attr_epen_ble_charging_mode.attr,
	&dev_attr_epen_ble_hist.attr,
#if WACOM_SEC_FACTORY
	&dev_attr_epen_fac_select_firmware.attr,
#endif
	&dev_attr_debug_info.attr,
	NULL,
};

static struct attribute_group epen_attr_group = {
	.attrs = epen_attributes,
};

static void print_spec_data(struct wacom_data *wacom)
{
	struct wacom_elec_data *edata = wacom->edata;
	u8 tmp_buf[WACOM_CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = WACOM_CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (!buff)
		return;

	input_info(true, wacom->dev, "%s: shift_value %d, spec ver %d.%d", __func__,
			edata->shift_value, edata->spec_ver[0], edata->spec_ver[1]);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xx_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->xx_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xy_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->xy_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yx_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->yx_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yy_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->yy_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xy_edg_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->xy_edg_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yx_edg_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->yx_edg_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xx_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->xx_spec[i] / POWER_OFFSET * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xy_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->xy_spec[i] / POWER_OFFSET * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yx_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->yx_spec[i] / POWER_OFFSET * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yy_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->yy_spec[i] / POWER_OFFSET * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "rxx_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->rxx_ref[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "rxy_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->rxy_ref[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "ryx_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->ryx_ref[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "ryy_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->ryy_ref[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "drxx_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->drxx_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "drxy_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->drxy_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "dryx_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->dryx_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "dryy_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->dryy_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "drxy_edg_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->drxy_edg_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "dryx_edg_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ",
				edata->dryx_edg_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xx_self_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->xx_self_spec[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yy_self_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->yy_self_spec[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}

static void run_elec_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	struct wacom_elec_data *edata = NULL;
	u8 *buff = NULL;
	u8 tmp_buf[WACOM_CMD_RESULT_WORD_LEN] = { 0 };
	u8 data[COM_ELEC_NUM] = { 0, };
	u16 tmp, offset;
	u8 cmd;
	int i, tx, rx;
	int retry = RETRY_COUNT;
	int ret;
	int max_ch;
	int buff_size;
	u32 count;
	int remain_data_count;
	u16 *elec_result = NULL;

	sec_cmd_set_default_result(sec);

	input_info(true, wacom->dev, "%s: parm[%d]\n", __func__, sec->cmd_param[0]);

	if (sec->cmd_param[0] < EPEN_ELEC_DATA_MAIN || sec->cmd_param[0] >= EPEN_ELEC_DATA_MAX) {
		input_err(true, wacom->dev,
				"%s: Abnormal parm[%d]\n", __func__, sec->cmd_param[0]);
		goto error;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev, "%s: power off state, skip!\n",
				__func__);
		goto error;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		goto error;
	}

	edata = wacom->edata = wacom->edatas[sec->cmd_param[0]];

	if (!edata) {
		input_err(true, wacom->dev, "%s: test spec is NULL\n", __func__);
		goto error;
	}

	max_ch = edata->max_x_ch + edata->max_y_ch;
	buff_size = max_ch * 2 * WACOM_CMD_RESULT_WORD_LEN + SEC_CMD_STR_LEN;

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (!buff)
		goto error;

	elec_result = kzalloc((max_ch + 1) * sizeof(u16), GFP_KERNEL);
	if (!elec_result)
		goto error;

	wacom_enable_irq(wacom, false);

	/* change wacom mode to 0x2D */
	mutex_lock(&wacom->mode_lock);
	wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_NONE);
	mutex_unlock(&wacom->mode_lock);

	/* wait for status event for normal mode of wacom */
	sec_delay(250);
	do {
		ret = wacom_recv(wacom, data, COM_COORD_NUM);
		if (ret != COM_COORD_NUM) {
			input_err(true, wacom->dev, "%s: failed to recv status data\n",
					__func__);
			sec_delay(50);
			continue;
		}

		/* check packet ID */
		if ((data[0] & 0x0F) != NOTI_PACKET && (data[1] != CMD_PACKET)) {
			input_info(true, wacom->dev,
					"%s: invalid status data %d\n",
					__func__, (RETRY_COUNT - retry + 1));
		} else {
			break;
		}

		sec_delay(50);
	} while (retry--);

	retry = RETRY_COUNT;
	cmd = COM_ELEC_XSCAN;
	ret = wacom_send(wacom, &cmd, 1);
	if (ret != 1) {
		input_err(true, wacom->dev,
				"%s: failed to send 0x%02X\n", __func__, cmd);
		goto error_test;
	}
	sec_delay(30);

	cmd = COM_ELEC_TRSX0;
	ret = wacom_send(wacom, &cmd, 1);
	if (ret != 1) {
		input_err(true, wacom->dev,
				"%s: failed to send 0x%02X\n", __func__, cmd);
		goto error_test;
	}
	sec_delay(30);

	cmd = COM_ELEC_SCAN_START;
	ret = wacom_send(wacom, &cmd, 1);
	if (ret != 1) {
		input_err(true, wacom->dev,
				"%s: failed to send 0x%02X\n", __func__, cmd);
		goto error_test;
	}
	sec_delay(30);

	for (tx = 0; tx < max_ch; tx++) {
		/* x scan mode receive data through x coils */
		remain_data_count = edata->max_x_ch;

		do {
			cmd = COM_ELEC_REQUESTDATA;
			ret = wacom_send(wacom, &cmd, 1);
			if (ret != 1) {
				input_err(true, wacom->dev,
						"%s: failed to send 0x%02X\n",
						__func__, cmd);
				goto error_test;
			}
			sec_delay(30);

			ret = wacom_recv(wacom, data, COM_ELEC_NUM);
			if (ret != COM_ELEC_NUM) {
				input_err(true, wacom->dev,
						"%s: failed to receive elec data\n",
						__func__);
				goto error_test;
			}
#if 0
			input_info(true, wacom->dev, "%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
					data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9],
					data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19],
					data[20], data[21], data[22], data[23], data[24], data[25], data[26], data[27],	data[28], data[29],
					data[30], data[31], data[32], data[33], data[34], data[35], data[36], data[37]);
#endif
			/* check packet ID & sub packet ID */
			if (((data[0] & 0x0F) != 0x0E) && (data[1] != 0x65)) {
				input_info(true, wacom->dev,
						"%s: %d data of wacom elec\n",
						__func__, (RETRY_COUNT - retry + 1));
				if (--retry) {
					continue;
				} else {
					input_err(true, wacom->dev,
							"%s: invalid elec data %d %d\n",
							__func__, (data[0] & 0x0F), data[1]);
					goto error_test;
				}
			}

			if (data[11] != 3) {
				input_info(true, wacom->dev,
						"%s: %d data of wacom elec\n",
						__func__, (RETRY_COUNT - retry + 1));
				if (--retry) {
					continue;
				} else {
					input_info(true, wacom->dev,
							"%s: invalid elec status %d\n",
							__func__, data[11]);
					goto error_test;
				}
			}

			for (i = 0; i < COM_ELEC_DATA_NUM; i++) {
				if (!remain_data_count)
					break;

				offset = COM_ELEC_DATA_POS + (i * 2);
				tmp = ((u16)(data[offset]) << 8) + data[offset + 1];
				rx = data[13] + i;

				edata->elec_data[rx * max_ch + tx] = tmp;

				remain_data_count--;
			}
		} while (remain_data_count);

		cmd = COM_ELEC_TRSCON;
		ret = wacom_send(wacom, &cmd, 1);
		if (ret != 1) {
			input_err(true, wacom->dev, "%s: failed to send 0x%02X\n",
					__func__, cmd);
			goto error_test;
		}
		sec_delay(30);
	}

	cmd = COM_ELEC_YSCAN;
	ret = wacom_send(wacom, &cmd, 1);
	if (ret != 1) {
		input_err(true, wacom->dev,
				"%s: failed to send 0x%02X\n", __func__, cmd);
		goto error_test;
	}
	sec_delay(30);

	cmd = COM_ELEC_TRSX0;
	ret = wacom_send(wacom, &cmd, 1);
	if (ret != 1) {
		input_err(true, wacom->dev,
				"%s: failed to send 0x%02X\n", __func__, cmd);
		goto error_test;
	}
	sec_delay(30);

	cmd = COM_ELEC_SCAN_START;
	ret = wacom_send(wacom, &cmd, 1);
	if (ret != 1) {
		input_err(true, wacom->dev,
				"%s: failed to send 0x%02X\n", __func__, cmd);
		goto error_test;
	}
	sec_delay(30);

	for (tx = 0; tx < max_ch; tx++) {
		/* y scan mode receive data through y coils */
		remain_data_count = edata->max_y_ch;

		do {
			cmd = COM_ELEC_REQUESTDATA;
			ret = wacom_send(wacom, &cmd, 1);
			if (ret != 1) {
				input_err(true, wacom->dev,
						"%s: failed to send 0x%02X\n",
						__func__, cmd);
				goto error_test;
			}
			sec_delay(30);

			ret = wacom_recv(wacom, data, COM_ELEC_NUM);
			if (ret != COM_ELEC_NUM) {
				input_err(true, wacom->dev,
						"%s: failed to receive elec data\n",
						__func__);
				goto error_test;
			}
#if 0
			input_info(true, wacom->dev, "%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
					data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9],
					data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19],
					data[20], data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29],
					data[30], data[31], data[32], data[33], data[34], data[35], data[36], data[37]);
#endif
			/* check packet ID & sub packet ID */
			if (((data[0] & 0x0F) != 0x0E) && (data[1] != 0x65)) {
				input_info(true, wacom->dev,
						"%s: %d data of wacom elec\n",
						__func__, (RETRY_COUNT - retry + 1));
				if (--retry) {
					continue;
				} else {
					input_err(true, wacom->dev,
							"%s: invalid elec data %d %d\n",
							__func__, (data[0] & 0x0F), data[1]);
					goto error_test;
				}
			}

			if (data[11] != 3) {
				input_info(true, wacom->dev,
						"%s: %d data of wacom elec\n",
						__func__, (RETRY_COUNT - retry + 1));
				if (--retry) {
					continue;
				} else {
					input_info(true, wacom->dev,
							"%s: invalid elec status %d\n",
							__func__, data[11]);
					goto error_test;
				}
			}

			for (i = 0; i < COM_ELEC_DATA_NUM; i++) {
				if (!remain_data_count)
					break;

				offset = COM_ELEC_DATA_POS + (i * 2);
				tmp = ((u16)(data[offset]) << 8) + data[offset + 1];
				rx = edata->max_x_ch + data[13] + i;

				edata->elec_data[rx * max_ch + tx] = tmp;

				remain_data_count--;
			}
		} while (remain_data_count);

		cmd = COM_ELEC_TRSCON;
		ret = wacom_send(wacom, &cmd, 1);
		if (ret != 1) {
			input_err(true, wacom->dev, "%s: failed to send 0x%02X\n",
					__func__, cmd);
			goto error_test;
		}
		sec_delay(30);
	}

	print_spec_data(wacom);

	print_elec_data(wacom);

	/* tx about x coil */
	for (tx = 0, i = 0; tx < edata->max_x_ch; tx++, i++) {
		edata->xx[i] = 0;
		edata->xy[i] = 0;
		/* rx about x coil */
		for (rx = 0; rx < edata->max_x_ch; rx++) {
			offset = rx * max_ch + tx;
			if ((edata->elec_data[offset] > edata->xx[i]) && (tx != rx))
				edata->xx[i] = edata->elec_data[offset];
			if (tx == rx)
				edata->xx_self[i] = edata->elec_data[offset];
		}
		/* rx about y coil */
		for (rx = edata->max_x_ch; rx < max_ch; rx++) {
			offset = rx * max_ch + tx;
			if (edata->elec_data[offset] > edata->xy[i])
				edata->xy[i] = edata->elec_data[offset];
		}

		edata->xy_edg[i] = min(edata->elec_data[(edata->max_x_ch + 1) * max_ch + tx],
							edata->elec_data[(max_ch - 2) * max_ch + tx]);
	}

	/* tx about y coil */
	for (tx = edata->max_x_ch, i = 0; tx < max_ch; tx++, i++) {
		edata->yx[i] = 0;
		edata->yy[i] = 0;
		/* rx about x coil */
		for (rx = 0; rx < edata->max_x_ch; rx++) {
			offset = rx * max_ch + tx;
			if (edata->elec_data[offset] > edata->yx[i])
				edata->yx[i] = edata->elec_data[offset];
		}
		/* rx about y coil */
		for (rx = edata->max_x_ch; rx < max_ch; rx++) {
			offset = rx * max_ch + tx;
			if ((edata->elec_data[offset] > edata->yy[i]) && (rx != tx))
				edata->yy[i] = edata->elec_data[offset];
			if (rx == tx)
				edata->yy_self[i] = edata->elec_data[offset];
		}

		edata->yx_edg[i] = min(edata->elec_data[max_ch + tx],
					edata->elec_data[(edata->max_x_ch - 2) * max_ch + tx]);
	}

	print_trx_data(wacom);

	calibration_trx_data(wacom);

	print_cal_trx_data(wacom);

	calculate_ratio(wacom);

	print_ratio_trx_data(wacom);

	make_decision(wacom, elec_result);

	print_difference_ratio_trx_data(wacom);

	count = elec_result[0] >> 8;
	if (!count) {
		snprintf(tmp_buf, sizeof(tmp_buf), "0_");
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	} else {
		if (count > 3)
			snprintf(tmp_buf, sizeof(tmp_buf), "3,");
		else
			snprintf(tmp_buf, sizeof(tmp_buf), "%d,", count);

		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));

		for (i = 0, count = 0; i < max_ch; i++) {
			if ((elec_result[i + 1] & SEC_SHORT) && count < 3) {
				if (i < edata->max_x_ch)
					snprintf(tmp_buf, sizeof(tmp_buf), "X%d,", i);
				else
					snprintf(tmp_buf, sizeof(tmp_buf), "Y%d,", i - edata->max_x_ch);

				strlcat(buff, tmp_buf, buff_size);
				memset(tmp_buf, 0x00, sizeof(tmp_buf));
				count++;
			}
		}
		buff[strlen(buff) - 1] = '_';
	}

	count = elec_result[0] & 0x00FF;
	if (!count) {
		snprintf(tmp_buf, sizeof(tmp_buf), "0_");
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	} else {
		if (count > 3)
			snprintf(tmp_buf, sizeof(tmp_buf), "3,");
		else
			snprintf(tmp_buf, sizeof(tmp_buf), "%d,", count);

		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));

		for (i = 0, count = 0; i < max_ch; i++) {
			if ((elec_result[i + 1] & SEC_OPEN) && count < 3) {
				if (i < edata->max_x_ch)
					snprintf(tmp_buf, sizeof(tmp_buf), "X%d,", i);
				else
					snprintf(tmp_buf, sizeof(tmp_buf), "Y%d,", i - edata->max_x_ch);

				strlcat(buff, tmp_buf, buff_size);
				memset(tmp_buf, 0x00, sizeof(tmp_buf));
				count++;
			}
		}
		buff[strlen(buff) - 1] = '_';
	}

	snprintf(tmp_buf, sizeof(tmp_buf), "%d,%d_", edata->max_x_ch, edata->max_y_ch);
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, sizeof(tmp_buf));

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,",  edata->xx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->xy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->yx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->yy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,",  edata->xx_self[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->yy_self[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->xy_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->yx_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	sec_cmd_set_cmd_result(sec, buff, buff_size);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(elec_result);
	kfree(buff);

	ret = wacom_start_stop_cmd(wacom, WACOM_STOP_AND_START_CMD);
	if (ret != 1) {
		input_err(true, wacom->dev, "%s: failed to set stop-start cmd, %d\n",
				__func__, ret);
	}

	wacom_enable_irq(wacom, true);

	return;

error_test:
	ret = wacom_start_stop_cmd(wacom, WACOM_STOP_AND_START_CMD);
	if (ret != 1) {
		input_err(true, wacom->dev, "%s: failed to set stop-start cmd, %d\n",
				__func__, ret);
	}

	wacom_enable_irq(wacom, true);

error:
	if (elec_result)
		kfree(elec_result);

	if (buff)
		kfree(buff);

	snprintf(tmp_buf, sizeof(tmp_buf), "NG");
	sec_cmd_set_cmd_result(sec, tmp_buf, sizeof(tmp_buf));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	return;
}

static void ready_elec_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char data;
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev, "%s: power off state, skip!\n",
				__func__);
		goto err_out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		goto err_out;
	}

	if (sec->cmd_param[0])
		data = COM_ASYNC_VSYNC;
	else
		data = COM_SYNC_VSYNC;

	ret = wacom_send(wacom, &data, 1);
	if (ret != 1) {
		input_err(true, wacom->dev,
				"%s: failed to send data(%02x, %d)\n",
				__func__, data, ret);
		goto err_out;
	}

	sec_delay(30);

	snprintf(buff, sizeof(buff), "OK\n");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, wacom->dev, "%s: %02x %s\n",
				__func__, data, buff);
	return;
err_out:
	snprintf(buff, sizeof(buff), "NG\n");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void get_elec_test_ver(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < EPEN_ELEC_DATA_MAIN || sec->cmd_param[0] >= EPEN_ELEC_DATA_MAX) {
		input_err(true, wacom->dev,
				"%s: Abnormal parm[%d]\n", __func__, sec->cmd_param[0]);
		goto err_out;
	}

	if (wacom->edatas[sec->cmd_param[0]] == NULL) {
		input_err(true, wacom->dev,
				"%s: Elec test spec is null[%d]\n", __func__, sec->cmd_param[0]);
		goto err_out;
	}

	snprintf(buff, sizeof(buff), "%d.%d",
			wacom->edatas[sec->cmd_param[0]]->spec_ver[0],
			wacom->edatas[sec->cmd_param[0]]->spec_ver[1]);

	input_info(true, wacom->dev, "%s: pos[%d] ver(%s)\n",
				__func__, sec->cmd_param[0], buff);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return;

err_out:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

/* have to add cmd from Q os for garage test */
static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (wacom->plat_data->img_version_of_ic[0] == MPU_W9018)
		snprintf(buff, sizeof(buff), "W9018");
	else if (wacom->plat_data->img_version_of_ic[0] == MPU_W9019)
		snprintf(buff, sizeof(buff), "W9019");
	else if (wacom->plat_data->img_version_of_ic[0] == MPU_W9020)
		snprintf(buff, sizeof(buff), "W9020");
	else if (wacom->plat_data->img_version_of_ic[0] == MPU_W9021)
		snprintf(buff, sizeof(buff), "W9021");
	else if (wacom->plat_data->img_version_of_ic[0] == MPU_WEZ01)
		snprintf(buff, sizeof(buff), "WEZ01");
	else if (wacom->plat_data->img_version_of_ic[0] == MPU_WEZ02)
		snprintf(buff, sizeof(buff), "WEZ02");
	else
		snprintf(buff, sizeof(buff), "N/A");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

/* Factory cmd for firmware update
 * argument represent what is source of firmware like below.
 *
 * 0 : [BUILT_IN] Getting firmware which is for user.
 * 1 : [UMS] Getting firmware from sd card.
 * 2 : none
 * 3 : [FFU] Getting firmware from apk.
 */

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s : reset is on going, skip!\n", __func__);
		goto err_out;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev, "%s: [ERROR] Wacom is stopped\n", __func__);
		goto err_out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > WACOM_VERIFICATION) {
		input_err(true, wacom->dev, "%s: [ERROR] parm error![%d]\n",
				__func__, sec->cmd_param[0]);
		goto err_out;
	}

	mutex_lock(&wacom->lock);

	switch (sec->cmd_param[0]) {
	case WACOM_BUILT_IN:
		ret = wacom_fw_update_on_hidden_menu(wacom, FW_BUILT_IN);
		break;
	case WACOM_SDCARD:
#if WACOM_PRODUCT_SHIP
		ret = wacom_fw_update_on_hidden_menu(wacom, FW_IN_SDCARD_SIGNED);
#else
		ret = wacom_fw_update_on_hidden_menu(wacom, FW_IN_SDCARD);
#endif
		break;
	default:
		input_err(true, wacom->dev, "%s: Not support command[%d]\n",
				__func__, sec->cmd_param[0]);
		mutex_unlock(&wacom->lock);
		goto err_out;
	}

	mutex_unlock(&wacom->lock);

	if (ret) {
		input_err(true, wacom->dev, "%s: Wacom fw update(%d) fail ret [%d]\n",
				__func__, sec->cmd_param[0], ret);
		goto err_out;
	}

	input_info(true, wacom->dev, "%s: Wacom fw update[%d]\n", __func__, sec->cmd_param[0]);

	snprintf(buff, sizeof(buff), "OK\n");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, wacom->dev, "%s: Done\n", __func__);

	return;

err_out:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	return;

}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%02X%02X%02X%02X",
			wacom->plat_data->img_version_of_bin[0], wacom->plat_data->img_version_of_bin[1],
			wacom->plat_data->img_version_of_bin[2], wacom->plat_data->img_version_of_bin[3]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%02X%02X%02X%02X",
			wacom->plat_data->img_version_of_ic[0], wacom->plat_data->img_version_of_ic[1],
			wacom->plat_data->img_version_of_ic[2], wacom->plat_data->img_version_of_ic[3]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void set_ic_reset(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		wacom->query_status = false;
		goto out;
	}

	wacom_enable_irq(wacom, false);

	wacom->function_result &= ~EPEN_EVENT_SURVEY;
	wacom->survey_mode = EPEN_SURVEY_MODE_NONE;

	/* Reset IC */
	wacom_reset_hw(wacom);
	/* I2C Test */
	wacom_query(wacom);

	wacom_enable_irq(wacom, true);

	input_info(true, wacom->dev, "%s: result %d\n", __func__, wacom->query_status);
out:
	if (wacom->query_status) {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	} else {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void get_checksum(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	wacom->checksum_result = false;

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev, "%s: power off state, skip!\n", __func__);
		goto out;
	}

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe_done yet, skip!\n", __func__);
		goto out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n", __func__);
		goto out;
	}

	sec_cmd_set_default_result(sec);

	wacom_enable_irq(wacom, false);

	wacom_checksum(wacom);

	wacom_enable_irq(wacom, true);

	input_info(true, wacom->dev,
			"%s: result %d\n", __func__, wacom->checksum_result);

out:
	if (wacom->checksum_result) {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	} else {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void get_digitizer_connection(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	input_info(true, wacom->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);

	ret = wacom_check_ub(wacom);
	if (ret < 0) {
		input_info(true, wacom->dev, "%s: digitizer is not attached\n", __func__);
		goto out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		wacom->connection_check = false;
		goto out;
	}

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe_done yet, OK & skip!\n", __func__);
		wacom->connection_check = false;
		goto out;
	}

	get_connection_test(wacom, WACOM_DIGITIZER_TEST);

out:
	if (wacom->module_ver == 0x2) {
		snprintf(buff, sizeof(buff), "%s %d %d %d %s %d\n",
				wacom->connection_check ? "OK" : "NG",
				wacom->module_ver, wacom->fail_channel,
				wacom->min_adc_val, wacom->error_cal ? "NG" : "OK",
				wacom->min_cal_val);
	} else {
		snprintf(buff, sizeof(buff), "%s %d %d %d\n",
				wacom->connection_check ? "OK" : "NG",
				wacom->module_ver, wacom->fail_channel,
				wacom->min_adc_val);
	}

	if (wacom->connection_check)
		sec->cmd_state = SEC_CMD_STATUS_OK;
	else
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void get_garage_connection(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	input_info(true, wacom->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);

	if (!wacom->support_garage_open_test) {
		input_err(true, wacom->dev, "%s: not support garage open test", __func__);

		snprintf(buff, sizeof(buff), "NA");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		sec_cmd_set_cmd_exit(sec);
		return;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		wacom->garage_connection_check = false;
		goto out;
	}

	get_connection_test(wacom, WACOM_GARAGE_TEST);

out:
	if (wacom->module_ver == 0x2) {
		snprintf(buff, sizeof(buff), "%s %d %d %d %s %d\n",
				wacom->garage_connection_check ? "OK" : "NG",
				wacom->module_ver, wacom->garage_fail_channel,
				wacom->garage_min_adc_val, wacom->garage_error_cal ? "NG" : "OK",
				wacom->garage_min_cal_val);
	} else {
		snprintf(buff, sizeof(buff), "%s %d %d %d\n",
				wacom->garage_connection_check ? "OK" : "NG",
				wacom->module_ver, wacom->garage_fail_channel,
				wacom->garage_min_adc_val);
	}

	if (wacom->garage_connection_check)
		sec->cmd_state = SEC_CMD_STATUS_OK;
	else
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void set_saving_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	static u32 call_count;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, wacom->dev, "%s: Abnormal parm[%d]\n",
					__func__, sec->cmd_param[0]);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto fail;
	}

	if (call_count < 0xFFFFFFF0)
		call_count++;

	wacom->battery_saving_mode = sec->cmd_param[0];

	input_info(true, wacom->dev, "%s: ps %s & pen %s [%d]\n",
			__func__, wacom->battery_saving_mode ? "on" : "off",
			(wacom->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
			call_count);

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s : not probe done & skip!\n", __func__);
		goto out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s : reset is on going, data saved and skip!\n", __func__);
		goto out;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF) && wacom->battery_saving_mode) {
		input_err(true, wacom->dev, "%s: already power off, save & return\n", __func__);
		goto out;
	}

	if (!(wacom->function_result & EPEN_EVENT_PEN_OUT)) {
		wacom_select_survey_mode(wacom, atomic_read(&wacom->screen_on));

		if (wacom->battery_saving_mode)
			forced_release_fullscan(wacom);
	}

out:
	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

fail:
	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void get_insert_status(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int pen_state = (wacom->function_result & EPEN_EVENT_PEN_OUT);

	sec_cmd_set_default_result(sec);

	input_info(true, wacom->dev, "%s : pen is %s\n",
				__func__, pen_state ? "OUT" : "IN");

	sec->cmd_state = SEC_CMD_STATUS_OK;
	snprintf(buff, sizeof(buff), "%d\n", pen_state ? 0 : 1);

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void set_wcharging_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	wacom->wcharging_mode = sec->cmd_param[0];
	input_info(true, wacom->dev, "%s: %d\n", __func__, wacom->wcharging_mode);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s : reset is on going, data saved and skip!\n", __func__);
		goto out;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev,
				"%s: power off, save & return\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_OK;
		goto out;
	}

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe is not done\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_OK;
		goto out;
	}

	ret = wacom_set_sense_mode(wacom);
	if (ret < 0) {
		input_err(true, wacom->dev, "%s: do not set sensitivity mode, %d\n",
						__func__, ret);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	if (sec->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void get_wcharging_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	input_info(true, wacom->dev, "%s: %s\n", __func__,
			!wacom->wcharging_mode ? "NORMAL" : "LOWSENSE");

	sec->cmd_state = SEC_CMD_STATUS_OK;
	snprintf(buff, sizeof(buff), "%s\n", !wacom->wcharging_mode ? "NORMAL" : "LOWSENSE");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void set_disable_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		goto err_out;
	}

	if (sec->cmd_param[0])
		wacom_disable_mode(wacom, WACOM_DISABLE);
	else
		wacom_disable_mode(wacom, WACOM_ENABLE);

	snprintf(buff, sizeof(buff), "OK");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	return;
err_out:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_screen_off_memo(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int val = 0;

	sec_cmd_set_default_result(sec);

#if WACOM_SEC_FACTORY
	input_info(true, wacom->dev, "%s : Not support screen off memo mode(%d) in Factory Bin\n",
				__func__, sec->cmd_param[0]);

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	goto out;
#endif

	val = !(!sec->cmd_param[0]);

	if (val)
		wacom->function_set |= EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO;
	else
		wacom->function_set &= (~EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO);

	input_info(true, wacom->dev,
			"%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
			wacom->battery_saving_mode ? "on" : "off",
			(wacom->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
			wacom->function_set, wacom->function_result);

	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s: probe is not done\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, data saved and skip!\n",
				__func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (!atomic_read(&wacom->screen_on))
		wacom_select_survey_mode(wacom, false);

	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	if (sec->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_epen_aod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int val = 0;

	sec_cmd_set_default_result(sec);
	sec->cmd_state = SEC_CMD_STATUS_OK;

#if WACOM_SEC_FACTORY
	input_info(true, wacom->dev, "%s : Not support aod mode(%d) in Factory Bin\n",
					__func__, val);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	goto out;
#endif

	val = !(!sec->cmd_param[0]);

	if (val)
		wacom->function_set |= EPEN_SETMODE_AOP_OPTION_AOD_LCD_OFF;
	else
		wacom->function_set &= ~EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON;

	input_info(true, wacom->dev,
			"%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
			wacom->battery_saving_mode ? "on" : "off",
			(wacom->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
			wacom->function_set, wacom->function_result);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, data saved and skip!\n",
				__func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (!atomic_read(&wacom->screen_on))
		wacom_select_survey_mode(wacom, false);


out:
	if (sec->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_aod_lcd_onoff_status(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int val = 0;
	int ret;

	sec_cmd_set_default_result(sec);

#if WACOM_SEC_FACTORY
	input_info(true, wacom->dev, "%s : Not support aod mode(%d) in Factory Bin\n",
					__func__, val);

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	goto out;
#endif

	val = !(!sec->cmd_param[0]);

	if (val)
		wacom->function_set |= EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS;
	else
		wacom->function_set &= ~EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS;

	if (!(wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOD) || atomic_read(&wacom->screen_on)) {
		sec->cmd_state = SEC_CMD_STATUS_OK;
		goto out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, data saved and skip!\n",
				__func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (wacom->survey_mode == EPEN_SURVEY_MODE_GARAGE_AOP) {
		if ((wacom->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON)
				== EPEN_SETMODE_AOP_OPTION_AOD_LCD_OFF) {
			forced_release_fullscan(wacom);
		}

		mutex_lock(&wacom->mode_lock);
		wacom_set_survey_mode(wacom, wacom->survey_mode);
		mutex_unlock(&wacom->mode_lock);
	}

	ret = wacom_set_sense_mode(wacom);
	if (ret < 0) {
		input_err(true, wacom->dev, "%s: do not set sensitivity mode, %d\n",
						__func__, ret);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	if (sec->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_epen_aot_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int val = 0;

	sec_cmd_set_default_result(sec);
	sec->cmd_state = SEC_CMD_STATUS_OK;

#if WACOM_SEC_FACTORY
	input_info(true, wacom->dev, "%s : Not support aod mode(%d) in Factory Bin\n",
					__func__, val);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	goto out;
#endif

	val = !(!sec->cmd_param[0]);

	if (val)
		wacom->function_set |= EPEN_SETMODE_AOP_OPTION_AOT;
	else
		wacom->function_set &= (~EPEN_SETMODE_AOP_OPTION_AOT);

	input_info(true, wacom->dev,
			"%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
			wacom->battery_saving_mode ? "on" : "off",
			(wacom->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
			wacom->function_set, wacom->function_result);

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, data saved and skip!\n",
				__func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (!atomic_read(&wacom->screen_on))
		wacom_select_survey_mode(wacom, false);

out:
	if (sec->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

#if WACOM_SEC_FACTORY
/* k|K|0 : FW_BUILT_IN, t|T|1 : FW_FACTORY_GARAGE u|U|2 : FW_FACTORY_UNIT */
static void set_fac_select_firmware(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 fw_update_way = FW_NONE;
	int ret;

	sec_cmd_set_default_result(sec);
	mutex_lock(&wacom->lock);
	switch (sec->cmd_param[0]) {
	case 0:
		fw_update_way = FW_BUILT_IN;
		break;
	case 1:
		fw_update_way = FW_FACTORY_GARAGE;
		break;
	case 2:
		fw_update_way = FW_FACTORY_UNIT;
		break;
	default:
		input_err(true, wacom->dev, "wrong parameter\n");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	ret = wacom_load_fw(wacom, fw_update_way);
	if (ret < 0) {
		input_info(true, wacom->dev, "failed to load fw data\n");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	wacom_unload_fw(wacom);

	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	if (sec->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	mutex_unlock(&wacom->lock);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void set_fac_garage_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, wacom->dev, "%s: abnormal param(%d)\n",
					__func__, sec->cmd_param[0]);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev, "%s: power off state, skip!\n",
				__func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0]) {
		/* change normal mode for garage monitoring */
		mutex_lock(&wacom->mode_lock);
		wacom_set_survey_mode(wacom, EPEN_SURVEY_MODE_NONE);
		mutex_unlock(&wacom->mode_lock);

		ret = wacom_start_stop_cmd(wacom, WACOM_STOP_CMD);
		if (ret != 1) {
			input_err(true, wacom->dev, "%s: failed to set stop cmd, %d\n",
					__func__, ret);
			wacom->fac_garage_mode = 0;
			goto fail_out;
		}

		sec_delay(50);
		wacom->fac_garage_mode = 1;

	} else {
		wacom->fac_garage_mode = 0;

		ret = wacom_start_stop_cmd(wacom, WACOM_STOP_AND_START_CMD);
		if (ret != 1) {
			input_err(true, wacom->dev, "%s: failed to set stop cmd, %d\n",
					__func__, ret);
			goto fail_out;
		}

		/* recovery wacom mode */
		wacom_select_survey_mode(wacom, atomic_read(&wacom->screen_on));
	}

	input_info(true, wacom->dev, "%s: done(%d)\n", __func__, sec->cmd_param[0]);

out:
	if (sec->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	return;

fail_out:
	/* recovery wacom mode */
	wacom_select_survey_mode(wacom, atomic_read(&wacom->screen_on));

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	snprintf(buff, sizeof(buff), "NG");
	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

}

static void get_fac_garage_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	snprintf(buff, sizeof(buff), "garage mode %s",
			wacom->fac_garage_mode ? "IN" : "OUT");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void get_fac_garage_rawdata(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char data[17] = { 0, };
	int ret, retry = 1;
	u8 cmd;

	sec_cmd_set_default_result(sec);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (!wacom->fac_garage_mode) {
		input_err(true, wacom->dev, "not in factory garage mode\n");
		goto out;
	}

	if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
		input_err(true, wacom->dev, "%s: power off state, skip!\n",
				__func__);
		goto out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, skip!\n",
				__func__);
		goto out;
	}

get_garage_retry:
	wacom_enable_irq(wacom, false);

	cmd = COM_REQUEST_GARAGEDATA;
	ret = wacom_send(wacom, &cmd, 1);
	if (ret < 0) {
		input_err(true, wacom->dev, "failed to send request garage data command %d\n", ret);
		sec_delay(30);

		goto fail_out;
	}

	sec_delay(30);

	ret = wacom_recv(wacom, data, sizeof(data));
	if (ret < 0) {
		input_err(true, wacom->dev, "failed to read garage raw data, %d\n", ret);

		wacom->garage_freq0 = wacom->garage_freq1 = 0;
		wacom->garage_gain0 = wacom->garage_gain1 = 0;

		goto fail_out;
	}

	wacom_enable_irq(wacom, true);

	input_info(true, wacom->dev, "%x %x %x %x %x %x %x %x %x %x\n",
			data[2], data[3], data[4], data[5], data[6], data[7],
			data[8], data[9], data[10], data[11]);

	wacom->garage_gain0 = data[6];
	wacom->garage_freq0 = ((u16)data[7] << 8) + data[8];

	wacom->garage_gain1 = data[9];
	wacom->garage_freq1 = ((u16)data[10] << 8) + data[11];

	if (wacom->garage_freq0 == 0 && retry > 0) {
		retry--;
		goto get_garage_retry;
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d,%d",
			wacom->garage_gain0, wacom->garage_freq0,
			wacom->garage_gain1, wacom->garage_freq1);

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	return;

fail_out:
	wacom_enable_irq(wacom, true);
out:
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	snprintf(buff, sizeof(buff), "NG");

	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}
#endif

static void debug(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	wacom->debug_flag = sec->cmd_param[0];

	snprintf(buff, sizeof(buff), "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);
}

static void set_fold_state(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	input_info(true, wacom->dev, "%s: %d\n", __func__, sec->cmd_param[0]);

	/* 1: folding, 0: unfolding */
	wacom->fold_state = sec->cmd_param[0];

	if (wacom->fold_state == FOLD_STATUS_FOLDING) {
		input_info(true, wacom->dev, "%s: folding device, turn off\n", __func__);
		mutex_lock(&wacom->lock);
		wacom_enable_irq(wacom, false);
		atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
		wacom_power(wacom, false);
		atomic_set(&wacom->screen_on, 0);
		wacom->reset_flag = false;
		wacom->survey_mode = EPEN_SURVEY_MODE_NONE;
		wacom->function_result &= ~EPEN_EVENT_SURVEY;
		mutex_unlock(&wacom->lock);
	}

	snprintf(buff, sizeof(buff), "OK\n");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, wacom->dev, "%s: Done\n", __func__);
}

#if !WACOM_SEC_FACTORY
static void sec_wacom_device_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_data *wacom = container_of(sec, struct wacom_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, wacom->dev, "%s: Not support command[%d]\n",
				__func__, sec->cmd_param[0]);
		goto err_out;
	}

	//check probe done & save data
	if (!wacom->probe_done) {
		input_err(true, wacom->dev, "%s : not probe done. skip and data saved\n", __func__);
		goto err_out;
	}

	if (wacom->reset_is_on_going) {
		input_err(true, wacom->dev, "%s: reset is on going, data saved and skip!\n",
				__func__);
		goto err_out;
	}

	if (sec->cmd_param[0]) {
		wacom->epen_blocked = WACOM_ENABLE;
		if (wacom->fold_state == FOLD_STATUS_UNFOLDING)
			sec_input_enable_device(wacom->plat_data->dev);
	} else {
		wacom->epen_blocked = WACOM_DISABLE;
		if (wacom->fold_state == FOLD_STATUS_UNFOLDING)
			sec_input_disable_device(wacom->plat_data->dev);
		wacom->epen_blocked = WACOM_DISABLE;
	}

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, wacom->dev, "%s: %s\n", __func__, buff);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	return;

err_out:
	if (sec->cmd_param[0])
		wacom->epen_blocked = WACOM_ENABLE;
	else
		wacom->epen_blocked = WACOM_DISABLE;
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, wacom->dev, "%s: Fail\n", __func__);

}
#endif

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "NA");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);
}

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD_H("run_elec_test", run_elec_test),},
	{SEC_CMD_H("ready_elec_test", ready_elec_test),},
	{SEC_CMD_H("get_elec_test_ver", get_elec_test_ver),},
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("set_ic_reset", set_ic_reset),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("get_checksum", get_checksum),},
	{SEC_CMD("get_digitizer_connection", get_digitizer_connection),},
	{SEC_CMD("get_garage_connection", get_garage_connection),},
	{SEC_CMD_H("set_saving_mode", set_saving_mode),},
	{SEC_CMD("get_insert_status", get_insert_status),},
	{SEC_CMD_H("set_wcharging_mode", set_wcharging_mode),},
	{SEC_CMD("get_wcharging_mode", get_wcharging_mode),},
	{SEC_CMD_H("set_disable_mode", set_disable_mode),},
	{SEC_CMD("set_screen_off_memo", set_screen_off_memo),},
	{SEC_CMD("set_epen_aod_enable", set_epen_aod_enable),},
	{SEC_CMD("set_aod_lcd_onoff_status", set_aod_lcd_onoff_status),},
	{SEC_CMD("set_epen_aot_enable", set_epen_aot_enable),},
#if WACOM_SEC_FACTORY
	{SEC_CMD_H("set_fac_select_firmware", set_fac_select_firmware),},
	{SEC_CMD("set_fac_garage_mode", set_fac_garage_mode),},
	{SEC_CMD("get_fac_garage_mode", get_fac_garage_mode),},
	{SEC_CMD("get_fac_garage_rawdata", get_fac_garage_rawdata),},
#endif
	{SEC_CMD("debug", debug),},
	{SEC_CMD("set_fold_state", set_fold_state),},
#if !WACOM_SEC_FACTORY
	{SEC_CMD_H("sec_wacom_device_enable", sec_wacom_device_enable),},
#endif
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

static int wacom_get_u64_spec_data(struct wacom_data *wacom, struct device_node *np,
					const char *propname, u64 *target, size_t length)
{
	u32 *temp_data = NULL;
	int ret, i;

	temp_data = kzalloc(length * sizeof(u32), GFP_KERNEL);
	if (!temp_data)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, propname, temp_data, length);
	if (ret) {
		input_err(true, wacom->dev, "%s: failed to read %s data %d\n", __func__, propname, ret);
		kfree(temp_data);
		return ret;
	}

	for (i = 0; i < length; i++)
		target[i] = temp_data[i];

	kfree(temp_data);
	return 0;
}

static int wacom_sec_elec_init(struct wacom_data *wacom, int pos)
{
	struct wacom_elec_data *edata = NULL;
	struct device_node *np = wacom->client->dev.of_node;
	u32 tmp[2];
	int max_len;
	int ret;
	int i;
	u8 tmp_name[30] = { 0 };

	snprintf(tmp_name, sizeof(tmp_name), "wacom_elec%d", pos);
	input_info(true, wacom->dev, "%s: init : %s\n", __func__, tmp_name);

	np = of_get_child_by_name(np, tmp_name);
	if (!np) {
		input_err(true, wacom->dev, "%s: node is not exist for elec\n", __func__);
		return -EINVAL;
	}

	edata = devm_kzalloc(wacom->dev, sizeof(*edata), GFP_KERNEL);
	if (!edata)
		return -ENOMEM;

	wacom->edatas[pos] = edata;

	ret = of_property_read_u32_array(np, "spec_ver", tmp, 2);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read sepc ver %d\n", ret);
		return -EINVAL;
	}
	edata->spec_ver[0] = tmp[0];
	edata->spec_ver[1] = tmp[1];

	ret = of_property_read_u32_array(np, "max_channel", tmp, 2);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read max channel %d\n", ret);
		return -EINVAL;
	}
	edata->max_x_ch = tmp[0];
	edata->max_y_ch = tmp[1];

	ret = of_property_read_u32(np, "shift_value", tmp);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read max channel %d\n", ret);
		return -EINVAL;
	}
	edata->shift_value = tmp[0];

	input_info(true, wacom->dev, "channel(%d %d), spec_ver %d.%d shift_value %d\n",
			edata->max_x_ch, edata->max_y_ch, edata->spec_ver[0], edata->spec_ver[1], edata->shift_value);

	max_len = edata->max_x_ch + edata->max_y_ch;

	edata->elec_data = devm_kzalloc(wacom->dev,
			max_len * max_len * sizeof(u16),
			GFP_KERNEL);
	if (!edata->elec_data)
		return -ENOMEM;

	edata->xx = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(u16), GFP_KERNEL);
	edata->xy = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(u16), GFP_KERNEL);
	edata->yx = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(u16), GFP_KERNEL);
	edata->yy = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(u16), GFP_KERNEL);
	if (!edata->xx || !edata->xy || !edata->yx || !edata->yy)
		return -ENOMEM;

	edata->xx_self = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(u16), GFP_KERNEL);
	edata->yy_self = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(u16), GFP_KERNEL);
	edata->xy_edg = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(u16), GFP_KERNEL);
	edata->yx_edg = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(u16), GFP_KERNEL);
	if (!edata->xx_self || !edata->yy_self || !edata->xy_edg || !edata->yx_edg)
		return -ENOMEM;

	edata->xx_xx = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->xy_xy = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yx_yx = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->yy_yy = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->xx_xx || !edata->xy_xy || !edata->yx_yx || !edata->yy_yy)
		return -ENOMEM;

	edata->xy_xy_edg = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yx_yx_edg = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->xy_xy_edg || !edata->yx_yx_edg)
		return -ENOMEM;

	edata->rxx = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->rxy = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->ryx = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->ryy = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->rxx || !edata->rxy || !edata->ryx || !edata->ryy)
		return -ENOMEM;

	edata->rxy_edg = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->ryx_edg = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->rxy_edg || !edata->ryx_edg)
		return -ENOMEM;

	edata->drxx = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->drxy = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->dryx = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->dryy = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->drxx || !edata->drxy || !edata->dryx || !edata->dryy)
		return -ENOMEM;

	edata->drxy_edg = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->dryx_edg = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->drxy_edg || !edata->dryx_edg)
		return -ENOMEM;

	edata->xx_ref = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->xy_ref = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yx_ref = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->yy_ref = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->xy_edg_ref = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yx_edg_ref = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->xx_ref || !edata->xy_ref || !edata->yx_ref || !edata->yy_ref || !edata->xy_edg_ref || !edata->yx_edg_ref)
		return -ENOMEM;

	ret = wacom_get_u64_spec_data(wacom, np, "xx_ref", edata->xx_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xx reference data %d\n", ret);
		return -EINVAL;
	}

	ret = wacom_get_u64_spec_data(wacom, np, "xy_ref", edata->xy_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xy reference data %d\n", ret);
		return -EINVAL;
	}

	ret = wacom_get_u64_spec_data(wacom, np, "yx_ref", edata->yx_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yx reference data %d\n", ret);
		return -EINVAL;
	}

	ret = wacom_get_u64_spec_data(wacom, np, "yy_ref", edata->yy_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yy reference data %d\n", ret);
		return -EINVAL;
	}

	ret = wacom_get_u64_spec_data(wacom, np, "xy_ref_edg", edata->xy_edg_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xy reference edg data %d\n", ret);
		return -EINVAL;
	}

	ret = wacom_get_u64_spec_data(wacom, np, "yx_ref_edg", edata->yx_edg_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yx reference edg data %d\n", ret);
		return -EINVAL;
	}

	edata->xx_spec = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->xy_spec = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yx_spec = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->yy_spec = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->xx_spec || !edata->xy_spec || !edata->yx_spec || !edata->yy_spec)
		return -ENOMEM;

	edata->xx_self_spec = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yy_self_spec = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->xx_self_spec || !edata->yy_self_spec)
		return -ENOMEM;

	ret = wacom_get_u64_spec_data(wacom, np, "xx_spec", edata->xx_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xx spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->xx_spec[i] = edata->xx_spec[i] * POWER_OFFSET;

	ret = wacom_get_u64_spec_data(wacom, np, "xy_spec", edata->xy_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xy spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->xy_spec[i] = edata->xy_spec[i] * POWER_OFFSET;

	ret = wacom_get_u64_spec_data(wacom, np, "yx_spec", edata->yx_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yx spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->yx_spec[i] = edata->yx_spec[i] * POWER_OFFSET;

	ret = wacom_get_u64_spec_data(wacom, np, "yy_spec", edata->yy_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yy spec data %d\n", ret);
		return -EINVAL;
	}

	ret = wacom_get_u64_spec_data(wacom, np, "xx_spec_self", edata->xx_self_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xx self spec data %d\n", ret);
		return -EINVAL;
	}

	ret = wacom_get_u64_spec_data(wacom, np, "yy_spec_self", edata->yy_self_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yy self spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->yy_spec[i] = edata->yy_spec[i] * POWER_OFFSET;

	edata->rxx_ref = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->rxy_ref = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->ryx_ref = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->ryy_ref = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->rxx_ref || !edata->rxy_ref || !edata->ryx_ref || !edata->ryy_ref)
		return -ENOMEM;

	ret = wacom_get_u64_spec_data(wacom, np, "rxx_ref", edata->rxx_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xx ratio reference data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->rxx_ref[i] = edata->rxx_ref[i] * POWER_OFFSET / power(edata->shift_value);

	ret = wacom_get_u64_spec_data(wacom, np, "rxy_ref", edata->rxy_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xy ratio reference data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->rxy_ref[i] = edata->rxy_ref[i] * POWER_OFFSET / power(edata->shift_value);

	ret = wacom_get_u64_spec_data(wacom, np, "ryx_ref", edata->ryx_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yx ratio reference data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->ryx_ref[i] = edata->ryx_ref[i] * POWER_OFFSET / power(edata->shift_value);

	ret = wacom_get_u64_spec_data(wacom, np, "ryy_ref", edata->ryy_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yy ratio reference data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->ryy_ref[i] = edata->ryy_ref[i] * POWER_OFFSET / power(edata->shift_value);

	edata->drxx_spec = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->drxy_spec = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->dryx_spec = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->dryy_spec = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->drxx_spec || !edata->drxy_spec || !edata->dryx_spec || !edata->dryy_spec)
		return -ENOMEM;

	edata->drxy_edg_spec = devm_kzalloc(wacom->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->dryx_edg_spec = devm_kzalloc(wacom->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->drxy_edg_spec || !edata->dryx_edg_spec)
		return -ENOMEM;


	ret = wacom_get_u64_spec_data(wacom, np, "drxx_spec", edata->drxx_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xx difference ratio spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->drxx_spec[i] = edata->drxx_spec[i] * POWER_OFFSET / power(edata->shift_value);

	ret = wacom_get_u64_spec_data(wacom, np, "drxy_spec", edata->drxy_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xy difference ratio spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->drxy_spec[i] = edata->drxy_spec[i] * POWER_OFFSET / power(edata->shift_value);

	ret = wacom_get_u64_spec_data(wacom, np, "dryx_spec", edata->dryx_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yx difference ratio spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->dryx_spec[i] = edata->dryx_spec[i] * POWER_OFFSET / power(edata->shift_value);

	ret = wacom_get_u64_spec_data(wacom, np, "dryy_spec", edata->dryy_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yy difference ratio spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->dryy_spec[i] = edata->dryy_spec[i] * POWER_OFFSET / power(edata->shift_value);

	ret = wacom_get_u64_spec_data(wacom, np, "drxy_spec_edg", edata->drxy_edg_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read xy difference ratio edg spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->drxy_edg_spec[i] = edata->drxy_edg_spec[i] * POWER_OFFSET / power(edata->shift_value);

	ret = wacom_get_u64_spec_data(wacom, np, "dryx_spec_edg", edata->dryx_edg_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, wacom->dev,
				"failed to read yx difference ratio edg  spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->dryx_edg_spec[i] = edata->dryx_edg_spec[i] * POWER_OFFSET / power(edata->shift_value);

	return 1;
}

int wacom_sec_fn_init(struct wacom_data *wacom)
{
	int retval = 0;
	int i = 0;

	retval = sec_cmd_init(&wacom->sec, wacom->dev, sec_cmds, ARRAY_SIZE(sec_cmds),
			SEC_CLASS_DEVT_WACOM, &epen_attr_group);
	if (retval < 0) {
		input_err(true, wacom->dev, "failed to sec_cmd_init\n");
		return retval;
	}

	for (i = EPEN_ELEC_DATA_MAIN ; i < EPEN_ELEC_DATA_MAX; i++)
		wacom_sec_elec_init(wacom, i);

	return 0;

}

void wacom_sec_remove(struct wacom_data *wacom)
{
	sysfs_remove_link(&wacom->sec.fac_dev->kobj, "input");
	sec_cmd_exit(&wacom->sec, SEC_CLASS_DEVT_WACOM);
}
