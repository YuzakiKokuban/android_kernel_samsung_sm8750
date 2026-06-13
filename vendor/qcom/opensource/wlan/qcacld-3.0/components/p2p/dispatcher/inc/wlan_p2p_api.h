/*
 * Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
 * DOC: Contains p2p public data structure definitions
 */

#ifndef _WLAN_P2P_API_H_
#define _WLAN_P2P_API_H_

#include <qdf_types.h>
#include <wlan_objmgr_vdev_obj.h>

/**
 * wlan_p2p_check_oui_and_force_1x1() - Function to get P2P client device
 * attributes from assoc request frames
 * @assoc_ie: pointer to assoc request frame IEs
 * @ie_len: length of the assoc request frame IE
 *
 * When assoc request is received from P2P STA device, this function checks
 * for specific OUI present in the P2P device info attribute. The caller should
 * take care of checking if this is called only in P2P GO mode.
 *
 * Return: True if OUI is present, else false.
 */
bool wlan_p2p_check_oui_and_force_1x1(uint8_t *assoc_ie, uint32_t ie_len);

/**
 * wlan_p2p_cleanup_roc_by_vdev() - Cleanup roc request by vdev
 * @vdev: pointer to vdev object
 * @sync: whether to wait for complete event
 *
 * This function call P2P API to cleanup roc request by vdev
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS wlan_p2p_cleanup_roc_by_vdev(struct wlan_objmgr_vdev *vdev,
					bool sync);

/**
 * wlan_p2p_status_connect() - Update P2P connection status
 * @vdev: vdev context
 *
 * Updates P2P connection status by up layer when connecting.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS wlan_p2p_status_connect(struct wlan_objmgr_vdev *vdev);

/**
 * wlan_p2p_is_action_frame_of_p2p_type() - Given action frame is p2p type or
 * not
 * @data_buf: data buffer address
 * @length: buffer length
 *
 * Return: bool
 */
bool wlan_p2p_is_action_frame_of_p2p_type(uint8_t *data_buf, uint32_t length);

/**
 * wlan_p2p_abort_scan() - Abort on going scan on p2p interfaces
 * @pdev: pdev object
 *
 * This function triggers an abort scan request to scan component to abort the
 * ongoing scan request on p2p vdevs.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS wlan_p2p_abort_scan(struct wlan_objmgr_pdev *pdev);

#ifdef WLAN_FEATURE_P2P_P2P_STA
/**
 * wlan_p2p_check_and_force_scc_go_plus_go() - Check and do force scc for
 * go plus go
 * @psoc: psoc object
 * @vdev: vdev object
 *
 * This function checks whether force scc is enabled or not. If it
 * is enabled then it will do force scc to remaining p2p go vdev if
 * user has initiated CSA to current vdev.
 *
 * Return: status
 */
QDF_STATUS
wlan_p2p_check_and_force_scc_go_plus_go(struct wlan_objmgr_psoc *psoc,
					struct wlan_objmgr_vdev *vdev);
#else
static inline QDF_STATUS
wlan_p2p_check_and_force_scc_go_plus_go(struct wlan_objmgr_psoc *psoc,
					struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}
#endif

/**
 * wlan_p2p_parse_assoc_ie_for_device_info() - wrapper API for
 * "p2p_parse_assoc_ie_for_device_info"
 *
 * @assoc_ie: Association request IE
 * @assoc_ie_len: Association IE length
 *
 * Return: pointer to P2P address
 */
const uint8_t *wlan_p2p_parse_assoc_ie_for_device_info(const uint8_t *assoc_ie,
						       uint32_t assoc_ie_len);

/**
 * wlan_p2p_extract_ap_assist_dfs_params() - Wrapper API to check for P2P2 IE
 * @vdev: VDEV object manager pointer
 * @ie: Pointer to IE buffer
 * @ie_len: Length go bytes pointed by @ie
 * @is_connected: If set to %true only connected APs are extracted
 * @freq: Frequency to filter out APs from the WLAN AP info attr
 * @is_self: If set to %true, the extracted AP info is filled in VDEV priv
 *
 * Check for presence of P2P2 IE and saves the attribute data into VDEV for
 * assisted AP info for DFS P2P group operation.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_p2p_extract_ap_assist_dfs_params(struct wlan_objmgr_vdev *vdev,
				      const uint8_t *ie, uint16_t ie_len,
				      bool is_connected, qdf_freq_t freq,
				      bool is_self);

/**
 * wlan_p2p_fw_support_ap_assist_dfs_group() - Wrapper API to get the FW
 * support for assisted AP P2P group
 * @psoc: PSOC object manager pointer
 *
 * Return: bool
 */
bool wlan_p2p_fw_support_ap_assist_dfs_group(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_p2p_check_ap_assist_dfs_group_go() - Wrapper API to check the status of
 * P2P GO to operate in ap assisted DFS group
 * @vdev: VDEV object manager of P2P GO
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_p2p_check_ap_assist_dfs_group_go(struct wlan_objmgr_vdev *vdev);

/**
 * wlan_p2p_validate_ap_assist_dfs_group() - Wrapper API to validate the
 * extracted params from the P2P2 IE
 * @vdev: VDEV object manager pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_p2p_validate_ap_assist_dfs_group(struct wlan_objmgr_vdev *vdev);

/**
 * wlan_p2p_get_ap_assist_dfs_params() - Wrapper API to get the assisted AP
 * params for P2P entity operation in DFS channel
 * @vdev: VDEV object manager
 * @is_dfs_owner: Pointer to get DFS owner cap bit
 * @is_valid_ap_assist: Is valid AP assist params
 * @ap_bssid: BSSID of the assisted AP
 * @opclass: Operating class of the assisted AP
 * @chan: Channel number of the assisted AP
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_p2p_get_ap_assist_dfs_params(struct wlan_objmgr_vdev *vdev,
					     bool *is_dfs_owner,
					     bool *is_valid_ap_assist,
					     struct qdf_mac_addr *ap_bssid,
					     uint8_t *opclass, uint8_t *chan);

/**
 * wlan_p2p_psoc_priv_set_sta_vdev_id() - Cache STA vdev id
 * @psoc: pointer to psoc
 * @vdev_id: vdev id to set
 *
 * Cache STA vdev_id in psoc p2p priv object.
 *
 * Return: None
 */
void wlan_p2p_psoc_priv_set_sta_vdev_id(struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id);

/**
 * wlan_p2p_psoc_priv_get_sta_vdev_id() - Get cached STA vdev id
 * @psoc: pointer to psoc
 *
 * Return: uint8_t
 */
uint8_t wlan_p2p_psoc_priv_get_sta_vdev_id(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_p2p_set_rand_mac_for_p2p_dev() - set P2P device mac addr to rx filters
 * @soc: pointer to psoc
 * @vdev_id: vdev id to fetch p2p_vdev_priv_obj
 * @freq: frequency on which the filtering(allow) is expected
 * @rnd_cookie: cookie value
 * @duration: duration of the filter validity. p2p_mac_clear_timeout() is called
 *            and filter would be removed upon timeout, if not removed already
 *
 * Return: None
 */
QDF_STATUS
wlan_p2p_set_rand_mac_for_p2p_dev(struct wlan_objmgr_psoc *soc,
				  uint32_t vdev_id, uint32_t freq,
				  uint64_t rnd_cookie, uint32_t duration);

/**
 * wlan_p2p_del_random_mac() - del mac filter from given vdev rand mac list
 * @soc: soc object
 * @vdev_id: vdev id
 * @rnd_cookie: random mac mgmt tx cookie
 *
 * This function will del the mac addr filter from vdev random mac addr list.
 * If there is no reference to mac addr, it will set a clear timer to flush it
 * in target finally.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_p2p_del_random_mac(struct wlan_objmgr_psoc *soc, uint32_t vdev_id,
			uint64_t rnd_cookie);
#endif
