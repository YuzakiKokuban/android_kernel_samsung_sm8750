/*
* Copyright (c) 2014 - 2016, 2018 - 2021 The Linux Foundation. All rights reserved.
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
 * ​Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*! @file core_interface.h
  @brief Interface file for core of the display subsystem.

  @details Display core is primarily used for loading and unloading different display device
  components viz primary, external and virtual. Display core is a statically linked library which
  runs in caller's process context.
*/
#ifndef __CORE_INTERFACE_H__
#define __CORE_INTERFACE_H__

#include <core/ipc_interface.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <memory>

#include "display_interface.h"
#include "sdm_types.h"
#include "buffer_allocator.h"
#include "buffer_sync_handler.h"
#include "socket_handler.h"
#include "ipc_interface.h"

/*! @brief Display manager interface version.

  @details Display manager interfaces are version tagged to maintain backward compatibility. This
  version is supplied as a default argument during display core initialization.

  Client may use an older version of interfaces and link to a higher version of display manager
  library, but vice versa is not allowed.

  A 32-bit client must use 32-bit display core library and a 64-bit client must use 64-bit display
  core library.

  Display manager interfaces follow default data structures alignment. Client must not override the
  default padding rules while using these interfaces.

  @warning It is assumed that client upgrades or downgrades display core interface all at once
  and recompile all binaries which use these interfaces. Mix and match of these interfaces can
  lead to unpredictable behaviour.

  @sa CoreInterface::CreateCore
*/
#define SDM_REVISION_MAJOR (1)
#define SDM_REVISION_MINOR (0)

#define SDM_VERSION_TAG ((uint32_t) ((SDM_REVISION_MAJOR << 24) | (SDM_REVISION_MINOR << 16) | \
                                    (sizeof(SDMCompatibility) << 8) | sizeof(int *)))

namespace sdm {

/*! @brief This enum represents max bandwidth limit mode.

  @sa DisplayInterface::SetMaxBandwidthMode
*/
enum HWBwModes {
  kBwVFEOff,       //!< Camera/video front end off. No change in device bandwidth limit.
  kBwVFEOn,        //!< Camera/video front end is on. Bandwidth limit reduced.
  kBwModeMax,      //!< Limiter for maximum available bandwidth modes.
};

/*! @brief Information on hardware for the first display

  @details This structure returns the display type of the first display on the device
  (internal display or HDMI etc) and whether it is currently connected.
*/
struct HWDisplayInterfaceInfo {
  SDMDisplayType type = kDisplayTypeMax;
  bool is_connected = false;
};

/*! @brief Information about a single display/monitor/screen

  @details This structure returns the display configuration and status of a single display. A
  list of this structure type 'HWDisplaysInfo' is used to return information on all available
  displays. See \link HWDisplaysInfo \endlink
*/
struct HWDisplayInfo {
  int32_t display_id = -1;                     //!< ID of this display (Display ID).
  SDMDisplayType display_type = kDisplayTypeMax;  //!< Type of display: BuiltIn/Pluggable/Virtual
  bool is_connected = false;                   //!< Connection status of the display.
  bool is_primary = false;                     //!< True only if this is the main display of the
                                               //!< device.
  bool is_wb_ubwc_supported = true;            //!< check hardware wb ubwc support
  bool is_reserved = false;                    //!< check if currently reserved by any display
  uint32_t max_linewidth = 0;                  //!< max width supported by connector
  uint32_t max_cwb = 0;                        //!< Maximum CWB instances supported concurrently,
                                               //!< and it is valid only for virtual display.
  bool has_disp_in_other_core = false;         //!< check if display is also present in other core
};

/*! @brief Information on all displays as a map with display_id as key.

  @details This map returns the display configuration and status of all displays.
*/
typedef std::map<int32_t, HWDisplayInfo> HWDisplaysInfo;

/*! @brief Display core interface.

  @details This class defines display core interfaces. It contains methods which client shall use
  to create/destroy different display devices. This interface is created during display core
  CreateCore() and remains valid until DestroyCore().

  @sa CoreInterface::CreateCore
  @sa CoreInterface::DestroyCore
*/
class CoreInterface {
 public:
  /*! @brief Method to create and get handle to display core interface.

    @details This method is the entry point into the display core. Client can create and operate on
    different display devices only through a valid interface handle obtained using this method. An
    object of display core is created and handle to this object is returned via output parameter.
    This interface shall be called only once.

    @param[in] buffer_allocator \link BufferAllocator \endlink
    @param[in] buffer_sync_handler \link BufferSyncHandler \endlink
    @param[in] socket_handler \link SocketHandler \endlink
    @param[in] ipc_intf \link IPCIntf \endlink
    @param[out] interface \link CoreInterface \endlink
    @param[in] version \link SDM_VERSION_TAG \endlink. Client must not override this argument.

    @return \link DisplayError \endlink

    @sa DestroyCore
  */
  static DisplayError CreateCore(BufferAllocator *buffer_allocator,
                                 BufferSyncHandler *buffer_sync_handler,
                                 SocketHandler *socket_handler,
                                 std::shared_ptr<IPCIntf> ipc_intf,
                                 CoreInterface **interface,
                                 std::bitset<8> core_ids = std::bitset<8>(0xFF),
                                 uint32_t version = SDM_VERSION_TAG);

  /*! @brief Method to release handle to display core interface.

    @details The object of corresponding display core is destroyed when this method is invoked.
    Client must explicitly destroy all created display device objects associated with this handle
    before invoking this method.

    @param[in] interface \link CoreInterface \endlink

    @return \link DisplayError \endlink

    @sa CreateCore
  */
  static DisplayError DestroyCore();

  /*! @brief Method to create a display device for a given type.

    @details Client shall use this method to create each of the connected display type. A handle to
    interface associated with this object is returned via output parameter which can be used to
    interact further with the display device.

    @param[in] type \link SDMDisplayType \endlink
    @param[in] event_handler \link DisplayEventHandler \endlink
    @param[out] interface \link DisplayInterface \endlink

    @return \link DisplayError \endlink

    @sa DestroyDisplay
  */
  virtual DisplayError CreateDisplay(SDMDisplayType type, DisplayEventHandler *event_handler,
                                     DisplayInterface **interface) = 0;

  /*! @brief Method to create a display device for a given display ID.

    @details Client shall use this method to create a DisplayInterface to a discovered display
    identified by its display ID. A handle to the DisplayInterface is returned via the 'interface'
    output parameter which can be used to interact further with the display device. Displays and
    their IDs must be discovered using GetDisplaysStatus().

    @param[in] display_id A display ID got from \link GetDisplaysStatus() \endlink
    @param[in] event_handler \link DisplayEventHandler \endlink
    @param[out] interface \link DisplayInterface \endlink

    @return \link DisplayError \endlink

    @sa DestroyDisplay
  */
  virtual DisplayError CreateDisplay(int32_t display_id, DisplayEventHandler *event_handler,
                                     DisplayInterface **interface) = 0;

  /*! @brief Method to create a null display.

    @details Client shall use this method to create a DisplayInterface to DisplayNull. A handle
    to the DisplayInterface is returned via the 'interface' output parameter which can be used to
    interact further with the display device.

    @param[out] interface \link DisplayInterface \endlink

    @return \link DisplayError \endlink

    @sa DestroyNullDisplay
  */
  virtual DisplayError CreateNullDisplay(DisplayInterface **interface) = 0;

  /*! @brief Method to destroy a display device.

    @details Client shall use this method to destroy each of the created display device objects.

    @param[in] interface \link DisplayInterface \endlink

    @return \link DisplayError \endlink

    @sa CreateDisplay
  */
  virtual DisplayError DestroyDisplay(DisplayInterface *interface) = 0;

  /*! @brief Method to destroy a null display device.

    @details Client shall use this method to destroy null display device objects.

    @param[in] interface \link DisplayInterface \endlink

    @return \link DisplayError \endlink

    @sa CreateNullDisplay
  */
  virtual DisplayError DestroyNullDisplay(DisplayInterface *interface) = 0;

#ifdef PROFILE_COVERAGE_DATA
  /*! @brief Method to destroy a display device.

  */
  virtual DisplayError DumpCodeCoverage() = 0;
#endif

  /*! @brief Method to update the bandwidth limit as per given mode.

    @param[in] mode indicate the mode or use case

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetMaxBandwidthMode(HWBwModes mode) = 0;

  /*! @brief Method to get characteristics of the first display.

    @details Client shall use this method to determine if the first display is HDMI, and whether
    it is currently connected.

    @param[in] hw_disp_info structure that this method will fill up with info.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetFirstDisplayInterfaceType(HWDisplayInterfaceInfo *hw_disp_info) = 0;

  /*! @brief Method to get an up-to-date list of all available displays.

    @details Client shall use this method to get the updated list of all available displays and
    their properties, usually in response to a hot-plug event. Client must use one of the returned
    HWDisplayInfo::display_ids when using CreateDisplay(int32_t display_id, ...) to create the
    DisplayInterface to the display.

    @param[out] hw_displays_info \link HWDisplaysInfo \endlink which is a map of \link HWDisplayInfo
    \endlink structures with display_id as the key.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetDisplaysStatus(HWDisplaysInfo *hw_displays_info) = 0;

  /*! @brief Method to get the maximum supported number of concurrent displays of a particular type.

    @details Client shall use this method to get the maximum number of DisplayInterface instances
    that can be created for a particular \link SDMDisplayType \endlink display. For the maximum
    number of concurrent DisplayInterfaces supported of all types, call with type kDisplayTypeMax.

    @param[in] type Type of display: BuiltIn/Pluggable/Virtual/kDisplayTypeMax
    key.

    @param[out] max_displays Maximum number of DisplayInterface instances possible.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetMaxDisplaysSupported(SDMDisplayType type, int32_t *max_displays) = 0;

  /*! @brief Method which returns true if the given format is supported by rotator otherwise false

    @param[in] \link LayerBufferFormat \endlink

    @return returns true if the given format is supported by rotator otherwise false
  */
  virtual bool IsRotatorSupportedFormat(LayerBufferFormat format) = 0;

  /*! @brief Method to reserve the resources for demura/ABC feature at bootup.

    @return returns true if resources are successfully reserved.
  */
  virtual DisplayError ReserveDemuraPipeResources() = 0;

  /*! @brief Method to request to get virtual display ID (h/w writeback block ID).

    @details Client shall use this method to get virtual display ID (h/w writeback block ID),
    if it is available, else it could be available after tearing down any lower priority usage
    writeback block out of existing usage. And it makes sure that tearing down usage has lower
    priority than requesting usage.

    @param[out] vdisp_id Virtual Display ID for DPU based virtual display.

    @return \link DisplayError \endlink
  */
  virtual DisplayError RequestVirtualDisplayId(int32_t *vdisp_id) = 0;

 protected:
  virtual ~CoreInterface() { }
};

}  // namespace sdm

#endif  // __CORE_INTERFACE_H__

