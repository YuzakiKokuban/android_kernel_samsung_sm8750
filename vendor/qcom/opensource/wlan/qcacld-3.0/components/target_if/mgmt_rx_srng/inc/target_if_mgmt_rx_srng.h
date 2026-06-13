/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef _TARGET_IF_MGMT_RX_SRNG_H_
#define _TARGET_IF_MGMT_RX_SRNG_H_

#include <wlan_mgmt_rx_srng_main.h>
#include <wlan_mgmt_rx_srng_ucfg_api.h>
#include <wlan_mgmt_rx_srng_public_structs.h>
#include <target_if.h>

/**
 * target_if_mgmt_rx_srng_register_rx_ops() - Register mgmt_rx_srng  RX ops
 * @rx_ops: mgmt_rx_srng component reception ops
 * Return: None
 */
void
target_if_mgmt_rx_srng_register_rx_ops(struct wlan_mgmt_rx_srng_rx_ops *rx_ops);

/**
 * target_if_mgmt_rx_srng_register_tx_ops() - Register mgmt_rx_srng TX ops
 * @tx_ops: mgmt_rx_srng component transmit ops
 *
 * Return: None
 */
void target_if_mgmt_rx_srng_register_tx_ops(struct wlan_mgmt_rx_srng_tx_ops
					   *tx_ops);

#ifdef FEATURE_MGMT_RX_OVER_SRNG
/**
 * target_if_mgmt_rx_srng_update_support() - update the feature support based
 *					on host and FW capability
 * @psoc: psoc object
 * @wmi_handle: wmi handle
 *
 * Return: None
 */
void target_if_mgmt_rx_srng_update_support(struct wlan_objmgr_psoc *psoc,
					   wmi_unified_t wmi_handle);
#else
static inline
void target_if_mgmt_rx_srng_update_support(struct wlan_objmgr_psoc *psoc,
					   wmi_unified_t wmi_handle)
{
}
#endif

#endif /* _TARGET_IF_MGMT_RX_SRNG_H_ */
