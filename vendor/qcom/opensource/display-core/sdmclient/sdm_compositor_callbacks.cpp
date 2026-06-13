/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "sdm_compositor_callbacks.h"

#include <debug_handler.h>

#define __CLASS__ "SDMCompositorCallbacks"

namespace sdm {

void SDMCompositorCallbacks::RegisterCallback(SDMCompositorCbIntf *cb, bool enable) {
  if (!enable || !cb) {
    callbacks_ = nullptr;
    return;
  } else {
    callbacks_ = cb;
  }
}

void SDMCompositorCallbacks::RegisterSideband(SDMSideBandCompositorCbIntf *cb, bool enable) {
  if (!enable || !cb) {
    sideband_ = nullptr;
  } else {
    sideband_ = cb;
  }
}

void SDMCompositorCallbacks::OnHotplug(uint64_t display, bool connected) {
  if (!callbacks_) {
    DLOGW("Callbacks interface is not initialized!");
    return;
  }

  callbacks_->OnHotplug(display, connected);
}

void SDMCompositorCallbacks::OnRefresh(uint64_t display) {
  if (!callbacks_) {
    DLOGW("Callbacks interface is not initialized!");
    return;
  }

  callbacks_->OnRefresh(display);
}

void SDMCompositorCallbacks::OnVsync(uint64_t display, int64_t timestamp,
                                     int32_t vsync_period_nanos) {
  if (!callbacks_) {
    DLOGW("Callbacks interface is not initialized!");
    return;
  }

  callbacks_->OnVsync(display, timestamp, vsync_period_nanos);
}

void SDMCompositorCallbacks::OnSeamlessPossible(uint64_t display) {
  if (!callbacks_) {
    DLOGW("Callbacks interface is not initialized!");
    return;
  }

  callbacks_->OnSeamlessPossible(display);
}

void SDMCompositorCallbacks::OnVsyncIdle(uint64_t display) {
  if (!callbacks_) {
    DLOGW("Callbacks interface is not initialized!");
    return;
  }

  callbacks_->OnVsyncIdle(display);
}

void SDMCompositorCallbacks::OnVsyncPeriodTimingChanged(uint64_t display,
                                                        SDMVsyncPeriodChangeTimeline &timeline) {
  if (!callbacks_) {
    DLOGW("Callbacks interface is not initialized!");
    return;
  }

  callbacks_->OnVsyncPeriodTimingChanged(display, timeline);
}

// sideband callbacks
void SDMCompositorCallbacks::NotifyQsyncChange(uint64_t display_id, bool qsync_enabled,
                                               uint32_t refresh_rate, uint32_t qsync_refresh_rate) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initialized!");
    return;
  }

  sideband_->NotifyQsyncChange(display_id, qsync_enabled, refresh_rate, qsync_refresh_rate);
}
void SDMCompositorCallbacks::NotifyCameraSmoothInfo(SDMCameraSmoothOp op, int32_t fps) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->NotifyCameraSmoothInfo(op, fps);
}

void SDMCompositorCallbacks::NotifyResolutionChange(uint64_t display_id,
                                                    SDMConfigAttributes &attr) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->NotifyResolutionChange(display_id, attr);
}

void SDMCompositorCallbacks::NotifyTUIEventDone(uint32_t ret, uint32_t disp_id,
                                                SDMTUIEventType type) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->NotifyTUIEventDone(ret, disp_id, type);
}

void SDMCompositorCallbacks::NotifyIdleStatus(bool status) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->NotifyIdleStatus(status);
}

void SDMCompositorCallbacks::NotifyCWBStatus(int32_t status, void *buffer) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->NotifyCWBStatus(status, buffer);
}

void SDMCompositorCallbacks::NotifyContentFps(const std::string &name, int32_t fps) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initialized!");
    return;
  }

  sideband_->NotifyContentFps(name, fps);
}

void SDMCompositorCallbacks::OnHdmiHotplug(bool connected) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->OnHdmiHotplug(connected);
}

int SDMCompositorCallbacks::GetDemuraFilePaths(const GenericPayload &in, GenericPayload *out) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return -1;
  }

  return sideband_->GetDemuraFilePaths(in, out);
}

void SDMCompositorCallbacks::OnCECMessageReceived(char *message, int len) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->OnCECMessageReceived(message, len);
}

// gl color convert callbacks
void SDMCompositorCallbacks::InitColorConvert(uint64_t display, bool secure) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->InitColorConvert(display, secure);
}

void SDMCompositorCallbacks::ColorConvertBlit(uint64_t display, ColorConvertBlitContext *ctx) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->ColorConvertBlit(display, ctx);
}

void SDMCompositorCallbacks::ResetColorConvert(uint64_t display) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->ResetColorConvert(display);
}

void SDMCompositorCallbacks::DestroyColorConvert(uint64_t display) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->DestroyColorConvert(display);
}

// Histogram callbacks
void SDMCompositorCallbacks::StartHistogram(uint64_t display, int max_frames) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->StartHistogram(display, max_frames);
}

void SDMCompositorCallbacks::StopHistogram(uint64_t display, bool teardown) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->StopHistogram(display, teardown);
}

void SDMCompositorCallbacks::NotifyHistogram(uint64_t display, int fd, uint64_t blob_id,
                                             uint32_t panel_width, uint32_t panel_height) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->NotifyHistogram(display, fd, blob_id, panel_width, panel_height);
}

std::string SDMCompositorCallbacks::DumpHistogram(uint64_t display) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return "";
  }

  return sideband_->DumpHistogram(display);
}

void SDMCompositorCallbacks::CollectHistogram(uint64_t display, uint64_t max_frames,
                                              uint64_t timestamp,
                                              int32_t samples_size[NUM_HISTOGRAM_COLOR_COMPONENTS],
                                              uint64_t *samples[NUM_HISTOGRAM_COLOR_COMPONENTS],
                                              uint64_t *numFrames) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->CollectHistogram(display, max_frames, timestamp, samples_size, samples, numFrames);
}
DisplayError SDMCompositorCallbacks::GetHistogramAttributes(uint64_t display, int32_t *format,
                                                            int32_t *dataspace,
                                                            uint8_t *supported_components) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return kErrorResources;
  }

  return sideband_->GetHistogramAttributes(display, format, dataspace, supported_components);
}

// gl layer stitch
void SDMCompositorCallbacks::StitchLayers(uint64_t display, LayerStitchContext *params) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->StitchLayers(display, params);
}

void SDMCompositorCallbacks::InitLayerStitch(uint64_t display) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->InitLayerStitch(display);
}

void SDMCompositorCallbacks::DestroyLayerStitch(uint64_t display) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initalized!");
    return;
  }

  sideband_->DestroyLayerStitch(display);
}

nsecs_t SDMCompositorCallbacks::SystemTime(int clock) {
  if (!sideband_) {
    DLOGW("Sideband intf is not initialized!");
    return 0;
  }

  return sideband_->SystemTime(clock);
}

}  // namespace sdm