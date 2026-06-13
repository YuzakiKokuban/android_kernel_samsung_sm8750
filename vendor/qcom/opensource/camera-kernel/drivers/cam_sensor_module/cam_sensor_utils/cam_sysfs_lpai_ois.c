/* Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "cam_sysfs_lpai_ois.h"
#include "cam_actuator_core.h"
#include "cam_ois_core.h"
#include "cam_sensor_cmn_header.h"
#include "cam_debug_util.h"
#include "cam_sysfs_init.h"
#include "cam_sec_eeprom_core.h"
#include "cam_sec_ois.h"

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
#include "cam_hw_bigdata.h"
#endif
static int ois_power = 0;

bool ois_is_camera_running(int* mask)
{
	bool is_running = false;
	struct cam_ois_ctrl_t *o_ctrl = NULL;
	int i = 0;

	for (i = 0; i < SEC_SENSOR_ID_MAX; i ++) {
		if (!g_o_ctrls[i])
			continue;

		o_ctrl = g_o_ctrls[i];
		if (o_ctrl->cam_ois_state != CAM_OIS_INIT) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"[%d] Already used by camera, ois_state %d",
				i, o_ctrl->cam_ois_state);
			if (mask != NULL)
				*mask |= (1 << i);
			is_running = true;
		}
	}

	return is_running;
}

int ois_get_first_running_camera(void)
{
	int i = 0;
	struct cam_ois_ctrl_t *o_ctrl = NULL;

	 for (i = 0; i < SEC_SENSOR_ID_MAX; i ++) {
		 if (!g_o_ctrls[i])
			 continue;

		 o_ctrl = g_o_ctrls[i];
		 CAM_DBG(CAM_SENSOR_UTIL,
				"[%d] ois_state %d",
				i, o_ctrl->cam_ois_state);
		 if (o_ctrl->cam_ois_state == CAM_OIS_START)
			return i;
	}

	return -1;
}

static int ois_notify_message(
	struct cam_ois_ctrl_t *o_ctrl, uint32_t event_id, void *payload)
{
	int rc = 0;
	struct cam_ois_message msg = { 0 };

	if (o_ctrl == NULL)
		return -1;

	memset(&msg, 0, sizeof(msg));
	msg.u.node_msg.event_type = OIS_FACTORY_CMD;
	msg.u.node_msg.event_cause = o_ctrl->soc_info.index;
	memcpy(msg.u.node_msg.payload, payload, sizeof(msg.u.node_msg.payload));

	rc = cam_ois_notify_message(
			&(o_ctrl->v4l2_dev_str.sd),
			&msg,
			event_id,
			V4L_EVENT_CAM_OIS_EVENT);

	return rc ? rc : 0;
}

static int ois_notify_message_and_wait(
	struct cam_ois_ctrl_t *o_ctrl, uint32_t event_id,
	void *payload, uint32_t timeout, char *buf,
	char *temp_buf, ssize_t temp_buf_size)
{
	int rc = 0, ret = -1;

	if (o_ctrl == NULL)
		return -1;

	mutex_lock(&o_ctrl->thread_mutex);

	if (temp_buf && temp_buf_size)
		memset(temp_buf, 0, temp_buf_size);

	o_ctrl->wakeup_condition = false;

	ois_notify_message(o_ctrl, event_id, payload);

	ret = wait_event_timeout(o_ctrl->wait,
		o_ctrl->wakeup_condition, timeout * HZ);
	if (ret == 0)
		CAM_ERR(CAM_SENSOR_UTIL, "timeout ret %d", ret);

	if (ret && buf && temp_buf && temp_buf_size)
		rc = scnprintf(buf, PAGE_SIZE, "%s", temp_buf);

	mutex_unlock(&o_ctrl->thread_mutex);

	return ret ? rc : -1;
}

int ois_handle_factory_test_result(
	struct cam_factory_test_cmd* dev_config)
{
	int rc = 0;
	uint8_t test_id = OIS_FCT_MAX;
	struct ois_fct_general_t* factory_payload = NULL;

	if (dev_config->test_status <= 0)
		return rc;

	factory_payload = (struct ois_fct_general_t*)dev_config->payload;
	test_id = factory_payload->test_id;

	switch(test_id) {
		case OIS_FCT_OIS_AUTOTEST:
			ois_handle_ois_autotest_result(dev_config->payload);
			break;
		case OIS_FCT_GYRO_NOISE_STDEV:
			ois_handle_gyro_noise_stdev_result(dev_config->payload);
			break;
		case OIS_FCT_GYRO_SELFTEST:
			ois_handle_gyro_selftest_result(dev_config->payload);
			break;
		case OIS_FCT_GYRO_CALIBRATION:
			ois_handle_gyro_calibration_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_VALID_CHECK:
			ois_handle_ois_valid_check_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_GET_FW:
			ois_handle_ois_get_fw_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_HALL_CAL_CHK:
			ois_handle_ois_hall_cal_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_CROSS_TALK:
			ois_handle_ois_cross_talk_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_GET_MGLESS:
			ois_handle_ois_get_mgless_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_HALL_POSITION:
			ois_handle_ois_hall_position_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_GET_EXIF:
			ois_handle_ois_get_exif_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_RESET_CHECK:
			ois_handle_ois_reset_check_result(dev_config->payload);
			break;
		case OIS_FCT_OIS_CENTER_SHIFT:
		case OIS_FCT_OIS_SET_MODE:
			break;
		case OIS_FCT_FORCE_KP:
			CAM_ERR(CAM_SENSOR_UTIL, "Force Kernel Panic to debug LPAIOIS issue");
			msleep(20);
			BUG_ON(1);
			break;
		case OIS_FCT_OIS_ERROR:
			ois_handle_ois_error(dev_config->payload);
			break;
		default:
			CAM_ERR(CAM_SENSOR_UTIL, "Not supported test id %u", test_id);
			break;
	};

	return rc;
}

char mgless_buf[1024];
void ois_handle_ois_get_mgless_result(uint8_t* payload)
{
	struct ois_fct_mgless_result_t* result_payload =
		(struct ois_fct_mgless_result_t*)payload;
	int offset = 0, i = 0;
	uint32_t mgless = result_payload->mgless_flag;
	int mglessY = 0, mglessX = 0;

	for (i = 0; i < CUR_MODULE_NUM; i++) {
		mglessX = ((mgless >> (2 * i)) & 0x01) ? 1 : 0;
		mglessY = ((mgless >> (2 * i)) & 0x02) ? 1 : 0;

		offset += sprintf(mgless_buf + offset, "%d, %d%s",
			mglessX, mglessY, ((i + 1) != CUR_MODULE_NUM ? ", " : "\0"));
	}
}

static ssize_t ois_mgless_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0, i = 0, offset = 0;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_general_t test_payload;
	char init_str[256] = { 0, };

	if (ois_power == 0) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, need to power up");
		goto error;
	}

	if (ois_is_camera_running(NULL)) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, camera is running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_GET_MGLESS;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_EVENT,
		(void*)&test_payload, 1, buf,
		mgless_buf, sizeof(mgless_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	for (i = 0; i < CUR_MODULE_NUM; i++)
		offset += scnprintf(init_str + offset,
			sizeof(init_str) - offset, "0, 0%s",
			((i + 1) != CUR_MODULE_NUM ? ", " : "\0"));

	return scnprintf(buf, PAGE_SIZE, "%s", init_str);
}

long raw_init_x = 0, raw_init_y = 0, raw_init_z = 0;
uint32_t ois_autotest_args[3] = { 150, 0x05, 0x2A }; // threshold, frequency, amplitude
char autotest_buf[1024] = { 0, };
static ssize_t ois_autotest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0, i = 0, offset = 0;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_autotest_t test_payload;
	char init_str[256] = { 0, };

	if (ois_power == 0) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, need to power up");
		goto error;
	}

	if (ois_is_camera_running(NULL)) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, camera is running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	for (i = 0; i < SEC_SENSOR_ID_MAX; i ++) {
		if (g_a_ctrls[i] == NULL || g_o_ctrls[i] == NULL)
			continue;
		cam_actuator_power_up(g_a_ctrls[i]);
		if (i != SEC_TELE2_SENSOR)
			cam_actuator_move_for_ois_test(g_a_ctrls[i]);
	}
	msleep(100);

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_AUTOTEST;
	test_payload.threshold = ois_autotest_args[0];
	test_payload.frequency = ois_autotest_args[1];
	test_payload.amplitude = ois_autotest_args[2];

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_EVENT,
		(void*)&test_payload, 5, buf,
		autotest_buf, sizeof(autotest_buf));

	for (i = 0; i < SEC_SENSOR_ID_MAX; i++) {
		if (g_a_ctrls[i] == NULL || g_o_ctrls[i] == NULL)
			continue;
		cam_actuator_power_down(g_a_ctrls[i]);
	}

	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	for (i = 0; i < CUR_MODULE_NUM; i++)
		offset += scnprintf(init_str + offset,
			sizeof(init_str) - offset, "fail, 0, fail, 0%s",
			((i + 1) != CUR_MODULE_NUM ? ", " : ""));

	return scnprintf(buf, PAGE_SIZE, "%s", init_str);
}

static ssize_t ois_autotest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char* token	  = NULL;
	char* pContext	  = NULL;
	uint32_t token_cnt = 0;
	uint32_t value	  = 0;
	uint32_t args[3] = { 150, 0x05, 0x2A };

	if (buf == NULL)
		return -1;

	pContext = (char*)buf;
	while ((token = strsep(&pContext, ","))) {
		if (kstrtoint(token, 10, &value))
			return -1;
		args[token_cnt++] = value;
		if (token_cnt >= 3)
			break;
	}

	ois_autotest_args[0] = args[0];
	ois_autotest_args[1] =
		(args[1] >= 1 && args[1] <= 255) ? args[1] : 0x05 ;
	ois_autotest_args[2]	=
		(args[2] >= 1 && args[2] <= 100) ? args[2] : 0x2A ;

	CAM_INFO(CAM_SENSOR_UTIL, "threshold %u, frequency %d, aplitude %d",
		ois_autotest_args[0], ois_autotest_args[1], ois_autotest_args[2]);

	return size;
}

#define GET_BIT(msk, idx)	   (((msk) >> (idx)) & 0x1)
void ois_handle_ois_autotest_result(uint8_t* payload)
{
	int offset = 0, i = 0;
	struct ois_fct_autotest_result_t* result_payload =
		(struct ois_fct_autotest_result_t*)payload;
	uint8_t test_result = 0;
	uint16_t* max_diff = NULL;
	uint8_t* error_count = NULL;
	uint8_t mgless_x = 0, mgless_y = 0;
	uint8_t result_x1 = 0, result_x2 = 0, result_y1 = 0, result_y2 = 0;

	for (i = 0; i < CUR_MODULE_NUM; i++) {
		test_result = result_payload->test_results[i];
		mgless_y = GET_BIT(test_result, 5);
		mgless_x = GET_BIT(test_result, 4);
		result_y2 = GET_BIT(test_result, 3);
		result_y1 = GET_BIT(test_result, 2);
		result_x2 = GET_BIT(test_result, 1);
		result_x1 = GET_BIT(test_result, 0);

		max_diff = &result_payload->max_diffs[3 * i];
		error_count = &result_payload->error_counts[3 * i];

		offset += sprintf(autotest_buf + offset, "%s%s, %d",
			(i > 0 ? ", " : ""),
			(result_x1 ? "pass" : "fail"),
			(result_x1 ? 0 : max_diff[0]));
		if (mgless_x || mgless_y) {
			if (mgless_x) {
				offset += sprintf(autotest_buf + offset, ", %s, %d",
					(result_x2 ? "pass" : "fail"),
					(result_x2 ? 0 : max_diff[2]));
			} else {
				offset += sprintf(autotest_buf + offset, ", pass, N");
			}
		}

		offset += sprintf(autotest_buf + offset, ", %s, %d",
			(result_y1 ? "pass" : "fail"),
			(result_y1 ? 0 : max_diff[1]));
		 if (mgless_x || mgless_y) {
			if (mgless_y) {
				offset += sprintf(autotest_buf + offset, ", %s, %d",
					(result_y2 ? "pass" : "fail"),
					(result_y2 ? 0 : max_diff[2]));
			} else {
				offset += sprintf(autotest_buf + offset, ", pass, N");
			}
		}

		if (mgless_x || mgless_y) {
			CAM_INFO(CAM_OIS, "[Module#%d] threshold = %d, sinx = %d, siny = %d, %s = %d, sinx_count = %d, siny_count = %d, %s_count = %d",
				i + 1, ois_autotest_args[0],
				max_diff[0], max_diff[1],
				(mgless_x ? "sinx2" : "siny2"),
				max_diff[2],
				error_count[0], error_count[1],
				(mgless_x ? "sinx2" : "siny2"),
				error_count[2]);
		}
		else
		{
			CAM_INFO(CAM_OIS, "[Module#%d] threshold = %d, sinx = %d, siny = %d, sinx_count = %d, siny_count = %d",
				i + 1, ois_autotest_args[0],
				max_diff[0], max_diff[1],
				error_count[0], error_count[1]);
		}
	}
}

static int ois_power_up(void)
{
	struct cam_ois_ctrl_t *o_ctrl = NULL;
	int rc = 0, i = 0;

	for (i = 0; i < SEC_SENSOR_ID_MAX; i ++) {
		if (g_o_ctrls[i] == NULL)
			continue;

		o_ctrl = g_o_ctrls[i];
		mutex_lock(&(o_ctrl->ois_mutex));
		rc = cam_ois_power_up(o_ctrl);
		if (rc < 0) {
			mutex_unlock(&(o_ctrl->ois_mutex));
			goto power_down;
		}
		CAM_INFO(CAM_SENSOR_UTIL, "[%d] power up", i);
		mutex_unlock(&(o_ctrl->ois_mutex));
	}

	ois_power = 1;

	return rc;
power_down:
	for (i = (i - 1); i >=0 ; i--) {
		if (g_o_ctrls[i] == NULL)
			continue;

		o_ctrl = g_o_ctrls[i];
		mutex_lock(&(o_ctrl->ois_mutex));
		rc = cam_ois_power_down(o_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR_UTIL,
				"[%d] fail to power down", i);
		mutex_unlock(&(o_ctrl->ois_mutex));
	}

	return rc;
}

static int ois_power_down(void)
{
	struct cam_ois_ctrl_t *o_ctrl = NULL;
	int rc = 0, i = 0;

	for (i = 0; i < SEC_SENSOR_ID_MAX; i ++) {
		if (g_o_ctrls[i] == NULL)
			continue;

		o_ctrl = g_o_ctrls[i];
		mutex_lock(&(o_ctrl->ois_mutex));
		rc = cam_ois_power_down(o_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR_UTIL,
			"[%d] fail to power down", i);

		CAM_INFO(CAM_SENSOR_UTIL, "[%d] power down", i);
		mutex_unlock(&(o_ctrl->ois_mutex));
	}

	ois_power = 0;
	return rc;
}

static ssize_t ois_power_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0, onoff = 0;

	if (buf == NULL || kstrtouint(buf, 10, &onoff))
		return -1;

	if ((ois_power == 0 && onoff == 0) ||
		(ois_power > 0 && onoff > 0))
		return size;

	if (ois_is_camera_running(NULL))
		return size;

	if (onoff)
		rc = ois_power_up();
	else
		rc = ois_power_down();

	if (rc == 0)
		return size;
	return 0;
}

char gyro_calibration_buf[1024];
void ois_handle_gyro_calibration_result(uint8_t* payload)
{
	int result = 0;
	struct ois_fct_calibration_result_t* result_payload =
		(struct ois_fct_calibration_result_t*)payload;
	uint8_t test_result = result_payload->test_result;
	int xgzero = result_payload->gyro_zero_offset_x;
	int ygzero = result_payload->gyro_zero_offset_y;
	int zgzero = result_payload->gyro_zero_offset_z;

	if (test_result && (result_payload->offset_test_oiserr == 0))
		result = 1;

	CAM_INFO(CAM_OIS, "result %d, raw_data_x %d, raw_data_y %d, raw_data_z %d",
		result, xgzero, ygzero, zgzero);

	scnprintf(gyro_calibration_buf, sizeof(gyro_calibration_buf),
		"%d,%s%d.%03d,%s%d.%03d,%s%d.%03d\n", result,
		(xgzero >= 0 ? "" : "-"), abs(xgzero) / 1000, abs(xgzero) % 1000,
		(ygzero >= 0 ? "" : "-"), abs(ygzero) / 1000, abs(ygzero) % 1000,
		(zgzero >= 0 ? "" : "-"), abs(zgzero) / 1000, abs(zgzero) % 1000);
}

static ssize_t gyro_calibration_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_general_t test_payload;

	if (ois_power == 0) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, need to power up");
		goto error;
	}

	if (ois_is_camera_running(NULL)) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, camera is running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_GYRO_CALIBRATION;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_EVENT,
		(void*)&test_payload, 1, buf,
		gyro_calibration_buf, sizeof(gyro_calibration_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	return scnprintf(buf, PAGE_SIZE, "0,0.0,0.0,0.0\n");
}

char noise_stdev_buf[1024];
void ois_handle_gyro_noise_stdev_result(uint8_t* payload)
{
	int result = 0;
	struct ois_fct_noise_stdev_result_t* result_payload =
		(struct ois_fct_noise_stdev_result_t*)payload;
	uint8_t test_result = result_payload->test_result;
	int stdev_x = result_payload->gyro_noise_stdev_x;
	int stdev_y = result_payload->gyro_noise_stdev_y;

	if (test_result)
		result = 1;

	CAM_INFO(CAM_OIS, "result: %d, stdev_x: %d, stdev_y: %d", test_result, stdev_x, stdev_y);

	scnprintf(noise_stdev_buf, sizeof(noise_stdev_buf),
		"%d,%s%d.%03d,%s%d.%03d\n", result,
		(stdev_x >= 0 ? "" : "-"), abs(stdev_x) / 1000, abs(stdev_x) % 1000,
		(stdev_y >= 0 ? "" : "-"), abs(stdev_y) / 1000, abs(stdev_y) % 1000);
 }

static ssize_t gyro_noise_stdev_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0, index = -1;
	uint32_t event_id = V4L_EVENT_CAM_OIS_FACTORY_EVENT;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_general_t test_payload;

	index = ois_get_first_running_camera();
	if ((index < 0) && !ois_power) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, need to power up");
		goto error;
	}

	if (index >= 0) {
		o_ctrl = g_o_ctrls[index];
		event_id = V4L_EVENT_CAM_OIS_FACTORY_WITH_CAMERA_EVENT;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_GYRO_NOISE_STDEV;

	rc = ois_notify_message_and_wait(o_ctrl,
		event_id,
		(void*)&test_payload, 1, buf,
		noise_stdev_buf, sizeof(noise_stdev_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	return scnprintf(buf, PAGE_SIZE, "0,0.0,0.0\n");
}

char gyro_selftest_buf[1024];
void ois_handle_gyro_selftest_result(uint8_t* payload)
{
	struct ois_fct_selftest_result_t* result_payload =
		(struct ois_fct_selftest_result_t*)payload;
	uint8_t total_result = result_payload->total_result;
	int xgzero = result_payload->gyro_zero_offset_x;
	int ygzero = result_payload->gyro_zero_offset_y;
	int zgzero = result_payload->gyro_zero_offset_z;

	CAM_INFO(CAM_SENSOR_UTIL, "Result : 0 (success), 1 (offset fail), 2 (selftest fail) , 3 (both fail)");
	CAM_INFO(CAM_SENSOR_UTIL, "Result : %d, result x = %d, result y = %d, result z = %d",
		total_result, xgzero, ygzero, zgzero);

	scnprintf(gyro_selftest_buf, sizeof(gyro_selftest_buf),
		"%d,%s%d.%03d,%s%d.%03d,%s%d.%03d\n", total_result,
		(xgzero >= 0 ? "" : "-"), abs(xgzero) / 1000, abs(xgzero) % 1000,
		(ygzero >= 0 ? "" : "-"), abs(ygzero) / 1000, abs(ygzero) % 1000,
		(zgzero >= 0 ? "" : "-"), abs(zgzero) / 1000, abs(zgzero) % 1000);
}

static ssize_t gyro_selftest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_selftest_t test_payload;

	if (ois_power == 0) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, need to power up");
		goto error;
	}

	if (ois_is_camera_running(NULL)) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, camera is running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_GYRO_SELFTEST;
	test_payload.gyro_zero_offset_x = raw_init_x;
	test_payload.gyro_zero_offset_y = raw_init_y;
	test_payload.gyro_zero_offset_z = raw_init_z;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_EVENT,
		(void*)&test_payload, 5, buf,
		gyro_selftest_buf, sizeof(gyro_selftest_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	return scnprintf(buf, PAGE_SIZE, "3,0.0,0.0,0.0\n");
}

int cam_ois_parsing_raw_data(
	uint8_t *buf, uint32_t buf_size,
	long *raw_data_x, long *raw_data_y, long *raw_data_z)
{
	int ret = 0, i = 0, j = 0, comma_offset = 0;
	bool detect_comma = false;
	int comma_offset_z = 0;
	bool detect_comma_z = false;
	char efs_data[MAX_EFS_DATA_LENGTH] = { 0 };
	uint32_t max_buf_size = buf_size;

	CAM_DBG(CAM_OIS, "E");

	i = 0;
	detect_comma = false;
	for (i = 0; i < buf_size; i++) {
		if (*(buf + i) == ',') {
			comma_offset = i;
			detect_comma = true;
			break;
		}
	}

	for (i = comma_offset + 1; i < buf_size; i++) {
	    if (*(buf + i) == ',') {
			comma_offset_z = i;
			detect_comma_z = true;
			break;
		}
	}
	max_buf_size = comma_offset_z;

	if (detect_comma) {
		memset(efs_data, 0x00, sizeof(efs_data));
		j = 0;
		for (i = 0; i < comma_offset; i++) {
			if (buf[i] != '.') {
				efs_data[j] = buf[i];
				j++;
			}
		}
		ret = kstrtol(efs_data, 10, raw_data_x);

		memset(efs_data, 0x00, sizeof(efs_data));
		j = 0;
		for (i = comma_offset + 1; i < max_buf_size; i++) {
			if (buf[i] != '.') {
				efs_data[j] = buf[i];
				j++;
			}
		}
		ret = kstrtol(efs_data, 10, raw_data_y);

		if (detect_comma_z) {
			memset(efs_data, 0x00, sizeof(efs_data));
			j = 0;
			for (i = comma_offset_z + 1; i < buf_size; i++) {
				if (buf[i] != '.') {
					efs_data[j] = buf[i];
					j++;
				}
			}
			ret = kstrtol(efs_data, 10, raw_data_z);
		}
	} else {
		CAM_INFO(CAM_OIS, "cannot find delimeter");
		ret = -1;
	}

	CAM_INFO(CAM_OIS, "X raw_x = %ld, raw_y = %ld, raw_z = %ld",
		*raw_data_x, *raw_data_y, *raw_data_z);

	return ret;
}


static ssize_t gyro_rawdata_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint8_t raw_data[MAX_EFS_DATA_LENGTH] = {0, };
	long raw_data_x = 0, raw_data_y = 0, raw_data_z = 0;
	long efs_size = 0;

	if (ois_power) {
		if (size > MAX_EFS_DATA_LENGTH || size == 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "count is abnormal, count = %d", size);
			return 0;
		}

		scnprintf(raw_data, sizeof(raw_data), "%s", buf);
		efs_size = strlen(raw_data);
		cam_ois_parsing_raw_data(
			raw_data, efs_size,
			&raw_data_x, &raw_data_y, &raw_data_z);

		raw_init_x = raw_data_x;
		raw_init_y = raw_data_y;
		raw_init_z = raw_data_z;

		CAM_INFO(CAM_SENSOR_UTIL, "efs data = %s, size = %ld, raw x = %ld, raw y = %ld, raw z = %ld",
			buf, efs_size, raw_data_x, raw_data_y, raw_data_z);
	} else {
		CAM_ERR(CAM_SENSOR_UTIL, "OIS power is not enabled.");
	}
	return size;
}

static ssize_t gyro_rawdata_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	long raw_data_x = 0, raw_data_y = 0, raw_data_z = 0;

	raw_data_x = raw_init_x;
	raw_data_y = raw_init_y;
	raw_data_z = raw_init_z;

	CAM_INFO(CAM_SENSOR_UTIL, "raw data x = %ld, raw data y = %ld, raw data z = %ld",
		raw_data_x, raw_data_y, raw_data_z);

	rc = scnprintf(buf, PAGE_SIZE, "%s%ld.%03ld,%s%ld.%03ld,%s%ld.%03ld\n",
		(raw_data_x >= 0 ? "" : "-"), abs(raw_data_x) / 1000, abs(raw_data_x) % 1000,
		(raw_data_y >= 0 ? "" : "-"), abs(raw_data_y) / 1000, abs(raw_data_y) % 1000,
		(raw_data_z >= 0 ? "" : "-"), abs(raw_data_z) / 1000, abs(raw_data_z) % 1000);

	CAM_INFO(CAM_SENSOR_UTIL, "%s", buf);

	if (rc)
		return rc;
	return 0;
}

char ois_fw_full[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
void ois_handle_ois_get_fw_result(uint8_t* payload)
{
	struct ois_fct_fw_ver_result_t* result_payload =
		(struct ois_fct_fw_ver_result_t*)payload;

	scnprintf(ois_fw_full, sizeof(ois_fw_full), "%s %s\n",
		result_payload->version, result_payload->version);
	CAM_INFO(CAM_SENSOR_UTIL, "[FW_DBG] OIS_fw_ver : %s", ois_fw_full);
}

static ssize_t ois_fw_full_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0, need_power = 0;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_general_t test_payload;

	if (ois_is_camera_running(NULL)) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, camera is running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	need_power = (ois_power == 0);
	if (need_power)
		ois_power_up();

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_GET_FW;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_EVENT,
		(void*)&test_payload, 1, buf,
		ois_fw_full, sizeof(ois_fw_full));

	if (need_power)
		ois_power_down();

	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	return scnprintf(buf, PAGE_SIZE, "NULL NULL\n");
}

static ssize_t ois_fw_full_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	CAM_INFO(CAM_SENSOR_UTIL, "[FW_DBG] buf : %s", buf);
	scnprintf(ois_fw_full, sizeof(ois_fw_full), "%s", buf);

	return size;
}

char ois_debug[40] = "NULL NULL NULL\n";
void ois_handle_ois_get_exif_result(uint8_t* payload)
{
	uint32_t offset = 0;
	const char* exif_tag = "ssois";
	struct ois_fct_exif_result_t* result_payload =
		(struct ois_fct_exif_result_t*)payload;

	offset = scnprintf(ois_debug, sizeof(ois_debug), "%s%s %s %s %x %x %x",
		exif_tag, result_payload->version, result_payload->version,
		"ISNULL", result_payload->ois_err, result_payload->ois_status,
		result_payload->ois_mode);

	ois_debug[offset] = '\0';
}

static ssize_t ois_exif_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int index = -1;

	index = ois_get_first_running_camera();
	if (index < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "camera is not running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "[FW_DBG] ois_debug : %s", ois_debug);
	return scnprintf(buf, PAGE_SIZE, "%s", ois_debug);

error:
	return scnprintf(buf, PAGE_SIZE, "NULL NULL NULL\n");
}

char reset_check_buf[1024];
void ois_handle_ois_reset_check_result(uint8_t* payload)
{
	uint32_t offset = 0;
	struct ois_fct_reset_check_result_t* result_payload =
		(struct ois_fct_reset_check_result_t*)payload;

	offset = scnprintf(reset_check_buf, sizeof(reset_check_buf),
        "%d",result_payload->ois_mode);

	reset_check_buf[offset] = '\0';
}

void ois_handle_ois_error(uint8_t* payload)
{
	struct ois_fct_hwbigdata_t* factory_payload = NULL;
	factory_payload = (struct ois_fct_hwbigdata_t*)payload;

	CAM_INFO(CAM_SENSOR_UTIL, "ois_handle_ois_error err_reg : 0x%X",
		factory_payload->ois_error);
	hw_bigdata_i2c_from_ois_error_reg(factory_payload->ois_error);
}

static ssize_t ois_reset_check(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0, index = -1;
	struct ois_fct_general_t test_payload;
	struct cam_ois_ctrl_t *o_ctrl = NULL;

	index = ois_get_first_running_camera();
	if (index < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "camera is not running");
		goto error;
	}
	o_ctrl = g_o_ctrls[index];

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_RESET_CHECK;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_WITH_CAMERA_EVENT,
		(void*)&test_payload, 1, buf,
		reset_check_buf, sizeof(reset_check_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return scnprintf(buf, PAGE_SIZE, "%s", reset_check_buf);

error:
	return scnprintf(buf, PAGE_SIZE, "0");
}

char hall_position_buf[1024];
void ois_handle_ois_hall_position_result(uint8_t* payload)
{
	struct ois_fct_hall_position_result_t* result_payload =
		(struct ois_fct_hall_position_result_t*)payload;
	uint32_t i = 0, offset = 0;
	uint16_t* target_position = NULL;
	uint16_t* hall_position = NULL;

	target_position = &result_payload->target_pos_x_m1;
	hall_position = &result_payload->hall_pos_x_m1;

	for (i = 0; i < CUR_MODULE_NUM; i++) {
		offset += scnprintf(hall_position_buf + offset,
			sizeof(hall_position_buf) - offset, "%u,%u,",
			target_position[(2 * i)], target_position[(2 * i) + 1]);
	}

	for (i = 0; i < CUR_MODULE_NUM; i++) {
		offset += scnprintf(hall_position_buf + offset,
			sizeof(hall_position_buf) - offset, "%u,%u%s",
			hall_position[(2 * i)], hall_position[(2 * i) + 1],
			((i + 1) != CUR_MODULE_NUM ? "," : ""));
	}
	hall_position_buf[offset] = '\0';
}

static ssize_t ois_hall_position_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0, index = -1, i = 0, offset = 0;
	struct ois_fct_general_t test_payload;
	struct cam_ois_ctrl_t *o_ctrl = NULL;
	char init_str[256] = { 0, };

	index = ois_get_first_running_camera();
	if (index < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "camera is not running");
		goto error;
	}
	o_ctrl = g_o_ctrls[index];

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_HALL_POSITION;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_WITH_CAMERA_EVENT,
		(void*)&test_payload, 2, buf,
		hall_position_buf, sizeof(hall_position_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	for (i = 0; i < CUR_MODULE_NUM; i++)
		offset += scnprintf(init_str + offset,
			sizeof(init_str) - offset, "0,0,0,0%s",
			((i + 1) != CUR_MODULE_NUM ? "," : ""));

	return scnprintf(buf, PAGE_SIZE, "%s", init_str);
}

static ssize_t ois_set_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0, index = -1;
	uint32_t mode = 0;
	struct ois_fct_ois_set_mode_t test_payload;
	struct cam_ois_ctrl_t *o_ctrl = NULL;

	if (buf == NULL || kstrtouint(buf, 10, &mode))
		return -1;

	index = ois_get_first_running_camera();
	if (index < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "camera is not running");
		return -1;
	}
	o_ctrl = g_o_ctrls[index];

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0xFF, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_SET_MODE;
	test_payload.ois_mode = mode;

	rc = ois_notify_message(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_WITH_CAMERA_EVENT,
		(void*)&test_payload);

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return size;
}

uint8_t ois_cal_mark[INDEX_MAX] = { 0, };
int ois_gain_result[INDEX_MAX] = {[0 ... INDEX_MAX - 1] = 2}; //0:normal, 1: No cal, 2: rear cal fail
uint8_t ois_xygg[INDEX_MAX][OIS_XYGG_SIZE] = { 0, };
static ssize_t ois_gain_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0, index = -1;
	uint32_t xgg = 0, ygg = 0;

	index = find_sysfs_index(attr);
	if (index < 0)
		return 0;

	CAM_INFO(CAM_SENSOR_UTIL, "[FW_DBG] %s ois gain result : %d",
		attr->attr.name, ois_gain_result[index]);
	if (ois_gain_result[index] == 0) {
		memcpy(&xgg, &ois_xygg[index][0], 4);
		memcpy(&ygg, &ois_xygg[index][4], 4);
		rc = scnprintf(buf, PAGE_SIZE, "%d,0x%x,0x%x",
			ois_gain_result[index], xgg, ygg);
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%d",
			ois_gain_result[index]);
	}
	if (rc)
		return rc;
	return 0;
}

int ois_sr_result[INDEX_MAX] = {[0 ... INDEX_MAX - 1] = 2}; //0:normal, 1: No cal, 2: rear cal fail
uint8_t ois_xysr[INDEX_MAX][OIS_XYSR_SIZE] = { 0, };
static ssize_t ois_supperssion_ratio_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0, index = -1;
	uint32_t xsr = 0, ysr = 0;

	index = find_sysfs_index(attr);
	if (index < 0)
		return 0;

	CAM_INFO(CAM_SENSOR_UTIL, "[FW_DBG] %s ois sr result : %d",
		attr->attr.name, ois_sr_result[index]);
	if (ois_sr_result[index] == 0) {
		memcpy(&xsr, &ois_xysr[index][0], 2);
		memcpy(&ysr, &ois_xysr[index][2], 2);
		rc = scnprintf(buf, PAGE_SIZE, "%d,%u.%02u,%u.%02u",
			ois_sr_result[index], (xsr / 100), (xsr % 100), (ysr / 100), (ysr % 100));
	} else {
		rc = scnprintf(buf, PAGE_SIZE, "%d",
			ois_sr_result[index]);
	}

	if (rc)
		return rc;
	return 0;
}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE) || defined(CONFIG_SAMSUNG_REAR_QUADRA)
int ois_cross_talk_result[INDEX_MAX] = {[0 ... INDEX_MAX - 1] = 2}; //0:normal, 1: No cal, 2: rear cal fail
uint8_t ois_cross_talk[INDEX_MAX][OIS_CROSSTALK_SIZE] = { 0, };
static ssize_t ois_read_cross_talk_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0, index = -1;
	uint32_t xcrosstalk = 0, ycrosstalk = 0;

	index = find_sysfs_index(attr);
	if (index < 0)
		return 0;

	CAM_INFO(CAM_SENSOR_UTIL, "[FW_DBG] %s read crosstalk result : %d",
		attr->attr.name, ois_cross_talk_result[index]);
	memcpy(&xcrosstalk, &ois_cross_talk[index][0], 2);
	memcpy(&ycrosstalk, &ois_cross_talk[index][2], 2);
	if (ois_cross_talk_result[index] == 0) { // normal
		rc = scnprintf(buf, PAGE_SIZE, "%u.%02u,%u.%02u",
			(xcrosstalk/ 100), (xcrosstalk % 100),
			(ycrosstalk / 100), (ycrosstalk % 100));
	} else if (ois_cross_talk_result[index] == 1) { // No cal
		rc = scnprintf(buf, PAGE_SIZE, "NONE");
	} else { // read cal fail
		rc = scnprintf(buf, PAGE_SIZE, "NG");
	}

	if (rc)
		return rc;
	return 0;
}
#endif

char ois_cross_talk_buf[1024];
void ois_handle_ois_cross_talk_result(uint8_t* payload)
{
	struct ois_fct_cross_talk_result_t* result_payload =
		(struct ois_fct_cross_talk_result_t*)payload;
	uint8_t test_result = result_payload->test_result;
	uint16_t *results = result_payload->read_hall_step;

	scnprintf(ois_cross_talk_buf, sizeof(ois_cross_talk_buf),
		"%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",(test_result ? 1 : 0),
		results[0], results[1], results[2], results[3],
		results[4], results[5], results[6], results[7],
		results[8], results[9]);
}

static ssize_t ois_check_cross_talk_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_general_t test_payload;

	if (ois_power == 0) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, need to power up");
		goto error;
	}

	if (ois_is_camera_running(NULL)) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, camera is running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_CROSS_TALK;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_EVENT,
		(void*)&test_payload, 2, buf,
		ois_cross_talk_buf, sizeof(ois_cross_talk_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	return scnprintf(buf, PAGE_SIZE, "0,0,0,0,0,0,0,0,0,0,0\n");
}

char ois_valid_buf[1024];
void ois_handle_ois_valid_check_result(uint8_t* payload)
{
	int i = 0, offset = 0;
	struct ois_fct_valid_check_result_t* result_payload =
		(struct ois_fct_valid_check_result_t*)payload;
	uint8_t result[MAX_MODULE_NUM] = { 1, 1, 1 };
	uint8_t *test_results = &result_payload->test_result_m1;

	for (i = 0; i < CUR_MODULE_NUM; i++) {
		result[i] = (test_results[i] & (0x02 | 0x04));
		offset += scnprintf(ois_valid_buf + offset,
			sizeof(ois_valid_buf) - offset, "%u%s",
			result[i], ((i + 1) != CUR_MODULE_NUM ? "," : "\n"));
		CAM_INFO(CAM_OIS, "result[%d] = %d, (val = 0x%x, err[x,y] = [%d, %d])",
			i, result[i], test_results[i], (test_results[i] & 0x2), (test_results[i] & 0x4));
	}
}

static ssize_t check_ois_valid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_general_t test_payload;

	if (ois_power == 0) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, need to power up");
		goto error;
	}

	if (ois_is_camera_running(NULL)) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, camera is running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_VALID_CHECK;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_EVENT,
		(void*)&test_payload, 1, buf,
		ois_valid_buf, sizeof(ois_valid_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	return scnprintf(buf, PAGE_SIZE, "0,0,0\n");
}

char hall_cal_buf[1024] = { 0, };
void ois_handle_ois_hall_cal_result(uint8_t* payload)
{
	struct ois_fct_hall_cal_chk_result_t* result_payload =
		(struct ois_fct_hall_cal_chk_result_t*)payload;
	uint8_t test_result = result_payload->test_result;
	uint16_t* results = &result_payload->ideal_pcal_x;

	scnprintf(hall_cal_buf, sizeof(hall_cal_buf),
		"%d,%u,%u,%u,%u,%u,%u,%u,%u", (test_result ? 1 : 0),
		results[0], results[1], results[2], results[3],
		results[4], results[5], results[6], results[7]);
}

static ssize_t ois_check_hall_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct cam_ois_ctrl_t *o_ctrl = g_o_ctrls[0];
	struct ois_fct_general_t test_payload;

	if (ois_power == 0) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, need to power up");
		goto error;
	}

	if (ois_is_camera_running(NULL)) {
		CAM_WARN(CAM_SENSOR_UTIL, "Not in right state, camera is running");
		goto error;
	}

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_HALL_CAL_CHK;

	rc = ois_notify_message_and_wait(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_EVENT,
		(void*)&test_payload, 1, buf,
		hall_cal_buf, sizeof(hall_cal_buf));
	if (rc < 0)
		goto error;

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return rc;

error:
	return scnprintf(buf, PAGE_SIZE, "0,0,0,0,0,0,0,0,0\n");
}

static ssize_t ois_ext_clk_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc = 0;
	uint32_t clk = 0;

	rc = scnprintf(buf, PAGE_SIZE, "%u", clk);

	if (rc)
		return rc;
	return 0;
}

static ssize_t ois_ext_clk_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	uint32_t clk = 0;

	if (buf == NULL || kstrtouint(buf, 10, &clk))
		return -1;
	CAM_INFO(CAM_SENSOR_UTIL, "new ois ext clk %u", clk);

	return size;
}

static ssize_t ois_center_shift_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0, index = -1, i = 0;
	char* token = NULL;
	char* str_shift = NULL;
	int val = 0, token_cnt = 0;
	int16_t shift[CUR_MODULE_NUM * 2] = { 0, };
	int16_t* dst = NULL;
	struct ois_fct_center_shift_t test_payload;
	struct cam_ois_ctrl_t *o_ctrl = NULL;

	if (buf == NULL)
		return -1;

	index = ois_get_first_running_camera();
	if (index < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "camera is not running");
		return -1;
	}
	o_ctrl = g_o_ctrls[index];

	CAM_INFO(CAM_SENSOR_UTIL, "E");

	str_shift = (char*)buf;
	while (((token = strsep(&str_shift, ",")) != NULL) &&
		(token_cnt < (CUR_MODULE_NUM * 2))) {
		rc = kstrtoint(token, 10, &val);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "invalid shift value %s", token);
			return -1;
		}
		shift[token_cnt++] = (int16_t)val;
	}

	for (i = 0; i < CUR_MODULE_NUM; i++)
		CAM_INFO(CAM_SENSOR_UTIL, "ois center shift M%d = (%d, %d)",
			(i+1), shift[2 * i], shift[2 * i + 1]);

	dst = (int16_t*)&test_payload.xcoffset_m1;
	memset(&test_payload, 0, sizeof(test_payload));
	test_payload.payload_type = OIS_FACTORY_CMD;
	test_payload.test_id = OIS_FCT_OIS_CENTER_SHIFT;
	memcpy(dst, shift, sizeof(shift));

	rc = ois_notify_message(o_ctrl,
		V4L_EVENT_CAM_OIS_FACTORY_WITH_CAMERA_EVENT,
		(void*)&test_payload);

	CAM_INFO(CAM_SENSOR_UTIL, "X");

	return size;
}

static DEVICE_ATTR(ois_power, S_IWUSR, NULL, ois_power_store);
static DEVICE_ATTR(ois_mgless, S_IRUGO, ois_mgless_show, NULL);
static DEVICE_ATTR(autotest, S_IRUGO|S_IWUSR|S_IWGRP, ois_autotest_show, ois_autotest_store);
static DEVICE_ATTR(calibrationtest, S_IRUGO, gyro_calibration_show, NULL);
static DEVICE_ATTR(ois_noise_stdev, S_IRUGO, gyro_noise_stdev_show, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, gyro_selftest_show, NULL);
static DEVICE_ATTR(ois_rawdata, S_IRUGO|S_IWUSR|S_IWGRP, gyro_rawdata_test_show, gyro_rawdata_test_store);
static DEVICE_ATTR(oisfw, S_IRUGO|S_IWUSR|S_IWGRP, ois_fw_full_show, ois_fw_full_store);
static DEVICE_ATTR(ois_exif, S_IRUGO, ois_exif_show, NULL);
static DEVICE_ATTR(reset_check, S_IRUGO, ois_reset_check, NULL);
static DEVICE_ATTR(ois_set_mode, S_IWUSR, NULL, ois_set_mode_store);
static DEVICE_ATTR(ois_gain_rear, S_IRUGO, ois_gain_show, NULL);
static DEVICE_ATTR(ois_supperssion_ratio_rear, S_IRUGO, ois_supperssion_ratio_show, NULL);
static DEVICE_ATTR(check_hall_cal, S_IRUGO, ois_check_hall_cal_show, NULL);
static DEVICE_ATTR(check_cross_talk, S_IRUGO, ois_check_cross_talk_show, NULL);
static DEVICE_ATTR(check_ois_valid, S_IRUGO, check_ois_valid_show, NULL);
static DEVICE_ATTR(ois_ext_clk, S_IRUGO|S_IWUSR|S_IWGRP, ois_ext_clk_show, ois_ext_clk_store);
static DEVICE_ATTR(ois_center_shift, S_IWUSR|S_IWGRP, NULL, ois_center_shift_store);
static DEVICE_ATTR(ois_hall_position, S_IRUGO, ois_hall_position_show, NULL);
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
static DEVICE_ATTR(ois_gain_rear3, S_IRUGO, ois_gain_show, NULL);
static DEVICE_ATTR(ois_supperssion_ratio_rear3, S_IRUGO, ois_supperssion_ratio_show, NULL);
static DEVICE_ATTR(rear3_read_cross_talk, S_IRUGO, ois_read_cross_talk_show, NULL);
#endif
#if defined(CONFIG_SAMSUNG_REAR_QUADRA)
static DEVICE_ATTR(ois_gain_rear4, S_IRUGO, ois_gain_show, NULL);
static DEVICE_ATTR(ois_supperssion_ratio_rear4, S_IRUGO, ois_supperssion_ratio_show, NULL);
static DEVICE_ATTR(rear4_read_cross_talk, S_IRUGO, ois_read_cross_talk_show, NULL);
#endif

const struct device_attribute *ois_attrs[] = {
	&dev_attr_ois_power,
	&dev_attr_ois_mgless,
	&dev_attr_autotest,
	&dev_attr_selftest,
	&dev_attr_ois_rawdata,
	&dev_attr_oisfw,
	&dev_attr_ois_exif,
	&dev_attr_calibrationtest,
	&dev_attr_ois_noise_stdev,
	&dev_attr_reset_check,
	&dev_attr_ois_set_mode,
	&dev_attr_ois_gain_rear,
	&dev_attr_ois_supperssion_ratio_rear,
	&dev_attr_check_cross_talk,
	&dev_attr_check_ois_valid,
	&dev_attr_ois_ext_clk,
	&dev_attr_check_hall_cal,
	&dev_attr_ois_center_shift,
	&dev_attr_ois_hall_position,
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	&dev_attr_ois_gain_rear3,
	&dev_attr_ois_supperssion_ratio_rear3,
	&dev_attr_rear3_read_cross_talk,
#endif
#if defined(CONFIG_SAMSUNG_REAR_QUADRA)
	&dev_attr_ois_gain_rear4,
	&dev_attr_ois_supperssion_ratio_rear4,
	&dev_attr_rear4_read_cross_talk,
#endif
	NULL, // DO NOT REMOVE
};

MODULE_DESCRIPTION("CAM_SYSFS_OIS_MCU");
MODULE_LICENSE("GPL v2");
