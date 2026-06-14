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

#include "vl53l5_api_core.h"
#include "vl53l5_dci_core.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_version.h"
#include "vl53l5_trans.h"
#include "vl53l5_rom_boot.h"
#include "vl53l5_register_utils.h"
#include "vl53l5_error_handler.h"
#include "vl53l5_load_firmware.h"
#include "vl53l5_commands.h"
#include "vl53l5_checks.h"
#include "vl53l5_globals.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_core_map_bh.h"
#include "dci_structs.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform_user_config.h"
#include "vl53l5_api_power.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_DCI, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_DCI, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_DCI, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FLUSH(status, ...) \
	do { \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_DCI, status, ##__VA_ARGS__);\
	_FLUSH_TRACE_TO_OUTPUT();\
	} while (0)

#ifdef VL53L5_LOG_ENABLE

union _driver_build_t {
	uint32_t bytes;
	struct {
		uint32_t build_type : 2;
		uint32_t vl53l5_meta_data_on : 1;
		uint32_t vl53l5_common_data_on : 1;
		uint32_t vl53l5_rng_timing_data_on : 1;
		uint32_t vl53l5_amb_rate_kcps_per_spad_on : 1;
		uint32_t vl53l5_effective_spad_count_on : 1;
		uint32_t vl53l5_amb_dmax_mm_on : 1;
		uint32_t vl53l5_silicon_temp_degc_start_on : 1;
		uint32_t vl53l5_silicon_temp_degc_end_on : 1;
		uint32_t vl53l5_no_of_targets_on : 1;
		uint32_t vl53l5_zone_id_on : 1;
		uint32_t vl53l5_sequence_idx_on : 1;
		uint32_t vl53l5_peak_rate_kcps_per_spad_on : 1;
		uint32_t vl53l5_median_phase_on : 1;
		uint32_t vl53l5_rate_sigma_kcps_per_spad_on : 1;
		uint32_t vl53l5_range_sigma_mm_on : 1;
		uint32_t vl53l5_median_range_mm_on : 1;
		uint32_t vl53l5_min_range_delta_mm_on : 1;
		uint32_t vl53l5_max_range_delta_mm_on : 1;
		uint32_t vl53l5_target_reflectance_est_pc_on : 1;
		uint32_t vl53l5_target_status_on : 1;
		uint32_t vl53l5_ref_timing_data_on : 1;
		uint32_t vl53l5_ref_channel_data_on : 1;
		uint32_t vl53l5_ref_target_data_on : 1;
		uint32_t vl53l5_zone_thresh_status_bytes_on : 1;
		uint32_t vl53l5_dyn_xtalk_op_persistent_data_on : 1;
	};
};
#endif

#define HW_TRAP_ERROR_GROUP 0xf
#define UI_PAGE 2

#define TIMED_RETENTION_OR_INTERRUPT(p_flags) \
	((p_flags)->timed_retention || (p_flags)->timed_interrupt)

#define TIMED_RETENTION_AND_INTERRUPT(p_flags) \
	((p_flags)->timed_retention && (p_flags)->timed_interrupt)

#define ASSIGN_STATUS_IF_NOT_ERROR(result, status) \
do {\
	if (status != STATUS_OK)\
		(void)result;\
	else\
		status = result;\
} while (0)

static int32_t _check_flags_consistent(
	struct vl53l5_ranging_mode_flags_t *p_flags)
{
	int32_t status = VL53L5_ERROR_NONE;

	if (TIMED_RETENTION_AND_INTERRUPT(p_flags))
		status = VL53L5_ERROR_INVALID_PARAMS;

	return status;
}

static void _get_driver_version(struct vl53l5_version_t *p_version)
{
	p_version->driver.ver_major = VL53L5_VERSION_MAJOR;
	p_version->driver.ver_minor = VL53L5_VERSION_MINOR;
	p_version->driver.ver_build = VL53L5_VERSION_BUILD;
	p_version->driver.ver_revision = VL53L5_VERSION_REVISION;
}

#ifdef VL53L5_LOG_ENABLE

static uint32_t _get_driver_build_details(void)
{
	union _driver_build_t db = {0};

	db.build_type = 0;
#ifdef VL53L5_META_DATA_ON
	db.vl53l5_meta_data_on = 1;
#endif
#ifdef VL53L5_COMMON_DATA_ON
	db.vl53l5_common_data_on = 1;
#endif
#ifdef VL53L5_RNG_TIMING_DATA_ON
	db.vl53l5_rng_timing_data_on = 1;
#endif
#ifdef VL53L5_AMB_RATE_KCPS_PER_SPAD_ON
	db.vl53l5_amb_rate_kcps_per_spad_on = 1;
#endif
#ifdef VL53L5_EFFECTIVE_SPAD_COUNT_ON
	db.vl53l5_effective_spad_count_on = 1;
#endif
#ifdef VL53L5_AMB_DMAX_MM_ON
	db.vl53l5_amb_dmax_mm_on = 1;
#endif
#ifdef VL53L5_SILICON_TEMP_DEGC_START_ON
	db.vl53l5_silicon_temp_degc_start_on = 1;
#endif
#ifdef VL53L5_SILICON_TEMP_DEGC_END_ON
	db.vl53l5_silicon_temp_degc_end_on = 1;
#endif
#ifdef VL53L5_NO_OF_TARGETS_ON
	db.vl53l5_no_of_targets_on = 1;
#endif
#ifdef VL53L5_ZONE_ID_ON
	db.vl53l5_zone_id_on = 1;
#endif
#ifdef VL53L5_SEQUENCE_IDX_ON
	db.vl53l5_sequence_idx_on = 1;
#endif
#ifdef VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON
	db.vl53l5_peak_rate_kcps_per_spad_on = 1;
#endif
#ifdef VL53L5_MEDIAN_PHASE_ON
	db.vl53l5_median_phase_on = 1;
#endif
#ifdef VL53L5_RATE_SIGMA_KCPS_PER_SPAD_ON
	db.vl53l5_rate_sigma_kcps_per_spad_on = 1;
#endif
#ifdef VL53L5_RANGE_SIGMA_MM_ON
	db.vl53l5_range_sigma_mm_on = 1;
#endif
#ifdef VL53L5_MEDIAN_RANGE_MM_ON
	db.vl53l5_median_range_mm_on = 1;
#endif
#ifdef VL53L5_MIN_RANGE_DELTA_MM_ON
	db.vl53l5_min_range_delta_mm_on = 1;
#endif
#ifdef VL53L5_MAX_RANGE_DELTA_MM_ON
	db.vl53l5_max_range_delta_mm_on = 1;
#endif
#ifdef VL53L5_TARGET_REFLECTANCE_EST_PC_ON
	db.vl53l5_target_reflectance_est_pc_on = 1;
#endif
#ifdef VL53L5_TARGET_STATUS_ON
	db.vl53l5_target_status_on = 1;
#endif
#ifdef VL53L5_REF_TIMING_DATA_ON
	db.vl53l5_ref_timing_data_on = 1;
#endif
#ifdef VL53L5_REF_CHANNEL_DATA_ON
	db.vl53l5_ref_channel_data_on = 1;
#endif
#ifdef VL53L5_REF_TARGET_DATA_ON
	db.vl53l5_ref_target_data_on = 1;
#endif
#ifdef VL53L5_ZONE_THRESH_STATUS_BYTES_ON
	db.vl53l5_zone_thresh_status_bytes_on = 1;
#endif
#ifdef VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_ON
	db.vl53l5_dyn_xtalk_op_persistent_data_on = 1;
#endif
	return db.bytes;
}
#endif

static int32_t _write_byte(
	struct vl53l5_dev_handle_t *p_dev, uint16_t write_index, uint8_t value)
{
	return vl53l5_write_multi(p_dev, write_index, &value, 1);
}

static int32_t _decode_fgc(
	struct vl53l5_module_info_t *p_module_info, uint8_t *p_buff)
{
	int32_t status = STATUS_OK;
	uint32_t fgc_4 = 0;
	uint32_t fgc_9 = 0;

	struct dci_grp__fmt_traceability_t fmt = {0};
	uint8_t *p_str = NULL;

	LOG_FUNCTION_START("");

	fmt.st_fgc_0_u.bytes = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	fmt.st_fgc_1_u.bytes = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	fmt.st_fgc_2_u.bytes = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	fmt.st_fgc_3_u.bytes = vl53l5_decode_uint32_t(BYTE_4, p_buff);

	fgc_4 = ((fmt.st_fgc_0_u.fgc_4__6_3 & 0x0f) << 3) |
		 (fmt.st_fgc_1_u.fgc_4__2_0 & 0x07);
	fgc_9 = ((fmt.st_fgc_1_u.fgc_9__6 & 0x01) << 6) |
		 (fmt.st_fgc_2_u.fgc_9__5_0 & 0x3f);

	p_str = p_module_info->fgc;
	*p_str++ = fmt.st_fgc_0_u.fgc_0 + 32;
	*p_str++ = fmt.st_fgc_0_u.fgc_1 + 32;
	*p_str++ = fmt.st_fgc_0_u.fgc_2 + 32;
	*p_str++ = fmt.st_fgc_0_u.fgc_3 + 32;
	*p_str++ = fgc_4 + 32;
	*p_str++ = fmt.st_fgc_1_u.fgc_5 + 32;
	*p_str++ = fmt.st_fgc_1_u.fgc_6 + 32;
	*p_str++ = fmt.st_fgc_1_u.fgc_7 + 32;
	*p_str++ = fmt.st_fgc_1_u.fgc_8 + 32;
	*p_str++ = fgc_9 + 32;
	*p_str++ = fmt.st_fgc_2_u.fgc_10 + 32;
	*p_str++ = fmt.st_fgc_2_u.fgc_11 + 32;

	*p_str++ = 32;
	*p_str++ = 32;
	*p_str++ = 32;
	*p_str++ = 32;
	*p_str++ = 32;
	*p_str++ = 32;

	*p_str = '\0';

	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_fw_version(
	struct vl53l5_dev_handle_t *p_dev, struct vl53l5_version_t *p_version)
{
	int32_t status = STATUS_OK;
	uint8_t *p_buff = NULL;
	uint32_t block_header = 0;
	const uint32_t end_of_data_footer_index = 12;

	if (p_dev->host_dev.revision_id == 0x0C)
		p_buff = &VL53L5_COMMS_BUFF(p_dev)[VL53L5_UI_DUMMY_BYTES];
	else
		p_buff = VL53L5_COMMS_BUFF(p_dev);

	if ((p_buff[end_of_data_footer_index] & 0x0f) !=
			DCI_BH__P_TYPE__END_OF_DATA) {
		status = VL53L5_DCI_END_BLOCK_ERROR;
		goto exit;
	}

	block_header = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	if (block_header != VL53L5_FW_VERSION_BH) {

		status = VL53L5_VERSION_IDX_NOT_PRESENT;
		goto exit;
	}
	p_buff += BYTE_4;
	p_version->firmware.ver_revision =
					vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	p_version->firmware.ver_build = vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	p_version->firmware.ver_minor = vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	p_version->firmware.ver_major = vl53l5_decode_uint16_t(BYTE_2, p_buff);

	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Firmware version: %u.%u.%u.%u\n",
		    p_version->firmware.ver_major,
		    p_version->firmware.ver_minor,
		    p_version->firmware.ver_build,
		    p_version->firmware.ver_revision);
exit:
	return status;
}

static int32_t _decode_module_info(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_module_info_t *p_module_info)
{
	int32_t status = STATUS_OK;
	uint32_t block_header = 0;
	uint8_t *p_buff = NULL;
	const uint32_t end_of_data_footer_index =
		VL53L5_FMT_TRACEABILITY_SZ + VL53L5_EWS_TRACEABILITY_SZ
		+ (BYTE_2 * VL53L5_DCI_UI_PACKED_DATA_BH_SZ);

	if (p_dev->host_dev.revision_id == 0x0C)
		p_buff = &VL53L5_COMMS_BUFF(p_dev)[VL53L5_UI_DUMMY_BYTES];
	else
		p_buff = VL53L5_COMMS_BUFF(p_dev);

	if ((p_buff[end_of_data_footer_index] & 0x0f) !=
			DCI_BH__P_TYPE__END_OF_DATA) {
		status = VL53L5_DCI_END_BLOCK_ERROR;
		goto exit;
	}

	block_header = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	if (block_header != VL53L5_FMT_TRACEABILITY_BH) {
		status = VL53L5_IDX_MISSING_FROM_RETURN_PACKET;
		goto exit;
	}
	p_buff += BYTE_4;
	status = _decode_fgc(p_module_info, p_buff);

	p_buff += (6 * BYTE_4);

	block_header = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	if (block_header != VL53L5_EWS_TRACEABILITY_BH) {
		status = VL53L5_IDX_MISSING_FROM_RETURN_PACKET;
		goto exit;
	}

	p_buff += (BYTE_2 * BYTE_4);

	p_module_info->module_id_hi = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;

	p_module_info->module_id_lo = vl53l5_decode_uint32_t(BYTE_4, p_buff);

exit:
	return status;
}

static int32_t _decode_start_range_return(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint8_t *p_buff = NULL;
	uint32_t block_header = 0;
	struct dci_grp__ui_rng_data_addr_t rng_data_addr = {0};
	uint32_t end_of_data_footer_index =
		VL53L5_UI_RNG_DATA_ADDR_SZ + VL53L5_DCI_UI_PACKED_DATA_BH_SZ;

	if (p_dev->host_dev.revision_id == 0x0C)
		p_buff = &VL53L5_COMMS_BUFF(p_dev)[VL53L5_UI_DUMMY_BYTES];
	else
		p_buff = VL53L5_COMMS_BUFF(p_dev);

	if ((p_buff[end_of_data_footer_index] & 0x0f) !=
			DCI_BH__P_TYPE__END_OF_DATA) {
		status = VL53L5_DCI_END_BLOCK_ERROR;
		goto exit;
	}

	block_header = vl53l5_decode_uint32_t(BYTE_4, p_buff);
	if (block_header != VL53L5_UI_RNG_DATA_ADDR_BH) {
		status = VL53L5_RANGE_DATA_ADDR_NOT_PRESENT;
		goto exit;
	}
	p_buff += BYTE_4;
	rng_data_addr.ui_rng_data_int_start_addr
		= vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	rng_data_addr.ui_rng_data_int_end_addr
		= vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	rng_data_addr.ui_rng_data_dummy_bytes
		= vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	rng_data_addr.ui_rng_data_offset_bytes
		= vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	rng_data_addr.ui_rng_data_size_bytes
		= vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	rng_data_addr.ui_device_info_start_addr
		= vl53l5_decode_uint16_t(BYTE_2, p_buff);

	if (rng_data_addr.ui_rng_data_size_bytes == 0) {
		status = VL53L5_ERROR_INVALID_RETURN_DATA_SIZE;
		goto exit;
	}

	p_dev->host_dev.range_data_addr.dev_info_start_addr =
		rng_data_addr.ui_device_info_start_addr;

	p_dev->host_dev.range_data_addr.data_start_offset_bytes =
		VL53L5_DEV_INFO_BLOCK_SZ
		+ rng_data_addr.ui_rng_data_offset_bytes;

	p_dev->host_dev.range_data_addr.data_size_bytes =
		((rng_data_addr.ui_rng_data_size_bytes
		  - rng_data_addr.ui_rng_data_offset_bytes)
		  - VL53L5_HEADER_FOOTER_BLOCK_SZ);

	trace_print(
		VL53L5_TRACE_LEVEL_DEBUG,
		"True data start offset: %d, size %d\n",
		p_dev->host_dev.range_data_addr.data_start_offset_bytes,
		p_dev->host_dev.range_data_addr.data_size_bytes);
exit:
	return status;
}

static bool _check_if_status_requires_handler(int32_t status)
{
	bool is_required = false;

	if ((status <= VL53L5_MAX_FW_ERROR) && (status > VL53L5_MIN_FW_ERROR)) {
		is_required = false;
		goto exit;
	}

	switch (status) {
	case VL53L5_ERROR_TIME_OUT:
	case VL53L5_ERROR_BOOT_COMPLETE_TIMEOUT:
	case VL53L5_ERROR_MCU_IDLE_TIMEOUT:
	case VL53L5_ERROR_RANGE_DATA_READY_TIMEOUT:
	case VL53L5_ERROR_CMD_STATUS_TIMEOUT:
	case VL53L5_ERROR_UI_FW_STATUS_TIMEOUT:
	case VL53L5_ERROR_UI_RAM_WATCHDOG_TIMEOUT:
	case VL53L5_ERROR_MCU_ERROR_AT_ROM_BOOT:
	case VL53L5_ERROR_FALSE_MCU_ERROR_AT_ROM_BOOT:
	case VL53L5_ERROR_MCU_ERROR_AT_RAM_BOOT:
	case VL53L5_ERROR_FALSE_MCU_ERROR_AT_RAM_BOOT:
	case VL53L5_ERROR_MCU_ERROR_POWER_STATE:
	case VL53L5_ERROR_FALSE_MCU_ERROR_POWER_STATE:
	case VL53L5_DEV_INFO_MCU_ERROR:
	case VL53L5_FALSE_DEV_INFO_MCU_ERROR:
	case VL53L5_ERROR_MCU_ERROR_WAIT_STATE:
		is_required = true;
		break;
	default:
		is_required = false;
		break;
	}
exit:
	return is_required;
}

static int32_t _get_error_go2_status(
	struct vl53l5_dev_handle_t *p_dev, uint8_t current_page)
{
	int32_t status = STATUS_OK;
	struct common_grp__status_t status_struct = {0};
	union dci_union__go2_status_0_go1_u go2_status_0;
	union dci_union__go2_status_1_go1_u go2_status_1;

	LOG_FUNCTION_START("");

	status = vl53l5_check_status_registers(
		p_dev, &go2_status_0, &go2_status_1, false,
		current_page);

	if (status != STATUS_OK)
		goto exit;

	if (go2_status_0.mcu__hw_trap_flag_go1) {

		if (go2_status_1.bytes != 0x00) {
			status_struct.status__group = HW_TRAP_ERROR_GROUP;
			status_struct.status__type = go2_status_1.bytes;
		} else {
			goto exit;
		}
	} else if ((go2_status_0.mcu__error_flag_go1) ||
				(go2_status_1.mcu__warning_flag_go1)) {
		if (current_page != UI_PAGE) {
			status = vl53l5_set_page(p_dev, UI_PAGE);
			if (status != STATUS_OK)
				goto exit;
		}

		if (go2_status_1.mcu__warning_flag_go1)

			status = vl53l5_get_secondary_warning_info(
				p_dev, &status_struct);
		else

			status = vl53l5_get_secondary_error_info(
				p_dev, &status_struct);

		if (current_page != UI_PAGE) {
			if (status != STATUS_OK)
				(void)vl53l5_set_page(p_dev, current_page);
			else
				status = vl53l5_set_page(p_dev, current_page);
		}
		if (status != STATUS_OK)
			goto exit;
	} else {

		goto exit;
	}

	if (status != STATUS_OK)
		goto exit;

	trace_print(
		VL53L5_TRACE_LEVEL_DEBUG,
		"group:%i type:%i stage:%i debug0:%i debug1:%i debug2:%i\n",
		status_struct.status__group,
		status_struct.status__type,
		status_struct.status__stage_id,
		status_struct.status__debug_0,
		status_struct.status__debug_1,
		status_struct.status__debug_2);
	status = vl53l5_compose_fw_status_code(p_dev, &status_struct);

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

static int32_t _get_dev_params(struct vl53l5_dev_handle_t *p_dev,
	uint32_t *p_block_headers,
	uint32_t num_block_headers,
	bool encode_version)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = vl53l5_encode_block_headers(
		p_dev, p_block_headers, num_block_headers, encode_version);
	if (status != STATUS_OK)
		goto exit;

	status = vl53l5_execute_command(p_dev, DCI_CMD_ID__GET_PARMS);

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

static int32_t _set_xshut_bypass(
	struct vl53l5_dev_handle_t *p_dev, uint8_t state)
{
	int32_t status = VL53L5_ERROR_NONE;

	status = vl53l5_set_page(p_dev, 0);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_set_xshut_bypass(p_dev, state);

	if (status < STATUS_OK)
		(void)vl53l5_set_page(p_dev, UI_PAGE);
	else
		status = vl53l5_set_page(p_dev, UI_PAGE);

exit:
	return status;
}

static int32_t _provoke_mcu_error(
	struct vl53l5_dev_handle_t *p_dev, int32_t *p_boot_status)
{
	int32_t status = VL53L5_ERROR_NONE;

	status = _write_byte(p_dev, 0x15, 22);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x14, 1);
	if (status < STATUS_OK)
		goto exit_reset_15;

	*p_boot_status = vl53l5_wait_mcu_boot(
		p_dev, VL53L5_BOOT_STATE_ERROR, VL53L5_STOP_COMMAND_TIMEOUT,
		VL53L5_DEFAULT_COMMS_POLLING_DELAY_MS);
	if (*p_boot_status == STATUS_OK) {
		status = VL53L5_ERROR_TIME_OUT;
		goto exit_reset_14;
	}

exit:
	return status;

exit_reset_14:
	(void)_write_byte(p_dev, 0x14, 0);

exit_reset_15:
	(void)_write_byte(p_dev, 0x15, 0);

	return status;
}

static int32_t _undo_provoke_mcu_error(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;

	status = _write_byte(p_dev, 0x14, 0);

	ASSIGN_STATUS_IF_NOT_ERROR(_write_byte(p_dev, 0x15, 0), status);

	return status;
}

static int32_t _force_timed_mode_stop(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;
	int32_t boot_status = VL53L5_ERROR_NONE;
	const int32_t expected_error_code = (int32_t)0xE2048200;

	status = vl53l5_set_page(p_dev, 0);
	if (status != STATUS_OK)
		goto exit;

	status = _provoke_mcu_error(p_dev, &boot_status);
	if (status != STATUS_OK)
		goto exit_xshut;

	if (!_check_if_status_requires_handler(boot_status)) {
		status = boot_status;
		goto exit_undo;
	}

	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "boot_status Value: %d\n",
		    boot_status);
	status = _get_error_go2_status(p_dev, 0);

	if (status == expected_error_code)
		status = VL53L5_ERROR_NONE;

exit_undo:
	ASSIGN_STATUS_IF_NOT_ERROR(_undo_provoke_mcu_error(p_dev), status);

exit_xshut:
	ASSIGN_STATUS_IF_NOT_ERROR(vl53l5_set_xshut_bypass(p_dev, 0), status);

	ASSIGN_STATUS_IF_NOT_ERROR(vl53l5_set_page(p_dev, UI_PAGE), status);

exit:
	return status;
}

int32_t vl53l5_init(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

#ifdef VL53L5_LOG_ENABLE
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Driver Build Value: 0x%x\n",
		    _get_driver_build_details());
#endif
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Max Targets Value: %d\n",
		    VL53L5_MAX_TARGETS);
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Max Zones Value: %d\n",
		    VL53L5_MAX_ZONES);
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Polling Delay Value: %d\n",
		    VL53L5_DEFAULT_COMMS_POLLING_DELAY_MS);

	trace_print(
		VL53L5_TRACE_LEVEL_INFO,
		"VL53L5 Bare Driver Version %d.%d.%02d.%04d\n",
		VL53L5_VERSION_MAJOR,
		VL53L5_VERSION_MINOR,
		VL53L5_VERSION_BUILD,
		VL53L5_VERSION_REVISION);

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = vl53l5_check_rom_firmware_boot(p_dev);
	if (status < STATUS_OK)
		goto exit;

	if (p_dev->host_dev.device_booted == true &&
		p_dev->host_dev.version_match.map_version_match != true) {
		status = vl53l5_check_map_version(p_dev);
		goto exit;
	}

	if (p_dev->host_dev.device_booted == true)
		goto exit;

	if (p_dev->host_dev.device_booted == false &&
	    !VL53L5_FW_BUFF_ISNULL(p_dev)) {
		status = vl53l5_load_firmware(p_dev);
		if (status < STATUS_OK)
			goto exit;
	} else if (p_dev->host_dev.revision_id == 0x0C) {
		goto exit_version;
	} else {
		status = VL53L5_ERROR_FW_BUFF_NOT_FOUND;
		goto exit;
	}

exit_version:

	status = vl53l5_check_map_version(p_dev);
	if (status != STATUS_OK)
		goto exit;

	p_dev->host_dev.power_state = VL53L5_POWER_STATE_HP_IDLE;

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_term(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	VL53L5_RESET_COMMS_BUFF(p_dev);
	p_dev->host_dev.version_match.map_version_match = false;

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_get_version(
	struct vl53l5_dev_handle_t *p_dev, struct vl53l5_version_t *p_version)
{
	int32_t status = STATUS_OK;
	uint32_t block_headers[] = {VL53L5_FW_VERSION_BH};

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_version)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = VL53L5_GET_VERSION_CHECK_STATUS(p_dev);

	if (status < STATUS_OK)
		goto exit;

	_get_driver_version(p_version);

	status = _get_dev_params(p_dev, block_headers, 1, false);
	if (status < STATUS_OK)
		goto exit;

	status = _decode_fw_version(p_dev, p_version);

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_get_module_info(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_module_info_t *p_module_info)
{
	int32_t status = STATUS_OK;
	uint32_t block_headers[] = {
		VL53L5_FMT_TRACEABILITY_BH,
		VL53L5_EWS_TRACEABILITY_BH
	};

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_module_info)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = VL53L5_GET_VERSION_CHECK_STATUS(p_dev);

	if (status < STATUS_OK)
		goto exit;

	status = _get_dev_params(p_dev, block_headers, 2, false);
	if (status < STATUS_OK)
		goto exit;

	status = _decode_module_info(p_dev, p_module_info);

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_start(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_ranging_mode_flags_t *p_flags)
{
	int32_t status = VL53L5_ERROR_NONE;
	bool bypass_set = false;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = VL53L5_GET_VERSION_CHECK_STATUS(p_dev);
	if (status < STATUS_OK)
		goto exit;

	if (!VL53L5_ISNULL(p_flags)) {

		status = _check_flags_consistent(p_flags);
		if (status != STATUS_OK)
			goto exit;

		if (TIMED_RETENTION_OR_INTERRUPT(p_flags)) {
			status = _set_xshut_bypass(p_dev, 1);
			if (status != STATUS_OK)
				goto exit;

			bypass_set = true;
		}
	}

	VL53L5_RESET_COMMS_BUFF(p_dev);
	status = vl53l5_execute_command(p_dev, DCI_CMD_ID__START_RANGE);
	if (status != STATUS_OK)
		goto exit_reset_bypass;

	status = _decode_start_range_return(p_dev);
	if (status != STATUS_OK)
		goto exit_reset_bypass;

	p_dev->host_dev.ui_dev_info.dev_info__ui_stream_count = 0xFF;

exit_reset_bypass:
	if ((status != STATUS_OK) && bypass_set)
		(void)_set_xshut_bypass(p_dev, 0);

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_stop(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_ranging_mode_flags_t *p_flags)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = VL53L5_GET_VERSION_CHECK_STATUS(p_dev);

	if (status < STATUS_OK)
		goto exit;

	if (!VL53L5_ISNULL(p_flags)) {

		status = _check_flags_consistent(p_flags);
		if (status != STATUS_OK)
			goto exit;

		if (TIMED_RETENTION_OR_INTERRUPT(p_flags)) {
			status = _force_timed_mode_stop(p_dev);
			if (status != STATUS_OK)
				goto exit;
		} else {

			VL53L5_RESET_COMMS_BUFF(p_dev);
			status = vl53l5_execute_command(p_dev,
							DCI_CMD_ID__STOP_RANGE);
			if (status != STATUS_OK)
				goto exit;
		}
	} else {

		VL53L5_RESET_COMMS_BUFF(p_dev);
		status = vl53l5_execute_command(p_dev, DCI_CMD_ID__STOP_RANGE);
		if (status != STATUS_OK)
			goto exit;
	}

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_read_device_error(
	struct vl53l5_dev_handle_t *p_dev,
	int32_t latest_status)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (!_check_if_status_requires_handler(latest_status)) {

		status = latest_status;
		goto exit;
	}

	status = _get_error_go2_status(p_dev, UI_PAGE);

exit:

	if (status == STATUS_OK)
		status = latest_status;

	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_get_device_parameters(
	struct vl53l5_dev_handle_t *p_dev,
	uint32_t *p_block_headers,
	uint32_t num_block_headers)
{
	int32_t status = STATUS_OK;
	uint8_t *p_buff = NULL;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_block_headers)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (num_block_headers == 0) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (p_dev->host_dev.revision_id == 0x0C)
		p_buff = &VL53L5_COMMS_BUFF(p_dev)[VL53L5_UI_DUMMY_BYTES];
	else
		p_buff = VL53L5_COMMS_BUFF(p_dev);

	status = VL53L5_GET_VERSION_CHECK_STATUS(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _get_dev_params(
			p_dev, p_block_headers, num_block_headers, true);
	if (status != STATUS_OK)
		goto exit;

	status = vl53l5_test_map_version(p_dev, p_buff);
	if (status != STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_set_device_parameters(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t *p_buff,
	uint32_t buff_count)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_buff)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (buff_count == 0) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = VL53L5_GET_VERSION_CHECK_STATUS(p_dev);
	if (status < STATUS_OK)
		goto exit;

	if (buff_count > VL53L5_COMMS_BUFF_MAX_COUNT(p_dev)) {
		status = VL53L5_MAX_BUFFER_SIZE_REACHED;
		goto exit;
	}

	if (p_buff != VL53L5_COMMS_BUFF(p_dev))
		memcpy(VL53L5_COMMS_BUFF(p_dev), p_buff, buff_count);

	VL53L5_COMMS_BUFF_COUNT(p_dev) = buff_count;

	status = vl53l5_test_map_version(p_dev, VL53L5_COMMS_BUFF(p_dev));
	if (status != STATUS_OK)
		goto exit;

	status = vl53l5_execute_command(p_dev, DCI_CMD_ID__SET_PARMS);
	if (status != STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

static int32_t _build_buffer(struct vl53l5_dev_handle_t *p_dev,
			     uint32_t bh, uint32_t value)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint8_t buffer[(BYTE_4 * BYTE_4)] = {0x40, 0x00, 0x00, 0x54,
					     0x0b, 0x00, 0x05, 0x00,};
	uint8_t *p_buff = &buffer[(BYTE_2 * BYTE_4)];

	if (value == 0) {
		status = VL53L5_ERROR_INVALID_VALUE;
		goto exit;
	}

	vl53l5_encode_uint32_t(bh, BYTE_4, p_buff);
	p_buff += BYTE_4;
	vl53l5_encode_uint32_t(value, BYTE_4, p_buff);

	status = vl53l5_set_device_parameters(p_dev, buffer, sizeof(buffer));
	if (status != VL53L5_ERROR_NONE)
		goto exit;

exit:
	return status;
}

int32_t vl53l5_set_ranging_rate_hz(
	struct vl53l5_dev_handle_t *p_dev,
	uint32_t ranging_rate_hz)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t rr = ranging_rate_hz << (BYTE_2 * BYTE_4);

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = _build_buffer(p_dev, VL53L5_RNG_RATE_CFG_BH, rr);

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

int32_t vl53l5_set_integration_time_us(
	struct vl53l5_dev_handle_t *p_dev,
	uint32_t integration_time_us)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = _build_buffer(
		p_dev, VL53L5_INT_MAX_CFG_BH, integration_time_us);

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}
