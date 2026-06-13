/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "wlan_ll_sap_api.h"
#include <../../core/src/wlan_ll_lt_sap_bearer_switch.h>
#include <../../core/src/wlan_ll_lt_sap_main.h>
#include "wlan_cm_api.h"
#include "wlan_policy_mgr_ll_sap.h"
#include "wlan_policy_mgr_api.h"
#include "wlan_reg_services_api.h"
#include "wlan_dfs_utils_api.h"
#include "wlan_mlme_vdev_mgr_interface.h"

#define SET_HT_MCS3(mcs) do { \
	mcs[0] = 0x0f;        \
	mcs[1] = 0x00;        \
	} while(0)

wlan_bs_req_id
wlan_ll_lt_sap_bearer_switch_get_id(struct wlan_objmgr_psoc *psoc)
{
	return ll_lt_sap_bearer_switch_get_id(psoc);
}

QDF_STATUS
wlan_ll_lt_sap_switch_bearer_to_ble(struct wlan_objmgr_psoc *psoc,
				struct wlan_bearer_switch_request *bs_request)
{
	return ll_lt_sap_switch_bearer_to_ble(psoc, bs_request);
}

void
wlan_ll_lt_sap_extract_ll_sap_cap(struct wlan_objmgr_psoc *psoc)
{
	ll_lt_sap_extract_ll_sap_cap(psoc);
}

QDF_STATUS
wlan_ll_lt_sap_switch_bearer_to_wlan(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_bearer_switch_request *bs_request)
{
	return ll_lt_sap_switch_bearer_to_wlan(psoc, bs_request);
}

static void
connect_start_bearer_switch_requester_cb(struct wlan_objmgr_psoc *psoc,
					 uint8_t vdev_id,
					 wlan_bs_req_id request_id,
					 QDF_STATUS status, uint32_t req_value,
					 void *request_params)
{
	wlan_cm_id cm_id = req_value;

	wlan_cm_bearer_switch_resp(psoc, vdev_id, cm_id, status);
}

QDF_STATUS
wlan_ll_sap_switch_bearer_on_sta_connect_start(struct wlan_objmgr_psoc *psoc,
					       qdf_list_t *scan_list,
					       uint8_t vdev_id,
					       wlan_cm_id cm_id)
{
	struct scan_cache_node *scan_node = NULL;
	qdf_list_node_t *cur_node = NULL, *next_node = NULL;
	uint32_t ch_freq = 0;
	struct scan_cache_entry *entry;
	struct wlan_objmgr_vdev *vdev;
	qdf_freq_t ll_lt_sap_freq;
	bool is_bearer_switch_required = false;
	QDF_STATUS status = QDF_STATUS_E_ALREADY;
	uint8_t ll_lt_sap_vdev_id;

	ll_lt_sap_vdev_id = wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);
	/* LL_LT SAP is not present, bearer switch is not required */
	if (ll_lt_sap_vdev_id == WLAN_INVALID_VDEV_ID)
		return status;
	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, ll_lt_sap_vdev_id,
						    WLAN_LL_SAP_ID);
	if (!vdev)
		return status;

	if (!scan_list || !qdf_list_size(scan_list))
		goto rel_ref;

	ll_lt_sap_freq = policy_mgr_get_ll_lt_sap_freq(psoc);
	qdf_list_peek_front(scan_list, &cur_node);

	while (cur_node) {
		qdf_list_peek_next(scan_list, cur_node, &next_node);

		scan_node = qdf_container_of(cur_node, struct scan_cache_node,
					     node);
		entry = scan_node->entry;
		ch_freq = entry->channel.chan_freq;

		/*
		 * Switch the bearer in case of SCC/MCC for LL_LT SAP
		 */
		if (policy_mgr_2_freq_always_on_same_mac(psoc, ch_freq,
							 ll_lt_sap_freq)) {
			ll_sap_debug("Scan list has BSS of freq %d on same mac with ll_lt sap %d",
				     ch_freq, ll_lt_sap_freq);
			is_bearer_switch_required = true;
			break;
		}

		ch_freq = 0;
		cur_node = next_node;
		next_node = NULL;
	}

	if (!is_bearer_switch_required)
		goto rel_ref;

	status = ll_lt_sap_switch_bearer(
				psoc, vdev_id, WLAN_BS_REQ_TO_NON_WLAN,
				BEARER_SWITCH_REQ_CONNECT,
				connect_start_bearer_switch_requester_cb,
				cm_id, NULL);
rel_ref:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_LL_SAP_ID);

	return status;
}

static void connect_complete_bearer_switch_requester_cb(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						wlan_bs_req_id request_id,
						QDF_STATUS status,
						uint32_t req_value,
						void *request_params)
{
	/* Drop this response as no action is required */
}

QDF_STATUS wlan_ll_sap_switch_bearer_on_sta_connect_complete(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id)
{
	QDF_STATUS status = QDF_STATUS_E_ALREADY;
	uint8_t ll_lt_sap_vdev_id;

	ll_lt_sap_vdev_id = wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);
	/* LL_LT SAP is not present, bearer switch is not required */
	if (ll_lt_sap_vdev_id == WLAN_INVALID_VDEV_ID)
		return status;

	status = ll_lt_sap_switch_bearer(
				psoc, vdev_id, WLAN_BS_REQ_TO_WLAN,
				BEARER_SWITCH_REQ_CONNECT,
				connect_complete_bearer_switch_requester_cb,
				0, NULL);

	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_ALREADY;

	return QDF_STATUS_SUCCESS;
}

static void ll_sap_csa_bearer_switch_requester_cb(
					struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id,
					wlan_bs_req_id request_id,
					QDF_STATUS status,
					uint32_t req_value,
					void *request_params)
{
	wlan_ll_sap_csa_bearer_switch_rsp(vdev_id);
}

QDF_STATUS wlan_ll_sap_switch_bearer_on_ll_sap_csa(
					struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id)
{
	return ll_lt_sap_switch_bearer(
				psoc, vdev_id, WLAN_BS_REQ_TO_NON_WLAN,
				BEARER_SWITCH_REQ_CSA,
				ll_sap_csa_bearer_switch_requester_cb,
				0, NULL);
}

static void wlan_ll_sap_csa_complete_bearer_switch_requester_cb(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						wlan_bs_req_id request_id,
						QDF_STATUS status,
						uint32_t req_value,
						void *request_params)
{
	/* Drop this response as no action is required */
}

QDF_STATUS wlan_ll_sap_switch_bearer_on_ll_sap_csa_complete(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id)
{
	return ll_lt_sap_switch_bearer(
			psoc, vdev_id, WLAN_BS_REQ_TO_WLAN,
			BEARER_SWITCH_REQ_CSA,
			wlan_ll_sap_csa_complete_bearer_switch_requester_cb,
			0, NULL);
}

static void wlan_ll_sap_stop_ap_bearer_switch_requester_cb(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						wlan_bs_req_id request_id,
						QDF_STATUS status,
						uint32_t req_value,
						void *request_params)
{
	/* Drop this response as no action is required */
}

QDF_STATUS wlan_ll_sap_switch_bearer_on_stop_ap(
					struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id)
{
	return ll_lt_sap_switch_bearer(
			psoc, vdev_id, WLAN_BS_REQ_TO_NON_WLAN,
			BEARER_SWITCH_REQ_STOP_AP,
			wlan_ll_sap_stop_ap_bearer_switch_requester_cb,
			0, NULL);
}

QDF_STATUS wlan_ll_lt_sap_get_freq_list(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_ll_lt_sap_freq_list *freq_list,
				uint8_t vdev_id,
				enum ll_sap_csa_source csa_src)
{
	return ll_lt_sap_get_freq_list(psoc, freq_list, vdev_id,
				       csa_src);
}

qdf_freq_t wlan_ll_lt_sap_override_freq(struct wlan_objmgr_psoc *psoc,
					uint32_t vdev_id,
					qdf_freq_t chan_freq)
{
	qdf_freq_t freq;

	if (!policy_mgr_is_vdev_ll_lt_sap(psoc, vdev_id))
		return chan_freq;

	if (wlan_reg_is_24ghz_ch_freq(chan_freq))
		goto get_valid_freq;
	/*
	 * If already any concurrent interface is present on this frequency,
	 * select a different frequency to start ll_lt_sap
	 */
	if (!policy_mgr_get_connection_count_with_ch_freq(chan_freq))
		return chan_freq;

get_valid_freq:
	freq = ll_lt_sap_get_valid_freq(psoc, vdev_id, 0,
					LL_SAP_CSA_CONCURENCY);

	ll_sap_debug("Vdev %d ll_lt_sap old freq %d new freq %d", vdev_id,
		     chan_freq, freq);

	return freq;
}

qdf_freq_t wlan_get_ll_lt_sap_restart_freq(struct wlan_objmgr_pdev *pdev,
					   qdf_freq_t chan_freq,
					   uint8_t vdev_id,
					   enum sap_csa_reason_code *csa_reason)
{
	struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(pdev);
	qdf_freq_t restart_freq;

	if (wlan_reg_is_disable_in_secondary_list_for_freq(pdev, chan_freq) &&
	    !utils_dfs_is_freq_in_nol(pdev, chan_freq)) {
		*csa_reason = CSA_REASON_CHAN_DISABLED;
		goto get_new_ll_lt_sap_freq;
	} else if (wlan_reg_is_passive_for_freq(pdev, chan_freq))  {
		*csa_reason = CSA_REASON_CHAN_PASSIVE;
		goto get_new_ll_lt_sap_freq;
	} else if (!policy_mgr_is_sap_freq_allowed(psoc,
				wlan_get_opmode_from_vdev_id(pdev, vdev_id),
				chan_freq)) {
		*csa_reason = CSA_REASON_UNSAFE_CHANNEL;
		goto get_new_ll_lt_sap_freq;
	} else if (policy_mgr_is_ll_lt_sap_restart_required(psoc)) {
		*csa_reason = CSA_REASON_CONCURRENT_STA_CHANGED_CHANNEL;
		goto get_new_ll_lt_sap_freq;
	}

	return chan_freq;

get_new_ll_lt_sap_freq:
	restart_freq = ll_lt_sap_get_valid_freq(psoc, vdev_id, 0,
						LL_SAP_CSA_CONCURENCY);

	ll_sap_debug("vdev %d old freq %d restart freq %d CSA reason %d ",
		     vdev_id, chan_freq, restart_freq, *csa_reason);
	return restart_freq;
}

static void fw_bearer_switch_requester_cb(struct wlan_objmgr_psoc *psoc,
					  uint8_t vdev_id,
					  wlan_bs_req_id request_id,
					  QDF_STATUS status,
					  uint32_t req_value,
					  void *request_params)
{
	/*
	 * Drop this response as all the responses to the FW is always
	 * forwarded
	 */
}

QDF_STATUS
wlan_ll_sap_fw_bearer_switch_req(struct wlan_objmgr_psoc *psoc,
				 enum bearer_switch_req_type req_type)
{
	struct wlan_bearer_switch_request bs_request = {0};
	QDF_STATUS status;
	uint8_t ll_lt_sap_vdev_id;

	ll_lt_sap_vdev_id = wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);

	bs_request.vdev_id = ll_lt_sap_vdev_id;
	bs_request.request_id = ll_lt_sap_bearer_switch_get_id(psoc);
	bs_request.req_type = req_type;
	bs_request.source = BEARER_SWITCH_REQ_FW;
	bs_request.requester_cb = fw_bearer_switch_requester_cb;

	if (req_type == WLAN_BS_REQ_TO_WLAN)
		status = ll_lt_sap_switch_bearer_to_wlan(psoc, &bs_request);

	else
		status = ll_lt_sap_switch_bearer_to_ble(psoc, &bs_request);

	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_ALREADY;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_ll_sap_oob_connect_response(
			struct wlan_objmgr_psoc *psoc,
			struct ll_sap_oob_connect_response_event rsp)
{
	struct ll_sap_ops *osif_cbk;
	struct wlan_objmgr_vdev *vdev;
	uint8_t i;
	struct ll_sap_vdev_priv_obj *ll_sap_obj;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, rsp.vdev_id,
						    WLAN_LL_SAP_ID);

	if (!vdev) {
		ll_sap_err("Invalid vdev %d for oob connect response",
			   rsp.vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

	if (!ll_sap_obj) {
		ll_sap_err("vdev %d ll_sap obj null",
			   wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_INVAL;
	}

	osif_cbk = ll_sap_get_osif_cbk();
	if (!osif_cbk || !osif_cbk->ll_sap_send_high_ap_availability_resp_cb) {
		ll_sap_err("invalid high ap availability ops");
		return QDF_STATUS_E_INVAL;
	}

	for (i = 0; i < MAX_HIGH_AP_AVAILABILITY_REQUESTS; i++) {
		/* Send response for all the valid cookies */
		if (ll_sap_obj->high_ap_availability_cookie[i] ==
							LL_SAP_INVALID_COOKIE)
			continue;

		osif_cbk->ll_sap_send_high_ap_availability_resp_cb(
				vdev,
				rsp.connect_resp_type,
				ll_sap_obj->high_ap_availability_cookie[i]);

		/* Reset the cookie once request is completed or cancelled */
		if (rsp.connect_resp_type ==
			HIGH_AP_AVAILABILITY_OPERATION_COMPLETED ||
		    rsp.connect_resp_type ==
			HIGH_AP_AVAILABILITY_OPERATION_CANCELLED)
			ll_sap_obj->high_ap_availability_cookie[i] =
							LL_SAP_INVALID_COOKIE;
	}

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LL_SAP_ID);

	return QDF_STATUS_SUCCESS;
}

void wlan_ll_lt_sap_get_mcs(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
			    uint8_t *mcs_set)

{
	if (!policy_mgr_is_vdev_ll_lt_sap(psoc, vdev_id))
		return;

	/* LL_LT_SAP supports upto MSC 3 only */
	SET_HT_MCS3(mcs_set);
}

uint64_t wlan_ll_sap_get_target_tsf(struct wlan_objmgr_vdev *vdev,
				    enum ll_sap_get_target_tsf get_tsf)
{
	struct ll_sap_vdev_priv_obj *ll_sap_obj;

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);
	if (!ll_sap_obj) {
		ll_sap_err("vdev %d ll_sap obj null", wlan_vdev_get_id(vdev));
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LL_SAP_ID);
		return QDF_STATUS_E_INVAL;
	}

	switch (get_tsf) {
	case TARGET_TSF_ECSA_ACTION_FRAME:
		return ll_sap_obj->target_tsf.twt_target_tsf;
	case TARGET_TSF_VDEV_RESTART:
		if (ll_sap_obj->target_tsf.twt_target_tsf)
			return ll_sap_obj->target_tsf.twt_target_tsf;
		else
			return ll_sap_obj->target_tsf.non_twt_target_tsf;
	case TARGET_TSF_GATT_MSG:
		return ll_sap_obj->target_tsf.non_twt_target_tsf;
	default:
		break;
	}

	return 0;
}

uint64_t
wlan_ll_sap_get_target_tsf_for_vdev_restart(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_objmgr_psoc *psoc;

	psoc = wlan_vdev_get_psoc(vdev);
	if (!psoc)
		return 0;

	/* send target_tsf as 0 for non ll_sap vdev */
	if (!policy_mgr_is_vdev_ll_lt_sap(psoc, wlan_vdev_get_id(vdev)))
		return 0;

	return wlan_ll_sap_get_target_tsf(vdev, TARGET_TSF_VDEV_RESTART);
}

QDF_STATUS wlan_ll_lt_sap_continue_csa_after_tsf_rsp(struct scheduler_msg *msg)
{
	if (!msg || !msg->bodyptr) {
		ll_sap_err("msg: 0x%pK", msg);
		return QDF_STATUS_E_NULL_VALUE;
	}

	ll_lt_sap_continue_csa_after_tsf_rsp(msg->bodyptr);
	qdf_mem_free(msg->bodyptr);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_ll_sap_reset_target_tsf_before_csa(
					struct wlan_objmgr_psoc *psoc,
					struct wlan_objmgr_vdev *vdev)
{
	struct ll_sap_vdev_priv_obj *ll_sap_vdev_obj;

	ll_sap_vdev_obj = ll_sap_get_vdev_priv_obj(vdev);
	if (!ll_sap_vdev_obj) {
		ll_sap_err("vdev %d ll_sap obj null", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_INVAL;
	}

	/* set below params as 0 before filling actual target tsf value to it */
	ll_sap_vdev_obj->target_tsf.twt_target_tsf = 0;
	ll_sap_vdev_obj->target_tsf.non_twt_target_tsf = 0;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_ll_sap_get_tsf_stats_before_csa(struct wlan_objmgr_psoc *psoc,
						struct wlan_objmgr_vdev *vdev)
{
	QDF_STATUS status;
	uint8_t vdev_id;

	if (!psoc || !vdev) {
		ll_sap_err("psoc or vdev is null");
		return QDF_STATUS_E_FAILURE;
	}

	vdev_id = wlan_vdev_get_id(vdev);

	status = ll_lt_sap_get_tsf_stats_for_csa(psoc, vdev_id);

	if (QDF_IS_STATUS_ERROR(status)) {
		ll_sap_err("vdev %d get next_sp_start_tsf and curr_tsf failed",
			   vdev_id);
		/*
		 * In failure case, update target_tsf with 0 which
		 * means immediate switch.
		 */
		wlan_ll_sap_notify_chan_switch_started(vdev);
		wlan_ll_sap_send_continue_vdev_restart(vdev);
	}

	return status;
}

bool wlan_ll_sap_is_bearer_switch_req_on_csa(struct wlan_objmgr_psoc *psoc,
					     uint8_t vdev_id)
{
	if (policy_mgr_is_vdev_ll_lt_sap(psoc, vdev_id) &&
	    ll_lt_sap_get_bearer_switch_cap_for_csa(psoc))
		return true;

	return false;
}

bool wlan_ll_lt_sap_is_freq_in_avoid_list(struct wlan_objmgr_psoc *psoc,
					  qdf_freq_t freq)
{
	struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj;

	ll_sap_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_LL_SAP);
	if (!ll_sap_psoc_obj) {
		ll_sap_err("psoc_ll_sap_obj is null");
		return false;
	}

	return ll_lt_sap_is_freq_in_avoid_list(ll_sap_psoc_obj, freq);
}

void wlan_ll_lt_store_to_avoid_list_and_flush_old(
					struct wlan_objmgr_psoc *psoc,
					qdf_freq_t freq,
					enum ll_sap_csa_source csa_src)
{
	ll_lt_store_to_avoid_list_and_flush_old(psoc, freq, csa_src);
}

qdf_freq_t
wlan_ll_sap_get_valid_freq_for_csa(struct wlan_objmgr_psoc *psoc,
				   uint8_t vdev_id, qdf_freq_t curr_freq,
				   enum ll_sap_csa_source csa_src)
{
	return ll_lt_sap_get_valid_freq(psoc, vdev_id, curr_freq, csa_src);
}

