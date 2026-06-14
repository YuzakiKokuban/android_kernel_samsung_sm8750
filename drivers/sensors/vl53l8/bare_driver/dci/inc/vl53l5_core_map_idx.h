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

#ifndef __VL53L5_CORE_MAP_IDX_H__
#define __VL53L5_CORE_MAP_IDX_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_MAP_VERSION_IDX \
	((uint16_t) 0x5400)

#define DEV_FW_VERSION_IDX \
	((uint16_t) 0x5404)

#define DEV_CFG_INFO_IDX \
	((uint16_t) 0x540c)

#define DEV_FMT_TRACEABILITY_IDX \
	((uint16_t) 0x5420)

#define DEV_EWS_TRACEABILITY_IDX \
	((uint16_t) 0x5438)

#define DEV_UI_RNG_DATA_ADDR_IDX \
	((uint16_t) 0x5444)

#define DEV_SILICON_TEMP_DATA_IDX \
	((uint16_t) 0x5450)

#define DEV_ZONE_CFG_IDX \
	((uint16_t) 0x5454)

#define DEV_DEVICE_MODE_CFG_IDX \
	((uint16_t) 0x545c)

#define DEV_RNG_RATE_CFG_IDX \
	((uint16_t) 0x5464)

#define DEV_INT_MAX_CFG_IDX \
	((uint16_t) 0x5468)

#define DEV_INT_MIN_CFG_IDX \
	((uint16_t) 0x546c)

#define DEV_INT_MPX_DELTA_CFG_IDX \
	((uint16_t) 0x5470)

#define DEV_INT_DSS_CFG_IDX \
	((uint16_t) 0x5474)

#define DEV_FACTORY_CAL_CFG_IDX \
	((uint16_t) 0x5478)

#define DEV_OUTPUT_DATA_CFG_IDX \
	((uint16_t) 0x5480)

#define DEV_OUTPUT_BH_CFG_IDX \
	((uint16_t) 0x5484)

#define DEV_OUTPUT_BH_ENABLES_OP_BH__ENABLES_IDX \
	((uint16_t) 0x548c)
#define DEV_OUTPUT_BH_ENABLES_IDX \
	((uint16_t) 0x548c)

#define DEV_OUTPUT_BH_LIST_OP_BH__LIST_IDX \
	((uint16_t) 0x549c)
#define DEV_OUTPUT_BH_LIST_IDX \
	((uint16_t) 0x549c)

#define DEV_INTERRUPT_CFG_IDX \
	((uint16_t) 0x569c)

#define DEV_NVM_LASER_SAFETY_INFO_IDX \
	((uint16_t) 0x56a4)

#define DEV_PATCH_HOOK_ENABLES_ARRAY_PATCH_HOOK_ENABLES_IDX \
	((uint16_t) 0x56a8)
#define DEV_PATCH_HOOK_ENABLES_ARRAY_IDX \
	((uint16_t) 0x56a8)

#define DEV_DEVICE_ERROR_STATUS_IDX \
	((uint16_t) 0x56b8)

#define DEV_DEVICE_WARNING_STATUS_IDX \
	((uint16_t) 0x56c4)

#define DEV_IC_CHECKER_0_IDX \
	((uint16_t) 0x56d0)

#define DEV_IC_CHECKER_1_IDX \
	((uint16_t) 0x56dc)

#define DEV_IC_CHECKER_2_IDX \
	((uint16_t) 0x56e8)

#define DEV_IC_CHECKER_3_IDX \
	((uint16_t) 0x56f4)

#define DEV_IC_CHECKER_4_IDX \
	((uint16_t) 0x5700)

#define DEV_IC_CHECKER_5_IDX \
	((uint16_t) 0x570c)

#define DEV_IC_CHECKER_6_IDX \
	((uint16_t) 0x5718)

#define DEV_IC_CHECKER_7_IDX \
	((uint16_t) 0x5724)

#define DEV_IC_CHECKER_8_IDX \
	((uint16_t) 0x5730)

#define DEV_IC_CHECKER_9_IDX \
	((uint16_t) 0x573c)

#define DEV_IC_CHECKER_10_IDX \
	((uint16_t) 0x5748)

#define DEV_IC_CHECKER_11_IDX \
	((uint16_t) 0x5754)

#define DEV_IC_CHECKER_12_IDX \
	((uint16_t) 0x5760)

#define DEV_IC_CHECKER_13_IDX \
	((uint16_t) 0x576c)

#define DEV_IC_CHECKER_14_IDX \
	((uint16_t) 0x5778)

#define DEV_IC_CHECKER_15_IDX \
	((uint16_t) 0x5784)

#define DEV_IC_CHECKER_16_IDX \
	((uint16_t) 0x5790)

#define DEV_IC_CHECKER_17_IDX \
	((uint16_t) 0x579c)

#define DEV_IC_CHECKER_18_IDX \
	((uint16_t) 0x57a8)

#define DEV_IC_CHECKER_19_IDX \
	((uint16_t) 0x57b4)

#define DEV_IC_CHECKER_20_IDX \
	((uint16_t) 0x57c0)

#define DEV_IC_CHECKER_21_IDX \
	((uint16_t) 0x57cc)

#define DEV_IC_CHECKER_22_IDX \
	((uint16_t) 0x57d8)

#define DEV_IC_CHECKER_23_IDX \
	((uint16_t) 0x57e4)

#define DEV_IC_CHECKER_24_IDX \
	((uint16_t) 0x57f0)

#define DEV_IC_CHECKER_25_IDX \
	((uint16_t) 0x57fc)

#define DEV_IC_CHECKER_26_IDX \
	((uint16_t) 0x5808)

#define DEV_IC_CHECKER_27_IDX \
	((uint16_t) 0x5814)

#define DEV_IC_CHECKER_28_IDX \
	((uint16_t) 0x5820)

#define DEV_IC_CHECKER_29_IDX \
	((uint16_t) 0x582c)

#define DEV_IC_CHECKER_30_IDX \
	((uint16_t) 0x5838)

#define DEV_IC_CHECKER_31_IDX \
	((uint16_t) 0x5844)

#define DEV_IC_CHECKER_32_IDX \
	((uint16_t) 0x5850)

#define DEV_IC_CHECKER_33_IDX \
	((uint16_t) 0x585c)

#define DEV_IC_CHECKER_34_IDX \
	((uint16_t) 0x5868)

#define DEV_IC_CHECKER_35_IDX \
	((uint16_t) 0x5874)

#define DEV_IC_CHECKER_36_IDX \
	((uint16_t) 0x5880)

#define DEV_IC_CHECKER_37_IDX \
	((uint16_t) 0x588c)

#define DEV_IC_CHECKER_38_IDX \
	((uint16_t) 0x5898)

#define DEV_IC_CHECKER_39_IDX \
	((uint16_t) 0x58a4)

#define DEV_IC_CHECKER_40_IDX \
	((uint16_t) 0x58b0)

#define DEV_IC_CHECKER_41_IDX \
	((uint16_t) 0x58bc)

#define DEV_IC_CHECKER_42_IDX \
	((uint16_t) 0x58c8)

#define DEV_IC_CHECKER_43_IDX \
	((uint16_t) 0x58d4)

#define DEV_IC_CHECKER_44_IDX \
	((uint16_t) 0x58e0)

#define DEV_IC_CHECKER_45_IDX \
	((uint16_t) 0x58ec)

#define DEV_IC_CHECKER_46_IDX \
	((uint16_t) 0x58f8)

#define DEV_IC_CHECKER_47_IDX \
	((uint16_t) 0x5904)

#define DEV_IC_CHECKER_48_IDX \
	((uint16_t) 0x5910)

#define DEV_IC_CHECKER_49_IDX \
	((uint16_t) 0x591c)

#define DEV_IC_CHECKER_50_IDX \
	((uint16_t) 0x5928)

#define DEV_IC_CHECKER_51_IDX \
	((uint16_t) 0x5934)

#define DEV_IC_CHECKER_52_IDX \
	((uint16_t) 0x5940)

#define DEV_IC_CHECKER_53_IDX \
	((uint16_t) 0x594c)

#define DEV_IC_CHECKER_54_IDX \
	((uint16_t) 0x5958)

#define DEV_IC_CHECKER_55_IDX \
	((uint16_t) 0x5964)

#define DEV_IC_CHECKER_56_IDX \
	((uint16_t) 0x5970)

#define DEV_IC_CHECKER_57_IDX \
	((uint16_t) 0x597c)

#define DEV_IC_CHECKER_58_IDX \
	((uint16_t) 0x5988)

#define DEV_IC_CHECKER_59_IDX \
	((uint16_t) 0x5994)

#define DEV_IC_CHECKER_60_IDX \
	((uint16_t) 0x59a0)

#define DEV_IC_CHECKER_61_IDX \
	((uint16_t) 0x59ac)

#define DEV_IC_CHECKER_62_IDX \
	((uint16_t) 0x59b8)

#define DEV_IC_CHECKER_63_IDX \
	((uint16_t) 0x59c4)

#define DEV_IC_VALID_TGT_STATUS_VAILD_TARGET_STATUS_IDX \
	((uint16_t) 0x59d0)
#define DEV_IC_VALID_TGT_STATUS_IDX \
	((uint16_t) 0x59d0)

#ifdef __cplusplus
}
#endif

#endif
