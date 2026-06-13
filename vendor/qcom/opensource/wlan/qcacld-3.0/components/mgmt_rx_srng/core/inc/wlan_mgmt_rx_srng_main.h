/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

/**
 * DOC: Declare private API which shall be used internally only
 * in mgmt rx srng component. This file shall include prototypes of
 * various notification handlers and logging functions.
 *
 * Note: This API should be never accessed out of mmt rx srng component.
 */
#ifdef FEATURE_MGMT_RX_OVER_SRNG

#ifndef _WLAN_MGMT_RX_SRNG_MAIN_H_
#define _WLAN_MGMT_RX_SRNG_MAIN_H_

#include <qdf_types.h>
#include <qdf_trace.h>
#include <wlan_cmn.h>
#include "wlan_objmgr_cmn.h"
#include "wlan_objmgr_global_obj.h"
#include "wlan_mgmt_rx_srng_priv.h"
#include <wlan_mgmt_rx_srng_tgt_api.h>

#define mgmt_rx_srng_fatal(params...) \
	QDF_TRACE_FATAL(QDF_MODULE_ID_MGMT_RX_SRNG, params)
#define mgmt_rx_srng_err(params...)\
	QDF_TRACE_ERROR(QDF_MODULE_ID_MGMT_RX_SRNG, params)
#define mgmt_rx_srng_warn(params...)\
	QDF_TRACE_WARN(QDF_MODULE_ID_MGMT_RX_SRNG, params)
#define mgmt_rx_srng_info(params...) \
	QDF_TRACE_INFO(QDF_MODULE_ID_MGMT_RX_SRNG, params)
#define mgmt_rx_srng_debug(params...) \
	QDF_TRACE_DEBUG(QDF_MODULE_ID_MGMT_RX_SRNG, params)

#define MGMT_RX_SRNG_ENTER() mgmt_rx_srng_debug("enter")
#define MGMT_RX_SRNG_EXIT() mgmt_rx_srng_debug("exit")

QDF_STATUS wlan_mgmt_rx_srng_psoc_create_notification(
				struct wlan_objmgr_psoc *psoc, void *arg);

QDF_STATUS wlan_mgmt_rx_srng_psoc_destroy_notification(
				struct wlan_objmgr_psoc *psoc, void *arg);

QDF_STATUS wlan_mgmt_rx_srng_pdev_create_notification(
				struct wlan_objmgr_pdev *pdev, void *arg);

QDF_STATUS wlan_mgmt_rx_srng_pdev_destroy_notification(
				struct wlan_objmgr_pdev *pdev, void *arg);

bool wlan_mgmt_rx_srng_cfg_enable(struct wlan_objmgr_psoc *psoc);
void wlan_mgmt_rx_srng_reap_buf(struct wlan_objmgr_pdev *pdev,
				struct mgmt_srng_reap_event_params *param);
#endif /* end of _WLAN_MGMT_RX_SRNG_MAIN_H_*/

#endif
