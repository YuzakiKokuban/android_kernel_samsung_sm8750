/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_DISPLAY_INTF_SETTINGS_H__
#define __SDM_DISPLAY_INTF_SETTINGS_H__

#include <core/display_interface.h>
#include <core/sdm_types.h>

namespace sdm {

enum SDMDisplayCapability {
  SDM_CAPS_SKIP_CLIENT_COLOR_TRANSFORM,
  SDM_CAPS_DOZE,
  SDM_CAPS_BRIGHTNESS,
  SDM_CAPS_PROTECTED_CONTENTS,
};

/**
 * The purpose of this interface is to set/get any configuration options that
 * can be changed within a display's lifetime. So unlike display_caps, these
 * values are not static and may be changed, whether that is driven internally
 * or from a client call.
 */

class SDMDisplaySettingsIntf {
public:
  virtual ~SDMDisplaySettingsIntf(){};

  /**
   * Set the backlight scale on the display (if applicable)
   *
   * @param display_id: The id of the specified display
   * @param level: the level in int value to set BL level
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorParameters if the level provided is invalid for any
   * reason
   */
  virtual DisplayError SetBacklightScale(uint64_t display_id,
                                         int32_t in_level) = 0;

  /**
   * Set the blend space on the given display
   *
   * @param display_id: The id of the specified display
   * @param blend_space: ColorPrimaries containing blend space info
   *
   * @return: kErrorNone if transaction succeeded
   */
  virtual DisplayError
  SetBlendSpace(uint64_t display_id,
                const PrimariesTransfer &in_blend_space) = 0;

  /**
   * Client can call this to enable/disable vsync on display.
   * Default sate is disabled
   *
   * @param display_id: The id of the specified display
   * @param enable: enabled vsync state
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError SetVSyncState(uint64_t display_id, bool in_enabled) = 0;

  /**
   * Set autorefresh flag on given display
   *
   * @param display_id: The id of the specified display
   * @param enable: State of autorefresh
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError SetAutoRefresh(uint64_t display_id, bool in_enable) = 0;

  /**
   * Override NoisePlugIn parameters of the given device
   *
   * @param display_id: The id of the specified display
   * @param override_en: enable flag for toggling override on/off
   * @param attn: output attenuation factor
   * @param noise_zpos: z-order position of noise layer
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorParameters if any of the given params are invalid
   */
  virtual DisplayError SetNoisePlugInOverride(uint64_t display_id,
                                              bool in_override_en,
                                              int32_t in_attn,
                                              int32_t in_noise_zpos) = 0;

  /**
   * Set the color mode on given display
   *
   * @param display_id: The id of the specified display
   * @param color_mode: String form of the requested color mode
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError SetColorMode(uint64_t display_id,
                                    const std::string &in_color_mode) = 0;

  /**
   * Set the variable vsync mode on a given display (if the display supports it)
   *
   * @param display_id: The id of the specified display
   * @param vsync_mode: The mode of variable vync to be set
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display doesn't support QSync
   */
  virtual DisplayError
  SetVariableVSyncMode(uint64_t display_id,
                       const VariableVSync &in_vsync_mode) = 0;

  /**
   * Set jitter configuration on given display (if the display supports it)
   *
   * @param display_id: The id of the specified display
   * @param jitter_type: 0 - None, 1 - Instantaneous jitter, 2- Long term jitter
   * @param value: max jitter value in percentage (0-10%)
   * @param time: jitter time (for LTJ)
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display doesn't support jitter
   */
  virtual DisplayError SetJitterConfig(uint64_t display_id,
                                       int32_t in_jitter_type, float in_value,
                                       int32_t in_time) = 0;

  /**
   * Method to set min/max luminance for dynamic tonemapping of external device
   * of WFD. Only supported on virtual displays.
   *
   * @param display_id: The id of the specified display
   * @param min_lum: min luminance supported by external device
   * @param max_lum: max luminance supported by external device
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError SetPanelLuminanceAttributes(uint64_t display_id,
                                                   float in_min_lum,
                                                   float in_max_lum) = 0;

  /**
   * Set the color mode with extended info
   *
   * @param display_id: The id of the specified display
   * @param color_mode: Mode attributes which will be set
   *
   * @return kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display doesn's support stc color
   * mode
   */
  virtual DisplayError
  SetExtendedColorMode(uint64_t display_id,
                       const ColorModeInfo &in_color_mode) = 0;

  /**
   * Get the supported color modes with their info from the display
   *
   * @param display_id: The id of the specified display
   *
   * @return: List of color modes and their info
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError GetColorModes(uint64_t display_id,
                                     uint32_t *out_num_modes,
                                     int32_t *int_out_modes) = 0;

  /**
   * Method to enable/disable dimming on given display
   *
   * @param display_id: The id of the specified display
   * @param int_enabled: 0- disable, >0- enable
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: UNDEFINED - something went wrong internally
   */
  virtual DisplayError SetDimmingEnable(uint64_t display_id,
                                        int32_t in_int_enabled) = 0;

  /**
   * Set minimal backlight value for diven display's dimming feature
   *
   * @param display_id: The id of the specified display
   * @param min_bl: Min backlight value in int form
   *
   * @return: kErrorNone If transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: UNDEFINED if something went wrong internally (same as above)
   */
  virtual DisplayError SetDimmingMinBacklight(uint64_t display_id,
                                              int32_t in_min_bl) = 0;

  /**
   * Get the current brightness for a given display
   * Only supported for builtin displays
   *
   * @param display_id: The id of the specified display
   *
   * @return: Display's brightness
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the given display doesn't support
   * brightness levels
   */
  virtual DisplayError GetPanelBrightness(uint64_t display_id, float *ret) = 0;

  /**
   * Set the brightness of the specified display
   * Only supported for builtin displays
   *
   * @param display_id: The id of the specified display
   * @param brightness: brightness from level 0.0f(min) to 1.0f(max) where -1.0f
   * represents off
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorParameters if the brightness value is invalid
   * @exception: kErrorNotSupported if the display doesn't support brightness
   * levels
   * @exception: kErrorResources if power state is pending, or if hw fails to write
   * the new level
   */
  virtual DisplayError SetPanelBrightness(uint64_t display_id,
                                          float in_brightness) = 0;

  /**
   * Get the current refresh rate for the given display
   *
   * @param display_id: The id of the specified display
   *
   * @return: int value of current refresh rate
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError GetRefreshRate(uint64_t display_id, int32_t *ret) = 0;

  /**
   * Set new refresh rate on the given display.
   * Not supported on virtual displays.
   *
   * @param display_id: The id of the specified display
   * @param refresh_rate: new refresh rate of the display
   * @param final_rate: indicates whether refresh rate is final rate or can be
   * changed by sdm
   * @param idle_screen: indicates whether screen is idle. Used to be false by
   * default, now needs to be explicitly declared
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display doesn't support refresh rate
   * changes (virtual display, inactive display, panel doesn't support dynamic
   * fps, QSync active, or bit clock rate update pending)
   * @exception: kErrorParameters if the fps is below display's min
   * supported/above max supported fps.
   */
  virtual DisplayError SetRefreshRate(uint64_t display_id,
                                      int32_t in_refresh_rate,
                                      bool in_final_rate,
                                      bool in_idle_screen) = 0;

  /**
   * Set active configuration for variable properties of the display device
   * using DisplayConfigVariableInfo.
   * Currently only supported for virtual displays, since we have to re-register
   * the display with the new DisplayConfigVariableInfo
   *
   * @param display_id: The id of the specified display
   * @param variable_info: Config info to be set as active
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display is not virtual
   * @exception: NO_RESOURCES if there are currently no resources to make the
   * change
   */
  virtual DisplayError SetActiveConfig(uint64_t display_id, int32_t config) = 0;

  /**
   * Get the index of the active configuration on the given display
   * For virtual displays, this will always be 0 as they don't have multiple
   * configuration options.
   *
   * @param display_id: variable_info
   *
   * @return: index of active configuration
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError GetActiveConfig(uint64_t display_id, Config *ret) = 0;

  virtual DisplayError GetAllDisplayAttributes(
      uint64_t display_id,
      std::map<uint32_t, DisplayConfigVariableInfo> *info) = 0;

  virtual DisplayError GetDisplayAttributes(uint64_t display_id,
                                            int32_t in_index,
                                            DisplayConfigVariableInfo *ret) = 0;

  /**
   * Get the VSync event state. The default state is disabled
   *
   * @param display_id: The id of the specified display
   *
   * @return: true if VSync state is enabled
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError GetVSyncState(uint64_t display_id, bool *ret) = 0;

  virtual DisplayError GetDataspaceSaturationMatrix(int32_t dataspace,
                                                    float *out_matrix) = 0;

  virtual DisplayError
  GetDisplayVsyncPeriod(uint64_t display,
                        VsyncPeriodNanos *out_vsync_period) = 0;

  virtual DisplayError GetReadbackBufferAttributes(uint64_t display,
                                                   int32_t *format,
                                                   int32_t *dataspace) = 0;

  virtual DisplayError GetRenderIntents(uint64_t display, int32_t color_mode,
                                        uint32_t *out_num_intents,
                                        int32_t *int_out_intents) = 0;

  virtual DisplayError
  GetReadbackBufferFence(uint64_t display,
                         shared_ptr<Fence> *release_fence) = 0;

  virtual DisplayError SetColorModeWithRenderIntent(uint64_t display,
                                                    int32_t mode,
                                                    int32_t intent) = 0;

  virtual DisplayError
  SetReadbackBuffer(uint64_t display, void *buffer,
                    const shared_ptr<Fence> &acquire_fence) = 0;

  virtual DisplayError SetColorTransform(uint64_t display,
                                         const std::vector<float> &matrix) = 0;

  virtual DisplayError SetDisplayBrightness(Display display,
                                            float brightness) = 0;

  virtual DisplayError SetCursorPosition(Display display, LayerId layer,
                                         int32_t x, int32_t y) = 0;

  virtual DisplayError GetDisplayName(Display display, uint32_t *out_size,
                                      char *out_name) = 0;

  virtual DisplayError getDisplayDecorationSupport(Display display,
                                                   uint32_t *format,
                                                   uint32_t *alpha) = 0;

  virtual DisplayError SetActiveConfigWithConstraints(
      Display display, Config config,
      const SDMVsyncPeriodChangeConstraints *vsync_period_change_constraints,
      SDMVsyncPeriodChangeTimeline *out_timeline) = 0;

  virtual DisplayError
  GetClientTargetProperty(Display display,
                          SDMClientTargetProperty *outClientTargetProperty) = 0;

  virtual bool IsHDRDisplay(uint64_t display) = 0;

  virtual DisplayError ConfigureDynRefreshRate(SDMBuiltInDisplayOps ops,
                                               int refresh_rate) = 0;

  virtual DisplayError GetDisplayBrightness(uint64_t display,
                                            float *brightness) = 0;

  virtual DisplayError ControlPartialUpdate(uint64_t disp_id, bool enable) = 0;

  virtual DisplayError ToggleScreenUpdate(bool on) = 0;

  virtual DisplayError SetIdleTimeout(uint32_t value) = 0;

  virtual DisplayError SetDisplayDppsAdROI(uint64_t display_id,
                                           uint32_t h_start, uint32_t h_end,
                                           uint32_t v_start, uint32_t v_end,
                                           uint32_t factor_in,
                                           uint32_t factor_out) = 0;

  virtual void UpdateVSyncSourceOnPowerModeOff() = 0;
  virtual void UpdateVSyncSourceOnPowerModeDoze() = 0;

  virtual DisplayError GetDSIClk(uint64_t disp_id, uint64_t *bit_clk) = 0;

  virtual DisplayError SetDSIClk(uint64_t disp_id, uint64_t bit_clk) = 0;

  virtual DisplayError SetQsyncMode(uint64_t disp_id, QSyncMode mode) = 0;

  virtual DisplayError GetActiveBuiltinDisplay(uint64_t *disp_id) = 0;

  virtual int GetDisplayConfigGroup(uint64_t display, DisplayConfigGroupInfo variable_config) = 0;
  
  virtual DisplayError SetupVRRConfig(uint64_t display_id) = 0;

  virtual int GetNotifyEptConfig(uint64_t display) = 0;

  virtual DisplayError PerformCacConfig(uint64_t disp_id, CacConfig cac_config, bool enable) = 0;
};

} // namespace sdm

#endif // __SDM_DISPLAY_INTF_SETTINGS_H__
