/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <aidl/vendor/qti/hardware/paleventnotifier/BnPALEventNotifier.h>
#include <aidl/vendor/qti/hardware/paleventnotifier/IPALEventNotifierCallback.h>
#include <aidl/vendor/qti/hardware/paleventnotifier/IPALEventNotifier.h>
#include <log/log.h>
#include "PalApi.h"


using PalCallbackConfig = ::aidl::vendor::qti::hardware::paleventnotifier::PalCallbackConfig;
using PalStreamType = ::aidl::vendor::qti::hardware::paleventnotifier::PalStreamType;

namespace aidl::vendor::qti::hardware::paleventnotifier {

class PalServerNotify : public BnPALEventNotifier {

  public:
    virtual ~PalServerNotify() {}
    ::ndk::ScopedAStatus ipc_pal_notify_register_callback(const std::shared_ptr<IPALEventNotifierCallback>& cb, int* ret) override;
};

}
