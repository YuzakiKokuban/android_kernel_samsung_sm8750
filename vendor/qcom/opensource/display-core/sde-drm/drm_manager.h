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
* Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __DRM_MANAGER_H__
#define __DRM_MANAGER_H__

#include <drm_interface.h>
#include <utils/multi_core_instantiator.h>
#include <mutex>
#include "drm_dpps_mgr_intf.h"
#include "drm_panel_feature_mgr_intf.h"

namespace sde_drm {

class DRMAtomicReqInterface;
class DRMPlaneManager;
class DRMPlane;
class DRMConnectorManager;
class DRMEncoderManager;
class DRMConnnector;
class DRMCrtcManager;
class DRMCrtc;

class DRMManager : public DRMManagerInterface {
 public:
  virtual ~DRMManager();
  virtual int RegisterDisplay(DRMDisplayType disp_type, DRMDisplayToken *token,
                              bool has_cac_loopback);
  virtual int RegisterDisplay(int32_t display_id, DRMDisplayToken *token);
  virtual void UnregisterDisplay(DRMDisplayToken *token);
  virtual void GetPlanesInfo(DRMPlanesInfo *info);
  virtual int GetCrtcInfo(uint32_t crtc_id, DRMCrtcInfo *info);
  virtual int GetConnectorInfo(uint32_t conn_id, DRMConnectorInfo *info);
  virtual int GetConnectorsInfo(DRMConnectorsInfo *infos);
  virtual int GetEncoderInfo(uint32_t encoder_id, DRMEncoderInfo *info);
  virtual int GetEncodersInfo(DRMEncodersInfo *infos);
  virtual void GetCrtcPPInfo(uint32_t crtc_id, DRMPPFeatureInfo *info);
  virtual int CreateAtomicReq(const DRMDisplayToken &token, DRMAtomicReqInterface **intf);
  virtual int DestroyAtomicReq(DRMAtomicReqInterface *intf);
  virtual int SetScalerLUT(const DRMScalerLUTInfo &lut_info);
  virtual int UnsetScalerLUT();
  virtual void GetDppsFeatureInfo(DRMDppsFeatureInfo *info);
  virtual void GetPanelFeature(DRMPanelFeatureInfo *info);
  virtual void SetPanelFeature(const DRMPanelFeatureInfo &info);
  virtual void MapPlaneToConnector(std::map<uint32_t, uint32_t> *plane_to_connector);
  virtual void GetRequiredDemuraFetchResourceCount(std::map<uint32_t, uint8_t>
                                                   *required_demura_fetch_cnt);
  virtual void GetInitialDemuraInfo(std::vector<uint32_t> *initial_demura_planes);
  virtual void MarkPanelFeatureForNullCommit(const DRMDisplayToken &token,
                                             const DRMPanelFeatureID &id);
  virtual uint32_t GetCrtcCount();

  virtual int GetPossibleEncoders(uint32_t connector_id, std::set<uint32_t> *possible_encoders);

  DRMPlaneManager *GetPlaneMgr();
  DRMConnectorManager *GetConnectorMgr();
  DRMEncoderManager *GetEncoderMgr();
  DRMCrtcManager *GetCrtcMgr();
  DRMDppsManagerIntf *GetDppsMgrIntf();
  DRMPanelFeatureMgrIntf *GetPanelFeatureMgrIntf();

  static DRMManager *GetInstance(int fd);
  static void Destroy(int fd);

 private:
  int Init(int drm_fd);

  int fd_ = -1;
  DRMPlaneManager *plane_mgr_ = {};
  DRMConnectorManager *conn_mgr_ = {};
  DRMEncoderManager *encoder_mgr_ = {};
  DRMCrtcManager *crtc_mgr_ = {};
  DRMDppsManagerIntf *dpps_mgr_intf_ = {};
  DRMPanelFeatureMgrIntf *panel_feature_mgr_intf_ = {};

  static sdm::MultiCoreInstance<int, DRMManager*> s_drm_instance;
  static std::mutex s_lock;
};

}  // namespace sde_drm
#endif  // __DRM_MANAGER_H__
