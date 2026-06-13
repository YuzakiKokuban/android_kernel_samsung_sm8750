/*
 * Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __WMA_PASN_PEER_API_H__
#define __WMA_PASN_PEER_API_H__

#include "qdf_types.h"
#include "osapi_linux.h"
#include "wmi_services.h"
#include "wmi_unified.h"
#include "wmi_unified_api.h"
#include "wlan_objmgr_psoc_obj.h"
#include "wma.h"

/**
 * struct pasn_peer_del_rsp_params  - Peer delete response parameters
 * @vdev_id: vdev id
 * @status: response status code
 */
struct pasn_peer_del_rsp_params {
	uint8_t vdev_id;
	uint32_t status;
};

/**
 * wma_pasn_peer_remove  - Remove RTT pasn peer and send peer delete command to
 * firmware
 * @psoc: PSOC object
 * @peer_addr: Peer mac address
 * @vdev_id: vdev id
 * @no_fw_peer_delete: Don't send peer delete to firmware
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wma_pasn_peer_remove(struct wlan_objmgr_psoc *psoc,
		     struct qdf_mac_addr *peer_addr,
		     uint8_t vdev_id, bool no_fw_peer_delete);

/**
 * wma_pasn_peer_create() - Create RTT PASN peer
 * @psoc: PSOC pointer
 * @peer_addr: Peer address
 * @vdev_id: vdev id
 * @type: type of PASN request (create/delete) for NAN/RTT
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wma_pasn_peer_create(struct wlan_objmgr_psoc *psoc,
		     struct qdf_mac_addr *peer_addr, uint8_t vdev_id,
		     uint8_t type);

/**
 * wma_pasn_handle_peer_create_conf() - Handle PASN peer create confirm event
 * @wma: WMA handle
 * @peer_mac: peer mac address
 * @req_msg_type: Request message type
 * @status: status
 * @vdev_id: vdev id
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wma_pasn_handle_peer_create_conf(tp_wma_handle wma,
				 struct qdf_mac_addr *peer_mac,
				 uint8_t req_msg_type,
				 QDF_STATUS status, uint8_t vdev_id);

/**
 * wma_delete_all_pasn_peers() - Delete all PASN peers
 * @vdev: Vdev object pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wma_delete_all_pasn_peers(struct wlan_objmgr_vdev *vdev);

QDF_STATUS
wma_resume_vdev_delete(tp_wma_handle wma, uint8_t vdev_id);

QDF_STATUS
wma_pasn_peer_delete_all_complete(struct wlan_objmgr_vdev *vdev);

/**
 * wma_nan_pasn_peer_remove() - Based on peer delete flag, send peer delete
 * request to target or remove NAN PASN peer from the object manager
 * @psoc: PSOC pointer
 * @vdev_id: vdev id
 * @peer_addr: Peer address
 * @type: type of PASN request (create/delete) for NAN/RTT
 * @objmgr_peer_delete: true means don't send peer delete to target but delete
 * object manager peer only and false means send peer delete to target but
 * do not delete object manager peer.
 *
 * Return: QDF status
 */
QDF_STATUS wma_nan_pasn_peer_remove(struct wlan_objmgr_psoc *psoc,
				    uint8_t vdev_id,
				    struct qdf_mac_addr *peer_addr,
				    uint8_t type,
				    bool objmgr_peer_delete);

/*
 * wma_pasn_peer_delete_handler() - handle peer delete
 * @psoc: PSOC pointer
 * @peer_mac: Peer address
 * @vdev_id: vdev id
 * @params: parameter to peer delete
 *
 * Return: QDF_STATUS
 */
void wma_pasn_peer_delete_handler(struct wlan_objmgr_psoc *psoc,
				  uint8_t *peer_mac, uint8_t vdev_id,
				  void *params);
#endif
