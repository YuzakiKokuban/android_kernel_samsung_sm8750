/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 */

#define LOG_TAG "StreamContextProxy"

#include "StreamContextProxy.h"
#include "Session.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include <unistd.h>

StreamContextProxy::StreamContextProxy(const struct pal_stream_attributes *sattr __unused,
                    struct pal_device *dattr __unused, const uint32_t no_of_devices __unused,
                    const struct modifier_kv *modifiers __unused, const uint32_t no_of_modifiers __unused,
                    const std::shared_ptr<ResourceManager> rm) :
               StreamCommon(sattr,dattr,no_of_devices,modifiers,no_of_modifiers,rm)
{
    //registering for a callback
    session->registerCallBack((session_callback)HandleCallBack, (uint64_t) this);
    rm->registerStream(this);
}

StreamContextProxy::~StreamContextProxy() {
    rm->deregisterStream(this);
}

int32_t  StreamContextProxy::setParameters(uint32_t param_id, void *payload)
{
    int32_t status = 0;

    if (!payload)
    {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "wrong params");
        goto error;
    }

    PAL_DBG(LOG_TAG, "start, set parameter %u, session handle - %p", param_id, session);

    mStreamMutex.lock();
    // Stream may not know about tags, so use setParameters instead of setConfig
    if (currentState == STREAM_IDLE) {
        PAL_ERR(LOG_TAG, "Invalid stream state: IDLE for param ID: %d", param_id);
        mStreamMutex.unlock();
        return -EINVAL;
    }
    switch (param_id) {
        case PAL_PARAM_ID_MODULE_CONFIG:
        {
            status = session->setParameters(this, 0, param_id, payload);
            if (status)
                PAL_ERR(LOG_TAG, "Error:setParam for volume boost failed with %d",
                    status);
            break;
        }
        default:
            PAL_ERR(LOG_TAG, "Error:Unsupported param id %u", param_id);
            status = -EINVAL;
            break;
    }

    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "exit, session parameter %u set with status %d", param_id, status);
error:
    return status;
}

void StreamContextProxy::ParseASPSEventPayload(uint32_t event_id,
                                   uint32_t event_size, void *data) {
    if (callback_) {
        PAL_INFO(LOG_TAG, "Notify detection event to client");
        callback_((pal_stream_handle_t *)this, event_id, (uint32_t *)data,
                   event_size, cookie_);
    }
}

void StreamContextProxy::HandleCallBack(uint64_t hdl, uint32_t event_id,
                                      void *data, uint32_t event_size) {
    StreamContextProxy *ContextProxy = nullptr;
    PAL_DBG(LOG_TAG, "Enter, event detected on SPF, event id = 0x%x", event_id);
    ContextProxy = (StreamContextProxy *)hdl;
    ContextProxy->ParseASPSEventPayload(event_id, event_size, data);
    PAL_DBG(LOG_TAG, "Exit");
}
