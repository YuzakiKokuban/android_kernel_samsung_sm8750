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
 * DOC: contains definitions for NAN component
 */

#include "nan_public_structs.h"
#include "wlan_nan_api.h"
#include "../../core/src/nan_main_i.h"
#include "wlan_objmgr_vdev_obj.h"
#include "wlan_nan_api_i.h"
#include "cfg_nan_api.h"
#include "wlan_vdev_mgr_tgt_if_tx_api.h"

inline enum nan_datapath_state wlan_nan_get_ndi_state(
					struct wlan_objmgr_vdev *vdev)
{
	enum nan_datapath_state val;
	struct nan_vdev_priv_obj *priv_obj = nan_get_vdev_priv_obj(vdev);

	if (!priv_obj) {
		nan_err("priv_obj is null");
		return NAN_DATA_INVALID_STATE;
	}

	qdf_spin_lock_bh(&priv_obj->lock);
	val = priv_obj->state;
	qdf_spin_unlock_bh(&priv_obj->lock);

	return val;
}

uint8_t wlan_nan_get_vdev_id_from_bssid(struct wlan_objmgr_pdev *pdev,
					tSirMacAddr bssid,
					wlan_objmgr_ref_dbgid dbg_id)
{
	return nan_get_vdev_id_from_bssid(pdev, bssid, dbg_id);
}

bool wlan_nan_is_disc_active(struct wlan_objmgr_psoc *psoc)
{
	return nan_is_disc_active(psoc);
}

bool wlan_nan_is_eht_capable(struct wlan_objmgr_psoc *psoc)
{
	return cfg_nan_is_eht_cap_enable(psoc);
}

QDF_STATUS
wlan_nan_vdev_delete_all_pasn_peers(struct wlan_objmgr_vdev *vdev)
{
	QDF_STATUS status;
	struct peer_delete_all_params param;

	param.vdev_id = wlan_vdev_get_id(vdev);
	param.peer_type_bitmap = BIT(WLAN_PEER_NAN_PASN);

	status = tgt_vdev_mgr_peer_delete_all_send(vdev, &param);
	if (QDF_IS_STATUS_ERROR(status))
		nan_err("Send vdev delete all peers failed");

	return status;
}

void wlan_nan_handle_pasn_peer_create_rsp(struct wlan_objmgr_psoc *psoc,
					  uint8_t vdev_id,
					  struct qdf_mac_addr *peer_mac,
					  uint8_t peer_create_status)
{
	nan_handle_pasn_peer_create_rsp(psoc, vdev_id, peer_mac,
					peer_create_status);
}

void wlan_nan_pasn_peer_handle_del_rsp(struct wlan_objmgr_psoc *psoc,
				       uint8_t *peer_mac, uint8_t vdev_id)
{
	nan_pasn_peer_handle_del_rsp(psoc, peer_mac, vdev_id);
}

QDF_STATUS wlan_nan_handle_delete_all_pasn_peers(struct wlan_objmgr_psoc *psoc,
						 uint8_t vdev_id)
{
	return nan_handle_delete_all_pasn_peers(psoc, vdev_id);
}

QDF_STATUS wlan_ndi_add_pasn_peer_to_nan(struct wlan_objmgr_psoc *psoc,
					 uint8_t vdev_id,
					 struct qdf_mac_addr *peer_mac)
{
	return ndi_add_pasn_peer_to_nan(psoc, vdev_id, peer_mac);
}

bool wlan_nan_is_sta_p2p_ndp_supp_by_fw(struct wlan_objmgr_psoc *psoc)
{
	struct nan_psoc_priv_obj *psoc_nan_obj;

	psoc_nan_obj = nan_get_psoc_priv_obj(psoc);
	if (!psoc_nan_obj) {
		nan_err("psoc_nan_obj is null");
		return false;
	}

	return psoc_nan_obj->nan_caps.sta_p2p_ndp_conc;
}

bool wlan_nan_is_sta_p2p_ndp_supported(struct wlan_objmgr_psoc *psoc)
{
	return (wlan_nan_is_sta_p2p_ndp_supp_by_fw(psoc) &&
		cfg_nan_get_support_sta_p2p_ndp(psoc));
}

qdf_freq_t wlan_nan_get_24ghz_social_ch_freq(struct wlan_objmgr_pdev *pdev)
{
	qdf_freq_t freq = 0;

	freq = wlan_nan_get_disc_24g_ch_freq(wlan_pdev_get_psoc(pdev));

	if (!freq && wlan_reg_is_freq_enabled(pdev, NAN_2GHZ_SOCIAL_CH_FREQ,
					      REG_CURRENT_PWR_MODE))
		freq = NAN_2GHZ_SOCIAL_CH_FREQ;

	return freq;
}

