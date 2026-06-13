/*
 * Copyright (c) 2016-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: contains core nan function definitions
 */

#include "wlan_utility.h"
#include "nan_ucfg_api.h"
#include "wlan_nan_api.h"
#include "target_if_nan.h"
#include "scheduler_api.h"
#include "wlan_policy_mgr_api.h"
#include "wlan_osif_request_manager.h"
#include "wlan_serialization_api.h"
#include "wlan_objmgr_cmn.h"
#include "wlan_tdls_ucfg_api.h"
#include "wlan_objmgr_global_obj.h"
#include "wlan_objmgr_psoc_obj.h"
#include "wlan_objmgr_pdev_obj.h"
#include "wlan_objmgr_vdev_obj.h"
#include "qdf_platform.h"
#include "wlan_osif_request_manager.h"
#include "wlan_p2p_api.h"
#include "wlan_dp_api.h"
#include "wlan_mlme_vdev_mgr_interface.h"
#include "wlan_if_mgr_public_struct.h"
#include "wlan_if_mgr_api.h"
#include "wlan_mlme_api.h"

bool nan_is_pairing_allowed(struct wlan_objmgr_psoc *psoc)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;

	if (!psoc) {
		nan_err("psoc is null");
		return false;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return false;
	}

	return psoc_nan_obj->nan_caps.nan_pairing_peer_create_cap;
}

bool nan_is_peer_exist_for_opmode(struct wlan_objmgr_psoc *psoc,
				  struct qdf_mac_addr *peer_mac_addr,
				  enum QDF_OPMODE opmode)
{
	struct wlan_objmgr_peer *peer;
	struct wlan_objmgr_vdev *vdev;
	bool is_peer_exist = false;

	if (!peer_mac_addr || qdf_is_macaddr_zero(peer_mac_addr))
		return false;

	peer = wlan_objmgr_get_peer_by_mac(psoc, peer_mac_addr->bytes,
					   WLAN_NAN_ID);
	if (!peer)
		return false;

	vdev = wlan_peer_get_vdev(peer);
	wlan_objmgr_vdev_get_ref(vdev, WLAN_NAN_ID);
	if (!vdev)
		goto peer_ref_rel;

	/* peer exist for given interface */
	if (wlan_vdev_mlme_get_opmode(vdev) == opmode)
		is_peer_exist = true;

	wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);

peer_ref_rel:
	wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
	return is_peer_exist;
}

void nan_update_pasn_peer_count(struct wlan_objmgr_vdev *vdev,
				bool is_increment)
{
	struct nan_vdev_priv_obj *nan_vdev_obj;

	nan_vdev_obj = nan_get_vdev_priv_obj(vdev);
	if (!nan_vdev_obj) {
		nan_err("NAN vdev priv obj is null");
		return;
	}

	if (is_increment)
		nan_vdev_obj->num_pasn_peers++;
	else if (nan_vdev_obj->num_pasn_peers)
		nan_vdev_obj->num_pasn_peers--;

	nan_debug("Pasn peer count:%d", nan_vdev_obj->num_pasn_peers);
}

/**
 * nan_add_peer_in_migrated_addr_list() - add peer address in the migrated list
 * @psoc: pointer to psoc object
 * @vdev_id: VDEV ID
 * @peer_mac_addr: peer mac address
 *
 * Return: QDF status
 */
static QDF_STATUS
nan_add_peer_in_migrated_addr_list(struct wlan_objmgr_psoc *psoc,
				   uint8_t vdev_id,
				   struct qdf_mac_addr *peer_mac_addr)
{
	struct nan_vdev_priv_obj *nan_vdev_priv;
	struct wlan_objmgr_vdev *vdev;
	uint8_t idx;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id, WLAN_NAN_ID);
	if (!vdev) {
		nan_err("vdev is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nan_vdev_priv = nan_get_vdev_priv_obj(vdev);
	if (!nan_vdev_priv) {
		nan_err("NAN vdev priv obj is null");
		status = QDF_STATUS_E_NULL_VALUE;
		goto ref_rel;
	}

	idx = nan_vdev_priv->num_peer_migrated++;
	if (nan_vdev_priv->num_peer_migrated > MAX_NAN_MIGRATED_PEERS) {
		nan_err("num migrated peers %d more than max migrated peers",
			nan_vdev_priv->num_peer_migrated);
		status = QDF_STATUS_E_FAILURE;
		goto ref_rel;
	}

	qdf_mem_copy(nan_vdev_priv->peer_migrated_addr_list[idx].bytes,
		     peer_mac_addr->bytes, QDF_MAC_ADDR_SIZE);

	nan_debug("add peer to migrated list at index %d", idx);

ref_rel:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
	return status;
}

/**
 * nan_remove_peer_in_migrated_addr_list() - remove peer address from the
 * migrated list
 * @psoc: pointer to psoc object
 * @vdev_id: VDEV ID
 * @peer_mac_addr: peer mac address
 *
 * Return: QDF status
 */
static QDF_STATUS
nan_remove_peer_in_migrated_addr_list(struct wlan_objmgr_psoc *psoc,
				      uint8_t vdev_id,
				      struct qdf_mac_addr *peer_mac_addr)
{
	struct wlan_objmgr_vdev *vdev;
	struct nan_vdev_priv_obj *nan_vdev_priv;
	uint8_t i = 0, idx;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id, WLAN_NAN_ID);
	if (!vdev) {
		nan_err("vdev is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nan_vdev_priv = nan_get_vdev_priv_obj(vdev);
	if (!nan_vdev_priv) {
		nan_err("NAN vdev priv obj is null");
		status = QDF_STATUS_E_NULL_VALUE;
		goto ref_rel;
	}

	idx = nan_vdev_priv->num_peer_migrated;
	if (idx > MAX_NAN_MIGRATED_PEERS) {
		nan_err("idx %d more than max migrated peers", idx);
		status = QDF_STATUS_E_FAILURE;
		goto ref_rel;
	}

	if (!idx) {
		nan_debug("last migrated peer removed");
		goto ref_rel;
	}

	for (i = 0; i < idx; i++) {
		if (qdf_is_macaddr_equal(
				peer_mac_addr,
				&nan_vdev_priv->peer_migrated_addr_list[i])) {
			/*
			 * move the peer address from last position to
			 * position i
			 */
			qdf_mem_copy(
			nan_vdev_priv->peer_migrated_addr_list[i].bytes,
			nan_vdev_priv->peer_migrated_addr_list[idx - 1].bytes,
			QDF_MAC_ADDR_SIZE);
			break;
		}
	}

	nan_debug("peer remove from migrated list at index %d with max peer %d",
		  i, idx);
	nan_vdev_priv->num_peer_migrated--;

ref_rel:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
	return status;
}

/**
 * nan_is_peer_migrated() - check if peer is migrated from NAN to NDP or not
 * @psoc: pointer to psoc object
 * @vdev_id: VDEV ID
 * @peer_mac_addr: peer mac address
 *
 * Return: true if peer is migrated otherwise false
 */
static bool
nan_is_peer_migrated(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
		     struct qdf_mac_addr *peer_mac_addr)
{
	struct wlan_objmgr_vdev *vdev;
	struct nan_vdev_priv_obj *nan_vdev_priv;
	uint8_t i;
	bool is_peer_migrated = false;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id, WLAN_NAN_ID);
	if (!vdev) {
		nan_err("vdev is null");
		return false;
	}

	nan_vdev_priv = nan_get_vdev_priv_obj(vdev);
	if (!nan_vdev_priv) {
		nan_err("NAN vdev priv obj is null");
		goto ref_rel;
	}

	if (!nan_vdev_priv->num_peer_migrated &&
	    nan_vdev_priv->num_peer_migrated > MAX_NAN_MIGRATED_PEERS)
		goto ref_rel;

	for (i = 0; i < nan_vdev_priv->num_peer_migrated; i++) {
		if (qdf_is_macaddr_equal(
				peer_mac_addr,
				&nan_vdev_priv->peer_migrated_addr_list[i])) {
			nan_debug("peer found");
			is_peer_migrated = true;
			goto ref_rel;
		}
	}

ref_rel:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
	return is_peer_migrated;
}

/**
 * nan_set_ndi_vdev_id_for_migrated_peer() - This API caches NDI VDEV ID in
 * NAN peer private object
 * @psoc: pointer to psoc object
 * @peer_mac_addr: peer mac address
 * @ndi_vdev_id: NDI VDEV ID
 *
 * Return: QDF status
 */
static QDF_STATUS
nan_set_ndi_vdev_id_for_migrated_peer(struct wlan_objmgr_psoc *psoc,
				      struct qdf_mac_addr *peer_mac_addr,
				      uint8_t ndi_vdev_id)
{
	struct wlan_objmgr_peer *peer;
	struct nan_peer_priv_obj *nan_peer_priv;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	peer = wlan_objmgr_get_peer_by_mac(psoc, peer_mac_addr->bytes,
					   WLAN_NAN_ID);
	if (!peer) {
		nan_err("peer is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nan_peer_priv = nan_get_peer_priv_obj(peer);
	if (!nan_peer_priv) {
		nan_err("NAN peer priv obj is null");
		status = QDF_STATUS_E_NULL_VALUE;
		goto ref_rel;
	}

	nan_peer_priv->ndi_vdev_id = ndi_vdev_id;

ref_rel:
	wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
	return status;
}

/**
 * nan_get_ndi_vdev_id_from_migrated_peer() - This API retrieves NDI VDEV ID
 * from the NAN peer private object
 * @psoc: pointer to psoc object
 * @peer_mac_addr: peer mac address
 *
 * Return: NDI VDEV ID
 */
static uint8_t
nan_get_ndi_vdev_id_from_migrated_peer(struct wlan_objmgr_psoc *psoc,
				       struct qdf_mac_addr *peer_mac_addr)
{
	struct wlan_objmgr_peer *peer;
	struct nan_peer_priv_obj *nan_peer_priv;
	uint8_t vdev_id = INVALID_VDEV_ID;

	peer = wlan_objmgr_get_peer_by_mac(psoc, peer_mac_addr->bytes,
					   WLAN_NAN_ID);
	if (!peer) {
		nan_err("peer is null");
		return vdev_id;
	}

	nan_peer_priv = nan_get_peer_priv_obj(peer);
	if (!nan_peer_priv) {
		nan_err("NAN peer priv obj is null");
		goto ref_rel;
	}

	vdev_id = nan_peer_priv->ndi_vdev_id;

ref_rel:
	wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
	return vdev_id;
}

/**
 * ndi_cleanup_pasn_peer_from_nan() - This API will delete the NAN PASN peer
 * for peer migration
 * @psoc: pointer to psoc object
 * @nan_vdev_id: NAN VDEV ID
 * @ndi_vdev_id: NDI VDEV ID
 * @peer_mac_addr: peer mac address
 *
 * Return: QDF status
 */
static QDF_STATUS
ndi_cleanup_pasn_peer_from_nan(struct wlan_objmgr_psoc *psoc,
			       uint8_t nan_vdev_id, uint8_t ndi_vdev_id,
			       struct qdf_mac_addr *peer_mac_addr)
{
	QDF_STATUS status;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct nan_pasn_peer_ops *peer_ops;

	if (nan_is_pairing_allowed(psoc)) {
		nan_debug(" NAN pairing is not allowed");
		return QDF_STATUS_SUCCESS;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer_ops = &psoc_nan_obj->cb_obj.pasn_peer_ops;
	if (!peer_ops->nan_pasn_peer_delete_cb) {
		nan_err("NAN PASN peer delete ops is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	status = peer_ops->nan_pasn_peer_delete_cb(psoc, nan_vdev_id,
						   peer_mac_addr,
						   NAN_PASN_PEER_DELETE,
						   false);

	if (QDF_IS_STATUS_ERROR(status)) {
		nan_err("NAN PASN peer delete request fails");
		return status;
	}

	status = nan_add_peer_in_migrated_addr_list(psoc, nan_vdev_id,
						    peer_mac_addr);
	if (QDF_IS_STATUS_ERROR(status)) {
		nan_err("fail to add peer addr in migrated list");
		return status;
	}

	status = nan_set_ndi_vdev_id_for_migrated_peer(psoc, peer_mac_addr,
						       ndi_vdev_id);

	if (QDF_IS_STATUS_ERROR(status))
		nan_err("fail to add ndi vdev id in NAN peer");

	return status;
}

QDF_STATUS ndi_add_pasn_peer_to_nan(struct wlan_objmgr_psoc *psoc,
				    uint8_t nan_vdev_id,
				    struct qdf_mac_addr *peer_mac_addr)
{
	QDF_STATUS status;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct nan_pasn_peer_ops *peer_ops;

	if (!nan_is_pairing_allowed(psoc)) {
		nan_debug(" NAN pairing is not allowed");
		return QDF_STATUS_SUCCESS;
	}

	if (!nan_is_peer_migrated(psoc, nan_vdev_id, peer_mac_addr)) {
		nan_debug("NAN PASN peer is not migrated");
		return QDF_STATUS_SUCCESS;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer_ops = &psoc_nan_obj->cb_obj.pasn_peer_ops;
	if (!peer_ops->nan_pasn_peer_create_cb) {
		nan_err("NAN PASN peer create ops is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	status = peer_ops->nan_pasn_peer_create_cb(psoc, peer_mac_addr,
						   nan_vdev_id,
						   NAN_PASN_PEER_CREATE);

	if (QDF_IS_STATUS_ERROR(status)) {
		nan_err("NAN PASN peer create request fails");
		return status;
	}

	status = nan_remove_peer_in_migrated_addr_list(psoc, nan_vdev_id,
						       peer_mac_addr);

	if (QDF_IS_STATUS_ERROR(status))
		nan_err("fail to remove peer addr in migrated list");

	return status;
}

QDF_STATUS nan_set_discovery_state(struct wlan_objmgr_psoc *psoc,
				   enum nan_disc_state new_state)
{
	enum nan_disc_state cur_state;
	struct nan_psoc_priv_obj *psoc_priv = nan_get_psoc_priv_obj(psoc);
	bool nan_state_change_allowed = false;
	QDF_STATUS status = QDF_STATUS_E_INVAL;

	if (!psoc_priv) {
		nan_err("nan psoc priv object is NULL");
		return QDF_STATUS_E_INVAL;
	}

	qdf_spin_lock_bh(&psoc_priv->lock);
	cur_state = psoc_priv->disc_state;
	if (cur_state == new_state) {
		qdf_spin_unlock_bh(&psoc_priv->lock);
		nan_err("curr_state: %u and new state: %u are same",
			cur_state, new_state);
		return status;
	}

	switch (new_state) {
	case NAN_DISC_DISABLED:
		nan_state_change_allowed = true;
		break;
	case NAN_DISC_ENABLE_IN_PROGRESS:
		if (cur_state == NAN_DISC_DISABLED)
			nan_state_change_allowed = true;
		break;
	case NAN_DISC_ENABLED:
		if (cur_state == NAN_DISC_ENABLE_IN_PROGRESS)
			nan_state_change_allowed = true;
		break;
	case NAN_DISC_DISABLE_IN_PROGRESS:
		if (cur_state == NAN_DISC_ENABLE_IN_PROGRESS ||
		    cur_state == NAN_DISC_ENABLED)
			nan_state_change_allowed = true;
		break;
	default:
		break;
	}

	if (nan_state_change_allowed) {
		psoc_priv->disc_state = new_state;
		status = QDF_STATUS_SUCCESS;
	}

	qdf_spin_unlock_bh(&psoc_priv->lock);

	nan_debug("NAN State transitioned from %d -> %d", cur_state,
		  psoc_priv->disc_state);

	return status;
}

enum nan_disc_state nan_get_discovery_state(struct wlan_objmgr_psoc *psoc)
{
	struct nan_psoc_priv_obj *psoc_priv = nan_get_psoc_priv_obj(psoc);

	if (!psoc_priv) {
		nan_err("nan psoc priv object is NULL");
		return NAN_DISC_DISABLED;
	}

	return psoc_priv->disc_state;
}

void nan_release_cmd(void *in_req, uint32_t cmdtype)
{
	struct wlan_objmgr_vdev *vdev = NULL;

	if (!in_req)
		return;

	switch (cmdtype) {
	case WLAN_SER_CMD_NDP_INIT_REQ: {
		struct nan_datapath_initiator_req *req = in_req;

		vdev = req->vdev;
		break;
	}
	case WLAN_SER_CMD_NDP_RESP_REQ: {
		struct nan_datapath_responder_req *req = in_req;

		vdev = req->vdev;
		break;
	}
	case WLAN_SER_CMD_NDP_DATA_END_INIT_REQ: {
		struct nan_datapath_end_req *req = in_req;

		vdev = req->vdev;
		break;
	}
	case WLAN_SER_CMD_NDP_END_ALL_REQ: {
		struct nan_datapath_end_all_ndps *req = in_req;

		vdev = req->vdev;
		break;
	}
	default:
		nan_err("invalid req type: %d", cmdtype);
		break;
	}

	if (vdev)
		wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
	else
		nan_err("vdev is null");

	qdf_mem_free(in_req);
}

static void nan_req_incomplete(void *req, uint32_t cmdtype)
{
	/* send msg to userspace if needed that cmd got incomplete */
}

static void nan_req_activated(void *in_req, uint32_t cmdtype)
{
	uint32_t req_type;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_vdev *vdev;
	struct wlan_nan_tx_ops *tx_ops;
	struct nan_psoc_priv_obj *psoc_nan_obj;

	switch (cmdtype) {
	case WLAN_SER_CMD_NDP_INIT_REQ: {
		struct nan_datapath_initiator_req *req = in_req;

		vdev = req->vdev;
		req_type = NDP_INITIATOR_REQ;
		break;
	}
	case WLAN_SER_CMD_NDP_RESP_REQ: {
		struct nan_datapath_responder_req *req = in_req;

		vdev = req->vdev;
		req_type = NDP_RESPONDER_REQ;
		break;
	}
	case WLAN_SER_CMD_NDP_DATA_END_INIT_REQ: {
		struct nan_datapath_end_req *req = in_req;

		vdev = req->vdev;
		req_type = NDP_END_REQ;
		break;
	}
	case WLAN_SER_CMD_NDP_END_ALL_REQ: {
		struct nan_datapath_end_all_ndps *req = in_req;

		vdev = req->vdev;
		req_type = NDP_END_ALL;
		break;
	}
	default:
		nan_alert("in correct cmdtype: %d", cmdtype);
		return;
	}

	if (!vdev) {
		nan_alert("vdev is null");
		return;
	}

	psoc = wlan_vdev_get_psoc(vdev);
	if (!psoc) {
		nan_alert("psoc is null");
		return;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return;
	}

	tx_ops = &psoc_nan_obj->tx_ops;
	if (!tx_ops) {
		nan_alert("tx_ops is null");
		return;
	}

	/* send ndp_intiator_req/responder_req/end_req to FW */
	tx_ops->nan_datapath_req_tx(in_req, req_type);
}

static QDF_STATUS nan_serialized_cb(struct wlan_serialization_command *ser_cmd,
				    enum wlan_serialization_cb_reason reason)
{
	void *req;

	if (!ser_cmd || !ser_cmd->umac_cmd) {
		nan_alert("cmd or umac_cmd is null");
		return QDF_STATUS_E_NULL_VALUE;
	}
	req = ser_cmd->umac_cmd;

	switch (reason) {
	case WLAN_SER_CB_ACTIVATE_CMD:
		nan_req_activated(req, ser_cmd->cmd_type);
		break;
	case WLAN_SER_CB_CANCEL_CMD:
	case WLAN_SER_CB_ACTIVE_CMD_TIMEOUT:
		nan_req_incomplete(req, ser_cmd->cmd_type);
		break;
	case WLAN_SER_CB_RELEASE_MEM_CMD:
		nan_release_cmd(req, ser_cmd->cmd_type);
		break;
	default:
		/* Do nothing but logging */
		nan_alert("invalid serialized cb reason: %d", reason);
		break;
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS nan_scheduled_msg_handler(struct scheduler_msg *msg)
{
	enum wlan_serialization_status status = 0;
	struct wlan_serialization_command cmd = {0};

	if (!msg || !msg->bodyptr) {
		nan_alert("msg or bodyptr is null");
		return QDF_STATUS_E_NULL_VALUE;
	}
	switch (msg->type) {
	case NDP_INITIATOR_REQ: {
		struct nan_datapath_initiator_req *req = msg->bodyptr;

		cmd.cmd_type = WLAN_SER_CMD_NDP_INIT_REQ;
		cmd.vdev = req->vdev;
		break;
	}
	case NDP_RESPONDER_REQ: {
		struct nan_datapath_responder_req *req = msg->bodyptr;

		cmd.cmd_type = WLAN_SER_CMD_NDP_RESP_REQ;
		cmd.vdev = req->vdev;
		break;
	}
	case NDP_END_REQ: {
		struct nan_datapath_end_req *req = msg->bodyptr;

		cmd.cmd_type = WLAN_SER_CMD_NDP_DATA_END_INIT_REQ;
		cmd.vdev = req->vdev;
		break;
	}
	case NDP_END_ALL: {
		struct nan_datapath_end_all_ndps *req = msg->bodyptr;

		cmd.cmd_type = WLAN_SER_CMD_NDP_END_ALL_REQ;
		cmd.vdev = req->vdev;
		break;
	}
	default:
		nan_err("wrong request type: %d", msg->type);
		return QDF_STATUS_E_INVAL;
	}

	/* TBD - support more than one req of same type or avoid */
	cmd.cmd_id = 0;
	cmd.cmd_cb = nan_serialized_cb;
	cmd.umac_cmd = msg->bodyptr;
	cmd.source = WLAN_UMAC_COMP_NAN;
	cmd.is_high_priority = false;
	cmd.cmd_timeout_duration = NAN_SER_CMD_TIMEOUT;
	nan_debug("cmd_type: %d", cmd.cmd_type);
	cmd.is_blocking = true;

	status = wlan_serialization_request(&cmd);
	/* following is TBD */
	if (status != WLAN_SER_CMD_ACTIVE && status != WLAN_SER_CMD_PENDING) {
		nan_err("unable to serialize command");
		wlan_objmgr_vdev_release_ref(cmd.vdev, WLAN_NAN_ID);
		qdf_mem_free(msg->bodyptr);
		msg->bodyptr = NULL;
		return QDF_STATUS_E_INVAL;
	}
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
nan_increment_ndp_sessions(struct wlan_objmgr_psoc *psoc,
			   struct qdf_mac_addr *peer_ndi_mac,
			   struct nan_datapath_channel_info *ndp_chan_info)
{
	struct wlan_objmgr_peer *peer;
	struct nan_peer_priv_obj *peer_nan_obj;

	peer = wlan_objmgr_get_peer_by_mac(psoc,
					   peer_ndi_mac->bytes,
					   WLAN_NAN_ID);

	if (!peer) {
		nan_err("peer object is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer_nan_obj = nan_get_peer_priv_obj(peer);
	if (!peer_nan_obj) {
		nan_err("peer_nan_obj is null");
		wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
		return QDF_STATUS_E_NULL_VALUE;
	}
	qdf_spin_lock_bh(&peer_nan_obj->lock);

	/*
	 * Store the first channel info in NDP Confirm as the home channel info
	 * and store it in the peer private object.
	 */
	if (!peer_nan_obj->active_ndp_sessions)
		qdf_mem_copy(&peer_nan_obj->home_chan_info, ndp_chan_info,
			     sizeof(struct nan_datapath_channel_info));

	peer_nan_obj->active_ndp_sessions++;
	nan_debug("Number of active session = %d for peer:"QDF_MAC_ADDR_FMT,
		  peer_nan_obj->active_ndp_sessions,
		  QDF_MAC_ADDR_REF(peer_ndi_mac->bytes));
	qdf_spin_unlock_bh(&peer_nan_obj->lock);
	wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_decrement_ndp_sessions(struct wlan_objmgr_psoc *psoc,
					     struct qdf_mac_addr *peer_ndi_mac)
{
	struct wlan_objmgr_peer *peer;
	struct nan_peer_priv_obj *peer_nan_obj;

	peer = wlan_objmgr_get_peer_by_mac(psoc,
					   peer_ndi_mac->bytes,
					   WLAN_NAN_ID);

	if (!peer) {
		nan_err("peer object is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer_nan_obj = nan_get_peer_priv_obj(peer);
	if (!peer_nan_obj) {
		nan_err("peer_nan_obj is null");
		wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
		return QDF_STATUS_E_NULL_VALUE;
	}

	qdf_spin_lock_bh(&peer_nan_obj->lock);
	if (!peer_nan_obj->active_ndp_sessions) {
		qdf_spin_unlock_bh(&peer_nan_obj->lock);
		nan_err("Active NDP sessions already zero!");
		wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
		return QDF_STATUS_E_FAILURE;
	}
	peer_nan_obj->active_ndp_sessions--;
	nan_debug("Number of active session = %d for peer:"QDF_MAC_ADDR_FMT,
		  peer_nan_obj->active_ndp_sessions,
		  QDF_MAC_ADDR_REF(peer_ndi_mac->bytes));
	qdf_spin_unlock_bh(&peer_nan_obj->lock);
	wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
ndi_remove_and_update_primary_connection(struct wlan_objmgr_psoc *psoc,
					 struct wlan_objmgr_vdev *vdev)
{
	struct nan_vdev_priv_obj *vdev_nan_obj;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct nan_peer_priv_obj *peer_nan_obj = NULL;
	struct wlan_objmgr_peer *peer, *peer_next;
	qdf_list_t *peer_list;
	void (*nan_conc_callback)(void);


	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("Invalid psoc nan private obj");
		return QDF_STATUS_E_NULL_VALUE;
	}

	vdev_nan_obj = nan_get_vdev_priv_obj(vdev);
	if (!vdev_nan_obj) {
		nan_err("Invalid vdev nan private obj");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer_list = &vdev->vdev_objmgr.wlan_peer_list;
	if (!peer_list) {
		nan_err("Peer list for vdev obj is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer = wlan_vdev_peer_list_peek_active_head(vdev, peer_list,
						    WLAN_NAN_ID);

	while (peer) {
		peer_nan_obj = nan_get_peer_priv_obj(peer);
		if (!peer_nan_obj)
			nan_err("NAN peer object for Peer " QDF_MAC_ADDR_FMT " is NULL",
				QDF_MAC_ADDR_REF(wlan_peer_get_macaddr(peer)));
		else if (peer_nan_obj->active_ndp_sessions)
			break;

		peer_next = wlan_peer_get_next_active_peer_of_vdev(vdev,
								   peer_list,
								   peer,
								   WLAN_NAN_ID);
		wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
		peer = peer_next;
	}

	if (!peer && NDI_CONCURRENCY_SUPPORTED(psoc)) {
		policy_mgr_decr_session_set_pcl(psoc, QDF_NDI_MODE,
						wlan_vdev_get_id(vdev));
		vdev_nan_obj->ndp_init_done = false;

		nan_conc_callback = psoc_nan_obj->cb_obj.nan_concurrency_update;
		if (nan_conc_callback)
			nan_conc_callback();

		return QDF_STATUS_SUCCESS;
	}

	if (peer_nan_obj && NDI_CONCURRENCY_SUPPORTED(psoc)) {
		psoc_nan_obj->cb_obj.update_ndi_conn(wlan_vdev_get_id(vdev),
						 &peer_nan_obj->home_chan_info);
		policy_mgr_update_connection_info(psoc, wlan_vdev_get_id(vdev));
		qdf_mem_copy(vdev_nan_obj->primary_peer_mac.bytes,
			     wlan_peer_get_macaddr(peer), QDF_MAC_ADDR_SIZE);
		policy_mgr_check_n_start_opportunistic_timer(psoc);
	}

	if (peer)
		wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
ndi_update_ndp_session(struct wlan_objmgr_vdev *vdev,
		       struct qdf_mac_addr *peer_ndi_mac,
		       struct nan_datapath_channel_info *ndp_chan_info)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_peer *peer;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct nan_vdev_priv_obj *vdev_nan_obj;
	struct nan_peer_priv_obj *peer_nan_obj;

	psoc = wlan_vdev_get_psoc(vdev);

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	vdev_nan_obj = nan_get_vdev_priv_obj(vdev);
	if (!vdev_nan_obj) {
		nan_err("NAN vdev private object is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer = wlan_objmgr_get_peer_by_mac(psoc,
					   peer_ndi_mac->bytes,
					   WLAN_NAN_ID);

	if (!peer) {
		nan_err("peer object is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer_nan_obj = nan_get_peer_priv_obj(peer);
	if (!peer_nan_obj) {
		nan_err("peer_nan_obj is null");
		wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
		return QDF_STATUS_E_NULL_VALUE;
	}

	qdf_spin_lock_bh(&peer_nan_obj->lock);
	qdf_mem_copy(&peer_nan_obj->home_chan_info, ndp_chan_info,
		     sizeof(*ndp_chan_info));
	psoc_nan_obj->cb_obj.update_ndi_conn(wlan_vdev_get_id(vdev),
					     &peer_nan_obj->home_chan_info);
	qdf_spin_unlock_bh(&peer_nan_obj->lock);
	wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);

	if (qdf_is_macaddr_equal(&vdev_nan_obj->primary_peer_mac,
				 peer_ndi_mac)) {
		/* TODO: Update policy mgr with connection info */
	}
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
ndi_update_policy_mgr_conn_table(struct nan_datapath_confirm_event *confirm,
				 struct wlan_objmgr_psoc *psoc, uint8_t vdev_id)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (policy_mgr_is_hw_dbs_capable(psoc)) {
		status = policy_mgr_update_and_wait_for_connection_update(psoc,
					vdev_id, confirm->ch[0].freq,
					POLICY_MGR_UPDATE_REASON_NDP_UPDATE);
		if (QDF_IS_STATUS_ERROR(status)) {
			nan_err("Failed to set or wait for HW mode change");
			return status;
		}
	}

	policy_mgr_incr_active_session(psoc, QDF_NDI_MODE, vdev_id);

	return status;
}

static QDF_STATUS nan_handle_confirm(struct nan_datapath_confirm_event *confirm)
{
	uint8_t vdev_id;
	struct wlan_objmgr_psoc *psoc;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct nan_vdev_priv_obj *vdev_nan_obj;
	struct wlan_objmgr_peer *peer;
	void (*nan_conc_callback)(void);

	vdev_id = wlan_vdev_get_id(confirm->vdev);
	psoc = wlan_vdev_get_psoc(confirm->vdev);
	if (!psoc) {
		nan_err("psoc is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer = wlan_objmgr_get_peer_by_mac(psoc,
					   confirm->peer_ndi_mac_addr.bytes,
					   WLAN_NAN_ID);
	if (!peer && confirm->rsp_code == NAN_DATAPATH_RESPONSE_ACCEPT) {
		nan_debug("Drop NDP confirm as peer isn't available");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (peer) {
		if (confirm->rsp_code == NAN_DATAPATH_RESPONSE_ACCEPT)
			wlan_dp_notify_ndp_channel_info(peer, &confirm->ch[0],
							confirm->num_channels);
		wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	vdev_nan_obj = nan_get_vdev_priv_obj(confirm->vdev);
	if (!vdev_nan_obj) {
		nan_err("vdev_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (confirm->rsp_code != NAN_DATAPATH_RESPONSE_ACCEPT &&
	    confirm->num_active_ndps_on_peer == 0) {
		/*
		 * This peer was created at ndp_indication but
		 * confirm failed, so it needs to be deleted
		 */
		nan_err("NDP confirm with reject and no active ndp sessions. deleting peer: "QDF_MAC_ADDR_FMT" on vdev_id: %d",
			QDF_MAC_ADDR_REF(confirm->peer_ndi_mac_addr.bytes),
			vdev_id);
		psoc_nan_obj->cb_obj.delete_peers_by_addr(vdev_id,
						confirm->peer_ndi_mac_addr);
	}

	/* Increment NDP sessions for the Peer */
	if (confirm->rsp_code == NAN_DATAPATH_RESPONSE_ACCEPT)
		nan_increment_ndp_sessions(psoc, &confirm->peer_ndi_mac_addr,
					   &confirm->ch[0]);

	psoc_nan_obj->cb_obj.os_if_ndp_event_handler(psoc, confirm->vdev,
						     NDP_CONFIRM, confirm);

	if (confirm->rsp_code == NAN_DATAPATH_RESPONSE_ACCEPT &&
	    !vdev_nan_obj->ndp_init_done) {
		/*
		 * If this is the NDI's first NDP, store the NDP instance in
		 * vdev object as its primary connection. If this instance ends
		 * the second NDP should take its place.
		 */
		qdf_mem_copy(vdev_nan_obj->primary_peer_mac.bytes,
			     &confirm->peer_ndi_mac_addr, QDF_MAC_ADDR_SIZE);

		psoc_nan_obj->cb_obj.update_ndi_conn(vdev_id, &confirm->ch[0]);

		if (NAN_CONCURRENCY_SUPPORTED(psoc)) {
			ndi_update_policy_mgr_conn_table(confirm, psoc,
							 vdev_id);
			vdev_nan_obj->ndp_init_done = true;

			nan_conc_callback = psoc_nan_obj->cb_obj.nan_concurrency_update;
			if (nan_conc_callback)
				nan_conc_callback();
		}
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_handle_initiator_rsp(
				struct nan_datapath_initiator_rsp *rsp,
				struct wlan_objmgr_vdev **vdev)
{
	struct wlan_objmgr_psoc *psoc;
	struct nan_psoc_priv_obj *psoc_nan_obj;

	*vdev = rsp->vdev;
	psoc = wlan_vdev_get_psoc(rsp->vdev);
	if (!psoc) {
		nan_err("psoc is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj->cb_obj.os_if_ndp_event_handler(psoc, rsp->vdev,
						     NDP_INITIATOR_RSP, rsp);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_handle_ndp_ind(
				struct nan_datapath_indication_event *ndp_ind)
{
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_objmgr_vdev *nan_vdev;
	uint8_t nan_vdev_id;
	uint8_t ndi_vdev_id;
	bool is_peer_migrated = false;

	ndi_vdev_id = wlan_vdev_get_id(ndp_ind->vdev);
	psoc = wlan_vdev_get_psoc(ndp_ind->vdev);
	if (!psoc) {
		nan_err("psoc is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nan_debug("role: %d, vdev: %d, csid: %d, peer_mac_addr "
		QDF_MAC_ADDR_FMT,
		ndp_ind->role, ndi_vdev_id, ndp_ind->ncs_sk_type,
		QDF_MAC_ADDR_REF(ndp_ind->peer_mac_addr.bytes));

	if ((ndp_ind->role == NAN_DATAPATH_ROLE_INITIATOR) ||
	    ((NAN_DATAPATH_ROLE_RESPONDER == ndp_ind->role) &&
	    (NAN_DATAPATH_ACCEPT_POLICY_ALL == ndp_ind->policy))) {
		nan_vdev =
		wlan_objmgr_get_vdev_by_opmode_from_psoc(psoc,
							 QDF_NAN_DISC_MODE,
							 WLAN_NAN_ID);
		if (!nan_vdev) {
			nan_err("vdev is null");
			return QDF_STATUS_E_NULL_VALUE;
		}

		nan_vdev_id = wlan_vdev_get_id(nan_vdev);
		wlan_objmgr_vdev_release_ref(nan_vdev, WLAN_NAN_ID);

		if (nan_is_peer_exist_for_opmode(psoc, &ndp_ind->peer_mac_addr,
						 QDF_NAN_DISC_MODE)) {
			status = ndi_cleanup_pasn_peer_from_nan(
						psoc, nan_vdev_id, ndi_vdev_id,
						&ndp_ind->peer_mac_addr);

			if (QDF_IS_STATUS_SUCCESS(status))
				is_peer_migrated = true;
		}

		if (!is_peer_migrated)
			status = psoc_nan_obj->cb_obj.add_ndi_peer(ndi_vdev_id,
							ndp_ind->peer_mac_addr);

		if (QDF_IS_STATUS_ERROR(status)) {
			nan_err("Couldn't add ndi peer, ndp_role: %d",
				ndp_ind->role);
			return status;
		}
	}
	psoc_nan_obj->cb_obj.os_if_ndp_event_handler(psoc,
						     ndp_ind->vdev,
						     NDP_INDICATION,
						     ndp_ind);

	return status;
}

static QDF_STATUS nan_handle_responder_rsp(
				struct nan_datapath_responder_rsp *rsp,
				struct wlan_objmgr_vdev **vdev)
{
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_objmgr_vdev *nan_vdev;
	uint8_t nan_vdev_id;
	uint8_t ndi_vdev_id;
	bool is_peer_migrated = false;

	*vdev = rsp->vdev;
	ndi_vdev_id = wlan_vdev_get_id(rsp->vdev);

	psoc = wlan_vdev_get_psoc(rsp->vdev);
	if (!psoc) {
		nan_err("psoc is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (QDF_IS_STATUS_SUCCESS(rsp->status) && rsp->create_peer) {
		nan_vdev = wlan_objmgr_get_vdev_by_opmode_from_psoc(
							psoc,
							QDF_NAN_DISC_MODE,
							WLAN_NAN_ID);
		if (!nan_vdev) {
			nan_err("vdev is null");
			return QDF_STATUS_E_NULL_VALUE;
		}

		nan_vdev_id = wlan_vdev_get_id(nan_vdev);
		if (nan_is_peer_exist_for_opmode(psoc, &rsp->peer_mac_addr,
						 QDF_NAN_DISC_MODE)) {
			status = ndi_cleanup_pasn_peer_from_nan(
							psoc, nan_vdev_id,
							ndi_vdev_id,
							&rsp->peer_mac_addr);

			if (QDF_IS_STATUS_SUCCESS(status))
				is_peer_migrated = true;
		}

		wlan_objmgr_vdev_release_ref(nan_vdev, WLAN_NAN_ID);

		if (!is_peer_migrated)
			status = psoc_nan_obj->cb_obj.add_ndi_peer(ndi_vdev_id,
							rsp->peer_mac_addr);
		if (QDF_IS_STATUS_ERROR(status)) {
			nan_err("Couldn't add ndi peer");
			rsp->status = QDF_STATUS_E_FAILURE;
		}
	}

	psoc_nan_obj->cb_obj.os_if_ndp_event_handler(psoc, rsp->vdev,
						     NDP_RESPONDER_RSP, rsp);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_handle_ndp_end_rsp(
			struct nan_datapath_end_rsp_event *rsp,
			struct wlan_objmgr_vdev **vdev)
{
	struct wlan_objmgr_psoc *psoc;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct osif_request *request;

	*vdev = rsp->vdev;
	psoc = wlan_vdev_get_psoc(rsp->vdev);
	if (!psoc) {
		nan_err("psoc is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	/* Unblock the wait here if NDP_END request is a failure */
	if (rsp->status != 0) {
		request = osif_request_get(psoc_nan_obj->ndp_request_ctx);
		if (request) {
			osif_request_complete(request);
			osif_request_put(request);
		}
	}
	psoc_nan_obj->cb_obj.os_if_ndp_event_handler(psoc, rsp->vdev,
						     NDP_END_RSP, rsp);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_handle_end_ind(
				struct nan_datapath_end_indication_event *ind)
{
	uint32_t i;
	struct wlan_objmgr_psoc *psoc;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_objmgr_vdev *vdev_itr;
	struct nan_vdev_priv_obj *vdev_nan_obj;
	struct osif_request *request;

	psoc = wlan_vdev_get_psoc(ind->vdev);
	if (!psoc) {
		nan_err("psoc is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	/* Decrement NDP sessions for all Peers in the event */
	for (i = 0; i < ind->num_ndp_ids; i++)
		nan_decrement_ndp_sessions(psoc,
					   &ind->ndp_map[i].peer_ndi_mac_addr);

	for (i = 0; i < ind->num_ndp_ids; i++) {
		vdev_itr = wlan_objmgr_get_vdev_by_id_from_psoc(psoc,
							ind->ndp_map[i].vdev_id,
							WLAN_NAN_ID);
		if (!vdev_itr) {
			nan_err("NAN vdev object is NULL");
			continue;
		}

		vdev_nan_obj = nan_get_vdev_priv_obj(vdev_itr);
		if (!vdev_nan_obj) {
			wlan_objmgr_vdev_release_ref(vdev_itr, WLAN_NAN_ID);
			nan_err("NAN vdev private object is NULL");
			continue;
		}

		if (qdf_is_macaddr_equal(&vdev_nan_obj->primary_peer_mac,
					 &ind->ndp_map[i].peer_ndi_mac_addr))
			ndi_remove_and_update_primary_connection(psoc,
								 vdev_itr);

		wlan_objmgr_vdev_release_ref(vdev_itr, WLAN_NAN_ID);
	}

	psoc_nan_obj->cb_obj.ndp_delete_peers(ind->ndp_map, ind->num_ndp_ids);
	psoc_nan_obj->cb_obj.os_if_ndp_event_handler(psoc, ind->vdev,
						     NDP_END_IND, ind);

	/* Unblock the NDP_END wait */
	request = osif_request_get(psoc_nan_obj->ndp_request_ctx);
	if (request) {
		osif_request_complete(request);
		osif_request_put(request);
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_handle_enable_rsp(struct nan_event_params *nan_event)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status;
	void (*call_back)(void *cookie);
	uint8_t vdev_id;
	void (*nan_conc_callback)(void);
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;

	psoc = nan_event->psoc;
	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_NAN_ID);
	if (!pdev) {
		nan_err("null pdev");
		status = QDF_STATUS_E_INVAL;
		goto fail;
	}

	vdev = wlan_objmgr_pdev_get_first_vdev(pdev, WLAN_NAN_ID);
	if (!vdev) {
		nan_err("No vdev is up yet, unable to proceed!");
		wlan_objmgr_pdev_release_ref(pdev, WLAN_NAN_ID);
		status = QDF_STATUS_E_INVAL;
		goto fail;
	}

	if (nan_event->is_nan_enable_success) {
		status = nan_set_discovery_state(psoc, NAN_DISC_ENABLED);

		if (QDF_IS_STATUS_SUCCESS(status)) {
			psoc_nan_obj->nan_disc_mac_id = nan_event->mac_id;
			vdev_id = nan_event->vdev_id;
			if (!ucfg_nan_is_vdev_creation_allowed(psoc)) {
				vdev_id = NAN_PSEUDO_VDEV_ID;
			} else if (vdev_id >= WLAN_MAX_VDEVS) {
				nan_err("Invalid NAN vdev_id: %u", vdev_id);
				wlan_objmgr_pdev_release_ref(pdev, WLAN_NAN_ID);
				wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
				goto fail;
			}
			nan_debug("NAN vdev_id: %u", vdev_id);
			policy_mgr_incr_active_session(psoc, QDF_NAN_DISC_MODE,
						       vdev_id);
			policy_mgr_process_force_scc_for_nan(psoc);

			if_mgr_deliver_event(vdev,
					     WLAN_IF_MGR_EV_NAN_POST_ENABLE,
					     NULL);

		} else {
			/*
			 * State set to DISABLED OR DISABLE_IN_PROGRESS, try to
			 * restore the single MAC mode.
			 */
			psoc_nan_obj->nan_social_ch_2g_freq = 0;
			psoc_nan_obj->nan_social_ch_5g_freq = 0;
			policy_mgr_check_n_start_opportunistic_timer(psoc);
		}
		wlan_objmgr_pdev_release_ref(pdev, WLAN_NAN_ID);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
		goto done;
	} else {
		nan_info("NAN enable has failed");
		wlan_objmgr_pdev_release_ref(pdev, WLAN_NAN_ID);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
		/* NAN Enable has failed, restore changes */
		goto fail;
	}

fail:
	psoc_nan_obj->nan_social_ch_2g_freq = 0;
	psoc_nan_obj->nan_social_ch_5g_freq = 0;
	nan_set_discovery_state(psoc, NAN_DISC_DISABLED);
	if (ucfg_is_nan_dbs_supported(psoc))
		policy_mgr_check_n_start_opportunistic_timer(psoc);

	/*
	 * If FW respond with NAN enable failure, then TDLS should be enable
	 * again if there is TDLS connection exist earlier.
	 * decrement the active TDLS session.
	 */
	ucfg_tdls_notify_connect_failure(psoc);

done:
	nan_cstats_log_nan_enable_resp_evt(nan_event);

	nan_conc_callback = psoc_nan_obj->cb_obj.nan_concurrency_update;
	if (nan_conc_callback)
		nan_conc_callback();
	call_back = psoc_nan_obj->cb_obj.ucfg_nan_request_process_cb;
	if (call_back)
		call_back(psoc_nan_obj->nan_disc_request_ctx);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS nan_disable_cleanup(struct wlan_objmgr_psoc *psoc)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	QDF_STATUS status;
	uint8_t vdev_id;
	void (*nan_conc_callback)(void);
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;

	if (!psoc) {
		nan_err("psoc is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_NAN_ID);
	if (!pdev) {
		nan_err("null pdev");
		return QDF_STATUS_E_NULL_VALUE;
	}

	vdev = wlan_objmgr_pdev_get_first_vdev(pdev, WLAN_NAN_ID);
	if (!vdev) {
		nan_err("No vdev is up yet, unable to proceed!");
		wlan_objmgr_pdev_release_ref(pdev, WLAN_NAN_ID);
		return QDF_STATUS_E_NULL_VALUE;
	}

	status = nan_set_discovery_state(psoc, NAN_DISC_DISABLED);
	if (QDF_IS_STATUS_SUCCESS(status)) {
		void (*call_back)(void *cookie);

		call_back = psoc_nan_obj->cb_obj.ucfg_nan_request_process_cb;
		vdev_id = policy_mgr_mode_specific_vdev_id(psoc,
							   PM_NAN_DISC_MODE);
		nan_debug("NAN vdev_id: %u", vdev_id);
		policy_mgr_decr_session_set_pcl(psoc, QDF_NAN_DISC_MODE,
						vdev_id);
		if (psoc_nan_obj->is_explicit_disable && call_back)
			call_back(psoc_nan_obj->nan_disc_request_ctx);

		nan_handle_emlsr_concurrency(psoc, false);
		policy_mgr_nan_sap_post_disable_conc_check(psoc);
		nan_cstats_log_nan_disable_resp_evt(vdev_id, psoc);

		if_mgr_deliver_event(vdev,
				     WLAN_IF_MGR_EV_NAN_POST_DISABLE,
				     NULL);
		wlan_objmgr_pdev_release_ref(pdev, WLAN_NAN_ID);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);

	} else {
		/* Should not happen, NAN state can always be disabled */
		nan_err("Cannot set NAN state to disabled!");
		wlan_objmgr_pdev_release_ref(pdev, WLAN_NAN_ID);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
		return QDF_STATUS_E_FAILURE;
	}
	nan_conc_callback = psoc_nan_obj->cb_obj.nan_concurrency_update;
	if (nan_conc_callback)
		nan_conc_callback();

	return status;
}

static QDF_STATUS nan_handle_disable_ind(struct nan_event_params *nan_event)
{
	return nan_disable_cleanup(nan_event->psoc);
}

static QDF_STATUS nan_handle_schedule_update(
				struct nan_datapath_sch_update_event *ind)
{
	struct wlan_objmgr_psoc *psoc;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_objmgr_peer *peer;

	psoc = wlan_vdev_get_psoc(ind->vdev);
	if (!psoc) {
		nan_err("psoc is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer = wlan_objmgr_get_peer_by_mac(psoc, ind->peer_addr.bytes,
					   WLAN_NAN_ID);
	if (peer) {
		wlan_dp_notify_ndp_channel_info(peer, &ind->ch[0],
						ind->num_channels);
		wlan_objmgr_peer_release_ref(peer, WLAN_NAN_ID);
	}

	ndi_update_ndp_session(ind->vdev, &ind->peer_addr, &ind->ch[0]);
	psoc_nan_obj->cb_obj.os_if_ndp_event_handler(psoc, ind->vdev,
						     NDP_SCHEDULE_UPDATE, ind);

	return QDF_STATUS_SUCCESS;
}

/**
 * nan_handle_host_update() - extract the vdev from host event
 * @evt: Event data received from firmware
 * @vdev: pointer to vdev
 *
 * Return: none
 */
static void nan_handle_host_update(struct nan_datapath_host_event *evt,
					 struct wlan_objmgr_vdev **vdev)
{
	*vdev = evt->vdev;
}

/**
 * nan_handle_de_ind() - API to handle DE indication event.
 * @nan_event: pointer to NAN event params structure
 *
 * This API caches NAN MAC address in NAN PSOC private object.
 *
 * Return: none
 */
static void nan_handle_de_ind(struct nan_event_params *nan_event)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_objmgr_psoc *psoc;

	psoc = nan_event->psoc;
	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return;
	}

	psoc_nan_obj->fw_nan_addr = nan_event->nan_mac_addr;

	nan_debug("nan addr " QDF_MAC_ADDR_FMT,
		  QDF_MAC_ADDR_REF(psoc_nan_obj->fw_nan_addr.bytes));
}

QDF_STATUS nan_discovery_event_handler(struct scheduler_msg *msg)
{
	struct nan_event_params *nan_event;
	struct nan_psoc_priv_obj *psoc_nan_obj;

	if (!msg || !msg->bodyptr) {
		nan_err("msg body is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nan_event = msg->bodyptr;
	if (!nan_event->psoc) {
		nan_err("psoc is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(nan_event->psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	switch (msg->type) {
	case nan_event_id_enable_rsp:
		nan_handle_enable_rsp(nan_event);
		break;
	case nan_event_id_disable_ind:
		nan_handle_disable_ind(nan_event);
		break;
	case nan_event_id_generic_rsp:
	case nan_event_id_error_rsp:
		break;
	case nan_event_id_de_ind:
		nan_handle_de_ind(nan_event);
		break;
	default:
		nan_err("Unknown event ID type - %d", msg->type);
		break;
	}

	psoc_nan_obj->cb_obj.os_if_nan_event_handler(nan_event);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS nan_datapath_event_handler(struct scheduler_msg *pe_msg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_serialization_queued_cmd_info cmd;

	cmd.requestor = WLAN_UMAC_COMP_NAN;
	cmd.cmd_id = 0;
	cmd.req_type = WLAN_SER_CANCEL_NON_SCAN_CMD;
	cmd.queue_type = WLAN_SERIALIZATION_ACTIVE_QUEUE;

	if (!pe_msg->bodyptr) {
		nan_err("msg body is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	switch (pe_msg->type) {
	case NDP_CONFIRM: {
		nan_handle_confirm(pe_msg->bodyptr);
		break;
	}
	case NDP_INITIATOR_RSP: {
		nan_handle_initiator_rsp(pe_msg->bodyptr, &cmd.vdev);
		cmd.cmd_type = WLAN_SER_CMD_NDP_INIT_REQ;
		wlan_serialization_remove_cmd(&cmd);
		break;
	}
	case NDP_INDICATION: {
		nan_handle_ndp_ind(pe_msg->bodyptr);
		break;
	}
	case NDP_RESPONDER_RSP:
		nan_handle_responder_rsp(pe_msg->bodyptr, &cmd.vdev);
		cmd.cmd_type = WLAN_SER_CMD_NDP_RESP_REQ;
		wlan_serialization_remove_cmd(&cmd);
		break;
	case NDP_END_RSP:
		nan_handle_ndp_end_rsp(pe_msg->bodyptr, &cmd.vdev);
		cmd.cmd_type = WLAN_SER_CMD_NDP_DATA_END_INIT_REQ;
		wlan_serialization_remove_cmd(&cmd);
		break;
	case NDP_END_IND:
		nan_handle_end_ind(pe_msg->bodyptr);
		break;
	case NDP_SCHEDULE_UPDATE:
		nan_handle_schedule_update(pe_msg->bodyptr);
		break;
	case NDP_HOST_UPDATE:
		nan_handle_host_update(pe_msg->bodyptr, &cmd.vdev);
		cmd.cmd_type = WLAN_SER_CMD_NDP_END_ALL_REQ;
		wlan_serialization_remove_cmd(&cmd);
		break;
	default:
		nan_alert("Unhandled NDP event: %d", pe_msg->type);
		status = QDF_STATUS_E_NOSUPPORT;
		break;
	}
	return status;
}

bool nan_is_enable_allowed(struct wlan_objmgr_psoc *psoc, uint32_t nan_ch_freq,
			   uint8_t vdev_id)
{
	if (!psoc) {
		nan_err("psoc object object is NULL");
		return false;
	}

	return (NAN_DISC_DISABLED == nan_get_discovery_state(psoc) &&
		policy_mgr_allow_concurrency(psoc, PM_NAN_DISC_MODE,
					     nan_ch_freq, HW_MODE_20_MHZ,
					     0, vdev_id));
}

bool nan_is_disc_active(struct wlan_objmgr_psoc *psoc)
{
	if (!psoc) {
		nan_err("psoc object object is NULL");
		return false;
	}

	return (NAN_DISC_ENABLED == nan_get_discovery_state(psoc) ||
		NAN_DISC_ENABLE_IN_PROGRESS == nan_get_discovery_state(psoc));
}

static QDF_STATUS nan_set_hw_mode(struct wlan_objmgr_psoc *psoc,
				  uint32_t nan_ch_freq)
{
	QDF_STATUS status = QDF_STATUS_E_INVAL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	uint8_t vdev_id;

	policy_mgr_stop_opportunistic_timer(psoc);

	if (policy_mgr_is_hw_mode_change_in_progress(psoc)) {
		status = policy_mgr_wait_for_connection_update(psoc);
		if (!QDF_IS_STATUS_SUCCESS(status)) {
			nan_err("Failed to wait for connection update");
			goto pre_enable_failure;
		}
	}

	pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_NAN_ID);
	if (!pdev) {
		nan_err("null pdev");
		status = QDF_STATUS_E_INVAL;
		goto pre_enable_failure;
	}

	/* Piggyback on any available vdev for policy manager update */
	vdev = wlan_objmgr_pdev_get_first_vdev(pdev, WLAN_NAN_ID);
	if (!vdev) {
		nan_err("No vdev is up yet, unable to proceed!");
		status = QDF_STATUS_E_INVAL;
		goto pre_enable_failure;
	}
	vdev_id = wlan_vdev_get_id(vdev);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);

	status = policy_mgr_update_and_wait_for_connection_update(psoc, vdev_id,
								  nan_ch_freq,
					POLICY_MGR_UPDATE_REASON_NAN_DISCOVERY);
	if (QDF_IS_STATUS_ERROR(status)) {
		nan_err("Failed to set or wait for HW mode change");
		goto pre_enable_failure;
	}

	if (wlan_util_is_vdev_in_cac_wait(pdev, WLAN_NAN_ID)) {
		nan_err_rl("cac is in progress");
		status = QDF_STATUS_E_FAILURE;
		goto pre_enable_failure;
	}

pre_enable_failure:
	if (pdev)
		wlan_objmgr_pdev_release_ref(pdev, WLAN_NAN_ID);

	return status;
}

void nan_handle_emlsr_concurrency(struct wlan_objmgr_psoc *psoc,
				  bool nan_enable)
{
	/*
	 * For ganges EMLSR disallowed is set via change notify.
	 */
	if (policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) &&
	    wlan_mlme_is_aux_emlsr_support(psoc))
		return;

	if (nan_enable) {
		/*
		 * Check if any set link is already progress,
		 * wait for it to complete
		 */
		policy_mgr_wait_for_set_link_update(psoc);

		wlan_handle_emlsr_sta_concurrency(psoc, true, false);

		/* Wait till rsp is received if NAN enable causes a set link */
		policy_mgr_wait_for_set_link_update(psoc);
	} else {
		wlan_handle_emlsr_sta_concurrency(psoc, false, true);
	}
}

bool nan_is_sta_sta_concurrency_present(struct wlan_objmgr_psoc *psoc)
{
	uint32_t sta_cnt;

	sta_cnt = policy_mgr_mode_specific_connection_count(psoc, PM_STA_MODE,
							    NULL);
	/* Allow if STA is not in connected state */
	if (!sta_cnt)
		return false;

	/*
	 * sta > 2 : (STA + STA + STA) or (ML STA + STA) or (ML STA + ML STA),
	 * STA concurrency will be present.
	 *
	 * ML STA: Although both links would be treated as separate STAs
	 * (sta cnt = 2) from policy mgr perspective, but it is not considered
	 * as STA concurrency
	 */
	if (sta_cnt > 2 ||
	    (sta_cnt == 2 && policy_mgr_is_non_ml_sta_present(psoc)))
		return true;

	return false;
}

QDF_STATUS nan_discovery_pre_enable(struct wlan_objmgr_pdev *pdev,
				    uint32_t nan_ch_freq)
{
	QDF_STATUS status = QDF_STATUS_E_INVAL;
	struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(pdev);
	struct wlan_objmgr_vdev *vdev = NULL;

	if (!psoc) {
		nan_err("psoc is null");
		return QDF_STATUS_E_INVAL;
	}

	status = nan_set_discovery_state(psoc, NAN_DISC_ENABLE_IN_PROGRESS);

	if (QDF_IS_STATUS_ERROR(status)) {
		nan_err("Unable to set NAN Disc State to ENABLE_IN_PROGRESS");
		goto pre_enable_failure;
	}

	if ((policy_mgr_get_sap_mode_count(psoc, NULL)) &&
	    !policy_mgr_nan_sap_pre_enable_conc_check(psoc, PM_NAN_DISC_MODE,
						      nan_ch_freq)) {
		nan_debug("NAN not enabled due to concurrency constraints");
		status = QDF_STATUS_E_INVAL;
		goto pre_enable_failure;
	}

	/*
	 * Reject STA+STA in below case
	 * Non-ML STA: STA+STA+NAN concurrency is not supported
	 */
	if (nan_is_sta_sta_concurrency_present(psoc)) {
		nan_err("STA+STA+NAN concurrency is not allowed");
		status = QDF_STATUS_E_FAILURE;
		goto pre_enable_failure;
	}

	wlan_p2p_abort_scan(pdev);

	if (policy_mgr_is_hw_dbs_capable(psoc)) {
		status = nan_set_hw_mode(psoc, nan_ch_freq);
		if (QDF_IS_STATUS_ERROR(status))
			goto pre_enable_failure;
	}

	nan_handle_emlsr_concurrency(psoc, true);

	/* Try to teardown TDLS links, but do not wait */
	status = ucfg_tdls_teardown_links(psoc);
	if (QDF_IS_STATUS_ERROR(status)) {
		nan_err("Failed to teardown TDLS links");
	} else {
		vdev = wlan_objmgr_pdev_get_first_vdev(pdev, WLAN_NAN_ID);
		if (!vdev) {
			nan_err("No vdev is up yet, unable to proceed!");
			status = QDF_STATUS_E_INVAL;
			goto pre_enable_failure;
		}

		if_mgr_deliver_event(vdev, WLAN_IF_MGR_EV_NAN_PRE_ENABLE, NULL);

		wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
	}
pre_enable_failure:
	if (QDF_IS_STATUS_ERROR(status))
		nan_set_discovery_state(psoc, NAN_DISC_DISABLED);

	return status;
}

static QDF_STATUS nan_discovery_disable_req(struct nan_disable_req *req)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_nan_tx_ops *tx_ops;

	/*
	 * State was already set to Disabled by failed Enable
	 * request OR by the Disable Indication event, drop the
	 * Disable request.
	 */
	if (NAN_DISC_DISABLED == nan_get_discovery_state(req->psoc))
		return QDF_STATUS_SUCCESS;

	psoc_nan_obj = nan_get_psoc_priv_obj(req->psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	tx_ops = &psoc_nan_obj->tx_ops;
	if (!tx_ops->nan_discovery_req_tx) {
		nan_err("NAN Discovery tx op is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	return tx_ops->nan_discovery_req_tx(req, NAN_DISABLE_REQ);
}

static QDF_STATUS nan_discovery_enable_req(struct nan_enable_req *req)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_nan_tx_ops *tx_ops;

	/*
	 * State was already set to Disable in progress by a disable request,
	 * drop the Enable request, start opportunistic timer and move back to
	 * the Disabled state.
	 */
	if (NAN_DISC_DISABLE_IN_PROGRESS ==
			nan_get_discovery_state(req->psoc)) {
		policy_mgr_check_n_start_opportunistic_timer(req->psoc);
		return nan_set_discovery_state(req->psoc, NAN_DISC_DISABLED);
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(req->psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj->nan_social_ch_2g_freq = req->social_chan_2g_freq;
	psoc_nan_obj->nan_social_ch_5g_freq = req->social_chan_5g_freq;

	tx_ops = &psoc_nan_obj->tx_ops;
	if (!tx_ops->nan_discovery_req_tx) {
		nan_err("NAN Discovery tx op is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	return tx_ops->nan_discovery_req_tx(req, NAN_ENABLE_REQ);
}

static QDF_STATUS nan_discovery_generic_req(struct nan_generic_req *req)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct wlan_nan_tx_ops *tx_ops;

	psoc_nan_obj = nan_get_psoc_priv_obj(req->psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	tx_ops = &psoc_nan_obj->tx_ops;
	if (!tx_ops->nan_discovery_req_tx) {
		nan_err("NAN Discovery tx op is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	return tx_ops->nan_discovery_req_tx(req, NAN_GENERIC_REQ);
}

QDF_STATUS nan_discovery_flush_callback(struct scheduler_msg *msg)
{
	struct wlan_objmgr_psoc *psoc;

	if (!msg || !msg->bodyptr) {
		nan_err("Null pointer for NAN Discovery message");
		return QDF_STATUS_E_INVAL;
	}

	switch (msg->type) {
	case NAN_ENABLE_REQ:
		psoc = ((struct nan_enable_req *)msg->bodyptr)->psoc;
		break;
	case NAN_DISABLE_REQ:
		psoc = ((struct nan_disable_req *)msg->bodyptr)->psoc;
		break;
	case NAN_GENERIC_REQ:
		psoc = ((struct nan_generic_req *)msg->bodyptr)->psoc;
		break;
	default:
		nan_err("Unsupported request type: %d", msg->type);
		qdf_mem_free(msg->bodyptr);
		return QDF_STATUS_E_INVAL;
	}

	wlan_objmgr_psoc_release_ref(psoc, WLAN_NAN_ID);
	qdf_mem_free(msg->bodyptr);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS nan_discovery_scheduled_handler(struct scheduler_msg *msg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!msg || !msg->bodyptr) {
		nan_alert("msg or bodyptr is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	switch (msg->type) {
	case NAN_ENABLE_REQ:
		status = nan_discovery_enable_req(msg->bodyptr);
		break;
	case NAN_DISABLE_REQ:
		status = nan_discovery_disable_req(msg->bodyptr);
		break;
	case NAN_GENERIC_REQ:
		status = nan_discovery_generic_req(msg->bodyptr);
		break;
	default:
		nan_err("Unsupported request type: %d", msg->type);
		qdf_mem_free(msg->bodyptr);
		return QDF_STATUS_E_FAILURE;
	}

	nan_discovery_flush_callback(msg);
	return status;
}

QDF_STATUS
wlan_nan_get_connection_info(struct wlan_objmgr_psoc *psoc,
			     struct policy_mgr_vdev_entry_info *conn_info)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;

	if (!psoc) {
		nan_err("psoc obj is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (nan_get_discovery_state(psoc) != NAN_DISC_ENABLED) {
		nan_err("NAN State needs to be Enabled");
		return QDF_STATUS_E_INVAL;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	/* For policy_mgr use NAN mandatory Social ch 6 */
	conn_info->mhz = psoc_nan_obj->nan_social_ch_2g_freq;
	conn_info->mac_id = psoc_nan_obj->nan_disc_mac_id;
	conn_info->chan_width = CH_WIDTH_20MHZ;

	return QDF_STATUS_SUCCESS;
}

qdf_freq_t wlan_nan_get_disc_24g_ch_freq(struct wlan_objmgr_psoc *psoc)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return 0;
	}

	if (nan_get_discovery_state(psoc) != NAN_DISC_ENABLED)
		return 0;

	return psoc_nan_obj->nan_social_ch_2g_freq;
}

uint32_t wlan_nan_get_disc_5g_ch_freq(struct wlan_objmgr_psoc *psoc)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return 0;
	}

	if (nan_get_discovery_state(psoc) != NAN_DISC_ENABLED)
		return 0;

	return psoc_nan_obj->nan_social_ch_5g_freq;
}

qdf_freq_t wlan_nan_get_5ghz_social_ch_freq(struct wlan_objmgr_pdev *pdev)
{
	qdf_freq_t freq = 0;

	freq = wlan_nan_get_disc_5g_ch_freq(wlan_pdev_get_psoc(pdev));

	if (freq)
		goto done;

	if (wlan_reg_is_freq_enabled(pdev, NAN_5GHZ_SOCIAL_CH_149_FREQ,
				     REG_CURRENT_PWR_MODE))
		freq = NAN_5GHZ_SOCIAL_CH_149_FREQ;
	else if (wlan_reg_is_freq_enabled(pdev, NAN_5GHZ_SOCIAL_CH_44_FREQ,
					  REG_CURRENT_PWR_MODE))
		freq = NAN_5GHZ_SOCIAL_CH_44_FREQ;

done:
	return freq;
}

bool wlan_nan_get_sap_conc_support(struct wlan_objmgr_psoc *psoc)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return 0;
	}

	return (psoc_nan_obj->nan_caps.nan_sap_supported &&
		ucfg_is_nan_conc_control_supported(psoc));
}

bool wlan_nan_is_beamforming_supported(struct wlan_objmgr_psoc *psoc)
{
	return ucfg_nan_is_beamforming_supported(psoc);
}

/*
 * The NAN Cluster ID is a MAC address that takes a value from
 * 50-6F-9A-01-00-00 to 50-6F-9A-01-FF-FF and is carried in the A3 field of
 * some of the NAN frames. The NAN Cluster ID is randomly chosen by the device
 * that initiates the NAN Cluster.
 */
#define NAN_CLUSTER_MATCH      "\x50\x6F\x9A\x01"
#define NAN_CLUSTER_MATCH_SIZE 4

/**
 * wlan_nan_is_bssid_in_cluster() - to check whether BSSID is a part of NAN
 * cluster
 * @bssid: BSSID present in mgmt frame
 *
 * Return: true if BSSID is part of NAN cluster
 */
static
bool wlan_nan_is_bssid_in_cluster(tSirMacAddr bssid)
{
	if (qdf_mem_cmp(bssid, NAN_CLUSTER_MATCH, NAN_CLUSTER_MATCH_SIZE) == 0)
		return true;

	return false;
}

/**
 * wlan_nan_extract_vdev_id_from_vdev_list() -retrieve vdev from vdev list in
 * pdev and check for nan vdev_id
 * @pdev: PDEV object
 * @dbg_id: Object Manager ref debug id
 *
 * API to get NAN vdev_id for only NAN BSSID.
 *
 * Return: NAN vdev_id
 */
static
uint8_t wlan_nan_extract_vdev_id_from_vdev_list(struct wlan_objmgr_pdev *pdev,
						wlan_objmgr_ref_dbgid dbg_id)
{
	struct wlan_objmgr_pdev_objmgr *objmgr = &pdev->pdev_objmgr;
	qdf_list_t *vdev_list = NULL;
	struct wlan_objmgr_vdev *vdev;
	qdf_list_node_t *node = NULL;
	qdf_list_node_t *prev_node = NULL;
	uint8_t vdev_id = WLAN_UMAC_VDEV_ID_MAX;

	wlan_pdev_obj_lock(pdev);

	vdev_list = &objmgr->wlan_vdev_list;
	if (qdf_list_peek_front(vdev_list, &node) != QDF_STATUS_SUCCESS)
		goto end;

	do {
		vdev = qdf_container_of(node, struct wlan_objmgr_vdev,
					vdev_node);
		if (wlan_objmgr_vdev_try_get_ref(vdev, dbg_id) ==
		    QDF_STATUS_SUCCESS) {
			if (wlan_vdev_mlme_get_opmode(vdev) ==
			    QDF_NAN_DISC_MODE) {
				vdev_id = wlan_vdev_get_id(vdev);
				wlan_objmgr_vdev_release_ref(vdev, dbg_id);
				goto end;
			}

			wlan_objmgr_vdev_release_ref(vdev, dbg_id);
		}

		prev_node = node;
	} while (qdf_list_peek_next(vdev_list, prev_node, &node) ==
		 QDF_STATUS_SUCCESS);
end:
	wlan_pdev_obj_unlock(pdev);

	return vdev_id;
}

uint8_t nan_get_vdev_id_from_bssid(struct wlan_objmgr_pdev *pdev,
				   tSirMacAddr bssid,
				   wlan_objmgr_ref_dbgid dbg_id)
{
	uint8_t vdev_id = WLAN_UMAC_VDEV_ID_MAX;

	if (wlan_nan_is_bssid_in_cluster(bssid))
		vdev_id = wlan_nan_extract_vdev_id_from_vdev_list(pdev, dbg_id);

	return vdev_id;
}

QDF_STATUS nan_pasn_flush_callback(struct scheduler_msg *msg)
{
	if (!msg || !msg->bodyptr) {
		nan_err("Null pointer for NAN Discovery message");
		return QDF_STATUS_E_INVAL;
	}

	switch (msg->type) {
	case NAN_PASN_PEER_CREATE_REQ:
	case NAN_PASN_PEER_DELETE_REQ:
	case NAN_PASN_PEER_DELETE_ALL_REQ:
		break;
	default:
		nan_err("Unsupported request type: %d", msg->type);
		qdf_mem_free(msg->bodyptr);
		return QDF_STATUS_E_INVAL;
	}

	qdf_mem_free(msg->bodyptr);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS nan_pasn_scheduled_handler(struct scheduler_msg *msg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct nan_pasn_peer_ops *peer_ops;
	struct nan_pasn_peer_req *peer_params;
	struct wlan_objmgr_vdev *vdev;

	if (!msg || !msg->bodyptr) {
		nan_err("msg or bodyptr is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer_params = msg->bodyptr;
	psoc_nan_obj = nan_get_psoc_priv_obj(peer_params->psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	peer_ops = &psoc_nan_obj->cb_obj.pasn_peer_ops;

	switch (msg->type) {
	case NAN_PASN_PEER_CREATE_REQ:
		if (!peer_ops->nan_pasn_peer_create_cb) {
			nan_err("NAN PASN peer create ops is NULL");
			return QDF_STATUS_E_NULL_VALUE;
		}

		status = peer_ops->nan_pasn_peer_create_cb(peer_params->psoc,
							&peer_params->peer_addr,
							peer_params->vdev_id,
							NAN_PASN_PEER_CREATE);
		break;
	case NAN_PASN_PEER_DELETE_REQ:
		if (!peer_ops->nan_pasn_peer_delete_cb) {
			nan_err("NAN PASN peer delete ops is NULL");
			return QDF_STATUS_E_NULL_VALUE;
		}

		status = peer_ops->nan_pasn_peer_delete_cb(peer_params->psoc,
							peer_params->vdev_id,
							&peer_params->peer_addr,
							NAN_PASN_PEER_DELETE,
							false);
		break;
	case NAN_PASN_PEER_DELETE_ALL_REQ:
		peer_ops = &psoc_nan_obj->cb_obj.pasn_peer_ops;
		if (!peer_ops->nan_pasn_peer_delete_all_cb) {
			nan_err("NAN PASN peer delete all ops is NULL");
			return QDF_STATUS_E_NULL_VALUE;
		}

		vdev = wlan_objmgr_get_vdev_by_id_from_psoc(peer_params->psoc,
							    peer_params->vdev_id,
							    WLAN_NAN_ID);
		if (!vdev) {
			nan_err("vdev is null");
			return QDF_STATUS_E_NULL_VALUE;
		}

		status = peer_ops->nan_pasn_peer_delete_all_cb(vdev);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
		break;

	default:
		nan_err("Unsupported request type: %d", msg->type);
		qdf_mem_free(msg->bodyptr);
		return QDF_STATUS_E_FAILURE;
	}

	nan_pasn_flush_callback(msg);
	return status;
}

void nan_handle_pasn_peer_create_rsp(struct wlan_objmgr_psoc *psoc,
				     uint8_t vdev_id,
				     struct qdf_mac_addr *peer_mac,
				     uint8_t peer_create_status)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	uint8_t *cookie;

	nan_debug("Received peer create response for " QDF_MAC_ADDR_FMT " and vdev id %d with status:%d",
		  QDF_MAC_ADDR_REF(peer_mac->bytes), vdev_id,
		  peer_create_status);

	if (!psoc) {
		nan_err("psoc is NULL");
		return;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return;
	}

	if (psoc_nan_obj->cb_obj.ucfg_nan_request_process_cb) {
		cookie = (uint8_t *)psoc_nan_obj->nan_pairing_create_ctx;
		psoc_nan_obj->cb_obj.ucfg_nan_request_process_cb(cookie);
	}
}

void nan_pasn_peer_handle_del_rsp(struct wlan_objmgr_psoc *psoc,
				  uint8_t *peer_mac, uint8_t vdev_id)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	struct nan_pasn_peer_ops *peer_ops;
	uint8_t *cookie;
	uint8_t ndi_vdev_id;
	struct qdf_mac_addr peer_mac_addr;
	bool is_peer_migrated;

	nan_debug("Received peer delete response for " QDF_MAC_ADDR_FMT " and vdev id %d",
		  QDF_MAC_ADDR_REF(peer_mac), vdev_id);

	if (!psoc) {
		nan_err("psoc is NULL");
		return;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return;
	}

	peer_ops = &psoc_nan_obj->cb_obj.pasn_peer_ops;
	if (!peer_ops->nan_pasn_peer_delete_cb) {
		nan_err("NAN PASN peer delete ops is NULL");
		return;
	}

	qdf_mem_copy(peer_mac_addr.bytes, peer_mac, QDF_MAC_ADDR_SIZE);

	is_peer_migrated = nan_is_peer_migrated(psoc, vdev_id, &peer_mac_addr);
	if (is_peer_migrated) {
		ndi_vdev_id = nan_get_ndi_vdev_id_from_migrated_peer(
								psoc,
								&peer_mac_addr);
		if (ndi_vdev_id == INVALID_VDEV_ID) {
			nan_err("vdev id is invalid");
			return;
		}
	}

	peer_ops->nan_pasn_peer_delete_cb(psoc, vdev_id, &peer_mac_addr,
					  NAN_PASN_PEER_DELETE, true);

	if (psoc_nan_obj->cb_obj.ucfg_nan_request_process_cb) {
		cookie = (uint8_t *)psoc_nan_obj->nan_pairing_create_ctx;
		psoc_nan_obj->cb_obj.ucfg_nan_request_process_cb(cookie);
	}

	if (is_peer_migrated)
		psoc_nan_obj->cb_obj.add_ndi_peer(ndi_vdev_id, peer_mac_addr);
}

/**
 * nan_delete_objmgr_pasn_peer() - Delete PASN peer objects for given vdev
 * @psoc: Pointer to psoc object
 * @object: pointer to object
 * @arg: pointer to arg
 *
 * Return: QDF_STATUS
 */
static void nan_delete_objmgr_pasn_peer(struct wlan_objmgr_psoc *psoc,
					void *object, void *arg)
{
	struct wlan_objmgr_peer *peer = object;
	struct wlan_objmgr_vdev *vdev = arg;
	uint8_t vdev_id, peer_vdev_id;
	enum wlan_peer_type peer_type;
	QDF_STATUS status;

	if (!peer) {
		nan_err("Peer is NULL");
		return;
	}

	peer_type = wlan_peer_get_peer_type(peer);
	if (peer_type != WLAN_PEER_NAN_PASN)
		return;

	if (!vdev) {
		nan_err("VDEV is NULL");
		return;
	}

	vdev_id = wlan_vdev_get_id(vdev);
	peer_vdev_id = wlan_vdev_get_id(wlan_peer_get_vdev(peer));
	if (vdev_id != peer_vdev_id)
		return;

	status = wlan_objmgr_peer_obj_delete(peer);
	if (QDF_IS_STATUS_ERROR(status))
		nan_err("Failed to delete peer");

	nan_update_pasn_peer_count(vdev, false);
}

QDF_STATUS nan_cleanup_pasn_peers(struct wlan_objmgr_psoc *psoc)
{
	QDF_STATUS status;
	struct nan_vdev_priv_obj *nan_vdev_obj;
	struct wlan_objmgr_vdev *vdev;

	vdev = wlan_objmgr_get_vdev_by_opmode_from_psoc(psoc, QDF_NAN_DISC_MODE,
							WLAN_NAN_ID);
	if (!vdev) {
		nan_err("vdev is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nan_debug("Iterate and delete PASN peers");
	status = wlan_objmgr_iterate_obj_list(psoc, WLAN_PEER_OP,
					      nan_delete_objmgr_pasn_peer,
					      vdev, 0, WLAN_NAN_ID);
	if (QDF_IS_STATUS_ERROR(status))
		nan_err("Delete objmgr peers failed");

	/*
	 * PASN Peer count should be zero here
	 */
	nan_vdev_obj = nan_get_vdev_priv_obj(vdev);
	if (nan_vdev_obj)
		nan_vdev_obj->num_pasn_peers = 0;

	wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);

	return status;
}

QDF_STATUS nan_handle_delete_all_pasn_peers(struct wlan_objmgr_psoc *psoc,
					    uint8_t vdev_id)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct nan_vdev_priv_obj *nan_vdev_obj;
	struct wlan_objmgr_vdev *vdev;
	struct nan_pasn_peer_ops *peer_ops;
	uint8_t *cookie;

	nan_debug("Received all peer delete response for vdev id %d", vdev_id);

	if (!psoc) {
		nan_err("psoc is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id, WLAN_NAN_ID);

	peer_ops = &psoc_nan_obj->cb_obj.pasn_peer_ops;
	if (!peer_ops->nan_pasn_peer_delete_all_complete_cb) {
		nan_err("pasn_peer_delete_all_complete_cb is NULL");
		status = QDF_STATUS_E_NULL_VALUE;
		goto ref_rel;
	}

	status = peer_ops->nan_pasn_peer_delete_all_complete_cb(vdev);
	if (QDF_IS_STATUS_ERROR(status)) {
		nan_err("delete all complete err");
		goto ref_rel;
	}

	nan_vdev_obj = nan_get_vdev_priv_obj(vdev);
	if (!nan_vdev_obj) {
		nan_err("NAN vdev priv obj is null");
		status = QDF_STATUS_E_FAILURE;
		goto ref_rel;
	}

	status = nan_cleanup_pasn_peers(psoc);
	if (QDF_IS_STATUS_ERROR(status)) {
		nan_err("nan clean up err");
		goto ref_rel;
	}

	nan_vdev_obj->is_delete_all_pasn_peer_in_progress = false;
	nan_vdev_obj->num_pasn_peers = 0;

	if (psoc_nan_obj->cb_obj.ucfg_nan_request_process_cb) {
		cookie = (uint8_t *)psoc_nan_obj->nan_pairing_create_ctx;
		psoc_nan_obj->cb_obj.ucfg_nan_request_process_cb(cookie);
	}

ref_rel:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_NAN_ID);
	return status;
}

struct qdf_mac_addr *nan_get_fw_addr(struct wlan_objmgr_psoc *psoc)
{
	struct nan_psoc_priv_obj *psoc_priv;

	psoc_priv = nan_get_psoc_priv_obj(psoc);
	if (!psoc_priv) {
		nan_err("psoc_nan_obj is null");
		return NULL;
	}

	return &psoc_priv->fw_nan_addr;
}
