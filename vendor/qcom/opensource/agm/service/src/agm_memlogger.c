/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <agm/agm_memlogger.h>
#include <agm/utils.h>

void agm_memlog_init()
{
    int ret = 0;
    ret = memLoggerInitQ(GRAPH_Q, MEMLOG_CFG_FILE); //initializes the queue for the debug logger
    if (ret) {
        AGM_LOGE("error in initializing memory queue %d", ret);
    }
    ret = memLoggerInitStatbuf(GRAPH_STATBUF, MEMLOG_CFG_FILE);
    if (ret) {
        AGM_LOGE("error in initializing graph static buffer %d", ret);
    }

    ret = memLoggerInitQ(SPF_RESET_Q, MEMLOG_CFG_FILE); //initializes the queue for the debug logger
    if (ret) {
        AGM_LOGE("error in initializing SPF Reset queue %d", ret);
    }
    ret = memLoggerInitStatbuf(SPF_RESET_STATBUF, MEMLOG_CFG_FILE);
    if (ret) {
        AGM_LOGE("error in initializing SPF reset static buffer %d", ret);
    }
    ret = gsl_register_global_event_cb(agm_memlog_spf_reset_cb, NULL);
    if (ret) {
        AGM_LOGE("error in initializing SPF reset callback %d", ret);
    }
}

void agm_memlog_deinit()
{
    memLoggerDeinitQ(GRAPH_Q);
    memLoggerDeinitStatbuf(GRAPH_STATBUF);
}

void agm_memlog_graph_enqueue(graph_queue_state qState, int qResult, void *qHandle)
{
    struct graph_queue que;

    que.timestamp = memLoggerFetchTimestamp();
    que.state = qState;
    que.result = qResult;
    que.stream_handle = qHandle;

    memLoggerEnqueue(GRAPH_Q, (void*) &que);
    if (qResult != 0)
    {
        memLoggerIncrementStatbuf(GRAPH_STATBUF, qState);
    }
}

void agm_memlog_spf_reset_enqueue(spf_reset_state qState)
{
    struct ssr_pdr_queue que;

    que.timestamp = memLoggerFetchTimestamp();
    que.state = qState;

    memLoggerEnqueue(SPF_RESET_Q, (void*) &que);
    memLoggerIncrementStatbuf(SPF_RESET_STATBUF, qState);
}

uint32_t agm_memlog_spf_reset_cb(enum gsl_global_event_ids event_id, void *event_payload, size_t event_payload_sz, void *client_data)
{
    if (event_id == GSL_GLOBAL_EVENT_AUDIO_SVC_DN)
    {
        agm_memlog_spf_reset_enqueue(SPF_RESET_DOWN);
    }
    else if (event_id == GSL_GLOBAL_EVENT_AUDIO_SVC_UP)
    {
        agm_memlog_spf_reset_enqueue(SPF_RESET_UP);
    }
    return 0;
}
