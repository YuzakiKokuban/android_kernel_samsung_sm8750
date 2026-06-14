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

#include "vl53l5_tcpm_patch_version_size.h"
#include "vl53l5_tcpm_patch_1_core_decode.h"
#include "vl53l5_tcpm_patch_1_core_enum_type.h"
#include "vl53l5_tcpm_patch_1_core_map_idx.h"
#include "vl53l5_tcpm_patch_1_core_dev_path.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform_log.h"

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START( \
		VL53L5_TRACE_MODULE_DCI_DECODE, fmt, ##__VA_ARGS__)

#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END( \
		VL53L5_TRACE_MODULE_DCI_DECODE, status, ##__VA_ARGS__)

#define LOG_FUNCTION_END_FMT(status, ...) \
	_LOG_FUNCTION_END_FMT( \
		VL53L5_TRACE_MODULE_DCI_DECODE, status, ##__VA_ARGS__)
static int32_t _decode_vl53l5_tcpm_patch_1_tcpm_patch_version_grp_debug_word(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct tcpm_patch_version_grp__debug_word_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_TCPM_PATCH_VERSION_GRP_DEBUG_WORD_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->tcpm_patch_map__debug__word =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_vl53l5_tcpm_patch_1_tcpm_patch_version_grp_debug2_word(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct tcpm_patch_version_grp__debug2_word_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_TCPM_PATCH_VERSION_GRP_DEBUG2_WORD_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->tcpm_patch_map__debug2__word =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_tcpm_patch_1_core_decode_cmd(
	uint16_t idx,
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	switch (idx) {
	case VL53L5_TCPM_1_TCPM_PATCH_MAP_DEBUG_WORD_IDX:
		status =
		_decode_vl53l5_tcpm_patch_1_tcpm_patch_version_grp_debug_word(
			buffer_size,
			buffer,
			&VL53L5_TCPM_PATCH_1_TCPM_PATCH_MAP_DEBUG_WORD(p_dev));
		break;
	case VL53L5_TCPM_1_TCPM_PATCH_MAP_DEBUG2_WORD_IDX:
		status =
		_decode_vl53l5_tcpm_patch_1_tcpm_patch_version_grp_debug2_word(
			buffer_size,
			buffer,
			&VL53L5_TCPM_PATCH_1_TCPM_PATCH_MAP_DEBUG2_WORD(p_dev));
		break;
	default:
		status = VL53L5_INVALID_IDX_DECODE_CMD_ERROR;
		break;
	}

	LOG_FUNCTION_END(status);
	return status;

}
