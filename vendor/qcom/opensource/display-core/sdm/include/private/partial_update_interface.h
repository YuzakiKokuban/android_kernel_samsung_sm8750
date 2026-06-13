/*
* Copyright (c) 2015, 2020-2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted
* provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright notice, this list of
*      conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright notice, this list of
*      conditions and the following disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its contributors may be used to
*      endorse or promote products derived from this software without specific prior written
*      permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __PARTIAL_UPDATE_INTERFACE_H__
#define __PARTIAL_UPDATE_INTERFACE_H__

#include <core/display_interface.h>
#include <core/buffer_allocator.h>
#include <private/spr_intf.h>

#include "hw_info_types.h"

namespace sdm {

struct PUConstraints {
  bool enable = true;             //!< If this is set, PU will be enabled or it will be disabled
};

class PartialUpdateInterface {
 public:
  virtual DisplayError Start(const PUConstraints &pu_constraints) = 0;
  virtual DisplayError GenerateROI(DispLayerStack *disp_layer_stack) = 0;
  virtual DisplayError Stop() = 0;
  virtual DisplayError SetSprIntf(std::shared_ptr<SPRIntf> intf) = 0;
  virtual DisplayError SetDetailEnhancerData(const DisplayDetailEnhancerData &de_data) = 0;

 protected:
  virtual ~PartialUpdateInterface() { }
};

}  // namespace sdm

#endif  // __PARTIAL_UPDATE_INTERFACE_H__

