/*
* Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define __STDC_FORMAT_MACROS

#include <ctype.h>
#include <time.h>
#include <drm/drm_fourcc.h>
#include <drm_lib_loader.h>
#include <drm_res_mgr.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/fb.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/formats.h>
#include <utils/sys.h>
#include <display/drm/sde_drm.h>
#include <private/color_params.h>
#include <utils/rect.h>
#include <utils/utils.h>
#include <utils/fence.h>
#include <private/hw_info_interface.h>
#include <dirent.h>

#include <sstream>
#include <ctime>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <limits>
#include <sync/sync.h>

#include "hw_device_drm.h"

#define __CLASS__ "HWDeviceDRM"

#ifndef DRM_FORMAT_MOD_QCOM_COMPRESSED
#define DRM_FORMAT_MOD_QCOM_COMPRESSED fourcc_mod_code(QCOM, 1)
#endif
#ifndef DRM_FORMAT_MOD_QCOM_DX
#define DRM_FORMAT_MOD_QCOM_DX fourcc_mod_code(QCOM, 0x2)
#endif
#ifndef DRM_FORMAT_MOD_QCOM_TIGHT
#define DRM_FORMAT_MOD_QCOM_TIGHT fourcc_mod_code(QCOM, 0x4)
#endif
#ifndef DRM_FORMAT_MOD_QCOM_LOSSY_8_5
#define DRM_FORMAT_MOD_QCOM_LOSSY_8_5 fourcc_mod_code(QCOM, 0x20)
#endif
#ifndef DRM_FORMAT_MOD_QCOM_LOSSY_2_1
#define DRM_FORMAT_MOD_QCOM_LOSSY_2_1 fourcc_mod_code(QCOM, 0x200)
#endif

#define DEST_SCALAR_OVERFETCH_SIZE 5

using drm_utils::DRMLibLoader;
using drm_utils::DRMMaster;
using drm_utils::DRMResMgr;
using sde_drm::DRMBlendType;
using sde_drm::DRMCacMode;
using sde_drm::DRMConnectorInfo;
using sde_drm::DRMCrtcInfo;
using sde_drm::DRMCscType;
using sde_drm::DRMCWbCaptureMode;
using sde_drm::DRMDisplayToken;
using sde_drm::DRMDisplayType;
using sde_drm::DRMMultiRectMode;
using sde_drm::DRMOps;
using sde_drm::DRMPowerMode;
using sde_drm::DRMPPFeatureInfo;
using sde_drm::DRMRect;
using sde_drm::DRMRotation;
using sde_drm::DRMSecureMode;
using sde_drm::DRMSecurityLevel;
using sde_drm::DRMSrcConfig;
using sde_drm::DRMTopology;
using sde_drm::DRMUcscGcMode;
using sde_drm::DRMUcscIgcMode;
using sde_drm::GetDRMManager;
using std::fstream;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;
using std::to_string;
using std::unordered_map;

namespace sdm {

std::unordered_map<uint32_t, std::atomic<uint32_t>> HWDeviceDRM::hw_dest_scaler_blocks_used_;
std::atomic<uint32_t> HWDeviceDRM::hw_ai_scaler_blocks_used_(0);
std::unordered_map<uint32_t, std::mutex> HWDeviceDRM::cwb_state_lock_;
bool HWDeviceDRM::reset_planes_luts_ = true;

static PPBlock GetPPBlock(const HWToneMapLut &lut_type) {
  PPBlock pp_block = kPPBlockMax;
  switch (lut_type) {
    case kDma1dIgc:
    case kDma1dGc:
      pp_block = kDGM;
      break;
    case kVig1dIgc:
    case kVig3dGamut:
      pp_block = kVIG;
      break;
    default:
      DLOGE("Unknown PP Block");
      break;
  }
  return pp_block;
}

static uint64_t GetDRMModifier(uint64_t default_modifier, HWCacColorComponent cac_color) {
  switch (cac_color) {
    case kCacRed:
      return DRM_FORMAT_MOD_QCOM_CAC_R;
    case kCacGreen:
      return DRM_FORMAT_MOD_QCOM_CAC_G;
    case kCacBlue:
      return DRM_FORMAT_MOD_QCOM_CAC_B;
    default:
      return default_modifier;
  }
}

static void GetDRMFormat(LayerBufferFormat format, uint32_t *drm_format,
                         uint64_t *drm_format_modifier, HWCacColorComponent cac_color) {
  switch (format) {
    case kFormatARGB8888:
      *drm_format = DRM_FORMAT_BGRA8888;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGBA8888:
      *drm_format = DRM_FORMAT_ABGR8888;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGBA8888Ubwc:
      *drm_format = DRM_FORMAT_ABGR8888;
      *drm_format_modifier = GetDRMModifier(DRM_FORMAT_MOD_QCOM_COMPRESSED, cac_color);
      break;
    case kFormatRGBA5551:
      *drm_format = DRM_FORMAT_ABGR1555;
      break;
    case kFormatRGBA4444:
      *drm_format = DRM_FORMAT_ABGR4444;
      break;
    case kFormatBGRA8888:
      *drm_format = DRM_FORMAT_ARGB8888;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGBX8888:
      *drm_format = DRM_FORMAT_XBGR8888;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGBX8888Ubwc:
      *drm_format = DRM_FORMAT_XBGR8888;
      *drm_format_modifier = GetDRMModifier(DRM_FORMAT_MOD_QCOM_COMPRESSED, cac_color);
      break;
    case kFormatBGRX8888:
      *drm_format = DRM_FORMAT_XRGB8888;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGB888:
      *drm_format = DRM_FORMAT_BGR888;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatBGR888:
      *drm_format = DRM_FORMAT_RGB888;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGB565:
      *drm_format = DRM_FORMAT_BGR565;
      break;
    case kFormatBGR565:
      *drm_format = DRM_FORMAT_RGB565;
      break;
    case kFormatBGR565Ubwc:
      *drm_format = DRM_FORMAT_BGR565;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED;
      break;
    case kFormatRGBA1010102:
      *drm_format = DRM_FORMAT_ABGR2101010;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGBA1010102Ubwc:
      *drm_format = DRM_FORMAT_ABGR2101010;
      *drm_format_modifier = GetDRMModifier(DRM_FORMAT_MOD_QCOM_COMPRESSED, cac_color);
      break;
    case kFormatARGB2101010:
      *drm_format = DRM_FORMAT_BGRA1010102;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGBX1010102:
      *drm_format = DRM_FORMAT_XBGR2101010;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatRGBX1010102Ubwc:
      *drm_format = DRM_FORMAT_XBGR2101010;
      *drm_format_modifier = GetDRMModifier(DRM_FORMAT_MOD_QCOM_COMPRESSED, cac_color);
      break;
    case kFormatXRGB2101010:
      *drm_format = DRM_FORMAT_BGRX1010102;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatBGRA1010102:
      *drm_format = DRM_FORMAT_ARGB2101010;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatABGR2101010:
      *drm_format = DRM_FORMAT_RGBA1010102;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatBGRX1010102:
      *drm_format = DRM_FORMAT_XRGB2101010;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatXBGR2101010:
      *drm_format = DRM_FORMAT_RGBX1010102;
      *drm_format_modifier = GetDRMModifier(*drm_format_modifier, cac_color);
      break;
    case kFormatYCbCr420SemiPlanar:
      *drm_format = DRM_FORMAT_NV12;
      break;
    case kFormatYCbCr420SemiPlanarVenus:
      *drm_format = DRM_FORMAT_NV12;
      break;
    case kFormatYCbCr420SPVenusUbwc:
      *drm_format = DRM_FORMAT_NV12;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED;
      break;
    case kFormatYCbCr420SPVenusTile:
      *drm_format = DRM_FORMAT_NV12;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_TILE;
      break;
    case kFormatYCrCb420SemiPlanar:
      *drm_format = DRM_FORMAT_NV21;
      break;
    case kFormatYCrCb420SemiPlanarVenus:
      *drm_format = DRM_FORMAT_NV21;
      break;
    case kFormatYCbCr420P010:
    case kFormatYCbCr420P010Venus:
      *drm_format = DRM_FORMAT_NV12;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_DX;
      break;
    case kFormatYCbCr420P010Ubwc:
      *drm_format = DRM_FORMAT_NV12;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED |
        DRM_FORMAT_MOD_QCOM_DX;
      break;
    case kFormatYCbCr420P010Tile:
      *drm_format = DRM_FORMAT_NV12;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_TILE |
        DRM_FORMAT_MOD_QCOM_DX;
      break;
    case kFormatYCbCr420TP10Ubwc:
      *drm_format = DRM_FORMAT_NV12;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED |
        DRM_FORMAT_MOD_QCOM_DX | DRM_FORMAT_MOD_QCOM_TIGHT;
      break;
    case kFormatYCbCr420TP10Tile:
      *drm_format = DRM_FORMAT_NV12;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_TILE |
        DRM_FORMAT_MOD_QCOM_DX | DRM_FORMAT_MOD_QCOM_TIGHT;
      break;
    case kFormatYCbCr422H2V1SemiPlanar:
      *drm_format = DRM_FORMAT_NV16;
      break;
    case kFormatYCrCb422H2V1SemiPlanar:
      *drm_format = DRM_FORMAT_NV61;
      break;
    case kFormatYCrCb420PlanarStride16:
      *drm_format = DRM_FORMAT_YVU420;
      break;
    case kFormatRGBA16161616F:
      *drm_format = DRM_FORMAT_ABGR16161616F;
      break;
    case kFormatRGBA16161616FUbwc:
      *drm_format = DRM_FORMAT_ABGR16161616F;
      *drm_format_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED;
      break;
    case kFormatRGBA8888UbwcLossy2To1:
      *drm_format = DRM_FORMAT_ABGR8888;
      *drm_format_modifier =
          DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_LOSSY_2_1;
      break;
    case kFormatRGBA8888UbwcLossy8To5:
      *drm_format = DRM_FORMAT_ABGR8888;
      *drm_format_modifier =
          DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_LOSSY_8_5;
      break;
    default:
      DLOGW("Unsupported format %s", GetFormatString(format));
  }
}

FrameBufferObject::FrameBufferObject(uint32_t fb_id, uint32_t core_id, LayerBufferFormat format,
                                     uint32_t width, uint32_t height, bool shallow, bool secure)
    : fb_id_(fb_id),
      core_id_(core_id),
      format_(format),
      width_(width),
      height_(height),
      shallow_(shallow),
      secure_(secure) {}

FrameBufferObject::~FrameBufferObject() {
  // Don't call RemoveFbId in case its a shallow copy from other display
  if (shallow_) {
    DLOGI("FBID: %d is a shallow copy", fb_id_);
    return;
  }

  DRMMaster *master;
  DRMMaster::GetInstance(&master, core_id_);
  int ret = master->RemoveFbId(fb_id_);
  if (ret < 0) {
    DLOGE("Removing fb_id %d failed with error %d", fb_id_, errno);
  }
}

uint32_t FrameBufferObject::GetFbId() {
  return fb_id_;
}

bool FrameBufferObject::IsEqual(LayerBufferFormat format, uint32_t width, uint32_t height,
                                bool secure) {
  // Create a new framebuffer object when the format, width, height, or secure flag gets updated
  return (format == format_ && width == width_ && height == height_ && secure == secure_);
}

HWDeviceDRM::Registry::Registry(BufferAllocator *buffer_allocator) :
  buffer_allocator_(buffer_allocator) {
  int value = 0;
  if (Debug::GetProperty(DISABLE_FBID_CACHE, &value) == kErrorNone) {
    disable_fbid_cache_ = (value == 1);
  }
}

void HWDeviceDRM::Registry::Init(Handle master, CacVersion cac_version, uint32_t core_id) {
  master_ = master;
  cac_version_ = cac_version;
  core_id_ = core_id;
}

int HWDeviceDRM::Registry::Register(HWLayersInfo *hw_layers_info) {
  uint32_t hw_layer_count = UINT32(hw_layers_info->hw_layers.size());
  int err = 0;
  bool fb_modified = false;

  for (uint32_t i = 0; i < hw_layer_count; i++) {
    Layer &layer = hw_layers_info->hw_layers.at(i);
    HWLayerConfig &layer_config = hw_layers_info->config[i];
    LayerBuffer input_buffer = layer.input_buffer;
    HWRotatorSession *hw_rotator_session = &hw_layers_info->config[i].hw_rotator_session;
    HWRotateInfo *hw_rotate_info = &hw_rotator_session->hw_rotate_info[0];
    fbid_cache_limit_ = input_buffer.flags.video ? VIDEO_FBID_LIMIT : UI_FBID_LIMIT;

    if (hw_rotator_session->mode == kRotatorOffline && hw_rotate_info->valid) {
      input_buffer = hw_rotator_session->output_buffer;
      fbid_cache_limit_ = OFFLINE_ROTATOR_FBID_LIMIT;
    }

    if (input_buffer.flags.interlace) {
      input_buffer.width *= 2;
      input_buffer.height /= 2;
    }
    int ret = MapBufferToFbId(&layer, input_buffer, &fb_modified,
                              layer_config.tunnel_pipes.size() > 0 ? true : false,
                              hw_layers_info->dummy_loopback_cac_info);
    if (!err) {
      err = ret;
      if (fb_modified) {
        hw_layers_info->common_info->updates_mask.set(kUpdateFBObject);
      }
    }
  }
  return err;
}

void HWDeviceDRM::Registry::GetBufInfoForTunnelPipe(HWCacColorComponent color,
                                                    BufferInfo *loopback_cac_info,
                                                    AllocatedBufferInfo *buf_info,
                                                    DRMBuffer *layout) {
  if ((cac_version_ != kCacVersionLoopback) || (color == kCacNone)) {
    return;
  }
  // Using the plane buffer fd and faking the buffer as full screen for CAC loopback
  buf_info->aligned_width = layout->width = loopback_cac_info->alloc_buffer_info.aligned_width;
  buf_info->aligned_height = layout->height = loopback_cac_info->alloc_buffer_info.aligned_height;
  buf_info->format = loopback_cac_info->buffer_config.format;
  buffer_allocator_->GetBufferLayout(*buf_info, layout->stride, layout->offset,
                                     &layout->num_planes);
}

int HWDeviceDRM::Registry::CreateFbId(const LayerBuffer &buffer, std::vector<uint32_t> *fb_id,
                                      BufferInfo *loopback_cac_info) {
  DRMMaster *master = reinterpret_cast<DRMMaster*>(master_);
  int ret = -1;

  if (!master) {
    DLOGE("Failed to acquire DRM Master instance");
    return ret;
  }

  uint32_t *fb_id_data = fb_id->data();
  DRMBuffer layout{};
  AllocatedBufferInfo buf_info{};
  buf_info.fd = layout.fd = buffer.planes[0].fd;
  buf_info.aligned_width = layout.width = buffer.width;
  buf_info.aligned_height = layout.height = buffer.height;
  buf_info.format = buffer.format;
  buf_info.usage = buffer.usage;
  buffer_allocator_->GetBufferLayout(buf_info, layout.stride, layout.offset, &layout.num_planes);
  if (buffer.format == kFormatRGBA8888UbwcLossy2To1) {
    layout.height *= 2;
  }
  for (int color = 0; color < fb_id->size(); color++) {
    GetBufInfoForTunnelPipe(static_cast<HWCacColorComponent>(color), loopback_cac_info, &buf_info,
                            &layout);
    GetDRMFormat(buf_info.format, &layout.drm_format, &layout.drm_format_modifier,
                 static_cast<HWCacColorComponent>(color));
    ret = master->CreateFbId(layout, fb_id_data);
    if (ret < 0) {
      DLOGE(
          "CreateFbId failed. width %d, height %d, format: %s, stride %u, "
          "cac_color %d, usage %d error %d",
          layout.width, layout.height, GetFormatString(buf_info.format), layout.stride[0], color,
          buffer.usage, errno);
    }
    fb_id_data++;
  }

  return ret;
}

int HWDeviceDRM::Registry::MapBufferToFbId(Layer *layer, const LayerBuffer &buffer,
                                           bool *fb_modified, bool is_cac_buffer,
                                           BufferInfo &loopback_cac_info) {
  if (buffer.planes[0].fd < 0) {
    return 0;
  }

  uint64_t handle_id = buffer.handle_id;
  bool secure_present =
      (buffer.flags.secure || buffer.flags.secure_display || buffer.flags.secure_camera);

  if (!handle_id || disable_fbid_cache_) {
    // In legacy path, clear fb_id map in each frame.
    layer->buffer_map->buffer_map.clear();
  } else {
    if (layer->composition == kCompositionCWBTarget) {
      layer->buffer_map->buffer_map.clear();
      auto it2 = output_buffer_map_.find(handle_id);
      if (it2 != output_buffer_map_.end()) {
        std::unordered_map<uint32_t, std::shared_ptr<LayerBufferObject>> dpu_buffer_map =
            it2->second;
        auto itr = dpu_buffer_map.find(core_id_);
        if (itr != dpu_buffer_map.end()) {
          FrameBufferObject *fb_obj = static_cast<FrameBufferObject*>(itr->second.get());
          if (fb_obj->IsEqual(buffer.format, buffer.width, buffer.height, secure_present)) {
            // make local copy of output_buffer_map_
            std::unordered_map<uint32_t,
                std::vector<std::shared_ptr<LayerBufferObject>>> temp_output_buffer;
            for (auto& itr2: output_buffer_map_[handle_id]) {
              std::vector<std::shared_ptr<LayerBufferObject>> temp_layer_buffer;
              temp_layer_buffer.push_back(itr2.second);
              temp_output_buffer[itr2.first] = temp_layer_buffer;
            }
            // update buffer_map with local output_buffer_map
            layer->buffer_map->buffer_map[handle_id] = temp_output_buffer;
            // Found fb_id for given handle_id key
            return 0;
          }
        }
      }
    }
    uint32_t fb_id_size = is_cac_buffer ? 4 : 1;
    auto it = layer->buffer_map->buffer_map.find(handle_id);
    if (it != layer->buffer_map->buffer_map.end()) {
      auto itr = it->second.find(core_id_);
      if (itr != it->second.end()) {
        FrameBufferObject *fb_obj = static_cast<FrameBufferObject*>(itr->second[kCacNone].get());
        if (fb_obj->IsEqual(buffer.format, buffer.width, buffer.height, secure_present) &&
            (it->second.size() >= fb_id_size)) {
          // Found fb_id for given handle_id key
          return 0;
        } else {
          // Erase from fb_id map if format or size have been modified
          layer->buffer_map->buffer_map.erase(it);
        }
      }
    }

    if (layer->buffer_map->buffer_map.size() >= fbid_cache_limit_) {
      // Clear fb_id map, if the size reaches cache limit.
      layer->buffer_map->buffer_map.clear();
    }
  }

  std::vector<uint32_t> fb_id(1);
  if (is_cac_buffer) {
    fb_id.resize(4);
  }
  if (CreateFbId(buffer, &fb_id, &loopback_cac_info) >= 0) {
    // Create and cache the fb_id in map
    std::vector<std::shared_ptr<LayerBufferObject>> fb_id_vec;
    for (int i = 0; i < fb_id.size(); i++) {
      std::shared_ptr<LayerBufferObject> bo =
          std::make_shared<FrameBufferObject>(fb_id[i], core_id_, buffer.format, buffer.width,
                                              buffer.height, false /* shallow */, secure_present);
      fb_id_vec.push_back(bo);
    }

    auto it = layer->buffer_map->buffer_map.find(handle_id);
    if (it != layer->buffer_map->buffer_map.end()) {
      it->second.insert({core_id_, fb_id_vec});
    } else {
      std::unordered_map<uint32_t, std::vector<std::shared_ptr<LayerBufferObject>>> dpu_buffer_map;
      dpu_buffer_map[core_id_] = fb_id_vec;
      layer->buffer_map->buffer_map[handle_id] = dpu_buffer_map;
    }
    *fb_modified = true;
  }
  return 0;
}

void HWDeviceDRM::Registry::MapOutputBufferToFbId(std::shared_ptr<LayerBuffer> output_buffer,
                                                  bool *fb_modified) {
  if (output_buffer->planes[0].fd < 0) {
    return;
  }

  uint64_t handle_id = output_buffer->handle_id;
  bool secure_present = (output_buffer->flags.secure || output_buffer->flags.secure_display ||
                         output_buffer->flags.secure_camera);

  if (!handle_id || disable_fbid_cache_) {
    // In legacy path, clear output buffer map in each frame.
    output_buffer_map_.clear();
  } else {
    auto it = output_buffer_map_.find(handle_id);
    if (it != output_buffer_map_.end()) {
      std::unordered_map<uint32_t, std::shared_ptr<LayerBufferObject>> dpu_buffer_map = it->second;
      auto itr = dpu_buffer_map.find(core_id_);
      if (itr != dpu_buffer_map.end()) {
        FrameBufferObject *fb_obj = static_cast<FrameBufferObject*>(itr->second.get());
        if (fb_obj->IsEqual(output_buffer->format, output_buffer->width, output_buffer->height,
                            secure_present)) {
          return;
        } else {
          output_buffer_map_.erase(it);
        }
      }
    }

    if (output_buffer_map_.size() >= UI_FBID_LIMIT) {
      // Clear output buffer map, if the size reaches cache limit.
      output_buffer_map_.clear();
    }
  }

  std::vector<uint32_t> fb_id(1);
  if (CreateFbId(*output_buffer, &fb_id) >= 0) {
    std::unordered_map<uint32_t, std::shared_ptr<LayerBufferObject>> dpu_buffer_map;
    dpu_buffer_map[core_id_] = std::make_shared<FrameBufferObject>(fb_id[kCacNone],
        core_id_, output_buffer->format, output_buffer->width, output_buffer->height,
        false /* shallow */, secure_present);
    output_buffer_map_[handle_id] = dpu_buffer_map;
    *fb_modified = true;
  }
}

void HWDeviceDRM::Registry::Clear() {
  output_buffer_map_.clear();
}

void HWDeviceDRM::Registry::GetFbId(Layer *layer, uint64_t handle_id,
                                    std::vector<uint32_t> *fb_id_vec) {
  auto it = layer->buffer_map->buffer_map.find(handle_id);
  if (it != layer->buffer_map->buffer_map.end()) {
    auto itr = it->second.find(core_id_);
    if (itr != it->second.end()) {
      for (int i = 0; i < itr->second.size(); i++) {
        FrameBufferObject *fb_obj = static_cast<FrameBufferObject *>(itr->second[i].get());
        fb_id_vec->push_back(fb_obj->GetFbId());
      }
    }
  }

  if (fb_id_vec->size() == 0) {
    fb_id_vec->push_back(0);  // failure case
  }
}

uint32_t HWDeviceDRM::Registry::GetOutputFbId(uint64_t handle_id) {
  auto it = output_buffer_map_.find(handle_id);
  if (it != output_buffer_map_.end()) {
    std::unordered_map<uint32_t, std::shared_ptr<LayerBufferObject>> dpu_buffer_map = it->second;
    auto itr = dpu_buffer_map.find(core_id_);
      if (itr != dpu_buffer_map.end()) {
        FrameBufferObject *fb_obj = static_cast<FrameBufferObject*>(itr->second.get());
        return fb_obj->GetFbId();
      }
  }

  return 0;
}

HWDeviceDRM::HWDeviceDRM(BufferAllocator *buffer_allocator, HWInfoInterface *hw_info_intf)
    : hw_info_intf_(hw_info_intf), registry_(buffer_allocator) {
  hw_info_intf_ = hw_info_intf;
}

DisplayError HWDeviceDRM::Init() {
  int ret = 0;
  DRMMaster *drm_master = {};
  DRMMaster::GetInstance(&drm_master, core_id_);
  drm_master->GetHandle(&dev_fd_);
  DRMLibLoader *drm_lib_loader = DRMLibLoader::GetInstance(core_id_);

  if (!drm_lib_loader) {
    DLOGW("Failed to retrieve DRMLibLoader instance");
    return kErrorResources;
  }

  drm_lib_loader->FuncGetDRMManager()(dev_fd_, &drm_mgr_intf_);

  if (-1 == display_id_) {
    if (drm_mgr_intf_->RegisterDisplay(disp_type_, &token_)) {
      DLOGE("RegisterDisplay (by type) failed for %s", device_name_);
      return kErrorResources;
    }
  } else if (drm_mgr_intf_->RegisterDisplay(display_id_, &token_)) {
    DLOGE("RegisterDisplay (by id) failed for %s - %d", device_name_, display_id_);
    return kErrorResources;
  }

  if (token_.conn_id > INT32_MAX) {
    DLOGE("Connector id %u beyond supported range", token_.conn_id);
    drm_mgr_intf_->UnregisterDisplay(&token_);
    return kErrorNotSupported;
  }

  ret = drm_mgr_intf_->CreateAtomicReq(token_, &drm_atomic_intf_);
  if (ret) {
    DLOGE("Failed creating atomic request for connector id %u. Error: %d.", token_.conn_id, ret);
    drm_mgr_intf_->UnregisterDisplay(&token_);
    return kErrorResources;
  }

  ret = drm_mgr_intf_->GetConnectorInfo(token_.conn_id, &connector_info_);
  if (ret) {
    DLOGE("Failed getting info for connector id %u. Error: %d.", token_.conn_id, ret);
    drm_mgr_intf_->DestroyAtomicReq(drm_atomic_intf_);
    drm_atomic_intf_ = {};
    drm_mgr_intf_->UnregisterDisplay(&token_);
    return kErrorHardware;
  }

  if (!connector_info_.is_connected || connector_info_.modes.empty()) {
    DLOGW("Device removal detected on connector id %u. Connector status %s and %zu modes.",
          token_.conn_id, connector_info_.is_connected ? "connected":"disconnected",
          connector_info_.modes.size());
    drm_mgr_intf_->DestroyAtomicReq(drm_atomic_intf_);
    drm_atomic_intf_ = {};
    drm_mgr_intf_->UnregisterDisplay(&token_);
    return kErrorDeviceRemoved;
  }

  hw_info_intf_->GetHWResourceInfo(&hw_resource_);

  InitializeConfigs();
  GetCWBCapabilities();
  PopulateHWPanelInfo();
  UpdateMixerAttributes();

  // TODO(user): In future, remove has_qseed3 member, add version and pass version to constructor
  if (hw_resource_.has_qseed3) {
    hw_scale_ = new HWScaleDRM(HWScaleDRM::Version::V2);
  }

  std::unique_ptr<HWColorManagerDrm> hw_color_mgr(new HWColorManagerDrm());
  hw_color_mgr_ = std::move(hw_color_mgr);

  registry_.Init(drm_master, hw_resource_.cac_version, core_id_);
  display_id_ = static_cast<int32_t>(token_.conn_id);

  int value = 0;
  if (Debug::GetProperty(FORCE_TONEMAPPING, &value) == kErrorNone) {
    force_tonemapping_ = (value == 1);
    DLOGI("force_tonemapping_ %d", force_tonemapping_);
  }

  value = 0;
  if (Debug::GetProperty(ASPECT_RATIO_THRESHOLD, &value) == kErrorNone) {
    aspect_ratio_threshold_ = 1 + (FLOAT(value) / 100);
    DLOGI("aspect_ratio_threshold_: %f", aspect_ratio_threshold_);
  }

  value = 0;
  Debug::GetProperty(ENABLE_BRIGHTNESS_DRM_PROP, &value);
  enable_brightness_drm_prop_ = (value == 1);

#ifdef SEC_GC_QC_SUPERHDR
  value = 0;
  Debug::GetProperty("vendor.display.enable_brightness_drm_prop_in_super_hdr", &value);
  enable_brightness_drm_prop_ |= (value == 1);

  DLOGI("vendor.display.enable_brightness_drm_prop_in_super_hdr:%d, enable_brightness_drm_prop_:%d", 
         value, enable_brightness_drm_prop_);
#endif

  return kErrorNone;
}

DisplayError HWDeviceDRM::Deinit() {
  DisplayError err = kErrorNone;
  // first_null_cycle_ = false after the first power-mode NullCommit is completed, or the first
  // frame commit handled the power transition.
  // Power-on will set the CRTC_SET_MODE anytime power mode is not deferred to first commit.
  // Without first commit, if display is disconnected, CRTC_SET_MODE is not set to NULL,
  // this leads to a synchronization issue.
  // So because of previously successful NullCommit, set CRTC_SET_MODE to NULL here for proper sync.

  // first_cycle = false after the first frame commit is completed.
  // A null-commit here is also needed if the first commit has gone through. e.g., If a
  // display is connected and disconnected, HWDeviceDRM::Deinit() may be called
  // before any driver commit happened on the device. The driver may have removed any not-in-use
  // connector (i.e., any connector which did not have a display commit on it and a crtc path
  // setup), so token_.conn_id may have been removed if there was no commit, resulting in
  // drmModeAtomicCommit() failure with ENOENT, 'No such file or directory'.
  if (!first_cycle_ || !first_null_cycle_) {
    ClearSolidfillStages();
    ClearNoiseLayerConfig();
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, token_.conn_id, 0);
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POWER_MODE, token_.conn_id, DRMPowerMode::OFF);
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_MODE, token_.crtc_id, nullptr);
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ACTIVE, token_.crtc_id, 0);
#ifdef TRUSTED_VM
    drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_VM_REQ_STATE, token_.crtc_id,
                              sde_drm::DRMVMRequestState::RELEASE);
#endif
    int ret = NullCommit(true /* synchronous */, false /* retain_planes */);
    if (ret) {
      DLOGE("Commit failed with error: %d", ret);
      err = kErrorHardware;
    }
  }
  delete hw_scale_;
  registry_.Clear();
  display_attributes_ = {};
  drm_mgr_intf_->DestroyAtomicReq(drm_atomic_intf_);
  drm_atomic_intf_ = {};
  drm_mgr_intf_->UnregisterDisplay(&token_);
  hw_dest_scaler_blocks_used_[core_id_] -= dest_scaler_blocks_used_;
  hw_ai_scaler_blocks_used_ -= ai_scaler_blocks_used_;
  return err;
}

void HWDeviceDRM::GetCWBCapabilities() {
  sde_drm::DRMConnectorsInfo conns_info = {};
  int ret = drm_mgr_intf_->GetConnectorsInfo(&conns_info);
  if (ret) {
    DLOGW("DRM Driver error %d while getting Connectors info.", ret);
    return;
  }
  for (auto &iter : conns_info) {
    if (iter.second.type == DRM_MODE_CONNECTOR_VIRTUAL) {
      has_cwb_crop_ = static_cast<bool>(iter.second.modes[current_mode_index_].has_cwb_crop);
      has_dedicated_cwb_ =
          static_cast<bool>(iter.second.modes[current_mode_index_].has_dedicated_cwb);
      has_cwb_dither_ = static_cast<bool>(iter.second.has_cwb_dither);
      if (!max_cwb_) {
        auto &conn_mode = iter.second.modes[current_mode_index_];
        if (has_dedicated_cwb_) {
          max_cwb_ = (conn_mode.max_cwb >= INT32_MAX || !conn_mode.max_cwb) ? 1 : conn_mode.max_cwb;
        }
      }
      DLOGI("Max supported CWB session = %d", max_cwb_);
      break;
    }
  }
}

DisplayError HWDeviceDRM::GetDisplayId(int32_t *display_id) {
  *display_id = display_id_;
  return kErrorNone;
}

void HWDeviceDRM::InitializeConfigs() {
  current_mode_index_ = 0;
  uint32_t modes_count = connector_info_.modes.size();
  uint32_t panel_mode_pref = (connector_info_.panel_mode == sde_drm::DRMPanelMode::COMMAND)
                                 ? DRM_MODE_FLAG_CMD_MODE_PANEL
                                 : DRM_MODE_FLAG_VID_MODE_PANEL;

  // Set mode with preferred panel mode if supported, otherwise set based on capability
  for (uint32_t mode_index = 0; mode_index < modes_count; mode_index++) {
    uint32_t sub_mode_index = connector_info_.modes[mode_index].curr_submode_index;
    connector_info_.modes[mode_index].curr_compression_mode =
              connector_info_.modes[mode_index].sub_modes[sub_mode_index].panel_compression_mode;
    connector_info_.modes[mode_index].curr_bpp_mode =
              connector_info_.modes[mode_index].sub_modes[sub_mode_index].bpp_mode;
    if (panel_mode_pref &
        connector_info_.modes[mode_index].sub_modes[sub_mode_index].panel_mode_caps) {
      connector_info_.modes[mode_index].cur_panel_mode = panel_mode_pref;
    } else if (panel_mode_pref == DRM_MODE_FLAG_VID_MODE_PANEL) {
      connector_info_.modes[mode_index].cur_panel_mode = DRM_MODE_FLAG_VID_MODE_PANEL;
    } else if (panel_mode_pref == DRM_MODE_FLAG_CMD_MODE_PANEL) {
      connector_info_.modes[mode_index].cur_panel_mode = DRM_MODE_FLAG_CMD_MODE_PANEL;
    }
    // Add mode variant if both panel modes are supported
    if (connector_info_.modes[mode_index].sub_modes[sub_mode_index].panel_mode_caps &
        DRM_MODE_FLAG_CMD_MODE_PANEL &&
        connector_info_.modes[mode_index].sub_modes[sub_mode_index].panel_mode_caps &
        DRM_MODE_FLAG_VID_MODE_PANEL) {
      sde_drm::DRMModeInfo mode_item = connector_info_.modes[mode_index];
      mode_item.cur_panel_mode =
          (connector_info_.modes[mode_index].cur_panel_mode == DRM_MODE_FLAG_CMD_MODE_PANEL)
              ? DRM_MODE_FLAG_VID_MODE_PANEL
              : DRM_MODE_FLAG_CMD_MODE_PANEL;
      connector_info_.modes.push_back(mode_item);
    }
  }
  // Update current mode with preferred mode
  for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
      if (connector_info_.modes[mode_index].mode.type & DRM_MODE_TYPE_PREFERRED) {
        DLOGI("Updating current display %d-%d mode %d to preferred mode %d.", display_id_,
              disp_type_, current_mode_index_, mode_index);
        current_mode_index_ = mode_index;
        break;
      }
  }

  display_attributes_.resize(connector_info_.modes.size());

  for (uint32_t i = 0; i < connector_info_.modes.size(); i++) {
    PopulateDisplayAttributes(i);
  }
  SetDisplaySwitchMode(current_mode_index_);
}

DisplayError HWDeviceDRM::PopulateDisplayAttributes(uint32_t index) {
  drmModeModeInfo mode = {};
  sde_drm::DRMModeInfo conn_mode = {};
  uint32_t mm_width = 0;
  uint32_t mm_height = 0;
  DRMTopology topology = DRMTopology::SINGLE_LM;

  if (default_mode_) {
    DRMResMgr *res_mgr = nullptr;
    int ret = DRMResMgr::GetInstance(&res_mgr);
    if (ret < 0) {
      DLOGE("Failed to acquire DRMResMgr instance");
      return kErrorResources;
    }

    res_mgr->GetMode(&mode);
    res_mgr->GetDisplayDimInMM(&mm_width, &mm_height);
  } else {
    uint32_t submode_idx = connector_info_.modes[index].curr_submode_index;
    conn_mode = connector_info_.modes[index];
    mode = conn_mode.mode;
    mm_width = connector_info_.mmWidth;
    mm_height = connector_info_.mmHeight;
    topology = connector_info_.modes[index].sub_modes[submode_idx].topology;
    if (connector_info_.modes[index].sub_modes[submode_idx].panel_mode_caps &
        DRM_MODE_FLAG_CMD_MODE_PANEL) {
      display_attributes_[index].smart_panel = true;
    }
  }

  display_attributes_[index].x_pixels = mode.hdisplay;
  display_attributes_[index].y_pixels = mode.vdisplay;
  display_attributes_[index].fps = mode.vrefresh;
  display_attributes_[index].vsync_period_ns =
    UINT32(1000000000L / display_attributes_[index].fps);

  /*
              Active                 Front           Sync           Back
              Region                 Porch                          Porch
     <-----------------------><----------------><-------------><-------------->
     <----- [hv]display ----->
     <------------- [hv]sync_start ------------>
     <--------------------- [hv]sync_end --------------------->
     <-------------------------------- [hv]total ----------------------------->
   */

  display_attributes_[index].v_front_porch = mode.vsync_start - mode.vdisplay;
  bool adjusted = (connector_info_.dyn_bitclk_support &&
                   conn_mode.fp_type == sde_drm::DynamicFrontPorchType::VERTICAL &&
                   !conn_mode.dyn_fp_list.empty());
  if (adjusted) {
    display_attributes_[index].v_front_porch =
        *std::min_element(conn_mode.dyn_fp_list.begin(), conn_mode.dyn_fp_list.end());
  }
  display_attributes_[index].v_pulse_width = mode.vsync_end - mode.vsync_start;
  display_attributes_[index].v_back_porch = mode.vtotal - mode.vsync_end;
  display_attributes_[index].v_total = mode.vtotal;
  display_attributes_[index].h_total = mode.htotal;

  // TODO(user): This clock should no longer be used as mode's pixel clock for RFI connectors.
  // Driver can expose list of dynamic pixel clocks, userspace needs to support dynamic change.
  display_attributes_[index].clock_khz = mode.clock;

  // If driver doesn't return panel width/height information, default to 320 dpi
  if (INT(mm_width) <= 0 || INT(mm_height) <= 0) {
    mm_width  = UINT32(((FLOAT(mode.hdisplay) * 25.4f) / 320.0f) + 0.5f);
    mm_height = UINT32(((FLOAT(mode.vdisplay) * 25.4f) / 320.0f) + 0.5f);
    DLOGW("Driver doesn't report panel physical width and height - defaulting to 320dpi");
  }

  display_attributes_[index].x_dpi = (FLOAT(mode.hdisplay) * 25.4f) / FLOAT(mm_width);
  display_attributes_[index].y_dpi = (FLOAT(mode.vdisplay) * 25.4f) / FLOAT(mm_height);
  SetTopology(topology, &display_attributes_[index].topology);
  SetTopologySplit(display_attributes_[index].topology,
                   &display_attributes_[index].topology_num_split);
  display_attributes_[index].is_device_split = (display_attributes_[index].topology_num_split > 1);
  display_attributes_[index].allowed_mode_switch = connector_info_.modes[index].allowed_mode_switch;
  display_attributes_[index].avr_step = connector_info_.modes[index].avr_step_fps;
  display_attributes_[index].early_ept_timeout = connector_info_.modes[index].early_ept_timeout;

  DLOGI(
      "Display %d-%d attributes[%d]: WxH: %dx%d, DPI: %fx%f, FPS: %d, LM_SPLIT: %d, V_BACK_PORCH:"
      " %d, V_FRONT_PORCH: %d [RFI Adjusted : %s], V_PULSE_WIDTH: %d, V_TOTAL: %d, H_TOTAL: %d,"
      " CLK: %dKHZ, TOPOLOGY: %d [SPLIT NUMBER: %d], HW_SPLIT: %d, AVR_STEP: %d",
      display_id_, disp_type_, index, display_attributes_[index].x_pixels,
      display_attributes_[index].y_pixels, display_attributes_[index].x_dpi,
      display_attributes_[index].y_dpi, display_attributes_[index].fps,
      display_attributes_[index].is_device_split, display_attributes_[index].v_back_porch,
      display_attributes_[index].v_front_porch, adjusted ? "True" : "False",
      display_attributes_[index].v_pulse_width, display_attributes_[index].v_total,
      display_attributes_[index].h_total, display_attributes_[index].clock_khz,
      display_attributes_[index].topology, display_attributes_[index].topology_num_split,
      mixer_attributes_.split_type, display_attributes_[index].avr_step);

  return kErrorNone;
}

void HWDeviceDRM::PopulateHWPanelInfo() {
  hw_panel_info_ = {};

  snprintf(hw_panel_info_.panel_name, sizeof(hw_panel_info_.panel_name), "%s",
           connector_info_.panel_name.c_str());

  uint32_t index = current_mode_index_;
  uint32_t sub_mode_index = connector_info_.modes[index].curr_submode_index;
  hw_panel_info_.split_info.left_split = display_attributes_[index].x_pixels;
  if (display_attributes_[index].is_device_split) {
    hw_panel_info_.split_info.left_split = hw_panel_info_.split_info.right_split =
        display_attributes_[index].x_pixels / 2;
  }

  int value = 0;
  bool enable_ai_scaler = false;
  if (Debug::GetProperty(ENABLE_AI_SCALER_PROP, &value) == kErrorNone) {
    enable_ai_scaler = (value == 1);
  }

  bool enable_abc = false;
  if (Debug::GetProperty(ENABLE_ABC, &value) == kErrorNone) {
    enable_abc = (value == 1);
  }

  bool enable_ssrc = false;
  if (Debug::GetProperty(AIQE_SSRC_ENABLE, &value) == kErrorNone) {
    enable_ssrc = (value == 1);
  }

  if (enable_ai_scaler || enable_abc || enable_ssrc) {
    hw_panel_info_.partial_update = false;
    hw_panel_info_.ssip_enabled = true;
  } else {
    hw_panel_info_.partial_update = connector_info_.modes[index].num_roi;
  }

  hw_panel_info_.has_ai_scaler = enable_ai_scaler;
  hw_panel_info_.left_roi_count = UINT32(connector_info_.modes[index].num_roi);
  hw_panel_info_.right_roi_count = UINT32(connector_info_.modes[index].num_roi);
  hw_panel_info_.left_align = connector_info_.modes[index].xstart;
  hw_panel_info_.top_align = connector_info_.modes[index].ystart;
  hw_panel_info_.width_align = connector_info_.modes[index].walign;
  hw_panel_info_.height_align = connector_info_.modes[index].halign;
  hw_panel_info_.min_roi_width = connector_info_.modes[index].wmin;
  hw_panel_info_.min_roi_height = connector_info_.modes[index].hmin;
  hw_panel_info_.needs_roi_merge = connector_info_.modes[index].roi_merge;
  hw_panel_info_.transfer_time_us = connector_info_.modes[index].transfer_time_us;
  hw_panel_info_.transfer_time_us_min = (connector_info_.modes[index].transfer_time_us_min)
                                            ? connector_info_.modes[index].transfer_time_us_min
                                            : 1;
  hw_panel_info_.transfer_time_us_max = connector_info_.modes[index].transfer_time_us_max;
  hw_panel_info_.allowed_mode_switch = connector_info_.modes[index].allowed_mode_switch;
  hw_panel_info_.panel_mode_caps =
                 connector_info_.modes[index].sub_modes[sub_mode_index].panel_mode_caps;
  hw_panel_info_.dynamic_fps = connector_info_.dynamic_fps;
  hw_panel_info_.qsync_support = connector_info_.qsync_support;
  hw_panel_info_.has_cwb_crop = has_cwb_crop_;
  if (connector_info_.dms_type == sde_drm::DMSType::DMS_VID_SEAMLESS) {
    hw_panel_info_.dms_type = kDMSVIDSeamless;
  } else if (connector_info_.dms_type == sde_drm::DMSType::DMS_VID_NON_SEAMLESS) {
    hw_panel_info_.dms_type = kDMSVIDNonSeamless;
  }
  drmModeModeInfo current_mode = connector_info_.modes[current_mode_index_].mode;
  if (hw_panel_info_.dynamic_fps) {
    uint32_t min_fps = current_mode.vrefresh;
    uint32_t max_fps = current_mode.vrefresh;
    for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
      if ((current_mode.vdisplay == connector_info_.modes[mode_index].mode.vdisplay) &&
          (current_mode.hdisplay == connector_info_.modes[mode_index].mode.hdisplay)) {
        if (min_fps > connector_info_.modes[mode_index].mode.vrefresh)  {
          min_fps = connector_info_.modes[mode_index].mode.vrefresh;
        }
        if (max_fps < connector_info_.modes[mode_index].mode.vrefresh)  {
          max_fps = connector_info_.modes[mode_index].mode.vrefresh;
        }
      }
    }
    hw_panel_info_.min_fps = min_fps;
    hw_panel_info_.max_fps = max_fps;
  } else {
    hw_panel_info_.min_fps = current_mode.vrefresh;
    hw_panel_info_.max_fps = current_mode.vrefresh;
  }

  uint32_t transfer_time_us_min = hw_panel_info_.transfer_time_us;
  for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
    if ((current_mode.vdisplay == connector_info_.modes[mode_index].mode.vdisplay) &&
        (current_mode.hdisplay == connector_info_.modes[mode_index].mode.hdisplay)) {
      if (transfer_time_us_min > connector_info_.modes[mode_index].transfer_time_us) {
        transfer_time_us_min = connector_info_.modes[mode_index].transfer_time_us;
      }
    }
  }

  if (hw_panel_info_.transfer_time_us_min <= 1) {
    hw_panel_info_.transfer_time_us_min = transfer_time_us_min;
  }

  if (connector_info_.qsync_fps > 0) {
    // For command mode panel, driver will set connector property qsync_fps
    hw_panel_info_.qsync_fps = connector_info_.qsync_fps;
  } else {
    // if for video mode panel, qsync_fps is not set, take default min_fps value
    hw_panel_info_.qsync_fps = hw_panel_info_.min_fps;
  }

  hw_panel_info_.is_primary_panel = connector_info_.is_primary;
  hw_panel_info_.is_pluggable = 0;
  hw_panel_info_.hdr_enabled = connector_info_.panel_hdr_prop.hdr_enabled;
  // Convert the luminance values to cd/m^2 units.
  hw_panel_info_.peak_luminance = FLOAT(connector_info_.panel_hdr_prop.peak_brightness) / 10000.0f;
  hw_panel_info_.blackness_level = FLOAT(connector_info_.panel_hdr_prop.blackness_level) / 10000.0f;
  hw_panel_info_.average_luminance = hw_panel_info_.peak_luminance;
  hw_panel_info_.primaries.white_point[0] = connector_info_.panel_hdr_prop.display_primaries[0];
  hw_panel_info_.primaries.white_point[1] = connector_info_.panel_hdr_prop.display_primaries[1];
  hw_panel_info_.primaries.red[0] = connector_info_.panel_hdr_prop.display_primaries[2];
  hw_panel_info_.primaries.red[1] = connector_info_.panel_hdr_prop.display_primaries[3];
  hw_panel_info_.primaries.green[0] = connector_info_.panel_hdr_prop.display_primaries[4];
  hw_panel_info_.primaries.green[1] = connector_info_.panel_hdr_prop.display_primaries[5];
  hw_panel_info_.primaries.blue[0] = connector_info_.panel_hdr_prop.display_primaries[6];
  hw_panel_info_.primaries.blue[1] = connector_info_.panel_hdr_prop.display_primaries[7];
  hw_panel_info_.dyn_bitclk_support = connector_info_.dyn_bitclk_support;
  hw_panel_info_.dpu_ctl_op_sync = connector_info_.dpu_ctl_op_sync;

  // no supprt for 90 rotation only flips or 180 supported
  hw_panel_info_.panel_orientation.rotation = 0;
  hw_panel_info_.panel_orientation.flip_horizontal =
    (connector_info_.panel_orientation == DRMRotation::FLIP_H) ||
    (connector_info_.panel_orientation == DRMRotation::ROT_180);
  hw_panel_info_.panel_orientation.flip_vertical =
    (connector_info_.panel_orientation == DRMRotation::FLIP_V) ||
    (connector_info_.panel_orientation == DRMRotation::ROT_180);

  GetHWDisplayPortAndMode();
  GetHWPanelMaxBrightness();
  if (connector_info_.modes[current_mode_index_].cur_panel_mode & DRM_MODE_FLAG_CMD_MODE_PANEL) {
    hw_panel_info_.mode = kModeCommand;
  }

  if (connector_info_.modes[current_mode_index_].cur_panel_mode &
             DRM_MODE_FLAG_VID_MODE_PANEL) {
    hw_panel_info_.mode = kModeVideo;
  }

  hw_panel_info_.vhm_support = connector_info_.modes[current_mode_index_].vhm_support;

  DLOGI_IF(kTagDriverConfig, "%s, Panel Interface = %s, Panel Mode = %s, Is Primary = %d",
           device_name_, interface_str_.c_str(),
           hw_panel_info_.mode == kModeVideo ? "Video" : "Command",
           hw_panel_info_.is_primary_panel);
  DLOGI_IF(kTagDriverConfig, "Partial Update = %d, Dynamic FPS = %d, HDR Panel = %d QSync = %d",
           hw_panel_info_.partial_update, hw_panel_info_.dynamic_fps, hw_panel_info_.hdr_enabled,
           hw_panel_info_.qsync_support);
  DLOGI_IF(kTagDriverConfig, "Align: left = %d, width = %d, top = %d, height = %d",
           hw_panel_info_.left_align, hw_panel_info_.width_align, hw_panel_info_.top_align,
           hw_panel_info_.height_align);
  DLOGI_IF(kTagDriverConfig, "ROI: min_width = %d, min_height = %d, need_merge = %d",
           hw_panel_info_.min_roi_width, hw_panel_info_.min_roi_height,
           hw_panel_info_.needs_roi_merge);
  DLOGI_IF(kTagDriverConfig, "FPS: min = %d, max = %d", hw_panel_info_.min_fps,
           hw_panel_info_.max_fps);
  DLOGI_IF(kTagDriverConfig, "Left Split = %d, Right Split = %d",
           hw_panel_info_.split_info.left_split, hw_panel_info_.split_info.right_split);
  DLOGI_IF(kTagDriverConfig, "Mode Transfer time = %d us", hw_panel_info_.transfer_time_us);
  DLOGI_IF(kTagDriverConfig, "Panel Minimum Transfer time = %d us",
           hw_panel_info_.transfer_time_us_min);
  DLOGI_IF(kTagDriverConfig, "Panel Maximum Transfer time = %d us",
           hw_panel_info_.transfer_time_us_max);
  DLOGI_IF(kTagDriverConfig, "Dynamic Bit Clk Support = %d", hw_panel_info_.dyn_bitclk_support);
}

DisplayError HWDeviceDRM::GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                                       uint8_t *out_data) {
  *out_port = token_.hw_port;
  std::vector<uint8_t> &edid = connector_info_.edid;

  if (out_data == nullptr) {
    *out_data_size = (uint32_t)(edid.size());
    if (*out_data_size == 0) {
      DLOGE("EDID blob is empty, no data to return");
      return kErrorDriverData;
    }
  } else {
    *out_data_size = std::min(*out_data_size, (uint32_t)(edid.size()));
    memcpy(out_data, edid.data(), *out_data_size);
  }

  return kErrorNone;
}

void HWDeviceDRM::GetHWDisplayPortAndMode() {
  hw_panel_info_.port = kPortDefault;
  hw_panel_info_.mode =
      (connector_info_.panel_mode == sde_drm::DRMPanelMode::VIDEO) ? kModeVideo : kModeCommand;

  if (default_mode_) {
    return;
  }

  switch (connector_info_.type) {
    case DRM_MODE_CONNECTOR_DSI:
      hw_panel_info_.port = kPortDSI;
      interface_str_ = "DSI";
      break;
    case DRM_MODE_CONNECTOR_LVDS:
      hw_panel_info_.port = kPortLVDS;
      interface_str_ = "LVDS";
      break;
    case DRM_MODE_CONNECTOR_eDP:
      hw_panel_info_.port = kPortEDP;
      interface_str_ = "EDP";
      break;
    case DRM_MODE_CONNECTOR_TV:
    case DRM_MODE_CONNECTOR_HDMIA:
    case DRM_MODE_CONNECTOR_HDMIB:
      hw_panel_info_.port = kPortDTV;
      interface_str_ = "HDMI";
      break;
    case DRM_MODE_CONNECTOR_VIRTUAL:
      hw_panel_info_.port = kPortWriteBack;
      interface_str_ = "Virtual";
      break;
    case DRM_MODE_CONNECTOR_DisplayPort:
      hw_panel_info_.port = kPortDP;
      interface_str_ = "DisplayPort";
      break;
  }

  return;
}

DisplayError HWDeviceDRM::GetActiveConfig(uint32_t *active_config) {
  *active_config = current_mode_index_;
  return kErrorNone;
}

DisplayError HWDeviceDRM::GetNumDisplayAttributes(uint32_t *count) {
  *count = UINT32(display_attributes_.size());
  return kErrorNone;
}

DisplayError HWDeviceDRM::GetDisplayAttributes(uint32_t index,
                                               HWDisplayAttributes *display_attributes) {
  if (index >= display_attributes_.size()) {
    DLOGW("Index > display_attributes_.size(). Return.");
    return kErrorParameters;
  }
  *display_attributes = display_attributes_[index];
  return kErrorNone;
}

DisplayError HWDeviceDRM::GetHWPanelInfo(HWPanelInfo *panel_info) {
  *panel_info = hw_panel_info_;
  return kErrorNone;
}

void HWDeviceDRM::SetDisplaySwitchMode(uint32_t index) {
  if (current_mode_index_ == index && !first_cycle_) {
    DLOGI("Mode %d already set", index);
    return;
  }
  uint32_t mode_flag = 0;
  uint32_t curr_mode_flag = 0, switch_mode_flag = 0;
  sde_drm::DRMModeInfo to_set = connector_info_.modes[index];
  sde_drm::DRMModeInfo current_mode = connector_info_.modes[current_mode_index_];
  uint64_t target_bit_clk = connector_info_.modes[current_mode_index_].curr_bit_clk_rate;
  uint32_t target_compression = connector_info_.modes[current_mode_index_].curr_compression_mode;
  uint32_t switch_index  = 0;

  if (to_set.cur_panel_mode & DRM_MODE_FLAG_CMD_MODE_PANEL) {
    mode_flag = DRM_MODE_FLAG_CMD_MODE_PANEL;
    switch_mode_flag = DRM_MODE_FLAG_VID_MODE_PANEL;
  } else if (to_set.cur_panel_mode & DRM_MODE_FLAG_VID_MODE_PANEL) {
    mode_flag = DRM_MODE_FLAG_VID_MODE_PANEL;
    switch_mode_flag = DRM_MODE_FLAG_CMD_MODE_PANEL;
  }

  if (current_mode.cur_panel_mode & DRM_MODE_FLAG_CMD_MODE_PANEL) {
    curr_mode_flag = DRM_MODE_FLAG_CMD_MODE_PANEL;
  } else if (current_mode.cur_panel_mode & DRM_MODE_FLAG_VID_MODE_PANEL) {
    curr_mode_flag = DRM_MODE_FLAG_VID_MODE_PANEL;
  }

  if (curr_mode_flag != mode_flag) {
    panel_mode_changed_ = mode_flag;
  }

  for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
    if ((to_set.mode.vdisplay == connector_info_.modes[mode_index].mode.vdisplay) &&
        (to_set.mode.hdisplay == connector_info_.modes[mode_index].mode.hdisplay) &&
        (to_set.mode.vrefresh == connector_info_.modes[mode_index].mode.vrefresh) &&
        (mode_flag & connector_info_.modes[mode_index].cur_panel_mode)) {
      for (uint32_t submode_idx = 0; submode_idx <
           connector_info_.modes[mode_index].sub_modes.size(); submode_idx++) {
        sde_drm::DRMSubModeInfo sub_mode = connector_info_.modes[mode_index].sub_modes[submode_idx];
        if (sub_mode.panel_compression_mode == target_compression) {
          connector_info_.modes[mode_index].curr_submode_index = submode_idx;
          index = mode_index;
          to_set.curr_bit_clk_rate = GetSupportedBitClkRate(index, target_bit_clk);
          break;
        }
      }
      break;
    }
  }

  current_mode_index_ = index;

  switch_mode_valid_ = false;
  for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
    if ((to_set.mode.vdisplay == connector_info_.modes[mode_index].mode.vdisplay) &&
        (to_set.mode.hdisplay == connector_info_.modes[mode_index].mode.hdisplay) &&
        (to_set.mode.vrefresh == connector_info_.modes[mode_index].mode.vrefresh) &&
        (switch_mode_flag & connector_info_.modes[mode_index].cur_panel_mode)) {
      for (uint32_t submode_idx = 0; submode_idx <
           connector_info_.modes[mode_index].sub_modes.size(); submode_idx++) {
        sde_drm::DRMSubModeInfo sub_mode = connector_info_.modes[mode_index].sub_modes[submode_idx];
        if (sub_mode.panel_compression_mode == target_compression) {
          connector_info_.modes[mode_index].curr_submode_index = submode_idx;
          switch_index = mode_index;
          switch_mode_valid_ = true;
          break;
        }
      }
      break;
    }
  }

  if (switch_mode_valid_) {
    if (mode_flag & DRM_MODE_FLAG_VID_MODE_PANEL) {
      video_mode_index_ = current_mode_index_;
      cmd_mode_index_ = switch_index;
    } else {
      video_mode_index_ = switch_index;
      cmd_mode_index_ = current_mode_index_;
    }
  }
  if (current_mode.mode.hdisplay == to_set.mode.hdisplay &&
    current_mode.mode.vdisplay == to_set.mode.vdisplay) {
    seamless_mode_switch_ = true;
  }
}

DisplayError HWDeviceDRM::SetDisplayAttributes(uint32_t index) {
  if (index >= display_attributes_.size()) {
    DLOGE("Invalid mode index %d mode size %d", index, UINT32(display_attributes_.size()));
    return kErrorParameters;
  }

  SetDisplaySwitchMode(index);
  PopulateHWPanelInfo();
  UpdateMixerAttributes();

  DLOGI_IF(kTagDriverConfig,
      "Display %d-%d attributes[%d]: WxH: %dx%d, DPI: %fx%f, FPS: %d, "
      "LM_SPLIT: %d, V_BACK_PORCH: %d,"
      " V_FRONT_PORCH: %d, V_PULSE_WIDTH: %d, V_TOTAL: %d, H_TOTAL: %d, CLK: %dKHZ, "
      "TOPOLOGY: %d, PanelMode %s", display_id_, disp_type_,
      index, display_attributes_[index].x_pixels, display_attributes_[index].y_pixels,
      display_attributes_[index].x_dpi, display_attributes_[index].y_dpi,
      display_attributes_[index].fps, display_attributes_[index].is_device_split,
      display_attributes_[index].v_back_porch, display_attributes_[index].v_front_porch,
      display_attributes_[index].v_pulse_width, display_attributes_[index].v_total,
      display_attributes_[index].h_total, display_attributes_[index].clock_khz,
      display_attributes_[index].topology,
      (connector_info_.modes[index].cur_panel_mode & DRM_MODE_FLAG_VID_MODE_PANEL) ?
      "Video" : "Command");

  return kErrorNone;
}

DisplayError HWDeviceDRM::SetDisplayAttributes(const HWDisplayAttributes &display_attributes) {
  return kErrorNotSupported;
}

DisplayError HWDeviceDRM::GetConfigIndex(char *mode, uint32_t *index) {
  return kErrorNone;
}

DisplayError HWDeviceDRM::PowerOn(const HWQosData &qos_data, SyncPoints *sync_points) {
  SetQOSData(qos_data);

  if (tui_state_ != kTUIStateNone || pending_cwb_teardown_) {
    DLOGI("Request deferred TUI state %d pending cwb teardown %d", tui_state_,
          pending_cwb_teardown_);
    pending_power_state_ = kPowerStateOn;
    return kErrorDeferred;
  }

  int64_t release_fence_fd = -1;
  int64_t retire_fence_fd = -1;

  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ACTIVE, token_.crtc_id, 1);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POWER_MODE, token_.conn_id, DRMPowerMode::ON);
  drm_atomic_intf_->Perform(DRMOps::CRTC_GET_RELEASE_FENCE, token_.crtc_id, &release_fence_fd);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_GET_RETIRE_FENCE, token_.conn_id, &retire_fence_fd);
  if (enable_brightness_drm_prop_ && cached_brightness_level_ != -1) {
#ifdef SEC_GC_QC_DEBUG
    DLOGI("Setting brightness to level %d. display(%d)", cached_brightness_level_, display_id_);
#endif
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_BRIGHTNESS, token_.conn_id,
                              cached_brightness_level_);
    current_brightness_ = cached_brightness_level_;
    cached_brightness_level_ = -1;
  }

  // On the first boot up of the display, make the power call synchronous. This is only applicable
  // to pluggable displays. Check HWPeripheralDRM::PowerOn. For builtin first power call defered
  // and handled in commit(synchronous for first cycle).
  bool is_synchronous = first_cycle_;
  if (hw_panel_info_.dpu_ctl_op_sync) {
    if (core_id_ != 0) {
      is_synchronous = true;
    } else {
      is_synchronous = false;
    }
  }
  int ret = NullCommit(is_synchronous, true /* retain_planes */);
  if (ret) {
    DLOGE("Failed with error: %d", ret);
    return kErrorHardware;
  }

  sync_points->retire_fence = Fence::Create(INT(retire_fence_fd), "retire_power_on");
  sync_points->release_fence = Fence::Create(INT(release_fence_fd), "release_power_on");

  DLOGI("retire_fence_fd = %d release_fence_fd = %d", INT(retire_fence_fd), INT(release_fence_fd));
  struct sync_file_info *finfo = sync_file_info(retire_fence_fd);
  DLOGI("RETIRE fence: fd: %s name = %s", Fence::GetStr(sync_points->retire_fence).c_str(), finfo->name);

  DLOGD_IF(kTagDriverConfig, "RELEASE fence: fd: %d", INT(release_fence_fd));
  pending_power_state_ = kPowerStateNone;

  last_power_mode_ = DRMPowerMode::ON;

  return kErrorNone;
}

DisplayError HWDeviceDRM::PowerOff(bool teardown, SyncPoints *sync_points) {
  DTRACE_SCOPED();
  if (!drm_atomic_intf_) {
    DLOGE("DRM Atomic Interface is null!");
    return kErrorUndefined;
  }

  if (first_cycle_) {
    return kErrorNone;
  }

  if ((tui_state_ != kTUIStateNone && tui_state_ != kTUIStateEnd) || pending_cwb_teardown_) {
    DLOGI("Request deferred TUI state %d pending cwb teardown %d", tui_state_,
          pending_cwb_teardown_);
    pending_power_state_ = kPowerStateOff;
    return kErrorDeferred;
  }

#ifdef SEC_GC_CMN_FINGERPRINT_INDISPLAY
  // Fix P190712-05212 : Unsetting Fingerprint Mask is requred when LCD Off
  if(hw_panel_info_.is_primary_panel) {
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_FINGERPRINT_MASK, token_.conn_id, 0);
    DLOGI("Display:%d Setting Fingerprint Indisplay Layer property = %u",display_id_, 0);
  }
#endif

  ResetROI();
  ClearSolidfillStages();
  int64_t retire_fence_fd = -1;
  if (bpp_mode_changed_) {
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_BPP_MODE, token_.conn_id, bpp_mode_changed_);
  }
  drmModeModeInfo current_mode = connector_info_.modes[current_mode_index_].mode;
  if (!IsSeamlessTransition()) {
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_MODE, token_.crtc_id, &current_mode);
  }
  if (enable_brightness_drm_prop_ && cached_brightness_level_ != -1) {
#ifdef SEC_GC_QC_DEBUG
    DLOGI("Setting brightness to level %d. display(%d)", cached_brightness_level_, display_id_);
#endif
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_BRIGHTNESS, token_.conn_id,
                              cached_brightness_level_);
    current_brightness_ = cached_brightness_level_;
    cached_brightness_level_ = -1;
  }
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POWER_MODE, token_.conn_id, DRMPowerMode::OFF);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ACTIVE, token_.crtc_id, 0);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_GET_RETIRE_FENCE, token_.conn_id, &retire_fence_fd);

  if (cwb_config_[core_id_].enabled) {
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, cwb_config_[core_id_].token.conn_id, 0);
    DLOGI("Tearing down the CWB topology");
  }

  bool is_synchronous = false;
  if (hw_panel_info_.dpu_ctl_op_sync) {
    is_synchronous = true;
  }
  int ret = NullCommit(is_synchronous, false /* retain_planes */);
  if (ret) {
    DLOGE(
        "Failed with error: %d, dynamic_fps=%d, seamless_mode_switch_=%d, vrefresh_=%d,"
        "panel_mode_changed_=%d bit_clk_rate_=%d bpp_mode_changed_=%d",
        ret, hw_panel_info_.dynamic_fps, seamless_mode_switch_, vrefresh_, panel_mode_changed_,
        bit_clk_rate_, bpp_mode_changed_);
    bpp_mode_changed_ = 0;
    return kErrorHardware;
  }

  if (cwb_config_[core_id_].enabled) {
    FlushConcurrentWriteback();
  }

  if (bpp_mode_changed_) {
    sde_drm::DRMModeInfo current_mode = connector_info_.modes[current_mode_index_];
    for (uint32_t submode_idx = 0; submode_idx < current_mode.sub_modes.size(); submode_idx++) {
      if (bpp_mode_changed_ == current_mode.sub_modes[submode_idx].bpp_mode) {
        connector_info_.modes[current_mode_index_].curr_submode_index = submode_idx;
        connector_info_.modes[current_mode_index_].curr_bpp_mode = bpp_mode_changed_;
      }
    }
    bpp_mode_changed_ = 0;
  }

  sync_points->retire_fence = Fence::Create(INT(retire_fence_fd), "retire_power_off");
  pending_power_state_ = kPowerStateNone;

  last_power_mode_ = DRMPowerMode::OFF;

  return kErrorNone;
}

DisplayError HWDeviceDRM::Doze(const HWQosData &qos_data, SyncPoints *sync_points) {
  DTRACE_SCOPED();

  if (first_cycle_ || tui_state_ != kTUIStateNone || last_power_mode_ != DRMPowerMode::OFF) {
    pending_power_state_ = kPowerStateDoze;
    return kErrorDeferred;
  }

  SetQOSData(qos_data);

  int64_t release_fence_fd = -1;
  int64_t retire_fence_fd = -1;

  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, token_.conn_id, token_.crtc_id);
  drmModeModeInfo current_mode = connector_info_.modes[current_mode_index_].mode;
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_MODE, token_.crtc_id, &current_mode);

  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ACTIVE, token_.crtc_id, 1);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POWER_MODE, token_.conn_id, DRMPowerMode::DOZE);
  drm_atomic_intf_->Perform(DRMOps::CRTC_GET_RELEASE_FENCE, token_.crtc_id, &release_fence_fd);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_GET_RETIRE_FENCE, token_.conn_id, &retire_fence_fd);
  if (enable_brightness_drm_prop_ && cached_brightness_level_ != -1) {
#ifdef SEC_GC_QC_DEBUG
    DLOGI("Setting brightness to level %d. display(%d)", cached_brightness_level_, display_id_);
#endif
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_BRIGHTNESS, token_.conn_id,
                              cached_brightness_level_);
    current_brightness_ = cached_brightness_level_;
    cached_brightness_level_ = -1;
  }

  bool is_synchronous = false;
  if (hw_panel_info_.dpu_ctl_op_sync && core_id_ != 0) {
    is_synchronous = true;
  }
  int ret = NullCommit(is_synchronous, true /* retain_planes */);
  if (ret) {
    DLOGE("Failed with error: %d", ret);
    return kErrorHardware;
  }

  sync_points->retire_fence = Fence::Create(INT(retire_fence_fd), "retire_doze");
  sync_points->release_fence = Fence::Create(release_fence_fd, "release_doze");
  DLOGD_IF(kTagDriverConfig, "RELEASE fence: fd: %d", INT(release_fence_fd));

  last_power_mode_ = DRMPowerMode::DOZE;

  return kErrorNone;
}

DisplayError HWDeviceDRM::DozeSuspend(const HWQosData &qos_data, SyncPoints *sync_points) {
  DTRACE_SCOPED();

  if (tui_state_ != kTUIStateNone && tui_state_ != kTUIStateEnd) {
    pending_power_state_ = kPowerStateDozeSuspend;
    return kErrorDeferred;
  }

  SetQOSData(qos_data);

  int64_t release_fence_fd = -1;
  int64_t retire_fence_fd = -1;

  if (first_cycle_) {
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, token_.conn_id, token_.crtc_id);
    drmModeModeInfo current_mode = connector_info_.modes[current_mode_index_].mode;
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_MODE, token_.crtc_id, &current_mode);
  }
  if (enable_brightness_drm_prop_ && cached_brightness_level_ != -1) {
#ifdef SEC_GC_QC_DEBUG
    DLOGI("Setting brightness to level %d. display(%d)", cached_brightness_level_, display_id_);
#endif
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_BRIGHTNESS, token_.conn_id,
                              cached_brightness_level_);
    current_brightness_ = cached_brightness_level_;
    cached_brightness_level_ = -1;
  }
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ACTIVE, token_.crtc_id, 1);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POWER_MODE, token_.conn_id,
                            DRMPowerMode::DOZE_SUSPEND);
  drm_atomic_intf_->Perform(DRMOps::CRTC_GET_RELEASE_FENCE, token_.crtc_id, &release_fence_fd);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_GET_RETIRE_FENCE, token_.conn_id, &retire_fence_fd);

  bool is_synchronous = false;
  if (hw_panel_info_.dpu_ctl_op_sync && core_id_ != 0) {
    is_synchronous = true;
  }
  int ret = NullCommit(is_synchronous, true /* retain_planes */);
  if (ret) {
    DLOGE("Failed with error: %d", ret);
    return kErrorHardware;
  }

  sync_points->retire_fence = Fence::Create(INT(retire_fence_fd), "retire_doze_suspend");
  sync_points->release_fence = Fence::Create(release_fence_fd, "release_doze_suspend");
  DLOGD_IF(kTagDriverConfig, "RELEASE fence: fd: %d", INT(release_fence_fd));

  pending_power_state_ = kPowerStateNone;

  last_power_mode_ = DRMPowerMode::DOZE_SUSPEND;

  return kErrorNone;
}

void HWDeviceDRM::SetQOSData(const HWQosData &qos_data) {
  if (!qos_data.valid) {
    return;
  }
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_CORE_CLK, token_.crtc_id, qos_data.clock_hz);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_CORE_AB, token_.crtc_id, qos_data.core_ab_bps);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_CORE_IB, token_.crtc_id, qos_data.core_ib_bps);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_LLCC_AB, token_.crtc_id, qos_data.llcc_ab_bps);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_LLCC_IB, token_.crtc_id, qos_data.llcc_ib_bps);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_DRAM_AB, token_.crtc_id, qos_data.dram_ab_bps);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_DRAM_IB, token_.crtc_id, qos_data.dram_ib_bps);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ROT_PREFILL_BW, token_.crtc_id,
                            qos_data.rot_prefill_bw_bps);
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ROT_CLK, token_.crtc_id, qos_data.rot_clock_hz);

  if (hw_resource_.has_cesta) {
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_UBWC_CLK, token_.crtc_id,
                              qos_data.ubwc_clock_hz);
  }
}

DisplayError HWDeviceDRM::Standby(SyncPoints *sync_points) {
  return kErrorNone;
}

void HWDeviceDRM::SetupAtomic(Fence::ScopedRef &scoped_ref, HWLayersInfo *hw_layers_info,
                              bool validate, int64_t *release_fence_fd, int64_t *retire_fence_fd) {
  if (default_mode_) {
    return;
  }

  DTRACE_SCOPED();
  uint32_t hw_layer_count = UINT32(hw_layers_info->hw_layers.size());
  HWQosData &qos_data = hw_layers_info->qos_data;
  DRMSecurityLevel crtc_security_level = DRMSecurityLevel::SECURE_NON_SECURE;
  uint32_t index = current_mode_index_;
  sde_drm::DRMModeInfo current_mode = connector_info_.modes[index];

#ifdef SEC_GC_CMN_FINGERPRINT_INDISPLAY
  if(hw_panel_info_.is_primary_panel) {
    // We assume that the fingerprint exist on the primary display only
    // Check all layers include marked as GPU composition
    static uint32_t mIndisplayLayer = 0;
    uint32_t isIndisplayLayer = hw_layers_info->fingerprint_present;

    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_FINGERPRINT_MASK, token_.conn_id, isIndisplayLayer);
    if(mIndisplayLayer != isIndisplayLayer){
      mIndisplayLayer = isIndisplayLayer;
      DLOGI("Display:%d Setting Fingerprint Indisplay Layer property = %u",display_id_, mIndisplayLayer);
    }
  }
#endif

  solid_fills_.clear();
  noise_cfg_ = {};
  bool resource_update = hw_layers_info->common_info->updates_mask.test(kUpdateResources);
  bool buffer_update = hw_layers_info->common_info->updates_mask.test(kSwapBuffers);
  bool fb_update = hw_layers_info->common_info->updates_mask.test(kUpdateFBObject);
  bool update_config = resource_update || buffer_update || tui_state_ == kTUIStateEnd ||
                       hw_layers_info->common_info->flags.geometry_changed || fb_update;
  bool update_luts = hw_layers_info->common_info->updates_mask.test(kUpdateLuts);

  if (hw_panel_info_.partial_update && update_config) {
    if (IsFullFrameUpdate(*hw_layers_info)) {
      ResetROI();
    } else {
      const int kNumMaxROIs = 4;
      DRMRect crtc_rects[kNumMaxROIs] = {{0, 0, mixer_attributes_.width, mixer_attributes_.height}};
      DRMRect conn_rects[kNumMaxROIs] = {{0, 0, display_attributes_[index].x_pixels,
                                          display_attributes_[index].y_pixels}};
      DRMRect spr_rects[kNumMaxROIs] = {{0, 0, mixer_attributes_.width, mixer_attributes_.height}};
      DestScaleInfoMap &dest_scale_info_map = hw_layers_info->dest_scale_info_map;

      if (dest_scale_info_map.size() && hw_layers_info->left_frame_roi.size() != 1) {
        DLOGE("left_frame_roi size %d, only 1 ROI supported in PU+DS case",
              hw_layers_info->left_frame_roi.size());
      }

      for (uint32_t i = 0; i < hw_layers_info->left_frame_roi.size(); i++) {
        auto &roi = hw_layers_info->left_frame_roi.at(i);
        // TODO(user): In multi PU, stitch ROIs vertically adjacent and upate plane destination

        LayerRect panel_roi = {};
        if (dest_scale_info_map.size() && dest_scale_info_map[0]->scale_data.enable.scale) {
          for (uint32_t i = 0; i < dest_scale_info_map.size(); i++) {
            panel_roi = Union(panel_roi, dest_scale_info_map[i]->panel_roi);
          }
        } else {
          panel_roi = roi;
          panel_roi.top += FLOAT(hw_layers_info->common_info->spr_overfetch_lines.top);
          panel_roi.bottom -= FLOAT(hw_layers_info->common_info->spr_overfetch_lines.bottom);
        }

        crtc_rects[i].left = UINT32(roi.left);
        crtc_rects[i].right = UINT32(roi.right);
        crtc_rects[i].top = UINT32(roi.top);
        crtc_rects[i].bottom = UINT32(roi.bottom);
        conn_rects[i].left = UINT32(panel_roi.left);
        conn_rects[i].right = UINT32(panel_roi.right);
        conn_rects[i].top = UINT32(panel_roi.top);
        conn_rects[i].bottom = UINT32(panel_roi.bottom);
        spr_rects[i].left = UINT32(roi.left);
        spr_rects[i].right = UINT32(roi.right);
        spr_rects[i].top = UINT32(roi.top +
                           FLOAT(hw_layers_info->common_info->spr_overfetch_lines.top));
        spr_rects[i].bottom =
            UINT32(roi.bottom - FLOAT(hw_layers_info->common_info->spr_overfetch_lines.bottom));
      }

      uint32_t num_rects = std::max(1u, UINT32(hw_layers_info->left_frame_roi.size()));
      drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ROI, token_.crtc_id, num_rects, crtc_rects,
                                spr_rects);
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_ROI, token_.conn_id, num_rects, conn_rects);
    }
  } else if (!hw_panel_info_.partial_update &&
             (current_mode.cur_panel_mode & DRM_MODE_FLAG_CMD_MODE_PANEL) && update_config) {
    if (!IsFullFrameUpdate(*hw_layers_info)) {
      DLOGW("Expected full frame ROI");
    }
    ResetROI();
  }

#ifdef TRUSTED_VM
  if (first_cycle_) {
    drm_atomic_intf_->Perform(sde_drm::DRMOps::RESET_PANEL_FEATURES, 0 /* argument is not used */);
    // On TUI start, need to clear the SSPP luts.
    drm_atomic_intf_->Perform(sde_drm::DRMOps::PLANES_RESET_LUT, token_.crtc_id);
  }
#endif

  if (reset_planes_luts_) {
    // Used in 1 cases:
    // 1. Since driver doesnt clear the SSPP luts during the adb shell stop/start, clear once
    drm_atomic_intf_->Perform(sde_drm::DRMOps::PLANES_RESET_LUT, token_.crtc_id);
  }

  if (enable_brightness_drm_prop_ && cached_brightness_level_ != -1) {
#ifdef SEC_GC_QC_DEBUG
    DLOGI("Setting brightness to level %d. display(%d)", cached_brightness_level_, display_id_);
#endif
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_BRIGHTNESS, token_.conn_id,
                              cached_brightness_level_);
    current_brightness_ = cached_brightness_level_;
    cached_brightness_level_ = -1;
  }

  for (uint32_t i = 0; i < hw_layer_count; i++) {
    Layer &layer = hw_layers_info->hw_layers.at(i);
    LayerBuffer *input_buffer = &layer.input_buffer;
    HWPipeInfo *left_pipe = &hw_layers_info->config[i].left_pipe;
    HWPipeInfo *right_pipe = &hw_layers_info->config[i].right_pipe;
    HWLayerConfig &layer_config = hw_layers_info->config[i];
    HWRotatorSession *hw_rotator_session = &layer_config.hw_rotator_session;

    std::vector<HWPipeInfo *> pipe_info_vec;
    if (left_pipe->valid) {
      pipe_info_vec.push_back(left_pipe);
    }
    if (right_pipe->valid) {
      pipe_info_vec.push_back(right_pipe);
    }
    for (int j = 0; j < hw_layers_info->config[i].tunnel_pipes.size(); j++) {
      pipe_info_vec.push_back(&hw_layers_info->config[i].tunnel_pipes[j]);
    }

    if (hw_layers_info->config[i].use_solidfill_stage) {
      hw_layers_info->config[i].hw_solidfill_stage.solid_fill_info = layer.solid_fill_info;
      AddSolidfillStage(hw_layers_info->config[i].hw_solidfill_stage, layer.plane_alpha);
      continue;
    }

    if (layer_config.hw_noise_layer_cfg.enable) {
      SetNoiseLayerConfig(layer_config.hw_noise_layer_cfg);
      continue;
    }

    for (uint32_t count = 0; count < pipe_info_vec.size(); count++) {
      HWPipeInfo *pipe_info = pipe_info_vec[count];
      HWRotateInfo *hw_rotate_info = (count <= 1) ?
                                     &hw_rotator_session->hw_rotate_info[count] : NULL;

      if (hw_rotator_session->mode == kRotatorOffline && hw_rotate_info && hw_rotate_info->valid) {
        input_buffer = &hw_rotator_session->output_buffer;
      }

      std::vector<uint32_t> fb_id = {};
      registry_.GetFbId(&layer, input_buffer->handle_id, &fb_id);

      if (pipe_info->valid && fb_id[pipe_info->cac_color]) {
        uint32_t pipe_id = pipe_info->pipe_id;

        if (update_config) {
          uint32_t fg_alpha = layer.plane_alpha;
          uint32_t bg_alpha = 0xffff - layer.plane_alpha;

          if (pipe_info->cac_mode && (pipe_info->cac_mode != kModeLoopbackUnpack)) {
            fg_alpha = bg_alpha = 0xffff;
          }

          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_ALPHA, pipe_id, fg_alpha);

          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_BG_ALPHA, pipe_id, bg_alpha);

          if ((hw_resource_.cac_version == kCacVersion2) ||
              ((hw_resource_.cac_version == kCacVersionLoopback))) {
            DRMCacMode target_mode = DRMCacMode::CAC_MODE_DISABLED;
            SetCacType(pipe_info->cac_mode, &target_mode);
            drm_atomic_intf_->Perform(DRMOps::PLANE_SET_CAC_TYPE, pipe_id, target_mode);
          }

          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_ZORDER, pipe_id, pipe_info->z_order);

          sde_drm::DRMFp16CscType fp16_csc_type = sde_drm::DRMFp16CscType::kFP16CscTypeMax;
          int fp16_igc_en = 0;
          int fp16_unmult_en = 0;
          drm_msm_fp16_gc fp16_gc_config = {.flags = 0, .mode = FP16_GC_MODE_INVALID};
          SelectFp16Config(layer.input_buffer, &fp16_igc_en, &fp16_unmult_en, &fp16_csc_type,
                           &fp16_gc_config, layer.blending);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_FP16_CSC_CONFIG, pipe_id, fp16_csc_type);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_FP16_IGC_CONFIG, pipe_id, fp16_igc_en);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_FP16_GC_CONFIG, pipe_id, &fp16_gc_config);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_FP16_UNMULT_CONFIG, pipe_id, fp16_unmult_en);

          // Account for PMA block activation directly at translation time to preserve layer
          // blending definition and avoid issues when a layer structure is reused.
          DRMBlendType blending = DRMBlendType::UNDEFINED;
          LayerBlending layer_blend = layer.blending;
          if (layer_blend == kBlendingPremultiplied) {
            // If blending type is premultiplied alpha, prevent performing alpha unmultiply
            // multiple times for INV PMA / FP16 / UCSC blocks
            if (fp16_unmult_en) {
              layer_blend = kBlendingCoverage;
              pipe_info->inverse_pma_info.inverse_pma = false;
              pipe_info->inverse_pma_info.op = kReset;
              if (pipe_info->ucsc_config.unmult_en) {
                pipe_info->ucsc_write_op[kUcscUnmult] = kReset;
                pipe_info->ucsc_config.unmult_en = 0;
              }
              DLOGI_IF(kTagDriverConfig, "PMA handled by FP16 UNMULT block - Pipe id: %u", pipe_id);
            } else if (pipe_info->ucsc_config.unmult_en) {
              layer_blend = kBlendingCoverage;
              if (pipe_info->inverse_pma_info.inverse_pma && (pipe_info->lut_info.size() > 0)) {
                // Inverse PMA set along with 3d gamut takes precedence over UCSC
                pipe_info->ucsc_write_op[kUcscUnmult] = kReset;
                pipe_info->ucsc_config.unmult_en = 0;
                DLOGI_IF(kTagDriverConfig, "PMA handled by Inverse PMA block - Pipe id: %u",
                         pipe_id);
              } else {
                // Inverse PMA set by color transform needs to be reset as it is handled by UCSC
                pipe_info->inverse_pma_info.inverse_pma = false;
                pipe_info->inverse_pma_info.op = kReset;
                DLOGI_IF(kTagDriverConfig, "PMA handled by UCSC UNMULT block - Pipe id: %u",
                         pipe_id);
              }
            } else if (pipe_info->inverse_pma_info.inverse_pma) {
              layer_blend = kBlendingCoverage;
              DLOGI_IF(kTagDriverConfig, "PMA handled by Inverse PMA block - Pipe id: %u", pipe_id);
            }
          }
          if (pipe_info->cac_mode) {
            layer_blend = pipe_info->cac_blend_type;
          }
          SetBlending(layer_blend, &blending);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_BLEND_TYPE, pipe_id, blending);

          DRMRect src = {};
          SetRect(pipe_info->src_roi, &src);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_SRC_RECT, pipe_id, src);

          if (IsValid(pipe_info->ext_src_roi)) {
            DRMRect src_ext = {};
            SetRect(pipe_info->ext_src_roi, &src_ext);
            drm_atomic_intf_->Perform(DRMOps::PLANE_SET_SRC_RECT_EXT, pipe_id, src_ext);
          }

          DRMRect dst = {};
          SetRect(pipe_info->dst_roi, &dst);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_DST_RECT, pipe_id, dst);

          if (IsValid(pipe_info->ext_dst_roi)) {
            DRMRect dst_ext = {};
            SetRect(pipe_info->ext_dst_roi, &dst_ext);
            drm_atomic_intf_->Perform(DRMOps::PLANE_SET_DST_RECT_EXT, pipe_id, dst_ext);
          }

          if (IsValid(pipe_info->img_roi)) {
            DRMRect img_roi = {};
            SetRect(pipe_info->img_roi, &img_roi);
            drm_atomic_intf_->Perform(DRMOps::PLANE_SET_IMG_SIZE_RECT, pipe_id, img_roi);
          }

          DRMRect excl = {};
          SetRect(pipe_info->excl_rect, &excl);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_EXCL_RECT, pipe_id, excl);

          uint32_t rot_bit_mask = 0;
          SetRotation(layer.transform, layer_config, &rot_bit_mask);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_ROTATION, pipe_id, rot_bit_mask);

          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_H_DECIMATION, pipe_id,
                                    pipe_info->horizontal_decimation);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_V_DECIMATION, pipe_id,
                                    pipe_info->vertical_decimation);

          DRMSecureMode fb_secure_mode;
          DRMSecurityLevel security_level;
          SetSecureConfig(layer.input_buffer, &fb_secure_mode, &security_level);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_FB_SECURE_MODE, pipe_id, fb_secure_mode);
          if (security_level > crtc_security_level) {
            crtc_security_level = security_level;
          }

          uint32_t config = 0;
          SetSrcConfig(layer.input_buffer, hw_rotator_session->mode, &config);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_SRC_CONFIG, pipe_id, config);;

          if (hw_scale_) {
            SDEScaler scaler_output = {};
            hw_scale_->SetScaler(pipe_info->scale_data, &scaler_output);
            // TODO(user): Remove qseed3 and add version check, then send appropriate scaler object
            if (hw_resource_.has_qseed3) {
              drm_atomic_intf_->Perform(DRMOps::PLANE_SET_SCALER_CONFIG, pipe_id,
                                        reinterpret_cast<uint64_t>(&scaler_output.scaler_v2));
            }
          }

          DRMCscType csc_type = DRMCscType::kCscTypeMax;
          SelectCscType(layer.input_buffer, &csc_type);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_CSC_CONFIG, pipe_id, &csc_type);

          DRMMultiRectMode multirect_mode;
          SetMultiRectMode(pipe_info->flags, &multirect_mode);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_MULTIRECT_MODE, pipe_id, multirect_mode);

          SetSsppTonemapFeatures(pipe_info);
        } else if (update_luts) {
          if (force_tonemapping_) {
            sde_drm::DRMFp16CscType fp16_csc_type = sde_drm::DRMFp16CscType::kFP16CscTypeMax;
            int fp16_igc_en = 0;
            int fp16_unmult_en = 0;
            drm_msm_fp16_gc fp16_gc_config = {.flags = 0, .mode = FP16_GC_MODE_INVALID};
            SelectFp16Config(layer.input_buffer, &fp16_igc_en, &fp16_unmult_en, &fp16_csc_type,
                             &fp16_gc_config, layer.blending);

            // Account for PMA block activation directly at translation time to preserve layer
            // blending definition and avoid issues when a layer structure is reused.
            DRMBlendType blending = DRMBlendType::UNDEFINED;
            LayerBlending layer_blend = layer.blending;
            if (layer_blend == kBlendingPremultiplied) {
              // If blending type is premultiplied alpha, prevent performing alpha unmultiply
              // multiple times for INV PMA / FP16 / UCSC blocks
              if (fp16_unmult_en) {
                layer_blend = kBlendingCoverage;
                pipe_info->inverse_pma_info.inverse_pma = false;
                pipe_info->inverse_pma_info.op = kReset;
                if (pipe_info->ucsc_config.unmult_en) {
                  pipe_info->ucsc_write_op[kUcscUnmult] = kReset;
                  pipe_info->ucsc_config.unmult_en = 0;
                }
                DLOGI_IF(kTagDriverConfig, "PMA handled by FP16 UNMULT block - Pipe id: %u",
                         pipe_id);
              } else if (pipe_info->ucsc_config.unmult_en) {
                layer_blend = kBlendingCoverage;
                if (pipe_info->inverse_pma_info.inverse_pma && (pipe_info->lut_info.size() > 0)) {
                  // Inverse PMA set along with 3d gamut takes precedence over UCSC
                  pipe_info->ucsc_write_op[kUcscUnmult] = kReset;
                  pipe_info->ucsc_config.unmult_en = 0;
                  DLOGI_IF(kTagDriverConfig, "PMA handled by Inverse PMA block - Pipe id: %u",
                           pipe_id);
                } else {
                  // Inverse PMA set by color transform needs to be reset as it is handled by UCSC
                  pipe_info->inverse_pma_info.inverse_pma = false;
                  pipe_info->inverse_pma_info.op = kReset;
                  DLOGI_IF(kTagDriverConfig, "PMA handled by UCSC UNMULT block - Pipe id: %u",
                           pipe_id);
                }
              } else if (pipe_info->inverse_pma_info.inverse_pma) {
                layer_blend = kBlendingCoverage;
                DLOGI_IF(kTagDriverConfig, "PMA handled by Inverse PMA block - Pipe id: %u",
                         pipe_id);
              }
            }
            SetBlending(layer_blend, &blending);
            drm_atomic_intf_->Perform(DRMOps::PLANE_SET_BLEND_TYPE, pipe_id, blending);
          }

          SetSsppTonemapFeatures(pipe_info);
        }

        drm_atomic_intf_->Perform(DRMOps::PLANE_SET_FB_ID, pipe_id, fb_id[pipe_info->cac_color]);
        drm_atomic_intf_->Perform(DRMOps::PLANE_SET_CRTC, pipe_id, token_.crtc_id);

        if (!validate && input_buffer->acquire_fence) {
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_INPUT_FENCE, pipe_id,
                                    scoped_ref.Get(input_buffer->acquire_fence));
        }
      }
    }
  }

  if (update_config) {
    SetSolidfillStages();
    ApplyNoiseLayerConfig();
    SetQOSData(qos_data);
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_SECURITY_LEVEL, token_.crtc_id, crtc_security_level);
  } else if (hw_layers_info->common_info->updates_mask.test(kChangeCwbConfig)) {
    SetQOSData(qos_data);
  }

  if (hw_layers_info->common_info->hw_avr_info.update.test(kUpdateAVRModeFlag)) {
    sde_drm::DRMQsyncMode mode = sde_drm::DRMQsyncMode::NONE;
    if (hw_layers_info->common_info->hw_avr_info.mode == kContinuousMode) {
      mode = sde_drm::DRMQsyncMode::CONTINUOUS;
    } else if (hw_layers_info->common_info->hw_avr_info.mode == kOneShotMode) {
      mode = sde_drm::DRMQsyncMode::ONESHOT;
    }
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_QSYNC_MODE, token_.conn_id, mode);
  }

  if (hw_layers_info->common_info->hw_avr_info.update.test(kUpdateAVRStepFlag)) {
    sde_drm::DRMAvrStepState state = hw_layers_info->common_info->hw_avr_info.step_enabled
                                         ? sde_drm::DRMAvrStepState::ENABLE
                                         : sde_drm::DRMAvrStepState::DISABLE;
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_AVR_STEP_STATE, token_.conn_id, state);
  }

  // dpps commit feature ops doesn't use the obj id, set it as -1
  drm_atomic_intf_->Perform(DRMOps::DPPS_COMMIT_FEATURE, -1, ((validate) ? 1 : 0));
  if (!validate) {
    drm_atomic_intf_->Perform(DRMOps::COMMIT_PANEL_FEATURES, 0 /* argument is not used */);
  }

  if (reset_output_fence_offset_ && !validate) {
    // Change back the fence_offset
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_OUTPUT_FENCE_OFFSET, token_.crtc_id, 0);
    reset_output_fence_offset_ = false;
  }

  // Set panel mode
  if (panel_mode_changed_ & DRM_MODE_FLAG_VID_MODE_PANEL) {
    if (!validate) {
      // Switch to video mode, corresponding change the fence_offset
      DLOGI("set property: switch to video mode");
      drm_atomic_intf_->Perform(DRMOps::CRTC_SET_OUTPUT_FENCE_OFFSET, token_.crtc_id, 1);
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_PANEL_MODE, token_.conn_id,
                                panel_mode_changed_);
    }
    ResetROI();
  }

  if (panel_mode_changed_ & DRM_MODE_FLAG_CMD_MODE_PANEL && !validate) {
    // Switch to command mode
    DLOGI("set property: switch to command mode");
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_PANEL_MODE, token_.conn_id,
                              panel_mode_changed_);
  }

  if (panel_compression_changed_ && !validate) {
    DLOGI("set property: change the compression mode");
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_DSC_MODE, token_.conn_id,
                              panel_compression_changed_);
  }

  if (!validate && release_fence_fd && retire_fence_fd) {
    drm_atomic_intf_->Perform(DRMOps::CRTC_GET_RELEASE_FENCE, token_.crtc_id, release_fence_fd);
    // Set retire fence offset.
    uint32_t offset = hw_layers_info->retire_fence_offset;
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_RETIRE_FENCE_OFFSET, token_.conn_id, offset);
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_GET_RETIRE_FENCE, token_.conn_id, retire_fence_fd);
  }

  DLOGI_IF(kTagDriverConfig, "%s::%s System Clock=%d Hz, Core: AB=%f KBps, IB=%f Bps, " \
           "LLCC: AB=%f Bps, IB=%f Bps, DRAM AB=%f Bps, IB=%f Bps, "\
           "Rot: Bw=%f Bps, Clock=%d Hz", validate ? "Validate" : "Commit", device_name_,
           qos_data.clock_hz, qos_data.core_ab_bps / 1000.f, qos_data.core_ib_bps / 1000.f,
           qos_data.llcc_ab_bps / 1000.f, qos_data.llcc_ib_bps / 1000.f,
           qos_data.dram_ab_bps / 1000.f, qos_data.dram_ib_bps / 1000.f,
           qos_data.rot_prefill_bw_bps / 1000.f, qos_data.rot_clock_hz);

  // Set refresh rate
  if (vrefresh_) {
    for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
      if ((current_mode.mode.vdisplay == connector_info_.modes[mode_index].mode.vdisplay) &&
          (current_mode.mode.hdisplay == connector_info_.modes[mode_index].mode.hdisplay) &&
          (current_mode.cur_panel_mode == connector_info_.modes[mode_index].cur_panel_mode) &&
          (vrefresh_ == connector_info_.modes[mode_index].mode.vrefresh)) {
        current_mode = connector_info_.modes[mode_index];
        break;
      }
    }
  }

  if (bit_clk_rate_) {
    // Set the new bit clk rate
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_DYN_BIT_CLK, token_.conn_id, bit_clk_rate_);
  }

  if (transfer_time_updated_) {
    // Skip updating the driver if driver is the one providing new transfer time
    if (connector_info_.modes[current_mode_index_].transfer_time_us != transfer_time_updated_) {
      UpdateTransferTime(transfer_time_updated_);
    } else {
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_TRANSFER_TIME, token_.conn_id,
                                transfer_time_updated_);
    }
  }

  if (first_cycle_) {
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_TOPOLOGY_CONTROL, token_.conn_id,
                              topology_control_);
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ACTIVE, token_.crtc_id, 1);
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, token_.conn_id, token_.crtc_id);
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POWER_MODE, token_.conn_id, DRMPowerMode::ON);
#ifdef TRUSTED_VM
    drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_VM_REQ_STATE, token_.crtc_id,
                              sde_drm::DRMVMRequestState::ACQUIRE);
#endif
    last_power_mode_ = DRMPowerMode::ON;
  } else if (pending_power_state_ != kPowerStateNone && !validate) {
    DRMPowerMode power_mode;
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ACTIVE, token_.crtc_id, 1);
    if (GetDRMPowerMode(pending_power_state_, &power_mode) == kErrorNone) {
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POWER_MODE, token_.conn_id, power_mode);
      last_power_mode_ = power_mode;
    }
  }

  // Set CRTC mode, only if display config changes
#ifdef SEC_GC_QC_DEBUG
  static uint32_t pre_vrefresh = 0;
  bool need_update = pre_vrefresh != current_mode.mode.vrefresh;

  if (first_cycle_ || vrefresh_ || update_mode_ || need_update) {
    DLOGI("update ActiveConfig. pre_vrefresh : %d, current_vrefresh: %d, update_mode_: %d", pre_vrefresh, current_mode.mode.vrefresh, update_mode_);
    pre_vrefresh = current_mode.mode.vrefresh;
#else
  if (first_cycle_ || vrefresh_ || update_mode_) {
#endif
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_MODE, token_.crtc_id, &current_mode.mode);
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_DSC_MODE, token_.conn_id,
                              current_mode.curr_compression_mode);
  }

  if (!validate && (hw_layers_info->common_info->set_idle_time_ms >= 0)) {
    DLOGI_IF(kTagDriverConfig, "Setting idle timeout to = %d ms",
             hw_layers_info->common_info->set_idle_time_ms);
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_IDLE_TIMEOUT, token_.crtc_id,
                              hw_layers_info->common_info->set_idle_time_ms);
  }

  if (hw_panel_info_.mode == kModeCommand) {
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_AUTOREFRESH, token_.conn_id, autorefresh_);
  }
}

void HWDeviceDRM::SetNoiseLayerConfig(const NoiseLayerConfig &noise_config) {
  noise_cfg_.enable = noise_config.enable;
  noise_cfg_.flags = noise_config.flags;
  noise_cfg_.zpos_noise = noise_config.zpos_noise;
  noise_cfg_.zpos_attn = noise_config.zpos_attn;
  noise_cfg_.attn_factor =  noise_config.attenuation_factor;
  noise_cfg_.noise_strength =  noise_config.noise_strength;
  noise_cfg_.alpha_noise = noise_config.alpha_noise;
  noise_cfg_.temporal_en = noise_config.temporal_en;
  DLOGV_IF(kTagDriverConfig, "Display %d-%d z_noise = %d z_attn = %d attn_f = %d"
           " noise_str = %d alpha noise = %d temporal_en = %d", display_id_, disp_type_,
           noise_cfg_.zpos_noise, noise_cfg_.zpos_attn, noise_cfg_.attn_factor,
           noise_cfg_.noise_strength, noise_cfg_.alpha_noise, noise_cfg_.temporal_en);
}

void HWDeviceDRM::ApplyNoiseLayerConfig() {
  if (hw_resource_.has_noise_layer) {
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_NOISELAYER_CONFIG, token_.crtc_id,
                              reinterpret_cast<uint64_t>(&noise_cfg_));
  }
}

void HWDeviceDRM::ClearNoiseLayerConfig() {
  noise_cfg_ = {};
  ApplyNoiseLayerConfig();
}

void HWDeviceDRM::AddSolidfillStage(const HWSolidfillStage &sf, uint32_t plane_alpha) {
  sde_drm::DRMSolidfillStage solidfill;
  solidfill.bounding_rect.left = UINT32(sf.roi.left);
  solidfill.bounding_rect.top = UINT32(sf.roi.top);
  solidfill.bounding_rect.right = UINT32(sf.roi.right);
  solidfill.bounding_rect.bottom = UINT32(sf.roi.bottom);
  solidfill.is_exclusion_rect  = sf.is_exclusion_rect;
  solidfill.plane_alpha = plane_alpha;
  solidfill.z_order = sf.z_order;
  if (!sf.solid_fill_info.bit_depth) {
    solidfill.color_bit_depth = 8;
    solidfill.alpha = (0xff000000 & sf.color) >> 24;
    solidfill.red = (0xff0000 & sf.color) >> 16;
    solidfill.green = (0xff00 & sf.color) >> 8;
    solidfill.blue = 0xff & sf.color;
  } else {
    solidfill.color_bit_depth = sf.solid_fill_info.bit_depth;
    solidfill.alpha = sf.solid_fill_info.alpha;
    solidfill.red = sf.solid_fill_info.red;
    solidfill.green = sf.solid_fill_info.green;
    solidfill.blue = sf.solid_fill_info.blue;
  }
  solid_fills_.push_back(solidfill);
  DLOGI_IF(kTagDriverConfig,
           "Add a solidfill stage at z_order: %d argb: [%x %x %x %x]"
           "plane_alpha: %x",
           solidfill.z_order, solidfill.alpha, solidfill.red, solidfill.green, solidfill.blue,
           solidfill.plane_alpha);
}

void HWDeviceDRM::SetSolidfillStages() {
  if (hw_resource_.num_solidfill_stages) {
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_SOLIDFILL_STAGES, token_.crtc_id,
                              reinterpret_cast<uint64_t> (&solid_fills_));
  }
}

void HWDeviceDRM::ClearSolidfillStages() {
  solid_fills_.clear();
  SetSolidfillStages();
}

DisplayError HWDeviceDRM::Validate(HWLayersInfo *hw_layers_info) {
  DTRACE_SCOPED();

  DisplayError err = kErrorNone;
  int ret = registry_.Register(hw_layers_info);
  if (ret) {
    return kErrorParameters;
  }
  Fence::ScopedRef scoped_ref;
  SetupAtomic(scoped_ref, hw_layers_info, true /* validate */, nullptr, nullptr);

  ret = drm_atomic_intf_->Validate();
  if (ret) {
    DLOGE("failed with error %d for %s", ret, device_name_);
    DumpHWLayers(hw_layers_info);
    vrefresh_ = 0;
    panel_mode_changed_ = 0;
    seamless_mode_switch_ = false;
    panel_compression_changed_ = 0;
    transfer_time_updated_ = 0;
    err = kErrorHardware;
  }

  return err;
}

DisplayError HWDeviceDRM::Commit(HWLayersInfo *hw_layers_info) {
  DTRACE_SCOPED();

  DisplayError err = kErrorNone;
  registry_.Register(hw_layers_info);

  if (default_mode_) {
    err = DefaultCommit(hw_layers_info);
  } else {
    err = AtomicCommit(hw_layers_info);
  }

  return err;
}

DisplayError HWDeviceDRM::DefaultCommit(HWLayersInfo *hw_layers_info) {
  DTRACE_SCOPED();


  for (Layer &layer : hw_layers_info->hw_layers) {
    layer.input_buffer.release_fence = nullptr;
  }

  DRMMaster *master = nullptr;
  int ret = DRMMaster::GetInstance(&master, core_id_);
  if (ret < 0) {
    DLOGE("Failed to acquire DRMMaster instance");
    return kErrorResources;
  }

  DRMResMgr *res_mgr = nullptr;
  ret = DRMResMgr::GetInstance(&res_mgr);
  if (ret < 0) {
    DLOGE("Failed to acquire DRMResMgr instance");
    return kErrorResources;
  }

  int dev_fd = -1;
  master->GetHandle(&dev_fd);

  uint32_t connector_id = 0;
  res_mgr->GetConnectorId(&connector_id);

  uint32_t crtc_id = 0;
  res_mgr->GetCrtcId(&crtc_id);

  drmModeModeInfo mode;
  res_mgr->GetMode(&mode);

  uint64_t handle_id = hw_layers_info->hw_layers.at(0).input_buffer.handle_id;
  std::vector<uint32_t> fb_id = {};
  registry_.GetFbId(&hw_layers_info->hw_layers.at(0), handle_id, &fb_id);
  ret = drmModeSetCrtc(dev_fd, crtc_id, fb_id[kCacNone], 0 /* x */, 0 /* y */, &connector_id,
                       1 /* num_connectors */, &mode);
  if (ret < 0) {
    DLOGE("drmModeSetCrtc failed dev fd %d, fb_id %d, crtc id %d, connector id %d, %s", dev_fd,
          fb_id[kCacNone], crtc_id, connector_id, strerror(errno));
    return kErrorHardware;
  }

  return kErrorNone;
}

DisplayError HWDeviceDRM::AtomicCommit(HWLayersInfo *hw_layers_info) {
  DTRACE_SCOPED();

  int64_t release_fence_fd = -1;
  int64_t retire_fence_fd = -1;

  // scoped fence fds will be automatically closed when function scope ends,
  // atomic commit will have these fds already set on kernel by then.
  Fence::ScopedRef scoped_ref;
  SetupAtomic(scoped_ref, hw_layers_info, false /* validate */,
                                   &release_fence_fd, &retire_fence_fd);

  bool sync_commit = synchronous_commit_ || first_cycle_;
  uint64_t elapse_timestamp = hw_layers_info->common_info->elapse_timestamp;

  struct timespec t = {0, 0};
  clock_gettime(CLOCK_MONOTONIC, &t);
  uint64_t current_time = (UINT64(t.tv_sec) * 1000000000LL + t.tv_nsec);

  // Handle the case where EPT and Current time delta is beyond the QSync MinFps period.
  if (hw_panel_info_.qsync_support &&
      (hw_layers_info->common_info->hw_avr_info.mode != kQsyncNone) &&
      (connector_info_.qsync_fps > 0)) {
    uint64_t qsync_fps_period = (1000.0f / FLOAT(connector_info_.qsync_fps)) * 1000000;
    if (hw_layers_info->common_info->expected_present_time > current_time) {
      if ((hw_layers_info->common_info->expected_present_time - current_time) >
          qsync_fps_period) {
        uint64_t vsync_period = display_attributes_[current_mode_index_].vsync_period_ns;
        if (hw_layers_info->common_info->expected_present_time > vsync_period) {
          elapse_timestamp =
              hw_layers_info->common_info->expected_present_time - vsync_period;
        }
      }
    }
  }

  if ((elapse_timestamp > 0) && (current_time < elapse_timestamp)) {
    usleep(UINT32((elapse_timestamp - current_time) / 1000));
  }

  int ret = drm_atomic_intf_->Commit(sync_commit, false /* retain_planes*/);
  shared_ptr<Fence> release_fence = Fence::Create(INT(release_fence_fd), "release");
  shared_ptr<Fence> retire_fence = Fence::Create(INT(retire_fence_fd), "retire");
  if (ret) {
    DLOGE("%s failed with error %d crtc %d", __FUNCTION__, ret, token_.crtc_id);
    DumpHWLayers(hw_layers_info);
    vrefresh_ = 0;
    panel_mode_changed_ = 0;
    seamless_mode_switch_ = false;
    panel_compression_changed_ = 0;
    transfer_time_updated_ = 0;
    return kErrorHardware;
  }

  DLOGD_IF(kTagDriverConfig, "RELEASE fence: fd: %s", Fence::GetStr(release_fence).c_str());
  DLOGD_IF(kTagDriverConfig, "RETIRE fence: fd: %s", Fence::GetStr(retire_fence).c_str());

  hw_layers_info->retire_fence = retire_fence;

  for (uint32_t i = 0; i < hw_layers_info->hw_layers.size(); i++) {
    Layer &layer = hw_layers_info->hw_layers.at(i);
    HWRotatorSession *hw_rotator_session = &hw_layers_info->config[i].hw_rotator_session;
    if (hw_rotator_session->mode == kRotatorOffline) {
      hw_rotator_session->output_buffer.release_fence = release_fence;
    } else {
      layer.input_buffer.release_fence = release_fence;
    }
  }

  hw_layers_info->sync_handle = release_fence;

  if (vrefresh_) {
    // Update current mode index if refresh rate is changed
    drmModeModeInfo current_mode = connector_info_.modes[current_mode_index_].mode;
    for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
      if ((current_mode.vdisplay == connector_info_.modes[mode_index].mode.vdisplay) &&
          (current_mode.hdisplay == connector_info_.modes[mode_index].mode.hdisplay) &&
          (vrefresh_ == connector_info_.modes[mode_index].mode.vrefresh)) {
        SetDisplaySwitchMode(mode_index);
        break;
      }
    }
    vrefresh_ = 0;
  }

  if (bit_clk_rate_) {
    // Update current mode index if bit clk rate is changed.
    connector_info_.modes[current_mode_index_].curr_bit_clk_rate = bit_clk_rate_;

    bit_clk_rate_ = 0;
  }

  if (transfer_time_updated_) {
    transfer_time_updated_ = 0;
    synchronous_commit_ = false;
  }

  if (panel_mode_changed_ & DRM_MODE_FLAG_CMD_MODE_PANEL) {
    panel_mode_changed_ = 0;
    synchronous_commit_ = false;
  } else if (panel_mode_changed_ & DRM_MODE_FLAG_VID_MODE_PANEL) {
    panel_mode_changed_ = 0;
    synchronous_commit_ = false;
    reset_output_fence_offset_ = true;
  }

  panel_compression_changed_ = 0;
  reset_planes_luts_ = false;
  first_cycle_ = false;
  update_mode_ = false;
  pending_power_state_ = kPowerStateNone;
  pending_cwb_teardown_ = false;
  // Inherently a real commit ensures null commit properties have happened, so update the member
  first_null_cycle_ = false;
  seamless_mode_switch_ = false;

  SetTUIState();

#ifdef TRUSTED_VM
  drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_VM_REQ_STATE, token_.crtc_id,
                            sde_drm::DRMVMRequestState::NONE);
#endif

  return kErrorNone;
}

DisplayError HWDeviceDRM::Flush(HWLayersInfo *hw_layers_info) {
  ClearSolidfillStages();
  ClearNoiseLayerConfig();
  ResetROI();
  bool sync_commit = (tui_state_ == kTUIStateStart || tui_state_ == kTUIStateEnd ||
                      secure_display_active_);

  // TODO(user): Handle via flush call
#ifdef TRUSTED_VM
  sync_commit = true;
#endif

  // dpps commit feature ops doesn't use the obj id, set it as -1
  drm_atomic_intf_->Perform(DRMOps::DPPS_COMMIT_FEATURE, -1);

  if (cwb_config_[core_id_].enabled) {
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, cwb_config_[core_id_].token.conn_id, 0);
    DLOGI("Tearing down the CWB topology");
  }

  int ret = NullCommit(sync_commit /* synchronous */, false /* retain_planes*/);
  if (ret) {
    DLOGE("failed with error %d", ret);
    return kErrorHardware;
  }

  if (cwb_config_[core_id_].enabled) {
    FlushConcurrentWriteback();
  }

  return kErrorNone;
}

void HWDeviceDRM::SetBlending(const LayerBlending &source, DRMBlendType *target) {
  switch (source) {
    case kBlendingPremultiplied:
      *target = DRMBlendType::PREMULTIPLIED;
      break;
    case kBlendingOpaque:
      *target = DRMBlendType::OPAQUE;
      break;
    case kBlendingCoverage:
      *target = DRMBlendType::COVERAGE;
      break;
    case kBlendingSkip:
      *target = DRMBlendType::SKIP_BLENDING;
      break;
    default:
      *target = DRMBlendType::UNDEFINED;
  }
}

void HWDeviceDRM::SetCacType(const HWPipeCacMode &cac_mode, DRMCacMode *target) {
  switch (cac_mode) {
    case kModeDisabled:
      *target = DRMCacMode::CAC_MODE_DISABLED;
      break;
    case kModeUnpack:
      *target = DRMCacMode::CAC_MODE_UNPACK;
      break;
    case kModeFetch:
      *target = DRMCacMode::CAC_MODE_FETCH;
      break;
    case kModeLoopbackUnpack:
      *target = DRMCacMode::CAC_MODE_LOOPBACK_UNPACK;
      break;
    case kModeLoopbackFetch:
      *target = DRMCacMode::CAC_MODE_LOOPBACK_FETCH;
      break;
    default:
      *target = DRMCacMode::CAC_MODE_DISABLED;
  }
}

void HWDeviceDRM::SetSrcConfig(const LayerBuffer &input_buffer, const HWRotatorMode &mode,
                               uint32_t *config) {
  // In offline rotation case, rotator will handle deinterlacing.
  if (mode == kRotatorInline) {
    if (input_buffer.flags.interlace) {
      *config |= (0x01 << UINT32(DRMSrcConfig::DEINTERLACE));
    }
  }
}

void HWDeviceDRM::SelectCscType(const LayerBuffer &input_buffer, DRMCscType *type) {
  if (type == NULL) {
    return;
  }

  *type = DRMCscType::kCscTypeMax;
  if (input_buffer.format < kFormatYCbCr420Planar) {
    return;
  }

  // Override YUV to RGB CSC in DV P5 cases. If extended content metadata is used
  // and is DV's sync word, it should be DV. Then using full range to judge it's P5.
  bool extended_md_present = input_buffer.extended_content_metadata != nullptr &&
                              input_buffer.extended_content_metadata->size;
  if (extended_md_present) {
    uint8_t *md = input_buffer.extended_content_metadata->metadataPayload;
    if (md[0] == 0x0 && md[1] == 0x0 && md[2] == 0x0 && md[3] == 0x1 && md[4] == 0x7c &&
        md[5] == 0x1) {
      if ((input_buffer.dataspace.range == QtiRange_Full) ||
          (input_buffer.dataspace.transfer == QtiTransfer_SMPTE_170M) ||
          (input_buffer.dataspace.transfer == QtiTransfer_sRGB)) {
        *type = DRMCscType::kCscYuv2RgbDolbyVisionP5;
        return;
      }
    }
  }

  switch (input_buffer.dataspace.colorPrimaries) {
    case QtiColorPrimaries_BT601_6_525:
    case QtiColorPrimaries_BT601_6_625:
      *type = ((input_buffer.dataspace.range == QtiRange_Full) ?
               DRMCscType::kCscYuv2Rgb601FR : DRMCscType::kCscYuv2Rgb601L);
      break;
    case QtiColorPrimaries_BT709_5:
      *type = ((input_buffer.dataspace.range == QtiRange_Full) ?
               DRMCscType::kCscYuv2Rgb709FR : DRMCscType::kCscYuv2Rgb709L);
      break;
    case QtiColorPrimaries_BT2020:
      *type = ((input_buffer.dataspace.range == QtiRange_Full) ?
                DRMCscType::kCscYuv2Rgb2020FR : DRMCscType::kCscYuv2Rgb2020L);
      break;
    case QtiColorPrimaries_DCIP3:
      *type = ((input_buffer.dataspace.range == QtiRange_Full) ?
                DRMCscType::kCscYuv2RgbDCIP3FR : DRMCscType::kCscTypeMax);
      break;
    default:
      break;
  }
}

void HWDeviceDRM::SelectFp16Config(const LayerBuffer &input_buffer, int *igc_en, int *unmult_en,
                                   sde_drm::DRMFp16CscType *csc_type, drm_msm_fp16_gc *gc,
                                   LayerBlending blend) {
  if (csc_type == NULL || gc == NULL || igc_en == NULL || unmult_en == NULL) {
    // FP16 block will be disabled by default for invalid params
    DLOGE("Invalid params");
    return;
  }

  *csc_type = sde_drm::DRMFp16CscType::kFP16CscTypeMax;
  *unmult_en = 0;
  *igc_en = 0;
  gc->flags = 0;
  gc->mode = FP16_GC_MODE_INVALID;

  if (!Is16BitFormat(input_buffer.format)) {
    return;
  }

  // FP16 block should only be configured for the expected use cases.
  // All other cases will be disabled by default.
  if ((input_buffer.dataspace.colorPrimaries == QtiColorPrimaries_BT709_5) &&
      (input_buffer.dataspace.range == QtiRange_Extended)) {
    *csc_type = sde_drm::DRMFp16CscType::kFP16CscSrgb2Bt2020;
    gc->mode = FP16_GC_MODE_PQ;
    if (input_buffer.dataspace.transfer == QtiTransfer_sRGB) {
      *igc_en = 1;
    } else if (input_buffer.dataspace.transfer == QtiTransfer_Linear) {
      *igc_en = 0;
    }

    if (blend == kBlendingPremultiplied) {
      *unmult_en = 1;
    }
  }
}

void HWDeviceDRM::SetRect(const LayerRect &source, DRMRect *target) {
  target->left = UINT32(source.left);
  target->top = UINT32(source.top);
  target->right = UINT32(source.right);
  target->bottom = UINT32(source.bottom);
}

void HWDeviceDRM::SetRotation(LayerTransform transform, const HWLayerConfig &layer_config,
                              uint32_t* rot_bit_mask) {
  HWRotatorMode mode = layer_config.hw_rotator_session.mode;
  // In offline rotation case, rotator will handle flips set via offline rotator interface.
  if (mode == kRotatorOffline) {
    *rot_bit_mask = 0;
    return;
  }

  // In no rotation case or inline rotation case, plane will handle flips
  // In DRM framework rotation is applied in counter-clockwise direction.
  if (layer_config.use_inline_rot && transform.rotation == 90) {
    // a) rotate 90 clockwise = rotate 270 counter-clockwise in DRM
    // rotate 270 is translated as hflip + vflip + rotate90
    // b) rotate 270 clockwise = rotate 90 counter-clockwise in DRM
    // c) hflip + rotate 90 clockwise = vflip + rotate 90 counter-clockwise in DRM
    // d) vflip + rotate 90 clockwise = hflip + rotate 90 counter-clockwise in DRM
    *rot_bit_mask = UINT32(DRMRotation::ROT_90);
    transform.flip_horizontal = !transform.flip_horizontal;
    transform.flip_vertical = !transform.flip_vertical;
  }

  if (transform.flip_horizontal) {
    *rot_bit_mask |= UINT32(DRMRotation::FLIP_H);
  }

  if (transform.flip_vertical) {
    *rot_bit_mask |= UINT32(DRMRotation::FLIP_V);
  }
}

bool HWDeviceDRM::EnableHotPlugDetection(int enable) {
  return true;
}

DisplayError HWDeviceDRM::SetCursorPosition(HWLayersInfo *hw_layers_info, int x, int y) {
  DTRACE_SCOPED();
  return kErrorNone;
}

DisplayError HWDeviceDRM::GetPPFeaturesVersion(PPFeatureVersion *vers) {
  struct DRMPPFeatureInfo info = {};

  if (!hw_color_mgr_)
    return kErrorNotSupported;

  for (uint32_t i = 0; i < kMaxNumPPFeatures; i++) {
    std::vector<DRMPPFeatureID> drm_id = {};
    memset(&info, 0, sizeof(struct DRMPPFeatureInfo));
    hw_color_mgr_->ToDrmFeatureId(kDSPP, i, &drm_id);
    if (drm_id.empty())
      continue;

    info.id = drm_id.at(0);

    drm_mgr_intf_->GetCrtcPPInfo(token_.crtc_id, &info);
    vers->version[i] = hw_color_mgr_->GetFeatureVersion(info);
  }
  return kErrorNone;
}

DisplayError HWDeviceDRM::SetPPFeature(PPFeatureInfo *feature) {
  int ret = 0;
  DRMCrtcInfo crtc_info = {};

  if (!hw_color_mgr_) {
    return kErrorNotSupported;
  }

  std::vector<DRMPPFeatureID> drm_id = {};
  DRMPPFeatureInfo kernel_params = {};
  bool crtc_feature = true;

  hw_color_mgr_->ToDrmFeatureId(kDSPP, feature->feature_id_, &drm_id);
  if (drm_id.empty()) {
    return kErrorNone;
  } else if (drm_id.at(0) == DRMPPFeatureID::kFeatureDither) {
    drm_mgr_intf_->GetCrtcInfo(token_.crtc_id, &crtc_info);
    if (crtc_info.has_spr)
      drm_id.at(0) = DRMPPFeatureID::kFeatureSprDither;
  }

  kernel_params.id = drm_id.at(0);
  drm_mgr_intf_->GetCrtcPPInfo(token_.crtc_id, &kernel_params);
  if (kernel_params.version == std::numeric_limits<uint32_t>::max())
    crtc_feature = false;

  DLOGV_IF(kTagDriverConfig, "feature_id = %d", feature->feature_id_);
  for (DRMPPFeatureID id : drm_id) {
    if (id >= kPPFeaturesMax) {
      DLOGE("Invalid feature id %d", id);
      continue;
    }

    kernel_params.id = id;
    ret = hw_color_mgr_->GetDrmFeature(feature, &kernel_params);
    if (!ret && crtc_feature)
      drm_atomic_intf_->Perform(DRMOps::CRTC_SET_POST_PROC,
                                token_.crtc_id, &kernel_params);
    else if (!ret && !crtc_feature)
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POST_PROC,
                                token_.conn_id, &kernel_params);

    hw_color_mgr_->FreeDrmFeatureData(&kernel_params);
  }

  return kErrorNone;
}

DisplayError HWDeviceDRM::SetVSyncState(bool enable) {
  return kErrorNotSupported;
}

void HWDeviceDRM::SetIdleTimeoutMs(uint32_t timeout_ms) {
  // TODO(user): This function can be removed after fb is deprecated
}

DisplayError HWDeviceDRM::SetDisplayMode(const HWDisplayMode hw_display_mode) {
  if (!switch_mode_valid_) {
    return kErrorNotSupported;
  }

  uint32_t mode_flag = 0;
  sde_drm::DRMModeInfo current_mode = connector_info_.modes[current_mode_index_];

  // Refresh rate change needed if new panel mode differs from current
  if (hw_display_mode == kModeCommand) {
    mode_flag = DRM_MODE_FLAG_CMD_MODE_PANEL;
    if (connector_info_.modes[cmd_mode_index_].mode.vrefresh != current_mode.mode.vrefresh) {
      vrefresh_ = connector_info_.modes[cmd_mode_index_].mode.vrefresh;
    }
    current_mode_index_ = cmd_mode_index_;
    connector_info_.modes[current_mode_index_].cur_panel_mode = mode_flag;
    DLOGI_IF(kTagDriverConfig, "switch panel mode to command");
  } else if (hw_display_mode == kModeVideo) {
    mode_flag = DRM_MODE_FLAG_VID_MODE_PANEL;
    if (connector_info_.modes[video_mode_index_].mode.vrefresh != current_mode.mode.vrefresh) {
      vrefresh_ = connector_info_.modes[video_mode_index_].mode.vrefresh;
    }
    current_mode_index_ = video_mode_index_;
    connector_info_.modes[current_mode_index_].cur_panel_mode = mode_flag;
    DLOGI_IF(kTagDriverConfig, "switch panel mode to video");
  }
  PopulateHWPanelInfo();
  panel_mode_changed_ = mode_flag;
  synchronous_commit_ = true;
  return kErrorNone;
}

DisplayError HWDeviceDRM::SetRefreshRate(uint32_t refresh_rate) {
  if (bit_clk_rate_) {
    // bit rate update pending.
    // Defer any refresh rate setting.
    return kErrorNotSupported;
  }

  // Check if requested refresh rate is valid
  sde_drm::DRMModeInfo current_mode = connector_info_.modes[current_mode_index_];
  for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
    if ((current_mode.mode.vdisplay == connector_info_.modes[mode_index].mode.vdisplay) &&
        (current_mode.mode.hdisplay == connector_info_.modes[mode_index].mode.hdisplay) &&
        (current_mode.cur_panel_mode == connector_info_.modes[mode_index].cur_panel_mode) &&
        (refresh_rate == connector_info_.modes[mode_index].mode.vrefresh)) {
      for (uint32_t submode_idx = 0; submode_idx <
           connector_info_.modes[mode_index].sub_modes.size(); submode_idx++) {
        sde_drm::DRMSubModeInfo sub_mode = connector_info_.modes[mode_index].sub_modes[submode_idx];
        if (sub_mode.panel_compression_mode == current_mode.curr_compression_mode) {
          connector_info_.modes[mode_index].curr_submode_index = submode_idx;
          vrefresh_ = refresh_rate;
          DLOGV_IF(kTagDriverConfig, "Set refresh rate to %d", refresh_rate);
          return kErrorNone;
        }
      }
    }
  }
  return kErrorNotSupported;
}



DisplayError HWDeviceDRM::GetHWScanInfo(HWScanInfo *scan_info) {
  return kErrorNotSupported;
}

DisplayError HWDeviceDRM::GetVideoFormat(uint32_t config_index, uint32_t *video_format) {
  return kErrorNotSupported;
}

DisplayError HWDeviceDRM::GetMaxCEAFormat(uint32_t *max_cea_format) {
  return kErrorNotSupported;
}

DisplayError HWDeviceDRM::OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level) {
  DisplayError error = kErrorNone;
  int fd = -1;
  char data[kMaxStringLength] = {'\0'};

  snprintf(data, sizeof(data), "/sys/devices/virtual/hdcp/msm_hdcp/min_level_change");

  fd = Sys::open_(data, O_WRONLY);
  if (fd < 0) {
    DLOGE("File '%s' could not be opened. errno = %d, desc = %s", data, errno, strerror(errno));
    return kErrorHardware;
  }

  snprintf(data, sizeof(data), "%d", min_enc_level);

  ssize_t err = Sys::pwrite_(fd, data, strlen(data), 0);
  if (err <= 0) {
    DLOGE("Write failed, Error = %s", strerror(errno));
    error = kErrorHardware;
  }

  Sys::close_(fd);

  return error;
}

DisplayError HWDeviceDRM::SetScaleLutConfig(HWScaleLutInfo *lut_info) {
  sde_drm::DRMScalerLUTInfo drm_lut_info = {};
  drm_lut_info.cir_lut = lut_info->cir_lut;
  drm_lut_info.dir_lut = lut_info->dir_lut;
  drm_lut_info.sep_lut = lut_info->sep_lut;
  drm_lut_info.cir_lut_size = lut_info->cir_lut_size;
  drm_lut_info.dir_lut_size = lut_info->dir_lut_size;
  drm_lut_info.sep_lut_size = lut_info->sep_lut_size;
  drm_mgr_intf_->SetScalerLUT(drm_lut_info);

  return kErrorNone;
}

DisplayError HWDeviceDRM::UnsetScaleLutConfig() {
  drm_mgr_intf_->UnsetScalerLUT();

  return kErrorNone;
}

DisplayError HWDeviceDRM::SetMixerAttributes(const HWMixerAttributes &mixer_attributes) {
  if (!dest_scaler_blocks_used_ && !ai_scaler_blocks_used_) {
    return kErrorNotSupported;
  }

  uint32_t index = current_mode_index_;

  if (mixer_attributes.width > display_attributes_[index].x_pixels ||
      mixer_attributes.height > display_attributes_[index].y_pixels) {
    DLOGW("Input resolution exceeds display resolution! input: res %dx%d display: res %dx%d",
          mixer_attributes.width, mixer_attributes.height, display_attributes_[index].x_pixels,
          display_attributes_[index].y_pixels);
    return kErrorNotSupported;
  }

  uint32_t topology_num_split = display_attributes_[index].topology_num_split;
  if (mixer_attributes.width % topology_num_split != 0) {
    DLOGW("Uneven LM split: topology:%d supported_split:%d mixer_width:%d",
          display_attributes_[index].topology, topology_num_split, mixer_attributes.width);
    return kErrorNotSupported;
  }

  uint32_t max_input_width = hw_resource_.hw_dest_scalar_info.max_input_width;
  uint32_t split_max_input_width = max_input_width - DEST_SCALAR_OVERFETCH_SIZE;
  uint32_t lm_split_width = mixer_attributes.width / topology_num_split;
  if (topology_num_split > 1 && lm_split_width > split_max_input_width) {
    DLOGW("Input width exceeds width limit in split LM mode. input_width %d width_limit %d",
          lm_split_width, split_max_input_width);
    return kErrorNotSupported;
  }

  if (mixer_attributes.width > max_input_width * topology_num_split) {
    DLOGW("Input width exceeds width limit! input_width %d width_limit %d", mixer_attributes.width,
          max_input_width * topology_num_split);
    return kErrorNotSupported;
  }

  float mixer_aspect_ratio = FLOAT(mixer_attributes.width) / FLOAT(mixer_attributes.height);
  float display_aspect_ratio =
      FLOAT(display_attributes_[index].x_pixels) / FLOAT(display_attributes_[index].y_pixels);

  float display_to_mixer_aspect_ratio = std::max(display_aspect_ratio, mixer_aspect_ratio) /
                                        std::min(display_aspect_ratio, mixer_aspect_ratio);
  if (display_to_mixer_aspect_ratio > aspect_ratio_threshold_) {
    DLOGW("Aspect ratio mismatch! input: res %dx%d display: res %dx%d", mixer_attributes.width,
          mixer_attributes.height, display_attributes_[index].x_pixels,
          display_attributes_[index].y_pixels);
    return kErrorNotSupported;
  }

  float scale_x = FLOAT(display_attributes_[index].x_pixels) / FLOAT(mixer_attributes.width);
  float scale_y = FLOAT(display_attributes_[index].y_pixels) / FLOAT(mixer_attributes.height);
  float max_scale_up = hw_resource_.hw_dest_scalar_info.max_scale_up;
  if (scale_x > max_scale_up || scale_y > max_scale_up) {
    DLOGW(
        "Up scaling ratio exceeds for destination scalar upscale limit scale_x %f scale_y %f "
        "max_scale_up %f",
        scale_x, scale_y, max_scale_up);
    return kErrorNotSupported;
  }

  float mixer_split_ratio = FLOAT(mixer_attributes_.split_left) / FLOAT(mixer_attributes_.width);

  mixer_attributes_ = mixer_attributes;
  mixer_attributes_.split_left = mixer_attributes_.width;
  mixer_attributes_.split_type = kNoSplit;
  mixer_attributes_.dest_scaler_blocks_used = dest_scaler_blocks_used_;  // No change.
  mixer_attributes_.ai_scaler_blocks_used = ai_scaler_blocks_used_;      // No change.
  if (display_attributes_[index].is_device_split) {
    mixer_attributes_.split_left = UINT32(FLOAT(mixer_attributes.width) * mixer_split_ratio);
    mixer_attributes_.split_type = kDualSplit;
    if (display_attributes_[index].topology == kQuadLMMerge ||
        display_attributes_[index].topology == kQuadLMDSCMerge ||
        display_attributes_[index].topology == kQuadLMMergeDSC ||
        display_attributes_[index].topology == kQuadLMDSC4HSMerge) {
      mixer_attributes_.split_type = kQuadSplit;
    }
  }

  return kErrorNone;
}

DisplayError HWDeviceDRM::GetMixerAttributes(HWMixerAttributes *mixer_attributes) {
  if (!mixer_attributes) {
    DLOGW("mixer_attributes invalid. Return.");
    return kErrorParameters;
  }

  *mixer_attributes = mixer_attributes_;

  return kErrorNone;
}

DisplayError HWDeviceDRM::DumpDebugData() {
  string out_dir_path = "/data/vendor/display/hw_recovery/";
  string devcd_dir_path = "/sys/class/devcoredump/";
  string device_str = device_name_;
  string devcd_path;
  string driver_name;

  // Attempt to make hw_recovery dir, it may exist
  if (mkdir(out_dir_path.c_str(), 0777) != 0 && errno != EEXIST) {
    DLOGW("Failed to create %s directory errno = %d, desc = %s", out_dir_path.c_str(), errno,
          strerror(errno));
    return kErrorPermission;
  }
  // If it does exist, ensure permissions are fine
  if (errno == EEXIST && chmod(out_dir_path.c_str(), 0777) != 0) {
    DLOGW("Failed to change permissions on %s directory", out_dir_path.c_str());
    return kErrorPermission;
  }

  string filename = out_dir_path + device_str + "_HWR_" + to_string(debug_dump_count_);
  ofstream dst;
  debug_dump_count_++;
  fstream src;
  char buffer[3*1024 + 1];
  auto dir = opendir(devcd_dir_path.c_str());

  if (dir == nullptr) {
    DLOGW("Failed to access %s directory", devcd_dir_path.c_str());
    return kErrorPermission;
  }

  // Find the devcd node corresponding to display driver
  while (auto i = readdir(dir)) {
    if (string(i->d_name).find("devcd") != string::npos) {
      devcd_path = devcd_dir_path + i->d_name + "/failing_device/uevent";
      src.open(devcd_path, fstream::in);
      if (src.fail()) {
        continue;
      } else {
        src >> driver_name;
        src.close();
        if (driver_name == "DRIVER=msm_drm") {
          closedir(dir);
          devcd_path = devcd_dir_path + i->d_name + "/data";
          src.rdbuf()->pubsetbuf(buffer, sizeof(buffer));
          src.open(devcd_path, fstream::in);
          dst.open(filename);
          dst << "---- Event Logs ----" << std::endl;
          dst << src.rdbuf() << std::endl;
          if (src.fail()) {
            DLOGW("Unable to read devcoredump node");
            return kErrorUndefined;
          }
          src.close();
          dst.close();
          DLOGI("Wrote hw_recovery file %s", filename.c_str());

          return kErrorNone;
        }
      }
    }
  }
  closedir(dir);
  DLOGW("Unable to find devcoredump data node");
  return kErrorNone;
}

void HWDeviceDRM::GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) const {
  *token = token_;
}

void HWDeviceDRM::UpdateMixerAttributes() {
  uint32_t index = current_mode_index_;

#ifdef SEC_GC_CMN_DDISCALER
  if (disp_type_ != DRMDisplayType::PERIPHERAL ||
      (disp_type_ == DRMDisplayType::PERIPHERAL && (mixer_attributes_.width == 0 || mixer_attributes_.height == 0))) {
#endif
  mixer_attributes_.width = display_attributes_[index].x_pixels;
  mixer_attributes_.height = display_attributes_[index].y_pixels;
  mixer_attributes_.split_left = display_attributes_[index].is_device_split
                                     ? hw_panel_info_.split_info.left_split
                                     : mixer_attributes_.width;
#ifdef SEC_GC_CMN_DDISCALER
  }
#endif
  mixer_attributes_.split_type = kNoSplit;
  if (display_attributes_[index].is_device_split) {
    mixer_attributes_.split_type = kDualSplit;
    if (display_attributes_[index].topology == kQuadLMMerge ||
        display_attributes_[index].topology == kQuadLMDSCMerge ||
        display_attributes_[index].topology == kQuadLMMergeDSC ||
        display_attributes_[index].topology == kQuadLMDSC4HSMerge) {
      mixer_attributes_.split_type = kQuadSplit;
    }
  }

  DLOGI("Mixer WxH %dx%d-%d for %s", mixer_attributes_.width, mixer_attributes_.height,
        mixer_attributes_.split_type, device_name_);
  update_mode_ = true;
}

void HWDeviceDRM::SetSecureConfig(const LayerBuffer &input_buffer, DRMSecureMode *fb_secure_mode,
                                  DRMSecurityLevel *security_level) {
  *fb_secure_mode = DRMSecureMode::NON_SECURE;
  *security_level = DRMSecurityLevel::SECURE_NON_SECURE;

  if (input_buffer.flags.secure) {
    if (input_buffer.flags.secure_camera) {
      // IOMMU configuration for this framebuffer mode is secure domain & requires
      // only stage II translation, when this buffer is accessed by Display H/W.
      // Secure and non-secure planes can be attached to this CRTC.
      *fb_secure_mode = DRMSecureMode::SECURE_DIR_TRANSLATION;
    } else if (input_buffer.flags.secure_display) {
      // IOMMU configuration for this framebuffer mode is secure domain & requires
      // only stage II translation, when this buffer is accessed by Display H/W.
      // Only secure planes can be attached to this CRTC.
      *fb_secure_mode = DRMSecureMode::SECURE_DIR_TRANSLATION;
      *security_level = DRMSecurityLevel::SECURE_ONLY;
    } else {
      // IOMMU configuration for this framebuffer mode is secure domain & requires both
      // stage I and stage II translations, when this buffer is accessed by Display H/W.
      // Secure and non-secure planes can be attached to this CRTC.
      *fb_secure_mode = DRMSecureMode::SECURE;
    }
  }
}

void HWDeviceDRM::SetTopology(sde_drm::DRMTopology drm_topology, HWTopology *hw_topology) {
  switch (drm_topology) {
    case DRMTopology::SINGLE_LM:            *hw_topology = kSingleLM;           break;
    case DRMTopology::SINGLE_LM_DSC:        *hw_topology = kSingleLMDSC;        break;
    case DRMTopology::DUAL_LM:              *hw_topology = kDualLM;             break;
    case DRMTopology::DUAL_LM_DSC:          *hw_topology = kDualLMDSC;          break;
    case DRMTopology::DUAL_LM_MERGE:        *hw_topology = kDualLMMerge;        break;
    case DRMTopology::DUAL_LM_MERGE_DSC:    *hw_topology = kDualLMMergeDSC;     break;
    case DRMTopology::DUAL_LM_DSCMERGE:     *hw_topology = kDualLMDSCMerge;     break;
    case DRMTopology::QUAD_LM_MERGE:        *hw_topology = kQuadLMMerge;        break;
    case DRMTopology::QUAD_LM_DSCMERGE:     *hw_topology = kQuadLMDSCMerge;     break;
    case DRMTopology::QUAD_LM_MERGE_DSC:    *hw_topology = kQuadLMMergeDSC;     break;
    case DRMTopology::QUAD_LM_DSC4HSMERGE:  *hw_topology = kQuadLMDSC4HSMerge;  break;
    case DRMTopology::PPSPLIT:              *hw_topology = kPPSplit;            break;
    default:                                *hw_topology = kUnknown;            break;
  }
}

void HWDeviceDRM::SetMultiRectMode(const uint32_t flags, DRMMultiRectMode *target) {
  *target = DRMMultiRectMode::NONE;
  if (flags & kMultiRect) {
    *target = DRMMultiRectMode::SERIAL;
    if (flags & kMultiRectParallelMode) {
      *target = DRMMultiRectMode::PARALLEL;
    }
  }
}

void HWDeviceDRM::SetSsppTonemapFeatures(HWPipeInfo *pipe_info) {
  SetSsppLutFeatures(pipe_info);

#ifdef UCSC_SUPPORTED
  if (pipe_info->ucsc_write_op.size() > 0) {
    SetUcscTonemapFeatures(pipe_info);
    // inverse pma needs to be set / reset for VIG since luts require legacy block
    if (pipe_info->sub_block_type == kHWVIGPipe) {
      SetLegacyTonemapFeatures(pipe_info);
    }
    return;
  }
#endif

  // sets legacy DGM CSC and inverse pma
  SetLegacyTonemapFeatures(pipe_info);
}

void HWDeviceDRM::SetLegacyTonemapFeatures(HWPipeInfo *pipe_info) {
  if (pipe_info->dgm_csc_info.op != kNoOp) {
    SDECsc csc = {};
    SetDGMCsc(pipe_info->dgm_csc_info, &csc);
    DLOGV_IF(kTagDriverConfig, "Call Perform DGM CSC Op = %s",
            (pipe_info->dgm_csc_info.op == kSet) ? "Set" : "Reset");
    drm_atomic_intf_->Perform(DRMOps::PLANE_SET_DGM_CSC_CONFIG, pipe_info->pipe_id,
                              reinterpret_cast<uint64_t>(&csc.csc_v1));
  }
  if (pipe_info->inverse_pma_info.op != kNoOp) {
    DLOGV_IF(kTagDriverConfig, "Call Perform Inverse PMA Op = %s",
            (pipe_info->inverse_pma_info.op == kSet) ? "Set" : "Reset");
    drm_atomic_intf_->Perform(DRMOps::PLANE_SET_INVERSE_PMA, pipe_info->pipe_id,
                             (pipe_info->inverse_pma_info.inverse_pma) ? 1: 0);
  }
}

#ifdef UCSC_SUPPORTED
void HWDeviceDRM::SetUcscTonemapFeatures(HWPipeInfo *pipe_info) {
  for (auto &it : pipe_info->ucsc_write_op) {
    switch (it.first) {
      case kUcscUnmult:
        DLOGV_IF(kTagDriverConfig, "Call Perform UCSC UNMULT enable = %d, display: %d-%d",
                 pipe_info->ucsc_config.unmult_en, display_id_, disp_type_);
        drm_atomic_intf_->Perform(DRMOps::PLANE_SET_UCSC_UNMULT_CONFIG, pipe_info->pipe_id,
                                  pipe_info->ucsc_config.unmult_en);
        break;
      case kUcscIgc:
        if (it.second != kNoOp) {
          DRMUcscIgcMode igc = DRMUcscIgcMode::UCSC_IGC_MODE_DISABLE;
          if (it.second == kSet) {
            SetUcscIgc(pipe_info->ucsc_config.igc_lut_sel, &igc);
          }
          DLOGV_IF(kTagDriverConfig, "Call Perform UCSC IGC Op = %s, display: %d-%d",
                   (it.second == kSet) ? "Set" : "Reset", display_id_, disp_type_);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_UCSC_IGC_CONFIG, pipe_info->pipe_id, igc);
        }
        break;
      case kUcscCsc:
        if (it.second != kNoOp) {
          drm_msm_ucsc_csc csc = {};
          if (it.second == kSet) {
            SetUcscCsc(pipe_info->ucsc_config.csc, &csc);
          }
          DLOGV_IF(kTagDriverConfig, "Call Perform UCSC CSC Op = %s, display: %d-%d",
                   (it.second == kSet) ? "Set" : "Reset", display_id_, disp_type_);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_UCSC_CSC_CONFIG, pipe_info->pipe_id,
                                    (it.second == kSet) ? &csc : nullptr);
        }
        break;
      case kUcscGc:
        if (it.second != kNoOp) {
          DRMUcscGcMode gc = DRMUcscGcMode::UCSC_GC_MODE_DISABLE;
          if (it.second == kSet) {
            SetUcscGc(pipe_info->ucsc_config.gc_lut_sel, &gc);
          }
          DLOGV_IF(kTagDriverConfig, "Call Perform UCSC GC Op = %s, display: %d-%d",
                   (it.second == kSet) ? "Set" : "Reset", display_id_, disp_type_);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_UCSC_GC_CONFIG, pipe_info->pipe_id, gc);
        }
        break;
      case kUcscAlphaDither:
        if (it.second != kNoOp) {
          DLOGV_IF(kTagDriverConfig, "Call Perform UCSC ALPHA DITHER Op = %s, display: %d-%d",
                   (it.second == kSet) ? "Set" : "Reset", display_id_, disp_type_);
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_UCSC_ALPHA_DITHER_CONFIG, pipe_info->pipe_id,
                                    pipe_info->ucsc_config.alpha_dither_en);
        }
        break;
      default:
        DLOGE("Unknown UCSC block - %d, display: %d-%d", it.first, display_id_, disp_type_);
        break;
    }
  }
}

void HWDeviceDRM::SetUcscIgc(const HWUcscIgcMode igc_lut_sel, sde_drm::DRMUcscIgcMode *igc) {
  switch (igc_lut_sel) {
    case kUcscIgcModeSrgb:
      *igc = DRMUcscIgcMode::UCSC_IGC_MODE_SRGB;
      break;
    case kUcscIgcModeRec709:
      *igc = DRMUcscIgcMode::UCSC_IGC_MODE_REC709;
      break;
    case kUcscIgcModeGamma2_2:
      *igc = DRMUcscIgcMode::UCSC_IGC_MODE_GAMMA2_2;
      break;
    case kUcscIgcModeHlg:
      *igc = DRMUcscIgcMode::UCSC_IGC_MODE_HLG;
      break;
    case kUcscIgcModePq:
      *igc = DRMUcscIgcMode::UCSC_IGC_MODE_PQ;
      break;
    default:
      *igc = DRMUcscIgcMode::UCSC_IGC_MODE_DISABLE;
      DLOGE("Invalid IGC mode");
      break;
  }
}

void HWDeviceDRM::SetUcscGc(const HWUcscGcMode gc_lut_sel, sde_drm::DRMUcscGcMode *gc) {
  switch (gc_lut_sel) {
    case kUcscGcModeSrgb:
      *gc = DRMUcscGcMode::UCSC_GC_MODE_SRGB;
      break;
    case kUcscGcModePq:
      *gc = DRMUcscGcMode::UCSC_GC_MODE_PQ;
      break;
    case kUcscGcModeGamma2_2:
      *gc = DRMUcscGcMode::UCSC_GC_MODE_GAMMA2_2;
      break;
    case kUcscGcModeHlg:
      *gc = DRMUcscGcMode::UCSC_GC_MODE_HLG;
      break;
    default:
      *gc = DRMUcscGcMode::UCSC_GC_MODE_DISABLE;
      DLOGE("Invalid GC mode");
      break;
  }
}

void HWDeviceDRM::SetUcscCsc(const HWUcscCsc &ucsc_csc, drm_msm_ucsc_csc *csc) {
  uint32_t i = 0;
  csc->cfg_param_0_len = UCSC_CSC_CFG0_PARAM_LEN;
  for (i = 0; i < csc->cfg_param_0_len; i++) {
    csc->cfg_param_0[i] = ucsc_csc.cfg_param_0[i];
    DLOGV_IF(kTagDriverConfig, " UCSC csc[%d] = %lld", i, csc->cfg_param_0[i]);
  }
  csc->cfg_param_1_len = UCSC_CSC_CFG1_PARAM_LEN;
  for (i = 0; i < csc->cfg_param_1_len; i++) {
    csc->cfg_param_1[i] = ucsc_csc.cfg_param_1[i];
  }
}
#endif

void HWDeviceDRM::SetDGMCsc(const HWPipeCscInfo &dgm_csc_info, SDECsc *csc) {
  SetDGMCscV1(dgm_csc_info.csc, &csc->csc_v1);
}

void HWDeviceDRM::SetDGMCscV1(const HWCsc &dgm_csc, sde_drm_csc_v1 *csc_v1) {
  uint32_t i = 0;
  for (i = 0; i < MAX_CSC_MATRIX_COEFF_SIZE; i++) {
    csc_v1->ctm_coeff[i] = dgm_csc.ctm_coeff[i];
    DLOGV_IF(kTagDriverConfig, " DGM csc_v1[%d] = %lld", i, csc_v1->ctm_coeff[i]);
  }
  for (i = 0; i < MAX_CSC_BIAS_SIZE; i++) {
    csc_v1->pre_bias[i] = dgm_csc.pre_bias[i];
    csc_v1->post_bias[i] = dgm_csc.post_bias[i];
  }
  for (i = 0; i < MAX_CSC_CLAMP_SIZE; i++) {
    csc_v1->pre_clamp[i] = dgm_csc.pre_clamp[i];
    csc_v1->post_clamp[i] = dgm_csc.post_clamp[i];
  }
}

void HWDeviceDRM::SetSsppLutFeatures(HWPipeInfo *pipe_info) {
  for (HWPipeTonemapLutInfo &lut_info : pipe_info->lut_info) {
    if (lut_info.op != kNoOp) {
      std::shared_ptr<PPFeatureInfo> feature = lut_info.pay_load;
      if (feature == nullptr) {
        DLOGE("Null Pointer for Op = %d lut type = %d", lut_info.op, lut_info.type);
        continue;
      }
      DRMPPFeatureInfo kernel_params = {};
      std::vector<DRMPPFeatureID> drm_id = {};
      PPBlock pp_block = GetPPBlock(lut_info.type);
      hw_color_mgr_->ToDrmFeatureId(pp_block, feature->feature_id_, &drm_id);
      for (DRMPPFeatureID id : drm_id) {
        if (id >= kPPFeaturesMax) {
          DLOGE("Invalid feature id %d", id);
          continue;
        }
        kernel_params.id = id;
        bool disable = (lut_info.op == kReset);
        DLOGV_IF(kTagDriverConfig, "Lut Type = %d PPBlock = %d Op = %s Disable = %d Feature = %p",
                 lut_info.type, pp_block, (lut_info.op ==kSet) ? "Set" : "Reset", disable,
                 feature.get());
        int ret = hw_color_mgr_->GetDrmFeature(feature.get(), &kernel_params, disable);
        if (!ret) {
          drm_atomic_intf_->Perform(DRMOps::PLANE_SET_POST_PROC, pipe_info->pipe_id,
                                    &kernel_params);
          hw_color_mgr_->FreeDrmFeatureData(&kernel_params);
        } else {
          DLOGE("GetDrmFeature failed for Lut type = %d", lut_info.type);
        }
      }
      drm_id.clear();
    }
  }
}

void HWDeviceDRM::AddDimLayerIfNeeded() {
  if (secure_display_active_ && hw_resource_.secure_disp_blend_stage >= 0) {
    HWSolidfillStage sf = {};
    sf.z_order = UINT32(hw_resource_.secure_disp_blend_stage);
    sf.roi = { 0.0, 0.0, FLOAT(mixer_attributes_.width), FLOAT(mixer_attributes_.height) };
    solid_fills_.clear();
    AddSolidfillStage(sf, 0xffff);
    SetSolidfillStages();
  }

  if (!secure_display_active_) {
    DRMSecurityLevel crtc_security_level = DRMSecurityLevel::SECURE_NON_SECURE;
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_SECURITY_LEVEL, token_.crtc_id, crtc_security_level);
  }
}

DisplayError HWDeviceDRM::NullCommit(bool synchronous, bool retain_planes) {
  DTRACE_SCOPED();
  AddDimLayerIfNeeded();
  drm_atomic_intf_->Perform(DRMOps::NULL_COMMIT_PANEL_FEATURES, 0 /* argument is not used */);
  int ret = drm_atomic_intf_->Commit(synchronous , retain_planes);
  if (ret) {
    DLOGE("failed with error %d, crtc=%u", ret, token_.crtc_id);
    return kErrorHardware;
  }
  DLOGI("Null commit succeeded crtc=%u", token_.crtc_id);

  if (first_null_cycle_)
    first_null_cycle_ = false;

  return kErrorNone;
}

void HWDeviceDRM::DumpConnectorModeInfo() {
  for (uint32_t i = 0; i < (uint32_t)connector_info_.modes.size(); i++) {
    DLOGI("Display %d-%d Mode[%d] Name:%s vref:%d hdisp:%d hsync_s:%d hsync_e:%d htotal:%d " \
          "vdisp:%d vsync_s:%d vsync_e:%d vtotal:%d\n", display_id_, disp_type_, i,
          connector_info_.modes[i].mode.name,
          connector_info_.modes[i].mode.vrefresh, connector_info_.modes[i].mode.hdisplay,
          connector_info_.modes[i].mode.hsync_start, connector_info_.modes[i].mode.hsync_end,
          connector_info_.modes[i].mode.htotal, connector_info_.modes[i].mode.vdisplay,
          connector_info_.modes[i].mode.vsync_start, connector_info_.modes[i].mode.vsync_end,
          connector_info_.modes[i].mode.vtotal);
  }
}

void HWDeviceDRM::ResetROI() {
  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_ROI, token_.crtc_id, 0, nullptr, nullptr);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_ROI, token_.conn_id, 0, nullptr);
}

bool HWDeviceDRM::IsFullFrameUpdate(const HWLayersInfo &hw_layer_info) {
  // Perform Full Frame Update for video mode
  if (connector_info_.modes[current_mode_index_].cur_panel_mode & DRM_MODE_FLAG_VID_MODE_PANEL) {
    return true;
  }

  LayerRect full_frame = {0, 0, FLOAT(mixer_attributes_.width), FLOAT(mixer_attributes_.height)};

  const LayerRect &frame_roi = hw_layer_info.left_frame_roi.at(0);
  // If multiple ROIs are present, then it's not fullscreen update.
  if (hw_layer_info.left_frame_roi.size() > 1 ||
      (IsValid(frame_roi) && !IsCongruent(full_frame, frame_roi))) {
    return false;
  }

  return true;
}

DisplayError HWDeviceDRM::SetBppMode(uint32_t bpp) {
  return kErrorNotSupported;
}

DisplayError HWDeviceDRM::SetDynamicDSIClock(uint64_t bit_clk_rate) {
  return kErrorNotSupported;
}

DisplayError HWDeviceDRM::GetDynamicDSIClock(uint64_t *bit_clk_rate) {
  return kErrorNotSupported;
}

DisplayError HWDeviceDRM::UpdateTransferTime(uint32_t transfer_time) {
  connector_info_.modes[current_mode_index_].transfer_time_us = transfer_time;
  PopulateHWPanelInfo();
  transfer_time_updated_ = transfer_time;
  synchronous_commit_ = true;
  return kErrorNone;
}

DisplayError HWDeviceDRM::SetJitterConfig(uint32_t jitter_type, float value, uint32_t time) {
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_JITTER_CONFIG, token_.conn_id, jitter_type, value,
                            time);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_GET_TRANSFER_TIME, token_.conn_id,
                            &transfer_time_updated_);
  return kErrorNone;
}

void HWDeviceDRM::DumpHWLayers(HWLayersInfo *hw_layers_info) {
  DestScaleInfoMap &dest_scale_info_map = hw_layers_info->dest_scale_info_map;
  uint32_t hw_layer_count = UINT32(hw_layers_info->hw_layers.size());
  std::vector<LayerRect> &left_frame_roi = hw_layers_info->left_frame_roi;
  std::vector<LayerRect> &right_frame_roi = hw_layers_info->right_frame_roi;
  //  DLOGI("HWLayers Stack: layer_count: %d, app_layer_count: %d, gpu_target_index: %d",
  //        hw_layer_count, hw_layers_info->app_layer_count, hw_layers_info->gpu_target_index);
  DLOGI("LayerStackFlags = 0x%" PRIu32 ",  blend_cs = {primaries = %d, transfer = %d}",
         UINT32(hw_layers_info->common_info->flags.flags),
         UINT32(hw_layers_info->common_info->blend_cs.primaries),
         UINT32(hw_layers_info->common_info->blend_cs.transfer));
  for (uint32_t i = 0; i < left_frame_roi.size(); i++) {
    DLOGI("left_frame_roi: x = %d, y = %d, w = %d, h = %d", INT(left_frame_roi[i].left),
        INT(left_frame_roi[i].top), INT(left_frame_roi[i].right), INT(left_frame_roi[i].bottom));
  }
  for (uint32_t i = 0; i < right_frame_roi.size(); i++) {
  DLOGI("right_frame_roi: x = %d, y = %d, w = %d h = %d", INT(right_frame_roi[i].left),
        INT(right_frame_roi[i].top), INT(right_frame_roi[i].right),
        INT(right_frame_roi[i].bottom));
  }

  for (uint32_t i = 0; i < dest_scale_info_map.size(); i++) {
    HWDestScaleInfo *dest_scalar_data = dest_scale_info_map[i];
    if (dest_scalar_data->scale_data.enable.scale) {
      HWScaleData &scale_data = dest_scalar_data->scale_data;
      DLOGI("Dest scalar index %d Mixer WxH %dx%d", i,
            dest_scalar_data->mixer_width, dest_scalar_data->mixer_height);
      DLOGI("Panel ROI [%d, %d, %d, %d]", INT(dest_scalar_data->panel_roi.left),
           INT(dest_scalar_data->panel_roi.top), INT(dest_scalar_data->panel_roi.right),
           INT(dest_scalar_data->panel_roi.bottom));
      DLOGI("Dest scalar Dst WxH %dx%d", scale_data.dst_width, scale_data.dst_height);
    }
  }

  for (uint32_t i = 0; i < hw_layer_count; i++) {
    HWLayerConfig &hw_config = hw_layers_info->config[i];
    HWRotatorSession &hw_rotator_session = hw_config.hw_rotator_session;
    HWSessionConfig &hw_session_config = hw_rotator_session.hw_session_config;
    DLOGI("========================= HW_layer: %d =========================", i);
    DLOGI("src_width = %d, src_height = %d, src_format = %d, src_LayerBufferFlags = 0x%" PRIx32 ,
             hw_layers_info->hw_layers[i].input_buffer.width,
             hw_layers_info->hw_layers[i].input_buffer.height,
             hw_layers_info->hw_layers[i].input_buffer.format,
             hw_layers_info->hw_layers[i].input_buffer.flags.flags);

    if (hw_config.use_inline_rot) {
      DLOGI("rotator = %s, rotation = %d, flip_horizontal = %s, flip_vertical = %s",
            "inline rotator", INT(hw_session_config.transform.rotation),
            hw_session_config.transform.flip_horizontal ? "true" : "false",
            hw_session_config.transform.flip_vertical ? "true" : "false");
    } else if (hw_rotator_session.mode == kRotatorOffline) {
      DLOGI("rotator = %s, rotation = %d, flip_horizontal = %s, flip_vertical = %s",
            "offline rotator", INT(hw_session_config.transform.rotation),
            hw_session_config.transform.flip_horizontal ? "true" : "false",
            hw_session_config.transform.flip_vertical ? "true" : "false");
    }
    if (hw_config.use_solidfill_stage) {
      HWSolidfillStage &hw_solidfill_stage = hw_config.hw_solidfill_stage;
      LayerSolidFill &solid_fill_info = hw_solidfill_stage.solid_fill_info;
      DLOGI("HW Solid fill info: z_order = %d, color = %d", hw_solidfill_stage.z_order,
            hw_solidfill_stage.color);
      DLOGI("bit_depth = %d, red = %d, green = %d, blue = %d, alpha = %d",
            solid_fill_info.bit_depth, solid_fill_info.red, solid_fill_info.green,
            solid_fill_info.blue, solid_fill_info.alpha);
    }
    for (uint32_t count = 0; count < 2; count++) {
      HWPipeInfo &left_pipe = hw_config.left_pipe;
      HWPipeInfo &right_pipe = hw_config.right_pipe;
      HWPipeInfo &pipe_info = (count == 0) ? left_pipe : right_pipe;
      HWScaleData &scale_data = pipe_info.scale_data;
      if (!pipe_info.valid) {
        continue;
      }
      std::string pipe = (count == 0) ? "left_pipe" : "right_pipe";
      DLOGI("pipe = %s, pipe_id = %d, z_order = %d, flags = 0x%X",
           pipe.c_str(), pipe_info.pipe_id, pipe_info.z_order, pipe_info.flags);
      DLOGI("src_rect: x = %d, y = %d, w = %d, h = %d", INT(pipe_info.src_roi.left),
            INT(pipe_info.src_roi.top), INT(pipe_info.src_roi.right - pipe_info.src_roi.left),
            INT(pipe_info.src_roi.bottom - pipe_info.src_roi.top));
      DLOGI("dst_rect: x = %d, y = %d, w = %d, h = %d", INT(pipe_info.dst_roi.left),
            INT(pipe_info.dst_roi.top), INT(pipe_info.dst_roi.right - pipe_info.dst_roi.left),
            INT(pipe_info.dst_roi.bottom - pipe_info.dst_roi.top));
      DLOGI("excl_rect: left = %d, top = %d, right = %d, bottom = %d",
            INT(pipe_info.excl_rect.left), INT(pipe_info.excl_rect.top),
            INT(pipe_info.excl_rect.right), INT(pipe_info.excl_rect.bottom));
      if (scale_data.enable.scale) {
      DLOGI("HWScaleData enable flags: scale = %s, direction_detection = %s, detail_enhance = %s,"
            " dyn_exp_disable = %s, dir45_detection = %s, corner_detection = %s",
            scale_data.enable.scale ? "true" : "false",
            scale_data.enable.direction_detection ? "true" : "false",
            scale_data.enable.detail_enhance ? "true" : "false",
            scale_data.enable.dyn_exp_disable ? "true" : "false",
            scale_data.enable.dir45_detection ? "true" : "false",
            scale_data.enable.corner_detection ? "true" : "false");
      DLOGI("lut_flags: lut_swap = 0x%X, lut_dir_wr = 0x%X, lut_y_cir_wr = 0x%X, "
            "lut_uv_cir_wr = 0x%X, lut_y_sep_wr = 0x%X, lut_uv_sep_wr = 0x%X",
            scale_data.lut_flag.lut_swap, scale_data.lut_flag.lut_dir_wr,
            scale_data.lut_flag.lut_y_cir_wr, scale_data.lut_flag.lut_uv_cir_wr,
            scale_data.lut_flag.lut_y_sep_wr, scale_data.lut_flag.lut_uv_sep_wr);
      DLOGI("dir_lut_idx = %d, y_rgb_cir_lut_idx = %d, uv_cir_lut_idx = %d, "
            "y_rgb_sep_lut_idx = %d, uv_sep_lut_idx = %d", scale_data.dir_lut_idx,
            scale_data.y_rgb_cir_lut_idx, scale_data.uv_cir_lut_idx,
            scale_data.y_rgb_sep_lut_idx, scale_data.uv_sep_lut_idx);
      }
    }
  }
}

DisplayError HWDeviceDRM::SetBlendSpace(const PrimariesTransfer &blend_space) {
  blend_space_ = blend_space;
  return kErrorNone;
}

DisplayError HWDeviceDRM::GetDRMPowerMode(const HWPowerState &power_state,
                                          DRMPowerMode *drm_power_mode) {
  if (!drm_power_mode) {
    return kErrorParameters;
  }

  switch (power_state) {
    case kPowerStateOn:
      *drm_power_mode = DRMPowerMode::ON;
      break;
    case kPowerStateOff:
      *drm_power_mode = DRMPowerMode::OFF;
      break;
    case kPowerStateDoze:
      *drm_power_mode = DRMPowerMode::DOZE;
      break;
    case kPowerStateDozeSuspend:
      *drm_power_mode = DRMPowerMode::DOZE_SUSPEND;
      break;
    default:
      return kErrorParameters;
  }
  return kErrorNone;
}

void HWDeviceDRM::SetTUIState() {
  if (tui_state_ == kTUIStateStart) {
    tui_state_ = kTUIStateInProgress;
  } else if (tui_state_ == kTUIStateEnd) {
    tui_state_ = kTUIStateNone;
  } else if (tui_state_ == kTUIStateInProgress) {
    tui_state_ = kTUIStateNone;
  }
}

void HWDeviceDRM::SetTopologySplit(HWTopology hw_topology, uint32_t *split_number) {
  switch (hw_topology) {
    case kDualLM:
    case kDualLMDSC:
    case kDualLMMerge:
    case kDualLMMergeDSC:
    case kDualLMDSCMerge:
      *split_number = 2;
      break;
    case kQuadLMMerge:
    case kQuadLMDSCMerge:
    case kQuadLMMergeDSC:
    case kQuadLMDSC4HSMerge:
      *split_number = 4;
      break;
    case kPPSplit:
    default:
      *split_number = 1;
  }
}

uint64_t HWDeviceDRM::GetSupportedBitClkRate(uint32_t new_mode_index,
                                uint64_t bit_clk_rate_request) {
  if (current_mode_index_ == new_mode_index) {
    uint32_t submode_idx = connector_info_.modes[current_mode_index_].curr_submode_index;
    sde_drm::DRMSubModeInfo curr_sub_mode =
                         connector_info_.modes[current_mode_index_].sub_modes[submode_idx];
    if ((std::find(curr_sub_mode.dyn_bitclk_list.begin(), curr_sub_mode.dyn_bitclk_list.end(),
         bit_clk_rate_request) != curr_sub_mode.dyn_bitclk_list.end())) {
      return bit_clk_rate_request;
    } else {
      DLOGW("Requested rate not supported: %" PRIu64, bit_clk_rate_request);
      return connector_info_.modes[current_mode_index_].curr_bit_clk_rate;
    }
  }

  uint32_t submode_idx = connector_info_.modes[new_mode_index].curr_submode_index;
  sde_drm::DRMSubModeInfo curr_sub_mode =
                       connector_info_.modes[new_mode_index].sub_modes[submode_idx];
  if ((std::find(curr_sub_mode.dyn_bitclk_list.begin(), curr_sub_mode.dyn_bitclk_list.end(),
       bit_clk_rate_request) != curr_sub_mode.dyn_bitclk_list.end())) {
    return bit_clk_rate_request;
  } else {
    DLOGW("Requested rate not supported: %" PRIu64, bit_clk_rate_request);
    return connector_info_.modes[new_mode_index].default_bit_clk_rate;
  }
}

bool HWDeviceDRM::SetupConcurrentWriteback(const HWLayersInfo &hw_layer_info, bool validate,
                                           int64_t *release_fence_fd) {
  bool enable = hw_resource_.has_concurrent_writeback && hw_layer_info.output_buffer &&
                (hw_layer_info.cwb_id != -1) && !pending_cwb_teardown_;
  if (!(enable || cwb_config_[core_id_].enabled)) {  // the frame is neither cwb setup nor cwb teardown frame
    return false;
  }

  bool setup_modes = enable && !cwb_config_[core_id_].enabled;
  // Modes can be setup in prepare or commit path.
  if (setup_modes && (SetupConcurrentWritebackModes(hw_layer_info.cwb_id) == kErrorNone)) {
    cwb_config_[core_id_].enabled = true;
  }

  if (cwb_config_[core_id_].enabled) {
    if (enable) {
      // Set DRM properties for Concurrent Writeback.
      ConfigureConcurrentWriteback(hw_layer_info);

      if (!validate && release_fence_fd) {
        // Set GET_RETIRE_FENCE property to get Concurrent Writeback fence.
        drm_atomic_intf_->Perform(DRMOps::CONNECTOR_GET_RETIRE_FENCE,
                                  cwb_config_[core_id_].token.conn_id,
                                  release_fence_fd);
        return true;
      }
    } else {
      // Tear down the Concurrent Writeback topology.
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, cwb_config_[core_id_].token.conn_id, 0);
      DLOGI("Tear down the Concurrent Writeback topology");
    }
  }

  return false;
}

DisplayError HWDeviceDRM::SetupConcurrentWritebackModes(int32_t writeback_id) {
  // To setup Concurrent Writeback topology, reserve the Connector ID of Virtual display
  if (drm_mgr_intf_->RegisterDisplay(writeback_id, &cwb_config_[core_id_].token)) {
    DLOGW("RegisterDisplay failed for Concurrent Writeback");
    return kErrorResources;
  }

  // Set the modes based on Primary display.
  std::vector<drmModeModeInfo> modes;
  for (auto &item : connector_info_.modes) {
    modes.push_back(item.mode);
  }

  // Inform the mode list to driver.
  struct sde_drm_wb_cfg cwb_cfg = {};
  cwb_cfg.connector_id = cwb_config_[core_id_].token.conn_id;
  cwb_cfg.flags = SDE_DRM_WB_CFG_FLAGS_CONNECTED;
  cwb_cfg.count_modes = UINT32(modes.size());
  cwb_cfg.modes = (uint64_t)modes.data();

  int ret = -EINVAL;
#ifdef DRM_IOCTL_SDE_WB_CONFIG
  ret = drmIoctl(dev_fd_, DRM_IOCTL_SDE_WB_CONFIG, &cwb_cfg);
#endif
  if (ret) {
    drm_mgr_intf_->UnregisterDisplay(&(cwb_config_[core_id_].token));
    DLOGE("Dump CWBConfig: mode_count %d flags %x", cwb_cfg.count_modes, cwb_cfg.flags);
    DumpConnectorModeInfo();
    return kErrorHardware;
  }

  return kErrorNone;
}

void HWDeviceDRM::ConfigureConcurrentWriteback(const HWLayersInfo &hw_layer_info) {
  CwbConfig *cwb_config = hw_layer_info.hw_cwb_config;
  std::shared_ptr<LayerBuffer> output_buffer = hw_layer_info.output_buffer;
  bool fb_modified = false;
  registry_.MapOutputBufferToFbId(output_buffer, &fb_modified);
  uint32_t &vitual_conn_id = cwb_config_[core_id_].token.conn_id;
  sde_drm::DRMWBUsageType cwb_usage = sde_drm::DRMWBUsageType::WB_USAGE_CWB;

  // Set the topology for Concurrent Writeback: [CRTC_PRIMARY_DISPLAY - CONNECTOR_VIRTUAL_DISPLAY].
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, vitual_conn_id, token_.crtc_id);
  // Set WB usage type as CWB
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_WB_USAGE_TYPE, vitual_conn_id, cwb_usage);

  // Set CRTC Capture Mode
  DRMCWbCaptureMode capture_mode = DRMCWbCaptureMode::MIXER_OUT;
  if (cwb_config->tap_point == CwbTapPoint::kDsppTapPoint) {
    capture_mode = DRMCWbCaptureMode::DSPP_OUT;
  } else if (cwb_config->tap_point == CwbTapPoint::kDemuraTapPoint) {
    capture_mode = DRMCWbCaptureMode::DEMURA_OUT;
  }

  drm_atomic_intf_->Perform(DRMOps::CRTC_SET_CAPTURE_MODE, token_.crtc_id, capture_mode);

  // Set Connector Output FB
  uint32_t fb_id = registry_.GetOutputFbId(output_buffer->handle_id);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_OUTPUT_FB_ID, vitual_conn_id, fb_id);

  // Set Connector Secure Mode
  bool secure = output_buffer->flags.secure;
  DRMSecureMode mode = secure ? DRMSecureMode::SECURE : DRMSecureMode::NON_SECURE;
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_FB_SECURE_MODE, vitual_conn_id, mode);

  // Set Connector Output Rect
  sde_drm::DRMRect full_frame = {};
  full_frame.left = 0;
  full_frame.top = 0;

  if (capture_mode == DRMCWbCaptureMode::MIXER_OUT) {
    full_frame.right = mixer_attributes_.width;
    full_frame.bottom = mixer_attributes_.height;
  } else {
    full_frame.right = display_attributes_[current_mode_index_].x_pixels;
    full_frame.bottom = display_attributes_[current_mode_index_].y_pixels;
  }

  sde_drm::DRMRect cwb_dst = full_frame;
  LayerRect cwb_roi = cwb_config->cwb_roi;

  if (has_cwb_crop_) {  // If CWB ROI feature is supported, then set WB connector's roi_v1 property
    // to PU ROI and DST_* properties to CWB ROI. Else, set DST_* properties to full frame ROI.
    bool is_full_frame_update = IsFullFrameUpdate(hw_layer_info);
    // Set WB connector's roi_v1 property to PU_ROI.
    if (is_full_frame_update) {
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_ROI, vitual_conn_id, 0, nullptr);
      DLOGV_IF(kTagDriverConfig, "roi_v1 of virtual connector is set NULL (Full Frame update).");
    } else {
      const int kNumMaxROIs = 4;
      sde_drm::DRMRect conn_rects[kNumMaxROIs] = {full_frame};
      for (uint32_t i = 0; i < hw_layer_info.left_frame_roi.size(); i++) {
        auto &roi = hw_layer_info.left_frame_roi.at(i);
        conn_rects[i].left = UINT32(roi.left);
        conn_rects[i].right = UINT32(roi.right);
        conn_rects[i].top = UINT32(roi.top);
        conn_rects[i].bottom = UINT32(roi.bottom);
      }
      uint32_t num_rects = std::max(1u, UINT32(hw_layer_info.left_frame_roi.size()));
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_ROI, vitual_conn_id, num_rects, conn_rects);
    }

    // Set WB connector's DST_* property to CWB_ROI.
    if (!is_full_frame_update || !cwb_config->pu_as_cwb_roi) {
      // Either incase of partial update or when client requests only its specified ROI to be set
      // as CWB ROI without including PU ROI in it (by setting pu_as_cwb_roi flag as false), then
      // set DST_* properties to aligned client specified ROI. This if condition is an optimization
      // check. Either we can keep this if with both the conditions or we can remove the if at all
      // with direct setting of the properties. This if condition prevents reseting cwb_dst to full
      // frame incase of full frame PU ROI with client requesting PU ROI to be included in CWB ROI.
      cwb_dst.left = UINT32(cwb_roi.left);
      cwb_dst.right = UINT32(cwb_roi.right);
      cwb_dst.top = UINT32(cwb_roi.top);
      cwb_dst.bottom = UINT32(cwb_roi.bottom);
    }
  }

  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_OUTPUT_RECT, vitual_conn_id, cwb_dst);
  ConfigureCWBDither(cwb_config->dither_info, vitual_conn_id, capture_mode);

  DLOGV_IF(kTagDriverConfig, "CWB Mode:%d roi.left:%u roi.top:%u roi.right:%u roi.bottom:%u",
           capture_mode, cwb_dst.left, cwb_dst.top, cwb_dst.right, cwb_dst.bottom);
}

DisplayError HWDeviceDRM::TeardownConcurrentWriteback(void) {
  if (cwb_config_[core_id_].enabled) {
    drm_mgr_intf_->UnregisterDisplay(&(cwb_config_[core_id_].token));
    cwb_config_[core_id_].enabled = false;
    registry_.Clear();
  }

  return kErrorNone;
}

void HWDeviceDRM::PostCommitConcurrentWriteback(std::shared_ptr<LayerBuffer> output_buffer) {
  if (hw_resource_.has_concurrent_writeback && output_buffer) {
    return;
  }

  TeardownConcurrentWriteback();
}

DisplayError HWDeviceDRM::GetFeatureSupportStatus(const HWFeature feature, uint32_t *status) {
  DisplayError error = kErrorNone;

  if (!status) {
    return kErrorParameters;
  }

  switch (feature) {
    case kAllowedModeSwitch:
      *status = connector_info_.modes[current_mode_index_].allowed_mode_switch;
      break;
    case kHasCwbCrop:
      *status = UINT32(has_cwb_crop_);
      break;
    case kHasDedicatedCwb:
      *status = UINT32(has_dedicated_cwb_);
      break;
    case kMaxSupportedCwb:
      *status = max_cwb_;
      break;
    default:
      DLOGW("Unable to get status of feature : %d", feature);
      error = kErrorParameters;
      break;
  }

  return error;
}

void HWDeviceDRM::FlushConcurrentWriteback() {
  TeardownConcurrentWriteback();
  DLOGI("Flushing out CWB Config. cwb_enabled = %d , cwb_disp_id : %d", cwb_config_[core_id_].enabled,
        display_id_);
}

DisplayError HWDeviceDRM::ConfigureCWBDither(void *payload, uint32_t conn_id,
                sde_drm::DRMCWbCaptureMode mode) {
  int ret = 0;
  DRMPPFeatureInfo kernel_params = {};

  if (has_cwb_dither_) {
    kernel_params.id = DRMPPFeatureID::kFeatureCWBDither;
  } else {
    kernel_params.id = DRMPPFeatureID::kFeatureDither;
    DLOGI_IF(kTagDriverConfig, "On display %d-%d pp cwb blocks not supported, to"
             " config pp-dither hw", display_id_, disp_type_);
  }

  if (payload && (mode == sde_drm::DRMCWbCaptureMode::DSPP_OUT ||
                  mode == sde_drm::DRMCWbCaptureMode::DEMURA_OUT)) {
    PPFeatureInfo * feature_wrapper = reinterpret_cast<PPFeatureInfo *>(payload);
    ret = hw_color_mgr_->GetDrmFeature(feature_wrapper, &kernel_params);
    if (ret)
      DLOGE("Failed to get drm feature %d params on display %d-%d, ret %d", kernel_params.id,
            display_id_, disp_type_,  ret);
  } else {
    kernel_params.type = sde_drm::kPropBlob;
    kernel_params.payload = NULL;
  }

  DLOGI_IF(kTagDriverConfig, "On display %d-%d CWB dither %s case", display_id_, disp_type_,
           kernel_params.payload ? "enable" : "disable");
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POST_PROC, conn_id, &kernel_params);
  hw_color_mgr_->FreeDrmFeatureData(&kernel_params);
  return kErrorNone;
}

DisplayError HWDeviceDRM::GetPanelBlMaxLvl(uint32_t *bl_max) {
  if (!bl_max) {
    DLOGE("Display %d-%d Invalid input param", display_id_, disp_type_);
    return kErrorParameters;
  }

  *bl_max = connector_info_.max_panel_backlight;
  return kErrorNone;
}

DisplayError HWDeviceDRM::SetPPConfig(void *payload, size_t size) {
  if (!payload || size != sizeof(DRMPPFeatureInfo)) {
    DLOGE("Invalid input params on display %d-%d payload %pK, size %zd expect size %zd",
          display_id_, disp_type_, payload, size, sizeof(DRMPPFeatureInfo));
      return kErrorParameters;
  }

  struct DRMPPFeatureInfo *info = reinterpret_cast<struct DRMPPFeatureInfo *> (payload);

  if (info->object_type == DRM_MODE_OBJECT_CONNECTOR && token_.conn_id) {
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_POST_PROC, token_.conn_id, payload);
  } else if (info->object_type == DRM_MODE_OBJECT_CRTC && token_.crtc_id) {
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_POST_PROC, token_.crtc_id, payload);
  } else {
    DLOGE("Invalid feature input, obj_type: 0x%x , feature_id: %d, event_type: 0x%x",
          info->object_type, info->id, info->event_type);
    return kErrorParameters;
  }

  return kErrorNone;
}

DisplayError HWDeviceDRM::CancelDeferredPowerMode() {
  DLOGI("Pending state reset %d on CRTC: %u", pending_power_state_, token_.crtc_id);
  pending_power_state_ = kPowerStateNone;

  return kErrorNone;
}

void HWDeviceDRM::HandleCwbTeardown(bool sync_teardown) {
  if (!cwb_config_[core_id_].enabled) {
    return;
  }

  DLOGI("Pending CWB teardown on CRTC: %u", token_.crtc_id);
  pending_cwb_teardown_ = true;
  if (sync_teardown) {
    // This perform call is just used to tear down CWB topology in case of single
    // threaded (sync) call for tear down to avoid blocking of sdm service (or screen freeze for
    // single thread execution), but it will execute in next cycle for complete tear down.
    // TODO(user): This may cause WB frame drop in next cycle for the display, which wants to
    // use it for a particular usage. If there is no any chance of synchronous call for tear down,
    // then it can be removed.
    drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, cwb_config_[core_id_].token.conn_id, 0);
    TeardownConcurrentWriteback();
  }
}

DisplayError HWDeviceDRM::NotifyExpectedPresent(uint64_t expected_present_time,
                                                uint32_t frame_interval_ns) {
#ifdef DRM_IOCTL_MSM_EARLY_EPT
  int ret = -1;
  struct drm_msm_display_early_ept early_ept_cfg = {};
  early_ept_cfg.connector_id = token_.conn_id;
  early_ept_cfg.flags = DRM_MSM_EARLY_EPT;
  early_ept_cfg.ept_ns = expected_present_time;
  early_ept_cfg.frame_interval = frame_interval_ns;
  ret = drmIoctl(dev_fd_, DRM_IOCTL_MSM_EARLY_EPT, &early_ept_cfg);
  if (ret < 0) {
    return kErrorHardware;
  }
#endif
  return kErrorNone;
}

}  // namespace sdm
