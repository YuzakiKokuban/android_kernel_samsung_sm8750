/*
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DEMURATN_CORE_UVM_FACT_INTF_H__
#define __DEMURATN_CORE_UVM_FACT_INTF_H__

#include "demuratn_core_uvm_intf.h"

#include <core/buffer_allocator.h>
#include <core/display_interface.h>
#include <memory>

#include "demura_intf.h"
#include "demuratn_cleanup_intf.h"

namespace sdm {

class DemuraTnCoreUvmFactoryIntf {
 public:
  virtual ~DemuraTnCoreUvmFactoryIntf() {}
  virtual std::shared_ptr<DemuraTnCoreUvmIntf> CreateDemuraTnCoreUvmIntf(
      std::shared_ptr<DemuraIntf>, BufferAllocator *, DisplayInterface *) = 0;
  virtual std::shared_ptr<DemuraTnCleanupIntf> CreateDemuraTnCleanupIntf(BufferAllocator *) = 0;
};

extern "C" DemuraTnCoreUvmFactoryIntf *GetDemuraTnCoreUvmFactoryIntf();

}  // namespace sdm
#endif  // __DEMURATN_CORE_UVM_FACT_INTF_H__
