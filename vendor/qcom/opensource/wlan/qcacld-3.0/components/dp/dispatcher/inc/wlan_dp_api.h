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
 * DOC: wlan_dp_api.h
 *
 */

#if !defined(_WLAN_DP_API_H_)
#define _WLAN_DP_API_H_

#include <cdp_txrx_cmn_struct.h>

/**
 * wlan_dp_update_peer_map_unmap_version() - update peer map unmap version
 * @version: Peer map unmap version pointer to be updated
 *
 * Return: None
 */
void wlan_dp_update_peer_map_unmap_version(uint8_t *version);

/**
 * wlan_dp_runtime_suspend() - Runtime suspend DP handler
 * @soc: CDP SoC handle
 * @pdev_id: DP PDEV ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_runtime_suspend(ol_txrx_soc_handle soc, uint8_t pdev_id);

/**
 * wlan_dp_runtime_resume() - Runtime suspend DP handler
 * @soc: CDP SoC handle
 * @pdev_id: DP PDEV ID
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_runtime_resume(ol_txrx_soc_handle soc, uint8_t pdev_id);

/**
 * wlan_dp_print_fisa_rx_stats() - Dump fisa stats
 * @stats_id: ID for the stats to be dumped
 *
 * Return: None
 */
void wlan_dp_print_fisa_rx_stats(enum cdp_fisa_stats_id stats_id);

/**
 * wlan_dp_set_fst_in_cmem() - Set flag to indicate FST is in CMEM
 * @fst_in_cmem: Flag to indicate FST is in CMEM
 *
 * Return: None
 */
void wlan_dp_set_fst_in_cmem(bool fst_in_cmem);

/**
 * wlan_dp_set_fisa_dynamic_aggr_size_support - Set flag to indicate dynamic
 *						MSDU aggregation size programming supported
 * @dynamic_aggr_size_support: Flag to indicate dynamic aggregation size support
 *
 * Return: None
 */
void wlan_dp_set_fisa_dynamic_aggr_size_support(bool dynamic_aggr_size_support);

#ifdef WLAN_FEATURE_LOCAL_PKT_CAPTURE
/**
 * wlan_dp_is_local_pkt_capture_active() - Get local packet capture config
 * @psoc: pointer to psoc object
 *
 * Return: true if local packet capture is active, false otherwise
 */
bool
wlan_dp_is_local_pkt_capture_active(struct wlan_objmgr_psoc *psoc);
#else
static inline bool
wlan_dp_is_local_pkt_capture_active(struct wlan_objmgr_psoc *psoc)
{
	return false;
}
#endif /* WLAN_FEATURE_LOCAL_PKT_CAPTURE */

#ifdef WLAN_SUPPORT_FLOW_PRIORTIZATION
/**
 * wlan_dp_fpm_is_tid_override() - check network buffer marked with tid override
 * @nbuf: pointer to network buffer
 * @tid: buffer to get tid id skb marked with tid override tag
 *
 * Return: True if skb marked with tid override tag
 */
bool wlan_dp_fpm_is_tid_override(qdf_nbuf_t nbuf, uint8_t *tid);
#endif

/**
 * wlan_dp_update_def_link() - update DP interface default link
 * @psoc: psoc handle
 * @intf_mac: interface MAC address
 * @vdev: objmgr vdev handle to set the def_link in dp_intf
 *
 */
void wlan_dp_update_def_link(struct wlan_objmgr_psoc *psoc,
			     struct qdf_mac_addr *intf_mac,
			     struct wlan_objmgr_vdev *vdev);

#ifdef IPA_WDS_EASYMESH_FEATURE
/**
 * wlan_dp_send_ipa_wds_peer_map_event() - Notify IPA upon wds peer map event
 * @cpsoc: pointer to psoc object
 * @peer_id: peer id of direct connected peer
 * @hw_peer_id: hw peer id of direct peer
 * @vdev_id: vdev id
 * @wds_mac_addr: mac address of wds peer
 * @peer_type: WDS peer ast entry type
 * @tx_ast_hashidx: AST hash index
 *
 * This API notifies IPA driver with [peer_id, wds_mac_addr] upon wds peer
 * map event from target.
 *
 * Return: 0 for success, otherwise error codes
 */
int wlan_dp_send_ipa_wds_peer_map_event(struct cdp_ctrl_objmgr_psoc *cpsoc,
					uint16_t peer_id, uint16_t hw_peer_id,
					uint8_t vdev_id, uint8_t *wds_mac_addr,
					enum cdp_txrx_ast_entry_type peer_type,
					uint32_t tx_ast_hashidx);

/**
 * wlan_dp_send_ipa_wds_peer_unmap_event() - Notify IPA upon wds peer unmap
 *					     event
 * @cpsoc: pointer to psoc object
 * @peer_id: peer id of direct connected peer
 * @vdev_id: vdev id
 * @wds_mac_addr: mac address of wds peer
 *
 * This API notifies IPA driver with [peer_id, wds_mac_addr] upon wds peer
 * unmap event from target.
 *
 * Return: 0 for success, otherwise error codes
 */
int wlan_dp_send_ipa_wds_peer_unmap_event(struct cdp_ctrl_objmgr_psoc *cpsoc,
					  uint16_t peer_id, uint8_t vdev_id,
					  uint8_t *wds_mac_addr);

/**
 * wlan_dp_send_ipa_wds_peer_disconnect() - Notify IPA of disconnecting wds
 *					    peer
 * @cpsoc: pointer to psoc object
 * @wds_mac_addr: mac address of wds peer
 * @vdev_id: vdev id
 *
 * This API notifies IPA driver with wds_mac_addr when direct connected peer
 * disconnects.
 *
 * Return: none
 */
void wlan_dp_send_ipa_wds_peer_disconnect(struct cdp_ctrl_objmgr_psoc *cpsoc,
					  uint8_t *wds_mac_addr,
					  uint8_t vdev_id);
#endif /* IPA_WDS_EASYMESH_FEATURE */

#ifdef WLAN_DP_DYNAMIC_RESOURCE_MGMT
/**
 * wlan_dp_notify_vdev_mac_id_migration() - Notify DP about vdev migration
 *
 * @vdev: objmgr vdev reference
 * @old_mac_id: old mac id vdev present
 * @new_mac_id: new migrating mac id
 *
 * This API notifies DP about vdev migration across MAC's
 *
 * Return: none
 */
void wlan_dp_notify_vdev_mac_id_migration(struct wlan_objmgr_vdev *vdev,
					  uint32_t old_mac_id,
					  uint32_t new_mac_id);

#ifdef WLAN_FEATURE_NAN
/**
 * wlan_dp_notify_ndp_channel_info() - Notify DP about NDP peer channel info
 *
 * @peer: objmgr peer reference
 * @ch_info: NDP channel info
 * @num_channels: Number of channels info
 *
 * This API notifies DP about vdev migration across MAC's
 *
 * Return: none
 */
void
wlan_dp_notify_ndp_channel_info(struct wlan_objmgr_peer *peer,
				struct nan_datapath_channel_info *ch_info,
				uint32_t num_channels);
#endif
#else
static inline
void wlan_dp_notify_vdev_mac_id_migration(struct wlan_objmgr_vdev *vdev,
					  uint32_t old_mac_id,
					  uint32_t new_mac_id)
{
}

#ifdef WLAN_FEATURE_NAN
static inline void
wlan_dp_notify_ndp_channel_info(struct wlan_objmgr_peer *peer,
				struct nan_datapath_channel_info *ch_info,
				uint32_t num_channels)
{
}
#endif
#endif /* WLAN_DP_DYNAMIC_RESOURCE_MGMT */
#endif
