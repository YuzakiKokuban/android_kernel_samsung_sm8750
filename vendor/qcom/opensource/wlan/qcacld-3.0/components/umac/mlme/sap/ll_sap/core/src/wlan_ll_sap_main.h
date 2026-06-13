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
 * DOC: contains ll_sap_definitions specific to the ll_sap module
 */

#ifndef _WLAN_LL_SAP_MAIN_H_
#define _WLAN_LL_SAP_MAIN_H_

#include "wlan_objmgr_psoc_obj.h"
#include "wlan_objmgr_vdev_obj.h"
#include "wlan_ll_sap_public_structs.h"

#define ll_sap_err(params...) QDF_TRACE_ERROR(QDF_MODULE_ID_LL_SAP, params)
#define ll_sap_info(params...) QDF_TRACE_INFO(QDF_MODULE_ID_LL_SAP, params)
#define ll_sap_debug(params...) QDF_TRACE_DEBUG(QDF_MODULE_ID_LL_SAP, params)

#define ll_sap_nofl_err(params...) \
	QDF_TRACE_ERROR_NO_FL(QDF_MODULE_ID_LL_SAP, params)
#define ll_sap_nofl_info(params...) \
	QDF_TRACE_INFO_NO_FL(QDF_MODULE_ID_LL_SAP, params)
#define ll_sap_nofl_debug(params...) \
	QDF_TRACE_DEBUG_NO_FL(QDF_MODULE_ID_LL_SAP, params)

#define MAX_HIGH_AP_AVAILABILITY_REQUESTS 2

/* Maximum number of peers for SAP */
#define SAP_MAX_NUM_PEERS 32

/* Maximum channel to store */
#define MAX_CHAN_TO_STORE 3

/**
 * struct store_freq_list - Stored freq list which needs to be avoided before
 * selecting new freq for CSA
 * @freq: Avoided frequency
 * @timestamp: time on which freq stored
 */
struct store_freq_list {
	qdf_freq_t freq;
	qdf_time_t timestamp;
};

/**
 * struct wlan_ll_lt_sap_avoid_freq - Avoid freq to be picked by CSA.
 * @freq_list: Freq list which needs to be avoided before
 * selecting new freq for CSA
 * @stored_num_ch: Total number of channel stored
 */
struct wlan_ll_lt_sap_avoid_freq {
	struct store_freq_list freq_list[MAX_CHAN_TO_STORE];
	uint8_t stored_num_ch;
};

/**
 * struct ll_sap_psoc_priv_obj - ll_sap private psoc obj
 * @tx_ops: Tx ops registered with Target IF interface
 * @rx_ops: Rx  ops registered with Target IF interface
 * @tsf_timer: TSF timer
 * @timer_vdev_id: vdev id for which tsf timer needs to
 * started
 * @is_ll_lt_sap_supported: Flag to check whether LL SAP is supported
 * @is_beared_switch_required: Flag to check whether bearer switch
 * is required or not
 * @avoid_freq: Avoid freq to be picked by CSA
 */
struct ll_sap_psoc_priv_obj {
	struct wlan_ll_sap_tx_ops tx_ops;
	struct wlan_ll_sap_rx_ops rx_ops;
	qdf_mc_timer_t tsf_timer;
	uint8_t timer_vdev_id;
	bool is_ll_lt_sap_supported;
	bool is_beared_switch_required;
	struct wlan_ll_lt_sap_avoid_freq avoid_freq;
};

/**
 * struct target_tsf: Target TSF param
 * @twt_target_tsf: Get target_tsf for twt session present
 * @non_twt_target_tsf: Get target_tsf for non twt session present
 */
struct target_tsf {
	uint64_t twt_target_tsf;
	uint64_t non_twt_target_tsf;
};

 /**
  * struct ll_sap_vdev_peer_entry - ll_sap vdev peer entries
  * @macpeer: peer mac address
  * @num_peer: number of peer
  */
struct ll_sap_vdev_peer_entry {
	struct qdf_mac_addr macaddr[SAP_MAX_NUM_PEERS];
	uint8_t num_peer;
};

/**
 * struct ll_sap_vdev_priv_obj - ll sap private vdev obj
 * @bearer_switch_ctx: Bearer switch context
 * @high_ap_availability_cookie: High AP availability cookie
 * @target_tsf: pointer to target_tsf structure
 */
struct ll_sap_vdev_priv_obj {
	struct bearer_switch_info *bearer_switch_ctx;
	uint16_t high_ap_availability_cookie[MAX_HIGH_AP_AVAILABILITY_REQUESTS];
	struct target_tsf target_tsf;
};

/**
 * ll_sap_get_vdev_priv_obj: get ll_sap priv object from vdev object
 * @vdev: pointer to vdev object
 *
 * Return: pointer to ll_sap vdev private object
 */
static inline
struct ll_sap_vdev_priv_obj *ll_sap_get_vdev_priv_obj(
						struct wlan_objmgr_vdev *vdev)
{
	struct ll_sap_vdev_priv_obj *obj;

	if (!vdev) {
		ll_sap_err("vdev is null");
		return NULL;
	}
	obj = wlan_objmgr_vdev_get_comp_private_obj(vdev,
						    WLAN_UMAC_COMP_LL_SAP);

	return obj;
}

/**
 * ll_sap_init() - initializes ll_sap component
 *
 * Return: QDF status
 */
QDF_STATUS ll_sap_init(void);

/**
 * ll_sap_deinit() - De-initializes ll_sap component
 *
 * Return: QDF status
 */
QDF_STATUS ll_sap_deinit(void);

/**
 * ll_sap_register_os_if_cb() - Register ll_sap osif callbacks
 * @ll_sap_global_ops: Ops which needs to be registered
 *
 * Return: None
 */
void ll_sap_register_os_if_cb(struct ll_sap_ops *ll_sap_global_ops);

/**
 * ll_sap_unregister_os_if_cb() - Un-register ll_sap osif callbacks
 *
 * Return: None
 */
void ll_sap_unregister_os_if_cb(void);

/**
 * ll_sap_get_osif_cbk() - API to get ll_sap osif callbacks
 *
 * Return: global ll_sap osif callback
 */
struct ll_sap_ops *ll_sap_get_osif_cbk(void);

/**
 * ll_sap_psoc_enable() - Enable ll_lt_sap psoc
 * @psoc: objmgr psoc pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ll_sap_psoc_enable(struct wlan_objmgr_psoc *psoc);

/**
 * ll_sap_psoc_disable() - Disable ll_lt_sap psoc
 * @psoc: objmgr psoc pointer
 *
 * Return: None
 */
QDF_STATUS ll_sap_psoc_disable(struct wlan_objmgr_psoc *psoc);

#endif /* _WLAN_LL_SAP_MAIN_H_ */
