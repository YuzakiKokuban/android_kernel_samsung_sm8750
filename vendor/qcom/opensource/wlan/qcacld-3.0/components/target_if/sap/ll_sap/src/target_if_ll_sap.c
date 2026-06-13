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

#include "target_if.h"
#include "target_if_ll_sap.h"
#include "wlan_ll_sap_public_structs.h"
#include "wmi_unified_ll_sap_api.h"
#include "../../umac/mlme/sap/ll_sap/core/src/wlan_ll_sap_main.h"
#include "wlan_ll_sap_api.h"

/**
 * target_if_send_oob_connect_request() - Send OOB connect request to FW
 * @psoc: pointer to psoc
 * @req: OOB connect request
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS target_if_send_oob_connect_request(
					struct wlan_objmgr_psoc *psoc,
					struct ll_sap_oob_connect_request req)
{
	struct wmi_unified *wmi_handle;
	struct wmi_oob_connect_request request;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("wmi_handle is null.");
		return QDF_STATUS_E_NULL_VALUE;
	}

	request.vdev_id = req.vdev_id;
	request.vdev_available_duration = req.vdev_available_duration;
	request.connect_req_type = req.connect_req_type;

	return wmi_unified_oob_connect_request_send(wmi_handle, request);
}

static int target_if_oob_connect_response_event_handler(ol_scn_t scn,
							uint8_t *event,
							uint32_t len)
{
	struct wlan_objmgr_psoc *psoc;
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;
	struct wmi_unified *wmi_handle;
	struct wlan_ll_sap_rx_ops *rx_ops;
	QDF_STATUS qdf_status;
	struct wmi_oob_connect_response_event response;
	struct ll_sap_oob_connect_response_event ll_sap_response;

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		target_if_err("psoc is null");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("wmi_handle is null");
		return -EINVAL;
	}

	psoc_ll_sap_obj = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_LL_SAP);

	if (!psoc_ll_sap_obj) {
		target_if_err("psoc_ll_sap_obj is null");
		return -EINVAL;
	}

	rx_ops = &psoc_ll_sap_obj->rx_ops;
	if (!rx_ops || !rx_ops->oob_connect_response) {
		target_if_err("Invalid ll_sap rx ops");
		return -EINVAL;
	}

	qdf_status = wmi_extract_oob_connect_response_event(wmi_handle, event,
							    len, &response);
	if (QDF_IS_STATUS_ERROR(qdf_status)) {
		target_if_err("parsing of event failed, %d", qdf_status);
		return -EINVAL;
	}

	ll_sap_response.vdev_id = response.vdev_id;
	ll_sap_response.connect_resp_type = response.connect_resp_type;

	rx_ops->oob_connect_response(psoc, ll_sap_response);

	return 0;
}

/**
 * target_if_send_audio_transport_switch_resp() - Send audio transport switch
 * response to fw
 * @psoc: pointer to psoc
 * @req_type: Bearer switch request type
 * @status: Status of the bearer switch request
 *
 * Return: pointer to tx ops
 */
static QDF_STATUS target_if_send_audio_transport_switch_resp(
					struct wlan_objmgr_psoc *psoc,
					enum bearer_switch_req_type req_type,
					enum bearer_switch_status status)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("wmi_handle is null.");
		return QDF_STATUS_E_NULL_VALUE;
	}

	return wmi_unified_audio_transport_switch_resp_send(wmi_handle,
							    req_type, status);
}

static int target_if_send_audio_transport_switch_req_event_handler(
							ol_scn_t scn,
							uint8_t *event,
							uint32_t len)
{
	struct wlan_objmgr_psoc *psoc;
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;
	enum bearer_switch_req_type req_type;
	struct wmi_unified *wmi_handle;
	struct wlan_ll_sap_rx_ops *rx_ops;
	QDF_STATUS qdf_status;

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		target_if_err("psoc is null");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("wmi_handle is null");
		return -EINVAL;
	}

	psoc_ll_sap_obj = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_LL_SAP);

	if (!psoc_ll_sap_obj) {
		target_if_err("psoc_ll_sap_obj is null");
		return -EINVAL;
	}

	rx_ops = &psoc_ll_sap_obj->rx_ops;
	if (!rx_ops || !rx_ops->audio_transport_switch_req) {
		target_if_err("Invalid ll_sap rx ops");
		return -EINVAL;
	}

	qdf_status = wmi_extract_audio_transport_switch_req_event(wmi_handle,
								  event, len,
								  &req_type);
	if (QDF_IS_STATUS_ERROR(qdf_status)) {
		target_if_err("parsing of event failed, %d", qdf_status);
		return -EINVAL;
	}
	rx_ops->audio_transport_switch_req(psoc, req_type);

	return 0;
}

/* Start TSF timer value */
#define START_TSF_TIMER_TIMEOUT 1000
/**
 * target_if_get_tsf_stats_for_csa() - Get tsf stats for ll_sap csa
 * @psoc: pointer to psoc
 * @vdev_id: vdev id
 *
 * Return: QDF_STATUS
 */
static
QDF_STATUS target_if_get_tsf_stats_for_csa(struct wlan_objmgr_psoc *psoc,
					   uint8_t vdev_id)
{
	struct wmi_unified *wmi_handle;
	struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj;
	QDF_STATUS status;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("wmi_handle is null.");
		return QDF_STATUS_E_NULL_VALUE;
	}

	ll_sap_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_LL_SAP);

	if (!ll_sap_psoc_obj) {
		target_if_err("psoc_ll_sap_obj is null");
		return QDF_STATUS_E_FAILURE;
	}

	status = qdf_mc_timer_start(&ll_sap_psoc_obj->tsf_timer,
				    START_TSF_TIMER_TIMEOUT);
	if (QDF_IS_STATUS_ERROR(status)) {
		ll_sap_err("Failed to start tsf timer");
		return status;
	}

	ll_sap_psoc_obj->timer_vdev_id = vdev_id;

	status = wmi_unified_get_tsf_stats_for_csa(wmi_handle, vdev_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		ll_sap_err("Failed to get tsf stats for csa");
		if (QDF_TIMER_STATE_RUNNING == qdf_mc_timer_get_current_state(
						&ll_sap_psoc_obj->tsf_timer))
			qdf_mc_timer_stop(&ll_sap_psoc_obj->tsf_timer);
	}

	return status;
}

bool target_if_ll_sap_is_twt_event_type_query_rsp(
				struct wlan_objmgr_psoc *psoc,
				uint8_t *evt_buf,
				struct twt_session_stats_event_param *params,
				struct twt_session_stats_info *twt_params)
{
	struct wmi_unified *wmi_hdl;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	wmi_hdl = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_hdl) {
		target_if_err("wmi_handle is null!");
		return QDF_STATUS_E_FAILURE;
	}

	status = wmi_extract_twt_session_stats_data(
				wmi_hdl, evt_buf,
				params, twt_params, 0);

	if (QDF_IS_STATUS_ERROR(status)) {
		target_if_err("Unable to extract twt params");
		return false;
	}

	if (twt_params->event_type == HOST_TWT_SESSION_QUERY_RSP)
		return true;

	return false;
}

QDF_STATUS target_if_ll_sap_continue_csa_after_tsf_rsp(
				struct wlan_objmgr_psoc *psoc,
				struct twt_session_stats_info *twt_params)
{
	struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj;
	struct ll_sap_csa_tsf_rsp *rsp;
	struct scheduler_msg msg = {0};
	QDF_STATUS status;

	ll_sap_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
						psoc,
						WLAN_UMAC_COMP_LL_SAP);

	if (!ll_sap_psoc_obj) {
		target_if_err("psoc_ll_sap_obj is null");
		return QDF_STATUS_E_FAILURE;
	}

	rsp = qdf_mem_malloc(sizeof(*rsp));
	if (!rsp)
		return QDF_STATUS_E_NOMEM;

	rsp->psoc = psoc;
	qdf_mem_copy(&rsp->twt_params, twt_params,
		     sizeof(struct twt_session_stats_info));

	msg.bodyptr = rsp;
	msg.callback = wlan_ll_lt_sap_continue_csa_after_tsf_rsp;
	msg.flush_callback = NULL;
	status = scheduler_post_message(QDF_MODULE_ID_TARGET_IF,
					QDF_MODULE_ID_LL_SAP,
					QDF_MODULE_ID_TARGET_IF, &msg);

	if (QDF_IS_STATUS_ERROR(status)) {
		target_if_err("vdev_id %d failed to post tsf stats rsp",
			      twt_params->vdev_id);
		qdf_mem_free(rsp);
	}

	return status;
}

void
target_if_ll_sap_register_tx_ops(struct wlan_ll_sap_tx_ops *tx_ops)
{
	tx_ops->send_audio_transport_switch_resp =
		target_if_send_audio_transport_switch_resp;
	tx_ops->send_oob_connect_request = target_if_send_oob_connect_request;
	tx_ops->get_tsf_stats_for_csa =	target_if_get_tsf_stats_for_csa;
}

void
target_if_ll_sap_register_rx_ops(struct wlan_ll_sap_rx_ops *rx_ops)
{
	rx_ops->audio_transport_switch_req =
		wlan_ll_sap_fw_bearer_switch_req;
	rx_ops->oob_connect_response =
		wlan_ll_sap_oob_connect_response;
}

QDF_STATUS
target_if_ll_sap_register_events(struct wlan_objmgr_psoc *psoc)
{
	QDF_STATUS ret;
	wmi_unified_t handle = get_wmi_unified_hdl_from_psoc(psoc);

	if (!handle) {
		target_if_err("handle is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	ret = wmi_unified_register_event_handler(
			handle,
			wmi_audio_transport_switch_type_event_id,
			target_if_send_audio_transport_switch_req_event_handler,
			WMI_RX_SERIALIZER_CTX);
	if (QDF_IS_STATUS_ERROR(ret)) {
		target_if_err("audio_transport_switch_req_event_handler registration failed");
		return ret;
	}

	ret = wmi_unified_register_event_handler(
			handle,
			wmi_vdev_oob_connection_response_event_id,
			target_if_oob_connect_response_event_handler,
			WMI_RX_SERIALIZER_CTX);
	if (QDF_IS_STATUS_ERROR(ret))
		target_if_err("oob_connect_response_event_handler registration failed");

	return ret;
}

QDF_STATUS
target_if_ll_sap_deregister_events(struct wlan_objmgr_psoc *psoc)
{
	QDF_STATUS ret;
	wmi_unified_t handle = get_wmi_unified_hdl_from_psoc(psoc);

	if (!handle) {
		target_if_err("handle is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	ret = wmi_unified_unregister_event_handler(
			handle,
			wmi_audio_transport_switch_type_event_id);

	ret = wmi_unified_unregister_event_handler(
			handle,
			wmi_vdev_oob_connection_response_event_id);

	return ret;
}
