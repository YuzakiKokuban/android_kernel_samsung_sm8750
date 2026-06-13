/* Copyright (c) 2015-2021 The Linux Foundation. All rights reserved.
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
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __COLOR_MANAGER_H__
#define __COLOR_MANAGER_H__

#include <stdlib.h>
#include <core/sdm_types.h>
#include <utils/locker.h>
#include <private/color_interface.h>
#include <private/snapdragon_color_intf.h>
#include <utils/sys.h>
#include <utils/debug.h>
#include <private/hw_interface.h>
#include <array>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <color_metadata.h>
#include <utils/formats.h>

#define COLOR_TRANSFORM_IDENTITY {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}

#include "dpu_core_mux.h"

#define BIT(x) (1 << x)

namespace sdm {

using snapdragoncolor::ColorMode;
using snapdragoncolor::ColorModeList;
using snapdragoncolor::GammaPostBlendConfig;
using snapdragoncolor::GamutConfig;
using snapdragoncolor::HwConfigOutputParams;
using snapdragoncolor::HwConfigPayload;
using snapdragoncolor::kHwConfigPayloadParam;
using snapdragoncolor::kModeList;
using snapdragoncolor::kModeRenderInputParams;
using snapdragoncolor::kNeedsUpdate;
using snapdragoncolor::kNotifyDisplayCalibrationMode;
using snapdragoncolor::kPbGamut;
using snapdragoncolor::kPbGC;
using snapdragoncolor::kPbIgc;
using snapdragoncolor::kPostBlendGammaHwConfig;
using snapdragoncolor::kPostBlendGamutHwConfig;
using snapdragoncolor::kPostBlendInverseGammaHwConfig;
using snapdragoncolor::kScModeRenderIntent;
using snapdragoncolor::kScModeSwAssets;
using snapdragoncolor::kSupportToneMap;
using snapdragoncolor::ModeRenderInputParams;
using snapdragoncolor::PostBlendGammaHwConfig;
using snapdragoncolor::PostBlendGamutHwConfig;
using snapdragoncolor::PostBlendInverseGammaHwConfig;
using snapdragoncolor::ScOps;
using snapdragoncolor::ScPayload;
using snapdragoncolor::ScPostBlendInterface;
using std::lock_guard;
using std::mutex;

enum FeatureOps {
  kFeatureSwitchMode,
  kFeatureOpsMax,
};

enum ControlOps {
  kControlNonPostedStart,
  kControlWithPostedStartDynSwitch,
  kControlPostedStart,
  kControlOpsMax,
};

class FeatureInterface {
 public:
  virtual ~FeatureInterface() {}
  virtual DisplayError Init() = 0;
  virtual DisplayError Deinit() = 0;
  virtual DisplayError SetParams(FeatureOps param_type, void *payload) = 0;
  virtual DisplayError GetParams(FeatureOps param_type, void *payload) = 0;
};

FeatureInterface* GetPostedStartFeatureCheckIntf(DPUCoreMux *dpu_core_mux,
                                                 PPFeaturesConfig *config, bool dyn_switch,
                                                 uint32_t core_id);

/*
 * ColorManager Intf
 * ColorManagerProxy and DPUColorManager extends Intf
 */
class ColorManagerIntf {
 public:
  virtual ~ColorManagerIntf() {}

  virtual DisplayError ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                    PPDisplayAPIPayload *out_payload,
                                    PPPendingParams *pending_action) = 0;
  virtual DisplayError ColorMgrGetNumOfModes(uint32_t *mode_cnt) = 0;
  virtual DisplayError ColorMgrGetModes(uint32_t *mode_cnt, SDEDisplayMode *modes) = 0;
  virtual DisplayError ColorMgrSetMode(int32_t color_mode_id) = 0;
  virtual void SetDETuningCFGpending(bool cfg_pending) = 0;
  virtual DisplayError ColorMgrGetModeInfo(int32_t mode_id, AttrVal *query) = 0;
  virtual DisplayError ColorMgrSetColorTransform(uint32_t length,
                                                  const double *trans_data) = 0;
  virtual bool NeedsPartialUpdateDisable() = 0;
  virtual DisplayError Commit() = 0;
  virtual DisplayError ColorMgrSetModeWithRenderIntent(int32_t color_mode_id,
                                               const PrimariesTransfer &blend_space,
                                               uint32_t intent) = 0;
  virtual DisplayError Validate(DispLayerStack *disp_layer_stack) = 0;
  virtual bool GameEnhanceSupported() = 0;
  virtual DisplayError ColorMgrGetStcModes(ColorModeList *mode_list) = 0;
  virtual DisplayError ColorMgrSetStcMode(const ColorMode &color_mode) = 0;
  virtual DisplayError Prepare() = 0;
  virtual bool IsValidateNeeded() = 0;

  virtual DisplayError ConfigureCWBDither(CwbConfig *cwb_cfg, bool free_data) = 0;
  virtual DisplayError NotifyDisplayCalibrationMode(bool in_calibration) = 0;
  virtual DisplayError ColorMgrSetLtmPccConfig(void* pcc_input, size_t size) = 0;
  virtual DisplayError ColorMgrSetSprIntf(std::shared_ptr<SPRIntf> spr_intf) = 0;
  virtual DisplayError ColorMgrIdleFallback(bool idle_fallback_hint) = 0;

  // TBD: Should remove these legacy API's?
  virtual DisplayError ApplyDefaultDisplayMode() = 0;
  virtual DisplayError ColorMgrGetDefaultModeID(int32_t *mode_id) = 0;
  virtual DisplayError ColorMgrCombineColorModes() = 0;
};

/*
 * ColorManager proxy to maintain necessary information to interact with underlying color service.
 * Each display object has its own proxy.
 */
class ColorManagerProxy : public ColorManagerIntf {
 public:
  static DisplayError Init();
  static void Deinit();

  /* Create ColorManagerProxy for this display object, following things need to be happening
   * 1. Instantiates concrete ColorInerface implementation.
   * 2. Pass all display object specific informations into it.
   * 3. Populate necessary resources.
   * 4. Need get panel name for hw_panel_info_.
   */
  static ColorManagerProxy *CreateColorManagerProxy(SDMDisplayType type,
                                                    DPUCoreMux *dpu_core_mux,
                                                    const HWDisplayAttributes &attribute,
                                                    const HWPanelInfo &panel_info,
                                                    DppsControlInterface *dpps_intf,
                                                    DisplayInterface *disp_intf,
                                                    const HWResourceInfo &hw_res_info,
                                                    uint32_t display_id);

  /* need reverse the effect of CreateColorManagerProxy. */
  ~ColorManagerProxy();

  static int getCoreId(uint32_t display_id);

  DisplayError ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                    PPDisplayAPIPayload *out_payload,
                                    PPPendingParams *pending_action);
  DisplayError ApplyDefaultDisplayMode();
  DisplayError ColorMgrGetNumOfModes(uint32_t *mode_cnt);
  DisplayError ColorMgrGetModes(uint32_t *mode_cnt, SDEDisplayMode *modes);
  DisplayError ColorMgrSetMode(int32_t color_mode_id);
  void SetDETuningCFGpending(bool cfg_pending);
  DisplayError ColorMgrGetModeInfo(int32_t mode_id, AttrVal *query);
  DisplayError ColorMgrSetColorTransform(uint32_t length, const double *trans_data);
  DisplayError ColorMgrGetDefaultModeID(int32_t *mode_id);
  DisplayError ColorMgrCombineColorModes();
  bool NeedsPartialUpdateDisable();
  DisplayError Commit();
  DisplayError ColorMgrSetModeWithRenderIntent(int32_t color_mode_id,
                                               const PrimariesTransfer &blend_space,
                                               uint32_t intent);
  DisplayError Validate(DispLayerStack *disp_layer_stack);
  bool GameEnhanceSupported();
  DisplayError ColorMgrGetStcModes(ColorModeList *mode_list);
  DisplayError ColorMgrSetStcMode(const ColorMode &color_mode);
  DisplayError Prepare();
  bool IsValidateNeeded();

  /* ConfigureCWBDither can get/release dither setting base on bool variable free_data
   * if free_data is false to get dither setting needs to be applied.
   * if free_data is true to release the dither setting that has been applied.
   */
  DisplayError ConfigureCWBDither(CwbConfig *cwb_cfg, bool free_data);
  DisplayError NotifyDisplayCalibrationMode(bool in_calibration);
  DisplayError ColorMgrSetLtmPccConfig(void* pcc_input, size_t size);
  DisplayError ColorMgrSetSprIntf(std::shared_ptr<SPRIntf> spr_intf);
  DisplayError ColorMgrIdleFallback(bool idle_fallback_hint);

 protected:
  ColorManagerProxy() {}
  ColorManagerProxy(int32_t id, SDMDisplayType type, DPUCoreMux *dpu_core_mux,
                    const HWDisplayAttributes &attr, const HWPanelInfo &info,
                    const uint32_t &core_id);

 private:
  static DynLib color_lib_;
  static DynLib stc_lib_;
  static CreateColorInterface create_intf_;
  static DestroyColorInterface destroy_intf_;
  static GetScPostBlendInterface create_stc_intf_;
  HWResourceInfo hw_res_info_;

  typedef DisplayError (ColorManagerProxy::*ConvertProc)(const HwConfigPayload &in_data,
                                        PPFeaturesConfig *out_data);
  typedef std::map<std::string, ConvertProc> ConvertTable;

  bool NeedAssetsUpdate();
  DisplayError UpdateModeHwassets(int32_t mode_id, snapdragoncolor::ColorMode color_mode,
                                  bool valid_meta_data, const ColorMetaData &meta_data);
  DisplayError ConvertToPPFeatures(const HwConfigOutputParams &params, PPFeaturesConfig *out_data);
  void DumpColorMetaData(const ColorMetaData &color_metadata);
  bool HasNativeModeSupport();
  DisplayError ApplySwAssets();

  uint32_t display_id_;
  SDMDisplayType device_type_;
  PPHWAttributes pp_hw_attributes_;
  DPUCoreMux *dpu_core_mux_;
  ColorInterface *color_intf_;
  PPFeaturesConfig pp_features_;
  FeatureInterface *feature_intf_;
  bool apply_mode_ = false;
  PrimariesTransfer cur_blend_space_ = {};
  uint32_t cur_intent_ = 0;
  int32_t cur_mode_id_ = -1;
  ColorMetaData meta_data_ = {};
  snapdragoncolor::ScPostBlendInterface *stc_intf_ = NULL;
  snapdragoncolor::ColorMode curr_mode_;
  bool needs_update_ = false;
  uint32_t core_id_;
  bool has_native_mode_ = false;
  bool prev_idle_fallback_hint_ = false;
  ColorMode prev_idle_fallback_mode_ = {};
  struct snapdragoncolor::ColorTransform curr_color_xform_ = {};
};

class ColorFeatureCheckingImpl : public FeatureInterface {
 public:
  explicit ColorFeatureCheckingImpl(DPUCoreMux *dpu_core_mux, PPFeaturesConfig *pp_features,
    bool dyn_switch, uint32_t core_id);
  virtual ~ColorFeatureCheckingImpl() { }

  DisplayError Init();
  DisplayError Deinit();
  DisplayError SetParams(FeatureOps param_type, void *payload);
  DisplayError GetParams(FeatureOps param_type, void *payload);

 private:
  friend class FeatureStatePostedStart;
  friend class FeatureStateDefaultTrigger;
  friend class FeatureStateSerializedTrigger;

  DPUCoreMux *dpu_core_mux_;
  PPFeaturesConfig *pp_features_;
  std::array<FeatureInterface*, kFrameTriggerMax> states_ = {{NULL}};
  FeatureInterface *curr_state_ = NULL;
  std::vector<PPGlobalColorFeatureID> single_buffer_feature_;
  void CheckColorFeature(FrameTriggerMode *mode);
  bool dyn_switch_ = false;
  uint32_t core_id_ = 0;
};

class FeatureStatePostedStart : public FeatureInterface {
 public:
  explicit FeatureStatePostedStart(ColorFeatureCheckingImpl *obj);
  virtual ~FeatureStatePostedStart() {}

  DisplayError Init();
  DisplayError Deinit();
  DisplayError SetParams(FeatureOps param_type, void *payload);
  DisplayError GetParams(FeatureOps param_type, void *payload);

 private:
  ColorFeatureCheckingImpl *obj_;
};

class FeatureStateDefaultTrigger : public FeatureInterface {
 public:
  explicit FeatureStateDefaultTrigger(ColorFeatureCheckingImpl *obj);
  virtual ~FeatureStateDefaultTrigger() {}

  DisplayError Init();
  DisplayError Deinit();
  DisplayError SetParams(FeatureOps param_type, void *payload);
  DisplayError GetParams(FeatureOps param_type, void *payload);

 private:
  ColorFeatureCheckingImpl *obj_;
};

class FeatureStateSerializedTrigger : public FeatureInterface {
 public:
  explicit FeatureStateSerializedTrigger(ColorFeatureCheckingImpl *obj);
  virtual ~FeatureStateSerializedTrigger() {}

  DisplayError Init();
  DisplayError Deinit();
  DisplayError SetParams(FeatureOps param_type, void *payload);
  DisplayError GetParams(FeatureOps param_type, void *payload);

 private:
  ColorFeatureCheckingImpl *obj_;
};

class DPUColorManager : public ColorManagerIntf {
 public:
  static DisplayError Init(const std::vector<HWResourceInfo> &hw_res_info);
  static DPUColorManager *CreateDpuColorManager(SDMDisplayType type,
                                                  DPUCoreMux *dpu_core_mux,
                                                  DisplayDeviceContext &display_device_ctx,
                                                  DisplayClientContext &display_client_ctx,
                                                  DppsControlInterface *dpps_intf,
                                                  DisplayInterface *disp_intf,
                                                  vector<HWResourceInfo> &hw_res_info,
                                                  DisplayId display_id_info);

  void Deinit();

  ~DPUColorManager();

  DisplayError ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                    PPDisplayAPIPayload *out_payload,
                                    PPPendingParams *pending_action);
  DisplayError ColorMgrGetNumOfModes(uint32_t *mode_cnt);
  DisplayError ColorMgrGetModes(uint32_t *mode_cnt, SDEDisplayMode *modes);
  DisplayError ColorMgrSetMode(int32_t color_mode_id);
  DisplayError ColorMgrGetModeInfo(int32_t mode_id, AttrVal *query);
  DisplayError ColorMgrSetColorTransform(uint32_t length, const double *trans_data);
  DisplayError Commit();
  DisplayError ColorMgrSetModeWithRenderIntent(int32_t color_mode_id,
                                               const PrimariesTransfer &blend_space,
                                               uint32_t intent);
  DisplayError Validate(DispLayerStack *disp_layer_stack);
  DisplayError CompareColorModeList(std::vector<ColorModeList> &mode_list);
  DisplayError CompareAttrVal(vector<AttrVal> &query);

  DisplayError ColorMgrGetStcModes(ColorModeList *mode_list);
  DisplayError ColorMgrSetStcMode(const ColorMode &color_mode);
  DisplayError Prepare();
  DisplayError ConfigureCWBDither(CwbConfig *cwb_cfg, bool free_data);
  DisplayError NotifyDisplayCalibrationMode(bool in_calibration);
  DisplayError ColorMgrSetLtmPccConfig(void* pcc_input, size_t size);
  DisplayError ColorMgrSetSprIntf(std::shared_ptr<SPRIntf> spr_intf);
  DisplayError ColorMgrIdleFallback(bool idle_fallback_hint);

  // TBD: Should remove these legacy API's?
  DisplayError ApplyDefaultDisplayMode();
  DisplayError ColorMgrGetDefaultModeID(int32_t *mode_id);
  DisplayError ColorMgrCombineColorModes();

  void SetDETuningCFGpending(bool cfg_pending);

  bool NeedsPartialUpdateDisable();
  bool GameEnhanceSupported();
  bool IsValidateNeeded();
  bool ComparePendingAction(vector<PPPendingParams> &pending_action);
  bool CompareSDEDisplayModes(vector<SDEDisplayMode> &mode);

 protected:
  DPUColorManager() {}
  explicit DPUColorManager(DisplayId id_info);

 private:
  int CreatePhysicalDisplayIds(DisplayId display_id_info);

  DisplayId display_id_info_;
  vector<HWResourceInfo> hw_res_info_;
  vector<ColorManagerProxy *> color_manager_proxy_list_;
  vector<uint32_t> display_id_list_;
};

class ColorMgrFactoryIntf {
 public:
  virtual ColorManagerIntf* CreateColorManagerIntf(SDMDisplayType type,
                                                    DPUCoreMux *dpu_core_mux,
                                                    DisplayDeviceContext &display_device_ctx,
                                                    DisplayClientContext &display_client_ctx,
                                                    DppsControlInterface *dpps_intf,
                                                    DisplayInterface *disp_intf,
                                                    vector<HWResourceInfo> &hw_res_info,
                                                    DisplayId display_id_info) = 0;
  virtual ~ColorMgrFactoryIntf() { }
};

extern "C" ColorMgrFactoryIntf* GetColorMgrFactoryIntf();

class ColorMgrFactoryIntfImpl : public ColorMgrFactoryIntf {
 public:
  virtual ColorManagerIntf* CreateColorManagerIntf(SDMDisplayType type,
                                                    DPUCoreMux *dpu_core_mux,
                                                    DisplayDeviceContext &display_device_ctx,
                                                    DisplayClientContext &display_client_ctx,
                                                    DppsControlInterface *dpps_intf,
                                                    DisplayInterface *disp_intf,
                                                    vector<HWResourceInfo> &hw_res_info,
                                                    DisplayId display_id_info);
  virtual ~ColorMgrFactoryIntfImpl() { }
};

}  // namespace sdm

#endif  // __COLOR_MANAGER_H__
