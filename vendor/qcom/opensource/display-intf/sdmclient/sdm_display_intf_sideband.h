/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_DISPLAY_INTF_SIDEBAND_H__
#define __SDM_DISPLAY_INTF_SIDEBAND_H__

#include <core/sdm_types.h>
#include <core/layer_buffer.h>

#include "sdm_display_intf_parcel.h"

namespace sdm {

class SDMDisplaySideBandIntf {
public:
  virtual ~SDMDisplaySideBandIntf() {}

  virtual DisplayError SetCameraLaunchStatus(uint32_t on) = 0;

  virtual DisplayError DisplayBWTransactionPending(bool *status) = 0;

  virtual DisplayError SetDisplayAnimating(uint64_t display_id,
                                           bool animating) = 0;

  virtual DisplayError ControlIdlePowerCollapse(bool enable,
                                                bool synchronous) = 0;

  virtual void SetClientUp() = 0;

  virtual bool IsBuiltInDisplay(uint64_t display) = 0;

  virtual bool IsAsyncVDSCreationSupported() = 0;

  virtual DisplayError CreateVirtualDisplay(int32_t width, int32_t height,
                                            int32_t format) = 0;

  virtual DisplayError TUIEventHandler(uint64_t display,
                                       SDMTUIEventType event_type) = 0;

  virtual DisplayError SetCameraSmoothInfo(SDMCameraSmoothOp op,
                                           int32_t fps) = 0;

  virtual DisplayError SetContentFps(const std::string& name, int32_t fps) = 0;

  virtual DisplayError PostBuffer(const CwbConfig &cwb_config, void *buffer,
                                  int32_t display_type) = 0;

  virtual int GetProperty(const char *property_name, char *value) = 0;

  virtual int GetProperty(const char *property_name, int *value) = 0;

  virtual void Dump(uint32_t *out_size, char *out_buffer) = 0;

  virtual DisplayError NotifyCallback(uint32_t command, SDMParcel *input_parcel,
                                      SDMParcel *output_parcel) = 0;
};

} //  namespace sdm

#endif // __SDM_DISPLAY_INTF_SIDEBAND_H__
