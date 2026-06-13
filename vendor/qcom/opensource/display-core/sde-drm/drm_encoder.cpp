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
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <stdint.h>
#include <stdlib.h>
#include <drm.h>
#include <display/drm/sde_drm.h>
#include <drm_logger.h>
#include <errno.h>
#include <stdlib.h>
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <iterator>

#include "drm_encoder.h"
#include "drm_utils.h"

namespace sde_drm {

using std::unique_ptr;
using std::map;
using std::pair;
using std::make_pair;
using std::set;
using std::distance;

#define __CLASS__ "DRMEncoderManager"

DRMEncoderManager::~DRMEncoderManager() {}

void DRMEncoderManager::Init(drmModeRes *resource) {
  for (int i = 0; i < resource->count_encoders; i++) {
    unique_ptr<DRMEncoder> encoder(new DRMEncoder(fd_));
    drmModeEncoder *libdrm_encoder = drmModeGetEncoder(fd_, resource->encoders[i]);
    if (!libdrm_encoder) {
      DRM_LOGE("Critical error: drmModeGetEncoder() failed for encoder %d.", resource->encoders[i]);
      continue;
    }
    encoder->InitAndParse(libdrm_encoder);
    encoder_pool_[resource->encoders[i]] = std::move(encoder);
  }

  // TODO(user): Remove call when driver reporting of encoders is consistent across all use cases
  InsertSecondaryDSI();
}

/*
 * This API is a workaround for maintaining appropriate HW port numbers for displays on platforms
 * that can dynamically change between single panel and secondary panel uses cases. It is required
 * that the port # for a pixel stream remains consistent, so userspace must account for the
 * possiblility of the secondary panel use case, even during single panel use case.

 * Driver rearchitecture for encoder creation is under discussion for future chipsets to avoid the
 * need of this function.
*/
void DRMEncoderManager::InsertSecondaryDSI() {
  uint32_t first_dsi_id = 0;
  bool second_dsi_found = false;
  bool first_dsi_found = false;

  for (auto encoder = encoder_pool_.begin(); encoder != encoder_pool_.end(); encoder++) {
    std::unique_ptr<DRMEncoder> &encoder_ptr = encoder->second;
    uint32_t encoder_type;
    encoder_ptr->GetType(&encoder_type);
    if (encoder_type == DRM_MODE_ENCODER_DSI) {
      if (!first_dsi_found) {
        encoder_ptr->GetId(&first_dsi_id);
        first_dsi_found = true;
      } else if (first_dsi_found) {
      // Secondary panel use case is active, below logic is skipped
      second_dsi_found = true;
      break;
      }
    }
  }

  if (first_dsi_found && !second_dsi_found) {
    // Single panel use case is active, inject new DSI encoder
    uint32_t enc_id = first_dsi_id + 1;
    unique_ptr<DRMEncoder> sec_dsi_enc(new DRMEncoder(fd_, enc_id, DRM_MODE_ENCODER_DSI));
    encoder_pool_[enc_id] = std::move(sec_dsi_enc);
    DRM_LOGI("Userspace has inserted secondary panel DSI encoder!");
  } else {
    DRM_LOGI("Userspace did not need to insert secondary panel DSI encoder, it is present.");
  }
}

void DRMEncoderManager::DumpByID(uint32_t id) {
  encoder_pool_.at(id)->Dump();
}

void DRMEncoderManager::DumpAll() {
  for (auto &encoder : encoder_pool_) {
    encoder.second->Dump();
  }
}

int DRMEncoderManager::GetEncoderInfo(uint32_t encoder_id, DRMEncoderInfo *info) {
  int ret = -ENODEV;
  auto iter = encoder_pool_.find(encoder_id);

  if (iter != encoder_pool_.end()) {
    encoder_pool_[encoder_id]->GetInfo(info);
    ret = 0;
  }
  return ret;
}

int DRMEncoderManager::GetEncoderList(std::vector<uint32_t> *encoder_ids) {
  if (!encoder_ids) {
    return -EINVAL;
  }
  encoder_ids->clear();
  for (auto &encoder : encoder_pool_) {
    encoder_ids->push_back(encoder.first);
  }
  return 0;
}

int DRMEncoderManager::Reserve(const std::set<uint32_t> &possible_encoders, DRMDisplayToken *token) {
  int ret = -ENODEV;
  for (auto encoder = encoder_pool_.begin(); encoder != encoder_pool_.end(); encoder++) {
    const uint32_t &encoder_id = encoder->first;
    if ((encoder->second)->GetStatus() == DRMStatus::FREE) {
      // If encoder is found in the set, the encoder is a candidate for selection and the encoder
      // type for display's connector and encoder-this-iteration are already matched implicitly
      if (possible_encoders.find(encoder_id) != possible_encoders.end()) {
        encoder->second->Lock();
        token->encoder_id = encoder_id;
        int encoder_index = distance(encoder_pool_.begin(), encoder) + 1;  // 1-indexed port
        // Port id format.
        // Bit 7   --> Display type 0: Pluggable 1: BuiltIn X:Virtual.
        // Bit 6   --> Pluggable: 0 for TMDS encoder, 1 for DPMST encoder.
        //             Builtin Or Virtual: X
        // Bit 5-4 --> Core id
        // Bit 3-0 --> Encoder index.
        uint32_t encoder_type;
        encoder->second->GetType(&encoder_type);
        token->hw_port = GetDisplayTypeCode(encoder_type) | encoder_index;
        // read card name to extract the core_id from it
        char *name = drmGetPrimaryDeviceNameFromFd(fd_);
        if (name == NULL) {
          return ret;
        }

        std::string device_name(name);
        // append core_id to the hw_port to differentiate individual displays in dual dpu usecase
        uint8_t core_id_mask = static_cast<uint8_t> (std::atoi(&device_name.back()) == 0 ?
                                                     (1 << 4) : (1 << 5));
        token->hw_port = token->hw_port | core_id_mask;
        ret = 0;
        break;
      }
    }
  }
  return ret;
}

int DRMEncoderManager::GetDisplayTypeCode(uint32_t encoder_type) {
  int disp_info = 0x2;
  switch (encoder_type) {
    case DRM_MODE_ENCODER_TMDS:
      disp_info = 0x0;
      break;
    case DRM_MODE_ENCODER_DPMST:
      disp_info = 0x1;
      break;
    default:
      break;
  }

  return (disp_info << 6);
}

void DRMEncoderManager::Free(DRMDisplayToken *token) {
  auto iter = encoder_pool_.find(token->encoder_id);
  if (iter != encoder_pool_.end()) {
    iter->second->Unlock();
  } else {
    DRM_LOGW("Failed! encoder_id %u not found! Cleaning up token anyway...", token->encoder_id);
  }
  token->encoder_id = 0;
  token->hw_port = 0;
}

int DRMEncoderManager::GetPossibleCrtcIndices(uint32_t encoder_id,
                                  std::set<uint32_t> *possible_crtc_indices) {
  return encoder_pool_[encoder_id]->GetPossibleCrtcIndices(possible_crtc_indices);
}

void DRMEncoderManager::MapCrtcToEncoder(std::map<uint32_t, uint32_t> *crtc_to_encoder) {
  if (!crtc_to_encoder) {
    DLOGE("Map is NULL! Not expected.");
    return;
  }

  crtc_to_encoder->clear();

  for (auto &encoder : encoder_pool_) {
    uint32_t crtc_id = 0;
    encoder.second->GetCrtc(&crtc_id);
    if (crtc_id)
      crtc_to_encoder->insert(make_pair(crtc_id, encoder.first));
  }
}

// ==============================================================================================//

#undef __CLASS__
#define __CLASS__ "DRMEncoder"

DRMEncoder::~DRMEncoder() {
  if (drm_encoder_) {
    drmModeFreeEncoder(drm_encoder_);
  }
}

void DRMEncoder::GetInfo(DRMEncoderInfo *info) {
  *info = encoder_info_;
}

void DRMEncoder::Lock() {
  status_ = DRMStatus::BUSY;
}

void DRMEncoder::Unlock() {
  status_ = DRMStatus::FREE;
}

void DRMEncoder::InitAndParse(drmModeEncoder *encoder) {
  drm_encoder_ = encoder;
  encoder_info_.type = drm_encoder_->encoder_type;
}

int DRMEncoder::GetPossibleCrtcIndices(std::set<uint32_t> *possible_crtc_indices) {
  if (!possible_crtc_indices) {
    return -EINVAL;
  }

  (*possible_crtc_indices).clear();
  std::bitset<32> possible_crtcs = drm_encoder_->possible_crtcs;
  for (uint32_t i = 0; i < possible_crtcs.size(); i++) {
    if (possible_crtcs[i]) {
      (*possible_crtc_indices).insert(i);
    }
  }

  return 0;
}

void DRMEncoder::Dump() {
  if (drm_encoder_) {
    DRM_LOGI("[driver-reported] id: %u encoder_type: %u crtc id: %u possible_crtcs: %u"
             "possible_clones: %u fd = %d",
             drm_encoder_->encoder_id, drm_encoder_->encoder_type, drm_encoder_->crtc_id,
             drm_encoder_->possible_crtcs, drm_encoder_->possible_clones, fd_);
  } else {
    DRM_LOGI("[userspace-only] id: %u encoder_type: %u fd = %d ", fake_id_, fake_type_, fd_);
  }
}

}  // namespace sde_drm
