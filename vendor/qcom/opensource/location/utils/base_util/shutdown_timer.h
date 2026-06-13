/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
  =============================================================================*/
#ifndef SHUTDOWN_TIMER_H
#define SHUTDOWN_TIMER_H

#include "LocTimer.h"
#include "loc_cfg.h"
#include "log_util.h"
#include <string>
#include <functional>
#define PROCESS_SHUTDOWN_TIMEOUT_IN_MS 300000

namespace qc_loc_fw {
class ProcessShutdownTimer : public loc_util::LocTimer {
public:
    ~ProcessShutdownTimer() = default;
    ProcessShutdownTimer(std::string name, const std::function<void()> shutdownCb) :
            loc_util::LocTimer(), mShutdownCb(shutdownCb), mActive(false),
            mShutdownTimerInMs(PROCESS_SHUTDOWN_TIMEOUT_IN_MS), mLaunchTriggerMask(0),
            mProcessName(name) {
        const loc_param_s_type gpsConfTable[] = {
            {"PROCESS_SHUTDOWN_TIMER_IN_MS", &mShutdownTimerInMs, NULL, 'n'}};
        UTIL_READ_CONF(LOC_PATH_GPS_CONF, gpsConfTable);
    }
    inline void start() {
        if (mLaunchTriggerMask > 0) {
            LOC_LOGd("Started shutdown timer for process %s", mProcessName.c_str());
            mActive = true;
            loc_util::LocTimer::start(mShutdownTimerInMs, false);
        }
    }

    inline void stop() {
       if (mLaunchTriggerMask > 0) {
        LOC_LOGd("Stopped shutdown timer for process %s", mProcessName.c_str());
        mActive = false;
        loc_util::LocTimer::stop();
        }
    }

    inline void restart() {
        if (mLaunchTriggerMask > 0) {
        LOC_LOGd("Restarted shutdown timer for process %s", mProcessName.c_str());
            stop();
            start();
        }
    }
    inline bool isActive() {
        return mActive;
    }
    inline void setLaunchTriggerMask(int16_t launchTriggerMask) {
        mLaunchTriggerMask = launchTriggerMask;
    }
private:
    inline virtual void timeOutCallback() override {
        mShutdownCb();
    }
    const std::function<void()> mShutdownCb;
    bool mActive;
    std::string mProcessName;
    uint32_t mShutdownTimerInMs;
    uint16_t mLaunchTriggerMask;
};
} // namespace qc_loc_fw
#endif /* SHUTDOWN_TIMER_H */
