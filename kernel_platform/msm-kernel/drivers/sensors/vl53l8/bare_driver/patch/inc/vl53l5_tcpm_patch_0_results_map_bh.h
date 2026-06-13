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

#ifndef __VL53L5_TCPM_PATCH_0_RESULTS_MAP_BH_H__
#define __VL53L5_TCPM_PATCH_0_RESULTS_MAP_BH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_TCPM_0_RNG_META_DATA_BH \
	((uint32_t) 0x000800c0U)
#define VL53L5_TCPM_0_RNG_COMMON_DATA_BH \
	((uint32_t) 0x00140080U)
#define VL53L5_TCPM_0_RNG_TIMING_DATA_BH \
	((uint32_t) 0x001c00c0U)
#define VL53L5_TCPM_0_RNG_PER_ZONE_DATA_AMB_RATE_KCPS_PER_SPAD_BH \
	((uint32_t) 0x00280444U)
#define VL53L5_TCPM_0_RNG_PER_ZONE_DATA_RNG__EFFECTIVE_SPAD_COUNT_BH \
	((uint32_t) 0x01380444U)
#define VL53L5_TCPM_0_RNG_PER_ZONE_DATA_AMB_DMAX_MM_BH \
	((uint32_t) 0x02480442U)
#define VL53L5_TCPM_0_RNG_PER_ZONE_DATA_SILICON_TEMP_DEGC__START_BH \
	((uint32_t) 0x02d00441U)
#define VL53L5_TCPM_0_RNG_PER_ZONE_DATA_SILICON_TEMP_DEGC__END_BH \
	((uint32_t) 0x03140441U)
#define VL53L5_TCPM_0_RNG_PER_ZONE_DATA_RNG__NO_OF_TARGETS_BH \
	((uint32_t) 0x03580441U)
#define VL53L5_TCPM_0_RNG_PER_ZONE_DATA_RNG__ZONE_ID_BH \
	((uint32_t) 0x039c0441U)
#define VL53L5_TCPM_0_RNG_PER_ZONE_DATA_RNG__SEQUENCE_IDX_BH \
	((uint32_t) 0x03e00441U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_PEAK_RATE_KCPS_PER_SPAD_BH \
	((uint32_t) 0x04241104U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_MEDIAN_PHASE_BH \
	((uint32_t) 0x08641104U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_RATE_SIGMA_KCPS_PER_SPAD_BH \
	((uint32_t) 0x0ca41104U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_TARGET_ZSCORE_BH \
	((uint32_t) 0x10e41102U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_RANGE_SIGMA_MM_BH \
	((uint32_t) 0x13041102U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_MEDIAN_RANGE_MM_BH \
	((uint32_t) 0x15241102U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_START_RANGE_MM_BH \
	((uint32_t) 0x17441102U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_END_RANGE_MM_BH \
	((uint32_t) 0x19641102U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_MIN_RANGE_DELTA_MM_BH \
	((uint32_t) 0x1b841101U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_MAX_RANGE_DELTA_MM_BH \
	((uint32_t) 0x1c941101U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_TARGET_REFLECTANCE_EST_PC_BH \
	((uint32_t) 0x1da41101U)
#define VL53L5_TCPM_0_RNG_PER_TARGET_DATA_TARGET_STATUS_BH \
	((uint32_t) 0x1eb41101U)
#define VL53L5_TCPM_0_REF_TIMING_DATA_BH \
	((uint32_t) 0x1fc400c0U)
#define VL53L5_TCPM_0_REF_CHANNEL_DATA_BH \
	((uint32_t) 0x1fd00100U)
#define VL53L5_TCPM_0_REF_TARGET_DATA_BH \
	((uint32_t) 0x1fe001c0U)
#define VL53L5_TCPM_0_SHARPENER_TARGET_DATA_SHARPENER__GROUP_INDEX_BH \
	((uint32_t) 0x1ffc1101U)
#define VL53L5_TCPM_0_SHARPENER_TARGET_DATA_SHARPENER__CONFIDENCE_BH \
	((uint32_t) 0x210c1101U)
#define VL53L5_TCPM_0_ZONE_THRESH_STATUS_ZONE_THRESH_STATUS_BYTES_BH \
	((uint32_t) 0x221c0081U)
#define VL53L5_TCPM_0_DYN_XTALK_OP_PERSISTENT_DATA_BH \
	((uint32_t) 0x222401c0U)

#define VL53L5_TCPM_0_GD_OP_STATUS_BH \
	((uint32_t) 0x292c00c0U)

#define VL53L5_TCPM_0_D16_PER_TARGET_DATA_DEPTH16_BH \
	((uint32_t) 0x29b81102U)

#ifdef __cplusplus
}
#endif

#endif
