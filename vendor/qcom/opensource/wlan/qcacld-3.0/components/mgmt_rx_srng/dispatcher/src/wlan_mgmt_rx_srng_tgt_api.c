/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#include "wlan_mgmt_rx_srng_tgt_api.h"

QDF_STATUS
tgt_mgmt_rx_srng_register_ev_handler(struct wlan_objmgr_psoc *psoc)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	struct wlan_mgmt_rx_srng_rx_ops *rx_ops;

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
				psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	rx_ops = &psoc_priv->rx_ops;
	if (!rx_ops->mgmt_rx_srng_register_reap_ev_handler)
		return QDF_STATUS_E_FAILURE;

	return rx_ops->mgmt_rx_srng_register_reap_ev_handler(psoc);
}

QDF_STATUS
tgt_mgmt_rx_srng_unregister_ev_handler(struct wlan_objmgr_psoc *psoc)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	struct wlan_mgmt_rx_srng_rx_ops *rx_ops;

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	rx_ops = &psoc_priv->rx_ops;
	if (!rx_ops->mgmt_rx_srng_unregister_reap_ev_handler)
		return QDF_STATUS_E_FAILURE;

	return rx_ops->mgmt_rx_srng_unregister_reap_ev_handler(psoc);
}

QDF_STATUS
tgt_mgmt_rx_srng_send_reap_threshold(struct wlan_objmgr_psoc *psoc,
				     uint32_t val)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	struct wlan_mgmt_rx_srng_tx_ops *tx_ops;

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		mgmt_rx_srng_err("psoc priv is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	tx_ops = &psoc_priv->tx_ops;
	if (!tx_ops->mgmt_rx_srng_send_threshold)
		return QDF_STATUS_E_FAILURE;

	return tx_ops->mgmt_rx_srng_send_threshold(psoc, val);
}

void
tgt_wlan_mgmt_rx_srng_reap_event(struct wlan_objmgr_pdev *pdev,
				 struct mgmt_srng_reap_event_params *params)
{
	wlan_mgmt_rx_srng_reap_buf(pdev, params);
}
