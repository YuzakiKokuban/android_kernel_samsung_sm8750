/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * Qualcomm Atheros, Inc. has chosen to take madwifi subject to the BSD license and terms.
 *
 * Copyright (c) 2008, Atheros Communications Inc.
 */

#ifndef _IEEE80211_DEFINES_H_
#define _IEEE80211_DEFINES_H_

#include <ieee80211.h>
#include <_ieee80211.h>

#define HOST_MAX_CHAINS 8 /* Max Tx/Rx chains */
#define MAX_CHAINS 4

#define IEEE80211_NWID_LEN                  32

#define IEEE80211_RU_INDEX_MAX 6
#define IEEE80211_DOT11_MAX 5
#define IEEE80211_MAX_MCS (14 + 1)

#define WLAN_DEFAULT_NETLINK_PID 0xffffffff

#define IEEE80211_IS_CHANNEL_VALID(c) (c && c->ic_freq)

/* Note this is dependent on firmware - currently can only have 3 per event */
#define BSTEERING_MAX_PEERS_PER_EVENT 3

/*
 * Opaque handle scan_entry.
 */
struct ieee80211_scan_entry;
typedef struct scan_cache_entry *wlan_scan_entry_t;

typedef struct _ieee80211_ssid {
	u_int32_t   len;
	u_int8_t    ssid[IEEE80211_NWID_LEN + 1];
} ieee80211_ssid;

/*
 * The type of the CBS event
 */
typedef enum wlan_cbs_event_type {
	/// CBS has completed
	CBS_COMPLETE,
	/// CBS was cancelled before completion
	CBS_CANCELLED
} wlan_cbs_event_type;

typedef struct ieee80211_neighbor_info {
	u_int32_t   phymode;                      /* ap channel width*/
	int         rssi;                         /* ap signal strength */
	u_int8_t    bssid[IEEE80211_ADDR_LEN];    /* BSSID information */
	u_int8_t    ssid_len;                     /* length of the ssid */
	u_int8_t    ssid[IEEE80211_NWID_LEN + 1]; /* SSID details */
	/* Flag to indicate if qbss load ie is present */
	u_int8_t    qbssload_ie_valid;
	/* number of station associated */
	u_int8_t    station_count;
	/* channel busy time in 0-255 scale */
	u_int8_t    channel_utilization;
} ieee80211_neighbor_info;

typedef enum _ieee80211_frame_type {
	IEEE80211_FRAME_TYPE_PROBEREQ,
	IEEE80211_FRAME_TYPE_BEACON,
	IEEE80211_FRAME_TYPE_PROBERESP,
	IEEE80211_FRAME_TYPE_ASSOCREQ,
	IEEE80211_FRAME_TYPE_ASSOCRESP,
	IEEE80211_FRAME_TYPE_AUTH
} ieee80211_frame_type;

typedef enum {
	BSTEER_SOURCE_LOCAL = 0, /* Disconnect initiated by AP */
	BSTEER_SOURCE_REMOTE /* Disconnect initiated by STA */
} BSTEER_DISCONNECT_SOURCE;

typedef enum {
	BSTEER_DISASSOC = 0, /* Disconnect as a result of disassoc */
	BSTEER_DEAUTH /* Disconnect as a result of deauth */
} BSTEER_DISCONNECT_TYPE;

/**
 * struct ieee80211_nodestats - Per/node (station) statistics available
 * when operating as an AP.
 * @ns_rx_mgmt_rssi: mgmt frame rssi
 * @ns_rx_mgmt: rx management frames
 * @ns_rx_noprivacy: rx w/ wep but privacy off
 * @ns_rx_wepfail:          rx wep processing failed
 * @ns_rx_ccmpmic:          rx CCMP MIC failure
 * @ns_rx_wpimic:           rx WAPI MIC failure
 * @ns_rx_tkipicv:          rx ICV check failed (TKIP)
 * @ns_tx_mgmt:             tx management frames
 * @ns_is_tx_not_ok:        tx not ok
 * @ns_ps_discard:          ps discard 'cuz of age
 * @ns_last_rx_mgmt_rate:        last received mgmt frame rate
 * @ns_psq_drops:          power save queue drops
 * @ns_tx_dropblock:
 * @ns_tx_assoc:           [re]associations
 * @ns_tx_assoc_fail:      [re]association failures
 * @ns_is_tx_nobuf:
 * @ns_rx_decryptcrc:       rx decrypt failed on crc
 * @ns_rx_data:            rx data frames
 * @ns_rx_ctrl:            rx control frames
 * @ns_rx_ucast:           rx unicast frames
 * @ns_rx_mcast:           rx multi/broadcast frames
 * @ns_rx_bytes:           rx data count (bytes)
 * @ns_last_per:           last packet error rate
 * @ns_rx_beacons:         rx beacon frames
 * @ns_rx_proberesp:       rx probe response frames
 * @ns_rx_dup:             rx discard 'cuz dup
 * @ns_rx_demicfail:       rx demic failed
 *   We log MIC and decryption failures against Transmitter STA stats.
 *   Though the frames may not actually be sent by STAs corresponding
 *   to TA, the stats are still valuable for some customers as a sort
 *   of rough indication.
 *   Also note that the mapping from TA to STA may fail sometimes.
 * @ns_rx_tkipmic:          rx TKIP MIC failure
 * @ns_rx_decap:           rx decapsulation failed
 * @ns_rx_defrag:          rx defragmentation failed
 * @ns_rx_disassoc:        rx disassociation
 * @ns_rx_deauth:          rx deauthentication
 * @ns_rx_action:          rx action
 * @ns_rx_unauth:          rx on unauthorized port
 * @ns_rx_unencrypted:     rx unecrypted w/ privacy
 * @ns_tx_data:            tx data frames
 * @ns_tx_data_success: tx data frames successfully transmitted (unicast only)
 * @ns_tx_wme:          tx data per AC
 * @ns_rx_wme:          rx data per AC
 * @ns_tx_ucast:           tx unicast frames
 * @ns_tx_mcast:           tx multi/broadcast frames
 * @ns_tx_bcast:           tx broadcast frames
 * @ns_tx_bytes:           tx data count (bytes)
 * @ns_tx_bytes_success:   tx success data count - unicast only (bytes)
 * @ns_tx_probereq:        tx probe request frames
 * @ns_tx_uapsd:           tx on uapsd queue
 * @ns_tx_discard:         tx dropped by NIC
 * @ns_tx_novlantag:       tx discard 'cuz no tag
 * @ns_tx_vlanmismatch:    tx discard 'cuz bad tag
 * @ns_tx_eosplost:        uapsd EOSP retried out
 * @ns_uapsd_triggers:     uapsd triggers
 * @ns_uapsd_duptriggers:   uapsd duplicate triggers
 * @ns_uapsd_ignoretriggers: uapsd duplicate triggers
 * @ns_uapsd_active:        uapsd duplicate triggers
 * @ns_uapsd_triggerenabled:  uapsd duplicate triggers
 * @ns_last_tx_rate:        last tx data rate
 * @ns_last_rx_rate:        last received data frame rate
 * @ns_dot11_tx_bytes:
 * @ns_dot11_rx_bytes:
 * @ns_tx_bytes_rate:
 * @ns_tx_data_rate:
 * @ns_rx_bytes_rate:
 * @ns_rx_data_rate:
 * @ns_tx_bytes_success_last:
 * @ns_tx_data_success_last:
 * @ns_rx_bytes_last:
 * @ns_rx_data_last:
 * @ns_tx_auth:            [re]authentications
 * @ns_tx_auth_fail:       [re]authentication failures
 * @ns_tx_deauth:          deauthentications
 * @ns_tx_deauth_code:     last deauth reason
 * @ns_tx_disassoc:        disassociations
 * @ns_tx_disassoc_code:   last disassociation reason
 * @ns_rssi_chain:  Ack RSSI per chain
 * @inactive_time:
 * @ns_tx_mu_blacklisted_cnt: number of time MU tx has been blacklisted
 * @ns_excretries: excessive retries
 * @ns_rx_ucast_bytes:     tx bytes of unicast frames
 * @ns_rx_mcast_bytes:     tx bytes of multi/broadcast frames
 * @ns_tx_ucast_bytes:     tx bytes of unicast frames
 * @ns_tx_mcast_bytes:     tx bytes of multi/broadcast frames
 * @ns_tx_bcast_bytes:     tx bytes of broadcast frames
 * @ns_rx_mpdus:           Number of rs mpdus
 * @ns_rx_ppdus:           Number of ppdus
 * @ns_rx_retries:         Number of rx retries
 * @ns_ppdu_tx_rate:       Avg per ppdu tx rate in kbps
 * @ns_ppdu_rx_rate:       Avg per ppdu rx rate in kbps
 * @ns_rssi: most recent received packet RSSI from connected sta
 *           used for MAP
 * @ns_rssi_time_delta: time delta in ms between rssi measurement
 *                      and IOCTL call
 * @ns_failed_retry_count: No of packets not trans successfully due
 *           to no of retrans attempts exceeding 802.11 retry limit
 * @ns_retry_count: No of packets that were successfully transmitted
 *           after one or more retransmissions
 * @ns_multiple_retry_count: No of packets that were successfully
 *           transmitted after more than one retransmission
 * @ru_tx:
 * @mcs_tx:
 * @ru_rx:
 * @mcs_rx:
 * @icmp_tx_ingress:
 * @icmp_tx_egress:
 * @icmp_rx_ingress:
 * @icmp_rx_egress:
 * @arp_tx_ingress:
 * @arp_tx_egress:
 * @arp_rx_ingress:
 * @arp_rx_egress:
 * @eap_tx_ingress:
 * @eap_tx_egress:
 * @eap_rx_ingress:
 * @eap_rx_egress:
 * @ns_tx_data_success_twt:    tx data frames successfully TX
 * @ns_rx_data_twt:            rx data frames
 * @ns_twt_event_type:  TWT session type
 * @ns_twt_flow_id:  TWT flow id
 * @ns_twt_bcast:     Broadcast TWT
 * @ns_twt_trig:     TWT trigger
 * @ns_twt_announ:  TWT announcement
 * @ns_twt_dialog_id:  TWT diag ID
 * @ns_twt_wake_dura_us:  Wake time duration in us
 * @ns_twt_wake_intvl_us:  Interval between wake perions in us
 * @ns_twt_sp_offset_us:  Time until first TWT SP occurs
 * @ns_avg_rssi:  avg rssi
 */
struct ieee80211_nodestats {
	/* All below fields are moved to cp_stats component */
	int8_t       ns_rx_mgmt_rssi;
	u_int32_t    ns_rx_mgmt;
	u_int32_t    ns_rx_noprivacy;
	u_int32_t    ns_rx_wepfail;
	u_int32_t    ns_rx_ccmpmic;
	u_int32_t    ns_rx_wpimic;
	u_int32_t    ns_rx_tkipicv;
	u_int32_t    ns_tx_mgmt;
	u_int32_t    ns_is_tx_not_ok;
	u_int32_t    ns_ps_discard;
	u_int32_t    ns_last_rx_mgmt_rate;
	u_int32_t    ns_psq_drops;
#ifdef ATH_SUPPORT_IQUE
	u_int32_t    ns_tx_dropblock;
#endif
	u_int32_t    ns_tx_assoc;
	u_int32_t    ns_tx_assoc_fail;
	u_int32_t    ns_is_tx_nobuf;
	u_int32_t    ns_rx_decryptcrc;
	/* end of cp stats fields */
	u_int32_t    ns_rx_data;

	u_int32_t    ns_rx_ctrl;
	u_int32_t    ns_rx_ucast;
	u_int32_t    ns_rx_mcast;
	u_int64_t    ns_rx_bytes;
	u_int64_t    ns_last_per;
	u_int64_t    ns_rx_beacons;
	u_int32_t    ns_rx_proberesp;

	u_int32_t    ns_rx_dup;
	u_int32_t    ns_rx_demicfail;

	/* We log MIC and decryption failures against Transmitter STA stats.
	 * Though the frames may not actually be sent by STAs corresponding
	 * to TA, the stats are still valuable for some customers as a sort
	 * of rough indication.
	 * Also note that the mapping from TA to STA may fail sometimes.
	 */
	u_int32_t    ns_rx_tkipmic;
	u_int32_t    ns_rx_decap;
	u_int32_t    ns_rx_defrag;
	u_int32_t    ns_rx_disassoc;
	u_int32_t    ns_rx_deauth;
	u_int32_t    ns_rx_action;
	u_int32_t    ns_rx_unauth;
	u_int32_t    ns_rx_unencrypted;

	u_int32_t    ns_tx_data;
	u_int32_t    ns_tx_data_success;
	u_int64_t    ns_tx_wme[4];
	u_int64_t    ns_rx_wme[4];
	u_int32_t    ns_tx_ucast;
	u_int32_t    ns_tx_mcast;
	u_int32_t    ns_tx_bcast;
	u_int64_t    ns_tx_bytes;
	u_int64_t    ns_tx_bytes_success;
	u_int32_t    ns_tx_probereq;
	u_int32_t    ns_tx_uapsd;
	u_int32_t    ns_tx_discard;
	u_int32_t    ns_tx_novlantag;
	u_int32_t    ns_tx_vlanmismatch;

	u_int32_t    ns_tx_eosplost;

	u_int32_t    ns_uapsd_triggers;
	u_int32_t    ns_uapsd_duptriggers;
	u_int32_t    ns_uapsd_ignoretriggers;
	u_int32_t    ns_uapsd_active;
	u_int32_t    ns_uapsd_triggerenabled;
	u_int32_t    ns_last_tx_rate;
	u_int32_t    ns_last_rx_rate;

	/* MIB-related state */
	u_int32_t    ns_dot11_tx_bytes;
	u_int32_t    ns_dot11_rx_bytes;
#if defined(ATH_SUPPORT_EXT_STAT) && (ATH_SUPPORT_EXT_STAT != 0)
	u_int32_t    ns_tx_bytes_rate;
	u_int32_t    ns_tx_data_rate;
	u_int32_t    ns_rx_bytes_rate;
	u_int32_t    ns_rx_data_rate;
	u_int32_t    ns_tx_bytes_success_last;
	u_int32_t    ns_tx_data_success_last;
	u_int32_t    ns_rx_bytes_last;
	u_int32_t    ns_rx_data_last;
#endif
	u_int32_t    ns_tx_auth;
	u_int32_t    ns_tx_auth_fail;
	u_int32_t    ns_tx_deauth;
	u_int32_t    ns_tx_deauth_code;
	u_int32_t    ns_tx_disassoc;
	u_int32_t    ns_tx_disassoc_code;
	u_int32_t    ns_rssi_chain[MAX_CHAINS];
	u_int32_t   inactive_time;
	u_int32_t   ns_tx_mu_blacklisted_cnt;
	u_int64_t   ns_excretries[WME_NUM_AC];
#if defined(UMAC_SUPPORT_STA_STATS_ENHANCEMENT) && \
	   (UMAC_SUPPORT_STA_STATS_ENHANCEMENT != 0)
	u_int64_t    ns_rx_ucast_bytes;
	u_int64_t    ns_rx_mcast_bytes;
	u_int64_t    ns_tx_ucast_bytes;
	u_int64_t    ns_tx_mcast_bytes;
	u_int64_t    ns_tx_bcast_bytes;
#endif
	u_int64_t    ns_rx_mpdus;
	u_int64_t    ns_rx_ppdus;
	u_int64_t    ns_rx_retries;
	u_int32_t    ns_ppdu_tx_rate;
	u_int32_t    ns_ppdu_rx_rate;
	u_int32_t    ns_rssi;
	u_int32_t    ns_rssi_time_delta;
	u_int32_t ns_failed_retry_count;
	u_int32_t ns_retry_count;
	u_int32_t ns_multiple_retry_count;
	u_int32_t ru_tx[IEEE80211_RU_INDEX_MAX];
	u_int32_t mcs_tx[IEEE80211_DOT11_MAX][IEEE80211_MAX_MCS];
	u_int32_t ru_rx[IEEE80211_RU_INDEX_MAX];
	u_int32_t mcs_rx[IEEE80211_DOT11_MAX][IEEE80211_MAX_MCS];

#ifdef VDEV_PEER_PROTOCOL_COUNT
	u_int16_t icmp_tx_ingress;
	u_int16_t icmp_tx_egress;
	u_int16_t icmp_rx_ingress;
	u_int16_t icmp_rx_egress;

	u_int16_t arp_tx_ingress;
	u_int16_t arp_tx_egress;
	u_int16_t arp_rx_ingress;
	u_int16_t arp_rx_egress;

	u_int16_t eap_tx_ingress;
	u_int16_t eap_tx_egress;
	u_int16_t eap_rx_ingress;
	u_int16_t eap_rx_egress;
#endif
	/* TWT stats */
#if defined(WLAN_SUPPORT_TWT) && (WLAN_SUPPORT_TWT != 0)
	u_int32_t ns_tx_data_success_twt;
	u_int32_t ns_rx_data_twt;
	u_int32_t ns_twt_event_type;
	u_int32_t ns_twt_flow_id:16,
		  ns_twt_bcast:1,
		  ns_twt_trig:1,
		  ns_twt_announ:1;
	u_int32_t ns_twt_dialog_id;
	u_int32_t ns_twt_wake_dura_us;
	u_int32_t ns_twt_wake_intvl_us;
	u_int32_t ns_twt_sp_offset_us;
#endif
	u_int32_t ns_avg_rssi;
};

/*
 * MAC ACL operations.
 */
typedef enum {
	IEEE80211_MACCMD_POLICY_OPEN    = 0,  /* set policy: no ACL's */
	IEEE80211_MACCMD_POLICY_ALLOW   = 1,  /* set policy: allow traffic */
	IEEE80211_MACCMD_POLICY_DENY    = 2,  /* set policy: deny traffic */
	IEEE80211_MACCMD_FLUSH          = 3,  /* flush ACL database */
	IEEE80211_MACCMD_DETACH         = 4,  /* detach ACL policy */
	/* set policy: RADIUS managed ACLs */
	IEEE80211_MACCMD_POLICY_RADIUS  = 5,
	/* ACL policies to take effect immediately */
	IEEE80211_MACCMD_IMMEDIATE      = 6
} ieee80211_acl_cmd;

typedef enum wlan_mlme_pdev_param {
	PDEV_SET_SEC_HM_WDS,
	PDEV_GET_ESP_INFO,
	PDEV_GET_DESC_POOLSIZE,
	PDEV_GET_PHY_ERRCNT,
	PDEV_GET_CAPABILITY,
	PDEV_GET_OPERABLE_CHAN,
	PDEV_GET_OPERABLE_CLASS,
	PDEV_GET_OPCLASS_FOR_CUR_HWMODE,
	PDEV_SET_EMESH_RELAYFS_INIT,
	PDEV_SET_EMESH_RELAYFS_DEINIT,
	PDEV_GET_CAC_TLV,
} wlan_mlme_pdev_param;

/* Interface for External modules to interact with wlan driver */
typedef ieee80211_frame_type wlan_frame_type;

typedef enum ieee80211_cwm_width wlan_ch_width;

typedef ieee80211_ssid wlan_ssid;

typedef enum ieee80211_phymode wlan_phy_mode;

#ifdef CONFIG_BAND_6GHZ
//#define MAX_OPERATING_CLASSES 22
#define MAX_OPERATING_CLASSES 32
#else
//#define MAX_OPERATING_CLASSES 17
#define MAX_OPERATING_CLASSES 32
#endif

#ifdef CONFIG_BAND_6GHZ
#define MAX_CHANNELS_PER_OP_CLASS  70
#else
#define MAX_CHANNELS_PER_OP_CLASS  25
#endif

#define WME_AC_MAX 4

#define MAX_HE_MCS 6

//Supported EHT MCS set value of supported EHT Tx and Rx MCS
#define MAX_SET_SUPPORT_EHT_TX_RX_MCS 8

#define IEEE1905_MAX_ROLES 2

typedef struct wlan_radio_basic_capabilities {
	// Maximum number of BSSes supported by this radio
	u_int8_t max_supp_bss;

	// Number of operating classes supported for this radio
	u_int8_t num_supp_op_classes;

	// Info for each supported operating class
	struct {
	// Operating class that this radio is capable of operating on
	// as defined in Table E-4.
	u_int8_t opclass;

	// Maximum transmit power EIRP the radio is capable of transmitting
	// in the current regulatory domain for the operating class.
	// The field is coded as 2's complement signed dBm.
	int8_t max_tx_pwr_dbm;

	// Number of statically non-operable channels in the operating class
	// Other channels from this operating class which are not listed here
	// are supported by this radio.
	u_int8_t num_non_oper_chan;

	// Channel number which is statically non-operable in the operating
	// class (i.e. the radio is never able to operate on this channel)
	u_int8_t non_oper_chan_num[MAX_CHANNELS_PER_OP_CLASS];
	} opclasses[MAX_OPERATING_CLASSES];

	// Basic Radio capabilities are valid
	u_int8_t wlan_radio_basic_capabilities_valid : 1;
} wlan_radio_basic_capabilities;

typedef struct wlan_op_chan {
	u_int8_t num_supp_op_classes;
	// Operating class that this radio is capable of operating on
	// as defined in Table E-4.
	u_int8_t opclass;

	// operating channel width
	wlan_ch_width ch_width;

	// Number of statically operable channels in the operating class
	u_int8_t num_oper_chan;

	// Channel number which is statically operable in the operating class
	u_int8_t oper_chan_num[MAX_CHANNELS_PER_OP_CLASS];

	u_int8_t dfs_required : 1;
} wlan_op_chan;

/*
 * Parameters to store the channel set information for an op class
 */
typedef struct wlan_op_class {
	// Operating class that this radio is capable of operating on
	// as defined in Table E-4.
	u_int8_t opclass;

	// operating channel width
	wlan_ch_width ch_width;

	// secondary channel location w.r.t. primary channel
	// The location is defined by constants
	// IEEE80211_SEC_CHAN_OFFSET_SC{N,A,B}
	u_int8_t sc_loc;

	// Number of channels in the operating class
	u_int8_t num_chan;

	// Channel numbers defined in the regulatory domain
	u_int8_t channels[MAX_CHANNELS_PER_OP_CLASS];
} wlan_op_class;

typedef struct wlan_esp_data {
	struct {
	u_int8_t ba_window_size;
	u_int8_t est_air_time_fraction;
	u_int16_t data_ppdu_dur_target;
	} per_ac[WME_AC_MAX];
} wlan_esp_data;

typedef struct wlan_app_ie_data {
	wlan_frame_type ftype;
	u_int8_t *ie;
	u_int32_t size;
	u_int8_t id;
} wlan_app_ie_data;

typedef enum wlan_acl_flag {
	WLAN_ACL_FLAG_PROBE_RESP_WH,
	WLAN_ACL_FLAG_AUTH_ALLOW,
} wlan_acl_flag;

typedef struct wlan_acl_data {
	u_int8_t dstmac[IEEE80211_ADDR_LEN];
	wlan_acl_flag flag;
	u_int16_t time_sec;
} wlan_acl_data;

typedef struct wlan_rssi_threshold_data {
	u_int8_t mac[IEEE80211_ADDR_LEN];
	u_int32_t inact_rssi_low;
	u_int32_t inact_rssi_high;
	u_int32_t low_rssi;
	u_int32_t low_rate_rssi;
	u_int32_t high_rate_rssi;
} wlan_rssi_threshold_data;

typedef struct wlan_chan {
	u_int16_t ic_freq;
	u_int8_t  ic_ieee;
	u_int64_t ic_flags;
} wlan_chan;

typedef struct wlan_node_info {
	wlan_ch_width max_chwidth;
	wlan_phy_mode phymode;
	u_int8_t num_streams;
	u_int8_t max_MCS;
	u_int8_t max_txpower;
	/* Set to 1 if this client is operating in Static SM Power Save mode */
	u_int8_t is_static_smps : 1,
	/* Set to 1 if this client supports MU-MIMO */
		 is_mu_mimo_supported : 1;
	u_int32_t tx_bitrate;
} wlan_node_info;

typedef struct wlan_ap_ht_capabilities {
	// Maximum number of supported Tx spatial streams
	u_int8_t max_tx_nss;
	// Maximum number of supported Rx spatial streams
	u_int8_t max_rx_nss;
	// HT caps, uses bit fields
	u_int16_t htcap;
} wlan_ap_ht_capabilities;

typedef struct wlan_ap_vht_capabilities {
	// Set to Tx VHT MCS Map field per Figure 9-562.
	// Supported VHT Tx MCS
	u_int16_t supp_tx_mcs;
	// Set to Rx VHT MCS Map field per Figure 9-562.
	// Supported VHT Rx MCS
	u_int16_t supp_rx_mcs;
	// Maximum number of supported Tx spatial streams
	u_int8_t max_tx_nss;
	// Maximum number of supported Rx spatial streams
	u_int8_t max_rx_nss;
	// VHT caps, uses bit fields
	u_int32_t vhtcap;
} wlan_ap_vht_capabilities;

/**
 * struct wlan_ap_he_capabilities - he capabilities of AP mode
 * @num_mcs_entries: number of mcs entries
 * @supported_he_mcs: supported he mcs rate
 * @max_tx_nss: max tx nss
 * @max_rx_nss: max tx nss
 * @phyinfo: Phy information
 * @he_relaxed_edca: HE Signalls EDCA support Enable:1 Disable:0
 * @he_spatial_reuse: HE Signals Spatial Reuse Enable:1 Disable:0
 * @he_multi_bss: HE Multi Bss Enable:1 Disable:0
 * @he_su_ppdu_1x_ltf_800ns_gi: HE Signals 800ns GI for 1x ltf
 * @he_su_mu_ppdu_4x_ltf_800ns_gi: HE Signals 800ns GI for 4x ltf
 * @he_ndp_4x_ltf_3200ns_gi: HE Signals 3200ns GI for NDP with 4x HE-LTF
 * @he_er_su_ppdu_1x_ltf_800ns_gi: HE Signals 800ns GI for ER SU with 4x HE-LTF
 * @he_er_su_ppdu_4x_ltf_800ns_gi: HE Signals 800ns GI for ER SU with 1x HE-LTF
 * @he_su_bfee: HE SU BFEE
 * @he_su_bfer: HE SU BFER
 * @he_mu_bfee: HE MU BFEE
 * @he_mu_bfer: HE MU BFER
 * @he_ul_muofdma: HE UL MU OFDMA
 * @he_dl_muofdma: HE DL MU OFDMA
 * @he_dl_muofdma_bfer: HE DL MU OFDMA BFER
 * @he_ul_mumimo: HE DL MU MIMO
 * @he_sounding_mode: HE/VHT, SU/MU and Trig/Non-Trig sounding
 * @he_muedca: HE MU EDCA
 */
typedef struct wlan_ap_he_capabilities {
	u_int8_t num_mcs_entries;
	u_int16_t supported_he_mcs[MAX_HE_MCS];
	u_int8_t max_tx_nss;
	u_int8_t max_rx_nss;
	u_int32_t phyinfo[HEHANDLE_CAP_PHYINFO_SIZE];
	u_int32_t he_relaxed_edca:1,
		  he_spatial_reuse:1,
		  he_multi_bss:1,
		  he_su_ppdu_1x_ltf_800ns_gi:1,
		  he_su_mu_ppdu_4x_ltf_800ns_gi:1,
		  he_ndp_4x_ltf_3200ns_gi:1,
		  he_er_su_ppdu_1x_ltf_800ns_gi:1,
		  he_er_su_ppdu_4x_ltf_800ns_gi:1,
		  he_su_bfee:1,
		  he_su_bfer:1,
		  he_mu_bfee:1,
		  he_mu_bfer:1,
		  he_ul_muofdma:1,
		  he_dl_muofdma:1,
		  he_dl_muofdma_bfer:1,
		  he_ul_mumimo:1,
		  he_sounding_mode:4,
		  he_muedca:1;
} wlan_ap_he_capabilities;

typedef struct wlan_ap_wifi6_capabilities {
	u_int8_t numofroles;
	struct {
	u_int8_t role : 2;
	u_int8_t he160 : 1;
	u_int8_t he80plus80 : 1;
	u_int16_t supported_he_mcs[MAX_HE_MCS];
	u_int8_t subeamformer : 1;
	u_int8_t subeamformee : 1;
	u_int8_t mu_beam_former_status : 1;
	u_int8_t beam_formee_sts_less_than_80supported : 1;
	u_int8_t beam_formee_sts_more_than_80supported : 1;
	u_int8_t ulmumimosupported : 1;
	u_int8_t ulofdmasupported : 1;
	u_int8_t dlofdmasupported : 1;
	u_int8_t maxuser_per_dl_mumimotxap : 4;
	u_int8_t max_user_per_dl_mumimorxap : 4;
	u_int8_t maxuserdlofdmatxap;
	u_int8_t maxuserdlofdmarxap;
	u_int8_t rtssupported : 1;
	u_int8_t murtssupported : 1;
	u_int8_t multibssidsupported : 1;
	u_int8_t muedcasupported : 1;
	u_int8_t twtrequestersupprted : 1;
	u_int8_t twtrespondersupported : 1;
	} role_cap[IEEE1905_MAX_ROLES];
} wlan_ap_wifi6_capabilities;

typedef struct wlan_ap_eht_capabilities {
	u_int8_t num_mcs_entries;
	u_int32_t supported_eht_mcs[MAX_SET_SUPPORT_EHT_TX_RX_MCS];
	u_int8_t max_tx_nss;
	u_int8_t max_rx_nss;
} wlan_ap_eht_capabilities;

/**
 * struct wlan_ap_cap - ap capabilities
 * @htcap: ht capabilities
 * @vhtcap: vht capabilities
 * @hecap: he capabilities
 * @wifi6cap: wifi6 capabilities
 * @ehtcap: eht capabilities
 * @wlan_ap_ht_capabilities_valid: HT capabilities are valid
 * @wlan_ap_vht_capabilities_valid: VHT capabilities are valid
 * @wlan_ap_he_capabilities_valid: HE capabilities are valid
 * @wlan_ap_wifi6_capabilites_valid: wifi6 capabilities are valid
 * @wlan_ap_eht_capabilities_valid: EHT capabilities are valid
 * @reserved: reserved bits
 */
typedef struct wlan_ap_cap {
	wlan_ap_ht_capabilities htcap;
	wlan_ap_vht_capabilities vhtcap;
	wlan_ap_he_capabilities hecap;
	wlan_ap_wifi6_capabilities wifi6cap;
	wlan_ap_eht_capabilities ehtcap;
	u_int8_t wlan_ap_ht_capabilities_valid : 1,
		 wlan_ap_vht_capabilities_valid : 1,
		 wlan_ap_he_capabilities_valid : 1,
		 wlan_ap_wifi6_capabilites_valid : 1,
		 wlan_ap_eht_capabilities_valid : 1,
		 reserved : 2;
} wlan_ap_cap;

/* Peer param and commands */
typedef struct wlan_client_he_capabilities {
	u_int8_t  he_cap_macinfo[HECAP_MACINFO_SIZE];
	u_int16_t he_cap_rxmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	u_int16_t he_cap_txmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	u_int32_t he_cap_phyinfo[HEHANDLE_CAP_PHYINFO_SIZE];
	u_int32_t he_cap_ppet_numnss_m1;
	u_int32_t he_cap_ppet_ru_mask;
	u_int32_t he_cap_ppet16_ppet8_ru3_ru0[HE_PPET16_PPET8_SIZE];
	u_int32_t he_cap_info_internal;
} wlan_client_he_capabilities;

typedef struct wlan_peer_cap {
	u_int8_t is_BTM_Supported : 1,
		 is_RRM_Supported : 1,
		 is_beacon_meas_supported : 1,
		 band_cap : 3;
	wlan_node_info info;
	u_int8_t bssid[IEEE80211_ADDR_LEN];
	u_int16_t htcap;
	u_int32_t vhtcap;
	wlan_client_he_capabilities hecap;
	u_int8_t rrmcaps[RRM_CAPS_LEN];
} wlan_peer_cap;

typedef enum wlan_mlme_vdev_param {
	VDEV_SET_IE,
	VDEV_CLR_IE,
	VDEV_SET_ACL,
	VDEV_CLR_ACL,
	VDEV_SET_ACL_TIMER,
	VDEV_SET_PEER_ACT_STATS,
	VDEV_SET_SEC_STA_WDS,
	VDEV_SET_MEC,
	VDEV_SET_MBO_IE_BSTM,
	VDEV_SET_WPS_ACL_ENABLE,
	VDEV_SET_WNM_BSS_PREF,
	VDEV_SET_SON_MAP_VERSION,
	VDEV_SET_MCTBL,
	VDEV_GET_NSS,
	VDEV_GET_CHAN,
	VDEV_GET_CHAN_WIDTH,
	VDEV_GET_CHAN_UTIL,
	VDEV_GET_APCAP,
	VDEV_GET_CONNECT_N_TX,
	VDEV_GET_SSID,
	VDEV_GET_MAX_PHYRATE,
	VDEV_GET_ACL,
	VDEV_GET_ACL_RSSI_THRESHOLDS,
	VDEV_GET_NODE_CAP,
	VDEV_GET_WDS,
} wlan_mlme_vdev_param;

union wlan_mlme_vdev_data {
	/* Setting App IE */
	wlan_app_ie_data appie;
	/* ACL configuration */
	wlan_acl_data acl;
	/* ACL RSSI thresholds */
	wlan_rssi_threshold_data acl_rssi;
	/* struct mapapcap_t apcap; */
	wlan_ap_cap apcap;
	/* struct ieee80211_bsteering_datarate_info_t rate_info; */
	wlan_node_info nodeinfo;
	/* wlan_channel */
	wlan_chan chan;
	/* channel width */
	wlan_ch_width chan_width;
	/* ssid and length */
	wlan_ssid ssid;
	/* For generic use, send and get bool flag */
	u_int8_t flag : 1;
	/* For generic use, send MAC address */
	u_int8_t mac[IEEE80211_ADDR_LEN];
	/* Number of spatial streams */
	u_int8_t nss;
	/* Rate in kbps */
	u_int32_t rate;
	/* Channel utilization 0-255 */
	u_int32_t chan_util;
	/* WNM BSS preference */
	u_int8_t bss_pref;
};

union wlan_mlme_pdev_data {
	wlan_esp_data esp_info;
	int32_t desc_poolsize;
	u_int32_t phy_errcnt;
	wlan_radio_basic_capabilities cap;
	wlan_op_chan op_chan;
	wlan_op_class op_class;
	u_int8_t hmwds_en : 1;
	u_int8_t skip_6ghz;
};

enum ieee80211_event_type {
	MLME_EVENT_NONE,
	MLME_EVENT_ASSOC_DISASSOC,
	MLME_EVENT_CBS_STATUS,
	MLME_EVENT_ACS_COMPLETE,
	MLME_EVENT_CAC_STATUS,
	MLME_EVENT_ASSOC_ALLOWANCE_STATUS,
	MLME_EVENT_ALD_WNM_FRAME_RECVD,
	MLME_EVENT_ALD_ANQP_FRAME_RECVD,
	MLME_EVENT_TX_AUTH_FAIL,
	MLME_EVENT_TX_AUTH_ALLOW,
	MLME_EVENT_TX_ASSOC_FAIL,
	MLME_EVENT_CLIENT_ASSOCIATED,
	MLME_EVENT_CLIENT_DISCONNECTED,
	MLME_EVENT_VDEV_STATE,
	MLME_EVENT_INST_RSSI,
	MLME_EVENT_TX_PWR_CHANGE,
	MLME_EVENT_CHAN_CHANGE,
	MLME_EVENT_MSCS,
	MLME_EVENT_SCS,
	MLME_EVENT_DSCP_POLICY_QUERY,
	MLME_EVENT_DSCP_POLICY_RESPONSE,
};

union wlan_mlme_peer_data {
	/* peer capabiliy */
	wlan_peer_cap peercap;
	/* peer max mcs */
	u_int32_t mcs;
	/* generic flag to enable/disable */
	u_int8_t enable : 1;
	/* map ie vlan id */
	u_int16_t vlan_id;
};

typedef enum wlan_mlme_peer_param {
	PEER_SET_KICKOUT,
	PEER_SET_KICKOUT_ALLOW,
	PEER_SET_EXT_STATS,
	PEER_SET_VLAN_ID,
	PEER_REQ_INST_STAT,
	PEER_GET_CAPABILITY,
	PEER_GET_MAX_MCS,
} wlan_mlme_peer_param;

typedef struct _ieee80211_tspec_info {
	u_int8_t    traffic_type;
	u_int8_t    direction;
	u_int8_t    dot1Dtag;
	u_int8_t    tid;
	u_int8_t    acc_policy_edca;
	u_int8_t    acc_policy_hcca;
	u_int8_t    aggregation;
	u_int8_t    psb;
	u_int8_t    ack_policy;
	u_int16_t   norminal_msdu_size;
	u_int16_t   max_msdu_size;
	u_int32_t   min_srv_interval;
	u_int32_t   max_srv_interval;
	u_int32_t   inactivity_interval;
	u_int32_t   suspension_interval;
	u_int32_t   srv_start_time;
	u_int32_t   min_data_rate;
	u_int32_t   mean_data_rate;
	u_int32_t   peak_data_rate;
	u_int32_t   max_burst_size;
	u_int32_t   delay_bound;
	u_int32_t   min_phy_rate;
	u_int16_t   surplus_bw;
	u_int16_t   medium_time;
} ieee80211_tspec_info;

#define IEEE80211_MAX_QOS_UP_RANGE       8
#define IEEE80211_MAX_QOS_DSCP_EXCEPT    21

struct ieee80211_dscp_range {
	u_int8_t low;
	u_int8_t high;
};

struct ieee80211_dscp_exception {
	u_int8_t dscp;
	u_int8_t up;
};

struct ieee80211_qos_map {
	/* user priority map fields */
	struct ieee80211_dscp_range
	    up[IEEE80211_MAX_QOS_UP_RANGE];
	u_int16_t valid;
	/* count of dscp exception fields */
	u_int16_t num_dscp_except;
	/* dscp exception fields */
	struct ieee80211_dscp_exception
	    dscp_exception[IEEE80211_MAX_QOS_DSCP_EXCEPT];
};

/* NOTE: This macro corresponds to macro ACS_RANK_DESC_LEN, Please change it
 * as well if changing this.
 */
#define ACS_RANK_DESC_DBG_LEN 80
typedef enum {
	/* acs is not yet started */
	ACS_RUN_IDLE,
	/* acs run is incomplete/cancelled due to ICM or some other reason */
	ACS_RUN_INCOMPLETE,
	ACS_RUN_COMPLETED,
} ACS_RUN_STATUS;

typedef enum {
	    ACS_DEFAULT,
	    ACS_SUCCESS,
	    ACS_FAILED_NBSS,
	    ACS_FAILED_RNDM,
} ACS_STATUS;

/* ACS Channel Ranking structure
 * rank: Channel Rank
 * desc: Reason in case of no rank
 */
typedef struct acs_rank_dbg {
	u_int32_t rank;
	char desc[ACS_RANK_DESC_DBG_LEN];
} acs_rank_dbg_t;

typedef enum {
	ACS_CHAN_STATS,
	ACS_NEIGHBOUR_GET_LIST_COUNT,
	ACS_NEIGHBOUR_GET_LIST,
	ACS_CHAN_NF_STATS,
} ACS_LIST_TYPE;

/* ACS debug structure to
 * export report to user tool
 */
struct ieee80211_acs_dbg {
	u_int8_t  nchans;
	u_int8_t  entry_id;
	u_int16_t chan_freq;
	enum wlan_band_id chan_band;
	u_int8_t  ieee_chan;
	u_int8_t  chan_nbss;
	int32_t   chan_maxrssi;
	int32_t   chan_minrssi;
	int16_t   noisefloor;
	int16_t   perchain_nf[HOST_MAX_CHAINS];
	int16_t   channel_loading;
	u_int32_t chan_load;
	u_int8_t  sec_chan;
	int32_t   chan_nbss_srp;
	int32_t   chan_srp_load;
	u_int8_t  chan_in_pool;
	u_int8_t  chan_radar_noise;
	int32_t neighbor_size;
	ieee80211_neighbor_info *neighbor_list;
	u_int32_t chan_80211_b_duration;
	acs_rank_dbg_t acs_rank;     /* ACS Rank and channel reject code */
	u_int8_t  acs_status;
	ACS_LIST_TYPE acs_type;
	uint32_t chan_availability;
	uint32_t chan_efficiency;
	uint32_t chan_nbss_near;
	uint32_t chan_nbss_mid;
	uint32_t chan_nbss_far;
	uint32_t chan_nbss_eff;
	uint32_t chan_grade;
	u_int8_t op_class;
	u_int8_t chan_width;
};

struct wlan_cfg8011_genric_params {
	unsigned int command; /* attribute 17*/
	unsigned int value;   /* attriute 18 */
	void *data;          /* attribute 19 */
	unsigned int data_len;
	unsigned int length; /* attribute 20 */
	unsigned int flags;  /* attribute 21 */
};

struct son_ald_cac_info {
	u_int16_t freq;
	u_int8_t radar_detected;
};

struct son_ald_chan_change_info {
	u_int16_t freq;
	u_int8_t chan_num;
};

enum wlan_vdev_state_ext {
	VDEV_STATE_CREATED,
	VDEV_STATE_DELETED,
	VDEV_STATE_STOPPED,
	VDEV_STATE_RECOVER_CREATE,
	VDEV_STATE_RECOVER_DELETE,
	VDEV_STATE_CONNECTED,
	VDEV_STATE_DISCONNECTED,
	VDEV_STATE_WDS_ENABLED,
	VDEV_STATE_WDS_DISABLED,
};

struct wlan_vdev_state_event {
	enum wlan_vdev_state_ext state;
	uint8_t bssid[IEEE80211_ADDR_LEN];
};

struct wlan_peer_inst_rssi {
	u_int32_t iRSSI;
	u_int8_t valid : 1;
};

struct wlan_rrm_frm_data {
	u_int8_t macaddr[IEEE80211_ADDR_LEN];
	u_int8_t dialog_token;
	u_int8_t num_meas_rpts;
};

struct wlan_act_frm_info {
	struct ieee80211_action *ia;
	u_int32_t ald_info:1;
	union {
	    u_int8_t macaddr[IEEE80211_ADDR_LEN];
	    struct wlan_rrm_frm_data rrm_data;
	} data;
};

/*
 * Metadata about a probe request received from a client that is useful
 * for making band steering decisions.
 */
struct wlan_probe_req_ind {
	/* The MAC address of the client that sent the probe request.*/
	u_int8_t sender_addr[IEEE80211_ADDR_LEN];
	/*  The RSSI of the received probe request.*/
	u_int8_t rssi;
	/* Whether the probe response was blocked or not */
	u_int8_t blocked   : 1,
	/* Whether SSID of incoming probe-request is NULL */
		 ssid_null : 1;
};

struct wlan_probe_req_data {
	struct wlan_probe_req_ind probe_req;
	u_int8_t is_chan_2G;
};

struct wlan_assoc_disassoc_event_info {
	u_int8_t macaddr[IEEE80211_ADDR_LEN];
	u_int8_t flag;
	u_int16_t reason;
};

struct wlan_assoc_allow_event_info {
	u_int8_t bssid[IEEE80211_ADDR_LEN];
	u_int8_t status;
};

/*
 * Metadata about an authentication message that was sent with a failure
 * code due to the client being prohibited by the ACL.
 */
struct wlan_auth_reject_ind {
	u_int8_t client_addr[IEEE80211_ADDR_LEN];
	u_int8_t rssi;
	u_int8_t bs_blocked:1,
		 bs_rejected:1;
	u_int32_t reason;
};

/*
 * STA stats per peer
 */
struct wlan_sta_stats_per_peer {
	/* The MAC address of the client */
	u_int8_t client_addr[IEEE80211_ADDR_LEN];
	/* Uplink RSSI */
	u_int8_t rssi;
	/* PER */
	u_int8_t per;
	/* The Tx byte count */
	u_int64_t tx_byte_count;
	/* The Rx byte count */
	u_int64_t rx_byte_count;
	/* The Tx packet count */
	u_int32_t tx_packet_count;
	/* The Rx packet count */
	u_int32_t rx_packet_count;
	/* Number of packets that could be transmitted due to errors */
	u_int32_t tx_error_packets;
	/* Number of transmitted packets which were retransmissions */
	u_int32_t tx_retrans;
	/* Number of packets received from STA that were dropped due to
	 * errors
	 */
	u_int32_t rx_error_packets;
	/* The last Tx rate (in Kbps) */
	u_int32_t tx_rate;
	/* ack rssi */
	u_int32_t ack_rssi;
	/* BSSID of the BSS that the STA was associated to */
	u_int8_t bssid[IEEE80211_ADDR_LEN];
	/* Flag to indicate that above stats are for disconnected client */
	u_int8_t is_disassoc_stats;
};

/*
 * Metadata for STA stats
 */
struct wlan_sta_stats_ind {
	/* Number of peers for which stats are provided */
	u_int8_t peer_count;
	/* Stats per peer */
	struct wlan_sta_stats_per_peer
		peer_stats[BSTEERING_MAX_PEERS_PER_EVENT];
};

/*
 * Metadata about a client disconnect event
 */
struct wlan_disconnect_ind {
	u_int8_t client_addr[IEEE80211_ADDR_LEN];
	BSTEER_DISCONNECT_SOURCE source; /* Who initiated the disconnect */
	BSTEER_DISCONNECT_TYPE type; /* Type - deauth or disassoc */
	u_int32_t reason; /* reason code */
};

/*
 * Metadata for disconnect and STA stats events
 */
struct wlan_client_disconnect_ind {
	struct wlan_disconnect_ind disconnect_event_data;
	struct wlan_sta_stats_ind sta_stats_event_data;
};

struct wlan_beacon_frm_info {
	u_int8_t *se_sonadv;
	u_int8_t snr;
	u_int8_t nss;
};

#endif //_IEEE80211_H_
