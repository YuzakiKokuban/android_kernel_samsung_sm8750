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

#ifndef _VL53L8_K_IOCTL_CODES_H_
#define _VL53L8_K_IOCTL_CODES_H_

#include "vl53l8_k_user_data.h"

#define VL53L8_IOCTL_CHAR 'p'

#define VL53L8_IOCTL_INIT\
	_IO(VL53L8_IOCTL_CHAR, 0x01)

#define VL53L8_IOCTL_TERM \
	_IO(VL53L8_IOCTL_CHAR, 0x02)

#define VL53L8_IOCTL_GET_VERSION \
	_IOR(VL53L8_IOCTL_CHAR, 0x03, struct vl53l8_k_version_t)

#define VL53L8_IOCTL_START \
	_IOW(VL53L8_IOCTL_CHAR, 0x04, struct vl53l5_ranging_mode_flags_t)

#define VL53L8_IOCTL_STOP \
	_IOW(VL53L8_IOCTL_CHAR, 0x05, struct vl53l5_ranging_mode_flags_t)
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
#define VL53L8_IOCTL_GET_RANGE \
	_IOR(VL53L8_IOCTL_CHAR, 0x06, struct vl53l5_range_data_t)
#else // SS_SUPPORT_SEC_CODE
#define VL53L8_IOCTL_GET_RANGE \
	_IOR(VL53L8_IOCTL_CHAR, 0x06, struct range_sensor_data_t)
#endif // SS_SUPPORT_SEC_CODE

#define VL53L8_IOCTL_SET_DEVICE_PARAMETERS \
	_IOW(VL53L8_IOCTL_CHAR, 0x08, int)

#define VL53L8_IOCTL_GET_DEVICE_PARAMETERS \
	_IOWR(VL53L8_IOCTL_CHAR, 0x09, struct vl53l8_k_get_parameters_data_t)

#define VL53L8_IOCTL_GET_MODULE_INFO \
	_IOR(VL53L8_IOCTL_CHAR, 0x0a, struct vl53l5_module_info_t)

#define VL53L8_IOCTL_GET_ERROR_INFO \
	_IOR(VL53L8_IOCTL_CHAR, 0x0b, int)

#define VL53L8_IOCTL_SET_POWER_MODE \
	_IOW(VL53L8_IOCTL_CHAR, 0x0c, enum vl53l5_power_states)

#define VL53L8_IOCTL_POLL_DATA_READY \
	_IO(VL53L8_IOCTL_CHAR, 0x0d)

#define VL53L8_IOCTL_READ_P2P_FILE \
	_IO(VL53L8_IOCTL_CHAR, 0x10)

#define VL53L8_IOCTL_READ_SHAPE_FILE \
	_IO(VL53L8_IOCTL_CHAR, 0x13)

#define VL53L8_IOCTL_PERFORM_CALIBRATION_300 \
	_IO(VL53L8_IOCTL_CHAR, 0x14)

#define VL53L8_IOCTL_READ_GENERIC_SHAPE \
	_IO(VL53L8_IOCTL_CHAR, 0x15)

#define VL53L8_IOCTL_PERFORM_CHARACTERISATION_1000 \
	_IO(VL53L8_IOCTL_CHAR, 0x19)

#define VL53L8_IOCTL_SET_RANGING_RATE \
	_IOW(VL53L8_IOCTL_CHAR, 0x1a, int)

#define VL53L8_IOCTL_SET_INTEGRATION_TIME_US \
	_IOW(VL53L8_IOCTL_CHAR, 0x1b, int)

#define VL53L8_IOCTL_SET_ASZ_TUNING \
	_IOW(VL53L8_IOCTL_CHAR, 0x1c, struct vl53l8_k_asz_tuning_t)

#define VL53L5_IOCTL_GET_CAL_DATA \
    _IOR(VL53L8_IOCTL_CHAR, 0x20, struct vl53l8_cal_data_t)

#define VL53L5_IOCTL_SET_CAL_DATA \
    _IOW(VL53L8_IOCTL_CHAR, 0x21, struct vl53l8_cal_data_t)

#define VL53L5_IOCTL_SET_PASS_FAIL \
    _IOW(VL53L8_IOCTL_CHAR, 0x22, struct vl53l8_update_data_t)

#define VL53L5_IOCTL_SET_FILE_LIST \
    _IOW(VL53L8_IOCTL_CHAR, 0x23, struct vl53l8_file_list_t)

#define VL53L8_IOCTL_SET_GLARE_FILTER_TUNING \
	_IOW(VL53L8_IOCTL_CHAR, 0x24, struct vl53l8_k_glare_filter_tuning_t)

#define VL53L8_IOCTL_SET_TRANSFER_SPEED_HZ \
	_IOW(VL53L8_IOCTL_CHAR, 0x25, unsigned int)
#ifdef SS_SUPPORT_SEC_CODE
#define VL53L8_IOCTL_GET_STATUS \
	_IOR(VL53L8_IOCTL_CHAR, 0x55, int)
#endif // SS_SUPPORT_SEC_CODE
#endif
