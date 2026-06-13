/*
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2009 Atheros Communications Inc.  All rights reserved.
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
 * DOC : wlan_son_ald_external.h
 * This header file refers to the internal header files that provide the
 * data structure definitions and parameters required by external programs
 * that interface via ioctl or similar mechanisms.  This hides the location
 * of the specific header files, and provides a control to limit what is
 * being exported for external use.
 */

#ifndef _WLAN_SON_ALD_EXTERNAL_H_
#define _WLAN_SON_ALD_EXTRRNAL_H_

struct son_ald_assoc_event_info {
	u_int8_t macaddr[QDF_MAC_ADDR_SIZE];
	u_int8_t flag;
	u_int16_t reason;
};

struct son_rrm_frm_data {
	 u_int8_t macaddr[QDF_MAC_ADDR_SIZE];
	 u_int8_t dialog_token;
	 u_int8_t num_meas_rpts;
};

struct son_act_frm_info {
	struct ieee80211_action *ia;
	u_int32_t ald_info:1;
	union {
		u_int8_t macaddr[QDF_MAC_ADDR_SIZE];
		struct son_rrm_frm_data rrm_data;
	} data;
};

#endif // _WLAN_SON_ALD_EXTRRNAL_H_
