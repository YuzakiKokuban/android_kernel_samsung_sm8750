/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2010, Atheros Communications Inc.
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

#ifndef _IEEE80211_IOCTL_H_
#define _IEEE80211_IOCTL_H_

#include <_ieee80211.h>
#include "ieee80211.h"
#include "ieee80211_defines.h"
#include <ext_ioctl_drv_if.h>
#include <wlan_son_ioctl.h>

#include <cfg80211_ven_cmd.h>

#define VENDOR_CHAN_FLAG2(_flag)  \
	    ((uint64_t)(_flag) << 32)

#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_6GHZ (1 << 4)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_A (1 << 5)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_B (1 << 6)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EXT_PSC (1 << 10)

#define TR69_MAX_BUF_LEN    800

#define MAP_DEFAULT_PPDU_DURATION 100
#define MAP_PPDU_DURATION_UNITS 50

#define DEFAULT_IDENTIFIER 0
#define COEX_CFG_MAX_ARGS 6

/*
 * BA Window Size Subfield Encoding
 */
typedef enum wlan_ba_window_size {
	ba_window_not_used = 0,
	ba_window_size_2 = 2,
	ba_window_size_4 = 4,
	ba_window_size_6 = 6,
	ba_window_size_8 = 8,
	ba_window_size_16 = 16,
	ba_window_size_32 = 32,
	ba_window_size_64 = 64,
} wlan_ba_window_size;

/*
 * Data Format Subfield Encoding
 */
typedef enum wlan_data_format {
	data_no_aggregation_enabled,
	data_amsdu_aggregation_enabled,
	data_ampdu_aggregation_enabled,
	data_amsdu_ampdu_aggregation_enabled,
	data_aggregation_max,  // always last
} wlan_data_format;

typedef struct beacon_rssi_info {
	struct beacon_rssi_stats {
	    u_int32_t   ni_last_bcn_snr;
	    u_int32_t   ni_last_bcn_age;
	    u_int32_t   ni_last_bcn_cnt;
	    u_int64_t   ni_last_bcn_jiffies;
	} stats;
	u_int8_t macaddr[IEEE80211_ADDR_LEN];
} beacon_rssi_info;

typedef struct ack_rssi_info {
	struct ack_rssi_stats {
	    u_int32_t   ni_last_ack_rssi;
	    u_int32_t   ni_last_ack_age;
	    u_int32_t   ni_last_ack_cnt;
	    u_int64_t   ni_last_ack_jiffies;
	} stats;
	u_int8_t macaddr[IEEE80211_ADDR_LEN];
} ack_rssi_info;

struct ieee80211_channel_info {
	uint8_t ieee;
	uint16_t freq;
	uint64_t flags;
	uint32_t flags_ext;
	uint8_t vhtop_ch_num_seg1;
	uint8_t vhtop_ch_num_seg2;
};

typedef struct ieee80211req_acs_r {
	u_int32_t index;
	u_int32_t data_size;
	void *data_addr;
} ieee80211req_acs_t;

#define ACS_MAX_CHANNEL_COUNT 255
typedef struct ieee80211_user_chanlist_r {
	u_int16_t n_chan;
	struct ieee80211_chan_def chans[ACS_MAX_CHANNEL_COUNT];
} ieee80211_user_chanlist_t;

typedef struct ieee80211_user_ctrl_tbl_r {
	u_int16_t ctrl_len;
	u_int8_t *ctrl_table_buff;
} ieee80211_user_ctrl_tbl_t;

/*
 * Wlan Latency Parameters
 */
typedef struct wlan_latency_info {
	uint32_t tid;
	uint32_t dl_ul_enable;
	uint32_t service_interval;
	uint32_t burst_size;
	uint32_t burst_size_add_or_sub;
	uint8_t  peer_mac[IEEE80211_ADDR_LEN];
} wlan_latency_info_t;

#define MAX_CUSTOM_CHANS	 101

typedef struct {
	/* Number of channels to scan, 0 for all channels */
	u_int8_t     scan_numchan_associated;
	u_int8_t     scan_numchan_nonassociated;
	struct ieee80211_chan_def
		scan_channel_list_associated[MAX_CUSTOM_CHANS];
	struct ieee80211_chan_def
		scan_channel_list_nonassociated[MAX_CUSTOM_CHANS];
} ieee80211req_custom_chan_t;

typedef struct ieee80211_wide_band_scan {
	u_int8_t bw_mode;
	u_int8_t sec_chan_offset;
} ieee80211_wide_band_scan_t;

/**
 * struct ieee80211_offchan_tx_test: off channel tx parameters
 * @band: band in which off chan tx has to happen
 * @ieee_chan: primary 20 offchan tx chan
 * @dwell_time: Dwell time
 * @wide_scan: indicated wide bandwidth scan
 * @ieee_cfreq2: center frequency of continuous bandwidth channel of center freq
 *	           of secondary 80 of 80+80 channel
 */
typedef struct ieee80211_offchan_tx_test {
	u_int8_t band;
	u_int8_t ieee_chan;
	u_int16_t dwell_time;
	ieee80211_wide_band_scan_t wide_scan;
	u_int8_t ieee_cfreq2;
} ieee80211_offchan_tx_test_t;

typedef struct ieee80211_node_info {
	u_int16_t count;
	u_int16_t bin_number;
	u_int32_t traf_rate;
} ieee80211_node_info_t;

struct ieee80211req_fake_mgmt {
	u_int32_t buflen;  /*application supplied buffer length */
	u_int8_t  *buf;
};

/*
 * command id's for use in tr069 request
 */
typedef enum _ieee80211_tr069_cmd_ {
	TR069_CHANHIST           = 1,
	TR069_TXPOWER            = 2,
	TR069_GETTXPOWER         = 3,
	TR069_GUARDINTV          = 4,
	TR069_GET_GUARDINTV      = 5,
	TR069_GETASSOCSTA_CNT    = 6,
	TR069_GETTIMESTAMP       = 7,
	TR069_GETDIAGNOSTICSTATE = 8,
	TR069_GETNUMBEROFENTRIES = 9,
	TR069_GET11HSUPPORTED    = 10,
	TR069_GETPOWERRANGE      = 11,
	TR069_SET_OPER_RATE      = 12,
	TR069_GET_OPER_RATE      = 13,
	TR069_GET_POSIBLRATE     = 14,
	TR069_SET_BSRATE         = 15,
	TR069_GET_BSRATE         = 16,
	TR069_GETSUPPORTEDFREQUENCY  = 17,
	TR069_GET_PLCP_ERR_CNT   = 18,
	TR069_GET_FCS_ERR_CNT    = 19,
	TR069_GET_PKTS_OTHER_RCVD = 20,
	TR069_GET_FAIL_RETRANS_CNT = 21,
	TR069_GET_RETRY_CNT      = 22,
	TR069_GET_MUL_RETRY_CNT  = 23,
	TR069_GET_ACK_FAIL_CNT   = 24,
	TR069_GET_AGGR_PKT_CNT   = 25,
	TR069_GET_STA_BYTES_SENT = 26,
	TR069_GET_STA_BYTES_RCVD = 27,
	TR069_GET_DATA_SENT_ACK  = 28,
	TR069_GET_DATA_SENT_NOACK = 29,
	TR069_GET_CHAN_UTIL      = 30,
	TR069_GET_RETRANS_CNT    = 31,
	TR069_GET_RRM_UTIL       = 32,
	TR069_GET_CSA_DEAUTH     = 33,
	TR069_SET_CSA_DEAUTH     = 34,
} ieee80211_tr069_cmd;

/*
 * common structure to handle tr069 commands;
 * the cmdid and data pointer has to be appropriately
 * filled in
 */
typedef struct{
	u_int32_t data_size;
	ieee80211_tr069_cmd cmdid;
	u_int8_t data_buff[TR69_MAX_BUF_LEN];
} ieee80211req_tr069_t;

typedef struct ieee80211req_fips {
	u_int32_t data_size;
	void *data_addr;
} ieee80211req_fips_t;

#define MAX_FW_UNIT_TEST_NUM_ARGS 100
/**
 * struct ieee80211_fw_unit_test_cmd - unit test command parameters
 * @module_id: module id
 * @num_args: number of arguments
 * @diag_token: Token representing the transaction ID (between host and fw)
 * @args: arguments
 */
struct ieee80211_fw_unit_test_cmd {
	    u_int32_t module_id;
	    u_int32_t num_args;
	    u_int32_t diag_token;
	    u_int32_t args[MAX_FW_UNIT_TEST_NUM_ARGS];
} __attribute__((packed));

/*
 * Parameters that can be configured by userspace on a per client
 * basis
 */
typedef struct ieee80211_acl_cli_param_t {
	u_int8_t  probe_rssi_hwm;
	u_int8_t  probe_rssi_lwm;
	u_int8_t  inact_snr_xing;
	u_int8_t  low_snr_xing;
	u_int8_t  low_rate_snr_xing;
	u_int8_t  high_rate_snr_xing;
	u_int8_t  auth_block;
	u_int8_t  auth_rssi_hwm;
	u_int8_t  auth_rssi_lwm;
	u_int8_t  auth_reject_reason;
} ieee80211_acl_cli_param_t;

typedef struct __ieee80211req_dbptrace {
	u_int32_t dp_trace_subcmd;
	u_int32_t val1;
	u_int32_t val2;
} ieee80211req_dptrace;

/**
 * struct coex_cfg_t - coex configuration command parameters
 * @type: config type (wmi_coex_config_type enum)
 * @arg: arguments based on config type
 */
typedef struct {
	u_int32_t type;
	u_int32_t arg[COEX_CFG_MAX_ARGS];
} coex_cfg_t;

struct ieee80211_twt_add_dialog {
	uint32_t dialog_id;
	uint32_t wake_intvl_us;
	uint32_t wake_intvl_mantis;
	uint32_t wake_dura_us;
	uint32_t sp_offset_us;
	uint32_t twt_cmd;
	uint32_t flags;
};

#ifdef WLAN_SUPPORT_TWT
struct ieee80211_twt_del_pause_dialog {
	uint32_t dialog_id;
#ifdef WLAN_SUPPORT_BCAST_TWT
	uint32_t b_twt_persistence;
#endif
};

struct ieee80211_twt_resume_dialog {
	uint32_t dialog_id;
	uint32_t sp_offset_us;
	uint32_t next_twt_size;
};

#ifdef WLAN_SUPPORT_BCAST_TWT
struct ieee80211_twt_btwt_sta_inv_remove {
	uint32_t dialog_id;
};
#endif
#endif

struct  ieee80211req_athdbg {
	u_int8_t cmd;
	u_int8_t needs_reply;
	u_int8_t dstmac[IEEE80211_ADDR_LEN];
	union {
	    u_long param[4];
	    ieee80211_rrm_beaconreq_info_t bcnrpt;
	    ieee80211_rrm_tsmreq_info_t    tsmrpt;
	    ieee80211_rrm_nrreq_info_t     neigrpt;
	    struct ieee80211_bstm_reqinfo   bstmreq;
	    struct ieee80211_bstm_reqinfo_target   bstmreq_target;
	    struct ieee80211_mscs_resp      mscsresp;
#if defined(WLAN_SUPPORT_DAP) && (WLAN_SUPPORT_DAP != 0)
	    struct ieee80211_qos_mgmt_ie    qos_data;
	    struct ieee80211_qos_req        dscp_req;
#endif
	    struct ieee80211_user_bssid_pref bssidpref;
	    ieee80211_tspec_info     tsinfo;
	    ieee80211_rrm_cca_info_t   cca;
	    ieee80211_rrm_rpihist_info_t   rpihist;
	    ieee80211_rrm_chloadreq_info_t chloadrpt;
	    ieee80211_rrm_stastats_info_t  stastats;
	    ieee80211_rrm_nhist_info_t     nhist;
	    ieee80211_rrm_frame_req_info_t frm_req;
	    ieee80211_rrm_lcireq_info_t    lci_req;
	    ieee80211req_rrmstats_t        rrmstats_req;
	    ieee80211req_acs_t             acs_rep;
	    ieee80211req_tr069_t           tr069_req;
	    struct timespec t_spec;
	    ieee80211req_fips_t fips_req;
	    struct ieee80211_qos_map       qos_map;
#if defined(QCA_SUPPORT_SON) && (QCA_SUPPORT_SON != 0)
	    ieee80211_bsteering_rssi_req_t bsteering_rssi_req;
#endif
	    ieee80211_acl_cli_param_t      acl_cli_param;
	    ieee80211_offchan_tx_test_t offchan_req;
#if defined(UMAC_SUPPORT_VI_DBG) && (UMAC_SUPPORT_VI_DBG != 0)
	ieee80211_vow_dbg_stream_param_t   vow_dbg_stream_param;
	ieee80211_vow_dbg_param_t	   vow_dbg_param;
#endif

#if defined(QCA_LTEU_SUPPORT) && (QCA_LTEU_SUPPORT != 0)
	    ieee80211req_mu_scan_t         mu_scan_req;
	    ieee80211req_lteu_cfg_t        lteu_cfg;
	    ieee80211req_ap_scan_t         ap_scan_req;
#endif
	    ieee80211req_custom_chan_t     custom_chan_req;
#if defined(QCA_AIRTIME_FAIRNESS) && (QCA_AIRTIME_FAIRNESS != 0)
	    atf_debug_req_t                atf_dbg_req;
#endif
	struct ieee80211_fw_unit_test_cmd	fw_unit_test_cmd;
	    ieee80211_user_ctrl_tbl_t      *user_ctrl_tbl;
	    ieee80211_user_chanlist_t      user_chanlist;
	    ieee80211req_dptrace           dptrace;
	    coex_cfg_t                     coex_cfg_req;
#if defined(ATH_ACL_SOFTBLOCKING) && (ATH_ACL_SOFTBLOCKING != 0)
	    u_int8_t                       acl_softblocking;
#endif
#ifdef WLAN_SUPPORT_TWT
	    struct ieee80211_twt_add_dialog twt_add;
	    struct ieee80211_twt_del_pause_dialog twt_del_pause;
	    struct ieee80211_twt_resume_dialog twt_resume;
#ifdef WLAN_SUPPORT_BCAST_TWT
	    struct ieee80211_twt_btwt_sta_inv_remove twt_btwt_sta_inv_remove;
#endif
#endif
#if defined(QCA_SUPPORT_SON) && (QCA_SUPPORT_SON != 0)
	    ieee80211_bsteering_innetwork_2g_req_t innetwork_2g_req;
#endif
	    ieee80211_node_info_t          node_info;
	    beacon_rssi_info               beacon_rssi_info;
	    ack_rssi_info                  ack_rssi_info;
#ifdef WLAN_SUPPORT_RX_FLOW_TAG
	    ieee80211_wlanconfig_rx_flow_tag rx_flow_tag_info;
#endif /* WLAN_SUPPORT_RX_FLOW_TAG */
	    ieee80211_user_nrresp_info_t   *neighrpt_custom;
#if defined(WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN) && \
	   (WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN != 0)
	    ieee80211_primary_allowed_chanlist_t primary_allowed_chanlist;
#endif
	    struct ieee80211req_fake_mgmt mgmt_frm;
#if defined(QCA_SUPPORT_SON) && (QCA_SUPPORT_SON != 0)
	    mesh_dbg_req_t mesh_dbg_req;
	    map_wifi6_stastats_t map_wifi6_sta_stats;
#endif
	    struct ieee80211_tpe_ie_config tpe_conf;
	    wlan_latency_info_t wlan_latency_info;
#ifdef QCA_SUPPORT_SCAN_SPCL_VAP_STATS
	    ieee80211_scan_spcl_vap_stats_t scan_spcl_vap_stats;
#endif
	} data;
};

struct ieee80211_smps_update_data {
	uint8_t is_static;
	uint8_t macaddr[IEEE80211_ADDR_LEN];
};

struct ieee80211_opmode_update_data {
	uint8_t max_chwidth;
	uint8_t num_streams;
	uint8_t macaddr[IEEE80211_ADDR_LEN];
};

/*
 * Get the active channel list info.
 */
struct ieee80211req_chaninfo {
	uint32_t ic_nchans;
	struct ieee80211_ath_channel ic_chans[IEEE80211_CHAN_MAX];
};

struct ieee80211req_channel_list {
	uint32_t nchans;
	struct ieee80211_channel_info chans[IEEE80211_CHAN_MAX];
};

/*
 * Retrieve per-node statistics.
 */
struct ieee80211req_sta_stats {
	union {
		/* NB: explicitly force 64-bit alignment */
		u_int8_t	macaddr[IEEE80211_ADDR_LEN];
		u_int64_t	pad;
	} is_u;
	struct ieee80211_nodestats is_stats;
};

/*
 * athdbg request
 */
enum {
	/* GET the ACS report */
	IEEE80211_DBGREQ_GETACSREPORT  =	20,
	/* SET ch list for acs reporting  */
	IEEE80211_DBGREQ_SETACSUSERCHANLIST  =    21,
	/*  Mesh config set and get */
	IEEE80211_DBGREQ_MESH_SET_GET_CONFIG          = 117,
};

/**
 * struct ieee80211_wlanconfig_ipa_wds - For deleting wds type ast entry
 * @wds_ni_macaddr  : wds ni macaddress
 * @wds_macaddr_cnt : wds node mac address cnt
 * @wds_macaddr : wds node mac address
 * @peer_macaddr : direct associated peer mac address
 */
struct ieee80211_wlanconfig_ipa_wds {
	u_int8_t  wds_ni_macaddr[IEEE80211_ADDR_LEN];
	u_int16_t wds_macaddr_cnt;
	u_int8_t  wds_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t  peer_macaddr[IEEE80211_ADDR_LEN];
};

/* generic structure to support sub-ioctl due to limited ioctl */
typedef enum {
	IEEE80211_WLANCONFIG_NOP = 0,
	IEEE80211_WLANCONFIG_IPA_WDS_REMOVE_ADDR = 81,
} IEEE80211_WLANCONFIG_CMDTYPE;

typedef enum {
	IEEE80211_WLANCONFIG_OK		  = 0,
	IEEE80211_WLANCONFIG_FAIL		= 1,
} IEEE80211_WLANCONFIG_STATUS;

struct ieee80211_wlanconfig_setmaxrate {
	u_int8_t mac[IEEE80211_ADDR_LEN];
	u_int8_t maxrate;
};

struct ieee80211_wlanconfig {
	IEEE80211_WLANCONFIG_CMDTYPE cmdtype;  /* sub-command */
	IEEE80211_WLANCONFIG_STATUS status;	 /* status code */
	union {
		struct ieee80211_wlanconfig_ipa_wds ipa_wds;
	} data;

	struct ieee80211_wlanconfig_setmaxrate smr;
};

#endif //_IEEE80211_IOCTL_H_
