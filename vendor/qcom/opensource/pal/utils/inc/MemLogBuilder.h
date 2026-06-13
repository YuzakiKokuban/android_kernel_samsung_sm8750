/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef MEMLOG_BUILDER_H
#define MEMLOG_BUILDER_H

#include "mem_logger.h"
#include "pal_state_queue.h"
#include "kpi_queue.h"
#include "ResourceManager.h"
#include "Stream.h"
#include <inttypes.h>

int palStateQueueBuilder(pal_state_queue &que, Stream *s, pal_state_queue_state state, int32_t error);
int palStateEnqueue(Stream *s, pal_state_queue_state state, int32_t error);
int palStateEnqueue(Stream *s, pal_state_queue_state state, int32_t error, union pal_mlog_str_info str_info);
pal_mlog_acdstr_info palStateACDStreamBuilder(Stream *s);
void kpiEnqueue(const char name[], bool isEnter);

#endif
