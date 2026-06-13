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

#ifndef __DRM_ENCODER_H__
#define __DRM_ENCODER_H__

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <map>
#include <memory>
#include <vector>
#include <utility>
#include <set>

#include "drm_interface.h"
#include "drm_utils.h"

namespace sde_drm {

class DRMEncoder {
 public:
  explicit DRMEncoder(int fd) : fd_(fd) {}
  DRMEncoder(int fd, uint32_t id, uint32_t type) : fd_(fd), fake_id_(id), fake_type_(type) {}
  void InitAndParse(drmModeEncoder *encoder);
  DRMStatus GetStatus() { return status_; }
  void GetInfo(DRMEncoderInfo *info);
  void GetType(uint32_t *encoder_type) {
    *encoder_type = drm_encoder_ ? drm_encoder_->encoder_type : fake_type_;
  }
  void GetId(uint32_t *encoder_id) {
    *encoder_id = drm_encoder_ ? drm_encoder_->encoder_id : fake_id_;
  }
  void GetCrtc(uint32_t *crtc_id) {
    *crtc_id = drm_encoder_ ? drm_encoder_->crtc_id : 0;
  }
  int GetPossibleCrtcIndices(std::set<uint32_t> *possible_crtc_indices);
  void Dump();
  void Lock();
  void Unlock();
  ~DRMEncoder();

 private:
  int fd_ = -1;
  drmModeEncoder *drm_encoder_ = {};
  DRMStatus status_ = DRMStatus::FREE;
  DRMEncoderInfo encoder_info_ = {};

  // Userspace injected data, only used for creating object not reported by the driver
  uint32_t fake_id_;
  uint32_t fake_type_;
};

class DRMEncoderManager {
 public:
  explicit DRMEncoderManager(int fd) : fd_(fd) {}
  ~DRMEncoderManager();
  void Init(drmModeRes *res);
  void InsertSecondaryDSI();
  void DeInit() {}
  void DumpAll();
  void DumpByID(uint32_t id);
  int Reserve(const std::set<uint32_t> &possible_encoders, DRMDisplayToken *token);
  void Free(DRMDisplayToken *token);
  int GetEncoderInfo(uint32_t encoder_id, DRMEncoderInfo *info);
  int GetEncoderList(std::vector<uint32_t> *encoder_ids);
  int GetPossibleCrtcIndices(uint32_t encoder_id, std::set<uint32_t> *possible_crtc_indices);
  void MapCrtcToEncoder(std::map<uint32_t, uint32_t> *crtc_to_encoder);

 private:
  int fd_ = -1;
  std::map<uint32_t, std::unique_ptr<DRMEncoder>> encoder_pool_{};
  int GetDisplayTypeCode(uint32_t encoder_type);
};

}  // namespace sde_drm

#endif  // __DRM_ENCODER_H__
