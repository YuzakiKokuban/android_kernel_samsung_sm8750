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
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __DRM_PANEL_FEATURE_MGR_H__
#define __DRM_PANEL_FEATURE_MGR_H__

#include <vector>
#include <mutex>
#include <deque>

#include "drm_interface.h"
#include "drm_property.h"
#include "drm_panel_feature_mgr_intf.h"

namespace sde_drm {

class DRMPanelFeatureMgr : public DRMPanelFeatureMgrIntf {
 public:
  virtual ~DRMPanelFeatureMgr() {}
  void Init(int fd, drmModeRes* res);
  void Deinit();
  void GetPanelFeatureInfo(DRMPanelFeatureInfo *info);
  void CachePanelFeature(const DRMPanelFeatureInfo &info);
  void CommitPanelFeatures(drmModeAtomicReq *req, const DRMDisplayToken &token);
  void NullCommitPanelFeatures(drmModeAtomicReq *req, const DRMDisplayToken &token);
  void ResetPanelFeatures(drmModeAtomicReq *req, const DRMDisplayToken &token);
  void MarkForNullCommit(const DRMDisplayToken &token, const DRMPanelFeatureID &id);

 private:
  int InitObjectProps(int obj_id, int obj_type);
  void ParseCapabilities(uint32_t blob_id, char* value, uint32_t max_len, const std::string str);
  void ParseDsppCapabilities(uint32_t blob_id, std::vector<int> *values, uint32_t *size,
                             const std::string str);
  void ParsePanelId(uint32_t blob_id, DRMPanelFeatureInfo *info);
  void ParseDemuraResources(drmModePropertyRes *prop, uint64_t value, DRMPanelFeatureInfo *info);
  void ApplyDirtyFeature(drmModeAtomicReq *req, const DRMDisplayToken &token,
                         DRMPanelFeatureInfo &info);

  std::mutex lock_;
  int dev_fd_ = -1;
  drmModeRes* drm_res_ = nullptr;
  DRMPropertyManager prop_mgr_ {};
  std::vector<struct DRMPanelFeatureInfo> dirty_features_ {};
  std::map<DRMPanelFeatureID, DRMProperty> drm_property_map_ {};
  std::map<DRMPanelFeatureID, DRMPropType> drm_prop_type_map_ {};
  std::map<DRMPanelFeatureID, std::deque<uint32_t>> drm_prop_blob_ids_cache_{};
  std::array<DRMPanelFeatureInfo, kDRMPanelFeatureMax> feature_info_tbl_ {};
  std::map<uint32_t /* obj_id */, DRMPanelFeatureID> apply_in_null_commit_ {};
};

}  // namespace sde_drm

#endif  // __DRM_PANEL_FEATURE_MGR_H__

