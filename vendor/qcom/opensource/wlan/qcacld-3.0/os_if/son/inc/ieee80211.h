/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _IEEE80211_H_
#define _IEEE80211_H_

#include "_ieee80211.h"

#define RRM_CAPS_LEN 5

#define HE_PPET16_PPET8_SIZE                            (8)

#define HEHANDLE_CAP_PHYINFO_SIZE       3
#define HEHANDLE_CAP_TXRX_MCS_NSS_SIZE  3

#define WME_NUM_AC              4       /* 4 AC categories */

#define IEEE80211_FC0_TYPE_DATA             0x08
#define IEEE80211_FC0_TYPE_MASK             0x0c
/* for TYPE_MGT */
#define IEEE80211_FC0_SUBTYPE_ASSOC_REQ     0x00
#define IEEE80211_FC0_SUBTYPE_ASSOC_RESP    0x10
#define IEEE80211_FC0_SUBTYPE_REASSOC_REQ   0x20
#define IEEE80211_FC0_SUBTYPE_REASSOC_RESP  0x30
#define IEEE80211_FC0_SUBTYPE_PROBE_REQ     0x40
#define IEEE80211_FC0_SUBTYPE_BEACON        0x80
#define IEEE80211_FC0_SUBTYPE_ACTION        0xd0
#define IEEE80211_FCO_SUBTYPE_ACTION_NO_ACK 0xe0
#define IEEE80211_FC0_SUBTYPE_MASK          0xf0

#if SUPPORT_11AX_D3
#define HECAP_PHYINFO_SIZE              11
#define HECAP_MACINFO_SIZE              8
#define HECAP_IE_MACINFO_SIZE           6
#else
#define HECAP_PHYINFO_SIZE              9
#define HECAP_MACINFO_SIZE              7
#define HECAP_IE_MACINFO_SIZE           5
#endif

#define HECAP_TXRX_MCS_NSS_SIZE         12
#define HECAP_PPET16_PPET8_MAX_SIZE     25

#ifdef WLAN_FEATURE_11BE
#define EHTHANDLE_CAP_PHYINFO_SIZE       2
#define EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE  3
#define EHTCAP_PHYINFO_SIZE              8
#define EHTCAP_MACINFO_SIZE
#endif

#define IEEE80211_SRP_SRCTRL_OBSS_PD_DISALLOWED_MASK      0x02 /* B1 */

/* is 802.11 address multicast/broadcast? */
#define IEEE80211_IS_MULTICAST(_a)  (*(_a) & 0x01)

#define QCA_OUI_WHC_REPT_INFO_SUBTYPE   0x00
#define QCA_OUI_WHC_AP_INFO_VERSION     0x01
#define QCA_OUI_WHC_AP_INFO_CAP_WDS     0x01
#define QCA_OUI_WHC_AP_INFO_CAP_SON     0x02

#define QCA_OUI_WHC_REPT_INFO_VERSION   0x00

/* HT capability flags */
#define IEEE80211_HTCAP_C_CHWIDTH40             0x0002
#define IEEE80211_HTCAP_C_SHORTGI20             0x0020
#define IEEE80211_HTCAP_C_SHORTGI40             0x0040

/* VHT capability flags */
/* B2-B3 Supported Channel Width */
#define IEEE80211_VHTCAP_SUP_CHAN_WIDTH_160    0x00000004 /* Supports 160 */
/* Support both 160 or 80+80 */
#define IEEE80211_VHTCAP_SUP_CHAN_WIDTH_80_160 0x00000008

/* B5 Short GI for 80MHz */
#define IEEE80211_VHTCAP_SHORTGI_80          0x00000020
/* B6 Short GI for 160 and 80+80 MHz */
#define IEEE80211_VHTCAP_SHORTGI_160	     0x00000040
/* B11 SU Beam former capable */
#define IEEE80211_VHTCAP_SU_BFORMER          0x00000800
/* B19 MU Beam Former */
#define IEEE80211_VHTCAP_MU_BFORMER          0x00080000

/*
 * 11AX TODO (Phase II) . Width parsing needs to be
 * revisited for addressing grey areas in width field
 */
#define IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE160                          0x4
#define IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE80_80                        0x8

/* categories */
#define IEEE80211_ACTION_CAT_PUBLIC               4   /* Public Action Frame */
#define IEEE80211_ACTION_CAT_RADIO                5   /* Radio management */
/* Protected Dual of public action frame */
#define IEEE80211_ACTION_CAT_PROT_DUAL            9

/* GAS initial request action frame identifier */
#define IEEE80211_ACTION_GAS_INITIAL_REQUEST 10

#define BITS_IN_A_BYTE 8

#define IEEE80211_IS_BROADCAST(_a)	          \
	((_a)[0] == 0xff &&                         \
	 (_a)[1] == 0xff &&                         \
	 (_a)[2] == 0xff &&                         \
	 (_a)[3] == 0xff &&                         \
	 (_a)[4] == 0xff &&                         \
	 (_a)[5] == 0xff)

/*
 * Reason codes
 *
 * Unlisted codes are reserved
 */

enum {
	IEEE80211_REASON_UNSPECIFIED        = 1,
	IEEE80211_REASON_AUTH_EXPIRE        = 2,
	IEEE80211_REASON_AUTH_LEAVE         = 3,
	IEEE80211_REASON_ASSOC_EXPIRE       = 4,
	IEEE80211_REASON_ASSOC_TOOMANY      = 5,
	IEEE80211_REASON_NOT_AUTHED         = 6,
	IEEE80211_REASON_NOT_ASSOCED        = 7,
	IEEE80211_REASON_ASSOC_LEAVE        = 8,
	IEEE80211_REASON_ASSOC_NOT_AUTHED   = 9,

	IEEE80211_REASON_ASSOC_BSSTM        = 12,
	IEEE80211_REASON_IE_INVALID         = 13,
	IEEE80211_REASON_MIC_FAILURE        = 14,
	IEEE80211_REASON_KICK_OUT           = 15,
	IEEE80211_REASON_INVALID_GROUP_CIPHER = 18,
	IEEE80211_REASON_INVALID_PAIRWISE_CIPHER = 19,
	IEEE80211_REASON_INVALID_AKMP         = 20,
	IEEE80211_REASON_UNSUPPORTED_RSNE_VER = 21,
	IEEE80211_REASON_RSN_REQUIRED         = 22,

	IEEE80211_REASON_QOS                = 32,
	IEEE80211_REASON_QOS_BANDWITDH      = 33,
	IEEE80211_REASON_DISASSOC_LOW_ACK  = 34,
	IEEE80211_REASON_QOS_TXOP           = 35,
	IEEE80211_REASON_QOS_LEAVE          = 36,
	IEEE80211_REASON_QOS_DECLINED       = 37,
	IEEE80211_REASON_QOS_SETUP_REQUIRED = 38,
	IEEE80211_REASON_QOS_TIMEOUT        = 39,
	IEEE80211_REASON_QOS_CIPHER         = 45,

	IEEE80211_STATUS_SUCCESS            = 0,
	IEEE80211_STATUS_UNSPECIFIED        = 1,
	IEEE80211_STATUS_CAPINFO            = 10,
	IEEE80211_STATUS_NOT_ASSOCED        = 11,
	IEEE80211_STATUS_OTHER              = 12,
	IEEE80211_STATUS_ALG                = 13,
	IEEE80211_STATUS_SEQUENCE           = 14,
	IEEE80211_STATUS_CHALLENGE          = 15,
	IEEE80211_STATUS_TIMEOUT            = 16,
	IEEE80211_STATUS_TOOMANY            = 17,
	IEEE80211_STATUS_BASIC_RATE         = 18,
	IEEE80211_STATUS_SP_REQUIRED        = 19,
	IEEE80211_STATUS_PBCC_REQUIRED      = 20,
	IEEE80211_STATUS_CA_REQUIRED        = 21,
	IEEE80211_STATUS_TOO_MANY_STATIONS  = 22,
	IEEE80211_STATUS_RATES              = 23,
	IEEE80211_STATUS_SHORTSLOT_REQUIRED = 25,
	IEEE80211_STATUS_DSSSOFDM_REQUIRED  = 26,
	IEEE80211_STATUS_NO_HT              = 27,
	IEEE80211_STATUS_REJECT_TEMP        = 30,
	IEEE80211_STATUS_MFP_VIOLATION      = 31,
	IEEE80211_STATUS_POOR_CHAN_CONDITION = 34,
	IEEE80211_STATUS_REFUSED            = 37,
	IEEE80211_STATUS_INVALID_PARAM      = 38,
	IEEE80211_STATUS_INVALID_ELEMENT    = 40,
	IEEE80211_STATUS_INVALID_GROUP_CIPHER = 41,
	IEEE80211_STATUS_INVALID_PAIRWISE_CIPHER = 42,
	IEEE80211_STATUS_INVALID_AKMP         = 43,
	IEEE80211_STATUS_UNSUPPORTED_RSNE_VER = 44,
	IEEE80211_STATUS_INVALID_RSNE_CAP     = 45,

	IEEE80211_STATUS_DLS_NOT_ALLOWED    = 48,
	IEEE80211_STATUS_INVALID_PMKID         = 53,
	IEEE80211_STATUS_ANTI_CLOGGING_TOKEN_REQ = 76,
	IEEE80211_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED = 77,
	IEEE80211_STATUS_NO_VHT             = 104,
	IEEE80211_STATUS_RADAR_DETECTED     = 105,
};

/*
 * Management information element payloads.
 */
enum {
	IEEE80211_ELEMID_VENDOR           = 221,  /* vendor private */
};

/*
 * Management Action Frames
 */

/* generic frame format */
struct ieee80211_action {
	u_int8_t    ia_category;
	u_int8_t    ia_action;
} __packed;

struct ieee80211_mscs_resp {
	  u_int8_t mscs_dialog_token;
	  u_int16_t status_code;
} __packed;

#if defined(WLAN_SUPPORT_DAP) && (WLAN_SUPPORT_DAP != 0)
#define IEEE80211_DSCP_ACTION_MAX_POLICIES 6

struct qos_mgmt_policy_attr {
	uint8_t attr_id;
	uint8_t len;
	uint8_t policy_id;
	uint8_t req_type;
	uint8_t dscp;
} __packed;

struct qos_mgmt_domain_attr {
	uint8_t attr_id;
	uint8_t len;
	char buf[30];
} __packed;

struct qos_mgmt_port_attr {
	uint8_t attr_id;
	uint8_t len;
	uint16_t s_port;
	uint16_t e_port;
} __packed;

struct ieee80211_qos_mgmt_attr {
	uint8_t elemid;
	uint8_t len;
	uint8_t tclas_attr_present;
	uint8_t domain_name_present;
	uint8_t port_range_present;
	uint8_t cap_attr;
	struct qos_mgmt_policy_attr policy_attr;
	struct tclas_element tclas;
	struct qos_mgmt_domain_attr domain_name;
	struct qos_mgmt_port_attr port_range;
} __packed;

struct ieee80211_qos_req {
	uint8_t no_of_policies;
	uint8_t reset;
	uint8_t policy_id_list[IEEE80211_DSCP_ACTION_MAX_POLICIES];
} __packed;
#endif

/*
 * Station information block; the mac address is used
 * to retrieve other data like stats, unicast key, etc.
 */
struct ieee80211req_sta_info {
	u_int16_t       isi_len;                /* length (mult of 4) */
	u_int16_t       isi_freq;               /* MHz */
	int32_t         isi_nf;                 /* noise floor */
	u_int16_t       isi_ieee;               /* IEEE channel number */
	u_int32_t       awake_time;             /* time is active mode */
	u_int32_t       ps_time;                /* time in power save mode */
	u_int64_t       isi_flags;              /* channel flags */
	u_int16_t       isi_state;              /* state flags */
	u_int8_t        isi_authmode;           /* authentication algorithm */
	int8_t          isi_rssi;
	int8_t          isi_min_rssi;
	int8_t          isi_max_rssi;
	u_int16_t       isi_capinfo;            /* capabilities */
	u_int16_t       isi_pwrcapinfo;         /* power capabilities */
	u_int8_t        isi_athflags;           /* Atheros capabilities */
	u_int8_t        isi_erp;                /* ERP element */
	u_int8_t    isi_ps;         /* psmode */
	u_int8_t        isi_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t        isi_nrates;
			/* negotiated rates */
	u_int8_t        isi_rates[IEEE80211_RATE_MAXSIZE];
	u_int8_t        isi_txrate;             /* index to isi_rates[] */
	u_int32_t   isi_txratekbps; /* tx rate in Kbps, for 11n */
	u_int16_t       isi_ie_len;             /* IE length */
	u_int16_t       isi_associd;            /* assoc response */
	u_int16_t       isi_txpower;            /* current tx power */
	u_int16_t       isi_vlan;               /* vlan tag */
	u_int16_t       isi_txseqs[17];         /* seq to be transmitted */
	u_int16_t       isi_rxseqs[17];         /* seq previous for qos frames*/
	u_int16_t       isi_inact;              /* inactivity timer */
	u_int8_t        isi_uapsd;              /* UAPSD queues */
	u_int8_t        isi_opmode;             /* sta operating mode */
	u_int8_t        isi_cipher;
	u_int32_t       isi_assoc_time;         /* sta association time */
	/* sta association time in timespec format */
	struct timespec isi_tr069_assoc_time;

	u_int16_t   isi_htcap;      /* HT capabilities */
	u_int32_t   isi_rxratekbps; /* rx rate in Kbps */
	/* We use this as a common variable for legacy rates
	 * and lln. We do not attempt to make it symmetrical
	 * to isi_txratekbps and isi_txrate, which seem to be
	 * separate due to legacy code.
	 */
	/* XXX frag state? */
	/* variable length IE data */
	u_int32_t isi_maxrate_per_client; /* Max rate per client */
	u_int16_t   isi_stamode;        /* Wireless mode for connected sta */
	u_int32_t isi_ext_cap;              /* Extended capabilities */
	u_int32_t isi_ext_cap2;              /* Extended capabilities 2 */
	u_int32_t isi_ext_cap3;              /* Extended capabilities 3 */
	u_int32_t isi_ext_cap4;              /* Extended capabilities 4 */
	u_int8_t isi_nss;         /* number of tx and rx chains */
	u_int8_t isi_supp_nss;    /* number of tx and rx chains supported */
	u_int8_t isi_is_256qam;    /* 256 QAM support */
	u_int8_t isi_operating_bands : 2; /* Operating bands */
#if defined(ATH_SUPPORT_EXT_STAT) && (ATH_SUPPORT_EXT_STAT != 0)
	u_int8_t  isi_chwidth;            /* communication band width */
	u_int32_t isi_vhtcap;             /* VHT capabilities */
#endif
#if defined(ATH_EXTRA_RATE_INFO_STA) && (ATH_EXTRA_RATE_INFO_STA != 0)
	u_int8_t isi_tx_rate_mcs;
	u_int8_t isi_tx_rate_flags;
	u_int8_t isi_rx_rate_mcs;
	u_int8_t isi_rx_rate_flags;
#endif
	u_int8_t isi_rrm_caps[RRM_CAPS_LEN];    /* RRM capabilities */
	u_int8_t isi_curr_op_class;
	u_int8_t isi_num_of_supp_class;
	u_int8_t isi_supp_class[MAX_NUM_OPCLASS_SUPPORTED];
	u_int8_t isi_nr_channels;
	u_int8_t isi_first_channel;
	u_int16_t isi_curr_mode;
	u_int8_t isi_beacon_measurement_support;
	enum wlan_band_id isi_band; /* band info: 2G,5G,6G */
	u_int8_t isi_is_he;
	u_int16_t isi_hecap_rxmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	u_int16_t isi_hecap_txmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	u_int32_t isi_hecap_phyinfo[HEHANDLE_CAP_PHYINFO_SIZE];
	u_int8_t isi_he_mu_capable;
#if defined(WLAN_FEATURE_11BE) && (WLAN_FEATURE_11BE != 0)
	u_int8_t isi_is_eht;
	u_int16_t isi_ehtcap_rxmcsnssmap[EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	u_int16_t isi_ehtcap_txmcsnssmap[EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	u_int32_t isi_ehtcap_phyinfo[EHTHANDLE_CAP_PHYINFO_SIZE];
#endif /* WLAN_FEATURE_11BE */
};

#define HECAP_TXRX_MCS_NSS_IDX7  7

#define HECAP_PHYBYTE_IDX0      0
#define HECAP_PHYBYTE_IDX7      7

#define HECAP_PHY_SRP_SR_BITS                         1
/* Max TX power elements supported */
#define IEEE80211_TPE_NUM_POWER_SUPPORTED         8
/* Spatial Multiplexing (SM) capabitlity bitmask */
#define IEEE80211_HTCAP_C_SM_MASK                0x000c
/* Capable of SM Power Save (Static) */
#define IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC    0x0000

struct ieee80211_ie_srp_extie {
	u_int8_t srp_id;
	u_int8_t srp_len;
	u_int8_t srp_id_extn;
	u_int8_t sr_control;
	union {
		struct {
			u_int8_t non_srg_obsspd_max_offset;
			u_int8_t srg_obss_pd_min_offset;
			u_int8_t srg_obss_pd_max_offset;
			u_int8_t srg_bss_color_bitmap[8];
			u_int8_t srg_partial_bssid_bitmap[8];
		} __packed nonsrg_srg_info;
		struct {
			u_int8_t non_srg_obsspd_max_offset;
		} __packed nonsrg_info;
		struct {
			u_int8_t srg_obss_pd_min_offset;
			u_int8_t srg_obss_pd_max_offset;
			u_int8_t srg_bss_color_bitmap[8];
			u_int8_t srg_partial_bssid_bitmap[8];
		} __packed srg_info;
	};
} __packed;

/*
 * 802.11ax HE Capability
 * with cabability & PPE (PPDU packet Extension) threshold
 */
struct ieee80211_ie_hecap {
	u_int8_t elem_id;
	u_int8_t elem_len;
	u_int8_t elem_id_ext;
	u_int8_t hecap_macinfo[HECAP_IE_MACINFO_SIZE];
	u_int8_t hecap_phyinfo[HECAP_PHYINFO_SIZE];
	u_int8_t hecap_txrx[HECAP_TXRX_MCS_NSS_SIZE];
	u_int8_t hecap_ppet[HECAP_PPET16_PPET8_MAX_SIZE];
} __packed;

struct ieee80211_tpe_payload {
	/* Max Tx Power Information field:
	 *     B0-B2     B3-B5     B6-B7
	 *  -------------------------------
	 *  |   Max   |   Max   |   Max   |
	 *  |  Tx Pwr |  Tx Pwr |  Tx Pwr |
	 *  |  Count  |   Intr  |   Cat   |
	 *  -------------------------------
	 *  <----------1 octet------------>
	 */
	u_int8_t tpe_info_cnt:3,     /* Transmit Power Information: Count */
		 tpe_info_intrpt:3,  /* Transmit Power Information: Interpret */
		 tpe_info_cat:2;     /* Transmit Power Information: Category */
	/* Local Max TxPower for 20,40,80,160MHz */
	u_int8_t local_max_txpwr[IEEE80211_TPE_NUM_POWER_SUPPORTED];
} __packed;

struct ieee80211_tpe_ie_config {
	struct ieee80211_tpe_payload tpe_payload;
	u_int8_t    tpe_val_len; /* Number of Tx Pwr values from user config */
} __packed;

/*
 * generic definitions for IEEE 802.11 frames
 */
struct ieee80211_frame {
	u_int8_t    i_fc[2];
	u_int8_t    i_dur[2];
	union {
		struct {
			u_int8_t    i_addr1[IEEE80211_ADDR_LEN];
		u_int8_t    i_addr2[IEEE80211_ADDR_LEN];
		u_int8_t    i_addr3[IEEE80211_ADDR_LEN];
		};
		u_int8_t    i_addr_all[3 * IEEE80211_ADDR_LEN];
	};
	u_int8_t    i_seq[2];
	/* possibly followed by addr4[IEEE80211_ADDR_LEN]; */
	/* see below */
} __packed;

/*
 * QCA Whole Home Coverage Rept Info information element.
 */
struct ieee80211_ie_whc_rept_info {
	u_int8_t    whc_rept_info_id;           /* IEEE80211_ELEMID_VENDOR */
	u_int8_t    whc_rept_info_len;          /* length in bytes */
	u_int8_t    whc_rept_info_oui[3];       /* 0x8c, 0xfd, 0xf0 */
	u_int8_t    whc_rept_info_type;
	u_int8_t    whc_rept_info_subtype;
	u_int8_t    whc_rept_info_version;
} __packed;

#endif //_IEEE80211_H_
