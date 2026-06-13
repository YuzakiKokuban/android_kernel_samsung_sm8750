/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef SDM_INTERFACE_FACTORY
#define SDM_INTERFACE_FACTORY

#pragma once
#include "sdm_interface_factory.h"
#include "concurrency_mgr.h"
#include <memory>

namespace sdm {

class SDMInterfaceFactoryImpl : public SDMInterfaceFactory {
public:
 std::shared_ptr<SDMDisplayCapsIntf> CreateCapsIntf();
 std::shared_ptr<SDMDisplayDrawCycleIntf> CreateDrawCycleIntf();
 std::shared_ptr<SDMDisplayLayerBuilderIntf> CreateLayerBuilderIntf();
 std::shared_ptr<SDMDisplayLifeCycleIntf> CreateLifeCycleIntf();
 std::shared_ptr<SDMDisplaySettingsIntf> CreateSettingsIntf();
 std::shared_ptr<SDMDisplaySideBandIntf> CreateSideBandIntf();
 std::shared_ptr<SDMDisplayAiqeIntf> CreateAiqeIntf();

 // not to be called by sdmclient clients, only used for sdm_display's
 // to get access to layer stacks
 static SDMInterfaceFactoryImpl *GetSDMFactoryInternal();
 std::shared_ptr<SDMLayerBuilder> GetLayerBuilderInternal() { return layer_builder_; }

private:
 std::shared_ptr<ConcurrencyMgr> GetConcurrencyMgrInstance();
 std::shared_ptr<SDMLayerBuilder> GetLayerBuilderInstance();

 std::shared_ptr<ConcurrencyMgr> concurrency_mgr_;
 std::shared_ptr<SDMLayerBuilder> layer_builder_;
};

} // namespace sdm

#endif
