/*
* Copyright (c) 2019,2021 The Linux Foundation. All rights reserved.
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

#ifndef __DRM_DPPS_MGR_IMP_H__
#define __DRM_DPPS_MGR_IMP_H__

#include "drm_interface.h"
#include "drm_property.h"
#include "drm_dpps_mgr_intf.h"
#include <mutex>

#define MAX_DISPLAY_COUNT 2

namespace sde_drm {

struct DRMDppsPropInfo {
  /* params to be set in Init */
  uint32_t version;
  DRMProperty prop_enum;
  uint32_t prop_id;
  bool is_event;
  /* params to be set in CacheDppsFeature */
  uint32_t obj_id;
  uint64_t value;
};

class DRMDppsManagerImp : public DRMDppsManagerIntf {
 public:
  ~DRMDppsManagerImp();
  void Init(int fd, drmModeRes* res);
  void CacheDppsFeature(uint32_t obj_id, va_list args);
  void CommitDppsFeatures(drmModeAtomicReq *req, const DRMDisplayToken &tok,
    uint32_t validate_only);
  void GetDppsFeatureInfo(DRMDppsFeatureInfo *info);

 private:
  int GetDrmResources(drmModeRes* res);
  int InitCrtcProps();
  int InitConnProps();
  int InitLtmBuffers(struct DRMDppsFeatureInfo *info);
  int DeInitLtmBuffers();

  struct DRMDppsPropInfo dpps_feature_[kDppsFeaturesMax];
  std::vector<struct DRMDppsPropInfo> dpps_dirty_prop_;
  std::vector<struct DRMDppsPropInfo> dpps_dirty_event_;
  /* key is the prop name, value is prop_id */
  std::map<std::string, uint32_t> dpps_prop_info_map_ = {};
  DRMPropertyManager prop_mgr_ {};
  int conn_id_ = -1;
  int crtc_id_ = -1;
  int drm_fd_ = -1;
  std::vector<std::pair<uint32_t, drm_msm_ltm_buffers_ctrl>> ltm_buffers_ctrl_map_;
  std::vector<std::pair<uint32_t, DRMDppsLtmBuffers>> ltm_buffers_map_;
  std::mutex api_lock_;
};

class DRMDppsManagerDummyImp : public DRMDppsManagerIntf {
 public:
  ~DRMDppsManagerDummyImp() {}
  void Init(int fd, drmModeRes* res) {}
  void CacheDppsFeature(uint32_t obj_id, va_list args) {}
  void CommitDppsFeatures(drmModeAtomicReq *req, const DRMDisplayToken &tok,
    uint32_t validate_only) {}
  void GetDppsFeatureInfo(DRMDppsFeatureInfo *info) {}
};
}  // namespace sde_drm
#endif  // __DRM_DPPS_MGR_IMP_H__
