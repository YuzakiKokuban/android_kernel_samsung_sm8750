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
#ifndef __SDM_DISPLAY_INTF_CAPS_H__
#define __SDM_DISPLAY_INTF_CAPS_H__

#include <core/core_interface.h>
#include <core/display_interface.h>
#include <core/sdm_types.h>

namespace sdm {

/**
 * This is a discovery interface for a system backend to determine the types
 * and capabilities of the possible displays that could be connected.
 *
 * Not as much documentation here is necessary for life cycle, effects, etc.
 * since this is a read-only interface.
 *
 * Exceptions will be returned as the service specific error code
 * in a ScopedAStatus object, so in case of a method failure, check that
 * code to see what went wrong. The code corresponds to the SdmClientError enum
 */

class SDMDisplayCapsIntf {
public:
  virtual ~SDMDisplayCapsIntf() {}

  virtual bool GetComposerStatus() = 0;

  /**
   * Get a list of currently connected displays, along with their display type,
   * display id, and other display + panel info
   */
  virtual DisplayError GetDisplaysStatus(HWDisplaysInfo *ret) = 0;

  /**
   * Query the capacities for a certain display type in the system
   *
   * @return: Number of diplays of the given type that are supported
   */
  virtual DisplayError GetMaxDisplaysSupported(SDMDisplayType in_type,
                                               int32_t *ret) = 0;

  virtual DisplayError GetDisplayType(uint64_t display_id, int32_t *ret) = 0;

  /**
   * Get identification into for a given display. This info is static and will
   * never change so clients can call this once and save the information.
   *
   * @param display_id: The id of the specified display
   *
   * @return: DisplayIdentificationParcel
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorDriverData if driver fails to provide the data on request
   */
  virtual DisplayError GetDisplayIdentificationData(uint64_t display_id,
                                                    uint8_t *port,
                                                    uint32_t *size,
                                                    uint8_t *data) = 0;

  /**
   * Get the maximum backlight level for the specified display, if it is
   * applicable
   *
   * @param display_id: The id of the specified display
   *
   * @return: int value of backlight max level
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError GetPanelBlMaxLvl(uint64_t display_id, int32_t *ret) = 0;

  /**
   * Get the max brightness level for a specified display
   *
   * @param display_id: The id of the specified display
   *
   * @return: int value of max brightness level
   *
   * @exception kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display doesn't have a panel with
   * brightness support
   */
  virtual DisplayError GetPanelMaxBrightness(uint64_t display_id,
                                             int32_t *ret) = 0;

  /**
   * Get the inclusive range for a display's possible refresh rates
   * The min and max refresh rates will be the same if the display doesn't
   * contain custom hardware panel info, typically for secondary displays and
   * command mode panels
   *
   * @param display_id: The id of the specified display
   *
   * @return: List of min and max refresh rates. min[i] and max[i] are a pair
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError GetRefreshRateRange(uint64_t display_id, uint32_t *min,
                                           uint32_t *max) = 0;

  /**
   * Check if underscan is supported for the given display
   * Only applicable to external/pluggable displays
   *
   * @param display_id: The id of the specified display
   *
   * @return boolean: true if underscan is supported, else false
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError IsUnderScanSupported(uint64_t display_id, bool *ret) = 0;

  /**
   * Get the attributes of a specified color mode on a given display
   *
   * @param display_id: The id of the specified display
   * @param color_mode: The color mode in string form to get attributes of
   *
   * @return: list of attribute key/values for the color mode, both in string
   * form. AIDL doesn't support maps in ndk backend yet, so it will be a list
   * instead and any client calling this function will have to search the list
   * manually
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display's color manager failed to
   * load or if the requested color mode doesn't have an attribute
   */
  virtual DisplayError
  GetColorModeAttr(uint64_t display_id, const std::string &in_color_mode,
                   std::vector<ColorModeAttributeVal> *ret) = 0;

  /**
   * Get the default color mode for the given display, in string mode
   *
   * @param display_id: The id of the specified display
   *
   * @return: string value of the display's default color mode
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display's color manager failed to
   * load
   */
  virtual DisplayError GetDefaultColorMode(uint64_t display_id,
                                           std::string *ret) = 0;

  // TODO(JJ): can encapsulate this in identification info to minimize traffic
  virtual DisplayError GetDisplayPort(uint64_t display_id,
                                      DisplayPort *ret) = 0;

  /**
   * Check if the display supports low power mode
   * Generally, only builtin displays will support this mode
   *
   * @param display_id: The id of the specified display
   *
   * @return: true if the display supports low power modes
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError GetDozeSupport(uint64_t display_id, int32_t *ret) = 0;

  /**
   * Get the supported DSI clock rates for a given display
   *
   * @param display_id: The id of the specified display
   *
   * @return: list of supported clock rates in long form
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display is not builtin/doesn't use
   * DSI
   */
  virtual DisplayError GetSupportedDSIClock(uint64_t display_id,
                                            std::vector<int64_t> *ret) = 0;

  /**
   * Get a list of the supported stc color modes for the given display
   * Generally only supported for builtin displays
   *
   * @param display_id: The id of the specified display
   *
   * @return: List of supported stc color modes
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display doesn't support stc color
   * modes
   */
  virtual DisplayError GetStcColorModes(uint64_t display_id) = 0;

  /**
   * Check if the given feature is supported on the given display
   *
   * @param display_id: The id of the given display
   * @param feature: The requested feature
   *
   * @return: true if requested feature is supported on given display
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError IsSupportedOnDisplay(uint64_t display_id,
                                            SupportedDisplayFeature in_feature,
                                            bool *ret) = 0;

  /**
   * Check the hdr capabilities on a given display
   *
   * @param display_id: The id of the specified display
   *
   * @return: parcel containing info about the display's hdr capabilities
   *          each pair at slot [i] is a tuple
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError GetHdrCapabilities(uint64_t display_id,
                                          uint32_t *out_num_types,
                                          int32_t *out_types,
                                          float *out_max_luminance,
                                          float *out_max_average_luminance,
                                          float *out_min_luminance) = 0;

  virtual DisplayError
  GetDisplayConnectionType(uint64_t display_id,
                           DisplayClass *display_class) = 0;

  virtual uint32_t GetMaxVirtualDisplayCount() = 0;

  virtual DisplayError GetFixedConfig(uint64_t display_id,
                                      DisplayConfigFixedInfo *info) = 0;

  virtual DisplayError GetDisplayConfigs(uint64_t display_id,
                                         std::vector<int32_t> *out_configs) = 0;

  virtual void GetCapabilities(uint32_t *outCount,
                               int32_t *outCapabilities) = 0;

  virtual DisplayError IsWbUbwcSupported(bool *value) = 0;

  virtual DisplayError IsSmartPanelConfig(uint64_t display_id,
                                          uint32_t config_id,
                                          bool *is_smart) = 0;

  virtual bool IsRotatorSupportedFormat(LayerBufferFormat format) = 0;

  virtual DisplayError GetDisplayHwId(uint64_t disp_id,
                                      int32_t *disp_hw_id) = 0;

  virtual DisplayError GetSupportedDisplayRefreshRates(
      int disp_id, std::vector<uint32_t> *supported_refresh_rates) = 0;

  virtual bool IsModeSwitchAllowed(uint64_t disp_id, int32_t config) = 0;

  virtual DisplayError GetDisplayPortId(uint32_t disp_id, int *port_id) = 0;

  virtual DisplayError IsCacV2Supported(uint32_t disp_id, bool *supported) = 0;
};

} // namespace sdm

#endif // __SDM_DISPLAY_INTF_CAPS_H__
