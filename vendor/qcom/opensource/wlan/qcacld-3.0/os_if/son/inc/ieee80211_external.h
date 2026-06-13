/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  Copyright (c) 2009 Atheros Communications Inc.  All rights reserved.
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

/*
 * brief: External Header File References
 * This header file refers to the internal header files that provide the
 * data structure definitions and parameters required by external programs
 * that interface via ioctl or similar mechanisms.  This hides the location
 * of the specific header files, and provides a control to limit what is
 * being exported for external use.
 */

#ifndef _IEEE80211_EXTERNAL_H_
#define _IEEE80211_EXTERNAL_H_

#include "ieee80211.h"
#include "ieee80211_defines.h"
#include "ieee80211_ioctl.h"

#define ATH_PARAM_SHIFT     0x1000

/*
 * this should be  defined the same qca_nl80211_vendor_subcmds_internal in
 * son component
 * TD: need son component to move corresponding structure to common part
 */
#if defined(WLAN_FEATURE_SON)
enum qca_nl80211_vendor_subcmds_internal {
	QCA_NL80211_VENDOR_SUBCMD_SMPS_UPDATE = 330,
	QCA_NL80211_VENDOR_SUBCMD_OPMODE_UPDATE = 331,
};
#endif

enum  {
	  HT20 = 0x0,
	  HT40 = 1,
	  VHT20 = 2,
	  VHT40 = 3,
	  VHT80 = 4,
	  VHT160 = 5,
	  VHT80_80 = 6,
	  HE20 = 7,
	  HE40 = 8,
	  HE80 = 9,
	  HE160 = 0xa,
	  HE80_80 = 0xb,
	  NONHT = 0xc,
#ifdef WLAN_FEATURE_11BE
	  EHT20 = 0xd,
	  EHT40 = 0xe,
	  EHT80 = 0xf,
	  EHT160 = 0x10,
	  EHT320 = 0x11,
#endif /* WLAN_FEATURE_11BE */
};

/*
 * TD: include ol_if_athvar.h
 * brief Get whether the Radio is tuned for low, high, full band or 2g.
 */
enum band_type {
	/* unable to retrieve band info due to some error */
	NO_BAND_INFORMATION_AVAILABLE = 0,
	HIGH_BAND_RADIO, /*RADIO_IN_HIGH_BAND*/
	FULL_BAND_RADIO,/* RADIO_IN_FULL_BAND */
	LOW_BAND_RADIO, /* RADIO_IN_LOW_BAND */
	NON_5G_RADIO, /* RADIO_IS_NON_5G */
	BAND_6G_RADIO, /*RADIO_IS_NON_5G_24G */
	BAND_5G_6G_RADIO /* RADIO_IS_EITHER_5G_6G */
};

/* enum to define secondary 20 channel offset
 * @EXT_CHAN_OFFSET_NA: no extension channel is present
 * @EXT_CHAN_OFFSET_ABOVE: above control channel
 * @EXT_CHAN_OFFSET_BELOW: below control channel
 */
enum sec20_chan_offset {
	    EXT_CHAN_OFFSET_NA    = 0,
	    EXT_CHAN_OFFSET_ABOVE = 1,
	    EXT_CHAN_OFFSET_BELOW = -1
};

#if defined(WLAN_FEATURE_SON) || defined(MORE_WIN_HEADER_FILE)
struct ol_ath_dbg_rx_rssi {
	    uint8_t     rx_rssi_pri20;
	    uint8_t     rx_rssi_sec20;
	    uint8_t     rx_rssi_sec40;
	    uint8_t     rx_rssi_sec80;
};

struct ol_ath_radiostats {
	uint64_t    tx_beacon;
	uint32_t    tx_buf_count;
	int32_t     tx_mgmt;
	int32_t     rx_mgmt;
	uint32_t    rx_num_mgmt;
	uint32_t    rx_num_ctl;
	uint32_t    tx_rssi;
	uint32_t    rx_rssi_comb;
	struct      ol_ath_dbg_rx_rssi rx_rssi_chain0;
	struct      ol_ath_dbg_rx_rssi rx_rssi_chain1;
	struct      ol_ath_dbg_rx_rssi rx_rssi_chain2;
	struct      ol_ath_dbg_rx_rssi rx_rssi_chain3;
	uint32_t    rx_overrun;
	uint32_t    rx_phyerr;
	uint32_t    ackrcvbad;
	uint32_t    rtsbad;
	uint32_t    rtsgood;
	uint32_t    fcsbad;
	uint32_t    nobeacons;
	uint32_t    mib_int_count;
	uint32_t    rx_looplimit_start;
	uint32_t    rx_looplimit_end;
	uint8_t     ap_stats_tx_cal_enable;
	uint8_t     self_bss_util;
	uint8_t     obss_util;
	uint8_t     ap_rx_util;
	uint8_t     free_medium;
	uint8_t     ap_tx_util;
	uint8_t     obss_rx_util;
	uint8_t     non_wifi_util;
	uint32_t    tgt_asserts;
	int16_t     chan_nf;
	int16_t     chan_nf_sec80;
	uint64_t    wmi_tx_mgmt;
	uint64_t    wmi_tx_mgmt_completions;
	uint32_t    wmi_tx_mgmt_completion_err;
	uint32_t    rx_mgmt_rssi_drop;
	uint32_t    tx_frame_count;
	uint32_t    rx_frame_count;
	uint32_t    rx_clear_count;
	uint32_t    cycle_count;
	uint32_t    phy_err_count;
	uint32_t    chan_tx_pwr;
	uint32_t    be_nobuf;
	uint32_t    tx_packets;
	uint32_t    rx_packets;
	uint32_t    tx_num_data;
	uint32_t    rx_num_data;
	uint32_t    tx_mcs[10];
	uint32_t    rx_mcs[10];
	uint64_t    rx_bytes;
	uint64_t    tx_bytes;
	uint32_t    tx_compaggr;
	uint32_t    rx_aggr;
	uint32_t    tx_bawadv;
	uint32_t    tx_compunaggr;
	uint32_t    rx_badcrypt;
	uint32_t    rx_badmic;
	uint32_t    rx_crcerr;
	uint32_t    rx_last_msdu_unset_cnt;
	uint32_t    rx_data_bytes;
	uint32_t    tx_retries;
	uint32_t    created_vap;
	uint32_t    active_vap;
	uint32_t    rnr_count;
	uint32_t    soc_status_6ghz;
};
#endif

#endif //_IEEE80211_EXTERNAL_H_
