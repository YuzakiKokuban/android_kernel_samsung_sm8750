/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __CAM_QRTR_COMMS_H__
#define __CAM_QRTR_COMMS_H__

#include <linux/qrtr.h>
#include <linux/net.h>
#include <net/sock.h>
#include <linux/spinlock_types.h>

#include "cam_trace.h"
#include "cam_debug_util.h"
#include "cam_common_util.h"
#include "cam_inter_vm_comms.h"
#include "cam_inter_vm_comms_data.h"
#include "cam_common_util.h"


#define CAM_HOST_SERVICE_SVC_ID 5022
#define CAM_HOST_SVC_VERS       1
#define CAM_HOST_SERVICE_INS_ID 1

struct cam_qrtr_comms_data {
	struct socket *sock;
	struct sockaddr_qrtr local_sock_addr;
	struct sockaddr_qrtr remote_sock_addr;
};

/**
 * @brief Returns the ops table for QRTR socket protocol.
 *
 * @param ops_table   : The inter VM comms function table pointer.
 *
 * @return Status of operation. Negative in case of error. Zero otherwise.
 */
int cam_get_inter_vm_comms_function_table(struct cam_inter_vm_comms_ops *ops_table);

#endif /*__CAM_QRTR_COMMS_H__*/
