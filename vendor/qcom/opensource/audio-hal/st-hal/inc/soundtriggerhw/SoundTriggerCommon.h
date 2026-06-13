/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <log/log.h>

#define STHAL_LOG_ERR             (0x1) /**< error message, represents code bugs that should be debugged and fixed.*/
#define STHAL_LOG_WARN            (0x2) /**< warning message, represents "may not be the best case scenario". */
#define STHAL_LOG_INFO            (0x4) /**< info message, additional info to support debug */
#define STHAL_LOG_DBG             (0x8) /**< debug message, required at minimum for debug.*/
#define STHAL_LOG_VERBOSE         (0x10)/**< verbose message, useful primarily to help developers debug low-level code */

static uint32_t sthal_log_lvl = STHAL_LOG_ERR|STHAL_LOG_WARN|STHAL_LOG_INFO|STHAL_LOG_DBG;

#define STHAL_ERR(log_tag, arg,...)                                          \
    if (sthal_log_lvl & STHAL_LOG_ERR) {                              \
        ALOGE("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__);\
    }
#define STHAL_WARN(log_tag, arg,...)                                          \
    if (sthal_log_lvl & STHAL_LOG_WARN) {                              \
        ALOGW("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__);\
    }
#define STHAL_DBG(log_tag, arg,...)                                           \
    if (sthal_log_lvl & STHAL_LOG_DBG) {                               \
        ALOGD("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__); \
    }
#define STHAL_INFO(log_tag, arg,...)                                         \
    if (sthal_log_lvl & STHAL_LOG_INFO) {                             \
        ALOGI("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__);\
    }
#define STHAL_VERBOSE(log_tag, arg,...)                                      \
    if (sthal_log_lvl & STHAL_LOG_VERBOSE) {                          \
        ALOGV("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__);\
    }