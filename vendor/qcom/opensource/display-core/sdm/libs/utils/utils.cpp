/*
* Copyright (c) 2016 - 2017, 2020 - 2021 The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of The Linux Foundation nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
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
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <unistd.h>
#include <math.h>
#include <utils/sys.h>
#include <utils/utils.h>
#include <utils/debug.h>
#include <utils/formats.h>
#include <utils/soc_info.h>

#include <algorithm>
#include <string>

#define __CLASS__ "Utils"

namespace sdm {

float gcd(float a, float b) {
  if (a < b) {
    std::swap(a, b);
  }

  while (b != 0) {
    float tmp = b;
    b = fmodf(a, b);
    a = tmp;
  }

  return a;
}

float lcm(float a, float b) {
  return (a * b) / gcd(a, b);
}

void CloseFd(int *fd) {
  if (*fd >= 0) {
    Sys::close_(*fd);
    *fd = -1;
  }
}

uint64_t GetSystemTimeInNs() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (uint64_t) ts.tv_sec * pow(10, 9) + (uint64_t)ts.tv_nsec;
}

void SetRealTimePriority() {
  struct sched_param param = {0};
  param.sched_priority = 2;
  if (sched_setscheduler(0, SCHED_FIFO | SCHED_RESET_ON_FORK, &param) != 0) {
    DLOGW("Couldn't set SCHED_FIFO: %d", errno);
  }
}

void AdjustSize(const int min_size, const int bound_start, const int bound_end, int *input_start,
                int *input_end) {
  // This fucntion is for expanding ROI dimension marked by input_start & input_end
  // to size min_size .
  int &start = *input_start;
  int &end = *input_end;
  const int size_correction = min_size - (end - start);
  start = start - (size_correction >> 1);
  end = end + (size_correction >> 1) + (size_correction & 1);

  if (start < bound_start) {
    start = bound_start;
    end = start + min_size;
  } else if (end > bound_end) {
    end = bound_end;
    start = end - min_size;
  }
}

void ApplyCwbRoiRestrictions(LayerRect &roi, const LayerRect &cwb_full_frame,
                             const int cwb_alignment_factor,
                             LayerBufferFormat format) {
  if (!IsUBWCFormat(format)) {
    return;
  }
  // Make ROI's (width * height) as 256B aligned
  uint32_t roi_width = UINT32(roi.right - roi.left);
  uint32_t roi_height = UINT32(roi.bottom - roi.top);

  // If Roi is already aligned or is congruent as full frame then return.
  if (((roi_width * roi_height) % cwb_alignment_factor == 0) || IsCongruent(roi, cwb_full_frame)) {
    return;
  }

  uint32_t width_to_expand = cwb_alignment_factor - (roi_width % cwb_alignment_factor);
  uint32_t height_to_expand = cwb_alignment_factor - (roi_height % cwb_alignment_factor);

  bool can_expand_width =
      ((roi_width + width_to_expand) <= UINT32(cwb_full_frame.right - cwb_full_frame.left));
  bool can_expand_height =
      ((roi_height + height_to_expand) <= UINT32(cwb_full_frame.bottom - cwb_full_frame.top));
  bool expand_height = false;  // If expand_height is true, it implies that expanding height is
  // more feasible than expanding width, whereas false implies expanding width is more feasible.

  if (!can_expand_width && !can_expand_height) {
    // Can't expand width or align to satisy the CWB alignment requirement, so set to full frame.
    roi = cwb_full_frame;
    return;
  } else if (!can_expand_width && can_expand_height) {
    // If expanding width to make width aligned (i.e. width * bpp = n * 256B) crosses full frame's
    // width boundaries, then try expanding height.
    expand_height = true;
  } else if (can_expand_width && can_expand_height) {
    // If expanding width adds more no. of pixels in Roi than expanding height to make the ROI
    // 256B aligned, then also prefer expanding height.
    if ((width_to_expand * roi_height) > (height_to_expand * roi_width)) {
      expand_height = true;
    }
  }

  if (!expand_height) {
    DLOGV_IF(kTagNone, "Expanding ROI width to %u ", roi_width + width_to_expand);
    int roi_left = INT(roi.left), roi_right = INT(roi.right);
    AdjustSize(roi_width + width_to_expand, 0, INT(cwb_full_frame.right), &roi_left, &roi_right);
    roi.left = FLOAT(roi_left);
    roi.right = FLOAT(roi_right);
  } else {
    DLOGV_IF(kTagNone, "Expanding ROI height to %u ", roi_height + height_to_expand);
    int roi_top = INT(roi.top), roi_bottom = INT(roi.bottom);
    AdjustSize(roi_height + height_to_expand, 0, INT(cwb_full_frame.bottom), &roi_top, &roi_bottom);
    roi.top = FLOAT(roi_top);
    roi.bottom = FLOAT(roi_bottom);
  }
}

const char *GetCompositionName(const LayerComposition &composition) {
  switch (composition) {
  case kCompositionGPU:           return "GPU";
  case kCompositionSDE:           return "SDE";
  case kCompositionCursor:        return "CURSOR";
  case kCompositionStitch:        return "STITCH";
  case kCompositionGPUTarget:     return "GPU_TARGET";
  case kCompositionStitchTarget:  return "STITCH_TARGET";
  case kCompositionDemura:        return "DEMURA";
  case kCompositionCWBTarget:     return "CWB_TARGET";
  case kCompositionIWE:           return "IWE";
  default:                        return "UNKNOWN";
  }
}

const char* GetSocName() {
  string soc_id;
  std::ifstream in("/sys/devices/soc0/soc_id");
  if (!in.is_open()) {
    DLOGW("Cannot open soc id file.");
    return "";
  }

  std::getline(in, soc_id);
  if (soc_id.length() <= 0) {
    DLOGW("Cannot get soc id");
    return "";
  }
  int soc_id_num = stoi(soc_id);

  switch (soc_id_num) {
    case kAnorakSocId:
      return kAnorakSocName;
    case kSunSocId:
      return kSunSocName;
    case kNiobeSocId1:
    case kNiobeSocId2:
      return kNiobeSocName;
    default:
      return "";
  }
}

bool IsXRVariant() {
  if (!strcmp(GetSocName(), kAnorakSocName) || !strcmp(GetSocName(), kNiobeSocName)) {
    return true;
  }

  return false;
}
}  // namespace sdm
