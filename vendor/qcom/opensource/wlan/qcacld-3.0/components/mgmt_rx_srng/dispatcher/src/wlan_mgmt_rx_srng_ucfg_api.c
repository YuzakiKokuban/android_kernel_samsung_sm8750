/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

/**
 * DOC: Public API implementation of mgmt_rx_srng called by north bound iface.
 */

#include "wlan_mgmt_rx_srng_ucfg_api.h"
#include "wlan_mgmt_rx_srng_main.h"
#include <qdf_str.h>
#include <qdf_status.h>

QDF_STATUS ucfg_mgmt_rx_srng_init(void)
{
	QDF_STATUS status;

	status = wlan_objmgr_register_psoc_create_handler(
				WLAN_UMAC_COMP_MGMT_RX_SRNG,
				wlan_mgmt_rx_srng_psoc_create_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to register psoc create handler");
		return status;
	}

	status = wlan_objmgr_register_psoc_destroy_handler(
				WLAN_UMAC_COMP_MGMT_RX_SRNG,
				wlan_mgmt_rx_srng_psoc_destroy_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to register psoc delete handler");
		goto fail_destroy_psoc;
	}

	status = wlan_objmgr_register_pdev_create_handler(
				WLAN_UMAC_COMP_MGMT_RX_SRNG,
				wlan_mgmt_rx_srng_pdev_create_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to register pdev create handler");
		goto fail_create_pdev;
	}

	status = wlan_objmgr_register_pdev_destroy_handler(
				WLAN_UMAC_COMP_MGMT_RX_SRNG,
				wlan_mgmt_rx_srng_pdev_destroy_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		mgmt_rx_srng_err("Failed to register pdev delete handler");
		goto fail_destroy_pdev;
	}

	return status;
fail_destroy_pdev:
	wlan_objmgr_unregister_pdev_create_handler(
			WLAN_UMAC_COMP_MGMT_RX_SRNG,
			wlan_mgmt_rx_srng_pdev_create_notification,
			NULL);

fail_create_pdev:
	wlan_objmgr_unregister_psoc_destroy_handler(
			WLAN_UMAC_COMP_MGMT_RX_SRNG,
			wlan_mgmt_rx_srng_psoc_destroy_notification,
			NULL);

fail_destroy_psoc:
	wlan_objmgr_unregister_psoc_create_handler(
			WLAN_UMAC_COMP_MGMT_RX_SRNG,
			wlan_mgmt_rx_srng_psoc_create_notification,
			NULL);

	return status;
}

void ucfg_mgmt_rx_srng_deinit(void)
{
	QDF_STATUS status;

	status = wlan_objmgr_unregister_pdev_destroy_handler(
				WLAN_UMAC_COMP_MGMT_RX_SRNG,
				wlan_mgmt_rx_srng_pdev_destroy_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status))
		mgmt_rx_srng_err("Failed to unregister pdev destroy handler");

	status = wlan_objmgr_unregister_pdev_create_handler(
				WLAN_UMAC_COMP_MGMT_RX_SRNG,
				wlan_mgmt_rx_srng_pdev_create_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status))
		mgmt_rx_srng_err("Failed to unregister pdev create handler");

	status = wlan_objmgr_unregister_psoc_destroy_handler(
			WLAN_UMAC_COMP_MGMT_RX_SRNG,
			wlan_mgmt_rx_srng_psoc_destroy_notification,
			NULL);
	if (QDF_IS_STATUS_ERROR(status))
		mgmt_rx_srng_err("Failed to unregister psoc destroy handler");

	status = wlan_objmgr_unregister_psoc_create_handler(
			WLAN_UMAC_COMP_MGMT_RX_SRNG,
			wlan_mgmt_rx_srng_psoc_create_notification,
			NULL);
	if (QDF_IS_STATUS_ERROR(status))
		mgmt_rx_srng_err("Failed to unregister psoc create handler");
}

bool ucfg_wlan_mgmt_rx_srng_enabled(struct wlan_objmgr_psoc *psoc)
{
	return wlan_mgmt_rx_srng_cfg_enable(psoc);
}

