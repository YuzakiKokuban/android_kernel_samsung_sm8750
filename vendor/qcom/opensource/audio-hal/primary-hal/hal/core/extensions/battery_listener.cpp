/*
* Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#define LOG_TAG "AHAL_BatteryListener_QTI"
#include "extensions/battery_listener.h"
#include <aidl/android/hardware/health/BnHealthInfoCallback.h>
#include <aidl/android/hardware/health/IHealth.h>
#include <aidl/android/hardware/health/IHealthInfoCallback.h>
#include <android/binder_manager.h>
#include <log/log.h>
#include <thread>

using aidl::android::hardware::health::BatteryStatus;
using aidl::android::hardware::health::HealthInfo;
using aidl::android::hardware::health::IHealthInfoCallback;
using aidl::android::hardware::health::BnHealthInfoCallback;
using aidl::android::hardware::health::IHealth;
using namespace std::literals::chrono_literals;

namespace android {

#define GET_HEALTH_SVC_RETRY_CNT 5
#define GET_HEALTH_SVC_WAIT_TIME_MS 500

struct BatteryListenerImpl : public BnHealthInfoCallback {
    typedef std::function<void(bool)> cb_fn_t;
    BatteryListenerImpl(cb_fn_t cb);
    ~BatteryListenerImpl();
    ndk::ScopedAStatus healthInfoChanged(const HealthInfo &info) override;
    static void serviceDied(void *cookie);
    bool isCharging() {
        std::lock_guard<std::mutex> _l(mLock);
        return statusToBool(mStatus);
    }
    void reset();
    status_t init();

  private:
    std::shared_ptr<IHealth> mHealth;
    BatteryStatus mStatus;
    cb_fn_t mCb;
    std::mutex mLock;
    std::condition_variable mCond;
    std::unique_ptr<std::thread> mThread;
    ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
    bool mDone;
    bool statusToBool(const BatteryStatus &s) const {
        return (s == BatteryStatus::CHARGING) || (s == BatteryStatus::FULL);
    }
};

static std::shared_ptr<BatteryListenerImpl> batteryListener;

status_t BatteryListenerImpl::init() {
    int tries = 0;
    auto service_name = std::string() + IHealth::descriptor + "/default";

    if (mHealth != NULL) return INVALID_OPERATION;

    do {
        mHealth = IHealth::fromBinder(
                ndk::SpAIBinder(AServiceManager_getService(service_name.c_str())));
        if (mHealth != NULL) break;
        usleep(GET_HEALTH_SVC_WAIT_TIME_MS * 1000);
        tries++;
    } while (tries < GET_HEALTH_SVC_RETRY_CNT);

    if (mHealth == NULL) {
        ALOGE("no health service found, retries %d", tries);
        return NO_INIT;
    } else {
        ALOGI("Get health service in %d tries", tries);
    }
    mStatus = BatteryStatus::UNKNOWN;
    auto ret = mHealth->getChargeStatus(&mStatus);
    if (!ret.isOk()) ALOGE("batterylistener: get charge status transaction error");

    if (mStatus == BatteryStatus::UNKNOWN) ALOGW("batterylistener: init: invalid battery status");
    mDone = false;
    mThread = std::make_unique<std::thread>([this]() {
        std::unique_lock<std::mutex> l(mLock);
        BatteryStatus local_status = mStatus;
        while (!mDone) {
            if (local_status == mStatus) {
                mCond.wait(l);
                continue;
            }
            local_status = mStatus;
            switch (local_status) {
                // NOT_CHARGING is a special event that indicates, a battery is connected,
                // but not charging. This is seen for approx a second
                // after charger is plugged in. A charging event is eventually received.
                // We must try to avoid an unnecessary cb to HAL
                // only to call it again shortly.
                // An option to deal with this transient event would be to ignore this.
                // Or process this event with a slight delay (i.e cancel this event
                // if a different event comes in within a timeout
                case BatteryStatus::NOT_CHARGING: {
                    auto mStatusnot_ncharging = [this, local_status]() {
                        return mStatus != local_status;
                    };
                    mCond.wait_for(l, 3s, mStatusnot_ncharging);
                    if (mStatusnot_ncharging()) // i.e event changed
                        break;
                }
                    [[fallthrough]];
                default:
                    bool c = statusToBool(local_status);
                    ALOGI("healthInfo cb thread: cb %s", c ? "CHARGING" : "NOT CHARGING");
                    l.unlock();
                    mCb(c);
                    l.lock();
                    break;
            }
        }
    });
    mHealth->registerCallback(batteryListener);
    binder_status_t binder_status =
            AIBinder_linkToDeath(mHealth->asBinder().get(), mDeathRecipient.get(), this);
    if (binder_status != STATUS_OK) {
        ALOGE("Failed to link to death, status %d", static_cast<int>(binder_status));
        return NO_INIT;
    }
    return NO_ERROR;
}

BatteryListenerImpl::BatteryListenerImpl(cb_fn_t cb)
    : mCb(cb), mDeathRecipient(AIBinder_DeathRecipient_new(BatteryListenerImpl::serviceDied)) {}

BatteryListenerImpl::~BatteryListenerImpl() {
    {
        std::lock_guard<std::mutex> _l(mLock);
        mDone = true;
        mCond.notify_one();
    }
    mThread->join();
}

void BatteryListenerImpl::reset() {
    std::lock_guard<std::mutex> _l(mLock);
    if (mHealth != nullptr) {
        mHealth->unregisterCallback(batteryListener);
        binder_status_t status =
                AIBinder_unlinkToDeath(mHealth->asBinder().get(), mDeathRecipient.get(), this);
        if (status != STATUS_OK && status != STATUS_DEAD_OBJECT) ALOGE("Cannot unlink to death");
    }
    mStatus = BatteryStatus::UNKNOWN;
    mDone = true;
    mCond.notify_one();
}
void BatteryListenerImpl::serviceDied(void *cookie) {
    BatteryListenerImpl *listener = reinterpret_cast<BatteryListenerImpl *>(cookie);
    {
        std::lock_guard<std::mutex> _l(listener->mLock);
        if (listener->mHealth == NULL) {
            ALOGE("health not initialized");
            return;
        }
        ALOGI("health service died, reinit");
        listener->mDone = true;
        listener->mCond.notify_one();
    }
    listener->mThread->join();
    std::lock_guard<std::mutex> _l(listener->mLock);
    listener->mHealth = NULL;
    listener->init();
}

// this callback seems to be a SYNC callback and so
// waits for return before next event is issued.
// therefore we need not have a queue to process
// NOT_CHARGING and CHARGING concurrencies.
// Replace single var by a list if this assumption is broken
ndk::ScopedAStatus BatteryListenerImpl::healthInfoChanged(const HealthInfo &info) {
    ALOGV("healthInfoChanged: %d", info.batteryStatus);
    std::unique_lock<std::mutex> l(mLock);
    if (info.batteryStatus != mStatus) {
        mStatus = info.batteryStatus;
        mCond.notify_one();
    }
    return ndk::ScopedAStatus::ok();
}

status_t batteryPropertiesListenerInit(BatteryListenerImpl::cb_fn_t cb) {
    batteryListener = ndk::SharedRefBase::make<BatteryListenerImpl>(cb);
    return batteryListener->init();
}

status_t batteryPropertiesListenerDeinit() {
    batteryListener->reset();
    return OK;
}

bool batteryPropertiesListenerIsCharging() {
    return batteryListener->isCharging();
}

} // namespace android

extern "C" {
void battery_properties_listener_init(battery_status_change_fn_t fn) {
    android::batteryPropertiesListenerInit([=](bool charging) { fn(charging); });
}

void battery_properties_listener_deinit() {
    android::batteryPropertiesListenerDeinit();
}

bool battery_properties_is_charging() {
    return android::batteryPropertiesListenerIsCharging();
}

} // extern C
