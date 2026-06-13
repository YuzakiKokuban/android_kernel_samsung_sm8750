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

#ifndef _WLAN_LL_LT_SAP_MAIN_H_
#define _WLAN_LL_LT_SAP_MAIN_H_

#include "wlan_ll_sap_main.h"
#include "wlan_mlme_public_struct.h"
#include <i_qdf_types.h>
#include <qdf_types.h>
#include "wlan_ll_sap_main.h"
#include "wlan_ll_sap_public_structs.h"

/**
 * ll_lt_sap_is_supported() - Check if ll_lt_sap is supported or not
 * @psoc: Pointer to psoc object
 * Return: True/False
 */
bool ll_lt_sap_is_supported(struct wlan_objmgr_psoc *psoc);

/**
 * ll_lt_sap_get_freq_list() - API to get frequency list for LL_LT_SAP
 * @psoc: Pointer to psoc object
 * @freq_list: Pointer to wlan_ll_lt_sap_freq_list structure
 * @vdev_id: Vdev Id
 * @src: ll_sap csa source
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ll_lt_sap_get_freq_list(struct wlan_objmgr_psoc *psoc,
				   struct wlan_ll_lt_sap_freq_list *freq_list,
				   uint8_t vdev_id, enum ll_sap_csa_source src);

/**
 * ll_lt_sap_get_valid_freq() - API to get valid frequency for LL_LT_SAP
 * @psoc: Pointer to psoc object
 * @vdev_id: Vdev Id of ll_lt_sap
 * @curr_freq: current frequency
 * @csa_src: LL_SAP csa source
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ll_lt_sap_get_valid_freq(struct wlan_objmgr_psoc *psoc,
				    uint8_t vdev_id, qdf_freq_t curr_freq,
				    enum ll_sap_csa_source csa_src);

/*
 * ll_lt_sap_init() - Initialize ll_lt_sap infrastructure
 * @vdev: Pointer to vdev
 *
 * Return: QDF_STATUS_SUCCESS if ll_lt_sap infra initialized successfully else
 * error code
 */
QDF_STATUS ll_lt_sap_init(struct wlan_objmgr_vdev *vdev);

/**
 * ll_lt_sap_deinit() - De-initialize ll_lt_sap infrastructure
 * @vdev: Pointer to vdev
 *
 * Return: QDF_STATUS_SUCCESS if ll_lt_sap infra de-initialized successfully
 * else error code
 */
QDF_STATUS ll_lt_sap_deinit(struct wlan_objmgr_vdev *vdev);

/**
 * ll_lt_sap_high_ap_availability() - Request for high ap availability
 * @vdev: LL_LT_SAP vdev
 * @operation: Type of the operation which needs to be performed
 * @duration: Duration for high AP availability
 * @cookie: Cookie of the request
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
ll_lt_sap_high_ap_availability(struct wlan_objmgr_vdev *vdev,
			       enum high_ap_availability_operation operation,
			       uint32_t duration, uint16_t cookie);

/**
 * ll_lt_sap_get_tsf_stats_for_csa() - Get tsf stats for csa
 * @psoc: pointer to psoc object
 * @vdev_id: vdev_id
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ll_lt_sap_get_tsf_stats_for_csa(
				struct wlan_objmgr_psoc *psoc,
				uint8_t vdev_id);

/**
 * ll_lt_sap_continue_csa_after_tsf_rsp() - Continue CSA after getting rsp
 * from firmware
 * @rsp: pointer to ll_sap_csa_tsf_rsp
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ll_lt_sap_continue_csa_after_tsf_rsp(struct ll_sap_csa_tsf_rsp *rsp);

/**
 * ll_lt_sap_get_bearer_switch_cap_for_csa() - Get bearer switch service
 * capability for LL_LT_SAP CSA
 * @psoc: pointer to psoc object
 *
 * Return: True/False
 */
bool ll_lt_sap_get_bearer_switch_cap_for_csa(struct wlan_objmgr_psoc *psoc);

/**
 * ll_lt_sap_is_freq_in_avoid_list() - Check whether given frequency is in
 * avoided list or not
 * @ll_sap_psoc_obj: pointer to ll_sap_psoc object
 * @freq: given frequency
 *
 * Return: True/False
 */
bool ll_lt_sap_is_freq_in_avoid_list(
			struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj,
			qdf_freq_t freq);

/**
 * ll_lt_store_to_avoid_list_and_flush_old() - Store the current frequency in
 * avoid list and flush old/expired frequencies from avoid list
 * @psoc: pointer to psoc object
 * @freq: given frequency
 * @csa_src: Source for CSA
 *
 * Return: True/False
 */
void ll_lt_store_to_avoid_list_and_flush_old(struct wlan_objmgr_psoc *psoc,
					     qdf_freq_t freq,
					     enum ll_sap_csa_source csa_src);

#endif /* _WLAN_LL_SAP_MAIN_H_ */
