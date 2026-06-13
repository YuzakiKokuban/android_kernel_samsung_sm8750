/*
* Copyright (c) 2019, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.

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

#ifndef __DRM_ATOMIC_REQ_H__
#define __DRM_ATOMIC_REQ_H__

#include <drm_interface.h>
#include <drm_utils.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <vector>

namespace sde_drm {

class DRMManager;

class DRMAtomicReq : public DRMAtomicReqInterface {
 public:
  DRMAtomicReq(int fd, DRMManager *drm_manager);
  virtual ~DRMAtomicReq();
  virtual int Perform(DRMOps op_code, uint32_t obj_id, ...);
  virtual int Commit(bool synchronous, bool retain_planes);
  virtual int Validate();
  int Init(const DRMDisplayToken &tok);

 private:
  drmModeAtomicReq *drm_atomic_req_ = {};
  DRMManager *drm_mgr_ = {};
  int fd_ = -1;
  DRMDisplayToken token_ = {};
};

}  // namespace sde_drm
#endif  // __DRM_ATOMIC_REQ_H__
