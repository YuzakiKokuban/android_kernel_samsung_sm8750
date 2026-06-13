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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cstring>
#include <errno.h>
#include <drm_logger.h>
#include <utils/debug.h>
#include <display_properties.h>

#include "libdrm_macros.h"
#include "drm/drm_fourcc.h"
#include "drm_dpps_mgr_imp.h"

#define __CLASS__ "DRMDppsManagerImp"
namespace sde_drm {

static DRMDppsManagerImp dpps_mgr;
static DRMDppsManagerDummyImp dpps_dummy_mgr;

DRMDppsManagerIntf* GetDppsManagerIntf()
{
#if (defined(__ANDROID__))
    int disable_power_features = 0;
    sdm::Debug::Get()->GetProperty(DISABLE_DPPS_FEATURES, \
                          &disable_power_features);

    if (disable_power_features) {
      return &dpps_dummy_mgr;
    }

    return &dpps_mgr;
#else
    return &dpps_dummy_mgr;
#endif
}

DRMDppsManagerImp::~DRMDppsManagerImp() {
  /* clean up the ION buffers for LTM */
  DeInitLtmBuffers();

  conn_id_ = -1;
  crtc_id_ = -1;
  drm_fd_ = -1;
}

int DRMDppsManagerImp::GetDrmResources(drmModeRes* res) {
  int enc_id = -1;
  drmModeConnector *conn = NULL;
  drmModeEncoder *enc = NULL;
  drmModeCrtc *crtc = NULL;

  if (drm_fd_ < 0 ) {
    DRM_LOGE("Invalid drm_fd_ %d", drm_fd_);
    return -EINVAL;
  }

  for (auto i = 0; i < res->count_connectors; i++) {
    conn = drmModeGetConnector(drm_fd_, res->connectors[i]);
    if (conn && conn->connector_type == DRM_MODE_CONNECTOR_DSI &&
        conn->count_modes && conn->connection == DRM_MODE_CONNECTED) {
      DRM_LOGI("Found connector %d", conn->connector_id);
      conn_id_ = conn->connector_id;
      break;
    }
    drmModeFreeConnector(conn);
    conn = NULL;
  }
  if (conn_id_ < 0 || !conn) {
    DRM_LOGE("Cannot find valid connector");
    conn_id_ = -1;
    return -EINVAL;
  }

  for (auto i = 0; i < conn->count_encoders; i++) {
    enc = drmModeGetEncoder(drm_fd_, conn->encoders[i]);
    if (enc && enc->encoder_type == DRM_MODE_ENCODER_DSI) {
      DRM_LOGI("Found encoder %d", enc->encoder_id);
      enc_id = enc->encoder_id;
      break;
    }
    drmModeFreeEncoder(enc);
    enc = NULL;
  }
  if (enc_id < 0 || !enc) {
    DRM_LOGE("Cannot find valid encoder");
    drmModeFreeConnector(conn);
    conn = NULL;
    res = NULL;
    conn_id_ = -1;
    return -EINVAL;
  }

  for (auto i = 0; i < res->count_crtcs; i++) {
    if (enc->possible_crtcs & (1 << i)) {
      crtc = drmModeGetCrtc(drm_fd_, res->crtcs[i]);
      if (crtc) {
        DRM_LOGI("Found crtc %d", crtc->crtc_id);
        crtc_id_ = crtc->crtc_id;
        break;
      }
      drmModeFreeCrtc(crtc);
      crtc = NULL;
    }
  }
  if (crtc_id_ < 0 || !crtc) {
    DRM_LOGE("Cannot find valid crtc");
    drmModeFreeEncoder(enc);
    drmModeFreeConnector(conn);
    enc = NULL;
    conn = NULL;
    conn_id_ = -1;
    crtc_id_ = -1;
    return -EINVAL;
  }
  return 0;
}

int DRMDppsManagerImp::InitCrtcProps() {
  if (drm_fd_ < 0 || crtc_id_ < 0) {
    DRM_LOGE("Invalid drm_fd_ %d or crtc_id_ %d", drm_fd_, crtc_id_);
    return -EINVAL;
  }

  drmModeObjectProperties *props =
    drmModeObjectGetProperties(drm_fd_, crtc_id_, DRM_MODE_OBJECT_CRTC);
  if (!props || !props->props || !props->prop_values) {
    drmModeFreeObjectProperties(props);
    return -EINVAL;
  }

  for (uint32_t j = 0; j < props->count_props; j++) {
    drmModePropertyRes *info = drmModeGetProperty(drm_fd_, props->props[j]);
    if (!info) {
      continue;
    }

    std::string property_name(info->name);
    DRMProperty prop_enum = prop_mgr_.GetPropertyEnum(property_name);
    if (prop_enum == DRMProperty::INVALID) {
      DRM_LOGD("DRMProperty %s missing from global property mapping", info->name);
      drmModeFreeProperty(info);
      continue;
    }

    prop_mgr_.SetPropertyId(prop_enum, info->prop_id);
    drmModeFreeProperty(info);
  }

  drmModeFreeObjectProperties(props);
  return 0;
}

int DRMDppsManagerImp::InitConnProps()
{
  if (drm_fd_ < 0 || conn_id_ < 0) {
    DRM_LOGE("Invalid drm_fd_ %d or conn_id_ %d", drm_fd_, conn_id_);
    return -EINVAL;
  }

  drmModeObjectProperties *props =
      drmModeObjectGetProperties(drm_fd_, conn_id_, DRM_MODE_OBJECT_CONNECTOR);
  if (!props || !props->props || !props->prop_values) {
    drmModeFreeObjectProperties(props);
    return -EINVAL;
  }

  for (uint32_t j = 0; j < props->count_props; j++) {
    drmModePropertyRes *info = drmModeGetProperty(drm_fd_, props->props[j]);
    if (!info) {
      continue;
    }

    std::string property_name(info->name);
    DRMProperty prop_enum = prop_mgr_.GetPropertyEnum(property_name);
    if (prop_enum == DRMProperty::INVALID) {
      DRM_LOGD("DRMProperty %s missing from global property mapping", info->name);
      drmModeFreeProperty(info);
      continue;
    }

    prop_mgr_.SetPropertyId(prop_enum, info->prop_id);
    drmModeFreeProperty(info);
  }

  drmModeFreeObjectProperties(props);
  return 0;
}

void DRMDppsManagerImp::Init(int fd, drmModeRes* res) {
  std::lock_guard<std::mutex> guard(api_lock_);
  int ret = 0;

  if (fd < 0 || !res) {
    DRM_LOGE("Invalid drm fd %d or res %pK", fd, res);
    return;
  }

  drm_fd_ = fd;
  ret = GetDrmResources(res);
  if (ret) {
    DRM_LOGE("Failed to get DRM resources %d", ret);
    return;
  } else {
    ret = InitCrtcProps();
    if (ret) {
      DRM_LOGE("Failed to initialize crtc properties %d", ret);
      return;
    }
    ret = InitConnProps();
    if (ret) {
      DRM_LOGE("Failed to initialize conn properties %d", ret);
      return;
    }
  }

  memset(&dpps_feature_, 0, sizeof(dpps_feature_));
  dpps_feature_[kFeatureAd4Mode] = DRMDppsPropInfo {
    (prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_MODE) == 0 ? 0U : 4U) /* version */,
    DRMProperty::SDE_DSPP_AD4_MODE, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_MODE),
    false /* is_event */};
  dpps_feature_[kFeatureAd4Init] = DRMDppsPropInfo {4 /* version */,
    DRMProperty::SDE_DSPP_AD4_INIT, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_INIT),
    false /* is_event */};
  dpps_feature_[kFeatureAd4Cfg] = DRMDppsPropInfo { 4 /* version */,
    DRMProperty::SDE_DSPP_AD4_CFG, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_CFG),
    false /* is_event */};
  dpps_feature_[kFeatureAd4Input] = DRMDppsPropInfo {4 /* version */,
    DRMProperty::SDE_DSPP_AD4_INPUT, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_INPUT),
    false /* is_event */};
  dpps_feature_[kFeatureAd4Backlight] = DRMDppsPropInfo {4 /* version */,
    DRMProperty::SDE_DSPP_AD4_BACKLIGHT, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_BACKLIGHT),
    false /* is_event */};
  dpps_feature_[kFeatureAd4Assertiveness] = DRMDppsPropInfo {4 /* version */,
    DRMProperty::SDE_DSPP_AD4_ASSERTIVENESS, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_ASSERTIVENESS),
    false /* is_event */};
  dpps_feature_[kFeatureAd4Roi] = DRMDppsPropInfo {4 /* version */,
    DRMProperty::SDE_DSPP_AD4_ROI, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_ROI),
    false /* is_event */};
  dpps_feature_[kFeatureAd4ManualStrength] = DRMDppsPropInfo {4 /* version */,
    DRMProperty::SDE_DSPP_AD4_STRENGTH, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_AD4_STRENGTH),
    false /* is_event */};
  dpps_feature_[kFeatureAbaHistCtrl] = DRMDppsPropInfo {1 /* version */,
    DRMProperty::SDE_DSPP_ABA_HIST_CTRL, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_ABA_HIST_CTRL),
    false /* is_event */};
  dpps_feature_[kFeatureAbaHistIRQ] = DRMDppsPropInfo {1 /* version */,
    DRMProperty::SDE_DSPP_ABA_HIST_IRQ, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_ABA_HIST_IRQ),
    false /* is_event */};
  dpps_feature_[kFeatureAbaLut] = DRMDppsPropInfo {1 /* version */,
    DRMProperty::SDE_DSPP_ABA_LUT, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_ABA_LUT),
    false /* is_event */};
  dpps_feature_[kFeatureSvBlScale] = DRMDppsPropInfo {1 /* version */,
    DRMProperty::SDE_DSPP_SV_BL_SCALE, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_SV_BL_SCALE),
    false /* is_event */};
  dpps_feature_[kFeatureBacklightScale] = DRMDppsPropInfo {1 /* version */,
    DRMProperty::SDE_DSPP_BL_SCALE, prop_mgr_.GetPropertyId(DRMProperty::SDE_DSPP_BL_SCALE),
    false /* is_event */};

  uint32_t i = (uint32_t)DRMProperty::INVALID;
  if (prop_mgr_.IsPropertyAvailable(DRMProperty::SDE_LTM_VERSION)) {
    i = static_cast<uint32_t>(DRMProperty::SDE_LTM_VERSION);
  } else if (prop_mgr_.IsPropertyAvailable(DRMProperty::SDE_LTM_VERSION_V2)) {
    i = static_cast<uint32_t>(DRMProperty::SDE_LTM_VERSION_V2);
  }

  if (i != (uint32_t)DRMProperty::INVALID) {
    dpps_feature_[kFeatureLtm].prop_enum = (DRMProperty)i;
    dpps_feature_[kFeatureLtm].prop_id = prop_mgr_.GetPropertyId((DRMProperty)i);
    dpps_feature_[kFeatureLtm].version = i - (uint32_t)DRMProperty::SDE_LTM_VERSION + 1;

    dpps_feature_[kFeatureLtmInit] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_INIT, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_INIT),
      false /* is_event */};
    dpps_feature_[kFeatureLtmCfg] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_CFG, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_CFG),
      false /* is_event */};
    dpps_feature_[kFeatureLtmNoiseThresh] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_NOISE_THRESH, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_NOISE_THRESH),
      false /* is_event */};
    dpps_feature_[kFeatureLtmBufferCtrl] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_BUFFER_CTRL, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_BUFFER_CTRL),
      false /* is_event */};
    dpps_feature_[kFeatureLtmQueueBuffer] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_QUEUE_BUFFER, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_QUEUE_BUFFER),
      false /* is_event */};
    dpps_feature_[kFeatureLtmQueueBuffer2] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_QUEUE_BUFFER2, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_QUEUE_BUFFER2),
      false /* is_event */};
    dpps_feature_[kFeatureLtmQueueBuffer3] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_QUEUE_BUFFER3, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_QUEUE_BUFFER3),
      false /* is_event */};
    dpps_feature_[kFeatureLtmHistCtrl] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_HIST_CTRL, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_HIST_CTRL),
      false /* is_event */};
    dpps_feature_[kFeatureLtmVlut] = DRMDppsPropInfo {1 /* version */,
      DRMProperty::SDE_LTM_VLUT, prop_mgr_.GetPropertyId(DRMProperty::SDE_LTM_VLUT),
      false /* is_event */};
  } else {
    DRM_LOGI("LTM properties are not available");
  }

  dpps_feature_[kFeaturePowerEvent] = DRMDppsPropInfo{1, DRMProperty::INVALID, 0, true /* is_event */};
  dpps_feature_[kFeatureAbaHistEvent] = DRMDppsPropInfo{1, DRMProperty::INVALID, 0, true /* is_event */};
  dpps_feature_[kFeatureBackLightEvent] = DRMDppsPropInfo{1, DRMProperty::INVALID, 0, true /* is_event */};
  dpps_feature_[kFeatureAdAttBlEvent] = DRMDppsPropInfo{1, DRMProperty::INVALID, 0, true /* is_event */};
  dpps_feature_[kFeatureLtmHistEvent] = DRMDppsPropInfo{1, DRMProperty::INVALID, 0, true /* is_event */};
  dpps_feature_[kFeatureLtmWbPbEvent] = DRMDppsPropInfo{1, DRMProperty::INVALID, 0, true /* is_event */};
  dpps_feature_[kFeatureLtmOffEvent] = DRMDppsPropInfo{1, DRMProperty::INVALID, 0, true /* is_event */};

  ltm_buffers_ctrl_map_.reserve(MAX_DISPLAY_COUNT);
  ltm_buffers_map_.reserve(MAX_DISPLAY_COUNT);
}

void DRMDppsManagerImp::CacheDppsFeature(uint32_t obj_id, va_list args) {
  std::lock_guard<std::mutex> guard(api_lock_);
  uint32_t feature_id = va_arg(args, uint32_t);
  uint64_t value = va_arg(args, uint64_t);
  struct DRMDppsPropInfo* info;

  if (feature_id >= kDppsFeaturesMax) {
    DRM_LOGE("Invalid feature id %d for obj_id 0x%x", feature_id, obj_id);
    return;
  }

  info = &dpps_feature_[feature_id];
  info->obj_id = obj_id;
  info->value = value;
  if (info->is_event) {
    dpps_dirty_event_.push_back(*info);
  } else {
    for (auto &it : dpps_dirty_prop_) {
      if ((it.obj_id == info->obj_id) && (it.prop_id == info->prop_id)) {
        it.value = info->value;
        return;
      }
    }
    dpps_dirty_prop_.push_back(*info);
  }
}

void DRMDppsManagerImp::CommitDppsFeatures(drmModeAtomicReq *req, const DRMDisplayToken &tok,
    uint32_t validate_only) {
  std::lock_guard<std::mutex> guard(api_lock_);
  int ret = 0;

  if (!req)
    return;

  // Set Dpps properties
  if (!dpps_dirty_prop_.empty()) {
    for (auto it = dpps_dirty_prop_.begin(); it != dpps_dirty_prop_.end();) {
      if (it->obj_id == tok.crtc_id || it->obj_id == tok.conn_id) {
        ret = drmModeAtomicAddProperty(req, it->obj_id, it->prop_id, it->value);
        if (ret < 0)
          DRM_LOGE("AtomicAddProperty failed obj_id 0x%x, prop_id %d ret %d ", it->obj_id,
                   it->prop_id, ret);
        else if (validate_only)
          it++;
        else
          it = dpps_dirty_prop_.erase(it);
      } else {
        it++;
      }
    }
  }

  // Set Dpps events
  if (!dpps_dirty_event_.empty()) {
    for (auto it = dpps_dirty_event_.begin(); it != dpps_dirty_event_.end();) {
      if (!it->value)
        continue;

      struct DRMDppsEventInfo info = *(struct DRMDppsEventInfo*)it->value;
      struct drm_msm_event_req event_req = {};
      int ret;
      if (it->obj_id == tok.crtc_id || it->obj_id == tok.conn_id) {
        event_req.object_id = it->obj_id;
        event_req.object_type = info.object_type;
        event_req.event = info.event_type;
        if (info.enable)
          ret = drmIoctl(info.drm_fd, DRM_IOCTL_MSM_REGISTER_EVENT, &event_req);
        else
          ret = drmIoctl(info.drm_fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &event_req);
        if (ret) {
          ret = -errno;
          if (ret == -EALREADY) {
            DRM_LOGI("Duplicated request to set event 0x%x, object_id %u, object_type 0x%x, enable %d",
                      event_req.event, event_req.object_id, info.object_type, info.enable);
          } else if (ret == -ENOENT || ret == -ENODEV || ret == -EACCES) {
            DRM_LOGW("Event 0x%x, object_id %u, object_type 0x%x, enable %d, ret %d",
                      event_req.event, event_req.object_id, info.object_type, info.enable, ret);
          } else {
            DRM_LOGE("Failed to set event 0x%x, object_id %u, object_type 0x%x, enable %d, ret %d",
                      event_req.event, event_req.object_id, info.object_type, info.enable, ret);
          }
        }
        if (ret == -ENODEV || ret == -ENOENT || ret == -EACCES)
          it++;
        else
          it = dpps_dirty_event_.erase(it);
      } else {
        it++;
      }
    }
  }
}

void DRMDppsManagerImp::GetDppsFeatureInfo(DRMDppsFeatureInfo *info)
{
  std::lock_guard<std::mutex> guard(api_lock_);
  int ret = 0;
  struct DRMDppsPropInfo* prop_info;

  if (!info) {
    DRM_LOGE("Invalid info NULL");
    return;
  }

  DRMDPPSFeatureID id = info->id;
  if (id >= kDppsFeaturesMax) {
    DRM_LOGE("Invalid feature id %d", id);
    return;
  }
  info->version = dpps_feature_[id].version;

  if ((id == kFeatureLtmBufferCtrl) && (dpps_feature_[kFeatureLtm].prop_id != 0)) {
    /* setup ION buffers for LTM */
    ret = InitLtmBuffers(info);
    if (ret) {
      DRM_LOGE("Failed to init LTM buffers %d", ret);
      return;
    } else {
      prop_info = &dpps_feature_[kFeatureLtmBufferCtrl];
      prop_info->obj_id = info->obj_id;
      for (const auto& it : ltm_buffers_ctrl_map_) {
        if (it.first == info->obj_id)
          prop_info->value = (uint64_t)&(it.second);
      }
      dpps_dirty_prop_.push_back(*prop_info);
    }
  }
}

int DRMDppsManagerImp::InitLtmBuffers(struct DRMDppsFeatureInfo *info) {
  int ret = 0;
  uint32_t buffer_size, i = 0, bpp = 0;
  void* uva;
  struct DRMDppsLtmBuffers *buffers;
  DRMDppsLtmBuffers ltm_buffers = {};
  drm_msm_ltm_buffers_ctrl ltm_buffers_ctrl = {};
  struct drm_prime_handle prime_req;
  struct drm_mode_fb_cmd2 fb_obj;
  struct drm_gem_close gem_close;

  if (drm_fd_ < 0) {
    DRM_LOGE("Invalid drm_fd_ %d", drm_fd_);
    return -EINVAL;
  }

  for (const auto& it : ltm_buffers_map_) {
    if (it.first == info->obj_id) {
      DRM_LOGE("LTM buffer already initialized, obj id %d", info->obj_id);
      return -EALREADY;
    }
  }

  if (!info->payload || info->payload_size != sizeof(struct DRMDppsLtmBuffers)) {
    DRM_LOGE("Invalid payload %pK size %d expected %zu", info->payload, info->payload_size,
       sizeof(struct DRMDppsLtmBuffers));
    return -EINVAL;
  }

  buffers = (struct DRMDppsLtmBuffers *)info->payload;
  if (buffers->num_of_buffers <= 0 || buffers->num_of_buffers > LTM_BUFFER_SIZE) {
    DRM_LOGE("Invalid number of buffers %d", buffers->num_of_buffers);
    return -EINVAL;
  }

  std::memset(&ltm_buffers, 0, sizeof(ltm_buffers));
  std::memset(&ltm_buffers_ctrl, 0, sizeof(ltm_buffers_ctrl));

  ltm_buffers.num_of_buffers = buffers->num_of_buffers;

  buffer_size = sizeof(struct drm_msm_ltm_stats_data) + LTM_GUARD_BYTES;
  std::memset(&fb_obj, 0, sizeof(drm_mode_fb_cmd2));
  fb_obj.pixel_format = DRM_FORMAT_YVYU;
  /* YVYU gives us a bpp of 16 (2 bytes) so we must take that into account */
  fb_obj.height = 2;
  /* add extra one to compensate integer rounding */
  fb_obj.width = buffer_size / (2 * fb_obj.height) + 1;
  /* bpp for YVYU is 16 */
  bpp = 16;
  fb_obj.flags = DRM_MODE_FB_MODIFIERS;
  fb_obj.pitches[0] = fb_obj.width * bpp / 8;

  for (i = 0; i < buffers->num_of_buffers && !ret; i++) {
    std::memset(&prime_req, 0, sizeof(drm_prime_handle));
    prime_req.fd = buffers->ion_buffer_fd[i];
    ret = drmIoctl(drm_fd_, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime_req);
    if (ret) {
      ret = -errno;
      DRM_LOGE("failed get prime handle: %d", ret);
      break;
    }
    ltm_buffers.ion_buffer_fd[i] = buffers->ion_buffer_fd[i];

    fb_obj.handles[0] = prime_req.handle;
    ret = drmIoctl(drm_fd_, DRM_IOCTL_MODE_ADDFB2, &fb_obj);
    if (ret) {
      ret = -errno;
      DRM_LOGE("return value from addFB2: %d", ret);
      break;
    }
    ltm_buffers.drm_fb_id[i] = buffers->drm_fb_id[i] = fb_obj.fb_id;
    ltm_buffers_ctrl.fds[i] = ltm_buffers.drm_fb_id[i];

    /**
     * ADDFB2 will take reference to GEM handles,
     * so drop reference taken by PrimeFDtoHandle now
     */
    std::memset(&gem_close, 0, sizeof(gem_close));
    gem_close.handle = prime_req.handle;
    ret = drmIoctl(drm_fd_, DRM_IOCTL_GEM_CLOSE, &gem_close);
    if(ret) {
      ret = -errno;
      DRM_LOGE("return value from GEM_CLOSE: %d\n", ret);
      break;
    }

    uva = drm_mmap(0, buffers->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                   buffers->ion_buffer_fd[i], 0);
    if (uva == MAP_FAILED) {
      ret = -errno;
      DRM_LOGE("failed to get uva: %d", ret);
      break;
    }
    ltm_buffers.uva[i] = buffers->uva[i] = uva;
  }

  if (!ret) {
    buffers->status = 0;
    ltm_buffers.buffer_size = buffers->buffer_size;
    ltm_buffers_ctrl.num_of_buffers = ltm_buffers.num_of_buffers;
    DRM_LOGV("InitLtmBuffers return successful");
  } else {
    DeInitLtmBuffers();
    buffers->status = ret;
    return ret;
  }

  ltm_buffers_map_.push_back(std::make_pair(info->obj_id, std::move(ltm_buffers)));
  ltm_buffers_ctrl_map_.push_back(std::make_pair(info->obj_id, std::move(ltm_buffers_ctrl)));
  return ret;
}

int DRMDppsManagerImp::DeInitLtmBuffers() {
  int ret = 0;
  uint32_t i = 0;

  if (drm_fd_ < 0) {
    return -EINVAL;
  }

  for (auto& it : ltm_buffers_map_) {
    DRMDppsLtmBuffers& ltm_buffers = it.second;
    for (i = 0; i < ltm_buffers.num_of_buffers; i++) {
      if (ltm_buffers.uva[i]) {
        drm_munmap(ltm_buffers.uva[i], ltm_buffers.buffer_size);
        ltm_buffers.uva[i] = NULL;
      }

      if (ltm_buffers.drm_fb_id[i] >= 0) {
#ifdef DRM_IOCTL_MSM_RMFB2
        ret = drmIoctl(drm_fd_, DRM_IOCTL_MSM_RMFB2, &ltm_buffers.drm_fb_id[i]);
        if (ret) {
          ret = errno;
          DRM_LOGE("RMFB2 failed for fb_id %d with error %d", ltm_buffers.drm_fb_id[i], ret);
        }
#endif
        ltm_buffers.drm_fb_id[i] = -1;
      }
      ltm_buffers.ion_buffer_fd[i] = -1;
    }

    ltm_buffers.num_of_buffers = 0;
    ltm_buffers.buffer_size = 0;
  }

  for (auto& it : ltm_buffers_ctrl_map_) {
    drm_msm_ltm_buffers_ctrl& ltm_buffers_ctrl = it.second;
    std::memset(&ltm_buffers_ctrl, 0, sizeof(ltm_buffers_ctrl));
  }

  ltm_buffers_map_.clear();
  ltm_buffers_ctrl_map_.clear();
  return 0;
}

}  // namespace sde_drm
