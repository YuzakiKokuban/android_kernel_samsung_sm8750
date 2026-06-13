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

#ifndef __DCI_PATCH_STRUCTS_H__
#define __DCI_PATCH_STRUCTS_H__

#include "vl53l5_results_config.h"
#include "dci_defs.h"
#include "dci_luts.h"
#include "dci_ui_union_structs.h"
#include "dci_patch_defs.h"
#include "dci_patch_luts.h"
#include "dci_patch_structs.h"
#include "dci_patch_union_structs.h"
#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vl53l5_gd_op_status_t {

	uint32_t gd__rate_ratio;

	uint8_t gd__confidence;

	uint8_t gd__plane_detected;

	uint8_t gd__ratio_detected;

	uint8_t gd__glass_detected;

	uint8_t gd__tgt_behind_glass_detected;

	uint8_t gd__mirror_detected;

	uint8_t gd__op_spare_0;

	uint8_t gd__op_spare_1;
};

struct dci_grp__depth16_value_t {

	union dci_union__depth16_value_u depth16;

	uint16_t d16_confidence_pc;
};

struct vl53l5_d16_per_tgt_results_t {
#ifdef VL53L5_DEPTH16_ON

	uint16_t depth16[VL53L5_MAX_TARGETS];
#endif

#if !defined(VL53L5_DEPTH16_ON)

	uint32_t dummy;
#endif

};

struct dci_grp__d16__ref_target_data_t {

	uint16_t depth16;

	uint16_t d16__ref_target__pad_0;
};

struct dci_grp__d16__buf__packet_header_t {

	union dci_union__d16__buf__packet_header_u packet_header;
};

struct dci_grp__d16__buf__glass_detect_t {

	union dci_union__d16__buf__glass_detect_u glass_detect;
};

struct dci_grp__d16__buf__format_t {

	uint32_t bh_list[DCI_D16_MAX_BH_LIST_SIZE];
};

#ifdef __cplusplus
}
#endif

#endif
