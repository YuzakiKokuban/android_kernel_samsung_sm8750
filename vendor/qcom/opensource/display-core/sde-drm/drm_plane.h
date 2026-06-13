/*
* Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
*
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __DRM_PLANE_H__
#define __DRM_PLANE_H__

#include <display/drm/sde_drm.h>
#include <drm_interface.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <mutex>

#include "drm_property.h"
#include "drm_pp_manager.h"

namespace sde_drm {

class DRMPlaneManager;

enum DRMPlaneLutState {
  kInactive,  //  Lut is not in use, default
  kActive,    //  Lut is in use
  kDirty,     //  Plane was unset while being LUT was active, mark LUT as dirty
              //  to make sure it's cleared the next time plane is used
};

class DRMPlane {
 public:
  explicit DRMPlane(int fd, uint32_t priority);
  ~DRMPlane();
  void InitAndParse(drmModePlane *plane);
  void GetId(uint32_t *id) { *id = drm_plane_->plane_id; }
  void GetType(DRMPlaneType *type) { *type = plane_type_info_.type; }
  void GetPriority(uint32_t *priority) { *priority = priority_; }
  void GetAssignedCrtc(uint32_t *crtc_id) { *crtc_id = assigned_crtc_id_; }
  void GetRequestedCrtc(uint32_t *crtc_id) { *crtc_id = requested_crtc_id_; }
  void GetCrtc(uint32_t *crtc_id) { *crtc_id = drm_plane_->crtc_id; }
  void SetAssignedCrtc(uint32_t crtc_id) { assigned_crtc_id_ = crtc_id; }
  void SetRequestedCrtc(uint32_t crtc_id) { requested_crtc_id_ = crtc_id; }
  bool SetScalerConfig(drmModeAtomicReq *req, uint64_t handle);
  bool SetCscConfig(drmModeAtomicReq *req, DRMCscType csc_type);
  bool ConfigureScalerLUT(drmModeAtomicReq *req, uint32_t dir_lut_blob_id,
                          uint32_t cir_lut_blob_id, uint32_t sep_lut_blob_id);
  const DRMPlaneTypeInfo& GetPlaneTypeInfo() { return plane_type_info_; }
  void SetDecimation(drmModeAtomicReq *req, uint32_t prop_id, uint32_t prop_value);
  void SetExclRect(drmModeAtomicReq *req, DRMRect rect);
  void SetSrcExtnRect(drmModeAtomicReq *req, DRMRect rect);
  void SetDstExtnRect(drmModeAtomicReq *req, DRMRect rect);
  void SetImgSizeRect(drmModeAtomicReq *req, DRMRect rect);
  void Perform(DRMOps code, drmModeAtomicReq *req, va_list args);
  void Dump();
  void SetMultiRectMode(drmModeAtomicReq *req, DRMMultiRectMode drm_multirect_mode);
  void Unset(bool is_commit, drmModeAtomicReq *req);
  void PostValidate(uint32_t crtc_id, bool success);
  void PostCommit(uint32_t crtc_id, bool success);
  bool SetDgmCscConfig(drmModeAtomicReq *req, uint64_t handle);
  void UpdatePPLutFeatureInuse(DRMPPFeatureInfo *data);
  void ResetColorLUTs(bool update_state, drmModeAtomicReq *req);
  void ResetColorLUTState(DRMTonemapLutType lut_type, bool update_state, drmModeAtomicReq *req);
  void ResetColorLUT(DRMPPFeatureID id, drmModeAtomicReq *req);
  void ResetCache(drmModeAtomicReq *req);
  void ResetPlanesLUT(drmModeAtomicReq *req);
  void GetIndex(uint8_t *index) { *index = plane_type_info_.pipe_idx; }
  void GetRect(uint8_t *rect) { *rect = plane_type_info_.master_plane_id ? 1 : 0; }
  bool SetFp16CscConfig(drmModeAtomicReq *req, DRMFp16CscType csc_type);
  bool SetFp16IgcConfig(drmModeAtomicReq *req, uint32_t igc_en);
  bool SetFp16UnmultConfig(drmModeAtomicReq *req, uint32_t unmult_en);
  bool SetFp16GcConfig(drmModeAtomicReq *req, drm_msm_fp16_gc *fp16_gc_config);
  void UnsetFp16CscConfig();
  void UnsetFp16GcConfig();
#ifdef UCSC_SUPPORTED
  void SetUcscCscConfig(drmModeAtomicReq *req, drm_msm_ucsc_csc *ucsc_csc_config);
  void UnsetUcscCscConfig();
#endif
  void SetCacType(drmModeAtomicReq *req, DRMCacMode drm_cac_mode);

 private:
  typedef std::map<DRMProperty, std::tuple<uint64_t, drmModePropertyRes *>> PropertyMap;
  void ParseProperties();
  void GetTypeInfo(const PropertyMap &props);
  void PerformWrapper(DRMOps code, drmModeAtomicReq *req, ...);

  int fd_ = -1;
  uint32_t priority_ = 0;
  drmModePlane *drm_plane_ = {};
  DRMPlaneTypeInfo plane_type_info_{};
  uint32_t assigned_crtc_id_ = 0;
  uint32_t requested_crtc_id_ = 0;
  DRMPropertyManager prop_mgr_ {};
  bool has_excl_rect_ = false;
  drm_clip_rect excl_rect_copy_ = {};
  drm_clip_rect src_rect_extn_copy_ = {};
  drm_clip_rect dst_rect_extn_copy_ = {};
  drm_clip_rect img_size_rect_copy_ = {};
  std::unique_ptr<DRMPPManager> pp_mgr_ {};
  std::unordered_map<uint32_t, uint64_t> tmp_prop_val_map_ {};
  std::unordered_map<uint32_t, uint64_t> committed_prop_val_map_ {};

  // Only applicable to planes that have scaler
  sde_drm_scaler_v2 scaler_v2_config_copy_ = {};
  sde_drm_csc_v1 csc_config_copy_ = {};
  bool is_lut_configured_ = false;

  bool dgm_csc_in_use_ = false;
  // Tone-mapping lut properties
  DRMPlaneLutState dgm_1d_lut_igc_state_ = kInactive;
  DRMPlaneLutState dgm_1d_lut_gc_state_ = kInactive;
  DRMPlaneLutState vig_1d_lut_igc_state_ = kInactive;
  DRMPlaneLutState vig_3d_lut_gamut_state_ = kInactive;

  // FP16 / UCSC properties
  uint32_t fp16_csc_blob_id_ = 0;
  DRMFp16CscType fp16_csc_type_ = kFP16CscTypeMax;
  uint32_t fp16_gc_blob_id_ = 0;
  drm_msm_fp16_gc fp16_gc_config_ = {.flags = 0, .mode = FP16_GC_MODE_INVALID};
#ifdef UCSC_SUPPORTED
  uint32_t ucsc_csc_blob_id_ = 0;
#endif
};

class DRMPlaneManager {
 public:
  explicit DRMPlaneManager(int fd);
  void Init();
  void DeInit() {}
  void GetPlanesInfo(DRMPlanesInfo *info);
  void DumpAll();
  void DumpByID(uint32_t id);
  void Perform(DRMOps code, uint32_t obj_id, drmModeAtomicReq *req, va_list args);
  void UnsetUnusedResources(uint32_t crtc_id, bool is_commit, drmModeAtomicReq *req);
  void ResetColorLutsOnUsedPlanes(uint32_t crtc_id, bool is_commit, drmModeAtomicReq *req);
  void RetainPlanes(uint32_t crtc_id);
  void SetScalerLUT(const DRMScalerLUTInfo &lut_info);
  void UnsetScalerLUT();
  void PostValidate(uint32_t crtc_id, bool success);
  void PostCommit(uint32_t crtc_id, bool success);
  void ResetCache(drmModeAtomicReq *req, uint32_t crtc_id);
  void ResetPlanesLUT(drmModeAtomicReq *req);
  void MapPlaneToCrtc(std::map<uint32_t, uint32_t> *plane_to_crtc);
  void GetPlaneIdsFromDescriptions(FetchResourceList &descriptions,
                                   std::vector<uint32_t> *plane_ids);

 private:
  void Perform(DRMOps code, drmModeAtomicReq *req, uint32_t obj_id, ...);

  int fd_ = -1;
  // Map of plane id to DRMPlane *
  std::map<uint32_t, std::unique_ptr<DRMPlane>> plane_pool_{};
  // Global Scaler LUT blobs
  uint32_t dir_lut_blob_id_ = 0;
  uint32_t cir_lut_blob_id_ = 0;
  uint32_t sep_lut_blob_id_ = 0;
  std::mutex lock_;
};

}  // namespace sde_drm

#endif  // __DRM_PLANE_H__
