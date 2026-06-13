/*
* Copyright (c) 2014 - 2018, 2020 The Linux Foundation. All rights reserved.
*/

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>
#include <errno.h>
#include <debug_handler.h>
#include <core/display_interface.h>
#include <display_properties.h>

namespace sdm {

using display::DebugHandler;

class Debug {
 public:
  static inline DebugHandler* Get() { return DebugHandler::Get(); }
  static int GetSimulationFlag();
  static bool GetExternalResolution(char *val);
  static void GetIdleTimeoutMs(uint32_t *active_ms, uint32_t *inactive_ms);
  static bool IsRotatorDownScaleDisabled();
  static bool IsRotatorEnabledForUi();
  static bool IsDecimationDisabled();
  static int GetMaxPipesPerMixer(SDMDisplayType display_type);
  static int GetMaxUpscale();
  static bool IsVideoModeEnabled();
  static bool IsRotatorUbwcDisabled();
  static bool IsRotatorSplitDisabled();
  static bool IsScalarDisabled();
  static bool IsUbwcTiledFrameBuffer();
  static bool IsAVRDisabled();
  static bool IsExtAnimDisabled();
  static bool IsPartialSplitDisabled();
  static bool IsSrcSplitPreferred();
  static bool GetPropertyDisableInlineMode();
  static bool GetPropertyDisableOfflineMode();
  static int GetWindowRect(bool primary, float *left, float *top, float *right, float *bottom);
  static int GetMixerResolution(uint32_t *width, uint32_t *height);
  static int GetNullDisplayResolution(uint32_t *width, uint32_t *height);
  static int GetReducedConfig(uint32_t *num_vig_pipes, uint32_t *num_dma_pipes);
  static int GetSecondaryMaxFetchLayers();
  static bool IsIWEEnabled();
  static int GetProperty(const char *property_name, char *value);
  static int GetProperty(const char *property_name, int *value);
  static void DumpCodeCoverage();
};

}  // namespace sdm

#endif  // __DEBUG_H__

