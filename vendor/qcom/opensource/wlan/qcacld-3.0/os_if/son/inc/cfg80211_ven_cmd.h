/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _CFG80211_VEN_CMD_
#define _CFG80211_VEN_CMD_

enum {
	/* MAC ACL operation */
	IEEE80211_PARAM_MACCMD        = 17,
	IEEE80211_PARAM_CHEXTOFFSET    = 118,
	IEEE80211_PARAM_CHWIDTH        = 122,
	/* Get Channel Utilization value (scale: 0 - 255) */
	IEEE80211_PARAM_CHAN_UTIL           = 261,
	/* override CAC timeout */
	IEEE80211_PARAM_DFS_CACTIMEOUT      = 298,
	/* to get status of acs */
	IEEE80211_PARAM_GET_ACS             = 309,
	/* to get status of CAC period */
	IEEE80211_PARAM_GET_CAC             = 310,
	/* to start acs scan report */
	IEEE80211_PARAM_START_ACS_REPORT        = 318,
	IEEE80211_PARAM_BANDWIDTH               = 336,
	/* TO get number of station associated*/
	IEEE80211_PARAM_STA_COUNT               = 386,
	/* Get per vap smart monitor stats */
	IEEE80211_PARAM_RX_FILTER_SMART_MONITOR    = 523,
	/* Get Frequency */
	IEEE80211_PARAM_GET_FREQUENCY              = 612,
};

enum _ol_ath_param_t {
	OL_ATH_PARAM_CBS = 380,
	OL_ATH_PARAM_CBS_DWELL_SPLIT_TIME = 382,
	OL_ATH_PARAM_CBS_WAIT_TIME = 384,
	OL_ATH_PARAM_BAND_INFO = 399,
	/* Set/Get next frequency for radar */
	OL_ATH_PARAM_NXT_RDR_FREQ = 444,
};

#endif
