/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef _WLAN_MGMT_RX_SRNG_TGT_API_H_
#define _WLAN_MGMT_RX_SRNG_TGT_API_H_

#include "wlan_mgmt_rx_srng_main.h"

QDF_STATUS
tgt_mgmt_rx_srng_register_ev_handler(struct wlan_objmgr_psoc *psoc);

QDF_STATUS
tgt_mgmt_rx_srng_unregister_ev_handler(struct wlan_objmgr_psoc *psoc);

QDF_STATUS
tgt_mgmt_rx_srng_send_reap_threshold(struct wlan_objmgr_psoc *psoc,
				     uint32_t val);

void
tgt_wlan_mgmt_rx_srng_reap_event(struct wlan_objmgr_pdev *pdev,
				 struct mgmt_srng_reap_event_params *params);
#endif
