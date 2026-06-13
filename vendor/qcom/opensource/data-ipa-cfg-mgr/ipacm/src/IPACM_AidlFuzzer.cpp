/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "IPACM_CmdQueue.h"
#include "IPACM_EvtDispatcher.h"
#include "IPACM_Defs.h"
#include "IPACM_Neighbor.h"
#include "IPACM_IfaceManager.h"
#include "IPACM_Log.h"
#include "IPACM_Wan.h"
#include "IPACM_LanToLan.h"
#include "IPACM_ConntrackListener.h"
#include "IPACM_ConntrackClient.h"
#include "IPACM_Netlink.h"
#include "IPACM_OffloadManager.h"
#include <AIDL.h>

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>

std::shared_ptr<AIDL> service;
uint32_t ipacm_event_stats[IPACM_EVENT_MAX];

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    IPACM_OffloadManager* OffloadMng = IPACM_OffloadManager::GetInstance();
    service = AIDL::makeIPAAIDL(1, OffloadMng);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if( service == nullptr ) return 0;

    FuzzedDataProvider provider(data, size);
    android::fuzzService(service->asBinder().get(), std::move(provider));

    return 0;
}
