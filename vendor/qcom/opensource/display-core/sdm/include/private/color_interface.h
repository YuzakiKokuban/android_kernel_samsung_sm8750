/* Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
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

#ifndef __COLOR_INTERFACE_H__
#define __COLOR_INTERFACE_H__

#include <string>
#include "core/sdm_types.h"
#include "color_params.h"
#include "spr_intf.h"

namespace sdm {

#define COLORMGR_LIBRARY_NAME "libsdm-color.so"
#define CREATE_COLOR_INTERFACE_NAME "CreateColorInterface"
#define DESTROY_COLOR_INTERFACE_NAME "DestroyColorInterface"
#define COLOR_REVISION_MAJOR (1)
#define COLOR_REVISION_MINOR (0)

#define COLOR_VERSION_TAG ((uint16_t)((COLOR_REVISION_MAJOR << 8) | COLOR_REVISION_MINOR))

#define STCMGR_LIBRARY_NAME "libsnapdragoncolor-manager.so"
#define CREATE_STC_INTERFACE_NAME "GetScPostBlendInterface"
#define STC_REVISION_MAJOR (2)
#define STC_REVISION_MINOR (0)

class ColorInterface;

typedef DisplayError (*CreateColorInterface)(uint16_t version, int32_t display_id,
                                             SDMDisplayType type,
                                             const PPHWAttributes &attributes,
                                             ColorInterface **interface);

typedef DisplayError (*DestroyColorInterface)(int32_t display_id);

typedef snapdragoncolor::ScPostBlendInterface *(*GetScPostBlendInterface)(uint32_t major_version,
                                                                          uint32_t minor_version);

class ColorModeInterface {
 public:
  virtual DisplayError ColorIntfGetActiveColorParam(uint32_t hint, uint32_t display_id,
                                                    void* data) = 0;
  virtual DisplayError ColorIntfSetActiveColorParam(uint32_t hint, uint32_t display_id,
                                                    void* data) = 0;
  virtual DisplayError ColorIntfSetHdrInterface(void *hdr_intf) = 0;

 protected:
  virtual ~ColorModeInterface() {}
};

extern "C" ColorModeInterface* GetColorModeInterface(int32_t display_id, SDMDisplayType type);
extern "C" void ReleaseColorModeInterface(int32_t display_id);

class ColorInterface {
 public:
  virtual DisplayError ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                            PPDisplayAPIPayload *out_payload,
                                            PPFeaturesConfig *out_features,
                                            PPPendingParams *pending_action) = 0;

  virtual DisplayError ApplyDefaultDisplayMode(PPFeaturesConfig *out_features) = 0;

  virtual DisplayError ColorIntfSetColorTransform(PPFeaturesConfig *out_features,
                                                uint32_t disp_id, uint32_t length,
                                                const double *trans_data) = 0;

  virtual DisplayError ColorIntfSetDisplayMode(PPFeaturesConfig *out_features,
                                             uint32_t disp_id, int32_t mode_id) = 0;

  virtual DisplayError ColorIntfGetNumDisplayModes(PPFeaturesConfig *out_features,
                                                 uint32_t disp_id, uint32_t *mode_cnt) = 0;

  virtual DisplayError ColorIntfEnumerateDisplayModes(PPFeaturesConfig *out_features,
                                                uint32_t disp_id, SDEDisplayMode *modes,
                                                uint32_t *mode_cnt) = 0;
  virtual DisplayError ColorIntfGetModeInfo(PPFeaturesConfig *out_features,
                                            uint32_t disp_id, int32_t mode_id,
                                            AttrVal *query) = 0;
  virtual DisplayError ColorIntfGetDefaultModeID(PPFeaturesConfig *out_features,
                                                 uint32_t disp_id, int32_t *mode_id) = 0;
  virtual DisplayError ColorIntfCombineColorModes() = 0;
  virtual DisplayError ColorIntfGameEnhancementSupported(bool *supported) = 0;
  virtual DisplayError ColorIntfConvertFeature(uint32_t disp_id,
                                               const snapdragoncolor::HwConfigPayload &in_data,
                                               PPFeaturesConfig *out_features) = 0;
  virtual DisplayError ColorIntfSetSprInterface(std::shared_ptr<SPRIntf> spr_intf) = 0;

 protected:
  virtual ~ColorInterface() {}
};

}  // namespace sdm

#endif  // __COLOR_INTERFACE_H__
