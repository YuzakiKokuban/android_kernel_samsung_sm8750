/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DEMURATN_CLEANUP_INTF_H__
#define __DEMURATN_CLEANUP_INTF_H__

#include <private/generic_intf.h>
#include <private/generic_payload.h>

namespace sdm {

enum DemuraTnCleanupParams {
  /* setter */
  kDemuraTnCleanupDeleteConfig,
  kDemuraTnCleanupParamsMax = 0xff,
};

enum DemuraTnCleanupOps {
  kDemuraTnCleanupOpsMax = 0xff,
};

struct DemuraTnCleanupDeleteConfigInput {
  uint64_t panel_id;
  bool delete_tn = false;
  bool delete_t0 = false;
};

enum DemuraTnCleanupType {
  kDeleteDemuraConfig,
  kDeleteDemuraTnConfig,
  kDemuraTnCleanupTypeMax,
};

using DemuraTnCleanupIntf = GenericIntf<DemuraTnCleanupParams, DemuraTnCleanupOps, GenericPayload>;
}  // namespace sdm
#endif  // __DEMURATN_CLEANUP_INTF_H__
