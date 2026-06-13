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

#ifndef _IEEE80211_WNM_PROTO_H_
#define _IEEE80211_WNM_PROTO_H_

#define IEEE80211_IPV4_LEN 4
#define IEEE80211_IPV6_LEN 16

#define MAC_ADDR_LEN 6

/* categories */
#define IEEE80211_ACTION_CAT_WNM    10   /* Wireless Network Management */

struct tclas_type3 {
	u_int16_t filter_offset;
	u_int8_t  *filter_value;
	u_int8_t  *filter_mask;
	u_int8_t  filter_len;
} __packed;

struct tclas_type10 {
	u_int8_t prot_instance;
	u_int8_t protocol;
	u_int8_t filter_len;
	u_int8_t *filter_value;
	u_int8_t *filter_mask;
} __packed;

struct tclas_type14_v4 {
	u_int8_t  version;
	u_int8_t  src_ip[IEEE80211_IPV4_LEN];
	u_int8_t  dst_ip[IEEE80211_IPV4_LEN];
	u_int16_t src_port;
	u_int16_t dst_port;
	u_int8_t  dscp;
	u_int8_t  protocol;
	u_int8_t  reserved;
} __packed;

struct tclas_type14_v6 {
	u_int8_t version;
	u_int8_t  src_ip[IEEE80211_IPV6_LEN];
	u_int8_t  dst_ip[IEEE80211_IPV6_LEN];
	u_int16_t src_port;
	u_int16_t dst_port;
	u_int8_t  type4_dscp;
	u_int8_t  type4_next_header;
	u_int8_t  flow_label[3];
} __packed;

typedef struct tclas_element {
	u_int8_t  elemid;
	u_int8_t  len;
	u_int8_t  up;
	u_int8_t  type;
	u_int8_t  mask;
	union {
		union {
			struct tclas_type14_v4  type14_v4;
			struct tclas_type14_v6  type14_v6;
		} type14;
		struct tclas_type3  type3;
		struct tclas_type10 type10;
	} tclas;
	TAILQ_ENTRY(tclas_element) tclas_next;
} ieee80211_tclas_element;

#define ieee80211_bstm_req_max_optie 10
struct ieee80211_bstm_reqinfo {
	u_int8_t dialogtoken;
	u_int8_t candidate_list;
	u_int8_t disassoc;
	u_int16_t disassoc_timer;
	u_int8_t validity_itvl;
	u_int8_t bssterm_inc;
	u_int64_t bssterm_tsf;
	u_int16_t bssterm_duration;
	u_int8_t abridged;
	u_int8_t optie_len;
	u_int8_t optie_buf[ieee80211_bstm_req_max_optie];
} __packed;

/* candidate BSSID for BSS Transition Management Request */
struct ieee80211_bstm_candidate {
	/* candidate BSSID */
	u_int8_t bssid[MAC_ADDR_LEN];
	/* channel number for the candidate BSSID */
	u_int8_t channel_number;
	/* preference from 1-255 (higher number = higher preference) */
	u_int8_t preference;
	/* operating class */
	u_int8_t op_class;
	/* PHY type */
	u_int8_t phy_type;
} __packed;

/*
 * maximum number of candidates in the list.  Value 3 was selected based on a
 * 2 AP network, so may be increased if needed
 */
#define ieee80211_bstm_req_max_candidates 3
/*
 * BSS Transition Management Request information that can be specified via
 * ioctl
 */
struct ieee80211_bstm_reqinfo_target {
	u_int8_t dialogtoken;
	u_int8_t num_candidates;
#if defined(QCN_IE) && (QCN_IE != 0)
	/*If we use trans reason code in QCN IE */
	u_int8_t qcn_trans_reason;
#endif
	struct ieee80211_bstm_candidate
		candidates[ieee80211_bstm_req_max_candidates];
	u_int8_t trans_reason;
	u_int8_t disassoc;
	u_int16_t disassoc_timer;
} __packed;

/*
 * When user, via "wifitool athX setbssidpref mac_addr pref_val"
 * command enters a bssid with preference value needed to
 * be assigned to that bssid, it is stored in a structure
 * as given below
 */
struct ieee80211_user_bssid_pref {
	u_int8_t bssid[MAC_ADDR_LEN];
	u_int8_t pref_val;
	u_int8_t regclass;
	u_int8_t chan;
} __packed;

#endif //_IEEE80211_WNM_PROTO_H_
