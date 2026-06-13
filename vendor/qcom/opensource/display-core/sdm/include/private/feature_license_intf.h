/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __FEATURE_LICENSE_INTF_H__
#define __FEATURE_LICENSE_INTF_H__

#include <private/generic_intf.h>
#include <private/generic_payload.h>

namespace sdm {

enum FeatureLicenseId {
  kDemura,
  kAntiAging,
  kFeatureLicenseIdMax = 255,
};

// Feature license intf params as enum
enum FeatureLicenseParams {
  kFeatureLicenseParamMax = 255,
};

// Feature license intf ops as enum
enum FeatureLicenseOps {
  kValidatePermission,
  kFeatureLicenseOpsMax = 255,
};

struct FeatureValidatePermissionInput {
  FeatureLicenseId id;
};

struct DemuraValidatePermissionInput : public FeatureValidatePermissionInput {
  uint64_t panel_id;
};

struct AntiAgingValidatePermissionInput : public FeatureValidatePermissionInput {
};

using FeatureLicenseIntf = GenericIntf<FeatureLicenseParams, FeatureLicenseOps, GenericPayload>;

class FeatureLicenseFactoryIntf {
 public:
  virtual ~FeatureLicenseFactoryIntf() {}
  virtual std::shared_ptr<FeatureLicenseIntf> CreateFeatureLicenseIntf() = 0;
};

extern "C" FeatureLicenseFactoryIntf *GetFeatureLicenseFactoryIntf();

}  // namespace sdm

#endif  // __FEATURE_LICENSE_INTF_H__
