// clang-format off
/*
* Copyright (c) 2017-2018, 2020-2021, The Linux Foundation. All rights reserved.
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
*/

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// clang-format on

#define __CLASS__ "HWColorManagerDRM"

#include <array>
#include <map>
#include <cstring>
#include <vector>
#include <new>

#ifdef PP_DRM_ENABLE
#include <display/drm/msm_drm_pp.h>
#endif
#include <utils/debug.h>
#include "hw_color_manager_drm.h"

#ifdef PP_DRM_ENABLE
static const uint32_t kPgcDataMask = 0x3FF;
static const uint32_t kPgcShift = 16;

static const uint32_t kIgcDataMask = 0xFFF;
static const uint32_t kIgcShift = 16;
static const uint32_t kRegU16Mask = 0xFFFF;

#ifdef DRM_MSM_PA_HSIC
static const uint32_t kPAHueMask = (1 << 12);
static const uint32_t kPASatMask = (1 << 13);
static const uint32_t kPAValMask = (1 << 14);
static const uint32_t kPAContrastMask = (1 << 15);
#endif

#ifdef DRM_MSM_SIXZONE
static const uint32_t kSixZoneP0Mask = 0x0FFF;
static const uint32_t kSixZoneP1Mask = 0x0FFF0FFF;
#ifdef SIXZONE_SV_ENABLE
static const uint32_t kSixZoneP2Mask = 0x0FFF0FFF;
#endif
static const uint32_t kSixZoneHueMask = (1 << 16);
static const uint32_t kSixZoneSatMask = (1 << 17);
static const uint32_t kSixZoneValMask = (1 << 18);
#endif

#ifdef DRM_MSM_MEMCOL
static const uint32_t kMemColorProtHueMask = (1 << 0);
static const uint32_t kMemColorProtSatMask = (1 << 1);
static const uint32_t kMemColorProtValMask = (1 << 2);
static const uint32_t kMemColorProtContMask = (1 << 3);
static const uint32_t kMemColorProtSixZoneMask = (1 << 4);
static const uint32_t kMemColorProtBlendMask = (1 << 5);

static const uint32_t kMemColorProtMask = \
  (kMemColorProtHueMask | kMemColorProtSatMask | kMemColorProtValMask | \
    kMemColorProtContMask | kMemColorProtSixZoneMask | kMemColorProtBlendMask);

static const uint32_t kMemColorSkinMask = (1 << 19);
static const uint32_t kMemColorSkyMask = (1 << 20);
static const uint32_t kMemColorFolMask = (1 << 21);

static const uint32_t kSourceFeatureV5 = 5;
#endif
#endif

namespace sdm {

typedef std::map<uint32_t, std::vector<DRMPPFeatureID>> DrmPPFeatureMap;

static const DrmPPFeatureMap g_dspp_map = {
  {kGlobalColorFeaturePcc,      {kFeaturePcc}},
  {kGlobalColorFeatureIgc,      {kFeatureIgc}},
  {kGlobalColorFeaturePgc,      {kFeaturePgc}},
  {kMixerColorFeatureGc,        {kFeatureMixerGc}},
  {kGlobalColorFeaturePaV2,     {kFeaturePASixZone,
                                 kFeaturePAHsic,
                                 kFeaturePAMemColSkin,
                                 kFeaturePAMemColSky,
                                 kFeaturePAMemColFoliage,
                                 kFeaturePAMemColProt}},
  {kGlobalColorFeatureDither,   {kFeatureDither}},
  {kGlobalColorFeatureGamut,    {kFeatureGamut}},
  {kGlobalColorFeaturePADither, {kFeaturePADither}},
  {kGlobalColorFeatureCWBDither, {kFeatureCWBDither}},
};

static const DrmPPFeatureMap g_vig_map = {
  {kSourceColorFeatureIgc,      {kFeatureVigIgc}},
  {kSourceColorFeatureGamut,    {kFeatureVigGamut}},
};

static const DrmPPFeatureMap g_dgm_map = {
  {kSourceColorFeatureIgc,      {kFeatureDgmIgc}},
  {kSourceColorFeatureGc,       {kFeatureDgmGc}},
};

static const std::array<const DrmPPFeatureMap *,
                        static_cast<int>(kPPBlockMax)> feature_map_list = {
  {&g_dspp_map, &g_vig_map, &g_dgm_map}
};

DisplayError (*HWColorManagerDrm::pp_features_[])(const PPFeatureInfo &,
                                                    DRMPPFeatureInfo *) = {
  [kFeaturePcc] = &HWColorManagerDrm::GetDrmPCC,
  [kFeatureIgc] = &HWColorManagerDrm::GetDrmIGC,
  [kFeaturePgc] = &HWColorManagerDrm::GetDrmPGC,
  [kFeatureMixerGc] = &HWColorManagerDrm::GetDrmMixerGC,
  [kFeaturePaV2] = NULL,
  [kFeatureDither] = &HWColorManagerDrm::GetDrmDither,
  [kFeatureSprDither] = &HWColorManagerDrm::GetDrmDither,
  [kFeatureGamut] = &HWColorManagerDrm::GetDrmGamut,
  [kFeaturePADither] = &HWColorManagerDrm::GetDrmPADither,
  [kFeaturePAHsic] = &HWColorManagerDrm::GetDrmPAHsic,
  [kFeaturePASixZone] = &HWColorManagerDrm::GetDrmPASixZone,
  [kFeaturePAMemColSkin] = &HWColorManagerDrm::GetDrmPAMemColSkin,
  [kFeaturePAMemColSky] = &HWColorManagerDrm::GetDrmPAMemColSky,
  [kFeaturePAMemColFoliage] = &HWColorManagerDrm::GetDrmPAMemColFoliage,
  [kFeaturePAMemColProt] = &HWColorManagerDrm::GetDrmPAMemColProt,
  [kFeatureDgmIgc] = &HWColorManagerDrm::GetDrmIGC,
  [kFeatureDgmGc] = &HWColorManagerDrm::GetDrmPGC,
  [kFeatureVigIgc] = &HWColorManagerDrm::GetDrmIGC,
  [kFeatureVigGamut] = &HWColorManagerDrm::GetDrmGamut,
  [kFeatureCWBDither] = &HWColorManagerDrm::GetDrmDither,
};

uint32_t HWColorManagerDrm::GetFeatureVersion(const DRMPPFeatureInfo &feature) {
  uint32_t version = PPFeatureVersion::kSDEPpVersionInvalid;

  switch (feature.id) {
    case kFeaturePcc:
      if (feature.version == 1) {
        version = PPFeatureVersion::kSDEPccV17;
      } else if (feature.version == 4) {
        version = PPFeatureVersion::kSDEPccV4;
      } else if (feature.version == 6) {
        version = PPFeatureVersion::kSDEPccV6;
      }
      break;
    case kFeatureIgc:
      if (feature.version == 3) {
        version = PPFeatureVersion::kSDEIgcV30;
      } else if (feature.version == 4) {
        version = PPFeatureVersion::kSDEIgcV40;
      } else if (feature.version == 5) {
        version = PPFeatureVersion::kSDEIgcV50;
      }
      break;
    case kFeaturePgc:
      if (feature.version == 1) {
        version = PPFeatureVersion::kSDEPgcV17;
      } else if (feature.version == 2) {
        version = PPFeatureVersion::kSDEPgcV2;
      }
      break;
    case kFeatureMixerGc:
        version = PPFeatureVersion::kSDEPgcV17;
      break;
    case kFeaturePASixZone:
      if (feature.version == 1) {
        version = PPFeatureVersion::kSDEPaV17;
      } else if (feature.version == 2) {
        version = PPFeatureVersion::kSDEPaV2;
      }
      break;
    case kFeaturePAHsic:
    case kFeaturePAMemColSkin:
    case kFeaturePAMemColSky:
    case kFeaturePAMemColFoliage:
    case kFeaturePAMemColProt:
      if (feature.version == 1)
        version = PPFeatureVersion::kSDEPaV17;
      break;
    case kFeatureDither:
    case kFeatureSprDither:
        version = PPFeatureVersion::kSDEDitherV17;
      break;
    case kFeatureGamut:
      if (feature.version == 1)
        version = PPFeatureVersion::kSDEGamutV17;
      else if (feature.version == 4)
        version = PPFeatureVersion::kSDEGamutV4;
      break;
    case kFeaturePADither:
        version = PPFeatureVersion::kSDEPADitherV17;
      break;
    case kFeatureCWBDither:
        version = PPFeatureVersion::kSDECWBDitherV2;
      break;
    default:
      break;
  }
  return version;
}

DisplayError HWColorManagerDrm::ToDrmFeatureId(PPBlock block, uint32_t id,
  std::vector<DRMPPFeatureID> *drm_id) {
  if (block < kDSPP || block >= kPPBlockMax) {
    DLOGE("Invalid input parameter, block = %d", (int) block);
    return kErrorParameters;
  }

  if (!drm_id) {
    DLOGE("Invalid output parameter, block = %d", (int) block);
    return kErrorParameters;
  }

  auto map = feature_map_list[block];
  auto drm_features = map->find(id);
  if (drm_features == map->end()) {
    return kErrorParameters;
  }
  for (DRMPPFeatureID fid : drm_features->second)
    drm_id->push_back(fid);

  return kErrorNone;
}

DisplayError HWColorManagerDrm::GetDrmFeature(PPFeatureInfo *in_data,
                                              DRMPPFeatureInfo *out_data,
                                              bool force_disable) {
  DisplayError ret = kErrorParameters;
  uint32_t flags = 0;

  if (!in_data || !out_data) {
    DLOGE("Invalid input parameter, in_data or out_data is NULL");
    return ret;
  }

  /* Cache and override the enable_flags_ if force_disable is requested */
  if (force_disable) {
    flags = in_data->enable_flags_;
    in_data->enable_flags_ = kOpsDisable;
  }

  if (pp_features_[out_data->id])
    ret = pp_features_[out_data->id](*in_data, out_data);


  /* Restore the original enable_flags_ */
  if (force_disable) {
    in_data->enable_flags_ = flags;
  }

  return ret;
}

void HWColorManagerDrm::FreeDrmFeatureData(DRMPPFeatureInfo *feature) {
  if (feature && feature->payload) {
#ifdef PP_DRM_ENABLE
    void *ptr = feature->payload;
#endif

    switch (feature->id) {
      case kFeaturePcc: {
#ifdef PP_DRM_ENABLE
        drm_msm_pcc *pcc = reinterpret_cast<drm_msm_pcc *>(ptr);
        delete pcc;
#endif
        break;
      }
      case kFeatureIgc:
      case kFeatureDgmIgc:
      case kFeatureVigIgc: {
#ifdef PP_DRM_ENABLE
        drm_msm_igc_lut *igc = reinterpret_cast<drm_msm_igc_lut *>(ptr);
        delete igc;
#endif
        break;
      }
      case kFeaturePgc:
      case kFeatureDgmGc: {
#ifdef PP_DRM_ENABLE
        drm_msm_pgc_lut *pgc = reinterpret_cast<drm_msm_pgc_lut *>(ptr);
        delete pgc;
#endif
        break;
      }
      case kFeatureDither:
      case kFeatureCWBDither:
      case kFeatureSprDither: {
#ifdef PP_DRM_ENABLE
        drm_msm_dither *dither = reinterpret_cast<drm_msm_dither *>(ptr);
        delete dither;
#endif
        break;
      }
      case kFeatureGamut:
      case kFeatureVigGamut: {
#ifdef PP_DRM_ENABLE
        drm_msm_3d_gamut *gamut = reinterpret_cast<drm_msm_3d_gamut *>(ptr);
        delete gamut;
#endif
        break;
      }
      case kFeaturePADither: {
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_PA_DITHER)
        drm_msm_pa_dither *pa_dither = reinterpret_cast<drm_msm_pa_dither *>(ptr);
        delete pa_dither;
#endif
        break;
      }
      case kFeaturePAHsic: {
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_PA_HSIC)
        drm_msm_pa_hsic *hsic = reinterpret_cast<drm_msm_pa_hsic *>(ptr);
        delete hsic;
#endif
        break;
      }
      case kFeaturePASixZone: {
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_SIXZONE)
        drm_msm_sixzone *sixzone = reinterpret_cast<drm_msm_sixzone *>(ptr);
        delete sixzone;
#endif
        break;
      }
      case kFeaturePAMemColSkin:
      case kFeaturePAMemColSky:
      case kFeaturePAMemColFoliage:
      case kFeaturePAMemColProt: {
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
        drm_msm_memcol *memcol = reinterpret_cast<drm_msm_memcol *>(ptr);
        delete memcol;
#endif
        break;
      }
      case kFeatureMixerGc:
      case kFeaturePaV2:
      default: {
        DLOGE("Invalid feature: %d\n", feature->id);
        return;
      }
    }
    feature->payload = nullptr;
  }
}

DisplayError HWColorManagerDrm::GetDrmPCC(const PPFeatureInfo &in_data,
                                          DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  struct SDEPccV4Cfg *sde_pcc = NULL;
  struct SDEPccV4Coeff *sde_pcc_coeffs = NULL;
  struct drm_msm_pcc *mdp_pcc = NULL;
  struct drm_msm_pcc_coeff *mdp_pcc_coeffs = NULL;
  uint32_t i = 0;

  switch (in_data.feature_version_) {
  case PPFeatureVersion::kSDEPccV4:
  case PPFeatureVersion::kSDEPccV6:
    sde_pcc = (struct SDEPccV4Cfg *)in_data.GetConfigData();
    break;
  default:
    DLOGE("Unsupported pcc feature version: %d", in_data.feature_version_);
    return kErrorParameters;
  }

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = sizeof(struct drm_msm_pcc);

  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_pcc = new drm_msm_pcc();
  if (!mdp_pcc) {
    DLOGE("Failed to allocate memory for pcc");
    return kErrorMemory;
  }

  mdp_pcc->flags = 0;
#ifdef PCC_BEFORE
  if (sde_pcc->flags & SDM_PCC_BEFORE_POS)
    mdp_pcc->flags |= PCC_BEFORE;
#endif

  for (i = 0; i < kMaxPCCChanel; i++) {
    switch (i) {
    case 0:
      sde_pcc_coeffs = &sde_pcc->red;
      mdp_pcc_coeffs = &mdp_pcc->r;
      mdp_pcc->r_rr = sde_pcc_coeffs->rr;
      mdp_pcc->r_gg = sde_pcc_coeffs->gg;
      mdp_pcc->r_bb = sde_pcc_coeffs->bb;
      break;
    case 1:
        sde_pcc_coeffs = &sde_pcc->green;
        mdp_pcc_coeffs = &mdp_pcc->g;
        mdp_pcc->g_rr = sde_pcc_coeffs->rr;
        mdp_pcc->g_gg = sde_pcc_coeffs->gg;
        mdp_pcc->g_bb = sde_pcc_coeffs->bb;
      break;
    case 2:
        sde_pcc_coeffs = &sde_pcc->blue;
        mdp_pcc_coeffs = &mdp_pcc->b;
        mdp_pcc->b_rr = sde_pcc_coeffs->rr;
        mdp_pcc->b_gg = sde_pcc_coeffs->gg;
        mdp_pcc->b_bb = sde_pcc_coeffs->bb;
      break;
    }
    mdp_pcc_coeffs->c = sde_pcc_coeffs->c;
    mdp_pcc_coeffs->r = sde_pcc_coeffs->r;
    mdp_pcc_coeffs->g = sde_pcc_coeffs->g;
    mdp_pcc_coeffs->b = sde_pcc_coeffs->b;
    mdp_pcc_coeffs->rg = sde_pcc_coeffs->rg;
    mdp_pcc_coeffs->gb = sde_pcc_coeffs->gb;
    mdp_pcc_coeffs->rb = sde_pcc_coeffs->rb;
    mdp_pcc_coeffs->rgb = sde_pcc_coeffs->rgb;
  }
  out_data->payload = mdp_pcc;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmIGC(const PPFeatureInfo &in_data,
                                          DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  struct SDEIgcV30LUTData *sde_igc = nullptr;
  struct drm_msm_igc_lut *mdp_igc;
  uint32_t *c0_c1_data_ptr = NULL;
  uint32_t *c2_data_ptr = NULL;
  struct SDEIgcV50LUTData *sde_igc_v5 = nullptr;

  switch (in_data.feature_version_) {
  case PPFeatureVersion::kSDEIgcV30:
  case PPFeatureVersion::kSDEIgcV40:
  case kSourceFeatureV5:
    sde_igc = (struct SDEIgcV30LUTData *) in_data.GetConfigData();
    break;
  case PPFeatureVersion::kSDEIgcV50:
    sde_igc_v5 = (struct SDEIgcV50LUTData *)in_data.GetConfigData();
    break;
  default:
    DLOGE("Unsupported igc feature version: %d", in_data.feature_version_);
    return kErrorParameters;
  }

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = sizeof(struct drm_msm_igc_lut);

  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_igc = new drm_msm_igc_lut();
  if (!mdp_igc) {
    DLOGE("Failed to allocate memory for igc");
    return kErrorMemory;
  }

  mdp_igc->flags = 0;
  if (sde_igc) {
    if (sde_igc->flags & IGC_DITHER_EN)
      mdp_igc->flags |= IGC_DITHER_ENABLE;
    mdp_igc->strength = sde_igc->strength;

    c0_c1_data_ptr = reinterpret_cast<uint32_t *>(sde_igc->c0_c1_data);
    c2_data_ptr = reinterpret_cast<uint32_t *>(sde_igc->c2_data);

    if (!c0_c1_data_ptr || !c2_data_ptr) {
      DLOGE("Invaid igc data pointer");
      delete mdp_igc;
      out_data->payload = NULL;
      return kErrorParameters;
    }

    int i;
    for (i = 0; i < IGC_TBL_LEN; i++) {
      mdp_igc->c0[i] = c0_c1_data_ptr[i] & kIgcDataMask;
      mdp_igc->c1[i] = (c0_c1_data_ptr[i] >> kIgcShift) & kIgcDataMask;
      mdp_igc->c2[i] = c2_data_ptr[i] & kIgcDataMask;
    }
    mdp_igc->c0_last = c0_c1_data_ptr[i] & kIgcDataMask;
    mdp_igc->c1_last = (c0_c1_data_ptr[i] >> kIgcShift) & kIgcDataMask;
    mdp_igc->c2_last = c2_data_ptr[i] & kIgcDataMask;

    out_data->payload = mdp_igc;
  } else if (sde_igc_v5) {
    if (sde_igc_v5->flags & IGC_DITHER_EN)
      mdp_igc->flags |= IGC_DITHER_ENABLE;
    mdp_igc->strength = sde_igc_v5->strength;

    c0_c1_data_ptr = reinterpret_cast<uint32_t *>(sde_igc_v5->c0_c1_data);
    c2_data_ptr = reinterpret_cast<uint32_t *>(sde_igc_v5->c2_data);

    if (!c0_c1_data_ptr || !c2_data_ptr) {
      DLOGE("Invaid igc data pointer");
      delete mdp_igc;
      out_data->payload = NULL;
      return kErrorParameters;
    }

    int i;
    for (i = 0; i < IGC_TBL_LEN; i++) {
      mdp_igc->c0[i] = c0_c1_data_ptr[i] & kRegU16Mask;
      mdp_igc->c1[i] = (c0_c1_data_ptr[i] >> kIgcShift) & kRegU16Mask;
      mdp_igc->c2[i] = c2_data_ptr[i] & kRegU16Mask;
    }
    mdp_igc->c0_last = c0_c1_data_ptr[i] & kRegU16Mask;
    mdp_igc->c1_last = (c0_c1_data_ptr[i] >> kIgcShift) & kRegU16Mask;
    mdp_igc->c2_last = c2_data_ptr[i] & kRegU16Mask;
 #ifdef IGC_HIGH_PREC_ENABLE
    if (sde_igc_v5->flags & SDM_IGC_HIGH_PREC_EN) {
      mdp_igc->flags |= IGC_HIGH_PREC_ENABLE;
      for (i = 0; i < IGC_TBL_LEN_EXTENDED; i++) {
        mdp_igc->c0_extended[i] = c0_c1_data_ptr[i + 1 + IGC_TBL_LEN] & kRegU16Mask;
        mdp_igc->c1_extended[i] =
            (c0_c1_data_ptr[i + 1 + IGC_TBL_LEN] >> kIgcShift) & kRegU16Mask;
        mdp_igc->c2_extended[i] = c2_data_ptr[i + 1 + IGC_TBL_LEN] & kRegU16Mask;
      }
    }
#endif

    out_data->payload = mdp_igc;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPGC(const PPFeatureInfo &in_data,
                                          DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  struct SDEPgcLUTData *sde_pgc;
  struct drm_msm_pgc_lut *mdp_pgc;

  sde_pgc = (struct SDEPgcLUTData *)in_data.GetConfigData();

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = sizeof(struct drm_msm_pgc_lut);

  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_pgc = new drm_msm_pgc_lut();
  if (!mdp_pgc) {
    DLOGE("Failed to allocate memory for pgc");
    return kErrorMemory;
  }

  mdp_pgc->flags = 0;
#ifdef PGC_HIGHPREC_EN
  if (sde_pgc->flags & SDM_PGC_HIGHPREC_MODE)
    mdp_pgc->flags |= PGC_HIGHPREC_EN;
#endif

  int i = 0, j = 0;
  for (; i < PGC_TBL_LEN; i++, j += 2) {
    mdp_pgc->c0[i] = (sde_pgc->c0_data[j] & kPgcDataMask) |
        (sde_pgc->c0_data[j + 1] & kPgcDataMask) << kPgcShift;
    mdp_pgc->c1[i] = (sde_pgc->c1_data[j] & kPgcDataMask) |
        (sde_pgc->c1_data[j + 1] & kPgcDataMask) << kPgcShift;
    mdp_pgc->c2[i] = (sde_pgc->c2_data[j] & kPgcDataMask) |
        (sde_pgc->c2_data[j + 1] & kPgcDataMask) << kPgcShift;
  }

#ifdef PGC_HIGHPREC_EN
  int len_ext = 0;
  if (mdp_pgc->flags & PGC_HIGHPREC_EN)
    len_ext = PGC_TBL_LEN_EXTENDED;
  for (i = 0; i < len_ext; i++, j += 2) {
    mdp_pgc->c0_extended[i] = (sde_pgc->c0_data[j] & kPgcDataMask) |
                              (sde_pgc->c0_data[j + 1] & kPgcDataMask)
                                  << kPgcShift;
    mdp_pgc->c1_extended[i] = (sde_pgc->c1_data[j] & kPgcDataMask) |
                              (sde_pgc->c1_data[j + 1] & kPgcDataMask)
                                  << kPgcShift;
    mdp_pgc->c2_extended[i] = (sde_pgc->c2_data[j] & kPgcDataMask) |
                              (sde_pgc->c2_data[j + 1] & kPgcDataMask)
                                  << kPgcShift;
  }
#endif

  out_data->payload = mdp_pgc;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmMixerGC(const PPFeatureInfo &in_data,
                                              DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  out_data->id = kPPFeaturesMax;
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAHsic(const PPFeatureInfo &in_data,
                                           DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_PA_HSIC)
  struct SDEPaData *sde_pa;
  struct drm_msm_pa_hsic *mdp_hsic;

  sde_pa = (struct SDEPaData *) in_data.GetConfigData();

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for pa hsic");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & (kPAHueMask | kPASatMask |
                        kPAValMask | kPAContrastMask))) {
    /* PA HSIC feature disable case, but other PA features active */
    return ret;
  }

  mdp_hsic = new drm_msm_pa_hsic();
  if (!mdp_hsic) {
    DLOGE("Failed to allocate memory for pa hsic");
    return kErrorMemory;
  }

  mdp_hsic->flags = 0;

  if (in_data.enable_flags_ & kPaHueEnable) {
    mdp_hsic->flags |= PA_HSIC_HUE_ENABLE;
    mdp_hsic->hue = sde_pa->hue_adj;
  }
  if (in_data.enable_flags_ & kPaSatEnable) {
    mdp_hsic->flags |= PA_HSIC_SAT_ENABLE;
    mdp_hsic->saturation = sde_pa->sat_adj;
  }
  if (in_data.enable_flags_ & kPaValEnable) {
    mdp_hsic->flags |= PA_HSIC_VAL_ENABLE;
    mdp_hsic->value = sde_pa->val_adj;
  }
  if (in_data.enable_flags_ & kPaContEnable) {
    mdp_hsic->flags |= PA_HSIC_CONT_ENABLE;
    mdp_hsic->contrast = sde_pa->cont_adj;
  }

  if (mdp_hsic->flags) {
    out_data->payload = mdp_hsic;
    out_data->payload_size = sizeof(struct drm_msm_pa_hsic);
  } else {
    /* PA HSIC configuration unchanged, no better return code available */
    delete mdp_hsic;
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPASixZone(const PPFeatureInfo &in_data,
                                                DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_SIXZONE)
  struct SDEPaData *sde_pa;

  sde_pa = (struct SDEPaData *) in_data.GetConfigData();

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for six zone");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & (kSixZoneHueMask | kSixZoneSatMask |
                        kSixZoneValMask))) {
    /* PA SixZone feature disable case, but other PA features active */
    return ret;
  }

  if (in_data.enable_flags_ & kPaSixZoneEnable) {
    struct drm_msm_sixzone *mdp_sixzone = NULL;

    if ((!sde_pa->six_zone_curve_p0 || !sde_pa->six_zone_curve_p1) ||
        (sde_pa->six_zone_len != SIXZONE_LUT_SIZE)) {
        DLOGE("Invaid sixzone curve");
        return kErrorParameters;
    }

    mdp_sixzone = new drm_msm_sixzone();
    if (!mdp_sixzone) {
      DLOGE("Failed to allocate memory for six zone");
      return kErrorMemory;
    }

    mdp_sixzone->flags = 0;

    if (sde_pa->mode & kSixZoneHueMask) {
      mdp_sixzone->flags |= SIXZONE_HUE_ENABLE;
    }
    if (sde_pa->mode & kSixZoneSatMask) {
      mdp_sixzone->flags |= SIXZONE_SAT_ENABLE;
    }
    if (sde_pa->mode & kSixZoneValMask) {
      mdp_sixzone->flags |= SIXZONE_VAL_ENABLE;
    }
#ifdef SIXZONE_SV_ENABLE
    if (sde_pa->six_zone_sv_enable) {
      mdp_sixzone->flags |= SIXZONE_SV_ENABLE;
    }
#endif

    mdp_sixzone->threshold = sde_pa->six_zone_thresh;
    mdp_sixzone->adjust_p0 = sde_pa->six_zone_adj_p0;
    mdp_sixzone->adjust_p1 = sde_pa->six_zone_adj_p1;
    mdp_sixzone->sat_hold = sde_pa->six_zone_sat_hold;
    mdp_sixzone->val_hold = sde_pa->six_zone_val_hold;
#ifdef SIXZONE_SV_ENABLE
    mdp_sixzone->sat_adjust_p0 = sde_pa->six_zone_sat_adj_p0;
    mdp_sixzone->sat_adjust_p1 = sde_pa->six_zone_sat_adj_p1;
#endif

    for (int i = 0; i < SIXZONE_LUT_SIZE; i++) {
      mdp_sixzone->curve[i].p0 = sde_pa->six_zone_curve_p0[i] & kSixZoneP0Mask;
      mdp_sixzone->curve[i].p1 = sde_pa->six_zone_curve_p1[i] & kSixZoneP1Mask;
#ifdef SIXZONE_SV_ENABLE
      mdp_sixzone->curve_p2[i] = sde_pa->six_zone_curve_p2[i] & kSixZoneP2Mask;
#endif
    }
    out_data->payload = mdp_sixzone;
    out_data->payload_size = sizeof(struct drm_msm_sixzone);
  } else {
    /* PA SixZone configuration unchanged, no better return code available */
    ret = kErrorPermission;
  }

#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAMemColSkin(const PPFeatureInfo &in_data,
                                                   DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
  struct SDEPaData *sde_pa;

  sde_pa = (struct SDEPaData *) in_data.GetConfigData();

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for memory color skin");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & kMemColorSkinMask)) {
    /* PA MemColSkin feature disable case, but other PA features active */
    return ret;
  }

  if (in_data.enable_flags_ & kPaSkinEnable) {
    struct drm_msm_memcol *mdp_memcol = NULL;
    struct SDEPaMemColorData *pa_memcol = &sde_pa->skin;

    mdp_memcol = new drm_msm_memcol();
    if (!mdp_memcol) {
      DLOGE("Failed to allocate memory for memory color skin");
      return kErrorMemory;
    }

    mdp_memcol->prot_flags = 0;
    mdp_memcol->color_adjust_p0 = pa_memcol->adjust_p0;
    mdp_memcol->color_adjust_p1 = pa_memcol->adjust_p1;
    mdp_memcol->color_adjust_p2 = pa_memcol->adjust_p2;
    mdp_memcol->blend_gain = pa_memcol->blend_gain;
    mdp_memcol->sat_hold = pa_memcol->sat_hold;
    mdp_memcol->val_hold = pa_memcol->val_hold;
    mdp_memcol->hue_region = pa_memcol->hue_region;
    mdp_memcol->sat_region = pa_memcol->sat_region;
    mdp_memcol->val_region = pa_memcol->val_region;

    out_data->payload = mdp_memcol;
    out_data->payload_size = sizeof(struct drm_msm_memcol);
  } else {
    /* PA MemColSkin configuration unchanged, no better return code available */
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAMemColSky(const PPFeatureInfo &in_data,
                                                  DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
  struct SDEPaData *sde_pa;

  sde_pa = (struct SDEPaData *) in_data.GetConfigData();

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for memory color sky");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & kMemColorSkyMask)) {
    /* PA MemColSky feature disable case, but other PA features active */
    return ret;
  }

  if (in_data.enable_flags_ & kPaSkyEnable) {
    struct drm_msm_memcol *mdp_memcol = NULL;
    struct SDEPaMemColorData *pa_memcol = &sde_pa->sky;

    mdp_memcol = new drm_msm_memcol();
    if (!mdp_memcol) {
      DLOGE("Failed to allocate memory for memory color sky");
      return kErrorMemory;
    }

    mdp_memcol->prot_flags = 0;
    mdp_memcol->color_adjust_p0 = pa_memcol->adjust_p0;
    mdp_memcol->color_adjust_p1 = pa_memcol->adjust_p1;
    mdp_memcol->color_adjust_p2 = pa_memcol->adjust_p2;
    mdp_memcol->blend_gain = pa_memcol->blend_gain;
    mdp_memcol->sat_hold = pa_memcol->sat_hold;
    mdp_memcol->val_hold = pa_memcol->val_hold;
    mdp_memcol->hue_region = pa_memcol->hue_region;
    mdp_memcol->sat_region = pa_memcol->sat_region;
    mdp_memcol->val_region = pa_memcol->val_region;

    out_data->payload = mdp_memcol;
    out_data->payload_size = sizeof(struct drm_msm_memcol);
  } else {
    /* PA MemColSky configuration unchanged, no better return code available */
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAMemColFoliage(const PPFeatureInfo &in_data,
                                                      DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
  struct SDEPaData *sde_pa;

  sde_pa = (struct SDEPaData *) in_data.GetConfigData();

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for memory color foliage");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & kMemColorFolMask)) {
    /* PA MemColFoliage feature disable case, but other PA features active */
    return ret;
  }

  if (in_data.enable_flags_ & kPaFoliageEnable) {
    struct drm_msm_memcol *mdp_memcol = NULL;
    struct SDEPaMemColorData *pa_memcol = &sde_pa->foliage;

    mdp_memcol = new drm_msm_memcol();
    if (!mdp_memcol) {
      DLOGE("Failed to allocate memory for memory color foliage");
      return kErrorMemory;
    }

    mdp_memcol->prot_flags = 0;
    mdp_memcol->color_adjust_p0 = pa_memcol->adjust_p0;
    mdp_memcol->color_adjust_p1 = pa_memcol->adjust_p1;
    mdp_memcol->color_adjust_p2 = pa_memcol->adjust_p2;
    mdp_memcol->blend_gain = pa_memcol->blend_gain;
    mdp_memcol->sat_hold = pa_memcol->sat_hold;
    mdp_memcol->val_hold = pa_memcol->val_hold;
    mdp_memcol->hue_region = pa_memcol->hue_region;
    mdp_memcol->sat_region = pa_memcol->sat_region;
    mdp_memcol->val_region = pa_memcol->val_region;

    out_data->payload = mdp_memcol;
    out_data->payload_size = sizeof(struct drm_msm_memcol);
  } else {
    /* PA MemColFoliage configuration unchanged, no better return code available */
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAMemColProt(const PPFeatureInfo &in_data,
                                                   DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
  struct SDEPaData *sde_pa;
  struct drm_msm_memcol *mdp_memcol;

  sde_pa = (struct SDEPaData *) in_data.GetConfigData();

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = sizeof(struct drm_msm_memcol);

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  out_data->payload = NULL;
  if (!(sde_pa->mode & kMemColorProtMask)) {
    /* PA MemColProt feature disable case, but other PA features active */
    return ret;
  }

  mdp_memcol = new drm_msm_memcol();
  if (!mdp_memcol) {
    DLOGE("Failed to allocate memory for memory color prot");
    return kErrorMemory;
  }

  mdp_memcol->prot_flags = 0;

  if (sde_pa->mode & kMemColorProtMask) {
    mdp_memcol->prot_flags |= (sde_pa->mode & kMemColorProtMask);
  }

  out_data->payload = mdp_memcol;

#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmDither(const PPFeatureInfo &in_data,
                                             DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  // Programming is the same for PP dither vs SPR dither
  struct SDEDitherCfg *sde_dither = NULL;
  struct drm_msm_dither *mdp_dither = NULL;

  sde_dither = (struct SDEDitherCfg *)in_data.GetConfigData();
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = sizeof(struct drm_msm_dither);

  if (in_data.enable_flags_ & kOpsDisable) {
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_dither = new drm_msm_dither();
  if (!mdp_dither) {
    DLOGE("Failed to allocate memory for dither");
    return kErrorMemory;
  }

  mdp_dither->flags = 0;
#ifdef DITHER_LUMA_MODE
  if (sde_dither->flags & SDM_DITHER_LUMA_MODE)
    mdp_dither->flags |= DITHER_LUMA_MODE;
#endif
  std::memcpy(mdp_dither->matrix, sde_dither->dither_matrix,
                sizeof(sde_dither->dither_matrix));
  mdp_dither->temporal_en = sde_dither->temporal_en;
  mdp_dither->c0_bitdepth = sde_dither->g_y_depth;
  mdp_dither->c1_bitdepth = sde_dither->b_cb_depth;
  mdp_dither->c2_bitdepth = sde_dither->r_cr_depth;
  mdp_dither->c3_bitdepth = 0;
  out_data->payload = mdp_dither;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmGamut(const PPFeatureInfo &in_data,
                                            DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  struct SDEGamutCfg *sde_gamut = NULL;
  struct drm_msm_3d_gamut *mdp_gamut = NULL;
  uint32_t size = 0;

  sde_gamut = (struct SDEGamutCfg *)in_data.GetConfigData();

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = sizeof(struct drm_msm_3d_gamut);
  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_gamut = new(std::nothrow) drm_msm_3d_gamut();
  if (!mdp_gamut) {
    DLOGE("Failed to allocate memory for gamut");
    return kErrorMemory;
  }

  if (sde_gamut->map_en)
    mdp_gamut->flags = GAMUT_3D_MAP_EN;
  else
    mdp_gamut->flags = 0;

  switch (sde_gamut->mode) {
    case SDEGamutCfgWrapper::GAMUT_FINE_MODE:
      mdp_gamut->mode = GAMUT_3D_MODE_17;
      size = GAMUT_3D_MODE17_TBL_SZ;
      break;
    case SDEGamutCfgWrapper::GAMUT_COARSE_MODE:
      mdp_gamut->mode = GAMUT_3D_MODE_5;
      size = GAMUT_3D_MODE5_TBL_SZ;
      break;
    case SDEGamutCfgWrapper::GAMUT_COARSE_MODE_13:
      mdp_gamut->mode = GAMUT_3D_MODE_13;
      size = GAMUT_3D_MODE13_TBL_SZ;
      break;
    default:
      DLOGE("Invalid gamut mode %d", sde_gamut->mode);
      delete mdp_gamut;
      return kErrorParameters;
  }

  if (sde_gamut->map_en) {
    std::memcpy(&mdp_gamut->scale_off[0][0], sde_gamut->scale_off_data[0],
                sizeof(uint32_t) * GAMUT_3D_SCALE_OFF_SZ);
    std::memcpy(&mdp_gamut->scale_off[1][0], sde_gamut->scale_off_data[1],
                sizeof(uint32_t) * GAMUT_3D_SCALE_OFF_SZ);
    std::memcpy(&mdp_gamut->scale_off[2][0], sde_gamut->scale_off_data[2],
                sizeof(uint32_t) * GAMUT_3D_SCALE_OFF_SZ);
  }

  for (uint32_t row = 0; row < GAMUT_3D_TBL_NUM; row++) {
    for (uint32_t col = 0; col < size; col++) {
      mdp_gamut->col[row][col].c0 = sde_gamut->c0_data[row][col];
      mdp_gamut->col[row][col].c2_c1 = sde_gamut->c1_c2_data[row][col];
    }
  }
  out_data->payload = mdp_gamut;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPADither(const PPFeatureInfo &in_data,
                                               DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_PA_DITHER)
  struct SDEPADitherData* sde_dither;
  struct drm_msm_pa_dither* mdp_dither;

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;

  out_data->payload_size = sizeof(struct drm_msm_pa_dither);
  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  sde_dither = (struct SDEPADitherData *)in_data.GetConfigData();
  if (sde_dither->matrix_size != DITHER_MATRIX_SZ) {
    DLOGE("Invalid dither matrix size %d, exp sz %d",
          sde_dither->matrix_size, DITHER_MATRIX_SZ);
    return kErrorParameters;
  }

  mdp_dither = new drm_msm_pa_dither();
  if (!mdp_dither) {
    DLOGE("Failed to allocate memory for dither");
    return kErrorMemory;
  }

  mdp_dither->flags = 0;
  mdp_dither->strength = sde_dither->strength;
  mdp_dither->offset_en = sde_dither->offset_en;
  std::memcpy(&mdp_dither->matrix[0],
              reinterpret_cast<void*>(sde_dither->matrix_data_addr),
              sizeof(uint32_t) * DITHER_MATRIX_SZ);
  out_data->payload = mdp_dither;
#endif
  return ret;
}

}  // namespace sdm
