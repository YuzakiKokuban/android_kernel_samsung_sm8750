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

#ifndef _VL53L8_K_IOCTL_DATA_H_
#define _VL53L8_K_IOCTL_DATA_H_

#include "vl53l8_k_user_def.h"
#include "vl53l5_api_ranging.h"
#include "vl53l5_api_range_decode.h"
#include "vl53l5_api_core.h"
#include "vl53l5_patch_api_core.h"
#include "vl53l5_power_states.h"

struct vl53l8_k_raw_buffer_data_t {

	uint8_t buffer[VL53L8_K_CONFIG_DATA_MAX_SIZE];

	uint32_t count;
};

struct vl53l8_k_get_parameters_data_t {
	struct vl53l8_k_raw_buffer_data_t raw_data;
	uint32_t headers[VL53L8_K_MAX_NUM_GET_HEADERS];
	uint32_t header_count;
};

struct vl53l8_k_asz_t {
	uint8_t asz_0[8];
	uint8_t asz_1[8];
	uint8_t asz_2[8];
	uint8_t asz_3[8];
};

struct vl53l8_k_asz_tuning_t {

	uint32_t asz_0_ll_zone_id;

	uint32_t asz_1_ll_zone_id;

	uint32_t asz_2_ll_zone_id;

	uint32_t asz_3_ll_zone_id;
};

struct vl53l8_k_bin_version {
	unsigned int fw_ver_major;
	unsigned int fw_ver_minor;
	unsigned int config_ver_major;
	unsigned int config_ver_minor;
};
struct vl53l8_k_version_t {
	struct vl53l5_version_t driver;
	struct vl53l5_patch_version_t patch;
	struct {
		uint16_t ver_major;
		uint16_t ver_minor;
		uint16_t ver_build;
		uint16_t ver_revision;
	} kernel;
	struct vl53l8_k_bin_version bin_version;
};

enum vl53l8_k_config_preset {
	VL53L8_CAL__NULL_XTALK_SHAPE = 0,
	VL53L8_CAL__GENERIC_XTALK_SHAPE,
	VL53L8_CAL__XTALK_GRID_SHAPE_MON,
	VL53L8_CAL__DYN_XTALK_MONITOR,
	VL53L8_CFG__STATIC__SS_0PC,
	VL53L8_CFG__STATIC__SS_1PC,
	VL53L8_CFG__STATIC__SS_2PC,
	VL53L8_CFG__STATIC__SS_4PC,
	VL53L8_CFG__XTALK_GEN1_8X8_1000,
	VL53L8_CFG__XTALK_GEN2_8X8_300,
	VL53L8_CFG__B2BWB_8X8_OPTION_0,
	VL53L8_CFG__B2BWB_8X8_OPTION_1,
	VL53L8_CFG__B2BWB_8X8_OPTION_2,
	VL53L8_CFG__B2BWB_8X8_OPTION_3,
	VL53L8_CFG__PATCH_DEFAULT_UCP4,
};
/*
VL53L8_CFG__B2BWB_8X8_OPTION_0  =>  D16 data only + MODE1
VL53L8_CFG__B2BWB_8X8_OPTION_1  =>  D16 data only + MODE3
VL53L8_CFG__B2BWB_8X8_OPTION_2  =>  D16 data + DMAX + AMB + PEAK + GD + TMP + MODE1
VL53L8_CFG__B2BWB_8X8_OPTION_3  =>  D16 data + DMAX + AMB + PEAK + GD + TMP + MODE3
*/
struct vl53l8_fw_header_t {
	unsigned int fw_ver_major;
	unsigned int fw_ver_minor;
	unsigned int config_ver_major;
	unsigned int config_ver_minor;
	unsigned int fw_offset;
	unsigned int fw_size;
	unsigned int buf00_offset;
	unsigned int buf00_size;
	unsigned int buf01_offset;
	unsigned int buf01_size;
	unsigned int buf02_offset;
	unsigned int buf02_size;
	unsigned int buf03_offset;
	unsigned int buf03_size;
	unsigned int buf04_offset;
	unsigned int buf04_size;
	unsigned int buf05_offset;
	unsigned int buf05_size;
	unsigned int buf06_offset;
	unsigned int buf06_size;
	unsigned int buf07_offset;
	unsigned int buf07_size;
	unsigned int buf08_offset;
	unsigned int buf08_size;
	unsigned int buf09_offset;
	unsigned int buf09_size;
	unsigned int buf10_offset;
	unsigned int buf10_size;
	unsigned int buf11_offset;
	unsigned int buf11_size;
	unsigned int buf12_offset;
	unsigned int buf12_size;
	unsigned int buf13_offset;
	unsigned int buf13_size;
	unsigned int buf14_offset;
	unsigned int buf14_size;
};

#endif
