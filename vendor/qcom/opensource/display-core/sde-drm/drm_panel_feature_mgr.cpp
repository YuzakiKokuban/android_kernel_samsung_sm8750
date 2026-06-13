/* Copyright (c) 2020-2021, The Linux Foundataion. All rights reserved.
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
*
*/

/*
 * ​​​​​Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
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
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of
 * its contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
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

#include "drm_panel_feature_mgr.h"

#include <drm_logger.h>
#include <errno.h>
#include <inttypes.h>

#include <cstring>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>

#include "display/drm/msm_drm_aiqe.h"

#define __CLASS__ "DRMPanelFeatureMgr"

namespace sde_drm {

using std::map;
using std::vector;
using std::mutex;
using std::lock_guard;

static DRMPanelFeatureMgr panel_feature_mgr;

// Demura Planes' Default Bit Indices
static uint8_t DEMURA_DMA1RECT0 = 0x1;
static uint8_t DEMURA_DMA1RECT1 = 0x2;
static uint8_t DEMURA_DMA3RECT0 = 0x3;
static uint8_t DEMURA_DMA3RECT1 = 0x4;

DRMPanelFeatureMgrIntf *GetPanelFeatureManagerIntf() {
  return &panel_feature_mgr;
}

void DRMPanelFeatureMgr::Init(int fd, drmModeRes* res) {
  lock_guard<mutex> lock(lock_);

  if (!res || (fd < 0)) {
    DRM_LOGE("Invalid arguments for init - fd %d and DRM resources pointer 0x%pK", fd, (void *)res);
    return;
  }

  drm_res_ = res;
  dev_fd_ = fd;

  for (int i = 0; i < res->count_crtcs; i++) {
    drmModeCrtc *crtc = drmModeGetCrtc(dev_fd_, res->crtcs[i]);
    if (crtc) {
      int err = InitObjectProps(crtc->crtc_id, DRM_MODE_OBJECT_CRTC);
      if (err) {
        DRM_LOGE("Failed to get crtc props %d", crtc->crtc_id);
      }
      drmModeFreeCrtc(crtc);
    }
  }

  for (int i = 0; i < res->count_connectors; i++) {
    drmModeConnector *conn = drmModeGetConnector(dev_fd_, res->connectors[i]);
    if (conn) {
      int err = InitObjectProps(conn->connector_id, DRM_MODE_OBJECT_CONNECTOR);
      if (err) {
        DRM_LOGE("Failed to get conn %d properties", conn->connector_id);
      }
      drmModeFreeConnector(conn);
    }
  }

  drm_property_map_[kDRMPanelFeatureDemuraResources] = DRMProperty::DEMURA_BOOT_PLANE_V1;
  drm_property_map_[kDRMPanelFeaturePanelId] = DRMProperty::DEMURA_PANEL_ID;
  drm_property_map_[kDRMPanelFeatureSPRUDC] = DRMProperty::SPR_UDC_CFG_V2;
  drm_property_map_[kDRMPanelFeatureSPRPackType] = DRMProperty::CAPABILITIES;
  drm_property_map_[kDRMPanelFeatureSPRPackTypeMode] = DRMProperty::CAPABILITIES;
  drm_property_map_[kDRMPanelFeatureDsppIndex] = DRMProperty::DSPP_CAPABILITIES;
  drm_property_map_[kDRMPanelFeatureDsppSPRInfo] = DRMProperty::DSPP_CAPABILITIES;
  drm_property_map_[kDRMPanelFeatureDsppDemuraInfo] = DRMProperty::DSPP_CAPABILITIES;
  drm_property_map_[kDRMPanelFeatureDsppRCInfo] = DRMProperty::DSPP_CAPABILITIES;
  drm_property_map_[kDRMPanelFeatureRCInit] = DRMProperty::DSPP_RC_MASK_V1;
  drm_property_map_[kDRMPanelFeatureDemuraCfg0Param2] = DRMProperty::DEMURA_CFG0_PARAM2;
  drm_property_map_[kDRMPanelFeatureAiqeSSRCConfig] = DRMProperty::SDE_DSPP_AIQE_SSRC_CONFIG_V1;
  drm_property_map_[kDRMPanelFeatureAiqeSSRCData] = DRMProperty::SDE_DSPP_AIQE_SSRC_DATA_V1;
  drm_property_map_[kDRMPanelFeatureAIScalerCfg] = DRMProperty::AI_SCALER_CFG_V1;
  drm_property_map_[kDRMPanelFeatureAiqeMdnie] = DRMProperty::SDE_DSPP_AIQE_MDNIE_V1;
  drm_property_map_[kDRMPanelFeatureAiqeMdnieArt] = DRMProperty::SDE_DSPP_AIQE_MDNIE_ART_V1;
  drm_property_map_[kDRMPanelFeatureAiqeMdnieIPC] = DRMProperty::SDE_DSPP_AIQE_MDNIE_IPC_V1;
  drm_property_map_[kDRMPanelFeatureAiqeCopr] = DRMProperty::SDE_DSPP_AIQE_COPR_V1;
  drm_property_map_[kDRMPanelFeatureABC] = DRMProperty::AIQE_ABC_V1;
  drm_property_map_[kDRMPanelFeatureDemuraBacklight] = DRMProperty::DEMURA_BACKLIGHT_V1;

  drm_prop_type_map_[kDRMPanelFeatureDemuraResources] = DRMPropType::kPropBitmask;
  drm_prop_type_map_[kDRMPanelFeatureDemuraInit] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeaturePanelId] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureSPRInit] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureSPRUDC] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureRCInit] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureSPRPackType] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureSPRPackTypeMode] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureDsppIndex] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureDsppSPRInfo] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureDsppDemuraInfo] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureDsppRCInfo] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureDemuraCfg0Param2] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureAiqeSSRCConfig] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureAiqeSSRCData] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureAIScalerCfg] = DRMPropType::kPropBlob;
  drm_prop_type_map_[kDRMPanelFeatureAiqeMdnie] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureAiqeMdnieArt] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureAiqeMdnieIPC] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureAiqeCopr] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureABC] = DRMPropType::kPropRange;
  drm_prop_type_map_[kDRMPanelFeatureDemuraBacklight] = DRMPropType::kPropRange;

  feature_info_tbl_[kDRMPanelFeatureDemuraResources] = DRMPanelFeatureInfo {
    kDRMPanelFeatureDemuraResources, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, 0, 0};
  feature_info_tbl_[kDRMPanelFeaturePanelId] = DRMPanelFeatureInfo {kDRMPanelFeaturePanelId,
      DRM_MODE_OBJECT_CONNECTOR, UINT32_MAX, 1, sizeof(uint64_t), 0};
  feature_info_tbl_[kDRMPanelFeatureSPRUDC] = DRMPanelFeatureInfo{
      kDRMPanelFeatureSPRUDC, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, sizeof(drm_msm_spr_udc_cfg), 0};
  feature_info_tbl_[kDRMPanelFeatureRCInit] = DRMPanelFeatureInfo {
      kDRMPanelFeatureRCInit, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, sizeof(drm_msm_rc_mask_cfg), 0};
  feature_info_tbl_[kDRMPanelFeatureSPRPackType] = DRMPanelFeatureInfo {kDRMPanelFeatureSPRPackType,
      DRM_MODE_OBJECT_CONNECTOR, UINT32_MAX, 1, 64, 0};
  feature_info_tbl_[kDRMPanelFeatureSPRPackTypeMode] = DRMPanelFeatureInfo{
      kDRMPanelFeatureSPRPackTypeMode, DRM_MODE_OBJECT_CONNECTOR, UINT32_MAX, 1, 64, 0};
  feature_info_tbl_[kDRMPanelFeatureDsppIndex] = DRMPanelFeatureInfo {kDRMPanelFeatureDsppIndex,
      DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, 64, 0};
  feature_info_tbl_[kDRMPanelFeatureDsppSPRInfo] = DRMPanelFeatureInfo {
    kDRMPanelFeatureDsppSPRInfo, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, 64, 0};
  feature_info_tbl_[kDRMPanelFeatureDsppDemuraInfo] = DRMPanelFeatureInfo {
    kDRMPanelFeatureDsppDemuraInfo, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, 64, 0};
  feature_info_tbl_[kDRMPanelFeatureDsppRCInfo] = DRMPanelFeatureInfo {
    kDRMPanelFeatureDsppRCInfo, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, 64, 0};
  feature_info_tbl_[kDRMPanelFeatureDemuraCfg0Param2] =
      DRMPanelFeatureInfo{kDRMPanelFeatureDemuraCfg0Param2,
                          DRM_MODE_OBJECT_CRTC,
                          UINT32_MAX,
                          1,
                          sizeof(drm_msm_dem_cfg0_param2),
                          0};
  feature_info_tbl_[kDRMPanelFeatureAiqeSSRCConfig] =
      DRMPanelFeatureInfo{kDRMPanelFeatureAiqeSSRCConfig,
                          DRM_MODE_OBJECT_CRTC,
                          UINT32_MAX,
                          1,
                          sizeof(drm_msm_ssrc_config),
                          0};
  feature_info_tbl_[kDRMPanelFeatureAiqeSSRCData] =
      DRMPanelFeatureInfo{kDRMPanelFeatureAiqeSSRCData,
                          DRM_MODE_OBJECT_CRTC,
                          UINT32_MAX,
                          1,
                          sizeof(drm_msm_ssrc_data),
                          0};
  feature_info_tbl_[kDRMPanelFeatureAIScalerCfg] = DRMPanelFeatureInfo{kDRMPanelFeatureAIScalerCfg,
                                                                       DRM_MODE_OBJECT_CRTC,
                                                                       UINT32_MAX,
                                                                       1,
                                                                       sizeof(drm_msm_ai_scaler),
                                                                       0};
  feature_info_tbl_[kDRMPanelFeatureAiqeMdnie] = DRMPanelFeatureInfo{
      kDRMPanelFeatureAiqeMdnie, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, sizeof(uint64_t), 0};
  feature_info_tbl_[kDRMPanelFeatureAiqeMdnieArt] = DRMPanelFeatureInfo{
      kDRMPanelFeatureAiqeMdnieArt, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, sizeof(uint64_t), 0};
  feature_info_tbl_[kDRMPanelFeatureAiqeMdnieIPC] = DRMPanelFeatureInfo{
      kDRMPanelFeatureAiqeMdnieIPC, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, sizeof(uint64_t), 0};
  feature_info_tbl_[kDRMPanelFeatureAiqeCopr] = DRMPanelFeatureInfo{
      kDRMPanelFeatureAiqeCopr, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, sizeof(uint64_t), 0};
  feature_info_tbl_[kDRMPanelFeatureABC] =
      DRMPanelFeatureInfo{kDRMPanelFeatureABC, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, 64, 0};
  feature_info_tbl_[kDRMPanelFeatureDemuraBacklight] = DRMPanelFeatureInfo{
      kDRMPanelFeatureDemuraBacklight, DRM_MODE_OBJECT_CRTC, UINT32_MAX, 1, sizeof(uint32_t), 0};
}

void DRMPanelFeatureMgr::Deinit() {
  int ret = 0;
  for (int i = kDRMPanelFeatureDsppIndex; i < kDRMPanelFeatureMax; i++) {
    DRMPanelFeatureID prop_id = static_cast<DRMPanelFeatureID>(i);
    DRM_LOGD("size of queue %d, feature %d", drm_prop_blob_ids_cache_[prop_id].size(), prop_id);
    for (; drm_prop_blob_ids_cache_[prop_id].size();
         drm_prop_blob_ids_cache_[prop_id].pop_front()) {
      uint32_t blob_id = drm_prop_blob_ids_cache_[prop_id].front();
      if (blob_id) {
        ret = drmModeDestroyPropertyBlob(dev_fd_, blob_id);
        if (ret) {
          DRM_LOGE("failed to destroy blob for feature %d, ret = %d", prop_id, ret);
          return;
        }
      }
    }
  }
}

int DRMPanelFeatureMgr::InitObjectProps(int obj_id, int obj_type) {
  if (dev_fd_ < 0 || obj_id < 0) {
    DRM_LOGE("Invalid dev_fd_ %d or crtc_id %d", dev_fd_, obj_id);
    return -EINVAL;
  }

  drmModeObjectProperties *props =
          drmModeObjectGetProperties(dev_fd_, obj_id, obj_type);
  if (!props || !props->props || !props->prop_values) {
    DRM_LOGE("Failed to get props for obj_id:%d obj_type:%d", obj_id, obj_type);
    drmModeFreeObjectProperties(props);
    return -EINVAL;
  }

  for (uint32_t j = 0; j < props->count_props; j++) {
    drmModePropertyRes *info = drmModeGetProperty(dev_fd_, props->props[j]);
    if (!info) {
      continue;
    }

    std::string property_name(info->name);
    DRMProperty prop_enum = prop_mgr_.GetPropertyEnum(property_name);
    if (prop_enum == DRMProperty::INVALID) {
      DRM_LOGD("DRMProperty %s missing from global property mapping", info->name);
      drmModeFreeProperty(info);
      continue;
    }

    if (prop_enum == DRMProperty::SPR_INIT_CFG_V1) {
      drm_property_map_[kDRMPanelFeatureSPRInit] = DRMProperty::SPR_INIT_CFG_V1;
      feature_info_tbl_[kDRMPanelFeatureSPRInit] = DRMPanelFeatureInfo{kDRMPanelFeatureSPRInit,
                                                                       DRM_MODE_OBJECT_CRTC,
                                                                       UINT32_MAX,
                                                                       1,
                                                                       sizeof(drm_msm_spr_init_cfg),
                                                                       0};
    } else if (prop_enum == DRMProperty::SPR_INIT_CFG_V2) {
      drm_property_map_[kDRMPanelFeatureSPRInit] = DRMProperty::SPR_INIT_CFG_V2;
      feature_info_tbl_[kDRMPanelFeatureSPRInit] = DRMPanelFeatureInfo{kDRMPanelFeatureSPRInit,
                                                                       DRM_MODE_OBJECT_CRTC,
                                                                       UINT32_MAX,
                                                                       2,
                                                                       sizeof(drm_msm_spr_init_cfg),
                                                                       0};
    } else if (prop_enum == DRMProperty::DEMURA_INIT_CFG_V1) {
      drm_property_map_[kDRMPanelFeatureDemuraInit] = DRMProperty::DEMURA_INIT_CFG_V1;
      feature_info_tbl_[kDRMPanelFeatureDemuraInit] = DRMPanelFeatureInfo{
                                                                        kDRMPanelFeatureDemuraInit,
                                                                        DRM_MODE_OBJECT_CRTC,
                                                                        UINT32_MAX,
                                                                        1,
                                                                        sizeof(drm_msm_dem_cfg),
                                                                        0};
    } else if (prop_enum == DRMProperty::DEMURA_INIT_CFG_V3) {
      drm_property_map_[kDRMPanelFeatureDemuraInit] = DRMProperty::DEMURA_INIT_CFG_V3;
      feature_info_tbl_[kDRMPanelFeatureDemuraInit] = DRMPanelFeatureInfo{
                                                                        kDRMPanelFeatureDemuraInit,
                                                                        DRM_MODE_OBJECT_CRTC,
                                                                        UINT32_MAX,
                                                                        3,
                                                                        sizeof(drm_msm_dem_cfg),
                                                                        0};
    }

    prop_mgr_.SetPropertyId(prop_enum, info->prop_id);
    drmModeFreeProperty(info);
  }

  drmModeFreeObjectProperties(props);
  return 0;
}

void DRMPanelFeatureMgr::ParsePanelId(uint32_t blob_id, DRMPanelFeatureInfo *info) {
  drmModePropertyBlobRes *blob = drmModeGetPropertyBlob(dev_fd_, blob_id);
  if (!blob) {
    return;
  }

  if (!blob->data) {
    return;
  }

  if (blob->length != sizeof(uint64_t)) {
    DRM_LOGE("Expecting %zu bytes but got %u", sizeof(uint64_t), blob->length);
    return;
  }

  uint64_t* panel_id = reinterpret_cast<uint64_t *>(info->prop_ptr);
  // Read as-is / big endian. Driver has supplied the value in this manner.
  uint8_t *data = (uint8_t*)(blob->data);
  for (size_t i = 0; i < blob->length; i++) {
    *panel_id = (*panel_id << 8) | *data;
    data++;
  }
  info->prop_size = sizeof(uint64_t);

  drmModeFreePropertyBlob(blob);
}

void DRMPanelFeatureMgr::ParseDemuraResources(drmModePropertyRes *prop, uint64_t value,
                                              DRMPanelFeatureInfo *info) {
  // Values come as bit indices, not fully-realized values, for DRM_MODE_PROP_BITMASK
  for (auto i = 0; i < prop->count_enums; i++) {
    std::string enum_name(prop->enums[i].name);
    if (enum_name == "demura_dma1_rect0") {
      DEMURA_DMA1RECT0 = (1 << prop->enums[i].value);
    } else if (enum_name == "demura_dma1_rect1") {
      DEMURA_DMA1RECT1 = (1 << prop->enums[i].value);
    } else if (enum_name == "demura_dma3_rect0") {
      DEMURA_DMA3RECT0 = (1 << prop->enums[i].value);
    } else if (enum_name == "demura_dma3_rect1") {
      DEMURA_DMA3RECT1 = (1 << prop->enums[i].value);
    }
  }

  FetchResourceList *frl = reinterpret_cast<FetchResourceList*>(info->prop_ptr);
  if (value & DEMURA_DMA1RECT0) {
    frl->push_back(std::make_tuple("DMA", 1, 0));
  }
  if (value & DEMURA_DMA1RECT1) {
    frl->push_back(std::make_tuple("DMA", 1, 1));
  }
  if (value & DEMURA_DMA3RECT0) {
    frl->push_back(std::make_tuple("DMA", 3, 0));
  }
  if (value & DEMURA_DMA3RECT1) {
    frl->push_back(std::make_tuple("DMA", 3, 1));
  }

  info->prop_size += frl->size();
}

void DRMPanelFeatureMgr::ParseDsppCapabilities(uint32_t blob_id, std::vector<int> *values,
                                               uint32_t *size, const std::string str) {
  drmModePropertyBlobRes *blob = drmModeGetPropertyBlob(dev_fd_, blob_id);
  if (!blob) {
    DRM_LOGW("Unable to find blob for id %d", blob_id);
    return;
  }

  if (!blob->data) {
    DRM_LOGW("Invalid blob - no data for for blob-id %d", blob_id);
    return;
  }

  char *fmt_str = new char[blob->length + 1];
  std::memcpy(fmt_str, blob->data, blob->length);
  fmt_str[blob->length] = '\0';
  std::stringstream stream(fmt_str);
  std::string line = {};
  // Search for panel feature property pattern. Which is defined as rc0=1, rc1=1
  const std::regex exp(str + "(\\d+)=1");
  std::smatch sm;
  while (std::getline(stream, line)) {
    std::regex_match(line, sm, exp);
    // smatch shall include full line as a match followed by the hw block # as a match
    if (sm.size() == 2) {
      std::string tmpstr(sm[1]);
      int temp = atoi(tmpstr.c_str());  // atoi safe to use due to regex success
      values->push_back(temp);
    }
  }

  *size = sizeof(int) * values->size();
  delete[] fmt_str;
}

void DRMPanelFeatureMgr::ParseCapabilities(uint32_t blob_id, char* value, uint32_t max_len,
                                           const std::string str) {
  drmModePropertyBlobRes *blob = drmModeGetPropertyBlob(dev_fd_, blob_id);
  if (!blob) {
    DRM_LOGW("Unable to find blob for id %d", blob_id);
    return;
  }

  if (!blob->data) {
    DRM_LOGW("Invalid blob - no data for for blob-id %d", blob_id);
    return;
  }

  char *fmt_str = new char[blob->length + 1];
  std::memcpy(fmt_str, blob->data, blob->length);
  fmt_str[blob->length] = '\0';
  std::stringstream stream(fmt_str);
  std::string line = {};
  std::string val = {};
  const std::string goal = str + "=";
  while (std::getline(stream, line)) {
    if (line.find(goal) != std::string::npos) {
      val = std::string(line, goal.length());
    }
  }

  if (max_len <= val.size()) {
    DRM_LOGW("Insufficient size max_len: %d actual size: %zu", max_len, val.size());
    return;
  }
  std::copy(val.begin(), val.end(), value);
  value[val.size()] = '\0';
  delete[] fmt_str;
}

void DRMPanelFeatureMgr::GetPanelFeatureInfo(DRMPanelFeatureInfo *info) {
  lock_guard<mutex> lock(lock_);

  if (!info) {
    DRM_LOGE("Invalid input, DRMPanelFeatureInfo is NULL");
    return;
  }

  if (info->prop_id > kDRMPanelFeatureMax) {
    DRM_LOGE("Invalid feature id %d", info->prop_id);
    return;
  }

  DRMProperty prop_enum = drm_property_map_[info->prop_id];
  if (!prop_mgr_.IsPropertyAvailable(prop_enum)) {
    DRM_LOGW("Property id is not available for DRMProperty: %d feature-id: %d",
             prop_enum, info->prop_id);
    return;
  }

  // memory is not allocated by client - populate default property info
  if (!info->prop_ptr) {
    *info = feature_info_tbl_[info->prop_id];
    return;
  }

  drmModeObjectProperties *props =
          drmModeObjectGetProperties(dev_fd_, info->obj_id, info->obj_type);
  if (!props || !props->props || !props->prop_values) {
    drmModeFreeObjectProperties(props);
    DRM_LOGE("Failed to Get properties for obj: %d type:%d", info->obj_id, info->obj_type);
    return;
  }

  for (uint32_t j = 0; j < props->count_props; j++) {
    drmModePropertyRes *property = drmModeGetProperty(dev_fd_, props->props[j]);
    if (!property) {
      continue;
    }

    std::string property_name(property->name);
    if (prop_enum != prop_mgr_.GetPropertyEnum(property_name)) {
      drmModeFreeProperty(property);
      continue;
    }

    if (info->prop_id == kDRMPanelFeatureSPRPackType) {
      ParseCapabilities(props->prop_values[j],
              reinterpret_cast<char *> (info->prop_ptr), info->prop_size, "spr_pack_type");
    } else if (info->prop_id == kDRMPanelFeatureSPRPackTypeMode) {
      ParseCapabilities(props->prop_values[j], reinterpret_cast<char *>(info->prop_ptr),
                        info->prop_size, "spr_pack_type_mode");
    } else if (info->prop_id == kDRMPanelFeatureDsppIndex) {
      ParseDsppCapabilities(props->prop_values[j],
              reinterpret_cast<std::vector<int> *>(info->prop_ptr), &(info->prop_size), "dspp");
    } else if (info->prop_id == kDRMPanelFeatureDemuraResources) {
      ParseDemuraResources(property, props->prop_values[j], info);
    } else if (info->prop_id == kDRMPanelFeaturePanelId) {
      ParsePanelId(props->prop_values[j], info);
    } else if (info->prop_id == kDRMPanelFeatureDsppSPRInfo) {
      ParseDsppCapabilities(props->prop_values[j],
              reinterpret_cast<std::vector<int> *>(info->prop_ptr), &(info->prop_size), "spr");
    } else if (info->prop_id == kDRMPanelFeatureDsppDemuraInfo) {
      ParseDsppCapabilities(props->prop_values[j],
              reinterpret_cast<std::vector<int> *>(info->prop_ptr), &(info->prop_size), "demura");
    } else if (info->prop_id == kDRMPanelFeatureDsppRCInfo) {
      ParseDsppCapabilities(props->prop_values[j],
              reinterpret_cast<std::vector<int> *>(info->prop_ptr), &(info->prop_size), "rc");
    } else if (drm_prop_type_map_[info->prop_id] == DRMPropType::kPropBlob) {
      drmModePropertyBlobRes *blob = drmModeGetPropertyBlob(dev_fd_, props->prop_values[j]);
      if (!blob || !blob->data || !blob->length) {
        return;
      }
      uint8_t *src_begin = reinterpret_cast<uint8_t *> (blob->data);
      uint8_t *src_end = src_begin + blob->length;
      uint8_t *dst = reinterpret_cast<uint8_t *> (info->prop_ptr);
      std::copy(src_begin, src_end, dst);
    } else {
      uint8_t *src_begin = reinterpret_cast<uint8_t *> (props->prop_values[j]);
      uint8_t *src_end = src_begin + info->prop_size;
      uint8_t *dst = reinterpret_cast<uint8_t *> (info->prop_ptr);
      std::copy(src_begin, src_end, dst);
    }

    drmModeFreeProperty(property);
  }

  drmModeFreeObjectProperties(props);
}

void DRMPanelFeatureMgr::CachePanelFeature(const DRMPanelFeatureInfo &info) {
  lock_guard<mutex> lock(lock_);

  if (info.prop_id >= kDRMPanelFeatureMax || info.obj_id == UINT32_MAX) {
    DRM_LOGE("invalid property info to set id %d value ptr %" PRIu64 , info.prop_id, info.prop_ptr);
    return;
  }

  for (auto &it : dirty_features_) {
    if ((it.obj_id == info.obj_id) && (it.prop_id == info.prop_id)) {
      it = info;
      DLOGW("Property is set twice obj_id %d, prop_id %d", it.obj_id, it.prop_id);
      return;
    }
  }

  dirty_features_.push_back(info);
}

void DRMPanelFeatureMgr::CommitPanelFeatures(drmModeAtomicReq *req, const DRMDisplayToken &token) {
  lock_guard<mutex> lock(lock_);
  for (auto it = dirty_features_.begin(); it != dirty_features_.end();) {
    if (it->obj_id == token.crtc_id || it->obj_id == token.conn_id) {
      DRMPanelFeatureInfo info = *it;
      ApplyDirtyFeature(req, token, info);
      it = dirty_features_.erase(it);
    } else {
      it++;
    }
  }
}

void DRMPanelFeatureMgr::NullCommitPanelFeatures(drmModeAtomicReq *req,
                                                 const DRMDisplayToken &token) {
  lock_guard<mutex> lock(lock_);
  for (auto it = dirty_features_.begin(); it != dirty_features_.end();) {
    if (it->obj_id == token.crtc_id || it->obj_id == token.conn_id) {
      DRMPanelFeatureInfo info = *it;

      auto entry_iter = apply_in_null_commit_.find(info.obj_id);
      if (entry_iter == apply_in_null_commit_.end()) {
        it++;
        continue;
      }

      if (entry_iter->second != info.prop_id) {
        it++;
        continue;
      }

      ApplyDirtyFeature(req, token, info);
      it = dirty_features_.erase(it);
      apply_in_null_commit_.erase(info.obj_id);
    } else {
      it++;
    }
  }
}

// LCOV_EXCL_START
void DRMPanelFeatureMgr::ResetPanelFeatures(drmModeAtomicReq *req,
                                                 const DRMDisplayToken &token) {
  lock_guard<mutex> lock(lock_);
  DRMPanelFeatureInfo info;

  info.prop_id = kDRMPanelFeatureSPRInit;
  info.obj_id = token.crtc_id;
  info.prop_ptr = 0;
  ApplyDirtyFeature(req, token, info);

  info.prop_id = kDRMPanelFeatureDemuraInit;
  ApplyDirtyFeature(req, token, info);

#ifndef TRUSTED_VM
  info.prop_id = kDRMPanelFeatureABC;
  ApplyDirtyFeature(req, token, info);
#endif

  info.prop_id = kDRMPanelFeatureSPRUDC;
  uint32_t prop_id = prop_mgr_.GetPropertyId(drm_property_map_[info.prop_id]);
  if (prop_id) {
    ApplyDirtyFeature(req, token, info);
  }

  info.prop_id = kDRMPanelFeatureRCInit;
  ApplyDirtyFeature(req, token, info);
}
// LCOV_EXCL_STOP

void DRMPanelFeatureMgr::MarkForNullCommit(const DRMDisplayToken &token, const DRMPanelFeatureID &id) {
  DRMPanelFeatureInfo &info = feature_info_tbl_[id];
  uint32_t obj_id = 0;
  switch (info.obj_type) {
    case DRM_MODE_OBJECT_CRTC:
      obj_id = token.crtc_id;
      break;
    case DRM_MODE_OBJECT_CONNECTOR:
      obj_id = token.conn_id;
      break;
    default:
      return;
  }
  apply_in_null_commit_[obj_id] = id;
  DLOGI("Marked %u for null commit", id);
}

void DRMPanelFeatureMgr::ApplyDirtyFeature(drmModeAtomicReq *req, const DRMDisplayToken &token,
                                           DRMPanelFeatureInfo &info) {
  int ret = 0;
  if (info.prop_id >= kDRMPanelFeatureMax) {
    DRM_LOGE("invalid property info to set id %d value ptr %" PRIu64, info.prop_id, info.prop_ptr);
    return;
  }

  // Commit only features meant for the given DisplayToken
  if (token.crtc_id != info.obj_id && token.conn_id != info.obj_id) {
    return;
  }

  uint32_t prop_id = prop_mgr_.GetPropertyId(drm_property_map_[info.prop_id]);
  if (!prop_id) {
    DRM_LOGE("prop_id is 0 for panel feature-id %u", info.prop_id);
    return;
  }
  uint64_t value = 0;

  if (DRMPropType::kPropBlob == drm_prop_type_map_[info.prop_id]) {
    uint32_t blob_id = 0;
    if (!info.prop_ptr) {
      // Reset the feature.
      ret = drmModeAtomicAddProperty(req, info.obj_id, prop_id, 0);
      if (ret < 0) {
        DRM_LOGE("failed to add property ret:%d, obj_id:%d prop_id:%u value:%" PRIu64,
                  ret, info.obj_id, prop_id, value);
      }
      DLOGI("Commited panel feature [disabled]: %u-%u", info.prop_id, prop_id);
      return;
    }

    ret = drmModeCreatePropertyBlob(dev_fd_, reinterpret_cast<void *> (info.prop_ptr),
            info.prop_size, &blob_id);
    if (ret || blob_id == 0) {
      DRM_LOGE("failed to create blob ret %d, id = %d prop_ptr:%" PRIu64 " prop_sz:%d",
              ret, blob_id, info.prop_ptr, info.prop_size);
      return;
    }

    if (drm_prop_blob_ids_cache_[info.prop_id].size() > 2) {
      DRM_LOGE("invalid blob count %d, for feature = %d, clearing stale blobs",
               drm_prop_blob_ids_cache_[info.prop_id].size(), info.prop_id);
      for (; drm_prop_blob_ids_cache_[info.prop_id].size();
           drm_prop_blob_ids_cache_[info.prop_id].pop_front()) {
        uint32_t stale_blob_id = drm_prop_blob_ids_cache_[info.prop_id].front();
        if (stale_blob_id) {
          ret = drmModeDestroyPropertyBlob(dev_fd_, stale_blob_id);
          if (ret) {
            DRM_LOGE("failed to destroy blob for feature %d, ret = %d", info.prop_id, ret);
            return;
          }
        }
      }
    } else if (drm_prop_blob_ids_cache_[info.prop_id].size() == 2) {
      ret = drmModeDestroyPropertyBlob(dev_fd_, drm_prop_blob_ids_cache_[info.prop_id].front());
      if (ret) {
        DRM_LOGE("failed to destroy blob for feature %d, ret = %d", info.prop_id, ret);
        return;
      }
      drm_prop_blob_ids_cache_[info.prop_id].pop_front();
    }
    drm_prop_blob_ids_cache_[info.prop_id].push_back(blob_id);
    DRM_LOGD("size of queue %d, property %d", drm_prop_blob_ids_cache_[info.prop_id].size(),
             info.prop_id);

    value = blob_id;
  } else if (DRMPropType::kPropRange == drm_prop_type_map_[info.prop_id]) {
    if (!info.prop_ptr) {
      // TBD: will be verified as part of Dynamic disable/en AIDL
      // Reset the feature.
      ret = drmModeAtomicAddProperty(req, info.obj_id, prop_id, 0);
      if (ret < 0) {
        DRM_LOGE(
            "failed to add property ret:%d, obj_id:%d prop_id:%u "
            "value:%" PRIu64,
            ret, info.obj_id, prop_id, value);
      }
      DLOGI("Commited panel feature [disabled]: %u-%u", info.prop_id, prop_id);
      return;
    }
    value = info.prop_ptr;
  } else {
    DRM_LOGE("Unsupported property type id = %d size:%d", info.prop_id, info.prop_size);
  }

  ret = drmModeAtomicAddProperty(req, info.obj_id, prop_id, value);
  if (ret < 0) {
    DRM_LOGE("failed to add property ret:%d, obj_id:%d prop_id:%x value:%" PRIu64,
              ret, info.obj_id, prop_id, value);
  }
  DLOGI("Commited panel feature [enabled]: %u-%u", info.prop_id, prop_id);
}

}  // namespace sde_drm
