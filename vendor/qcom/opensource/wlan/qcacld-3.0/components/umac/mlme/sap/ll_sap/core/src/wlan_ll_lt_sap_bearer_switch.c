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

#include "wlan_ll_sap_main.h"
#include "wlan_ll_lt_sap_bearer_switch.h"
#include "wlan_sm_engine.h"
#include "wlan_policy_mgr_api.h"
#include "wlan_policy_mgr_ll_sap.h"

#define BEARER_SWITCH_TIMEOUT 5000
#define BEARER_SWITCH_WLAN_REQ_TIMEOUT 5000

#define BS_PREFIX_FMT "BS_SM_%d req_id 0x%x: "
#define BS_PREFIX_REF(vdev_id, req_id) (vdev_id), (req_id)


wlan_bs_req_id ll_lt_sap_bearer_switch_get_id(struct wlan_objmgr_psoc *psoc)
{
	wlan_bs_req_id request_id = BS_REQ_ID_INVALID;
	struct ll_sap_vdev_priv_obj *ll_sap_obj;
	struct wlan_objmgr_vdev *vdev;
	uint8_t vdev_id;

	vdev_id = wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);
	if (vdev_id == WLAN_INVALID_VDEV_ID)
		return request_id;
	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id,
						    WLAN_LL_SAP_ID);
	if (!vdev) {
		ll_sap_err(BS_PREFIX_FMT "Vdev is NULL",
			   BS_PREFIX_REF(vdev_id, request_id));
		return request_id;
	}

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);
	if (!ll_sap_obj) {
		ll_sap_err(BS_PREFIX_FMT "ll sap obj is NULL",
			   BS_PREFIX_REF(vdev_id, request_id));

		goto rel_ref;
	}

	request_id = qdf_atomic_inc_return(
				&ll_sap_obj->bearer_switch_ctx->request_id);

	ll_sap_nofl_debug(BS_PREFIX_FMT, BS_PREFIX_REF(vdev_id, request_id));
rel_ref:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_LL_SAP_ID);

	return request_id;
}

bool __ll_lt_sap_is_bs_ctx_valid(struct bearer_switch_info *bs_ctx,
				 const char *func)
{
	if (!bs_ctx) {
		ll_sap_nofl_err("BS_SM context is null (via %s)", func);
		return false;
	}
	return true;
}

bool __ll_lt_sap_is_bs_req_valid(struct wlan_bearer_switch_request *bs_req,
				 const char *func)
{
	if (!bs_req) {
		ll_sap_nofl_err("BS_SM request is null (via %s)", func);
		return false;
	}

	if (bs_req->vdev_id >= WLAN_UMAC_PSOC_MAX_VDEVS) {
		ll_sap_nofl_err("Invalid vdev id %d in BS_SM request",
				bs_req->vdev_id);
		return false;
	}

	return true;
}

/**
 * ll_lt_sap_deliver_audio_transport_switch_resp_to_fw() - Deliver audio
 * transport switch response to FW
 * @vdev: Vdev on which the request is received
 * @req_type: Transport switch type for which the response is received
 * @status: Status of the response
 *
 * Return: None
 */
static void
ll_lt_sap_deliver_audio_transport_switch_resp_to_fw(
					struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_req_type req_type,
					enum bearer_switch_status status)
{
	struct wlan_objmgr_psoc *psoc;
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;
	struct wlan_ll_sap_tx_ops *tx_ops;

	psoc = wlan_vdev_get_psoc(vdev);

	psoc_ll_sap_obj = wlan_objmgr_psoc_get_comp_private_obj(
						psoc,
						WLAN_UMAC_COMP_LL_SAP);

	if (!psoc_ll_sap_obj) {
		ll_sap_err("psoc_ll_sap_obj is null");
		return;
	}

	tx_ops = &psoc_ll_sap_obj->tx_ops;

	if (!tx_ops->send_audio_transport_switch_resp) {
		ll_sap_err("deliver_audio_transport_switch_resp op is NULL");
		return;
	}

	tx_ops->send_audio_transport_switch_resp(psoc, req_type, status);
}

/**
 * bs_req_timeout_cb() - Callback which will be invoked on bearer switch req
 * timeout
 * @user_data: Bearer switch context
 *
 * API to handle the timeout for the bearer switch requests
 *
 * Return: None
 */

static void bs_req_timeout_cb(void *user_data)
{
	struct bearer_switch_info *bs_ctx = user_data;
	struct wlan_bearer_switch_request *bs_req = NULL;
	uint8_t i;
	enum wlan_bearer_switch_sm_evt event;

	for (i = 0; i < MAX_BEARER_SWITCH_REQUESTERS; i++) {
		if (!bs_ctx->requests[i].requester_cb)
			continue;

		/*
		 * It is always the first cached request for which the request
		 * to switch the bearer is sent (other requests for bearer
		 * switch are just cached) and for the same this timeout has
		 * happened
		 */
		bs_req = &bs_ctx->requests[i];
		break;
	}
	if (!bs_req) {
		ll_sap_err("BS_SM No request pending");
		return;
	}

	if (bs_req->req_type == WLAN_BS_REQ_TO_WLAN)
		event = WLAN_BS_SM_EV_SWITCH_TO_WLAN_TIMEOUT;
	else
		event = WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_TIMEOUT;
	bs_sm_deliver_event(wlan_vdev_get_psoc(bs_ctx->vdev), event,
			    sizeof(*bs_req), bs_req);
}

/**
 * ll_lt_sap_stop_bs_req_timer() - Stop bearer switch request timer
 * @bs_ctx: Bearer switch context
 * @req_id: Request id for which timer needs to be stopped
 *
 * API to stop bearer switch request timer
 *
 * Return: None
 */
static void ll_lt_sap_stop_bs_req_timer(struct bearer_switch_info *bs_ctx,
					uint32_t req_id)
{
	QDF_STATUS status;

	if (QDF_TIMER_STATE_RUNNING ==
		qdf_mc_timer_get_current_state(&bs_ctx->bs_request_timer)) {
		status = qdf_mc_timer_stop(&bs_ctx->bs_request_timer);
		if (QDF_IS_STATUS_ERROR(status))
			ll_sap_err(BS_PREFIX_FMT "failed to stop timer",
				   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
						 req_id));
	} else {
		ll_sap_err(BS_PREFIX_FMT "timer is not running",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 req_id));
	}
}

/**
 * ll_lt_sap_stop_bs_wlan_req_timer() - Stop bearer switch wlan request timer
 * @bs_ctx: Bearer switch context
 * @req_id: Request id for which timer needs to be stopped
 *
 * API to stop bearer switch request timer
 *
 * Return: None
 */
static void ll_lt_sap_stop_bs_wlan_req_timer(struct bearer_switch_info *bs_ctx,
					     uint32_t req_id)
{
	QDF_STATUS status;

	if (QDF_TIMER_STATE_RUNNING !=
		qdf_mc_timer_get_current_state(&bs_ctx->bs_wlan_request_timer))
		return;

	status = qdf_mc_timer_stop(&bs_ctx->bs_wlan_request_timer);
	if (QDF_IS_STATUS_ERROR(status))
		ll_sap_err(BS_PREFIX_FMT "failed to stop timer",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 req_id));
}

/**
 * bs_set_state() - Set bearer switch state in the bearer switch state machine
 * @bs_ctx: Bearer switch context
 * @state: State which needs to be set
 *
 * API to Set the bearer switch state
 *
 * Return: None
 */
static void bs_set_state(struct bearer_switch_info *bs_ctx,
			 enum wlan_bearer_switch_sm_state state)
{
	if (state < BEARER_SWITCH_MAX)
		bs_ctx->sm.bs_state = state;
	else
		ll_sap_err("BS_SM vdev %d state (%d) is invalid",
			   wlan_vdev_get_id(bs_ctx->vdev), state);
}

/**
 * bs_sm_state_update() - Update the bearer switch state in the bearer switch
 * state machine
 * @bs_ctx: Bearer switch context
 * @state: State which needs to be set
 *
 * API to update the bearer switch state
 *
 * Return: None
 */
static void bs_sm_state_update(struct bearer_switch_info *bs_ctx,
			       enum wlan_bearer_switch_sm_state state)
{
	if (!ll_lt_sap_is_bs_ctx_valid(bs_ctx))
		return;

	bs_set_state(bs_ctx, state);
}

static void
ll_lt_sap_invoke_req_callback_f(struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req,
				QDF_STATUS status, const char *func)
{
	if (bs_req->source == BEARER_SWITCH_REQ_FW) {
		if (status == QDF_STATUS_E_ALREADY ||
		    status == QDF_STATUS_SUCCESS)
			ll_lt_sap_deliver_audio_transport_switch_resp_to_fw(
						bs_ctx->vdev,
						bs_req->req_type,
						WLAN_BS_STATUS_COMPLETED);
		else
			ll_sap_err("Invalid response for FW request");
		return;
	}

	if (!bs_req->requester_cb) {
		ll_sap_err("%s BS_SM vdev %d NULL cbk, req_vdev %d src %d req %d arg val %d",
			   func, wlan_vdev_get_id(bs_ctx->vdev), bs_req->vdev_id,
			   bs_req->source, bs_req->request_id,
			   bs_req->arg_value);
		return;
	}

	/* Invoke the requester callback without waiting for the response */
	bs_req->requester_cb(wlan_vdev_get_psoc(bs_ctx->vdev), bs_req->vdev_id,
			     bs_req->request_id, status, bs_req->arg_value,
			     bs_req->arg);
}

#define ll_lt_sap_invoke_req_callback(bs_ctx, bs_req, status) \
	ll_lt_sap_invoke_req_callback_f(bs_ctx, bs_req, status, __func__)

/**
 * ll_lt_sap_bs_increament_ref_count() - Increment the bearer switch ref count
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request corresponding to which the ref count needs to
 * be incremented
 *
 * API to increment the bearer switch ref count
 *
 * Return: None
 */
static inline void
ll_lt_sap_bs_increament_ref_count(struct bearer_switch_info *bs_ctx,
				  struct wlan_bearer_switch_request *bs_req)
{
	uint32_t ref_count;
	uint32_t total_ref_count;

	total_ref_count = qdf_atomic_inc_return(&bs_ctx->total_ref_count);
	if (bs_req->source == BEARER_SWITCH_REQ_FW)
		ref_count = qdf_atomic_inc_return(&bs_ctx->fw_ref_count);
	else
		ref_count = qdf_atomic_inc_return(
			&bs_ctx->ref_count[bs_req->vdev_id][bs_req->source]);

	ll_sap_nofl_debug(BS_PREFIX_FMT "vdev %d src %d count %d Total req count %d",
			  BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					bs_req->request_id),
			  bs_req->vdev_id, bs_req->source, ref_count,
			  total_ref_count);
}

/**
 * ll_lt_sap_bs_decreament_ref_count() - Decreament the bearer switch ref count
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request corresponding to which the ref count needs to
 * be decremented
 *
 * API to decreament the bearer switch ref count
 *
 * Return: None
 */
static inline QDF_STATUS
ll_lt_sap_bs_decreament_ref_count(struct bearer_switch_info *bs_ctx,
				  struct wlan_bearer_switch_request *bs_req)
{
	uint32_t ref_count;
	uint32_t total_ref_count;

	/*
	 * If the ref count is 0 for the requested module, it means that this
	 * module did not requested for wlan to non wlan transition earlier, so
	 * reject this operation.
	 */
	if (bs_req->source == BEARER_SWITCH_REQ_FW) {
		if (!qdf_atomic_read(&bs_ctx->fw_ref_count)) {
			ll_sap_debug(BS_PREFIX_FMT "ref_count is zero for FW",
				     BS_PREFIX_REF(
						wlan_vdev_get_id(bs_ctx->vdev),
						bs_req->request_id));
			return QDF_STATUS_E_INVAL;
		}
		ref_count = qdf_atomic_dec_return(&bs_ctx->fw_ref_count);
	} else if (!qdf_atomic_read(
			&bs_ctx->ref_count[bs_req->vdev_id][bs_req->source])) {
		ll_sap_debug(BS_PREFIX_FMT "vdev %d src %d count is zero",
			     BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					   bs_req->request_id),
			     bs_req->vdev_id, bs_req->source);
		return QDF_STATUS_E_INVAL;
	} else {
		ref_count = qdf_atomic_dec_return(
			&bs_ctx->ref_count[bs_req->vdev_id][bs_req->source]);
	}
	total_ref_count = qdf_atomic_dec_return(&bs_ctx->total_ref_count);
	ll_sap_nofl_debug(BS_PREFIX_FMT "vdev %d src %d count %d Total req count %d",
			  BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					bs_req->request_id),
			  bs_req->vdev_id, bs_req->source,
			  ref_count, total_ref_count);

	return QDF_STATUS_SUCCESS;
}

/**
 * ll_lt_sap_cache_bs_request() - Cache the bearer switch request
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request which needs to be cached
 *
 * API to cache the bearer switch request in the bearer switch context
 *
 * Return: None
 */
static void
ll_lt_sap_cache_bs_request(struct bearer_switch_info *bs_ctx,
			   struct wlan_bearer_switch_request *bs_req)
{
	uint8_t i;

	for (i = 0; i < MAX_BEARER_SWITCH_REQUESTERS; i++) {
		/*
		 * Find the empty slot in the requests array to cache the
		 * current request
		 */
		if (bs_ctx->requests[i].requester_cb)
			continue;
		ll_sap_debug(BS_PREFIX_FMT "req_vdev %d cache %d request at %d",
			     BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					   bs_req->request_id),
			     bs_req->vdev_id, bs_req->req_type, i);
		bs_ctx->requests[i].vdev_id = bs_req->vdev_id;
		bs_ctx->requests[i].request_id = bs_req->request_id;
		bs_ctx->requests[i].req_type = bs_req->req_type;
		bs_ctx->requests[i].source = bs_req->source;
		bs_ctx->requests[i].requester_cb = bs_req->requester_cb;
		bs_ctx->requests[i].arg_value = bs_req->arg_value;
		bs_ctx->requests[i].arg = bs_req->arg;
		break;
	}
	if (i >= MAX_BEARER_SWITCH_REQUESTERS)
		ll_sap_err(BS_PREFIX_FMT "Max number of requests reached",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 bs_req->request_id));
}

static void ll_lt_sap_reorder_bs_req(struct bearer_switch_info *bs_ctx)
{
	uint8_t i, j;

	for (i = 0; i < MAX_BEARER_SWITCH_REQUESTERS; i++) {
		if (!bs_ctx->requests[i].requester_cb)
			continue;

		for (j = 0; j < i; j++) {
			if (bs_ctx->requests[j].requester_cb)
				continue;

			bs_ctx->requests[j].requester_cb =
				bs_ctx->requests[i].requester_cb;
			bs_ctx->requests[j].arg =
					bs_ctx->requests[i].arg;
			bs_ctx->requests[j].arg_value =
					bs_ctx->requests[i].arg_value;
			bs_ctx->requests[j].req_type =
					bs_ctx->requests[i].req_type;
			bs_ctx->requests[j].vdev_id =
					bs_ctx->requests[i].vdev_id;
			bs_ctx->requests[j].request_id =
					bs_ctx->requests[i].request_id;
			bs_ctx->requests[j].source =
					bs_ctx->requests[i].source;

			bs_ctx->requests[i].requester_cb = NULL;
			bs_ctx->requests[i].arg = NULL;
			bs_ctx->requests[i].arg_value = 0;
		}
	}
}

/**
 * ll_lt_sap_flush_bs_wlan_req() - API to flush bearer switch
 * requests to wlan bearer from cached requests
 * @bs_ctx: Bearer switch info
 *
 * This function clears the wlan bearer requests from the cached list, there is
 * no need to invoke the callbacks of wlan switch requests as these were already
 * invoked as soon as the requests are received.
 *
 * Return: None
 */
static void
ll_lt_sap_flush_bs_wlan_req(struct bearer_switch_info *bs_ctx)
{
	uint8_t i;

	for (i = 0; i < MAX_BEARER_SWITCH_REQUESTERS; i++) {
		if (bs_ctx->requests[i].requester_cb &&
		    bs_ctx->requests[i].req_type != WLAN_BS_REQ_TO_WLAN)
			continue;

		bs_ctx->requests[i].requester_cb = NULL;
		bs_ctx->requests[i].arg = NULL;
		bs_ctx->requests[i].arg_value = 0;
	}

	ll_lt_sap_reorder_bs_req(bs_ctx);
}

/*
 * ll_lt_sap_invoke_bs_requester_cbks() - Invoke callbacks of all the cached
 * requests
 * @bs_ctx: Bearer switch context
 * @Status: Status with which the callbacks needs to be invoked
 *
 * API to invoke the callbacks of the cached requests
 *
 * Return: None
 */
static void
ll_lt_sap_invoke_bs_requester_cbks(struct bearer_switch_info *bs_ctx,
				   QDF_STATUS status)
{
	struct wlan_objmgr_psoc *psoc;
	uint8_t i;

	psoc = wlan_vdev_get_psoc(bs_ctx->vdev);

	if (!psoc) {
		ll_sap_err("BS_SM invalid psoc");
		return;
	}

	for (i = 0; i < MAX_BEARER_SWITCH_REQUESTERS; i++) {
		if (!bs_ctx->requests[i].requester_cb)
			continue;

		/*
		 * Callbacks of WLAN_BS_REQ_TO_WLAN already invoked without
		 * waiting for any response so no need to invoke from here
		 */
		if (bs_ctx->requests[i].req_type == WLAN_BS_REQ_TO_WLAN)
			continue;

		if ((bs_ctx->requests[i].source == BEARER_SWITCH_REQ_FW) &&
		    (status == QDF_STATUS_E_TIMEOUT))
			ll_lt_sap_deliver_audio_transport_switch_resp_to_fw(
						bs_ctx->vdev,
						bs_ctx->requests[i].req_type,
						WLAN_BS_STATUS_TIMEOUT);
		else
			bs_ctx->requests[i].requester_cb(
						psoc,
						bs_ctx->requests[i].vdev_id,
						bs_ctx->requests[i].request_id,
						status,
						bs_ctx->requests[i].arg_value,
						bs_ctx->requests[i].arg);
		bs_ctx->requests[i].requester_cb = NULL;
		bs_ctx->requests[i].arg = NULL;
		bs_ctx->requests[i].arg_value = 0;
	}
	ll_lt_sap_reorder_bs_req(bs_ctx);
}

/*
 * ll_lt_sap_find_first_valid_bs_wlan_req() - Find first valid wlan switch
 * request from the cached requests
 * @bs_ctx: Bearer switch context
 *
 * API to find first valid wlan switch request from the cached requests
 *
 * Return: If found return bearer switch request, else return NULL
 */
static struct wlan_bearer_switch_request *
ll_lt_sap_find_first_valid_bs_wlan_req(struct bearer_switch_info *bs_ctx)
{
	uint8_t i;

	for (i = 0; i < MAX_BEARER_SWITCH_REQUESTERS; i++) {
		if (bs_ctx->requests[i].requester_cb &&
		    bs_ctx->requests[i].req_type == WLAN_BS_REQ_TO_WLAN)
			return &bs_ctx->requests[i];
	}
	return NULL;
}

/*
 * ll_lt_sap_find_first_valid_bs_non_wlan_req() - Find first valid non-wlan
 * switch request from the cached requests
 * @bs_ctx: Bearer switch context
 *
 * API to find first valid non-wlan switch request from the cached requests
 *
 * Return: If found return bearer switch request, else return NULL
 */
static struct wlan_bearer_switch_request *
ll_lt_sap_find_first_valid_bs_non_wlan_req(struct bearer_switch_info *bs_ctx)
{
	uint8_t i;

	for (i = 0; i < MAX_BEARER_SWITCH_REQUESTERS; i++) {
		if (bs_ctx->requests[i].requester_cb &&
		    bs_ctx->requests[i].req_type == WLAN_BS_REQ_TO_NON_WLAN)
			return &bs_ctx->requests[i];
	}
	return NULL;
}

/*
 * ll_lt_sap_find_bs_req_by_id() - Find bearer switch request
 * based on request id from the cached requests
 * @bs_ctx: Bearer switch context
 *
 * Return: If found return bearer switch request, else return NULL
 */
static struct wlan_bearer_switch_request *
ll_lt_sap_find_bs_req_by_id(struct bearer_switch_info *bs_ctx,
			    wlan_bs_req_id request_id)
{
	uint8_t i;

	for (i = 0; i < MAX_BEARER_SWITCH_REQUESTERS; i++) {
		if (bs_ctx->requests[i].requester_cb &&
		    bs_ctx->requests[i].request_id == request_id)
			return &bs_ctx->requests[i];
	}
	return NULL;
}

/*
 * ll_lt_sap_send_bs_req_to_userspace() - Send bearer switch request to user
 * space
 * @vdev: ll_lt sap vdev
 * @req_type: Type of the request
 * @source: Source of the request
 *
 * API to send bearer switch request to userspace
 *
 * Return: None
 */
static void
ll_lt_sap_send_bs_req_to_userspace(struct wlan_objmgr_vdev *vdev,
				   enum bearer_switch_req_type req_type,
				   enum bearer_switch_req_source source)
{
	struct ll_sap_ops *osif_cbk;

	osif_cbk = ll_sap_get_osif_cbk();
	if (osif_cbk && osif_cbk->ll_sap_send_audio_transport_switch_req_cb)
		osif_cbk->ll_sap_send_audio_transport_switch_req_cb(vdev,
								    req_type,
								    source);
}

/**
 * ll_lt_sap_handle_bs_request_on_stop_ap() - API to handle bearer switch
 * in stop ap case
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * If LL_LT_SAP is stopping, no need to start the timer, no need to cache the
 * request, no need to invoke the cbk as well Just move state machine to
 * non-wlan state.
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_request_on_stop_ap(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	ll_lt_sap_send_bs_req_to_userspace(bs_ctx->vdev, bs_req->req_type,
					   bs_req->source);
	bs_sm_transition_to(bs_ctx, BEARER_NON_WLAN);

	return;
}

/**
 * ll_lt_sap_continue_bs_to_wlan() - Callback which will be invoked on bearer
 * switch wlan req timeout
 * @user_data: Bearer switch context
 *
 * API to handle the wlan bearer switch request once bearer switch wlan timer
 * expires
 *
 * Return: None
 */
static void
ll_lt_sap_continue_bs_to_wlan(void *user_data)
{
	struct bearer_switch_info *bs_ctx = user_data;
	struct wlan_bearer_switch_request *bs_req = NULL;
	QDF_STATUS status;

	if (qdf_atomic_read(&bs_ctx->total_ref_count)) {
		/*
		 * Remove switch to wlan request from the cache list if
		 * it was cached
		 */
		ll_lt_sap_flush_bs_wlan_req(bs_ctx);
		return;
	}

	bs_req = ll_lt_sap_find_first_valid_bs_wlan_req(bs_ctx);

	if (!ll_lt_sap_is_bs_req_valid(bs_req)) {
		ll_sap_err("BS_SM vdev %d Invalid total ref count %d",
			   wlan_vdev_get_id(bs_ctx->vdev),
			   qdf_atomic_read(&bs_ctx->total_ref_count));
		return;
	}

	ll_lt_sap_send_bs_req_to_userspace(bs_ctx->vdev, bs_req->req_type,
					   bs_req->source);

	/*
	 * Switch back to wlan is sent here, hence set the non wlan requested
	 * as false
	 */
	bs_ctx->sm.is_non_wlan_requested = false;

	status = qdf_mc_timer_start(&bs_ctx->bs_request_timer,
				    BEARER_SWITCH_TIMEOUT);
	if (QDF_IS_STATUS_ERROR(status))
		ll_sap_err(BS_PREFIX_FMT "Failed to start timer",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 bs_req->request_id));

	bs_sm_transition_to(bs_ctx, BEARER_WLAN_REQUESTED);
}

/**
 * ll_lt_sap_handle_bs_to_wlan_in_non_wlan_state() - API to handle bearer switch
 * to wlan in non-wlan state.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * If total_ref_count is non-zero, means non-wlan bearer should be there and
 * no further action is required, just invoke the callback of the requester
 * with status as success.
 * If total_ref_count is zero, means none of the module wants to be in the
 * non-wlan bearer, send the wlan bearer switch request to userspace.and
 * cache the request so that if timeout or failure happens, state machine can
 * be moved from BEARER_WLAN_REQUESTED to BEARER_WLAN
 * If the last_status is not QDF_STATUS_SUCCESS, means the last request to
 * switch the bearer to non-wlan was failed/timedout and the state is moved to
 * non-wlan, irrespective of the last status follow the same flow and send the
 * request to switch the bearer to wlan
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_wlan_in_non_wlan_state(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	bool is_bs_req_cached = false;

	if (ll_lt_sap_find_bs_req_by_id(bs_ctx, bs_req->request_id))
		is_bs_req_cached = true;

	/*
	 * If the bearer switch request is already cached then don't decreament
	 * the ref count as the ref count would have already been decremented
	 * when the request was cached.
	 * For example if bearer switch to wlan is received in non-wlan
	 * requested state, then the ref count is decreamented and request is
	 * cached. Once the response to the current non-wlan bearer switch
	 * request is received then switch to wlan will be executed and this
	 * function will be invoked
	 */
	if (!is_bs_req_cached) {
		status = ll_lt_sap_bs_decreament_ref_count(bs_ctx, bs_req);
		if (QDF_IS_STATUS_ERROR(status))
			return;
	}

	/*
	 * If host driver did not requested for non wlan bearer then don't send
	 * a request to switch back to wlan
	 */
	if (!bs_ctx->sm.is_non_wlan_requested) {
		ll_sap_debug("Non wlan is not requested, don't switch to wlan");
		goto invoke_requester_cb;
	}

	if (qdf_atomic_read(&bs_ctx->total_ref_count))
		goto invoke_requester_cb;

	/*
	 * If the request is not already cached, then only cache this request
	 * For example if the bearer switch to wlan is received in non-wlan
	 * requested state, then this request will be cached in non-wlan
	 * requested state itself
	 */
	if (!is_bs_req_cached)
		ll_lt_sap_cache_bs_request(bs_ctx, bs_req);

	status = qdf_mc_timer_start(&bs_ctx->bs_wlan_request_timer,
				    BEARER_SWITCH_WLAN_REQ_TIMEOUT);
	if (QDF_IS_STATUS_ERROR(status)) {
		ll_sap_err(BS_PREFIX_FMT "Failed to start timer",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 bs_req->request_id));
		ll_lt_sap_continue_bs_to_wlan(bs_ctx);
	}

invoke_requester_cb:
	ll_lt_sap_invoke_req_callback(bs_ctx, bs_req, status);
}

/**
 * ll_lt_sap_handle_bs_to_wlan_completed_wlan_in_non_wlan_state() - API to
 * handle bearer switch to wlan completed event in non-wlan state.
 * @bs_ctx: Bearer switch context
 *
 * This event is possible only if current bearer is non-wlan and user space
 * switches the bearer to wlan first time after bringing up the LL_LT_SAP.
 * Since host driver did not request for the bearer switch so there will not
 * be any valid bearer switch request.
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_wlan_completed_wlan_in_non_wlan_state(
				struct bearer_switch_info *bs_ctx)
{
	bs_sm_transition_to(bs_ctx, BEARER_WLAN);
}

/**
 * ll_lt_sap_handle_bs_to_non_wlan_in_non_wlan_state() - API to handle bearer
 * switch to non-wlan in non-wlan state.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Increment the ref_count. check last_status, if last status is
 * QDF_STATUS_SUCCESS, then just invoke the requester callback with success, no
 * need to cache the request.
 * If last status is not QDF_STATUS_SUCCESS, it means last request was not
 * success, so cache this request and send a non-wlan bearer switch request to
 * userspace.
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_non_wlan_in_non_wlan_state(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	QDF_STATUS status;

	/* If bearer is non-wlan and LL_LT_SAP is getting stopped,
	 * then just return
	 */
	if (bs_req->source == BEARER_SWITCH_REQ_STOP_AP) {
		ll_sap_debug("Bearer is already non-wlan");
		return;
	}

	ll_lt_sap_bs_increament_ref_count(bs_ctx, bs_req);

	ll_lt_sap_stop_bs_wlan_req_timer(bs_ctx, bs_req->request_id);
	/*
	 * Remove switch to wlan request from the cache list if
	 * it was cached
	 */
	ll_lt_sap_flush_bs_wlan_req(bs_ctx);

	if (QDF_IS_STATUS_SUCCESS(bs_ctx->last_status))
		return ll_lt_sap_invoke_req_callback(bs_ctx, bs_req,
						     QDF_STATUS_E_ALREADY);

	ll_lt_sap_cache_bs_request(bs_ctx, bs_req);

	ll_lt_sap_send_bs_req_to_userspace(bs_ctx->vdev, bs_req->req_type,
					   bs_req->source);

	bs_ctx->sm.is_non_wlan_requested = true;

	status = qdf_mc_timer_start(&bs_ctx->bs_request_timer,
				    BEARER_SWITCH_TIMEOUT);
	if (QDF_IS_STATUS_ERROR(status))
		ll_sap_err(BS_PREFIX_FMT "Failed to start timer",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 bs_req->request_id));

	bs_sm_transition_to(bs_ctx, BEARER_NON_WLAN_REQUESTED);
}

/**
 * ll_lt_sap_handle_bs_to_wlan_in_non_wlan_requested_state() - API to handle
 * bearer switch to wlan in non-wlan requested state
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Decreament the ref_count.
 * 1) If total_ref_count is non-zero, it means there is some module
 * which wants to be in the non-wlan state, invoke the caller of current
 * requester with success and let the ongoing request for non wlan bearer
 * switch get complete.
 * 2) If total_ref_count is zero, it means no other module wants to be
 * in non-wlan state, cache this wlan bearer switch request and invoke the
 * callback of the caller with status as success, once the response of
 * ongoing request of the non-wlan switch is received then check the
 * total_ref_count and if it is 0, then send the request to switch to
 * wlan in ll_lt_sap_handle_bs_to_non_wlan_completed,
 * ll_lt_sap_handle_bs_to_non_wlan_failure or in
 * ll_lt_sap_handle_bs_to_non_wlan_timeout.
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_wlan_in_non_wlan_requested_state(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	QDF_STATUS status;

	status = ll_lt_sap_bs_decreament_ref_count(bs_ctx, bs_req);

	if (QDF_IS_STATUS_ERROR(status))
		return;

	if (!bs_req->requester_cb) {
		ll_sap_err(BS_PREFIX_FMT "NULL cbk, req_vdev %d src %d arg val %d",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 bs_req->request_id),
			   bs_req->vdev_id, bs_req->source,
			   bs_req->arg_value);
		return;
	}

	ll_lt_sap_cache_bs_request(bs_ctx, bs_req);
	ll_lt_sap_invoke_req_callback(bs_ctx, bs_req, status);
}

/**
 * ll_lt_sap_handle_bs_to_non_wlan_in_non_wlan_requested_state() - API to handle
 * bearer switch to non wlan in non-wlan requested state.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Increment the ref_count.
 * Request to switch the bearer to non-wlan is already sent, cache this request
 * and on receiving the response of the current non-wlan request invoke the
 * callbacks of all the requesters in ll_lt_sap_handle_bs_to_non_wlan_completed,
 * ll_lt_sap_handle_bs_to_non_wlan_failure,
 * ll_lt_sap_handle_bs_to_non_wlan_timeout
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_non_wlan_in_non_wlan_requested_state(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	ll_lt_sap_bs_increament_ref_count(bs_ctx, bs_req);

	ll_lt_sap_cache_bs_request(bs_ctx, bs_req);
}

/**
 * ll_lt_sap_handle_bs_to_non_wlan_timeout() - API to handle bearer switch
 * to non-wlan timeout event.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Move the bearer state to BEARER_NON_WLAN, even if this request is timedout as
 * requester of this request needs to invoke the bearer switch to wlan again
 * to reset the ref counts and in that path the state will be moved to
 * BEARER_WLAN and the request to switch the bearer to userspace will still be
 * sent irrespective of last_status and userspace should return success as
 * the bearer is already wlan
 * If total_ref_count is non-zero, means non-wlan bearer should be there and
 * no further action is required.
 * If total_ref_count is zero, means none of the module wants to be in the
 * non-wlan bearer and the current module which has requested for non-wlan
 * bearer also requested for the wlan bearer before this event/response is
 * received(switch to wlan in non-wlan requested state) and this request for
 * wlan bearer switch should have been cached get that request and deliver the
 * event to switch the bearer to wlan.
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_non_wlan_timeout(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	struct wlan_bearer_switch_request *first_bs_req;

	bs_sm_transition_to(bs_ctx, BEARER_NON_WLAN);

	bs_ctx->last_status = QDF_STATUS_E_TIMEOUT;
	ll_lt_sap_invoke_bs_requester_cbks(bs_ctx, QDF_STATUS_E_TIMEOUT);

	/*
	 * If total_ref_count is non-zero, means non-wlan bearer should be
	 * there, so no further action is required
	 */
	if (qdf_atomic_read(&bs_ctx->total_ref_count))
		return;

	first_bs_req = ll_lt_sap_find_first_valid_bs_wlan_req(bs_ctx);

	if (!ll_lt_sap_is_bs_req_valid(first_bs_req)) {
		ll_sap_err("BS_SM vdev %d Invalid total ref count %d",
			   wlan_vdev_get_id(bs_ctx->vdev),
			   qdf_atomic_read(&bs_ctx->total_ref_count));
	}

	bs_sm_deliver_event(wlan_vdev_get_psoc(bs_ctx->vdev),
			    WLAN_BS_SM_EV_SWITCH_TO_WLAN,
			    sizeof(*first_bs_req), first_bs_req);
}

/**
 * ll_lt_sap_handle_bs_to_non_wlan_completed() - API to handle bearer switch
 * to non-wlan complete event.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * If total_ref_count is non-zero, means non-wlan bearer should be there and
 * no further action is required.
 * If total_ref_count is zero, means none of the module wants to be in the
 * non-wlan bearer and the current module which has requested for non-wlan
 * bearer also requested for the wlan bearer before this event/response is
 * received(switch to wlan in non-wlan requested state) and this request for
 * wlan bearer switch should have been cached get that request and deliver the
 * event to switch the bearer to wlan.
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_non_wlan_completed(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	struct wlan_bearer_switch_request *first_bs_req;

	ll_lt_sap_stop_bs_req_timer(bs_ctx, bs_req->request_id);

	bs_sm_transition_to(bs_ctx, BEARER_NON_WLAN);

	bs_ctx->last_status = QDF_STATUS_SUCCESS;

	ll_lt_sap_invoke_bs_requester_cbks(bs_ctx, QDF_STATUS_SUCCESS);

	/*
	 * If total_ref_count is non-zero, means non-wlan bearer should be
	 * there, so no further action is required
	 */
	if (qdf_atomic_read(&bs_ctx->total_ref_count))
		return;

	first_bs_req = ll_lt_sap_find_first_valid_bs_wlan_req(bs_ctx);

	if (!ll_lt_sap_is_bs_req_valid(first_bs_req)) {
		ll_sap_err("BS_SM vdev %d Invalid total ref count %d",
			   wlan_vdev_get_id(bs_ctx->vdev),
			   qdf_atomic_read(&bs_ctx->total_ref_count));
	}

	bs_sm_deliver_event(wlan_vdev_get_psoc(bs_ctx->vdev),
			    WLAN_BS_SM_EV_SWITCH_TO_WLAN,
			    sizeof(*first_bs_req), first_bs_req);
}

/**
 * ll_lt_sap_handle_bs_to_non_wlan_failure() - API to handle bearer switch
 * to non-wlan failure event.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Move the bearer state to BEARER_NON_WLAN, even if this request is failed as
 * requester of this request needs to invoke the bearer switch to wlan again
 * to reset the ref counts and in that path the state will be moved to
 * BEARER_WLAN and the request to switch the bearer to userspace will still be
 * sent irrespective of last_status and userspace should return success as
 * the bearer is already wlan
 * If total_ref_count is non-zero, means non-wlan bearer should be there and
 * no further action is required.
 * If total_ref_count is zero, means none of the module wants to be in the
 * non-wlan bearer and the current module which has requested for non-wlan
 * bearer also requested for the wlan bearer before this event/response is
 * received(switch to wlan in non-wlan requested state) and this request for
 * wlan bearer switch should have been cached get that request and deliver the
 * event to switch the bearer to wlan.
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_non_wlan_failure(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	struct wlan_bearer_switch_request *first_bs_req;

	ll_lt_sap_stop_bs_req_timer(bs_ctx, bs_req->request_id);

	bs_sm_transition_to(bs_ctx, BEARER_NON_WLAN);

	bs_ctx->last_status = QDF_STATUS_E_FAILURE;

	ll_lt_sap_invoke_bs_requester_cbks(bs_ctx, QDF_STATUS_E_FAILURE);

	/*
	 * If total_ref_count is non-zero, means non-wlan bearer should be
	 * there, so no further action is required
	 */
	if (qdf_atomic_read(&bs_ctx->total_ref_count))
		return;

	first_bs_req = ll_lt_sap_find_first_valid_bs_wlan_req(bs_ctx);

	if (!ll_lt_sap_is_bs_req_valid(first_bs_req)) {
		ll_sap_err("BS_SM vdev %d Invalid total ref count %d",
			   wlan_vdev_get_id(bs_ctx->vdev),
			   qdf_atomic_read(&bs_ctx->total_ref_count));
	}

	bs_sm_deliver_event(wlan_vdev_get_psoc(bs_ctx->vdev),
			    WLAN_BS_SM_EV_SWITCH_TO_WLAN,
			    sizeof(*first_bs_req), first_bs_req);
}

/**
 * ll_lt_sap_handle_bs_to_wlan_in_wlan_state() - API to handle bearer switch
 * to wlan in wlan state.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Ideally this scenario should not occur, but in any case if this happens then
 * simply invoke the requester callback with QDF_STATUS_E_ALREADY status
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_wlan_in_wlan_state(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	ll_lt_sap_invoke_req_callback(bs_ctx, bs_req, QDF_STATUS_E_ALREADY);
}

/**
 * ll_lt_sap_handle_bs_to_non_wlan_in_wlan_state() - API to handle bearer switch
 * to non-wlan in wlan state.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Increment the ref count, cache the request, send the non-wlan bearer switch
 * request to userspace and transition thestate to BEARER_NON_WLAN_REQUESTED
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_non_wlan_in_wlan_state(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	QDF_STATUS status;

	if (!bs_req->requester_cb) {
		ll_sap_err(BS_PREFIX_FMT "NULL cbk, req_vdev %d src %d arg val %d",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 bs_req->request_id),
			   bs_req->vdev_id, bs_req->source,
			   bs_req->arg_value);
		return;
	}

	if (bs_req->source == BEARER_SWITCH_REQ_STOP_AP)
		return ll_lt_sap_handle_bs_request_on_stop_ap(bs_ctx, bs_req);

	/*
	 * If bearer switch request is already cached and this function is
	 * invoked after that, then don't increment the ref count and don't
	 * cache the BS request again.
	 * For example, if a bearer switch to wlan is already requested and
	 * before response to this request is received, a non-wlan bearer switch
	 * request is received, then that request is cached and state machine
	 * waits for the ongoing requests completion, once the ongoing request
	 * to wlan is completed then this cached request to non-wlan is executed
	 * so in that case there is no need to cache this request again.
	 */
	if (!ll_lt_sap_find_bs_req_by_id(bs_ctx, bs_req->request_id)) {
		ll_lt_sap_bs_increament_ref_count(bs_ctx, bs_req);

		ll_lt_sap_cache_bs_request(bs_ctx, bs_req);
	}

	ll_lt_sap_send_bs_req_to_userspace(bs_ctx->vdev, bs_req->req_type,
					   bs_req->source);

	bs_ctx->sm.is_non_wlan_requested = true;

	bs_sm_transition_to(bs_ctx, BEARER_NON_WLAN_REQUESTED);

	status = qdf_mc_timer_start(&bs_ctx->bs_request_timer,
				    BEARER_SWITCH_TIMEOUT);
	if (QDF_IS_STATUS_ERROR(status))
		ll_sap_err(BS_PREFIX_FMT "Failed to start timer",
			   BS_PREFIX_REF(wlan_vdev_get_id(bs_ctx->vdev),
					 bs_req->request_id));
}

/**
 * ll_lt_sap_handle_bs_to_wlan_in_wlan_req_state() - API to handle bearer switch
 * to wlan in wlan requested state.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * This scenario is not possible as if already switch to wlan is
 * requested it means total_ref_count is already zero, so no other
 * module should request for the bearer to switch to wlan. Hence drop
 * this request.
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_wlan_in_wlan_req_state(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
}

/**
 * ll_lt_sap_handle_bs_to_non_wlan_in_wlan_req_state() - API to handle bearer
 * switch to non-wlan in wlan requested state.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Increment the reference count and cache the request so that on
 * receiving the response of the ongoing wlan switch request, switch
 * to non-wlan can be issued from ll_lt_sap_handle_bs_to_wlan_completed,
 * ll_lt_sap_handle_bs_to_wlan_timeout or from
 * ll_lt_sap_handle_bs_to_wlan_failure
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_non_wlan_in_wlan_req_state(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	ll_lt_sap_bs_increament_ref_count(bs_ctx, bs_req);

	ll_lt_sap_cache_bs_request(bs_ctx, bs_req);
}

/**
 * ll_lt_sap_switch_to_non_wlan_from_wlan() - API to issue bearer switch
 * request to non-wlan bearer from wlan-requested state response
 * @bs_ctx: Bearer switch info
 *
 * This function handles the case when someone requested for non-wlan
 * bearer-switch in between of ongoing non-wlan to wlan bearer switch request.
 * check if any non-wlan bearer switch request is issued before receiving this
 * response then switch to non-wlan bearer
 *
 * Return: None
 */
static void
ll_lt_sap_switch_to_non_wlan_from_wlan(struct bearer_switch_info *bs_ctx)
{
	struct wlan_bearer_switch_request *bs_req;

	/* no request for wlan to no-wlan bearer switch */
	if (!qdf_atomic_read(&bs_ctx->total_ref_count))
		return;

	bs_req = ll_lt_sap_find_first_valid_bs_non_wlan_req(bs_ctx);

	if (!ll_lt_sap_is_bs_req_valid(bs_req)) {
		ll_sap_err("BS_SM vdev %d Invalid total ref count %d",
			   wlan_vdev_get_id(bs_ctx->vdev),
			   qdf_atomic_read(&bs_ctx->total_ref_count));
	}

	bs_sm_deliver_event(wlan_vdev_get_psoc(bs_ctx->vdev),
			    WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN,
			    sizeof(*bs_req), bs_req);
}

/**
 * ll_lt_sap_handle_bs_to_wlan_timeout() - API to handle bearer switch
 * to wlan timeout event.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * Transition the state to wlan even in case of timeout as well, because from
 * the wlan point of view total_ref_count is 0 which means it is ready for
 * wlan bear Update last_status as QDF_STATUS_E_FAILURE and check if any
 * non-wlan bearer switch request is issued before receiving this response
 * then switch to non-wlan bearer
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_wlan_timeout(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	bs_sm_transition_to(bs_ctx, BEARER_WLAN);

	if (bs_req->source == BEARER_SWITCH_REQ_FW)
		ll_lt_sap_deliver_audio_transport_switch_resp_to_fw(
							bs_ctx->vdev,
							bs_req->req_type,
							WLAN_BS_STATUS_TIMEOUT);

	bs_ctx->last_status = QDF_STATUS_E_TIMEOUT;

	/* Remove this request from the cache list if it was cached */
	ll_lt_sap_flush_bs_wlan_req(bs_ctx);

	ll_lt_sap_switch_to_non_wlan_from_wlan(bs_ctx);
}

/**
 * ll_lt_sap_handle_bs_to_wlan_completed() - API to handle bearer switch
 * to wlan complete event.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * This event will be delivered by vendor command in the wlan requested state.
 * Stop the bearer switch timer, move the state to BEARER_WLAN and update the
 * last_status as QDF_STATUS_SUCCESS and check if any non-wlan bearer switch
 * request is issued before receiving this response then switch to non-wlan
 * bearer
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_wlan_completed(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	ll_lt_sap_stop_bs_req_timer(bs_ctx, bs_req->request_id);

	bs_sm_transition_to(bs_ctx, BEARER_WLAN);

	bs_ctx->last_status = QDF_STATUS_SUCCESS;

	/* Remove this request from the cache list if it was cached */
	ll_lt_sap_flush_bs_wlan_req(bs_ctx);

	ll_lt_sap_switch_to_non_wlan_from_wlan(bs_ctx);
}

/**
 * ll_lt_sap_handle_bs_to_wlan_failure() - API to handle bearer switch
 * to wlan failure event.
 * @bs_ctx: Bearer switch context
 * @bs_req: Bearer switch request
 *
 * This event will be delivered by vendor command in the wlan requested state
 * Stop the bearer switch timer,transition the state to wlan even in case of
 * failure as well, because from the wlan point of view total_ref_count is 0
 * which means it is ready for wlan bearer
 * Update last_status as QDF_STATUS_E_FAILURE and check if any non-wlan bearer
 * switch request is issued before receiving this response then switch to
 * non-wlan bearer
 *
 * Return: None
 */
static void
ll_lt_sap_handle_bs_to_wlan_failure(
				struct bearer_switch_info *bs_ctx,
				struct wlan_bearer_switch_request *bs_req)
{
	ll_lt_sap_stop_bs_req_timer(bs_ctx, bs_req->request_id);

	bs_sm_transition_to(bs_ctx, BEARER_WLAN);

	bs_ctx->last_status = QDF_STATUS_E_FAILURE;

	/* Remove this request from the cache list if it was cached */
	ll_lt_sap_flush_bs_wlan_req(bs_ctx);

	ll_lt_sap_switch_to_non_wlan_from_wlan(bs_ctx);
}

/**
 * bs_state_non_wlan_entry() - Entry API for non wlan state for bearer switch
 * state machine
 * @ctx: Bearer switch context
 *
 * API to perform operations on moving to non-wlan state
 *
 * Return: void
 */
static void bs_state_non_wlan_entry(void *ctx)
{
	struct bearer_switch_info *bs_ctx = ctx;

	bs_sm_state_update(bs_ctx, BEARER_NON_WLAN);
}

/**
 * bs_state_non_wlan_exit() - Exit API for non wlan state for bearer switch
 * state machine
 * @ctx: Bearer switch context
 *
 * API to perform operations on exiting from non-wlan state
 *
 * Return: void
 */
static void bs_state_non_wlan_exit(void *ctx)
{
}

/**
 * bs_state_non_wlan_event() - Non-wlan State event handler for bearer switch
 * state machine
 * @ctx: Bearer switch context
 * @event: event
 * @data_len: length of @data
 * @data: event data
 *
 * API to handle events in Non-wlan state
 *
 * Return: bool
 */
static bool bs_state_non_wlan_event(void *ctx, uint16_t event,
				    uint16_t data_len, void *data)
{
	bool event_handled = true;
	struct bearer_switch_info *bs_ctx = ctx;
	struct wlan_bearer_switch_request *bs_req = data;

	if (event != WLAN_BS_SM_EV_SWITCH_TO_WLAN_COMPLETED &&
	    !ll_lt_sap_is_bs_req_valid(bs_req))
		return false;
	if (!ll_lt_sap_is_bs_ctx_valid(bs_ctx))
		return false;

	switch (event) {
	case WLAN_BS_SM_EV_SWITCH_TO_WLAN:
		ll_lt_sap_handle_bs_to_wlan_in_non_wlan_state(bs_ctx, bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN:
		ll_lt_sap_handle_bs_to_non_wlan_in_non_wlan_state(bs_ctx,
								  bs_req);
		break;
	/*
	 * This event is possible when userspace first time sends the request
	 * to switch the bearer.
	 */
	case WLAN_BS_SM_EV_SWITCH_TO_WLAN_COMPLETED:
		ll_lt_sap_handle_bs_to_wlan_completed_wlan_in_non_wlan_state(
									bs_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

/**
 * bs_state_non_wlan_req_entry() - Entry API for non wlan requested state for
 * bearer switch state machine
 * @ctx: Bearer switch context
 *
 * API to perform operations on moving to non-wlan requested state
 *
 * Return: void
 */
static void bs_state_non_wlan_req_entry(void *ctx)
{
	struct bearer_switch_info *bs_ctx = ctx;

	bs_sm_state_update(bs_ctx, BEARER_NON_WLAN_REQUESTED);
}

/**
 * bs_state_non_wlan_req_exit() - Exit API for non wlan requested state for
 * bearer switch state machine
 * @ctx: Bearer switch context
 *
 * API to perform operations on exiting from non-wlan requested state
 *
 * Return: void
 */
static void bs_state_non_wlan_req_exit(void *ctx)
{
}

/**
 * bs_state_non_wlan_req_event() - Non-wlan requested State event handler for
 * bearer switch state machine
 * @ctx: Bearer switch context
 * @event: event
 * @data_len: length of @data
 * @data: event data
 *
 * API to handle events in Non-wlan requested state
 *
 * Return: bool
 */
static bool bs_state_non_wlan_req_event(void *ctx, uint16_t event,
					uint16_t data_len, void *data)
{
	bool event_handled = true;
	struct bearer_switch_info *bs_ctx = ctx;
	struct wlan_bearer_switch_request *bs_req = data;

	if (!ll_lt_sap_is_bs_req_valid(bs_req))
		return false;
	if (!ll_lt_sap_is_bs_ctx_valid(bs_ctx))
		return false;

	switch (event) {
	case WLAN_BS_SM_EV_SWITCH_TO_WLAN:
		ll_lt_sap_handle_bs_to_wlan_in_non_wlan_requested_state(bs_ctx,
									bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN:
		ll_lt_sap_handle_bs_to_non_wlan_in_non_wlan_requested_state(
									bs_ctx,
									bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_TIMEOUT:
		ll_lt_sap_handle_bs_to_non_wlan_timeout(bs_ctx, bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_COMPLETED:
		ll_lt_sap_handle_bs_to_non_wlan_completed(bs_ctx, bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_FAILURE:
		ll_lt_sap_handle_bs_to_non_wlan_failure(bs_ctx, bs_req);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

/**
 * bs_state_wlan_entry() - Entry API for wlan state for bearer switch
 * state machine
 * @ctx: Bearer switch context
 *
 * API to perform operations on moving to wlan state
 *
 * Return: void
 */
static void bs_state_wlan_entry(void *ctx)
{
	struct bearer_switch_info *bs_ctx = ctx;

	bs_sm_state_update(bs_ctx, BEARER_WLAN);
}

/**
 * bs_state_wlan_exit() - Exit API for wlan state for bearer switch
 * state machine
 * @ctx: Bearer switch context
 *
 * API to perform operations on exiting from wlan state
 *
 * Return: void
 */
static void bs_state_wlan_exit(void *ctx)
{
}

/**
 * bs_state_wlan_event() - Wlan State event handler for bearer switch
 * state machine
 * @ctx: Bearer switch context
 * @event: event
 * @data_len: length of @data
 * @data: event data
 *
 * API to handle events in Wlan state
 *
 * Return: bool
 */
static bool bs_state_wlan_event(void *ctx, uint16_t event,
				uint16_t data_len, void *data)
{
	bool event_handled = true;
	struct bearer_switch_info *bs_ctx = ctx;
	struct wlan_bearer_switch_request *bs_req = data;

	if (!ll_lt_sap_is_bs_req_valid(bs_req))
		return false;
	if (!ll_lt_sap_is_bs_ctx_valid(bs_ctx))
		return false;

	switch (event) {
	case WLAN_BS_SM_EV_SWITCH_TO_WLAN:
		ll_lt_sap_handle_bs_to_wlan_in_wlan_state(bs_ctx, bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN:
		ll_lt_sap_handle_bs_to_non_wlan_in_wlan_state(bs_ctx, bs_req);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

/**
 * bs_state_wlan_req_entry() - Entry API for Wlan requested state for
 *bearer switch state machine
 * @ctx: Bearer switch context
 *
 * API to perform operations on moving to Wlan requested state
 *
 * Return: void
 */
static void bs_state_wlan_req_entry(void *ctx)
{
	struct bearer_switch_info *bs_ctx = ctx;

	bs_sm_state_update(bs_ctx, BEARER_WLAN_REQUESTED);
}

/**
 * bs_state_wlan_req_exit() - Exit API for Wlan requested state for
 * bearer switch state machine
 * @ctx: Bearer switch context
 *
 * API to perform operations on exiting from Wlan requested state
 *
 * Return: void
 */
static void bs_state_wlan_req_exit(void *ctx)
{
}

/**
 * bs_state_wlan_req_event() - Wlan requested State event handler for
 * bearer switch state machine
 * @ctx: Bearer switch context
 * @event: event
 * @data_len: length of @data
 * @data: event data
 *
 * API to handle events in Wlan state
 *
 * Return: bool
 */
static bool bs_state_wlan_req_event(void *ctx, uint16_t event,
				    uint16_t data_len, void *data)
{
	bool event_handled = true;
	struct bearer_switch_info *bs_ctx = ctx;
	struct wlan_bearer_switch_request *bs_req = data;

	if (!ll_lt_sap_is_bs_req_valid(bs_req))
		return false;
	if (!ll_lt_sap_is_bs_ctx_valid(bs_ctx))
		return false;

	switch (event) {
	case WLAN_BS_SM_EV_SWITCH_TO_WLAN:
		ll_lt_sap_handle_bs_to_wlan_in_wlan_req_state(bs_ctx, bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN:
		ll_lt_sap_handle_bs_to_non_wlan_in_wlan_req_state(bs_ctx,
								  bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_WLAN_TIMEOUT:
		ll_lt_sap_handle_bs_to_wlan_timeout(bs_ctx, bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_WLAN_FAILURE:
		ll_lt_sap_handle_bs_to_wlan_failure(bs_ctx, bs_req);
		break;
	case WLAN_BS_SM_EV_SWITCH_TO_WLAN_COMPLETED:
		ll_lt_sap_handle_bs_to_wlan_completed(bs_ctx, bs_req);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

struct wlan_sm_state_info bs_sm_info[] = {
	{
		(uint8_t)BEARER_NON_WLAN,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"NON_WLAN",
		bs_state_non_wlan_entry,
		bs_state_non_wlan_exit,
		bs_state_non_wlan_event
	},
	{
		(uint8_t)BEARER_NON_WLAN_REQUESTED,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"NON_WLAN_REQUESTED",
		bs_state_non_wlan_req_entry,
		bs_state_non_wlan_req_exit,
		bs_state_non_wlan_req_event
	},
	{
		(uint8_t)BEARER_WLAN_REQUESTED,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"WLAN_REQUESTED",
		bs_state_wlan_req_entry,
		bs_state_wlan_req_exit,
		bs_state_wlan_req_event
	},
	{
		(uint8_t)BEARER_WLAN,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"WLAN",
		bs_state_wlan_entry,
		bs_state_wlan_exit,
		bs_state_wlan_event
	},
	{
		(uint8_t)BEARER_SWITCH_MAX,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"INVALID",
		NULL,
		NULL,
		NULL
	},
};

static const char *bs_sm_event_names[] = {
	"EV_SW_TO_WLAN",
	"EV_SW_TO_NON_WLAN",
	"EV_SW_TO_WLAN_TIMEOUT",
	"EV_SW_TO_NON_WLAN_TIMEOUT",
	"EV_SW_TO_WLAN_COMPLETED",
	"EV_SW_TO_NON_WLAN_COMPLETED",
	"EV_SW_TO_WLAN_FAILURE",
	"EV_SW_TO_NON_WLAN_FAILURE",
};

/**
 * bs_events_init() - Initialize events related to Bearer switch
 * @bs_ctx: Bearer switch context
 *
 * Return: None
 */
static void bs_events_init(struct bearer_switch_info *bs_ctx)
{
	qdf_event_create(&bs_ctx->p2p_go_bs_copletion_event);
}

/**
 * bs_events_deinit () - De-Initialize events related to Bearer switch
 * @bs_ctx: Bearer switch context
 *
 * Return: None
 */
static void bs_events_deinit(struct bearer_switch_info *bs_ctx)
{
	qdf_event_destroy(&bs_ctx->p2p_go_bs_copletion_event);
}

/**
 * bs_timers_init() - Initialize Bearer switch request timers
 * @bs_ctx: Bearer switch context
 *
 * Return: None
 */

static void bs_timers_init(struct bearer_switch_info *bs_ctx)
{
	qdf_mc_timer_init(&bs_ctx->bs_request_timer, QDF_TIMER_TYPE_SW,
			  bs_req_timeout_cb, bs_ctx);
	qdf_mc_timer_init(&bs_ctx->bs_wlan_request_timer, QDF_TIMER_TYPE_SW,
			  ll_lt_sap_continue_bs_to_wlan, bs_ctx);
}

/**
 * bs_timers_deinit() - De-initialize Bearer switch request timers
 * @bs_ctx: Bearer switch context
 *
 * Return: None
 */
static void bs_timers_deinit(struct bearer_switch_info *bs_ctx)
{
	if (QDF_TIMER_STATE_RUNNING ==
		qdf_mc_timer_get_current_state(&bs_ctx->bs_request_timer))
		qdf_mc_timer_stop(&bs_ctx->bs_request_timer);

	qdf_mc_timer_destroy(&bs_ctx->bs_request_timer);

	if (QDF_TIMER_STATE_RUNNING ==
		qdf_mc_timer_get_current_state(&bs_ctx->bs_wlan_request_timer))
		qdf_mc_timer_stop(&bs_ctx->bs_wlan_request_timer);

	qdf_mc_timer_destroy(&bs_ctx->bs_wlan_request_timer);
}

QDF_STATUS bs_sm_create(struct bearer_switch_info *bs_ctx)
{
	struct wlan_sm *sm;
	uint8_t name[WLAN_SM_ENGINE_MAX_NAME];

	qdf_scnprintf(name, sizeof(name), "BS_%d",
		      wlan_vdev_get_id(bs_ctx->vdev));
	sm = wlan_sm_create(name, bs_ctx,
			    BEARER_NON_WLAN,
			    bs_sm_info,
			    QDF_ARRAY_SIZE(bs_sm_info),
			    bs_sm_event_names,
			    QDF_ARRAY_SIZE(bs_sm_event_names));
	if (!sm) {
		ll_sap_err("Vdev %d BS_SM State Machine creation failed",
			   wlan_vdev_get_id(bs_ctx->vdev));
		return QDF_STATUS_E_NOMEM;
	}
	bs_ctx->sm.sm_hdl = sm;

	bs_lock_create(bs_ctx);

	bs_timers_init(bs_ctx);

	bs_events_init(bs_ctx);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS bs_sm_destroy(struct bearer_switch_info *bs_ctx)
{
	bs_events_deinit(bs_ctx);
	bs_timers_deinit(bs_ctx);
	bs_lock_destroy(bs_ctx);
	wlan_sm_delete(bs_ctx->sm.sm_hdl);

	return QDF_STATUS_SUCCESS;
}

enum wlan_bearer_switch_sm_state
bs_get_state(struct bearer_switch_info *bearer_switch_ctx)
{
	if (!bearer_switch_ctx || !bearer_switch_ctx->vdev)
		return BEARER_SWITCH_MAX;

	return bearer_switch_ctx->sm.bs_state;
}

QDF_STATUS bs_sm_deliver_event(struct wlan_objmgr_psoc *psoc,
			       enum wlan_bearer_switch_sm_evt event,
			       uint16_t data_len, void *data)
{
	QDF_STATUS status;
	struct bearer_switch_info *bearer_switch_ctx;
	struct wlan_objmgr_vdev *vdev;
	struct ll_sap_vdev_priv_obj *ll_sap_obj;
	uint8_t vdev_id;

	vdev_id = wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);
	if (vdev_id == WLAN_INVALID_VDEV_ID)
		return QDF_STATUS_E_INVAL;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id,
						    WLAN_LL_SAP_ID);
	if (!vdev)
		return QDF_STATUS_E_INVAL;

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

	if (!ll_sap_obj) {
		ll_sap_err("BS_SM vdev %d ll_sap obj null",
			   wlan_vdev_get_id(vdev));
		status = QDF_STATUS_E_INVAL;
		goto rel_ref;
	}

	bearer_switch_ctx = ll_sap_obj->bearer_switch_ctx;
	if (!bearer_switch_ctx) {
		status = QDF_STATUS_E_FAILURE;
		goto rel_ref;
	}

	bs_lock_acquire(bearer_switch_ctx);
	status = wlan_sm_dispatch(bearer_switch_ctx->sm.sm_hdl,
				  event, data_len, data);
	bs_lock_release(bearer_switch_ctx);

rel_ref:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_LL_SAP_ID);

	return status;
}

QDF_STATUS
ll_lt_sap_switch_bearer_to_ble(struct wlan_objmgr_psoc *psoc,
			       struct wlan_bearer_switch_request *bs_request)
{
	return bs_sm_deliver_event(psoc, WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN,
				   sizeof(*bs_request), bs_request);
}

QDF_STATUS
ll_lt_sap_switch_bearer_to_wlan(struct wlan_objmgr_psoc *psoc,
				struct wlan_bearer_switch_request *bs_request)
{
	return bs_sm_deliver_event(psoc, WLAN_BS_SM_EV_SWITCH_TO_WLAN,
				   sizeof(*bs_request), bs_request);
}

QDF_STATUS
ll_lt_sap_request_for_audio_transport_switch(
					struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_req_type req_type)
{
	struct ll_sap_vdev_priv_obj *ll_sap_obj;
	struct bearer_switch_info *bearer_switch_ctx;

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

	if (!ll_sap_obj) {
		ll_sap_err("BS_SM vdev %d ll_sap obj null",
			   wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_INVAL;
	}
	bearer_switch_ctx = ll_sap_obj->bearer_switch_ctx;
	if (!bearer_switch_ctx)
		return QDF_STATUS_E_INVAL;

	/*
	 * Request to switch to non-wlan can always be accepted so,
	 * always return success
	 */
	if (req_type == WLAN_BS_REQ_TO_NON_WLAN) {
		ll_sap_debug("BS_SM vdev %d WLAN_BS_REQ_TO_NON_WLAN accepted",
			     wlan_vdev_get_id(vdev));
		return QDF_STATUS_SUCCESS;
	} else if (req_type == WLAN_BS_REQ_TO_WLAN) {
		/*
		 * Total_ref_count zero indicates that no module wants to stay
		 * in non-wlan mode so this request can be accepted
		 */
		if (!qdf_atomic_read(&bearer_switch_ctx->total_ref_count) &&
		    (QDF_TIMER_STATE_RUNNING !=
			qdf_mc_timer_get_current_state(
				&bearer_switch_ctx->bs_wlan_request_timer))) {
			ll_sap_debug("BS_SM vdev %d WLAN_BS_REQ_TO_WLAN accepted",
				     wlan_vdev_get_id(vdev));
			return QDF_STATUS_SUCCESS;
		}
		ll_sap_debug("BS_SM vdev %d WLAN_BS_REQ_TO_WLAN rejected, total ref count %d timer state %d",
			     wlan_vdev_get_id(vdev),
			     qdf_atomic_read(&bearer_switch_ctx->total_ref_count),
			     qdf_mc_timer_get_current_state(
				&bearer_switch_ctx->bs_wlan_request_timer));

		return QDF_STATUS_E_FAILURE;
	}
	ll_sap_err("BS_SM vdev %d Invalid audio transport type %d",
		   wlan_vdev_get_id(vdev), req_type);

	return QDF_STATUS_E_INVAL;
}

/**
 * ll_lt_sap_deliver_wlan_audio_transport_switch_resp() - Deliver wlan
 * audio transport switch response to BS_SM
 * @vdev: ll_lt sap vdev
 * @status: Status of the request
 *
 * Return: None
 */
static void ll_lt_sap_deliver_wlan_audio_transport_switch_resp(
					struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_status status)
{
	struct wlan_bearer_switch_request *bs_request;
	struct ll_sap_vdev_priv_obj *ll_sap_obj;
	struct bearer_switch_info *bs_ctx;

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

	if (!ll_sap_obj) {
		ll_sap_err("BS_SM vdev %d ll_sap obj null",
			   wlan_vdev_get_id(vdev));
		return;
	}

	bs_ctx = ll_sap_obj->bearer_switch_ctx;
	if (!bs_ctx)
		return;

	bs_request = ll_lt_sap_find_first_valid_bs_wlan_req(bs_ctx);

	/*
	 * If bs_request is cached in the BS_SM, it means this is a response
	 * to the host driver's request of bearer switch so deliver the event
	 * to the BS_SM
	 * If there is no cached request in BS_SM, it means that some other
	 * module (other than wlan) has performed the bearer switch and it is
	 * not a response of the wlan module's bearer switch request.
	 */
	if (status == WLAN_BS_STATUS_COMPLETED)
		bs_sm_deliver_event(wlan_vdev_get_psoc(vdev),
				    WLAN_BS_SM_EV_SWITCH_TO_WLAN_COMPLETED,
				    sizeof(*bs_request), bs_request);
	else if (status == WLAN_BS_STATUS_REJECTED)
		bs_sm_deliver_event(wlan_vdev_get_psoc(vdev),
				    WLAN_BS_SM_EV_SWITCH_TO_WLAN_FAILURE,
				    sizeof(*bs_request), bs_request);
	else
		ll_sap_err("BS_SM vdev %d Invalid BS status %d",
			   wlan_vdev_get_id(vdev), status);
}

/**
 * ll_lt_sap_deliver_non_wlan_audio_transport_switch_resp() - Deliver non wlan
 * audio transport switch response to BS_SM
 * @vdev: ll_lt sap vdev
 * @status: Status of the request
 *
 * Return: None
 */
static void ll_lt_sap_deliver_non_wlan_audio_transport_switch_resp(
					struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_status status)
{
	struct wlan_bearer_switch_request *bs_request;
	struct ll_sap_vdev_priv_obj *ll_sap_obj;
	struct bearer_switch_info *bs_ctx;

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

	if (!ll_sap_obj) {
		ll_sap_err("BS_SM vdev %d ll_sap obj null",
			   wlan_vdev_get_id(vdev));
		return;
	}

	bs_ctx = ll_sap_obj->bearer_switch_ctx;
	if (!bs_ctx)
		return;

	bs_request = ll_lt_sap_find_first_valid_bs_non_wlan_req(bs_ctx);

	/*
	 * If bs_request is cached in the BS_SM, it means this is a response
	 * to the host driver's request of bearer switch so deliver the event
	 * to the BS_SM
	 */
	if (bs_request) {
		if (status == WLAN_BS_STATUS_COMPLETED)
			bs_sm_deliver_event(
				wlan_vdev_get_psoc(vdev),
				WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_COMPLETED,
				sizeof(*bs_request), bs_request);
		else if (status == WLAN_BS_STATUS_REJECTED)
			bs_sm_deliver_event(
				wlan_vdev_get_psoc(vdev),
				WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_FAILURE,
				sizeof(*bs_request), bs_request);
		else
			ll_sap_err(BS_PREFIX_FMT "Invalid BS status %d",
				   BS_PREFIX_REF(wlan_vdev_get_id(vdev),
						 bs_request->request_id),
				   status);

		return;
	}

	/*
	 * If there is no cached request in BS_SM, it means that some other
	 * module has performed the bearer switch and it is not a response of
	 * the wlan bearer switch request, so just update the current state of
	 * the state machine
	 */
	ll_sap_debug("Bearer switch for non-wlan module's request");

	bs_sm_state_update(bs_ctx, BEARER_NON_WLAN);
}

void
ll_lt_sap_deliver_audio_transport_switch_resp(
				struct wlan_objmgr_vdev *vdev,
				enum bearer_switch_req_type req_type,
				enum bearer_switch_status status)
{
	ll_lt_sap_deliver_audio_transport_switch_resp_to_fw(vdev, req_type,
							   status);

	if (req_type == WLAN_BS_REQ_TO_NON_WLAN)
		ll_lt_sap_deliver_non_wlan_audio_transport_switch_resp(
								vdev,
								status);

	else if (req_type == WLAN_BS_REQ_TO_WLAN) {
		struct ll_sap_vdev_priv_obj *ll_sap_obj;
		struct bearer_switch_info *bs_ctx;

		ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

		if (!ll_sap_obj) {
			ll_sap_err("BS_SM vdev %d ll_sap obj null",
				   wlan_vdev_get_id(vdev));
			return;
		}

		bs_ctx = ll_sap_obj->bearer_switch_ctx;
		if (!bs_ctx)
			return;
		/* If any module wants to be in non-wlan bearer then
		 * dont update the state of the BS state machine, this will
		 * be updated when the respective module will decrement its
		 * reference count.
		 */
		if (qdf_atomic_read(&bs_ctx->total_ref_count)) {
			ll_sap_debug("Reject bearer switch to wlan, ref_count %d",
				      qdf_atomic_read(&bs_ctx->total_ref_count));
			return;
		}
		ll_lt_sap_deliver_wlan_audio_transport_switch_resp(
								vdev,
								status);
	}
	else
		ll_sap_err("Vdev %d Invalid req_type %d ",
			   wlan_vdev_get_id(vdev), req_type);
}

static void p2p_go_start_bearer_switch_requester_cb(struct wlan_objmgr_psoc *psoc,
						    uint8_t vdev_id,
						    wlan_bs_req_id request_id,
						    QDF_STATUS status,
						    uint32_t req_value,
						    void *request_params)
{
	struct bearer_switch_info *bearer_switch_ctx = request_params;

	qdf_event_set(&bearer_switch_ctx->p2p_go_bs_copletion_event);
}

static void p2p_go_complete_bearer_switch_requester_cb(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						wlan_bs_req_id request_id,
						QDF_STATUS status,
						uint32_t req_value,
						void *request_params)
{
	/* Drop this response as no action is required */
}

QDF_STATUS
ll_lt_sap_switch_bearer(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
			enum bearer_switch_req_type req_type,
			enum bearer_switch_req_source source,
			bearer_switch_requester_cb requester_cb,
			uint32_t arg_value, void *arg)
{
	struct wlan_bearer_switch_request bs_request = {0};
	QDF_STATUS status;

	bs_request.vdev_id = vdev_id;
	bs_request.request_id = ll_lt_sap_bearer_switch_get_id(psoc);
	bs_request.req_type = req_type;
	bs_request.source = source;
	bs_request.requester_cb = requester_cb;
	bs_request.arg_value = arg_value;
	bs_request.arg = arg;

	if (bs_request.req_type == WLAN_BS_REQ_TO_NON_WLAN)
		status = ll_lt_sap_switch_bearer_to_ble(psoc, &bs_request);
	else if (bs_request.req_type == WLAN_BS_REQ_TO_WLAN)
		status = ll_lt_sap_switch_bearer_to_wlan(psoc, &bs_request);
	else
		status = QDF_STATUS_E_INVAL;

	if (QDF_IS_STATUS_ERROR(status))
		ll_sap_err("vdev %d beaer_switch failed for req_type %d source %d status %d",
			   bs_request.vdev_id, bs_request.req_type,
			   bs_request.source, status);

	return status;
}

void ll_lt_sap_switch_bearer_for_p2p_go_start(struct wlan_objmgr_psoc *psoc,
					      uint8_t vdev_id,
					      qdf_freq_t oper_freq,
					      enum QDF_OPMODE device_mode)
{
	qdf_freq_t ll_lt_sap_freq = 0;
	QDF_STATUS status;
	struct bearer_switch_info *bearer_switch_ctx;
	struct ll_sap_vdev_priv_obj *ll_sap_obj;
	uint8_t ll_lt_sap_vdev_id;
	struct wlan_objmgr_vdev *ll_lt_sap_vdev;

	if (device_mode != QDF_P2P_GO_MODE)
		return;

	ll_lt_sap_vdev_id = wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);
	if (ll_lt_sap_vdev_id == WLAN_INVALID_VDEV_ID)
		return;

	ll_lt_sap_vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc,
							      ll_lt_sap_vdev_id,
							      WLAN_LL_SAP_ID);
	if (!ll_lt_sap_vdev)
		return;

	ll_sap_obj = ll_sap_get_vdev_priv_obj(ll_lt_sap_vdev);

	if (!ll_sap_obj) {
		ll_sap_err("BS_SM vdev %d ll_sap obj null", ll_lt_sap_vdev_id);
		goto rel_ref;
	}

	bearer_switch_ctx = ll_sap_obj->bearer_switch_ctx;

	if (!bearer_switch_ctx)
		goto rel_ref;

	ll_lt_sap_freq = policy_mgr_get_ll_lt_sap_freq(psoc);

	if (!ll_lt_sap_freq)
		goto rel_ref;

	/*If P2P_GO is not coming in MCC, then do not do the bearer switch */
	if (!policy_mgr_2_freq_always_on_same_mac(psoc, oper_freq,
						  ll_lt_sap_freq))
		goto rel_ref;

	ll_sap_debug("P2P GO freq %d on same mac with ll_lt sap %d", oper_freq,
		     ll_lt_sap_freq);

	status = ll_lt_sap_switch_bearer(
				psoc, vdev_id, WLAN_BS_REQ_TO_NON_WLAN,
				BEARER_SWITCH_REQ_P2P_GO,
				p2p_go_start_bearer_switch_requester_cb,
				0, bearer_switch_ctx);

	if (QDF_IS_STATUS_ERROR(status))
		goto rel_ref;

	/*
	 * Wait for bearer switch completion here as start bss should complete
	 * in the same context
	 */
	qdf_event_reset(&bearer_switch_ctx->p2p_go_bs_copletion_event);
	status = qdf_wait_for_event_completion(
				&bearer_switch_ctx->p2p_go_bs_copletion_event,
				BEARER_SWITCH_TIMEOUT);
	if (QDF_IS_STATUS_ERROR(status))
		ll_sap_err("bearer switch to non-wlan failed for vdev %d",
			   vdev_id);
rel_ref:
	wlan_objmgr_vdev_release_ref(ll_lt_sap_vdev, WLAN_LL_SAP_ID);
}

void ll_lt_sap_switch_bearer_on_p2p_go_complete(struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						enum QDF_OPMODE device_mode)
{
	uint8_t ll_lt_sap_vdev_id;

	if (device_mode != QDF_P2P_GO_MODE)
		return;

	ll_lt_sap_vdev_id = wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);
	/* LL_LT SAP is not present, bearer switch is not required */
	if (ll_lt_sap_vdev_id == WLAN_INVALID_VDEV_ID)
		return;

	ll_lt_sap_switch_bearer(psoc, vdev_id, WLAN_BS_REQ_TO_WLAN,
				BEARER_SWITCH_REQ_P2P_GO,
				p2p_go_complete_bearer_switch_requester_cb,
				0, NULL);
}
