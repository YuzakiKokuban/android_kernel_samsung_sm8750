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

#ifndef _VL53L8_K_IOCTL_CONTROLS_H_
#define _VL53L8_K_IOCTL_CONTROLS_H_

#include "vl53l8_k_module_data.h"
#include "vl53l8_k_user_data.h"

int vl53l8_get_firmware_version(struct vl53l8_k_module_t *p_module, struct vl53l8_k_version_t *p_version);
int vl53l8_ioctl_get_version(struct vl53l8_k_module_t *p_module, void __user *p);
int vl53l8_ioctl_init(struct vl53l8_k_module_t *p_module);

int vl53l8_ioctl_term(struct vl53l8_k_module_t *p_module);
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_get_version(struct vl53l8_k_module_t *p_module, void __user *p);
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE

int vl53l8_ioctl_get_module_info(struct vl53l8_k_module_t *p_module, void __user *p);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_start(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_ioctl_stop(struct vl53l8_k_module_t *p_module, void __user *p);
#else // SS_SUPPORT_SEC_CODE
int vl53l8_ioctl_start(struct vl53l8_k_module_t *p_module);

int vl53l8_ioctl_stop(struct vl53l8_k_module_t *p_module);
#endif // SS_SUPPORT_SEC_CODE

int vl53l8_ioctl_get_range(struct vl53l8_k_module_t *p_module, void __user *p);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_set_device_parameters(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_ioctl_get_device_parameters(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_ioctl_get_error_info(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_ioctl_set_power_mode(struct vl53l8_k_module_t *p_module, void __user *p);
#else // SS_SUPPORT_SEC_CODE
int vl53l8_set_device_parameters(struct vl53l8_k_module_t *p_module, int config);

int vl53l8_ioctl_set_power_mode(struct vl53l8_k_module_t *p_module,
				void __user *p, enum vl53l5_power_states state);

#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
void vl53l8_last_error_counter(struct vl53l8_k_module_t *p_module, int err);
#endif // CONFIG_SENSORS_LAF_FAILURE_DEBUG
#endif // SS_SUPPORT_SEC_CODE

int vl53l8_ioctl_poll_data_ready(struct vl53l8_k_module_t *p_module);

int vl53l8_ioctl_read_p2p_calibration(struct vl53l8_k_module_t *p_module);

int vl53l8_ioctl_read_shape_calibration(struct vl53l8_k_module_t *p_module);

int vl53l8_perform_calibration(struct vl53l8_k_module_t *p_module, int flow);

int vl53l8_ioctl_read_generic_shape(struct vl53l8_k_module_t *p_module);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
int vl53l8_ioctl_set_ranging_rate(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_ioctl_set_integration_time_us(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_ioctl_set_asz_tuning(struct vl53l8_k_module_t *p_module,
			    void __user *p);
int vl53l8_ioctl_set_glare_filter_tuning(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_ioctl_set_transfer_speed_hz(struct vl53l8_k_module_t *p_module, void __user *p);
#else // SS_SUPPORT_SEC_CODE
int vl53l8_ioctl_set_ranging_rate(struct vl53l8_k_module_t *p_module, uint32_t rate);

int vl53l8_ioctl_set_integration_time_us(struct vl53l8_k_module_t *p_module, uint32_t integration);

int vl53l8_ioctl_set_asz_tuning(struct vl53l8_k_module_t *p_module,
			    void __user *p, struct vl53l8_k_asz_tuning_t *asz_layout);

#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
void vl53l8_check_ldo_onoff(struct vl53l8_k_module_t *data);
#endif // CONFIG_SENSORS_LAF_FAILURE_DEBUG

#ifdef CONFIG_SEPARATE_IO_CORE_POWER
int vl53l8_regulator_init_state(struct vl53l8_k_module_t *data);
#define ALL_VDD_ENABLED 0x7
#else // CONFIG_SEPARATE_IO_CORE_POWER
#define ALL_VDD_ENABLED 0x3
#endif

void vl53l8_power_onoff(struct vl53l8_k_module_t *p_module, bool on);
int vl53l8_ldo_onoff(struct vl53l8_k_module_t *p_module, int io, bool on);
int vl53l8_pin_control(struct vl53l8_k_module_t *p_module, bool on);

int vl53l8_ioctl_read_open_cal_p2p_calibration(struct vl53l8_k_module_t *p_module);
int vl53l8_ioctl_read_open_cal_shape_calibration(struct vl53l8_k_module_t *p_module);

int vl53l8_ioctl_get_cal_data(struct vl53l8_k_module_t *p_module, void __user *p);
int vl53l8_ioctl_get_status(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_ioctl_set_cal_data(struct vl53l8_k_module_t *p_module, void __user *p);
int vl53l8_ioctl_set_pass_fail(struct vl53l8_k_module_t *p_module, void __user *p);
int vl53l8_ioctl_set_file_list(struct vl53l8_k_module_t *p_module, void __user *p);

int vl53l8_input_report(struct vl53l8_k_module_t *p_module, int type, int cmd);

int vl53l8_set_integration_time_us(struct vl53l8_k_module_t *p_module, uint32_t integration);
int vl53l8_set_ranging_rate(struct vl53l8_k_module_t *p_module, uint32_t rate);
int vl53l8_set_transfer_speed_hz(struct vl53l8_k_module_t *p_module, unsigned int freq);

int vl53l8_load_factory_calibration(struct vl53l8_k_module_t *p_module);
int vl53l8_load_open_calibration(struct vl53l8_k_module_t *p_module);

void vl53l8_load_calibration(struct vl53l8_k_module_t *p_module);
void vl53l8_rotate_report_data_int32(int32_t *raw_data, int rotation);

#ifdef SS_SUPPORT_DATA_ROTATION
void vl53l8_rotate_report_data_u16(u16 *raw_data, int rotation);
void vl53l8_rotate_report_data_u32(u32 *raw_data, int rotation);
#endif // SS_SUPPORT_DATA_ROTATION
#endif // SS_SUPPORT_SEC_CODE

#endif
