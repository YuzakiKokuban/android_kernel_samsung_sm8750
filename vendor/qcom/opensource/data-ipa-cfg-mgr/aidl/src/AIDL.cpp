/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
 */
/*
 * ​​​​​Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef DBG
    #define DBG true
#endif /* DBG */
#define LOG_TAG "IPAAIDLService"

/* Kernel Includes */
#include <linux/netfilter/nfnetlink_compat.h>

/* External Includes */
#include <cutils/log.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <android/binder_manager.h>

/* Internal Includes */
#include "AIDL.h"
#include "LocalLogBuffer.h"
#include "PrefixParser.h"

/* Namespace pollution avoidance */

using RET = ::IOffloadManager::RET;
using Prefix = ::IOffloadManager::Prefix;

using ::std::vector;

/* ------------------------------ PUBLIC ------------------------------------ */
::std::shared_ptr<AIDL> AIDL::makeIPAAIDL(int version, IOffloadManager* mgr) {
    if (DBG)
        ALOGI("makeIPAAIDL(%d, %s)", version,
                (mgr != nullptr) ? "provided" : "null");
    if (nullptr == mgr) return NULL;
    else if (version != 1) return NULL;
    ::std::shared_ptr<AIDL> ret = ndk::SharedRefBase::make<AIDL>(mgr);
    if (nullptr == ret) return NULL;
    const std::string instance = std::string(AIDL::descriptor) + "/default";
    AServiceManager_addService(ret->asBinder().get(), instance.c_str());
    return ret;
} /* makeIPAAIDL */

AIDL::AIDL(IOffloadManager* mgr) : mLogs("AIDL Function Calls", 50) {
    mIPA = mgr;
    mCb = nullptr;
    mCbIpa = nullptr;
    mCbCt = nullptr;
} /* AIDL */


/* ------------------------------ PRIVATE ----------------------------------- */
void AIDL::doLogcatDump() {
    ALOGD("mHandles");
    ALOGD("========");
    /* @TODO This will segfault if they aren't initialized and I don't currently
     * care to check for initialization in a function that isn't used anyways
     * ALOGD("fd1->%d", mHandle1.get());
     * ALOGD("fd2->%d", mHandle2.get());
     */
    ALOGD("========");
} /* doLogcatDump */

AIDL::BoolResult AIDL::makeInputCheckFailure(string customErr) {
    BoolResult ret;
    ret.success = false;
    ret.errMsg = "Failed Input Checks: " + customErr;
    return ret;
} /* makeInputCheckFailure */

AIDL::BoolResult AIDL::ipaResultToBoolResult(RET in) {
    BoolResult ret;
    ret.success = (in >= RET::SUCCESS);
    switch (in) {
        case RET::FAIL_TOO_MANY_PREFIXES:
            ret.errMsg = "Too Many Prefixes Provided";
            break;
        case RET::FAIL_UNSUPPORTED:
            ret.errMsg = "Unsupported by Hardware";
            break;
        case RET::FAIL_INPUT_CHECK:
            ret.errMsg = "Failed Input Checks";
            break;
        case RET::FAIL_HARDWARE:
            ret.errMsg = "Hardware did not accept";
            break;
        case RET::FAIL_TRY_AGAIN:
            ret.errMsg = "Try Again";
            break;
        case RET::SUCCESS:
            ret.errMsg = "Successful";
            break;
        case RET::SUCCESS_DUPLICATE_CONFIG:
            ret.errMsg = "Successful: Was a duplicate configuration";
            break;
        case RET::SUCCESS_NO_OP:
            ret.errMsg = "Successful: No action needed";
            break;
        case RET::SUCCESS_OPTIMIZED:
            ret.errMsg = "Successful: Performed optimized version of action";
            break;
        default:
            ret.errMsg = "Unknown Error";
            break;
    }
    return ret;
} /* ipaResultToBoolResult */

void AIDL::registerEventListeners() {
    registerIpaCb();
    registerCtCb();
} /* registerEventListeners */

void AIDL::registerIpaCb() {
    if (isInitialized() && mCbIpa == nullptr) {
        LocalLogBuffer::FunctionLog fl("registerEventListener");
        mCbIpa = new IpaEventRelay(mCb);
        mIPA->registerEventListener(mCbIpa);
        mLogs.addLog(fl);
    } else {
        ALOGE("Failed to registerIpaCb (isInitialized()=%s, (mCbIpa == nullptr)=%s)",
                isInitialized() ? "true" : "false",
                (mCbIpa == nullptr) ? "true" : "false");
    }
} /* registerIpaCb */

void AIDL::registerCtCb() {
    if (isInitialized() && mCbCt == nullptr) {
        LocalLogBuffer::FunctionLog fl("registerCtTimeoutUpdater");
        // We can allways use the 1.0 callback here since it is always guarenteed to
        // be non-nullptr if any version is created.
        mCbCt = new CtUpdateAmbassador(mCb);
        mIPA->registerCtTimeoutUpdater(mCbCt);
        mLogs.addLog(fl);
    } else {
        ALOGE("Failed to registerCtCb (isInitialized()=%s, (mCbCt == nullptr)=%s)",
                isInitialized() ? "true" : "false",
                (mCbCt == nullptr) ? "true" : "false");
    }
} /* registerCtCb */

void AIDL::unregisterEventListeners() {
    unregisterIpaCb();
    unregisterCtCb();
} /* unregisterEventListeners */

void AIDL::unregisterIpaCb() {
    if (mCbIpa != nullptr) {
        LocalLogBuffer::FunctionLog fl("unregisterEventListener");
        mIPA->unregisterEventListener(mCbIpa);
        mCbIpa = nullptr;
        mLogs.addLog(fl);
    } else {
        ALOGE("Failed to unregisterIpaCb");
    }
} /* unregisterIpaCb */

void AIDL::unregisterCtCb() {
    if (mCbCt != nullptr) {
        LocalLogBuffer::FunctionLog fl("unregisterCtTimeoutUpdater");
        mIPA->unregisterCtTimeoutUpdater(mCbCt);
        mCbCt = nullptr;
        mLogs.addLog(fl);
    } else {
        ALOGE("Failed to unregisterCtCb");
    }
} /* unregisterCtCb */

void AIDL::clearHandles() {
    ALOGI("clearHandles()");
    mHandle1.release();
    mHandle2.release();
} /* clearHandles */

bool AIDL::isInitialized() {
    // Only have to check 1.0 Callback since it will always be created.
    return mCb.get() != nullptr;
} /* isInitialized */

void AIDL::clientDeathCallback(void* cookie)
{
  AIDL* self = reinterpret_cast<AIDL*>(cookie);
  self->stopOffload();
}


/* -------------------------- IOffload ------------------------------- */
ScopedAStatus AIDL::initOffload
(
    const ScopedFileDescriptor& fd1,
    const ScopedFileDescriptor& fd2,
    const shared_ptr<ITetheringOffloadCallback>& cb
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    BoolResult res;

    if (isInitialized()) {
        res = makeInputCheckFailure("Already initialized");
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, res.errMsg.c_str());
    }

    if (fd1.get() < 0) {
        res = makeInputCheckFailure("Invalid file descriptor (fd1)");
        fl.setResult(res.success, res.errMsg);

        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    }

    if (fd2.get() < 0) {
        res = makeInputCheckFailure("Invalid file descriptor (fd2)");
        fl.setResult(res.success, res.errMsg);

        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    }

    /* Dup the FD so we have our own copy. */
    mHandle1 = ndk::ScopedFileDescriptor(dup(fd1.get()));
    mHandle2 = ndk::ScopedFileDescriptor(dup(fd2.get()));

    /* Log the DUPed FD instead of the actual input FD so that we can lookup
     * this value in ls -l /proc/<pid>/<fd>
     */
    fl.addArg("fd1", mHandle1.get());
    fl.addArg("fd2", mHandle2.get());

    RET ipaReturn = mIPA->provideFd(mHandle1.get(), UDP_SUBSCRIPTIONS);
    if (ipaReturn == RET::SUCCESS) {
        ipaReturn = mIPA->provideFd(mHandle2.get(), TCP_SUBSCRIPTIONS);
    }

    if (ipaReturn != RET::SUCCESS) {
        ALOGE("IPACM failed to accept the FDs (%d %d)", mHandle1.get(), mHandle2.get());
        clearHandles();
    } else {
        /* @TODO remove logs after stabilization */
        ALOGI("IPACM was provided two FDs (%d, %d)", mHandle1.get(), mHandle2.get());
    }

    res = ipaResultToBoolResult(ipaReturn);
    if (!res.success) {
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    }

    /* Should storing the CB be a function? */
    mCb = cb;
    // As long as 1 callback version is supported we are fine.
    if (mCb == nullptr) {
        res = makeInputCheckFailure("callbacks are nullptr");
    } else {
        registerEventListeners();
        res = ipaResultToBoolResult(RET::SUCCESS);
    }

    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(
          AIBinder_DeathRecipient_new(&clientDeathCallback));
    AIBinder_linkToDeath(mCb->asBinder().get(), mDeathRecipient.get(),
          reinterpret_cast<void*>(this));

    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);

    if (!res.success)
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    return ndk::ScopedAStatus::ok();
} /* initOffload */

ScopedAStatus AIDL::stopOffload() {
    LocalLogBuffer::FunctionLog fl(__func__);

    if (!isInitialized()) {
        BoolResult res = makeInputCheckFailure("Was never initialized");
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, res.errMsg.c_str());
    }

    /* Should removing the CB be a function? */
    AIBinder_unlinkToDeath(mCb->asBinder().get(), mDeathRecipient.get(),
            reinterpret_cast<void*>(this));
    mCb = nullptr;
    unregisterEventListeners();

    RET ipaReturn = mIPA->stopAllOffload();
    if (ipaReturn != RET::SUCCESS) {
        /* Ignore IPAs return value here and provide why stopAllOffload
         * failed.  However, if IPA failed to clearAllFds, then we can't
         * clear our map because they may still be in use.
         */
        RET ret = mIPA->clearAllFds();
        if (ret == RET::SUCCESS) {
            clearHandles();
        }
    } else {
        ipaReturn = mIPA->clearAllFds();
        /* If IPA fails, they may still be using these for some reason. */
        if (ipaReturn == RET::SUCCESS) {
            clearHandles();
        } else {
            ALOGE("IPACM failed to return success for clearAllFds so they will not be released...");
        }
    }

    BoolResult res = ipaResultToBoolResult(ipaReturn);

    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);

    if (!res.success)
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    return ndk::ScopedAStatus::ok();
} /* stopOffload */

ScopedAStatus AIDL::setLocalPrefixes
(
    const vector<string>& prefixes
) {
    BoolResult res;
    PrefixParser parser;

    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("prefixes", prefixes);

    memset(&res,0,sizeof(BoolResult));

    if (!isInitialized()) {
        res = makeInputCheckFailure("Not initialized");
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, res.errMsg.c_str());
    }

    for (size_t i = 0; i < prefixes.size(); i++) {
        size_t pos = prefixes[i].find("/");
        if (pos == string::npos) {
            res = makeInputCheckFailure("Invalid prefix");
            fl.setResult(res.success, res.errMsg);
            mLogs.addLog(fl);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
        }
    }

    if(prefixes.size() < 1) {
        res = ipaResultToBoolResult(RET::FAIL_INPUT_CHECK);
    } else if (!parser.add(prefixes)) {
        res = makeInputCheckFailure(parser.getLastErrAsStr());
    } else {
        res = ipaResultToBoolResult(RET::SUCCESS);
    }

    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);

    if (!res.success)
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    return ndk::ScopedAStatus::ok();
} /* setLocalPrefixes */

ScopedAStatus AIDL::getForwardedStats
(
    const string& upstream,
    ForwardedStats* aidl_return
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("upstream", upstream);

    OffloadStatistics ret;
    ForwardedStats stats;
    RET ipaReturn = mIPA->getStats(upstream.c_str(), true, ret);
    if (ipaReturn == RET::SUCCESS) {
        stats.rxBytes = ret.getTotalRxBytes();
        stats.txBytes = ret.getTotalTxBytes();
        fl.setResult(ret.getTotalRxBytes(), ret.getTotalTxBytes());
    } else {
        /* @TODO Ensure the output is zeroed, but this is probably not enough to
         * tell Framework that an error has occurred.  If, for example, they had
         * not yet polled for statistics previously, they may incorrectly assume
         * that simply no statistics have transpired on hardware path.
         *
         * Maybe ITetheringOffloadCallback:onEvent(OFFLOAD_STOPPED_ERROR) is
         * enough to handle this case, time will tell.
         */
        stats.rxBytes = 0;
        stats.txBytes = 0;
        fl.setResult(0, 0);
    }

    *aidl_return = std::move(stats);
    mLogs.addLog(fl);
    return ndk::ScopedAStatus::ok();
} /* getForwardedStats */

ScopedAStatus AIDL::setUpstreamParameters
(
    const string& iface,
    const string& v4Addr,
    const string& v4Gw,
    const vector<string>& v6Gws
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("iface", iface);
    fl.addArg("v4Addr", v4Addr);
    fl.addArg("v4Gw", v4Gw);
    fl.addArg("v6Gws", v6Gws);

    BoolResult res;

    PrefixParser v4AddrParser;
    PrefixParser v4GwParser;
    PrefixParser v6GwParser;

    /* @TODO maybe we should enforce that these addresses and gateways are fully
     * qualified here.  But then, how do we allow them to be empty/null as well
     * while still preserving a sane API on PrefixParser?
     */
    if (!isInitialized()) {
        res = makeInputCheckFailure("Not initialized (setUpstreamParameters)");
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, res.errMsg.c_str());
    }

    if (!v4AddrParser.addV4(v4Addr) && !v4Addr.empty()) {
        res = makeInputCheckFailure(v4AddrParser.getLastErrAsStr());
    } else if (!v4GwParser.addV4(v4Gw) && !v4Gw.empty()) {
        res = makeInputCheckFailure(v4GwParser.getLastErrAsStr());
    } else if (v6Gws.size() >= 1 && !v6GwParser.addV6(v6Gws)) {
        res = makeInputCheckFailure(v6GwParser.getLastErrAsStr());
    } else if (iface.size()>= 1) {
        RET ipaReturn = mIPA->setUpstream(
                iface.c_str(),
                v4GwParser.getFirstPrefix(),
                v6GwParser.getFirstPrefix());
        res = ipaResultToBoolResult(ipaReturn);
    } else {
    /* send NULL iface string when upstream down */
        RET ipaReturn = mIPA->setUpstream(
                NULL,
                v4GwParser.getFirstPrefix(IP_FAM::V4),
                v6GwParser.getFirstPrefix(IP_FAM::V6));
        res = ipaResultToBoolResult(ipaReturn);
    }

    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);

    if (!res.success)
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    return ndk::ScopedAStatus::ok();
} /* setUpstreamParameters */

ScopedAStatus AIDL::addDownstream
(
    const string& iface,
    const string& prefix
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("iface", iface);
    fl.addArg("prefix", prefix);

    BoolResult res;
    PrefixParser prefixParser;

    if (!isInitialized()) {
        res = makeInputCheckFailure("Not initialized (addDownstream)");
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, res.errMsg.c_str());
    }

    if (!prefixParser.add(prefix)) {
        res = makeInputCheckFailure(prefixParser.getLastErrAsStr());
    } else {
        RET ipaReturn = mIPA->addDownstream(
                iface.c_str(),
                prefixParser.getFirstPrefix());
        res = ipaResultToBoolResult(ipaReturn);
    }

    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);

    if (!res.success)
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    return ndk::ScopedAStatus::ok();
} /* addDownstream */

ScopedAStatus AIDL::removeDownstream
(
        const string& iface,
        const string& prefix
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("iface", iface);
    fl.addArg("prefix", prefix);

    PrefixParser prefixParser;
    BoolResult res;

    if (!isInitialized()) {
        res = makeInputCheckFailure("Not initialized (removeDownstream)");
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, res.errMsg.c_str());
    }

    if (!prefixParser.add(prefix)) {
        res = makeInputCheckFailure(prefixParser.getLastErrAsStr());
    } else {
        RET ipaReturn = mIPA->removeDownstream(
                iface.c_str(),
                prefixParser.getFirstPrefix());
        res = ipaResultToBoolResult(ipaReturn);
    }

    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);

    if (!res.success)
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    return ndk::ScopedAStatus::ok();
} /* removeDownstream */

ScopedAStatus AIDL::setDataWarningAndLimit
(
    const string& upstream,
    int64_t warningBytes,
    int64_t limitBytes
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("upstream", upstream);
    fl.addArg("warningBytes", warningBytes);
    fl.addArg("limitBytes", limitBytes);

    BoolResult res;

    if (!isInitialized()) {
        res = makeInputCheckFailure("Not initialized (setDataWarningAndLimit)");
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, res.errMsg.c_str());
    }

    if (warningBytes < 0 || limitBytes < 0) {
        res = makeInputCheckFailure("Warning and limit bytes cannot be negative");
    } else {
        RET ipaReturn = mIPA->setQuotaWarning(upstream.c_str(), limitBytes, warningBytes);
        if(ipaReturn == RET::FAIL_TRY_AGAIN) {
            ipaReturn = RET::SUCCESS;
        }
        res = ipaResultToBoolResult(ipaReturn);
    }

    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);

    if (!res.success)
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, res.errMsg.c_str());
    return ndk::ScopedAStatus::ok();
} /* setDataWarningAndLimit */
