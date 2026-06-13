/*
* Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include "sdm_comp_interface.h"
#include "sdm_comp_impl.h"

#define __CLASS__ "SDMCompInterface"

namespace sdm {

int SDMCompInterface::Create(SDMCompInterface **intf) {
  if (!intf) {
    DLOGE("intf pointer is NULL");
    return -EINVAL;
  }
  SDMCompImpl *sdm_comp_impl = SDMCompImpl::GetInstance();
  if (!sdm_comp_impl) {
    return -EINVAL;
  }
  int error = sdm_comp_impl->Init();
  if (error != 0) {
    DLOGE("Init failed with %d", error);
    return error;
  }
  *intf = sdm_comp_impl;

  return error;
}

int SDMCompInterface::Destroy(SDMCompInterface *intf) {
  if (!intf) {
    DLOGE("intf pointer is NULL");
    return -EINVAL;
  }

  SDMCompImpl *sdm_comp_impl = static_cast<SDMCompImpl *>(intf);
  int error = sdm_comp_impl->Deinit();
  if (error != 0) {
    DLOGE("Deinit failed with %d", error);
    return error;
  }

  return 0;
}

}  // namespace sdm
