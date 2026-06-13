#ifndef AGM_MEMLOGGER_H
#define AGM_MEMLOGGER_H
/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "mem_logger.h"
#include "graph_queue.h"
#include "spf_reset_queue.h"
#include "gsl_intf.h"

/// @brief Initializes all memlog queues tracking AGM memory states
void agm_memlog_init();

/// @brief Deinitializes all memlog queues tracking AGM memory states
void agm_memlog_deinit();

/// @brief Enqueues data to graph memory queue & static buffer
/// @param qState the current graph state (open, close, start, etc)
/// @param qResult the success or failure of the call
/// @param qHandle the graph handle of the graph object
void agm_memlog_graph_enqueue(graph_queue_state qState, int qResult, void *qHandle);
/**/

/// @brief Enqueues data to SPF reset memory queue & static buffer
/// @param qState the state of the SPF reset, down or up
void agm_memlog_spf_reset_enqueue(spf_reset_state qState);

/// @brief Callback function that registers new instances of SPF notifications going down or back up
/// @param events_id the ID of the event registered
/// @param event_payload the payload of the event registered
/// @param event_payload_sz the size of the event payload
/// @param client_data the data the client passes
/// @return an error code representing status
uint32_t agm_memlog_spf_reset_cb(enum gsl_global_event_ids event_id, void *event_payload, size_t event_payload_sz, void *client_data);

#endif