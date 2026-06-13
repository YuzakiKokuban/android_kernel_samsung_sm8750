/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef WLAN_MGMT_RX_SRNG_CFG_H__
#define WLAN_MGMT_RX_SRNG_CFG_H__

#ifdef FEATURE_MGMT_RX_OVER_SRNG
/*
 * <ini>
 * mgmt_rx_srng_enable- Control management packets over SRNG path
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini is used to enable reception of mgmt packets over SRNG instead of CE
 *
 * Supported Feature: Management RX over SRNG
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_FEATURE_MGMT_RX_SRNG_ENABLE \
	CFG_INI_BOOL("mgmt_rx_srng_enable", false, \
		     "Enable/Disable mgmt packets reception over SRING path")

#define WLAN_CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD_MIN 1
#define WLAN_CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD_MAX 20
#define WLAN_CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD 5
/*
 * <ini>
 * mgmt_rx_srng_reap_threshold - Threshold at which FW will send the reap event
 *				to host
 * @Min: 1
 * @Max: 20
 * @Default: 5
 *
 * This ini is used to set the count interval at which FW will sent reap event
 * to host. This reap event will indicate the host to fetch the management
 * packets from new SRNG.
 *
 * Supported Feature: Management RX over SRNG
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD \
	CFG_INI_INT("mgmt_rx_srng_reap_threshold", \
		    WLAN_CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD_MIN, \
		    WLAN_CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD_MAX, \
		    WLAN_CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD, \
		    CFG_VALUE_OR_DEFAULT, \
		    "Count interval at which FW will send reap event to host")

#define CFG_MGMT_RX_SRNG \
	CFG(CFG_FEATURE_MGMT_RX_SRNG_ENABLE) \
	CFG(CFG_MGMT_RX_SRNG_REAP_EVENT_THRESHOLD)
#else
#define CFG_MGMT_RX_SRNG
#endif
#endif /* WLAN_MGMT_RX_SRNG_CFG_H__*/
