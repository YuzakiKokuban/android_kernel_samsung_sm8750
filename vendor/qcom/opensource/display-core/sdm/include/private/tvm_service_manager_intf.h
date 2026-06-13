/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TVM_SERVICE_MANAGER_INTF_H___
#define __TVM_SERVICE_MANAGER_INTF_H___

#include <private/generic_payload.h>
#include <private/generic_intf.h>

namespace sdm {

enum TvmDispServiceManagerParams {
  kStartVmFileTransferService,
  kStartDemuraTnService,
  kTvmDispServiceManagerParamMax,
};

enum TvmDispServiceManagerOps {
  kTvmDispServiceManagerOpsMax,
};

using TvmDispServiceManagerIntf =
    GenericIntf<TvmDispServiceManagerParams, TvmDispServiceManagerOps, GenericPayload>;

}  // namespace sdm

#endif  // __TVM_SERVICE_MANAGER_INTF_H___
