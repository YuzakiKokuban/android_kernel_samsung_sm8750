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

#include "vl53l5_patch_api_core.h"
#include "vl53l5_api_core.h"
#include "vl53l5_dci_decode.h"
#include "vl53l5_patch_core_map_bh.h"
#include "vl53l5_tcpm_patch_0_core_map_bh.h"
#include "vl53l5_tcpm_patch_1_core_map_bh.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_patch_error_codes.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_patch_version.h"
#include "vl53l5_patch_version_size.h"
#include "vl53l5_tcpm_patch_version_size.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_API, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)
#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_API, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_API, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L5_TRACE_MODULE_API, status, fmt,\
			      ##__VA_ARGS__)
#define LOG_FUNCTION_END_FLUSH(status, ...) \
	do { \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_API, status, ##__VA_ARGS__);\
	_FLUSH_TRACE_TO_OUTPUT();\
	} while (0)

static int32_t _decode_patch_version_structs(
				struct vl53l5_dev_handle_t *p_dev,
				struct vl53l5_patch_version_t *p_version);

int32_t vl53l5_assign_core(struct vl53l5_dev_handle_t *p_dev,
			   struct vl53l5_core_data_t *p_core)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_core)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	p_dev->host_dev.ppatch_core_dev = &p_core->patch;
	p_dev->host_dev.ptcpm_patch_0_core_dev = &p_core->tcpm_0_patch;
	p_dev->host_dev.ptcpm_patch_1_core_dev = &p_core->tcpm_1_patch;

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_unassign_core(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	p_dev->host_dev.ppatch_core_dev = NULL;
	p_dev->host_dev.ptcpm_patch_0_core_dev = NULL;
	p_dev->host_dev.ptcpm_patch_1_core_dev = NULL;
exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_get_patch_version(struct vl53l5_dev_handle_t *p_dev,
				 struct vl53l5_patch_version_t *p_version)

{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t block_headers[] = {
		VL53L5_PATCH_VERSION_BH,
		VL53L5_TCPM_0_PAGE_BH,
		VL53L5_TCPM_0_TCPM_PATCH_VERSION_BH,
	};
	uint32_t num_bh = sizeof(block_headers) / sizeof(block_headers[0]);

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_version)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = vl53l5_get_device_parameters(p_dev, block_headers, num_bh);
	if (status < STATUS_OK)
		goto exit;

	status = _decode_patch_version_structs(p_dev, p_version);

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

static int32_t _decode_patch_version_structs(
				struct vl53l5_dev_handle_t *p_dev,
				struct vl53l5_patch_version_t *p_version)
{
	int32_t status = STATUS_OK;
	uint8_t *p_buff = NULL;
	uint32_t block_header = 0;
	const uint32_t end_of_data_footer_index = 28;

	p_buff =  &VL53L5_COMMS_BUFF(p_dev)[VL53L5_UI_DUMMY_BYTES];

	if ((p_buff[end_of_data_footer_index] & 0x0f) !=
			DCI_BH__P_TYPE__END_OF_DATA) {
		status = VL53L5_DCI_END_BLOCK_ERROR;

		goto exit;
	}

	p_buff += 8;

	block_header = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	if (block_header != VL53L5_PATCH_VERSION_BH) {
		status = VL53L5_PATCH_VERSION_IDX_NOT_PRESENT;
		goto exit;
	}
	p_buff += 4;
	p_version->patch_version.ver_major =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	p_version->patch_version.ver_minor =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Patch version: %u.%u\n",
		    p_version->patch_version.ver_major,
		    p_version->patch_version.ver_minor);

	p_buff += 4;

	block_header = vl53l5_decode_uint32_t(4, p_buff);
	if (block_header != VL53L5_TCPM_0_TCPM_PATCH_VERSION_BH) {
		status = VL53L5_TCPM_VERSION_IDX_NOT_PRESENT;
		goto exit;
	}
	p_buff += 4;
	p_version->tcpm_version.ver_major =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	p_version->tcpm_version.ver_minor =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;

	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "TCPM version: %u.%u\n",
		    p_version->tcpm_version.ver_major,
		    p_version->tcpm_version.ver_minor);

	p_version->patch_driver.ver_major = VL53L5_PATCH_VERSION_MAJOR;
	p_version->patch_driver.ver_minor = VL53L5_PATCH_VERSION_MINOR;
	p_version->patch_driver.ver_build = VL53L5_PATCH_VERSION_BUILD;
	p_version->patch_driver.ver_revision = VL53L5_PATCH_VERSION_REVISION;

	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Patch driver version: %u.%u.%u.%u\n",
		    p_version->patch_driver.ver_major,
		    p_version->patch_driver.ver_minor,
		    p_version->patch_driver.ver_build,
		    p_version->patch_driver.ver_revision);

exit:
	return status;
}
