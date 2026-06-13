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

#ifndef __VL53L5_PATCH_RESULTS_CONFIG_H__
#define __VL53L5_PATCH_RESULTS_CONFIG_H__

#include "vl53l5_results_build_config.h"
#include "dci_defs.h"
#include "dci_ui_memory_defs.h"
#include "dci_size.h"
#include "dci_ui_size.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_DUMMY_BYTES_SZ \
	VL53L5_CONFIG_DUMMY_BYTES_SZ
#define VL53L5_DUMMY_FOOTER_BYTES_SZ \
	4U

#define VL53L5_PATCH_MAP_RESULTS_PACKET_META_SIZE \
	(VL53L5_DCI_UI_DEV_INFO_SZ \
	 + VL53L5_DUMMY_BYTES_SZ \
	 + (2 * VL53L5_DCI_UI_RNG_DATA_HEADER_SZ) \
	 + (3 * VL53L5_DCI_UI_PACKED_DATA_BH_SZ) \
	 + VL53L5_DUMMY_FOOTER_BYTES_SZ)

#define VL53L5_PATCH_MAP_RESULTS_TOTAL_SIZE_BYTES \
	VL53L5_PATCH_MAP_RESULTS_PACKET_META_SIZE

#ifdef __cplusplus
}
#endif

#endif
