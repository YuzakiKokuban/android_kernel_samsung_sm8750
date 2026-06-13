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
*/

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following
 * license:
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

#ifdef PP_DRM_ENABLE
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <display/drm/msm_drm_pp.h>
#endif
#include <errno.h>
#include <drm_logger.h>
#include <cstring>
#include <algorithm>
#include <memory>
#include <map>
#include <string>

#include "drm_pp_manager.h"
#include "drm_property.h"

#define __CLASS__ "DRMPPManager"
namespace sde_drm {

DRMPPManager::DRMPPManager(int fd) : fd_(fd) {
}

DRMPPManager::~DRMPPManager() {
#ifdef PP_DRM_ENABLE
  DRMPPPropInfo prop_info = {};

  /* free previously created blob to avoid memory leak */
  for (int i = 0; i < kPPFeaturesMax; i++) {
    prop_info = pp_prop_map_[i];
    for (int j = 0; j < NUM_CACHED_BLOB_ID; j++) {
      if (prop_info.blob_id[j] > 0) {
        drmModeDestroyPropertyBlob(fd_, prop_info.blob_id[j]);
        prop_info.blob_id[j] = 0;
      }
    }
  }
#endif
  fd_ = -1;
}

void DRMPPManager::Init(const DRMPropertyManager &pm , uint32_t object_type) {
  object_type_ = object_type;
  for (uint32_t i = (uint32_t)DRMProperty::INVALID + 1; i < (uint32_t)DRMProperty::MAX; i++) {
    /* parse all the object properties and store the PP properties
     * into DRMPPManager class
    */
    if (!pm.IsPropertyAvailable((DRMProperty)i)) {
      continue;
    }

    if (i >= (uint32_t)DRMProperty::SDE_DSPP_GAMUT_V3 && i <=
        (uint32_t)DRMProperty::SDE_DSPP_GAMUT_V5) {
      pp_prop_map_[kFeatureGamut].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureGamut].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureGamut].version = i - (uint32_t)DRMProperty::SDE_DSPP_GAMUT_V3 + 3;
      DRM_LOGI("Gamut version %d, prop_id %d", pp_prop_map_[kFeatureGamut].version,
               pp_prop_map_[kFeatureGamut].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_GC_V1 && i <=
               (uint32_t)DRMProperty::SDE_DSPP_GC_V2) {
      pp_prop_map_[kFeaturePgc].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePgc].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePgc].version = i - (uint32_t)DRMProperty::SDE_DSPP_GC_V1 + 1;
      DRM_LOGI("Pgc version %d, prop_id %d", pp_prop_map_[kFeaturePgc].version,
               pp_prop_map_[kFeaturePgc].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_IGC_V2 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_IGC_V5) {
      pp_prop_map_[kFeatureIgc].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureIgc].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureIgc].version = i - (uint32_t)DRMProperty::SDE_DSPP_IGC_V2 + 2;
      DRM_LOGI("Igc version %d, prop_id %d", pp_prop_map_[kFeatureIgc].version,
               pp_prop_map_[kFeatureIgc].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_PCC_V3 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_PCC_V6) {
      pp_prop_map_[kFeaturePcc].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePcc].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePcc].version = i - (uint32_t)DRMProperty::SDE_DSPP_PCC_V3 + 3;
      DRM_LOGI("Pcc version %d, prop_id %d", pp_prop_map_[kFeaturePcc].version,
               pp_prop_map_[kFeaturePcc].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_PA_HSIC_V1 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_PA_HSIC_V2) {
      pp_prop_map_[kFeaturePAHsic].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePAHsic].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePAHsic].version = i - (uint32_t)DRMProperty::SDE_DSPP_PA_HSIC_V1 + 1;
      DRM_LOGI("PaHsic version %d, prop_id %d", pp_prop_map_[kFeaturePAHsic].version,
               pp_prop_map_[kFeaturePAHsic].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_PA_SIXZONE_V1 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_PA_SIXZONE_V2) {
      pp_prop_map_[kFeaturePASixZone].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePASixZone].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePASixZone].version = i - (uint32_t)DRMProperty::SDE_DSPP_PA_SIXZONE_V1 + 1;
      DRM_LOGI("SixZone version %d, prop_id %d", pp_prop_map_[kFeaturePASixZone].version,
               pp_prop_map_[kFeaturePASixZone].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_SKIN_V1 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_SKIN_V2) {
      pp_prop_map_[kFeaturePAMemColSkin].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePAMemColSkin].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePAMemColSkin].version = i - (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_SKIN_V1 + 1;
      DRM_LOGI("MemColor skin version %d, prop_id %d", pp_prop_map_[kFeaturePAMemColSkin].version,
               pp_prop_map_[kFeaturePAMemColSkin].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_SKY_V1 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_SKY_V2) {
      pp_prop_map_[kFeaturePAMemColSky].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePAMemColSky].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePAMemColSky].version = i - (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_SKY_V1 + 1;
      DRM_LOGI("MemColor sky version %d, prop_id %d", pp_prop_map_[kFeaturePAMemColSky].version,
               pp_prop_map_[kFeaturePAMemColSky].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_FOLIAGE_V1 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_FOLIAGE_V2) {
      pp_prop_map_[kFeaturePAMemColFoliage].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePAMemColFoliage].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePAMemColFoliage].version = i - (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_FOLIAGE_V1 + 1;
      DRM_LOGI("MemColor foliage version %d, prop_id %d", pp_prop_map_[kFeaturePAMemColFoliage].version,
               pp_prop_map_[kFeaturePAMemColFoliage].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_PROT_V1 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_PROT_V2) {
      pp_prop_map_[kFeaturePAMemColProt].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePAMemColProt].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePAMemColProt].version = i - (uint32_t)DRMProperty::SDE_DSPP_PA_MEMCOL_PROT_V1 + 1;
      DRM_LOGI("MemColor prot version %d, prop_id %d", pp_prop_map_[kFeaturePAMemColProt].version,
               pp_prop_map_[kFeaturePAMemColProt].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_PA_DITHER_V1 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_PA_DITHER_V2) {
      pp_prop_map_[kFeaturePADither].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePADither].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePADither].version = i - (uint32_t)DRMProperty::SDE_DSPP_PA_DITHER_V1 + 1;
      DRM_LOGI("PA Dither version %d, prop_id %d", pp_prop_map_[kFeaturePADither].version,
               pp_prop_map_[kFeaturePADither].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_PP_DITHER_V1 &&
               i <= (uint32_t)DRMProperty::SDE_PP_DITHER_V2) {
      pp_prop_map_[kFeatureDither].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureDither].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureDither].version = i - (uint32_t)DRMProperty::SDE_PP_DITHER_V1 + 1;
      DRM_LOGI("PP dither version %d, prop_id %d", pp_prop_map_[kFeatureDither].version,
               pp_prop_map_[kFeatureDither].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_SPR_DITHER_V1 &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_SPR_DITHER_V1) {
      pp_prop_map_[kFeatureSprDither].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureSprDither].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureSprDither].version = i - (uint32_t)DRMProperty::SDE_DSPP_SPR_DITHER_V1 + 1;
      DRM_LOGI("DSPP SPR dither version %d, prop_id %d", pp_prop_map_[kFeatureSprDither].version,
               pp_prop_map_[kFeatureSprDither].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_VIG_3D_LUT_GAMUT_V5 &&
               i <= (uint32_t)DRMProperty::SDE_VIG_3D_LUT_GAMUT_V6) {
      pp_prop_map_[kFeatureVigGamut].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureVigGamut].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureVigGamut].version = i - (uint32_t)DRMProperty::SDE_VIG_3D_LUT_GAMUT_V5 + 5;
      DRM_LOGI("Vig Gamut version %d, prop_id %d", pp_prop_map_[kFeatureVigGamut].version,
               pp_prop_map_[kFeatureVigGamut].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_VIG_1D_LUT_IGC_V5 &&
               i <= (uint32_t)DRMProperty::SDE_VIG_1D_LUT_IGC_V6) {
      pp_prop_map_[kFeatureVigIgc].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureVigIgc].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureVigIgc].version = i - (uint32_t)DRMProperty::SDE_VIG_1D_LUT_IGC_V5 + 5;
      DRM_LOGI("Vig Igc version %d, prop_id %d", pp_prop_map_[kFeatureVigIgc].version,
               pp_prop_map_[kFeatureVigIgc].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DGM_1D_LUT_IGC_V5 &&
               i <= (uint32_t)DRMProperty::SDE_DGM_1D_LUT_IGC_V5) {
      pp_prop_map_[kFeatureDgmIgc].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureDgmIgc].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureDgmIgc].version = i - (uint32_t)DRMProperty::SDE_DGM_1D_LUT_IGC_V5 + 5;
      DRM_LOGI("Dgm Igc version %d, prop_id %d", pp_prop_map_[kFeatureDgmIgc].version,
               pp_prop_map_[kFeatureDgmIgc].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_DGM_1D_LUT_GC_V5 &&
               i <= (uint32_t)DRMProperty::SDE_DGM_1D_LUT_GC_V5) {
      pp_prop_map_[kFeatureDgmGc].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureDgmGc].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureDgmGc].version = i - (uint32_t)DRMProperty::SDE_DGM_1D_LUT_GC_V5 + 5;
      DRM_LOGI("Dgm Gc version %d, prop_id %d", pp_prop_map_[kFeatureDgmGc].version,
               pp_prop_map_[kFeatureDgmGc].prop_id);
    } else if (i >= (uint32_t)DRMProperty::SDE_PP_CWB_DITHER_V2 &&
               i <= (uint32_t)DRMProperty::SDE_PP_CWB_DITHER_V2) {
      pp_prop_map_[kFeatureCWBDither].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureCWBDither].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureCWBDither].version = i - (uint32_t)DRMProperty::SDE_PP_CWB_DITHER_V2 + 2;
      DRM_LOGI("PP CWB dither version %d, prop_id %d", pp_prop_map_[kFeatureCWBDither].version,
               pp_prop_map_[kFeatureCWBDither].prop_id);
    } else if (i >= (uint32_t)DRMProperty::DIMMING_BL_LUT &&
               i <= (uint32_t)DRMProperty::DIMMING_BL_LUT) {
      pp_prop_map_[kFeatureDimmingBlLut].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureDimmingBlLut].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureDimmingBlLut].version = i - (uint32_t)DRMProperty::DIMMING_BL_LUT;
    } else if (i >= (uint32_t)DRMProperty::DIMMING_DYN_CTRL &&
               i <= (uint32_t)DRMProperty::DIMMING_DYN_CTRL) {
      pp_prop_map_[kFeatureDimmingDynCtrl].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureDimmingDynCtrl].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureDimmingDynCtrl].version = i - (uint32_t)DRMProperty::DIMMING_DYN_CTRL;
    } else if (i >= (uint32_t)DRMProperty::DIMMING_MIN_BL &&
               i <= (uint32_t)DRMProperty::DIMMING_MIN_BL) {
      pp_prop_map_[kFeatureDimmingMinBl].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeatureDimmingMinBl].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeatureDimmingMinBl].version = i - (uint32_t)DRMProperty::DIMMING_MIN_BL;
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_ABA_HIST_CTRL &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_ABA_HIST_CTRL) {
      pp_prop_map_[kFeaturePaHistCtrl].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePaHistCtrl].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePaHistCtrl].version = i - (uint32_t)DRMProperty::SDE_DSPP_ABA_HIST_CTRL;
    } else if (i >= (uint32_t)DRMProperty::SDE_DSPP_ABA_HIST_IRQ &&
               i <= (uint32_t)DRMProperty::SDE_DSPP_ABA_HIST_IRQ) {
      pp_prop_map_[kFeaturePaHistIrq].prop_enum = (DRMProperty)i;
      pp_prop_map_[kFeaturePaHistIrq].prop_id = pm.GetPropertyId((DRMProperty)i);
      pp_prop_map_[kFeaturePaHistIrq].version = i - (uint32_t)DRMProperty::SDE_DSPP_ABA_HIST_IRQ;
    }
  }
  return;
}

void DRMPPManager::GetPPInfo(DRMPPFeatureInfo *info) {
  if (!info)
    return;
  if (info->id > kPPFeaturesMax)
    return;

  info->version = pp_prop_map_[info->id].version;
  info->object_type = object_type_;
  return;
}

void DRMPPManager::SetPPFeature(drmModeAtomicReq *req, uint32_t obj_id, DRMPPFeatureInfo &feature) {
  if (!req) {
      DRM_LOGE("Invalid input param: req %pK", req);
      return;
  }

  /* handle event register/de-register */
  if (feature.is_event) {
    SetPPEvent(obj_id, feature);
    return;
  }

  /* handle feature setting */
  if (feature.id >= kPPFeaturesMax)
    return;

  switch (feature.type) {
    case kPropEnum:
    case kPropRange:
      SetPPRangeProperty(req, obj_id, &pp_prop_map_[feature.id], feature);
      break;
    case kPropBlob:
      SetPPBlobProperty(req, obj_id, &pp_prop_map_[feature.id], feature);
      break;
    default:
      DRM_LOGE("Unsupported feature type %d", feature.type);
      break;
  }

  return;
}

int DRMPPManager::SetPPRangeProperty(drmModeAtomicReq *req, uint32_t obj_id,
                                    struct DRMPPPropInfo *prop_info,
                                    DRMPPFeatureInfo &feature) {
  int ret = DRM_ERR_INVALID;
#ifdef PP_DRM_ENABLE
  uint64_t value = 0;

  if (!feature.payload) {
    DRM_LOGE("invalid payload for feature %d", feature.id);
    return ret;
  }
  value = *((uint64_t *)feature.payload);
  ret = drmModeAtomicAddProperty(req, obj_id, prop_info->prop_id, value);
  if (ret < 0) {
    DRM_LOGE("failed to add property ret %d id %d value %llu", ret, prop_info->prop_id, value);
  } else {
    ret = 0;
  }
#endif
  return ret;
}

int DRMPPManager::SetPPBlobProperty(drmModeAtomicReq *req, uint32_t obj_id,
                                    struct DRMPPPropInfo *prop_info,
                                    DRMPPFeatureInfo &feature) {
  int ret = DRM_ERR_INVALID;
#ifdef PP_DRM_ENABLE
  uint32_t blob_id = 0;

  if (!feature.payload) {
    // feature disable case
    drmModeAtomicAddProperty(req, obj_id, prop_info->prop_id, 0);
    return 0;
  }

  /* free previously created blob for this feature if exist */
  if (prop_info->blob_id[prop_info->blob_id_index] > 0) {
    ret = drmModeDestroyPropertyBlob(fd_, prop_info->blob_id[prop_info->blob_id_index]);
    if (ret) {
      DRM_LOGE("failed to destroy property blob for feature %d, ret = %d", feature.id, ret);
      return ret;
    } else {
      prop_info->blob_id[prop_info->blob_id_index] = 0;
    }
  }

  ret = drmModeCreatePropertyBlob(fd_, feature.payload, feature.payload_size, &blob_id);
  if (ret || blob_id == 0) {
    DRM_LOGE("failed to create property blob ret %d, blob_id = %d", ret, blob_id);
    return DRM_ERR_INVALID;
  }

  prop_info->blob_id[prop_info->blob_id_index] = blob_id;
  prop_info->blob_id_index = (++prop_info->blob_id_index) % NUM_CACHED_BLOB_ID;
  drmModeAtomicAddProperty(req, obj_id, prop_info->prop_id, blob_id);

#endif
  return ret;
}

void DRMPPManager::SetPPEvent(uint32_t obj_id, DRMPPFeatureInfo &feature) {
#ifdef PP_DRM_ENABLE
  int ret  = 0;
  bool enable;
  struct drm_msm_event_req event_req = {};

  if (!feature.payload || feature.payload_size != sizeof(bool)) {
    DRM_LOGE("Invalid input params: payload %pK payload_size %d exp size %d",
             feature.payload, feature.payload_size, sizeof(bool));
    return;
  }

  enable = *(reinterpret_cast<bool *>(feature.payload));
  event_req.object_id = obj_id;
  event_req.object_type = object_type_;
  event_req.event = feature.event_type;
  if (enable)
    ret = drmIoctl(feature.drm_fd, DRM_IOCTL_MSM_REGISTER_EVENT, &event_req);
  else
    ret = drmIoctl(feature.drm_fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &event_req);
  if (ret) {
    ret = -errno;
    if (ret == -EALREADY) {
      DRM_LOGI("Duplicated request to set event 0x%x, object_id %u, object_type 0x%x, enable %d",
          event_req.event, event_req.object_id, object_type_, enable);
    } else if (ret == -ENOENT || ret == -ENODEV || ret == -EACCES) {
      DRM_LOGW("Event 0x%x, object_id %u, object_type 0x%x, enable %d, ret %d",
          event_req.event, event_req.object_id, object_type_, enable, ret);
    } else {
      DRM_LOGE("Failed to set event 0x%x, object_id %u, object_type 0x%x, enable %d, ret %d",
          event_req.event, event_req.object_id, object_type_, enable, ret);
    }
  } else {
    DRM_LOGI("set event, 0x%x, object_id %u, object_type 0x%x, enable %d",
             event_req.event, event_req.object_id, object_type_, enable);
  }
#endif
  return;
}

}  // namespace sde_drm
