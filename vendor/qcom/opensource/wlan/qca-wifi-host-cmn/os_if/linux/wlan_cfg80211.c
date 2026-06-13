/*
 * Copyright (c) 2016-2017 The Linux Foundation. All rights reserved.
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
 * DOC: defines driver functions interfacing with linux kernel
 */
#include "wlan_cfg80211.h"

#if defined(WLAN_FEATURE_11BE) && defined(CFG80211_11BE_BASIC)
enum nl80211_chan_width
wlan_cfg80211_get_nl80211_chwidth(enum phy_ch_width phy_chwidth)
{
	switch (phy_chwidth) {
	case CH_WIDTH_5MHZ:
		return NL80211_CHAN_WIDTH_5;
	case CH_WIDTH_10MHZ:
		return NL80211_CHAN_WIDTH_10;
	case CH_WIDTH_20MHZ:
		return NL80211_CHAN_WIDTH_20;
	case CH_WIDTH_40MHZ:
		return NL80211_CHAN_WIDTH_40;
	case CH_WIDTH_80MHZ:
		return NL80211_CHAN_WIDTH_80;
	case CH_WIDTH_160MHZ:
		return NL80211_CHAN_WIDTH_160;
	case CH_WIDTH_80P80MHZ:
		return NL80211_CHAN_WIDTH_80P80;
	case CH_WIDTH_320MHZ:
		return NL80211_CHAN_WIDTH_320;
	case CH_WIDTH_MAX:
	case CH_WIDTH_INVALID:
	default:
		osif_debug("Invalid channel width %u", phy_chwidth);
		return NL80211_CHAN_WIDTH_20;
	}
}

enum phy_ch_width
wlan_cfg80211_get_phy_ch_width(enum nl80211_chan_width nl_chwidth)
{
	switch (nl_chwidth) {
	case NL80211_CHAN_WIDTH_5:
		return CH_WIDTH_5MHZ;
	case NL80211_CHAN_WIDTH_10:
		return CH_WIDTH_10MHZ;
	case NL80211_CHAN_WIDTH_20:
		return CH_WIDTH_20MHZ;
	case NL80211_CHAN_WIDTH_40:
		return CH_WIDTH_40MHZ;
	case NL80211_CHAN_WIDTH_80:
		return CH_WIDTH_80MHZ;
	case NL80211_CHAN_WIDTH_160:
		return CH_WIDTH_160MHZ;
	case NL80211_CHAN_WIDTH_80P80:
		return CH_WIDTH_80P80MHZ;
	case NL80211_CHAN_WIDTH_320:
		return CH_WIDTH_320MHZ;
	default:
		osif_debug("Invalid channel width %u", nl_chwidth);
		return CH_WIDTH_INVALID;
	}
}
#else
enum nl80211_chan_width
wlan_cfg80211_get_nl80211_chwidth(enum phy_ch_width phy_chwidth)
{
	switch (phy_chwidth) {
	case CH_WIDTH_5MHZ:
		return NL80211_CHAN_WIDTH_5;
	case CH_WIDTH_10MHZ:
		return NL80211_CHAN_WIDTH_10;
	case CH_WIDTH_20MHZ:
		return NL80211_CHAN_WIDTH_20;
	case CH_WIDTH_40MHZ:
		return NL80211_CHAN_WIDTH_40;
	case CH_WIDTH_80MHZ:
		return NL80211_CHAN_WIDTH_80;
	case CH_WIDTH_160MHZ:
	case CH_WIDTH_MAX:
		return NL80211_CHAN_WIDTH_160;
	case CH_WIDTH_80P80MHZ:
		return NL80211_CHAN_WIDTH_80P80;
	case CH_WIDTH_INVALID:
	default:
		osif_debug("Invalid channel width %u", phy_chwidth);
		return NL80211_CHAN_WIDTH_20;
	}
}

enum phy_ch_width
wlan_cfg80211_get_phy_ch_width(enum nl80211_chan_width nl_chwidth)
{
	switch (nl_chwidth) {
	case NL80211_CHAN_WIDTH_5:
		return CH_WIDTH_5MHZ;
	case NL80211_CHAN_WIDTH_10:
		return CH_WIDTH_10MHZ;
	case NL80211_CHAN_WIDTH_20:
		return CH_WIDTH_20MHZ;
	case NL80211_CHAN_WIDTH_40:
		return CH_WIDTH_40MHZ;
	case NL80211_CHAN_WIDTH_80:
		return CH_WIDTH_80MHZ;
	case NL80211_CHAN_WIDTH_160:
		return CH_WIDTH_160MHZ;
	case NL80211_CHAN_WIDTH_80P80:
		return CH_WIDTH_80P80MHZ;
	default:
		osif_debug("Invalid nl80211 channel width %u", nl_chwidth);
		return CH_WIDTH_INVALID;
	}
}
#endif /* WLAN_FEATURE_11BE */

#define NUM_BITS_IN_BYTE       8

void wlan_cfg80211_set_feature(uint8_t *feature_flags, uint8_t feature)
{
	uint32_t index;
	uint8_t bit_mask;

	index = feature / NUM_BITS_IN_BYTE;
	bit_mask = 1 << (feature % NUM_BITS_IN_BYTE);
	feature_flags[index] |= bit_mask;
}
