/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "dpu_core_factory.h"
#include "dpu_single_core.h"
#include "dpu_multi_core.h"

#define __CLASS__ "DPUCoreFactory"

namespace sdm {
DisplayError DPUCoreFactory::Create(
    DisplayId display_id, SDMDisplayType type,
    sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
    BufferAllocator *buffer_allocator, DPUCoreMux **dpu_core_mux) {
  DPUCoreMux *dpu_core = nullptr;
  if (hw_info_intf.Size() == 1) {
    dpu_core = new DPUSingleCore(display_id, type, hw_info_intf, buffer_allocator);
  } else {
    dpu_core = new DPUMultiCore(display_id, type, hw_info_intf, buffer_allocator);
  }

  DisplayError error = dpu_core->Init();
  if (error != kErrorNone) {
    DLOGE("DPUCore create failed");
    return error;
  }

  *dpu_core_mux = dpu_core;
  return error;
}

}  // namespace sdm
