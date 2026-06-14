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

#include "vl53l5_dci_ranging.h"
#include "vl53l5_dci_helpers.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_driver_dev_path.h"
#include "vl53l5_platform.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform_user_config.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_DCI, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_DCI, fmt, ##__VA_ARGS__)

#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_DCI, status, ##__VA_ARGS__)

#define DATA_PRESENT(p_dev)\
	(VL53L5_GO2_STATUS_0(p_dev).mcu__ui_range_data_present_go1 == 1)

#define HW_TRAP(p_dev)\
	(VL53L5_GO2_STATUS_0(p_dev).mcu__hw_trap_flag_go1 == 1)

#define MCU_ERROR(p_dev)\
	(VL53L5_GO2_STATUS_0(p_dev).mcu__error_flag_go1 == 1)

#define MCU_WARNING(p_dev)\
	(VL53L5_GO2_STATUS_1(p_dev).mcu__warning_flag_go1 == 1)

#define MCU_CP_COLLAPSE(p_dev)\
	(VL53L5_GO2_STATUS_1(p_dev).mcu__cp_collapse_flag_go1 == 1)

#define STREAM_COUNT_NEW(p_dev, last_stream_id)\
	(VL53L5_UI_DEV_STREAM(p_dev) != last_stream_id)

#define RANGE_DATA_READ_SIZE(p_dev) \
	(p_dev->host_dev.range_data_addr.data_start_offset_bytes\
		+ p_dev->host_dev.range_data_addr.data_size_bytes\
		+ VL53L5_HEADER_FOOTER_BLOCK_SZ)

static int32_t _read_range_ui(
	struct vl53l5_dev_handle_t *p_dev, uint32_t count);

static void _decode_device_info_block(struct vl53l5_dev_handle_t *p_dev);

static int32_t _check_range_header_footer(struct vl53l5_dev_handle_t *p_dev);

int32_t vl53l5_dci_read_range(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t read_size = 0;
	uint8_t previous_stream_id = 0;

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (p_dev->host_dev.range_data_addr.data_size_bytes == 0) {
		status = VL53L5_ERROR_INVALID_RETURN_DATA_SIZE;
		goto exit;
	}

	read_size = RANGE_DATA_READ_SIZE(p_dev);

	if (p_dev->host_dev.revision_id == 0x0C)
		read_size += VL53L5_UI_DUMMY_BYTES;

	if (read_size > p_dev->host_dev.comms_buff_max_count) {

		status = VL53L5_DATA_EXCEEDS_CMD_BUFFER_SIZE;
		goto exit;
	}

	status = _read_range_ui(p_dev, read_size);
	if (status < STATUS_OK)
		goto exit;

	previous_stream_id = VL53L5_UI_DEV_STREAM(p_dev);

	_decode_device_info_block(p_dev);

	status = vl53l5_dci_check_device_info(p_dev,
					      previous_stream_id,
					      true,
					      true);
	if (status < STATUS_OK)
		goto exit;

	status = _check_range_header_footer(p_dev);

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_dci_get_device_info(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint32_t count = VL53L5_DEV_INFO_BLOCK_SZ;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (p_dev->host_dev.revision_id == 0x0C)
		count += VL53L5_UI_DUMMY_BYTES;

	status = _read_range_ui(p_dev, count);
	if (status < STATUS_OK)
		goto exit;

	_decode_device_info_block(p_dev);

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_dci_check_device_info(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t last_stream_id,
	bool check_stream_count,
	bool check_data_present)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if ((!check_stream_count) && (!check_data_present)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (HW_TRAP(p_dev)) {
		if (VL53L5_GO2_STATUS_1(p_dev).bytes != 0)

			status = VL53L5_DEV_INFO_MCU_ERROR;
		else

			status = VL53L5_FALSE_DEV_INFO_MCU_ERROR;
		goto exit;
	}
	if (MCU_ERROR(p_dev)) {

		status = VL53L5_DEV_INFO_MCU_ERROR;
		goto exit;
	}

	if (MCU_CP_COLLAPSE(p_dev)) {
		VL53L5_UI_DEV_STREAM(p_dev) = 0xff;

		status = VL53L5_TOO_HIGH_AMBIENT_WARNING;
		goto exit;
	}
	if (MCU_WARNING(p_dev) && p_dev->host_dev.mcu_warnings_on) {

		status = VL53L5_DEV_INFO_MCU_ERROR;
		goto exit;
	}

	if ((check_stream_count && (!STREAM_COUNT_NEW(p_dev, last_stream_id)))
			|| (check_data_present && (!DATA_PRESENT(p_dev)))) {

		status = VL53L5_NO_NEW_RANGE_DATA_ERROR;
		goto exit;
	}

exit:
#ifdef VL53L5_LOG_ENABLE
	if ((status != VL53L5_ERROR_NONE) &&
			(status != VL53L5_NO_NEW_RANGE_DATA_ERROR) &&
			(p_dev != NULL)) {
		trace_print(
			VL53L5_TRACE_LEVEL_ERRORS,
			"Error occurred reading device info block\n");
		trace_print(
			VL53L5_TRACE_LEVEL_ERRORS,
			"go2_status_0.bytes = %02x\n",
			VL53L5_GO2_STATUS_0(p_dev).bytes);
		trace_print(
			VL53L5_TRACE_LEVEL_ERRORS,
			"go2_status_0.bytes = %02x\n",
			VL53L5_GO2_STATUS_1(p_dev).bytes);
		trace_print(
			VL53L5_TRACE_LEVEL_ERRORS,
			"device_status      = %02x\n",
			VL53L5_UI_DEV_STATUS(p_dev));
		trace_print(
			VL53L5_TRACE_LEVEL_ERRORS,
			"ui_stream_count    = %02x\n",
			VL53L5_UI_DEV_STREAM(p_dev));
	}
#endif
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _read_range_ui(struct vl53l5_dev_handle_t *p_dev, uint32_t count)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint8_t *buffer = VL53L5_COMMS_BUFF(p_dev);
	uint16_t index = p_dev->host_dev.range_data_addr.dev_info_start_addr;

	if (VL53L5_COMMS_BUFF_MAX_COUNT(p_dev) < count) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	VL53L5_RESET_COMMS_BUFF(p_dev);

	if (p_dev->host_dev.revision_id == 0x0C)
		index -= 4;

	status = vl53l5_read_multi(p_dev, index, buffer, count);
	if (status < STATUS_OK)
		goto exit;

	VL53L5_COMMS_BUFF_COUNT(p_dev) = count;

	status = vl53l5_dci_swap_buffer_byte_ordering(
		buffer, VL53L5_COMMS_BUFF_COUNT(p_dev));

exit:
	return status;
}

static void _decode_device_info_block(struct vl53l5_dev_handle_t *p_dev)
{
	uint8_t *p_buff = NULL;

	if (p_dev->host_dev.revision_id == 0x0C)
		p_buff = &VL53L5_COMMS_BUFF(p_dev)[VL53L5_UI_DUMMY_BYTES];
	else
		p_buff = VL53L5_COMMS_BUFF(p_dev);

	VL53L5_GO2_STATUS_0(p_dev).bytes =
		vl53l5_decode_uint8_t(BYTE_1, p_buff++);

	VL53L5_GO2_STATUS_1(p_dev).bytes =
		vl53l5_decode_uint8_t(BYTE_1, p_buff++);

	p_dev->host_dev.ui_dev_info.dev_info__device_status =
		vl53l5_decode_uint8_t(BYTE_1, p_buff++);

	VL53L5_UI_DEV_STREAM(p_dev) =
		vl53l5_decode_uint8_t(BYTE_1, p_buff++);
}

static int32_t _check_range_header_footer(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint16_t header_id = 0;
	uint16_t inverse_header_id = 0;
	uint16_t footer_id = 0;
	uint8_t *p_buff = NULL;
	uint32_t start_idx =
		p_dev->host_dev.range_data_addr.data_start_offset_bytes -
		VL53L5_HEADER_FOOTER_BLOCK_SZ;
	uint32_t end_idx = p_dev->host_dev.range_data_addr.data_size_bytes;

	if (p_dev->host_dev.revision_id == 0x0C)
		p_buff = &VL53L5_COMMS_BUFF(p_dev)[VL53L5_UI_DUMMY_BYTES];
	else
		p_buff = VL53L5_COMMS_BUFF(p_dev);

	p_buff += start_idx;

	header_id = vl53l5_decode_uint16_t(BYTE_2, p_buff);
	inverse_header_id = ~header_id;

	p_buff += end_idx;

	footer_id = vl53l5_decode_uint16_t(BYTE_2, p_buff);

	if (inverse_header_id != footer_id) {
		trace_print(VL53L5_TRACE_LEVEL_ERRORS,
			"Error. Header: 0x%04x ~Header: 0x%04x Footer: 0x%04x\n",
			header_id, inverse_header_id, footer_id);
		status = VL53L5_DCI_RANGE_INTEGRITY_ERROR;
	} else {
		trace_print(VL53L5_TRACE_LEVEL_DEBUG,
			"Okay. Header: 0x%04x ~Header: 0x%04x Footer: 0x%04x\n",
			header_id, inverse_header_id, footer_id);
		status = STATUS_OK;
	}

	return status;
}
