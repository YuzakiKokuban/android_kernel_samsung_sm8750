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
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_DISPLAY_INTF_LIFECYCLE_H__
#define __SDM_DISPLAY_INTF_LIFECYCLE_H__

#include <core/display_interface.h>
#include <core/socket_handler.h>
#include <utils/fence.h>

#include "debug_callback_intf.h"
#include "sdm_compositor_cb_intf.h"
#include "sdm_compositor_sideband_cb_intf.h"

using std::shared_ptr;

namespace sdm {

enum CompositorSyncType {
  CompositorSyncTypeAcquire,
  CompositorSyncTypeRelease,
};

class SDMDisplayLifeCycleIntf {
public:
  SDMDisplayLifeCycleIntf() {}
  virtual ~SDMDisplayLifeCycleIntf(){};

  virtual void RegisterSideBandCallback(SDMSideBandCompositorCbIntf *cb,
                                        bool enable) = 0;

  virtual void RegisterCompositorCallback(SDMCompositorCbIntf *cb, bool enable) = 0;

  /**
   * Create and initialize a display with SDM
   *
   * For builtin displays: Query GetDisplayList immediately to determine if any
   * builtin displays are connected, and initialize them afterwards.
   *
   * For external displays, call this after receiving a hotplug notification
   *
   * For virtual displays, compositor will call this in order to create the
   * display
   *
   * @param display_id: The SDM id of the display to be created/initialized.
   * This can be queried through GetDisplayList()
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError CreateDisplay(SDMDisplayType type, int32_t width,
                                     int32_t height, int32_t *format,
                                     uint64_t *display_id) = 0;

  /**
   * De-init and destroy a display with SDM
   *
   * For builtin/external: call this method after a hotplug disconnect is
   * detected
   *
   * For virtual: Compositor calls this whenever the virtual display should be
   * destroyed
   *
   * @param display_id: The SDM id of the display to be destroyed
   *
   * @exception: kErrorParameters if display_id is not found
   */
  virtual DisplayError DestroyDisplay(uint64_t display_id) = 0;

  /**
   * Get a list of currently connected displays, along with their display type,
   * display id, and other display + panel info
   */
  virtual DisplayError
  GetDisplayList(std::vector<SDMDisplayInfo> *display_info) = 0;

  /**
   * Set the state of specified display
   * In case of power off:
   *    - The display is off. In this state, there is nothing displayed on the
   * screen.
   *
   * In case of power on:
   *    - The display is on. In this state, regular UI and application layers
   * are visible.
   *
   * In case of doze:
   *    - The display is on but set to a low power state. In this state, only
   * ambient information is displayed on the screen such as the time, weather,
   * and notifications.
   *
   * In case of doze suspend:
   *    - The display is configured similar to doze mode but may stop applying
   * updates on the display. In this state, the current contents will be
   * displayed indefinitely until the power mode changes.
   *
   * @param display_id: The id of the specified display
   * @param state: The state for the display to be set to
   * @param teardown: Set this flag to force a full teardown on pluggable
   * displays if requested state is OFF - no-op for other display types.
   *
   * @return: fd of the release fence
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: NO_RESOURCES if the transition to requested state is deferred
   * (can be deferred active secure session, pending CaptureFeedback teardown)
   */
  virtual DisplayError SetDisplayState(uint64_t display_id, DisplayState state,
                                       bool teardown, Fence *fence) = 0;

  virtual DisplayError SetPowerMode(uint64_t display_id, int32_t int_mode) = 0;

  virtual DisplayError Init(BufferAllocator *buffer_allocator,
                            SocketHandler *socket_handler,
                            DebugCallbackIntf *debug_handler) = 0;

  virtual bool IsDisplayConnected(uint64_t display_id) = 0;

  virtual DisplayError SetDisplayStatus(uint64_t disp_id,
                                        SDMDisplayStatus status) = 0;

  virtual DisplayError GetConfigCount(uint64_t disp_id, uint32_t *count) = 0;

  virtual DisplayError TryDrawMethod(Display display,
                                     DisplayDrawMethod drawMethod) = 0;

  virtual void CompositorSync(CompositorSyncType syncType) = 0;
};

} // namespace sdm

#endif // __SDM_DISPLAY_INTF_LIFECYCLE_H__
