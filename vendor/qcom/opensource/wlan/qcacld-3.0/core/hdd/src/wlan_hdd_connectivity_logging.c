/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: wlan_hdd_connectivity_logging.c
 *
 * Implementation for the Common connectivity and roam logging api.
 */

#include "wlan_hdd_connectivity_logging.h"

#define GET_ATTR_OFFSET(member) \
	qdf_offsetof(struct wlan_log_record, member)

#define ATTR_GET_VALUE(type, record, field_offset) \
	(*(type *)((uint8_t *)record + field_offset))

/**
 * struct connectivity_log_attr  - Connectivity logging attribute info
 * @attribute_id: Vendor attribute ID. Defined by enum qca_wlan_vendor_attr_diag
 * @attribute_type: NL type of the attribute
 * @attribute_length: Length of the attribute
 * @field_offset: Field offset
 */
struct connectivity_log_attr {
	enum qca_wlan_vendor_attr_diag attribute_id;
	uint8_t attribute_type;
	uint16_t attribute_length;
	uint16_t field_offset;
};

#ifdef WLAN_FEATURE_CONNECTIVITY_LOGGING
/**
 * wlan_hdd_send_connectivity_log_to_user  - Send the connectivity log buffer
 * to userspace
 * @rec: Pointer to the log record
 * @hdd_context: HDD global context
 * @num_records: Number of records
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
wlan_hdd_send_connectivity_log_to_user(struct wlan_log_record *rec,
				       void *hdd_context,
				       uint8_t num_records)
{
	struct hdd_context *hdd_ctx;
	struct nlattr *attr, *attr1;
	struct sk_buff *vendor_event;
	uint16_t len, i = 0;
	QDF_STATUS status;

	hdd_enter();

	hdd_ctx = hdd_context;
	if (wlan_hdd_validate_context(hdd_ctx))
		return QDF_STATUS_E_FAILURE;

	len = wlan_hdd_get_connectivity_log_event_len(rec, num_records);

	vendor_event = wlan_cfg80211_vendor_event_alloc(
			hdd_ctx->wiphy, NULL, len + NLMSG_HDRLEN,
			QCA_NL80211_VENDOR_SUBCMD_DIAG_EVENT_INDEX,
			GFP_ATOMIC);
	if (!vendor_event) {
		hdd_err("wlan_cfg80211_vendor_event_alloc failed");
		return QDF_STATUS_E_NOMEM;
	}

	attr = nla_nest_start(vendor_event,
			      QCA_WLAN_VENDOR_ATTR_DIAG_EVENT);
	if (!attr)
		goto failure;

	for (i = 0; i < num_records; i++) {
		attr1 = nla_nest_start(vendor_event, i);
		if (!attr1)
			goto failure;

		status = wlan_hdd_fill_connectivity_logging_data(vendor_event,
								 &rec[i]);
		if (QDF_IS_STATUS_ERROR(status)) {
			hdd_err_rl("Failed to fill fat idx:%d subtype:%d",
				   i, rec[i].log_subtype);
			goto failure;
		}

		nla_nest_end(vendor_event, attr1);
	}

	nla_nest_end(vendor_event, attr);
	wlan_cfg80211_vendor_event(vendor_event, GFP_ATOMIC);

	hdd_exit();

	return QDF_STATUS_SUCCESS;
failure:
	hdd_err("NLA fill failed num_records:%d", num_records);
	wlan_cfg80211_vendor_free_skb(vendor_event);

	return QDF_STATUS_E_FAILURE;
}

void wlan_hdd_start_connectivity_logging(struct hdd_context *hdd_ctx)
{
	struct wlan_cl_osif_cbks hdd_cb;

	hdd_cb.wlan_connectivity_log_send_to_usr =
			wlan_hdd_send_connectivity_log_to_user;
	wlan_connectivity_logging_start(hdd_ctx->psoc, &hdd_cb, hdd_ctx);
}
#endif

#ifdef CONNECTIVITY_DIAG_EVENT
static enum wlan_diag_connect_fail_reason
wlan_hdd_convert_con_fail_reason_to_diag_reason(
				enum wlan_cm_connect_fail_reason reason)
{
	switch (reason) {
	case CM_NO_CANDIDATE_FOUND:
		return WLAN_DIAG_NO_CANDIDATE_FOUND;
	case CM_ABORT_DUE_TO_NEW_REQ_RECVD:
		return WLAN_DIAG_ABORT_DUE_TO_NEW_REQ_RECVD;
	case CM_BSS_SELECT_IND_FAILED:
		return WLAN_DIAG_BSS_SELECT_IND_FAILED;
	case CM_PEER_CREATE_FAILED:
		return WLAN_DIAG_PEER_CREATE_FAILED;
	case CM_JOIN_FAILED:
		return WLAN_DIAG_JOIN_FAILED;
	case CM_JOIN_TIMEOUT:
		return WLAN_DIAG_JOIN_TIMEOUT;
	case CM_AUTH_FAILED:
		return WLAN_DIAG_AUTH_FAILED;
	case CM_AUTH_TIMEOUT:
		return WLAN_DIAG_AUTH_TIMEOUT;
	case CM_ASSOC_FAILED:
		return WLAN_DIAG_ASSOC_FAILED;
	case CM_ASSOC_TIMEOUT:
		return WLAN_DIAG_ASSOC_TIMEOUT;
	case CM_HW_MODE_FAILURE:
		return WLAN_DIAG_HW_MODE_FAILURE;
	case CM_SER_FAILURE:
		return WLAN_DIAG_SER_FAILURE;
	case CM_SER_TIMEOUT:
		return WLAN_DIAG_SER_TIMEOUT;
	case CM_GENERIC_FAILURE:
		return WLAN_DIAG_GENERIC_FAILURE;
	case CM_VALID_CANDIDATE_CHECK_FAIL:
		return WLAN_DIAG_VALID_CANDIDATE_CHECK_FAIL;
	default:
		hdd_err("Invalid connect fail reason code");
	}

	return WLAN_DIAG_UNSPECIFIC_REASON;
}

void
wlan_hdd_connectivity_fail_event(struct wlan_objmgr_vdev *vdev,
				 struct wlan_cm_connect_resp *rsp)
{
	WLAN_HOST_DIAG_EVENT_DEF(wlan_diag_event, struct wlan_diag_connect);

	qdf_mem_zero(&wlan_diag_event, sizeof(struct wlan_diag_connect));

	if (wlan_vdev_mlme_get_opmode(vdev) != QDF_STA_MODE)
		return;

	if (wlan_vdev_mlme_is_mlo_vdev(vdev) &&
	    (wlan_vdev_mlme_is_mlo_link_switch_in_progress(vdev) ||
	     wlan_vdev_mlme_is_mlo_link_vdev(vdev)))
		return;

	wlan_diag_event.diag_cmn.vdev_id = wlan_vdev_get_id(vdev);

	wlan_diag_event.diag_cmn.timestamp_us = qdf_get_time_of_the_day_us();
	wlan_diag_event.diag_cmn.ktime_us = qdf_ktime_to_us(qdf_ktime_get());
	wlan_diag_event.subtype = WLAN_CONN_DIAG_CONNECT_FAIL_EVENT;
	qdf_mem_copy(wlan_diag_event.diag_cmn.bssid, rsp->bssid.bytes,
		     QDF_MAC_ADDR_SIZE);

	wlan_diag_event.version = DIAG_CONN_VERSION;
	wlan_diag_event.freq = rsp->freq;
	wlan_diag_event.reason =
	wlan_hdd_convert_con_fail_reason_to_diag_reason(rsp->reason);

	WLAN_HOST_DIAG_EVENT_REPORT(&wlan_diag_event, EVENT_WLAN_CONN);
}
#endif
