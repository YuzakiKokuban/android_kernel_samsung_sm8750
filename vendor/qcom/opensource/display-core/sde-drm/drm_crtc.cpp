/*
* Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
*    * Neither the name of The Linux Foundation nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.

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
* ​​​​​Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <stdint.h>
#include <stdlib.h>
#include <drm.h>
#include <display/drm/sde_drm.h>
#include <drm_logger.h>
#include <drm/drm_fourcc.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

#include "drm_utils.h"
#include "drm_crtc.h"
#include "drm_property.h"

namespace sde_drm {

using std::string;
using std::stringstream;
using std::unique_ptr;
using std::map;
using std::mutex;
using std::lock_guard;
using std::pair;
using std::vector;

// CRTC Security Levels
static uint8_t SECURE_NON_SECURE = 0;
static uint8_t SECURE_ONLY = 1;

// CWB Capture Modes
static uint8_t CAPTURE_MIXER_OUT = 0;
static uint8_t CAPTURE_DSPP_OUT = 1;
static uint8_t CAPTURE_DEMURA_OUT = 2;

// Idle PC states
static uint8_t IDLE_PC_STATE_NONE = 0;
static uint8_t IDLE_PC_STATE_ENABLE = 1;
static uint8_t IDLE_PC_STATE_DISABLE = 2;

// CRTC Cache States
static uint8_t CACHE_STATE_DISABLED = 0;
static uint8_t CACHE_STATE_ENABLED = 1;

// VM Request states
static uint8_t VM_REQ_STATE_NONE = 0;
static uint8_t VM_REQ_STATE_RELEASE = 1;
static uint8_t VM_REQ_STATE_ACQUIRE = 2;

static void PopulateSecurityLevels(drmModePropertyRes *prop) {
  static bool security_levels_populated = false;
  if (!security_levels_populated) {
    for (auto i = 0; i < prop->count_enums; i++) {
      string enum_name(prop->enums[i].name);
      if (enum_name == "sec_and_non_sec") {
        SECURE_NON_SECURE = prop->enums[i].value;
      } else if (enum_name == "sec_only") {
        SECURE_ONLY = prop->enums[i].value;
      }
    }
    security_levels_populated = true;
  }
}

static void PopulateCWbCaptureModes(drmModePropertyRes *prop,
                                    std::vector<DRMCWbCaptureMode> *tap_points) {
  static bool capture_modes_populated = false;
  if (!capture_modes_populated) {
    for (auto i = 0; i < prop->count_enums; i++) {
      string enum_name(prop->enums[i].name);
      if (enum_name == "capture_mixer_out") {
        CAPTURE_MIXER_OUT = prop->enums[i].value;
        tap_points->push_back(DRMCWbCaptureMode::MIXER_OUT);
      } else if (enum_name == "capture_pp_out") {
        CAPTURE_DSPP_OUT = prop->enums[i].value;
        tap_points->push_back(DRMCWbCaptureMode::DSPP_OUT);
      } else if (enum_name == "capture_demura_out") {
        CAPTURE_DEMURA_OUT = prop->enums[i].value;
        tap_points->push_back(DRMCWbCaptureMode::DEMURA_OUT);
      }
    }
    capture_modes_populated = true;
  }
}

static void PopulateIdlePCStates(drmModePropertyRes *prop) {
  static bool idle_pc_state_populated = false;
  if (!idle_pc_state_populated) {
    for (auto i = 0; i < prop->count_enums; i++) {
      string enum_name(prop->enums[i].name);
      if (enum_name == "idle_pc_none") {
        IDLE_PC_STATE_NONE = prop->enums[i].value;
      } else if (enum_name == "idle_pc_enable") {
        IDLE_PC_STATE_ENABLE = prop->enums[i].value;
      } else if (enum_name == "idle_pc_disable") {
        IDLE_PC_STATE_DISABLE = prop->enums[i].value;
      }
    }
    idle_pc_state_populated = true;
  }
}

static void PopulateCacheStates(drmModePropertyRes *prop) {
  static bool cache_states_populated = false;
  if (!cache_states_populated) {
    for (auto i = 0; i < prop->count_enums; i++) {
      string enum_name(prop->enums[i].name);
      if (enum_name == "cache_state_disabled") {
        CACHE_STATE_DISABLED = prop->enums[i].value;
      } else if (enum_name == "cache_state_enabled") {
        CACHE_STATE_ENABLED = prop->enums[i].value;
      }
    }
    cache_states_populated = true;
  }
}

static void PopulateVMRequestStates(drmModePropertyRes *prop) {
  static bool idle_pc_state_populated = false;
  if (!idle_pc_state_populated) {
    for (auto i = 0; i < prop->count_enums; i++) {
      string enum_name(prop->enums[i].name);
      if (enum_name == "vm_req_none") {
        VM_REQ_STATE_NONE = prop->enums[i].value;
      } else if (enum_name == "vm_req_release") {
        VM_REQ_STATE_RELEASE = prop->enums[i].value;
      } else if (enum_name == "vm_req_acquire") {
        VM_REQ_STATE_ACQUIRE = prop->enums[i].value;
      }
    }
    idle_pc_state_populated = true;
  }
}

#define __CLASS__ "DRMCrtcManager"

void DRMCrtcManager::Init(drmModeRes *resource) {
  lock_guard<mutex> lock(lock_);
  for (int i = 0; i < resource->count_crtcs; i++) {
    unique_ptr<DRMCrtc> crtc(new DRMCrtc(fd_, i));
    drmModeCrtc *libdrm_crtc = drmModeGetCrtc(fd_, resource->crtcs[i]);
    if (libdrm_crtc) {
      crtc->InitAndParse(libdrm_crtc);
      crtc_pool_[resource->crtcs[i]] = std::move(crtc);
    } else {
      DRM_LOGE("Critical error: drmModeGetCrtc() failed for crtc %d.", resource->crtcs[i]);
    }
  }
}

void DRMCrtcManager::DumpByID(uint32_t id) {
  lock_guard<mutex> lock(lock_);
  crtc_pool_.at(id)->Dump();
}

void DRMCrtcManager::DumpAll() {
  lock_guard<mutex> lock(lock_);
  for (auto &crtc : crtc_pool_) {
    crtc.second->Dump();
  }
}

void DRMCrtcManager::Perform(DRMOps code, uint32_t obj_id, drmModeAtomicReq *req,
                             va_list args) {
  lock_guard<mutex> lock(lock_);
  auto it = crtc_pool_.find(obj_id);
  if (it == crtc_pool_.end()) {
    DRM_LOGE("Invalid crtc id %d", obj_id);
    return;
  }

  if (code == DRMOps::CRTC_SET_DEST_SCALER_CONFIG) {
    if (crtc_pool_.at(obj_id)->ConfigureScalerLUT(req, dir_lut_blob_id_, cir_lut_blob_id_,
                                                  sep_lut_blob_id_)) {
      DRM_LOGD("CRTC %d: Configuring scaler LUTs", obj_id);
    }
  }

  it->second->Perform(code, req, args);
}

void DRMCrtcManager::SetScalerLUT(const DRMScalerLUTInfo &lut_info) {
  lock_guard<mutex> lock(lock_);
  // qseed3lite lut is hardcoded in HW. No need to program from sw.
  DRMCrtcInfo info;
  crtc_pool_.begin()->second->GetInfo(&info);
  if (info.qseed_version == QSEEDVersion::V3LITE) {
    return;
  }

  if (lut_info.dir_lut_size) {
    drmModeCreatePropertyBlob(fd_, reinterpret_cast<void *>(lut_info.dir_lut),
                              lut_info.dir_lut_size, &dir_lut_blob_id_);
  }
  if (lut_info.cir_lut_size) {
    drmModeCreatePropertyBlob(fd_, reinterpret_cast<void *>(lut_info.cir_lut),
                              lut_info.cir_lut_size, &cir_lut_blob_id_);
  }
  if (lut_info.sep_lut_size) {
    drmModeCreatePropertyBlob(fd_, reinterpret_cast<void *>(lut_info.sep_lut),
                              lut_info.sep_lut_size, &sep_lut_blob_id_);
  }
}

void DRMCrtcManager::UnsetScalerLUT() {
  lock_guard<mutex> lock(lock_);
  if (dir_lut_blob_id_) {
    drmModeDestroyPropertyBlob(fd_, dir_lut_blob_id_);
    dir_lut_blob_id_ = 0;
  }
  if (cir_lut_blob_id_) {
    drmModeDestroyPropertyBlob(fd_, cir_lut_blob_id_);
    cir_lut_blob_id_ = 0;
  }
  if (sep_lut_blob_id_) {
    drmModeDestroyPropertyBlob(fd_, sep_lut_blob_id_);
    sep_lut_blob_id_ = 0;
  }
}

int DRMCrtcManager::GetCrtcInfo(uint32_t crtc_id, DRMCrtcInfo *info) {
  lock_guard<mutex> lock(lock_);
  if (crtc_id == 0) {
    crtc_pool_.begin()->second->GetInfo(info);
  } else {
    auto iter = crtc_pool_.find(crtc_id);
    if (iter ==  crtc_pool_.end()) {
      DRM_LOGE("Invalid crtc id %d", crtc_id);
      return -ENODEV;
    } else {
      iter->second->GetInfo(info);
    }
  }

  return 0;
}

void DRMCrtcManager::GetPPInfo(uint32_t crtc_id, DRMPPFeatureInfo *info) {
  lock_guard<mutex> lock(lock_);
  auto it = crtc_pool_.find(crtc_id);
  if (it == crtc_pool_.end()) {
    DRM_LOGE("Invalid crtc id %d", crtc_id);
    return;
  }

  it->second->GetPPInfo(info);
}

int DRMCrtcManager::Reserve(const std::set<uint32_t> &possible_crtc_indices,
                             DRMDisplayToken *token) {
  lock_guard<mutex> lock(lock_);
  for (auto &item : crtc_pool_) {
    if (item.second->GetStatus() == DRMStatus::FREE) {
      if (possible_crtc_indices.find(item.second->GetIndex()) != possible_crtc_indices.end()) {
        item.second->Lock();
        token->crtc_id = item.first;
        token->crtc_index = item.second->GetIndex();
        return 0;
      }
    }
  }

  return -ENODEV;
}

void DRMCrtcManager::Free(DRMDisplayToken *token) {
  lock_guard<mutex> lock(lock_);
  crtc_pool_.at(token->crtc_id)->Unlock();
  token->crtc_id = 0;
  token->crtc_index = 0;
}

void DRMCrtcManager::PostValidate(uint32_t crtc_id, bool success) {
  lock_guard<mutex> lock(lock_);
  crtc_pool_.at(crtc_id)->PostValidate(success);
}

void DRMCrtcManager::PostCommit(uint32_t crtc_id, bool success) {
  lock_guard<mutex> lock(lock_);
  crtc_pool_.at(crtc_id)->PostCommit(success);
}

void DRMCrtcManager::GetCrtcList(std::vector<uint32_t> *crtc_ids) {
  lock_guard<mutex> lock(lock_);
  for (auto &crtc : crtc_pool_) {
    crtc_ids->emplace_back(crtc.first);
  }
}

// ==============================================================================================//

#undef __CLASS__
#define __CLASS__ "DRMCrtc"

DRMCrtc::~DRMCrtc() {
  if (drm_crtc_) {
    drmModeFreeCrtc(drm_crtc_);
  }
}

void DRMCrtc::ParseProperties() {
  drmModeObjectProperties *props =
    drmModeObjectGetProperties(fd_, drm_crtc_->crtc_id, DRM_MODE_OBJECT_CRTC);
  if (!props || !props->props || !props->prop_values) {
    drmModeFreeObjectProperties(props);
    return;
  }

  for (uint32_t j = 0; j < props->count_props; j++) {
    drmModePropertyRes *info = drmModeGetProperty(fd_, props->props[j]);
    if (!info) {
      continue;
    }

    string property_name(info->name);
    DRMProperty prop_enum = prop_mgr_.GetPropertyEnum(property_name);
    if (prop_enum == DRMProperty::INVALID) {
      DRM_LOGD("DRMProperty %s missing from global property mapping", info->name);
      drmModeFreeProperty(info);
      continue;
    }

    if (prop_enum == DRMProperty::NOISE_LAYER_V1) {
      crtc_info_.has_noise_layer = true;
    }

    if (prop_enum == DRMProperty::SECURITY_LEVEL) {
      PopulateSecurityLevels(info);
    }

    if (prop_enum == DRMProperty::CAPTURE_MODE) {
      crtc_info_.concurrent_writeback = true;
      PopulateCWbCaptureModes(info, &crtc_info_.tap_points);
    }

    if (prop_enum == DRMProperty::IDLE_PC_STATE) {
      PopulateIdlePCStates(info);
    }

    if (prop_enum == DRMProperty::CACHE_STATE) {
      PopulateCacheStates(info);
    }

    if (prop_enum == DRMProperty::VM_REQ_STATE) {
      PopulateVMRequestStates(info);
    }

    if (prop_enum == DRMProperty::UBWC_CLK) {
      crtc_info_.has_cesta = true;
    }

    prop_mgr_.SetPropertyId(prop_enum, info->prop_id);
    if (prop_enum == DRMProperty::CAPABILITIES) {
      ParseCapabilities(props->prop_values[j]);
    }
    drmModeFreeProperty(info);
  }

  drmModeFreeObjectProperties(props);
}

void DRMCrtc::ParseCapabilities(uint64_t blob_id) {
  drmModePropertyBlobRes *blob = drmModeGetPropertyBlob(fd_, blob_id);
  if (!blob) {
    return;
  }

  if (!blob->data) {
    return;
  }

  char *fmt_str = new char[blob->length + 1];
  memcpy (fmt_str, blob->data, blob->length);
  fmt_str[blob->length] = '\0';
  stringstream stream(fmt_str);
  DRM_LOGI("stream str %s len %zu blob str %s len %d", stream.str().c_str(), stream.str().length(),
           blob->data, blob->length);
  string line = {};
  string max_blendstages = "max_blendstages=";
  string qseed_type = "qseed_type=";
  string has_src_split = "has_src_split=";
  string sdma_rev = "smart_dma_rev=";
  string core_ib_ff = "core_ib_ff=";
  string dest_scale_prefill_lines = "dest_scale_prefill_lines=";
  string undersized_prefill_lines = "undersized_prefill_lines=";
  string macrotile_prefill_lines = "macrotile_prefill_lines=";
  string yuv_nv12_prefill_lines = "yuv_nv12_prefill_lines=";
  string linear_prefill_lines = "linear_prefill_lines=";
  string downscaling_prefill_lines = "downscaling_prefill_lines=";
  string xtra_prefill_lines = "xtra_prefill_lines=";
  string amortizable_threshold = "amortizable_threshold=";
  string max_bandwidth_low = "max_bandwidth_low=";
  string max_bandwidth_high = "max_bandwidth_high=";
  string max_mdp_clk = "max_mdp_clk=";
  string core_clk_ff = "core_clk_ff=";
  string comp_ratio_rt = "comp_ratio_rt=";
  string comp_ratio_nrt = "comp_ratio_nrt=";
  string hw_version = "hw_version=";
  string solidfill_stages = "dim_layer_v1_max_layers=";
  string has_hdr = "has_hdr=";
  string has_micro_idle = "has_uidle=";
  string min_prefill_lines = "min_prefill_lines=";
  string num_mnocports = "num_mnoc_ports=";
  string mnoc_bus_width = "axi_bus_width=";

  crtc_info_.max_solidfill_stages = 0;  // default _
  string dest_scaler_count = "dest_scaler_count=";
  string max_dest_scale_up = "max_dest_scale_up=";
  string max_dest_scaler_input_width = "max_dest_scaler_input_width=";
  string max_dest_scaler_output_width = "max_dest_scaler_output_width=";
  string sec_ui_blendstage = "sec_ui_blendstage=";
  string vig = "vig=";
  string dma = "dma=";
  string scaling = "scale=";
  string rotation = "inline_rot=";
  string linewidth_constraints = "sspp_linewidth_usecases=";
  string linewidth_values = "sspp_linewidth_values=";
  string limit_constraint = "limit_usecase=";
  string limit_value = "limit_value=";
  string use_baselayer_for_stage = "use_baselayer_for_stage=";
  string ubwc_version = "UBWC version=";
  string spr = "spr=";
  string rc_count = "rc_count=";
  string rc_total_mem_size = "rc_mem_size=";
  string demura_count = "demura_count=";
  string abc_count = "abc_count=";
  string dspp_count = "dspp_count=";
  string skip_inline_rot_threshold="skip_inline_rot_threshold=";
  string dsc_block_count = "dsc_block_count=";
  string cac_version = "cac_version=";
  string ddr_version = "DDR version=";
  string ai_scaler_count = "ai_scaler_count=";

  while (std::getline(stream, line)) {
    if (line.find(max_blendstages) != string::npos) {
      crtc_info_.max_blend_stages = std::stoi(string(line, max_blendstages.length()));
    } else if (line.find(qseed_type) != string::npos) {
      if (string(line, qseed_type.length()) == "qseed2") {
        crtc_info_.qseed_version = QSEEDVersion::V2;
      } else if (string(line, qseed_type.length()) == "qseed3") {
        crtc_info_.qseed_version = QSEEDVersion::V3;
      } else if (string(line, qseed_type.length()) == "qseed3lite") {
        crtc_info_.qseed_version = QSEEDVersion::V3LITE;
      }
    } else if (line.find(has_src_split) != string::npos) {
      crtc_info_.has_src_split = std::stoi(string(line, has_src_split.length()));
    } else if (line.find(sdma_rev) != string::npos) {
      if (string(line, sdma_rev.length()) == "smart_dma_v2p5")
        crtc_info_.smart_dma_rev = SmartDMARevision::V2p5;
      else if (string(line, sdma_rev.length()) == "smart_dma_v2")
        crtc_info_.smart_dma_rev = SmartDMARevision::V2;
      else if (string(line, sdma_rev.length()) == "smart_dma_v1")
        crtc_info_.smart_dma_rev = SmartDMARevision::V1;
    } else if (line.find(core_ib_ff) != string::npos) {
      crtc_info_.ib_fudge_factor = std::stof(string(line, core_ib_ff.length()));
    } else if (line.find(dest_scale_prefill_lines) != string::npos) {
      crtc_info_.dest_scale_prefill_lines =
        std::stoi(string(line, dest_scale_prefill_lines.length()));
    } else if (line.find(undersized_prefill_lines) != string::npos) {
      crtc_info_.undersized_prefill_lines =
        std::stoi(string(line, undersized_prefill_lines.length()));
    } else if (line.find(macrotile_prefill_lines) != string::npos) {
      crtc_info_.macrotile_prefill_lines =
        std::stoi(string(line, macrotile_prefill_lines.length()));
    } else if (line.find(yuv_nv12_prefill_lines) != string::npos) {
      crtc_info_.nv12_prefill_lines = std::stoi(string(line, yuv_nv12_prefill_lines.length()));
    } else if (line.find(linear_prefill_lines) != string::npos) {
      crtc_info_.linear_prefill_lines = std::stoi(string(line, linear_prefill_lines.length()));
    } else if (line.find(downscaling_prefill_lines) != string::npos) {
      crtc_info_.downscale_prefill_lines =
        std::stoi(string(line, downscaling_prefill_lines.length()));
    } else if (line.find(xtra_prefill_lines) != string::npos) {
      crtc_info_.extra_prefill_lines = std::stoi(string(line, xtra_prefill_lines.length()));
    } else if (line.find(amortizable_threshold) != string::npos) {
      crtc_info_.amortized_threshold = std::stoi(string(line, amortizable_threshold.length()));
    } else if (line.find(max_bandwidth_low) != string::npos) {
      crtc_info_.max_bandwidth_low = std::stoull(string(line, max_bandwidth_low.length()));
    } else if (line.find(max_bandwidth_high) != string::npos) {
      crtc_info_.max_bandwidth_high = std::stoull(string(line, max_bandwidth_high.length()));
    } else if (line.find(max_mdp_clk) != string::npos) {
      crtc_info_.max_sde_clk = std::stoi(string(line, max_mdp_clk.length()));
    } else if (line.find(core_clk_ff) != string::npos) {
      crtc_info_.clk_fudge_factor = std::stof(string(line, core_clk_ff.length()));
    } else if (line.find(comp_ratio_rt) != string::npos) {
      ParseCompRatio(line.substr(comp_ratio_rt.length()), true);
    } else if (line.find(comp_ratio_nrt) != string::npos) {
      ParseCompRatio(line.substr(comp_ratio_nrt.length()), false);
    } else if (line.find(hw_version) != string::npos) {
      crtc_info_.hw_version =
        static_cast<uint32_t>(std::stoull(string(line, hw_version.length())));
    } else if (line.find(solidfill_stages) != string::npos) {
      crtc_info_.max_solidfill_stages =  std::stoi(string(line, solidfill_stages.length()));
    } else if (line.find(dest_scaler_count) != string::npos) {
      crtc_info_.dest_scaler_count = std::stoi(string(line, dest_scaler_count.length()));
    } else if (line.find(max_dest_scale_up) != string::npos) {
      crtc_info_.max_dest_scale_up = std::stoi(string(line, max_dest_scale_up.length()));
    } else if (line.find(max_dest_scaler_input_width) != string::npos) {
      crtc_info_.max_dest_scaler_input_width =
                          std::stoi(string(line, max_dest_scaler_input_width.length()));
    } else if (line.find(max_dest_scaler_output_width) != string::npos) {
      crtc_info_.max_dest_scaler_output_width =
                         std::stoi(string(line, max_dest_scaler_output_width.length()));
    } else if (line.find(has_hdr) != string::npos) {
      crtc_info_.has_hdr = std::stoi(string(line, has_hdr.length()));
    } else if (line.find(min_prefill_lines) != string::npos) {
      crtc_info_.min_prefill_lines = std::stoi(string(line, min_prefill_lines.length()));
    } else if (line.find(sec_ui_blendstage) != string::npos) {
      crtc_info_.secure_disp_blend_stage = std::stoi(string(line, (sec_ui_blendstage).length()));
    } else if (line.find(num_mnocports) != string::npos) {
      crtc_info_.num_mnocports = std::stoi(string(line, num_mnocports.length()));
    } else if (line.find(mnoc_bus_width) != string::npos) {
      crtc_info_.mnoc_bus_width = std::stoi(string(line, mnoc_bus_width.length()));
    } else if (line.find(linewidth_constraints) != string::npos) {
      crtc_info_.line_width_constraints_count =
                            std::stoi(string(line, (linewidth_constraints).length()));
    } else if (line.find(vig) != string::npos) {
      crtc_info_.vig_limit_index = std::stoi(string(line, (vig).length()));
    } else if (line.find(dma) != string::npos) {
      crtc_info_.dma_limit_index = std::stoi(string(line, (dma).length()));
    } else if (line.find(scaling) != string::npos) {
      crtc_info_.scaling_limit_index = std::stoi(string(line, (scaling).length()));
    } else if (line.find(rotation) != string::npos) {
      crtc_info_.rotation_limit_index = std::stoi(string(line, (rotation).length()));
    } else if (line.find(linewidth_values) != string::npos) {
      uint32_t num_linewidth_values = std::stoi(string(line, (linewidth_values).length()));
      vector< pair <uint32_t,uint32_t> > constraint_vector;
      for (uint32_t i = 0; i < num_linewidth_values; i++) {
        uint32_t constraint = 0;
        uint32_t value  = 0;
        std::getline(stream, line);
        if (line.find(limit_constraint) != string::npos) {
          constraint = std::stoi(string(line, (limit_constraint).length()));
        }
        std::getline(stream, line);
        if (line.find(limit_value) != string::npos) {
          value = std::stoi(string(line, (limit_value).length()));
        }
        if (value) {
          constraint_vector.push_back(std::make_pair(constraint,value));
        }
      }
      crtc_info_.line_width_limits = std::move(constraint_vector);
    } else if (line.find(has_micro_idle) != string::npos) {
      crtc_info_.has_micro_idle = std::stoi(string(line, (has_micro_idle).length()));
    } else if (line.find(use_baselayer_for_stage) != string::npos) {
      crtc_info_.use_baselayer_for_stage =
                         std::stoi(string(line, use_baselayer_for_stage.length()));
    } else if (line.find(ubwc_version) != string::npos) {
      crtc_info_.ubwc_version = (std::stoi(string(line, ubwc_version.length()))) >> 28;
    } else if (line.find(spr) != string::npos) {
      crtc_info_.has_spr = std::stoi(string(line, spr.length())) == -1 ? false: true;
    } else if (line.find(rc_count) != string::npos) {
      crtc_info_.rc_count = std::stoi(string(line, rc_count.length()));
    } else if (line.find(rc_total_mem_size) != string::npos) {
      crtc_info_.rc_total_mem_size = std::stoi(string(line, rc_total_mem_size.length()));
    } else if (line.find(demura_count) != string::npos) {
      crtc_info_.demura_count = std::stoi(string(line, demura_count.length()));
    } else if (line.find(abc_count) != string::npos) {
      crtc_info_.abc_count = std::stoi(string(line, abc_count.length()));
    } else if (line.find(dspp_count) != string::npos) {
      crtc_info_.dspp_count = std::stoi(string(line, dspp_count.length()));
    } else if (line.find(skip_inline_rot_threshold) != string::npos) {
      crtc_info_.skip_inline_rot_threshold =
        std::stoi(string(line, skip_inline_rot_threshold.length()));
    } else if (line.find(dsc_block_count) != string::npos) {
      crtc_info_.dsc_block_count = std::stoi(string(line, dsc_block_count.length()));
    } else if (line.find(cac_version) != string::npos) {
      if (string(line, cac_version.length()) == "cac_v1") {
        crtc_info_.cac_version = CacVersion::V1;
      } else if (string(line, cac_version.length()) == "cac_v2") {
        crtc_info_.cac_version = CacVersion::V2;
      } else if (string(line, cac_version.length()) == "cac_loopback") {
        crtc_info_.cac_version = CacVersion::Loopback;
      }
    } else if (line.find(ddr_version) != string::npos) {
      if (string(line, ddr_version.length()) == "DDR4") {
        crtc_info_.ddr_version = DDRVersion::kDDRVersion4;
      } else if(string(line, ddr_version.length()) == "DDR5") {
        crtc_info_.ddr_version = DDRVersion::kDDRVersion5;
      } else if(string(line, ddr_version.length()) == "DDR5X") {
        crtc_info_.ddr_version = DDRVersion::kDDRVersion5x;
      }
    } else if (line.find(ai_scaler_count) != string::npos) {
      crtc_info_.ai_scaler_count = std::stoi(string(line, ai_scaler_count.length()));
    }
  }
  drmModeFreePropertyBlob(blob);
  delete[] fmt_str;
}

void DRMCrtc::ParseCompRatio(string line, bool real_time) {
  CompRatioMap &comp_ratio_map =
    real_time ? crtc_info_.comp_ratio_rt_map : crtc_info_.comp_ratio_nrt_map;
  std::vector<string> format_cr_list;

  Tokenize(line, &format_cr_list, ' ');

  for (uint32_t i = 0; i < format_cr_list.size(); i++) {
    std::vector<string> format_cr;
    Tokenize(format_cr_list.at(i), &format_cr, '/');
    std::string format = format_cr.at(0);
    uint64_t vendor_code = stoi(format_cr.at(1));
    uint64_t fmt_modifier = stoi(format_cr.at(2));
    float comp_ratio = std::stof(format_cr.at(3));
    uint64_t modifier = 0;

    if (vendor_code == DRM_FORMAT_MOD_VENDOR_QCOM) {
      // Macro from drm_fourcc.h to form modifier
      modifier = fourcc_mod_code(QCOM, fmt_modifier);
    }

    std::pair<uint32_t, uint64_t> drm_format =
      std::make_pair(fourcc_code(format[0], format[1], format[2], format[3]), modifier);
    comp_ratio_map.insert(std::make_pair(drm_format, comp_ratio));
  }
}

void DRMCrtc::GetInfo(DRMCrtcInfo *info) {
  *info = crtc_info_;
}

void DRMCrtc::Lock() {
  status_ = DRMStatus::BUSY;
}

void DRMCrtc::Unlock() {
  if (mode_blob_id_) {
    drmModeDestroyPropertyBlob(fd_, mode_blob_id_);
    mode_blob_id_ = 0;
  }

  tmp_prop_val_map_.clear();
  committed_prop_val_map_.clear();
  status_ = DRMStatus::FREE;
}

void DRMCrtc::SetModeBlobID(uint64_t blob_id) {
  if (mode_blob_id_) {
    drmModeDestroyPropertyBlob(fd_, mode_blob_id_);
  }

  mode_blob_id_ = blob_id;
}

void DRMCrtc::InitAndParse(drmModeCrtc *crtc) {
  drm_crtc_ = crtc;
  ParseProperties();
  pp_mgr_ = std::unique_ptr<DRMPPManager>(new DRMPPManager(fd_));
  pp_mgr_->Init(prop_mgr_, DRM_MODE_OBJECT_CRTC);
}

void DRMCrtc::Perform(DRMOps code, drmModeAtomicReq *req, va_list args) {
  uint32_t obj_id = drm_crtc_->crtc_id;

  switch (code) {
    case DRMOps::CRTC_SET_MODE: {
      drmModeModeInfo *mode = va_arg(args, drmModeModeInfo *);
      uint32_t prop_id = prop_mgr_.GetPropertyId(DRMProperty::MODE_ID);
      uint32_t blob_id = 0;

      if (mode) {
        if (drmModeCreatePropertyBlob(fd_, (const void *)mode, sizeof(drmModeModeInfo), &blob_id)) {
          DRM_LOGE("drmModeCreatePropertyBlob failed for CRTC_SET_MODE, crtc %d", obj_id);
          return;
        }
      }

      AddProperty(req, obj_id, prop_id, blob_id, true /* cache */, tmp_prop_val_map_);
      SetModeBlobID(blob_id);
      DRM_LOGD("CRTC %d: Set mode %s", obj_id, mode ? mode->name : "null");
    } break;

    case DRMOps::CRTC_SET_OUTPUT_FENCE_OFFSET: {
      uint32_t offset = va_arg(args, uint32_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::OUTPUT_FENCE_OFFSET),
                  offset, true /* cache */, tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_SET_CORE_CLK: {
      uint32_t core_clk = va_arg(args, uint32_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::CORE_CLK), core_clk, true /* cache */,
                  tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_SET_CORE_AB: {
      uint64_t core_ab = va_arg(args, uint64_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::CORE_AB), core_ab, true /* cache */,
                  tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_SET_CORE_IB: {
      uint64_t core_ib = va_arg(args, uint64_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::CORE_IB), core_ib, true /* cache */,
                  tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_SET_LLCC_AB: {
      uint64_t llcc_ab = va_arg(args, uint64_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::LLCC_AB), llcc_ab, true /* cache */,
                  tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_SET_LLCC_IB: {
      uint64_t llcc_ib = va_arg(args, uint64_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::LLCC_IB), llcc_ib, true /* cache */,
                  tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_SET_DRAM_AB: {
      uint64_t dram_ab = va_arg(args, uint64_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::DRAM_AB), dram_ab, true /* cache */,
                  tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_SET_DRAM_IB: {
      uint64_t dram_ib = va_arg(args, uint64_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::DRAM_IB), dram_ib, true /* cache */,
                  tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_SET_ROT_PREFILL_BW: {
      uint64_t rot_bw = va_arg(args, uint64_t);
      drmModeAtomicAddProperty(req, obj_id,
                               prop_mgr_.GetPropertyId(DRMProperty::ROT_PREFILL_BW), rot_bw);
    }; break;

    case DRMOps::CRTC_SET_ROT_CLK: {
      uint32_t rot_clk = va_arg(args, uint32_t);
      AddProperty(req, obj_id,
                  prop_mgr_.GetPropertyId(DRMProperty::ROT_CLK), rot_clk, true /* cache */,
                  tmp_prop_val_map_);
    }; break;

    case DRMOps::CRTC_GET_RELEASE_FENCE: {
      int64_t *fence = va_arg(args, int64_t *);
      *fence = -1;
      uint32_t prop_id = prop_mgr_.GetPropertyId(DRMProperty::OUTPUT_FENCE);
      AddProperty(req, obj_id, prop_id, reinterpret_cast<uint64_t>(fence), false /* cache */,
                  tmp_prop_val_map_);
    } break;

    case DRMOps::CRTC_SET_ACTIVE: {
      uint32_t enable = va_arg(args, uint32_t);
      AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::ACTIVE), enable,
                  true /* cache */, tmp_prop_val_map_);
      DRM_LOGD("CRTC %d: Set active %d", obj_id, enable);
      if (enable == 0) {
        ClearVotesCache();
      }
    } break;

    case DRMOps::CRTC_SET_POST_PROC: {
      DRMPPFeatureInfo *data = va_arg(args, DRMPPFeatureInfo*);
      if (data)
        pp_mgr_->SetPPFeature(req, obj_id, *data);
      DRM_LOGD("CRTC %d: Set post proc", obj_id);
    } break;

    case DRMOps::CRTC_SET_ROI: {
      uint32_t num_roi = va_arg(args, uint32_t);
      DRMRect *crtc_rois = va_arg(args, DRMRect *);
      DRMRect *spr_rois = va_arg(args, DRMRect *);
      SetROI(req, obj_id, num_roi, crtc_rois, spr_rois);
    } break;

    case DRMOps::CRTC_SET_SECURITY_LEVEL: {
      int security_level = va_arg(args, int);
      uint32_t crtc_security_level = SECURE_NON_SECURE;
      if (security_level == (int)DRMSecurityLevel::SECURE_ONLY) {
        crtc_security_level = SECURE_ONLY;
      }
      AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::SECURITY_LEVEL),
                  crtc_security_level, true /* cache */, tmp_prop_val_map_);
    } break;

    case DRMOps::CRTC_SET_SOLIDFILL_STAGES: {
      uint64_t dim_stages = va_arg(args, uint64_t);
      const std::vector<DRMSolidfillStage> *solid_fills =
        reinterpret_cast <std::vector <DRMSolidfillStage> *> (dim_stages);
      SetSolidfillStages(req, obj_id, solid_fills);
    } break;

    case DRMOps::CRTC_SET_NOISELAYER_CONFIG: {
      uint64_t data = va_arg(args, uint64_t);
      const DRMNoiseLayerConfig *noise_cfg = reinterpret_cast<DRMNoiseLayerConfig *>(data);
      SetNoiseLayerConfig(req, obj_id, noise_cfg);
    } break;

    case DRMOps::CRTC_SET_IDLE_TIMEOUT: {
      uint32_t timeout_ms = va_arg(args, uint32_t);
      AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::IDLE_TIME),
                  timeout_ms, true /* cache */, tmp_prop_val_map_);
    } break;

    case DRMOps::CRTC_SET_DEST_SCALER_CONFIG: {
      uint32_t prop_id = prop_mgr_.GetPropertyId(DRMProperty::DEST_SCALER);
      uint64_t dest_scaler = va_arg(args, uint64_t);
      sde_drm_dest_scaler_data *ds_data = reinterpret_cast<sde_drm_dest_scaler_data *>
                                           (dest_scaler);
      dest_scale_data_ = *ds_data;
      AddProperty(req, obj_id, prop_id,
                  reinterpret_cast<uint64_t>(&dest_scale_data_), false /* cache */,
                  tmp_prop_val_map_);
    } break;

    case DRMOps::CRTC_SET_CAPTURE_MODE: {
      int capture_mode = va_arg(args, int);
      uint32_t cwb_capture_mode = CAPTURE_MIXER_OUT;
      if (capture_mode == (int)DRMCWbCaptureMode::DSPP_OUT) {
        cwb_capture_mode = CAPTURE_DSPP_OUT;
      } else if (capture_mode == (int)DRMCWbCaptureMode::DEMURA_OUT) {
        cwb_capture_mode = CAPTURE_DEMURA_OUT;
      }
      uint32_t prop_id = prop_mgr_.GetPropertyId(DRMProperty::CAPTURE_MODE);
      AddProperty(req, obj_id, prop_id, cwb_capture_mode, true /* cache */, tmp_prop_val_map_);
    } break;

    case DRMOps::CRTC_SET_IDLE_PC_STATE: {
      if (!prop_mgr_.IsPropertyAvailable(DRMProperty::IDLE_PC_STATE)) {
        return;
      }
      int drm_idle_pc_state = va_arg(args, int);
      uint32_t idle_pc_state = IDLE_PC_STATE_NONE;
      switch (drm_idle_pc_state) {
        case static_cast<int>(DRMIdlePCState::ENABLE):
          idle_pc_state = IDLE_PC_STATE_ENABLE;
          break;
        case static_cast<int>(DRMIdlePCState::DISABLE):
          idle_pc_state = IDLE_PC_STATE_DISABLE;
          break;
        default:
          idle_pc_state = IDLE_PC_STATE_NONE;
          break;
      }
      AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::IDLE_PC_STATE), idle_pc_state,
                  true /* cache */, tmp_prop_val_map_);
      DRM_LOGD("CRTC %d: Set idle_pc_state %d", obj_id, idle_pc_state);
    }; break;

    case DRMOps::CRTC_SET_CACHE_STATE: {
      int cache_state = va_arg(args, int);
      uint32_t crtc_cache_state = CACHE_STATE_DISABLED;
      if (cache_state == (int)DRMCacheState::ENABLED) {
        crtc_cache_state = CACHE_STATE_ENABLED;
      }
      AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::CACHE_STATE),
                  crtc_cache_state, false /* cache */, tmp_prop_val_map_);
    } break;

    case DRMOps::CRTC_SET_VM_REQ_STATE: {
      if (!prop_mgr_.IsPropertyAvailable(DRMProperty::VM_REQ_STATE)) {
        return;
      }
      int drm_vm_req_state = va_arg(args, int);
      uint32_t vm_req_state = VM_REQ_STATE_NONE;
      switch (drm_vm_req_state) {
        case static_cast<int>(DRMVMRequestState::RELEASE):
          vm_req_state = VM_REQ_STATE_RELEASE;
          break;
        case static_cast<int>(DRMVMRequestState::ACQUIRE):
          vm_req_state = VM_REQ_STATE_ACQUIRE;
          break;
        default:
          vm_req_state = VM_REQ_STATE_NONE;
          break;
      }
      AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::VM_REQ_STATE), vm_req_state,
                  true /* cache */, tmp_prop_val_map_);
      DRM_LOGD("CRTC %d: Set vm_req_state %d", obj_id, vm_req_state);
    }; break;

    case DRMOps::CRTC_RESET_CACHE: {
      tmp_prop_val_map_.clear();
      committed_prop_val_map_.clear();
    } break;

    case DRMOps::CRTC_SET_UBWC_CLK: {
      if (!prop_mgr_.IsPropertyAvailable(DRMProperty::UBWC_CLK)) {
          return;
      }

      uint32_t ubwc_clk = va_arg(args, uint32_t);
      AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::UBWC_CLK),
                  ubwc_clk, true /* cache */, tmp_prop_val_map_);
    }; break;

    default:
      DRM_LOGE("Invalid opcode %d to set the property on crtc %d", code, obj_id);
      break;
  }
}

void DRMCrtc::SetROI(drmModeAtomicReq *req, uint32_t obj_id, uint32_t num_roi, DRMRect *crtc_rois,
                     DRMRect *spr_rois) {
#ifdef SDE_MAX_ROI_V1
  if (num_roi > SDE_MAX_ROI_V1 || !prop_mgr_.IsPropertyAvailable(DRMProperty::ROI_V1)) {
    return;
  }
  if (!num_roi || !crtc_rois || !spr_rois) {
    AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::ROI_V1),
                0, false /* cache */, tmp_prop_val_map_);
    DRM_LOGD("CRTC ROI is set to NULL to indicate full frame update");
    return;
  }
  memset(&roi_v1_, 0, sizeof(roi_v1_));
  roi_v1_.num_rects = num_roi;

  for (uint32_t i = 0; i < num_roi; i++) {
    roi_v1_.roi[i].x1 = crtc_rois[i].left;
    roi_v1_.roi[i].x2 = crtc_rois[i].right;
    roi_v1_.roi[i].y1 = crtc_rois[i].top;
    roi_v1_.roi[i].y2 = crtc_rois[i].bottom;
    DRM_LOGD("CRTC %d, ROI[l,t,b,r][%d %d %d %d]", obj_id,
             roi_v1_.roi[i].x1, roi_v1_.roi[i].y1, roi_v1_.roi[i].x2, roi_v1_.roi[i].y2);
#ifdef SDE_DRM_SPR_ROI
    roi_v1_.roi_feature_flags = SDE_DRM_ROI_SPR_FLAG_EN;
    roi_v1_.spr_roi[i].x1 = spr_rois[i].left;
    roi_v1_.spr_roi[i].x2 = spr_rois[i].right;
    roi_v1_.spr_roi[i].y1 = spr_rois[i].top;
    roi_v1_.spr_roi[i].y2 = spr_rois[i].bottom;
    DRM_LOGD("CRTC %d, SPR ROI[l,t,b,r][%d %d %d %d]", obj_id, roi_v1_.spr_roi[i].x1,
             roi_v1_.spr_roi[i].y1, roi_v1_.spr_roi[i].x2, roi_v1_.spr_roi[i].y2);
#endif
  }
  AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::ROI_V1),
              reinterpret_cast<uint64_t>(&roi_v1_), false /* cache */, tmp_prop_val_map_);
#endif
}

void DRMCrtc::SetSolidfillStages(drmModeAtomicReq *req, uint32_t obj_id,
                                 const std::vector<DRMSolidfillStage> *solid_fills) {
#if defined SDE_MAX_DIM_LAYERS
  memset(&drm_dim_layer_v1_, 0, sizeof(drm_dim_layer_v1_));
  uint32_t shift;

  drm_dim_layer_v1_.num_layers = solid_fills->size();
  for (uint32_t i = 0; i < solid_fills->size(); i++) {
    const DRMSolidfillStage &sf = solid_fills->at(i);
    float plane_alpha = (sf.plane_alpha / 65535.0f);
    drm_dim_layer_v1_.layer_cfg[i].stage = sf.z_order;
    drm_dim_layer_v1_.layer_cfg[i].rect.x1 = (uint16_t)sf.bounding_rect.left;
    drm_dim_layer_v1_.layer_cfg[i].rect.y1 = (uint16_t)sf.bounding_rect.top;
    drm_dim_layer_v1_.layer_cfg[i].rect.x2 = (uint16_t)sf.bounding_rect.right;
    drm_dim_layer_v1_.layer_cfg[i].rect.y2 = (uint16_t)sf.bounding_rect.bottom;
    drm_dim_layer_v1_.layer_cfg[i].flags =
      sf.is_exclusion_rect ? SDE_DRM_DIM_LAYER_EXCLUSIVE : SDE_DRM_DIM_LAYER_INCLUSIVE;

    // @sde_mdss_color: expects in [g b r a] order where as till now solidfill is in [a r g b].
    // As no support for passing plane alpha, Multiply Alpha color component with plane_alpa.
    shift = kSolidFillHwBitDepth - sf.color_bit_depth;
    drm_dim_layer_v1_.layer_cfg[i].color_fill.color_0 = (sf.green & 0x3FF) << shift;
    drm_dim_layer_v1_.layer_cfg[i].color_fill.color_1 = (sf.blue & 0x3FF) << shift;
    drm_dim_layer_v1_.layer_cfg[i].color_fill.color_2 = (sf.red & 0x3FF) << shift;
    // alpha is set as 8 or 16 bit depending on range max
    if (alpha_range_.second == UINT8_MAX) {
      uint32_t alpha = (sf.color_bit_depth == 8) ? sf.alpha : sf.alpha >> 8;
      drm_dim_layer_v1_.layer_cfg[i].color_fill.color_3 = (uint32_t)((alpha & 0xFF) * plane_alpha);
    } else {
      uint32_t alpha = (sf.color_bit_depth == 8) ? (sf.alpha << 8) + sf.alpha : sf.alpha;
      drm_dim_layer_v1_.layer_cfg[i].color_fill.color_3 =
          (uint32_t)((alpha & 0xFFFF) * plane_alpha);
    }
    DRM_LOGD("CRTC %d: Set solid fill alpha %d", obj_id,
             drm_dim_layer_v1_.layer_cfg[i].color_fill.color_3);
  }
  AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::DIM_STAGES_V1),
              reinterpret_cast<uint64_t> (&drm_dim_layer_v1_), false /* cache */,
              tmp_prop_val_map_);
#endif
}

void DRMCrtc::SetNoiseLayerConfig(drmModeAtomicReq *req, uint32_t obj_id,
                                 const DRMNoiseLayerConfig *noise_cfg) {
  drm_msm_noise_layer_cfg *cfg = nullptr;
  drm_noise_layer_v1_ = {};
  if (noise_cfg->enable) {
    drm_noise_layer_v1_.flags = noise_cfg->temporal_en ? DRM_NOISE_TEMPORAL_FLAG : 0;
    drm_noise_layer_v1_.zposn = noise_cfg->zpos_noise;
    drm_noise_layer_v1_.zposattn = noise_cfg->zpos_attn;
    drm_noise_layer_v1_.strength = noise_cfg->noise_strength;
    // set noise alpha and attn to 16 bit if range max is not UINT8_MAX
    // TODO(user): invert and reset when attn factor and alpha noise is given as 16 bit
    drm_noise_layer_v1_.attn_factor = (alpha_range_.second == UINT8_MAX)
                                          ? noise_cfg->attn_factor
                                          : (noise_cfg->attn_factor << 8) + noise_cfg->attn_factor;
    drm_noise_layer_v1_.alpha_noise = (alpha_range_.second == UINT8_MAX)
                                          ? noise_cfg->alpha_noise
                                          : (noise_cfg->alpha_noise << 8) + noise_cfg->alpha_noise;
    DRM_LOGD("CRTC %d: Set noise attn %d alpha %d", obj_id, drm_noise_layer_v1_.attn_factor,
             drm_noise_layer_v1_.alpha_noise);
    cfg = &drm_noise_layer_v1_;
  }
  AddProperty(req, obj_id, prop_mgr_.GetPropertyId(DRMProperty::NOISE_LAYER_V1),
              reinterpret_cast<uint64_t>(cfg), false /* cache */, tmp_prop_val_map_);
}

void DRMCrtc::Dump() {
  DRM_LOGE("id: %d\tbuffer_id: %d\tpos:(%d, %d)\tsize:(%dx%d)\n", drm_crtc_->crtc_id,
           drm_crtc_->buffer_id, drm_crtc_->x, drm_crtc_->y, drm_crtc_->width, drm_crtc_->height);
}

bool DRMCrtc::ConfigureScalerLUT(drmModeAtomicReq *req, uint32_t dir_lut_blob_id,
                                 uint32_t cir_lut_blob_id, uint32_t sep_lut_blob_id) {
  if (is_lut_configured_ && is_lut_validated_) {
    return false;
  }
  if (dir_lut_blob_id) {
    AddProperty(req, drm_crtc_->crtc_id,
                prop_mgr_.GetPropertyId(DRMProperty::DS_LUT_ED), dir_lut_blob_id,
                false /* cache */, tmp_prop_val_map_);
  }
  if (cir_lut_blob_id) {
    AddProperty(req, drm_crtc_->crtc_id,
                prop_mgr_.GetPropertyId(DRMProperty::DS_LUT_CIR), cir_lut_blob_id,
                false /* cache */, tmp_prop_val_map_);
  }
  if (sep_lut_blob_id) {
    AddProperty(req, drm_crtc_->crtc_id,
                prop_mgr_.GetPropertyId(DRMProperty::DS_LUT_SEP), sep_lut_blob_id,
                false /* cache */, tmp_prop_val_map_);
  }
  is_lut_validation_in_progress_ = true;
  return true;
}

void DRMCrtc::PostCommit(bool success) {
  if (success) {
    if (is_lut_validated_) {
      is_lut_configured_ = true;
    }
    committed_prop_val_map_ = tmp_prop_val_map_;
  } else {
    tmp_prop_val_map_ = committed_prop_val_map_;
  }
}

void DRMCrtc::PostValidate(bool success) {
  if (success && is_lut_validation_in_progress_)  {
    is_lut_validated_ = true;
  }

  tmp_prop_val_map_ = committed_prop_val_map_;
}

void DRMCrtc::ClearVotesCache() {
  // On subsequent SET_ACTIVE 1, commit these to MDP driver and re-add to cache automatically
  tmp_prop_val_map_.erase(prop_mgr_.GetPropertyId(DRMProperty::CORE_CLK));
  tmp_prop_val_map_.erase(prop_mgr_.GetPropertyId(DRMProperty::CORE_AB));
  tmp_prop_val_map_.erase(prop_mgr_.GetPropertyId(DRMProperty::CORE_IB));
  tmp_prop_val_map_.erase(prop_mgr_.GetPropertyId(DRMProperty::LLCC_AB));
  tmp_prop_val_map_.erase(prop_mgr_.GetPropertyId(DRMProperty::LLCC_IB));
  tmp_prop_val_map_.erase(prop_mgr_.GetPropertyId(DRMProperty::DRAM_AB));
  tmp_prop_val_map_.erase(prop_mgr_.GetPropertyId(DRMProperty::DRAM_IB));
}

uint32_t DRMCrtcManager::GetCrtcCount() {
  return crtc_pool_.size();
}

}  // namespace sde_drm
