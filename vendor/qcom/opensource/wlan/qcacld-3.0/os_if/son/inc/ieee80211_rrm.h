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

#ifndef _IEEE80211_RRM_H_
#define _IEEE80211_RRM_H_

#define IEEE80211_MAX_REQ_IE              255
#define IEEE80211_MAX_NR_COUNTRY_CODE 3
#define IEEE80211_MAX_NR_MCS_SET      16
#define IEEE80211_NR_RM_CAP_LEN 5
#define IEEE80211_NR_HTOP_LEN 5

#define IEEE80211_RRM_NUM_CHANREQ_MAX 16
#define IEEE80211_RRM_NUM_CHANREP_MAX 2
#define IEEE80211_MAX_VENDOR_OUI 5
#define IEEE80211_MAX_VENDOR_BUF 32
#define IEEE80211_NUM_REGCLASS 5

#define IEEE80211_RRM_MEASRPT_MODE_SUCCESS         0x00

struct ieee80211_beaconreq_chaninfo {
	u_int8_t regclass;
	u_int8_t numchans;
	u_int8_t channum[IEEE80211_RRM_NUM_CHANREQ_MAX];
};

struct ieee80211_beaconreq_wb_chan {
	u_int8_t chan_width;
	u_int8_t centerfreq0;
	u_int8_t centerfreq1;
} __packed;

struct ieee80211_beaconreq_vendor {
	u_int8_t oui[IEEE80211_MAX_VENDOR_OUI];
	u_int8_t buf[IEEE80211_MAX_VENDOR_BUF];
};

typedef struct ieee80211_rrm_beaconreq_info_s {
#define MAX_SSID_LEN 32
	u_int16_t   num_rpt;
	u_int8_t    num_regclass;
	u_int8_t    regclass[IEEE80211_NUM_REGCLASS];
	u_int8_t    channum;
	u_int16_t   random_ivl;
	u_int16_t   duration;
	u_int8_t    reqmode;
	u_int8_t    reqtype;
	u_int8_t    bssid[6];
	u_int8_t    mode;
	u_int8_t    req_ssid;
	u_int8_t    rep_cond;
	u_int8_t    rep_thresh;
	u_int8_t    rep_detail;
	u_int8_t    req_ie;
	u_int8_t    lastind;
	u_int8_t    num_chanrep;
	struct ieee80211_beaconreq_chaninfo
		apchanrep[IEEE80211_RRM_NUM_CHANREP_MAX];
	u_int8_t   ssidlen;
	u_int8_t   ssid[MAX_SSID_LEN];
	u_int8_t   req_ielen;
	u_int8_t   req_iebuf[IEEE80211_MAX_REQ_IE];
	u_int8_t   extreq_ielen;
	u_int8_t   extreq_ie[IEEE80211_MAX_REQ_IE];
	u_int8_t   req_extie;
	u_int8_t   req_bcnrpt_disabled;
	u_int8_t   req_rptdetail_disabled;
	u_int8_t   req_wbandchan;
	u_int8_t   req_vendor;
	u_int8_t   vendor_oui_len;
	u_int8_t   vendor_buf_len;
	struct     ieee80211_beaconreq_wb_chan wb_chan;
	struct     ieee80211_beaconreq_vendor vendor_info;
#undef MAX_SSID_LEN
} ieee80211_rrm_beaconreq_info_t;

/**
 * struct ieee80211_nr_resp_tsf - TSF information subelement
 * @tsf_offset: TSF Offset
 * @bcn_int:    beacon interval
 */
struct ieee80211_nr_resp_tsf {
	u_int16_t tsf_offset;
	u_int16_t bcn_int;
} __packed;

/**
 * struct ieee80211_nr_resp_country - Condensed Country String subelement
 * @country_string: Country code
 */
struct ieee80211_nr_resp_country {
	u_int8_t country_string[IEEE80211_MAX_NR_COUNTRY_CODE];
} __packed;

/**
 * struct ieee80211_nr_cand_pref - BSS Transition candidate Preference
 *   subelement
 * @preference: Network Preference
 */
struct ieee80211_nr_cand_pref {
	u_int8_t preference;
} __packed;

/**
 * struct ieee80211_nr_term_duration - BSS Termination Duration subelement
 * @tsf: TSF time of the BSS transmitting the neighbor report
 * @duration: Number of minutes for which BSS is not present
 */
struct ieee80211_nr_term_duration {
	u_int64_t tsf;
	u_int16_t duration;
} __packed;

/**
 * struct ieee80211_nr_bearing - Bearing subelement
 * @bearing:  Relative Direction of the neighbor
 * @distance: Relative Distance of the neighbor
 * @height:   Relative Height in meters
 *
 */
struct ieee80211_nr_bearing {
	u_int16_t bearing;
	u_int32_t distance;
	u_int16_t height;
} __packed;

/**
 * struct ieee80211_nr_htcap - HT capabilities subelement
 * @htcap_info: HT capability information
 * @ampdu_param: AMPDU parameters
 * @mcs: supported MCS set
 * @ht_extcap: HT extended capabilities
 * @txbeam_caps: Tx beamforming capabilities
 * @asel_caps: ASEL capabilities
 */
struct ieee80211_nr_htcap {
	u_int16_t htcap_info;
	u_int8_t ampdu_param;
	u_int8_t mcs[IEEE80211_MAX_NR_MCS_SET];
	u_int16_t ht_extcap;
	u_int32_t txbeam_caps;
	u_int8_t asel_caps;
} __packed;

/**
 * struct ieee80211_nr_vhtcap - VHT capabilities subelement
 * Supported VHT-MCS and NSS Set field is 64 bits long.
 * Breaking it to four, 16 bit fields.
 * @vhtcap_info:     VHT capabilities info
 * @mcs:             Supported VHT-MCS & NSS set
 * @rx_highest_rate: Supported VHT-MCS & NSS set
 * @tx_vht_mcs:      Supported VHT-MCS & NSS set
 * @tx_highest_rate: Supported VHT-MCS & NSS set
 */
struct ieee80211_nr_vhtcap {
	u_int32_t vhtcap_info;
	u_int16_t mcs;
	u_int16_t rx_highest_rate;
	u_int16_t tx_vht_mcs;
	u_int16_t tx_highest_rate;
} __packed;

/**
 * struct ieee80211_nr_vhtop - VHT Operation subelement
 * @vhtop_info: vht operation information
 * @vht_mcs_nss: Basic VHT-MCS & NSS set
 */
struct ieee80211_nr_vhtop {
	struct ieee80211_beaconreq_wb_chan vhtop_info;
	uint16_t vht_mcs_nss;
} __packed;

/**
 * struct ieee80211_nr_htop - HT Operation subelement
 * @chan:      Primary channel
 * @htop_info: HT operation information
 * @htop_mcs:  Basic HT MCS set
 */
struct ieee80211_nr_htop {
	u_int8_t chan;
	u_int8_t htop_info[IEEE80211_NR_HTOP_LEN];
	u_int8_t htop_mcs[IEEE80211_MAX_NR_MCS_SET];
} __packed;

/**
 * struct ieee80211_nr_meas_pilot - Measurement Pilot Transmission subelement
 * @pilot: Number of TUs between measurement pilot
 * @vendor: Subelement
 */
struct ieee80211_nr_meas_pilot {
	u_int8_t pilot;
	struct ieee80211_beaconreq_vendor vendor;
} __packed;

/**
 * struct ieee80211_nr_rm_en_caps - RM Enabled capabilities subelement
 * @rm_cap: RM Enabled capabilities
 */
struct ieee80211_nr_rm_en_caps {
	u_int8_t rm_cap[IEEE80211_NR_RM_CAP_LEN];
} __packed;

/**
 * struct ieee80211_nr_sec_chan - Secondary channel offset subelement
 * @sec_chan_offset: Position of sec. channel relative to primary
 */
struct ieee80211_nr_sec_chan {
	u_int8_t sec_chan_offset;
} __packed;

/**
 * struct ieee80211_nr_resp_info_s - Custom Neighbor report response
 * @bssid:            BSSID of the BSS being reported
 * @bssi_info:           BSSID Information field
 * @regclass:            Operating class
 * @channum:             Last known primary channel
 * @phy_type:            Phy type of AP indicated
 * @subie_pres:          Bitmap indicating presence of subelements
 * @pilot_vndroui_len:   Length of pilot vendor oui
 * @pilot_vndrbuf_len:   Length of pilot vendor buffer
 * @vendor_oui_len:      Length of vendor oui
 * @vendor_buf_len:      Length of vendor buffer
 * @nr_tsf:              TSF Information subelement   (optional)
 * @nr_country:          Condensed country string subelement (optional)
 * @nr_cand_pref:        BSS Transition candidate preference (optional)
 * @nr_term_duration:    BSS Termination duration (optional)
 * @nr_bearing:          Bearing subelement (optional)
 * @nr_wb_chan:          Wide bandwidth Channel (optional)
 * @nr_ht_cap:           HT Capabilities subelement (optional)
 * @nr_ht_op:            HT Operation subelement (optional)
 * @nr_sec_chan:         Secondary channel offset subelement (optional)
 * @nr_meas_pilot:       Measurement pilot Transmission  (optional)
 * @nr_rm_en_caps:       RM enabled capabilities  (optional)
 * @nr_vhtcaps:          VHT Capabilities (optional)
 * @nr_vht_op:           VHT Operation (optional)
 * @nr_vendor:           Vendor Specific (optional)
 */
typedef struct ieee80211_nr_resp_info_s {
	u_int8_t  bssid[6];
	u_int32_t bssi_info;
	u_int8_t  regclass;
	u_int8_t  channum;
	u_int8_t  phy_type;
	u_int32_t subie_pres;
	u_int8_t pilot_vndroui_len;
	u_int8_t pilot_vndrbuf_len;
	u_int8_t vendor_oui_len;
	u_int8_t vendor_buf_len;
	struct ieee80211_nr_resp_tsf  nr_tsf;
	struct ieee80211_nr_resp_country  nr_country;
	struct ieee80211_nr_cand_pref  nr_cand_pref;
	struct ieee80211_nr_term_duration nr_term_duration;
	struct ieee80211_nr_bearing nr_bearing;
	struct ieee80211_beaconreq_wb_chan nr_wb_chan;
	struct ieee80211_nr_htcap nr_ht_cap;
	struct ieee80211_nr_htop nr_ht_op;
	struct ieee80211_nr_sec_chan nr_sec_chan;
	struct ieee80211_nr_meas_pilot nr_meas_pilot;
	struct ieee80211_nr_rm_en_caps nr_rm_en_caps;
	struct ieee80211_nr_vhtcap nr_vhtcaps;
	struct ieee80211_nr_vhtop nr_vht_op;
	struct ieee80211_beaconreq_vendor nr_vendor;
} __packed ieee80211_cust_nrresp_info_t;

typedef struct ieee80211_user_nr_rep_r {
	u_int8_t num_report;
	u_int8_t  dialog_token;
	ieee80211_cust_nrresp_info_t *custom_nrresp_info;
} ieee80211_user_nrresp_info_t;

typedef struct ieee80211_rrm_cca_info_s {
	u_int16_t num_rpts;
	u_int8_t dstmac[6];
	u_int8_t chnum;
	u_int64_t tsf;
	u_int16_t m_dur;
} ieee80211_rrm_cca_info_t;

typedef struct ieee80211_rrm_rpihist_info_s {
	u_int16_t num_rpts;
	u_int8_t dstmac[6];
	u_int8_t chnum;
	u_int64_t tsf;
	u_int16_t m_dur;
} ieee80211_rrm_rpihist_info_t;

typedef struct ieee80211_rrm_chloadreq_info_s {
	u_int8_t dstmac[6];
	u_int16_t num_rpts;
	u_int8_t regclass;
	u_int8_t chnum;
	u_int16_t r_invl;
	u_int16_t m_dur;
	u_int8_t cond;
	u_int8_t c_val;
} ieee80211_rrm_chloadreq_info_t;

typedef struct ieee80211_rrm_nhist_info_s {
	u_int16_t num_rpts;
	u_int8_t dstmac[6];
	u_int8_t regclass;
	u_int8_t chnum;
	u_int16_t r_invl;
	u_int16_t m_dur;
	u_int8_t cond;
	u_int8_t c_val;
} ieee80211_rrm_nhist_info_t;

typedef struct ieee80211_rrm_frame_req_info_s {
	u_int8_t dstmac[6];
	u_int8_t peermac[6];
	u_int16_t num_rpts;
	u_int8_t regclass;
	u_int8_t chnum;
	u_int16_t r_invl;
	u_int16_t m_dur;
	u_int8_t ftype;
} ieee80211_rrm_frame_req_info_t;

typedef struct ieee80211_rrm_lcireq_info_s {
	u_int8_t dstmac[6];
	u_int16_t num_rpts;
	u_int8_t location;
	u_int8_t lat_res;
	u_int8_t long_res;
	u_int8_t alt_res;
	u_int8_t azi_res;
	u_int8_t azi_type;
} ieee80211_rrm_lcireq_info_t;

typedef struct ieee80211_rrm_stastats_info_s {
	u_int8_t dstmac[6];
	u_int16_t num_rpts;
	u_int16_t m_dur;
	u_int16_t r_invl;
	u_int8_t  gid;
} ieee80211_rrm_stastats_info_t;

typedef struct ieee80211_rrm_tsmreq_info_s {
	u_int16_t   num_rpt;
	u_int16_t   rand_ivl;
	u_int16_t   meas_dur;
	u_int8_t    reqmode;
	u_int8_t    reqtype;
	u_int8_t    tid;
	u_int8_t    macaddr[6];
	u_int8_t    bin0_range;
	u_int8_t    trig_cond;
	u_int8_t    avg_err_thresh;
	u_int8_t    cons_err_thresh;
	u_int8_t    delay_thresh;
	u_int8_t    meas_count;
	u_int8_t    trig_timeout;
} ieee80211_rrm_tsmreq_info_t;

typedef struct ieee80211_rrm_nrreq_info_s {
	u_int8_t dialogtoken;
	u_int8_t ssid[32];
	u_int8_t ssid_len;
	u_int8_t *essid;
	u_int8_t essid_len;
	u_int8_t meas_count; /* Request for LCI/LCR may come in any order */
	u_int8_t meas_token[2];
	u_int8_t meas_req_mode[2];
	u_int8_t meas_type[2];
	u_int8_t loc_sub[2];
} ieee80211_rrm_nrreq_info_t;

typedef struct ieee80211req_rrmstats_s {
	u_int32_t index;
	u_int32_t data_size;
	void *data_addr;
} ieee80211req_rrmstats_t;

/*
 * Radio Measurement capabilities (from RM Enabled Capabilities element)
 * IEEE Std 802.11-2016, 9.4.2.45, Table 9-157
 * byte 1 (out of 5)
 */
#define IEEE80211_RRM_CAPS_BEACON_REPORT_PASSIVE	BIT(4)
#define IEEE80211_RRM_CAPS_BEACON_REPORT_ACTIVE		BIT(5)
#endif //__IEEE80211_RRM_H_
