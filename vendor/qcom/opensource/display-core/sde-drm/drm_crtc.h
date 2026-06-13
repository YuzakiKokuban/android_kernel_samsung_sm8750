/*
* Copyright (c) 2019, 2021, The Linux Foundation. All rights reserved.
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

#ifndef __DRM_CRTC_H__
#define __DRM_CRTC_H__

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <mutex>

#include "drm_interface.h"
#include "drm_utils.h"
#include "drm_pp_manager.h"
#include "drm_property.h"

namespace sde_drm {

class DRMCrtc {
 public:
  DRMCrtc(int fd, uint32_t crtc_index) : fd_(fd), crtc_index_(crtc_index) {}
  void InitAndParse(drmModeCrtc *crtc);
  DRMStatus GetStatus() { return status_; }
  void GetInfo(DRMCrtcInfo *info);
  void SetModeBlobID(uint64_t blob_id);
  bool ConfigureScalerLUT(drmModeAtomicReq *req, uint32_t dir_lut_blob_id,
                          uint32_t cir_lut_blob_id, uint32_t sep_lut_blob_id);
  void PostValidate(bool success);
  void PostCommit(bool success);
  void Perform(DRMOps code, drmModeAtomicReq *req, va_list args);
  int GetIndex() { return crtc_index_; }
  void Dump();
  void Lock();
  void Unlock();
  void GetPPInfo(DRMPPFeatureInfo *info) {
    if (pp_mgr_)
      pp_mgr_->GetPPInfo(info);
  }
  ~DRMCrtc();

 private:
  void ParseProperties();
  void ParseCapabilities(uint64_t blob_id);
  void ParseCompRatio(std::string line, bool real_time);
  void SetROI(drmModeAtomicReq *req, uint32_t obj_id, uint32_t num_roi, DRMRect *crtc_rois,
              DRMRect *spr_rois);
  void SetSolidfillStages(drmModeAtomicReq *req, uint32_t obj_id,
                          const std::vector<DRMSolidfillStage> *solid_fills);
  void SetNoiseLayerConfig(drmModeAtomicReq *req, uint32_t obj_id,
                           const DRMNoiseLayerConfig *noise_cfg);
  void ClearVotesCache();

  // Currently hardcoded to 10. In future we need to query bit depth from driver.
  static const int kSolidFillHwBitDepth = 10;

  int fd_ = -1;
  uint32_t crtc_index_ = {};
  uint64_t mode_blob_id_ = 0;
  drmModeCrtc *drm_crtc_ = {};
  DRMStatus status_ = DRMStatus::FREE;
  DRMCrtcInfo crtc_info_ = {};
  DRMPropertyManager prop_mgr_ {};
  bool is_lut_configured_ = false;
  bool is_lut_validated_ = false;
  bool is_lut_validation_in_progress_ = false;
  std::unique_ptr<DRMPPManager> pp_mgr_{};
  std::unordered_map<uint32_t, uint64_t> tmp_prop_val_map_ {};
  std::unordered_map<uint32_t, uint64_t> committed_prop_val_map_ {};
#if defined SDE_MAX_DIM_LAYERS
  sde_drm_dim_layer_v1 drm_dim_layer_v1_ {};
#endif
#ifdef SDE_MAX_ROI_V1
  sde_drm_roi_v1 roi_v1_ {};
#endif
  sde_drm_dest_scaler_data dest_scale_data_ = {};
  drm_msm_noise_layer_cfg drm_noise_layer_v1_ = {};
};

class DRMCrtcManager {
 public:
  explicit DRMCrtcManager(int fd) : fd_(fd) {}
  void Init(drmModeRes *res);
  void DeInit() {}
  void DumpAll();
  void DumpByID(uint32_t id);
  int Reserve(const std::set<uint32_t> &possible_crtc_indices, DRMDisplayToken *token);
  void Free(DRMDisplayToken *token);
  void Perform(DRMOps code, uint32_t obj_id, drmModeAtomicReq *req, va_list args);
  int GetCrtcInfo(uint32_t crtc_id, DRMCrtcInfo *info);
  void SetScalerLUT(const DRMScalerLUTInfo &lut_info);
  void UnsetScalerLUT();
  void GetPPInfo(uint32_t crtc_id, DRMPPFeatureInfo *info);
  void PostValidate(uint32_t crtc_id, bool success);
  void PostCommit(uint32_t crtc_id, bool success);
  void GetCrtcList(std::vector<uint32_t> *crtc_ids);
  uint32_t GetCrtcCount();

 private:
  int fd_ = -1;
  std::map<uint32_t, std::unique_ptr<DRMCrtc>> crtc_pool_{};
    // GLobal Scaler LUT blobs
  uint32_t dir_lut_blob_id_ = 0;
  uint32_t cir_lut_blob_id_ = 0;
  uint32_t sep_lut_blob_id_ = 0;
  std::mutex lock_;
};

}  // namespace sde_drm

#endif  // __DRM_CRTC_H__
