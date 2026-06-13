/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <stdarg.h>
#include <sys/mman.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/utils.h>

#include <map>
#include <string>
#include <vector>
#include <iomanip>

#include "concurrency_mgr.h"
#include "sdm_color_mode_stc.h"

#define __CLASS__ "SDMColorModeStc"

namespace sdm {

static SDMColorMode GetColorModeFromBlendSpace(const ColorPrimaries &gamut,
                                               const GammaTransfer &transfer) {
  if (gamut == ColorPrimaries_BT709_5 && transfer == Transfer_sRGB) {
    return SDMColorMode::COLOR_MODE_SRGB;
  }
  if (gamut == ColorPrimaries_DCIP3 && transfer == Transfer_sRGB) {
    return SDMColorMode::COLOR_MODE_DISPLAY_P3;
  }
  if (gamut == ColorPrimaries_BT2020 && transfer == Transfer_SMPTE_ST2084) {
    return SDMColorMode::COLOR_MODE_BT2100_PQ;
  }
  if (gamut == ColorPrimaries_BT2020 && transfer == Transfer_HLG) {
    return SDMColorMode::COLOR_MODE_BT2100_HLG;
  }
  return SDMColorMode::COLOR_MODE_NATIVE;
}

static SDMRenderIntent
GetSDMRenderIntentFromStcIntent(const snapdragoncolor::RenderIntent &intent) {
  if (intent == snapdragoncolor::RenderIntent::kNative) {
    return SDMRenderIntent::COLORIMETRIC;
  } else if (intent < snapdragoncolor::RenderIntent::kOemCustomStart) {
    return static_cast<SDMRenderIntent>(intent - 1);
  } else {
    return static_cast<SDMRenderIntent>(intent);
  }
}

SDMColorModeStc::SDMColorModeStc(DisplayInterface *display_intf)
    : SDMColorModeMgr(display_intf) {}

DisplayError SDMColorModeStc::Init() {
  DisplayError error = display_intf_->GetStcColorModes(&stc_mode_list_);
  if (error != kErrorNone) {
    DLOGW("Failed to get Stc color modes, error %d", error);
    stc_mode_list_.list.clear();
  } else {
    DLOGI("Stc mode count %zu", stc_mode_list_.list.size());
  }

  PopulateColorModes();
  return kErrorNone;
}

DisplayError SDMColorModeStc::DeInit() {
  stc_mode_list_.list.clear();
  color_mode_map_.clear();
  return kErrorNone;
}

void SDMColorModeStc::PopulateColorModes() {
  if (!stc_mode_list_.list.size()) {
    snapdragoncolor::ColorMode color_mode = {};
    color_mode.intent = snapdragoncolor::kNative;
    color_mode_map_[SDMColorMode::COLOR_MODE_NATIVE]
                   [SDMRenderIntent::COLORIMETRIC][kSdrType] = color_mode;
    DLOGI("No color mode supported, add Native mode");
    return;
  }

  for (uint32_t i = 0; i < stc_mode_list_.list.size(); i++) {
    snapdragoncolor::ColorMode stc_mode = stc_mode_list_.list[i];
    if (stc_mode.intent == snapdragoncolor::kNative) {
      // Setting Max for native mode gamut and gamma
      stc_mode.gamut = ColorPrimaries_Max;
      stc_mode.gamma = Transfer_Max;
      color_mode_map_[SDMColorMode::COLOR_MODE_NATIVE]
                     [SDMRenderIntent::COLORIMETRIC][kSdrType] = stc_mode;
      DLOGI("Color mode NATIVE supported");
    } else {
      SDMColorMode mode =
          GetColorModeFromBlendSpace(stc_mode.gamut, stc_mode.gamma);
      SDMRenderIntent render_intent =
          GetSDMRenderIntentFromStcIntent(stc_mode.intent);
      DynamicRangeType dynamic_range = kSdrType;
      if (std::find(stc_mode.hw_assets.begin(), stc_mode.hw_assets.end(),
                    snapdragoncolor::kPbHdrBlob) != stc_mode.hw_assets.end()) {
        dynamic_range = kHdrType;
      }
      if (mode == SDMColorMode::COLOR_MODE_BT2100_PQ ||
          mode == SDMColorMode::COLOR_MODE_BT2100_HLG) {
        dynamic_range = kHdrType;
      }
      color_mode_map_[mode][render_intent][dynamic_range] = stc_mode;
      DLOGI("Add into map: mode %d, render_intent %d, dynamic_range %d", mode,
            render_intent, dynamic_range);
    }
  }
}

int32_t
SDMColorModeStc::GetStcColorModeFromMap(const SDMColorMode &mode,
                                        const SDMRenderIntent &intent,
                                        const DynamicRangeType &dynamic_range,
                                        snapdragoncolor::ColorMode *out_mode) {
  if (!out_mode) {
    DLOGE("Invalid parameters : out_mode %pK", out_mode);
    return -EINVAL;
  }

  if (color_mode_map_.find(mode) == color_mode_map_.end()) {
    DLOGE("Color mode = %d is not supported", mode);
    return -EINVAL;
  }

  if (color_mode_map_[mode].find(intent) == color_mode_map_[mode].end()) {
    DLOGE("Render intent = %d is not supported", intent);
    return -EINVAL;
  }

  auto iter = color_mode_map_[mode][intent].find(dynamic_range);
  if (iter != color_mode_map_[mode][intent].end()) {
    // Found the mode
    *out_mode = iter->second;
    return 0;
  }

  // Fall back to colormetric mode if current render intent is not support
  iter =
      color_mode_map_[mode][SDMRenderIntent::COLORIMETRIC].find(dynamic_range);
  if (iter != color_mode_map_[mode][SDMRenderIntent::COLORIMETRIC].end()) {
    *out_mode = iter->second;
    DLOGW("Fall back to colormetric mode since render intent %d is not support",
          intent);
    return 0;
  }

  if (dynamic_range == kHdrType) {
    // Fall back to SDR mode if current render intent is not support in hdr
    // modes.
    iter = color_mode_map_[mode][intent].find(kSdrType);
    if (iter != color_mode_map_[mode][intent].end()) {
      *out_mode = iter->second;
      DLOGW(
          "Fall back to sdr mode since render intent %d is not support in hdr",
          intent);
      return 0;
    }

    // Fall back to SDR colormetric mode if no hdr mode support.
    iter = color_mode_map_[mode][SDMRenderIntent::COLORIMETRIC].find(kSdrType);
    if (iter != color_mode_map_[mode][SDMRenderIntent::COLORIMETRIC].end()) {
      *out_mode = iter->second;
      DLOGW("Fall back to sdr colormetric mode if no hdr mode support");
      return 0;
    }
  }
  DLOGW("Can't find color mode %d intent %d range %d", mode, intent,
        dynamic_range);

  return -EINVAL;
}

uint32_t SDMColorModeStc::GetColorModeCount() {
  uint32_t count = UINT32(color_mode_map_.size());
  DLOGI("Supported color mode count = %d", count);
  return std::max(1U, count);
}

DisplayError SDMColorModeStc::GetColorModes(uint32_t *out_num_modes,
                                            SDMColorMode *out_modes) {
  if (!out_num_modes || !out_modes) {
    DLOGE("Invalid parameters : out_num_modes %pK out_mode %pK", out_num_modes,
          out_modes);
    return kErrorParameters;
  }
  auto it = color_mode_map_.begin();
  *out_num_modes = std::min(*out_num_modes, UINT32(color_mode_map_.size()));
  for (uint32_t i = 0; i < *out_num_modes; it++, i++) {
    out_modes[i] = it->first;
    DLOGI("Color mode = %d is supported", out_modes[i]);
  }
  return kErrorNone;
}

uint32_t SDMColorModeStc::GetRenderIntentCount(SDMColorMode mode) {
  uint32_t count = UINT32(color_mode_map_[mode].size());
  DLOGI("mode: %d supported rendering intent count = %d", mode, count);
  return std::max(1U, count);
}

DisplayError SDMColorModeStc::GetRenderIntents(SDMColorMode mode,
                                               uint32_t *out_num_intents,
                                               SDMRenderIntent *out_intents) {
  if (!out_num_intents || !out_intents) {
    DLOGE("Invalid parameters : out_num_intents %pK out_intents %pK",
          out_num_intents, out_intents);
    return kErrorParameters;
  }
  if (color_mode_map_.find(mode) == color_mode_map_.end()) {
    DLOGE("Color mode = %d is not supported", mode);
    return kErrorParameters;
  }
  auto it = color_mode_map_[mode].begin();
  *out_num_intents =
      std::min(*out_num_intents, UINT32(color_mode_map_[mode].size()));
  for (uint32_t i = 0; i < *out_num_intents; it++, i++) {
    out_intents[i] = it->first;
    DLOGI("Color mode = %d is supported with render intent = %d", mode,
          out_intents[i]);
  }
  return kErrorNone;
}

DisplayError SDMColorModeStc::SetColorTransform(const float *matrix, SDMColorTransform hint) {
  if (!matrix) {
    DLOGE("Invalid parameters : matrix %pK", matrix);
    return kErrorParameters;
  }
  auto status = kErrorNone;
  double color_matrix[kColorTransformMatrixCount] = {0};
  CopyColorTransformMatrix(matrix, color_matrix);

  DisplayError error = display_intf_->SetColorTransform(
      kColorTransformMatrixCount, color_matrix);
  if (error != kErrorNone) {
    DLOGE("Failed to set Color Transform Matrix");
    status = kErrorNotSupported;
  }
  CopyColorTransformMatrix(matrix, color_matrix_);
  return status;
}

DisplayError
SDMColorModeStc::CacheColorModeWithRenderIntent(SDMColorMode mode,
                                                SDMRenderIntent intent) {
  if (current_color_mode_ == mode && current_render_intent_ == intent) {
    return kErrorNone;
  }

  current_color_mode_ = mode;
  current_render_intent_ = intent;
  apply_mode_ = true;
  return kErrorNone;
}

DisplayError
SDMColorModeStc::ApplyCurrentColorModeWithRenderIntent(bool hdr_present) {
  DisplayError error = kErrorNone;

  if (color_mode_map_.empty()) {
    return kErrorNone;
  }

  if (!apply_mode_) {
    if ((hdr_present && curr_dynamic_range_ == kHdrType) ||
        (!hdr_present && curr_dynamic_range_ == kSdrType))
      return kErrorNone;
  }

  curr_dynamic_range_ = (hdr_present) ? kHdrType : kSdrType;
  int32_t ret = 0;
  snapdragoncolor::ColorMode mode;
  ret = GetStcColorModeFromMap(current_color_mode_, current_render_intent_,
                               curr_dynamic_range_, &mode);
  if (ret) {
    DLOGW("Cannot find mode for current_color_mode_ %d, current_render_intent_ "
          "%d, hdr_present %d",
          current_color_mode_, current_render_intent_, hdr_present);
    return kErrorNone;
  }

  DLOGI("Applying Stc mode (gamut %d gamma %d intent %d), curr mode %d, render "
        "intent %d, hdr "
        "present %d",
        mode.gamut, mode.gamma, mode.intent, current_color_mode_,
        current_render_intent_, hdr_present);
  error = display_intf_->SetStcColorMode(mode);
  if (error != kErrorNone) {
    DLOGE("Failed to apply Stc color mode: gamma %d gamut %d intent %d err %d",
          mode.gamma, mode.gamut, mode.intent, error);
    return kErrorNotSupported;
  }

  apply_mode_ = false;

  DLOGV_IF(kTagQDCM, "Successfully applied mode = %d, intent = %d, range = %d",
           current_color_mode_, current_render_intent_, curr_dynamic_range_);
  return kErrorNone;
}

DisplayError
SDMColorModeStc::NotifyDisplayCalibrationMode(bool in_calibration) {
  DisplayError error = kErrorNone;
  error = display_intf_->NotifyDisplayCalibrationMode(in_calibration);
  if (error != kErrorNone) {
    return kErrorNotSupported;
  }
  return kErrorNone;
}

void SDMColorModeStc::Dump(std::ostringstream *os) {
  *os << "color modes supported: \n";
  for (auto it : color_mode_map_) {
    *os << "mode: " << static_cast<int32_t>(it.first) << " RIs { ";
    for (auto render_intent_it : it.second) {
      *os << static_cast<int32_t>(render_intent_it.first) << " ";
    }
    *os << "} \n";
  }
  *os << "current mode: " << static_cast<uint32_t>(current_color_mode_)
      << std::endl;
  *os << "current render_intent: "
      << static_cast<uint32_t>(current_render_intent_) << std::endl;
  if (curr_dynamic_range_ == kHdrType) {
    *os << "current dynamic_range: HDR" << std::endl;
  } else {
    *os << "current dynamic_range: SDR" << std::endl;
  }
  *os << "current transform: ";
  for (uint32_t i = 0; i < kColorTransformMatrixCount; i++) {
    if (i % 4 == 0) {
      *os << std::endl;
    }
    *os << std::fixed << std::setprecision(2) << std::setw(6)
        << std::setfill(' ') << color_matrix_[i] << " ";
  }
  *os << std::endl;
}

} // namespace sdm
