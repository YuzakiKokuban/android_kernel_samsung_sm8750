/*
* Copyright (c) 2017, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of The Linux Foundation nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
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

#include <dlfcn.h>

#include "drm_lib_loader.h"

#define __CLASS__ "DRMLibLoader"

using std::mutex;
using std::lock_guard;

namespace drm_utils {

sdm::MultiCoreInstance<uint32_t, DRMLibLoader*> DRMLibLoader::s_instance;
mutex DRMLibLoader::s_lock;

DRMLibLoader *DRMLibLoader::GetInstance(uint32_t core_id) {
  lock_guard<mutex> obj(s_lock);
  auto iter = s_instance.Find(core_id);
  if (iter == s_instance.End()) {
    DRMLibLoader *loader = new DRMLibLoader();
    s_instance[core_id] = loader;
  }
  return s_instance[core_id];
}

void DRMLibLoader::Destroy(uint32_t core_id) {
  lock_guard<mutex> obj(s_lock);
  if (s_instance[core_id]) {
    delete s_instance[core_id];
    s_instance[core_id] = nullptr;
    s_instance.Erase(core_id);
  }
}

DRMLibLoader::DRMLibLoader() {
  if (Open("libsdedrm.so")) {
    if (Sym("GetDRMManager", reinterpret_cast<void **>(&func_get_drm_manager_)) &&
        Sym("DestroyDRMManager", reinterpret_cast<void **>(&func_destroy_drm_manager_))) {
      is_loaded_ = true;
    }
  }
}

DRMLibLoader::~DRMLibLoader() {
  if (lib_) {
    ::dlclose(lib_);
    lib_ = nullptr;
  }
}

bool DRMLibLoader::Open(const char *lib_name) {
  lib_ = ::dlopen(lib_name, RTLD_NOW);

  return (lib_ != nullptr);
}

bool DRMLibLoader::Sym(const char *func_name, void **func_ptr) {
  if (lib_) {
    *func_ptr = ::dlsym(lib_, func_name);
  }

  return (*func_ptr != nullptr);
}

}  // namespace drm_utils
