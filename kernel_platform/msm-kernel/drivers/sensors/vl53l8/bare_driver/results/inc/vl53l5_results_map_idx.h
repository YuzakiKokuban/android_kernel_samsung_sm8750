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

#ifndef __VL53L5_RESULTS_MAP_IDX_H__
#define __VL53L5_RESULTS_MAP_IDX_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_RNG_META_DATA_IDX \
	((uint16_t) 0x59d8)

#define DEV_RNG_COMMON_DATA_IDX \
	((uint16_t) 0x59e4)

#define DEV_RNG_TIMING_DATA_IDX \
	((uint16_t) 0x59ec)

#define DEV_RNG_PER_ZONE_DATA_AMB_RATE_KCPS_PER_SPAD_IDX \
	((uint16_t) 0x59f8)
#define DEV_RNG_PER_ZONE_DATA_IDX \
	((uint16_t) 0x59f8)

#define DEV_RNG_PER_ZONE_DATA_RNG__EFFECTIVE_SPAD_COUNT_IDX \
	((uint16_t) 0x5b08)

#define DEV_RNG_PER_ZONE_DATA_AMB_DMAX_MM_IDX \
	((uint16_t) 0x5c18)

#define DEV_RNG_PER_ZONE_DATA_SILICON_TEMP_DEGC__START_IDX \
	((uint16_t) 0x5ca0)

#define DEV_RNG_PER_ZONE_DATA_SILICON_TEMP_DEGC__END_IDX \
	((uint16_t) 0x5ce4)

#define DEV_RNG_PER_ZONE_DATA_RNG__NO_OF_TARGETS_IDX \
	((uint16_t) 0x5d28)

#define DEV_RNG_PER_ZONE_DATA_RNG__ZONE_ID_IDX \
	((uint16_t) 0x5d6c)

#define DEV_RNG_PER_ZONE_DATA_RNG__SEQUENCE_IDX_IDX \
	((uint16_t) 0x5db0)

#define DEV_RNG_PER_TARGET_DATA_PEAK_RATE_KCPS_PER_SPAD_IDX \
	((uint16_t) 0x5df4)
#define DEV_RNG_PER_TARGET_DATA_IDX \
	((uint16_t) 0x5df4)

#define DEV_RNG_PER_TARGET_DATA_MEDIAN_PHASE_IDX \
	((uint16_t) 0x6234)

#define DEV_RNG_PER_TARGET_DATA_RATE_SIGMA_KCPS_PER_SPAD_IDX \
	((uint16_t) 0x6674)

#define DEV_RNG_PER_TARGET_DATA_TARGET_ZSCORE_IDX \
	((uint16_t) 0x6ab4)

#define DEV_RNG_PER_TARGET_DATA_RANGE_SIGMA_MM_IDX \
	((uint16_t) 0x6cd4)

#define DEV_RNG_PER_TARGET_DATA_MEDIAN_RANGE_MM_IDX \
	((uint16_t) 0x6ef4)

#define DEV_RNG_PER_TARGET_DATA_START_RANGE_MM_IDX \
	((uint16_t) 0x7114)

#define DEV_RNG_PER_TARGET_DATA_END_RANGE_MM_IDX \
	((uint16_t) 0x7334)

#define DEV_RNG_PER_TARGET_DATA_MIN_RANGE_DELTA_MM_IDX \
	((uint16_t) 0x7554)

#define DEV_RNG_PER_TARGET_DATA_MAX_RANGE_DELTA_MM_IDX \
	((uint16_t) 0x7664)

#define DEV_RNG_PER_TARGET_DATA_TARGET_REFLECTANCE_EST_PC_IDX \
	((uint16_t) 0x7774)

#define DEV_RNG_PER_TARGET_DATA_TARGET_STATUS_IDX \
	((uint16_t) 0x7884)

#define DEV_REF_TIMING_DATA_IDX \
	((uint16_t) 0x7994)

#define DEV_REF_CHANNEL_DATA_IDX \
	((uint16_t) 0x79a0)

#define DEV_REF_TARGET_DATA_IDX \
	((uint16_t) 0x79b0)

#define DEV_SHARPENER_TARGET_DATA_SHARPENER__GROUP_INDEX_IDX \
	((uint16_t) 0x79cc)
#define DEV_SHARPENER_TARGET_DATA_IDX \
	((uint16_t) 0x79cc)

#define DEV_SHARPENER_TARGET_DATA_SHARPENER__CONFIDENCE_IDX \
	((uint16_t) 0x7adc)

#define DEV_ZONE_THRESH_STATUS_ZONE_THRESH_STATUS_BYTES_IDX \
	((uint16_t) 0xafb8)
#define DEV_ZONE_THRESH_STATUS_IDX \
	((uint16_t) 0xafb8)

#define DEV_DYN_XTALK_OP_PERSISTENT_DATA_IDX \
	((uint16_t) 0xafc0)

#ifdef __cplusplus
}
#endif

#endif
