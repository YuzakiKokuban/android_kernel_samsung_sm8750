/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _OS_IF_DP_STC_H_
#define _OS_IF_DP_STC_H_

#include "qdf_types.h"
#include "qca_vendor.h"
#include "wlan_cfg80211.h"
#include "wlan_dp_public_struct.h"

#ifdef WLAN_DP_FEATURE_STC
typedef struct wlan_dp_stc_txrx_samples (*txrx_samples_t)[DP_TXRX_SAMPLES_WINDOW_MAX];

extern const struct nla_policy
flow_classify_result_policy[QCA_WLAN_VENDOR_ATTR_FLOW_CLASSIFY_RESULT_MAX  + 1];

#define FEATURE_FLOW_CLASSIFY_COMMANDS							\
	{										\
		.info.vendor_id = QCA_NL80211_VENDOR_ID,				\
		.info.subcmd = QCA_NL80211_VENDOR_SUBCMD_FLOW_CLASSIFY_RESULT,		\
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |					\
			WIPHY_VENDOR_CMD_NEED_NETDEV,					\
		.doit = wlan_hdd_cfg80211_flow_classify_report_cmd,			\
		vendor_command_policy(flow_classify_result_policy,			\
				QCA_WLAN_VENDOR_ATTR_FLOW_CLASSIFY_RESULT_MAX)		\
	},

#define FEATURE_FLOW_STATS_EVENTS					\
	[QCA_NL80211_VENDOR_SUBCMD_FLOW_STATS_INDEX] = {		\
		.vendor_id = QCA_NL80211_VENDOR_ID,			\
		.subcmd = QCA_NL80211_VENDOR_SUBCMD_FLOW_STATS,		\
	},

#define FEATURE_FLOW_REPORT_EVENTS						\
	[QCA_NL80211_VENDOR_SUBCMD_CLASSIFIED_FLOW_REPORT_INDEX] = {		\
		.vendor_id = QCA_NL80211_VENDOR_ID,				\
		.subcmd = QCA_NL80211_VENDOR_SUBCMD_CLASSIFIED_FLOW_REPORT,	\
	},

/**
 * os_if_dp_flow_classify_result() - Handler to process flow classify result
 * @wiphy: wiphy handle
 * @data: user layer input
 * @data_len: length of user layer input
 *
 * Return: QDF_STATUS
 */
QDF_STATUS os_if_dp_flow_classify_result(struct wiphy *wiphy, const void *data,
					 int data_len);

/**
 * osif_dp_register_stc_callbacks() - Set STC callbacks
 * @cb_obj: callback object
 *
 * Return: None
 */
void osif_dp_register_stc_callbacks(struct wlan_dp_psoc_callbacks *cb_obj);
#else

#define FEATURE_FLOW_CLASSIFY_COMMANDS
#define FEATURE_FLOW_STATS_EVENTS
#define FEATURE_FLOW_REPORT_EVENTS

static inline
QDF_STATUS os_if_dp_flow_classify_result(struct wiphy *wiphy, const void *data,
					 int data_len)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
void osif_dp_register_stc_callbacks(struct wlan_dp_psoc_callbacks *cb_obj)
{
}
#endif

#endif /* _OS_IF_DP_STC_H_ */
