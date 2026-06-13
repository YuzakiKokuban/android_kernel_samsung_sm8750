/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
#ifndef __SDM_SERVICES_H__
#define __SDM_SERVICES_H__

#include <utils/locker.h>

#include <vector>
#include <core/buffer_allocator.h>
#include <core/socket_handler.h>

#include "sdm_color_manager.h"
#include "sdm_display.h"
#include "sdm_display_builder.h"
#include "sdm_display_builtin.h"
#include "sdm_display_intf_parcel.h"
#include "sdm_services_cb_intf.h"
#include "sdm_tui.h"

namespace sdm {

enum {
  SDM_SERVICE_COMMAND_LIST_START = 1,
  SDM_SERVICE_GET_PANEL_BRIGHTNESS = 2,           // Provides ability to get the panel brightness
  SDM_SERVICE_SET_PANEL_BRIGHTNESS = 3,           // Provides ability to set the panel brightness
  SDM_SERVICE_CONNECT_SDM_CLIENT = 4,             // Connect to qservice
  SDM_SERVICE_SCREEN_REFRESH = 5,                 // Refresh screen through SF invalidate
  SDM_SERVICE_GET_DISPLAY_VISIBLE_REGION = 11,    // Get the visibleRegion for dpy
  SDM_SERVICE_SET_SECONDARY_DISPLAY_STATUS = 12,  // Sets secondary display status
  SDM_SERVICE_SET_MAX_PIPES_PER_MIXER = 13,       // Set max pipes per mixer for MDPComp
  SDM_SERVICE_DYNAMIC_DEBUG = 15,                 // Enable more logging on the fly
  SDM_SERVICE_SET_IDLE_TIMEOUT = 16,              // Set idle timeout for GPU fallback
  SDM_SERVICE_CONFIGURE_DYN_REFRESH_RATE = 18,    //  Enable/Disable/Set refresh rate dynamically
  SDM_SERVICE_CONTROL_PARTIAL_UPDATE = 19,  // Provides ability to enable/disable partial update
  SDM_SERVICE_TOGGLE_SCREEN_UPDATES = 20,   // Provides ability to set the panel brightness
  SDM_SERVICE_SET_FRAME_DUMP_CONFIG = 21,   // Provides ability to set the frame dump config
  SDM_SERVICE_CONNECT_HDMI_CLIENT = 23,     // Connect HDMI CEC HAL Client
  SDM_SERVICE_QDCM_SVC_CMDS = 24,           // request QDCM services.
  SDM_SERVICE_SET_ACTIVE_CONFIG = 25,       // Set a specified display config
  SDM_SERVICE_GET_ACTIVE_CONFIG = 26,       // Get the current config index
  SDM_SERVICE_GET_CONFIG_COUNT = 27,        // Get the number of supported display configs
  SDM_SERVICE_GET_DISPLAY_ATTRIBUTES_FOR_CONFIG = 28,  // Get attr for specified config
  SDM_SERVICE_SET_DISPLAY_MODE = 29,                   // Set display mode to command or video mode
  SDM_SERVICE_SET_CAMERA_STATUS = 30,  // To notify display when camera is on and off
  SDM_SERVICE_MIN_HDCP_ENCRYPTION_LEVEL_CHANGED = 31,
  SDM_SERVICE_GET_BW_TRANSACTION_STATUS = 32,   // Client can query BW transaction status.
  SDM_SERVICE_SET_LAYER_MIXER_RESOLUTION = 33,  // Enables client to set layer mixer resolution.
  SDM_SERVICE_SET_COLOR_MODE = 34,              // Overrides the QDCM mode on the display
  SDM_SERVICE_SET_COLOR_MODE_BY_ID = 36,        // Overrides the QDCM mode using the given mode ID
  SDM_SERVICE_GET_COMPOSER_STATUS =
      37,                           // Get composer init status-true if primary display init is done
  SDM_SERVICE_SET_QSYNC_MODE = 38,  // Set qsync mode. 0 - (none)disable qsync, 1 - continuous mode.
  SDM_SERVICE_SET_COLOR_MODE_WITH_RENDER_INTENT = 39,  // Overrides the QDCM mode with render intent
  SDM_SERVICE_SET_IDLE_PC = 40,                        // Enable/disable Idle power collapse
  SDM_SERVICE_SET_DPPS_AD4_ROI_CONFIG = 41,            // Set ad4 roi config for debug
  SDM_SERVICE_SET_DSI_CLK = 42,                        // Set DSI Clk.
  SDM_SERVICE_GET_DSI_CLK = 43,                        // Get DSI Clk.
  SDM_SERVICE_GET_SUPPORTED_DSI_CLK = 44,              // Get supported DSI Clk.
  SDM_SERVICE_SET_COLOR_MODE_FROM_CLIENT = 45,  // Overrides the QDCM mode using the given mode ID
  SDM_SERVICE_SET_FRAME_TRIGGER_MODE = 46,      // Set frame trigger mode for debug
  SDM_SERVICE_SET_PANEL_LUMINANCE = 47,         // Set Panel Luminance attributes.
  SDM_SERVICE_SET_BRIGHTNESS_SCALE = 48,        // Set brightness scale ratio
  SDM_SERVICE_SET_COLOR_SAMPLING_ENABLED = 49,  // Toggle the collection of display color stats
  SDM_SERVICE_SET_VSYNC_STATE = 50,             // Enable/disable Vsync
  SDM_SERVICE_NOTIFY_TUI_TRANSITION = 51,       // Notify TUI transition prepare/start/stop
  SDM_SERVICE_GET_DISPLAY_PORT_ID = 52,         // Get the port id for a given display
  SDM_SERVICE_SET_NOISE_PLUGIN_OVERRIDE = 53,   // Override NoisePlugIn parameters
  SDM_SERVICE_SET_DIMMING_ENABLE = 54,          // Set display dimming enablement
  SDM_SERVICE_SET_DIMMING_MIN_BL = 55,          // Set display dimming minimal backlight value
  SDM_SERVICE_DUMP_CODE_COVERAGE = 56,        // Dump the code coverage data for userspace projects
  SDM_SERVICE_UPDATE_TRANSFER_TIME = 57,      // Update transfer time
  SDM_SERVICE_SET_JITTER_CONFIG = 58,         // Watchdog TE Jitter Configuration
  SDM_SERVICE_RETRIEVE_DEMURATN_FILES = 59,   // Retrieve DemuraTn files from TVM
  SDM_SERVICE_SET_DEMURA_STATE = 60,          // Enable/disable demura feature
  SDM_SERVICE_SET_DEMURA_CONFIG = 61,         // Set the demura configuration index
  SDM_SERVICE_SET_BPP_MODE = 62,              // Set Panel bpp to 24bpp or 30bpp
  SDM_SERVICE_PERFORM_CAC_CONFIG = 63,        // Set CAC Configuration for the display
  SDM_SERVICE_SET_PANEL_FEATURE_CONFIG = 64,  // Common function, Set cfg for panel features
  SDM_SERVICE_COMMAND_LIST_END = 400,
};

enum {
  SDM_SERVICE_END = 0,
  SDM_SERVICE_START,
};

enum {
  SDM_SERVICE_DEBUG_ALL,
  SDM_SERVICE_DEBUG_MDPCOMP,
  SDM_SERVICE_DEBUG_VSYNC,
  SDM_SERVICE_DEBUG_VD,
  SDM_SERVICE_DEBUG_PIPE_LIFECYCLE,
  SDM_SERVICE_DEBUG_DRIVER_CONFIG,
  SDM_SERVICE_DEBUG_ROTATOR,
  SDM_SERVICE_DEBUG_QDCM,
  SDM_SERVICE_DEBUG_SCALAR,
  SDM_SERVICE_DEBUG_CLIENT,
  SDM_SERVICE_DEBUG_DISPLAY,
  SDM_SERVICE_DEBUG_IWE,
  SDM_SERVICE_DEBUG_WB_USAGE,
  SDM_SERVICE_DEBUG_MAX_VAL =
      SDM_SERVICE_DEBUG_WB_USAGE, // Used to check each bit of the debug command
                                  // paramater. Update DEBUG_MAX_VAL when adding
                                  // new debug tag.
};

enum {
  SDM_SERVICE_PREF_POST_PROCESSING,
  SDM_SERVICE_PREF_PARTIAL_UPDATE,
  SDM_SERVICE_ENABLE_PARTIAL_UPDATE,
};

enum {
  SDM_SERVICE_QSYNC_MODE_NONE,
  SDM_SERVICE_QSYNC_MODE_CONTINUOUS,
  SDM_SERVICE_QSYNC_MODE_ONESHOT, // Not supported
};

enum {
  SDM_SERVICE_TUI_TRANSITION_PREPARE,
  SDM_SERVICE_TUI_TRANSITION_START,
  SDM_SERVICE_TUI_TRANSITION_END,
};

class SDMServices {
public:
  enum CwbConfigFlag {
    kCwbFlagPuAsCwbROI,
    kCwbFlagAvoidRefresh,
  };

  explicit SDMServices(SDMServicesCbIntf *cb, BufferAllocator *buffer_allocator,
                       SocketHandler *socket_handler)
      : cb_(cb), buffer_allocator_(buffer_allocator), socket_handler_(socket_handler) {}

  void Init(SDMDisplayBuilder *disp, BufferAllocator *buffer_allocator,
            Locker *locker, SDMTrustedUI *tui);
  void Deinit();

  DisplayError notifyCallback(uint32_t command, SDMParcel *input_parcel,
                              SDMParcel *output_parcel);
  DisplayError GetConfigCount(int disp_id, uint32_t *count);
  DisplayError GetActiveConfigIndex(int disp_id, uint32_t *config);
  DisplayError SetNoisePlugInOverride(int32_t disp_id, bool override_en,
                                      int32_t attn, int32_t noise_zpos);
  DisplayError MinHdcpEncryptionLevelChanged(int disp_id,
                                             uint32_t min_enc_level);
  DisplayError ControlPartialUpdate(int disp_id, bool enable);
  DisplayError ToggleScreenUpdate(int disp_id, bool on);
  DisplayError ControlIdlePowerCollapse(int disp_id, bool enable, bool sync);
  DisplayError GetDisplayBrightnessPercent(uint32_t display, float *brightness);
  DisplayError SetDisplayDppsAdROI(uint32_t display_id, uint32_t h_start,
                                   uint32_t h_end, uint32_t v_start,
                                   uint32_t v_end, uint32_t factor_in,
                                   uint32_t factor_out);
  DisplayError SetActiveConfigIndex(int disp_id, uint32_t config);
  DisplayError SetIdleTimeout(int value);
  DisplayError SetCameraLaunchStatus(int camera_status);
  DisplayError DisplayBWTransactionPending(bool *state);
  DisplayError GetDisplayMaxBrightness(uint32_t display,
                                       uint32_t *max_brightness_level);

private:
  DisplayError DynamicDebug(int type, bool enable, int verbose_level);
  DisplayError RefreshScreen(int idx);
  DisplayError ValidateFrameDumpConfig(uint32_t frame_dump_count,
                                       uint32_t bit_mask_disp_type,
                                       uint32_t bit_mask_layer_type,
                                       int32_t *processable_cwb_requests);

  DisplayError SetFrameDumpConfig(uint32_t frame_dump_count,
                                  std::bitset<32> bit_mask_display_type,
                                  uint32_t bit_mask_layer_type,
                                  int32_t processable_cwb_requests,
                                  int32_t output_format, CwbConfig cwb_config);
  DisplayError SetMaxMixerStages(std::bitset<32> bit_mask_display_type,
                                 int max_mixer_stages);
  DisplayError SetDisplayMode(int mode);
  DisplayError SetDisplayStatus(int disp_id, SDMDisplayStatus disp_status);
  DisplayError ConfigureRefreshRate(uint32_t operation, uint32_t refresh_rate);
  DisplayError
  GetDisplayAttributesForConfig(int disp_id, int config,
                                DisplayConfigVariableInfo *var_info);
  DisplayError GetVisibleDisplayRect(int disp_id, SDMRect *visible_rect);
  DisplayError SetMixerResolution(int dpy, uint32_t width, uint32_t height);
  DisplayError SetColorModeOverride(int dpy, SDMColorMode mode);
  DisplayError SetColorModeWithRenderIntentOverride(int disp_idx,
                                                    SDMColorMode mode,
                                                    int intent);
  DisplayError SetColorModeById(int dpy, int mode);
  DisplayError SetQSyncMode(QSyncMode qsync_mode);
  DisplayError SetColorSamplingEnabled(int dpy, int enabled_cmd);
  DisplayError SetDsiClk(int disp_id, uint64_t clk);
  DisplayError SetJitterConfig(uint32_t jitter_type, float jitter_val,
                               uint32_t jitter_time);
  DisplayError GetDsiClk(int disp_id, uint64_t *clk);
  DisplayError GetSupportedDsiClk(int disp_id, std::vector<uint64_t> *bitrates);
  DisplayError SetPanelLuminanceAttributes(int disp_id, float min_lum,
                                           float max_lum);
  DisplayError SetColorModeFromClient(int disp_id, int mode);
  DisplayError SetFrameTriggerMode(int disp_id, int mode);
  DisplayError SetDisplayBrightnessScale(int disp_id, int level, int dre_case);
  DisplayError SetBppMode(uint32_t bpp);
  DisplayError UpdateTransferTime(uint32_t transfer_time);
  DisplayError RetrieveDemuraTnFiles(int disp_id);
  DisplayError QdcmCMDDispatch(uint32_t display_id,
                               const PPDisplayAPIPayload &req_payload,
                               PPDisplayAPIPayload *resp_payload,
                               PPPendingParams *pending_action);

  DisplayError DynamicDebug(SDMParcel *input_parcel);
  DisplayError SetIdleTimeout(SDMParcel *input_parcel);
  DisplayError SetFrameDumpConfig(SDMParcel *input_parcel);
  DisplayError SetMaxMixerStages(SDMParcel *input_parcel);
  DisplayError SetDisplayMode(SDMParcel *input_parcel);
  DisplayError ConfigureRefreshRate(SDMParcel *input_parcel);
  DisplayError SetNoisePlugInOverride(SDMParcel *input_parcel);
  DisplayError SetActiveConfigIndex(SDMParcel *input_parcel);
  DisplayError SetCameraLaunchStatus(SDMParcel *input_parcel);
  DisplayError QdcmCMDHandler(SDMParcel *input_parcel,
                              SDMParcel *output_parcel);
  DisplayError GetDisplayAttributesForConfig(SDMParcel *input_parcel,
                                             SDMParcel *output_parcel);
  DisplayError GetVisibleDisplayRect(SDMParcel *input_parcel,
                                     SDMParcel *output_parcel);
  DisplayError SetMixerResolution(SDMParcel *input_parcel);
  DisplayError SetColorModeOverride(SDMParcel *input_parcel);
  DisplayError SetColorModeWithRenderIntentOverride(SDMParcel *input_parcel);

  DisplayError SetColorModeById(SDMParcel *input_parcel);
  DisplayError SetColorModeFromClient(SDMParcel *input_parcel);
  DisplayError SetBppMode(SDMParcel *input_parcel);
  DisplayError SetQSyncMode(SDMParcel *input_parcel);
  DisplayError SetIdlePC(SDMParcel *input_parcel);
  DisplayError RefreshScreen(SDMParcel *input_parcel);
  DisplayError SetAd4RoiConfig(SDMParcel *input_parcel);
  DisplayError SetJitterConfig(SDMParcel *input_parcel);
  DisplayError SetDsiClk(SDMParcel *input_parcel);
  DisplayError GetDsiClk(SDMParcel *input_parcel, SDMParcel *output_parcel);
  DisplayError GetSupportedDsiClk(SDMParcel *input_parcel,
                                  SDMParcel *output_parcel);
  DisplayError SetFrameTriggerMode(SDMParcel *input_parcel);
  DisplayError SetPanelLuminanceAttributes(SDMParcel *input_parcel);
  DisplayError setColorSamplingEnabled(SDMParcel *input_parcel);
  DisplayError UpdateTransferTime(SDMParcel *input_parcel);
  DisplayError ProcessDisplayBrightnessScale(SDMParcel *input_parcel);
  DisplayError PerformCacConfig(SDMParcel *input_parcel);
  DisplayError SetDisplayStatus(SDMParcel *input_parcel,
                                SDMParcel *output_parcel);
  DisplayError ToggleScreenUpdate(SDMParcel *input_parcel,
                                  SDMParcel *output_parcel);
  DisplayError MinHdcpEncryptionLevelChanged(SDMParcel *input_parcel,
                                             SDMParcel *output_parcel);
  DisplayError ControlPartialUpdate(SDMParcel *input_parcel,
                                    SDMParcel *output_parcel);
  DisplayError GetActiveConfigIndex(SDMParcel *input_parcel,
                                    SDMParcel *output_parcel);
  DisplayError GetConfigCount(SDMParcel *input_parcel,
                              SDMParcel *output_parcel);
  DisplayError GetDisplayBrightness(SDMParcel *input_parcel,
                                    SDMParcel *output_parcel);
  DisplayError SetDisplayBrightness(SDMParcel *input_parcel,
                                    SDMParcel *output_parcel);
  DisplayError GetComposerStatus(SDMParcel *input_parcel,
                                 SDMParcel *output_parcel);
  DisplayError SetVSyncState(SDMParcel *input_parcel, SDMParcel *output_parcel);
  DisplayError HandleTUITransition(SDMParcel *input_parcel,
                                   SDMParcel *output_parcel);
  DisplayError SetDimmingEnable(SDMParcel *input_parcel,
                                SDMParcel *output_parcel);
  DisplayError SetDimmingMinBl(SDMParcel *input_parcel,
                               SDMParcel *output_parcel);
  DisplayError RetrieveDemuraTnFiles(SDMParcel *input_parcel,
                                     SDMParcel *output_parcel);
  DisplayError SetDemuraState(SDMParcel *input_parcel,
                              SDMParcel *output_parcel);
  DisplayError SetDemuraConfig(SDMParcel *input_parcel,
                               SDMParcel *output_parcel);
  DisplayError DisplayBWTransactionPending(SDMParcel *input_parcel,
                                           SDMParcel *output_parcel);
  DisplayError GetDisplayPortId(SDMParcel *input_parcel, SDMParcel *output_parcel);
  DisplayError SetPanelFeatureConfig(SDMParcel *input_parcel, SDMParcel *output_parcel);

  typedef DisplayError (SDMServices::*VndCmdSetHandler)(
      SDMParcel *input_parcel);
  typedef DisplayError (SDMServices::*VndCmdGetHandler)(
      SDMParcel *input_parcel, SDMParcel *output_parcel);

  std::unordered_map<uint32_t, VndCmdSetHandler> vnd_handlers_set_ = {
      {SDM_SERVICE_DYNAMIC_DEBUG, &SDMServices::DynamicDebug},
      {SDM_SERVICE_SCREEN_REFRESH, &SDMServices::RefreshScreen},
      {SDM_SERVICE_SET_IDLE_TIMEOUT, &SDMServices::SetIdleTimeout},
      {SDM_SERVICE_SET_FRAME_DUMP_CONFIG, &SDMServices::SetFrameDumpConfig},
      {SDM_SERVICE_SET_MAX_PIPES_PER_MIXER, &SDMServices::SetMaxMixerStages},
      {SDM_SERVICE_SET_DISPLAY_MODE, &SDMServices::SetDisplayMode},
      {SDM_SERVICE_CONFIGURE_DYN_REFRESH_RATE, &SDMServices::ConfigureRefreshRate},
      {SDM_SERVICE_SET_NOISE_PLUGIN_OVERRIDE, &SDMServices::SetNoisePlugInOverride},
      {SDM_SERVICE_SET_ACTIVE_CONFIG, &SDMServices::SetActiveConfigIndex},
      {SDM_SERVICE_SET_CAMERA_STATUS, &SDMServices::SetCameraLaunchStatus},
      {SDM_SERVICE_SET_LAYER_MIXER_RESOLUTION, &SDMServices::SetMixerResolution},
      {SDM_SERVICE_SET_COLOR_MODE, &SDMServices::SetColorModeOverride},
      {SDM_SERVICE_SET_COLOR_MODE_WITH_RENDER_INTENT,
       &SDMServices::SetColorModeWithRenderIntentOverride},
      {SDM_SERVICE_SET_COLOR_MODE_BY_ID, &SDMServices::SetColorModeById},
      {SDM_SERVICE_SET_QSYNC_MODE, &SDMServices::SetQSyncMode},
      {SDM_SERVICE_SET_COLOR_SAMPLING_ENABLED, &SDMServices::setColorSamplingEnabled},
      {SDM_SERVICE_SET_IDLE_PC, &SDMServices::SetIdlePC},
      {SDM_SERVICE_SET_DPPS_AD4_ROI_CONFIG, &SDMServices::SetAd4RoiConfig},
      {SDM_SERVICE_SET_DSI_CLK, &SDMServices::SetDsiClk},
      {SDM_SERVICE_SET_PANEL_LUMINANCE, &SDMServices::SetPanelLuminanceAttributes},
      {SDM_SERVICE_SET_COLOR_MODE_FROM_CLIENT, &SDMServices::SetColorModeFromClient},
      {SDM_SERVICE_SET_FRAME_TRIGGER_MODE, &SDMServices::SetFrameTriggerMode},
      {SDM_SERVICE_SET_BRIGHTNESS_SCALE, &SDMServices::ProcessDisplayBrightnessScale},
#ifdef PROFILE_COVERAGE_DATA
      {SDM_SERVICE_DUMP_CODE_COVERAGE, &SDMServices::DumpCodeCoverage},
#endif
      {SDM_SERVICE_UPDATE_TRANSFER_TIME, &SDMServices::UpdateTransferTime},
      {SDM_SERVICE_PERFORM_CAC_CONFIG, &SDMServices::PerformCacConfig},
      {SDM_SERVICE_SET_BPP_MODE, &SDMServices::SetBppMode},
  };

  std::unordered_map<uint32_t, VndCmdGetHandler> vnd_handlers_get_ = {
      {SDM_SERVICE_SET_SECONDARY_DISPLAY_STATUS, &SDMServices::SetDisplayStatus},
      {SDM_SERVICE_TOGGLE_SCREEN_UPDATES, &SDMServices::ToggleScreenUpdate},
      {SDM_SERVICE_QDCM_SVC_CMDS, &SDMServices::QdcmCMDHandler},
      {SDM_SERVICE_MIN_HDCP_ENCRYPTION_LEVEL_CHANGED, &SDMServices::MinHdcpEncryptionLevelChanged},
      {SDM_SERVICE_CONTROL_PARTIAL_UPDATE, &SDMServices::ControlPartialUpdate},
      {SDM_SERVICE_GET_ACTIVE_CONFIG, &SDMServices::GetActiveConfigIndex},
      {SDM_SERVICE_GET_CONFIG_COUNT, &SDMServices::GetConfigCount},
      {SDM_SERVICE_GET_DISPLAY_ATTRIBUTES_FOR_CONFIG, &SDMServices::GetDisplayAttributesForConfig},
      {SDM_SERVICE_GET_PANEL_BRIGHTNESS, &SDMServices::GetDisplayBrightness},
      {SDM_SERVICE_SET_PANEL_BRIGHTNESS, &SDMServices::SetDisplayBrightness},
      {SDM_SERVICE_GET_DISPLAY_VISIBLE_REGION, &SDMServices::GetVisibleDisplayRect},
      {SDM_SERVICE_GET_BW_TRANSACTION_STATUS, &SDMServices::DisplayBWTransactionPending},
      {SDM_SERVICE_GET_COMPOSER_STATUS, &SDMServices::GetComposerStatus},
      {SDM_SERVICE_GET_DSI_CLK, &SDMServices::GetDsiClk},
      {SDM_SERVICE_GET_SUPPORTED_DSI_CLK, &SDMServices::GetSupportedDsiClk},
      {SDM_SERVICE_SET_VSYNC_STATE, &SDMServices::SetVSyncState},
      {SDM_SERVICE_NOTIFY_TUI_TRANSITION, &SDMServices::HandleTUITransition},
      {SDM_SERVICE_SET_DIMMING_ENABLE, &SDMServices::SetDimmingEnable},
      {SDM_SERVICE_SET_DIMMING_MIN_BL, &SDMServices::SetDimmingMinBl},
      {SDM_SERVICE_RETRIEVE_DEMURATN_FILES, &SDMServices::RetrieveDemuraTnFiles},
      {SDM_SERVICE_SET_DEMURA_STATE, &SDMServices::SetDemuraState},
      {SDM_SERVICE_SET_DEMURA_CONFIG, &SDMServices::SetDemuraConfig},
      {SDM_SERVICE_GET_DISPLAY_PORT_ID, &SDMServices::GetDisplayPortId},
      {SDM_SERVICE_SET_PANEL_FEATURE_CONFIG, &SDMServices::SetPanelFeatureConfig},
  };

  int bw_mode_release_fd_ = -1;

  SDMServicesCbIntf *cb_ = nullptr;
  SDMDisplayBuilder *disp_ = nullptr;
  SDMTrustedUI *tui_ = nullptr;
  Locker *locker_ = nullptr;
  SDMColorManager *color_mgr_ = nullptr;
  BufferAllocator *buffer_allocator_ = nullptr;
  SocketHandler *socket_handler_ = nullptr;
  std::map<PanelFeatureVendorServiceType, std::string> panel_feature_data_type_map_ = {};
};

} // namespace sdm

#endif // __SDM_SERVICES_H__
