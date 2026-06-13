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

#include "vl53l5_error_handler.h"
#include "vl53l5_commands.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform.h"
#include "vl53l5_trans.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_dci_core.h"
#include "vl53l5_core_map_bh.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_POWER_API, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_POWER_API, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_POWER_API, status, ##__VA_ARGS__)

#define PAGE_SELECT 0x7FFF

#define GO2_STATUS_0 0x6
#define GO2_STATUS_1 0x7

#define BASE_FW_ERROR 0xE0000000

#define FW_ERROR_MIN_ID 128

static int32_t _get_status_struct(
	struct vl53l5_dev_handle_t *p_dev,
	struct common_grp__status_t *p_status,
	uint32_t block_header)
{
	int32_t status = STATUS_OK;
	uint32_t check_block_header = 0;
	uint8_t *p_buff = NULL;

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out;
	}
	if (VL53L5_ISNULL(p_status)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out;
	}

	status = vl53l5_encode_block_headers(p_dev, &block_header, 1, false);
	if (status != STATUS_OK)
		goto out;

	status = vl53l5_execute_command(p_dev, DCI_CMD_ID__GET_PARMS);
	if (status != STATUS_OK)
		goto out;

	if (p_dev->host_dev.revision_id == 0x0C)
		p_buff =  &VL53L5_COMMS_BUFF(p_dev)[VL53L5_UI_DUMMY_BYTES];
	else
		p_buff =  VL53L5_COMMS_BUFF(p_dev);

	check_block_header = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	if (check_block_header != block_header) {
		status = VL53L5_IDX_MISSING_FROM_RETURN_PACKET;
		goto out;
	}

	p_buff += 4;
	p_status->status__group = vl53l5_decode_int16_t(BYTE_2, p_buff);
	p_buff += 2;
	p_status->status__type = vl53l5_decode_int16_t(BYTE_2, p_buff);
	p_buff += 2;
	p_status->status__stage_id = vl53l5_decode_int16_t(BYTE_2, p_buff);
	p_buff += 2;
	p_status->status__debug_0 = vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += 2;
	p_status->status__debug_1 = vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += 2;
	p_status->status__debug_2 = vl53l5_decode_uint16_t(BYTE_2, p_buff);

	trace_print(
		VL53L5_TRACE_LEVEL_DEBUG,
		"group:%i type:%i stage:%i debug0:%i debug1:%i debug2:%i\n",
		p_status->status__group,
		p_status->status__type,
		p_status->status__stage_id,
		p_status->status__debug_0,
		p_status->status__debug_1,
		p_status->status__debug_2);
out:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_check_status_registers(
	struct vl53l5_dev_handle_t *p_dev,
	union dci_union__go2_status_0_go1_u *p_go2_status_0,
	union dci_union__go2_status_1_go1_u *p_go2_status_1,
	bool ignore_warnings,
	uint8_t current_page)
{
	int32_t status = STATUS_OK;
	uint8_t wr_byte = 0;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out_no_page_change;
	}
	if (VL53L5_ISNULL(p_go2_status_0)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out_no_page_change;
	}
	if (VL53L5_ISNULL(p_go2_status_1)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out_no_page_change;
	}

	if (current_page != 0) {
		wr_byte = 0x00;
		status = vl53l5_write_multi(p_dev, PAGE_SELECT, &wr_byte, 1);
		if (status < STATUS_OK)
			goto out_no_page_change;
	}

	status = vl53l5_read_multi(
		p_dev, GO2_STATUS_0, &p_go2_status_0->bytes, 1);
	if (status < STATUS_OK)
		goto out;

	trace_print(
		VL53L5_TRACE_LEVEL_DEBUG,
		"go2 status 0\n"
		"bytes: 0x%x\n"
		"mcu__boot_complete_go1: %x\n"
		"mcu__analog_checks_ok_go1: %x\n"
		"mcu__threshold_triggered_g01: %x\n"
		"mcu__error_flag_go1: %x\n"
		"mcu__ui_range_data_present_go1: %x\n"
		"mcu__ui_new_range_data_avail_go1: %x\n"
		"mcu__ui_update_blocked_go1: %x\n"
		"mcu__hw_trap_flag_go1: %x\n",
		p_go2_status_0->bytes,
		p_go2_status_0->mcu__boot_complete_go1,
		p_go2_status_0->mcu__analog_checks_ok_go1,
		p_go2_status_0->mcu__threshold_triggered_g01,
		p_go2_status_0->mcu__error_flag_go1,
		p_go2_status_0->mcu__ui_range_data_present_go1,
		p_go2_status_0->mcu__ui_new_range_data_avail_go1,
		p_go2_status_0->mcu__ui_update_blocked_go1,
		p_go2_status_0->mcu__hw_trap_flag_go1);

	if ((p_go2_status_0->mcu__error_flag_go1) &&
			(!p_go2_status_0->mcu__hw_trap_flag_go1)) {

		p_go2_status_1->bytes = 0;
		goto out;
	}

	status = vl53l5_read_multi(
		p_dev, GO2_STATUS_1, &p_go2_status_1->bytes, 1);
	if (status < STATUS_OK)
		goto out;

	trace_print(
		VL53L5_TRACE_LEVEL_DEBUG,
		"go2 status 1\n"
		"bytes: 0x%x\n"
		"mcu__avdd_reg_ok_go1: %x\n"
		"mcu__pll_lock_ok_go1: %x\n"
		"mcu__ls_watchdog_pass_go1: %x\n"
		"mcu__warning_flag_go1: %x\n"
		"mcu__cp_collapse_flag_go1: %x\n"
		"mcu__spare0: %x\n"
		"mcu__initial_ram_boot_complete: %x\n"
		"mcu__spare1: %x\n",
		p_go2_status_1->bytes,
		p_go2_status_1->mcu__avdd_reg_ok_go1,
		p_go2_status_1->mcu__pll_lock_ok_go1,
		p_go2_status_1->mcu__ls_watchdog_pass_go1,
		p_go2_status_1->mcu__warning_flag_go1,
		p_go2_status_1->mcu__cp_collapse_flag_go1,
		p_go2_status_1->mcu__spare0,
		p_go2_status_1->mcu__initial_ram_boot_complete,
		p_go2_status_1->mcu__spare1);

	if (p_go2_status_0->mcu__hw_trap_flag_go1 &&
			(p_go2_status_1->bytes == 0)) {
		status = VL53L5_ERROR_FALSE_MCU_ERROR_IN_BANK_CHECK;
		goto out;
	}

	if (!p_go2_status_0->mcu__hw_trap_flag_go1 &&
			ignore_warnings &&
			p_go2_status_1->mcu__warning_flag_go1)
		p_go2_status_1->bytes = 0;

out:
	if (current_page != 0) {

		wr_byte = current_page;

		if (status < STATUS_OK)
			(void)vl53l5_write_multi(
				p_dev, PAGE_SELECT, &wr_byte, 1);
		else
			status = vl53l5_write_multi(
				p_dev, PAGE_SELECT, &wr_byte, 1);
	}

out_no_page_change:

	if (status < STATUS_OK) {
		if (!VL53L5_ISNULL(p_go2_status_0))
			p_go2_status_0->bytes = 0;
		if (!VL53L5_ISNULL(p_go2_status_1))
			p_go2_status_1->bytes = 0;
	}

	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_get_secondary_error_info(
	struct vl53l5_dev_handle_t *p_dev,
	struct common_grp__status_t *p_status)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _get_status_struct(
		p_dev, p_status, VL53L5_DEVICE_ERROR_STATUS_BH);
	if (status != STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_get_secondary_warning_info(
	struct vl53l5_dev_handle_t *p_dev,
	struct common_grp__status_t *p_status)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _get_status_struct(
		p_dev, p_status, VL53L5_DEVICE_WARNING_STATUS_BH);
	if (status != STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_compose_fw_status_code(
	struct vl53l5_dev_handle_t *p_dev,
	struct common_grp__status_t *p_status)
{
	int32_t status = STATUS_OK;
	uint32_t tmp = 0;
	uint16_t *p_reinterpret = NULL;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out;
	}
	if (VL53L5_ISNULL(p_status)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto out;
	}

	if (!p_status->status__type)
		goto out;

	status = BASE_FW_ERROR;

	p_reinterpret = (uint16_t *)&p_status->status__group;
	tmp |= ((uint32_t)(0xff & *p_reinterpret) << 24);

	p_reinterpret = (uint16_t *)&p_status->status__type;
	tmp |= ((uint32_t)(0xff & *p_reinterpret) << 16);

	p_reinterpret = (uint16_t *)&p_status->status__stage_id;
	tmp |= ((uint32_t)(0xff & *p_reinterpret) << 8);

	p_reinterpret = (uint16_t *)&p_status->status__debug_2;
	tmp |= (uint32_t)(0xff & *p_reinterpret);

	status += (int32_t)tmp;
out:
	LOG_FUNCTION_END(status);
	return status;
}
