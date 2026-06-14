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

#ifndef _VL53L5_PATCH_API_CORE_H_
#define _VL53L5_PATCH_API_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vl53l5_platform_user_data.h"
#include "vl53l5_patch_core_map.h"
#include "vl53l5_tcpm_patch_0_core_map.h"
#include "vl53l5_tcpm_patch_1_core_map.h"

#define VL53L5_TCPM_0_PAGE_BH 0x0001000c
#define VL53L5_TCPM_1_PAGE_BH 0x0002000c

struct vl53l5_core_data_t {
	struct vl53l5_patch_core_dev_t patch;
	struct vl53l5_tcpm_patch_0_core_dev_t tcpm_0_patch;
	struct vl53l5_tcpm_patch_1_core_dev_t tcpm_1_patch;
};

struct vl53l5_patch_version_t {
	struct {

		uint16_t ver_major;

		uint16_t ver_minor;
	} patch_version;
	struct {

		uint16_t ver_major;

		uint16_t ver_minor;
	} tcpm_version;
	struct {

		uint16_t ver_major;

		uint16_t ver_minor;

		uint16_t ver_build;

		uint16_t ver_revision;
	} patch_driver;
};

int32_t vl53l5_assign_core(struct vl53l5_dev_handle_t *p_dev,
			   struct vl53l5_core_data_t *p_core);

int32_t vl53l5_unassign_core(struct vl53l5_dev_handle_t *p_dev);

int32_t vl53l5_get_patch_version(struct vl53l5_dev_handle_t *p_dev,
				 struct vl53l5_patch_version_t *p_version);

#ifdef __cplusplus
}
#endif

#endif
