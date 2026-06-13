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
#ifndef _CT_UPDATE_AMBASSADOR_H_
#define _CT_UPDATE_AMBASSADOR_H_

/* AIDL Includes */
#include <aidl/android/hardware/tetheroffload/BnTetheringOffloadCallback.h>

/* Internal Includes */
#include "IOffloadManager.h"

using ::std::shared_ptr;

/* Namespace pollution avoidance */
using aidl::android::hardware::tetheroffload::NetworkProtocol;
using AIDLIpAddrPortPair = aidl::android::hardware::tetheroffload::IPv4AddrPortPair;
using AIDLNatTimeoutUpdate = aidl::android::hardware::tetheroffload::NatTimeoutUpdate;
using aidl::android::hardware::tetheroffload::ITetheringOffloadCallback;

using IpaIpAddrPortPair = ::IOffloadManager::ConntrackTimeoutUpdater::IpAddrPortPair;
using IpaNatTimeoutUpdate = ::IOffloadManager::ConntrackTimeoutUpdater::NatTimeoutUpdate;
using IpaL4Protocol = ::IOffloadManager::ConntrackTimeoutUpdater::L4Protocol;


class CtUpdateAmbassador : public IOffloadManager::ConntrackTimeoutUpdater {
public:
    CtUpdateAmbassador(const shared_ptr<ITetheringOffloadCallback>& /* cb */);
    /* ------------------- CONNTRACK TIMEOUT UPDATER ------------------------ */
    void updateTimeout(IpaNatTimeoutUpdate /* update */);
private:
    static bool translate(IpaNatTimeoutUpdate /* in */, AIDLNatTimeoutUpdate& /* out */);
    static bool translate(IpaIpAddrPortPair /* in */, AIDLIpAddrPortPair& /* out */);
    static bool L4ToNetwork(IpaL4Protocol /* in */, NetworkProtocol& /* out */);
    const shared_ptr<ITetheringOffloadCallback>& mFramework;
}; /* CtUpdateAmbassador */
#endif /* _CT_UPDATE_AMBASSADOR_H_ */
