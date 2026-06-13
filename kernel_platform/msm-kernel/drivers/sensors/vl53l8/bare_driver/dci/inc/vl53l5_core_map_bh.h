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

#ifndef __VL53L5_CORE_MAP_BH_H__
#define __VL53L5_CORE_MAP_BH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_MAP_VERSION_BH \
	((uint32_t) 0x54000040U)
#define VL53L5_FW_VERSION_BH \
	((uint32_t) 0x54040080U)
#define VL53L5_CFG_INFO_BH \
	((uint32_t) 0x540c0140U)
#define VL53L5_FMT_TRACEABILITY_BH \
	((uint32_t) 0x54200180U)
#define VL53L5_EWS_TRACEABILITY_BH \
	((uint32_t) 0x543800c0U)
#define VL53L5_UI_RNG_DATA_ADDR_BH \
	((uint32_t) 0x544400c0U)
#define VL53L5_SILICON_TEMP_DATA_BH \
	((uint32_t) 0x54500040U)
#define VL53L5_ZONE_CFG_BH \
	((uint32_t) 0x54540080U)
#define VL53L5_DEVICE_MODE_CFG_BH \
	((uint32_t) 0x545c0080U)
#define VL53L5_RNG_RATE_CFG_BH \
	((uint32_t) 0x54640040U)
#define VL53L5_INT_MAX_CFG_BH \
	((uint32_t) 0x54680040U)
#define VL53L5_INT_MIN_CFG_BH \
	((uint32_t) 0x546c0040U)
#define VL53L5_INT_MPX_DELTA_CFG_BH \
	((uint32_t) 0x54700040U)
#define VL53L5_INT_DSS_CFG_BH \
	((uint32_t) 0x54740040U)
#define VL53L5_FACTORY_CAL_CFG_BH \
	((uint32_t) 0x54780080U)
#define VL53L5_OUTPUT_DATA_CFG_BH \
	((uint32_t) 0x54800040U)
#define VL53L5_OUTPUT_BH_CFG_BH \
	((uint32_t) 0x54840080U)
#define VL53L5_OUTPUT_BH_ENABLES_OP_BH__ENABLES_BH \
	((uint32_t) 0x548c0044U)
#define VL53L5_OUTPUT_BH_LIST_OP_BH__LIST_BH \
	((uint32_t) 0x549c0804U)
#define VL53L5_INTERRUPT_CFG_BH \
	((uint32_t) 0x569c0080U)
#define VL53L5_NVM_LASER_SAFETY_INFO_BH \
	((uint32_t) 0x56a40040U)
#define VL53L5_PATCH_HOOK_ENABLES_ARRAY_PATCH_HOOK_ENABLES_BH \
	((uint32_t) 0x56a80044U)
#define VL53L5_DEVICE_ERROR_STATUS_BH \
	((uint32_t) 0x56b800c0U)
#define VL53L5_DEVICE_WARNING_STATUS_BH \
	((uint32_t) 0x56c400c0U)
#define VL53L5_IC_CHECKER_0_BH \
	((uint32_t) 0x56d000c0U)
#define VL53L5_IC_CHECKER_1_BH \
	((uint32_t) 0x56dc00c0U)
#define VL53L5_IC_CHECKER_2_BH \
	((uint32_t) 0x56e800c0U)
#define VL53L5_IC_CHECKER_3_BH \
	((uint32_t) 0x56f400c0U)
#define VL53L5_IC_CHECKER_4_BH \
	((uint32_t) 0x570000c0U)
#define VL53L5_IC_CHECKER_5_BH \
	((uint32_t) 0x570c00c0U)
#define VL53L5_IC_CHECKER_6_BH \
	((uint32_t) 0x571800c0U)
#define VL53L5_IC_CHECKER_7_BH \
	((uint32_t) 0x572400c0U)
#define VL53L5_IC_CHECKER_8_BH \
	((uint32_t) 0x573000c0U)
#define VL53L5_IC_CHECKER_9_BH \
	((uint32_t) 0x573c00c0U)
#define VL53L5_IC_CHECKER_10_BH \
	((uint32_t) 0x574800c0U)
#define VL53L5_IC_CHECKER_11_BH \
	((uint32_t) 0x575400c0U)
#define VL53L5_IC_CHECKER_12_BH \
	((uint32_t) 0x576000c0U)
#define VL53L5_IC_CHECKER_13_BH \
	((uint32_t) 0x576c00c0U)
#define VL53L5_IC_CHECKER_14_BH \
	((uint32_t) 0x577800c0U)
#define VL53L5_IC_CHECKER_15_BH \
	((uint32_t) 0x578400c0U)
#define VL53L5_IC_CHECKER_16_BH \
	((uint32_t) 0x579000c0U)
#define VL53L5_IC_CHECKER_17_BH \
	((uint32_t) 0x579c00c0U)
#define VL53L5_IC_CHECKER_18_BH \
	((uint32_t) 0x57a800c0U)
#define VL53L5_IC_CHECKER_19_BH \
	((uint32_t) 0x57b400c0U)
#define VL53L5_IC_CHECKER_20_BH \
	((uint32_t) 0x57c000c0U)
#define VL53L5_IC_CHECKER_21_BH \
	((uint32_t) 0x57cc00c0U)
#define VL53L5_IC_CHECKER_22_BH \
	((uint32_t) 0x57d800c0U)
#define VL53L5_IC_CHECKER_23_BH \
	((uint32_t) 0x57e400c0U)
#define VL53L5_IC_CHECKER_24_BH \
	((uint32_t) 0x57f000c0U)
#define VL53L5_IC_CHECKER_25_BH \
	((uint32_t) 0x57fc00c0U)
#define VL53L5_IC_CHECKER_26_BH \
	((uint32_t) 0x580800c0U)
#define VL53L5_IC_CHECKER_27_BH \
	((uint32_t) 0x581400c0U)
#define VL53L5_IC_CHECKER_28_BH \
	((uint32_t) 0x582000c0U)
#define VL53L5_IC_CHECKER_29_BH \
	((uint32_t) 0x582c00c0U)
#define VL53L5_IC_CHECKER_30_BH \
	((uint32_t) 0x583800c0U)
#define VL53L5_IC_CHECKER_31_BH \
	((uint32_t) 0x584400c0U)
#define VL53L5_IC_CHECKER_32_BH \
	((uint32_t) 0x585000c0U)
#define VL53L5_IC_CHECKER_33_BH \
	((uint32_t) 0x585c00c0U)
#define VL53L5_IC_CHECKER_34_BH \
	((uint32_t) 0x586800c0U)
#define VL53L5_IC_CHECKER_35_BH \
	((uint32_t) 0x587400c0U)
#define VL53L5_IC_CHECKER_36_BH \
	((uint32_t) 0x588000c0U)
#define VL53L5_IC_CHECKER_37_BH \
	((uint32_t) 0x588c00c0U)
#define VL53L5_IC_CHECKER_38_BH \
	((uint32_t) 0x589800c0U)
#define VL53L5_IC_CHECKER_39_BH \
	((uint32_t) 0x58a400c0U)
#define VL53L5_IC_CHECKER_40_BH \
	((uint32_t) 0x58b000c0U)
#define VL53L5_IC_CHECKER_41_BH \
	((uint32_t) 0x58bc00c0U)
#define VL53L5_IC_CHECKER_42_BH \
	((uint32_t) 0x58c800c0U)
#define VL53L5_IC_CHECKER_43_BH \
	((uint32_t) 0x58d400c0U)
#define VL53L5_IC_CHECKER_44_BH \
	((uint32_t) 0x58e000c0U)
#define VL53L5_IC_CHECKER_45_BH \
	((uint32_t) 0x58ec00c0U)
#define VL53L5_IC_CHECKER_46_BH \
	((uint32_t) 0x58f800c0U)
#define VL53L5_IC_CHECKER_47_BH \
	((uint32_t) 0x590400c0U)
#define VL53L5_IC_CHECKER_48_BH \
	((uint32_t) 0x591000c0U)
#define VL53L5_IC_CHECKER_49_BH \
	((uint32_t) 0x591c00c0U)
#define VL53L5_IC_CHECKER_50_BH \
	((uint32_t) 0x592800c0U)
#define VL53L5_IC_CHECKER_51_BH \
	((uint32_t) 0x593400c0U)
#define VL53L5_IC_CHECKER_52_BH \
	((uint32_t) 0x594000c0U)
#define VL53L5_IC_CHECKER_53_BH \
	((uint32_t) 0x594c00c0U)
#define VL53L5_IC_CHECKER_54_BH \
	((uint32_t) 0x595800c0U)
#define VL53L5_IC_CHECKER_55_BH \
	((uint32_t) 0x596400c0U)
#define VL53L5_IC_CHECKER_56_BH \
	((uint32_t) 0x597000c0U)
#define VL53L5_IC_CHECKER_57_BH \
	((uint32_t) 0x597c00c0U)
#define VL53L5_IC_CHECKER_58_BH \
	((uint32_t) 0x598800c0U)
#define VL53L5_IC_CHECKER_59_BH \
	((uint32_t) 0x599400c0U)
#define VL53L5_IC_CHECKER_60_BH \
	((uint32_t) 0x59a000c0U)
#define VL53L5_IC_CHECKER_61_BH \
	((uint32_t) 0x59ac00c0U)
#define VL53L5_IC_CHECKER_62_BH \
	((uint32_t) 0x59b800c0U)
#define VL53L5_IC_CHECKER_63_BH \
	((uint32_t) 0x59c400c0U)
#define VL53L5_IC_VALID_TGT_STATUS_VAILD_TARGET_STATUS_BH \
	((uint32_t) 0x59d00081U)

#ifdef __cplusplus
}
#endif

#endif
