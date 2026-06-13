/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SDM_COMPOSITOR_CALLBACKS_H__
#define __SDM_COMPOSITOR_CALLBACKS_H__

#include "sdm_compositor_cb_intf.h"
#include "sdm_compositor_sideband_cb_intf.h"

#include <core/sdm_types.h>

namespace sdm {

class SDMCompositorCallbacks {
 public:
  void RegisterCallback(SDMCompositorCbIntf *cb, bool enable);
  void RegisterSideband(SDMSideBandCompositorCbIntf *cb, bool enable);

  // compositor callbacks
  void OnHotplug(uint64_t display, bool connected);
  void OnRefresh(uint64_t display);
  void OnVsync(uint64_t display, int64_t timestamp, int32_t vsync_period_nanos);
  void OnSeamlessPossible(uint64_t display);
  void OnVsyncIdle(uint64_t display);
  void OnVsyncPeriodTimingChanged(uint64_t display, SDMVsyncPeriodChangeTimeline &timeline);

  // sideband callbacks
  void NotifyQsyncChange(uint64_t display_id, bool qsync_enabled, uint32_t refresh_rate,
                         uint32_t qsync_refresh_rate);

  void NotifyCameraSmoothInfo(SDMCameraSmoothOp op, int32_t fps);

  void NotifyResolutionChange(uint64_t display_id, SDMConfigAttributes &attr);

  void NotifyTUIEventDone(uint32_t ret, uint32_t disp_id, SDMTUIEventType type);

  void NotifyIdleStatus(bool status);

  void NotifyCWBStatus(int32_t status, void *buffer);

  void NotifyContentFps(const std::string &name, int32_t fps);

  void OnHdmiHotplug(bool connected);

  int GetDemuraFilePaths(const GenericPayload &in, GenericPayload *out);

  void OnCECMessageReceived(char *message, int len);

  // gl color convert callbacks
  void InitColorConvert(uint64_t display, bool secure);
  void ColorConvertBlit(uint64_t display, ColorConvertBlitContext *ctx);
  void ResetColorConvert(uint64_t display);
  void DestroyColorConvert(uint64_t display);

  // Histogram callbacks
  void StartHistogram(uint64_t display, int max_frames);
  void StopHistogram(uint64_t display, bool teardown);
  void NotifyHistogram(uint64_t display, int fd, uint64_t blob_id, uint32_t panel_width,
                       uint32_t panel_height);
  std::string DumpHistogram(uint64_t display);
  void CollectHistogram(uint64_t display, uint64_t max_frames, uint64_t timestamp,
                        int32_t samples_size[NUM_HISTOGRAM_COLOR_COMPONENTS],
                        uint64_t *samples[NUM_HISTOGRAM_COLOR_COMPONENTS], uint64_t *numFrames);
  DisplayError GetHistogramAttributes(uint64_t display, int32_t *format, int32_t *dataspace,
                                      uint8_t *supported_components);

  // gl layer stitch
  void StitchLayers(uint64_t display, LayerStitchContext *params);
  void InitLayerStitch(uint64_t display);
  void DestroyLayerStitch(uint64_t display);

  int InitUevent();
  int NextUevent(char *buffer, int buffer_length);

  nsecs_t SystemTime(int clock);

 private:
  // non-owning reference - must always be reset to null on/before client deinit
  SDMCompositorCbIntf *callbacks_ = nullptr;
  SDMSideBandCompositorCbIntf *sideband_ = nullptr;
};

}  // namespace sdm

#endif