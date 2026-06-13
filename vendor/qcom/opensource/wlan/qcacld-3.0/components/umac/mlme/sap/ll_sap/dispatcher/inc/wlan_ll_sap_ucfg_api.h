/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: This file contains ll_sap north bound interface declarations
 */

#ifndef _WLAN_LL_SAP_UCFG_API_H_
#define _WLAN_LL_SAP_UCFG_API_H_
#include "wlan_ll_sap_public_structs.h"

#ifdef WLAN_FEATURE_LL_LT_SAP

/**
 * ucfg_ll_sap_init() - initializes ll_sap component
 *
 * Return: QDF status
 */
QDF_STATUS ucfg_ll_sap_init(void);

/**
 * ucfg_ll_sap_deinit() - De-initializes ll_sap component
 *
 * Return: QDF status
 */
QDF_STATUS ucfg_ll_sap_deinit(void);

/**
 * ucfg_is_ll_lt_sap_supported() - Check if ll_lt_sap is supported or not
 *@psoc: Psoc pointer
 *
 * Return: True/False
 */
bool ucfg_is_ll_lt_sap_supported(struct wlan_objmgr_psoc *psoc);

/**
 * ucfg_ll_lt_sap_high_ap_availability() - Request for high ap availability
 * @vdev: LL_LT_SAP vdev
 * @operation: Type of the operation which needs to be performed
 * @duration: Duration for high AP availability
 * @cookie: Cookie of the request
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
ucfg_ll_lt_sap_high_ap_availability(
			struct wlan_objmgr_vdev *vdev,
			enum high_ap_availability_operation operation,
			uint32_t duration, uint16_t cookie);

/**
 * ucfg_ll_lt_sap_request_for_audio_transport_switch() - Request to switch the
 * audio transport medium
 * @vdev: Vdev on which the request is received
 * @req_type: Requested transport switch type
 *
 * Return: Accepted/Rejected
 */
QDF_STATUS ucfg_ll_lt_sap_request_for_audio_transport_switch(
					struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_req_type req_type);

/**
 * ucfg_ll_lt_sap_deliver_audio_transport_switch_resp() - Deliver audio
 * transport switch response
 * @vdev: Vdev on which the request is received
 * @req_type: Transport switch type for which the response is received
 * @status: Status of the response
 *
 * Return: None
 */
void ucfg_ll_lt_sap_deliver_audio_transport_switch_resp(
					struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_req_type req_type,
					enum bearer_switch_status status);

/**
 * ucfg_ll_sap_register_cb() - Register ll_sap osif callbacks
 * @ll_sap_global_ops: Ops which needs to be registered
 *
 * Return: None
 */
void ucfg_ll_sap_register_cb(struct ll_sap_ops *ll_sap_global_ops);

/**
 * ucfg_ll_sap_unregister_cb() - Un-register ll_sap osif callbacks
 *
 * Return: None
 */
void ucfg_ll_sap_unregister_cb(void);

/**
 * ucfg_ll_sap_psoc_enable() - Enable ll_lt_sap psoc
 * @psoc: objmgr psoc pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ucfg_ll_sap_psoc_enable(struct wlan_objmgr_psoc *psoc);

/**
 * ucfg_ll_sap_psoc_disable() - Disable ll_lt_sap psoc
 * @psoc: objmgr psoc pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ucfg_ll_sap_psoc_disable(struct wlan_objmgr_psoc *psoc);

/**
 * ucfg_ll_lt_sap_switch_bearer_for_p2p_go_start() - Switch the bearer for
 * P2P GO start
 * @psoc: Pointer to psoc
 * @vdev_id: vdev id of the current vdev
 * @oper_freq: Frequency on which P2P_GO wants to come up
 * @device_mode: Device mode of the current vdev
 *
 * This API checks if LL_LT_SAP is present and P2P_GO is coming up in MCC with
 * LL_LT_SAP, then it switches the bearer as P2P GO vdev start takes some time
 * and till then LL_LT_SAP will not be able to stay on that channel continuously
 *
 * Return: None
 */
void ucfg_ll_lt_sap_switch_bearer_for_p2p_go_start(struct wlan_objmgr_psoc *psoc,
						   uint8_t vdev_id,
						   qdf_freq_t oper_freq,
						   enum QDF_OPMODE device_mode);

/**
 * ucfg_ll_lt_sap_switch_bearer_on_p2p_go_complete() - Switch the bearer back to
 * wlan P2P GO start complete
 * @psoc: Pointer to psoc
 * @vdev_id: vdev id of the current vdev
 * @device_mode: Device mode of the current vdev
 *
 * This API checks if LL_LT_SAP is present and P2P_GO is coming up in MCC with
 * LL_LT_SAP, then it switches the bearer as P2P GO vdev start takes some time
 * and till then LL_LT_SAP will not be able to stay on that channel continuously
 *
 * Return: None
 */
void ucfg_ll_lt_sap_switch_bearer_on_p2p_go_complete(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						enum QDF_OPMODE device_mode);

/**
 * ucfg_ll_lt_sap_get_target_tsf() - API to get target_tsf
 * @vdev: vdev object
 * @target_tsf: target_tsf param
 */
void ucfg_ll_lt_sap_get_target_tsf(struct wlan_objmgr_vdev *vdev,
				   uint64_t *target_tsf);

/**
 * ucfg_ll_sap_get_valid_freq_for_csa() - Wrapper api to get valid freq for
 * LL_LT_SAP
 * @psoc: Pointer to psoc
 * @vdev_id: Vdev ID
 * @curr_freq: current frequency
 * @csa_src: csa source
 *
 * Return: Frequency
 */
qdf_freq_t
ucfg_ll_sap_get_valid_freq_for_csa(struct wlan_objmgr_psoc *psoc,
				   uint8_t vdev_id, qdf_freq_t curr_freq,
				   enum ll_sap_csa_source csa_src);
#else
static inline QDF_STATUS ucfg_ll_sap_init(void)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS ucfg_ll_sap_deinit(void)
{
	return QDF_STATUS_SUCCESS;
}

static inline bool ucfg_is_ll_lt_sap_supported(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

static inline QDF_STATUS
ucfg_ll_lt_sap_high_ap_availability(
				struct wlan_objmgr_vdev *vdev,
				enum high_ap_availability_operation operation,
				uint32_t duration, uint16_t cookie)
{
	return QDF_STATUS_E_INVAL;
}

static inline QDF_STATUS
ucfg_ll_lt_sap_request_for_audio_transport_switch(
					struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_req_type req_type)
{
	return QDF_STATUS_E_INVAL;
}

static inline void
ucfg_ll_lt_sap_deliver_audio_transport_switch_resp(
					struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_req_type req_type,
					enum bearer_switch_status status)
{
}

static inline void ucfg_ll_sap_register_cb(struct ll_sap_ops *ll_sap_global_ops)
{
}

static inline void ucfg_ll_sap_unregister_cb(void)
{
}

static inline QDF_STATUS ucfg_ll_sap_psoc_enable(struct wlan_objmgr_psoc *psoc)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS ucfg_ll_sap_psoc_disable(struct wlan_objmgr_psoc *psoc)
{
	return QDF_STATUS_SUCCESS;
}

static inline void ucfg_ll_lt_sap_switch_bearer_for_p2p_go_start(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						qdf_freq_t oper_freq,
						enum QDF_OPMODE device_mode)
{
}

static inline void ucfg_ll_lt_sap_switch_bearer_on_p2p_go_complete(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						enum QDF_OPMODE device_mode)
{
}

static inline qdf_freq_t
ucfg_ll_sap_get_valid_freq_for_csa(struct wlan_objmgr_psoc *psoc,
				   uint8_t vdev_id, qdf_freq_t curr_freq,
				   enum ll_sap_csa_source csa_src)
{
	return 0;
}
#endif /* WLAN_FEATURE_LL_LT_SAP */
#endif /* _WLAN_LL_SAP_UCFG_API_H_ */

