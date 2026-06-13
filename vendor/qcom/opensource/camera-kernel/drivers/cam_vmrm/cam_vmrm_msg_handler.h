/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_VMRM_MSG_HANDLER_H_
#define _CAM_VMRM_MSG_HANDLER_H_

/* message destination type */
#define CAM_MSG_DST_TYPE_HW_INSTANCE 0
#define CAM_MSG_DST_TYPE_DRIVER_NODE 1
#define CAM_MSG_DST_TYPE_VMRM        2

/* max length of bus client name */
#define CAM_ICC_CLIENT_NAME_MAX      32

/* message type */
#define CAM_HW_RESOURCE_ACQUIRE     0
#define CAM_HW_RESOURCE_RELEASE     1
#define CAM_HW_RESOURCE_SET_ACQUIRE 2
#define CAM_HW_RESOURCE_SET_RELEASE 3
#define CAM_SOC_ENABLE_RESOURCE     4
#define CAM_SOC_DISABLE_RESOURCE    5
#define CAM_CLK_SET_RATE            6
#define CAM_CLK_SET_RATE_LEVEL      7
#define CAM_ICC_VOTE                8
#define CAM_HW_POWER_UP             9
#define CAM_HW_POWER_DOWN           10
#define CAM_ICP_POWER_COLLAPSE      11
#define CAM_ICP_POWER_RESUME        12
#define CAM_ICP_SHUTDOWN            13
#define CAM_MSG_TYPE_MAX            14

/* vm communication message payload data_size only includes the data size*/
struct cam_vmrm_msg {
	uint32_t msg_dst_type;
	uint32_t msg_dst_id;
	uint32_t msg_type;
	uint32_t source_vmid;
	uint32_t des_vmid;
	uint32_t data_size;
	bool     response_msg;
	bool     need_response;
	int      response_result;
	uint8_t  data[];
};

/**
 * struct cam_msg_set_clk_rate - camera set clock rate message data payload
 *
 * @cesta_client_idx:          Cesta client index
 * @clk_rate_high:             Clock high rate
 * @clk_rate_low:              Clock low rate
 *
 */
struct cam_msg_set_clk_rate {
	int           cesta_client_idx;
	unsigned long clk_rate_high;
	unsigned long clk_rate_low;
};

/**
 * struct cam_msg_set_clk_rate_level - camera set clock rate level message data payload
 *
 * @cesta_client_idx:          Cesta client index
 * @clk_level_high:            Clock high level
 * @clk_level_low:             Clock low level
 * @do_not_set_src_clk:        Set src clk flag
 * @clk_rate:                  Clock rate
 *
 */
struct cam_msg_set_clk_rate_level {
	int           cesta_client_idx;
	int32_t       clk_level_high;
	int32_t       clk_level_low;
	bool          do_not_set_src_clk;
	unsigned long clk_rate;
};

/**
 * struct cam_msg_icc_vote - camera icc vote message data payload
 *
 * @name:                    Bus client name
 * @ab:                      Arbitrated Bandwidth
 * @ib:                      Instantaneous Bandwidth
 *
 */
struct cam_msg_icc_vote {
	char name[CAM_ICC_CLIENT_NAME_MAX];
	uint64_t ab;
	uint64_t ib;
};

#endif /* _CAM_VMRM_MSG_HANDLER_H_ */
