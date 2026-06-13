/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef _WLAN_MGMT_RX_SRNG_PUBLIC_STRUCTS_H_
#define _WLAN_MGMT_RX_SRNG_PUBLIC_STRUCTS_H_

struct mgmt_srng_reap_event_params {
	uint32_t tail_ptr;
	uint32_t timestamp;
};

struct wlan_mgmt_rx_srng_tx_ops {
	QDF_STATUS (*mgmt_rx_srng_send_threshold)(struct wlan_objmgr_psoc *psoc,
						  uint32_t thres);
};

struct wlan_mgmt_rx_srng_rx_ops {
	QDF_STATUS (*mgmt_rx_srng_register_reap_ev_handler)
						(struct wlan_objmgr_psoc *psoc);
	QDF_STATUS (*mgmt_rx_srng_unregister_reap_ev_handler)
						(struct wlan_objmgr_psoc *psoc);
};

#endif
