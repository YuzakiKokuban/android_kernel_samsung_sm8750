/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "PowerHintSession.h"
#include "utils.h"
#include "hint-data.h"
#include "performance.h"

#define CPU_BOOST_HINT      0x0000104E
#define THREAD_LOW_LATENCY  0x40CD0000
#define MAX_THREADS 32
#define MAX_BOOST 200
#define MIN_BOOST -200

#include <android-base/logging.h>
#include "android/binder_auto_utils.h"
#define LOG_TAG "QTI PowerHAL"

std::unordered_map<PowerHintSessionImpl*, int32_t> mPowerHintSessions;
std::mutex mSessionLock;

static int validateBoost(int boostVal, int boostSum) {
    boostSum += boostVal;
    if(boostSum > MAX_BOOST)
        return MAX_BOOST;
    else if(boostSum < MIN_BOOST)
        return MIN_BOOST;
    return boostSum;
}

void PowerHintSessionImpl::resetBoost() {
    if(mHandle > 0)
        release_request(mHandle);
    mHandle = -1;
    mLastAction = LOAD_RESET;
}

static std::string printThreads(std::vector<int32_t>& threadIds) {
    std::string str;
    for(int num : threadIds)
        str = str + std::to_string(num) + " ";
    return str;
}

bool PowerHintSessionImpl::perfBoost(int boostVal, int hintType) {
    int tBoostSum = mBoostSum;
    int mHandlePerfHint = -1;

    if(hintType == LOAD_RESET){
        resetBoost();
        return true;
    }

    if(hintType == LOAD_RESUME && mLastAction != LOAD_RESET) {
        tBoostSum = 0;
    }

    tBoostSum = validateBoost(boostVal, tBoostSum);
    if(tBoostSum != 0) {
        mHandlePerfHint = perf_hint_enable(CPU_BOOST_HINT, tBoostSum);
        if(mHandlePerfHint < 0) {
            LOG(ERROR) << "Unable to acquire Perf hint for" << CPU_BOOST_HINT;
            return false;
        }
    }

    if(mHandle > 0)
        release_request(mHandle);
    mBoostSum = tBoostSum;
    mHandle = mHandlePerfHint;
    mLastAction = hintType;
    return true;
}

bool isSessionAlive(PowerHintSessionImpl* session) {
    if(mPowerHintSessions.find(session) != mPowerHintSessions.end())
        return true;
    return false;
}

bool isSessionActive(PowerHintSessionImpl* session) {
    if(!isSessionAlive(session))
        return false;
    if(mPowerHintSessions[session] == 1)
        return true;
    return false;
}

std::shared_ptr<aidl::android::hardware::power::IPowerHintSession> setPowerHintSession(int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds){
    LOG(INFO) << "setPowerHintSession ";
    std::shared_ptr<aidl::android::hardware::power::IPowerHintSession> mPowerSession = ndk::SharedRefBase::make<PowerHintSessionImpl>(tgid, uid, threadIds);

    if(mPowerSession == nullptr) {
        return nullptr;
    }
    return mPowerSession;
}

int64_t getSessionPreferredRate(){
    return 16666666L;
}

void setSessionActivity(PowerHintSessionImpl* session, bool flag) {
    std::lock_guard<std::mutex> mLockGuard(mSessionLock);
    if(flag)
        mPowerHintSessions[session] = 1;
    else
        mPowerHintSessions[session] = 0;
}

PowerHintSessionImpl::PowerHintSessionImpl(int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds){
     mUid = uid;
     mTgid = tgid;
     mBoostSum = 0;
     mHandle = -1;
     mLastAction = -1;
     setSessionActivity(this, true);
     mThreadIds = threadIds;
     mThreadHandle = setThreadPipelining(mThreadIds);
}

PowerHintSessionImpl::~PowerHintSessionImpl(){
     close();
}

int PowerHintSessionImpl::setThreadPipelining(std::vector<int32_t>& threadIds) {
    int mHandleTid = -1;
    int size = threadIds.size() * 2;
    int args[MAX_THREADS];

    if(size > MAX_THREADS)
        return -1;

    for(int i=0; i<size; i+=2) {
        args[i] = THREAD_LOW_LATENCY;
        args[i+1] = threadIds[i/2];
    }

    mHandleTid = interaction_with_handle(0, 0, size, args);
    if(mHandleTid < 0) {
            LOG(ERROR) << "Unable to put these threads into pipeline";
            return -1;
    }

    LOG(INFO) << "Thread Low Latency Handle " << mHandleTid << " for threads " << printThreads(threadIds);
    return mHandleTid;
}

void PowerHintSessionImpl::removePipelining() {
    if(mThreadHandle > 0) {
        release_request(mThreadHandle);
        LOG(INFO) << "Handle " << mThreadHandle << " released";
    }
    mThreadHandle = -1;
}

void PowerHintSessionImpl::resumeThreadPipelining() {
    mThreadHandle = setThreadPipelining(mThreadIds);
}

ndk::ScopedAStatus PowerHintSessionImpl::updateTargetWorkDuration(int64_t in_targetDurationNanos){
    LOG(INFO) << "updateTargetWorkDuration " << in_targetDurationNanos;
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus PowerHintSessionImpl::reportActualWorkDuration(const std::vector<::aidl::android::hardware::power::WorkDuration>& in_durations){
    LOG(INFO) << "reportActualWorkDuration ";
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus PowerHintSessionImpl::pause(){
    LOG(INFO) << "PowerHintSessionImpl::pause ";
    if(isSessionAlive(this)) {
        setSessionActivity(this, false);
        sendHint(aidl::android::hardware::power::SessionHint::CPU_LOAD_RESET);
        removePipelining();
    }
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus PowerHintSessionImpl::resume(){
    LOG(INFO) << "PowerHintSessionImpl::resume ";
    if(isSessionAlive(this)) {
        sendHint(aidl::android::hardware::power::SessionHint::CPU_LOAD_RESUME);
        resumeThreadPipelining();
        setSessionActivity(this, true);
    }
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus PowerHintSessionImpl::close(){
    LOG(INFO) << "PowerHintSessionImpl::close ";

    if(isSessionAlive(this)) {
        sendHint(aidl::android::hardware::power::SessionHint::CPU_LOAD_RESET);
        removePipelining();
        mThreadIds.clear();

        mSessionLock.lock();
        mPowerHintSessions.erase(this);
        mSessionLock.unlock();
    }
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus PowerHintSessionImpl::sendHint(aidl::android::hardware::power::SessionHint hint){
    LOG(INFO) << "PowerHintSessionImpl::sendHint ";
    if(!isSessionActive(this))
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    switch(hint)
    {
        case aidl::android::hardware::power::SessionHint::CPU_LOAD_UP:
            perfBoost(20, LOAD_UP);
            break;
        case aidl::android::hardware::power::SessionHint::CPU_LOAD_DOWN:
            perfBoost(-20, LOAD_DOWN);
            break;
        case aidl::android::hardware::power::SessionHint::CPU_LOAD_RESET:
            perfBoost(0, LOAD_RESET);
            break;
        case aidl::android::hardware::power::SessionHint::CPU_LOAD_RESUME:
            perfBoost(0, LOAD_RESUME);
            break;
        case aidl::android::hardware::power::SessionHint::POWER_EFFICIENCY:
            perfBoost(-20, LOAD_DOWN);
            break;
    }
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus PowerHintSessionImpl::setThreads(const std::vector<int32_t>& threadIds){
    LOG(INFO) << "PowerHintSessionImpl::setThreads ";
    if (threadIds.size() == 0) {
        LOG(ERROR) << "Error: threadIds.size() shouldn't be " << threadIds.size();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    if(mThreadHandle > 0) {
        release_request(mThreadHandle);
        mThreadIds.clear();
        mThreadHandle = -1;
    }

    mThreadIds = threadIds;
    mThreadHandle = setThreadPipelining(mThreadIds);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerHintSessionImpl::setMode(aidl::android::hardware::power::SessionMode mode, bool enabled) {
	return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerHintSessionImpl::getSessionConfig(aidl::android::hardware::power::SessionConfig* _aidl_return) {
    _aidl_return->id = 1;
    return ndk::ScopedAStatus::ok();
}
