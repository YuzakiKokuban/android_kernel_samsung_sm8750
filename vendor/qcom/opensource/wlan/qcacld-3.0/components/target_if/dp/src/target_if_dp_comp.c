/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * DOC: target_if_dp_comp.c
 */

#include "target_if_dp_comp.h"
#include "target_if.h"
#include "qdf_status.h"
#include "wmi.h"
#include "wmi_unified_api.h"
#include "wmi_unified_priv.h"
#include "wmi_unified_param.h"
#include "wlan_objmgr_psoc_obj.h"
#include "wlan_dp_public_struct.h"
#include "cdp_txrx_cmn.h"
#include "cdp_txrx_ops.h"
#include "wlan_dp_main.h"
#include <wlan_cm_api.h>

/**
 * target_if_dp_get_arp_stats_event_handler() - arp stats event handler
 * @scn: scn
 * @data: buffer with event
 * @datalen: buffer length
 *
 * Return: Return: 0 on success, failure code otherwise.
 */
static int
target_if_dp_get_arp_stats_event_handler(ol_scn_t scn, uint8_t *data,
					 uint32_t datalen)
{
	WMI_VDEV_GET_ARP_STAT_EVENTID_param_tlvs *param_buf;
	wmi_vdev_get_arp_stats_event_fixed_param *data_event;
	wmi_vdev_get_connectivity_check_stats *connect_stats_event;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_dp_psoc_nb_ops *nb_ops;
	uint8_t *buf_ptr;
	struct dp_rsp_stats rsp = {0};

	if (!scn || !data) {
		dp_err("scn: 0x%pK, data: 0x%pK", scn, data);
		return -EINVAL;
	}
	param_buf = (WMI_VDEV_GET_ARP_STAT_EVENTID_param_tlvs *)data;
	if (!param_buf) {
		dp_err("Invalid get arp stats event");
		return -EINVAL;
	}
	data_event = param_buf->fixed_param;
	if (!data_event) {
		dp_err("Invalid get arp stats data event");
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		dp_err("null psoc");
		return -EINVAL;
	}

	nb_ops = dp_intf_get_rx_ops(psoc);
	if (!nb_ops) {
		dp_err("null tx ops");
		return -EINVAL;
	}

	rsp.arp_req_enqueue = data_event->arp_req_enqueue;
	rsp.vdev_id = data_event->vdev_id;
	rsp.arp_req_tx_success = data_event->arp_req_tx_success;
	rsp.arp_req_tx_failure = data_event->arp_req_tx_failure;
	rsp.arp_rsp_recvd = data_event->arp_rsp_recvd;
	rsp.out_of_order_arp_rsp_drop_cnt =
		data_event->out_of_order_arp_rsp_drop_cnt;
	rsp.dad_detected = data_event->dad_detected;
	rsp.connect_status = data_event->connect_status;
	rsp.ba_session_establishment_status =
		data_event->ba_session_establishment_status;

	buf_ptr = (uint8_t *)data_event;
	buf_ptr = buf_ptr + sizeof(wmi_vdev_get_arp_stats_event_fixed_param) +
		  WMI_TLV_HDR_SIZE;
	connect_stats_event = (wmi_vdev_get_connectivity_check_stats *)buf_ptr;

	if (((connect_stats_event->tlv_header & 0xFFFF0000) >> 16 ==
	      WMITLV_TAG_STRUC_wmi_vdev_get_connectivity_check_stats)) {
		rsp.connect_stats_present = true;
		rsp.tcp_ack_recvd = connect_stats_event->tcp_ack_recvd;
		rsp.icmpv4_rsp_recvd = connect_stats_event->icmpv4_rsp_recvd;
		dp_debug("tcp_ack_recvd %d icmpv4_rsp_recvd %d",
			 connect_stats_event->tcp_ack_recvd,
			 connect_stats_event->icmpv4_rsp_recvd);
	}

	nb_ops->osif_dp_get_arp_stats_evt(psoc, &rsp);

	return QDF_STATUS_SUCCESS;
}

/**
 * target_if_dp_arp_stats_register_event_handler() - register event handler
 * @psoc: psoc handle
 *
 * Return: Return: 0 on success, failure code otherwise.
 */
static QDF_STATUS
target_if_dp_arp_stats_register_event_handler(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;
	QDF_STATUS ret_val;

	if (!psoc) {
		dp_err("PSOC is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		dp_err("wmi_handle is null");
		return QDF_STATUS_E_INVAL;
	}

	ret_val = wmi_unified_register_event_handler(wmi_handle,
				wmi_get_arp_stats_req_id,
				target_if_dp_get_arp_stats_event_handler,
				WMI_RX_WORK_CTX);
	if (QDF_IS_STATUS_ERROR(ret_val))
		dp_err("Failed to register event_handler");

	return QDF_STATUS_SUCCESS;
}

/**
 * target_if_dp_arp_stats_unregister_event_handler() - unregister event handler
 * @psoc: psoc handle
 *
 * Return: Return: 0 on success, failure code otherwise.
 */
static QDF_STATUS
target_if_dp_arp_stats_unregister_event_handler(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;

	if (!psoc) {
		dp_err("PSOC is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		dp_err("wmi_handle is null");
		return QDF_STATUS_E_INVAL;
	}

	wmi_unified_unregister_event_handler(wmi_handle,
					     wmi_get_arp_stats_req_id);
	return QDF_STATUS_SUCCESS;
}

/**
 * target_if_dp_get_arp_req_stats() - send get arp stats request command to fw
 * @psoc: psoc handle
 * @req_buf: get arp stats request buffer
 *
 * Return: Return: 0 on success, failure code otherwise.
 */
static QDF_STATUS
target_if_dp_get_arp_req_stats(struct wlan_objmgr_psoc *psoc,
			       struct dp_get_arp_stats_params *req_buf)
{
	QDF_STATUS status;
	struct get_arp_stats *arp_stats;
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_vdev *vdev;

	if (!psoc) {
		dp_err("PSOC is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		dp_err("wmi_handle is null");
		return QDF_STATUS_E_INVAL;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc,
						    req_buf->vdev_id,
						    WLAN_DP_ID);
	if (!vdev) {
		dp_err("Can't get vdev by vdev_id:%d", req_buf->vdev_id);
		return QDF_STATUS_E_INVAL;
	}
	if (!wlan_cm_is_vdev_active(vdev)) {
		dp_debug("vdev id:%d is not started", req_buf->vdev_id);
		status = QDF_STATUS_E_INVAL;
		goto release_ref;
	}

	arp_stats = (struct get_arp_stats *)req_buf;
	status = wmi_unified_get_arp_stats_req(wmi_handle, arp_stats);
	if (QDF_IS_STATUS_ERROR(status))
		dp_err("failed to send get arp stats to FW");
release_ref:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_DP_ID);
	return status;
}

/**
 * target_if_dp_set_arp_req_stats() - send set arp stats request command to fw
 * @psoc: psoc handle
 * @req_buf: set srp stats request buffer
 *
 * Return: Return: 0 on success, failure code otherwise.
 */
static QDF_STATUS
target_if_dp_set_arp_req_stats(struct wlan_objmgr_psoc *psoc,
			       struct dp_set_arp_stats_params *req_buf)
{
	QDF_STATUS status;
	struct set_arp_stats *arp_stats;
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_vdev *vdev;

	if (!psoc) {
		dp_err("PSOC is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		dp_err("wmi_handle is null");
		return QDF_STATUS_E_INVAL;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc,
						    req_buf->vdev_id,
						    WLAN_DP_ID);
	if (!vdev) {
		dp_err("Can't get vdev by vdev_id:%d", req_buf->vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	status = wlan_vdev_is_up(vdev);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("vdev id:%d is not started", req_buf->vdev_id);
		status = QDF_STATUS_E_INVAL;
		goto release_ref;
	}
	arp_stats = (struct set_arp_stats *)req_buf;
	status = wmi_unified_set_arp_stats_req(wmi_handle, arp_stats);
	if (QDF_IS_STATUS_ERROR(status))
		dp_err("failed to set arp stats to FW");

release_ref:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_DP_ID);
	return status;
}

/**
 * target_if_dp_lro_config_cmd() - process the LRO config command
 * @psoc: Pointer to psoc handle
 * @dp_lro_cmd: Pointer to LRO configuration parameters
 *
 * This function sends down the LRO configuration parameters to
 * the firmware to enable LRO, sets the TCP flags and sets the
 * seed values for the toeplitz hash generation
 *
 * Return: QDF_STATUS_SUCCESS for success otherwise failure
 */
static QDF_STATUS
target_if_dp_lro_config_cmd(struct wlan_objmgr_psoc *psoc,
			    struct cdp_lro_hash_config *dp_lro_cmd)
{
	struct wmi_lro_config_cmd_t wmi_lro_cmd = {0};
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!dp_lro_cmd || !wmi_handle) {
		dp_err("wmi_handle or dp_lro_cmd is null");
		return QDF_STATUS_E_FAILURE;
	}

	wmi_lro_cmd.lro_enable = dp_lro_cmd->lro_enable;
	wmi_lro_cmd.tcp_flag = dp_lro_cmd->tcp_flag;
	wmi_lro_cmd.tcp_flag_mask = dp_lro_cmd->tcp_flag_mask;
	qdf_mem_copy(wmi_lro_cmd.toeplitz_hash_ipv4,
		     dp_lro_cmd->toeplitz_hash_ipv4,
		     LRO_IPV4_SEED_ARR_SZ * sizeof(uint32_t));
	qdf_mem_copy(wmi_lro_cmd.toeplitz_hash_ipv6,
		     dp_lro_cmd->toeplitz_hash_ipv6,
		     LRO_IPV6_SEED_ARR_SZ * sizeof(uint32_t));

	return wmi_unified_lro_config_cmd(wmi_handle, &wmi_lro_cmd);
}

#ifdef WLAN_DP_FEATURE_STC
static QDF_STATUS
target_if_dp_send_opm_stats_cmd(struct wlan_objmgr_psoc *psoc,
				uint8_t pdev_id)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		dp_err("wmi_handle is null");
		return QDF_STATUS_E_FAILURE;
	}

	return wmi_unified_send_opm_stats_cmd(wmi_handle, pdev_id);
}
#endif

/**
 * target_if_dp_send_dhcp_ind() - process set arp stats request command to fw
 * @vdev_id: vdev id
 * @dhcp_ind: DHCP indication.
 *
 * Return: 0 on success, failure code otherwise.
 */
static QDF_STATUS
target_if_dp_send_dhcp_ind(uint16_t vdev_id,
			   struct dp_dhcp_ind *dhcp_ind)
{
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_psoc *psoc;
	wmi_peer_set_param_cmd_fixed_param peer_set_param_fp = {0};
	QDF_STATUS status;

	psoc = wlan_objmgr_get_psoc_by_id(0, WLAN_PSOC_TARGET_IF_ID);
	if (!psoc) {
		dp_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		dp_err("Unable to get wmi handle");
		return QDF_STATUS_E_NULL_VALUE;
	}

	/* fill in values */
	peer_set_param_fp.vdev_id = vdev_id;
	peer_set_param_fp.param_id = WMI_HOST_PEER_CRIT_PROTO_HINT_ENABLED;

	if (dhcp_ind->dhcp_start)
		peer_set_param_fp.param_value = 1;
	else
		peer_set_param_fp.param_value = 0;

	WMI_CHAR_ARRAY_TO_MAC_ADDR(dhcp_ind->peer_mac_addr.bytes,
				   &peer_set_param_fp.peer_macaddr);

	status = wmi_unified_process_dhcp_ind(wmi_handle,
					      &peer_set_param_fp);
	wlan_objmgr_psoc_release_ref(psoc, WLAN_PSOC_TARGET_IF_ID);

	return status;
}

static QDF_STATUS
target_if_dp_active_traffic_map(struct wlan_objmgr_psoc *psoc,
				struct dp_active_traffic_map_params *req_buf)
{
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_peer *peer;
	struct peer_active_traffic_map_params cmd = {0};
	QDF_STATUS status;

	if (!psoc || !req_buf) {
		target_if_err("Invalid params");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("Invalid wmi handle");
		return -EINVAL;
	}

	peer = wlan_objmgr_get_peer_by_mac(psoc, req_buf->mac.bytes,
					   WLAN_DP_ID);
	if (!peer) {
		target_if_err("Peer not found in the list");
		return -EINVAL;
	}

	cmd.vdev_id = req_buf->vdev_id;
	qdf_mem_copy(cmd.peer_macaddr.bytes, req_buf->mac.bytes,
		     QDF_MAC_ADDR_SIZE);
	cmd.active_traffic_map = req_buf->active_traffic_map;

	status = wmi_unified_peer_active_traffic_map_send(wmi_handle, &cmd);

	if (peer)
		wlan_objmgr_peer_release_ref(peer, WLAN_DP_ID);

	return status;
}

#ifdef WLAN_DP_FEATURE_STC
static inline void
dp_register_tx_ops_opm_stats(struct wlan_dp_psoc_sb_ops *sb_ops)
{
	sb_ops->dp_send_opm_stats_cmd = target_if_dp_send_opm_stats_cmd;
}
#else
static inline void
dp_register_tx_ops_opm_stats(struct wlan_dp_psoc_sb_ops *sb_ops)
{
}
#endif

#ifdef IPA_WDI3_VLAN_SUPPORT
/**
 * target_if_dp_send_pdev_pkt_routing_vlan() - Send pdev_update_pkt_routing WMI
 *					       to target for VLAN tagged packets
 * @psoc: psoc objmgr handle
 * @pdev_id: host pdev id
 * @dest_ring: destination ring that VLAN tagged packets should be routed to
 *
 * Return: void
 */
static void
target_if_dp_send_pdev_pkt_routing_vlan(struct wlan_objmgr_psoc *psoc,
					uint8_t pdev_id,
					uint32_t dest_ring)
{
	uint32_t len = sizeof(wmi_pdev_update_pkt_routing_cmd_fixed_param);
	wmi_pdev_update_pkt_routing_cmd_fixed_param *cmd;
	uint8_t target_pdev_id;
	wmi_unified_t wmi_hdl;
	QDF_STATUS status;
	uint32_t tlvlen;
	wmi_buf_t buf;
	uint32_t tag;

	wmi_hdl = (wmi_unified_t)get_wmi_unified_hdl_from_psoc(psoc);
	if (qdf_unlikely(!wmi_hdl))
		return;

	buf = wmi_buf_alloc(wmi_hdl, len);
	if (!buf) {
		target_if_err("wmi_buf_alloc failed");
		return;
	}

	cmd = (wmi_pdev_update_pkt_routing_cmd_fixed_param *)wmi_buf_data(buf);

	tag = WMITLV_TAG_STRUC_wmi_pdev_update_pkt_routing_cmd_fixed_param;
	tlvlen = WMITLV_GET_STRUCT_TLVLEN(
			wmi_pdev_update_pkt_routing_cmd_fixed_param);
	WMITLV_SET_HDR(&cmd->tlv_header, tag, tlvlen);

	target_pdev_id = wmi_hdl->ops->convert_host_pdev_id_to_target(wmi_hdl,
								      pdev_id);
	cmd->pdev_id = target_pdev_id;
	cmd->op_code = WMI_PDEV_ADD_PKT_ROUTING;
	cmd->routing_type_bitmap = BIT(WMI_PDEV_ROUTING_TYPE_VLAN);
	cmd->dest_ring = dest_ring;
	cmd->meta_data = WMI_PDEV_ROUTING_TYPE_VLAN;
	cmd->dest_ring_handler = WMI_PDEV_WIFIRXCCE_USE_CCE_E;

	target_if_debug("Set RX PKT ROUTING TYPE pdev_id: %u opcode: %u",
			cmd->pdev_id, cmd->op_code);
	target_if_debug("routing_bitmap: %u, dest_ring: %u",
			cmd->routing_type_bitmap, cmd->dest_ring);
	target_if_debug("dest_ring_handler: %u, meta_data: 0x%x",
			cmd->dest_ring_handler, cmd->meta_data);

	wmi_mtrace(WMI_PDEV_UPDATE_PKT_ROUTING_CMDID, cmd->pdev_id, 0);
	status = wmi_unified_cmd_send(wmi_hdl, buf, len,
				      WMI_PDEV_UPDATE_PKT_ROUTING_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_buf_free(buf);
		target_if_err("WMI_PDEV_UPDATE_PKT_ROUTING_CMDID failed");
	}
}

static inline void
target_if_dp_register_ipa_vlan_ops(struct wlan_dp_psoc_sb_ops *sb_ops)
{
	sb_ops->dp_send_pdev_pkt_routing_vlan =
		target_if_dp_send_pdev_pkt_routing_vlan;
}
#else /* !IPA_WDI3_VLAN_SUPPORT */
static inline void
target_if_dp_register_ipa_vlan_ops(struct wlan_dp_psoc_sb_ops *sb_ops)
{
}
#endif /* IPA_WDI3_VLAN_SUPPORT */

void target_if_dp_register_tx_ops(struct wlan_dp_psoc_sb_ops *sb_ops)
{
	sb_ops->dp_arp_stats_register_event_handler =
		target_if_dp_arp_stats_register_event_handler;
	sb_ops->dp_arp_stats_unregister_event_handler =
		target_if_dp_arp_stats_unregister_event_handler;
	sb_ops->dp_get_arp_req_stats =
		target_if_dp_get_arp_req_stats;
	sb_ops->dp_set_arp_req_stats =
		target_if_dp_set_arp_req_stats;
	sb_ops->dp_lro_config_cmd = target_if_dp_lro_config_cmd;
	sb_ops->dp_send_dhcp_ind =
		target_if_dp_send_dhcp_ind;
	sb_ops->dp_send_active_traffic_map = target_if_dp_active_traffic_map;
	dp_register_tx_ops_opm_stats(sb_ops);
	target_if_dp_register_ipa_vlan_ops(sb_ops);
}

void target_if_dp_register_rx_ops(struct wlan_dp_psoc_nb_ops *nb_ops)
{
}
