/*******************************************************************************
* Copyright (c) 2023, STMicroelectronics - All Rights Reserved
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

#ifndef _VL53L8_K_MODULE_DATA_H_
#define _VL53L8_K_MODULE_DATA_H_

#include <linux/miscdevice.h>
#include <linux/completion.h>
#include "vl53l8_k_user_api.h"
#include "vl53l5_platform_user_data.h"
#include "vl53l8_k_state_def.h"
#include "vl53l8_k_spi_def.h"
#include "vl53l8_k_gpio_def.h"
#include "vl53l8_k_driver_config.h"
#include "vl53l8_k_glare_filter.h"

#ifdef SS_SUPPORT_SEC_CODE
#include <linux/regulator/consumer.h>
#include <uapi/linux/sensor/range_sensor.h>
#ifdef CONFIG_SENSORS_VL53L8_QCOM
#include <linux/sensor/sensors_core.h> //for sec dump
#endif
#include <linux/input.h> 	//for input_dev
#endif // SS_SUPPORT_SEC_CODE
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
#define MAX_TABLE 152
#endif
#define AVDD	0
#define IOVDD	1
#define COREVDD	2
#define VDD_MAX_CNT	3
struct vl53l8_k_module_t {
#ifdef STM_VL53L5_KOR_CODE
		struct input_dev *input_dev_ps;
		/*!< input device used for sending event */
#endif 

	int id;

	char name[64];

	struct miscdevice miscdev;

	struct vl53l5_dev_handle_t stdev;

	const char *firmware_name;

	int comms_type;

	int config_preset;

	struct mutex mutex;

	struct mutex cal_mutex;

	struct vl53l8_k_spi_data_t spi_info;

	struct vl53l8_k_gpio_settings_t gpio;

	unsigned char comms_buffer[VL53L5_COMMS_BUFFER_SIZE_BYTES];

	enum vl53l8_k_state_preset state_preset;

	struct vl53l5_ranging_mode_flags_t ranging_flags;

	enum vl53l5_power_states power_state;

	enum vl53l8_range_servicing_mode range_mode;

	struct range_t {
#ifndef SS_SUPPORT_SEC_CODE
		int count;
#else // SS_SUPPORT_SEC_CODE
		unsigned long count;
#endif // SS_SUPPORT_SEC_CODE

		bool is_valid;

		struct vl53l5_range_data_t data;
	} range;

	struct calibration_data_t {

		struct vl53l5_version_t version;

		struct vl53l5_patch_version_t patch_version;

		struct vl53l5_module_info_t info;

		struct vl53l5_calibration_data_t cal_data;
	} calibration;

	int polling_count;
#ifdef SS_SUPPORT_SEC_CODE
	int ioctl_enable_status;
#endif // SS_SUPPORT_SEC_CODE
	unsigned int polling_start_time_ms;

	unsigned int polling_sleep_time_ms;

	bool irq_is_active;

	bool irq_is_running;

	int irq_val;

	bool irq_wq_is_running;

	struct workqueue_struct *irq_wq;

	struct work_struct data_work;

	struct delayed_work dwork;

	struct list_head reader_list;

	wait_queue_head_t wait_queue;

	int last_driver_error;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
	struct errdata_t {
		int last_error_code;
		u8 last_error_cnt;
	} errdata[MAX_TABLE];
#endif // CONFIG_SENSORS_LAF_FAILURE_DEBUG
	struct vl53l5_module_info_t m_info;

	struct vl53l8_k_glare_filter_tuning_t gf_tuning;

	struct vl53l5_patch_version_t patch_ver;

	unsigned int transfer_speed_hz;
	
	struct vl53l8_k_bin_version bin_version;

#ifdef SS_SUPPORT_SEC_CODE
	struct vl53l8_cal_data_t cal_data;
	struct device *factory_device;
	struct input_dev *input_dev; //for input_dev
	struct regulator *avdd_vreg;
	struct regulator *iovdd_vreg;
#ifdef CONFIG_SEPARATE_IO_CORE_POWER
	struct regulator *corevdd_vreg;
	struct delayed_work power_delayed_work;
#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
	struct work_struct resume_work;
#endif // CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
#endif // CONFIG_SEPARATE_IO_CORE_POWER
	struct notifier_block dump_nb;
	struct vl53l5_range_fac_results_t {
#ifdef VL53L5_PER_ZONE_RESULTS_ON
		struct vl53l5_range_per_zone_results_t per_zone_results;
#endif

#ifdef VL53L5_PER_TGT_RESULTS_ON
		struct vl53l5_range_per_tgt_results_t per_tgt_results;
#endif
#ifdef VL53L5_D16_PER_TARGET_DATA_ON
		struct vl53l5_d16_per_tgt_results_t d16_per_target_data;
#endif
#ifdef VL53L5_GD_OP_STATUS_ON
		struct vl53l5_gd_op_status_t gd_op_status;
#endif 

	} range_data;
	struct range_sensor_data_t	af_range_data;
	struct stuck_t {
		uint8_t stuck_cnt;
		bool stuck_check;
	} stuck;

	const char *genshape_name;
	const char *avdd_vreg_name;
	const char *iovdd_vreg_name;
#ifdef CONFIG_SEPARATE_IO_CORE_POWER
	const char *corevdd_vreg_name;
#endif // CONFIG_SEPARATE_IO_CORE_POWER
	uint32_t fac_rotation_mode;
	uint32_t rotation_mode;
	uint32_t force_suspend_count;
	uint32_t integration_init;
	uint32_t rate_init;

	int32_t data[64];
	int32_t number_of_zones;
	int32_t print_counter;
	int32_t min;
	int32_t avg;
	int32_t max;
	int32_t max_peak_signal;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
	int8_t ldo_status;
#endif // CONFIG_SENSORS_LAF_FAILURE_DEBUG
	int8_t enabled;
	int8_t test_mode;
	int8_t failed_count;
	int8_t update_flag;
	int8_t pass_fail_flag;
	int8_t file_list;
	uint8_t ldo_prev_state[VDD_MAX_CNT];

	bool load_calibration;
	bool read_p2p_cal_data;
	bool read_data_valid;
	bool suspend_state;
	bool probe_done;
	bool sensors_register_done;
#endif // SS_SUPPORT_SEC_CODE
	// ASZs Tuning Parameter
	uint32_t asz_0_ll_zone_id;
	uint32_t asz_1_ll_zone_id;
	uint32_t asz_2_ll_zone_id;
	uint32_t asz_3_ll_zone_id;

	// Integration time 2msec ~ 15msec
	uint32_t integration;
	// Sampling rate : 1hz ~15hz
	uint32_t rate;
};

#ifdef SS_SUPPORT_SEC_CODE
#define CMD_READ_CAL_FILE		0 // 0x1
#define CMD_WRITE_CAL_FILE		1 // 0x2
#define CMD_WRITE_P2P_FILE		2 // 0x4
#define CMD_WRITE_SHAPE_FILE	3 // 0x8
#define CMD_READ_P2P_CAL_FILE		4 // 0x10
#define CMD_READ_SHAPE_CAL_FILE		5 // 0x20
#define CMD_CHECK_CAL_FILE_TYPE		6 // 0x40
#define CMD_DELTE_OPENCAL_FILE		7 // 0x80


#define TIMEOUT_CNT 50
#endif // SS_SUPPORT_SEC_CODE

#endif
