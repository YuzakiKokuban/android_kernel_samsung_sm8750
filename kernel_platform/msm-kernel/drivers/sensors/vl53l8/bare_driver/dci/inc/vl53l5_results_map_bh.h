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

#ifndef __VL53L5_RESULTS_MAP_BH_H__
#define __VL53L5_RESULTS_MAP_BH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_RNG_META_DATA_BH \
	((uint32_t) 0x59d800c0U)
#define VL53L5_RNG_COMMON_DATA_BH \
	((uint32_t) 0x59e40080U)
#define VL53L5_RNG_TIMING_DATA_BH \
	((uint32_t) 0x59ec00c0U)
#define VL53L5_RNG_PER_ZONE_DATA_AMB_RATE_KCPS_PER_SPAD_BH \
	((uint32_t) 0x59f80444U)
#define VL53L5_RNG_PER_ZONE_DATA_RNG__EFFECTIVE_SPAD_COUNT_BH \
	((uint32_t) 0x5b080444U)
#define VL53L5_RNG_PER_ZONE_DATA_AMB_DMAX_MM_BH \
	((uint32_t) 0x5c180442U)
#define VL53L5_RNG_PER_ZONE_DATA_SILICON_TEMP_DEGC__START_BH \
	((uint32_t) 0x5ca00441U)
#define VL53L5_RNG_PER_ZONE_DATA_SILICON_TEMP_DEGC__END_BH \
	((uint32_t) 0x5ce40441U)
#define VL53L5_RNG_PER_ZONE_DATA_RNG__NO_OF_TARGETS_BH \
	((uint32_t) 0x5d280441U)
#define VL53L5_RNG_PER_ZONE_DATA_RNG__ZONE_ID_BH \
	((uint32_t) 0x5d6c0441U)
#define VL53L5_RNG_PER_ZONE_DATA_RNG__SEQUENCE_IDX_BH \
	((uint32_t) 0x5db00441U)
#define VL53L5_RNG_PER_TARGET_DATA_PEAK_RATE_KCPS_PER_SPAD_BH \
	((uint32_t) 0x5df41104U)
#define VL53L5_RNG_PER_TARGET_DATA_MEDIAN_PHASE_BH \
	((uint32_t) 0x62341104U)
#define VL53L5_RNG_PER_TARGET_DATA_RATE_SIGMA_KCPS_PER_SPAD_BH \
	((uint32_t) 0x66741104U)
#define VL53L5_RNG_PER_TARGET_DATA_TARGET_ZSCORE_BH \
	((uint32_t) 0x6ab41102U)
#define VL53L5_RNG_PER_TARGET_DATA_RANGE_SIGMA_MM_BH \
	((uint32_t) 0x6cd41102U)
#define VL53L5_RNG_PER_TARGET_DATA_MEDIAN_RANGE_MM_BH \
	((uint32_t) 0x6ef41102U)
#define VL53L5_RNG_PER_TARGET_DATA_START_RANGE_MM_BH \
	((uint32_t) 0x71141102U)
#define VL53L5_RNG_PER_TARGET_DATA_END_RANGE_MM_BH \
	((uint32_t) 0x73341102U)
#define VL53L5_RNG_PER_TARGET_DATA_MIN_RANGE_DELTA_MM_BH \
	((uint32_t) 0x75541101U)
#define VL53L5_RNG_PER_TARGET_DATA_MAX_RANGE_DELTA_MM_BH \
	((uint32_t) 0x76641101U)
#define VL53L5_RNG_PER_TARGET_DATA_TARGET_REFLECTANCE_EST_PC_BH \
	((uint32_t) 0x77741101U)
#define VL53L5_RNG_PER_TARGET_DATA_TARGET_STATUS_BH \
	((uint32_t) 0x78841101U)
#define VL53L5_REF_TIMING_DATA_BH \
	((uint32_t) 0x799400c0U)
#define VL53L5_REF_CHANNEL_DATA_BH \
	((uint32_t) 0x79a00100U)
#define VL53L5_REF_TARGET_DATA_BH \
	((uint32_t) 0x79b001c0U)
#define VL53L5_SHARPENER_TARGET_DATA_SHARPENER__GROUP_INDEX_BH \
	((uint32_t) 0x79cc1101U)
#define VL53L5_SHARPENER_TARGET_DATA_SHARPENER__CONFIDENCE_BH \
	((uint32_t) 0x7adc1101U)

#define VL53L5_ZONE_THRESH_STATUS_ZONE_THRESH_STATUS_BYTES_BH \
	((uint32_t) 0xafb80081U)
#define VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_BH \
	((uint32_t) 0xafc001c0U)

#ifdef __cplusplus
}
#endif

#endif
