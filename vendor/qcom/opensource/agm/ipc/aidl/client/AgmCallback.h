/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <agm/agm_api.h>
#include <aidl/vendor/qti/hardware/agm/IAGM.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include <agm/AgmLegacyToAidl.h>
#include <agm/BinderStatus.h>
#include <aidl/vendor/qti/hardware/agm/BnAGMCallback.h>
#include <log/log.h>

namespace aidl::vendor::qti::hardware::agm {
class AgmCallback : public BnAGMCallback {
  public:
    AgmCallback(uint32_t sessionId, agm_event_cb callback, uint32_t event, void* clientData);
    virtual ~AgmCallback();

    ::ndk::ScopedAStatus eventCallback(
            const ::aidl::vendor::qti::hardware::agm::AgmEventCallbackParameter& in_eventParam)
            override;

    ::ndk::ScopedAStatus eventCallbackReadWriteDone(
            const ::aidl::vendor::qti::hardware::agm::AgmReadWriteEventCallbackParams&
                    in_rwDonePayload) override;

  private:
    uint32_t mSessionId = 0;
    agm_event_cb mCallback = NULL;
    uint32_t mEvent = 0;
    void* mClientData = NULL;
};
}