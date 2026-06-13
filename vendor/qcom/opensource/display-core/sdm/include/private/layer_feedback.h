/*
* Copyright (c) 2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above
*    copyright notice, this list of conditions and the following
*    disclaimer in the documentation and/or other materials provided
*    with the distribution.
*  * Neither the name of The Linux Foundation nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
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

#ifndef __LAYER_FEEDBACK_H__
#define __LAYER_FEEDBACK_H__

#include <stdint.h>
#include <vector>

using std::vector;
namespace sdm {

struct LayerFeedback {
  explicit LayerFeedback(uint32_t unsupported_size) {
    unsupported_list_.resize(unsupported_size, false);
  }

  // unsupported_list_[i] is true if layer at index i is unsupported by DPU
  vector<bool> unsupported_list_;

  // set of layers contending for common DPU resource
  // identified by original index in app layer stack
  // layers are sorted in descending order of cost
  vector<uint8_t> contention_list_;

  // number of layers that must be removed from
  // contention list to avoid resource contention
  uint8_t contention_count_ = 0;

  bool wfd_in_use_ = false;

  bool cwb_in_use_ = false;
};

}  // namespace sdm

#endif  // __LAYER_FEEDBACK_H__
