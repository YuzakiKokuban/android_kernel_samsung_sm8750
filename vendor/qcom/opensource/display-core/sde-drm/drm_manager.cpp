/*
* Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 */

#include <drm_logger.h>

#include <string.h>
#include "drm_atomic_req.h"
#include "drm_connector.h"
#include "drm_crtc.h"
#include "drm_encoder.h"
#include "drm_manager.h"
#include "drm_plane.h"

using std::lock_guard;
using std::mutex;
using std::pair;
using std::make_pair;

extern "C" {

int GetDRMManager(int fd, sde_drm::DRMManagerInterface **intf) {
  sde_drm::DRMManager *drm_mgr = sde_drm::DRMManager::GetInstance(fd);
  if (!drm_mgr) {
    return -ENODEV;
  }

  *intf = drm_mgr;
  return 0;
}

int DestroyDRMManager(int fd) {
  sde_drm::DRMManager::Destroy(fd);
  return 0;
}

}  // extern "C"

namespace sde_drm {

#define __CLASS__ "DRMManager"

sdm::MultiCoreInstance<int, DRMManager*> DRMManager::s_drm_instance;
mutex DRMManager::s_lock;

DRMManager *DRMManager::GetInstance(int fd) {
  lock_guard<mutex> lock(s_lock);

  auto iter = s_drm_instance.Find(fd);
  if (iter == s_drm_instance.End()) {
    DRMManager *drm_manager = new DRMManager();

    int ret = drm_manager ? drm_manager->Init(fd) : DRM_ERR_INVALID;
    if (ret) {
      delete drm_manager;
      drm_manager = nullptr;
    }

    s_drm_instance[fd] = drm_manager;
  }

  return s_drm_instance[fd];
}

void DRMManager::Destroy(int fd) {
  lock_guard<mutex> lock(s_lock);

  auto iter = s_drm_instance.Find(fd);
  if (iter != s_drm_instance.End()) {
    delete s_drm_instance[fd];
    s_drm_instance[fd] = nullptr;
    s_drm_instance.Erase(fd);
  }
}

int DRMManager::Init(int drm_fd) {
  fd_ = drm_fd;

  drmSetClientCap(fd_, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
  drmSetClientCap(fd_, DRM_CLIENT_CAP_ATOMIC, 1);

  drmModeRes *resource = drmModeGetResources(fd_);
  if (resource == NULL) {
    DRM_LOGE("drmModeGetResources failed");
    return DRM_ERR_INVALID;
  }

  conn_mgr_ = new DRMConnectorManager(fd_);
  if (!conn_mgr_) {
    DRM_LOGE("Failed to get Connector Mgr");
    return DRM_ERR_INVALID;
  }
  conn_mgr_->Init(resource);

  encoder_mgr_ = new DRMEncoderManager(fd_);
  if (!encoder_mgr_) {
    DRM_LOGE("Failed to get Encoder Mgr");
    return DRM_ERR_INVALID;
  }
  encoder_mgr_->Init(resource);

  crtc_mgr_ = new DRMCrtcManager(fd_);
  if (!crtc_mgr_) {
    DRM_LOGE("Failed to get Crtc Mgr");
    return DRM_ERR_INVALID;
  }
  crtc_mgr_->Init(resource);

  plane_mgr_ = new DRMPlaneManager(fd_);
  if (!plane_mgr_) {
    DRM_LOGE("Failed to get Plane Mgr");
    return DRM_ERR_INVALID;
  }
  plane_mgr_->Init();

  dpps_mgr_intf_ = GetDppsManagerIntf();
  if (dpps_mgr_intf_)
    dpps_mgr_intf_->Init(fd_, resource);

  panel_feature_mgr_intf_ = GetPanelFeatureManagerIntf();
  if (!panel_feature_mgr_intf_) {
    DRM_LOGE("Failed to get Panel feature Mgr");
    return DRM_ERR_INVALID;
  }
  panel_feature_mgr_intf_->Init(fd_, resource);

  drmModeFreeResources(resource);

  return 0;
}

int DRMManager::GetConnectorInfo(uint32_t conn_id, DRMConnectorInfo *info) {
  *info = {};
  return conn_mgr_->GetConnectorInfo(conn_id, info);
}

int DRMManager::GetConnectorsInfo(DRMConnectorsInfo *infos) {
  *infos = {};
  int ret = -ENODEV;
  std::vector<uint32_t> conn_ids;
  conn_mgr_->Update();
  conn_mgr_->GetConnectorList(&conn_ids);
  for (auto iter : conn_ids) {
    DRMConnectorInfo info;
    ret = GetConnectorInfo(iter, &info);
    if (!ret) {
      (*infos)[iter] = info;
    } else {
      break;
    }
  }

  return ret;
}

int DRMManager::GetEncoderInfo(uint32_t encoder_id, DRMEncoderInfo *info) {
  *info = {};
  return encoder_mgr_->GetEncoderInfo(encoder_id, info);
}

int DRMManager::GetEncodersInfo(DRMEncodersInfo *infos) {
  *infos = {};
  int ret = -ENODEV;
  std::vector<uint32_t> encoder_ids;
  encoder_mgr_->GetEncoderList(&encoder_ids);
  for (auto iter : encoder_ids) {
    DRMEncoderInfo info;
    ret = GetEncoderInfo(iter, &info);
    if (!ret) {
      (*infos)[iter] = info;
    } else {
      break;
    }
  }

  return ret;
}

int DRMManager::GetCrtcInfo(uint32_t crtc_id, DRMCrtcInfo *info) {
  *info = {};
  return crtc_mgr_->GetCrtcInfo(crtc_id, info);
}

void DRMManager::GetPlanesInfo(DRMPlanesInfo *info) {
  plane_mgr_->GetPlanesInfo(info);
}

void DRMManager::GetCrtcPPInfo(uint32_t crtc_id, DRMPPFeatureInfo *info) {
  crtc_mgr_->GetPPInfo(crtc_id, info);
}

DRMPlaneManager *DRMManager::GetPlaneMgr() {
  return plane_mgr_;
}

DRMConnectorManager *DRMManager::GetConnectorMgr() {
  return conn_mgr_;
}

DRMEncoderManager *DRMManager::GetEncoderMgr() {
  return encoder_mgr_;
}

DRMCrtcManager *DRMManager::GetCrtcMgr() {
  return crtc_mgr_;
}

DRMDppsManagerIntf *DRMManager::GetDppsMgrIntf() {
  return dpps_mgr_intf_;
}

int DRMManager::RegisterDisplay(DRMDisplayType disp_type, DRMDisplayToken *token,
                                bool has_cac_loopback) {
  int ret = conn_mgr_->Reserve(disp_type, token, has_cac_loopback);
  if (ret) {
    if (ret == -ENODEV) {
      DRM_LOGI("display type %d is not present", disp_type);
    } else {
      DRM_LOGE("Error reserving connector for display type %d. Error = %d (%s)", disp_type, ret,
               strerror(abs(ret)));
    }
    return ret;
  }

  std::set<uint32_t> possible_encoders;
  ret = conn_mgr_->GetPossibleEncoders(token->conn_id, &possible_encoders);
  if (ret) {
    DRM_LOGE("Error retreiving possible encoders for display type %d. Error = %d (%s)", disp_type,
             ret, strerror(abs(ret)));
    return ret;
  }

  ret = encoder_mgr_->Reserve(possible_encoders, token);
  if (ret) {
    DRM_LOGE("Error reserving encoder for display type %d. Error = %d (%s)", disp_type, ret,
             strerror(abs(ret)));
    conn_mgr_->Free(token);
    return ret;
  }

  std::set<uint32_t> possible_crtc_indices;
  ret = encoder_mgr_->GetPossibleCrtcIndices(token->encoder_id, &possible_crtc_indices);
  if (ret) {
    DRM_LOGE("Error retreiving possible crtcs for display type %d. Error = %d (%s)", disp_type,
             ret, strerror(abs(ret)));
    return ret;
  }

  ret = crtc_mgr_->Reserve(possible_crtc_indices, token);
  if (ret) {
    DRM_LOGE("Error reserving crtc for display type %d. Error = %d (%s)", disp_type, ret,
             strerror(abs(ret)));
    encoder_mgr_->Free(token);
    conn_mgr_->Free(token);
    return ret;
  }

  return 0;
}

int DRMManager::RegisterDisplay(int32_t display_id, DRMDisplayToken *token) {
  int ret = conn_mgr_->Reserve(display_id, token);
  if (ret) {
    DRM_LOGE("Error reserving connector %d. Error = %d (%s)", display_id, ret, strerror(abs(ret)));
    return ret;
  }

  std::set<uint32_t> possible_encoders;
  ret = conn_mgr_->GetPossibleEncoders(token->conn_id, &possible_encoders);
  if (ret) {
    DRM_LOGE("Error retreiving possible encoders for display id %d. Error = %d (%s)", display_id,
             ret, strerror(abs(ret)));
    return ret;
  }

  ret = encoder_mgr_->Reserve(possible_encoders, token);
  if (ret) {
    DRM_LOGE("Error reserving encoder for display %d. Error: %d (%s)", display_id, ret,
             strerror(abs(ret)));
    return ret;
  }

  std::set<uint32_t> possible_crtc_indices;
  ret = encoder_mgr_->GetPossibleCrtcIndices(token->encoder_id, &possible_crtc_indices);
  if (ret) {
    DRM_LOGE("Error retreiving possible crtcs for display id %d. Error = %d (%s)", display_id,
             ret, strerror(abs(ret)));
    encoder_mgr_->Free(token);
    conn_mgr_->Free(token);
    return ret;
  }

  ret = crtc_mgr_->Reserve(possible_crtc_indices, token);
  if (ret) {
    DRM_LOGE("Error reserving crtc for display %d. Error: %d (%s)", display_id,
             ret, strerror(abs(ret)));
    encoder_mgr_->Free(token);
    conn_mgr_->Free(token);
    return ret;
  }

  return 0;
}

void DRMManager::UnregisterDisplay(DRMDisplayToken *token) {
  conn_mgr_->Free(token);
  encoder_mgr_->Free(token);
  crtc_mgr_->Free(token);
}

DRMManager::~DRMManager() {
  if (conn_mgr_) {
    conn_mgr_->DeInit();
    delete conn_mgr_;
    conn_mgr_ = NULL;
  }
  if (encoder_mgr_) {
    encoder_mgr_->DeInit();
    delete encoder_mgr_;
    encoder_mgr_ = NULL;
  }
  if (crtc_mgr_) {
    crtc_mgr_->DeInit();
    delete crtc_mgr_;
    crtc_mgr_ = NULL;
  }
  if (plane_mgr_) {
    plane_mgr_->DeInit();
    delete plane_mgr_;
    plane_mgr_ = NULL;
  }
  if (panel_feature_mgr_intf_) {
    panel_feature_mgr_intf_->Deinit();
  }
}

int DRMManager::CreateAtomicReq(const DRMDisplayToken &token, DRMAtomicReqInterface **intf) {
  DRMAtomicReq *req = new DRMAtomicReq(fd_, this);
  int ret = req ? req->Init(token) : -ENOMEM;

  if (ret < 0) {
    DRM_LOGE("Initializing DRMAtomicReqInterface failed with error %d (%s)", ret,
             strerror(abs(ret)));
    delete req;
    return ret;
  }
  *intf = req;

  return 0;
}

int DRMManager::DestroyAtomicReq(DRMAtomicReqInterface *intf) {
  delete intf;
  return 0;
}

int DRMManager::SetScalerLUT(const DRMScalerLUTInfo &lut_info) {
  plane_mgr_->SetScalerLUT(lut_info);
  crtc_mgr_->SetScalerLUT(lut_info);
  return 0;
}

int DRMManager::UnsetScalerLUT() {
  plane_mgr_->UnsetScalerLUT();
  crtc_mgr_->UnsetScalerLUT();
  return 0;
}

void DRMManager::GetDppsFeatureInfo(DRMDppsFeatureInfo *info) {
  if (dpps_mgr_intf_)
    dpps_mgr_intf_->GetDppsFeatureInfo(info);
}

DRMPanelFeatureMgrIntf* DRMManager::GetPanelFeatureMgrIntf() {
  return panel_feature_mgr_intf_;
}

void DRMManager::GetPanelFeature(DRMPanelFeatureInfo *info) {
  if (panel_feature_mgr_intf_) {
    panel_feature_mgr_intf_->GetPanelFeatureInfo(info);
  } else {
    DRM_LOGE("Failed, panel feature mgr not available");
  }
}

void DRMManager::SetPanelFeature(const DRMPanelFeatureInfo &info) {
  if (panel_feature_mgr_intf_) {
    panel_feature_mgr_intf_->CachePanelFeature(info);
  } else {
    DRM_LOGE("Failed, panel feature mgr not available");
  }
}

void DRMManager::MarkPanelFeatureForNullCommit(const DRMDisplayToken &token,
                                               const DRMPanelFeatureID &id) {
  if (panel_feature_mgr_intf_) {
    panel_feature_mgr_intf_->MarkForNullCommit(token, id);
  } else {
    DRM_LOGE("Failed, panel feature mgr not available");
  }
}

void DRMManager::MapPlaneToConnector(std::map<uint32_t, uint32_t> *plane_to_connector) {
  if (!plane_to_connector) {
    DRM_LOGE("Map is NULL! Not expected.");
    return;
  }

  plane_to_connector->clear();

  std::map<uint32_t, uint32_t> plane_to_crtc;
  std::map<uint32_t, uint32_t> crtc_to_encoder;
  std::map<uint32_t, uint32_t> encoder_to_connector;

  // Cont. Splash planes are detected by CRTC existence on the PLANE
  // These are the planes that ultimately need to know their CONNECTOR
  plane_mgr_->MapPlaneToCrtc(&plane_to_crtc);
  if (!plane_to_crtc.size()) {
    DRM_LOGI("No cont. splash planes found");
    return;
  }

  // CRTC is connected to ENCODER. Find the ENCODERs who have CRTCs and establish
  // reverse lookup CRTC --> ENCODER
  encoder_mgr_->MapCrtcToEncoder(&crtc_to_encoder);
  if (!crtc_to_encoder.size()) {
    DRM_LOGW("Planes are associated with CRTCs but no CRTC to ENCODERs found");
    return;
  }

  // ENCODER is connected to CONNECTOR. Find the CONNECTORs who have ENCODERs and establish
  // reverse lookup ENCODER --> CONNECTOR
  conn_mgr_->MapEncoderToConnector(&encoder_to_connector);
  if (!encoder_to_connector.size()) {
    DRM_LOGW("CRTCs are associated with ENCODERs but no ENCODERs to CONNECTORs found");
    return;
  }

  // Link the Cont. Splash PLANEs with their CONNECTOR
  for (auto &plane_entry : plane_to_crtc) {
    uint32_t plane_id = plane_entry.first;
    auto &crtc_id = plane_entry.second;
    auto enc_entry = crtc_to_encoder.find(crtc_id);
    if (enc_entry == crtc_to_encoder.end()) {
      DRM_LOGW("Plane %u mapped to crtc %u didn't have an encoder. Not expected.", plane_id,
               crtc_id);
      continue;
    }
    auto &encoder_id = enc_entry->second;
    auto conn_entry = encoder_to_connector.find(encoder_id);
    if (conn_entry == encoder_to_connector.end()) {
      DRM_LOGW("Plane %u, crtc %u, encoder %u, didn't have a connector. Not expected.",
             plane_id, crtc_id, encoder_id);
      continue;
    }
    uint32_t connector_id = conn_entry->second;

    plane_to_connector->insert(make_pair(plane_id, connector_id));
  }
}

void DRMManager::GetRequiredDemuraFetchResourceCount(
                 std::map<uint32_t, uint8_t> *required_demura_fetch_cnt) {
  conn_mgr_->GetPreferredModeLMCounts(required_demura_fetch_cnt);
}

void DRMManager::GetInitialDemuraInfo(std::vector<uint32_t> *initial_demura_planes) {
  if (panel_feature_mgr_intf_) {
    initial_demura_planes->clear();
    std::vector<uint32_t> crtc_ids;
    crtc_mgr_->GetCrtcList(&crtc_ids);
    FetchResourceList frl;
    for (auto &id : crtc_ids) {
      DRMPanelFeatureInfo info;
      info.prop_id = kDRMPanelFeatureDemuraResources;
      info.obj_type = DRM_MODE_OBJECT_CRTC;
      info.obj_id = id;
      info.prop_ptr = reinterpret_cast<uint64_t>(&frl);
      panel_feature_mgr_intf_->GetPanelFeatureInfo(&info);
    }
    // Safe to assume pipe to crtc to Demura associations are functionally correct
    plane_mgr_->GetPlaneIdsFromDescriptions(frl, initial_demura_planes);
  }
}

uint32_t DRMManager::GetCrtcCount() {
  return crtc_mgr_->GetCrtcCount();
}

int DRMManager::GetPossibleEncoders(uint32_t connector_id, std::set<uint32_t> *possible_encoders) {
  int ret = conn_mgr_->GetPossibleEncoders(connector_id, possible_encoders);
  return ret;
}
}  // namespace sde_drm
