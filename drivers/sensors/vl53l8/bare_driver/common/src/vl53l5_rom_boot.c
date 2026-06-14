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

#include "vl53l5_rom_boot.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform.h"
#include "vl53l5_register_utils.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_POWER_API, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_POWER_API, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_POWER_API, status, ##__VA_ARGS__)

#define CHECK_FOR_TIMEOUT(status, p_dev, start_ms, end_ms, timeout_ms) \
	(status = vl53l5_check_for_timeout(\
		(p_dev), start_ms, end_ms, timeout_ms))

#define MCU_FIRST_BOOT_COMPLETE(p_dev) \
	(VL53L5_GO2_STATUS_1(p_dev).mcu__initial_ram_boot_complete == 1)

#define MCU_FIRST_BOOT_NOT_COMPLETE(p_dev) \
	(VL53L5_GO2_STATUS_1(p_dev).mcu__initial_ram_boot_complete == 0)

#define MCU_FIRST_BOOT_COMPLETE_REVISION_C(p_dev) \
	(VL53L5_GO2_STATUS_1(p_dev).mcu__spare0 == 1)

#define MCU_FIRST_BOOT_NOT_COMPLETE_CUT_1_4(p_dev) \
	(VL53L5_GO2_STATUS_1(p_dev).mcu__spare0 == 0)

#define DEFAULT_PAGE 2
#define GO2_PAGE 0
#define DEVICE_ID 0x0
#define REVISION_ID 0x1
#define GO2_STATUS_1 0x07
#define BYTE_SIZE_1 1

static int32_t _write_byte(
	struct vl53l5_dev_handle_t *p_dev, uint16_t address, uint8_t value)
{
	return vl53l5_write_multi(p_dev, address, &value, BYTE_SIZE_1);
}

static int32_t _check_device_booted(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	VL53L5_GO2_STATUS_1(p_dev).bytes = 0;

	status = vl53l5_read_multi(p_dev, GO2_STATUS_1,
			&VL53L5_GO2_STATUS_1(p_dev).bytes, BYTE_SIZE_1);
	if (status != STATUS_OK)
		goto exit;

	if (p_dev->host_dev.revision_id == 0x0c) {
		if (MCU_FIRST_BOOT_COMPLETE_REVISION_C(p_dev))
			p_dev->host_dev.device_booted = true;
		else
			p_dev->host_dev.device_booted = false;
	} else {
		if (MCU_FIRST_BOOT_COMPLETE(p_dev))
			p_dev->host_dev.device_booted = true;
		else
			p_dev->host_dev.device_booted = false;
	}

exit:
	return status;
}

static int32_t _check_rom_firmware_boot(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	status = vl53l5_wait_mcu_boot(p_dev, VL53L5_BOOT_STATE_HIGH, 0, 0);

	(void)_write_byte(p_dev, 0x000E, 0x01);

	return status;
}

int32_t vl53l5_check_rom_firmware_boot(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = vl53l5_set_page(p_dev, GO2_PAGE);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_read_multi(
		p_dev, DEVICE_ID, &p_dev->host_dev.device_id, BYTE_SIZE_1);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = vl53l5_read_multi(
		p_dev, REVISION_ID, &p_dev->host_dev.revision_id, BYTE_SIZE_1);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = _check_device_booted(p_dev);
	if (status < STATUS_OK)
		goto exit_change_page;

	if (p_dev->host_dev.device_booted == true)
		goto exit_change_page;

	if ((p_dev->host_dev.device_id == 0xF0) &&
	    (p_dev->host_dev.revision_id == 0x02 ||
	     p_dev->host_dev.revision_id == 0x0C)) {
		trace_print(VL53L5_TRACE_LEVEL_INFO,
			    "device id 0x%x revision id 0x%x\n",
			    p_dev->host_dev.device_id,
			    p_dev->host_dev.revision_id);
		status = _check_rom_firmware_boot(p_dev);
		if (status < STATUS_OK)
			goto exit_change_page;
	} else {
		trace_print(VL53L5_TRACE_LEVEL_ERRORS,
			    "Unsupported device id 0x%x revision id 0x%x\n",
			    p_dev->host_dev.device_id,
			    p_dev->host_dev.revision_id);

		status = VL53L5_UNKNOWN_SILICON_REVISION;
		goto exit_change_page;
	}

exit_change_page:

	if (status != STATUS_OK)
		(void)vl53l5_set_page(p_dev, DEFAULT_PAGE);
	else
		status = vl53l5_set_page(p_dev, DEFAULT_PAGE);

exit:
	LOG_FUNCTION_END(status);
	return status;
}
