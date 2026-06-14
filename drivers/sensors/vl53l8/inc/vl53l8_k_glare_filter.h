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

#ifndef _VL53L8_K_GLARE_FILTER_H_
#define _VL53L8_K_GLARE_FILTER_H_

#include "vl53l5_platform_user_data.h"
#include "vl53l8_k_user_data.h"

#define VL53L8_K_D16_CONF_00PC 1

#define VL53L8_K_D16_CONF_14PC 2

#define VL53L8_K_D16_CONF_29PC 3

#define VL53L8_K_GAIN_SCALE_FACTOR 64U

#define VL53L8_K_GSF_x_GSF \
	(VL53L8_K_GAIN_SCALE_FACTOR * VL53L8_K_GAIN_SCALE_FACTOR)

#define VL53L8_K_REFLECTANCE_X256 (13 * 256)

#define VL53L8_K_RANGE 100

#define VL53L8_K_SIGNAL_X16 (16 * 30000)

#define VL53L8_K_GLARE_LUT_SIZE 8

#define VL53L8_K_Y_SCALE_FACTOR  64

#define VL53L8_K_PEAK_RATE_FRACTIONAL_BITS 11

#define VL53L8_K_MEDIAN_RANGE_FRACTIONAL_BITS 2

struct vl53l8_k_glare_filter_tuning_t {

	int32_t range_min_clip;
	int32_t max_filter_range;
	bool remove_glare_targets;

	int32_t range_x4[VL53L8_K_GLARE_LUT_SIZE];

	uint32_t refl_thresh_x256[VL53L8_K_GLARE_LUT_SIZE];
	uint32_t lut_range[VL53L8_K_GLARE_LUT_SIZE];
	uint32_t threshold_numerator[VL53L8_K_GLARE_LUT_SIZE];
};

int32_t vl53l8_k_glare_filter(
	struct vl53l8_k_glare_filter_tuning_t *p_tuning,
	struct vl53l5_range_data_t *p_results);

void vl53l8_k_glare_detect_init(struct vl53l8_k_glare_filter_tuning_t *p_tuning);

#endif
