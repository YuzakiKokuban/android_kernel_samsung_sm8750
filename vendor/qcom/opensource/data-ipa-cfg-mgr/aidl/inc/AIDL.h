/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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
#ifndef _AIDL_H_
#define _AIDL_H_

/* AIDL Includes */
#include <aidl/android/hardware/tetheroffload/BnOffload.h>

/* External Includes */
#include <string>
#include <vector>

/* Internal Includes */
#include "CtUpdateAmbassador.h"
#include "IOffloadManager.h"
#include "IpaEventRelay.h"
#include "LocalLogBuffer.h"

/* Avoid the namespace litering everywhere */
using RET = ::IOffloadManager::RET;
using Prefix = ::IOffloadManager::Prefix;

using ::std::string;
using ::std::vector;
using ::std::shared_ptr;

using ::ndk::ScopedAStatus;
using ::ndk::ScopedFileDescriptor;

using namespace aidl::android::hardware::tetheroffload;
using aidl::android::hardware::tetheroffload::ForwardedStats;
using aidl::android::hardware::tetheroffload::ITetheringOffloadCallback;

#define KERNEL_PAGE 4096

class AIDL : public BnOffload {
public:
    /* Static Const Definitions */
    static const uint32_t UDP_SUBSCRIPTIONS =
            NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY;
    static const uint32_t TCP_SUBSCRIPTIONS =
            NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY;

    /* Interface to IPACM */
    /**
     * @TODO This will likely need to be extended into a proper FactoryPattern
     * when version bumps are needed.
     *
     * IPACM does not need to talk directly back to the returned HAL class.  The other methods that
     * IPACM needs to call are covered by registering the event listeners.  If IPACM did need to
     * talk directly back to the HAL object, without HAL registering a callback, these methods would
     * need to be defined in the HAL base class.
     *
     * This would slightly break backwards compatibility so it should be discouraged; however, the
     * base class could define a sane default implementation and not require that the child class
     * implement this new method.  This "sane default implementation" might only be possible in the
     * case of listening to async events; if IPACM needs to query something, then this would not
     * be backwards compatible and should be done via registering a callback so that IPACM knows
     * this version of HAL supports that functionality.
     *
     * The above statements assume that only one version of the HAL will be instantiated at a time.
     * Yet, it seems possible that a HAL_V1 and HAL_V2 service could both be registered, extending
     * support to both old and new client implementations.  It would be difficult to multiplex
     * information from both versions.  Additionally, IPACM would be responsible for instantiating
     * two HALs (makeIPAAIDL(1, ...); makeIPAAIDL(2, ...)) which makes signaling between HAL versions
     * (see next paragraph) slightly more difficult but not impossible.
     *
     * If concurrent versions of HAL are required, there will likely need to only be one master.
     * Whichever version of HAL receives a client first may be allowed to take over control while
     * other versions would be required to return failures (ETRYAGAIN: another version in use) until
     * that version of the client relinquishes control.  This should work seemlessly because we
     * currently have an assumption that only one client will be present in system image.
     * Logically, that client will have only a single version (or if it supports multiple, it will
     * always attempt the newest version of HAL before falling back) and therefore no version
     * collisions could possibly occur.
     *
     * Dislaimer:
     * ==========
     * Supporting multiple versions of an interface, in the same code base, at runtime, comes with a
     * significant carrying cost and overhead in the form of developer headaches.  This should not
     * be done lightly and should be extensively scoped before committing to the effort.
     *
     * Perhaps the notion of minor version could be introduced to bridge the gaps created above.
     * For example, 1.x and 1.y could be ran concurrently and supported from the same IPACM code.
     * Yet, a major version update, would not be backwards compatible.  This means that a 2.x HAL
     * could not linked into the same IPACM code base as a 1.x HAL.
     */
    AIDL(IOffloadManager*  /* mgr */);

    static shared_ptr<AIDL> makeIPAAIDL(int /* version */, IOffloadManager* /* mgr */);

    ScopedAStatus initOffload(
            const ScopedFileDescriptor& /* fd1 */,
            const ScopedFileDescriptor& /* fd2 */,
            const shared_ptr<ITetheringOffloadCallback>&  /* cb */) override;
    ScopedAStatus stopOffload() override;
    ScopedAStatus setLocalPrefixes(
            const vector<string>& /* prefixes */) override;
    ScopedAStatus getForwardedStats(
            const string& /* upstream */,
            ForwardedStats* /* aidl_return */) override;
    ScopedAStatus setUpstreamParameters(
            const string& /* iface */,
            const string& /* v4Addr */,
            const string& /* v4Gw */,
            const vector<string>& /* v6Gws */) override;
    ScopedAStatus addDownstream(
            const string& /* iface */,
            const string& /* prefix */) override;
    ScopedAStatus removeDownstream(
            const string& /* iface */,
            const string& /* prefix */) override;
    ScopedAStatus setDataWarningAndLimit(
            const string& /* upstream */,
            int64_t /* warningBytes */,
            int64_t /* limitBytes */) override;

    static void clientDeathCallback(void* cookie);

private:
    typedef struct BoolResult {
        bool success;
        string errMsg;
    } boolResult_t;

    void doLogcatDump();

    static BoolResult makeInputCheckFailure(string /* customErr */);
    static BoolResult ipaResultToBoolResult(RET /* in */);

    void registerEventListeners();
    void registerIpaCb();
    void registerCtCb();
    void unregisterEventListeners();
    void unregisterIpaCb();
    void unregisterCtCb();

    void clearHandles();

    bool isInitialized();

    IOffloadManager* mIPA;
    ScopedFileDescriptor mHandle1;
    ScopedFileDescriptor mHandle2;
    LocalLogBuffer mLogs;
    shared_ptr<ITetheringOffloadCallback> mCb;
    IpaEventRelay *mCbIpa;
    CtUpdateAmbassador *mCbCt;

    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
}; /* AIDL */
#endif /* _AIDL_H_ */
