/*
* Copyright (c) 2017-2018, 2020, The Linux Foundation. All rights reserved.
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

#include <utils/debug.h>
#include <utils/utils.h>

#include <private/hw_interface.h>
#ifndef TARGET_HEADLESS
#include "hw_peripheral_drm.h"
#include "hw_virtual_drm.h"
#include "hw_tv_drm.h"
#endif

#define __CLASS__ "HWInterface"

namespace sdm {

DisplayError HWInterface::Create(int32_t display_id, SDMDisplayType type,
                                 HWInfoInterface *hw_info_intf,
                                 BufferAllocator *buffer_allocator, HWInterface **intf) {
  DisplayError error = kErrorNone;
  HWInterface *hw = nullptr;

  switch (type) {
#ifndef TARGET_HEADLESS
    case kBuiltIn:
        hw = new HWPeripheralDRM(display_id, buffer_allocator, hw_info_intf);
      break;
    case kPluggable:
        hw = new HWTVDRM(display_id, buffer_allocator, hw_info_intf);
      break;
    case kVirtual:
        hw = new HWVirtualDRM(display_id, buffer_allocator, hw_info_intf);
      break;
#endif
    default:
      DLOGE("Undefined display type");
      return kErrorUndefined;
  }

  error = hw->Init();
  if (error != kErrorNone) {
    delete hw;
    if (kErrorDeviceRemoved == error) {
      DLOGW("Init on HWInterface for display %d-%d aborted.", display_id, type);
    } else {
      DLOGE("Init on HWInterface for display %d-%d failed.", display_id, type);
    }
    return error;
  }
  *intf = hw;

  return error;
}

DisplayError HWInterface::Destroy(HWInterface *intf) {
  if (intf) {
    intf->Deinit();
    delete intf;
  }

  return kErrorNone;
}

}  // namespace sdm
