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

/**
 * DOC: contains prototypes for NAN component
 */

#ifndef _WLAN_NAN_API_I_H_
#define _WLAN_NAN_API_I_H_

#include "wlan_objmgr_cmn.h"
#include "nan_public_structs.h"

#ifdef WLAN_FEATURE_NAN
/**
 * wlan_nan_get_ndi_state: get ndi state from vdev obj
 * @vdev: pointer to vdev object
 *
 * Return: ndi state
 */
enum nan_datapath_state wlan_nan_get_ndi_state(struct wlan_objmgr_vdev *vdev);

/**
 * wlan_nan_get_vdev_id_from_bssid() - get NAN vdev_id for BSSID
 * @pdev: PDEV object
 * @bssid: BSSID present in mgmt frame
 * @dbg_id:   Object Manager ref debug id
 *
 * This is wrapper API for nan_get_vdev_id_from_bssid()
 *
 * Return: NAN vdev_id
 */
uint8_t wlan_nan_get_vdev_id_from_bssid(struct wlan_objmgr_pdev *pdev,
					tSirMacAddr bssid,
					wlan_objmgr_ref_dbgid dbg_id);

/**
 * wlan_nan_is_disc_active() - Check if NAN discovery is active
 * @psoc: Pointer to PSOC object
 *
 * Return: True if Discovery is active
 */
bool wlan_nan_is_disc_active(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_nan_is_eht_capable() - Get NAN EHT capability
 * @psoc: pointer to psoc object
 *
 * Return: Boolean flag indicating whether the NAN EHT capability is
 * disabled or not
 */
bool wlan_nan_is_eht_capable(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_nan_vdev_delete_all_pasn_peers: send delete all pasn peers cmd to FW
 * @vdev: pointer to vdev object
 *
 * Return: status of operation
 */
QDF_STATUS
wlan_nan_vdev_delete_all_pasn_peers(struct wlan_objmgr_vdev *vdev);

/*
 * wlan_nan_handle_pasn_peer_create_rsp: This API is wrapper for function
 * "nan_handle_pasn_peer_create_rsp"
 * @psoc: PSOC object
 * @vdev_id: vdev id
 * @peer_mac: peer mac address
 * @peer_create_status: peer create status
 *
 * Return: None
 */
void wlan_nan_handle_pasn_peer_create_rsp(struct wlan_objmgr_psoc *psoc,
					  uint8_t vdev_id,
					  struct qdf_mac_addr *peer_mac,
					  uint8_t peer_create_status);
/**
 * wlan_nan_pasn_peer_handle_del_rsp: wrapper API for
 *                                    "nan_pasn_peer_handle_del_rsp"
 * @psoc: pointer to psoc object
 * @peer_mac: address of peer
 * @vdev_id: vdev id
 *
 * Return: None
 */
void wlan_nan_pasn_peer_handle_del_rsp(struct wlan_objmgr_psoc *psoc,
				       uint8_t *peer_mac, uint8_t vdev_id);
/**
 * wlan_nan_handle_delete_all_pasn_peers: This API is wrapper for function
 * "nan_handle_delete_all_pasn_peers"
 * @psoc: pointer to psoc object
 * @vdev_id: vdev id
 *
 * Return: Success when handled response, otherwise error
 */
QDF_STATUS wlan_nan_handle_delete_all_pasn_peers(struct wlan_objmgr_psoc *psoc,
						 uint8_t vdev_id);

/**
 * wlan_ndi_add_pasn_peer_to_nan(): This API is wrapper for function
 * "ndi_add_pasn_peer_to_nan"
 * @psoc: pointer to PSOC object
 * @vdev_id: VDEV id
 * @peer_mac: address of peer
 *
 * Return: QDF status
 */
QDF_STATUS
wlan_ndi_add_pasn_peer_to_nan(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
			      struct qdf_mac_addr *peer_mac);

/**
 * wlan_nan_is_sta_p2p_ndp_supp_by_fw() - Check if STA+P2P+NDP conc is supported
 * @psoc: pointer to PSOC object
 *
 * Return: True if STA + P2P +NDP is supported
 */
bool wlan_nan_is_sta_p2p_ndp_supp_by_fw(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_nan_is_sta_p2p_ndp_supported() - Check if STA+P2P+NDP conc is supported
 * @psoc: pointer to PSOC object
 *
 * Return: True if STA + P2P +NDP is supported
 */
bool wlan_nan_is_sta_p2p_ndp_supported(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_nan_get_24ghz_social_ch_freq(): Get NAN 2.4 GHz social channel
 * @pdev: PDEV object
 *
 * API fetches 2.4 GHz NAN social channel from nan_psoc_priv_obj
 *
 * Return: NAN social channel frequency
 */
qdf_freq_t
wlan_nan_get_24ghz_social_ch_freq(struct wlan_objmgr_pdev *pdev);
#else
static inline
enum nan_datapath_state wlan_nan_get_ndi_state(struct wlan_objmgr_vdev *vdev)
{
	return NAN_DATA_INVALID_STATE;
}

static inline
uint8_t wlan_nan_get_vdev_id_from_bssid(struct wlan_objmgr_pdev *pdev,
					tSirMacAddr bssid,
					wlan_objmgr_ref_dbgid dbg_id)
{
	return INVALID_VDEV_ID;
}

static inline
bool wlan_nan_is_disc_active(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

static inline bool
wlan_nan_is_eht_capable(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

static inline QDF_STATUS
wlan_nan_vdev_delete_all_pasn_peers(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
void wlan_nan_handle_pasn_peer_create_rsp(struct wlan_objmgr_psoc *psoc,
					  uint8_t vdev_id,
					  struct qdf_mac_addr *peer_mac,
					  uint8_t peer_create_status)
{
}

static inline void
wlan_nan_pasn_peer_handle_del_rsp(struct wlan_objmgr_psoc *psoc,
				  uint8_t *peer_mac, uint8_t vdev_id)
{
}

static inline QDF_STATUS
wlan_nan_handle_delete_all_pasn_peers(struct wlan_objmgr_psoc *psoc,
				      uint8_t vdev_id)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
wlan_ndi_add_pasn_peer_to_nan(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
			      struct qdf_mac_addr *peer_mac)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline bool
wlan_nan_is_sta_p2p_ndp_supp_by_fw(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

static inline bool
wlan_nan_is_sta_p2p_ndp_supported(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

static inline qdf_freq_t
wlan_nan_get_24ghz_social_ch_freq(struct wlan_objmgr_pdev *pdev)
{
	return 0;
}

#endif /*WLAN_FEATURE_NAN */
#endif /*_WLAN_NAN_API_I_H_ */
