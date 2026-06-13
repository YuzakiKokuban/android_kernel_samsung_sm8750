/*
 * Copyright (c) 2015 - 2018, 2020-2021, The Linux Foundation. All rights
 * reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <core/buffer_sync_handler.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include "concurrency_mgr.h"
#include "sdm_debugger.h"
#include <core/buffer_allocator.h>
#include <private/color_params.h>
#include <utils/constants.h>
#include <utils/debug.h>

#define __CLASS__ "SDMColorManager"

namespace sdm {

uint32_t
SDMColorManager::Get8BitsARGBColorValue(const PPColorFillParams &params) {
  uint32_t argb_color = ((params.color.r << 16) & 0xff0000) |
                        ((params.color.g << 8) & 0xff00) |
                        ((params.color.b) & 0xff);
  return argb_color;
}

DisplayError
SDMColorManager::CreatePayloadFromParcel(SDMParcel *in, uint32_t *disp_id,
                                         PPDisplayAPIPayload *sink) {
  DisplayError ret = kErrorNone;
  uint32_t id(0);
  uint32_t size(0);

  id = UINT32(in->readInt32());
  size = UINT32(in->readInt32());
  if (size > 0 && size == in->dataAvail()) {
    const void *data = in->readInplace(size);
    const uint8_t *temp = reinterpret_cast<const uint8_t *>(data);

    sink->size = size;
    sink->payload = const_cast<uint8_t *>(temp);
    *disp_id = id;
  } else {
    DLOGW("Failing size checking, size = %d", size);
    ret = kErrorNotSupported;
  }

  return ret;
}

void SDMColorManager::MarshallStructIntoParcel(const PPDisplayAPIPayload &data,
                                               SDMParcel *out_parcel) {
  if (data.fd > 0) {
    int err = out_parcel->writeDupFileDescriptor(data.fd);
    if (err) {
      DLOGE("writeDupFileDescriptor status = %d", err);
    }
    close(data.fd);
  }

  out_parcel->writeInt32(INT32(data.size));
  if (data.payload)
    out_parcel->write(data.payload, data.size);
}

SDMColorManager *SDMColorManager::CreateColorManager(BufferAllocator *buffer_allocator,
                                                     SocketHandler *socket_handler) {
  SDMColorManager *color_mgr = new SDMColorManager(buffer_allocator, socket_handler);

  if (color_mgr) {
    // Load display API interface library. And retrieve color API function
    // tables.
    DynLib &color_apis_lib = color_mgr->color_apis_lib_;
    if (color_apis_lib.Open(DISPLAY_API_INTERFACE_LIBRARY_NAME)) {
      if (!color_apis_lib.Sym(DISPLAY_API_FUNC_TABLES,
                              &color_mgr->color_apis_)) {
        DLOGE("Fail to retrieve = %s from %s", DISPLAY_API_FUNC_TABLES,
              DISPLAY_API_INTERFACE_LIBRARY_NAME);
        delete color_mgr;
        return NULL;
      }
    } else {
      DLOGW("Unable to load = %s", DISPLAY_API_INTERFACE_LIBRARY_NAME);
      delete color_mgr;
      return NULL;
    }
    DLOGI("Successfully loaded %s", DISPLAY_API_INTERFACE_LIBRARY_NAME);

    // Load diagclient library and invokes its entry point to pass in display
    // APIs.
    DynLib &diag_client_lib = color_mgr->diag_client_lib_;
    if (diag_client_lib.Open(QDCM_DIAG_CLIENT_LIBRARY_NAME)) {
      if (!diag_client_lib.Sym(
              INIT_QDCM_DIAG_CLIENT_NAME,
              reinterpret_cast<void **>(&color_mgr->qdcm_diag_init_)) ||
          !diag_client_lib.Sym(
              DEINIT_QDCM_DIAG_CLIENT_NAME,
              reinterpret_cast<void **>(&color_mgr->qdcm_diag_deinit_))) {
        DLOGE("Fail to retrieve = %s from %s", INIT_QDCM_DIAG_CLIENT_NAME,
              QDCM_DIAG_CLIENT_LIBRARY_NAME);
      } else {
        // invoke Diag Client entry point to initialize.
        color_mgr->qdcm_diag_init_(color_mgr->color_apis_);
        DLOGI("Successfully loaded %s and %s and diag_init'ed",
              DISPLAY_API_INTERFACE_LIBRARY_NAME,
              QDCM_DIAG_CLIENT_LIBRARY_NAME);
      }
    } else {
      DLOGW("Unable to load = %s", QDCM_DIAG_CLIENT_LIBRARY_NAME);
      // only QDCM Diag client failed to be loaded and system still should
      // function.
    }
  } else {
    DLOGE("Unable to create SDMColorManager");
    return NULL;
  }

  return color_mgr;
}

SDMColorManager::SDMColorManager(BufferAllocator *buffer_allocator, SocketHandler *socket_handler)
    : buffer_allocator_(buffer_allocator), socket_handler_(socket_handler) {}

SDMColorManager::~SDMColorManager() {}

void SDMColorManager::DestroyColorManager() {
  if (qdcm_mode_mgr_) {
    delete qdcm_mode_mgr_;
  }
  if (qdcm_diag_deinit_) {
    qdcm_diag_deinit_();
  }
  delete this;
}

DisplayError SDMColorManager::EnableQDCMMode(bool enable,
                                             SDMDisplay *sdm_display) {
  DisplayError ret = kErrorNone;

  if (!qdcm_mode_mgr_) {
    qdcm_mode_mgr_ = SDMQDCMModeManager::CreateQDCMModeMgr(socket_handler_);
    if (!qdcm_mode_mgr_) {
      DLOGE("Unable to create QDCM operating mode manager.");
      ret = kErrorNotSupported;
    }
  }

  if (qdcm_mode_mgr_) {
    ret = qdcm_mode_mgr_->EnableQDCMMode(enable, sdm_display);
  }

  return ret;
}

DisplayError SDMColorManager::SetSolidFill(const void *params, bool enable,
                                           SDMDisplay *sdm_display) {
  SCOPE_LOCK(locker_);
  LayerSolidFill solid_fill_color;

  if (params) {
    solid_fill_params_ = *reinterpret_cast<const PPColorFillParams *>(params);
  } else {
    solid_fill_params_ = PPColorFillParams();
  }

  if (solid_fill_params_.color.r_bitdepth !=
          solid_fill_params_.color.b_bitdepth ||
      solid_fill_params_.color.r_bitdepth !=
          solid_fill_params_.color.g_bitdepth) {
    DLOGE("invalid bit depth r %d g %d b %d",
          solid_fill_params_.color.r_bitdepth,
          solid_fill_params_.color.g_bitdepth,
          solid_fill_params_.color.b_bitdepth);
    return kErrorNotSupported;
  }

  solid_fill_color.bit_depth = solid_fill_params_.color.r_bitdepth;
  solid_fill_color.red = solid_fill_params_.color.r;
  solid_fill_color.blue = solid_fill_params_.color.b;
  solid_fill_color.green = solid_fill_params_.color.g;
  solid_fill_color.alpha = 0xffff;

  if (enable) {
    LayerRect solid_fill_rect = {
        FLOAT(solid_fill_params_.rect.x),
        FLOAT(solid_fill_params_.rect.y),
        FLOAT(solid_fill_params_.rect.x) + FLOAT(solid_fill_params_.rect.width),
        FLOAT(solid_fill_params_.rect.y) +
            FLOAT(solid_fill_params_.rect.height),
    };

    sdm_display->Perform(SDMBuiltInDisplayOps::SET_QDCM_SOLID_FILL_INFO,
                         &solid_fill_color);
    sdm_display->Perform(SDMBuiltInDisplayOps::SET_QDCM_SOLID_FILL_RECT,
                         &solid_fill_rect);
  } else {
    solid_fill_color.red = 0;
    solid_fill_color.blue = 0;
    solid_fill_color.green = 0;
    solid_fill_color.alpha = 0;
    sdm_display->Perform(SDMBuiltInDisplayOps::UNSET_QDCM_SOLID_FILL_INFO,
                         &solid_fill_color);
  }

  return kErrorNone;
}

DisplayError SDMColorManager::SetFrameCapture(void *params, bool enable,
                                              SDMDisplay *sdm_display) {
  SCOPE_LOCK(locker_);
  DisplayError ret = kErrorNone;

  PPFrameCaptureData *frame_capture_data =
      reinterpret_cast<PPFrameCaptureData *>(params);

  if (enable) {
    std::memset(&buffer_info, 0x00, sizeof(buffer_info));

    CwbTapPoint cwb_tappoint = CwbTapPoint::kLmTapPoint;
    // frame_capture_data->input_params.flags == 0x0 => DSPP tappoint
    // frame_capture_data->input_params.flags == 0x1 => LM tappoint
    // frame_capture_data->input_params.flags == 0x2 => DEMURA tappoint
    switch (frame_capture_data->input_params.flags) {
    case 0x0: // DSPP mode
      cwb_tappoint = CwbTapPoint::kDsppTapPoint;
      break;
    case 0x1: // Layer mixer mode
      cwb_tappoint = CwbTapPoint::kLmTapPoint;
      break;
    case 0x2: // Demura mode
      cwb_tappoint = CwbTapPoint::kDemuraTapPoint;
      break;
    default:
      DLOGE("Tapppoint %d NOT supported.",
            frame_capture_data->input_params.flags);
      return kErrorNotSupported;
    }

    CwbConfig cwb_config = {};
    cwb_config.tap_point = cwb_tappoint;
    cwb_config.cwb_roi.left = FLOAT(frame_capture_data->input_params.rect.x);
    cwb_config.cwb_roi.top = FLOAT(frame_capture_data->input_params.rect.y);
    cwb_config.cwb_roi.right =
        cwb_config.cwb_roi.left +
        FLOAT(frame_capture_data->input_params.rect.width);
    cwb_config.cwb_roi.bottom =
        cwb_config.cwb_roi.top +
        FLOAT(frame_capture_data->input_params.rect.height);

    ret = sdm_display->GetCwbBufferResolution(
        &cwb_config, &buffer_info.buffer_config.width,
        &buffer_info.buffer_config.height);
    if (ret != 0) {
      DLOGE("Buffer Resolution setting failed. ret: %d", ret);
      return ret;
    }

    if (frame_capture_data->input_params.out_pix_format ==
        PP_PIXEL_FORMAT_RGB_888) {
      buffer_info.buffer_config.format = kFormatRGB888;
    } else if (frame_capture_data->input_params.out_pix_format ==
               PP_PIXEL_FORMAT_RGB_2101010) {
      buffer_info.buffer_config.format = kFormatRGBA1010102;
    } else {
      DLOGE("Pixel-format: %d NOT support.",
            frame_capture_data->input_params.out_pix_format);
      return kErrorNotSupported;
    }

    buffer_info.buffer_config.buffer_count = 1;
    buffer_info.alloc_buffer_info.fd = -1;
    buffer_info.alloc_buffer_info.stride = 0;
    buffer_info.alloc_buffer_info.size = 0;

    int err = buffer_allocator_->AllocateBuffer(&buffer_info);
    if (err != 0) {
      DLOGE("Buffer allocation failed. ret: %d", err);
      return kErrorMemory;
    } else {
      void *buffer =
          mmap(NULL, buffer_info.alloc_buffer_info.size, PROT_READ | PROT_WRITE,
               MAP_SHARED, buffer_info.alloc_buffer_info.fd, 0);

      if (buffer == MAP_FAILED) {
        DLOGE("mmap failed. err = %d", errno);
        frame_capture_data->buffer = NULL;
        buffer_allocator_->FreeBuffer(&buffer_info);
        return kErrorNotSupported;
      } else {
        frame_capture_data->buffer = reinterpret_cast<uint8_t *>(buffer);
        frame_capture_data->buffer_stride =
            buffer_info.alloc_buffer_info.stride;
        frame_capture_data->buffer_size = buffer_info.alloc_buffer_info.size;
      }

      ret = sdm_display->FrameCaptureAsync(buffer_info, cwb_config);
      if (ret != kErrorNone) {
        DLOGE("FrameCaptureAsync failed. ret = %d", ret);
      }
    }
  } else {
    ret = sdm_display->GetFrameCaptureStatus();
    if (ret == kErrorNone) {
      if (frame_capture_data->buffer != NULL) {
        if (munmap(frame_capture_data->buffer,
                   buffer_info.alloc_buffer_info.size) != 0) {
          DLOGE("munmap failed. err = %d", errno);
        }
      }

      if (frame_capture_data->input_params.dither_payload) {
        DLOGV_IF(kTagQDCM, "free cwb dither data");
        delete frame_capture_data->input_params.dither_payload;
        frame_capture_data->input_params.dither_payload = nullptr;
      }
      frame_capture_data->input_params.dither_flags = 0x0;

      if (buffer_allocator_ != NULL) {
        std::memset(frame_capture_data, 0x00, sizeof(PPFrameCaptureData));
        int err = buffer_allocator_->FreeBuffer(&buffer_info);
        if (err != 0) {
          DLOGE("FreeBuffer failed. ret = %d", err);
        }
      }
    } else {
      DLOGE("GetFrameCaptureStatus failed. ret = %d", ret);
    }
  }
  return ret;
}

DisplayError
SDMColorManager::SetHWDetailedEnhancerConfig(void *params,
                                             SDMDisplay *sdm_display) {
  DisplayError err = kErrorNone;
  if (sdm_display) {
    // Move DE config converting to sdm_display. Here to send tuning params.
    err = sdm_display->SetHWDetailedEnhancerConfig(params);
    if (err) {
      DLOGW("SetDetailEnhancerConfig failed. err = %d", err);
    }
  }
  return err;
}

DisplayError SDMColorManager::SetDetailedEnhancer(void *params,
                                                  SDMDisplay *sdm_display) {
  SCOPE_LOCK(locker_);
  DisplayError err = kErrorNone;
  err = SetHWDetailedEnhancerConfig(params, sdm_display);
  return err;
}

const SDMQDCMModeManager::ActiveFeatureCMD
    SDMQDCMModeManager::kActiveFeatureCMD[] = {
        SDMQDCMModeManager::ActiveFeatureCMD("cabl:on", "cabl:off",
                                             "cabl:status", "running"),
        SDMQDCMModeManager::ActiveFeatureCMD("ad:on", "ad:off",
                                             "ad:query:status", "running"),
        SDMQDCMModeManager::ActiveFeatureCMD("svi:on", "svi:off", "svi:status",
                                             "running"),
};

const char *const SDMQDCMModeManager::kSocketName = "pps";
const char *const SDMQDCMModeManager::kTagName = "surfaceflinger";
const char *const SDMQDCMModeManager::kPackageName = "colormanager";

SDMQDCMModeManager *SDMQDCMModeManager::CreateQDCMModeMgr(SocketHandler *socket_handler) {
  SDMQDCMModeManager *mode_mgr = new SDMQDCMModeManager();

  if (!mode_mgr) {
    DLOGW("No memory to create SDMQDCMModeManager.");
    return NULL;
  } else {
    mode_mgr->socket_fd_ = socket_handler->GetSocketFd(kDpps);
    if (mode_mgr->socket_fd_ < 0) {
      // it should not be disastrous and we still can grab wakelock in QDCM
      // mode.
      DLOGW("Unable to connect to dpps socket!");
    }

    // retrieve system GPU idle timeout value for later to recover.
    mode_mgr->entry_timeout_ = UINT32(SDMDebugHandler::GetIdleTimeoutMs());
  }

  return mode_mgr;
}

SDMQDCMModeManager::~SDMQDCMModeManager() {
  if (socket_fd_ >= 0)
    ::close(socket_fd_);
}

DisplayError SDMQDCMModeManager::EnableActiveFeatures(
    bool enable, const SDMQDCMModeManager::ActiveFeatureCMD &cmds,
    bool *was_running) {
  DisplayError ret = kErrorNone;
  ssize_t size = 0;
  char response[kSocketCMDMaxLength] = {
      0,
  };

  if (socket_fd_ < 0) {
    DLOGW("No socket connection available - assuming dpps is not enabled");
    return kErrorNone;
  }

  if (!enable) { // if client requesting to disable it.
    // query CABL status, if off, no action. keep the status.
    size = ::write(socket_fd_, cmds.cmd_query_status,
                   strlen(cmds.cmd_query_status));
    if (size < 0) {
      DLOGW("Unable to send data over socket %s", ::strerror(errno));
      ret = kErrorNotSupported;
    } else {
      size = ::read(socket_fd_, response, kSocketCMDMaxLength);
      if (size < 0) {
        DLOGW("Unable to read data over socket %s", ::strerror(errno));
        ret = kErrorNotSupported;
      } else if (!strncmp(response, cmds.running, strlen(cmds.running))) {
        *was_running = true;
      }
    }

    if (*was_running) { // if was running, it's requested to disable it.
      size = ::write(socket_fd_, cmds.cmd_off, strlen(cmds.cmd_off));
      if (size < 0) {
        DLOGW("Unable to send data over socket %s", ::strerror(errno));
        ret = kErrorNotSupported;
      }
    }
  } else { // if was running, need enable it back.
    if (*was_running) {
      size = ::write(socket_fd_, cmds.cmd_on, strlen(cmds.cmd_on));
      if (size < 0) {
        DLOGW("Unable to send data over socket %s", ::strerror(errno));
        ret = kErrorNotSupported;
      }
    }
  }

  return ret;
}

DisplayError SDMQDCMModeManager::EnableQDCMMode(bool enable,
                                                SDMDisplay *sdm_display) {
  DisplayError ret = kErrorNone;

  ret =
      EnableActiveFeatures((enable ? false : true),
                           kActiveFeatureCMD[kCABLFeature], &cabl_was_running_);

  // if enter QDCM mode, disable GPU fallback idle timeout.
  if (sdm_display) {
    int inactive_ms = IDLE_TIMEOUT_INACTIVE_MS;
    Debug::Get()->GetProperty(IDLE_TIME_INACTIVE_PROP, &inactive_ms);
    uint32_t timeout = enable ? 0 : entry_timeout_;
    sdm_display->SetIdleTimeoutMs(timeout, inactive_ms);
  }

  return ret;
}

} // namespace sdm
