/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2016-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_REQ_MGR_DEBUG_H_
#define _CAM_REQ_MGR_DEBUG_H_

#include <linux/debugfs.h>
#include "cam_req_mgr_core.h"
#include "cam_debug_util.h"

struct camera_submodule_bind_time_node {
	char *name;
	long bind_time_usec;
	struct list_head list;
};

int cam_req_mgr_debug_register(struct cam_req_mgr_core_device *core_dev);
int cam_req_mgr_debug_unregister(void);

/* cam_req_mgr_debug_delay_detect()
 * @brief    : increment debug_fs varaible by 1 whenever delay occurred.
 */
void cam_req_mgr_debug_delay_detect(void);
void cam_req_mgr_debug_record_bind_time(unsigned long time_in_usec);
void cam_req_mgr_debug_record_bind_latency(const char *driver_name, unsigned long time_in_usec);
void cam_req_mgr_debug_bind_latency_cleanup(void);
#endif
