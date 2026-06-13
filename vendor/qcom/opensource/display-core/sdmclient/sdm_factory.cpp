/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "sdm_factory.h"
#include <debug_handler.h>
#include <dlfcn.h>

#define __CLASS__ "SDMInterfaceFactory"

namespace sdm {

static SDMInterfaceFactoryImpl factory_ = {};

SDMInterfaceFactory *GetSDMInterfaceFactory() { return &factory_; }

// only used by sdm_display, not to be called by clients
SDMInterfaceFactoryImpl *SDMInterfaceFactoryImpl::GetSDMFactoryInternal() {
  return &factory_;
}

std::shared_ptr<ConcurrencyMgr> SDMInterfaceFactoryImpl::GetConcurrencyMgrInstance() {
  if (!concurrency_mgr_) {
    concurrency_mgr_ = std::make_shared<ConcurrencyMgr>();
  }

  return concurrency_mgr_;
}

std::shared_ptr<SDMLayerBuilder> SDMInterfaceFactoryImpl::GetLayerBuilderInstance() {
  if (!layer_builder_) {
    layer_builder_ = std::make_shared<SDMLayerBuilder>();
  }

  return layer_builder_;
}

std::shared_ptr<SDMDisplayCapsIntf> SDMInterfaceFactoryImpl::CreateCapsIntf() {
  std::shared_ptr<SDMDisplayCapsIntf> caps = GetConcurrencyMgrInstance();
  if (!caps) {
    DLOGE("Unable to open sdm capabilities interface");
    return nullptr;
  }

  return caps;
}

std::shared_ptr<SDMDisplayDrawCycleIntf> SDMInterfaceFactoryImpl::CreateDrawCycleIntf() {
  std::shared_ptr<SDMDisplayDrawCycleIntf> draw_cycle = GetConcurrencyMgrInstance();
  if (!draw_cycle) {
    DLOGE("Unable to open sdm draw cycle interface");
    return nullptr;
  }

  return draw_cycle;
}

std::shared_ptr<SDMDisplayLayerBuilderIntf> SDMInterfaceFactoryImpl::CreateLayerBuilderIntf() {
  std::shared_ptr<SDMDisplayLayerBuilderIntf> layer_builder = GetLayerBuilderInstance();
  if (!layer_builder) {
    DLOGE("Unable to open sdm layer builder interface");
    return nullptr;
  }

  return layer_builder;
}

std::shared_ptr<SDMDisplayLifeCycleIntf> SDMInterfaceFactoryImpl::CreateLifeCycleIntf() {
  std::shared_ptr<SDMDisplayLifeCycleIntf> life_cycle = GetConcurrencyMgrInstance();
  if (!life_cycle) {
    DLOGE("Unable to open sdm life cycle interface");
    return nullptr;
  }

  return life_cycle;
}

std::shared_ptr<SDMDisplaySettingsIntf> SDMInterfaceFactoryImpl::CreateSettingsIntf() {
  std::shared_ptr<SDMDisplaySettingsIntf> settings = GetConcurrencyMgrInstance();
  if (!settings) {
    DLOGE("Unable to open sdm settings interface");
    return nullptr;
  }

  return settings;
}

std::shared_ptr<SDMDisplaySideBandIntf> SDMInterfaceFactoryImpl::CreateSideBandIntf() {
  std::shared_ptr<SDMDisplaySideBandIntf> sideband = GetConcurrencyMgrInstance();
  if (!sideband) {
    DLOGE("Unable to open sdm sideband interface");
    return nullptr;
  }

  return sideband;
}

std::shared_ptr<SDMDisplayAiqeIntf> SDMInterfaceFactoryImpl::CreateAiqeIntf() {
  std::shared_ptr<SDMDisplayAiqeIntf> aqie_intf = GetConcurrencyMgrInstance();
  if (!aqie_intf) {
    DLOGI("Unable to retrieve aiqe intf");
    return nullptr;
  }

  return aqie_intf;
}

} // namespace sdm
