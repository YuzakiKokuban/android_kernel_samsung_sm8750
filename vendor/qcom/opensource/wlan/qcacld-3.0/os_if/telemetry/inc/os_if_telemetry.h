/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
 /**
  * DOC: os_if_telemetry.h
  *
  *
  */
#ifndef __OSIF_TELEMETRY_H__
#define __OSIF_TELEMETRY_H__

#include "qdf_types.h"
#include "qca_vendor.h"
#include <wlan_objmgr_psoc_obj.h>

#ifdef WLAN_FEATURE_TELEMETRY

#define STATS_POLICY_MAX \
	QCA_WLAN_VENDOR_ATTR_ASYNC_STATS_POLICY_MAX
#define STATS_POLICY_INVALID \
	QCA_WLAN_VENDOR_ATTR_ASYNC_STATS_POLICY_INVALID
#define STATS_POLICY_TYPE \
	QCA_WLAN_VENDOR_ATTR_ASYNC_STATS_POLICY_STATS_TYPE
#define STATS_POLICY_ACTION \
	QCA_WLAN_VENDOR_ATTR_ASYNC_STATS_POLICY_ACTION
#define STATS_POLICY_PERIODICITY \
	QCA_WLAN_VENDOR_ATTR_ASYNC_STATS_POLICY_STATS_PERIODICITY

/**
 * os_if_telemetry_init() - Telemetry initialization
 * @psoc: Pointer to psoc context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS os_if_telemetry_init(struct wlan_objmgr_psoc *psoc);

#ifdef WLAN_DP_FEATURE_STC
#define FEATURE_ASYNC_STATS_VENDOR_COMMANDS			           \
	{								   \
		.info.vendor_id = QCA_NL80211_VENDOR_ID,		   \
		.info.subcmd = QCA_NL80211_VENDOR_SUBCMD_ASYNC_STATS_POLICY,    \
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |			   \
			WIPHY_VENDOR_CMD_NEED_NETDEV |			   \
			WIPHY_VENDOR_CMD_NEED_RUNNING,			   \
		.doit = wlan_hdd_cfg80211_telemetry_stats_service,	   \
		vendor_command_policy(stats_service_policy,	           \
					STATS_POLICY_MAX)                  \
	},								   \

extern const struct nla_policy
stats_service_policy[STATS_POLICY_MAX + 1];
/**
 * os_if_telemetry_stats_service() - Telemetry start service
 * @vdev: Pointer to psoc context
 * @data: NL vendor data
 * @data_len: Length of @data
 *
 * Return: QDF_STATUS
 */
QDF_STATUS os_if_telemetry_stats_service(struct wlan_objmgr_vdev *vdev,
					 const void *data, int data_len);
#else
static inline
QDF_STATUS os_if_telemetry_stats_service(struct wlan_objmgr_vdev *vdev,
					 const void *data, int data_len)
{
	return QDF_STATUS_SUCCESS;
}

#endif /* WLAN_DP_FEATURE_STC */
#else
static inline
QDF_STATUS os_if_telemetry_init(struct wlan_objmgr_psoc *psoc)
{
	return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_FEATURE_TELEMETRY */
#endif /* __OSIF_TELEMETRY_H__ */
