/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

/**
 * DOC: target_if_mgmt_rx_srng.c
 */
#include <target_if_mgmt_rx_srng.h>
#include <wlan_mgmt_rx_srng_tgt_api.h>
#include <wmi_unified_api.h>
#include <target_if.h>
#include <init_deinit_lmac.h>
#include <cds_api.h>

static int target_if_mgmt_srng_reap_event_handler(void *handle, uint8_t *buf,
						  uint32_t data_len)
{
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_pdev *pdev;
	struct mgmt_srng_reap_event_params params;
	QDF_STATUS status;

	psoc = target_if_get_psoc_from_scn_hdl(handle);
	if (!psoc) {
		mgmt_rx_srng_err("psoc is NULL");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("Invalid WMI handle");
		return -EINVAL;
	}

	pdev = target_if_get_pdev_from_scn_hdl(handle);
	if (!pdev) {
		mgmt_rx_srng_err("pdev is NULL");
		return -EINVAL;
	}

	status = wmi_unified_extract_mgmt_srng_reap_event(wmi_handle,
							  buf, &params);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Extract mgmt rx srng reap event failed");
		return -EINVAL;
	}

	tgt_wlan_mgmt_rx_srng_reap_event(pdev, &params);
	return 0;
}

static QDF_STATUS target_if_mgmt_rx_srng_send_reap_event_threshold(
		struct wlan_objmgr_psoc *psoc, uint32_t val)

{
	wmi_unified_t wmi_handle = lmac_get_wmi_unified_hdl(psoc);
	struct pdev_params param = {};
	int ret;

	if (!wmi_handle) {
		target_if_err("Invalid wmi handle");
		return QDF_STATUS_E_INVAL;
	}

	target_if_debug("psoc:%pK, mgmt_rx_reap_thres:%d", psoc, val);

	param.param_id = wmi_pdev_param_mgmt_srng_reap_event_threshold;
	param.param_value = val;

	ret = wmi_unified_pdev_param_send(wmi_handle, &param,  0);

	if (ret) {
		mgmt_rx_srng_err("Fail to send mgmt srng reap event thres");
		return QDF_STATUS_E_FAILURE;
	}
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
target_if_register_mgmt_rx_srng_reap_event(struct wlan_objmgr_psoc *psoc)
{
	wmi_unified_t wmi_handle;
	QDF_STATUS status;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);

	if (!wmi_handle) {
		mgmt_rx_srng_err("wmi_handle is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	status = wmi_unified_register_event_handler(
				wmi_handle, wmi_mgmt_srng_reap_eventid,
				target_if_mgmt_srng_reap_event_handler,
				WMI_RX_TASKLET_CTX);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Fail to register mgmt rx srng reap handler");
		return status;
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
target_if_unregister_mgmt_rx_srng_reap_event(struct wlan_objmgr_psoc *psoc)
{
	wmi_unified_t wmi_handle;
	QDF_STATUS status;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		mgmt_rx_srng_err("Invalid wmi handle");
		return QDF_STATUS_E_INVAL;
	}

	status = wmi_unified_unregister_event(wmi_handle,
					      wmi_mgmt_srng_reap_eventid);
	if (status)
		mgmt_rx_srng_err("unregister mgmt rx srng reap evt failed");

	return status;
}

void
target_if_mgmt_rx_srng_register_rx_ops(struct wlan_mgmt_rx_srng_rx_ops *rx_ops)
{
	if (!rx_ops) {
		target_if_err("mgmt_rx_srng rx_ops is null");
		return;
	}

	rx_ops->mgmt_rx_srng_register_reap_ev_handler =
				target_if_register_mgmt_rx_srng_reap_event;

	rx_ops->mgmt_rx_srng_unregister_reap_ev_handler =
				target_if_unregister_mgmt_rx_srng_reap_event;
}

void
target_if_mgmt_rx_srng_register_tx_ops(struct wlan_mgmt_rx_srng_tx_ops *tx_ops)
{
	if (!tx_ops) {
		target_if_err("mgmt_rx_srng tx_ops is null");
		return;
	}

	tx_ops->mgmt_rx_srng_send_threshold =
			target_if_mgmt_rx_srng_send_reap_event_threshold;
}

void target_if_mgmt_rx_srng_update_support(struct wlan_objmgr_psoc *psoc,
					   wmi_unified_t wmi_handle)
{
	struct mgmt_rx_srng_psoc_priv *psoc_priv;
	struct target_psoc_info *tgt_hdl;
	bool cfg_enable;

	psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
					psoc, WLAN_UMAC_COMP_MGMT_RX_SRNG);
	if (!psoc_priv) {
		target_if_err("mgmt rx srng psoc priv is NULL");
		return;
	}

	tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
	if (!tgt_hdl) {
		target_if_err("target_psoc_info is null in mgmt_rx_srng");
		return;
	}
	cfg_enable = psoc_priv->mgmt_rx_srng_is_enable;

	if (wmi_service_enabled(wmi_handle, wmi_service_mgmt_rx_srng_support) &&
	    cfg_enable) {
		psoc_priv->mgmt_rx_srng_is_enable = true;
		tgt_hdl->info.wlan_res_cfg.mgmt_rx_srng_support = true;
		target_if_debug("mgmt_rx_srng_support is enabled");
	} else {
		psoc_priv->mgmt_rx_srng_is_enable = false;
		target_if_debug("mgmt_rx_srng_support is not enabled");
	}
}
