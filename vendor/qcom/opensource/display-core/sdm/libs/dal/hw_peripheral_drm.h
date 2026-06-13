/*
Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*
*    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
* GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __HW_PERIPHERAL_DRM_H__
#define __HW_PERIPHERAL_DRM_H__

#include <map>
#include <vector>
#include <string>
#include "hw_device_drm.h"

namespace sdm {

class HWPeripheralDRM : public HWDeviceDRM, public PanelFeaturePropertyIntf {
 public:
  explicit HWPeripheralDRM(int32_t display_id, BufferAllocator *buffer_allocator,
                           HWInfoInterface *hw_info_intf);
  virtual ~HWPeripheralDRM() {}
  virtual PanelFeaturePropertyIntf *GetPanelFeaturePropertyIntf() { return this; }
  virtual int GetPanelFeature(PanelFeaturePropertyInfo *feature_info);
  virtual int SetPanelFeature(const PanelFeaturePropertyInfo &feature_info);
  virtual DisplayError GetPanelBrightnessBasePath(std::string *base_path) const;
  virtual DisplayError GetQsyncFps(uint32_t *qsync_fps);

 protected:
  virtual DisplayError Init();
  virtual DisplayError Validate(HWLayersInfo *hw_layers_info);
  virtual DisplayError Commit(HWLayersInfo *hw_layers_info);
  virtual DisplayError Flush(HWLayersInfo *hw_layers_info);
  virtual DisplayError SetDppsFeature(void *payload, size_t size);
  virtual DisplayError GetDppsFeatureInfo(void *payload, size_t size);
  virtual DisplayError HandleSecureEvent(SecureEvent secure_event, const HWQosData &qos_data);
  virtual DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous);
  virtual DisplayError PowerOn(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError PowerOff(bool teardown, SyncPoints *sync_points);
  virtual DisplayError Doze(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError DozeSuspend(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError SetDisplayDppsAdROI(void *payload);
  virtual DisplayError SetJitterConfig(uint32_t jitter_type, float value, uint32_t time);
  virtual DisplayError SetDynamicDSIClock(uint64_t bit_clk_rate);
  virtual DisplayError GetDynamicDSIClock(uint64_t *bit_clk_rate);
  virtual DisplayError SetDisplayAttributes(uint32_t index);
  virtual DisplayError SetDisplayMode(const HWDisplayMode hw_display_mode);
  virtual DisplayError SetBppMode(uint32_t bpp);
  virtual DisplayError SetRefreshRate(uint32_t refresh_rate);
  virtual DisplayError SetFrameTrigger(FrameTriggerMode mode);
  virtual DisplayError SetPanelBrightness(int level);
  virtual DisplayError GetPanelBrightness(int *level);
  virtual void GetHWPanelMaxBrightness();
  virtual DisplayError SetBLScale(uint32_t level);
  virtual DisplayError EnableSelfRefresh(SelfRefreshState self_refresh_state);
  virtual DisplayError SetAlternateDisplayConfig(uint32_t *alt_config);
  virtual DisplayError UpdateTransferTime(uint32_t transfer_time);
  void SetDestScalarData(const HWLayersInfo &hw_layer_info);
  virtual bool IsAVRStepSupported(uint32_t config_index);
  virtual bool IsVRRSupported();

 private:
  void InitDestScaler();
  void SetDestScalarData(const DestScaleInfoMap dest_scale_info_map);
  void SetAIScalerData(const AIScalerInfoMap ai_scale_info_map);
  void ResetDestScalarCache();
  void CreatePanelFeaturePropertyMap();
  void SetIdlePCState() {
    drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_IDLE_PC_STATE, token_.crtc_id,
                              idle_pc_state_);
  }
  void CacheDestScalarData();
  void SetSelfRefreshState();
  void SetVMReqState();
  void ResetPropertyCache();
  void InitAIScaler();
  bool IsCACEnabled(const HWLayersInfo *hw_layers_info);
  DisplayError UpdateLoopBackConnector();
  DisplayError ConfigureLoopbackCAC(const bool cac_enabled);

  struct DestScalarCache {
    SDEScaler scalar_data = {};
    uint32_t flags = {};
  };

  struct AIScalerCache {
    struct drm_msm_ai_scaler scaler_data = {};
  };

  sde_drm_dest_scaler_data sde_dest_scalar_data_ = {};
  struct drm_msm_ai_scaler sde_ai_scaler_cfg_ = {};
  std::vector<SDEScaler> scalar_data_ = {};
  sde_drm::DRMIdlePCState idle_pc_state_ = sde_drm::DRMIdlePCState::NONE;
  bool idle_pc_enabled_ = true;
  std::vector<DestScalarCache> dest_scalar_cache_ = {};
  std::vector<AIScalerCache> ai_scaler_cache_ = {};
  drm_msm_ad4_roi_cfg ad4_roi_cfg_ = {};
  bool needs_ds_update_ = false;
  bool needs_ai_scaler_update_ = false;
  void PopulateBitClkRates();
  std::vector<uint64_t> bitclk_rates_;
  std::string brightness_base_path_ = "";
  SelfRefreshState self_refresh_state_ = kSelfRefreshNone;
  bool ltm_hist_en_ = false;
  bool aba_hist_en_ = false;
  std::map<PanelFeaturePropertyID, sde_drm::DRMPanelFeatureID> panel_feature_property_map_ {};

#ifdef SEC_GC_QC_SUPERHDR
private:
  bool GetSmoothDimOn();
  bool max_brightness_expanded_ = false;
  int brightness_set_ = -1;
#endif
};

}  // namespace sdm

#endif  // __HW_PERIPHERAL_DRM_H__
