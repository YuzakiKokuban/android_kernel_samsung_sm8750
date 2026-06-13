/* Copyright (c) 2020, The Linux Foundataion. All rights reserved.
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
*
*/

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __GENERIC_PAYLOAD_H__
#define __GENERIC_PAYLOAD_H__

#include <errno.h>
#include <debug_handler.h>
#include <assert.h>
#include <functional>
#include <cstring>

// Do not define __CLASS__ for logging in shared utility header like this one

namespace sdm {

struct GenericPayload {
 public:
  GenericPayload():
    type_size(0), payload(nullptr), array_size(0), release(nullptr) {}

  GenericPayload(const GenericPayload &in) {
    type_size = 0;
    payload = nullptr;
    array_size = 0;
    if (in.payload) {
      display::DebugHandler::Get()->Error("GenericPayload::%s:New GenericPayload will not copy"
               "payload data! Use CopyPayload on a new GenericPayload instance.", __FUNCTION__);
    }
    copy_constructed = true;
  }

  GenericPayload& operator=(const GenericPayload &) = delete;

  template<typename A> int CopyPayload(const GenericPayload &in) {
    if (sizeof(A) != in.type_size) {
      return -EINVAL;
    }
    A* p = reinterpret_cast<A *>(in.payload);
    type_size = sizeof(A);
    array_size = in.array_size;

    if (payload != nullptr && release != nullptr) {
      release();
    }
    A* p2 = nullptr;
    if (array_size > 1) {
      p2 = new A[array_size];
    } else {
      p2 = new A();
    }
    if (p2 == nullptr) {
      return -ENOMEM;
    }
    *p2 = *p;
    payload = reinterpret_cast<uint8_t *>(p2);
    if (array_size > 1) {
      release = std::function<void(void)>([p2]() -> void {delete [] p2;});
    } else {
      release = std::function<void(void)>([p2]() -> void {delete p2;});
    }
    return 0;
  }

  template<typename A> int CreatePayload(A *&p) {
    if (payload) {
      p = nullptr;
      return -EALREADY;
    }

    p = new A();
    if (p == nullptr) {
      return -ENOMEM;
    }

    type_size = sizeof(A);
    array_size = 1;
    payload = reinterpret_cast<uint8_t *>(p);
    release = std::function<void(void)>([p]() -> void {delete p;});

    return 0;
  }

  template<typename A> int CreatePayload(A *&p, uint32_t sz) {
    if (payload) {
      p = nullptr;
      return -EALREADY;
    }

    if (!sz) {
      return -EINVAL;
    }

    p = new A[sz];
    if (p == nullptr) {
      return -ENOMEM;
    }

    type_size = sizeof(A);
    array_size = sz;
    payload = reinterpret_cast<uint8_t *>(p);
    release = std::function<void(void)>([p]() -> void {delete [] p;});

    return 0;
  }

  template<typename A> int GetPayload(A *&p, uint32_t *sz) const {
    if ((sz == nullptr) || (sizeof(A) != type_size)) {
      p = nullptr;
      return -EINVAL;
    }

    if (!sz) {
      return -EINVAL;
    }

    p = reinterpret_cast<A *>(payload);
    *sz = 0;
    if (p == nullptr && copy_constructed) {
      display::DebugHandler::Get()->Error("GenericPayload::%s:Payload was not properly"
                                          "copied via CopyPayload", __FUNCTION__);
      return -ENOMEM;
    } else if (p == nullptr) {
      display::DebugHandler::Get()->Error("GenericPayload::%s:Payload was not properly"
                                          "created via CreatePayload", __FUNCTION__);
      return -ENOMEM;
    }
    *sz = array_size;

    return 0;
  }

  template <typename A>
  int GetPayload(A *&p, uint32_t *num_elem, uint32_t *type_sz) const {
    if (!num_elem || !type_sz) {
      p = nullptr;
      return -EINVAL;
    }

    if (sizeof(A) > type_size) {
      p = nullptr;
      return -EINVAL;
    }

    p = reinterpret_cast<A *>(payload);
    if (p == nullptr && copy_constructed) {
      display::DebugHandler::Get()->Error(
          "GenericPayload::%s:Payload was not properly"
          "copied via CopyPayload",
          __FUNCTION__);
      return -ENOMEM;
    } else if (p == nullptr) {
      display::DebugHandler::Get()->Error(
          "GenericPayload::%s:Payload was not properly"
          "created via CreatePayload",
          __FUNCTION__);
      return -ENOMEM;
    }
    *num_elem = array_size;
    *type_sz = type_size;

    return 0;
  }

  void DeletePayload() {
    if (payload != nullptr && release != nullptr) {
      release();
    }

    type_size = 0;
    payload = nullptr;
    array_size = 0;
  }

  ~GenericPayload() {
    DeletePayload();
  }

 private:
  uint32_t type_size;
  uint8_t *payload;
  uint32_t array_size;
  std::function<void(void)> release;
  bool copy_constructed = false;
};

}  // namespace sdm

#endif  // __GENERIC_PAYLOAD_H__

