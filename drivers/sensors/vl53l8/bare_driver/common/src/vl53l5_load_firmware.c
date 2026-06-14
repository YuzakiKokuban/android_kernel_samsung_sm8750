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

#include "vl53l5_load_firmware.h"
#include "vl53l5_platform.h"
#include "vl53l5_register_utils.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_dci_helpers.h"
#include "dci_ui_memory_defs.h"
#include "vl53l5_platform_user_config.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_LOAD_FIRMWARE, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_LOAD_FIRMWARE, fmt, \
			    ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_LOAD_FIRMWARE, status, \
			  ##__VA_ARGS__)

#define CHECK_FOR_TIMEOUT(status, p_dev, start_ms, end_ms, timeout_ms) \
	(status = vl53l5_check_for_timeout(\
		(p_dev), start_ms, end_ms, timeout_ms))

#define MAX_FW_FILE_SIZE 100000
#define WRITE_CHUNK_SIZE(p_dev) VL53L5_COMMS_BUFF_MAX_COUNT(p_dev)

static int32_t _write_byte(
	struct vl53l5_dev_handle_t *p_dev, uint16_t address, uint8_t value)
{
	return vl53l5_write_multi(p_dev, address, &value, 1);
}

static int32_t _read_byte(
	struct vl53l5_dev_handle_t *p_dev, uint16_t address, uint8_t *p_value)
{
	return vl53l5_read_multi(p_dev, address, p_value, 1);
}

static int32_t _write_page(struct vl53l5_dev_handle_t *p_dev, uint8_t *p_buffer,
			   uint16_t page_offset, uint32_t page_size,
			   uint32_t max_chunk_size, uint32_t *p_write_count)
{
	int32_t status = STATUS_OK;
	uint32_t write_size = 0;
	uint32_t remainder_size = 0;
	uint8_t *p_write_buff = NULL;

	if ((page_offset + max_chunk_size) < page_size)
		write_size = max_chunk_size;
	else
		write_size = page_size - page_offset;

	if (*p_write_count > p_dev->host_dev.fw_buff_count) {

		p_write_buff = p_dev->host_dev.p_comms_buff;
		memset(p_write_buff, 0, write_size);
	} else {
		if ((p_dev->host_dev.fw_buff_count - *p_write_count)
				 < write_size) {

			p_write_buff = p_dev->host_dev.p_comms_buff;
			remainder_size =
				p_dev->host_dev.fw_buff_count - *p_write_count;
			memcpy(p_write_buff,
			       p_buffer + *p_write_count,
			       remainder_size);
			memset(p_write_buff + remainder_size,
			       0,
			       write_size - remainder_size);
		} else {

			p_write_buff = p_buffer + *p_write_count;
		}
	}
	status = vl53l5_write_multi(p_dev, page_offset, p_write_buff,
				    write_size);
	if (status < STATUS_OK)
		goto exit;

	(*p_write_count) += write_size;

exit:
	return status;
}

static int32_t _write_data_to_ram(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	uint16_t tcpm_page_offset = p_dev->host_dev.patch_data.tcpm_page_offset;
	uint8_t tcpm_page = p_dev->host_dev.patch_data.tcpm_page;
	uint8_t current_page = 0;
	uint32_t tcpm_page_size = 0;
	uint32_t write_count = 0;
	uint32_t tcpm_offset = p_dev->host_dev.patch_data.tcpm_offset;
	uint32_t tcpm_size = p_dev->host_dev.patch_data.tcpm_size;
	uint32_t current_size = tcpm_size;
	uint8_t *write_buff = NULL;

	p_dev->host_dev.fw_buff_count = tcpm_size;

	write_buff = &p_dev->host_dev.p_fw_buff[tcpm_offset];

	LOG_FUNCTION_START("");

	p_dev->host_dev.firmware_load = true;

	for (current_page = tcpm_page; current_page < 12; current_page++) {
		status = _write_byte(p_dev, 0x7FFF, current_page);
		if (status < STATUS_OK)
			goto exit;

		if (current_page == 9)
			tcpm_page_size = 0x8000;
		if (current_page == 10)
			tcpm_page_size = 0x8000;
		if (current_page == 11)
			tcpm_page_size = 0x5000;
		if (current_size < tcpm_page_size)
			tcpm_page_size = current_size;

		for (tcpm_page_offset = 0; tcpm_page_offset < tcpm_page_size;
				tcpm_page_offset += WRITE_CHUNK_SIZE(p_dev)) {
			status = _write_page(p_dev, write_buff,
				tcpm_page_offset, tcpm_page_size,
				WRITE_CHUNK_SIZE(p_dev), &write_count);
			if (status != STATUS_OK)
				goto exit;
		}

		if (write_count == tcpm_size)
			break;
		current_size -= write_count;
	}

exit:
	p_dev->host_dev.firmware_load = false;

	LOG_FUNCTION_END(status);
	return status;
}

static void _decode_patch_struct(struct vl53l5_dev_handle_t *p_dev)
{
	uint8_t *p_buff = p_dev->host_dev.p_fw_buff;

	LOG_FUNCTION_START("");

	p_dev->host_dev.patch_data.patch_ver_major =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "p_dev->host_dev.patch_data.patch_ver_major: 0x%x\n",
		    p_dev->host_dev.patch_data.patch_ver_major);

	p_dev->host_dev.patch_data.patch_ver_minor =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "p_dev->host_dev.patch_data.patch_ver_minor: 0x%x\n",
		    p_dev->host_dev.patch_data.patch_ver_minor);

	p_dev->host_dev.patch_data.patch_ver_build =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "p_dev->host_dev.patch_data.patch_ver_build: 0x%x\n",
		    p_dev->host_dev.patch_data.patch_ver_build);

	p_dev->host_dev.patch_data.patch_ver_revision =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "p_dev->host_dev.patch_data.patch_ver_revision: 0x%x\n",
		    p_dev->host_dev.patch_data.patch_ver_revision);

	p_buff += BYTE_4;

	p_dev->host_dev.patch_data.patch_offset =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.patch_offset: 0x%x\n",
		p_dev->host_dev.patch_data.patch_offset);

	p_dev->host_dev.patch_data.patch_size =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.patch_size: 0x%x\n",
		p_dev->host_dev.patch_data.patch_size);

	p_dev->host_dev.patch_data.patch_checksum =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.patch_checksum: 0x%x\n",
		p_dev->host_dev.patch_data.patch_checksum);

	p_dev->host_dev.patch_data.tcpm_offset =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.tcpm_offset: 0x%x\n",
		p_dev->host_dev.patch_data.tcpm_offset);

	p_dev->host_dev.patch_data.tcpm_size =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.tcpm_size: 0x%x\n",
		p_dev->host_dev.patch_data.tcpm_size);

	p_dev->host_dev.patch_data.tcpm_page =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.tcpm_page: 0x%x\n",
		p_dev->host_dev.patch_data.tcpm_page);

	p_dev->host_dev.patch_data.tcpm_page_offset =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.tcpm_page_offset: 0x%x\n",
		p_dev->host_dev.patch_data.tcpm_page_offset);

	p_buff += 48;

	p_dev->host_dev.patch_data.checksum_en_offset =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.checksum_en_offset: 0x%x\n",
		p_dev->host_dev.patch_data.checksum_en_offset);

	p_dev->host_dev.patch_data.checksum_en_size =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.checksum_en_size: 0x%x\n",
		p_dev->host_dev.patch_data.checksum_en_size);

	p_dev->host_dev.patch_data.checksum_en_page =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.checksum_en_page: 0x%x\n",
		p_dev->host_dev.patch_data.checksum_en_page);

	p_dev->host_dev.patch_data.checksum_en_page_offset =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.checksum_en_page_offset: 0x%x\n",
		p_dev->host_dev.patch_data.checksum_en_page_offset);

	p_dev->host_dev.patch_data.patch_code_offset =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.patch_code_offset: 0x%x\n",
		p_dev->host_dev.patch_data.patch_code_offset);

	p_dev->host_dev.patch_data.patch_code_size =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.patch_code_size: 0x%x\n",
		p_dev->host_dev.patch_data.patch_code_size);

	p_dev->host_dev.patch_data.patch_code_page =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.patch_code_page: 0x%x\n",
		p_dev->host_dev.patch_data.patch_code_page);

	p_dev->host_dev.patch_data.patch_code_page_offset =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		"p_dev->host_dev.patch_data.patch_code_page_offset: 0x%x\n",
		p_dev->host_dev.patch_data.patch_code_page_offset);

	LOG_FUNCTION_END(0);
}

static int32_t _write_patch_code(
		struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint16_t page =
		(uint16_t)p_dev->host_dev.patch_data.patch_code_page;
	uint16_t addr =
		(uint16_t)p_dev->host_dev.patch_data.patch_code_page_offset;
	uint32_t size = p_dev->host_dev.patch_data.patch_code_size;
	uint32_t offset = p_dev->host_dev.patch_data.patch_code_offset;
	uint8_t *reg_data = &p_dev->host_dev.p_fw_buff[offset];

	LOG_FUNCTION_START("");

	trace_print(VL53L5_TRACE_LEVEL_DEBUG, "page: 0x%x addr: 0x%x\n",
		    page, addr);

	status = _write_byte(p_dev, 0x7FFF, page);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_write_multi(p_dev, addr, reg_data, size);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _write_checksum_en(
		struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint16_t page =
		(uint16_t)p_dev->host_dev.patch_data.checksum_en_page;
	uint16_t addr =
		(uint16_t)p_dev->host_dev.patch_data.checksum_en_page_offset;
	uint32_t size = p_dev->host_dev.patch_data.checksum_en_size;
	uint32_t offset = p_dev->host_dev.patch_data.checksum_en_offset;
	uint8_t *reg_data = &p_dev->host_dev.p_fw_buff[offset];

	LOG_FUNCTION_START("");

	trace_print(VL53L5_TRACE_LEVEL_DEBUG, "page: 0x%x addr: 0x%x\n",
		    page, addr);

	status = _write_byte(p_dev, 0x7FFF, page);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_write_multi(p_dev, addr, reg_data, size);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _check_fw_checksum(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint32_t checksum = 0;
	uint32_t expected_checksum = p_dev->host_dev.patch_data.patch_checksum;
	uint8_t data[4] = {0};
	uint16_t ui_addr = (uint16_t)(DCI_UI__FIRMWARE_CHECKSUM_IDX & 0xFFFF);

	LOG_FUNCTION_START("");

	data[0] = 0;
	status = vl53l5_read_multi(p_dev, ui_addr, data, 4);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_dci_swap_buffer_byte_ordering(data, BYTE_4);
	if (status < STATUS_OK)
		goto exit;

	checksum = vl53l5_decode_uint32_t(BYTE_4, data);

	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Expected Checksum: 0x%x Actual Checksum: 0x%x\n",
		    expected_checksum, checksum);

	if (checksum != expected_checksum) {
		status = VL53L5_ERROR_INIT_FW_CHECKSUM;
		goto exit;
	}

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _enable_host_access_to_go1_async(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint32_t start_time_ms   = 0;
	uint32_t current_time_ms = 0;
	uint8_t m_status = 0;

	LOG_FUNCTION_START("");

	if (p_dev->host_dev.revision_id == 2) {
		status = _write_byte(p_dev, 0x7FFF, 0x02);
		if (status < STATUS_OK)
			goto exit;
		status = _write_byte(p_dev, 0x03, 0x12);
		if (status < STATUS_OK)
			goto exit;
		status = _write_byte(p_dev, 0x7FFF, 0x01);
		if (status < STATUS_OK)
			goto exit;
	} else {
		status = _write_byte(p_dev, 0x7FFF, 0x01);
		if (status < STATUS_OK)
			goto exit;
		status = _write_byte(p_dev, 0x06, 0x01);
		if (status < STATUS_OK)
			goto exit;
	}

	m_status = 0;
	status = vl53l5_get_tick_count(p_dev, &start_time_ms);
	if (status < STATUS_OK)
		goto exit;

	while ((m_status & 0x04) == 0) {
		status = _read_byte(p_dev, 0x21, &m_status);
		if (status < STATUS_OK)
			goto exit;

		status = vl53l5_get_tick_count(p_dev, &current_time_ms);
		if (status < STATUS_OK)
			goto exit;

		CHECK_FOR_TIMEOUT(
			status, p_dev, start_time_ms, current_time_ms,
			VL53L5_BOOT_COMPLETION_POLLING_TIMEOUT_MS);
		if (status < STATUS_OK) {
			trace_print(
			VL53L5_TRACE_LEVEL_ERRORS,
			"ERROR: timeout waiting for mcu idle m_status %02x\n",
			m_status);
			status = VL53L5_ERROR_MCU_IDLE_TIMEOUT;
			goto exit;
		}
	}

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0C, 0x01);
	if (status < STATUS_OK)
		goto exit;
exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _reset_mcu_and_wait_boot(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint8_t u_start[] = {0, 0, 0x42, 0};

	LOG_FUNCTION_START("");

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_write_multi(p_dev, 0x114, u_start, 4);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0B, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0C, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0B, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_wait_mcu_boot(p_dev, VL53L5_BOOT_STATE_HIGH,
				      0, VL53L5_MCU_BOOT_WAIT_DELAY);

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _disable_pll(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x400F, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21A, 0x43);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21A, 0x03);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21A, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21A, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x219, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21B, 0x00);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _set_to_power_on_status(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x101, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x102, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x010a, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x4002, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x4002, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x103, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x010a, 0x03);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _wait_for_boot_complete_before_fw_load(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _enable_host_access_to_go1_async(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _set_to_power_on_status(p_dev);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _wait_for_boot_complete_after_fw_load(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _disable_pll(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _reset_mcu_and_wait_boot(p_dev);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_load_firmware(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	trace_print(VL53L5_TRACE_LEVEL_INFO, "\n\n#### load_firmware ####\n\n");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_FW_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_FW_BUFF_NOT_FOUND;
		goto exit;
	}
	if (VL53L5_FW_BUFF_ISEMPTY(p_dev)) {
		status = VL53L5_ERROR_FW_BUFF_NOT_FOUND;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	_decode_patch_struct(p_dev);

	status = _wait_for_boot_complete_before_fw_load(p_dev);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = _write_data_to_ram(p_dev);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = _write_patch_code(p_dev);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = _write_checksum_en(p_dev);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = _wait_for_boot_complete_after_fw_load(p_dev);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = _write_byte(p_dev, 0x7FFF, 0x02);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = _check_fw_checksum(p_dev);
	if (status < STATUS_OK)
		goto exit_change_page;

exit_change_page:
	if (status < STATUS_OK)

		(void)_write_byte(p_dev, 0x7FFF, 0x02);

exit:
	LOG_FUNCTION_END(status);
	return status;
}
