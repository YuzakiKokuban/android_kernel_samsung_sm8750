/*
 * Copyright (c) 2022, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "StreamCommonProxy"

#include "StreamCommonProxy.h"
#include "Session.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include <unistd.h>

StreamCommonProxy::StreamCommonProxy(const struct pal_stream_attributes *sattr __unused,
                    struct pal_device *dattr __unused, const uint32_t no_of_devices __unused,
                    const struct modifier_kv *modifiers __unused, const uint32_t no_of_modifiers __unused,
                    const std::shared_ptr<ResourceManager> rm) :
               StreamCommon(sattr,dattr,no_of_devices,modifiers,no_of_modifiers,rm)
{
    //registering for a callback
    rm->registerStream(this);
}

StreamCommonProxy::~StreamCommonProxy() {
    rm->deregisterStream(this);
}

int32_t StreamCommonProxy::getParameters(uint32_t param_id, void **payload)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "get parameter %u", param_id);
    switch (param_id) {
        case PAL_PARAM_ID_SVA_WAKEUP_MODULE_VERSION:
        {
            status = session->getParameters(this, 0, param_id, payload);
            if (status)
                PAL_ERR(LOG_TAG, "Error:getParam failed with %d",
                    status);
            break;
        }
        default:
            PAL_ERR(LOG_TAG, "Error:Unsupported param id %u", param_id);
            status = -EINVAL;
            break;
    }

    PAL_DBG(LOG_TAG, "get parameter status %d", status);
    return status;
}