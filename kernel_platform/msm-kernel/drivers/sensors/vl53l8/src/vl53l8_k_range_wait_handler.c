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
#include "vl53l8_k_module_data.h"
#include "vl53l8_k_error_codes.h"
#include "vl53l8_k_logging.h"
#include "vl53l8_k_range_wait_handler.h"
#include "vl53l5_platform.h"

int vl53l8_k_check_data_ready(struct vl53l8_k_module_t *p_module, int *p_ready)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	p_module->polling_count++;

	status = vl53l5_check_data_ready(&p_module->stdev);
	if (status == VL53L5_ERROR_NONE) {
		*p_ready = 1;
	} else {
		*p_ready = 0;
		if (status == VL53L5_NO_NEW_RANGE_DATA_ERROR ||
		    status == VL53L5_TOO_HIGH_AMBIENT_WARNING)
			status = 0;
		else

			status = vl53l5_read_device_error(&p_module->stdev,
							  status);
	}

	if (status != VL53L5_ERROR_NONE)
		status = vl53l5_read_device_error(&p_module->stdev, status);

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_k_check_for_timeout(struct vl53l8_k_module_t *p_module,
			      int timeout_ms,
			      int *p_timeout_occurred)
{
	int status = VL53L5_ERROR_NONE;
	unsigned int current_time_ms = 0;

	LOG_FUNCTION_START("");

	status = vl53l5_get_tick_count(&p_module->stdev, &current_time_ms);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	status = vl53l5_check_for_timeout(
		&p_module->stdev, p_module->polling_start_time_ms,
		current_time_ms, timeout_ms);
	if (status == VL53L5_ERROR_TIME_OUT)
		*p_timeout_occurred = 1;
	else
		*p_timeout_occurred = 0;

out:

	if (status != VL53L5_ERROR_NONE)
		status = vl53l5_read_device_error(&p_module->stdev, status);

	LOG_FUNCTION_END(status);
	return status;
}
