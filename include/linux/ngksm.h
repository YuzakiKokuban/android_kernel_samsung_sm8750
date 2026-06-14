/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef __LINUX_NGKSM_H
#define __LINUX_NGKSM_H

#include <linux/types.h>

// NGKSM Kernel Interface
extern int noinline ngksm_send_message(const char *feature_code,
                      const char *detail, int64_t value);

#endif /* __LINUX_NGKSM_H */
