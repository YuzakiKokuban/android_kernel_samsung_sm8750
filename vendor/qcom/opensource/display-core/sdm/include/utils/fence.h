/*
* Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

#ifndef __FENCE_H__
#define __FENCE_H__

#include <core/buffer_sync_handler.h>
#include <unistd.h>
#include <utility>
#include <memory>
#include <string>
#include <vector>

namespace sdm {

using std::shared_ptr;
using std::string;
using std::to_string;

class Fence {
 public:
  enum class Status : int32_t {
    kSignaled = 0,
    kPending
  };

  // This class methods allow client to get access to the native file descriptor of fence object
  // during the scope of this class object. Underlying file descriptor is duped and returned to
  // the client. Duped file descriptors are closed as soon as scope ends. Client can get access
  // to multiple fences using the same scoped reference.
  class ScopedRef {
   public:
    ~ScopedRef();
    int Get(const shared_ptr<Fence> &fence);

   private:
    std::vector<int> dup_fds_ = {};
  };

  ~Fence();

  // Must be set once before using any other method of this class.
  static void Set(BufferSyncHandler *buffer_sync_handler);

  // Ownership of the file descriptor is transferred to this method.
  // Client must not close the file descriptor anymore regardless of the object creation status.
  // nullptr will be retured for invalid fd i.e. -1.
  static shared_ptr<Fence> Create(int fd, const string &name);

  // Ownership of returned fd lies with caller. Caller must explicitly close the fd.
  static int Dup(const shared_ptr<Fence> &fence);

  static shared_ptr<Fence> Merge(const shared_ptr<Fence> &fence1, const shared_ptr<Fence> &fence2);

  static shared_ptr<Fence> Merge(const std::vector<shared_ptr<Fence>> &fences,
                                 bool ignore_signaled);

  // Wait on null fence will return success.
  static int Wait(const shared_ptr<Fence> &fence);
  static int Wait(const shared_ptr<Fence> &fence, int timeout);

  // Status check on null fence will return signaled.
  static Status GetStatus(const shared_ptr<Fence> &fence);

  static string GetStr(const shared_ptr<Fence> &fence);

  // Write all fences info to the output stream.
  static void Dump(std::ostringstream *os);

 private:
  explicit Fence(int fd, const string &name);
  Fence(const Fence &fence) = delete;
  Fence& operator=(const Fence &fence) = delete;
  Fence(Fence &&fence) = delete;
  Fence& operator=(Fence &&fence) = delete;
  static int Get(const shared_ptr<Fence> &fence);

  static BufferSyncHandler *g_buffer_sync_handler_;
  static std::vector<std::weak_ptr<Fence>> wps_;
  int fd_ = -1;
  string name_ = "";
};

}  // namespace sdm

#endif  // __FENCE_H__
