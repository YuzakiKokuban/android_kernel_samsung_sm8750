// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sensor_core.h"
#include "cam_sensor_util.h"
#include "cam_soc_util.h"
#include "cam_trace.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#include "cam_req_mgr_dev.h"
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
#include "cam_hw_bigdata.h"
#endif
#if defined(CONFIG_SEC_KUNIT)
#include "cam_sensor_util_unit_test.h"
#endif
#if defined(CONFIG_CAMERA_CDR_TEST)
#include "cam_clock_data_recovery.h"
#endif

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
#include "cam_sensor_mipi.h"

static int disable_adaptive_mipi;
module_param(disable_adaptive_mipi, int, 0644);
#endif

#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
#include "cam_sensor_thread.h"
#include <linux/slab.h>

static int frame_cnt_dbg;
module_param(frame_cnt_dbg, int, 0644);
#endif

#if defined(CONFIG_SENSOR_RETENTION)
#include "cam_sensor_retention.h"

static int disable_sensor_retention;
module_param(disable_sensor_retention, int, 0644);
#endif

#define CAM_SENSOR_PIPELINE_DELAY_MASK        0xFF
#define CAM_SENSOR_MODESWITCH_DELAY_SHIFT     8
#define CAM_SENSOR_MAX_PER_REQ_SETTINGS       4

#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
static int i2c_debug_cnt;
module_param(i2c_debug_cnt, int, 0644);
/*
adb shell "echo s5khp2 > /sys/module/camera/parameters/debug_sensor_name"
adb shell "echo 10 > /sys/module/camera/parameters/i2c_debug_cnt"
*/

#if defined(CONFIG_SAMSUNG_READ_BPC_FROM_OTP)
#include "cam_sensor_bpc.h"
#endif

extern int to_dump_when_sof_freeze__sen_id;

void cam_sensor_dbg_regdump(struct cam_sensor_ctrl_t* s_ctrl);
void cam_sensor_dbg_regdump_stream_on_fail(struct cam_sensor_ctrl_t* s_ctrl);
void cam_sensor_parse_reg(
	struct cam_sensor_ctrl_t* s_ctrl,
	struct i2c_settings_list* i2c_list,
	int32_t *debug_sen_id,
	e_sensor_reg_upd_event_type *sen_update_type);
void cam_sensor_dbg_print_vc(struct cam_sensor_ctrl_t* s_ctrl);
void cam_sensor_dbg_print_by_upd_type(struct cam_sensor_ctrl_t* , int32_t);
void cam_sensor_i2c_dump_util(
	struct cam_sensor_ctrl_t* s_ctrl,
	struct i2c_settings_list* i2c_list,
	int i2c_debug_cnt);
#endif

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
//#define HWB_FILE_OPERATION 1
uint32_t sec_sensor_position;
uint32_t sec_sensor_clk_size;
#endif

#if defined(CONFIG_CAMERA_SYSFS_V2)
extern uint32_t sId[SEC_SENSOR_ID_MAX];
#endif

#if defined(CONFIG_SENSOR_RETENTION)
int cam_sensor_write_settings(
	struct camera_io_master *io_master_info,
	struct cam_sensor_i2c_reg_setting* settings,
	uint32_t settings_size)
{
	int32_t rc = 0;
	uint32_t i = 0, size = 0;
	struct cam_sensor_i2c_reg_setting reg_setting;

	for (i = 0; i < settings_size; i++) {
		if (size < settings[i].size)
			size = settings[i].size;
	}

	reg_setting.reg_setting = kmalloc(sizeof(struct cam_sensor_i2c_reg_array) * size, GFP_KERNEL);
	if (reg_setting.reg_setting != NULL) {
		for (i = 0; i < settings_size; i++) {
			size = settings[i].size;
			memcpy(reg_setting.reg_setting,
				settings[i].reg_setting,
				sizeof(struct cam_sensor_i2c_reg_array) * size);
			reg_setting.size = size;
			reg_setting.addr_type = settings[i].addr_type;
			reg_setting.data_type = settings[i].data_type;
			reg_setting.delay = settings[i].delay;


#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
			if (i2c_debug_cnt > 0) {
				int32_t k;

				for (k = 0; k < reg_setting.size && k < i2c_debug_cnt; k++) {
					if (k == 0) {
						CAM_INFO(CAM_SENSOR,
							"[I2C_DBG] ====== size : %d ======",
							reg_setting.size);
					}
					CAM_INFO(CAM_SENSOR,
						"[I2C_DBG] [%d] addr : 0x%04X, data : 0x%04X", k,
						reg_setting.reg_setting[k].reg_addr,
						reg_setting.reg_setting[k].reg_data);
				}
			}
#endif

			rc = camera_io_dev_write(io_master_info,
				&reg_setting);
			if (rc < 0)
				CAM_ERR(CAM_SENSOR,
					"Failed to random write I2C settings[%d]: %d", i, rc);
		}

		if (reg_setting.reg_setting) {
			kfree(reg_setting.reg_setting);
			reg_setting.reg_setting = NULL;
		}
	}
	else {
		CAM_ERR(CAM_SENSOR,"out of memory");
	}

	return rc;
}
#endif

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
int32_t cam_check_stream_on(
	struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t ret = 0;
	uint16_t sensor_id = 0;

	if (disable_adaptive_mipi) {
		CAM_INFO(CAM_SENSOR, "Disabled Adaptive MIPI");
		return ret;
	}

	sensor_id = s_ctrl->sensordata->slave_info.sensor_id;
	switch (sensor_id) {
		case SENSOR_ID_S5KGN3:
		case SENSOR_ID_S5KHP2:
		case SENSOR_ID_IMX754:
		case SENSOR_ID_IMX854:
		case SENSOR_ID_IMX564:
		case SENSOR_ID_S5K3K1:
		case SENSOR_ID_S5KJN3:
		case SENSOR_ID_S5K3LU:
		case SENSOR_ID_IMX874:
			ret = 1;
			break;
		default:
			ret =0;
			break;
	}

	return ret;
}

int cam_sensor_apply_adaptive_mipi_settings(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	const struct cam_mipi_sensor_mode *cur_mipi_sensor_mode;
	struct i2c_settings_list mipi_i2c_list;
	uint16_t sensor_id = 0;

	sensor_id = s_ctrl->sensordata->slave_info.sensor_id;

	if (cam_check_stream_on(s_ctrl))
	{
		cam_mipi_init_setting(s_ctrl);
		cam_mipi_update_info(s_ctrl);
		cam_mipi_get_clock_string(s_ctrl);
 	}

	if (cam_check_stream_on(s_ctrl)
		&& s_ctrl->mipi_clock_index_new != INVALID_MIPI_INDEX
		&& s_ctrl->i2c_data.streamon_settings.is_settings_valid) {
		CAM_INFO(CAM_SENSOR, "[AM_DBG] Write MIPI setting before Stream On setting. mipi_index : %d",
			s_ctrl->mipi_clock_index_new);

 		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
		memset(&mipi_i2c_list, 0, sizeof(mipi_i2c_list));

		memcpy(&mipi_i2c_list.i2c_settings,
			cur_mipi_sensor_mode->mipi_setting[s_ctrl->mipi_clock_index_new].clk_setting,
			sizeof(struct cam_sensor_i2c_reg_setting));

		CAM_INFO(CAM_SENSOR, "[AM_DBG] Picked MIPI clock : %s",
			cur_mipi_sensor_mode->mipi_setting[s_ctrl->mipi_clock_index_new].str_mipi_clk);

		if (mipi_i2c_list.i2c_settings.size > 0)
			rc = camera_io_dev_write(&s_ctrl->io_master_info,
				&(mipi_i2c_list.i2c_settings));
	}

	return rc;
}
#endif

#if defined(CONFIG_CAMERA_FRAME_CNT_CHECK)
int cam_sensor_wait_stream_onoff(
	struct cam_sensor_ctrl_t *s_ctrl,
	bool onoff)
{
	int rc = 0, i = 0;
	uint32_t reg_data = 0;
	uint32_t retries = 100;
	uint32_t FRAME_COUNT_REG_ADDR = 0x0005;

	CAM_DBG(CAM_SENSOR, "E");

	for (i = 0; i < retries; i++) {
		rc = camera_io_dev_read(&s_ctrl->io_master_info,
			FRAME_COUNT_REG_ADDR, &reg_data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE, false);
		if (rc < 0)
			break;

		if (((onoff == true) && (reg_data != 0xff)) ||
			((onoff == false) && (reg_data == 0xff))) {
				rc = 0;
				break;
		}
		usleep_range(1000, 1010);
	}

	if (rc < 0)
	   CAM_ERR(CAM_SENSOR, "[CNT_DBG] Fail to wait stream %s, rc %d",
		   (onoff ? "on" : "off"), rc);

	CAM_DBG(CAM_SENSOR, "X");

	return rc;
}
#endif

#if defined(CONFIG_SAMSUNG_CAMERA)
int cam_sensor_pre_apply_settings(
	struct cam_sensor_ctrl_t *s_ctrl,
	enum cam_sensor_packet_opcodes opcode)
{
	int rc = 0;
	switch (opcode) {
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF: {
#if defined(CONFIG_CAMERA_FRAME_CNT_CHECK)
			cam_sensor_wait_stream_onoff(s_ctrl, true);
#endif
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON: {
#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
			rc = cam_sensor_apply_adaptive_mipi_settings(s_ctrl);
#endif
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_CONFIG:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE:
		default:
			return 0;
	}
	return rc;
}

int cam_sensor_post_apply_settings(
	struct cam_sensor_ctrl_t *s_ctrl,
	enum cam_sensor_packet_opcodes opcode)
{
	int rc = 0;

	switch (opcode) {
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF: {
#if defined(CONFIG_CAMERA_FRAME_CNT_CHECK)
			cam_sensor_wait_stream_onoff(s_ctrl, false);
#endif
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON: {
#if defined(CONFIG_SENSOR_RETENTION)
			s_ctrl->retention_stream_on = true;
#endif
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_CONFIG: {
#if defined(CONFIG_SENSOR_RETENTION)
			s_ctrl->retention_stream_on = false;
#endif
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE:
		default:
			return 0;
	}
	return rc;
}
#endif

#if defined(CONFIG_SAMSUNG_READ_BPC_FROM_OTP)
static int32_t cam_sensor_is_need_to_read_otp(struct cam_sensor_ctrl_t *s_ctrl, uint32_t * sensor_revision)
{
	char read_otp_reset[5] = "BEEF";
	uint32_t read_value = 0xBEEF;
	int32_t rc = 0;

	rc = camera_io_dev_read(&s_ctrl->io_master_info, SENSOR_REVISION_ADDR, &read_value,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD, false);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to read SENSOR_REVISION_ADDR");

	CAM_INFO(CAM_SENSOR, "[BPC] Revision read_value = 0x%x", read_value);
	*sensor_revision = read_value;

	if (read_value < S5KHP2_SENSOR_REVISION_EVT1) {
		CAM_INFO(CAM_SENSOR, "[BPC] Sensor revision is not EVT1 = 0x%x", read_value);
		return 0;
	}

	if (memcmp(otp_data, read_otp_reset, sizeof(read_otp_reset)) != 0) {
		CAM_INFO(CAM_SENSOR, "[BPC] Sensor is Same");
		return 0;
	}
	return 1;
}

int cam_sensor_wait_otp_mode(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = -1;
	uint32_t wait_ms = 50;

	CAM_INFO(CAM_SENSOR, "[BPC] cam_sensor_wait_otp_mode");

	rc = camera_io_dev_poll(&s_ctrl->io_master_info,
			BPC_OTP_READ_STATUS_ADDR, BPC_OTP_READ_STATUS_OK, 0,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,
			wait_ms);

	/* If rc is 1 then read is successful but poll is failure */
	if ((rc < 0) || (rc == 1)) {
		CAM_ERR(CAM_SENSOR, "[BPC] Fail cam-sensor_wait_otp_mode %d", rc);
		rc = (rc == 1) ? -1 : rc;
	}

	return rc;
}

static int cam_bpc_match_crc(void)
{
	uint32_t calculated_crc = 0x0;
	uint32_t check_sum_crc = 0x0;
	uint32_t cur_otp = 0x0;
	uint32_t i = 0x0;
	CAM_INFO(CAM_SENSOR, "[BPC] cam_bpc_match_crc E");

	for (i = 0 ; i < (BPC_OTP_SIZE_MAX - 4) ; i += 4)
	{
		cur_otp = (otp_data[i] << 24) | (otp_data[i + 1] << 16) | (otp_data[i + 2] << 8) | (otp_data[i + 3]);
		if (i == 0)
		{
			calculated_crc = cur_otp;
		}
		else
		{
			calculated_crc = ~(calculated_crc ^ cur_otp);
		}
		if (cur_otp == BPC_OTP_TERMINATE_CODE_FOR_CRC)
		{
			CAM_INFO(CAM_SENSOR, "[BPC] Meet Terminate code for CRC index = %u cur_otp = 0x%x",i, cur_otp);
			break;
		}
	}

	check_sum_crc = (otp_data[BPC_OTP_SIZE_MAX - 4] << 24) |
					(otp_data[BPC_OTP_SIZE_MAX - 3] << 16) |
					(otp_data[BPC_OTP_SIZE_MAX - 2] << 8) |
					(otp_data[BPC_OTP_SIZE_MAX - 1]);

	CAM_INFO(CAM_SENSOR, "[BPC] previous_crc = 0x%x", calculated_crc);
	CAM_INFO(CAM_SENSOR, "[BPC] check_sum_crc = 0x%x", check_sum_crc);


	if (calculated_crc != check_sum_crc)
	{
		CAM_ERR(CAM_SENSOR, "[BPC] Crc mismatched = calculated crc 0x%x check_sum_crc 0x%x", calculated_crc, check_sum_crc);
		return -EINVAL;
	}

	CAM_INFO(CAM_SENSOR, "[BPC] cam_bpc_match_crc X");
	return 0;
}

int cam_sensor_read_bpc_from_otp(struct cam_sensor_ctrl_t *s_ctrl, uint32_t sensor_revision)
{
	int32_t rc = 0;
	uint32_t addr = 0, size = 0, read_size = 0;
	uint8_t *memptr;
	uint32_t otp_index = 0;
	char read_opt_reset[5] = "BEEF";

	memptr = otp_data;
	CAM_INFO(CAM_SENSOR, "[BPC] E sensor_revision = 0x%x", sensor_revision);

	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
			read_otp_setttings, ARRAY_SIZE(read_otp_setttings));

	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "[BPC] Failed update register to read otp rc = %d", rc);
		return rc;
	}

	if (cam_sensor_wait_otp_mode(s_ctrl) == 0){
		rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
			bpc_end_sequence_setttings, ARRAY_SIZE(bpc_end_sequence_setttings));
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,	"[BPC] Failed to write bpc_configure_dram rc = %d", rc);
			return rc;
		}

		size = BPC_OTP_SIZE_MAX;
		addr = 0;

		while(size > 0) {
			read_size = size;
			if (size > PAGE_SIZE) {
				read_size = PAGE_SIZE;
			}

			rc = camera_io_dev_read_seq(&s_ctrl->io_master_info,
				addr, memptr,
				CAMERA_SENSOR_I2C_TYPE_WORD,
				CAMERA_SENSOR_I2C_TYPE_BYTE,
				read_size);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM, "read failed rc %d",
					rc);
				return rc;
			}
			size -= read_size;
			addr += read_size;
			otp_index += PAGE_SIZE;
			memptr = otp_data + otp_index;
		}
	}
	else{
		CAM_ERR(CAM_SENSOR,	"[BPC] Failed to cam_sensor_wait_otp_mode rc = %d", rc);
	}

	if (sensor_revision >= S5KHP2_SENSOR_SUPPORT_BPC_CRC_SENSOR_REVISION)
	{
		if (cam_bpc_match_crc() < 0)
		{
			CAM_ERR(CAM_SENSOR, "[BPC] CRC error");
			// Need to enable after getting module that is writted CRC checksum
			memcpy(otp_data, read_opt_reset, sizeof(read_opt_reset));
		}
	}

	CAM_INFO(CAM_SENSOR, "[BPC] X");

	return rc;
}
#endif

extern struct completion *cam_sensor_get_i3c_completion(uint32_t index);

static int cam_sensor_notify_v4l2_error_event(
	struct cam_sensor_ctrl_t *s_ctrl,
	uint32_t error_type, uint32_t error_code)
{
	int                        rc = 0;
	struct cam_req_mgr_message req_msg = {0};

	req_msg.session_hdl = s_ctrl->bridge_intf.session_hdl;
	req_msg.u.err_msg.device_hdl = s_ctrl->bridge_intf.device_hdl;
	req_msg.u.err_msg.link_hdl = s_ctrl->bridge_intf.link_hdl;
	req_msg.u.err_msg.error_type = error_type;
	req_msg.u.err_msg.request_id = s_ctrl->last_applied_req;
	req_msg.u.err_msg.resource_size = 0x0;
	req_msg.u.err_msg.error_code = error_code;

	CAM_DBG(CAM_SENSOR,
		"v4l2 error event [type: %u code: %u] for req: %llu on %s",
		error_type, error_code, s_ctrl->last_applied_req,
		s_ctrl->sensor_name);

	rc = cam_req_mgr_notify_message(&req_msg,
		V4L_EVENT_CAM_REQ_MGR_ERROR,
		V4L_EVENT_CAM_REQ_MGR_EVENT);
	if (rc)
		CAM_ERR(CAM_SENSOR,
			"Notifying v4l2 error [type: %u code: %u] failed for req id:%llu on %s",
			error_type, error_code, s_ctrl->last_applied_req,
			s_ctrl->sensor_name);

	return rc;
}

static int cam_sensor_notify_msg_req_mgr(
	enum cam_req_mgr_msg_type msg_type,
	struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	struct cam_req_mgr_notify_msg msg = {0};

	msg.link_hdl = s_ctrl->bridge_intf.link_hdl;
	msg.dev_hdl = s_ctrl->bridge_intf.device_hdl;
	msg.msg_type = msg_type;

	if (s_ctrl->bridge_intf.crm_cb &&
		s_ctrl->bridge_intf.crm_cb->add_req) {
		rc = s_ctrl->bridge_intf.crm_cb->notify_msg(&msg);
		if (rc) {
			CAM_ERR(CAM_SENSOR,
				"Notifying req mgr message: %u failed rc: %d", msg_type, rc);
		}
	}

	CAM_DBG(CAM_SENSOR,
		"Successfully notified message : %u to req mgr", msg_type);

	return rc;
}

static int cam_sensor_update_req_mgr(
	struct cam_sensor_ctrl_t *s_ctrl,
	struct cam_packet *csl_packet)
{
	int rc = 0;
	struct cam_req_mgr_add_request add_req;

	memset(&add_req, 0, sizeof(add_req));
	add_req.link_hdl = s_ctrl->bridge_intf.link_hdl;
	add_req.req_id = csl_packet->header.request_id;
	CAM_DBG(CAM_SENSOR, " Rxed Req Id: %llu",
		csl_packet->header.request_id);
	add_req.dev_hdl = s_ctrl->bridge_intf.device_hdl;
	if (s_ctrl->bridge_intf.crm_cb &&
		s_ctrl->bridge_intf.crm_cb->add_req) {
		rc = s_ctrl->bridge_intf.crm_cb->add_req(&add_req);
		if (rc) {
			if (rc == -EBADR)
				CAM_INFO(CAM_SENSOR,
					"Adding request: %llu failed with request manager rc: %d, it has been flushed",
					csl_packet->header.request_id, rc);
			else
				CAM_ERR(CAM_SENSOR,
					"Adding request: %llu failed with request manager rc: %d",
					csl_packet->header.request_id, rc);
			return rc;
		}
	}

	CAM_DBG(CAM_SENSOR, "Successfully add req: %llu to req mgr",
			add_req.req_id);
	return rc;
}

static void cam_sensor_release_stream_rsc(
	struct cam_sensor_ctrl_t *s_ctrl)
{
	struct i2c_settings_array *i2c_set = NULL;
	int rc;

	i2c_set = &(s_ctrl->i2c_data.streamoff_settings);
	if (i2c_set->is_settings_valid == 1) {
		i2c_set->is_settings_valid = -1;
		rc = delete_request(i2c_set);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"failed while deleting Streamoff settings");
	}

	i2c_set = &(s_ctrl->i2c_data.streamon_settings);
	if (i2c_set->is_settings_valid == 1) {
		i2c_set->is_settings_valid = -1;
		rc = delete_request(i2c_set);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"failed while deleting Streamon settings");
	}
}

static void cam_sensor_release_per_frame_resource(
	struct cam_sensor_ctrl_t *s_ctrl)
{
	struct i2c_settings_array *i2c_set = NULL;
	int i, rc;

	if (s_ctrl->i2c_data.deferred_frame_update != NULL) {
		for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
			i2c_set = &(s_ctrl->i2c_data.deferred_frame_update[i]);
			if (i2c_set->is_settings_valid == 1) {
				i2c_set->is_settings_valid = -1;
				rc = delete_request(i2c_set);
				if (rc < 0)
					CAM_ERR(CAM_SENSOR,
						"delete deferred frame_update setting for request: %lld rc: %d",
						i2c_set->request_id, rc);
			}
		}
	}

	if (s_ctrl->i2c_data.per_frame != NULL) {
		for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
			i2c_set = &(s_ctrl->i2c_data.per_frame[i]);
			if (i2c_set->is_settings_valid == 1) {
				i2c_set->is_settings_valid = -1;
				rc = delete_request(i2c_set);
				if (rc < 0)
					CAM_ERR(CAM_SENSOR,
						"delete per frame setting for request: %lld rc: %d",
						i2c_set->request_id, rc);
			}
		}
	}

	if (s_ctrl->i2c_data.frame_skip != NULL) {
		for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
			i2c_set = &(s_ctrl->i2c_data.frame_skip[i]);
			if (i2c_set->is_settings_valid == 1) {
				i2c_set->is_settings_valid = -1;
				rc = delete_request(i2c_set);
				if (rc < 0)
					CAM_ERR(CAM_SENSOR,
						"delete frame skip setting for request: %lld rc: %d",
						i2c_set->request_id, rc);
			}
		}
	}

	if (s_ctrl->i2c_data.bubble_update != NULL) {
		for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
			i2c_set = &(s_ctrl->i2c_data.bubble_update[i]);
			if (i2c_set->is_settings_valid == 1) {
				i2c_set->is_settings_valid = -1;
				rc = delete_request(i2c_set);
				if (rc < 0)
					CAM_ERR(CAM_SENSOR,
						"delete bubble update setting for request: %lld rc: %d",
						i2c_set->request_id, rc);
			}
		}
	}
}

static int cam_sensor_handle_res_info(struct cam_sensor_res_info *res_info,
	struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint32_t idx = 0;

	if (!s_ctrl || !res_info) {
		CAM_ERR(CAM_SENSOR, "Invalid params: res_info: %s, s_ctrl: %s",
			CAM_IS_NULL_TO_STR(res_info),
			CAM_IS_NULL_TO_STR(s_ctrl));
		return -EINVAL;
	}

	idx = s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY;

	s_ctrl->sensor_res[idx].res_index = res_info->res_index;
	strscpy(s_ctrl->sensor_res[idx].caps, res_info->caps,
		sizeof(s_ctrl->sensor_res[idx].caps));
	s_ctrl->sensor_res[idx].width = res_info->width;
	s_ctrl->sensor_res[idx].height = res_info->height;
	s_ctrl->sensor_res[idx].fps = res_info->fps;
	s_ctrl->sensor_res[idx].request_id = s_ctrl->last_updated_req;

	if (res_info->num_valid_params > 0) {
		if (res_info->valid_param_mask & CAM_SENSOR_FEATURE_MASK)
			s_ctrl->sensor_res[idx].feature_mask =
				res_info->params[0];

		if (res_info->valid_param_mask & CAM_SENSOR_NUM_BATCHED_FRAMES)
			s_ctrl->num_batched_frames = res_info->params[1];
	}

	s_ctrl->is_res_info_updated = true;
#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
	s_ctrl->sensor_width = s_ctrl->sensor_res[idx].width;
#endif

	/* If request id is 0, it will be during an initial config/acquire */
	CAM_INFO(CAM_SENSOR,
		"Sensor[%s-%d] Feature: 0x%x updated for request id: %lu, res index: %u, width: %d, height: %d, capability: %s, fps: %u",
		s_ctrl->sensor_name, s_ctrl->soc_info.index,
		s_ctrl->sensor_res[idx].feature_mask,
		s_ctrl->sensor_res[idx].request_id, s_ctrl->sensor_res[idx].res_index,
		s_ctrl->sensor_res[idx].width, s_ctrl->sensor_res[idx].height,
		s_ctrl->sensor_res[idx].caps, s_ctrl->sensor_res[idx].fps);

	return rc;
}

static int cam_sensor_handle_frame_info(struct cam_sensor_ctrl_t *s_ctrl,
	struct cam_sensor_frame_info *frame_info)
{
	int rc = 0;
	struct cam_req_mgr_notify_msg msg = {0};

	CAM_DBG(CAM_SENSOR,
		"sensor:%d req:%llu frame info: frame sync shift:%llu frame duration:%llu blanking duration:%llu",
		s_ctrl->soc_info.index, s_ctrl->last_updated_req,
		frame_info->frame_sync_shift, frame_info->frame_duration,
		frame_info->blanking_duration);

	if (!s_ctrl->bridge_intf.crm_cb ||
		!s_ctrl->bridge_intf.crm_cb->notify_msg) {
		CAM_ERR(CAM_SENSOR, "Invalid crm_cb:%p or notify_msg:%p",
			s_ctrl->bridge_intf.crm_cb,
			s_ctrl->bridge_intf.crm_cb ?
			s_ctrl->bridge_intf.crm_cb->notify_msg :
			NULL);
		return -EINVAL;
	}

	msg.link_hdl = s_ctrl->bridge_intf.link_hdl;
	msg.req_id = s_ctrl->last_updated_req;
	msg.dev_hdl = s_ctrl->bridge_intf.device_hdl;
	msg.msg_type = CAM_REQ_MGR_MSG_SENSOR_FRAME_INFO;
	msg.u.frame_info.frame_sync_shift = frame_info->frame_sync_shift;
	msg.u.frame_info.frame_duration = frame_info->frame_duration;
	msg.u.frame_info.blanking_duration = frame_info->blanking_duration;
	if ((s_ctrl->stream_off_on_flush) && (s_ctrl->sensor_state == CAM_SENSOR_STANDBY))
		msg.u.frame_info.use_for_wd = true;

	rc = s_ctrl->bridge_intf.crm_cb->notify_msg(&msg);
	if (rc) {
		CAM_ERR(CAM_SENSOR,
			"Failed to notify msg SENSOR_FRAME_INFO to CRM at req:%llu, rc:%d",
			s_ctrl->last_updated_req, rc);
		return rc;
	}

	return rc;
}

static int32_t cam_sensor_generic_blob_handler(void *user_data,
	uint32_t blob_type, uint32_t blob_size, uint8_t *blob_data)
{
	int rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl =
		(struct cam_sensor_ctrl_t *) user_data;

	if (!blob_data || !blob_size) {
		CAM_ERR(CAM_SENSOR, "Invalid blob info %pK %u", blob_data,
			blob_size);
		return -EINVAL;
	}

	switch (blob_type) {
	case CAM_SENSOR_GENERIC_BLOB_RES_INFO: {
		struct cam_sensor_res_info *res_info =
			(struct cam_sensor_res_info *) blob_data;

		if (blob_size < sizeof(struct cam_sensor_res_info)) {
			CAM_ERR(CAM_SENSOR, "RES_INFO: Invalid blob size expected: 0x%x actual: 0x%x",
				sizeof(struct cam_sensor_res_info), blob_size);
			return -EINVAL;
		}

		rc = cam_sensor_handle_res_info(res_info, s_ctrl);
		break;
	}
	case CAM_SENSOR_GENERIC_BLOB_FRAME_INFO: {
		struct cam_sensor_frame_info *frame_info =
			(struct cam_sensor_frame_info *) blob_data;

		if (blob_size < sizeof(struct cam_sensor_frame_info)) {
			CAM_ERR(CAM_SENSOR, "FRAME_INFO: Invalid blob size expected: 0x%x actual: 0x%x",
				sizeof(struct cam_sensor_frame_info), blob_size);
			return -EINVAL;
		}

		rc = cam_sensor_handle_frame_info(s_ctrl, frame_info);
		break;
	}
	default:
		CAM_WARN(CAM_SENSOR, "Invalid blob type %d", blob_type);
		break;
	}

	return rc;
}

static int32_t cam_sensor_pkt_parse(struct cam_sensor_ctrl_t *s_ctrl,
	void *arg)
{
	int32_t i, rc = 0;
	uintptr_t generic_ptr;
	struct cam_control *ioctl_ctrl = NULL;
	struct cam_packet *csl_packet = NULL;
	struct cam_packet *csl_packet_u = NULL;
	struct cam_cmd_buf_desc *cmd_desc = NULL;
	struct cam_buf_io_cfg *io_cfg = NULL;
	struct i2c_settings_array *i2c_reg_settings = NULL;
	size_t len_of_buff = 0;
	size_t remain_len = 0;
	int64_t prev_updated_req;
	uint32_t cmd_buf_type, idx;
	struct cam_config_dev_cmd config;
	struct i2c_data_settings *i2c_data = NULL;
	size_t packet_size = 0;

	ioctl_ctrl = (struct cam_control *)arg;

	if (ioctl_ctrl->handle_type != CAM_HANDLE_USER_POINTER) {
		CAM_ERR(CAM_SENSOR, "Invalid Handle Type");
		return -EINVAL;
	}

	if (copy_from_user(&config,
		u64_to_user_ptr(ioctl_ctrl->handle),
		sizeof(config)))
		return -EFAULT;

	rc = cam_mem_get_cpu_buf(
		config.packet_handle,
		&generic_ptr,
		&len_of_buff);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed in getting the packet: %d", rc);
		return rc;
	}

	remain_len = len_of_buff;
	if ((sizeof(struct cam_packet) > len_of_buff) ||
		((size_t)config.offset >= len_of_buff -
		sizeof(struct cam_packet))) {
		CAM_ERR(CAM_SENSOR,
			"Inval cam_packet strut size: %zu, len_of_buff: %zu",
			 sizeof(struct cam_packet), len_of_buff);
		rc = -EINVAL;
		goto put_ref;
	}

	remain_len -= (size_t)config.offset;
	csl_packet_u = (struct cam_packet *)(generic_ptr +
		(uint32_t)config.offset);
	packet_size = csl_packet_u->header.size;
	if (packet_size <= remain_len) {
		rc = cam_common_mem_kdup((void **)&csl_packet,
			csl_packet_u, packet_size);
		if (rc) {
			CAM_ERR(CAM_SENSOR, "Alloc and copy request: %lld packet fail",
				csl_packet_u->header.request_id);
			goto put_ref;
		}
	} else {
		CAM_ERR(CAM_SENSOR, "Invalid packet header size %u",
			packet_size);
		goto put_ref;
	}

	if (cam_packet_util_validate_packet(csl_packet,
		remain_len)) {
		CAM_ERR(CAM_SENSOR, "Invalid packet params");
		rc = -EINVAL;
		goto end;
	}

	if ((csl_packet->header.op_code & 0xFFFFFF) !=
		CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG &&
		(csl_packet->header.op_code & 0xFFFFFF) !=
		CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF &&
		csl_packet->header.request_id <= s_ctrl->last_flush_req
		&& s_ctrl->last_flush_req != 0) {
		CAM_ERR(CAM_SENSOR,
			"reject request %lld, last request to flush %u",
			csl_packet->header.request_id, s_ctrl->last_flush_req);
		rc = -EBADR;
		goto end;
	}

	if (csl_packet->header.request_id > s_ctrl->last_flush_req)
		s_ctrl->last_flush_req = 0;

	prev_updated_req = s_ctrl->last_updated_req;
	s_ctrl->is_res_info_updated = false;

	i2c_data = &(s_ctrl->i2c_data);
	CAM_DBG(CAM_SENSOR, "Header OpCode: %d", csl_packet->header.op_code);
	switch (csl_packet->header.op_code & 0xFFFFFF) {
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG: {
		i2c_reg_settings = &i2c_data->init_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_CONFIG: {
		i2c_reg_settings = &i2c_data->config_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON: {
		if (s_ctrl->streamon_count > 0) {
			delete_request(&i2c_data->streamon_settings);
			s_ctrl->streamon_count = 0;
		}

		s_ctrl->streamon_count = s_ctrl->streamon_count + 1;
		i2c_reg_settings = &i2c_data->streamon_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF: {
		if (s_ctrl->streamoff_count > 0) {
			delete_request(&i2c_data->streamoff_settings);
			s_ctrl->streamoff_count = 0;
			s_ctrl->is_stream_off_pkt_updated = true;
		}

		s_ctrl->streamoff_count = s_ctrl->streamoff_count + 1;
		i2c_reg_settings = &i2c_data->streamoff_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_READ: {
		i2c_reg_settings = &(i2c_data->read_settings);
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;

		CAM_DBG(CAM_SENSOR, "number of IO configs: %d:",
			csl_packet->num_io_configs);
		if (csl_packet->num_io_configs == 0) {
			CAM_ERR(CAM_SENSOR, "No I/O configs to process");
			goto end;
		}

		io_cfg = (struct cam_buf_io_cfg *) ((uint8_t *)
			&csl_packet->payload_flex +
			csl_packet->io_configs_offset);

		if (io_cfg == NULL) {
			CAM_ERR(CAM_SENSOR, "I/O config is invalid(NULL)");
			goto end;
		}
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE: {
		if (s_ctrl->sensor_state < CAM_SENSOR_STANDBY) {
			CAM_WARN(CAM_SENSOR,
				"Rxed Update packets without linking in state: %d",
				s_ctrl->sensor_state);
			goto end;
		}

		i2c_reg_settings =
			&i2c_data->per_frame[csl_packet->header.request_id %
				MAX_PER_FRAME_ARRAY];
		CAM_DBG(CAM_SENSOR, "Received Packet: %lld req: %lld",
			csl_packet->header.request_id % MAX_PER_FRAME_ARRAY,
			csl_packet->header.request_id);
		if (i2c_reg_settings->is_settings_valid == 1) {
			CAM_ERR(CAM_SENSOR,
				"Already some pkt in offset req : %lld",
				csl_packet->header.request_id);
			/*
			 * Update req mgr even in case of failure.
			 * This will help not to wait indefinitely
			 * and freeze. If this log is triggered then
			 * fix it.
			 */
			rc = cam_sensor_update_req_mgr(s_ctrl, csl_packet);
			if (rc)
				CAM_ERR(CAM_SENSOR,
					"Failed in adding request to req_mgr");
			goto end;
		}
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_FRAME_SKIP_UPDATE: {
		if (s_ctrl->sensor_state < CAM_SENSOR_STANDBY) {
			CAM_WARN(CAM_SENSOR,
				"Rxed Update packets without linking in state: %d",
				s_ctrl->sensor_state);
			goto end;
		}

		i2c_reg_settings =
			&i2c_data->frame_skip[csl_packet->header.request_id %
				MAX_PER_FRAME_ARRAY];
		CAM_DBG(CAM_SENSOR, "Received not ready packet: %lld req: %lld",
			csl_packet->header.request_id % MAX_PER_FRAME_ARRAY,
			csl_packet->header.request_id);
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_BUBBLE_UPDATE: {
		if (s_ctrl->sensor_state < CAM_SENSOR_STANDBY) {
			CAM_WARN(CAM_SENSOR,
				"Rxed Update packets without linking in state: %d",
				s_ctrl->sensor_state);
			goto end;
		}

		i2c_reg_settings =
			&i2c_data->bubble_update[csl_packet->header.request_id %
				MAX_PER_FRAME_ARRAY];
		CAM_DBG(CAM_SENSOR, "Received bubble update packet: %lld req: %lld",
			csl_packet->header.request_id % MAX_PER_FRAME_ARRAY,
			csl_packet->header.request_id);
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_NOP: {
		if (s_ctrl->sensor_state < CAM_SENSOR_STANDBY) {
			CAM_WARN(CAM_SENSOR,
				"Rxed Update packets without linking in state: %d",
				s_ctrl->sensor_state);
			goto end;
		}

		i2c_reg_settings =
			&i2c_data->per_frame[csl_packet->header.request_id %
				MAX_PER_FRAME_ARRAY];
		i2c_reg_settings->request_id = csl_packet->header.request_id;
		i2c_reg_settings->is_settings_valid = 1;

		rc = cam_sensor_update_req_mgr(s_ctrl, csl_packet);
		if (rc)
			CAM_ERR(CAM_SENSOR,
				"Failed in adding request to req_mgr");
		break;
	}
	default:
		CAM_ERR(CAM_SENSOR, "Invalid Packet Header opcode: %d",
			csl_packet->header.op_code & 0xFFFFFF);
		rc = -EINVAL;
		goto end;
	}

	cmd_desc = (struct cam_cmd_buf_desc *)
		((uint8_t *)&csl_packet->payload +
		csl_packet->cmd_buf_offset);

	CAM_DBG(CAM_SENSOR, "cam:%d req:%llu number of command buffers:%d",
		s_ctrl->soc_info.index, csl_packet->header.request_id,
		csl_packet->num_cmd_buf);

	for (i = 0; i < csl_packet->num_cmd_buf; i++) {
		if (!cmd_desc[i].length)
			continue;

		rc = cam_packet_util_validate_cmd_desc(&cmd_desc[i]);
		if (rc) {
			CAM_ERR(CAM_SENSOR, "invalud cmd[%d] buf", i);
			rc = -EINVAL;
			goto end;
		}

		cmd_buf_type = cmd_desc[i].meta_data;
		CAM_DBG(CAM_SENSOR, "cam:%d cmd_buf_type:%d",
			s_ctrl->soc_info.index, cmd_buf_type);

		switch (cmd_buf_type) {
		case CAM_SENSOR_PACKET_I2C_COMMANDS: {
			rc = cam_sensor_i2c_command_parser(&s_ctrl->io_master_info,
				i2c_reg_settings, &cmd_desc[i], 1, io_cfg);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Fail parsing I2C Pkt: %d", rc);
				goto end;
			}
			break;
		}
		case CAM_SENSOR_PACKET_DEFERRED_I2C_COMMANDS_META: {
			struct i2c_settings_array *i2c_reg_settings_deferred = NULL;

			i2c_reg_settings_deferred =
				&i2c_data->deferred_frame_update[csl_packet->header.request_id %
					MAX_PER_FRAME_ARRAY];

			if (i2c_reg_settings_deferred->is_settings_valid)
				CAM_WARN(CAM_SENSOR,
					"Sensor[%s] receiving duplicate deferred meta settings for req: %llu",
					s_ctrl->sensor_name, csl_packet->header.request_id);

			i2c_reg_settings_deferred->request_id = csl_packet->header.request_id;
			rc = cam_sensor_i2c_command_parser(&s_ctrl->io_master_info,
				i2c_reg_settings_deferred, &cmd_desc[i], 1, io_cfg);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Fail parsing I2C Pkt: %d", rc);
				goto end;
			}
			break;
		}
		case CAM_SENSOR_PACKET_GENERIC_BLOB: {
			s_ctrl->last_updated_req = csl_packet->header.request_id;
			idx = s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY;

			rc = cam_packet_util_process_generic_cmd_buffer(&cmd_desc[i],
				cam_sensor_generic_blob_handler, s_ctrl);
			if (rc)
				s_ctrl->sensor_res[idx].request_id = 0;

			break;
		}
		case CAM_SENSOR_PACKET_BUBBLE_UPD_I2C_COMMANDS_META: {
			struct i2c_settings_array *i2c_reg_settings_bubble = NULL;

			i2c_reg_settings_bubble =
				&i2c_data->bubble_update[csl_packet->header.request_id %
				MAX_PER_FRAME_ARRAY];
			if (i2c_reg_settings_bubble->is_settings_valid)
				CAM_WARN(CAM_SENSOR,
					"Sensor[%s] receiving duplicate bubble meta settings for req: %llu",
					s_ctrl->sensor_name, csl_packet->header.request_id);
			i2c_reg_settings_bubble->request_id = csl_packet->header.request_id;
			rc = cam_sensor_i2c_command_parser(&s_ctrl->io_master_info,
				i2c_reg_settings_bubble, &cmd_desc[i], 1, io_cfg);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Fail parsing I2C Pkt: %d", rc);
				goto end;
			}
			break;
		}
		case CAM_SENSOR_PACKET_FRAME_SKIP_I2C_COMMANDS_META: {
			struct i2c_settings_array *i2c_reg_settings_frame_skip = NULL;

			i2c_reg_settings_frame_skip =
				&i2c_data->frame_skip[csl_packet->header.request_id %
				MAX_PER_FRAME_ARRAY];
			if (i2c_reg_settings_frame_skip->is_settings_valid)
				CAM_WARN(CAM_SENSOR,
					"Sensor[%s] receiving duplicate frame skip meta settings for req: %llu",
					s_ctrl->sensor_name, csl_packet->header.request_id);
			i2c_reg_settings_frame_skip->request_id = csl_packet->header.request_id;

			rc = cam_sensor_i2c_command_parser(&s_ctrl->io_master_info,
				i2c_reg_settings_frame_skip, &cmd_desc[i], 1, io_cfg);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Fail parsing I2C Pkt: %d", rc);
				goto end;
			}
			break;
		}
		default:
			CAM_ERR(CAM_SENSOR, "invalid cmd buf type %d",
				cmd_buf_type);
			rc = -EINVAL;
			goto end;
		}
	}

	/*
	 * If no res info in current request, then we pick previous
	 * resolution info as current resolution info.
	 * Don't copy the sensor resolution info when the request id
	 * is invalid.
	 */
	if ((!s_ctrl->is_res_info_updated) && (csl_packet->header.request_id != 0)) {
		/*
		 * Update the last updated req at two places.
		 * 1# Got generic blob: The req id can be zero for the initial res info updating
		 * 2# Copy previous res info: The req id can't be zero, in case some queue info
		 * are override by slot0.
		 */
		s_ctrl->last_updated_req = csl_packet->header.request_id;
		s_ctrl->sensor_res[s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY] =
			s_ctrl->sensor_res[prev_updated_req % MAX_PER_FRAME_ARRAY];
	}

	if ((csl_packet->header.op_code & 0xFFFFFF) ==
		CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE) {
		i2c_reg_settings->request_id =
			csl_packet->header.request_id;
		rc = cam_sensor_update_req_mgr(s_ctrl, csl_packet);
		if (rc) {
			CAM_ERR(CAM_SENSOR,
				"Failed in adding request to req_mgr");
			goto end;
		}
	}

	if ((csl_packet->header.op_code & 0xFFFFFF) ==
		CAM_SENSOR_PACKET_OPCODE_SENSOR_FRAME_SKIP_UPDATE) {
		i2c_reg_settings->request_id =
			csl_packet->header.request_id;
	}

	if ((csl_packet->header.op_code & 0xFFFFFF) ==
		CAM_SENSOR_PACKET_OPCODE_SENSOR_BUBBLE_UPDATE) {
		i2c_reg_settings->request_id =
			csl_packet->header.request_id;
	}

end:
	cam_common_mem_free(csl_packet);
put_ref:
	cam_mem_put_cpu_buf(config.packet_handle);
	return rc;
}

static int32_t cam_sensor_i2c_modes_util(
	struct camera_io_master *io_master_info,
	struct i2c_settings_list *i2c_list)
{
	int32_t rc = 0;
	uint32_t i, size;

	if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_RANDOM) {
#if defined(CONFIG_SAMSUNG_CAMERA)
		struct cam_sensor_i2c_reg_array *reg_setting;
		uint32_t i2c_size = 0, org_size = 0, offset = 0;

		if (i2c_list->i2c_settings.size >  CCI_I2C_MAX_WRITE) {
			reg_setting = i2c_list->i2c_settings.reg_setting;
			org_size = i2c_list->i2c_settings.size;

			while(offset < org_size) {
				i2c_list->i2c_settings.reg_setting = reg_setting + offset;
				i2c_size = (uint32_t)(org_size - offset);
				if (i2c_size > CCI_I2C_MAX_WRITE)
					i2c_size = CCI_I2C_MAX_WRITE - 1;
				i2c_list->i2c_settings.size = i2c_size;
				rc = camera_io_dev_write(io_master_info,
					&(i2c_list->i2c_settings));
				if (rc < 0)
					break;
				offset += i2c_size;
			}
			i2c_list->i2c_settings.reg_setting = reg_setting;
			i2c_list->i2c_settings.size = org_size;
		}
		else
#endif
		rc = camera_io_dev_write(io_master_info,
			&(i2c_list->i2c_settings));
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to random write I2C settings: %d",
				rc);
			return rc;
		}
	} else if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_SEQ) {
		rc = camera_io_dev_write_continuous(
			io_master_info,
			&(i2c_list->i2c_settings),
			CAM_SENSOR_I2C_WRITE_SEQ);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to seq write I2C settings: %d",
				rc);
			return rc;
		}
	} else if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_BURST) {
#if defined(CONFIG_SAMSUNG_CAMERA)
		if(io_master_info->master_type == CCI_MASTER &&
			i2c_list->i2c_settings.size > CHUNK_SIZE){
			rc = camera_io_dev_write_continuous_split(i2c_list,io_master_info);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"Failed to CCI burst write split I2C settings: %d",
					rc);
			}
		}
		else {
#endif
		rc = camera_io_dev_write_continuous(
			io_master_info,
			&(i2c_list->i2c_settings),
			CAM_SENSOR_I2C_WRITE_BURST);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to burst write I2C settings: %d",
				rc);
			return rc;
		}
#if defined(CONFIG_SAMSUNG_CAMERA)
		}
#endif
	} else if (i2c_list->op_code == CAM_SENSOR_I2C_POLL) {
		size = i2c_list->i2c_settings.size;
		for (i = 0; i < size; i++) {
			rc = camera_io_dev_poll(
			io_master_info,
			i2c_list->i2c_settings.reg_setting[i].reg_addr,
			i2c_list->i2c_settings.reg_setting[i].reg_data,
			i2c_list->i2c_settings.reg_setting[i].data_mask,
			i2c_list->i2c_settings.addr_type,
			i2c_list->i2c_settings.data_type,
			i2c_list->i2c_settings.reg_setting[i].delay);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"i2c poll apply setting Fail: %d", rc);
				return rc;
			}
		}
	}

	return rc;
}

int32_t cam_sensor_update_i2c_info(struct cam_cmd_i2c_info *i2c_info,
	struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_cci_client   *cci_client = NULL;
	struct cam_sensor_qup_client   *qup_client = NULL;

	if (s_ctrl->io_master_info.master_type == CCI_MASTER) {
		cci_client = s_ctrl->io_master_info.cci_client;
		if (!cci_client) {
			CAM_ERR(CAM_SENSOR, "failed: cci_client %pK",
				cci_client);
			return -EINVAL;
		}
		cci_client->cci_i2c_master = s_ctrl->cci_i2c_master;
		cci_client->sid = i2c_info->slave_addr >> 1;
		cci_client->retries = 3;
		cci_client->id_map = 0;
		cci_client->i2c_freq_mode = i2c_info->i2c_freq_mode;
		CAM_DBG(CAM_SENSOR, "CCI: %d Master: %d slave_addr: 0x%x freq_mode: %d",
			cci_client->cci_device, cci_client->cci_i2c_master, i2c_info->slave_addr,
			i2c_info->i2c_freq_mode);
	} else if (s_ctrl->io_master_info.master_type == I2C_MASTER) {
		qup_client = s_ctrl->io_master_info.qup_client;
		if (!qup_client) {
			CAM_ERR(CAM_SENSOR, "failed: qup_client %pK",
				qup_client);
			return -EINVAL;
		}
		qup_client->i2c_client->addr = i2c_info->slave_addr;
		CAM_DBG(CAM_SENSOR, "Slave addr: 0x%x", i2c_info->slave_addr);
	}

	s_ctrl->sensordata->slave_info.sensor_slave_addr =
		i2c_info->slave_addr;
	return rc;
}

int32_t cam_sensor_update_slave_info(void *probe_info,
	uint32_t cmd, struct cam_sensor_ctrl_t *s_ctrl, uint8_t probe_ver)
{
	int32_t rc = 0;
	struct cam_cmd_probe *sensor_probe_info;
	struct cam_cmd_probe_v2 *sensor_probe_info_v2;

	memset(s_ctrl->sensor_name, 0, CAM_SENSOR_NAME_MAX_SIZE);
	memset(s_ctrl->io_master_info.sensor_name, 0, CAM_SENSOR_NAME_MAX_SIZE);

	if (probe_ver == CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE) {
		sensor_probe_info = (struct cam_cmd_probe *)probe_info;
		s_ctrl->sensordata->slave_info.sensor_id_reg_addr =
			sensor_probe_info->reg_addr;
		s_ctrl->sensordata->slave_info.sensor_id =
			sensor_probe_info->expected_data;
		s_ctrl->sensordata->slave_info.sensor_id_mask =
			sensor_probe_info->data_mask;
		s_ctrl->pipeline_delay =
			sensor_probe_info->reserved;
		s_ctrl->modeswitch_delay = 0;

		s_ctrl->sensor_probe_addr_type = sensor_probe_info->addr_type;
		s_ctrl->sensor_probe_data_type = sensor_probe_info->data_type;

		s_ctrl->probe_sensor_slave_addr = 0;
	} else if (probe_ver == CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE_V2) {
		sensor_probe_info_v2 = (struct cam_cmd_probe_v2 *)probe_info;
		s_ctrl->sensordata->slave_info.sensor_id_reg_addr =
			sensor_probe_info_v2->reg_addr;
		s_ctrl->sensordata->slave_info.sensor_id =
			sensor_probe_info_v2->expected_data;
		s_ctrl->sensordata->slave_info.sensor_id_mask =
			sensor_probe_info_v2->data_mask;
		s_ctrl->pipeline_delay =
			(sensor_probe_info_v2->pipeline_delay &
			CAM_SENSOR_PIPELINE_DELAY_MASK);
		s_ctrl->modeswitch_delay = (sensor_probe_info_v2->pipeline_delay >>
			CAM_SENSOR_MODESWITCH_DELAY_SHIFT);
		s_ctrl->sensor_probe_addr_type =
			sensor_probe_info_v2->addr_type;
		s_ctrl->sensor_probe_data_type =
			sensor_probe_info_v2->data_type;

		memcpy(s_ctrl->sensor_name, sensor_probe_info_v2->sensor_name,
			CAM_SENSOR_NAME_MAX_SIZE-1);

		s_ctrl->probe_sensor_slave_addr =
			sensor_probe_info_v2->reserved[0];
		memcpy(s_ctrl->io_master_info.sensor_name, sensor_probe_info_v2->sensor_name,
			CAM_SENSOR_NAME_MAX_SIZE-1);
	}

	CAM_DBG(CAM_SENSOR,
		"%s Sensor Addr: 0x%x sensor_id: 0x%x sensor_mask: 0x%x sensor_pipeline_delay:0x%x",
		s_ctrl->sensor_name,
		s_ctrl->sensordata->slave_info.sensor_id_reg_addr,
		s_ctrl->sensordata->slave_info.sensor_id,
		s_ctrl->sensordata->slave_info.sensor_id_mask,
		s_ctrl->pipeline_delay);
	return rc;
}

int32_t cam_handle_cmd_buffers_for_probe(void *cmd_buf,
	struct cam_sensor_ctrl_t *s_ctrl,
	int32_t cmd_buf_num, uint32_t cmd,
	uint32_t cmd_buf_length, size_t remain_len,
	uint32_t probe_ver, struct cam_cmd_buf_desc *cmd_desc)
{
	int32_t rc = 0;
	size_t required_size = 0;

	switch (cmd_buf_num) {
	case 0: {
		struct cam_cmd_i2c_info *i2c_info = NULL;
		void *probe_info;

		if (probe_ver == CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE)
			required_size = sizeof(struct cam_cmd_i2c_info) +
				sizeof(struct cam_cmd_probe);
		else if(probe_ver == CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE_V2)
			required_size = sizeof(struct cam_cmd_i2c_info) +
				sizeof(struct cam_cmd_probe_v2);

		if (remain_len < required_size) {
			CAM_ERR(CAM_SENSOR,
				"not enough buffer for cam_cmd_i2c_info");
			return -EINVAL;
		}
		i2c_info = (struct cam_cmd_i2c_info *)cmd_buf;
		rc = cam_sensor_update_i2c_info(i2c_info, s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed in Updating the i2c Info");
			return rc;
		}
		probe_info = cmd_buf + sizeof(struct cam_cmd_i2c_info);
		rc = cam_sensor_update_slave_info(probe_info, cmd, s_ctrl,
							probe_ver);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Updating the slave Info");
			return rc;
		}
	}
		break;
	case 1: {
		rc = cam_sensor_update_power_settings(cmd_buf,
			cmd_buf_length, &s_ctrl->sensordata->power_info,
			remain_len);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed in updating power settings");
			return rc;
		}
	}
		break;
#if defined(CONFIG_SENSOR_RETENTION)
	case 2: {
		struct i2c_settings_array *i2c_reg_settings = NULL;
		struct i2c_data_settings *i2c_data = NULL;
		struct cam_buf_io_cfg *io_cfg = NULL;

		CAM_DBG(CAM_SENSOR, "[RET_DBG] Receive Init Setting for booting");
		i2c_data = &(s_ctrl->i2c_data);
		i2c_reg_settings = &i2c_data->init_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		rc = cam_sensor_i2c_command_parser(&s_ctrl->io_master_info,
				i2c_reg_settings, cmd_desc, 1, io_cfg);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed in updating Init Setting for booting");
			return rc;
		}
	}
		break;
#endif
#if defined(CONFIG_SENSOR_RETENTION)
	case 3: {
#else
	case 2: {
#endif
		struct i2c_settings_array *i2c_reg_settings = NULL;
		struct i2c_data_settings *i2c_data = NULL;
		struct cam_buf_io_cfg *io_cfg = NULL;

		CAM_DBG(CAM_SENSOR, "reg_bank unlock settings");
		i2c_data = &(s_ctrl->i2c_data);
		i2c_reg_settings = &i2c_data->reg_bank_unlock_settings;
		i2c_reg_settings->request_id = 0;
		rc = cam_sensor_i2c_command_parser(&s_ctrl->io_master_info,
				i2c_reg_settings, cmd_desc, 1, io_cfg);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed in updating reg_bank unlock settings");
			return rc;
		}
	}
		break;
#if defined(CONFIG_SENSOR_RETENTION)
	case 4: {
#else
	case 3: {
#endif

		struct i2c_settings_array *i2c_reg_settings = NULL;
		struct i2c_data_settings *i2c_data = NULL;
		struct cam_buf_io_cfg *io_cfg = NULL;

		CAM_DBG(CAM_SENSOR, "reg_bank lock settings");
		i2c_data = &(s_ctrl->i2c_data);
		i2c_reg_settings = &i2c_data->reg_bank_lock_settings;
		i2c_reg_settings->request_id = 0;
		rc = cam_sensor_i2c_command_parser(&s_ctrl->io_master_info,
				i2c_reg_settings, cmd_desc, 1, io_cfg);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed in updating reg_bank lock settings");
			return rc;
		}
	}
		break;

	default:
		CAM_ERR(CAM_SENSOR, "Invalid command buffer");
		break;
	}
	return rc;
}

int32_t cam_handle_mem_ptr(uint64_t handle, uint32_t cmd,
	struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0, i;
	uint32_t *cmd_buf;
	void *ptr;
	size_t len;
	struct cam_packet *pkt = NULL;
	struct cam_packet *pkt_u = NULL;
	struct cam_cmd_buf_desc *cmd_desc = NULL;
	uintptr_t cmd_buf1 = 0;
	uintptr_t packet = 0;
	size_t    remain_len = 0;
	uint32_t  probe_ver = 0;
	size_t    packet_size = 0;

	rc = cam_mem_get_cpu_buf(handle,
		&packet, &len);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to get the command Buffer");
		return -EINVAL;
	}

	pkt_u = (struct cam_packet *)packet;
	if (pkt_u == NULL) {
		CAM_ERR(CAM_SENSOR, "packet pos is invalid");
		rc = -EINVAL;
		goto put_ref;
	}

	packet_size = pkt_u->header.size;
	if (packet_size <= len) {
		rc = cam_common_mem_kdup((void **)&pkt, pkt_u,
			packet_size);
		if (rc) {
			CAM_ERR(CAM_SENSOR, "Alloc and copy local pkt fail");
			goto put_ref;
		}
	} else {
		CAM_ERR(CAM_SENSOR, "Invalid packet header size %u",
			packet_size);
		rc = -EINVAL;
		goto put_ref;
	}

	if (cam_packet_util_validate_packet(pkt,
		len)) {
		CAM_ERR(CAM_SENSOR, "Invalid packet params");
		rc = -EINVAL;
		goto end;
	}

	if ((len < sizeof(struct cam_packet)) ||
		(pkt->cmd_buf_offset >= (len - sizeof(struct cam_packet)))) {
		CAM_ERR(CAM_SENSOR, "Not enough buf provided");
		rc = -EINVAL;
		goto end;
	}

	cmd_desc = (struct cam_cmd_buf_desc *)
		((uint32_t *)&pkt->payload_flex + pkt->cmd_buf_offset/4);
	if (cmd_desc == NULL) {
		CAM_ERR(CAM_SENSOR, "command descriptor pos is invalid");
		rc = -EINVAL;
		goto end;
	}

	probe_ver = pkt->header.op_code & 0xFFFFFF;
	CAM_DBG(CAM_SENSOR, "Received Header opcode: %u", probe_ver);

	for (i = 0; i < pkt->num_cmd_buf; i++) {
		rc = cam_packet_util_validate_cmd_desc(&cmd_desc[i]);
		if (rc)
			goto end;

		if (!(cmd_desc[i].length))
			continue;
		rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
			&cmd_buf1, &len);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to parse the command Buffer Header");
			goto end;
		}
		if (cmd_desc[i].offset >= len) {
			CAM_ERR(CAM_SENSOR,
				"offset past length of buffer");
			cam_mem_put_cpu_buf(cmd_desc[i].mem_handle);
			rc = -EINVAL;
			goto end;
		}
		remain_len = len - cmd_desc[i].offset;
		if (cmd_desc[i].length > remain_len) {
			CAM_ERR(CAM_SENSOR,
				"Not enough buffer provided for cmd");
			cam_mem_put_cpu_buf(cmd_desc[i].mem_handle);
			rc = -EINVAL;
			goto end;
		}
		cmd_buf = (uint32_t *)cmd_buf1;
		cmd_buf += cmd_desc[i].offset/4;
		ptr = (void *) cmd_buf;

		rc = cam_handle_cmd_buffers_for_probe(ptr, s_ctrl,
			i, cmd, cmd_desc[i].length, remain_len, probe_ver, &cmd_desc[i]);

		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to parse the command Buffer Header");
			cam_mem_put_cpu_buf(cmd_desc[i].mem_handle);
			goto end;
		}
		cam_mem_put_cpu_buf(cmd_desc[i].mem_handle);
	}

end:
	cam_common_mem_free(pkt);
put_ref:
	cam_mem_put_cpu_buf(handle);
	return rc;
}

void cam_sensor_query_cap(struct cam_sensor_ctrl_t *s_ctrl,
	struct  cam_sensor_query_cap *query_cap)
{
	query_cap->pos_roll = s_ctrl->sensordata->pos_roll;
	query_cap->pos_pitch = s_ctrl->sensordata->pos_pitch;
	query_cap->pos_yaw = s_ctrl->sensordata->pos_yaw;
	query_cap->secure_camera = 0;
	query_cap->actuator_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_ACTUATOR];
	query_cap->csiphy_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY];
	query_cap->eeprom_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_EEPROM];
	query_cap->flash_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_LED_FLASH];
	query_cap->ois_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_OIS];
	query_cap->slot_info =
		s_ctrl->soc_info.index;
}

static uint16_t cam_sensor_id_by_mask(struct cam_sensor_ctrl_t *s_ctrl,
	uint32_t chipid)
{
	uint16_t sensor_id = (uint16_t)(chipid & 0xFFFF);
	int16_t sensor_id_mask = s_ctrl->sensordata->slave_info.sensor_id_mask;

	if (!sensor_id_mask)
		sensor_id_mask = ~sensor_id_mask;

	sensor_id &= sensor_id_mask;
	sensor_id_mask &= -sensor_id_mask;
	sensor_id_mask -= 1;
	while (sensor_id_mask) {
		sensor_id_mask >>= 1;
		sensor_id >>= 1;
	}
	return sensor_id;
}

void cam_sensor_shutdown(struct cam_sensor_ctrl_t *s_ctrl)
{
	struct cam_sensor_power_ctrl_t *power_info =
		&s_ctrl->sensordata->power_info;
	int rc = 0;

#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
	cam_sensor_thread_destroy(s_ctrl);
#endif

	if ((s_ctrl->sensor_state == CAM_SENSOR_INIT) &&
		(s_ctrl->is_probe_succeed == 0))
		return;

#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
	if (s_ctrl->sensor_state == CAM_SENSOR_START) {
		CAM_INFO(CAM_SENSOR, "STREAM OFF and RETENTION state check before Powerdown to keep sensor in right state");
		if (to_dump_when_sof_freeze__sen_id == s_ctrl->sensordata->slave_info.sensor_id) {
			cam_sensor_dbg_regdump(s_ctrl);
			to_dump_when_sof_freeze__sen_id = 0;
		}
	}
#endif

	cam_sensor_release_stream_rsc(s_ctrl);
	cam_sensor_release_per_frame_resource(s_ctrl);

#if defined(CONFIG_SENSOR_RETENTION)
	if (s_ctrl->retention_info.retention_support)
		cam_sensor_util_retention_power_down(power_info,
			&(s_ctrl->soc_info));
#endif

	if (s_ctrl->sensor_state != CAM_SENSOR_INIT)
#if defined(CONFIG_SENSOR_RETENTION)
	{
		s_ctrl->retention_info.retention_enter(s_ctrl);
		cam_sensor_power_down(s_ctrl);
	}
#else
		cam_sensor_power_down(s_ctrl);
#endif


	if (s_ctrl->bridge_intf.device_hdl != -1) {
		rc = cam_destroy_device_hdl(s_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"dhdl already destroyed: rc = %d", rc);
	}

	s_ctrl->bridge_intf.device_hdl = -1;
	s_ctrl->bridge_intf.link_hdl = -1;
	s_ctrl->bridge_intf.session_hdl = -1;
	CAM_MEM_FREE(power_info->power_setting);
	CAM_MEM_FREE(power_info->power_down_setting);
	power_info->power_setting = NULL;
	power_info->power_down_setting = NULL;
	power_info->power_setting_size = 0;
	power_info->power_down_setting_size = 0;
	s_ctrl->streamon_count = 0;
	s_ctrl->streamoff_count = 0;
	s_ctrl->is_probe_succeed = 0;
	s_ctrl->last_flush_req = 0;
	s_ctrl->sensor_state = CAM_SENSOR_INIT;
}

int cam_sensor_match_id(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint32_t chipid = 0;
	struct cam_camera_slave_info *slave_info;
	
#if defined(CONFIG_SEC_PA3Q_PROJECT)
	struct cam_sensor_i2c_reg_array s5kjn3_clock_stretch_setting[] = {
		{ 0xFCFC, 0x4000, 0x00, 0x00 },
		{ 0x6006, 0x0600, 0x00, 0x00 },
	};

	struct cam_sensor_i2c_reg_setting s5kjn3_clock_stretch_settings[] =	{
		{
			s5kjn3_clock_stretch_setting,
			ARRAY_SIZE(s5kjn3_clock_stretch_setting),
			CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD,
			0
		},
	};
#endif

	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!slave_info) {
		CAM_ERR(CAM_SENSOR, " failed: %pK",
			 slave_info);
		return -EINVAL;
	}

	if (s_ctrl->hw_no_ops)
		return rc;

	if (s_ctrl->io_master_info.master_type == I2C_MASTER) {
		if (s_ctrl->probe_sensor_slave_addr != 0) {
			CAM_DBG(CAM_SENSOR, "%s read id: 0x%x -> 0x%x", s_ctrl->sensor_name,
				s_ctrl->io_master_info.qup_client->i2c_client->addr,
				s_ctrl->probe_sensor_slave_addr);
			s_ctrl->io_master_info.qup_client->i2c_client->addr =
				s_ctrl->probe_sensor_slave_addr;
		}
	} else if (s_ctrl->io_master_info.master_type == CCI_MASTER) {
		if (s_ctrl->probe_sensor_slave_addr != 0) {
			CAM_DBG(CAM_SENSOR, "%s read id: 0x%x -> 0x%x", s_ctrl->sensor_name,
				s_ctrl->io_master_info.cci_client->sid << 1,
				s_ctrl->probe_sensor_slave_addr);
			s_ctrl->io_master_info.cci_client->sid =
				s_ctrl->probe_sensor_slave_addr >> 1;
		}
	}
#if defined(CONFIG_SEC_PA3Q_PROJECT)
	if (strstr("s5kjn3", s_ctrl->sensor_name) != NULL) {
		rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
			s5kjn3_clock_stretch_settings, ARRAY_SIZE(s5kjn3_clock_stretch_settings));
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"Failed to clock stretch setting: %d", rc);
	}
#endif
	rc = camera_io_dev_read(
		&(s_ctrl->io_master_info),
		slave_info->sensor_id_reg_addr,
		&chipid, s_ctrl->sensor_probe_addr_type,
		s_ctrl->sensor_probe_data_type, true);

	CAM_INFO(CAM_SENSOR, "%s read id: 0x%x expected id 0x%x:",
		s_ctrl->sensor_name, chipid, slave_info->sensor_id);

	if (cam_sensor_id_by_mask(s_ctrl, chipid) != slave_info->sensor_id) {
		CAM_WARN(CAM_SENSOR, "%s read id: 0x%x expected id 0x%x:",
				s_ctrl->sensor_name, chipid,
				slave_info->sensor_id);
		return -ENODEV;
	}
	return rc;
}

int cam_sensor_stream_off(struct cam_sensor_ctrl_t *s_ctrl)
{
	int               rc = 0;
	struct timespec64 ts;
	uint64_t          ms, sec, min, hrs;

	if (s_ctrl->sensor_state != CAM_SENSOR_START &&
		(s_ctrl->sensor_state != CAM_SENSOR_STANDBY)) {
		rc = -EINVAL;
		CAM_WARN(CAM_SENSOR,
			"Not in right state to stop %s state: %d",
			s_ctrl->sensor_name, s_ctrl->sensor_state);
		goto end;
	}

	if ((!s_ctrl->stream_off_on_flush ||
		s_ctrl->is_stream_off_pkt_updated) &&
		s_ctrl->i2c_data.streamoff_settings.is_settings_valid &&
		(s_ctrl->i2c_data.streamoff_settings.request_id == 0)) {
		rc = cam_sensor_apply_settings(s_ctrl, 0,
			CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"cannot apply streamoff settings for %s",
				s_ctrl->sensor_name);
	}

	cam_sensor_release_per_frame_resource(s_ctrl);
	s_ctrl->last_flush_req = 0;
	s_ctrl->sensor_state = CAM_SENSOR_ACQUIRE;
	s_ctrl->stream_off_on_flush = false;
	s_ctrl->is_stream_off_pkt_updated = false;
	memset(s_ctrl->sensor_res, 0, sizeof(s_ctrl->sensor_res));

	CAM_GET_TIMESTAMP(ts);
	CAM_CONVERT_TIMESTAMP_FORMAT(ts, hrs, min, sec, ms);

	CAM_INFO(CAM_SENSOR,
		"%llu:%llu:%llu.%llu CAM_STOP_DEV Success for %s sensor_id:0x%x,sensor_slave_addr:0x%x",
		hrs, min, sec, ms,
		s_ctrl->sensor_name,
		s_ctrl->sensordata->slave_info.sensor_id,
		s_ctrl->sensordata->slave_info.sensor_slave_addr);

end:
	return rc;
}

int32_t cam_sensor_driver_cmd(struct cam_sensor_ctrl_t *s_ctrl,
	void *arg)
{
	int rc = 0, pkt_opcode = 0;
	struct cam_control *cmd = (struct cam_control *)arg;
	struct cam_sensor_power_ctrl_t *power_info =
		&s_ctrl->sensordata->power_info;
	struct timespec64 ts;
	uint64_t ms, sec, min, hrs;
#if defined(CONFIG_SAMSUNG_CAMERA)
	int i = 0, retry = 3;
#endif
#if defined(CONFIG_CAMERA_CDR_TEST)
	int32_t cam_idx = 0;
#endif
#if defined(CONFIG_SAMSUNG_READ_BPC_FROM_OTP)
	uint32_t sensor_revision = 0;
#endif

	if (!s_ctrl || !arg) {
		CAM_ERR(CAM_SENSOR, "s_ctrl is NULL");
		return -EINVAL;
	}

	if (cmd->op_code != CAM_SENSOR_PROBE_CMD) {
		if (cmd->handle_type != CAM_HANDLE_USER_POINTER) {
			CAM_ERR(CAM_SENSOR, "Invalid handle type: %d",
				cmd->handle_type);
			return -EINVAL;
		}
	}

#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
	s_ctrl->is_bubble_packet = false;
#endif

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	switch (cmd->op_code) {
	case CAM_SENSOR_PROBE_CMD: {
		if (s_ctrl->is_probe_succeed == 1) {
			CAM_WARN(CAM_SENSOR,
				"Sensor %s already Probed in the slot",
				s_ctrl->sensor_name);
			break;
		}

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
		sec_sensor_position = s_ctrl->id;
#endif

		if (cmd->handle_type ==
			CAM_HANDLE_MEM_HANDLE) {
			rc = cam_handle_mem_ptr(cmd->handle, cmd->op_code,
				s_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Get Buffer Handle Failed");
				goto release_mutex;
			}
		} else {
			CAM_ERR(CAM_SENSOR, "Invalid Command Type: %d",
				cmd->handle_type);
			rc = -EINVAL;
			goto release_mutex;
		}

		/* Parse and fill vreg params for powerup settings */
		rc = msm_camera_fill_vreg_params(
			&s_ctrl->soc_info,
			s_ctrl->sensordata->power_info.power_setting,
			s_ctrl->sensordata->power_info.power_setting_size);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Fail in filling vreg params for %s PUP rc %d",
				s_ctrl->sensor_name, rc);
			goto free_power_settings;
		}

		/* Parse and fill vreg params for powerdown settings*/
		rc = msm_camera_fill_vreg_params(
			&s_ctrl->soc_info,
			s_ctrl->sensordata->power_info.power_down_setting,
			s_ctrl->sensordata->power_info.power_down_setting_size);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Fail in filling vreg params for %s PDOWN rc %d",
				s_ctrl->sensor_name, rc);
			goto free_power_settings;
		}

		/* Power up and probe sensor */
		rc = cam_sensor_power_up(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Power up failed for %s sensor_id: 0x%x, slave_addr: 0x%x",
				s_ctrl->sensor_name,
				s_ctrl->sensordata->slave_info.sensor_id,
				s_ctrl->sensordata->slave_info.sensor_slave_addr
				);
			goto free_power_settings;
		}
#if defined(CONFIG_SENSOR_RETENTION)
		cam_sensor_util_retention_power_up(power_info,
			&(s_ctrl->soc_info));
#endif

		if (s_ctrl->i2c_data.reg_bank_unlock_settings.is_settings_valid) {
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				CAM_SENSOR_PACKET_OPCODE_SENSOR_REG_BANK_UNLOCK);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "REG_bank unlock failed");
				cam_sensor_power_down(s_ctrl);
				goto free_power_settings;
			}
			rc = delete_request(&(s_ctrl->i2c_data.reg_bank_unlock_settings));
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"failed while deleting REG_bank unlock settings");
				cam_sensor_power_down(s_ctrl);
				goto free_power_settings;
			}
		}

		/* Match sensor ID */
		rc = cam_sensor_match_id(s_ctrl);
#if !defined(CONFIG_SAMSUNG_CAMERA)
		if (rc < 0) {
			CAM_INFO(CAM_SENSOR,
				"Probe failed for %s slot:%d, slave_addr:0x%x, sensor_id:0x%x",
				s_ctrl->sensor_name,
				s_ctrl->soc_info.index,
				s_ctrl->sensordata->slave_info.sensor_slave_addr,
				s_ctrl->sensordata->slave_info.sensor_id);
			cam_sensor_power_down(s_ctrl);
			goto free_power_settings;
		}
#endif
#if defined(CONFIG_SEC_KUNIT)
		cam_sensor_hook_io_client(s_ctrl);
#endif

#if defined(CONFIG_SAMSUNG_READ_BPC_FROM_OTP)
		if ((s_ctrl->sensordata->slave_info.sensor_id == S5KHP2_SENSOR_ID) &&
			cam_sensor_is_need_to_read_otp(s_ctrl, &sensor_revision))
			cam_sensor_read_bpc_from_otp(s_ctrl, sensor_revision);
#endif

#if defined(CONFIG_SENSOR_RETENTION)
		cam_sensor_get_retention_info(s_ctrl);
		s_ctrl->retention_info.retention_init(s_ctrl);
		if (s_ctrl->i2c_data.init_settings.is_settings_valid &&
			(s_ctrl->i2c_data.init_settings.request_id == 0)) {
			rc = delete_request(&s_ctrl->i2c_data.init_settings);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"%s: Fail in deleting the Init settings",
					s_ctrl->sensor_name);
			}
		}
#endif


		if (s_ctrl->i2c_data.reg_bank_lock_settings.is_settings_valid) {
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				CAM_SENSOR_PACKET_OPCODE_SENSOR_REG_BANK_LOCK);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "REG_bank lock failed");
				cam_sensor_power_down(s_ctrl);
				goto free_power_settings;
			}
			rc = delete_request(&(s_ctrl->i2c_data.reg_bank_lock_settings));
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"failed while deleting REG_bank lock settings");
				cam_sensor_power_down(s_ctrl);
				goto free_power_settings;
			}
		}

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
		if (rc < 0) {
			CAM_ERR(CAM_UTIL, "[HWB]failed rc %d\n", rc);
			if (s_ctrl != NULL) {
				hw_bigdata_i2c_from_sensor(s_ctrl);
			}
		}
		hw_bigdata_update_cam_entrance_cnt(s_ctrl);
#endif

		rc = cam_sensor_power_down(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Fail in %s sensor Power Down",
				s_ctrl->sensor_name);
			goto free_power_settings;
		}

#if defined(CONFIG_CAMERA_SYSFS_V2)
		sId[s_ctrl->soc_info.index] =
			s_ctrl->sensordata->slave_info.sensor_id;
#endif

		/*
		 * Set probe succeeded flag to 1 so that no other camera shall
		 * probed on this slot
		 */
		s_ctrl->is_probe_succeed = 1;
		s_ctrl->sensor_state = CAM_SENSOR_INIT;

		CAM_INFO(CAM_SENSOR,
				"Probe success for %s slot:%d,slave_addr:0x%x,sensor_id:0x%x",
				s_ctrl->sensor_name,
				s_ctrl->soc_info.index,
				s_ctrl->sensordata->slave_info.sensor_slave_addr,
				s_ctrl->sensordata->slave_info.sensor_id);

	}
		break;
	case CAM_ACQUIRE_DEV: {
		struct cam_sensor_acquire_dev sensor_acq_dev;
		struct cam_create_dev_hdl bridge_params;

		if ((s_ctrl->is_probe_succeed == 0) ||
			(s_ctrl->sensor_state != CAM_SENSOR_INIT)) {
			CAM_WARN(CAM_SENSOR,
				"Not in right state to aquire %s state: %d",
				s_ctrl->sensor_name, s_ctrl->sensor_state);
			rc = -EINVAL;
			goto release_mutex;
		}

		if (s_ctrl->bridge_intf.device_hdl != -1) {
			CAM_ERR(CAM_SENSOR,
				"%s Device is already acquired",
				s_ctrl->sensor_name);
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = copy_from_user(&sensor_acq_dev,
			u64_to_user_ptr(cmd->handle),
			sizeof(sensor_acq_dev));
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed Copying from user");
			goto release_mutex;
		}

		bridge_params.session_hdl = sensor_acq_dev.session_handle;
		bridge_params.ops = &s_ctrl->bridge_intf.ops;
		bridge_params.v4l2_sub_dev_flag = 0;
		bridge_params.media_entity_flag = 0;
		bridge_params.priv = s_ctrl;
		bridge_params.dev_id = CAM_SENSOR;

		sensor_acq_dev.device_handle =
			cam_create_device_hdl(&bridge_params);
		if (sensor_acq_dev.device_handle <= 0) {
			rc = -EFAULT;
			CAM_ERR(CAM_SENSOR, "Can not create device handle");
			goto release_mutex;
		}
		s_ctrl->bridge_intf.device_hdl = sensor_acq_dev.device_handle;
		s_ctrl->bridge_intf.session_hdl = sensor_acq_dev.session_handle;

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
		if (MAX_CAMERAS > s_ctrl->id) {
			sec_sensor_position = s_ctrl->id;
			CAM_DBG(CAM_UTIL, "[HWB]sensor_position: %d", sec_sensor_position);
		}
#endif

		CAM_DBG(CAM_SENSOR, "%s Device Handle: %d",
			s_ctrl->sensor_name, sensor_acq_dev.device_handle);
		if (copy_to_user(u64_to_user_ptr(cmd->handle),
			&sensor_acq_dev,
			sizeof(struct cam_sensor_acquire_dev))) {
			CAM_ERR(CAM_SENSOR, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}

#if defined(CONFIG_SAMSUNG_CAMERA)
		for (i = 0; i < retry; i++) {
#endif
		rc = cam_sensor_power_up(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Sensor Power up failed for %s sensor_id:0x%x, slave_addr:0x%x",
				s_ctrl->sensor_name,
				s_ctrl->sensordata->slave_info.sensor_id,
				s_ctrl->sensordata->slave_info.sensor_slave_addr
				);
			goto release_mutex;
		}
#if defined(CONFIG_SAMSUNG_CAMERA)
		/* Match sensor ID */
		rc = cam_sensor_match_id(s_ctrl);
		if (rc == 0)
			break;
		else {
			cam_sensor_power_down(s_ctrl);
			msleep(20);
			if (i < (retry - 1))
				continue;
			s_ctrl->bridge_intf.device_hdl = -1;
			s_ctrl->bridge_intf.link_hdl = -1;
			s_ctrl->bridge_intf.session_hdl = -1;
			goto release_mutex;
		}
		}
#endif

#if defined(CONFIG_CAMERA_CDR_TEST)
		cam_idx = cam_clock_data_get_camera_type(s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY]);

		if (cam_idx != 0) {
			if (cam_clock_data_recovery_is_requested(cam_idx)) {
				cam_clock_data_recovery_get_timestamp(cam_idx, CDR_START_TS);
			}
		}
#endif

		s_ctrl->sensor_state = CAM_SENSOR_ACQUIRE;
		s_ctrl->last_flush_req = 0;
		s_ctrl->is_stopped_by_user = false;
		s_ctrl->last_updated_req = 0;
		s_ctrl->last_applied_req = 0;
		s_ctrl->num_batched_frames = 0;
		s_ctrl->last_applied_done_timestamp = 0;
		s_ctrl->stream_off_on_flush = false;
		s_ctrl->is_stream_off_pkt_updated = false;
		memset(s_ctrl->sensor_res, 0, sizeof(s_ctrl->sensor_res));
		CAM_INFO(CAM_SENSOR,
			"CAM_ACQUIRE_DEV Success for %s sensor_id:0x%x,sensor_slave_addr:0x%x",
			s_ctrl->sensor_name,
			s_ctrl->sensordata->slave_info.sensor_id,
			s_ctrl->sensordata->slave_info.sensor_slave_addr);

#if defined(CONFIG_SENSOR_RETENTION)
		s_ctrl->retention_info.retention_exit(s_ctrl);
#endif
	}
		break;
	case CAM_RELEASE_DEV: {
		if ((s_ctrl->sensor_state == CAM_SENSOR_INIT) ||
			(s_ctrl->sensor_state == CAM_SENSOR_START)) {
			rc = -EINVAL;
			CAM_WARN(CAM_SENSOR,
				"Not in right state to release %s state: %d",
				s_ctrl->sensor_name, s_ctrl->sensor_state);
			goto release_mutex;
		}

		if (s_ctrl->bridge_intf.link_hdl != -1) {
			CAM_ERR(CAM_SENSOR,
				"%s Device [%d] still active on link 0x%x",
				s_ctrl->sensor_name,
				s_ctrl->sensor_state,
				s_ctrl->bridge_intf.link_hdl);
			rc = -EAGAIN;
			goto release_mutex;
		}

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
		hw_bigdata_debug_info();
#if defined(CONFIG_SAMSUNG_ACTUATOR_READ_HALL_VALUE)
		hw_bigdata_hall_from_actuator(s_ctrl);
#endif
#endif
#if defined(CONFIG_SENSOR_RETENTION)
		s_ctrl->retention_info.retention_enter(s_ctrl);
#endif

		rc = cam_sensor_power_down(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Sensor Power Down failed for %s sensor_id: 0x%x, slave_addr:0x%x",
				s_ctrl->sensor_name,
				s_ctrl->sensordata->slave_info.sensor_id,
				s_ctrl->sensordata->slave_info.sensor_slave_addr
				);
			goto release_mutex;
		}

		cam_sensor_release_per_frame_resource(s_ctrl);
		cam_sensor_release_stream_rsc(s_ctrl);
		if (s_ctrl->bridge_intf.device_hdl == -1) {
			CAM_ERR(CAM_SENSOR,
				"Invalid Handles: %s link hdl: %d device hdl: %d",
				s_ctrl->sensor_name,
				s_ctrl->bridge_intf.device_hdl,
				s_ctrl->bridge_intf.link_hdl);
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = cam_destroy_device_hdl(s_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"Failed in destroying %s device hdl",
				s_ctrl->sensor_name);
		s_ctrl->bridge_intf.device_hdl = -1;
		s_ctrl->bridge_intf.link_hdl = -1;
		s_ctrl->bridge_intf.session_hdl = -1;

		s_ctrl->sensor_state = CAM_SENSOR_INIT;
		CAM_INFO(CAM_SENSOR,
			"CAM_RELEASE_DEV Success for %s sensor_id:0x%x, slave_addr:0x%x",
			s_ctrl->sensor_name,
			s_ctrl->sensordata->slave_info.sensor_id,
			s_ctrl->sensordata->slave_info.sensor_slave_addr);
		s_ctrl->streamon_count = 0;
		s_ctrl->streamoff_count = 0;
		s_ctrl->last_flush_req = 0;
		s_ctrl->last_applied_done_timestamp = 0;
		s_ctrl->stream_off_on_flush = false;
		s_ctrl->is_stream_off_pkt_updated = false;
	}
		break;
	case CAM_QUERY_CAP: {
		struct  cam_sensor_query_cap sensor_cap;

		CAM_DBG(CAM_SENSOR, "%s Sensor Queried", s_ctrl->sensor_name);
		cam_sensor_query_cap(s_ctrl, &sensor_cap);
		if (copy_to_user(u64_to_user_ptr(cmd->handle),
			&sensor_cap, sizeof(struct  cam_sensor_query_cap))) {
			CAM_ERR(CAM_SENSOR, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		break;
	}
	case CAM_START_DEV: {
		struct cam_req_mgr_timer_notify timer;
		if ((s_ctrl->sensor_state == CAM_SENSOR_INIT) ||
			(s_ctrl->sensor_state == CAM_SENSOR_START) ||
			s_ctrl->sensor_state == CAM_SENSOR_STANDBY) {
			rc = -EINVAL;
			CAM_WARN(CAM_SENSOR,
			"Not in right state to start %s state: %d",
			s_ctrl->sensor_name,
			s_ctrl->sensor_state);
			goto release_mutex;
		}

#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
		// To print frame count,
		// echo 1 > /sys/module/camera/parameters/frame_cnt_dbg
		if (frame_cnt_dbg > 0)
		{
			rc = cam_sensor_thread_create(s_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Failed create sensor thread");
				goto release_mutex;
			}
		}
#endif

		if (s_ctrl->i2c_data.streamon_settings.is_settings_valid &&
			(s_ctrl->i2c_data.streamon_settings.request_id == 0)) {
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"cannot apply streamon settings for %s",
					s_ctrl->sensor_name);
				goto release_mutex;
			}
		}
		s_ctrl->sensor_state = CAM_SENSOR_START;

		if (s_ctrl->stream_off_after_eof)
			s_ctrl->is_stopped_by_user = false;

		if (s_ctrl->bridge_intf.crm_cb &&
			s_ctrl->bridge_intf.crm_cb->notify_timer) {
			timer.link_hdl = s_ctrl->bridge_intf.link_hdl;
			timer.dev_hdl = s_ctrl->bridge_intf.device_hdl;
			timer.state = true;
			rc = s_ctrl->bridge_intf.crm_cb->notify_timer(&timer);
			if (rc) {
				CAM_ERR(CAM_SENSOR,
					"%s Enable CRM SOF freeze timer failed rc: %d",
					s_ctrl->sensor_name, rc);
				return rc;
			}
		}

		CAM_GET_TIMESTAMP(ts);
		CAM_CONVERT_TIMESTAMP_FORMAT(ts, hrs, min, sec, ms);

		CAM_INFO(CAM_SENSOR,
			"%llu:%llu:%llu.%llu CAM_START_DEV Success for %s sensor_id:0x%x,sensor_slave_addr:0x%x num_batched_frames:%d",
			hrs, min, sec, ms,
			s_ctrl->sensor_name,
			s_ctrl->sensordata->slave_info.sensor_id,
			s_ctrl->sensordata->slave_info.sensor_slave_addr,
			s_ctrl->num_batched_frames);
	}
		break;
	case CAM_STOP_DEV: {
		if (s_ctrl->stream_off_after_eof) {
			s_ctrl->is_stopped_by_user = true;
			CAM_DBG(CAM_SENSOR,
				"Ignore stop dev cmd, sensor %s already in streamed off state",
				s_ctrl->sensor_name);
			goto release_mutex;
		}

#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
		cam_sensor_thread_destroy(s_ctrl);
#endif

		rc = cam_sensor_stream_off(s_ctrl);
		if (rc)
			goto release_mutex;
	}
		break;
	case CAM_CONFIG_DEV: {
		rc = cam_sensor_pkt_parse(s_ctrl, arg);
		if (rc < 0) {
			if (rc == -EBADR)
				CAM_INFO(CAM_SENSOR,
					"%s:Failed pkt parse. rc: %d, it has been flushed",
					s_ctrl->sensor_name, rc);
			else
				CAM_ERR(CAM_SENSOR,
					"%s:Failed pkt parse. rc: %d",
					s_ctrl->sensor_name, rc);
			goto release_mutex;
		}
		if (s_ctrl->i2c_data.init_settings.is_settings_valid &&
			(s_ctrl->i2c_data.init_settings.request_id == 0)) {

			pkt_opcode =
				CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG;
#if defined(CONFIG_SENSOR_RETENTION)
			if (disable_sensor_retention == false &&
				s_ctrl->retention_info.retention_support) {
				if (s_ctrl->retention_checksum == true)
					CAM_INFO(CAM_SENSOR, "[RET_DBG] %s : Retention checksum pass, Skip write init", s_ctrl->sensor_name);
				else {
					CAM_INFO(CAM_SENSOR, "[RET_DBG] %s : Retention checksum fail. Full reset.", s_ctrl->sensor_name);
					cam_sensor_util_retention_power_down(power_info,
						&(s_ctrl->soc_info));
					cam_sensor_power_down(s_ctrl);
					cam_sensor_power_up(s_ctrl);
					cam_sensor_util_retention_power_up(power_info,
						&(s_ctrl->soc_info));
					s_ctrl->retention_info.retention_init(s_ctrl);
					s_ctrl->retention_info.retention_exit(s_ctrl);
				}
			}
			else
#endif
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				pkt_opcode);

			if ((rc == -EAGAIN) &&
			(s_ctrl->io_master_info.master_type == CCI_MASTER)) {
				/* If CCI hardware is resetting we need to wait
				 * for sometime before reapply
				 */
				CAM_WARN(CAM_SENSOR,
					"%s: Reapplying the Init settings due to cci hw reset",
					s_ctrl->sensor_name);
				usleep_range(1000, 1010);
				rc = cam_sensor_apply_settings(s_ctrl, 0,
					pkt_opcode);
			}
			s_ctrl->i2c_data.init_settings.request_id = -1;

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
			if (rc < 0) {
				CAM_ERR(CAM_UTIL, "[HWB]failed rc %d\n", rc);
				if (s_ctrl != NULL) {
					hw_bigdata_i2c_from_sensor(s_ctrl);
				}
			}
			hw_bigdata_update_eeprom_error_cnt(s_ctrl);
			hw_bigdata_update_cam_entrance_cnt(s_ctrl);
#endif

			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"%s: cannot apply init settings rc= %d",
					s_ctrl->sensor_name, rc);
				delete_request(&s_ctrl->i2c_data.init_settings);
				goto release_mutex;
			}
			rc = delete_request(&s_ctrl->i2c_data.init_settings);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"%s: Fail in deleting the Init settings",
					s_ctrl->sensor_name);
				goto release_mutex;
			}
		}

		if (s_ctrl->i2c_data.config_settings.is_settings_valid &&
			(s_ctrl->i2c_data.config_settings.request_id == 0)) {
			if (s_ctrl->sensor_state == CAM_SENSOR_START) {
				delete_request(&s_ctrl->i2c_data.config_settings);
				CAM_ERR(CAM_SENSOR,
					"%s: get config setting in start state",
					s_ctrl->sensor_name);
					goto release_mutex;
			}

			rc = cam_sensor_apply_settings(s_ctrl, 0,
				CAM_SENSOR_PACKET_OPCODE_SENSOR_CONFIG);

			s_ctrl->i2c_data.config_settings.request_id = -1;

			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"%s: cannot apply config settings",
					s_ctrl->sensor_name);
				delete_request(
					&s_ctrl->i2c_data.config_settings);
				goto release_mutex;
			}
			rc = delete_request(&s_ctrl->i2c_data.config_settings);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"%s: Fail in deleting the config settings",
					s_ctrl->sensor_name);
				goto release_mutex;
			}
			s_ctrl->sensor_state = CAM_SENSOR_CONFIG;
		}

		if (s_ctrl->i2c_data.read_settings.is_settings_valid) {
			if (!s_ctrl->hw_no_ops)
				rc = cam_sensor_i2c_read_data(
					&s_ctrl->i2c_data.read_settings,
					&s_ctrl->io_master_info);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "%s: cannot read data: %d",
					s_ctrl->sensor_name, rc);
				delete_request(&s_ctrl->i2c_data.read_settings);
				goto release_mutex;
			}
			rc = delete_request(
				&s_ctrl->i2c_data.read_settings);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"%s: Fail in deleting the read settings",
					s_ctrl->sensor_name);
				goto release_mutex;
			}
		}

		if ((s_ctrl->stream_off_on_flush) && (s_ctrl->sensor_state == CAM_SENSOR_CONFIG)) {
			rc = cam_sensor_notify_msg_req_mgr(
				CAM_REQ_MGR_MSG_NOTIFY_FOR_SYNCED_RESUME, s_ctrl);
			if (!rc)
				s_ctrl->stream_off_on_flush = false;
		}
	}
		break;
	default:
		CAM_ERR(CAM_SENSOR, "%s: Invalid Opcode: %d",
			s_ctrl->sensor_name, cmd->op_code);
		rc = -EINVAL;
		goto release_mutex;
	}

release_mutex:
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;

free_power_settings:
	CAM_MEM_FREE(power_info->power_setting);
	CAM_MEM_FREE(power_info->power_down_setting);
	power_info->power_setting = NULL;
	power_info->power_down_setting = NULL;
	power_info->power_down_setting_size = 0;
	power_info->power_setting_size = 0;
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

int cam_sensor_publish_dev_info(struct cam_req_mgr_device_info *info)
{
	int rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl = NULL;

	if (!info)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(info->dev_hdl);

	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	info->dev_id = CAM_REQ_MGR_DEVICE_SENSOR;
	snprintf(info->name, sizeof(info->name), "%s(%s)",
		CAM_SENSOR_NAME, s_ctrl->sensor_name);
	if (s_ctrl->num_batched_frames >= 2) {
		info->p_delay = 1;
		info->m_delay = s_ctrl->modeswitch_delay;
	} else if (s_ctrl->pipeline_delay >= 1 && s_ctrl->pipeline_delay <= 3) {
		info->p_delay = s_ctrl->pipeline_delay;
		info->m_delay = s_ctrl->modeswitch_delay;
	} else {
		info->p_delay = CAM_PIPELINE_DELAY_2;
		info->m_delay = CAM_MODESWITCH_DELAY_2;
	}
	info->trigger = CAM_TRIGGER_POINT_SOF;
	info->resume_sync_on = true;

	CAM_DBG(CAM_REQ, "num batched frames %d p_delay is %d",
		s_ctrl->num_batched_frames, info->p_delay);

	return rc;
}

int cam_sensor_establish_link(struct cam_req_mgr_core_dev_link_setup *link)
{
	struct cam_sensor_ctrl_t *s_ctrl = NULL;

	if (!link)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(link->dev_hdl);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	mutex_lock(&s_ctrl->cam_sensor_mutex);
	if (link->link_enable) {
		s_ctrl->bridge_intf.link_hdl = link->link_hdl;
		s_ctrl->bridge_intf.crm_cb = link->crm_cb;
	} else {
		s_ctrl->bridge_intf.link_hdl = -1;
		s_ctrl->bridge_intf.crm_cb = NULL;
	}
	mutex_unlock(&s_ctrl->cam_sensor_mutex);

	return 0;
}

int cam_sensor_power(struct v4l2_subdev *sd, int on)
{
	struct cam_sensor_ctrl_t *s_ctrl = v4l2_get_subdevdata(sd);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "s_ctrl ptr is NULL");
		return -EINVAL;
	}

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	if (!on && s_ctrl->sensor_state == CAM_SENSOR_START) {
		cam_sensor_power_down(s_ctrl);
		s_ctrl->sensor_state = CAM_SENSOR_ACQUIRE;
	}
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));

	return 0;
}

int cam_sensor_power_up(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	struct cam_sensor_power_ctrl_t *power_info;
	struct cam_camera_slave_info   *slave_info;
	struct cam_hw_soc_info         *soc_info = &s_ctrl->soc_info;
	struct completion              *i3c_probe_completion = NULL;

	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "failed: %pK", s_ctrl);
		return -EINVAL;
	}

	if (s_ctrl->hw_no_ops)
		return rc;

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	if (s_ctrl != NULL) {
		hw_bigdata_init_mipi_param_sensor(s_ctrl);
	}
#endif

	power_info = &s_ctrl->sensordata->power_info;
	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!power_info || !slave_info) {
		CAM_ERR(CAM_SENSOR, "failed: %pK %pK", power_info, slave_info);
		return -EINVAL;
	}

	if (s_ctrl->bob_pwm_switch) {
		if (cam_sensor_bob_pwm_mode_switch(soc_info,
			s_ctrl->bob_reg_index, true))
			CAM_WARN(CAM_SENSOR, "BoB PWM setup failed");
	}

	if (s_ctrl->aon_camera_id != NOT_AON_CAM) {
		CAM_INFO(CAM_SENSOR,
			"Setup for Main Camera with csiphy index: %d",
			s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY]);
		rc = cam_sensor_util_aon_ops(true,
			s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY]);
		if (rc) {
			CAM_ERR(CAM_SENSOR,
				"Main camera access operation is not successful rc: %d",
				rc);
			return rc;
		}
	}

	if (s_ctrl->io_master_info.master_type == I3C_MASTER)
		i3c_probe_completion = cam_sensor_get_i3c_completion(s_ctrl->soc_info.index);

	/* Check before power_up if i3c sensor has the master handle, Which means
	 * either Camera Daemon had restarted and probe cmd is issued again (or)
	 * acquire is called from UMD
	 */
	if ((s_ctrl->io_master_info.master_type == I3C_MASTER) &&
		(s_ctrl->io_master_info.qup_client != NULL)) {
		s_ctrl->io_master_info.qup_client->i3c_wait_for_hotjoin = true;
	}

	rc = cam_sensor_core_power_up(power_info, soc_info, i3c_probe_completion);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "core power up failed:%d", rc);
		goto powerup_failure;
	}

	rc = camera_io_init(&(s_ctrl->io_master_info));
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "cci_init failed: rc: %d", rc);
		goto cci_failure;
	}

	return rc;

cci_failure:
	if (cam_sensor_util_power_down(power_info, soc_info))
		CAM_ERR(CAM_SENSOR, "power down failure");

powerup_failure:
	if (s_ctrl->aon_camera_id != NOT_AON_CAM) {
		if (cam_sensor_util_aon_ops(false,
			s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY]))
			CAM_ERR(CAM_SENSOR,
				"Main camera disable CPAS operation is not successful");
	}
	return rc;

}

int cam_sensor_power_down(struct cam_sensor_ctrl_t *s_ctrl)
{
	struct cam_sensor_power_ctrl_t *power_info;
	struct cam_hw_soc_info *soc_info;
	int rc = 0;

	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "failed: s_ctrl %pK", s_ctrl);
		return -EINVAL;
	}

	if (s_ctrl->hw_no_ops)
		return rc;

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	if (s_ctrl != NULL) {
		hw_bigdata_deinit_mipi_param_sensor(s_ctrl);
	}
#endif

	power_info = &s_ctrl->sensordata->power_info;
	soc_info = &s_ctrl->soc_info;

	if (!power_info) {
		CAM_ERR(CAM_SENSOR, "failed: %s power_info %pK",
			s_ctrl->sensor_name, power_info);
		return -EINVAL;
	}

	rc = cam_sensor_util_power_down(power_info, soc_info);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "%s core power down failed:%d",
			s_ctrl->sensor_name, rc);
		goto powerdown_failure;
	}

	if (s_ctrl->aon_camera_id != NOT_AON_CAM) {
		CAM_INFO(CAM_SENSOR,
			"Setup for AON FW with csiphy index: %d",
			s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY]);
		rc = cam_sensor_util_aon_ops(false,
			s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY]);
		if (rc) {
			CAM_ERR(CAM_SENSOR,
				"AON FW access operation is not successful rc: %d",
				rc);
			return rc;
		}
	}

	if (s_ctrl->bob_pwm_switch) {
		rc = cam_sensor_bob_pwm_mode_switch(soc_info,
			s_ctrl->bob_reg_index, false);
		if (rc) {
			CAM_WARN(CAM_SENSOR,
				"%s BoB PWM setup failed rc: %d",
				s_ctrl->sensor_name, rc);
			rc = 0;
		}
	}

	camera_io_release(&(s_ctrl->io_master_info));

	return rc;

powerdown_failure:
	if (s_ctrl->aon_camera_id != NOT_AON_CAM) {
		if (cam_sensor_util_aon_ops(false,
			s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY]))
			CAM_ERR(CAM_SENSOR,
				"Main camera disable CPAS operation is not successful");
	}
	return rc;

}

int cam_sensor_apply_settings(struct cam_sensor_ctrl_t *s_ctrl,
	int64_t req_id, enum cam_sensor_packet_opcodes opcode)
{
	int rc = 0, offset, i, j;
	uint64_t top = 0, del_req_id = 0;
	struct i2c_settings_array *i2c_set = NULL;
	struct i2c_settings_array *i2c_set_per_req[CAM_SENSOR_MAX_PER_REQ_SETTINGS];
	struct i2c_settings_list *i2c_list;
	ktime_t current_time;
	struct timespec64 current_ts;
#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
	int32_t to_dbg_sen_id = -1;
	e_sensor_reg_upd_event_type sen_upd_evt_type = e_sensor_upd_event_invalid;
#endif

	if (req_id == 0) {
		switch (opcode) {
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON: {
			i2c_set = &s_ctrl->i2c_data.streamon_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG: {
			i2c_set = &s_ctrl->i2c_data.init_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_CONFIG: {
			i2c_set = &s_ctrl->i2c_data.config_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF: {
			i2c_set = &s_ctrl->i2c_data.streamoff_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_REG_BANK_UNLOCK: {
			i2c_set = &s_ctrl->i2c_data.reg_bank_unlock_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_REG_BANK_LOCK: {
			i2c_set = &s_ctrl->i2c_data.reg_bank_lock_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_FRAME_SKIP_UPDATE:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE_V2:
		default:
			return 0;
		}
		if (i2c_set->is_settings_valid == 1) {
#if defined(CONFIG_SAMSUNG_CAMERA)
			cam_sensor_pre_apply_settings(s_ctrl, opcode);
#endif
			list_for_each_entry(i2c_list,
				&(i2c_set->list_head), list) {
#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
				if (debug_sensor_name[0] != '\0') {
					cam_sensor_i2c_dump_util(s_ctrl, i2c_list, i2c_debug_cnt);
				}
				cam_sensor_parse_reg(s_ctrl, i2c_list,
					&to_dbg_sen_id,
					&sen_upd_evt_type);
#endif
				if (!s_ctrl->hw_no_ops)
					rc = cam_sensor_i2c_modes_util(
						&(s_ctrl->io_master_info),
						i2c_list);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR,
						"Failed to apply settings: %d",
						rc);
					return rc;
				}
			}
#if defined(CONFIG_SAMSUNG_CAMERA)
			cam_sensor_post_apply_settings(s_ctrl, opcode);
#endif
		}
	} else if (req_id > 0) {
		offset = req_id % MAX_PER_FRAME_ARRAY;

		i2c_set_per_req[0] = s_ctrl->i2c_data.per_frame;
		i2c_set_per_req[1] = s_ctrl->i2c_data.frame_skip;
		i2c_set_per_req[2] = s_ctrl->i2c_data.bubble_update;
		i2c_set_per_req[3] = s_ctrl->i2c_data.deferred_frame_update;

		if (opcode == CAM_SENSOR_PACKET_OPCODE_SENSOR_FRAME_SKIP_UPDATE)
			i2c_set = s_ctrl->i2c_data.frame_skip;
		else if (opcode == CAM_SENSOR_PACKET_OPCODE_SENSOR_BUBBLE_UPDATE) {
#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
			s_ctrl->is_bubble_packet = true;
#endif
			i2c_set = s_ctrl->i2c_data.bubble_update;
			/*
			 * If bubble update isn't valid, then we just use
			 * per frame update.
			 */
			if (!(i2c_set[offset].is_settings_valid == 1) &&
				(i2c_set[offset].request_id == req_id))
				i2c_set = s_ctrl->i2c_data.per_frame;
		} else if (opcode == CAM_SENSOR_PACKET_OPCODE_SENSOR_DEFERRED_META)
			i2c_set = s_ctrl->i2c_data.deferred_frame_update;
		else
			i2c_set = s_ctrl->i2c_data.per_frame;

		if (i2c_set[offset].is_settings_valid == 1 &&
			i2c_set[offset].request_id == req_id) {
			list_for_each_entry(i2c_list,
				&(i2c_set[offset].list_head), list) {
#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
				if (debug_sensor_name[0] != '\0') {
					cam_sensor_i2c_dump_util(s_ctrl, i2c_list, i2c_debug_cnt);
				}
				cam_sensor_parse_reg(s_ctrl, i2c_list,
					&to_dbg_sen_id,
					&sen_upd_evt_type);
#endif
				if (!s_ctrl->hw_no_ops)
					rc = cam_sensor_i2c_modes_util(
						&(s_ctrl->io_master_info),
						i2c_list);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR,
						"Failed to apply settings: %d",
						rc);
					return rc;
				}
			}
			CAM_DBG(CAM_SENSOR, "applied req_id: %llu", req_id);
		} else {
			CAM_DBG(CAM_SENSOR,
				"Invalid/NOP request to apply: %lld", req_id);
		}

		/* Record the sensor setting applied done timestamp */
		current_time = ktime_get();
		current_ts = ktime_to_timespec64(current_time);
		s_ctrl->last_applied_done_timestamp = current_ts.tv_sec * NSEC_PER_SEC +
			current_ts.tv_nsec;
		CAM_DBG(CAM_REQ,
			"Apply req:%lld done on %ld:%06ld last_applied_done_timestamp:0x%llx",
			req_id, current_ts.tv_sec,
			current_ts.tv_nsec/NSEC_PER_USEC, s_ctrl->last_applied_done_timestamp);

		s_ctrl->last_applied_req = req_id;
		CAM_DBG(CAM_REQ,
			"Sensor[%d] updating last_applied [req id: %lld last_applied: %lld] with opcode:%d",
			s_ctrl->soc_info.index, req_id, s_ctrl->last_applied_req, opcode);

		/* Change the logic dynamically */
		for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
			if ((req_id >=
				i2c_set[i].request_id) &&
				(top <
				i2c_set[i].request_id) &&
				(i2c_set[i].is_settings_valid
					== 1)) {
				del_req_id = top;
				top = i2c_set[i].request_id;
			}
		}

		if (top < req_id) {
			if ((((top % MAX_PER_FRAME_ARRAY) - (req_id %
				MAX_PER_FRAME_ARRAY)) >= BATCH_SIZE_MAX) ||
				(((top % MAX_PER_FRAME_ARRAY) - (req_id %
				MAX_PER_FRAME_ARRAY)) <= -BATCH_SIZE_MAX))
				del_req_id = req_id;
		}

		if (!del_req_id)
			return rc;

		CAM_DBG(CAM_SENSOR, "top: %llu, del_req_id:%llu",
			top, del_req_id);

		/* Delete all the per request settings */
		for (i = 0; i < CAM_SENSOR_MAX_PER_REQ_SETTINGS; i++) {
			i2c_set = i2c_set_per_req[i];
			for (j = 0; j < MAX_PER_FRAME_ARRAY; j++) {
				if ((del_req_id >
					i2c_set[j].request_id) && (
					i2c_set[j].is_settings_valid
						== 1)) {
					i2c_set[j].request_id = 0;
					rc = delete_request(
						&(i2c_set[j]));
					if (rc < 0)
						CAM_ERR(CAM_SENSOR,
							"Delete request Fail:%lld rc:%d i:%d j:%d",
							del_req_id, rc, i, j);
				}
			}
		}
	}

#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
	if ((to_dbg_sen_id == s_ctrl->sensordata->slave_info.sensor_id) &&
		(sen_upd_evt_type != e_sensor_upd_event_invalid))
		cam_sensor_dbg_print_by_upd_type(s_ctrl, sen_upd_evt_type);
#endif

	return rc;
}

int32_t cam_sensor_apply_request(struct cam_req_mgr_apply_request *apply)
{
	int32_t rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl = NULL;
	int32_t curr_idx, last_applied_idx;
	enum cam_sensor_packet_opcodes opcode =
		CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE;

	if (!apply)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(apply->dev_hdl);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	if ((apply->recovery) && (apply->request_id > 0)) {
		if (apply->request_id <= s_ctrl->last_applied_req) {
			/*
			 * Use bubble update for request reapply
			 */
			curr_idx = apply->request_id % MAX_PER_FRAME_ARRAY;
			last_applied_idx = s_ctrl->last_applied_req % MAX_PER_FRAME_ARRAY;
			opcode = CAM_SENSOR_PACKET_OPCODE_SENSOR_BUBBLE_UPDATE;
			CAM_INFO(CAM_REQ,
				"Sensor[%d] update req id: %lld [last_applied: %lld] with opcode:%d recovery: %d last_applied_res_idx: %u current_res_idx: %u",
				s_ctrl->soc_info.index, apply->request_id,
				s_ctrl->last_applied_req, opcode, apply->recovery,
				s_ctrl->sensor_res[last_applied_idx].res_index,
				s_ctrl->sensor_res[curr_idx].res_index);
		}
	}

	CAM_DBG(CAM_REQ,
		"Sensor[%d] update req id: %lld [last_applied: %lld] with opcode:%d recovery: %d",
		s_ctrl->soc_info.index, apply->request_id,
		s_ctrl->last_applied_req, opcode, apply->recovery);
	trace_cam_apply_req("Sensor", s_ctrl->soc_info.index, apply->request_id, apply->link_hdl);

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	rc = cam_sensor_apply_settings(s_ctrl, apply->request_id,
		opcode);
	apply->last_applied_done_timestamp = s_ctrl->last_applied_done_timestamp;
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

int32_t cam_sensor_notify_frame_skip(struct cam_req_mgr_apply_request *apply)
{
	int32_t rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl = NULL;
	enum cam_sensor_packet_opcodes opcode =
		CAM_SENSOR_PACKET_OPCODE_SENSOR_FRAME_SKIP_UPDATE;

	if (!apply)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(apply->dev_hdl);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	CAM_DBG(CAM_REQ, "Sensor[%d] handle frame skip for req id: %lld",
		s_ctrl->soc_info.index, apply->request_id);
	trace_cam_notify_frame_skip("Sensor", apply->request_id);
	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	rc = cam_sensor_apply_settings(s_ctrl, apply->request_id,
		opcode);
	/*
	 * If mode switch delay is 1, and there are no further requests
	 */
	if ((s_ctrl->modeswitch_delay == CAM_MODESWITCH_DELAY_1) && (apply->no_further_requests)) {
		cam_sensor_apply_settings(s_ctrl, apply->request_id,
			CAM_SENSOR_PACKET_OPCODE_SENSOR_DEFERRED_META);
		CAM_DBG(CAM_SENSOR, "Sensor[%d] applying deferred settings from req id: %lld",
			s_ctrl->soc_info.index, apply->request_id);
	}

	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

int32_t cam_sensor_flush_request(struct cam_req_mgr_flush_request *flush_req)
{
	int32_t rc = 0, i;
	uint32_t cancel_req_id_found = 0;
	struct cam_sensor_ctrl_t *s_ctrl = NULL;
	struct i2c_settings_array *i2c_set = NULL;

	if (!flush_req)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(flush_req->dev_hdl);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	if ((s_ctrl->sensor_state != CAM_SENSOR_START) &&
		(s_ctrl->sensor_state != CAM_SENSOR_CONFIG)) {
		mutex_unlock(&(s_ctrl->cam_sensor_mutex));
		return rc;
	}

	if (s_ctrl->i2c_data.deferred_frame_update == NULL) {
		CAM_ERR(CAM_SENSOR, "i2c frame data is NULL");
		mutex_unlock(&(s_ctrl->cam_sensor_mutex));
		return -EINVAL;
	}

	if (s_ctrl->i2c_data.per_frame == NULL) {
		CAM_ERR(CAM_SENSOR, "i2c frame data is NULL");
		mutex_unlock(&(s_ctrl->cam_sensor_mutex));
		return -EINVAL;
	}

	if (s_ctrl->i2c_data.frame_skip == NULL) {
		CAM_ERR(CAM_SENSOR, "i2c not ready data is NULL");
		mutex_unlock(&(s_ctrl->cam_sensor_mutex));
		return -EINVAL;
	}

	if (flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_ALL) {
		s_ctrl->last_flush_req = flush_req->req_id;
		CAM_DBG(CAM_SENSOR, "last reqest to flush is %lld",
			flush_req->req_id);

		if (s_ctrl->stream_off_after_eof || flush_req->enable_sensor_standby) {
			cam_sensor_stream_off(s_ctrl);
			s_ctrl->is_stopped_by_user = false;
			if (flush_req->enable_sensor_standby) {
				s_ctrl->sensor_state = CAM_SENSOR_STANDBY;
				s_ctrl->stream_off_on_flush = true;
			}
		}

		s_ctrl->stream_off_on_flush = flush_req->enable_sensor_standby;
	}

	for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
		i2c_set = &(s_ctrl->i2c_data.deferred_frame_update[i]);

		if ((flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ)
				&& (i2c_set->request_id != flush_req->req_id))
			continue;

		if (i2c_set->is_settings_valid == 1) {
			rc = delete_request(i2c_set);
			if (rc < 0)
				CAM_ERR(CAM_SENSOR,
					"delete request for no valid deferred req: %lld rc: %d",
					i2c_set->request_id, rc);

			if (flush_req->type ==
				CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ) {
				cancel_req_id_found = 1;
				break;
			}
		}
	}

	for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
		i2c_set = &(s_ctrl->i2c_data.per_frame[i]);

		if ((flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ)
				&& (i2c_set->request_id != flush_req->req_id))
			continue;

		if (i2c_set->is_settings_valid == 1) {
			rc = delete_request(i2c_set);
			if (rc < 0)
				CAM_ERR(CAM_SENSOR,
					"delete request: %lld rc: %d",
					i2c_set->request_id, rc);

			if (flush_req->type ==
				CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ) {
				cancel_req_id_found = 1;
				break;
			}
		}
	}

	for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
		i2c_set = &(s_ctrl->i2c_data.frame_skip[i]);

		if ((flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ)
				&& (i2c_set->request_id != flush_req->req_id))
			continue;

		if (i2c_set->is_settings_valid == 1) {
			rc = delete_request(i2c_set);
			if (rc < 0)
				CAM_ERR(CAM_SENSOR,
					"delete request for not ready packet: %lld rc: %d",
					i2c_set->request_id, rc);

			if (flush_req->type ==
				CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ) {
				cancel_req_id_found = 1;
				break;
			}
		}
	}

	if (flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ &&
		!cancel_req_id_found)
		CAM_DBG(CAM_SENSOR,
			"Flush request id:%lld not found in the pending list",
			flush_req->req_id);

	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

int cam_sensor_process_evt(struct cam_req_mgr_link_evt_data *evt_data)
{
	int                        rc = 0, offset;
	struct cam_sensor_ctrl_t  *s_ctrl = NULL;
	struct i2c_settings_array *i2c_set = NULL;

	if (!evt_data)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(evt_data->dev_hdl);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	CAM_DBG(CAM_SENSOR, "Received evt:%d", evt_data->evt_type);

	mutex_lock(&(s_ctrl->cam_sensor_mutex));

	switch (evt_data->evt_type) {
	case CAM_REQ_MGR_LINK_EVT_EOF:
		if (s_ctrl->stream_off_after_eof) {
			rc = cam_sensor_stream_off(s_ctrl);
			if (rc) {
				CAM_ERR(CAM_SENSOR, "Failed to stream off %s",
					s_ctrl->sensor_name);

				cam_sensor_notify_v4l2_error_event(s_ctrl,
					CAM_REQ_MGR_ERROR_TYPE_FULL_RECOVERY,
					CAM_REQ_MGR_SENSOR_STREAM_OFF_FAILED);
			}
		}
		break;
	case CAM_REQ_MGR_LINK_EVT_UPDATE_PROPERTIES:
		if (evt_data->u.properties_mask &
			CAM_LINK_PROPERTY_SENSOR_STANDBY_AFTER_EOF)
			s_ctrl->stream_off_after_eof = true;
		else
			s_ctrl->stream_off_after_eof = false;

		CAM_DBG(CAM_SENSOR, "sensor %s stream off after eof:%s",
			s_ctrl->sensor_name,
			CAM_BOOL_TO_YESNO(s_ctrl->stream_off_after_eof));
		break;
	case CAM_REQ_MGR_LINK_EVT_RESUME_HW: {
		struct timespec64 ts;
		uint64_t ms, sec, min, hrs;

		if (s_ctrl->i2c_data.streamon_settings.is_settings_valid &&
			(s_ctrl->i2c_data.streamon_settings.request_id == 0)) {
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"cannot apply streamon settings for %s",
					s_ctrl->sensor_name);
				break;
			}
			s_ctrl->sensor_state = CAM_SENSOR_START;
			s_ctrl->stream_off_on_flush = false;
			CAM_GET_TIMESTAMP(ts);
			CAM_CONVERT_TIMESTAMP_FORMAT(ts, hrs, min, sec, ms);

			CAM_INFO(CAM_SENSOR,
				"%llu:%llu:%llu.%llu CAM_START_DEV for %s sensor_id:0x%x,sensor_slave_addr:0x%x num_batched_frames:%d post flush",
				hrs, min, sec, ms,
				s_ctrl->sensor_name,
				s_ctrl->sensordata->slave_info.sensor_id,
				s_ctrl->sensordata->slave_info.sensor_slave_addr,
				s_ctrl->num_batched_frames);
		}
	}
		break;
	case CAM_REQ_MGR_LINK_EVT_FRAME_DURATION_CHANGING: {
		evt_data->u.frame_duration_changing = false;
		offset = evt_data->req_id % MAX_PER_FRAME_ARRAY;
		i2c_set = s_ctrl->i2c_data.frame_skip;

		if (i2c_set[offset].is_settings_valid &&
			(i2c_set[offset].request_id == evt_data->req_id))
			evt_data->u.frame_duration_changing = true;

		i2c_set = s_ctrl->i2c_data.bubble_update;
		if (i2c_set[offset].is_settings_valid && evt_data->u.is_recovery &&
			(evt_data->req_id <= s_ctrl->last_applied_req) &&
			(i2c_set[offset].request_id == evt_data->req_id))
			evt_data->u.frame_duration_changing = true;
	}
		break;
#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
	case CAM_REQ_MGR_LINK_EVT_SOF_FREEZE:
	case CAM_REQ_MGR_LINK_EVT_ERR:
		CAM_INFO(CAM_SENSOR, "[FREEZE_DBG][%s] sof freeze proc_evt %d", s_ctrl->sensor_name,
			evt_data->evt_type);
		to_dump_when_sof_freeze__sen_id = s_ctrl->sensordata->slave_info.sensor_id;
		break;
#endif
	default:
		/* No handling */
		break;
	}

	mutex_unlock(&(s_ctrl->cam_sensor_mutex));

	return rc;
}

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
int msm_is_sec_get_sensor_position(uint32_t *cam_position)
{
	*cam_position = sec_sensor_position;
	return 0;
}

int msm_is_sec_get_sensor_comp_mode(uint32_t **sensor_clk_size)
{
	*sensor_clk_size = &sec_sensor_clk_size;
	return 0;
}
#endif
