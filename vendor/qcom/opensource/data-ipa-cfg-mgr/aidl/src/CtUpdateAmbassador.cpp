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
    #define DBG false
#endif /* DBG */
#define LOG_TAG "IPAHALService/CtUpdateAmbassador"

/* External Includes */
#include <arpa/inet.h>
#include <cutils/log.h>

/* AIDL Includes */
#include <aidl/android/hardware/tetheroffload/BnTetheringOffloadCallback.h>

/* Internal Includes */
#include "CtUpdateAmbassador.h"

/* Namespace pollution avoidance */
using aidl::android::hardware::tetheroffload::ITetheringOffloadCallback;
using aidl::android::hardware::tetheroffload::NetworkProtocol;
using AIDLIpAddrPortPair = aidl::android::hardware::tetheroffload::IPv4AddrPortPair;
using AIDLNatTimeoutUpdate = aidl::android::hardware::tetheroffload::NatTimeoutUpdate;

using IpaIpAddrPortPair = ::IOffloadManager::ConntrackTimeoutUpdater::IpAddrPortPair;
using IpaNatTimeoutUpdate = ::IOffloadManager::ConntrackTimeoutUpdater::NatTimeoutUpdate;
using IpaL4Protocol = ::IOffloadManager::ConntrackTimeoutUpdater::L4Protocol;


CtUpdateAmbassador::CtUpdateAmbassador(
        const shared_ptr<ITetheringOffloadCallback>& cb) : mFramework(cb) {
} /* CtUpdateAmbassador */

void CtUpdateAmbassador::updateTimeout(IpaNatTimeoutUpdate in) {
    if (DBG) {
        ALOGD("updateTimeout(src={%#010X, %#04X}, dest={%#010X, %#04X}, Proto=%d)",
                in.src.ipAddr, in.src.port, in.dst.ipAddr, in.dst.port,
                in.proto);
    }
    AIDLNatTimeoutUpdate out;
    if (!translate(in, out)) {
        /* Cannot log the input outside of DBG flag because it contains sensitive
         * information.  This will lead to a two step debug if the information
         * cannot be gleaned from IPACM logs.  The other option is to improve this
         * with the use of our local log.  That would likely still be hard to
         * instruct testers to collect logs, because, assuming timeout updates
         * are numerous, it will overrun the ring quickly.  Therefore, the tester
         * would have to know the exact moment as issue occurred.  Or we make the
         * ring massive.  This would lead to a significant memory overhead.
         * Because of this overhead, we would likely not want to check in a change
         * with it and once we provide a debug build for increasing buffer size,
         * why not just define the DBG flag?
         */
        ALOGE("Failed to translate timeout event :(");
    } else {
        auto ret = mFramework->updateTimeout(out);
        if (!ret.isOk()) {
            ALOGE("Triggering updateTimeout Callback failed.");
        }
    }
} /* updateTimeout */

bool CtUpdateAmbassador::translate(IpaNatTimeoutUpdate in, AIDLNatTimeoutUpdate &out) {
    return translate(in.src, out.src)
            && translate(in.dst, out.dst)
            && L4ToNetwork(in.proto, out.proto);
} /* translate */

bool CtUpdateAmbassador::translate(IpaIpAddrPortPair in, AIDLIpAddrPortPair& out) {
    char ipAddrStr[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, &(in.ipAddr), ipAddrStr, INET_ADDRSTRLEN) == nullptr) {
        /* errno would be valid here with EAFNOSUPPORT or ENOSPC, neither should really
         * be possible in our scenario though.
         */
        return false;
    }

    out.addr = ipAddrStr;
    out.port = in.port;

    return true;
} /* translate */

bool CtUpdateAmbassador::L4ToNetwork(IpaL4Protocol in, NetworkProtocol &out) {
    bool ret = false;
    switch(in) {
        case IpaL4Protocol::TCP:
            ret = true;
            out = NetworkProtocol::TCP;
            break;
        case IpaL4Protocol::UDP:
            ret = true;
            out = NetworkProtocol::UDP;
            break;
        default:
            ret = false;
            break;
    }
    return ret;
} /* L4ToNetwork */
