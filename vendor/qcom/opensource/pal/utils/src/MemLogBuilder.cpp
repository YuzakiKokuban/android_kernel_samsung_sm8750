/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "MemLogBuilder.h"
#include "Device.h"
#include <hwbinder/IPCThreadState.h>

#define LOG_TAG "PAL: memLoggerBuilder"

int palStateQueueBuilder(pal_state_queue &que, Stream *s, pal_state_queue_state state, int32_t error)
{
    PAL_DBG(LOG_TAG, "Entered PAL State Queue Builder");
    int ret = 0;
    std::vector <std::shared_ptr<Device>> aDevices;
    struct pal_stream_attributes sAttr;
    struct pal_device strDevAttr;
    pal_stream_type_t type;

    if (!memLoggerIsQueueInitialized())
    {
        PAL_ERR(LOG_TAG, "queues failed to initialize");
        goto exit;
    }

    ret = s->getAssociatedDevices(aDevices);
    if(ret != 0)
    {
        PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", ret);
        goto exit;
    }

    ret = s->getStreamType(&type);
    if(ret != 0)
    {
        PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", ret);
        goto exit;
    }

    memset(&que, 0, sizeof(que));
    s->getStreamAttributes(&sAttr);
    que.stream_handle = (uint64_t) s;     // array to store queue element
    que.stream_type = type;
    que.direction = sAttr.direction;
    que.state = state;
    que.error = error;

    PAL_DBG(LOG_TAG, "Stream handle = " "%" PRId64 "\n", s);

    memset(que.device_attr, 0, sizeof(que.device_attr));

    for(int i=0; i<aDevices.size(); i++)
    {
        if(i < STATE_DEVICE_MAX_SIZE)
        {
            aDevices[i]->getDeviceAttributes(&strDevAttr, s);
            que.device_attr[i].device = strDevAttr.id;
            que.device_attr[i].sample_rate = strDevAttr.config.sample_rate;
            que.device_attr[i].bit_width = strDevAttr.config.bit_width;
            que.device_attr[i].channels = strDevAttr.config.ch_info.channels;
        }
    }

    que.timestamp = memLoggerFetchTimestamp();

    PAL_DBG(LOG_TAG, "TIME IS:" "%" PRId64 "\n", que.timestamp);

exit:
    return ret;
}

int palStateEnqueue(Stream *s, pal_state_queue_state state, int32_t error)
{
    int ret = 0;
    struct pal_state_queue que;
    ret = palStateQueueBuilder(que, s, state, error);
    if (ret != 0)
    {
        PAL_ERR(LOG_TAG, "palStateQueueBuilder failed with status = %d", ret);
        goto exit;
    }

    ret = memLoggerEnqueue(PAL_STATE_Q, (void*) &que);
    if (ret != 0)
    {
        PAL_ERR(LOG_TAG, "memLoggerEnqueue failed with status = %d", ret);
    }

exit:
    return ret;
}


int palStateEnqueue(Stream *s, pal_state_queue_state state, int32_t error, pal_mlog_str_info str_info)
{
    int ret = 0;
    struct pal_state_queue que;
    ret = palStateQueueBuilder(que, s, state, error);
    if (ret != 0)
    {
        PAL_ERR(LOG_TAG, "palStateQueueBuilder failed with status = %d", ret);
        goto exit;
    }
    memcpy(&(que.str_info), &str_info, sizeof(que.str_info));

    ret = memLoggerEnqueue(PAL_STATE_Q, (void*) &que);
    if (ret != 0)
    {
        PAL_ERR(LOG_TAG, "memLoggerEnqueue failed with status = %d", ret);
    }

exit:
    return ret;
}

void kpiEnqueue(const char name[], bool isEnter)
{
    struct kpi_queue que;

    strlcpy(que.func_name, name, sizeof(que.func_name));
    que.pid = ::android::hardware::IPCThreadState::self()->getCallingPid();
    que.timestamp = memLoggerFetchTimestamp();
    que.type = isEnter;

    memLoggerEnqueue(KPI_Q, (void*) &que);
}
