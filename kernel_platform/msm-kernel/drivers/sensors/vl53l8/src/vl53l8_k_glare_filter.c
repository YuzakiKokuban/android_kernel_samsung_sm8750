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

#include "vl53l8_k_glare_filter.h"
#include "vl53l8_k_error_codes.h"
#include "dci_patch_union_structs.h"
#include "vl53l5_platform_algo_limits.h"
#include "vl53l5_results_build_config.h"
#include "vl53l8_k_logging.h"

#define VL53L8_TARGET_STATUS__TARGETDUETOGLARE ((uint8_t) 18U)

static uint32_t _interpolate_threshold(int32_t tgt_x, int32_t x1, int32_t x2,
				       uint32_t y1, uint32_t y2)
{
	uint32_t value = 0;
	int32_t diff0 = 0;
	int32_t diff1 = 0;
	int32_t denom = 0;

	if (VL53L8_K_Y_SCALE_FACTOR == 0) {
		value = VL53L8_K_GF_DIVIDE_BY_ZERO_ERROR;
		goto out;
	}

	diff0 = tgt_x - x1;
	diff1 = ((int32_t)y2 - (int32_t)y1) / VL53L8_K_Y_SCALE_FACTOR;
	value = diff0 * diff1;
	denom = x2 - x1;

	if (denom == 0) {
		value = VL53L8_K_GF_DIVIDE_BY_ZERO_ERROR;
		goto out;
	}

	value = value / denom;
	value *= VL53L8_K_Y_SCALE_FACTOR;
	value += (int32_t)y1;

	vl53l8_k_log_debug("%d, %d, %u, %u, %d, %d\n",
			    x1, x2, y1, y2, tgt_x, value);

out:
	return value;
}

static int32_t vl53l8_k_glare_detect(int32_t tgt_range, uint32_t tgt_peak_x16,
			struct vl53l8_k_glare_filter_tuning_t *p_tuning,
			uint8_t *glare_detected)
{

	int32_t status = VL53L5_ERROR_NONE;
	uint32_t peak_threshold_x16 = 0;
	int32_t clipped_range = 0;
	uint32_t glare_threshold_numerator = 0;
	uint32_t i = 0;

	if (tgt_range > p_tuning->max_filter_range) {
		*glare_detected = 0;
		goto out;
	}

	if (tgt_range < p_tuning->lut_range[0]) {
		glare_threshold_numerator =
			p_tuning->threshold_numerator[0];
	} else if (tgt_range >= p_tuning->lut_range[VL53L8_K_GLARE_LUT_SIZE - 1]) {
		glare_threshold_numerator =
			p_tuning->threshold_numerator[VL53L8_K_GLARE_LUT_SIZE - 1];
	} else {

		for (i = 1; i < (VL53L8_K_GLARE_LUT_SIZE - 1); i++) {
			if (p_tuning->lut_range[i] > tgt_range)
				break;
		}
		i--;

		glare_threshold_numerator = _interpolate_threshold(
				tgt_range,
				p_tuning->lut_range[i],
				p_tuning->lut_range[i + 1],
				p_tuning->threshold_numerator[i],
				p_tuning->threshold_numerator[i + 1]);
	}

	if (tgt_range < p_tuning->range_min_clip)
		clipped_range = p_tuning->range_min_clip;
	else
		clipped_range = tgt_range;

	peak_threshold_x16 = glare_threshold_numerator /
				(clipped_range * clipped_range);
	vl53l8_k_log_debug(
		"tgt_range=%u, tgt_peak_x16=%u, peak_threshold_x16=%u\n",
		tgt_range, tgt_peak_x16, peak_threshold_x16);

	if (tgt_peak_x16 < peak_threshold_x16)
		*glare_detected = 1;
	else
		*glare_detected = 0;

	vl53l8_k_log_debug("glare detected: %u", *glare_detected);
out:
	if (status < VL53L5_ERROR_NONE)
		vl53l8_k_log_error("ERROR: %i\n", status);
	return status;
};

#if defined(VL53L5_TARGET_STATUS_ON) && \
	defined(VL53L5_MEDIAN_RANGE_MM_ON) && \
	defined(VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON) && \
	defined(VL53L5_NO_OF_TARGETS_ON)

static void copy_target_data(struct vl53l5_range_per_tgt_results_t *pper_tgt,
				int dest_idx, int src_idx)
{
#ifdef VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON
	pper_tgt->peak_rate_kcps_per_spad[dest_idx] =
		pper_tgt->peak_rate_kcps_per_spad[src_idx];
#endif
#ifdef VL53L5_MEDIAN_PHASE_ON
	pper_tgt->median_phase[dest_idx] =
		pper_tgt->median_phase[src_idx];
#endif
#ifdef VL53L5_RATE_SIGMA_KCPS_PER_SPAD_ON
	pper_tgt->rate_sigma_kcps_per_spad[dest_idx] =
		pper_tgt->rate_sigma_kcps_per_spad[src_idx];
#endif
#ifdef VL53L5_TARGET_ZSCORE_ON
	pper_tgt->target_zscore[dest_idx] = pper_tgt->target_zscore[src_idx];
#endif
#ifdef VL53L5_RANGE_SIGMA_MM_ON
	pper_tgt->range_sigma_mm[dest_idx] = pper_tgt->range_sigma_mm[src_idx];
#endif
#ifdef VL53L5_MEDIAN_RANGE_MM_ON
	pper_tgt->median_range_mm[dest_idx] = pper_tgt->median_range_mm[src_idx];
#endif
#ifdef VL53L5_START_RANGE_MM_ON
	pper_tgt->start_range_mm[dest_idx] = pper_tgt->start_range_mm[src_idx];
#endif
#ifdef VL53L5_END_RANGE_MM_ON
	pper_tgt->end_range_mm[dest_idx] = pper_tgt->end_range_mm[src_idx];
#endif
#ifdef VL53L5_MIN_RANGE_DELTA_MM_ON
	pper_tgt->min_range_delta_mm[dest_idx] =
		pper_tgt->min_range_delta_mm[src_idx];
#endif
#ifdef VL53L5_MAX_RANGE_DELTA_MM_ON
	pper_tgt->max_range_delta_mm[dest_idx] =
		pper_tgt->max_range_delta_mm[src_idx];
#endif
#ifdef VL53L5_TARGET_REFLECTANCE_EST_PC_ON
	pper_tgt->target_reflectance_est_pc[dest_idx] =
		pper_tgt->target_reflectance_est_pc[src_idx];
#endif
#ifdef VL53L5_TARGET_STATUS_ON
	pper_tgt->target_status[dest_idx] = pper_tgt->target_status[src_idx];
#endif

	return;
}

static int32_t vl53l8_k_median_glare_filter(
			struct vl53l8_k_glare_filter_tuning_t *p_tuning,
			struct vl53l5_range_data_t *p_results)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t row = 0;
	uint32_t col = 0;
	uint32_t zone = 0;
	uint32_t idx = 0;
	uint32_t asz = 0;
	uint32_t grid_size = 0;
	uint32_t tgt_idx = 0;
	int32_t range = 0;
	uint32_t peak_rate_x16 = 0;
	uint8_t glare_detected = 0;
#if defined(VL53L5_RESULTS_DATA_ENABLED) && !defined(VL53L5_PATCH_DATA_ENABLED)
	struct vl53l5_range_results_t *presults = &p_results->core;
#endif
#if !defined(VL53L5_RESULTS_DATA_ENABLED) && defined(VL53L5_PATCH_DATA_ENABLED)
	struct vl53l5_tcpm_patch_0_results_dev_t *presults =
		&p_results->tcpm_0_patch;
#endif
	uint32_t no_of_zones =
		(uint32_t)presults->common_data.rng__no_of_zones;
	uint32_t no_of_asz_zones =
		(uint32_t)presults->common_data.rng__acc__no_of_zones;
	uint32_t max_targets_per_zone =
		(uint32_t)presults->common_data.rng__max_targets_per_zone;
	struct vl53l5_range_per_tgt_results_t *pper_tgt =
		&presults->per_tgt_results;
	struct vl53l5_range_per_zone_results_t *pper_zone =
		&presults->per_zone_results;

	if (no_of_zones == 64)
		grid_size = 8;
	else
		grid_size = 4;

	for (tgt_idx = 0 ; tgt_idx < max_targets_per_zone ; tgt_idx++) {
		for (row = 0; row < grid_size; row++) {
			for (col = 0; col < grid_size; col++) {
				zone = (row * grid_size) + col;
				idx = (zone * max_targets_per_zone) + tgt_idx;
				if (tgt_idx < pper_zone->rng__no_of_targets[zone]) {
					range = pper_tgt->median_range_mm[idx] >>
					VL53L8_K_MEDIAN_RANGE_FRACTIONAL_BITS;
					peak_rate_x16 = pper_tgt->peak_rate_kcps_per_spad[idx] >>
					(VL53L8_K_PEAK_RATE_FRACTIONAL_BITS - 4);

					status = vl53l8_k_glare_detect(
						range, peak_rate_x16, p_tuning,
						&glare_detected);
					if (status < VL53L5_ERROR_NONE)
						goto out;
					if (glare_detected)
						pper_tgt->target_status[idx] =
						VL53L8_TARGET_STATUS__TARGETDUETOGLARE;

				}
			}
		}
		for (asz = 0; asz < no_of_asz_zones; asz++) {
			zone = (grid_size * grid_size) + asz;
			idx = (zone * max_targets_per_zone) + tgt_idx;
			if (tgt_idx < pper_zone->rng__no_of_targets[zone]) {
				range = pper_tgt->median_range_mm[idx] >>
					VL53L8_K_MEDIAN_RANGE_FRACTIONAL_BITS;
				peak_rate_x16 = pper_tgt->peak_rate_kcps_per_spad[idx] >>
					(VL53L8_K_PEAK_RATE_FRACTIONAL_BITS - 4);
				status = vl53l8_k_glare_detect(
					range, peak_rate_x16, p_tuning,
					&glare_detected);
				if (status < VL53L5_ERROR_NONE)
					goto out;
				if (glare_detected)
					pper_tgt->target_status[idx] =
					VL53L8_TARGET_STATUS__TARGETDUETOGLARE;
			}
		}
	}
out:
	if (status < VL53L5_ERROR_NONE)
		vl53l8_k_log_error("ERROR: %i\n", status);

	return status;
}

static int32_t vl53l8_k_median_glare_remove(
			struct vl53l8_k_glare_filter_tuning_t *p_tuning,
			struct vl53l5_range_data_t *p_results)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t row = 0;
	uint32_t col = 0;
	uint32_t zone = 0;
	uint32_t idx = 0;
	uint32_t asz = 0;
	uint32_t grid_size = 0;
	uint32_t tgt_idx = 0;
	uint8_t tgt_status = 0;
#if defined(VL53L5_RESULTS_DATA_ENABLED) && !defined(VL53L5_PATCH_DATA_ENABLED)
	struct vl53l5_range_results_t *presults = &p_results->core;
#endif
#if !defined(VL53L5_RESULTS_DATA_ENABLED) && defined(VL53L5_PATCH_DATA_ENABLED)
	struct vl53l5_tcpm_patch_0_results_dev_t *presults =
		&p_results->tcpm_0_patch;
#endif
	uint32_t no_of_zones =
		(uint32_t)presults->common_data.rng__no_of_zones;
	uint32_t no_of_asz_zones =
		(uint32_t)presults->common_data.rng__acc__no_of_zones;
	uint32_t max_targets_per_zone =
		(uint32_t)presults->common_data.rng__max_targets_per_zone;
	struct vl53l5_range_per_tgt_results_t *pper_tgt =
		&presults->per_tgt_results;
	struct vl53l5_range_per_zone_results_t *pper_zone =
		&presults->per_zone_results;

	if (p_tuning->remove_glare_targets && (max_targets_per_zone > 2)) {
		vl53l8_k_log_error("ERROR: only supports a maximum of 2 targets\n");
		status = VL53L8_K_GF_INVALID_MAX_TARGETS_ERROR;
		goto out;
	}

	if (no_of_zones == 64)
		grid_size = 8;
	else
		grid_size = 4;

	for (tgt_idx = 0 ; tgt_idx < max_targets_per_zone ; tgt_idx++) {
		for (row = 0; row < grid_size; row++) {
			for (col = 0; col < grid_size; col++) {
				zone = (row * grid_size) + col;
				idx = (zone * max_targets_per_zone) + tgt_idx;
				if (tgt_idx < pper_zone->rng__no_of_targets[zone]) {
					tgt_status = pper_tgt->target_status[idx];
					if (tgt_status == VL53L8_TARGET_STATUS__TARGETDUETOGLARE) {
						if (tgt_idx == 0 && pper_zone->rng__no_of_targets[zone] == 2)
							copy_target_data(pper_tgt, idx, idx+1);

						pper_zone->rng__no_of_targets[zone]--;
					}
				}
			}
		}

		for (asz = 0; asz < no_of_asz_zones; asz++) {
			zone = (grid_size * grid_size) + asz;
			idx = (zone * max_targets_per_zone) + tgt_idx;
			if (tgt_idx < pper_zone->rng__no_of_targets[zone]) {
				tgt_status = pper_tgt->target_status[idx];
				if (tgt_status == VL53L8_TARGET_STATUS__TARGETDUETOGLARE) {
					if (tgt_idx == 0 && pper_zone->rng__no_of_targets[zone] == 2)
						copy_target_data(pper_tgt, idx, idx+1);

					pper_zone->rng__no_of_targets[zone]--;
				}
			}
		}
	}
out:
	if (status < VL53L5_ERROR_NONE)
		vl53l8_k_log_error("ERROR: %i\n", status);

	return status;
}

#endif

#if defined(VL53L5_PATCH_DATA_ENABLED) && \
	defined(VL53L5_DEPTH16_ON) && \
	defined(VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON)
static int32_t vl53l8_k_d16_glare_filter(
	struct vl53l8_k_glare_filter_tuning_t *p_tuning,
	struct vl53l5_tcpm_patch_0_results_dev_t *p_results)
{
	int32_t status = VL53L5_ERROR_NONE;
	union dci_union__depth16_value_u d16_data = {0};
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	uint32_t grid_size = 0;
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	uint32_t zone = 0;
	uint32_t tgt = 0;
	uint32_t idx = 0;
	uint32_t peak_rate_x16 = 0;
	uint8_t glare_detected = 0;
	uint32_t no_of_zones =
		(uint32_t)p_results->common_data.rng__no_of_zones;
	uint32_t no_of_asz_zones =
		(uint32_t)p_results->common_data.rng__acc__no_of_zones;
	uint32_t max_targets_per_zone =
		(uint32_t)p_results->common_data.rng__max_targets_per_zone;
	uint32_t total_zones = no_of_zones + no_of_asz_zones;
	uint32_t *rate_kcps = p_results->per_tgt_results.peak_rate_kcps_per_spad;
	uint16_t *depth16 = p_results->d16_per_target_data.depth16;

	if (p_tuning->remove_glare_targets && (max_targets_per_zone > 2)) {
		vl53l8_k_log_error("ERROR: only supports a maximum of 2 targets\n");
		status = VL53L8_K_GF_INVALID_MAX_TARGETS_ERROR;
		goto out;
	}
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (no_of_zones == 64)
		grid_size = 8;
	else
		grid_size = 4;
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
	for (zone = 0; zone < total_zones; zone++) {
		for (tgt = 0; tgt < max_targets_per_zone; tgt++) {
			d16_data.bytes = depth16[idx];
			if ((d16_data.confidence == VL53L8_K_D16_CONF_00PC) ||
				(d16_data.confidence == VL53L8_K_D16_CONF_14PC)) {

				continue;
			} else {
				peak_rate_x16 = rate_kcps[idx] >>
					(VL53L8_K_PEAK_RATE_FRACTIONAL_BITS - 4);

				status = vl53l8_k_glare_detect(
					d16_data.range_mm, peak_rate_x16,
					p_tuning, &glare_detected);
				if (status < VL53L5_ERROR_NONE)
					goto out;
				if (glare_detected) {
					if (!p_tuning->remove_glare_targets) {

						d16_data.confidence =
							VL53L8_K_D16_CONF_29PC;
						depth16[idx] = d16_data.bytes;
					} else {

						d16_data.confidence =
							VL53L8_K_D16_CONF_00PC;
						depth16[idx] = d16_data.bytes;
						if ((tgt == 0) && (tgt <
							(max_targets_per_zone -
							1))) {

							depth16[idx] =
							       depth16[idx + 1];
							rate_kcps[idx] =
							     rate_kcps[idx + 1];

							d16_data.confidence =
							VL53L8_K_D16_CONF_00PC;
							depth16[idx + 1] =
								d16_data.bytes;

							tgt--;
							idx--;
						}
					}
				}
			}
			idx++;
		}
	}

out:
	if (status < VL53L5_ERROR_NONE)
		vl53l8_k_log_error("ERROR: %i\n", status);

	return status;
}
#endif

int32_t vl53l8_k_glare_filter(
	struct vl53l8_k_glare_filter_tuning_t *p_tuning,
	struct vl53l5_range_data_t *p_results)
{
	int32_t status = VL53L5_ERROR_NONE;
#if !defined(VL53L5_RESULTS_DATA_ENABLED) && defined(VL53L5_PATCH_DATA_ENABLED)
#if defined(VL53L5_DEPTH16_ON) && \
	defined(VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON)
	status = vl53l8_k_d16_glare_filter(p_tuning, &p_results->tcpm_0_patch);
		if (status != VL53L5_ERROR_NONE)
			goto out;
#endif
#if defined(VL53L5_TARGET_STATUS_ON) && \
	defined(VL53L5_MEDIAN_RANGE_MM_ON) && \
	defined(VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON)
	status = vl53l8_k_median_glare_filter(p_tuning, p_results);
		if (status != VL53L5_ERROR_NONE)
			goto out;
	if (p_tuning->remove_glare_targets) {
		status = vl53l8_k_median_glare_remove(p_tuning, p_results);
			if (status != VL53L5_ERROR_NONE)
				goto out;
	}
#endif
#endif

#if defined(VL53L5_RESULTS_DATA_ENABLED) && !defined(VL53L5_PATCH_DATA_ENABLED)
#if defined(VL53L5_TARGET_STATUS_ON) && \
	defined(VL53L5_MEDIAN_RANGE_MM_ON) && \
	defined(VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON)
	status = vl53l8_k_median_glare_filter(p_tuning, p_results);
		if (status != VL53L5_ERROR_NONE)
			goto out;
	if (p_tuning->remove_glare_targets) {
		status = vl53l8_k_median_glare_remove(p_tuning, p_results);
			if (status != VL53L5_ERROR_NONE)
				goto out;
	}
#endif
#endif
out:
	if (status < VL53L5_ERROR_NONE)
		vl53l8_k_log_error("ERROR: %i\n", status);

	return status;
}

void vl53l8_k_glare_detect_init(struct vl53l8_k_glare_filter_tuning_t *p_tuning)
{
	uint32_t i = 0;

	for (i = 0; i < VL53L8_K_GLARE_LUT_SIZE; i++) {
		p_tuning->lut_range[i] = p_tuning->range_x4[i] / 4;

		p_tuning->threshold_numerator[i] = VL53L8_K_RANGE *
					(((p_tuning->refl_thresh_x256[i] / 16) *

					VL53L8_K_SIGNAL_X16 * VL53L8_K_RANGE) /
					(VL53L8_K_REFLECTANCE_X256 / 16));
		vl53l8_k_log_debug("i=%d  range=%d  numerator=%u",
					i, p_tuning->lut_range[i],
					p_tuning->threshold_numerator[i]);
	}
}
