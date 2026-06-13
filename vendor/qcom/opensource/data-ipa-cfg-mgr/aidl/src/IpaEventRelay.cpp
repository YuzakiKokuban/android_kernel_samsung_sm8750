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
#define LOG_TAG "IPAHALService/IpaEventRelay"
/* External Includes */
#include <cutils/log.h>

/* Internal Includes */
#include "IpaEventRelay.h"

/* Namespace pollution avoidance */
using aidl::android::hardware::tetheroffload::ITetheringOffloadCallback;


IpaEventRelay::IpaEventRelay(
        const shared_ptr<ITetheringOffloadCallback>& cb) : mFramework(cb) {
} /* IpaEventRelay */

void IpaEventRelay::sendEvent(OffloadCallbackEvent event) {
    // Events need to be sent for the version passed in and all versions defined after that.
    // This ensures all new versions get the correct events, but vrsion where events where not
    // defined do not.
    ndk::ScopedAStatus ret;

    ALOGI("Triggering onEvent");
    ret = mFramework->onEvent(event);

    if (!ret.isOk()) {
        ALOGE("Triggering onEvent Callback failed.");
    }
}

void IpaEventRelay::onOffloadStarted() {
    ALOGI("onOffloadStarted()");
    sendEvent(OffloadCallbackEvent::OFFLOAD_STARTED);
} /* onOffloadStarted */

void IpaEventRelay::onOffloadStopped(StoppedReason reason) {
    ALOGI("onOffloadStopped(%d)", reason);
    if( reason == StoppedReason::REQUESTED ) {
        /*
         * No way to communicate this to Framework right now, they make an
         * assumption that offload is stopped when they remove the
         * configuration.
         */
    }
    else if ( reason == StoppedReason::ERROR ) {
        sendEvent(OffloadCallbackEvent::OFFLOAD_STOPPED_ERROR);
    }
    else if ( reason == StoppedReason::UNSUPPORTED ) {
        sendEvent(OffloadCallbackEvent::OFFLOAD_STOPPED_UNSUPPORTED);
    }
    else {
        ALOGE("Unknown stopped reason(%d)", reason);
    }
} /* onOffloadStopped */

void IpaEventRelay::onOffloadSupportAvailable() {
    ALOGI("onOffloadSupportAvailable()");
    sendEvent(OffloadCallbackEvent::OFFLOAD_SUPPORT_AVAILABLE);
} /* onOffloadSupportAvailable */

void IpaEventRelay::onLimitReached() {
    ALOGI("onLimitReached()");
    sendEvent(OffloadCallbackEvent::OFFLOAD_STOPPED_LIMIT_REACHED);
} /* onLimitReached */

/** V1_1 API's **/
void IpaEventRelay::onWarningReached() {
    ALOGI("onWarningReached()");
    sendEvent(OffloadCallbackEvent::OFFLOAD_WARNING_REACHED);
} /* onWarningReached */
