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

#ifndef __DRM_PP_MANAGER_H__
#define __DRM_PP_MANAGER_H__

#include <limits>
#include "drm_utils.h"
#include "drm_interface.h"
#include "drm_property.h"

#define NUM_CACHED_BLOB_ID 2

namespace sde_drm {

struct DRMPPPropInfo {
  DRMProperty prop_enum;
  uint32_t version = std::numeric_limits<uint32_t>::max();
  uint32_t prop_id;
  uint32_t blob_id[NUM_CACHED_BLOB_ID];
  uint32_t blob_id_index;
};

class DRMPPManager {
 public:
  explicit DRMPPManager(int fd);
  ~DRMPPManager();
  void Init(const DRMPropertyManager &pm, uint32_t object_type);
  void DeInit() {}
  void GetPPInfo(DRMPPFeatureInfo *info);
  void SetPPFeature(drmModeAtomicReq *req, uint32_t obj_id, DRMPPFeatureInfo &feature);

 private:
  int SetPPBlobProperty(drmModeAtomicReq *req, uint32_t obj_id, struct DRMPPPropInfo *prop_info,
                        DRMPPFeatureInfo &feature);
  int SetPPRangeProperty(drmModeAtomicReq *req, uint32_t obj_id, struct DRMPPPropInfo *prop_info,
                        DRMPPFeatureInfo &feature);
  void SetPPEvent(uint32_t obj_id, DRMPPFeatureInfo &feature);

  int fd_ = -1;
  uint32_t object_type_ = std::numeric_limits<uint32_t>::max();
  DRMPPPropInfo pp_prop_map_[kPPFeaturesMax] = {};
};

}  // namespace sde_drm
#endif  // __DRM_PP_MANAGER_H__
