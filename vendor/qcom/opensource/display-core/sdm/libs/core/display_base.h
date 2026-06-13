/*
* Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted
* provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright notice, this list of
*      conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright notice, this list of
*      conditions and the following disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its contributors may be used to
*      endorse or promote products derived from this software without specific prior written
*      permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __DISPLAY_BASE_H__
#define __DISPLAY_BASE_H__

#include <core/display_interface.h>
#include <private/abc_feature_fact_intf.h>
#include <private/color_interface.h>
#include <private/demuratn_core_uvm_fact_intf.h>
#include <private/feature_license_intf.h>
#include <private/hw_events_interface.h>
#include <private/hw_interface.h>
#include <private/noise_plugin_dbg.h>
#include <private/noise_plugin_intf.h>
#include <private/panel_feature_factory_intf.h>
#include <private/panel_feature_property_intf.h>
#include <private/rc_intf.h>
#include <private/strategy_interface.h>
#include <utils/multi_core_instantiator.h>

#include <limits.h>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>  // NOLINT
#include <string>
#include <vector>
#include <atomic>

#include "comp_manager.h"
#include "color_manager.h"
#include "dpu_core_factory.h"
#include "dpu_core_mux.h"

using aiqe::GetABCFeatureFactIntf;

#define GET_PANEL_FEATURE_FACTORY "GetPanelFeatureFactoryIntf"
#define GET_DEMURATN_FACTORY "GetDemuraTnCoreUvmFactoryIntf"
#define GET_FEATURE_LICENSE_FACTORY "GetFeatureLicenseFactoryIntf"
#define GET_ABC_FACTORY "GetABCFeatureFactIntf"

namespace sdm {

#define NOISE_PLUGIN_VERSION_MAJOR (1)  // Noise Plugin major version number
#define NOISE_PLUGIN_VERSION_MINOR (0)  // Noise Plugin minor version number

using std::recursive_mutex;
using std::lock_guard;

typedef PanelFeatureFactoryIntf* (*GetPanelFeatureFactory)();
typedef DemuraTnCoreUvmFactoryIntf* (*GetDemuraTnFactory)();
typedef FeatureLicenseFactoryIntf* (*GetFeatureLicenseFactory)();
typedef aiqe::ABCFeatureFactIntf *(*GetABCFactory)();

class DisplayBase : public DisplayInterface, public CompManagerEventHandler {
 public:
  DisplayBase(SDMDisplayType display_type, DisplayEventHandler *event_handler,
              HWDeviceType hw_device_type, BufferAllocator *buffer_allocator,
              CompManager *comp_manager,
              sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf);
  DisplayBase(DisplayId display_id, SDMDisplayType display_type, DisplayEventHandler *event_handler,
              HWDeviceType hw_device_type, BufferAllocator *buffer_allocator,
              CompManager *comp_manager,
              sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf);
  virtual ~DisplayBase();
  virtual DisplayError Init();
  virtual DisplayError Deinit();
  virtual DisplayError Prepare(LayerStack *layer_stack);
  virtual DisplayError PrePrepare(LayerStack *layer_stack);
  virtual DisplayError CommitOrPrepare(LayerStack *layer_stack);
  virtual DisplayError Commit(LayerStack *layer_stack);
  virtual DisplayError Flush(LayerStack *layer_stack);
  virtual DisplayError FlushLocked(LayerStack *layer_stack);
  virtual DisplayError GetDisplayState(DisplayState *state);
  virtual DisplayError GetNumVariableInfoConfigs(uint32_t *count);
  virtual DisplayError GetConfig(uint32_t index, DisplayConfigVariableInfo *variable_info);
  virtual DisplayError GetConfig(DisplayConfigFixedInfo *fixed_info);
  virtual DisplayError GetRealConfig(uint32_t index, DisplayConfigVariableInfo *variable_info);
  virtual DisplayError GetActiveConfig(uint32_t *index);
  virtual DisplayError GetVSyncState(bool *enabled);
  virtual DisplayError SetDrawMethod(DisplayDrawMethod draw_method);
  virtual DisplayError SetDisplayState(DisplayState state, bool teardown,
                                       shared_ptr<Fence> *release_fence);
  virtual DisplayError SetActiveConfig(uint32_t index);
  virtual DisplayError SetActiveConfig(DisplayConfigVariableInfo *variable_info) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetNoisePlugInOverride(bool override_en, int32_t attn, int32_t noise_zpos);
  virtual DisplayError SetMaxMixerStages(uint32_t max_mixer_stages);
  virtual DisplayError ControlPartialUpdate(bool enable, uint32_t *pending) {
    return kErrorNotSupported;
  }
  virtual DisplayError DisablePartialUpdateOneFrame() {
    return kErrorNotSupported;
  }
  virtual DisplayError DisablePartialUpdateOneFrameInternal() {
    return kErrorNotSupported;
  }
  virtual DisplayError SetDisplayMode(uint32_t mode) {
    return kErrorNotSupported;
  }
  virtual bool IsUnderscanSupported() {
    return false;
  }
  virtual DisplayError SetPanelBrightness(float brightness) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetBppMode(uint32_t bpp) {
    return kErrorNotSupported;
  }
  virtual DisplayError OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level);
  virtual DisplayError ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                            PPDisplayAPIPayload *out_payload,
                                            PPPendingParams *pending_action);
  virtual DisplayError SetJitterConfig(uint32_t jitter_type, float value, uint32_t time) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetDynamicDSIClock(uint64_t bit_clk_rate) {
    return kErrorNotSupported;
  }
#ifdef SEC_GC_QC_DYN_CLK
  virtual DisplayError SetDynamicDSIClockCustom(uint64_t bit_clk_rate) {
  	return kErrorNotSupported;
  }
#endif
  virtual DisplayError GetDynamicDSIClock(uint64_t *bit_clk_rate) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetSupportedDSIClock(std::vector<uint64_t> *bitclk_rates) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetPanelLuminanceAttributes(float min_lum, float max_lum) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetColorModeCount(uint32_t *mode_count);
  virtual DisplayError GetColorModes(uint32_t *mode_count, std::vector<std::string> *color_modes);
  virtual DisplayError GetColorModeAttr(const std::string &color_mode, AttrVal *attr);
  virtual DisplayError SetColorMode(const std::string &color_mode);
  virtual DisplayError SetColorModeById(int32_t color_mode_id);
  virtual DisplayError GetColorModeName(int32_t mode_id, std::string *mode_name);
  virtual DisplayError SetColorTransform(const uint32_t length, const double *color_transform);
  virtual DisplayError GetDefaultColorMode(std::string *color_mode);
  virtual DisplayError ApplyDefaultDisplayMode(void);
  virtual DisplayError SetCursorPosition(int x, int y);
  virtual DisplayError GetRefreshRateRange(uint32_t *min_refresh_rate, uint32_t *max_refresh_rate);
  virtual DisplayError GetPanelBrightness(float *brightness) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetPanelBrightnessLevel(int *level) { return kErrorNotSupported; }
  virtual DisplayError GetPanelMaxBrightness(uint32_t *max_brightness_level) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetVSyncState(bool enable);
  virtual void SetIdleTimeoutMs(uint32_t active_ms, uint32_t inactive_ms) {}
  virtual DisplayError SetMixerResolution(uint32_t width, uint32_t height);
  virtual DisplayError GetMixerResolution(uint32_t *width, uint32_t *height);
  virtual DisplayError SetFrameBufferConfig(const DisplayConfigVariableInfo &variable_info);
  virtual DisplayError GetFrameBufferConfig(DisplayConfigVariableInfo *variable_info);
  virtual DisplayError SetDetailEnhancerData(const DisplayDetailEnhancerData &de_data);
  virtual DisplayError GetDisplayPort(DisplayPort *port);
  virtual DisplayError GetDisplayId(int32_t *display_id);
  virtual DisplayError GetConnectorId(int32_t *conn_id);
  virtual DisplayError GetDisplayType(SDMDisplayType *display_type);
  virtual bool IsPrimaryDisplay();
  virtual DisplayError SetCompositionState(LayerComposition composition_type, bool enable);
  virtual DisplayError GetClientTargetSupport(uint32_t width, uint32_t height,
                                              LayerBufferFormat format,
                                              const Dataspace &color_metadata);
  virtual DisplayError HandleSecureEvent(SecureEvent secure_event, bool *needs_refresh);
  virtual DisplayError ValidateCwbRoiWithOutputBuffer(const LayerBuffer &output_buffer,
                                                      CwbConfig &cwb_config);
  virtual DisplayError CaptureCwb(const LayerBuffer &output_buffer, const CwbConfig &config);
  virtual DisplayError PostHandleSecureEvent(SecureEvent secure_event) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetDisplayDppsAdROI(void *payload) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetQSyncMode(QSyncMode qsync_mode) { return kErrorNotSupported; }
  virtual void AppendRCMaskData(std::ostringstream &os);
  virtual std::string Dump();
  virtual DisplayError InitializeColorModes();
  virtual DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous) {
    return kErrorNotSupported;
  }
  virtual bool IsSupportSsppTonemap();
  virtual DisplayError GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                                    uint8_t *out_data);
  virtual DisplayError SetFrameTriggerMode(FrameTriggerMode mode) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetRefreshRate(uint32_t *refresh_rate) { return kErrorNotSupported; }
  virtual DisplayError SetBLScale(uint32_t level) { return kErrorNotSupported; }
  DisplayError GetPanelBlMaxLvl(uint32_t *bl_max);
  DisplayError SetPPConfig(void *payload, size_t size);
  DisplayError SetDimmingEnable(int int_enabled);
  DisplayError SetDimmingMinBl(int min_bl);
  DisplayError RetrieveDemuraTnFiles() {
    return kErrorNotSupported;
  }
  void ScreenRefresh();
  virtual bool CheckResourceState(bool *res_exhausted);
  virtual bool GameEnhanceSupported();
  virtual DisplayError GetQSyncMode(QSyncMode *qsync_mode) { return kErrorNotSupported; }
  virtual DisplayError colorSamplingOn();
  virtual DisplayError colorSamplingOff();
  virtual DisplayError ReconfigureDisplay();
  virtual DisplayError GetStcColorModes(snapdragoncolor::ColorModeList *mode_list) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetStcColorMode(const snapdragoncolor::ColorMode &color_mode) {
    return kErrorNotSupported;
  }
  virtual DisplayError ClearLUTs() {
    return kErrorNotSupported;
  }
  virtual DisplayError IsSupportedOnDisplay(SupportedDisplayFeature feature, uint32_t *supported);
  virtual bool IsWriteBackSupportedFormat(const LayerBufferFormat &format);
  virtual DisplayError GetCwbBufferResolution(CwbConfig *cwb_config, uint32_t *x_pixels,
                                              uint32_t *y_pixels);
  virtual DisplayError NotifyDisplayCalibrationMode(bool in_calibration) {
    return kErrorNotSupported;
  }
  virtual bool HasDemura() { return false; }
  virtual DisplayError GetPanelFeatureInfo(PanelFeatureInfo *info);
  virtual DisplayError GetOutputBufferAcquireFence(shared_ptr<Fence> *out_fence);
  virtual bool IsValidated() {
    ClientLock lock(disp_mutex_);
    return validated_ && !needs_validate_;
  }
  DisplayError DestroyLayer() {
    ClientLock lock(disp_mutex_);
    return kErrorNone;
  }
  virtual DisplayError GetQsyncFps(uint32_t *qsync_fps) { return kErrorNotSupported; }
  virtual void FlushConcurrentWriteback();
  virtual DisplayError SetAlternateDisplayConfig(uint32_t *alt_config) {
    return kErrorNotSupported;
  }
  virtual DisplayError ForceToneMapUpdate(LayerStack *layer_stack);
  virtual void EnableLlccDuringAodMode(LayerStack *layer_stack);
  virtual DisplayError UpdateTransferTime(uint32_t transfer_time) { return kErrorNotSupported; }
  virtual void NotifyCwbDone(int32_t status, const LayerBuffer& buffer);
  virtual void Refresh();
  virtual void OnCwbTeardown(bool sync_teardown);
  virtual bool HandleCwbTeardown();
  virtual uint32_t GetAvailableMixerCount();
  virtual DisplayError SetDemuraState(int state) { return kErrorNotSupported; }
  virtual DisplayError SetDemuraConfig(int demura_idx) { return kErrorNotSupported; }
  virtual DisplayError SetABCState(bool state) { return kErrorNotSupported; }
  virtual DisplayError SetABCReconfig() { return kErrorNotSupported; }
  virtual DisplayError SetABCMode(const string &mode_name) { return kErrorNotSupported; }
  virtual void RefreshOnIdleTimeoutForCwb(bool is_cwb_requested);
  virtual void ResetDispLayerStack();
  virtual bool HasNoiseLayer();
  virtual bool HasConcurrentWriteback();
  virtual bool HasSrcTonemap();
  virtual DisplayError PerformCacConfig(CacConfig config, bool enable) {
    return kErrorNotSupported;
  }
  virtual bool IsCacV2Supported() { return false; }
  virtual DisplayError
  PanelOprInfo(const std::string &client_name, bool enable,
               SdmDisplayCbInterface<PanelOprPayload> *cb_intf) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetPaHistCollection(
      const std::string &client_name, bool enable,
      SdmDisplayCbInterface<PaHistCollectionPayload> *cb_intf) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetPaHistBins(std::array<uint32_t, HIST_BIN_SIZE> *buf) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetSsrcMode(const std::string &mode) { return kErrorNotSupported; }
  virtual DisplayError SetVRRState(bool state) { return kErrorNotSupported; }
  virtual DisplayError NotifyExpectedPresent(uint64_t expected_present_time,
                                             uint32_t frame_interval_ns);
  virtual DisplayError SetPanelFeatureConfig(int32_t type, void *data) {
    return kErrorNotSupported;
  }

  virtual DisplayError PanelBacklightInfo(const std::string &client_name, bool enable,
                                          SdmDisplayCbInterface<PanelBacklightPayload> *cb_intf) {
    return kErrorNotSupported;
  }
  virtual DisplayError EnableCopr(bool en) { return kErrorNotSupported; }
  virtual DisplayError GetCoprStats(std::vector<int> *stats) { return kErrorNotSupported; }

 protected:
  struct DisplayMutex {
    std::recursive_mutex client_mutex;
    std::condition_variable_any client_cv;
    std::recursive_mutex worker_mutex;
    std::condition_variable_any worker_cv;
    bool worker_busy = false;
    bool worker_exit = false;
  };

  class ClientLock {
   public:
    explicit ClientLock(DisplayMutex &disp_mutex) : disp_mutex_(disp_mutex) {
      disp_mutex_.client_mutex.lock();
      disp_mutex_.worker_mutex.lock();
      while (disp_mutex_.worker_busy) {
        disp_mutex_.worker_cv.wait(disp_mutex_.worker_mutex);
      }
    }

    ~ClientLock() {
      disp_mutex_.worker_mutex.unlock();
      disp_mutex_.client_mutex.unlock();
    }

    void NotifyWorker() {
      disp_mutex_.worker_busy = true;
      disp_mutex_.worker_cv.notify_one();
    }

   private:
    DisplayMutex &disp_mutex_;
  };

  const char *kBt2020Pq = "bt2020_pq";
  const char *kBt2020Hlg = "bt2020_hlg";
  const char *kDisplayBt2020 = "display_bt2020";
  virtual DisplayError BuildLayerStackStats(LayerStack *layer_stack);
  virtual DisplayError ValidateGPUTargetParams();
  void CommitLayerParams(LayerStack *layer_stack);
  void PostCommitLayerParams();
  DisplayError ValidateScaling(uint32_t width, uint32_t height);
  DisplayError ValidateDataspace(const Dataspace &color_metadata);
  void HwRecovery(const HWRecoveryEvent sdm_event_code);

  const char *GetName(const LayerComposition &composition);
  bool NeedsMixerReconfiguration(LayerStack *layer_stack, uint32_t *new_mixer_width,
                                 uint32_t *new_mixer_height);
  DisplayError ReconfigureMixer(uint32_t width, uint32_t height);
  bool NeedsDownScale(const LayerRect &src_rect, const LayerRect &dst_rect, bool needs_rotation);
  void DeInitializeColorModes();
  DisplayError SetColorModeInternal(const std::string &color_mode,
                                    const std::string &str_render_intent,
                                    const PrimariesTransfer &pt);
  DisplayError GetValueOfModeAttribute(const AttrVal &attr, const std::string &type,
                                       std::string *value);
  bool IsSupportColorModeAttribute(const std::string &color_mode);
  void ClearColorInfo();
  void GetColorPrimaryTransferFromAttributes(const AttrVal &attr,
      std::vector<PrimariesTransfer> *supported_pt);
  bool DisplayPowerResetPending();
  bool SetHdrModeAtStart(LayerStack *layer_stack);
  PrimariesTransfer GetBlendSpaceFromColorMode();
  bool IsHdrMode(const AttrVal &attr);
  void InsertBT2020PqHlgModes(const std::string &str_render_intent);
  DisplayError InitRC();
  bool EnableRC();
  DisplayError HandlePendingVSyncEnable(const shared_ptr<Fence> &retire_fence);
  DisplayError ResetPendingPowerState(const shared_ptr<Fence> &retire_fence);
  DisplayError GetPendingDisplayState(DisplayState *disp_state);
  void SetPendingPowerState(DisplayState state);
  DisplayError SetupPanelFeatureFactory();
  void CommitThread();
  virtual void HandleAsyncCommit();
  void MMRMEvent(uint32_t clk);
  void CheckMMRMState();
  DisplayError SetVSyncStateLocked(bool enable);
  virtual DisplayError SetUpCommit(LayerStack *layer_stack);
  DisplayError PerformCommit(std::map<uint32_t, HWLayersInfo> &hw_layers_info);
  virtual DisplayError PostCommit();
  bool IsPrimaryDisplayLocked();
  virtual DisplayError CommitLocked(LayerStack *layer_stack);
  void ConfigureCwbParams(LayerStack *layer_stack);
  void ProcessPowerEvent();
  DisplayError SetHWDetailedEnhancerConfig(void *params);
  DisplayError NoiseInit();
  DisplayError HandleNoiseLayer(LayerStack *layer_stack);
  void PrepareForAsyncTransition();
  virtual void IdleTimeout() {}
  std::chrono::system_clock::time_point WaitUntil();
  virtual void Abort();
  DisplayError DisableDestinationScalar();

  DisplayMutex disp_mutex_;
  std::thread commit_thread_;
  DisplayId display_id_info_ = {};
  int32_t display_id_ = -1;
  SDMDisplayType display_type_;
  DisplayEventHandler *event_handler_ = NULL;
  HWDeviceType hw_device_type_;
  DPUCoreMux *dpu_core_mux_ = NULL;
  HWInterface *hw_intf_ = NULL;
  std::vector<HWResourceInfo> hw_resource_info_;
  BufferAllocator *buffer_allocator_ {};
  CompManager *comp_manager_ = NULL;
  DisplayState state_ = kStateOff;
  bool active_ = false;
  Handle hw_device_ = 0;
  Handle display_comp_ctx_ = 0;
  DispLayerStack *disp_layer_stack_;
  static constexpr size_t kDispStackCount = 2;
  DispLayerStack disp_layer_stacks_[kDispStackCount];
  std::atomic<bool> clearstack_;
  uint8_t disp_stack_index_ = 0;
  bool needs_validate_ = true;  // maintains validation state between Prepare/Commit Cycle
  bool vsync_enable_ = false;
  uint32_t max_mixer_stages_ = 0;
  sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf_;
  std::bitset<32> core_id_;
  uint32_t primary_core_id_ = 0;
  int core_count_ = 0;
  ColorManagerIntf *color_mgr_ = NULL;
  bool partial_update_control_ = true;
  HWEventsInterface *hw_events_intf_ = NULL;
  bool disable_pu_one_frame_ = false;
  bool pu_pending_ = false;
  uint32_t num_color_modes_ = 0;
  std::vector<SDEDisplayMode> color_modes_;
  typedef std::map<std::string, SDEDisplayMode *> ColorModeMap;
  ColorModeMap color_mode_map_ = {};
  typedef std::map<std::string, AttrVal> ColorModeAttrMap;
  ColorModeAttrMap color_mode_attr_map_ = {};
  std::vector<PrimariesTransfer> color_modes_cs_ = {};  // Gamut+Gamma(color space) of color mode
  uint32_t req_mixer_width_ = 0;
  uint32_t req_mixer_height_ = 0;
  std::string current_color_mode_ = "hal_native";
  bool hw_recovery_logs_captured_ = false;
  int disable_hw_recovery_dump_ = 0;
  uint32_t hw_recovery_count_ = 0;
  uint32_t hw_recovery_threshold_ = 1;
  std::map<uint32_t, HWQosData> cached_qos_data_;
  std::map<uint32_t, uint32_t> default_clock_hz_;
  std::map<uint32_t, LayerBuffer> cached_framebuffer_ = {};
  bool drop_hw_vsync_ = false;
  uint32_t current_refresh_rate_ = 0;
  bool drop_skewed_vsync_ = false;
  bool disable_llcc_during_aod_ = false;
  bool custom_mixer_resolution_ = false;
  bool vsync_enable_pending_ = false;
  HWPowerState pending_power_state_ = kPowerStateNone;
  QSyncMode qsync_mode_ = kQSyncModeNone;
  std::bitset<kUpdateAVRFlagMax> needs_avr_update_ = {};
  bool force_lm_to_fb_config_ = false;

  static Locker display_power_reset_lock_;
  static bool display_power_reset_pending_;
  static int32_t mmrm_floor_clk_vote_;
  SecureEvent secure_event_ = kSecureEventMax;
  bool rc_panel_feature_init_ = false;
  bool spr_enable_ = false;
  bool spr_bypassed_ = false;
  bool rc_enable_prop_ = false;
  bool rc_config_enable_ = false;  // Specifies if RC is enabled by RCCore
  RCLayersInfo rc_info_ = {};  // when rc_config_enable_ is true, this holds RC top/bottom info
  DynLib abc_feature_impl_lib_;
  aiqe::ABCFeatureFactIntf *abc_factory_ = nullptr;
  PanelFeatureFactoryIntf *pf_factory_ = nullptr;
  PanelFeaturePropertyIntf *prop_intf_ = nullptr;
  DemuraTnCoreUvmFactoryIntf *demuratn_factory_ = nullptr;
  FeatureLicenseFactoryIntf *feature_license_factory_ = nullptr;
  bool first_cycle_ = true;
  bool registered_hw_events_ = false;
  bool unified_draw_supported_ = true;  // By default supported, unless disabled by property.
  bool validated_ = false;  // display validation status based on sideband events driver events etc.
  shared_ptr<Fence> retire_fence_ = nullptr;
  DisplayDrawMethod draw_method_ = kDrawDefault;
  bool noise_disable_prop_ = false;
  NoiseLayerConfig noise_layer_info_ = {};
  std::unique_ptr<NoisePlugInIntf> noise_plugin_intf_ = nullptr;
  NoisePlugInFactoryIntf *noise_plugin_factory_intf_ = nullptr;
  bool noise_plugin_override_en_ = false;
  int32_t noise_override_zpos_ = -1;  // holds the overriden zpos/idx, used to mark sde_preferred
  bool handle_idle_timeout_ = false;
  bool pending_commit_ = false;
  uint32_t active_refresh_rate_ = 0;
  bool disable_cwb_idle_fallback_ = false;
  bool idle_fallback_on_dspp_ = false;
  bool allow_tonemap_native_ = false;
  bool avoid_qsync_mode_change_ = false;
  LayerBuffer cwb_output_buf_ = {};
  bool cwb_active_ = false;
  DisplayClientContext client_ctx_ = {};
  DisplayDeviceContext device_ctx_;
  static std::atomic<uint32_t> hw_rc_blocks_in_use_;
  uint32_t rc_blocks_reserved_ = 0;
  DynLib extension_lib_;
  bool ssrc_feature_enabled_ = false;
  bool xr_variant_ = false;
  BufferInfo dummy_loopback_cac_info_ = {};

 private:
  // Max tolerable power-state-change wait-times in milliseconds.
  static const int kPowerStateTimeout = 5000;

  void StartCommitThread();
  bool StartDisplayPowerReset();
  void EndDisplayPowerReset();
  DisplayError PrepareRC(LayerStack *layer_stack);
  DisplayError ValidateCwbConfigInfo(CwbConfig *cwb_config, const LayerBufferFormat &format);
  bool IsValidCwbRoi(const LayerRect &cwb_roi, const LayerRect &full_frame);
  DisplayError GetNoisePluginParams(LayerStack *layer_stack);
  DisplayError InsertNoiseLayer(LayerStack *layer_stack);
  void WaitForCompletion(SyncPoints *sync_points);
  DisplayError PerformHwCommit(std::map<uint32_t, HWLayersInfo> &hw_layers_info);
  void CacheRetireFence();
  void CacheFrameBuffer();
  void CacheDisplayComposition();
  void UpdateFrameBuffer();
  void CleanupOnError();
  bool IsValidateNeeded();
  DisplayError InitBorderLayers();
  std::vector<LayerRect> GetBorderRects();
  void GenerateBorderLayers(const std::vector<LayerRect> &border_rects);
  uint32_t GetMixerCountFromTopology(HWTopology topology);
  unsigned int rc_cached_res_width_ = 0;
  unsigned int rc_cached_res_height_ = 0;
  unsigned int rc_cached_mixer_width_ = 0;
  unsigned int rc_cached_mixer_height_ = 0;
  unsigned int rc_cached_fb_width_ = 0;
  unsigned int rc_cached_fb_height_ = 0;
  std::unique_ptr<RCIntf> rc_core_ = nullptr;
  bool rc_prepared_ = false;  // Used to avoid calling into RC core b/w Prepare and PrePrepare
  bool mmrm_updated_ = false;
  uint32_t mmrm_requested_clk_ = 0;
  static bool primary_active_;
  bool draw_method_set_ = false;
  bool cwb_configured_ = false;
  bool transition_done_ = false;
  bool gpu_comp_frame_ = false;
  uint32_t retire_fence_offset_ = 0;
  std::mutex power_mutex_;
  std::condition_variable cv_;
  Layer noise_layer_ = {};
  DisplayError ConfigureCwbForIdleFallback(LayerStack *layer_stack);
  bool cwb_fence_wait_ = false;
  bool enable_cwb_cpu_boosting_ = false;
  bool force_refresh_to_process_cwb_ = false;
  bool enable_client_control_cwb_refresh_ = false;
  std::vector<Layer> border_layers_;
  bool windowed_display_ = false;
  LayerRect window_rect_ = {};
  bool enable_win_rect_mask_ = false;
  HWDisplayMode default_panel_mode_ = kModeDefault;
  bool idle_hint_set_ = false;
  uint32_t idle_active_ms_ = 0;
  int32_t mirror_src_display_id_ = -1;
  bool needs_mirror_source_validation_ = false;
  bool enable_ai_scaler_ = false;
};

}  // namespace sdm

#endif  // __DISPLAY_BASE_H__
