/*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __DPU_CORE_MUX_H__
#define __DPU_CORE_MUX_H__

#include <stdio.h>
#include <malloc.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/formats.h>
#include <utils/rect.h>
#include <utils/utils.h>
#include <utils/multi_core_instantiator.h>
#include <core/sdm_types.h>
#include <drm_interface.h>

#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

#include <private/hw_interface.h>
#include <private/hw_info_interface.h>

namespace sdm {

class DPUCoreMux {
 public:
  virtual DisplayError Destroy() = 0;
  virtual DisplayError Init() = 0;
  virtual ~DPUCoreMux(){};
  virtual DisplayError GetDisplayId(int32_t *display_id) = 0;
  virtual DisplayError GetActiveConfig(uint32_t *active_config) = 0;
  virtual DisplayError GetDefaultConfig(uint32_t *default_config) = 0;
  virtual DisplayError GetNumDisplayAttributes(uint32_t *count) = 0;
  virtual DisplayError GetDisplayAttributes(uint32_t index, DisplayDeviceContext *device_ctx,
                                            DisplayClientContext *client_ctx) = 0;
  virtual DisplayError GetHWPanelInfo(DisplayDeviceContext *device_ctx,
                                      DisplayClientContext *client_ctx) = 0;
  virtual DisplayError SetDisplayAttributes(uint32_t index) = 0;
  virtual DisplayError SetDisplayAttributes(const HWDisplayAttributes &display_attributes) = 0;
  virtual DisplayError GetConfigIndex(char *mode, uint32_t *index) = 0;
  virtual DisplayError PowerOn(std::map<uint32_t, HWQosData> &qos_data,
                               SyncPoints *sync_points) = 0;
  virtual DisplayError PowerOff(bool teardown, SyncPoints *sync_points) = 0;
  virtual DisplayError Doze(std::map<uint32_t, HWQosData> &qos_data, SyncPoints *sync_points) = 0;
  virtual DisplayError DozeSuspend(std::map<uint32_t, HWQosData> &qos_data,
                                   SyncPoints *sync_points) = 0;
  virtual DisplayError Standby(SyncPoints *sync_points) = 0;
  virtual DisplayError Validate(std::map<uint32_t, HWLayersInfo> &hw_layers_info) = 0;
  virtual DisplayError Commit(std::map<uint32_t, HWLayersInfo> &hw_layers_info) = 0;
  virtual DisplayError Flush(std::map<uint32_t, HWLayersInfo> &hw_layers_info) = 0;
  virtual DisplayError GetPPFeaturesVersion(PPFeatureVersion *vers, uint32_t core_id) = 0;
  virtual DisplayError SetPPFeature(PPFeatureInfo *feature, uint32_t &core_id) = 0;
  virtual DisplayError SetVSyncState(bool enable) = 0;
  virtual void SetIdleTimeoutMs(uint32_t timeout_ms) = 0;
  virtual DisplayError SetDisplayMode(const HWDisplayMode hw_display_mode) = 0;
  virtual DisplayError SetRefreshRate(uint32_t refresh_rate) = 0;
  virtual DisplayError SetPanelBrightness(int level) = 0;
  virtual DisplayError GetHWScanInfo(HWScanInfo *scan_info) = 0;
  virtual DisplayError GetVideoFormat(uint32_t config_index, uint32_t *video_format) = 0;
  virtual DisplayError GetMaxCEAFormat(uint32_t *max_cea_format) = 0;
  virtual DisplayError SetCursorPosition(std::map<uint32_t, HWLayersInfo> &hw_layers_info, int x,
                                         int y) = 0;
  virtual DisplayError OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level) = 0;
  virtual DisplayError GetPanelBrightness(int *level) = 0;
  virtual DisplayError SetAutoRefresh(bool enable) = 0;
  virtual DisplayError SetScaleLutConfig(HWScaleLutInfo *lut_info) = 0;
  virtual DisplayError UnsetScaleLutConfig() = 0;
  virtual DisplayError SetMixerAttributes(const HWMixerAttributes &mixer_attributes) = 0;
  virtual DisplayError GetMixerAttributes(DisplayDeviceContext *device_ctx,
                                          DisplayClientContext *client_ctx) = 0;
  virtual DisplayError DumpDebugData() = 0;
  virtual DisplayError SetDppsFeature(void *payload, size_t size) = 0;
  virtual DisplayError GetDppsFeatureInfo(void *payload, size_t size) = 0;
  virtual DisplayError HandleSecureEvent(SecureEvent secure_event,
                                         std::map<uint32_t, HWQosData> &qos_data) = 0;
  virtual DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous) = 0;
  virtual DisplayError SetDisplayDppsAdROI(void *payload) = 0;
  virtual DisplayError SetDynamicDSIClock(uint64_t bit_clk_rate) = 0;
  virtual DisplayError GetDynamicDSIClock(uint64_t *bit_clk_rate) = 0;
  virtual DisplayError GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                                    uint8_t *out_data) = 0;
  virtual DisplayError SetFrameTrigger(FrameTriggerMode mode, uint32_t core_id) = 0;
  virtual DisplayError SetFrameTrigger(FrameTriggerMode mode) = 0;
  virtual DisplayError SetBLScale(uint32_t level) = 0;
  virtual DisplayError GetPanelBlMaxLvl(uint32_t *max_bl) = 0;
  virtual DisplayError GetPanelBrightnessBasePath(std::string *base_path) const = 0;
  virtual DisplayError SetBlendSpace(const PrimariesTransfer &blend_space) = 0;
  virtual DisplayError EnableSelfRefresh(SelfRefreshState self_refresh_state) = 0;
  virtual PanelFeaturePropertyIntf *GetPanelFeaturePropertyIntf() = 0;
  virtual DisplayError GetFeatureSupportStatus(const HWFeature feature, uint32_t *status) = 0;
  virtual void FlushConcurrentWriteback() = 0;
  virtual DisplayError SetAlternateDisplayConfig(uint32_t *alt_config) = 0;
  virtual DisplayError GetQsyncFps(uint32_t *qsync_fps) = 0;
  virtual DisplayError CancelDeferredPowerMode() = 0;
  virtual void GetHWInterface(HWInterface **intf) = 0;
  virtual void GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) const = 0;
  virtual DisplayError SetPPConfig(void *payload, size_t size) = 0;
  virtual DisplayError GetFbConfig(uint32_t width, uint32_t height,
                                   DisplayDeviceContext *device_ctx,
                                   DisplayClientContext *client_ctx) = 0;
};

}  // namespace sdm

#endif  // __DPU_CORE_MUX_H__
