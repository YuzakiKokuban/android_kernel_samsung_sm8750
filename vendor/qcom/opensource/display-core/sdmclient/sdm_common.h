/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SDM_COMMON_H__
#define __SDM_COMMON_H__

#include <core/sdm_types.h>

namespace sdm {

#define PAGE_SIZE 4096
#define PROPERTY_VALUE_MAX 255

nsecs_t nanoseconds_to_seconds(nsecs_t secs);
nsecs_t nanoseconds_to_milliseconds(nsecs_t secs);
size_t strlcpy(char *dst, const char *src, size_t size);

int uevent_init();
int uevent_next_event(char *buffer, int buffer_length);

}  // namespace sdm

#endif  // __SDM_COMMON_H__