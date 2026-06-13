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

#include "wmi_unified.h"
#include "wmi_unified_param.h"
#include <wmi_unified_priv.h>
#include <wmi.h>
#include "wmi_unified_ll_sap_tlv.h"

static WMI_AUDIO_TRANSPORT_SWITCH_RESPONSE_STATUS
wmi_convert_host_to_target_audio_switch_status(enum bearer_switch_status status)
{
	if (status == WLAN_BS_STATUS_COMPLETED)
		return WMI_AUDIO_TRANSPORT_SWITCH_STATUS_SUCCESS;
	else if (status == WLAN_BS_STATUS_REJECTED)
		return WMI_AUDIO_TRANSPORT_SWITCH_STATUS_FAIL;
	else
		return WMI_AUDIO_TRANSPORT_SWITCH_STATUS_TIMEOUT;
}

static WMI_AUDIO_TRANSPORT_SWITCH_TYPE
wmi_convert_host_to_target_audio_switch_type(
					enum bearer_switch_req_type req_type)
{
	if (req_type == WLAN_BS_REQ_TO_NON_WLAN)
		return WMI_AUDIO_TRANSPORT_SWITCH_TYPE_NON_WLAN;
	else
		return WMI_AUDIO_TRANSPORT_SWITCH_TYPE_WLAN;
}

static enum bearer_switch_req_type
wmi_convert_target_to_host_audio_switch_type(uint32_t req_type)
{
	if (req_type == WMI_AUDIO_TRANSPORT_SWITCH_TYPE_NON_WLAN)
		return WLAN_BS_REQ_TO_NON_WLAN;
	else if (req_type == WMI_AUDIO_TRANSPORT_SWITCH_TYPE_WLAN)
		return WLAN_BS_REQ_TO_WLAN;
	else
		return WLAN_BS_REQ_INVALID;
}

static VDEV_OOB_CONNECT_REQ_RESP_TYPE
wmi_convert_host_to_target_connect_req_type(
			enum high_ap_availability_operation req_type)
{
	if (req_type == HIGH_AP_AVAILABILITY_OPERATION_REQUEST)
		return VDEV_OOB_CONNECT_REQUEST;
	else
		return VDEV_OOB_CONNECT_CANCEL;
}

static enum high_ap_availability_operation
wmi_convert_target_to_host_connect_resp_type(
					VDEV_OOB_CONNECT_REQ_RESP_TYPE req_type)
{
	if (req_type == VDEV_OOB_CONNECT_STARTED)
		return HIGH_AP_AVAILABILITY_OPERATION_STARTED;
	else if (req_type == VDEV_OOB_CONNECT_COMPLETED)
		return HIGH_AP_AVAILABILITY_OPERATION_COMPLETED;
	else if (req_type == VDEV_OOB_CONNECT_CANCELLED)
		return HIGH_AP_AVAILABILITY_OPERATION_CANCELLED;
	else
		return HiGH_AP_AVAILABILITY_OPERATION_INVALID;
}

QDF_STATUS audio_transport_switch_resp_tlv(
					wmi_unified_t wmi_hdl,
					enum bearer_switch_req_type req_type,
					enum bearer_switch_status status)
{
	wmi_audio_transport_switch_resp_status_cmd_fixed_param *cmd;
	wmi_buf_t buf;
	QDF_STATUS qdf_status;

	buf = wmi_buf_alloc(wmi_hdl, sizeof(*cmd));
	if (!buf)
		return QDF_STATUS_E_FAILURE;

	cmd = (wmi_audio_transport_switch_resp_status_cmd_fixed_param *)wmi_buf_data(buf);

	WMITLV_SET_HDR(
	&cmd->tlv_header,
	WMITLV_TAG_STRUC_wmi_audio_transport_switch_resp_status_cmd_fixed_param,
	WMITLV_GET_STRUCT_TLVLEN
	(wmi_audio_transport_switch_resp_status_cmd_fixed_param));

	cmd->switch_type = wmi_convert_host_to_target_audio_switch_type(
								req_type);
	cmd->switch_response_status =
		wmi_convert_host_to_target_audio_switch_status(status);

	wmi_nofl_debug("LL_LT_SAP Audio switch type %d status %d",
		       cmd->switch_type, cmd->switch_response_status);

	qdf_status = wmi_unified_cmd_send(
				wmi_hdl, buf, sizeof(*cmd),
				WMI_AUDIO_TRANSPORT_SWITCH_RESP_STATUS_CMDID);

	if (QDF_IS_STATUS_ERROR(qdf_status))
		wmi_buf_free(buf);

	return qdf_status;
}

QDF_STATUS
extract_audio_transport_switch_req_event_tlv(
				wmi_unified_t wmi_handle,
				uint8_t *event, uint32_t len,
				enum bearer_switch_req_type *req_type)
{
	WMI_AUDIO_TRANSPORT_SWITCH_TYPE_EVENTID_param_tlvs *param_buf = NULL;
	wmi_audio_transport_switch_type_event_fixed_param *fixed_param = NULL;

	if (!event || !len) {
		wmi_debug("Empty transport switch request event");
		return QDF_STATUS_E_FAILURE;
	}

	param_buf = (WMI_AUDIO_TRANSPORT_SWITCH_TYPE_EVENTID_param_tlvs *)event;
	if (!param_buf) {
		wmi_err("received null buf from target");
		return QDF_STATUS_E_INVAL;
	}

	fixed_param = (wmi_audio_transport_switch_type_event_fixed_param *)
					param_buf->fixed_param;
	if (!fixed_param) {
		wmi_err("received null event data from target");
		return QDF_STATUS_E_INVAL;
	}

	*req_type = wmi_convert_target_to_host_audio_switch_type(
						fixed_param->switch_type);

	wmi_nofl_debug("LL_LT_SAP FW requested bearer switch to %d", *req_type);

	if (*req_type == WLAN_BS_REQ_INVALID)
		return QDF_STATUS_E_INVAL;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS oob_connect_request_tlv(wmi_unified_t wmi_hdl,
				   struct wmi_oob_connect_request request)
{
	wmi_vdev_oob_connection_req_cmd_fixed_param *cmd;
	wmi_buf_t buf;
	QDF_STATUS qdf_status;

	buf = wmi_buf_alloc(wmi_hdl, sizeof(*cmd));
	if (!buf)
		return QDF_STATUS_E_FAILURE;

	cmd = (wmi_vdev_oob_connection_req_cmd_fixed_param *)wmi_buf_data(buf);

	WMITLV_SET_HDR(
	&cmd->tlv_header,
	WMITLV_TAG_STRUC_wmi_vdev_oob_connection_req_cmd_fixed_param,
	WMITLV_GET_STRUCT_TLVLEN
	(wmi_vdev_oob_connection_req_cmd_fixed_param));

	cmd->connect_req_type = wmi_convert_host_to_target_connect_req_type(
						request.connect_req_type);
	cmd->vdev_id = request.vdev_id;
	cmd->vdev_available_duration = request.vdev_available_duration;

	wmi_nofl_debug("LL_LT_SAP vdev %d OOB connect req type %d duration %d",
		       cmd->vdev_id, cmd->connect_req_type,
		       cmd->vdev_available_duration);

	qdf_status = wmi_unified_cmd_send(wmi_hdl, buf, sizeof(*cmd),
					  WMI_VDEV_OOB_CONNECTION_REQ_CMDID);

	if (QDF_IS_STATUS_ERROR(qdf_status))
		wmi_buf_free(buf);

	return qdf_status;
}

QDF_STATUS extract_oob_connect_response_event_tlv(
			wmi_unified_t wmi_handle,
			uint8_t *event, uint32_t len,
			struct wmi_oob_connect_response_event *response)
{
	WMI_VDEV_OOB_CONNECTION_RESP_EVENTID_param_tlvs *param_buf = NULL;
	wmi_vdev_oob_connection_resp_event_fixed_param *fixed_param = NULL;

	if (!event || !len) {
		wmi_debug("Empty transport switch request event");
		return QDF_STATUS_E_FAILURE;
	}

	param_buf = (WMI_VDEV_OOB_CONNECTION_RESP_EVENTID_param_tlvs *)event;
	if (!param_buf) {
		wmi_err("received null buf from target");
		return QDF_STATUS_E_INVAL;
	}

	fixed_param = (wmi_vdev_oob_connection_resp_event_fixed_param *)
					param_buf->fixed_param;
	if (!fixed_param) {
		wmi_err("received null event data from target");
		return QDF_STATUS_E_INVAL;
	}

	response->connect_resp_type =
			wmi_convert_target_to_host_connect_resp_type(
						fixed_param->connect_resp_type);
	response->vdev_id = fixed_param->vdev_id;

	wmi_nofl_debug("LL_LT_SAP FW vdev %d OOB connect response %d",
		       response->vdev_id, response->connect_resp_type);

	if (response->connect_resp_type ==
					HiGH_AP_AVAILABILITY_OPERATION_INVALID)
		return QDF_STATUS_E_INVAL;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS get_tsf_stats_for_csa_tlv(wmi_unified_t wmi_hdl, uint8_t vdev_id)
{
	wmi_vdev_get_twt_session_stats_info_cmd_fixed_param *cmd;
	wmi_buf_t buf;
	QDF_STATUS qdf_status;

	buf = wmi_buf_alloc(wmi_hdl, sizeof(*cmd));
	if (!buf)
		return QDF_STATUS_E_FAILURE;

	cmd = (wmi_vdev_get_twt_session_stats_info_cmd_fixed_param *)wmi_buf_data(buf);

	WMITLV_SET_HDR(&cmd->tlv_header,
	WMITLV_TAG_STRUC_wmi_vdev_get_twt_session_stats_info_cmd_fixed_param,
	WMITLV_GET_STRUCT_TLVLEN
	(wmi_vdev_get_twt_session_stats_info_cmd_fixed_param));

	cmd->vdev_id = vdev_id;

	wmi_nofl_debug("get tsf stats for vdev_id %d", cmd->vdev_id);

	qdf_status = wmi_unified_cmd_send(
				wmi_hdl, buf, sizeof(*cmd),
				WMI_VDEV_GET_TWT_SESSION_STATS_INFO_CMDID);

	if (QDF_IS_STATUS_ERROR(qdf_status))
		wmi_buf_free(buf);

	return qdf_status;
}
