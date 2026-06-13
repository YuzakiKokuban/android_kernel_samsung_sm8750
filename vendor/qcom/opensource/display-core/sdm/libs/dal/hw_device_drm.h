/*
* Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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

#ifndef __HW_DEVICE_DRM_H__
#define __HW_DEVICE_DRM_H__

#include <utils/formats.h>
#include <private/hw_interface.h>
#include <drm_interface.h>
#include <drm_master.h>
#include <errno.h>
#include <pthread.h>
#include <xf86drmMode.h>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

#include "hw_scale_drm.h"
#include "hw_color_manager_drm.h"

#define IOCTL_LOGE(ioctl, type) \
  DLOGE("ioctl %s, device = %d errno = %d, desc = %s", #ioctl, type, errno, strerror(errno))

#define UI_FBID_LIMIT 4
#define VIDEO_FBID_LIMIT 32
#define OFFLINE_ROTATOR_FBID_LIMIT 2

using drm_utils::DRMBuffer;
using sde_drm::DRMPowerMode;
namespace sdm {
class HWInfoInterface;

struct SDECsc {
  struct sde_drm_csc_v1 csc_v1 = {};
  // More here, maybe in a union
};

struct HWCwbConfig {
  bool enabled = false;
  sde_drm::DRMDisplayToken token = {};  // display token to be used for virtual connector while CWB
};

class HWDeviceDRM : public HWInterface {
 public:
  HWDeviceDRM(BufferAllocator *buffer_allocator, HWInfoInterface *hw_info_intf);
  virtual ~HWDeviceDRM() {}
  virtual DisplayError Init();
  virtual DisplayError Deinit();
  void GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) const;
  virtual PanelFeaturePropertyIntf *GetPanelFeaturePropertyIntf() { return nullptr; }
  virtual DisplayError GetPanelBrightnessBasePath(std::string *base_path) const {
    return kErrorNotSupported;
  }

 protected:
  // From HWInterface
  virtual DisplayError GetDisplayId(int32_t *display_id);
  virtual DisplayError GetActiveConfig(uint32_t *active_config);
  virtual DisplayError GetDefaultConfig(uint32_t *default_config) { return kErrorNotSupported; }
  virtual DisplayError GetNumDisplayAttributes(uint32_t *count);
  virtual DisplayError GetDisplayAttributes(uint32_t index,
                                            HWDisplayAttributes *display_attributes);
  virtual DisplayError GetHWPanelInfo(HWPanelInfo *panel_info);
  virtual DisplayError SetDisplayAttributes(uint32_t index);
  virtual DisplayError SetDisplayAttributes(const HWDisplayAttributes &display_attributes);
  virtual DisplayError GetConfigIndex(char *mode, uint32_t *index);
  virtual DisplayError PowerOn(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError PowerOff(bool teardown, SyncPoints *sync_points);
  virtual DisplayError Doze(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError DozeSuspend(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError Standby(SyncPoints *sync_points);
  virtual DisplayError Validate(HWLayersInfo *hw_layers_info);
  virtual DisplayError Commit(HWLayersInfo *hw_layers_info);
  virtual DisplayError Flush(HWLayersInfo *hw_layers_info);
  DisplayError SetupConcurrentWritebackModes(int32_t writeback_id);
  bool SetupConcurrentWriteback(const HWLayersInfo &hw_layer_info, bool validate,
                                int64_t *release_fence_fd);
  DisplayError TeardownConcurrentWriteback(void);
  void ConfigureConcurrentWriteback(const HWLayersInfo &hw_layer_info);
  void PostCommitConcurrentWriteback(std::shared_ptr<LayerBuffer> output_buffer);
  virtual DisplayError GetPPFeaturesVersion(PPFeatureVersion *vers);
  virtual DisplayError SetPPFeature(PPFeatureInfo *feature);
  // This API is no longer supported, expectation is to call the correct API on HWEvents
  virtual DisplayError SetVSyncState(bool enable);
  virtual void SetIdleTimeoutMs(uint32_t timeout_ms);
  virtual DisplayError SetDisplayMode(const HWDisplayMode hw_display_mode);
  virtual DisplayError SetBppMode(uint32_t bpp);
  virtual DisplayError SetRefreshRate(uint32_t refresh_rate);
  virtual DisplayError SetPanelBrightness(int level) { return kErrorNotSupported; }
  virtual DisplayError GetHWScanInfo(HWScanInfo *scan_info);
  virtual DisplayError GetVideoFormat(uint32_t config_index, uint32_t *video_format);
  virtual DisplayError GetMaxCEAFormat(uint32_t *max_cea_format);
  virtual DisplayError SetCursorPosition(HWLayersInfo *hw_layers_info, int x, int y);
  virtual DisplayError OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level);
  virtual DisplayError GetPanelBrightness(int *level) { return kErrorNotSupported; }
  virtual void GetHWPanelMaxBrightness() { return; }
  virtual DisplayError SetAutoRefresh(bool enable) { autorefresh_ = enable; return kErrorNone; }
  virtual DisplayError SetScaleLutConfig(HWScaleLutInfo *lut_info);
  virtual DisplayError UnsetScaleLutConfig();
  virtual DisplayError SetMixerAttributes(const HWMixerAttributes &mixer_attributes);
  virtual DisplayError GetMixerAttributes(HWMixerAttributes *mixer_attributes);
  virtual void InitializeConfigs();
  virtual DisplayError DumpDebugData();
  virtual void PopulateHWPanelInfo();
  virtual DisplayError SetDppsFeature(void *payload, size_t size) { return kErrorNotSupported; }
  virtual DisplayError GetDppsFeatureInfo(void *payload, size_t size) { return kErrorNotSupported; }
  virtual DisplayError HandleSecureEvent(SecureEvent secure_event, const HWQosData &qos_data) {
    switch (secure_event) {
      case kTUITransitionPrepare:
      case kTUITransitionUnPrepare:
        tui_state_ = kTUIStateInProgress;
        break;
      case kTUITransitionStart:
        tui_state_ = kTUIStateStart;
        break;
      case kTUITransitionEnd:
        tui_state_ = kTUIStateEnd;
        break;
      default:
        break;
    }
    return kErrorNone;
  }
  virtual DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetDisplayDppsAdROI(void *payload) { return kErrorNotSupported; }
  virtual DisplayError SetJitterConfig(uint32_t jitter_type, float value, uint32_t time);
  virtual DisplayError SetDynamicDSIClock(uint64_t bit_clk_rate);
  virtual DisplayError GetDynamicDSIClock(uint64_t *bit_clk_rate);
  virtual DisplayError GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                                    uint8_t *out_data);
  virtual DisplayError SetFrameTrigger(FrameTriggerMode mode) { return kErrorNotSupported; }
  virtual DisplayError SetBLScale(uint32_t level) { return kErrorNotSupported; }
  virtual DisplayError SetBlendSpace(const PrimariesTransfer &blend_space);
  virtual DisplayError EnableSelfRefresh(SelfRefreshState self_refresh_state) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetFeatureSupportStatus(const HWFeature feature, uint32_t *status);
  virtual void FlushConcurrentWriteback();
  virtual DisplayError UpdateTransferTime(uint32_t transfer_time);
  virtual DisplayError SetAlternateDisplayConfig(uint32_t *alt_config) {
    return kErrorNotSupported;
  }
  virtual DisplayError CancelDeferredPowerMode();
  virtual void HandleCwbTeardown(bool sync_teardown);
  virtual bool IsAVRStepSupported(uint32_t config_index) { return false; }
  virtual DisplayError NotifyExpectedPresent(uint64_t expected_present_time,
                                             uint32_t frame_interval_ns);
  virtual bool IsVRRSupported() { return false; }

  enum {
    kHWEventVSync,
    kHWEventBlank,
  };

  enum TUIState {
    kTUIStateNone,
    kTUIStateStart,
    kTUIStateInProgress,
    kTUIStateEnd,
  };

  static const int kMaxStringLength = 1024;
  static const int kNumPhysicalDisplays = 2;
  static const int kMaxSysfsCommandLength = 12;

  DisplayError SetFormat(const LayerBufferFormat &source, uint32_t *target);
  DisplayError SetStride(HWDeviceType device_type, LayerBufferFormat format, uint32_t width,
                         uint32_t *target);
  DisplayError PopulateDisplayAttributes(uint32_t index);
  void GetHWDisplayPortAndMode();
  bool EnableHotPlugDetection(int enable);
  void UpdateMixerAttributes();
  void SetSolidfillStages();
  void AddSolidfillStage(const HWSolidfillStage &sf, uint32_t plane_alpha);
  void ClearSolidfillStages();
  void SetNoiseLayerConfig(const NoiseLayerConfig &noise_config);
  void ApplyNoiseLayerConfig();
  void ClearNoiseLayerConfig();
  void SetBlending(const LayerBlending &source, sde_drm::DRMBlendType *target);
  void SetSrcConfig(const LayerBuffer &input_buffer, const HWRotatorMode &mode, uint32_t *config);
  void SelectCscType(const LayerBuffer &input_buffer, sde_drm::DRMCscType *type);
  void SelectFp16Config(const LayerBuffer &input_buffer, int *igc_en, int *unmult_en,
                        sde_drm::DRMFp16CscType *csc_type, drm_msm_fp16_gc *gc,
                        LayerBlending blend);
  void SetRect(const LayerRect &source, sde_drm::DRMRect *target);
  void SetRotation(LayerTransform transform, const HWLayerConfig &layer_config,
                   uint32_t* rot_bit_mask);
  DisplayError DefaultCommit(HWLayersInfo *hw_layers_info);
  DisplayError AtomicCommit(HWLayersInfo *hw_layers_info);
  void SetupAtomic(Fence::ScopedRef &scoped_ref, HWLayersInfo *hw_layers_info, bool validate,
                   int64_t *release_fence_fd, int64_t *retire_fence_fd);
  void SetSecureConfig(const LayerBuffer &input_buffer, sde_drm::DRMSecureMode *fb_secure_mode,
                       sde_drm::DRMSecurityLevel *security_level);
  void SetTopology(sde_drm::DRMTopology drm_topology, HWTopology *hw_topology);
  void SetMultiRectMode(const uint32_t flags, sde_drm::DRMMultiRectMode *target);
  void SetSsppTonemapFeatures(HWPipeInfo *pipe_info);
  void SetLegacyTonemapFeatures(HWPipeInfo *pipe_info);
#ifdef UCSC_SUPPORTED
  void SetUcscTonemapFeatures(HWPipeInfo *pipe_info);
  void SetUcscIgc(const HWUcscIgcMode igc_lut_sel, sde_drm::DRMUcscIgcMode *igc);
  void SetUcscGc(const HWUcscGcMode gc_lut_sel, sde_drm::DRMUcscGcMode *gc);
  void SetUcscCsc(const HWUcscCsc &ucsc_csc, drm_msm_ucsc_csc *csc);
#endif
  void SetDGMCsc(const HWPipeCscInfo &dgm_csc_info, SDECsc *csc);
  void SetDGMCscV1(const HWCsc &dgm_csc, sde_drm_csc_v1 *csc_v1);
  void SetSsppLutFeatures(HWPipeInfo *pipe_info);
  void AddDimLayerIfNeeded();
  DisplayError NullCommit(bool synchronous, bool retain_planes);
  void DumpConnectorModeInfo();
  void ResetROI();
  void SetQOSData(const HWQosData &qos_data);
  void DumpHWLayers(HWLayersInfo *hw_layers_info);
  bool IsFullFrameUpdate(const HWLayersInfo &hw_layer_info);
  DisplayError GetDRMPowerMode(const HWPowerState &power_state, DRMPowerMode *drm_power_mode);
  void SetTUIState();
  DisplayError ConfigureCWBDither(void *payload, uint32_t conn_id,
                                  sde_drm::DRMCWbCaptureMode mode);
  void SetTopologySplit(HWTopology hw_topology, uint32_t *split_number);
  uint64_t GetSupportedBitClkRate(uint32_t new_mode_index,
                                  uint64_t bit_clk_rate_request);
  DisplayError GetPanelBlMaxLvl(uint32_t *bl_max);
  DisplayError SetPPConfig(void *payload, size_t size);
  DisplayError GetQsyncFps(uint32_t *qsync_fps) { return kErrorNotSupported; }
  void SetDestScalarData(const HWLayersInfo &hw_layer_info) {
    return;
  };
  void SetCacType(const HWPipeCacMode &cac_mode, sde_drm::DRMCacMode *target);

  class Registry {
   public:
    explicit Registry(BufferAllocator *buffer_allocator);
    // Init master
    void Init(Handle master, CacVersion cac_version, uint32_t core_id);
    // Called on each Validate and Commit to map the handle_id to fb_id of each layer buffer.
    int Register(HWLayersInfo *hw_layers_info);
    // Called on display disconnect to clear output buffer map and remove fb_ids.
    void Clear();
    // Create the fd_id for the given buffer.
    int CreateFbId(const LayerBuffer &buffer, std::vector<uint32_t> *fb_id,
                   BufferInfo *loopback_cac_info = nullptr);
    // Find handle_id in the layer map. Else create fb_id and add <handle_id,fb_id> in map.
    int MapBufferToFbId(Layer *layer, const LayerBuffer &buffer, bool *fb_modified,
                        bool is_cac_buffer, BufferInfo &loopback_cac_info);
    // Find handle_id in output buffer map. Else create fb_id and add <handle_id,fb_id> in map.
    void MapOutputBufferToFbId(std::shared_ptr<LayerBuffer> buffer, bool *fb_modified);
    // Find fb_id for given handle_id in the layer map.
    void GetFbId(Layer *layer, uint64_t handle_id, std::vector<uint32_t> *fb_id);
    // Find fb_id for given handle_id in output buffer map.
    uint32_t GetOutputFbId(uint64_t handle_id);

   private:
    void GetBufInfoForTunnelPipe(HWCacColorComponent color, BufferInfo *loopback_cac_info,
                                 AllocatedBufferInfo *buf_info, DRMBuffer *layout);
    bool disable_fbid_cache_ = false;
    std::unordered_map<uint64_t, std::unordered_map<uint32_t, std::shared_ptr<LayerBufferObject>>>
                                                              output_buffer_map_;
    BufferAllocator *buffer_allocator_ = {};
    uint8_t fbid_cache_limit_ = UI_FBID_LIMIT;
    Handle master_ = nullptr;
    CacVersion cac_version_ = kCacVersionNone;
    uint32_t core_id_;
  };

 protected:
  void SetDisplaySwitchMode(uint32_t index);
  DisplayError UpdateLoopBackConnector();
  bool IsSeamlessTransition() {
    return (hw_panel_info_.dynamic_fps && (vrefresh_ || seamless_mode_switch_)) ||
     panel_mode_changed_ || bit_clk_rate_;
  }

  const char *device_name_ = {};
  bool default_mode_ = false;
  int32_t display_id_ = -1;
  uint32_t core_id_ = 0;
  sde_drm::DRMDisplayType disp_type_ = {};
  HWInfoInterface *hw_info_intf_ = {};
  int dev_fd_ = -1;
  Registry registry_;
  sde_drm::DRMDisplayToken token_ = {};
  sde_drm::DRMDisplayToken loopback_token_ = {};
  bool loopback_cac_configured_ = false;
  HWResourceInfo hw_resource_ = {};
  HWPanelInfo hw_panel_info_ = {};
  HWScaleDRM *hw_scale_ = {};
  sde_drm::DRMManagerInterface *drm_mgr_intf_ = {};
  sde_drm::DRMAtomicReqInterface *drm_atomic_intf_ = {};
  std::vector<HWDisplayAttributes> display_attributes_ = {};
  uint32_t current_mode_index_ = 0;
  sde_drm::DRMConnectorInfo connector_info_ = {};
  bool first_cycle_ = true;
  bool first_null_cycle_ = true;
  HWMixerAttributes mixer_attributes_ = {};
  std::vector<sde_drm::DRMSolidfillStage> solid_fills_ {};
  sde_drm::DRMNoiseLayerConfig noise_cfg_ = {};
  bool secure_display_active_ = false;
  TUIState tui_state_ = kTUIStateNone;
  uint64_t debug_dump_count_ = 0;
  bool synchronous_commit_ = false;
  uint32_t topology_control_ = 0;
  uint32_t vrefresh_ = 0;
  uint32_t panel_mode_changed_ = 0;
  uint32_t panel_compression_changed_ = 0;
  uint32_t bpp_mode_changed_ = 0;
  bool reset_output_fence_offset_ = false;
  uint64_t bit_clk_rate_ = 0;
  bool update_mode_ = false;
  HWPowerState pending_power_state_ = kPowerStateNone;
  uint32_t video_mode_index_ = 0;
  uint32_t cmd_mode_index_ = 0;
  bool switch_mode_valid_ = false;
  bool doze_poms_switch_done_ = false;
  bool pending_poms_switch_ = false;
  bool active_ = false;
  bool pending_cwb_teardown_ = false;
  PrimariesTransfer blend_space_ = {};
  DRMPowerMode last_power_mode_ = DRMPowerMode::OFF;
  uint32_t dest_scaler_blocks_used_ = 0;  // Dest scaler blocks in use by this HWDeviceDRM instance.
  static bool reset_planes_luts_;
  // Destination scaler blocks in use by all HWDeviceDRM instances.
  static std::unordered_map<uint32_t, std::atomic<uint32_t>> hw_dest_scaler_blocks_used_;

  bool has_cwb_crop_ = false;       // virtual connector supports CWB ROI feature.
  bool has_dedicated_cwb_ = false;  // virtual connector supports dedicated CWB feature.
  uint32_t max_cwb_ = 0;            // Max number of concurrent CWB operations on virtual connector.
  bool has_cwb_dither_ = false;     // virtual connector supports CWB Dither feature.
  uint32_t transfer_time_updated_ = 0;
  std::unordered_map<uint32_t, HWCwbConfig> cwb_config_;
  // cwb state lock. Set before accesing or updating cwb_config_
  static std::unordered_map<uint32_t, std::mutex> cwb_state_lock_;
  bool force_tonemapping_ = false;
  uint32_t ai_scaler_blocks_used_ = 0;  // AI scaler blocks in use by this HWDeviceDRM instance.
  static std::atomic<uint32_t> hw_ai_scaler_blocks_used_;
  bool enable_brightness_drm_prop_ = false;
  int cached_brightness_level_ = -1;
  int current_brightness_ = -1;
  int32_t loopback_conn_id_ = -1;

 private:
  void GetCWBCapabilities();

  std::string interface_str_ = "DSI";
  bool autorefresh_ = false;
  std::unique_ptr<HWColorManagerDrm> hw_color_mgr_ = {};
  bool seamless_mode_switch_ = false;
  float aspect_ratio_threshold_ = 1.0;
};

}  // namespace sdm

#endif  // __HW_DEVICE_DRM_H__
