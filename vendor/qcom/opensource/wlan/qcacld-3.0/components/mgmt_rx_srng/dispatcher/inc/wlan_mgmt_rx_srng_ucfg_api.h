/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

/**
 * DOC: Declare public API related to the mgmt_rx_srng called by north bound
 * HDD/OSIF/LIM
 */

#ifndef _WLAN_MGMT_RX_SRNG_UCFG_API_H_
#define _WLAN_MGMT_RX_SRNG_UCFG_API_H_

#include <qdf_status.h>
#include "wlan_objmgr_cmn.h"

#ifdef FEATURE_MGMT_RX_OVER_SRNG
QDF_STATUS ucfg_mgmt_rx_srng_init(void);
void ucfg_mgmt_rx_srng_deinit(void);
bool ucfg_wlan_mgmt_rx_srng_enabled(struct wlan_objmgr_psoc *psoc);
#else
static inline QDF_STATUS ucfg_mgmt_rx_srng_init(void)
{
	return QDF_STATUS_SUCCESS;
}

static inline void ucfg_mgmt_rx_srng_deinit(void)
{
}

static inline bool ucfg_wlan_mgmt_rx_srng_enabled(struct wlan_objmgr_psoc *psoc)
{
	return false;
}
#endif

#endif
