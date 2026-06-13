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

#include "page_map_switch.h"
#include "page_map_defs.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_decode_switch.h"
#ifdef VL53L5_PATCH_DATA_ENABLED
#include "vl53l5_tcpm_patch_0_decode_switch.h"
#include "vl53l5_tcpm_patch_1_decode_switch.h"
#endif

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START( \
		VL53L5_TRACE_MODULE_DCI_DECODE, fmt, ##__VA_ARGS__)

#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END( \
		VL53L5_TRACE_MODULE_DCI_DECODE, status, ##__VA_ARGS__)

#define LOG_FUNCTION_END_FMT(status, ...) \
	_LOG_FUNCTION_END_FMT( \
		VL53L5_TRACE_MODULE_DCI_DECODE, status, ##__VA_ARGS__)
int32_t dci_page_map_switch(
	uint16_t idx,
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_dev_handle_t *p_dev,
	uint16_t page_index)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	switch (page_index) {
	case PAGE_DEF__PAGE_VL53L5_DEV:
		status = vl53l5_decode_switch(idx, buffer_size, buffer, p_dev);
		break;
#ifdef VL53L5_PATCH_DATA_ENABLED
	case PAGE_DEF__PAGE_PATCH_0_DEV:
		status = vl53l5_tcpm_patch_0_decode_switch(idx, buffer_size, buffer, p_dev);
		break;
	case PAGE_DEF__PAGE_PATCH_1_DEV:
		status = vl53l5_tcpm_patch_1_decode_switch(idx, buffer_size, buffer, p_dev);
		break;
#endif

	default:
		status = VL53L5_INVALID_PAGE_ERROR;
		break;
	}

	LOG_FUNCTION_END(status);
	return status;

}
