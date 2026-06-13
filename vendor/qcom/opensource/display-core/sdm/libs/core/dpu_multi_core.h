/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DPU_MULTI_CORE_H__
#define __DPU_MULTI_CORE_H__

#include "dpu_core_mux.h"

namespace sdm {

class DPUMultiCore : public DPUCoreMux {
 public:
  DisplayError Destroy();

  DPUMultiCore(DisplayId display_id, SDMDisplayType type,
               MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
               BufferAllocator *buffer_allocator);
  DisplayError Init();
  DisplayError GetDisplayId(int32_t *display_id);
  DisplayError GetActiveConfig(uint32_t *active_config);
  DisplayError GetDefaultConfig(uint32_t *default_config);
  DisplayError GetNumDisplayAttributes(uint32_t *count);
  DisplayError GetDisplayAttributes(uint32_t index, DisplayDeviceContext *device_ctx,
                                    DisplayClientContext *client_ctx);
  DisplayError GetHWPanelInfo(DisplayDeviceContext *device_ctx, DisplayClientContext *client_ctx);
  DisplayError SetDisplayAttributes(uint32_t index);
  DisplayError SetDisplayAttributes(const HWDisplayAttributes &display_attributes);
  DisplayError GetConfigIndex(char *mode, uint32_t *index);
  DisplayError PowerOn(std::map<uint32_t, HWQosData> &qos_data, SyncPoints *sync_points);
  DisplayError PowerOff(bool teardown, SyncPoints *sync_points);
  DisplayError Doze(std::map<uint32_t, HWQosData> &qos_data, SyncPoints *sync_points);
  DisplayError DozeSuspend(std::map<uint32_t, HWQosData> &qos_data, SyncPoints *sync_points);
  DisplayError Standby(SyncPoints *sync_points);
  DisplayError Validate(std::map<uint32_t, HWLayersInfo> &hw_layers_info);
  DisplayError Commit(std::map<uint32_t, HWLayersInfo> &hw_layers_info);
  DisplayError Flush(std::map<uint32_t, HWLayersInfo> &hw_layers_info);
  DisplayError GetPPFeaturesVersion(PPFeatureVersion *vers, uint32_t core_id);
  DisplayError SetPPFeature(PPFeatureInfo *feature, uint32_t &core_id);
  DisplayError SetVSyncState(bool enable);
  void SetIdleTimeoutMs(uint32_t timeout_ms);
  DisplayError SetDisplayMode(const HWDisplayMode hw_display_mode);
  DisplayError SetRefreshRate(uint32_t refresh_rate);
  DisplayError SetPanelBrightness(int level);
  DisplayError GetHWScanInfo(HWScanInfo *scan_info);
  DisplayError GetVideoFormat(uint32_t config_index, uint32_t *video_format);
  DisplayError GetMaxCEAFormat(uint32_t *max_cea_format);
  DisplayError SetCursorPosition(std::map<uint32_t, HWLayersInfo> &hw_layers_info, int x, int y);
  DisplayError OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level);
  DisplayError GetPanelBrightness(int *level);
  DisplayError SetAutoRefresh(bool enable);
  DisplayError SetScaleLutConfig(HWScaleLutInfo *lut_info);
  DisplayError UnsetScaleLutConfig();
  DisplayError SetMixerAttributes(const HWMixerAttributes &mixer_attributes);
  DisplayError GetMixerAttributes(DisplayDeviceContext *device_ctx,
                                  DisplayClientContext *client_ctx);
  DisplayError DumpDebugData();
  DisplayError SetDppsFeature(void *payload, size_t size);
  DisplayError GetDppsFeatureInfo(void *payload, size_t size);
  DisplayError HandleSecureEvent(SecureEvent secure_event, std::map<uint32_t, HWQosData> &qos_data);
  DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous);
  DisplayError SetDisplayDppsAdROI(void *payload);
  DisplayError SetDynamicDSIClock(uint64_t bit_clk_rate);
  DisplayError GetDynamicDSIClock(uint64_t *bit_clk_rate);
  DisplayError GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                            uint8_t *out_data);
  DisplayError SetFrameTrigger(FrameTriggerMode mode, uint32_t core_id);
  DisplayError SetFrameTrigger(FrameTriggerMode mode);
  DisplayError SetBLScale(uint32_t level);
  DisplayError GetPanelBlMaxLvl(uint32_t *max_bl);
  DisplayError GetPanelBrightnessBasePath(std::string *base_path) const;
  DisplayError SetBlendSpace(const PrimariesTransfer &blend_space);
  DisplayError EnableSelfRefresh(SelfRefreshState self_refresh_state);
  PanelFeaturePropertyIntf *GetPanelFeaturePropertyIntf();
  DisplayError GetFeatureSupportStatus(const HWFeature feature, uint32_t *status);
  void FlushConcurrentWriteback();
  DisplayError SetAlternateDisplayConfig(uint32_t *alt_config);
  DisplayError GetQsyncFps(uint32_t *qsync_fps);
  DisplayError CancelDeferredPowerMode();
  void GetHWInterface(HWInterface **intf);
  void GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) const;
  DisplayError SetPPConfig(void *payload, size_t size);
  DisplayError GetFbConfig(uint32_t width, uint32_t height, DisplayDeviceContext *device_ctx,
                           DisplayClientContext *client_ctx);
  ~DPUMultiCore() {}

 private:
  std::map<uint32_t, HWInterface *> hw_intf_;
  std::vector<uint32_t> core_ids_;
  DisplayId display_id_ = {};
  SDMDisplayType type_;
  MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf_;
  BufferAllocator *buffer_allocator_;
  bool dpu_ctl_op_sync_ = false;
  std::vector<uint32_t> op_sync_sequence_;
  template <typename T>
  bool AreAllEntriesSame(std::vector<T> &vec);
  void SetOpSyncHint(bool dpu_ctl_op_sync);
};

}  // namespace sdm

#endif  // __DPU_MULTI_CORE_H__
