/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DEMURATN_CORE_UVM_INTF_H__
#define __DEMURATN_CORE_UVM_INTF_H__

#include <private/generic_intf.h>
#include <private/generic_payload.h>

namespace sdm {

enum DemuraTnCoreState {
  kDemuraTnCoreNotReady,
  kDemuraTnCoreReady,
  kDemuraTnCoreError,
  kDemuraTnCoreStateMax,
};

enum DemuraTnCoreUvmParams {
  /* Getter: DemuraTnCoreState */
  kDemuraTnCoreUvmParamInitReady,
  /* Getter/Setter: bool */
  kDemuraTnCoreUvmParamEnable,
  /* Setter: none */
  kDemuraTnCoreUvmParamRetrieveFiles,
  /* Setter: enum DisplayState  */
  kDemuraTnCoreUvmParamDisplayState,
  /* Setter: int  */
  kDemuraTnCoreUvmParamCWBSamplingPeriod,
  /* Setter: bool  */
  kDemuraTnCoreUvmParamEventsCtrl,
  kDemuraTnCoreUvmParamsMax = 2048,
  kDemuraTnCoreUvmPrivParamsStart = 2049,
  kDemuraTnCoreUvmPrivParamsEnd = 4096,
};

enum DemuraTnCoreUvmOps {
  /* No public op is supported for now */
  kDemuraTnCoreUvmOpsMax = 2048,
  kDemuraTnCoreUvmOpsPrivOpsStart = 2049,
  kDemuraTnCoreUvmOpsPrivOpsEnd = 4096,
};

using DemuraTnCoreUvmIntf = GenericIntf<DemuraTnCoreUvmParams, DemuraTnCoreUvmOps, GenericPayload>;
}  // namespace sdm
#endif  // __DEMURATN_CORE_UVM_INTF_H__
