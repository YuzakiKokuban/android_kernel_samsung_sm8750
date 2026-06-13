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

#ifndef __VL53L5_TCPM_PATCH_0_RESULTS_DEV_PATH_H__
#define __VL53L5_TCPM_PATCH_0_RESULTS_DEV_PATH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev) \
	((p_dev)->host_dev.ptcpm_patch_0_results_dev)
#define VL53L5_TCPM_PATCH_0_META_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->meta_data
#define VL53L5_TCPM_PATCH_0_COMMON_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->common_data
#define VL53L5_TCPM_PATCH_0_RNG_TIMING_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->rng_timing_data
#define VL53L5_TCPM_PATCH_0_PER_ZONE_RESULTS(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->per_zone_results
#define VL53L5_TCPM_PATCH_0_PER_TGT_RESULTS(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->per_tgt_results
#define VL53L5_TCPM_PATCH_0_REF_TIMING_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->ref_timing_data
#define VL53L5_TCPM_PATCH_0_REF_CHANNEL_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->ref_channel_data
#define VL53L5_TCPM_PATCH_0_REF_TARGET_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->ref_target_data
#define VL53L5_TCPM_PATCH_0_SHARPENER_TARGET_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->sharpener_target_data
#define VL53L5_TCPM_PATCH_0_ZONE_THRESH_STATUS(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->zone_thresh_status
#define VL53L5_TCPM_PATCH_0_DYN_XTALK_OP_PERSISTENT_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->dyn_xtalk_op_persistent_data
#define VL53L5_TCPM_PATCH_0_D16_BUF_META(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->d16_buf_meta
#define VL53L5_TCPM_PATCH_0_D16_PKT_META(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->d16_pkt_meta
#define VL53L5_TCPM_PATCH_0_D16_BLK_META(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->d16_blk_meta
#define VL53L5_TCPM_PATCH_0_D16_BUF_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->d16_buf_data
#define VL53L5_TCPM_PATCH_0_SZ_COMMON_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->sz_common_data
#define VL53L5_TCPM_PATCH_0_SZ_ZONE_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->sz_zone_data
#define VL53L5_TCPM_PATCH_0_SZ_TARGET_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->sz_target_data
#define VL53L5_TCPM_PATCH_0_GD_OP_STATUS(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->gd_op_status
#define VL53L5_TCPM_PATCH_0_GD_OP_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->gd_op_data
#define VL53L5_TCPM_PATCH_0_GD2_OP_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->gd2_op_data
#define VL53L5_TCPM_PATCH_0_GD2_OP_DATA_ARR(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->gd2_op_data_arr
#define VL53L5_TCPM_PATCH_0_D16_PER_TARGET_DATA(p_dev) \
	VL53L5_TCPM_PATCH_0_RESULTS_DEV(p_dev)->d16_per_target_data

#ifdef __cplusplus
}
#endif

#endif
