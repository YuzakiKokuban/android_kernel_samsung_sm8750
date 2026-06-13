/* Copyright (c) 2015 - 2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <dlfcn.h>
#include <private/color_interface.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <algorithm>
#include <vector>
#include <string>
#include <set>
#include <map>

#include "color_manager.h"

#define __CLASS__ "ColorManager"

namespace sdm {

DynLib ColorManagerProxy::color_lib_;
DynLib ColorManagerProxy::stc_lib_;
CreateColorInterface ColorManagerProxy::create_intf_ = NULL;
DestroyColorInterface ColorManagerProxy::destroy_intf_ = NULL;

GetScPostBlendInterface ColorManagerProxy::create_stc_intf_ = NULL;

bool NeedsToneMap(const std::vector<Layer> &layers) {
  for (auto &layer : layers) {
    if (layer.request.flags.dest_tone_map) {
      return true;
    }
  }
  return false;
}

// Below two functions are part of concrete implementation for SDM core private
// color_params.h
void PPFeaturesConfig::Reset() {
  for (int i = 0; i < kMaxNumPPFeatures; i++) {
    if (feature_[i]) {
      delete feature_[i];
      feature_[i] = NULL;
    }
  }
  dirty_ = false;
  next_idx_ = 0;
}

DisplayError PPFeaturesConfig::RetrieveNextFeature(PPFeatureInfo **feature) {
  DisplayError ret = kErrorNone;
  uint32_t i(0);

  for (i = next_idx_; i < kMaxNumPPFeatures; i++) {
    if (feature_[i]) {
      *feature = feature_[i];
      next_idx_ = i + 1;
      break;
    }
  }

  if (i == kMaxNumPPFeatures) {
    ret = kErrorParameters;
    next_idx_ = 0;
  }

  return ret;
}

// TBD: handle Posted start for command mode
FeatureInterface* GetPostedStartFeatureCheckIntf(DPUCoreMux *dpu_core_mux,
                                                  PPFeaturesConfig *config,
                                                  bool dyn_switch,
                                                  uint32_t core_id) {
  return new ColorFeatureCheckingImpl(dpu_core_mux, config, dyn_switch, core_id);
}

DisplayError ColorManagerProxy::Init() {
  DisplayError error = kErrorNone;

  // Load color service library and retrieve its entry points.
  if (!color_lib_) {
    if (color_lib_.Open(COLORMGR_LIBRARY_NAME)) {
      if (!color_lib_.Sym(CREATE_COLOR_INTERFACE_NAME, reinterpret_cast<void **>(&create_intf_)) ||
          !color_lib_.Sym(DESTROY_COLOR_INTERFACE_NAME,
                                              reinterpret_cast<void **>(&destroy_intf_))) {
        DLOGW("Fail to retrieve = %s from %s", CREATE_COLOR_INTERFACE_NAME, COLORMGR_LIBRARY_NAME);
        error = kErrorResources;
      }
    } else {
      DLOGW("Fail to load = %s", COLORMGR_LIBRARY_NAME);
      error = kErrorResources;
    }
  }

  // Load Stc manager library and retrieve its entry points.
  if (!stc_lib_) {
    if (stc_lib_.Open(STCMGR_LIBRARY_NAME)) {
      if (!stc_lib_.Sym(CREATE_STC_INTERFACE_NAME, reinterpret_cast<void **>(&create_stc_intf_))) {
        DLOGW("Fail to retrieve = %s from %s", CREATE_STC_INTERFACE_NAME, STCMGR_LIBRARY_NAME);
        error = kErrorResources;
      }
    } else {
      DLOGW("Fail to load = %s", STCMGR_LIBRARY_NAME);
      error = kErrorResources;
    }
  }

  return error;
}

void ColorManagerProxy::Deinit() {
  if (color_lib_) {
    color_lib_.~DynLib();
  }

  if (stc_lib_) {
    stc_lib_.~DynLib();
  }
}

ColorManagerProxy::ColorManagerProxy(int32_t id, SDMDisplayType type, DPUCoreMux *dpu_core_mux,
                                     const HWDisplayAttributes &attr,
                                     const HWPanelInfo &info, const uint32_t &core_id)
    : display_id_(id), device_type_(type), pp_hw_attributes_(), dpu_core_mux_(dpu_core_mux),
      color_intf_(NULL), pp_features_(), feature_intf_(NULL), core_id_(core_id) {
  int32_t enable_posted_start_dyn = 0;
  bool dyn_switch = false;

  // Initialize PCC with identity
  curr_color_xform_.coeff_array = COLOR_TRANSFORM_IDENTITY;

  Debug::Get()->GetProperty(ENABLE_POSTED_START_DYN_PROP, &enable_posted_start_dyn);
  if (info.mode == kModeCommand) {
    switch (enable_posted_start_dyn) {
    case kControlWithPostedStartDynSwitch:
      dyn_switch = true;
      [[fallthrough]];
    case kControlPostedStart:
      feature_intf_ = GetPostedStartFeatureCheckIntf(dpu_core_mux, &pp_features_, dyn_switch,
                                                                                  core_id_);
      if (!feature_intf_) {
        DLOGI("Failed to create feature interface");
      } else {
        DisplayError err = feature_intf_->Init();
        if (err) {
          DLOGE("Failed to init feature interface");
          delete feature_intf_;
          feature_intf_ = NULL;
        }
      }
      break;
    default:
      break;
    }
  }
}

ColorManagerProxy *ColorManagerProxy::CreateColorManagerProxy(SDMDisplayType type,
                                                              DPUCoreMux *dpu_core_mux,
                                                              const HWDisplayAttributes &attribute,
                                                              const HWPanelInfo &panel_info,
                                                              DppsControlInterface *dpps_intf,
                                                              DisplayInterface *disp_intf,
                                                              const HWResourceInfo &hw_res_info,
                                                              uint32_t display_id) {
  DisplayError error = kErrorNone;
  PPFeatureVersion versions;
  ColorManagerProxy *color_manager_proxy = NULL;
  uint32_t core_id = 0;
  bool allow_tonemap_native = 0;
  int32_t prop = 0;

  if (Debug::Get()->GetProperty(ALLOW_TONEMAP_NATIVE, &prop) == kErrorNone) {
    allow_tonemap_native = (prop == 1);
  }

  // check if all resources are available before invoking factory method from libsdm-color.so.
  if (!color_lib_ || !create_intf_ || !destroy_intf_) {
    DLOGW("Information for %s isn't available!", COLORMGR_LIBRARY_NAME);
    return NULL;
  }

  // check if all resources are available before invoking libsnapdragoncolor-manager.so.
  if (!stc_lib_ || !create_stc_intf_) {
    DLOGW("Information for %s isn't available!", STCMGR_LIBRARY_NAME);
    return NULL;
  }

  core_id = ColorManagerProxy::getCoreId(display_id);
  color_manager_proxy = new ColorManagerProxy(display_id, type, dpu_core_mux,
                                                  attribute, panel_info, core_id);

  if (color_manager_proxy) {
    color_manager_proxy->hw_res_info_ = hw_res_info;
    // 1. need query post-processing feature version from HWInterface.
    error = color_manager_proxy->dpu_core_mux_->GetPPFeaturesVersion(&versions, core_id);
    PPHWAttributes &hw_attr = color_manager_proxy->pp_hw_attributes_;
    if (error != kErrorNone) {
      DLOGW("Fail to get DSPP feature versions");
    } else {
      hw_attr.Set(color_manager_proxy->hw_res_info_, panel_info, attribute, versions, dpps_intf);
      DLOGI("PAV2 version is versions = %d, version = %d ",
            hw_attr.version.version[kGlobalColorFeaturePaV2],
            versions.version[kGlobalColorFeaturePaV2]);
    }

    // 2. instantiate concrete ColorInterface from libsdm-color.so, pass all hardware info in.
    error = create_intf_(COLOR_VERSION_TAG, color_manager_proxy->display_id_,
                         color_manager_proxy->device_type_, hw_attr,
                         &color_manager_proxy->color_intf_);
    if (error != kErrorNone) {
      DLOGW("Unable to instantiate concrete ColorInterface from %s", COLORMGR_LIBRARY_NAME);
      delete color_manager_proxy;
      color_manager_proxy = NULL;
      return color_manager_proxy;
    }

    // 3. instantiate concrete create_stc_intf_ from libsnapdragoncolor_manager.so
    color_manager_proxy->stc_intf_ = create_stc_intf_(STC_REVISION_MAJOR, STC_REVISION_MINOR);
    if (!color_manager_proxy->stc_intf_) {
      DLOGW("Unable to instantiate concrete StcInterface from %s", STCMGR_LIBRARY_NAME);
      delete color_manager_proxy;
      color_manager_proxy = NULL;
      return color_manager_proxy;
    } else {
      int err = color_manager_proxy->stc_intf_->Init(hw_attr.panel_name);
      if (err) {
        DLOGW("Failed to init Stc interface, err %d", err);
        delete color_manager_proxy->stc_intf_;
        color_manager_proxy->stc_intf_ = NULL;
      } else {
        // pass the display interface to STC manager for digital dimming
        ScPayload payload;
        payload.len = sizeof(disp_intf);
        payload.prop = snapdragoncolor::kDisplayIntf;
        payload.payload = reinterpret_cast<uint64_t>(disp_intf);
        int ret = color_manager_proxy->stc_intf_->SetProperty(payload);
        if (ret) {
          DLOGW("Failed to SetProperty, property = %d error = %d", payload.prop, ret);
        }

        ScPayload pp_ver_pay;
        pp_ver_pay.len = sizeof(versions);
        pp_ver_pay.prop = snapdragoncolor::kSetPPFeatureVersion;
        pp_ver_pay.payload = reinterpret_cast<uint64_t>(&versions);
        ret = color_manager_proxy->stc_intf_->SetProperty(pp_ver_pay);
        if (ret) {
          DLOGW("Failed to SetProperty, property = %d error = %d",
                pp_ver_pay.prop, ret);
        }
      }

      if (color_manager_proxy->HasNativeModeSupport()) {
        color_manager_proxy->curr_mode_.gamut = allow_tonemap_native ?
                                                ColorPrimaries_BT709_5 : ColorPrimaries_Max;
        color_manager_proxy->curr_mode_.gamma = allow_tonemap_native ?
                                                Transfer_sRGB : Transfer_Max;
        color_manager_proxy->curr_mode_.intent = snapdragoncolor::kNative;
      }
    }
  }

  return color_manager_proxy;
}

ColorManagerProxy::~ColorManagerProxy() {
  if (destroy_intf_)
    destroy_intf_(display_id_);
  color_intf_ = NULL;
  if (feature_intf_) {
    feature_intf_->Deinit();
    delete feature_intf_;
    feature_intf_ = NULL;
  }
  if (stc_intf_) {
    stc_intf_->DeInit();
    delete stc_intf_;
    stc_intf_ = NULL;
  }
}

DisplayError ColorManagerProxy::ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                                     PPDisplayAPIPayload *out_payload,
                                                     PPPendingParams *pending_action) {
  DisplayError ret = kErrorNone;

  // On completion, dspp_features_ will be populated and mark dirty with all resolved dspp
  // feature list with paramaters being transformed into target requirement.
  ret = color_intf_->ColorSVCRequestRoute(in_payload, out_payload, &pp_features_, pending_action);

  if (!stc_intf_) {
    return ret;
  }

  if (!ret && pending_action->action == kGetNumRenderIntents) {
    uint32_t num_render_intent = 0;
    ScPayload payload;
    payload.len = sizeof(num_render_intent);
    payload.prop = snapdragoncolor::kGetNumRenderIntents;
    payload.payload = reinterpret_cast<uint64_t>(&num_render_intent);
    int err = stc_intf_->GetProperty(&payload);
    if (err) {
      DLOGE("Failed to get number of render intents, err %d", err);
      return kErrorUndefined;
    }

    if (!out_payload) {
      DLOGE("Out payload is NULL!");
      return kErrorParameters;
    }

    uint32_t *size = NULL;
    ret = out_payload->CreatePayload<uint32_t>(size);
    if (ret || !size) {
      DLOGE("Failed to create response payload err %d, size %p", ret, size);
      return ret;
    }

    *size = num_render_intent;
  } else if (!ret && pending_action->action == kGetRenderIntents) {
    snapdragoncolor::RenderIntentMapList render_intent_map = {};
    ScPayload payload;
    payload.len = sizeof(render_intent_map);
    payload.prop = snapdragoncolor::kGetRenderIntents;
    payload.payload = reinterpret_cast<uint64_t>(&render_intent_map);
    int err = stc_intf_->GetProperty(&payload);
    if (err) {
      DLOGE("Failed to get number of render intents, err %d", err);
      return kErrorUndefined;
    }

    pending_action->action = kSetRenderIntentsData;
    pending_action->params = reinterpret_cast<void *>(&render_intent_map);
    ret = color_intf_->ColorSVCRequestRoute(in_payload, out_payload, &pp_features_, pending_action);
  }
  return ret;
}

DisplayError ColorManagerProxy::ApplyDefaultDisplayMode(void) {
  DisplayError ret = kErrorNone;

  // On POR, will be invoked from prepare<> request once bootanimation is done.
  ret = color_intf_->ApplyDefaultDisplayMode(&pp_features_);

  return ret;
}

// Update cfg write status per physical display
void ColorManagerProxy::SetDETuningCFGpending(bool cfg_pending) {
  PPDETuningCfgData *de_tuning_cfg_data(pp_features_.GetDETuningCfgData());
  de_tuning_cfg_data->cfg_pending = cfg_pending;
}

bool ColorManagerProxy::NeedsPartialUpdateDisable() {
  Locker &locker(pp_features_.GetLocker());
  SCOPE_LOCK(locker);
  bool pu_disable = pp_features_.IsPuDisable();
  if (pu_disable) {
    // TODO(user): Enabling PU along with call to ReconfigureDisplay will result in
    // unexpected reset of disable_pu_. But this is a rare case.
    pp_features_.MarkPuEnable();
  }
  return (pu_disable || pp_features_.IsDirty() || needs_update_ || apply_mode_ ||
    pp_features_.IsSwAssetDirty());
}

int ColorManagerProxy::getCoreId(uint32_t display_id) {
  uint16_t core_id_bit_map;
  int i = 0;

  core_id_bit_map = DisplayId::GetCoreIdMap(display_id);
  if (core_id_bit_map <= 0) {
    DLOGE("Invalid core id map = %d", core_id_bit_map);
    return -1;
  }

  // return set bit
  for (i = 0; i < CORE_ID_SIZE_IN_BITS; i++) {
    if (core_id_bit_map & BIT(i)) {
      return i;
    }
  }

  return -1;
}

DisplayError ColorManagerProxy::Commit() {
  Locker &locker(pp_features_.GetLocker());
  SCOPE_LOCK(locker);

  DisplayError ret = kErrorNone;
  bool is_dirty = pp_features_.IsDirty();
  if (feature_intf_) {
    feature_intf_->SetParams(kFeatureSwitchMode, &is_dirty);
  }

  if (is_dirty) {
    // pass core to set PP features on specific core
    DLOGV("Setting PPFeatures for core_id=%d", core_id_);
    while (ret == kErrorNone) {
      PPFeatureInfo *feature = nullptr;
      if (pp_features_.RetrieveNextFeature(&feature) || !feature) {
        break;
      }
      ret = dpu_core_mux_->SetPPFeature(feature, core_id_);
    }

    // Once all features were consumed, then destroy all feature instance from feature_list,
    pp_features_.Reset();
  }

  return ret;
}

void PPHWAttributes::Set(const HWResourceInfo &hw_res,
                         const HWPanelInfo &panel_info,
                         const DisplayConfigVariableInfo &attr,
                         const PPFeatureVersion &feature_ver,
                         DppsControlInterface *intf) {
  HWResourceInfo &res = *this;
  res = hw_res;
  HWPanelInfo &panel = *this;
  panel = panel_info;
  DisplayConfigVariableInfo &attributes = *this;
  attributes = attr;
  version = feature_ver;
  dpps_intf = intf;
  max_brightness = panel_info.panel_max_brightness;

  if (strlen(panel_info.panel_name)) {
    snprintf(&panel_name[0], sizeof(panel_name), "%s", &panel_info.panel_name[0]);
    char *tmp = panel_name;
    while ((tmp = strstr(tmp, " ")) != NULL)
      *tmp = '_';
    if ((tmp = strstr(panel_name, "\n")) != NULL)
      *tmp = '\0';
  }
}

bool ColorManagerProxy::NeedAssetsUpdate() {
  bool need_update = false;
  if (!stc_intf_) {
    return need_update;
  }
  ScPayload payload;

  payload.len = sizeof(need_update);
  payload.prop = kNeedsUpdate;
  payload.payload = reinterpret_cast<uint64_t>(&need_update);
  stc_intf_->GetProperty(&payload);
  return need_update;
}

bool ColorManagerProxy::HasNativeModeSupport() {
  has_native_mode_ = false;
  if (!stc_intf_) {
    return has_native_mode_;
  }

  snapdragoncolor::ColorModeList stc_color_modes = {};
  ColorMgrGetStcModes(&stc_color_modes);
  for (auto &iter : stc_color_modes.list) {
    if (iter.intent == snapdragoncolor::kNative) {
      has_native_mode_ = true;
    }
  }

  return has_native_mode_;
}

DisplayError ColorManagerProxy::ColorMgrGetNumOfModes(uint32_t *mode_cnt) {
  return color_intf_->ColorIntfGetNumDisplayModes(&pp_features_, 0, mode_cnt);
}

DisplayError ColorManagerProxy::ColorMgrGetModes(uint32_t *mode_cnt,
                                                 SDEDisplayMode *modes) {
  return color_intf_->ColorIntfEnumerateDisplayModes(&pp_features_, 0, modes, mode_cnt);
}

DisplayError ColorManagerProxy::ColorMgrSetMode(int32_t color_mode_id) {
  return color_intf_->ColorIntfSetDisplayMode(&pp_features_, 0, color_mode_id);
}

DisplayError ColorManagerProxy::ColorMgrGetModeInfo(int32_t mode_id, AttrVal *query) {
  return color_intf_->ColorIntfGetModeInfo(&pp_features_, 0, mode_id, query);
}

static bool TransformIsIdentity(std::array<float, snapdragoncolor::kMatrixSize> &coeff) {
  if (coeff.at(0) == 1.0 && coeff.at(5) == 1.0 && coeff.at(10) == 1.0 && coeff.at(15) == 1.0) {
    if (coeff.at(1) == 0.0 && coeff.at(2) == 0.0 && coeff.at(3) == 0.0 && coeff.at(4) == 0.0 &&
        coeff.at(6) == 0.0 && coeff.at(7) == 0.0 && coeff.at(8) == 0.0 && coeff.at(9) == 0.0 &&
        coeff.at(11) == 0.0 && coeff.at(12) == 0.0 && coeff.at(13) == 0.0 && coeff.at(14) == 0.0) {
      return true;
    }
  }

  return false;
}

DisplayError ColorManagerProxy::ColorMgrSetColorTransform(uint32_t length,
                                                          const double *trans_data) {
  if (!trans_data) {
    DLOGE("Invalid parameters");
    return kErrorParameters;
  }

  if (length != snapdragoncolor::kMatrixSize) {
    DLOGE("The length of matrix is not as expected : %d, len = %d", snapdragoncolor::kMatrixSize,
          length);
    return kErrorParameters;
  }

  if (!stc_intf_) {
    DLOGW("STC interface is NULL");
    return kErrorNone;
  }

  struct snapdragoncolor::ColorTransform color_transform = {};
  for (uint32_t i = 0; i < length; i++) {
    color_transform.coeff_array[i] = static_cast<float>(*(trans_data + i));
  }

  curr_color_xform_.coeff_array = color_transform.coeff_array;
  ScPayload in_data = {};
  in_data.prop = snapdragoncolor::kSetColorTransform;
  in_data.len = sizeof(color_transform);
  in_data.payload = reinterpret_cast<uint64_t>(&color_transform);
  int result = stc_intf_->SetProperty(in_data);
  if (result) {
    DLOGE("Failed to SetProperty prop = %d, error = %d", in_data.prop, result);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError ColorManagerProxy::ColorMgrGetDefaultModeID(int32_t *mode_id) {
  return color_intf_->ColorIntfGetDefaultModeID(&pp_features_, 0, mode_id);
}

DisplayError ColorManagerProxy::ColorMgrCombineColorModes() {
  return color_intf_->ColorIntfCombineColorModes();
}

DisplayError ColorManagerProxy::ColorMgrSetModeWithRenderIntent(int32_t color_mode_id,
                                         const PrimariesTransfer &blend_space, uint32_t intent) {
  cur_blend_space_ = blend_space;
  cur_intent_ = intent;
  cur_mode_id_ = color_mode_id;
  apply_mode_ = true;
  return kErrorNone;
}

DisplayError ColorManagerProxy::ColorMgrSetSprIntf(std::shared_ptr<SPRIntf> spr_intf) {
  return color_intf_->ColorIntfSetSprInterface(spr_intf);
}

DisplayError ColorManagerProxy::Validate(DispLayerStack *disp_layer_stack) {
  DisplayError ret = kErrorNone;
  if (!disp_layer_stack) {
    return ret;
  }

  bool valid_meta_data = false;
  bool update_meta_data = false;
  Layer hdr_layer = {};
  bool hdr_present = false;

  // ToDo(devanshi): handle for vector of HWLayersInfo
  valid_meta_data = NeedsToneMap(disp_layer_stack->info.begin()->second.hw_layers);
  if (valid_meta_data) {
    if (disp_layer_stack->info.begin()->second.hdr_layer_info.in_hdr_mode &&
        disp_layer_stack->info.begin()->second.hdr_layer_info.operation == HWHDRLayerInfo::kSet) {
      hdr_layer = *(disp_layer_stack->stack->layers.at(
                    UINT32(disp_layer_stack->info.begin()->second.hdr_layer_info.layer_index)));
      hdr_present = true;
    }

    if (hdr_present && hdr_layer.input_buffer.dynamicMetadata.dynamicMetaDataValid &&
        hdr_layer.input_buffer.dynamicMetadata.dynamicMetaDataLen) {
      update_meta_data = true;
      meta_data_ = convertToLegacyColorMetadata(&hdr_layer.input_buffer);
    }
  }

  if (needs_update_ || apply_mode_ || update_meta_data) {
    UpdateModeHwassets(cur_mode_id_, curr_mode_, update_meta_data, meta_data_);
    DumpColorMetaData(meta_data_);
    apply_mode_ = false;
    needs_update_ = false;
  }

  {
    Locker &locker(pp_features_.GetLocker());
    SCOPE_LOCK(locker);
    bool dirty = pp_features_.IsSwAssetDirty();
    if (dirty) {
      pp_features_.ClearSwAssertDirty();
    }
  }
  return kErrorNone;
}

DisplayError ColorManagerProxy::Prepare() {
  DisplayError ret = kErrorNone;

  ret = ApplySwAssets();
  return ret;
}

bool ColorManagerProxy::IsValidateNeeded() {
  bool dirty = false;
  {
    Locker &locker(pp_features_.GetLocker());
    SCOPE_LOCK(locker);
    dirty = pp_features_.IsSwAssetDirty();
  }

  needs_update_ = NeedAssetsUpdate();
  return (dirty || needs_update_ || apply_mode_);
}

DisplayError ColorManagerProxy::ApplySwAssets() {
  DisplayError error = kErrorNone;

  if (!needs_update_ && !apply_mode_) {
    return error;
  }

  if (!stc_intf_) {
    DLOGE("STC interface is NULL");
    return kErrorUndefined;
  }

  ScPayload in_data = {};
  struct ModeRenderInputParams mode_params = {};
  mode_params.color_mode = curr_mode_;
  in_data.prop = kModeRenderInputParams;
  in_data.len = sizeof(mode_params);
  in_data.payload = reinterpret_cast<uint64_t>(&mode_params);

  ScPayload out_data = {};
  struct HwConfigOutputParams sw_params = {};
  out_data.prop = kHwConfigPayloadParam;
  out_data.len = sizeof(sw_params);
  out_data.payload = reinterpret_cast<uint64_t>(&sw_params);

  int err = stc_intf_->ProcessOps(kScModeSwAssets, in_data, &out_data);
  if (err) {
    DLOGE("Failed to process kScModeSwAssets, err %d", err);
    error = kErrorUndefined;
  } else if (!sw_params.payload.empty()) {
    error = ConvertToPPFeatures(sw_params, &pp_features_);
    if (error != kErrorNone) {
      DLOGE("Failed to update Stc SW assets, error %d", error);
      return error;
    }
  }

  return error;
}

DisplayError ColorManagerProxy::NotifyDisplayCalibrationMode(bool in_calibration) {
  if (!stc_intf_) {
    return kErrorUndefined;
  }

  ScPayload payload;
  payload.len = sizeof(in_calibration);
  payload.prop = kNotifyDisplayCalibrationMode;
  payload.payload = reinterpret_cast<uint64_t>(&in_calibration);
  int ret = stc_intf_->SetProperty(payload);
  if (ret) {
    DLOGE("Failed to SetProperty, property = %d error = %d", payload.prop, ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

bool ColorManagerProxy::GameEnhanceSupported() {
  bool supported = false;

  if (color_intf_) {
    color_intf_->ColorIntfGameEnhancementSupported(&supported);
  }

  return supported;
}

DisplayError ColorManagerProxy::ConvertToPPFeatures(const HwConfigOutputParams &params,
                                                    PPFeaturesConfig *out_data) {
  if (!out_data) {
    DLOGE("Invalid input parameters");
    return kErrorParameters;
  }

  if (params.payload.empty()) {
    return kErrorNone;
  }

  DisplayError error = kErrorNone;
  for (auto it = params.payload.begin(); it != params.payload.end(); it++) {
    error = color_intf_->ColorIntfConvertFeature(UINT32(display_id_), *it, out_data);
    if (error != kErrorNone) {
      if (error == kErrorNotSupported)
        DLOGW("Failed to convert %s feature to PPFeature : err %d", it->hw_asset.c_str(), error);
      else
        DLOGE("Failed to convert %s feature to PPFeature : err %d", it->hw_asset.c_str(), error);
      return error;
    }
  }
  return error;
}

DisplayError ColorManagerProxy::UpdateModeHwassets(int32_t mode_id,
                                  snapdragoncolor::ColorMode color_mode, bool valid_meta_data,
                                  const ColorMetaData &meta_data) {
  if (!stc_intf_) {
    return kErrorUndefined;
  }

  DisplayError error = kErrorNone;
  struct snapdragoncolor::ModeRenderInputParams mode_params = {};
  struct snapdragoncolor::HwConfigOutputParams hw_params = {};
  mode_params.valid_meta_data = valid_meta_data;
  mode_params.meta_data = meta_data;
  mode_params.color_mode = color_mode;
  mode_params.mode_id = mode_id;

  ScPayload in_data = {};
  ScPayload out_data = {};
  in_data.prop = kModeRenderInputParams;
  in_data.len = sizeof(mode_params);
  in_data.payload = reinterpret_cast<uint64_t>(&mode_params);

  out_data.prop = kHwConfigPayloadParam;
  out_data.len = sizeof(hw_params);
  out_data.payload = reinterpret_cast<uint64_t>(&hw_params);
  int result = stc_intf_->ProcessOps(kScModeRenderIntent, in_data, &out_data);
  if (result) {
    DLOGE("Failed to call ProcessOps, error = %d", result);
    return kErrorUndefined;
  }

  error = ConvertToPPFeatures(hw_params, &pp_features_);
  if (error != kErrorNone) {
    if (error == kErrorNotSupported)
      DLOGW("Failed to convert hw assets to PP features, error = %d", error);
    else
      DLOGE("Failed to convert hw assets to PP features, error = %d", error);
    return error;
  }
  pp_features_.MarkAsDirty();
  return error;
}

void ColorManagerProxy::DumpColorMetaData(const ColorMetaData &color_metadata) {
  DLOGI_IF(kTagResources, "Primaries = %d, Range = %d, Transfer = %d, Matrix Coeffs = %d",
           color_metadata.colorPrimaries, color_metadata.range, color_metadata.transfer,
           color_metadata.matrixCoefficients);

  for (uint32_t i = 0; i < 3; i++) {
    for (uint32_t j = 0; j < 2; j++) {
      DLOGV_IF(kTagResources, "RGB Primaries[%d][%d] = %d", i, j,
               color_metadata.masteringDisplayInfo.primaries.rgbPrimaries[i][j]);
    }
  }
  DLOGV_IF(kTagResources, "White Point[0] = %d White Point[1] = %d",
           color_metadata.masteringDisplayInfo.primaries.whitePoint[0],
           color_metadata.masteringDisplayInfo.primaries.whitePoint[1]);
  DLOGV_IF(kTagResources, "Max Disp Luminance = %d Min Disp Luminance= %d",
           color_metadata.masteringDisplayInfo.maxDisplayLuminance,
           color_metadata.masteringDisplayInfo.minDisplayLuminance);
  DLOGV_IF(kTagResources, "Max ContentLightLevel = %d Max AvgLightLevel = %d",
           color_metadata.contentLightLevel.maxContentLightLevel,
           color_metadata.contentLightLevel.minPicAverageLightLevel);
  DLOGV_IF(kTagResources, "DynamicMetaDataValid = %d DynamicMetaDataLen = %d",
           color_metadata.dynamicMetaDataValid,
           color_metadata.dynamicMetaDataLen);
}

DisplayError ColorManagerProxy::ColorMgrGetStcModes(ColorModeList *mode_list) {
  if (!stc_intf_) {
    DLOGE("STC interface is NULL");
    return kErrorUndefined;
  }

  ScPayload payload;
  payload.len = sizeof(ColorModeList);
  payload.prop = kModeList;
  payload.payload = reinterpret_cast<uint64_t>(mode_list);

  int err = stc_intf_->GetProperty(&payload);
  if (err) {
    DLOGE("Failed to get Stc color modes, err %d", err);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError ColorManagerProxy::ColorMgrSetStcMode(const ColorMode &color_mode) {
  curr_mode_ = color_mode;
  apply_mode_ = true;
  return kErrorNone;
}

DisplayError ColorManagerProxy::ColorMgrSetLtmPccConfig(void* pcc_input, size_t size) {
  if (!stc_intf_) {
    DLOGE("STC interface is NULL");
    return kErrorUndefined;
  }

  ScPayload in_data = {};
  in_data.prop = snapdragoncolor::kSetLtmPccConfig;
  if (pcc_input) {
    in_data.payload = reinterpret_cast<uint64_t>(pcc_input);
    in_data.len = size;
  } else {
    in_data.payload = reinterpret_cast<uint64_t>(nullptr);
    in_data.len = 0;
  }
  int result = stc_intf_->SetProperty(in_data);
  if (result) {
    DLOGE("Failed to SetProperty prop = %d, error = %d", in_data.prop, result);
    return kErrorUndefined;
  }
  return kErrorNone;
}

DisplayError ColorManagerProxy::ConfigureCWBDither(CwbConfig *cwb_cfg, bool free_data) {
  DisplayError error = kErrorNone;

  if (!stc_intf_ || (!cwb_cfg && !free_data)) {
    DLOGE("Invalid stc_intf_ %pK, cwb_cfg %pK", stc_intf_, cwb_cfg);
    return kErrorParameters;
  }

  //<<! free dither data
  PPFrameCaptureData *frame_capture_data(pp_features_.GetFrameCaptureData());
  if (free_data) {
    if (frame_capture_data->input_params.dither_payload) {
      DLOGV_IF(kTagQDCM, "free cwb dither data");
      delete frame_capture_data->input_params.dither_payload;
      frame_capture_data->input_params.dither_payload = nullptr;
    }
    frame_capture_data->input_params.dither_flags = 0x0;
    return kErrorNone;
  }
  cwb_cfg->dither_info = nullptr;

  //<<! Only the first frame goes to get pp-dither when multi-frames need to be captured
  //<<! dither_flags is 0x0: dither settings from current color mode
  //<<! dither_flags is 0x1: dither settings from QDCM PC tool
  if (frame_capture_data->input_params.dither_flags == 0x0 &&
      !frame_capture_data->input_params.dither_payload) {
    snapdragoncolor::HwConfigOutputParams dither_hw_params = {};
    ScPayload output = {};
    output.len = sizeof(dither_hw_params);
    output.prop = snapdragoncolor::kGetGlobalDitherHwConfig;
    output.payload = reinterpret_cast<uint64_t>(&dither_hw_params);
    int ret = stc_intf_->GetProperty(&output);
    if (ret) {
      DLOGE("Failed to get propety of global dither hw config");
      return kErrorUndefined;
    }

    if (dither_hw_params.payload.empty()) {
      DLOGV_IF(kTagQDCM, "No dither hardware asset found in color mode");
      return kErrorNone;
    }

    if (dither_hw_params.payload[0].hw_asset.empty() ||
        dither_hw_params.payload[0].hw_payload_len == 0) {
      DLOGE("Invalid hw_asset.empty is %d, hw_payload_len %u",
            dither_hw_params.payload[0].hw_asset.empty(),
            dither_hw_params.payload[0].hw_payload_len);
      return kErrorParameters;
    }

    //<<! update asset name from kPbDither to kPbCWBDither
    //<<! convert data struct from dither_coeff_data to SDEDitherCfg
    dither_hw_params.payload[0].hw_asset = snapdragoncolor::kPbCWBDither;
    error = ConvertToPPFeatures(dither_hw_params, &pp_features_);
    if (error != kErrorNone) {
      DLOGE("Failed to convert cwb dither feature, error %d", error);
      return error;
    }
  }

  //<<! config the payload to hw_cwb_config
  PPFeatureInfo *dither_payload = frame_capture_data->input_params.dither_payload;
  if (dither_payload && (dither_payload->enable_flags_ & kOpsEnable))
    cwb_cfg->dither_info = dither_payload;
  DLOGV_IF(kTagQDCM, "config cwb dither data done");
  return error;
}

DisplayError ColorManagerProxy::ColorMgrIdleFallback(bool idle_fallback_hint) {
  DisplayError error = kErrorNone;

  if(prev_idle_fallback_hint_ == idle_fallback_hint) {
    return kErrorNone;
  }

  if (!has_native_mode_) {
    DLOGE("Native mode is missing from the calibration file");
    return kErrorNotSupported;
  }

  prev_idle_fallback_hint_ = idle_fallback_hint;
  bool curr_xform_is_identity = TransformIsIdentity(curr_color_xform_.coeff_array);

  if (idle_fallback_hint) {
    ColorMode idle_fallback_mode;
    struct snapdragoncolor::ColorTransform color_transform = {};
    color_transform.coeff_array = COLOR_TRANSFORM_IDENTITY;

    // Storing current ColorMode which will be used while exiting IdleFallBack
    prev_idle_fallback_mode_ = curr_mode_;

    //Set Native mode on idle fallback
    idle_fallback_mode.gamut = ColorPrimaries_Max;
    idle_fallback_mode.gamma = Transfer_Max;
    idle_fallback_mode.intent = snapdragoncolor::RenderIntent::kNative;
    idle_fallback_mode.intent_name = "Standard";

    DLOGV_IF(kTagQDCM, "idle fallback entry mode: gamut: %d, gamma: %d, intent: %d",
      idle_fallback_mode.gamut, idle_fallback_mode.gamma, idle_fallback_mode.intent);
    error = ColorMgrSetStcMode(idle_fallback_mode);

    if (stc_intf_ && !curr_xform_is_identity) {
      ScPayload in_data = {};
      in_data.prop = snapdragoncolor::kSetColorTransform;
      in_data.len = sizeof(color_transform);
      in_data.payload = reinterpret_cast<uint64_t>(&color_transform);
      if (stc_intf_->SetProperty(in_data)) {
        DLOGE("Failed to set identity transform on idle fallback entry!");
        error = kErrorUndefined;
      }
    }

    return error;
  }

  DLOGV_IF(kTagQDCM, "idle fallback exit mode: gamut: %d, gamma: %d, intent: %d",
      prev_idle_fallback_mode_.gamut, prev_idle_fallback_mode_.gamma,
      prev_idle_fallback_mode_.intent);
  error = ColorMgrSetStcMode(prev_idle_fallback_mode_);
  prev_idle_fallback_mode_ = {};

  if (stc_intf_ && !curr_xform_is_identity) {
    ScPayload in_data = {};
    in_data.prop = snapdragoncolor::kSetColorTransform;
    in_data.len = sizeof(curr_color_xform_);
    in_data.payload = reinterpret_cast<uint64_t>(&curr_color_xform_);
    if (stc_intf_->SetProperty(in_data)) {
      DLOGE("Failed to set color transform on idle fallback exit!");
      error = kErrorUndefined;
    }
  }

  return error;
}

ColorFeatureCheckingImpl::ColorFeatureCheckingImpl(DPUCoreMux *dpu_core_mux,
                                                   PPFeaturesConfig *pp_features,
                                                   bool dyn_switch, uint32_t core_id)
  : dpu_core_mux_(dpu_core_mux), pp_features_(pp_features), dyn_switch_(dyn_switch),
    core_id_(core_id) {}

DisplayError ColorFeatureCheckingImpl::Init() {
  states_.at(kFrameTriggerDefault) = new FeatureStateDefaultTrigger(this);
  states_.at(kFrameTriggerSerialize) = new FeatureStateSerializedTrigger(this);
  states_.at(kFrameTriggerPostedStart) = new FeatureStatePostedStart(this);

  if (std::any_of(states_.begin(), states_.end(),
      [](const FeatureInterface *p) {
      if (!p) {
        return true;
      } else {
        return false;
      }})) {
    std::all_of(states_.begin(), states_.end(),
      [](const FeatureInterface *p) {
      if (p) {delete p;} return true;});
    states_.fill(NULL);
    curr_state_ = NULL;
  } else {
    curr_state_ = states_.at(kFrameTriggerDefault);
  }

  if (curr_state_) {
    single_buffer_feature_.clear();
    single_buffer_feature_.push_back(kGlobalColorFeatureIgc);
    single_buffer_feature_.push_back(kGlobalColorFeatureGamut);
  } else {
    DLOGE("Failed to create curr_state_");
    return kErrorMemory;
  }
  return kErrorNone;
}

DisplayError ColorFeatureCheckingImpl::Deinit() {
  std::all_of(states_.begin(), states_.end(),
    [](const FeatureInterface *p)
    {if (p) {delete p;} return true;});
  states_.fill(NULL);
  curr_state_ = NULL;
  single_buffer_feature_.clear();
  return kErrorNone;
}

DisplayError ColorFeatureCheckingImpl::SetParams(FeatureOps param_type,
                                                 void *payload) {
  DisplayError error = kErrorNone;
  FrameTriggerMode mode = kFrameTriggerDefault;

  if (!payload) {
    DLOGE("Invalid input payload");
    return kErrorParameters;
  }

  if (!curr_state_) {
    DLOGE("Invalid curr state");
    return kErrorParameters;
  }

  bool is_dirty = *reinterpret_cast<bool *>(payload);
  switch (param_type) {
  case kFeatureSwitchMode:
    if (is_dirty) {
      CheckColorFeature(&mode);
    } else {
      mode = kFrameTriggerPostedStart;
    }
    DLOGV_IF(kTagQDCM, "Set frame trigger mode %d", mode);
    error = curr_state_->SetParams(param_type, &mode);
    if (error) {
      DLOGE_IF(kTagQDCM, "Failed to set params to state, error %d", error);
    }
    break;
  default:
    DLOGW("unhandled param_type %d", param_type);
    error = kErrorNotSupported;
    break;
  }
  return error;
}

DisplayError ColorFeatureCheckingImpl::GetParams(FeatureOps param_type,
                                                 void *payload) {
  DisplayError error = kErrorNone;

  if (!payload) {
    DLOGE("Invalid input payload");
    return kErrorParameters;
  }

  if (!curr_state_) {
    DLOGE("Invalid curr state");
    return kErrorParameters;
  }

  switch (param_type) {
  case kFeatureSwitchMode:
    if (curr_state_) {
      curr_state_->GetParams(param_type, payload);
    } else {
      DLOGE_IF(kTagQDCM, "curr_state_ NULL");
      error = kErrorUndefined;
    }
    break;
  default:
    DLOGW("unhandled param_type %d", param_type);
    error = kErrorNotSupported;
    break;
  }
  return error;
}

// This function checks through the feature list for the single buffer features.
// If there is single buffer feature existed in the feature list, the posted start
// should be disabled.
void ColorFeatureCheckingImpl::CheckColorFeature(FrameTriggerMode *mode) {
  PPFeatureInfo *feature = NULL;
  PPGlobalColorFeatureID id = kMaxNumPPFeatures;

  if (!pp_features_) {
    DLOGW("Invalid pp features");
    *mode = kFrameTriggerPostedStart;
    return;
  }

// Due to lack of hardware support for SB LUTDMA on older targets,
// control path has to be switched dynamically to non posted start and
// switch back to posted start after programming the SB LUTs.
// This restriction can be removed for targets supporting
// SB programming of color modules through SB LUTDMA during the blanking period.

  if (dyn_switch_) {
    for (uint32_t i = 0; i < single_buffer_feature_.size(); i++) {
      id = single_buffer_feature_[i];
      feature = pp_features_->GetFeature(id);
      if (feature && (feature->enable_flags_ & kOpsEnable)) {
        *mode = kFrameTriggerDefault;
        return;
      }
    }
  }
  *mode = kFrameTriggerPostedStart;
}

FeatureStatePostedStart::FeatureStatePostedStart(ColorFeatureCheckingImpl *obj)
  : obj_(obj) {}

DisplayError FeatureStatePostedStart::Init() {
  return kErrorNone;
}

DisplayError FeatureStatePostedStart::Deinit() {
  return kErrorNone;
}

DisplayError FeatureStatePostedStart::SetParams(FeatureOps param_type,
                                                void *payload) {
  DisplayError error = kErrorNone;
  FrameTriggerMode mode = kFrameTriggerPostedStart;

  if (!obj_) {
    DLOGE("Invalid param obj_");
    return kErrorParameters;
  }

  if (!payload) {
    DLOGE("Invalid payload");
    return kErrorParameters;
  }

  switch (param_type) {
  case kFeatureSwitchMode:
    mode = *(reinterpret_cast<FrameTriggerMode *>(payload));
    if (mode >= kFrameTriggerMax) {
      DLOGE("Invalid mode %d", mode);
      return kErrorParameters;
    }
    if (mode != kFrameTriggerPostedStart) {
      error = obj_->dpu_core_mux_->SetFrameTrigger(mode, obj_->core_id_);
      if (!error) {
        obj_->curr_state_ = obj_->states_.at(mode);
      }
    } else {
      DLOGV_IF(kTagQDCM, "Already in posted start mode");
    }
    break;
  default:
    DLOGW("unhandled param_type %d", param_type);
    error = kErrorNotSupported;
    break;
  }
  return error;
}

DisplayError FeatureStatePostedStart::GetParams(FeatureOps param_type,
                                                void *payload) {
  DisplayError error = kErrorNone;

  if (!obj_) {
    DLOGE("Invalid param obj_");
    return kErrorParameters;
  }

  if (!payload) {
    DLOGE("Invalid payload");
    return kErrorParameters;
  }

  switch (param_type) {
  case kFeatureSwitchMode:
    *(reinterpret_cast<FrameTriggerMode *>(payload)) = kFrameTriggerPostedStart;
    break;
  default:
    DLOGW("unhandled param_type %d", param_type);
    error = kErrorNotSupported;
    break;
  }

  return error;
}

FeatureStateDefaultTrigger::FeatureStateDefaultTrigger(ColorFeatureCheckingImpl *obj)
  : obj_(obj) {}

DisplayError FeatureStateDefaultTrigger::Init() {
  return kErrorNone;
}

DisplayError FeatureStateDefaultTrigger::Deinit() {
  return kErrorNone;
}

DisplayError FeatureStateDefaultTrigger::SetParams(FeatureOps param_type,
                                                   void *payload) {
  DisplayError error = kErrorNone;
  FrameTriggerMode mode = kFrameTriggerDefault;

  if (!obj_) {
    DLOGE("Invalid param obj_");
    return kErrorParameters;
  }

  if (!payload) {
    DLOGE("Invalid payload");
    return kErrorParameters;
  }

  switch (param_type) {
  case kFeatureSwitchMode:
    mode = *(reinterpret_cast<FrameTriggerMode *>(payload));
    if (mode >= kFrameTriggerMax) {
      DLOGE("Invalid mode %d", mode);
      return kErrorParameters;
    }
    if (mode != kFrameTriggerDefault) {
      error = obj_->dpu_core_mux_->SetFrameTrigger(mode, obj_->core_id_);
      if (!error) {
        obj_->curr_state_ = obj_->states_.at(mode);
      }
    } else {
      DLOGV_IF(kTagQDCM, "Already in default trigger mode");
    }
    break;
  default:
    DLOGW("unhandled param_type %d", param_type);
    error = kErrorNotSupported;
    break;
  }
  return error;
}

DisplayError FeatureStateDefaultTrigger::GetParams(FeatureOps param_type,
                                                   void *payload) {
  DisplayError error = kErrorNone;

  if (!obj_) {
    DLOGE("Invalid param obj_");
    return kErrorParameters;
  }

  if (!payload) {
    DLOGE("Invalid payload");
    return kErrorParameters;
  }

  switch (param_type) {
  case kFeatureSwitchMode:
    *(reinterpret_cast<FrameTriggerMode *>(payload)) = kFrameTriggerDefault;
    break;
  default:
    DLOGW("unhandled param_type %d", param_type);
    error = kErrorNotSupported;
    break;
  }

  return error;
}

FeatureStateSerializedTrigger::FeatureStateSerializedTrigger(ColorFeatureCheckingImpl *obj)
  : obj_(obj) {}

DisplayError FeatureStateSerializedTrigger::Init() {
  return kErrorNone;
}

DisplayError FeatureStateSerializedTrigger::Deinit() {
  return kErrorNone;
}

DisplayError FeatureStateSerializedTrigger::SetParams(FeatureOps param_type,
                                                      void *payload) {
  DisplayError error = kErrorNone;
  FrameTriggerMode mode = kFrameTriggerSerialize;

  if (!obj_) {
    DLOGE("Invalid param obj_");
    return kErrorParameters;
  }

  if (!payload) {
    DLOGE("Invalid payload");
    return kErrorParameters;
  }

  switch (param_type) {
  case kFeatureSwitchMode:
    mode = *(reinterpret_cast<FrameTriggerMode *>(payload));
    if (mode >= kFrameTriggerMax) {
      DLOGE("Invalid mode %d", mode);
      return kErrorParameters;
    }
    if (mode != kFrameTriggerSerialize) {
      error = obj_->dpu_core_mux_->SetFrameTrigger(mode, obj_->core_id_);
      if (!error) {
        obj_->curr_state_ = obj_->states_.at(mode);
      }
    } else {
      DLOGV_IF(kTagQDCM, "Already in serialized trigger mode");
    }
    break;
  default:
    DLOGW("unhandled param_type %d", param_type);
    error = kErrorNotSupported;
    break;
  }
  return error;
}

DisplayError FeatureStateSerializedTrigger::GetParams(FeatureOps param_type,
                                                      void *payload) {
  DisplayError error = kErrorNone;

  if (!obj_) {
    DLOGE("Invalid param obj_");
    return kErrorParameters;
  }

  if (!payload) {
    DLOGE("Invalid payload");
    return kErrorParameters;
  }

  switch (param_type) {
  case kFeatureSwitchMode:
    *(reinterpret_cast<FrameTriggerMode *>(payload)) = kFrameTriggerSerialize;
    break;
  default:
    DLOGW("unhandled param_type %d", param_type);
    error = kErrorNotSupported;
    break;
  }

  return error;
}

#undef __CLASS__
#define __CLASS__ "DPUColorManager"

DPUColorManager::DPUColorManager(DisplayId id_info)
                                : display_id_info_(id_info) {
}

DPUColorManager::~DPUColorManager() {
}

DisplayError DPUColorManager::Init(const std::vector<HWResourceInfo>& hw_res_info) {
  return kErrorNone;
}

void DPUColorManager::Deinit() {
}

int DPUColorManager::CreatePhysicalDisplayIds(DisplayId display_id_info) {
  int ret = 0, core_id = 0;
  std::map<uint8_t, uint32_t> conn_id_map;

  core_id = display_id_info.GetCoreIdMap();
  conn_id_map = display_id_info.GetConnIdMap();

  if (!core_id) {
    DLOGE("No dpu is present core_id = %d", core_id);
    return -EINVAL;
  }

  // Create physical display ID for each physical display
  for (int i = 0; core_id >> i; i++) {
    uint32_t disp_id = 0;
    if (core_id & BIT(i)) {
      disp_id = DisplayId(i, conn_id_map).GetDisplayId();
      DLOGV("Creating physical display id for core=%d, id=0x%x", i, disp_id);
      display_id_list_.push_back(disp_id);
    }
  }

  return ret;
}

// Create physical display ID and create color manager proxy per physical display
DPUColorManager *DPUColorManager::CreateDpuColorManager(SDMDisplayType type,
                                                        DPUCoreMux *dpu_core_mux,
                                                        DisplayDeviceContext &display_device_ctx,
                                                        DisplayClientContext &display_client_ctx,
                                                        DppsControlInterface *dpps_intf,
                                                        DisplayInterface *disp_intf,
                                                        vector<HWResourceInfo> &hw_res_info,
                                                        DisplayId display_id_info) {
  DPUColorManager *dpu_color_manager = NULL;
  int ret = 0;

  dpu_color_manager = new DPUColorManager(display_id_info);
  if (!dpu_color_manager) {
    DLOGE("failed to create dpu color_manager");
    return NULL;
  }

  // From a logical display ID create physical display ID per physical display
  ret = dpu_color_manager->CreatePhysicalDisplayIds(display_id_info);
  if (ret || dpu_color_manager->display_id_list_.size() == 0) {
    DLOGE("Failed to create local display ids ret=%d, display_id count=%d",
            ret, dpu_color_manager->display_id_list_.size());
    return NULL;
  }

  dpu_color_manager->hw_res_info_ = hw_res_info;

  // Create color manager proxy per physical display
  for (int i = 0; i < dpu_color_manager->display_id_list_.size(); i++) {
    ColorManagerProxy *colorManager = NULL;
    DLOGV("Creating colorManagerProxy for core=%d, display_id=%d",
                                              i, dpu_color_manager->display_id_list_[i]);
    colorManager = ColorManagerProxy::CreateColorManagerProxy(type, dpu_core_mux,
                                                          display_device_ctx[i].display_attributes,
                                                          display_device_ctx[i].hw_panel_info,
                                                          dpps_intf, disp_intf, hw_res_info[i],
                                                          dpu_color_manager->display_id_list_[i]);
    if (!colorManager) {
      DLOGE("Failed to create ColorManagerProxy for core=%d, display id=%d",
                                              i, dpu_color_manager->display_id_list_[i]);
      goto exit;
    }

    dpu_color_manager->color_manager_proxy_list_.push_back(colorManager);
  }

  return dpu_color_manager;

exit:
  delete dpu_color_manager;
  dpu_color_manager = NULL;
  return dpu_color_manager;
}

bool DPUColorManager::ComparePendingAction(vector<PPPendingParams>& pending_action) {
  bool same_pending_action = true;

  for (int i = 1; i < pending_action.size(); i++) {
    if (pending_action[0].action != pending_action[i].action) {
        same_pending_action = false;
        DLOGE("pending action are different for cores, core_0=%d, core_%d=%d",
                        pending_action[0].action, i, pending_action[i].action);
        return same_pending_action;
    }
  }

  return same_pending_action;
}

DisplayError DPUColorManager::ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                                    PPDisplayAPIPayload *out_payload,
                                                    PPPendingParams *out_pending_action) {
  DisplayError error = kErrorNone;
  int color_mgr_cnt = color_manager_proxy_list_.size();
  PPPendingParams pending_action;
  vector<PPPendingParams> pending_action_list;
  bool same_pending_action = true;

  pending_action_list.reserve(color_mgr_cnt);

  // loop through for each physical display and get the pending data
  for (int i = 0; i < color_mgr_cnt; i++) {
    DLOGV("Getting Color SVC Request Route for core=%d", i);
    pending_action.action = out_pending_action->action;
    pending_action.params = out_pending_action->params;
    error = color_manager_proxy_list_[i]->ColorSVCRequestRoute(in_payload, out_payload,
                                                                         &pending_action);
    if (error) {
        DLOGE("failed to Request SVC Route error=%d", error);
        return error;
    }

    pending_action_list.push_back(pending_action);
  }

  // compare that the pending actions are same across all the displays
  if (color_mgr_cnt > 1) {
    same_pending_action = ComparePendingAction(pending_action_list);
    if (!same_pending_action) {
      error = kErrorNotSupported;
      DLOGE("pending actions are not same error=%d", error);
      return error;
    }
  }

  // Return only one pending action as both are same.
  out_pending_action->action = pending_action_list[0].action;
  out_pending_action->params = pending_action_list[0].params;

  return error;
}

void DPUColorManager::SetDETuningCFGpending(bool cfg_pending) {
  for (auto& color_mgr : color_manager_proxy_list_)
    color_mgr->SetDETuningCFGpending(cfg_pending);
}

DisplayError DPUColorManager::ColorMgrGetNumOfModes(uint32_t *mode_cnt) {
  DisplayError error = kErrorNone;
  // std::set holds unique, ordered elements
  std::set<uint32_t> mode_count_set;
  uint32_t mode_count = 0;
  int i = 0;

  // Get mode count for both DPU's
  // Check whether mode_count is same for both DPU or not
  for (auto& color_mgr : color_manager_proxy_list_) {
    DLOGV("Get num of modes for core=%d", i++);
    error = color_mgr->ColorMgrGetNumOfModes(&mode_count);
    if (error) {
      DLOGE("Failed to get mode count %d", error);
      return error;
    }

    mode_count_set.insert(mode_count);
  }

  // check if mode count is same for all displays
  if (mode_count_set.size() > 1) {
    DLOGE("Different num of mode for both displays mode_count %d", mode_count_set.size());
    error = kErrorNotSupported;
    return error;
  }

  DLOGV("num of modes for all cores is %d", mode_count);
  *mode_cnt = mode_count;

  return error;
}

// compare and check if all displays have same modes or not
bool DPUColorManager::CompareSDEDisplayModes(vector<SDEDisplayMode>& mode) {
  bool is_same_mode = true;

  for (int i = 1; i < mode.size(); i++) {
    if ((mode[0].id != mode[i].id) && (mode[0].type != mode[i].type) &&
          (mode[0].name != mode[i].name))
      is_same_mode = false;
  }

  return is_same_mode;
}

DisplayError DPUColorManager::ColorMgrGetModes(uint32_t *out_mode_cnt,
                                                    SDEDisplayMode *out_mode) {
  DisplayError error = kErrorNone;
  int color_mgr_cnt = color_manager_proxy_list_.size();
  vector<uint32_t> mode_count;
  vector<SDEDisplayMode> modes;
  bool equal_mode_count = false;

  mode_count.reserve(color_mgr_cnt);
  modes.reserve(color_mgr_cnt);

  // loop for all physical display and get color modes for each
  for (int i = 0; i < color_mgr_cnt; i++) {
    DLOGV("Get mode for core=%d", i);
    mode_count[i] = *out_mode_cnt;
    error = color_manager_proxy_list_[i]->ColorMgrGetModes(&mode_count[i], &modes[i]);
    if (error) {
      DLOGE("Failed to get mode count %d", error);
      return error;
    }
  }

  // Check if mode count is same or not for all the DPU
  equal_mode_count = std::equal(mode_count.begin(), mode_count.end(), mode_count.begin());
  if (!equal_mode_count) {
    error = kErrorNotSupported;
    DLOGE("Received mode count is not same error: %d", error);
    return error;
  }

  if (!CompareSDEDisplayModes(modes)) {
    error = kErrorNotSupported;
    DLOGE("Received modes are not same, error: %d", error);
    return error;
  }

  // return only 1 mode as all the modes are same
  *out_mode_cnt = mode_count[0];
  out_mode->id = modes[0].id;
  out_mode->type = modes[0].type;
  snprintf(out_mode->name, out_mode->kMaxModeNameSize, "%s", modes[0].name);

  return error;
}

DisplayError DPUColorManager::CompareAttrVal(vector<AttrVal>& query) {
  DisplayError error = kErrorNone;

  for (int i = 1; i < query.size(); i++) {
    auto& attribute1 = query[0];
    auto& attribute2 = query[i];

    if (attribute1.size() != attribute2.size()) {
      DLOGE("attribute size is different");
      return kErrorNotSupported;
    } else {
      for (int j = 0; j < attribute1.size(); j++) {
          if ((attribute1[j].first != attribute2[j].first) ||
              (attribute1[j].second != attribute2[j].second))
              return kErrorNotSupported;
      }
    }
  }

  return error;
}

DisplayError DPUColorManager::ColorMgrSetMode(int32_t color_mode_id) {
  DisplayError error = kErrorNone;

  // set color mode to all the physical displays
  for (auto& color_mgr : color_manager_proxy_list_) {
    error = color_mgr->ColorMgrSetMode(color_mode_id);
    if (error) {
      DLOGE("Failed to set mode error=%d", error);
      return error;
    }
  }

  return error;
}

DisplayError DPUColorManager::ColorMgrGetModeInfo(int32_t in_mode_id,
                                                    AttrVal *out_query) {
  DisplayError error = kErrorNone;
  int color_mgr_cnt = color_manager_proxy_list_.size();
  vector<AttrVal> query;

  query.reserve(color_mgr_cnt);

  // loop for all physical display and get mode info
  for (int i = 0; i < color_mgr_cnt; i++) {
    DLOGV("Get Mode Info for core=%d", i);
    error = color_manager_proxy_list_[i]->ColorMgrGetModeInfo(in_mode_id, &query[i]);
    if (error) {
      DLOGE("Failed to get mode count %d", error);
      return error;
    }
  }

  // compare and check if mode info is same across all the displays or not
  error = CompareAttrVal(query);
  if (!error) {
    DLOGE("Received attribute values are different for DPU's");
    return kErrorNotSupported;
  }

  // assing only one vector, as all are same
  out_query->assign(query[0].begin(), query[0].end());

  return error;
}

DisplayError DPUColorManager::ColorMgrSetColorTransform(uint32_t length,
                                                    const double *trans_data) {
  DisplayError error = kErrorNone;

  // set color transform for each physical display
  for (auto& color_mgr : color_manager_proxy_list_) {
    error = color_mgr->ColorMgrSetColorTransform(length, trans_data);
    if (error) {
      DLOGE("Failed to set color transform, error=%d", error);
      return error;
    }
  }

  return error;
}

//TBD: Need validation on Dual DPU command mode architecture
bool DPUColorManager::NeedsPartialUpdateDisable() {
  std::set<bool> pu_update_set;

  for (auto& color_mgr : color_manager_proxy_list_) {
    pu_update_set.insert(color_mgr->NeedsPartialUpdateDisable());
  }

  if (pu_update_set.size() != 1) {
    return true;
  }

  return *(pu_update_set.begin());
}

DisplayError DPUColorManager::Commit() {
  DisplayError error = kErrorNone;

  // commit for each physical display
  for (auto& color_mgr : color_manager_proxy_list_) {
    error = color_mgr->Commit();
    if (error) {
      DLOGE("Failed to commit, error=%d", error);
      return error;
    }
  }

  return error;
}

DisplayError DPUColorManager::ColorMgrSetModeWithRenderIntent(int32_t color_mode_id,
                                              const PrimariesTransfer &blend_space,
                                              uint32_t intent) {
  DisplayError error = kErrorNone;
  int i = 0;

  // for each physical display set same color mode with render intent
  for (auto& color_mgr : color_manager_proxy_list_) {
    DLOGI("Set Mode with render intent for core=%d", i++);
    error = color_mgr->ColorMgrSetModeWithRenderIntent(color_mode_id, blend_space, intent);
    if (error) {
      DLOGE("Failed to set color mode with render intent, error=%d", error);
      return error;
    }
  }

  return error;
}

DisplayError DPUColorManager::Validate(DispLayerStack *disp_layer_stack) {
  DisplayError error = kErrorNone;
  int i = 0;

  for (auto& color_mgr : color_manager_proxy_list_) {
    DLOGV("Validate for core=%d", i++);
    error = color_mgr->Validate(disp_layer_stack);
    if (error) {
      DLOGE("Failed to get mode count %d", error);
      return error;
    }
  }

  return error;
}

// game enhance is not supported
bool DPUColorManager::GameEnhanceSupported() {
  return false;
}

// compare and check if the mode list is same for all the physical displays
DisplayError DPUColorManager::CompareColorModeList(std::vector<ColorModeList>& mode_list) {
  DisplayError error = kErrorNone;

  for (int i = 1; i < mode_list.size(); i++) {
    if (mode_list[0].version != mode_list[i].version) {
      return kErrorUndefined;
    }

    if (mode_list[0].list.size() != mode_list[i].list.size()) {
      return kErrorNotSupported;
    } else {
      for (int j = 0; j < mode_list[0].list.size(); j++) {
        if ((mode_list[0].list[j].gamut != mode_list[i].list[j].gamut) ||
            (mode_list[0].list[j].gamma != mode_list[i].list[j].gamma) ||
            (mode_list[0].list[j].intent != mode_list[i].list[j].intent) ||
            (mode_list[0].list[j].intent_name != mode_list[i].list[j].intent_name))
            return kErrorNotSupported;
      }
    }
  }

  return error;
}

DisplayError DPUColorManager::ColorMgrGetStcModes(ColorModeList *out_mode_list) {
  DisplayError error = kErrorNone;
  ColorModeList mode;
  std::vector<ColorModeList> mode_list;

  // loop throught all the physical displays and get mode list for each
  for (auto& color_mgr : color_manager_proxy_list_) {
    error = color_mgr->ColorMgrGetStcModes(&mode);
    if (error) {
      DLOGE("Failed to get mode %d", error);
      return error;
    }

    mode_list.push_back(mode);

    // To avoid push_back into same local mode list by STC Manager
    mode.list.clear();
  }

  // compare and check if the mode list is same for all the physical displays
  if (color_manager_proxy_list_.size() > 1) {
    error = CompareColorModeList(mode_list);
    if (error) {
      DLOGE("Received Color Mode list is different, error=%d", error);
      return error;
    }
  }

  // mode_list conatins same modes. Copy 1st mode to out mode.
  out_mode_list->list = mode_list[0].list;

  return error;
}

DisplayError DPUColorManager::ColorMgrSetStcMode(const ColorMode &color_mode) {
  DisplayError error = kErrorNone;
  int i = 0;

  // set color mode for each physical display
  for (auto& color_mgr : color_manager_proxy_list_) {
    DLOGV("Set STC Mode for core=%d", i++);
    error = color_mgr->ColorMgrSetStcMode(color_mode);
    if (error) {
      DLOGE("Failed to set stc mode, error=%d", error);
      return error;
    }
  }

  return error;
}

DisplayError DPUColorManager::Prepare() {
  DisplayError error = kErrorNone;
  int i = 0;

  for (auto& color_mgr : color_manager_proxy_list_) {
    DLOGV("calling prepare for core=%d", i++);
    error = color_mgr->Prepare();
    if (error) {
      DLOGE("Failed to prepare, error=%d", error);
      return error;
    }
  }

  return error;
}

bool DPUColorManager::IsValidateNeeded() {
  int color_mgr_cnt = color_manager_proxy_list_.size();
  vector<bool> needed(color_mgr_cnt, false);  // assigning all to false

  for (int i = 0; i < color_mgr_cnt; i++) {
    DLOGV("Check if validation needed for core=%d", i);
    needed[i] = color_manager_proxy_list_[i]->IsValidateNeeded();
  }

  for (int i = 1; i < color_mgr_cnt; i++) {
    if (needed[0] != needed[i]) {
      DLOGW("Need validate for DPU's are different, DPU0=%d, DPU%d=%d",
                                    needed[0], i, needed[i]);
    }
  }

  return needed[0];
}

DisplayError DPUColorManager::ConfigureCWBDither(CwbConfig *cwb_cfg, bool free_data) {
  return kErrorNotSupported;
}

// TBD: Not validated, requires validation
DisplayError DPUColorManager::NotifyDisplayCalibrationMode(bool in_calibration) {
  DisplayError error = kErrorNone;
  int i = 0;

  for (auto& color_mgr : color_manager_proxy_list_) {
    DLOGV("Notify Display Calibration Mode for core=%d", i++);
    error = color_mgr->NotifyDisplayCalibrationMode(in_calibration);
    if (error) {
      DLOGE("Failed to Notify Display Calibration Mode, error=%d", error);
      return error;
    }
  }

  return error;
}

DisplayError DPUColorManager::ColorMgrSetLtmPccConfig(void *pcc_input, size_t size) {
  return kErrorNotSupported;
}

DisplayError DPUColorManager::ColorMgrSetSprIntf(std::shared_ptr<SPRIntf> spr_intf) {
  return kErrorNotSupported;
}

DisplayError DPUColorManager::ColorMgrIdleFallback(bool idle_fallback_hint) {
  return kErrorNotSupported;
}

// TBD: Should remove this legacy API?
DisplayError DPUColorManager::ApplyDefaultDisplayMode() {
  return kErrorNotSupported;
}

// TBD: Should remove this legacy API?
DisplayError DPUColorManager::ColorMgrGetDefaultModeID(int32_t *mode_id) {
  return kErrorNotSupported;
}

// TBD: Should remove this legacy API?
DisplayError DPUColorManager::ColorMgrCombineColorModes() {
  return kErrorNotSupported;
}

#undef __CLASS__
#define __CLASS__ "ColorMgrFactoryIntfImpl"

static ColorMgrFactoryIntfImpl color_mgr_impl;

ColorManagerIntf* ColorMgrFactoryIntfImpl::CreateColorManagerIntf(SDMDisplayType type,
                                                    DPUCoreMux *dpu_core_mux,
                                                    DisplayDeviceContext &display_device_ctx,
                                                    DisplayClientContext &display_client_ctx,
                                                    DppsControlInterface *dpps_intf,
                                                    DisplayInterface *disp_intf,
                                                    vector<HWResourceInfo> &hw_res_info,
                                                    DisplayId display_id_info) {
  ColorManagerIntf* ptr;

  if (!dpu_core_mux) {
    DLOGE("Null interface for dpu core mux");
    return nullptr;
  }

  // extract core id, logical display ID from DisplayId class
  std::bitset<8> core_id_bit_set(display_id_info.GetCoreIdMap());
  uint32_t display_id = display_id_info.GetDisplayId();
  uint32_t core_id = 0;

  // Depending of core count either create DPUColorManager or create ColorManagerProxy
  if (core_id_bit_set.count() > 1) {
    DLOGV("Creating CreateDpuColorManager");
    ptr = DPUColorManager::CreateDpuColorManager(type, dpu_core_mux,
                                                  display_device_ctx,
                                                  display_client_ctx,
                                                  dpps_intf, disp_intf,
                                                  hw_res_info,
                                                  display_id_info);
  } else {
    core_id = ColorManagerProxy::getCoreId(display_id);
    DLOGV("Creating CreateColorManagerProxy for core_id=%d", core_id);
    ptr = ColorManagerProxy::CreateColorManagerProxy(type, dpu_core_mux,
                                                    display_client_ctx.display_attributes,
                                                    display_client_ctx.hw_panel_info,
                                                    dpps_intf, disp_intf,
                                                    hw_res_info[core_id],
                                                    display_id);
  }

  return ptr;
}

ColorMgrFactoryIntf* GetColorMgrFactoryIntf() {
  return &color_mgr_impl;
}

}  // namespace sdm
