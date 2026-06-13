/*
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

/*
 * DOC: contains interface manager public api
 */
#include "wlan_if_mgr_nan.h"
#include "wlan_mlo_link_force.h"
#include "wlan_if_mgr_public_struct.h"

#if defined(WLAN_FEATURE_NAN)
QDF_STATUS if_mgr_nan_post_enable(struct wlan_objmgr_vdev *vdev,
				  struct if_mgr_event_data *event_data)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_pdev *pdev;
	QDF_STATUS status;

	pdev = wlan_vdev_get_pdev(vdev);
	if (!pdev)
		return QDF_STATUS_E_FAILURE;
	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc)
		return QDF_STATUS_E_FAILURE;
	status = ml_nlink_conn_change_notify(
			psoc, wlan_vdev_get_id(vdev),
			ml_nlink_nan_post_enable_evt, NULL);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS if_mgr_nan_pre_enable(struct wlan_objmgr_vdev *vdev,
				 struct if_mgr_event_data *event_data)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_pdev *pdev;
	QDF_STATUS status;

	pdev = wlan_vdev_get_pdev(vdev);
	if (!pdev)
		return QDF_STATUS_E_FAILURE;
	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc)
		return QDF_STATUS_E_FAILURE;
	status = ml_nlink_conn_change_notify(
			psoc, wlan_vdev_get_id(vdev),
			ml_nlink_nan_pre_enable_evt, NULL);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS if_mgr_nan_post_disable(struct wlan_objmgr_vdev *vdev,
				   struct if_mgr_event_data *event_data)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_pdev *pdev;
	QDF_STATUS status;

	pdev = wlan_vdev_get_pdev(vdev);
	if (!pdev)
		return QDF_STATUS_E_FAILURE;
	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc)
		return QDF_STATUS_E_FAILURE;
	status = ml_nlink_conn_change_notify(
			psoc, wlan_vdev_get_id(vdev),
			ml_nlink_nan_post_disable_evt, NULL);
	return QDF_STATUS_SUCCESS;
}
#endif
