/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DPU_CORE_FACTORY_H__
#define __DPU_CORE_FACTORY_H__

#include "dpu_core_mux.h"

namespace sdm {

class DPUCoreFactory {
 public:
  static DisplayError Create(DisplayId display_id, SDMDisplayType type,
                             MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                             BufferAllocator *buffer_allocator, DPUCoreMux **dpu_core_mux);
};

}  // namespace sdm

#endif  // __DPU_CORE_FACTORY_H__
