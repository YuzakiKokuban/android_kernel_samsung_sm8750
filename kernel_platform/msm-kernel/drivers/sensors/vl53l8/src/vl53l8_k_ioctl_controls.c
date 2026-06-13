/*******************************************************************************
* Copyright (c) 2022, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L8 Kernel Driver and is dual licensed,
* either 'STMicroelectronics Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L8 Kernel Driver may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
*******************************************************************************/

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/string.h>
#include "vl53l8_k_ioctl_controls.h"
#include "vl53l8_k_driver_config.h"
#include "vl53l8_k_logging.h"
#include "vl53l8_k_error_codes.h"
#include "vl53l8_k_range_wait_handler.h"
#include "vl53l8_k_version.h"
#include "vl53l8_k_user_data.h"
#include "vl53l5_platform.h"
#include "vl53l5_api_ranging.h"
#include "vl53l5_api_power.h"
#include "vl53l5_api_core.h"
#include "vl53l5_api_range_decode.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_api_calibration_decode.h"
#include "vl53l5_platform_algo_limits.h"
#include "vl53l5_platform_algo_macros.h"
#include "vl53l5_patch_api_core.h"
#include "vl53l8_k_glare_filter.h"

#ifdef VL53L5_TCDM_ENABLE
#include "vl53l5_tcdm_dump.h"
#endif


#ifdef VL53L8_INTERRUPT
#include "vl53l8_k_interrupt.h"
#endif


#ifdef SS_SUPPORT_SEC_CODE
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>
#define MAX_FAIL_COUNT 60 // 33 * 60 = 2980 msec
#define FAC_CAL		1
#define USER_CAL	2
#endif // SS_SUPPORT_SEC_CODE

#define VL53L8_ASZ_0_BH 0xdfa00081U
#define VL53L8_ASZ_1_BH 0xdfe40081U
#define VL53L8_ASZ_2_BH 0xe0280081U
#define VL53L8_ASZ_3_BH 0xe06c0081U

#ifdef VL53L8_RAD2PERP_CAL

#define VL53L8_K_R2P_DEF_LENS_UM \
	((uint32_t) 293801U)

#define VL53L8_K_R2P_SPAD_PITCH_UM \
	((uint32_t) 7895U)
#define VL53L8_K_R2P_GRID_8X8_DATA_SCALE_FACTOR_OFF \
	((uint32_t)176U)
#define VL53L8_K_R2P_GRID_4X4_DATA_SCALE_FACTOR_OFF \
	((uint32_t)28U)
#endif

int p2p_bh[] = VL53L8_K_P2P_CAL_BLOCK_HEADERS;
int shape_bh[] = VL53L8_K_SHAPE_CAL_BLOCK_HEADERS;

#ifdef VL53L8_FORCE_ERROR_COMMAND
#define VL53L8_FW_ERROR_HANDLER_DEBUG_BH ((unsigned int) 0xd2700040U)

#define ERR_DBG__RANGING_CTRL__RANGING_TMOUT ((unsigned int) 34U)
#endif

#ifdef VL53L8_RAD2PERP_CAL
static int _perform_rad2perp_calibration(struct vl53l8_k_module_t *p_module);
#endif

static int vl53l8_k_assign_asz(struct vl53l8_k_asz_t *p_asz,
			       struct vl53l8_k_asz_tuning_t *p_tuning,
			       uint8_t *p_buffer, uint32_t *p_count);

static void _copy_buffer(uint8_t *pcomms_buff,
				uint8_t *pconfig_buff,
				uint32_t buff_size,
				uint32_t *p_count)
{
	memcpy(&pcomms_buff[*p_count], pconfig_buff, buff_size);
	*p_count += buff_size;
}

static int _check_state(struct vl53l8_k_module_t *p_module,
		 enum vl53l8_k_state_preset expected_state)
{
	enum vl53l8_k_state_preset current_state;
	int is_state = VL53L5_ERROR_NONE;

	current_state = p_module->state_preset;

	vl53l8_k_log_debug("current state: %i expected state: %i",
				current_state, expected_state);
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (current_state != expected_state)
		is_state = VL53L8_K_ERROR_DEVICE_STATE_INVALID;
#else // SS_SUPPORT_SEC_CODE
	if (current_state < expected_state) {
		vl53l8_k_log_error("current %i expected %i\n",
				current_state, expected_state);
		is_state = VL53L8_K_ERROR_DEVICE_STATE_INVALID;
	}
#endif // SS_SUPPORT_SEC_CODE

	return is_state;
}

static int _poll_for_new_data(struct vl53l8_k_module_t *p_module,
			      int sleep_time_ms,
			      int timeout_ms)
{
	int status = VL53L5_ERROR_NONE;
	int data_ready = 0;
	int timeout_occurred = 0;

	LOG_FUNCTION_START("");

	p_module->polling_count = 0;
	status = vl53l5_get_tick_count(
		&p_module->stdev, &p_module->polling_start_time_ms);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	do {

		status = vl53l8_k_check_data_ready(p_module, &data_ready);
		if ((data_ready) || (status != VL53L5_ERROR_NONE))
			goto out;

		status = vl53l8_k_check_for_timeout(p_module,
						    timeout_ms,
						    &timeout_occurred);
		if (status != VL53L5_ERROR_NONE)
			goto out;

		if (timeout_occurred)
			goto out;

		status = vl53l5_wait_ms(
			&p_module->stdev, p_module->polling_sleep_time_ms);
		if (status != VL53L5_ERROR_NONE)
			goto out;

	} while (1);

out:

	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d\n", status);
	}

	if (status == VL53L5_ERROR_TIME_OUT)
		status = VL53L8_K_ERROR_RANGE_POLLING_TIMEOUT;

	LOG_FUNCTION_END(status);
	return status;
}

static int _set_device_params(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;
	const char *fw_path;
	struct spi_device *spi;
	const struct vl53l8_fw_header_t *header;
	unsigned char *fw_data;
	const struct firmware *fw_entry;
	unsigned int count = 0;
#ifdef VL53L8_FORCE_ERROR_COMMAND
	unsigned int force_error_block[] = {
			VL53L5_MAP_VERSION_BH,
			((MAP_VERSION_MINOR << 16) | MAP_VERSION_MAJOR),
			VL53L5_FW_ERROR_HANDLER_DEBUG_BH,
			ERR_DBG__RANGING_CTRL__RANGING_TMOUT};
#endif // VL53L8_FORCE_ERROR_COMMAND

	LOG_FUNCTION_START("");

	spi = p_module->spi_info.device;
	fw_path = p_module->firmware_name;

	vl53l8_k_log_debug("Req FW : %s", fw_path);
	status = request_firmware(&fw_entry, fw_path, &spi->dev);
	if (status) {
		vl53l8_k_log_error("FW %s not available\n", fw_path);
		goto out;
	}

	fw_data = (unsigned char *)fw_entry->data;
	header = (struct vl53l8_fw_header_t *)fw_data;

	vl53l8_k_log_debug("Bin config ver %i.%i",
			header->config_ver_major, header->config_ver_minor);

	switch (p_module->config_preset) {
	case VL53L8_CAL__NULL_XTALK_SHAPE:
		vl53l8_k_log_info(
			"NULL_XTALK_SHAPE offset: 0x%08x size: 0x%08x\n",
			header->buf00_offset,
			header->buf00_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf00_offset],
			     header->buf00_size,
			     &count);
	break;
	case VL53L8_CAL__GENERIC_XTALK_SHAPE:
		vl53l8_k_log_info(
			"GENERIC_XTALK_SHAPE offset: 0x%08x size: 0x%08x\n",
			header->buf01_offset,
			header->buf01_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf01_offset],
			     header->buf01_size,
			     &count);
	break;
	case VL53L8_CAL__XTALK_GRID_SHAPE_MON:
		vl53l8_k_log_info(
			"XTALK_GRID_SHAPE_MON offset: 0x%08x size: 0x%08x",
			header->buf02_offset,
			header->buf02_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf02_offset],
			     header->buf02_size,
			     &count);
	break;
	case VL53L8_CAL__DYN_XTALK_MONITOR:
		vl53l8_k_log_info(
			"DYN_XTALK_MONITOR offset: 0x%08x size: 0x%08x",
			header->buf03_offset,
			header->buf03_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf03_offset],
			     header->buf03_size,
			     &count);
	break;
	case VL53L8_CFG__STATIC__SS_0PC:
		vl53l8_k_log_info(
			"STATIC__SS_0PC offset: 0x%08x size: 0x%08x",
			header->buf04_offset,
			header->buf04_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf04_offset],
			     header->buf04_size,
			     &count);
	break;
	case VL53L8_CFG__STATIC__SS_1PC:
		vl53l8_k_log_info(
			"STATIC__SS_1PC offset: 0x%08x size: 0x%08x",
			header->buf05_offset,
			header->buf05_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf05_offset],
			     header->buf05_size,
			     &count);
	break;
	case VL53L8_CFG__STATIC__SS_2PC:
		vl53l8_k_log_info(
			"STATIC__SS_2PC offset: 0x%08x size: 0x%08x",
			header->buf06_offset,
			header->buf06_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf06_offset],
			     header->buf06_size,
			     &count);
	break;
	case VL53L8_CFG__STATIC__SS_4PC:
		vl53l8_k_log_info(
			"STATIC__SS_4PC offset: 0x%08x size: 0x%08x",
			header->buf07_offset,
			header->buf07_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf07_offset],
			     header->buf07_size,
			     &count);
	break;
	case VL53L8_CFG__XTALK_GEN1_8X8_1000:
		vl53l8_k_log_info(
			"XTALK_GEN1_8X8_1000 offset: 0x%08x size: 0x%08x",
			header->buf08_offset,
			header->buf08_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf08_offset],
			     header->buf08_size,
			     &count);
	break;
	case VL53L8_CFG__XTALK_GEN2_8X8_300:
		vl53l8_k_log_info(
			"XTALK_GEN2_8X8_300 offset: 0x%08x size: 0x%08x",
			header->buf09_offset,
			header->buf09_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf09_offset],
			     header->buf09_size,
			     &count);
	break;
	case VL53L8_CFG__B2BWB_8X8_OPTION_0:
		vl53l8_k_log_info(
			"B2BWB_8X8_OPTION_0 offset: 0x%08x size: 0x%08x",
			header->buf10_offset,
			header->buf10_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf10_offset],
			     header->buf10_size,
			     &count);
	break;
	case VL53L8_CFG__B2BWB_8X8_OPTION_1:
		vl53l8_k_log_info(
			"B2BWB_8X8_OPTION_1 offset: 0x%08x size: 0x%08x",
			header->buf11_offset,
			header->buf11_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf11_offset],
			     header->buf11_size,
			     &count);
	break;
	case VL53L8_CFG__B2BWB_8X8_OPTION_2:
		vl53l8_k_log_info(
			"B2BWB_8X8_OPTION_2 offset: 0x%08x size: 0x%08x",
			header->buf12_offset,
			header->buf12_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf12_offset],
			     header->buf12_size,
			     &count);
	break;
	case VL53L8_CFG__B2BWB_8X8_OPTION_3:
		vl53l8_k_log_info(
			"B2BWB_8X8_OPTION_3 offset: 0x%08x size: 0x%08x",
			header->buf13_offset,
			header->buf13_size);
		_copy_buffer(p_module->stdev.host_dev.p_comms_buff,
			     &fw_data[header->buf13_offset],
			     header->buf13_size,
			     &count);
	break;
#ifdef VL53L8_FORCE_ERROR_COMMAND
	case VL53L8_CFG__FORCE_ERROR:
		vl53l8_k_log_info("VL53L8_CFG__FORCE_ERROR");

		status = vl53l5_set_device_parameters(&p_module->stdev,
				(unsigned char *)force_error_block,
				(unsigned int)sizeof(force_error_block));
		break;
#endif
	default:
		vl53l8_k_log_error("Invalid preset: %i\n",
				   p_module->config_preset);
		status = VL53L8_K_ERROR_INVALID_CONFIG_PRESET;
	break;
	};

	if (status != VL53L5_ERROR_NONE)
		goto out_release;

	status = vl53l5_set_device_parameters(
			&p_module->stdev,
			p_module->stdev.host_dev.p_comms_buff,
			count);

out_release:
	vl53l8_k_log_debug("Release FW");
	release_firmware(fw_entry);
out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d\n", status);
	}

	LOG_FUNCTION_END(status);
	return status;
}

static int _start_poll_stop(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Calibration vl53l5_start");
	status = vl53l5_start(&p_module->stdev, NULL);
	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("start failed\n");
		goto out;
	}

	vl53l8_k_log_debug("Poll data");
	status = _poll_for_new_data(
		p_module, p_module->polling_sleep_time_ms,
		VL53L8_K_MAX_CALIBRATION_POLL_TIME_MS);
	if (status != VL53L5_ERROR_NONE)
		goto out_stop;

	status = vl53l5_stop(&p_module->stdev, NULL);
	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("stop failed\n");
		goto out;
	}

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d\n", status);
	}
	LOG_FUNCTION_END(status);
	return status;

out_stop:
	(void)vl53l5_stop(&p_module->stdev, NULL);
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d\n", status);
	}
	LOG_FUNCTION_END(status);
	return status;
}

static void _encode_device_info(struct calibration_data_t *pcal, char *buffer)
{
	char *p_buff = buffer;
	int i = 0;

	vl53l5_encode_uint16_t(pcal->version.driver.ver_major, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(pcal->version.driver.ver_minor, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(pcal->version.driver.ver_build, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
			pcal->version.driver.ver_revision, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
			pcal->version.firmware.ver_major, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
			pcal->version.firmware.ver_minor, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
			pcal->version.firmware.ver_build, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
			pcal->version.firmware.ver_revision, BYTE_2, p_buff);
	p_buff += BYTE_2;

	vl53l5_encode_uint16_t(
		pcal->patch_version.patch_version.ver_major, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
		pcal->patch_version.patch_version.ver_minor, BYTE_2, p_buff);
	p_buff += BYTE_2;

	vl53l5_encode_uint16_t(
		pcal->patch_version.tcpm_version.ver_major, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
		pcal->patch_version.tcpm_version.ver_minor, BYTE_2, p_buff);
	p_buff += BYTE_2;

	vl53l5_encode_uint16_t(
		pcal->patch_version.patch_driver.ver_major, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
		pcal->patch_version.patch_driver.ver_minor, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
		pcal->patch_version.patch_driver.ver_build, BYTE_2, p_buff);
	p_buff += BYTE_2;
	vl53l5_encode_uint16_t(
		pcal->patch_version.patch_driver.ver_revision, BYTE_2, p_buff);
	p_buff += BYTE_2;

	while (i < VL53L5_FGC_STRING_LENGTH) {
		vl53l5_encode_uint8_t(
			pcal->info.fgc[i / BYTE_1],
			BYTE_1,
			p_buff);
		p_buff += BYTE_1;
		i += BYTE_1;
	}
	vl53l5_encode_uint32_t(pcal->info.module_id_lo, BYTE_4, p_buff);
	p_buff += BYTE_4;
	vl53l5_encode_uint32_t(pcal->info.module_id_hi, BYTE_4, p_buff);

}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
static int _write_p2p_calibration(struct vl53l8_k_module_t *p_module)
#else // SS_SUPPORT_SEC_CODE
static int _write_p2p_calibration(struct vl53l8_k_module_t *p_module, int flow)
#endif // SS_SUPPORT_SEC_CODE

{
	int status = VL53L5_ERROR_NONE;
	int num_bh = VL53L8_K_P2P_BH_SZ;
	int *p_buffer[] = {&p2p_bh[0]};
	char buffer[VL53L8_K_DEVICE_INFO_SZ] = {0};

	LOG_FUNCTION_START("");

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	status = vl53l5_get_version(&p_module->stdev,
				    &p_module->calibration.version);

	if (status != VL53L5_ERROR_NONE)
		goto out;

	status = vl53l5_get_patch_version(&p_module->stdev,
					  &p_module->calibration.patch_version);

	if (status != VL53L5_ERROR_NONE)
		goto out;

	status = vl53l5_get_module_info(&p_module->stdev,
					&p_module->calibration.info);

	if (status != VL53L5_ERROR_NONE)
		goto out;

	status = vl53l5_get_device_parameters(&p_module->stdev,
					      p_buffer[0],
					      num_bh);

	if (status != VL53L5_ERROR_NONE)
		goto out;

	_encode_device_info(&p_module->calibration, buffer);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("filename %s", VL53L8_CAL_P2P_FILENAME);
	vl53l8_k_log_debug("file size %i", VL53L8_K_P2P_FILE_SIZE + 4);
	status = vl53l5_write_file(&p_module->stdev,
				   p_module->stdev.host_dev.p_comms_buff,
				   VL53L8_K_P2P_FILE_SIZE + 4,
				   VL53L8_CAL_P2P_FILENAME,
				   buffer,
				   VL53L8_K_DEVICE_INFO_SZ);

	if (status < VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("write file %s failed: %d ",
				   VL53L8_CAL_P2P_FILENAME, status);
		goto out;
	}
#else // SS_SUPPORT_SEC_CODE
	mutex_lock(&p_module->cal_mutex);
	memset(&p_module->cal_data, 0, sizeof(struct vl53l8_cal_data_t));
	memcpy(p_module->cal_data.pcal_data, buffer, VL53L8_K_DEVICE_INFO_SZ);
	memcpy(p_module->cal_data.pcal_data + VL53L8_K_DEVICE_INFO_SZ,
			&p_module->stdev.host_dev.p_comms_buff[4],
			VL53L8_K_P2P_FILE_SIZE + 4);

	p_module->cal_data.size = VL53L8_K_DEVICE_INFO_SZ + VL53L8_K_P2P_FILE_SIZE + 4;

	if (flow == 1)
		p_module->cal_data.cmd = CMD_WRITE_P2P_FILE;
	else
		p_module->cal_data.cmd = CMD_WRITE_CAL_FILE;

	mutex_unlock(&p_module->cal_mutex);

	vl53l8_k_log_info("cmd %d shape size %d\n", p_module->cal_data.cmd, p_module->cal_data.size);
	status = vl53l8_input_report(p_module, 2, p_module->cal_data.cmd);
#endif // SS_SUPPORT_SEC_CODE

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d\n", status);
	}
out_state:
	LOG_FUNCTION_END(status);
	return status;
}

int _write_shape_calibration(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;
	int num_bh = VL53L8_K_SHAPE_CAL_BLOCK_HEADERS_SZ;
	int *p_buffer[] = {&shape_bh[0]};

	LOG_FUNCTION_START("");

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	status = vl53l5_get_device_parameters(&p_module->stdev,
					      p_buffer[0],
					      num_bh);

	if (status != VL53L5_ERROR_NONE)
		goto out;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("filename %s", VL53L8_CAL_SHAPE_FILENAME);
	vl53l8_k_log_debug("file size %i", VL53L8_K_SHAPE_FILE_SIZE + 4);
	status = vl53l5_write_file(&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				VL53L8_K_SHAPE_FILE_SIZE + 4,
				VL53L8_CAL_SHAPE_FILENAME,
				NULL,
				0);

	if (status < VL53L5_ERROR_NONE) {
		vl53l8_k_log_error(
			"write file %s failed: %d ",
				VL53L8_CAL_SHAPE_FILENAME, status);
		goto out;
	}
#else // SS_SUPPORT_SEC_CODE
	mutex_lock(&p_module->cal_mutex);
	memset(&p_module->cal_data, 0, sizeof(struct vl53l8_cal_data_t));
	memcpy(p_module->cal_data.pcal_data,
			&p_module->stdev.host_dev.p_comms_buff[4],
			VL53L8_K_SHAPE_FILE_SIZE + 4);

	p_module->cal_data.size = VL53L8_K_SHAPE_FILE_SIZE + 4;
	p_module->cal_data.cmd = CMD_WRITE_SHAPE_FILE;
	mutex_unlock(&p_module->cal_mutex);

	vl53l8_k_log_info("shape size %d\n", p_module->cal_data.size);
	status = vl53l8_input_report(p_module, 2, p_module->cal_data.cmd);
#endif // SS_SUPPORT_SEC_CODE

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d\n", status);
	}
out_state:
	LOG_FUNCTION_END(status);
	return status;
}

#ifdef VL53L5_TCDM_ENABLE
static void _tcdm_dump(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;
	char *p_data;
	int count = 0;
	int buff_size = 100000 * sizeof(char);

	LOG_FUNCTION_START("");

	p_data = kzalloc(buff_size, GFP_KERNEL);
	if (p_data == NULL) {
		vl53l8_k_log_error("Allocate failed");
		goto out;
	}

	status = vl53l5_tcdm_dump(&p_module->stdev, p_data, &count);

	if (status < VL53L5_ERROR_NONE)
		goto out_free;

	status = vl53l5_write_file(&p_module->stdev,
				p_data,
				count,
				VL53L8_TCDM_FILENAME,
				NULL,
				0);

	if (status < VL53L5_ERROR_NONE) {
		vl53l8_k_log_error(
				"write file %s failed: %d ",
				VL53L8_TCDM_FILENAME, status);
		goto out_free;
	}
out_free:
	kfree(p_data);
out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("failed %i", status);
	}
	LOG_FUNCTION_END(status);
}
#endif

static void _decode_device_info(struct calibration_data_t *pcal,
				char *p_comms_buff)
{
	int i = 0;
	char *p_buff = p_comms_buff;

	pcal->version.driver.ver_major = vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->version.driver.ver_minor = vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->version.driver.ver_build = vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->version.driver.ver_revision =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	pcal->version.firmware.ver_major =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->version.firmware.ver_minor =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->version.firmware.ver_build =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->version.firmware.ver_revision =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	pcal->patch_version.patch_version.ver_major =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->patch_version.patch_version.ver_minor =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	pcal->patch_version.tcpm_version.ver_major =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->patch_version.tcpm_version.ver_minor =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	pcal->patch_version.patch_driver.ver_major =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->patch_version.patch_driver.ver_minor =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->patch_version.patch_driver.ver_build =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	pcal->patch_version.patch_driver.ver_revision =
				vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	while (i < VL53L5_FGC_STRING_LENGTH) {
		pcal->info.fgc[i] = vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		i++;
	}

	pcal->info.module_id_lo = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	pcal->info.module_id_hi = vl53l5_decode_uint32_t(BYTE_4, p_buff);

	vl53l8_k_log_debug(
		"API Ver: %d.%d.%d.%d",
		pcal->version.driver.ver_major,
		pcal->version.driver.ver_minor,
		pcal->version.driver.ver_build,
		pcal->version.driver.ver_revision);
	vl53l8_k_log_debug(
		"FW Ver : %d.%d.%d.%d",
		pcal->version.firmware.ver_major,
		pcal->version.firmware.ver_minor,
		pcal->version.firmware.ver_build,
		pcal->version.firmware.ver_revision);
	vl53l8_k_log_debug(
		"Patch API Ver: %d.%d.%d.%d",
		pcal->patch_version.patch_driver.ver_major,
		pcal->patch_version.patch_driver.ver_minor,
		pcal->patch_version.patch_driver.ver_build,
		pcal->patch_version.patch_driver.ver_revision);
	vl53l8_k_log_debug(
		"Patch Ver: %d.%d",
		pcal->patch_version.patch_version.ver_major,
		pcal->patch_version.patch_version.ver_minor);
	vl53l8_k_log_debug(
		"Patch TCPM Ver: %d.%d",
		pcal->patch_version.tcpm_version.ver_major,
		pcal->patch_version.tcpm_version.ver_minor);

	vl53l8_k_log_debug("fgc %s", (char *)pcal->info.fgc);

	vl53l8_k_log_debug("module hi %u lo %u", pcal->info.module_id_hi,
			   pcal->info.module_id_lo);
}

int vl53l8_ioctl_init(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;
	int _comms_buff_count = sizeof(p_module->comms_buffer);
	const char *fw_path;
	struct spi_device *spi;
	const struct vl53l8_fw_header_t *header;
	unsigned char *fw_data;
	const struct firmware *fw_entry;

	LOG_FUNCTION_START("");

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l58_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L8_STATE_PRESENT);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;
#else // SS_SUPPORT_SEC_CODE
	p_module->stdev.status_probe = -1;
	if (p_module->last_driver_error == VL53L8_PROBE_FAILED)
		return VL53L8_PROBE_FAILED;

	memset(&p_module->stdev, 0, sizeof(struct vl53l5_dev_handle_t));
	memset(&p_module->comms_buffer, 0, VL53L5_COMMS_BUFFER_SIZE_BYTES);
#endif // SS_SUPPORT_SEC_CODE

	if (!p_module->firmware_name) {
		vl53l8_k_log_error("FW name not in dts");
#ifdef SS_SUPPORT_SEC_CODE
		p_module->stdev.status_probe = -2;
#endif // SS_SUPPORT_SEC_CODE
		goto out;
	}

	spi = p_module->spi_info.device;
	fw_path = p_module->firmware_name;
	vl53l8_k_log_debug("Load FW : %s", fw_path);

	vl53l8_k_log_debug("Req FW");
	status = request_firmware(&fw_entry, fw_path, &spi->dev);
	if (status) {
#ifdef SS_SUPPORT_SEC_CODE
		p_module->stdev.status_probe = -3;
#endif // SS_SUPPORT_SEC_CODE
		vl53l8_k_log_error("FW %s not available\n", fw_path);
		goto out;
	}

	fw_data = (unsigned char *)fw_entry->data;
	header = (struct vl53l8_fw_header_t *)fw_data;

	vl53l8_k_log_info("Bin FW ver %i.%i\n",
				header->fw_ver_major, header->fw_ver_minor);

	vl53l8_k_log_info("Bin config ver %i.%i\n",
			header->config_ver_major, header->config_ver_minor);

	p_module->bin_version.fw_ver_major = header->fw_ver_major;
	p_module->bin_version.fw_ver_minor = header->fw_ver_minor;
	p_module->bin_version.config_ver_major = header->config_ver_major;
	p_module->bin_version.config_ver_minor = header->config_ver_minor;
	p_module->stdev.host_dev.p_fw_buff = &fw_data[header->fw_offset];
	p_module->stdev.host_dev.fw_buff_count = header->fw_size;

	p_module->last_driver_error = 0;

	VL53L5_ASSIGN_COMMS_BUFF(&p_module->stdev, p_module->comms_buffer,
				 _comms_buff_count);

#ifdef SS_SUPPORT_SEC_CODE
#ifndef CONFIG_SEPARATE_IO_CORE_POWER
	vl53l8_power_onoff(p_module, false);
	usleep_range(2000, 2100);

	vl53l8_power_onoff(p_module, true);
	usleep_range(5000, 5100);
#endif
#endif // SS_SUPPORT_SEC_CODE
	status = vl53l5_init(&p_module->stdev);

	if (status != VL53L5_ERROR_NONE)
		goto out_powerdown;

	vl53l8_k_log_debug("Release FW");
	release_firmware(fw_entry);

	p_module->stdev.host_dev.p_fw_buff = NULL;
	p_module->stdev.host_dev.fw_buff_count = 0;

	status = vl53l5_get_patch_version(&p_module->stdev, &p_module->patch_ver);
	if (status != VL53L5_ERROR_NONE)
		goto out;

#ifdef VL53L8_INTERRUPT
	if (p_module->irq_val == 0) {
		status = vl53l8_k_start_interrupt(p_module);
		if (status != VL53L5_ERROR_NONE) {
			vl53l8_k_log_error("Failed to start interrupt: %d", status);
			goto out;
		}
		disable_irq(p_module->irq_val);
		p_module->irq_is_active = false;
		vl53l8_k_log_info("disable irq");
	}
#endif
	p_module->state_preset = VL53L8_STATE_INITIALISED;
#ifdef SS_SUPPORT_SEC_CODE
	vl53l8_ioctl_get_module_info(p_module, NULL);

	p_module->stdev.last_dev_error = VL53L5_ERROR_NONE;
	p_module->stdev.status_probe = STATUS_OK;
#endif // SS_SUPPORT_SEC_CODE
	p_module->last_driver_error = STATUS_OK;

out:
	if (status != VL53L5_ERROR_NONE) {
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, status);
#endif
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
out_state:
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	LOG_FUNCTION_END(status);
	return status;
out_powerdown:
	vl53l8_k_log_debug("Release FW");
	release_firmware(fw_entry);

	if (status != VL53L5_ERROR_NONE) {
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, status);
#endif // CONFIG_SENSORS_LAF_FAILURE_DEBUG
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	(void)vl53l8_ioctl_term(p_module);
	vl53l8_power_onoff(p_module, false);
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_ioctl_term(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	status = _check_state(p_module, VL53L8_STATE_RANGING);
	if (status != VL53L5_ERROR_NONE) {
		status = _check_state(p_module, VL53L8_STATE_INITIALISED);
		if (status != VL53L5_ERROR_NONE) {
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
			goto out_state;
#else // SS_SUPPORT_SEC_CODE
			status = _check_state(p_module, VL53L8_STATE_LOW_POWER);
			if (status != VL53L5_ERROR_NONE)
				goto out_state;
#endif // SS_SUPPORT_SEC_CODE
		}
	} else {
		status = vl53l5_stop(&p_module->stdev,
				     &p_module->ranging_flags);
		if (status != VL53L5_ERROR_NONE)
			goto out;
	}
#ifdef VL53L8_INTERRUPT
	disable_irq(p_module->gpio.interrupt);
	status = vl53l8_k_stop_interrupt(p_module);
	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_error(
			"Failed to stop interrupt: %d", status);
		goto out;
	}
#endif

	status = vl53l5_term(&p_module->stdev);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	p_module->state_preset = VL53L8_STATE_PRESENT;

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_ioctl_get_version(struct vl53l8_k_module_t *p_module, void __user *p)
{
	int status = VL53L5_ERROR_NONE;
	struct vl53l8_k_version_t *p_version = NULL;

	LOG_FUNCTION_START("");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	p_version = kzalloc(sizeof(struct vl53l8_k_version_t), GFP_KERNEL);
	if (p_version == NULL) {
		vl53l8_k_log_error("Allocate Failed");
		status = VL53L8_K_ERROR_FAILED_TO_ALLOCATE_VERSION;
		goto out;
	}

	status = vl53l5_get_version(&p_module->stdev, &p_version->driver);
	if (status != VL53L5_ERROR_NONE)
		goto out_free;

	status = vl53l5_get_patch_version(&p_module->stdev, &p_version->patch);
	if (status != VL53L5_ERROR_NONE)
		goto out_free;

	p_version->kernel.ver_major = VL53L8_K_VER_MAJOR;
	p_version->kernel.ver_minor = VL53L8_K_VER_MINOR;
	p_version->kernel.ver_build = VL53L8_K_VER_BUILD;
	p_version->kernel.ver_revision = VL53L8_K_VER_REVISION;

	memcpy(&p_version->bin_version, &p_module->bin_version, sizeof(struct vl53l8_k_bin_version));

	vl53l8_k_log_info(
		"Driver Ver: %d.%d.%d.%d",
		VL53L8_K_VER_MAJOR,
		VL53L8_K_VER_MINOR,
		VL53L8_K_VER_BUILD,
		VL53L8_K_VER_REVISION);
	vl53l8_k_log_info(
		"API Ver: %d.%d.%d.%d",
		p_version->driver.driver.ver_major,
		p_version->driver.driver.ver_minor,
		p_version->driver.driver.ver_build,
		p_version->driver.driver.ver_revision);
	vl53l8_k_log_info(
		"FW Ver : %d.%d.%d.%d",
		p_version->driver.firmware.ver_major,
		p_version->driver.firmware.ver_minor,
		p_version->driver.firmware.ver_build,
		p_version->driver.firmware.ver_revision);
	vl53l8_k_log_info(
		"Patch API Ver: %d.%d.%d.%d",
		p_version->patch.patch_driver.ver_major,
		p_version->patch.patch_driver.ver_minor,
		p_version->patch.patch_driver.ver_build,
		p_version->patch.patch_driver.ver_revision);
	vl53l8_k_log_info(
		"Patch Ver: %d.%d",
		p_version->patch.patch_version.ver_major,
		p_version->patch.patch_version.ver_minor);
	vl53l8_k_log_info(
		"TCPM Ver: %d.%d",
		p_version->patch.tcpm_version.ver_major,
		p_version->patch.tcpm_version.ver_minor);
	
	vl53l8_k_log_info(
		"BIN Config Ver: %d.%d",
			p_version->bin_version.config_ver_major,
			p_version->bin_version.config_ver_minor);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
		status = copy_to_user(p, p_version, sizeof(struct vl53l8_k_version_t));
#else // SS_SUPPORT_SEC_CODE
		if (p != NULL)
			status = copy_to_user(p, p_version, sizeof(struct vl53l8_k_version_t));
		else
			status = STATUS_OK;
#endif // SS_SUPPORT_SEC_CODE

	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_VERSION;
		goto out;
	}
out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_free:
	kfree(p_version);
out_state:
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_ioctl_get_module_info(struct vl53l8_k_module_t *p_module,
				 void __user *p)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	status = vl53l5_get_module_info(&p_module->stdev, &p_module->m_info);
	if (status != VL53L5_ERROR_NONE)
		goto out;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_info("FGC: %s", (char *)p_module->m_info.fgc);
	vl53l8_k_log_info("ID : %x.%x",
			  p_module->m_info.module_id_hi,
			  p_module->m_info.module_id_lo);
	status = copy_to_user(p, &p_module->m_info,
				sizeof(struct vl53l5_module_info_t));
#else // SS_SUPPORT_SEC_CODE
	vl53l8_k_log_info(" %x.%x\n",
			  p_module->m_info.module_id_hi,
			  p_module->m_info.module_id_lo);
		if (p != NULL)
			status = copy_to_user(p, &p_module->m_info, sizeof(struct vl53l5_module_info_t));
		else
			status = STATUS_OK;
#endif // SS_SUPPORT_SEC_CODE

	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_MODULE_INFO;
		goto out;
	}

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d\n", status);
	}

out_state:
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	LOG_FUNCTION_END(status);
	return status;
}

#ifdef SS_SUPPORT_SEC_CODE
void vl53l8_load_calibration(struct vl53l8_k_module_t *p_module)
{
	int ret;
	int status = 0;
	int cal_type = 0;

	status = vl53l8_input_report(p_module, 6, CMD_CHECK_CAL_FILE_TYPE);
	if (status < 0)
		vl53l8_k_log_error("could not find file_list");

	if ((p_module->file_list & 3) == 3) {
		vl53l8_k_log_info("Do user cal: %d", p_module->file_list);
		cal_type = USER_CAL;
	} else {
		vl53l8_k_log_info("Do fac cal: %d", p_module->file_list);
		cal_type = FAC_CAL;
	}

	if (cal_type != USER_CAL)
		ret = vl53l8_load_factory_calibration(p_module);
	else
		ret = vl53l8_load_open_calibration(p_module);

	if (ret < 0) {
		vl53l8_k_log_error("Cal data load fail");
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, VL53L8_CALFILE_LOAD_ERROR);
#endif
	}
}

int vl53l8_load_open_calibration(struct vl53l8_k_module_t *p_module)
{
	int p2p, cha;

	vl53l8_k_log_info("Open CAL Load\n");

	p_module->stdev.status_cal = 0;
	p_module->load_calibration = false;
	p_module->read_p2p_cal_data = false;

	vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);
	usleep_range(2000, 2100);

	cha = vl53l8_ioctl_read_open_cal_shape_calibration(p_module);
	msleep(100);

	p2p = vl53l8_ioctl_read_open_cal_p2p_calibration(p_module);
	usleep_range(2000, 2100);

	vl53l8_k_log_error("cha %d, p2p %d\n", cha, p2p);

	if (cha == STATUS_OK)
		p_module->load_calibration = true;
	if (p2p == STATUS_OK)
		p_module->read_p2p_cal_data = true;

	msleep(20);

	if (cha != STATUS_OK)
		return cha;
	else if (p2p != STATUS_OK)
		return p2p;
	else
		return STATUS_OK;
}

int vl53l8_load_factory_calibration(struct vl53l8_k_module_t *p_module)
{
	int cha, p2p;

	vl53l8_k_log_info("Load CAL\n");

	p_module->stdev.status_cal = 0;
	p_module->load_calibration = false;
	p_module->read_p2p_cal_data = false;

	vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);

	usleep_range(2000, 2100);

	cha = vl53l8_ioctl_read_generic_shape(p_module);
	usleep_range(2000, 2100);

	p2p = vl53l8_ioctl_read_p2p_calibration(p_module);
	usleep_range(2000, 2100);

	vl53l8_k_log_error("cha %d, p2p %d\n", cha, p2p);

	if (cha != STATUS_OK)
		return cha;

	p_module->load_calibration = true;

	if (p2p != STATUS_OK)
		return p2p;

	p_module->read_p2p_cal_data = true;

	return STATUS_OK;
}

#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
void vl53l8_check_ldo_onoff(struct vl53l8_k_module_t *data)
{
	int ldo_status = 0, comp = ALL_VDD_ENABLED;
	int reg_enabled = 0;

	if (data->avdd_vreg) {
		reg_enabled = regulator_is_enabled(data->avdd_vreg);
		vl53l8_k_log_info("avdd reg_enabled=%d\n", reg_enabled);
		ldo_status |= (reg_enabled & 0x01) << AVDD;
	} else
		vl53l8_k_log_error("avdd error\n");

	if (data->iovdd_vreg) {
		reg_enabled = regulator_is_enabled(data->iovdd_vreg);
		vl53l8_k_log_info("iovdd reg_enabled=%d\n", reg_enabled);
		ldo_status |= (reg_enabled & 0x01) << IOVDD;
	} else
		vl53l8_k_log_error("vdd_vreg err\n");

#ifdef CONFIG_SEPARATE_IO_CORE_POWER
	if (data->corevdd_vreg) {
		reg_enabled = regulator_is_enabled(data->corevdd_vreg);
		vl53l8_k_log_info("corevdd reg_enabled=%d\n", reg_enabled);
		ldo_status |= (reg_enabled & 0x01) << COREVDD;
	} else
		vl53l8_k_log_error("vdd_vreg err\n");
#endif // CONFIG_SEPARATE_IO_CORE_POWER
	data->ldo_status = comp ^ ldo_status;
	vl53l8_last_error_counter(data, data->ldo_status);
	data->ldo_status = 0;

	return;
}
#endif // CONFIG_SENSORS_LAF_FAILURE_DEBUG
int vl53l8_ioctl_start(struct vl53l8_k_module_t *p_module)
#else // SS_SUPPORT_SEC_CODE
int vl53l8_ioctl_start(struct vl53l8_k_module_t *p_module, void __user *p)
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
{
	int status = VL53L5_ERROR_NONE;

	struct vl53l8_k_asz_tuning_t asz_tuning;

	LOG_FUNCTION_START("");

	if (p_module->state_preset == VL53L8_STATE_RANGING)
		return status;
	if (p_module->stdev.status_probe < 0)
		return VL53L5_DEVICE_STATE_INVALID;
#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
	cancel_work_sync(&p_module->resume_work);
#endif
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#else // SS_SUPPORT_SEC_CODE
#ifdef VL53L8_INTERRUPT
	if (!p_module->irq_is_active) {
		enable_irq(p_module->irq_val);
		vl53l8_k_log_info("enable_irq");
	}
#endif
	memset(&p_module->range.data.tcpm_0_patch.d16_per_target_data, 1, sizeof(struct vl53l5_d16_per_tgt_results_t));

	vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);

	status = vl53l8_set_device_parameters(p_module, VL53L8_CFG__B2BWB_8X8_OPTION_3);
	if (status != STATUS_OK) {
		vl53l8_k_log_error("set param err");
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, status);
#endif
	}
	status = vl53l8_ioctl_set_integration_time_us(p_module, p_module->integration);
	if (status != STATUS_OK)
		vl53l8_k_log_error("set integ time err");

	status = vl53l8_ioctl_set_ranging_rate(p_module, p_module->rate);
	if (status != STATUS_OK)
		vl53l8_k_log_error("set raging rate err");

	asz_tuning.asz_0_ll_zone_id = p_module->asz_0_ll_zone_id;
	asz_tuning.asz_1_ll_zone_id = p_module->asz_1_ll_zone_id;
	asz_tuning.asz_2_ll_zone_id = p_module->asz_2_ll_zone_id;
	asz_tuning.asz_3_ll_zone_id = p_module->asz_3_ll_zone_id;
	vl53l8_ioctl_set_asz_tuning(p_module, NULL, &asz_tuning);

	vl53l8_load_calibration(p_module);
#endif // SS_SUPPORT_SEC_CODE

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	vl53l8_k_log_debug("glare : patch version major =%d, minor =%d", p_module->patch_ver.patch_version.ver_major, p_module->patch_ver.patch_version.ver_minor);

	if (p_module->patch_ver.patch_version.ver_major < 6 &&
	    p_module->patch_ver.patch_version.ver_minor >= 0) {
		p_module->gf_tuning.range_min_clip = 20;
		p_module->gf_tuning.max_filter_range = 500;
		p_module->gf_tuning.remove_glare_targets = true;
		p_module->gf_tuning.range_x4[0] = 0;// 0 * 4;
		p_module->gf_tuning.range_x4[1] = 12;// 3 * 4;
		p_module->gf_tuning.range_x4[2] = 16;// 4 * 4;
		p_module->gf_tuning.range_x4[3] = 40;// 10 * 4;
		p_module->gf_tuning.range_x4[4] = 44;// 11 * 4;
		p_module->gf_tuning.range_x4[5] = 400;// 100 * 4;
		p_module->gf_tuning.range_x4[6] = 1200;// 300 * 4;
		p_module->gf_tuning.range_x4[7] = 1204;// 301 * 4;
		p_module->gf_tuning.refl_thresh_x256[0] = 0;
		p_module->gf_tuning.refl_thresh_x256[0] = 0;
		p_module->gf_tuning.refl_thresh_x256[0] = 16;
		p_module->gf_tuning.refl_thresh_x256[0] = 16;
		p_module->gf_tuning.refl_thresh_x256[0] = 512;// 2 * 256;
		p_module->gf_tuning.refl_thresh_x256[0] = 512;// 2 * 256;
		p_module->gf_tuning.refl_thresh_x256[0] = 0;
		p_module->gf_tuning.refl_thresh_x256[0] = 0;

		vl53l8_k_log_debug("glare : Glare Init");
		vl53l8_k_glare_detect_init(&p_module->gf_tuning);
	}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (p != NULL) {
		status = copy_from_user(
			&p_module->ranging_flags, p,
			sizeof(struct vl53l5_ranging_mode_flags_t));
		if (status != VL53L5_ERROR_NONE) {
			status = VL53L8_K_ERROR_FAILED_TO_COPY_FLAGS;
			goto out;
		}
	}
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE

	status = vl53l5_start(&p_module->stdev, &p_module->ranging_flags);
	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_debug("vl53l5_start fail");
		goto out;
	}
	p_module->range.count = 0;
	p_module->range.is_valid = 0;
	p_module->polling_start_time_ms = 0;

	p_module->stuck.stuck_check = true;
	p_module->stuck.stuck_cnt = 0;

#ifdef VL53L8_INTERRUPT
	p_module->irq_is_active = true;
#endif

	p_module->state_preset = VL53L8_STATE_RANGING;

out:
	if (status != VL53L5_ERROR_NONE) {
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, status);
#endif
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
		vl53l8_ioctl_stop(p_module);
	}
out_state:
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	LOG_FUNCTION_END(status);
	return status;
}

#define FIRST_TARGET    0
#define SECOND_TARGET   1
#define UPPER_LEFT      0
#define UPPER_RIGHT     7
#define BOTTOM_LEFT     56
#define BOTTOM_RIGHT    63

#define ASZ_1 64
#define ASZ_2 65
#define ASZ_3 66
#define ASZ_4 67

#ifdef SS_SUPPORT_SEC_CODE
#ifdef SS_SUPPORT_DATA_ROTATION
void vl53l8_rotate_report_data_u16(u16 *raw_data, int rotation)
{
	u16 data[8][8];
	int i, j;

	if (rotation == 0)
		return;

	for (i = 0; i < 8 ; i++) {
		for (j = 0; j < 8 ; j++)
			data[i][j] = raw_data[8*j + i];
	}

	if (rotation == 90) { //Clockwise 90 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*i + 7-j] = data[i][j];
		}
	} else if (rotation == 180) { //Clockwise 180 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*(7-j) + 7-i] = data[i][j];
		}
	} else if (rotation == 270) { //Clockwise 270 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*(7-i) + j] = data[i][j];
		}
	}
}

void vl53l8_rotate_report_data_u32(u32 *raw_data, int rotation)
{
	u32 data[8][8];
	int i, j;

	if (rotation == 0)
		return;

	for (i = 0; i < 8 ; i++) {
		for (j = 0; j < 8 ; j++)
			data[i][j] = raw_data[8*j + i];
	}

	if (rotation == 90) { //Clockwise 90 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*i + 7-j] = data[i][j];
		}
	} else if (rotation == 180) { //Clockwise 180 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*(7-j) + 7-i] = data[i][j];
		}
	} else if (rotation == 270) { //Clockwise 270 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*(7-i) + j] = data[i][j];
		}
	}
}
#endif // SS_SUPPORT_DATA_ROTATION

void vl53l8_rotate_report_data_int32(int32_t *raw_data, int rotation)
{
	int32_t data[8][8];
	int i, j;

	if (rotation == 0)
		return;

	for (i = 0; i < 8 ; i++) {
		for (j = 0; j < 8 ; j++)
			data[i][j] = raw_data[8*j + i];
	}

	if (rotation == 90) { //Clockwise 90 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*i + 7-j] = data[i][j];
		}
	} else if (rotation == 180) { //Clockwise 180 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*(7-j) + 7-i] = data[i][j];
		}
	} else if (rotation == 270) { //Clockwise 270 degree rotation
		for (i = 0; i < 8 ; i++) {
			for (j = 0; j < 8 ; j++)
				raw_data[8*(7-i) + j] = data[i][j];
		}
	}
}

void vl53l8_copy_report_data(struct vl53l8_k_module_t *p_module)
{
#ifdef SS_TEST_ENABLE
	int idx = 0;
	int i, j = 0;
#endif

	mutex_lock(&p_module->mutex);
	/*depth16*/
	memcpy(p_module->af_range_data.depth16,
		&p_module->range.data.tcpm_0_patch.d16_per_target_data,
		sizeof(p_module->af_range_data.depth16));

	/*dmax*/
	memcpy(p_module->af_range_data.dmax,
		p_module->range.data.tcpm_0_patch.per_zone_results.amb_dmax_mm,
		sizeof(p_module->af_range_data.dmax));

	/*peak signal*/
	memcpy(p_module->af_range_data.peak_signal,
		p_module->range.data.tcpm_0_patch.per_tgt_results.peak_rate_kcps_per_spad,
		sizeof(p_module->af_range_data.peak_signal));
	mutex_unlock(&p_module->mutex);

	/*glass detection*/
	p_module->af_range_data.glass_detection_flag = p_module->range.data.tcpm_0_patch.gd_op_status.gd__glass_detected;

	/*ASZ*/
	p_module->af_range_data.depth16[UPPER_LEFT] =
			p_module->range.data.tcpm_0_patch.d16_per_target_data.depth16[ASZ_1];
	p_module->af_range_data.depth16[UPPER_RIGHT] =
			p_module->range.data.tcpm_0_patch.d16_per_target_data.depth16[ASZ_2];
	p_module->af_range_data.depth16[BOTTOM_LEFT] =
			p_module->range.data.tcpm_0_patch.d16_per_target_data.depth16[ASZ_3];
	p_module->af_range_data.depth16[BOTTOM_RIGHT] =
			p_module->range.data.tcpm_0_patch.d16_per_target_data.depth16[ASZ_4];

	p_module->af_range_data.dmax[UPPER_LEFT] =
			p_module->range.data.tcpm_0_patch.per_zone_results.amb_dmax_mm[ASZ_1];
	p_module->af_range_data.dmax[UPPER_RIGHT] =
			p_module->range.data.tcpm_0_patch.per_zone_results.amb_dmax_mm[ASZ_2];
	p_module->af_range_data.dmax[BOTTOM_LEFT] =
			p_module->range.data.tcpm_0_patch.per_zone_results.amb_dmax_mm[ASZ_3];
	p_module->af_range_data.dmax[BOTTOM_RIGHT] =
			p_module->range.data.tcpm_0_patch.per_zone_results.amb_dmax_mm[ASZ_4];

	p_module->af_range_data.peak_signal[UPPER_LEFT] =
			p_module->range.data.tcpm_0_patch.per_tgt_results.peak_rate_kcps_per_spad[ASZ_1];
	p_module->af_range_data.peak_signal[UPPER_RIGHT] =
			p_module->range.data.tcpm_0_patch.per_tgt_results.peak_rate_kcps_per_spad[ASZ_2];
	p_module->af_range_data.peak_signal[BOTTOM_LEFT] =
			p_module->range.data.tcpm_0_patch.per_tgt_results.peak_rate_kcps_per_spad[ASZ_3];
	p_module->af_range_data.peak_signal[BOTTOM_RIGHT] =
			p_module->range.data.tcpm_0_patch.per_tgt_results.peak_rate_kcps_per_spad[ASZ_4];

#ifdef SS_SUPPORT_DATA_ROTATION
	vl53l8_rotate_report_data_u16(p_module->af_range_data.depth16, p_module->rotation_mode);
	vl53l8_rotate_report_data_u16(p_module->af_range_data.dmax, p_module->rotation_mode);
	vl53l8_rotate_report_data_u32(p_module->af_range_data.peak_signal, p_module->rotation_mode);
#endif //SS_SUPPORT_DATA_ROTATION

#ifdef SS_TEST_ENABLE
	for (i = 0; i < p_module->print_counter; i++) {
		for (j = 0; j < p_module->print_counter; j++) {
			idx = (i * p_module->print_counter + j);
			p_module->data[idx] = (p_module->af_range_data.depth16[idx]) & 0x1FFFU;
		}
	}

	for (i = 0; i < 64; i += 8) {
		vl53l8_k_log_info("ALL dist[0]:%d, %d, %d, %d, %d, %d, %d, %d\n",
		p_module->data[i], p_module->data[i+1], p_module->data[i+2],
		p_module->data[i+3], p_module->data[i+4], p_module->data[i+5],
		p_module->data[i+6], p_module->data[i+7]);
	}

	for (i = 0; i < p_module->print_counter; i++) {
		for (j = 0; j < p_module->print_counter; j++) {
			idx = (i * p_module->print_counter + j);
			p_module->data[idx] = (p_module->af_range_data.peak_signal[idx] >> 11);
		}
	}

	for (i = 0; i < 64; i += 8) {
		vl53l8_k_log_info("ALL peak[0]:%d, %d, %d, %d, %d, %d, %d, %d\n",
		p_module->data[i], p_module->data[i+1], p_module->data[i+2],
		p_module->data[i+3], p_module->data[i+4], p_module->data[i+5],
		p_module->data[i+6], p_module->data[i+7]);
	}


	for (i = 0; i < p_module->print_counter; i++) {
		for (j = 0; j < p_module->print_counter; j++) {
			idx = (i * p_module->print_counter + j);
			p_module->data[idx] = (p_module->af_range_data.dmax[idx]);
		}
	}

	for (i = 0; i < 64; i += 8) {
		vl53l8_k_log_info("ALL dmax[0]:%d, %d, %d, %d, %d, %d, %d, %d\n",
		p_module->data[i], p_module->data[i+1], p_module->data[i+2],
		p_module->data[i+3], p_module->data[i+4], p_module->data[i+5],
		p_module->data[i+6], p_module->data[i+7]);
	}
#endif
}
#endif

#ifdef SS_SUPPORT_SEC_CODE
int vl53l8_ioctl_stop(struct vl53l8_k_module_t *p_module)
#else
int vl53l8_ioctl_stop(struct vl53l8_k_module_t *p_module, void __user *p)
#endif
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L8_STATE_RANGING);
#else // SS_SUPPORT_SEC_CODE
	if (p_module->state_preset == VL53L8_STATE_LOW_POWER)
		return status;

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
#endif

	if (status != VL53L5_ERROR_NONE)
		goto out_state;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (p != NULL) {
		status = copy_from_user(
			&p_module->ranging_flags, p,
			sizeof(struct vl53l5_ranging_mode_flags_t));
		if (status != VL53L5_ERROR_NONE) {
			status = VL53L8_K_ERROR_FAILED_TO_COPY_FLAGS;
			goto out;
		}
	}

#ifdef VL53L8_INTERRUPT
	if (p_module->irq_is_active)
		disable_irq(p_module->irq_val);

	vl53l8_k_log_info("disable irq");
	p_module->irq_is_active = false;
#endif
#endif

	status = vl53l5_stop(&p_module->stdev, &p_module->ranging_flags);
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
	if (status != VL53L5_ERROR_NONE)
		vl53l8_last_error_counter(p_module, status);
#endif

#ifdef SS_SUPPORT_SEC_CODE
#ifdef VL53L8_INTERRUPT
	usleep_range(10000, 10100);
	if (p_module->irq_is_active)
		disable_irq(p_module->irq_val);
	vl53l8_k_log_info("disable irq");
	p_module->irq_is_active = false;
#endif
#endif

	if (status != VL53L5_ERROR_NONE)
		goto out;

	p_module->range.count = 0;
	p_module->range.is_valid = 0;
	p_module->polling_start_time_ms = 0;
	p_module->state_preset = VL53L8_STATE_INITIALISED;
	p_module->rate = p_module->rate_init;
	p_module->integration = p_module->integration_init;

#ifdef SS_SUPPORT_SEC_CODE
	vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_ULP_IDLE);
#endif

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif
	LOG_FUNCTION_END(status);
	return status;
}



int vl53l8_ioctl_get_range(struct vl53l8_k_module_t *p_module, void __user *p)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	//vl53l8_k_log_debug("Lock");
	//mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L8_STATE_RANGING);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

#ifndef VL53L8_INTERRUPT
	if (p_module->range_mode == VL53L8_RANGE_SERVICE_DEFAULT) {
		status = vl53l5_get_range_data(&p_module->stdev);
		if (status == VL53L5_NO_NEW_RANGE_DATA_ERROR ||
		    status == VL53L5_TOO_HIGH_AMBIENT_WARNING)
			status = VL53L5_ERROR_NONE;
		if (status != VL53L5_ERROR_NONE)
			goto out;

		status = vl53l5_decode_range_data(&p_module->stdev,
						  &p_module->range.data);
		if (status != VL53L5_ERROR_NONE)
			goto out;

		if (p_module->patch_ver.patch_version.ver_major < 6 &&
		    p_module->patch_ver.patch_version.ver_minor >= 0) {
			status = vl53l8_k_glare_filter(&p_module->gf_tuning,
							&p_module->range.data);
			if (status != VL53L5_ERROR_NONE)
				goto out;
		}

		p_module->range.count++;
		p_module->range.is_valid = 1;
	}
#endif
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (p != NULL) {
		status = copy_to_user(
			p, &p_module->range.data, sizeof(struct vl53l5_range_data_t));
		if (status != VL53L5_ERROR_NONE) {
			status = VL53L8_K_ERROR_FAILED_TO_COPY_RANGE_DATA;
			goto out;
		}
	}
#else // SS_SUPPORT_SEC_CODE
	if (status == STATUS_OK) {
		vl53l8_copy_report_data(p_module);
		if (p != NULL) {
			status = copy_to_user(
				p, &p_module->af_range_data, sizeof(struct range_sensor_data_t));
		}
		if (status != STATUS_OK)
			status = VL53L8_K_ERROR_FAILED_TO_COPY_RANGE_DATA;
	}
#endif
#if defined(STM_VL53L8_SUPPORT_LEGACY_CODE) || !defined(VL53L8_INTERRUPT)
out:
#endif
	if (status != VL53L5_ERROR_NONE)
		status = vl53l5_read_device_error(&p_module->stdev, status);
#ifdef VL53L8_INTERRUPT
	if (p_module->last_driver_error != VL53L5_ERROR_NONE &&
	    p_module->last_driver_error != status) {
		status = p_module->last_driver_error;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
	}
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_debug("Disable IRQ");
		disable_irq(p_module->gpio.interrupt);
		p_module->irq_is_active = false;
	}
#endif
#endif
	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("Failed: %d", status);
#ifdef VL53L5_TCDM_ENABLE
		if (status < VL53L8_K_ERROR_NOT_DEFINED) {
			vl53l8_k_log_info("TCDM Dump");
			_tcdm_dump(p_module);
		}
#endif
	}
out_state:

	//vl53l8_k_log_debug("Unlock");
	//mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}

#ifdef SS_SUPPORT_SEC_CODE
int vl53l8_get_firmware_version(struct vl53l8_k_module_t *p_module, struct vl53l8_k_version_t *p_version)
{
	int status = VL53L5_ERROR_NONE;;

	status = vl53l5_get_version(&p_module->stdev, &p_version->driver);
	if (status != VL53L5_ERROR_NONE)
		return status;

	status = vl53l5_get_patch_version(&p_module->stdev, &p_version->patch);
	if (status != VL53L5_ERROR_NONE)
		return status;

	memcpy(&p_version->bin_version, &p_module->bin_version, sizeof(struct vl53l8_k_bin_version));

	return status;
}

int vl53l8_set_device_parameters(struct vl53l8_k_module_t *p_module,
				       int config)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	//vl53l8_k_log_debug("Lock");
	//mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	p_module->config_preset = config;
	status = STATUS_OK;

	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_CONFIG_PRESET;
		goto out;
	}

	status = _set_device_params(p_module);

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:

	//vl53l8_k_log_debug("Unlock");
	//mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
}

#ifdef CONFIG_SEPARATE_IO_CORE_POWER
int vl53l8_regulator_init_state(struct vl53l8_k_module_t *data)
{
	int voltage;

	if (data->avdd_vreg_name) {
		if (data->avdd_vreg == NULL) {
			data->avdd_vreg = regulator_get(&data->spi_info.device->dev, data->avdd_vreg_name);
			if (IS_ERR(data->avdd_vreg)) {
				data->avdd_vreg = NULL;
				return VL53L8_LDO_AVDD_ERROR;
			}
			if (data->avdd_vreg) {
				voltage = regulator_get_voltage(data->avdd_vreg);
				if (voltage < 0) {
					vl53l8_k_log_error("avdd dummy voltage=%d\n", voltage);
					data->avdd_vreg = NULL;
					return VL53L8_LDO_AVDD_ERROR;
				}
			}
		}
	}

	if (data->iovdd_vreg_name) {
		if (data->iovdd_vreg == NULL) {
			data->iovdd_vreg = regulator_get(&data->spi_info.device->dev, data->iovdd_vreg_name);
			if (IS_ERR(data->iovdd_vreg)) {
				data->iovdd_vreg = NULL;
				return VL53L8_LDO_AVDD_ERROR;
			}
			if (data->iovdd_vreg) {
				voltage = regulator_get_voltage(data->iovdd_vreg);
				if (voltage < 0) {
					vl53l8_k_log_error("iovdd_vreg dummy voltage=%d\n", voltage);
					data->iovdd_vreg = NULL;
					return VL53L8_LDO_IOVDD_ERROR;
				}
			}
		}
	}

	if (data->corevdd_vreg_name) {
		if (data->corevdd_vreg == NULL) {
			data->corevdd_vreg = regulator_get(&data->spi_info.device->dev, data->corevdd_vreg_name);
			if (IS_ERR(data->corevdd_vreg)) {
				data->corevdd_vreg = NULL;
				return VL53L8_LDO_AVDD_ERROR;
			}
			if (data->corevdd_vreg) {
				voltage = regulator_get_voltage(data->corevdd_vreg);
				if (voltage < 0) {
					vl53l8_k_log_error("corevdd_vreg dummy voltage=%d\n", voltage);
					data->corevdd_vreg = NULL;
					return VL53L8_LDO_COREVDD_ERROR;
				}
			}
		}
	}

	return STATUS_OK;
}
#endif

int vl53l8_ldo_onoff(struct vl53l8_k_module_t *data, int io, bool on)
{
	int ret = 0;
#ifndef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
	int voltage = 0;
	int reg_enabled = 0;
#endif

	vl53l8_k_log_info("%d %s\n", io, on ? "on" : "off");

	if (io == AVDD) {
		if (data->avdd_vreg_name) {
			if (data->avdd_vreg == NULL) {
				vl53l8_k_log_error("avdd is null\n");

				data->avdd_vreg = regulator_get(&data->spi_info.device->dev, data->avdd_vreg_name);
				if (IS_ERR(data->avdd_vreg)) {
					data->avdd_vreg = NULL;
					vl53l8_k_log_error("failed avdd %s\n", data->avdd_vreg_name);
				}
			}
		}

		if (data->avdd_vreg) {
#ifndef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
			voltage = regulator_get_voltage(data->avdd_vreg);
			reg_enabled = regulator_is_enabled(data->avdd_vreg);
			vl53l8_k_log_debug("avdd reg_enabled=%d voltage=%d\n", reg_enabled, voltage);
#endif

			if (on) {
#ifndef CONFIG_SEPARATE_IO_CORE_POWER
				if (reg_enabled == 0)
#endif
				{
					vl53l8_k_log_info("avdd on\n");
					ret = regulator_enable(data->avdd_vreg);
					if (ret) {
						vl53l8_k_log_error("avdd enable fail\n");
						return ret;
					}
				}
			} else {
#ifndef CONFIG_SEPARATE_IO_CORE_POWER
				if (reg_enabled == 1)
#endif
				{
					vl53l8_k_log_info("avdd off\n");
					ret = regulator_disable(data->avdd_vreg);
					if (ret) {
						vl53l8_k_log_error("avdd disable fail\n");
						return ret;
					}
				}
			}
		} else
			vl53l8_k_log_info("avdd error\n");
	} else if (io == IOVDD) {
		if (data->iovdd_vreg_name) {
			if (data->iovdd_vreg == NULL) {
				vl53l8_k_log_error("iovdd is null\n");

				data->iovdd_vreg = regulator_get(&data->spi_info.device->dev, data->iovdd_vreg_name);
				if (IS_ERR(data->iovdd_vreg)) {
					data->iovdd_vreg = NULL;
					vl53l8_k_log_error("failed iovdd %s\n", data->iovdd_vreg_name);
				}
			}
		}

		if (data->iovdd_vreg) {
#ifndef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
			voltage = regulator_get_voltage(data->iovdd_vreg);
			reg_enabled = regulator_is_enabled(data->iovdd_vreg);
			vl53l8_k_log_debug("iovdd reg_enabled=%d voltage=%d\n", reg_enabled, voltage);
#endif
			if (on) {
#ifndef CONFIG_SEPARATE_IO_CORE_POWER
				if (reg_enabled == 0)
#endif
				{
					vl53l8_k_log_info("iovdd on\n");
					ret = regulator_enable(data->iovdd_vreg);
					if (ret) {
						vl53l8_k_log_error("iovdd enable err\n");
						return ret;
					}
				}
			} else {
#ifndef CONFIG_SEPARATE_IO_CORE_POWER
				if (reg_enabled == 1)
#endif
				{
					vl53l8_k_log_info("iovdd off\n");
					ret = regulator_disable(data->iovdd_vreg);
					if (ret) {
						vl53l8_k_log_error("iovdd disable err\n");
						return ret;
					}
				}
			}
		} else
			vl53l8_k_log_info("vdd_vreg err\n");
#ifdef CONFIG_SEPARATE_IO_CORE_POWER
	} else if (io == COREVDD) {
		if (data->corevdd_vreg_name) {
			if (data->corevdd_vreg == NULL) {
				vl53l8_k_log_error("corevdd is null\n");

				data->corevdd_vreg = regulator_get(&data->spi_info.device->dev, data->corevdd_vreg_name);
				if (IS_ERR(data->corevdd_vreg)) {
					data->corevdd_vreg = NULL;
					vl53l8_k_log_error("failed corevdd %s\n", data->corevdd_vreg_name);
				}
			}
		}

		if (data->corevdd_vreg) {
#ifndef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
			voltage = regulator_get_voltage(data->corevdd_vreg);
			reg_enabled = regulator_is_enabled(data->corevdd_vreg);
			vl53l8_k_log_info("corevdd reg_enabled=%d voltage=%d\n", reg_enabled, voltage);
#endif
			if (on) {
				vl53l8_k_log_info("corevdd on\n");
				ret = regulator_enable(data->corevdd_vreg);
				if (ret) {
					vl53l8_k_log_error("corevdd enable err\n");
					return ret;
				}
			} else {
				vl53l8_k_log_info("corevdd off\n");
				ret = regulator_disable(data->corevdd_vreg);
				if (ret) {
					vl53l8_k_log_error("corevdd disable err\n");
					return ret;
				}
			}
		} else
			vl53l8_k_log_info("vdd_vreg err\n");
#endif
	} else {
		vl53l8_k_log_info("wrong io %d\n", io);
	}

	return ret;
}

void vl53l8_ldo_control(struct vl53l8_k_module_t *data, int regulator, bool on)
{
	int ret = 0;

	if (data->ldo_prev_state[regulator] != on)
		ret = vl53l8_ldo_onoff(data, regulator, on);

	if (ret < 0) {
		vl53l8_k_log_error("ldo[%d] on:%d err:%d\n", regulator, on, ret);
		return;
	}

	data->ldo_prev_state[regulator] = on;
}

void vl53l8_power_onoff(struct vl53l8_k_module_t *p_module, bool on)
{
	if (!on)
		p_module->state_preset = VL53L8_STATE_PRESENT;

	vl53l8_ldo_control(p_module, IOVDD, on);
	vl53l8_ldo_control(p_module, AVDD, on);
#ifdef CONFIG_SEPARATE_IO_CORE_POWER
	vl53l8_ldo_control(p_module, COREVDD, on);
#endif

#ifndef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
	if (on) {
		int status;

		usleep_range(1000, 1100);
		p_module->stdev.host_dev.p_fw_buff = NULL;
		status = vl53l5_init(&p_module->stdev);
		if (status < 0) {
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
			vl53l8_last_error_counter(p_module, status);
#endif
			vl53l8_k_log_error("resume init err");
			p_module->stdev.last_dev_error = VL53L8_RESUME_INIT_ERROR;
			return;
		}

		p_module->stdev.last_dev_error = VL53L5_ERROR_NONE;
		p_module->stdev.status_probe = STATUS_OK;
		p_module->last_driver_error = STATUS_OK;

		p_module->state_preset = VL53L8_STATE_INITIALISED;
		p_module->power_state = VL53L5_POWER_STATE_HP_IDLE;
		vl53l8_k_log_info("VL53L8_STATE_INITIALISED");
	}
#endif
}

int vl53l8_ioctl_get_status(struct vl53l8_k_module_t *p_module, void __user *p)
{
	int status = STATUS_OK;

	if (p != NULL) {
		status = copy_to_user(p, &p_module->ioctl_enable_status, sizeof(int));
	} else {
		vl53l8_k_log_error("failed to get enable status");
		status = VL53L5_ERROR_INVALID_PARAMS;
	}

	return status;
}

int vl53l8_ioctl_get_cal_data(struct vl53l8_k_module_t *p_module, void __user *p)
{
	int status = STATUS_OK;

	if (p != NULL) {
		status = copy_to_user(p, &p_module->cal_data, sizeof(struct vl53l8_cal_data_t));
		vl53l8_k_log_info("get data %d %d", p_module->cal_data.cmd, p_module->cal_data.size);
	} else {
		vl53l8_k_log_info("get is null");
		status = VL53L5_ERROR_INVALID_PARAMS;
	}

	return status;
}

int vl53l8_ioctl_set_cal_data(struct vl53l8_k_module_t *p_module, void __user *p)
{
	int status = VL53L5_ERROR_INVALID_PARAMS;

	mutex_lock(&p_module->cal_mutex);
	if (p != NULL) {
		status = copy_from_user(&p_module->cal_data, p, sizeof(struct vl53l8_cal_data_t));
		vl53l8_k_log_info("set cal cmd %d, %d", p_module->cal_data.cmd, p_module->cal_data.size);

		if ((p_module->cal_data.size > 0)
			&& (p_module->cal_data.size <= p_module->stdev.host_dev.comms_buff_max_count)) {
			memset(p_module->stdev.host_dev.p_comms_buff, 0, p_module->stdev.host_dev.comms_buff_max_count);
			memcpy(p_module->stdev.host_dev.p_comms_buff, p_module->cal_data.pcal_data, p_module->cal_data.size);

			p_module->pass_fail_flag |= 1 << p_module->cal_data.cmd;
			p_module->update_flag |= 1 << p_module->cal_data.cmd;

			status = STATUS_OK;
		} else {
			vl53l8_k_log_error("Over Size %d", p_module->stdev.host_dev.comms_buff_max_count);
#ifdef SS_TEST_ENABLE
			panic("Cal Data Size Error");
#endif
		}
	} else {
		vl53l8_k_log_info("set cal data is null");
	}

	mutex_unlock(&p_module->cal_mutex);
	return status;
}

int vl53l8_ioctl_set_pass_fail(struct vl53l8_k_module_t *p_module,
				       void __user *p)
{
	int status = STATUS_OK;

	if (p != NULL) {
		struct vl53l8_update_data_t result;

		status = copy_from_user(&result, p, sizeof(struct vl53l8_update_data_t));
		if (status == STATUS_OK) {
			p_module->pass_fail_flag |= result.pass_fail << result.cmd;
			p_module->update_flag |= 1 << result.cmd;
			vl53l8_k_log_info("update %x, %d", p_module->update_flag, result.pass_fail);
		} else {
			vl53l8_k_log_error("copy from user err");
		}
	} else
		status = VL53L5_ERROR_INVALID_PARAMS;

	return status;
}

int vl53l8_ioctl_set_file_list(struct vl53l8_k_module_t *p_module,
				       void __user *p)
{
	int status = STATUS_OK;

	if (p != NULL) {
		struct vl53l8_file_list_t list;

		status = copy_from_user(&list, p, sizeof(struct vl53l8_file_list_t));
		if (status == STATUS_OK)
			p_module->file_list = list.file_list;
		else
			vl53l8_k_log_error("copy from user err");
	} else
		status = VL53L5_ERROR_INVALID_PARAMS;

	return status;
}

int vl53l8_input_report(struct vl53l8_k_module_t *p_module, int type, int cmd)
{
	int status = STATUS_OK;
	int cnt = 0;

	if (cmd >= 8) {
		status = VL53L8_IO_ERROR;
		vl53l8_k_log_info("Invalid cmd %d", cmd);
		return status;
	}

	if (p_module->input_dev) {
		p_module->update_flag &= ~(1 << cmd);
		p_module->pass_fail_flag &= ~(1 << cmd);

		vl53l8_k_log_info("send event %d", type);
		input_report_rel(p_module->input_dev, REL_MISC, type);
		input_sync(p_module->input_dev);

		while (!(p_module->update_flag & 1 << cmd)
			&& cnt++ < TIMEOUT_CNT)
			usleep_range(2000, 2100);

		vl53l8_k_log_info("cnt %d / %x,%x", cnt, p_module->update_flag, p_module->pass_fail_flag);
		p_module->update_flag &= ~(1 << cmd);
		if (p_module->pass_fail_flag & (1 << cmd))
			p_module->pass_fail_flag &= ~(1 << cmd);
		else
			status = VL53L8_IO_ERROR;
		vl53l8_k_log_info("cmd %d / %x,%x", cmd, p_module->update_flag, p_module->pass_fail_flag);
	} else {
		vl53l8_k_log_error("input_dev err %d %d", type, cmd);
	}
	return status;
}
#endif
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_set_device_parameters(struct vl53l8_k_module_t *p_module,
				       void __user *p)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	status = copy_from_user(&p_module->config_preset, p,
				sizeof(int));
	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_CONFIG_PRESET;
		goto out;
	}

	status = _set_device_params(p_module);

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:

	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_ioctl_get_device_parameters(struct vl53l8_k_module_t *p_module,
				       void __user *p)
{
	int status;
	struct vl53l8_k_get_parameters_data_t *p_get_data;
	int count = 0;

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	p_get_data = kzalloc(
		sizeof(struct vl53l8_k_get_parameters_data_t), GFP_KERNEL);
	if (p_get_data == NULL) {
		vl53l8_k_log_error("Allocate Failed");
		status = VL53L8_K_ERROR_FAILED_TO_ALLOCATE_PARAMETER_DATA;
		goto out;
	}

	status = copy_from_user(
		p_get_data, p, sizeof(struct vl53l8_k_get_parameters_data_t));
	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_PARAMETER_DATA;
		goto out_free;
	}

	status = vl53l5_get_device_parameters(&p_module->stdev,
					      p_get_data->headers,
					      p_get_data->header_count);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	count = (p_module->stdev.host_dev.comms_buff_count - 4);

	memcpy(p_get_data->raw_data.buffer,
	       p_module->stdev.host_dev.p_comms_buff,
	       count);
	p_get_data->raw_data.count = count;

	status = copy_to_user(
		p, p_get_data, sizeof(struct vl53l8_k_get_parameters_data_t));

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
		p_module->last_driver_error = status;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
	}
out_free:
	kfree(p_get_data);
out_state:

	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_ioctl_get_error_info(struct vl53l8_k_module_t *p_module,
				void __user *p)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = copy_to_user(p, &p_module->last_driver_error, sizeof(int));
	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_ERROR_INFO;
		vl53l8_k_log_error("Failed: %d", status);
		goto out;
	}

	p_module->last_driver_error = VL53L5_ERROR_NONE;
out:

	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}
#endif
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_set_power_mode(struct vl53l8_k_module_t *p_module,
				void __user *p)
#else // SS_SUPPORT_SEC_CODE
int vl53l8_ioctl_set_power_mode(struct vl53l8_k_module_t *p_module,
				void __user *p, enum vl53l5_power_states state)
#endif
{
	int status = VL53L5_ERROR_NONE;
	enum vl53l8_k_state_preset current_state;
	const char *fw_path;
	struct spi_device *spi;
	const struct vl53l8_fw_header_t *header;
	unsigned char *fw_data;
	const struct firmware *fw_entry = NULL;
	enum vl53l5_power_states current_power_state;

	LOG_FUNCTION_START("");

	//vl53l8_k_log_debug("Lock");
	//mutex_lock(&p_module->mutex);

	current_state = p_module->state_preset;
	current_power_state = p_module->power_state;

	if (current_state < VL53L8_STATE_LOW_POWER) {
		//vl53l8_k_log_debug("Unlock");
		//mutex_unlock(&p_module->mutex);
		status = VL53L8_K_ERROR_DEVICE_STATE_INVALID;
		goto out_state;
	}

	vl53l8_k_log_debug("Current state : %i", p_module->power_state);

	if (p_module->power_state == VL53L5_POWER_STATE_ULP_IDLE) {
		spi = p_module->spi_info.device;
		fw_path = p_module->firmware_name;
		vl53l8_k_log_debug("Load FW : %s", fw_path);

		vl53l8_k_log_debug("Req FW");
		status = request_firmware(&fw_entry, fw_path, &spi->dev);
		if (status) {
			vl53l8_k_log_error("FW %s not available",
						fw_path);
			goto out;
		}

		fw_data = (unsigned char *)fw_entry->data;
		header = (struct vl53l8_fw_header_t *)fw_data;

		vl53l8_k_log_info("Binary FW ver %i.%i",
				header->fw_ver_major, header->fw_ver_minor);

		p_module->stdev.host_dev.p_fw_buff =
						&fw_data[header->fw_offset];
		p_module->stdev.host_dev.fw_buff_count = header->fw_size;
	}
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	status = copy_from_user(&p_module->power_state, p,
				sizeof(enum vl53l5_power_states));
#else // SS_SUPPORT_SEC_CODE
	if (p != NULL) {
		status = copy_from_user(&p_module->power_state, p,
					sizeof(enum vl53l5_power_states));
	} else {
		p_module->power_state = state;
		status = STATUS_OK;
	}
#endif

	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_POWER_MODE;
		goto out;
	}

	vl53l8_k_log_debug("Req state : %i", p_module->power_state);

	status = vl53l5_set_power_mode(&p_module->stdev,
				       p_module->power_state);
	if (status == VL53L5_ERROR_POWER_STATE) {
		vl53l8_k_log_error("Invalid state %i", p_module->power_state);

		p_module->power_state = current_power_state;
		(void)vl53l5_set_power_mode(&p_module->stdev,
						p_module->power_state);
		status = VL53L8_K_ERROR_INVALID_POWER_STATE;
		goto out;
	} else if (status != VL53L5_ERROR_NONE)
		goto out;

	switch (p_module->power_state) {
	case VL53L5_POWER_STATE_ULP_IDLE:
	case VL53L5_POWER_STATE_LP_IDLE_COMMS:
		p_module->state_preset = VL53L8_STATE_LOW_POWER;
		break;
	case VL53L5_POWER_STATE_HP_IDLE:
		p_module->state_preset = VL53L8_STATE_INITIALISED;
		break;
	default:
		vl53l8_k_log_error("Invalid state %i", p_module->power_state);

		p_module->state_preset = current_state;
		status = VL53L8_K_ERROR_INVALID_POWER_STATE;
		break;
	}

out:

	vl53l8_k_log_debug("Release FW");
	release_firmware(fw_entry);

	p_module->stdev.host_dev.p_fw_buff = NULL;
	p_module->stdev.host_dev.fw_buff_count = 0;
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
	//vl53l8_k_log_debug("Unlock");
	//mutex_unlock(&p_module->mutex);
	//LOG_FUNCTION_END(status);
	return status;
}


int vl53l8_ioctl_poll_data_ready(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	//vl53l8_k_log_debug("Lock");
	//mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L8_STATE_RANGING);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	status = _poll_for_new_data(
		p_module, p_module->polling_sleep_time_ms,
		VL53L8_K_MAX_CALIBRATION_POLL_TIME_MS);

	if (status != VL53L5_ERROR_NONE) {
		if ((status == VL53L8_K_ERROR_DEVICE_NOT_INITIALISED) ||
			(status == VL53L8_K_ERROR_RANGE_POLLING_TIMEOUT))
			status = VL53L8_K_ERROR_DEVICE_NOT_RANGING;
		goto out;
	}
out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
	vl53l8_k_log_debug("Unlock");
	//mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_ioctl_read_p2p_calibration(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;
//	unsigned int file_size = VL53L8_K_P2P_FILE_SIZE + 4 +
//				 VL53L8_K_DEVICE_INFO_SZ;

	LOG_FUNCTION_START("");

	//vl53l8_k_log_debug("Lock");
	//mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE) {
#ifdef SS_SUPPORT_SEC_CODE
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -10;
#endif
		goto out_state;
	}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_info("Read filename %s", VL53L8_CAL_P2P_FILENAME);
	vl53l8_k_log_debug("Read file size %i", file_size);

	status = vl53l5_read_file(&p_module->stdev,
				  p_module->stdev.host_dev.p_comms_buff,
				  file_size,
				  VL53L8_CAL_P2P_FILENAME);

	if (status < VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("read file %s failed: %d ",
				   VL53L8_CAL_P2P_FILENAME, status);
		goto out;
	}
#else // SS_SUPPORT_SEC_CODE
		status = vl53l8_input_report(p_module, 1, CMD_READ_CAL_FILE);
		if (status != STATUS_OK) {
			vl53l8_k_log_error("Read Cal Failed");
			if (!p_module->stdev.status_cal)
				p_module->stdev.status_cal = -11;
			goto out;
		} else {
			vl53l8_k_log_info("Read file size %d", p_module->cal_data.size);
		}
#endif

	_decode_device_info(&p_module->calibration,
			    p_module->stdev.host_dev.p_comms_buff);

	status = vl53l5_decode_calibration_data(
		&p_module->stdev,
		&p_module->calibration.cal_data,
		&p_module->stdev.host_dev.p_comms_buff[VL53L8_K_DEVICE_INFO_SZ],
		VL53L8_K_P2P_FILE_SIZE + 4);

	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("Fail vl53l8_decode_calibration_data %d\n", status);
#ifdef SS_SUPPORT_SEC_CODE
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -12;
#endif
		goto out;
	}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_info("FGC: %s", (char *)p_module->calibration.info.fgc);
	vl53l8_k_log_info("CAL ID : %x.%x",
			  p_module->calibration.info.module_id_hi,
			  p_module->calibration.info.module_id_lo);
#else
	vl53l8_k_log_info("ID %x.%x\n",
			  p_module->calibration.info.module_id_hi,
			  p_module->calibration.info.module_id_lo);
#endif

	if ((p_module->calibration.info.module_id_hi ==
	     p_module->m_info.module_id_hi) &&
	    (p_module->calibration.info.module_id_lo ==
	     p_module->m_info.module_id_lo)) {
		status = vl53l5_set_device_parameters(
				&p_module->stdev,
				&p_module->stdev.host_dev.p_comms_buff[VL53L8_K_DEVICE_INFO_SZ],
				VL53L8_K_P2P_FILE_SIZE);

		if (status != VL53L5_ERROR_NONE) {
#ifdef SS_SUPPORT_SEC_CODE
			if (!p_module->stdev.status_cal)
				p_module->stdev.status_cal = -13;
#endif
			goto out;
		}
	} else {
		vl53l8_k_log_error("device and cal id was not matched %x.%x\n",
				   p_module->m_info.module_id_hi,
				   p_module->m_info.module_id_lo);
#ifdef SS_SUPPORT_SEC_CODE
		status = VL53L5_CONFIG_ERROR_INVALID_VERSION;
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -14;
#endif
	}

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
	//vl53l8_k_log_debug("Unlock");
	//mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_read_shape_calibration(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	//vl53l8_k_log_debug("Lock");
	//mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	vl53l8_k_log_debug("Read filename %s", VL53L8_CAL_SHAPE_FILENAME);
	vl53l8_k_log_debug("Read file size %i", VL53L8_K_SHAPE_FILE_SIZE);
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	status = vl53l5_read_file(&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				VL53L8_K_SHAPE_FILE_SIZE + 4,
				VL53L8_CAL_SHAPE_FILENAME);
#endif
	if (status < VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("read file %s failed: %d ",
				   VL53L8_CAL_SHAPE_FILENAME, status);
		goto out;
	}

	status = vl53l5_decode_calibration_data(
		&p_module->stdev,
		&p_module->calibration.cal_data,
		p_module->stdev.host_dev.p_comms_buff,
		VL53L8_K_P2P_FILE_SIZE + 4);

	status = vl53l5_set_device_parameters(
				&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				VL53L8_K_P2P_FILE_SIZE);

	if (status != VL53L5_ERROR_NONE)
		goto out;

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
	//vl53l8_k_log_debug("Unlock");
	//mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}
#endif

int vl53l8_ioctl_read_open_cal_p2p_calibration(struct vl53l8_k_module_t *p_module)
{
	int status = STATUS_OK;
	unsigned char *p_buff = NULL;

	LOG_FUNCTION_START("");

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != STATUS_OK) {
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -15;
		goto out_state;
	}

	status = vl53l8_input_report(p_module, 4, CMD_READ_P2P_CAL_FILE);
	if (status != STATUS_OK) {
		vl53l8_k_log_error("Read Cal Failed");
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -16;
		goto out;
	} else {
		vl53l8_k_log_info("Read file size %d", p_module->cal_data.size);
	}

	_decode_device_info(&p_module->calibration,
			    p_module->stdev.host_dev.p_comms_buff);

	status = vl53l5_decode_calibration_data(
	    &p_module->stdev,
	    &p_module->calibration.cal_data,
	    &p_module->stdev.host_dev.p_comms_buff[VL53L8_K_DEVICE_INFO_SZ],
	    VL53L8_K_P2P_FILE_SIZE + 4);

	if (status != STATUS_OK) {
		vl53l8_k_log_info("Fail vl53l5_decode_calibration_data %d", status);
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -17;
		goto out;
	}
	vl53l8_k_log_info("CAL ID : %x.%x",
			  p_module->calibration.info.module_id_hi,
			  p_module->calibration.info.module_id_lo);

	if ((p_module->calibration.info.module_id_hi
			== p_module->m_info.module_id_hi)
			&& (p_module->calibration.info.module_id_lo
			== p_module->m_info.module_id_lo)) {
		p_buff = p_module->stdev.host_dev.p_comms_buff;
		p_buff += VL53L8_K_DEVICE_INFO_SZ;
		status = vl53l5_set_device_parameters(&p_module->stdev, p_buff,
						VL53L8_K_P2P_FILE_SIZE);

		if (status != STATUS_OK) {
			if (!p_module->stdev.status_cal)
				p_module->stdev.status_cal = -18;
			goto out;
		}
	} else {
		vl53l8_k_log_error("device and cal id was not matched %x.%x",
			  p_module->m_info.module_id_hi,
			  p_module->m_info.module_id_lo);
		status = VL53L5_CONFIG_ERROR_INVALID_VERSION;
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -19;
	}

out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_ioctl_read_open_cal_shape_calibration(struct vl53l8_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != STATUS_OK) {
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -20;
		goto out_state;
	}

	status = vl53l8_input_report(p_module, 5, CMD_READ_SHAPE_CAL_FILE);
	if (status != STATUS_OK) {
		vl53l8_k_log_error("Read Cal Failed");
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -21;
		goto out;
	} else {
		vl53l8_k_log_info("Read file size %d", p_module->cal_data.size);
	}

	_decode_device_info(&p_module->calibration,
			    p_module->stdev.host_dev.p_comms_buff);

	status = vl53l5_decode_calibration_data(
	    &p_module->stdev,
	    &p_module->calibration.cal_data,
	    p_module->stdev.host_dev.p_comms_buff,
	    VL53L8_K_SHAPE_FILE_SIZE + 4);

	if (status != STATUS_OK) {
		vl53l8_k_log_info("Fail vl53l8_decode_calibration_data %d", status);
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -22;
		goto out;
	}

	status = vl53l5_set_device_parameters(
					&p_module->stdev,
					p_module->stdev.host_dev.p_comms_buff,
					VL53L8_K_SHAPE_FILE_SIZE);

	if (status != STATUS_OK)
		goto out;
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
	LOG_FUNCTION_END(status);
	return status;
}


int vl53l8_perform_calibration(struct vl53l8_k_module_t *p_module, int flow)
{
	int status = VL53L5_ERROR_NONE;
	int file_status = VL53L5_ERROR_NONE;
	int xtalk = 0;

	LOG_FUNCTION_START("");

	//vl53l8_k_log_debug("Lock");
	//mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	switch (flow) {
	case 0:
		xtalk = VL53L8_CFG__XTALK_GEN2_8X8_300;
		break;
	case 1:
		xtalk = VL53L8_CFG__XTALK_GEN1_8X8_1000;
		break;
	default:
		vl53l8_k_log_error("Invalid cal flow: %d", flow);
		status = VL53L8_K_ERROR_INVALID_CALIBRATION_FLOW;
		goto out;
	};

#ifdef VL53L8_RAD2PERP_CAL
	status = _perform_rad2perp_calibration(p_module);

	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("Rad2Perp Failed");
		goto out;
	}
#endif

	p_module->config_preset = xtalk;
	status = _set_device_params(p_module);

	if (status != VL53L5_ERROR_NONE)
		goto out;

	status = _start_poll_stop(p_module);

	if (status != VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("Xtalk Failed");
		goto out;
	}

	if (flow == 1) {
		file_status = _write_shape_calibration(p_module);
		if (file_status != VL53L5_ERROR_NONE)
			goto out;
	}
	file_status = _write_p2p_calibration(p_module, flow);
	if (file_status != VL53L5_ERROR_NONE)
		goto out;

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
#ifdef VL53L5_TCDM_ENABLE
		if (status < VL53L8_K_ERROR_NOT_DEFINED) {
			vl53l8_k_log_info("TCDM Dump");
			_tcdm_dump(p_module);
		}
#endif
	}
	if (status == VL53L5_ERROR_NONE && file_status != VL53L5_ERROR_NONE) {
		status = file_status;
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
	//vl53l8_k_log_debug("Unlock");
	//mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}

#ifdef SS_SUPPORT_SEC_CODE
int vl53l8_readfile_genshape(struct vl53l8_k_module_t *p_module,
				char *dst, size_t size)
{
	int status = STATUS_OK;
	struct spi_device *spi;
	const struct firmware *fw_entry = NULL;

	LOG_FUNCTION_START("");
	if (!p_module->genshape_name || dst == NULL) {
		vl53l8_k_log_error(
			"generic shape name does not declared");
		p_module->stdev.status_cal = -1;
		status = -EPERM;
		goto out;
	}

	spi = p_module->spi_info.device;
	status = request_firmware(&fw_entry, p_module->genshape_name, &spi->dev);
	if (status) {
		vl53l8_k_log_error("Firmware %s not available", p_module->genshape_name);
		p_module->stdev.status_cal = -2;
		status = -2;
		goto out;
	}

	if (size > fw_entry->size) {
		size = fw_entry->size;
		vl53l8_k_log_error("Firmware size: %d, req size: %d", (int)fw_entry->size, (int)size);
	}
	memcpy(dst, (char *)fw_entry->data, size);
	vl53l8_k_log_info("Read genshape %s done", p_module->genshape_name);
	release_firmware(fw_entry);
out:
	LOG_FUNCTION_END(status);
	return status;

}
#endif


int vl53l8_ioctl_read_generic_shape(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	//vl53l8_k_log_debug("Lock");
	//mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE) {
#ifdef SS_SUPPORT_SEC_CODE
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -5;
#endif
		goto out_state;

	}

#ifdef SS_SUPPORT_SEC_CODE
	status = vl53l8_readfile_genshape(p_module, p_module->stdev.host_dev.p_comms_buff,
							VL53L8_K_SHAPE_FILE_SIZE + 4);
	if (status != STATUS_OK)
		goto out;

	_decode_device_info(&p_module->calibration,
			    p_module->stdev.host_dev.p_comms_buff);

	status = vl53l5_decode_calibration_data(
	    &p_module->stdev,
	    &p_module->calibration.cal_data,
	    p_module->stdev.host_dev.p_comms_buff,
	    VL53L8_K_SHAPE_FILE_SIZE + 4);

	if (status != STATUS_OK) {
#ifdef SS_SUPPORT_SEC_CODE
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -7;
#endif
		goto out;
	}
#endif

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_info("Read filename %s", VL53L8_GENERIC_SHAPE_FILENAME);

	vl53l8_k_log_debug("Read file size %i", VL53L8_K_SHAPE_FILE_SIZE + 4);

	status = vl53l5_read_file(&p_module->stdev,
				  p_module->stdev.host_dev.p_comms_buff,
				  VL53L8_K_SHAPE_FILE_SIZE + 4,
				  VL53L8_GENERIC_SHAPE_FILENAME);

	if (status < VL53L5_ERROR_NONE) {
		vl53l8_k_log_error("read file %s failed: %d ",
				   VL53L8_GENERIC_SHAPE_FILENAME, status);
		goto out;
	}
#endif

	status = vl53l5_set_device_parameters(
					&p_module->stdev,
					p_module->stdev.host_dev.p_comms_buff,
					VL53L8_K_SHAPE_FILE_SIZE);

	if (status != VL53L5_ERROR_NONE) {
#ifdef SS_SUPPORT_SEC_CODE
		if (!p_module->stdev.status_cal)
			p_module->stdev.status_cal = -8;
#endif // SS_SUPPORT_SEC_CODE
		goto out;
	}

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
out_state:
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif

	LOG_FUNCTION_END(status);
	return status;
}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_set_ranging_rate(struct vl53l8_k_module_t *p_module,
				  void __user *p)
#else // SS_SUPPORT_SEC_CODE
int vl53l8_ioctl_set_ranging_rate(struct vl53l8_k_module_t *p_module,
				  uint32_t rate)
#endif // SS_SUPPORT_SEC_CODE
{
	int status = VL53L5_ERROR_NONE;
	uint32_t ranging_rate = 0;

	LOG_FUNCTION_START("");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#endif
	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (p != NULL) {
		status = copy_from_user(&ranging_rate, p, sizeof(int));
		if (status != VL53L5_ERROR_NONE) {
			status = VL53L8_K_ERROR_FAILED_TO_COPY_RANGING_RATE;
			goto out;
		}
	}
#else
		ranging_rate = rate;
#endif

	status = vl53l5_set_ranging_rate_hz(&p_module->stdev, ranging_rate);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
out:
#endif
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}

out_state:
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif
	LOG_FUNCTION_END(status);
	return status;
}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_set_integration_time_us(struct vl53l8_k_module_t *p_module,
					 void __user *p)
#else // SS_SUPPORT_SEC_CODE
int vl53l8_ioctl_set_integration_time_us(struct vl53l8_k_module_t *p_module,
					 uint32_t integration)
#endif // SS_SUPPORT_SEC_CODE
{
	int status = VL53L5_ERROR_NONE;
	uint32_t max_integration = 0;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);


	if (p != NULL) {
		status = copy_from_user(&max_integration, p, sizeof(int));
		if (status != VL53L5_ERROR_NONE) {
			status = VL53L8_K_ERROR_FAILED_TO_COPY_MAX_INTEGRATION;
			goto out;
		}
	}
#else
	max_integration = integration;
#endif

	status = vl53l5_set_integration_time_us(&p_module->stdev, max_integration);
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
out:
#endif
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
#endif
	return status;
}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_set_asz_tuning(struct vl53l8_k_module_t *p_module,
			    void __user *p)
#else
int vl53l8_ioctl_set_asz_tuning(struct vl53l8_k_module_t *p_module,
			    void __user *p, struct vl53l8_k_asz_tuning_t *asz_layout)
#endif
{
	int status = VL53L5_ERROR_NONE;
	uint8_t buffer[56] = {0};
	uint8_t map_buffer[BYTE_8] = {
		0x40, 0x00, 0x00, 0x54,
		0x0b, 0x00, 0x05, 0x00,};
	uint32_t count = 0;
	struct vl53l8_k_asz_tuning_t asz_tuning = {0};
	struct vl53l8_k_asz_t asz_struct = {0};

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#endif

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	if (p != NULL)
		status = copy_from_user(&asz_tuning, p, sizeof(struct vl53l8_k_asz_tuning_t));
	else if (asz_layout != NULL)
		memcpy(&asz_tuning, asz_layout, sizeof(struct vl53l8_k_asz_tuning_t));
	else {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out_state;
	};

	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_ASZ_TUNING;
		goto out;
	}

	_copy_buffer(buffer, map_buffer, BYTE_8, &count);

	status = vl53l8_k_assign_asz(&asz_struct, &asz_tuning, buffer, &count);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	status = vl53l5_set_device_parameters(&p_module->stdev, buffer, count);
out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}

out_state:
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
#endif
	return status;
}

static void _build_asz_buffer(uint8_t *p_buffer, uint32_t *p_count,
			      struct vl53l8_k_asz_t *p_asz)
{
	uint8_t asz_buffer[48] = {0};
	uint8_t *p_buff = asz_buffer;

	vl53l5_encode_uint32_t(VL53L8_ASZ_0_BH, BYTE_4, p_buff);
	p_buff += BYTE_4;
	memcpy(p_buff, p_asz->asz_0, BYTE_8);
	p_buff += BYTE_8;
	vl53l5_encode_uint32_t(VL53L8_ASZ_1_BH, BYTE_4, p_buff);
	p_buff += BYTE_4;
	memcpy(p_buff, p_asz->asz_1, BYTE_8);
	p_buff += BYTE_8;
	vl53l5_encode_uint32_t(VL53L8_ASZ_2_BH, BYTE_4, p_buff);
	p_buff += BYTE_4;
	memcpy(p_buff, p_asz->asz_2, BYTE_8);
	p_buff += BYTE_8;
	vl53l5_encode_uint32_t(VL53L8_ASZ_3_BH, BYTE_4, p_buff);
	p_buff += BYTE_4;
	memcpy(p_buff, p_asz->asz_3, BYTE_8);

	_copy_buffer(p_buffer, asz_buffer, sizeof(asz_buffer), p_count);
}

static int _populate_asz_struct(uint8_t *p_zone, uint32_t zone_idx)
{
	int status = VL53L5_ERROR_NONE;
	uint32_t c = 0;
	uint32_t r = 0;
	uint32_t i = 0;
	uint32_t asz_size = 2;
	uint32_t grid_size = 8;

	for (i = 1; i < grid_size; i++) {
		if (zone_idx == ((grid_size * i) - 1) || (zone_idx >= 55)) {
			vl53l8_k_log_info("Zone id %i is invalid", zone_idx);
			status = VL53L8_K_ERROR_INVALID_VALUE;
			goto out;
		}
	}

	for (i = 0; i < 4; i++) {
		c = i % asz_size;
		r = i / asz_size;
		p_zone[i] = (uint8_t)(zone_idx + c  + (r * grid_size));
		vl53l8_k_log_debug("zone[%i] %i", i, p_zone[i]);
	}

	for (i = 4; i < 8; i++)
		p_zone[i] = (uint8_t)255;

out:
	return status;
}

static int vl53l8_k_assign_asz(struct vl53l8_k_asz_t *p_asz,
			       struct vl53l8_k_asz_tuning_t *p_tuning,
			       uint8_t *p_buffer, uint32_t *p_count)
{
	int status = VL53L5_ERROR_NONE;

	vl53l8_k_log_debug("asz 0 zone idx %i", p_tuning->asz_0_ll_zone_id);
	status = _populate_asz_struct(p_asz->asz_0, p_tuning->asz_0_ll_zone_id);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	vl53l8_k_log_debug("asz 2 zone idx %i", p_tuning->asz_1_ll_zone_id);
	status = _populate_asz_struct(p_asz->asz_1, p_tuning->asz_1_ll_zone_id);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	vl53l8_k_log_debug("asz 3 zone idx %i", p_tuning->asz_2_ll_zone_id);
	status = _populate_asz_struct(p_asz->asz_2, p_tuning->asz_2_ll_zone_id);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	vl53l8_k_log_debug("asz 4 zone idx %i", p_tuning->asz_3_ll_zone_id);
	status = _populate_asz_struct(p_asz->asz_3, p_tuning->asz_3_ll_zone_id);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	_build_asz_buffer(p_buffer, p_count, p_asz);
out:
	return status;
}

#ifdef VL53L8_RAD2PERP_CAL
static void _buf_r2p_offsets(uint32_t cal__optical_centre__x,
			     uint32_t cal__optical_centre__y,
			     int32_t *pr2p_init__centre_offset__x,
			     int32_t *pr2p_init__centre_offset__y)
{

	int32_t fmt_x = (int32_t)cal__optical_centre__x;
	int32_t fmt_y = (int32_t)cal__optical_centre__y;
	int32_t offset_x = 0;
	int32_t offset_y = 0;

	if (fmt_x > 0 && fmt_y > 0) {

		offset_x = 16 * ((fmt_x + 8) / 16);
		offset_y = 16 * ((fmt_y + 8) / 16);

		offset_x -= fmt_x;
		offset_y -= fmt_y;
	}

	*pr2p_init__centre_offset__x = offset_x - 62;
	*pr2p_init__centre_offset__y = offset_y - 62;
}

static uint32_t _buf_r2p_common(uint32_t dpos2,
				uint32_t sq_spad_pitch,
				uint64_t sq_efl)
{

	uint64_t  tans2     = 0U;
	uint32_t  tmp       = 0U;
	uint32_t  r2p_coeff = 0U;

	tans2 = MUL_32X32_64(dpos2, sq_spad_pitch);
	tans2 = tans2 << 20U;
	tans2 += (sq_efl >> 1U);
	tans2 = DIV_64D64_64(tans2, sq_efl);

	tans2 += 1 << 24U;
	if (tans2 > (uint64_t)VL53L5_UINT32_MAX)
		tans2 = (uint64_t)VL53L5_UINT32_MAX;

	tmp = INTEGER_SQRT((uint32_t)tans2);
	r2p_coeff = (1U << 24U) + (tmp >> 1U);
	r2p_coeff /= tmp;

	return r2p_coeff;
}

static uint32_t _buf_r2p_calc3(uint32_t zone_id,
				uint32_t grid_size,
				uint32_t grid_pitch,
				int32_t x_offset_spads,
				int32_t y_offset_spads,
				uint32_t spad_pitch_um,
				uint32_t efl_um)
{

	uint32_t col = zone_id % grid_size;
	uint32_t row = zone_id / grid_size;
	int32_t xs = 0;
	int32_t ys = 0;
	int32_t xe = 0;
	int32_t ye = 0;
	int32_t x = 0;
	int32_t y = 0;
	int32_t hpos2 = 0;
	int32_t vpos2 = 0;
	uint32_t dpos2 = 0;
	uint32_t dpos2_zone = 0U;
	uint32_t no_of_spads = grid_pitch * grid_pitch;
	uint32_t sq_spad_pitch = spad_pitch_um * spad_pitch_um;
	uint64_t sq_efl = MUL_32X32_64(efl_um, efl_um);
	uint32_t r2p_coeff = 0U;

	xs = ((int32_t)(col * grid_pitch) * 4) + x_offset_spads;
	ys = ((int32_t)(row * grid_pitch) * 4) + y_offset_spads;
	xe = xs + 4 * (int32_t)grid_pitch;
	ye = ys + 4 * (int32_t)grid_pitch;

	for (y = ys; y < ye; y += 4) {

		vpos2 = y * y;

		for (x = xs; x < xe; x += 4) {
			hpos2 = x * x;
			dpos2 = (uint32_t)(hpos2) + (uint32_t)vpos2;

			dpos2_zone += (uint32_t)dpos2;
		}
	}

	dpos2_zone += (no_of_spads >> 1U);
	dpos2_zone = dpos2_zone / no_of_spads;

	r2p_coeff = _buf_r2p_common(dpos2_zone, sq_spad_pitch, sq_efl);

	return r2p_coeff;
}

static void _buf_r2p_base(uint32_t grid_size, uint32_t grid_pitch,
			  int32_t x_offset_spads,
			  int32_t y_offset_spads, uint32_t efl_um,
			  uint8_t *pbuffer)
{
	uint32_t z          = 0U;
	uint32_t r2p_coeff  = 0U;
	uint8_t *buff = pbuffer;

	for (z = 0U; z < (grid_size * grid_size); z++) {
		r2p_coeff = 4096U;
		r2p_coeff = _buf_r2p_calc3(z, grid_size, grid_pitch,
					   x_offset_spads,
					   y_offset_spads,
					   VL53L8_K_R2P_SPAD_PITCH_UM,
					   efl_um);
		vl53l5_encode_uint16_t((uint16_t)r2p_coeff, BYTE_2, buff);
		buff += BYTE_2;
	}
}

static void _perform_buf_r2p(uint32_t r2p_optical_centre_x,
			     uint32_t r2p_optical_centre_y,
			     uint8_t *pbuffer)
{
	uint32_t lens_efl_um = VL53L8_K_R2P_DEF_LENS_UM;
	int32_t  r2p_init__centre_offset__x = -62;
	int32_t  r2p_init__centre_offset__y = -62;
	uint8_t *buff = pbuffer;

	_buf_r2p_offsets(r2p_optical_centre_x,
			 r2p_optical_centre_y,
			 &r2p_init__centre_offset__x,
			 &r2p_init__centre_offset__y);

	_buf_r2p_base(8U, 4U, r2p_init__centre_offset__x,
		      r2p_init__centre_offset__y, lens_efl_um,
		      &buff[VL53L8_K_R2P_GRID_8X8_DATA_SCALE_FACTOR_OFF]);

	_buf_r2p_base(4U, 8U, r2p_init__centre_offset__x,
		      r2p_init__centre_offset__y, lens_efl_um,
		      &buff[VL53L8_K_R2P_GRID_4X4_DATA_SCALE_FACTOR_OFF]);

}

static int _get_optical_center(struct vl53l8_k_module_t *p_module,
				uint8_t *cal__optical_centre__x,
				uint8_t *cal__optical_centre__y)
{
	int32_t status = VL53L5_ERROR_NONE;
	int buffer[] = {VL53L5_CAL_OPTICAL_CENTRE_BH};

	status = vl53l5_get_device_parameters(&p_module->stdev, buffer, 1);

	if (status != VL53L5_ERROR_NONE)
		goto out;

	status = vl53l5_decode_calibration_data(
		&p_module->stdev,
		&p_module->calibration.cal_data,
		p_module->stdev.host_dev.p_comms_buff,
		p_module->stdev.host_dev.comms_buff_count);

	*cal__optical_centre__x =
		p_module->calibration.cal_data.core.cal_optical_centre.cal__optical_centre__x;
	*cal__optical_centre__y =
		p_module->calibration.cal_data.core.cal_optical_centre.cal__optical_centre__y;
out:
	return status;
}

static int _perform_rad2perp_calibration(struct vl53l8_k_module_t *p_module)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint8_t cal__optical_centre__x = 0;
	uint8_t cal__optical_centre__y = 0;
	uint8_t r2p_buffer[] = VL53L8_K_RAD2PERP_DEFAULT;

	status = _get_optical_center(p_module,
				     &cal__optical_centre__x,
				     &cal__optical_centre__y);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	_perform_buf_r2p(cal__optical_centre__x, cal__optical_centre__y,
			 r2p_buffer);

	status = vl53l5_set_device_parameters(&p_module->stdev,
					      r2p_buffer, sizeof(r2p_buffer));

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}

	return status;
}
#endif
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_set_glare_filter_tuning(struct vl53l8_k_module_t *p_module,
					 void __user *p)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	status = copy_from_user(&p_module->gf_tuning, p,
			sizeof(struct vl53l8_k_glare_filter_tuning_t));
	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_CALIBRATION;
		goto out;
	}

	vl53l8_k_glare_detect_init(&p_module->gf_tuning);

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}

out_state:
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_ioctl_set_transfer_speed_hz(struct vl53l8_k_module_t *p_module,
					 void __user *p)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L8_STATE_INITIALISED);
	if (status != VL53L5_ERROR_NONE)
		goto out_state;

	status = copy_from_user(&p_module->transfer_speed_hz, p,
						sizeof(unsigned int));

	vl53l8_k_log_debug("transfer hz  %d", p_module->transfer_speed_hz);


	if (status != VL53L5_ERROR_NONE) {
		status = VL53L8_K_ERROR_FAILED_TO_COPY_CALIBRATION;
		goto out;
	}

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l8_k_log_error("Failed: %d", status);
	}

out_state:
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;

}
#endif

#ifdef SS_SUPPORT_SEC_CODE
int vl53l8_set_transfer_speed_hz(struct vl53l8_k_module_t *p_module, unsigned int freq)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");
	p_module->transfer_speed_hz = freq;

	vl53l8_k_log_debug("transfer hz  %d", p_module->transfer_speed_hz);

	LOG_FUNCTION_END(status);
	return status;
}
#endif // SS_SUPPORT_SEC_CODE
