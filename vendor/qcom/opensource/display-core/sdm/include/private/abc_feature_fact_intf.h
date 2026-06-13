/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __ABC_FEATURE_FACT_INTF_H__
#define __ABC_FEATURE_FACT_INTF_H__

#include <core/buffer_allocator.h>
#include <core/buffer_sync_handler.h>
#include <core/display_interface.h>
#include <memory>
#include <private/demura_intf.h>
#include <private/panel_feature_property_intf.h>

namespace aiqe {

struct ABCFeatureCreateParams {
  const sdm::DemuraInputConfig input_cfg;
};

class ABCFeatureFactIntf {
 public:
  virtual ~ABCFeatureFactIntf() {}
  virtual std::unique_ptr<sdm::DemuraIntf> CreateABCIntf(const sdm::DemuraInputConfig &input_cfg,
                                                         sdm::PanelFeaturePropertyIntf *prop_intf,
                                                         sdm::BufferAllocator *allocator,
                                                         sdm::DisplayInterface *display_intf) = 0;
};

extern "C" ABCFeatureFactIntf *GetABCFeatureFactIntf();
}  // namespace aiqe

#endif  // __ABC_FEATURE_FACT_INTF_H__