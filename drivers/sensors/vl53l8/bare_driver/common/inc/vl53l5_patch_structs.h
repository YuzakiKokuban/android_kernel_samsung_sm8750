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

#ifndef __VL53L5_PATCH_STRUCTS_H__
#define __VL53L5_PATCH_STRUCTS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "vl53l5_types.h"

#define VL53L5_PATCH_LOAD 1
#define VL53L5_PATCH_BOOT 2
#define VL53L5_RAM_LOAD 3

struct vl53l5_patch_data_t {
	uint32_t patch_ver_major;
	uint32_t patch_ver_minor;
	uint32_t patch_ver_build;
	uint32_t patch_ver_revision;

	uint32_t boot_flag;

	uint32_t patch_offset;

	uint32_t patch_size;

	uint32_t patch_checksum;

	uint32_t tcpm_offset;

	uint32_t tcpm_size;

	uint32_t tcpm_page;

	uint32_t tcpm_page_offset;

	uint32_t hooks_offset;

	uint32_t hooks_size;

	uint32_t hooks_page;

	uint32_t hooks_page_offset;

	uint32_t breakpoint_en_offset;

	uint32_t breakpoint_en_size;

	uint32_t breakpoint_en_page;

	uint32_t breakpoint_en_page_offset;

	uint32_t breakpoint_offset;

	uint32_t breakpoint_size;

	uint32_t breakpoint_page;

	uint32_t breakpoint_page_offset;

	uint32_t checksum_en_offset;

	uint32_t checksum_en_size;

	uint32_t checksum_en_page;

	uint32_t checksum_en_page_offset;

	uint32_t patch_code_offset;

	uint32_t patch_code_size;

	uint32_t patch_code_page;

	uint32_t patch_code_page_offset;

	uint32_t dci_tcpm_patch_0_offset;

	uint32_t dci_tcpm_patch_0_size;

	uint32_t dci_tcpm_patch_0_page;

	uint32_t dci_tcpm_patch_0_page_offset;

	uint32_t dci_tcpm_patch_1_offset;

	uint32_t dci_tcpm_patch_1_size;

	uint32_t dci_tcpm_patch_1_page;

	uint32_t dci_tcpm_patch_1_page_offset;
};

#ifdef __cplusplus
}
#endif

#endif
