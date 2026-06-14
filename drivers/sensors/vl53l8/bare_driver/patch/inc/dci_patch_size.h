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

#ifndef __DCI_PATCH_SIZE_H__
#define __DCI_PATCH_SIZE_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_GD_OP_STATUS_SZ \
	((uint16_t) 12)

#define VL53L5_GD_OP_DATA_SZ \
	((uint16_t) 44)

#define VL53L5_GD2_OP_DATA_SZ \
	((uint16_t) 36)

#define VL53L5_GD2_OP_DATA_ARR_SZ \
	((uint16_t) 48)

#define VL53L5_GODA_GD_UPPER_BOUNDS_MM_SZ \
	((uint16_t) 16)

#define VL53L5_GODA_GD_LOWER_BOUNDS_MM_SZ \
	((uint16_t) 16)

#define VL53L5_GODA_GD_BORDER_VARIANCE_SZ \
	((uint16_t) 16)

#define VL53L5_SZ_COMMON_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_SZ_ZONE_DATA_SZ \
	((uint16_t) 48)

#define VL53L5_SZD_AMB_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 16)

#define VL53L5_SZD_RNG_EFFECTIVE_SPAD_COUNT_SZ \
	((uint16_t) 16)

#define VL53L5_SZD_AMB_DMAX_MM_SZ \
	((uint16_t) 8)

#define VL53L5_SZD_RNG_NO_OF_TARGETS_SZ \
	((uint16_t) 4)

#define VL53L5_SZD_RNG_ZONE_ID_SZ \
	((uint16_t) 4)

#define VL53L5_SZ_TARGET_DATA_SZ \
	((uint16_t) 144)

#define VL53L5_STD_PEAK_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 64)

#define VL53L5_STD_RANGE_SIGMA_MM_SZ \
	((uint16_t) 32)

#define VL53L5_STD_MEDIAN_RANGE_MM_SZ \
	((uint16_t) 32)

#define VL53L5_STD_TARGET_STATUS_SZ \
	((uint16_t) 16)

#define VL53L5_DEPTH16_VALUE_SZ \
	((uint16_t) 4)

#define VL53L5_D16_PER_TARGET_DATA_SZ \
	((uint16_t) 544)

#define VL53L5_DPTD_DEPTH16_SZ \
	((uint16_t) 2 * VL53L5_MAX_TARGETS)

#define VL53L5_D16_REF_TARGET_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_D16_BUF_PACKET_HEADER_SZ \
	((uint16_t) 4)

#define VL53L5_D16_BUF_GLASS_DETECT_SZ \
	((uint16_t) 4)

#define VL53L5_D16_BLK_META_SZ \
	((uint16_t) 16)

#define VL53L5_D16_BUF_META_SZ \
	((uint16_t) 8)

#define VL53L5_D16_PKT_META_SZ \
	((uint16_t) 16)

#define VL53L5_D16_BUF_FORMAT_SZ \
	((uint16_t) 64)

#define VL53L5_DBF_BH_LIST_SZ \
	((uint16_t) 64)

#define VL53L5_D16_BUF_DATA_SZ \
	((uint16_t) 1536)

#define VL53L5_DBD_DATA_SZ \
	((uint16_t) 1536)

#ifdef __cplusplus
}
#endif

#endif
