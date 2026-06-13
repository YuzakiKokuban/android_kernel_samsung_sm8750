/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: contains ll_lt_sap_declarations specific to the ll_lt_sap module
 */

#ifndef __WLAN_HDD_LL_LT_SAP_H
#define __WLAN_HDD_LL_LT_SAP_H

#include "wlan_hdd_main.h"
#include "qca_vendor.h"

extern const struct nla_policy
	wlan_hdd_ll_lt_sap_transport_switch_policy
	[QCA_WLAN_VENDOR_ATTR_AUDIO_TRANSPORT_SWITCH_MAX + 1];

extern const struct nla_policy
	wlan_hdd_ll_lt_sap_high_ap_availability_policy
	[QCA_WLAN_VENDOR_ATTR_HIGH_AP_AVAILABILITY_MAX + 1];

#define FEATURE_LL_LT_SAP_VENDOR_COMMANDS				      \
{									      \
	.info.vendor_id = QCA_NL80211_VENDOR_ID,			      \
	.info.subcmd = QCA_NL80211_VENDOR_SUBCMD_AUDIO_TRANSPORT_SWITCH,      \
	.flags = WIPHY_VENDOR_CMD_NEED_WDEV |				      \
			WIPHY_VENDOR_CMD_NEED_NETDEV |			      \
			WIPHY_VENDOR_CMD_NEED_RUNNING,			      \
	.doit = wlan_hdd_cfg80211_ll_lt_sap_transport_switch,		      \
	vendor_command_policy(wlan_hdd_ll_lt_sap_transport_switch_policy,     \
			      QCA_WLAN_VENDOR_ATTR_AUDIO_TRANSPORT_SWITCH_MAX)\
},									      \
{									      \
	.info.vendor_id = QCA_NL80211_VENDOR_ID,			      \
	.info.subcmd = QCA_NL80211_VENDOR_SUBCMD_HIGH_AP_AVAILABILITY,        \
	.flags = WIPHY_VENDOR_CMD_NEED_WDEV |				      \
			WIPHY_VENDOR_CMD_NEED_NETDEV |			      \
			WIPHY_VENDOR_CMD_NEED_RUNNING,			      \
	.doit = wlan_hdd_cfg80211_ll_lt_sap_high_ap_availability,	      \
	vendor_command_policy(wlan_hdd_ll_lt_sap_high_ap_availability_policy, \
			      QCA_WLAN_VENDOR_ATTR_HIGH_AP_AVAILABILITY_MAX)  \
},

/**
 * wlan_hdd_cfg80211_ll_lt_sap_transport_switch() - Request to switch the
 * transport
 * @wiphy:   pointer to wireless wiphy structure.
 * @wdev:    pointer to wireless_dev structure.
 * @data:    Pointer to the data to be passed via vendor interface
 * @data_len:Length of the data to be passed
 *
 * Return:   Return the Success or Failure code.
 */
int wlan_hdd_cfg80211_ll_lt_sap_transport_switch(struct wiphy *wiphy,
						 struct wireless_dev *wdev,
						 const void *data,
						 int data_len);

/**
 * wlan_hdd_cfg80211_ll_lt_sap_high_ap_availability() - Request for high AP
 * availability
 * @wiphy:   pointer to wireless wiphy structure.
 * @wdev:    pointer to wireless_dev structure.
 * @data:    Pointer to the data to be passed via vendor interface
 * @data_len:Length of the data to be passed
 *
 * Return:   Return the Success or Failure code.
 */
int wlan_hdd_cfg80211_ll_lt_sap_high_ap_availability(struct wiphy *wiphy,
						     struct wireless_dev *wdev,
						     const void *data,
						     int data_len);

#ifdef WLAN_FEATURE_LL_LT_SAP
/**
 * wlan_hdd_ll_lt_sap_get_csa_timestamp() - GET CSA timestamp for LL_LT_SAP
 * @psoc: psoc object
 * @vdev: vdev object
 * @target_tsf: target_tsf parameter
 *
 * Return: int
 */
int wlan_hdd_ll_lt_sap_get_csa_timestamp(struct wlan_objmgr_psoc *psoc,
					 struct wlan_objmgr_vdev *vdev,
					 uint64_t *target_tsf);
#else
static inline
int wlan_hdd_ll_lt_sap_get_csa_timestamp(struct wlan_objmgr_psoc *psoc,
					 struct wlan_objmgr_vdev *vdev,
					 uint64_t *target_tsf)
{
	return -EINVAL;
}
#endif
#endif /* __WLAN_HDD_LL_LT_SAP_H */
