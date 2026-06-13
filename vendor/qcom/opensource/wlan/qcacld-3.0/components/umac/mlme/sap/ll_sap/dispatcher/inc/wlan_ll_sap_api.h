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
 * DOC: contains ll_lt_sap API definitions specific to the bearer
 * switch, channel selection functionalities
 */

#ifndef _WLAN_LL_LT_SAP_API_H_
#define _WLAN_LL_LT_SAP_API_H_

#include <scheduler_api.h>
#include <wlan_cmn.h>
#include <wlan_objmgr_vdev_obj.h>
#include "wlan_objmgr_psoc_obj.h"
#include "wlan_ll_sap_public_structs.h"
#include "wlan_cm_public_struct.h"
#include "wlan_policy_mgr_public_struct.h"

#ifdef WLAN_FEATURE_LL_LT_SAP
/**
 * wlan_ll_lt_sap_bearer_switch_get_id() - Get the request id for bearer switch
 * request
 * @psoc: Pointer to psoc
 * Return: Bearer switch request id
 */
wlan_bs_req_id
wlan_ll_lt_sap_bearer_switch_get_id(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_ll_lt_sap_switch_bearer_to_ble() - Switch audio transport to BLE
 * @psoc: Pointer to psoc
 * @bs_request: Pointer to bearer switch request
 * Return: QDF_STATUS_SUCCESS on successful bearer switch else failure
 */
QDF_STATUS wlan_ll_lt_sap_switch_bearer_to_ble(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_bearer_switch_request *bs_request);
/* wlan_ll_lt_sap_switch_bearer_to_wlan() - Switch audio transport to WLAN
 * @psoc: Pointer to psoc
 * @bs_request: Pointer to bearer switch request
 * Return: QDF_STATUS_SUCCESS on successful bearer switch else failure
 */
QDF_STATUS
wlan_ll_lt_sap_switch_bearer_to_wlan(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_bearer_switch_request *bs_request);

/**
 * wlan_ll_sap_switch_bearer_on_sta_connect_start() - Switch bearer during
 * station connection start
 * @psoc: Pointer to psoc
 * @scan_list: Pointer to the candidate list
 * @vdev_id: Vdev id of the requesting vdev
 * @cm_id: connection manager id of the current connect request
 * Return: QDF_STATUS_SUCCESS on successful bearer switch
 *         QDF_STATUS_E_ALREADY, if bearer switch is not required
 *         else failure
 */
QDF_STATUS wlan_ll_sap_switch_bearer_on_sta_connect_start(
						struct wlan_objmgr_psoc *psoc,
						qdf_list_t *scan_list,
						uint8_t vdev_id,
						wlan_cm_id cm_id);
/**
 * wlan_ll_sap_switch_bearer_on_sta_connect_complete() - Switch bearer during
 * station connection complete
 * @psoc: Pointer to psoc
 * @vdev_id: Vdev id of the requesting vdev
 * Return: QDF_STATUS_SUCCESS on successful bearer switch
 *         QDF_STATUS_E_ALREADY, if bearer switch is not required
 *         else failure
 */
QDF_STATUS wlan_ll_sap_switch_bearer_on_sta_connect_complete(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id);

/**
 * wlan_ll_sap_switch_bearer_on_ll_sap_csa() - Switch bearer when LL_LT_SAP
 * csa starts
 * @psoc: Pointer to psoc
 * @vdev_id: vdev id
 * Return: QDF_STATUS_SUCCESS on successful bearer switch
 *	   else failure
 */
QDF_STATUS wlan_ll_sap_switch_bearer_on_ll_sap_csa(
					struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id);

/**
 * wlan_ll_sap_switch_bearer_on_ll_sap_csa_complete() - Switch bearer
 * when LL_LT_SAP csa completes
 * @psoc: Pointer to psoc
 * @vdev_id: vdev id
 * Return: QDF_STATUS_SUCCESS on successful bearer switch
 *         else failure
 */
QDF_STATUS wlan_ll_sap_switch_bearer_on_ll_sap_csa_complete(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id);

/**
 * wlan_ll_sap_switch_bearer_on_stop_ap() - Switch bearer when stop_ap is
 * issued on LL_LT_SAP
 * @psoc: Pointer to psoc
 * @vdev_id: vdev id
 * Return: QDF_STATUS_SUCCESS on successful bearer switch
 *	   else failure
 */
QDF_STATUS wlan_ll_sap_switch_bearer_on_stop_ap(
					struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id);

/**
 * wlan_ll_lt_sap_get_freq_list() - Get frequency list for LL_LT_SAP
 * @psoc: Pointer to psoc object
 * @freq_list: Pointer to wlan_ll_lt_sap_freq_list structure
 * @vdev_id: Vdev Id
 * @csa_src: csa source
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_ll_lt_sap_get_freq_list(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_ll_lt_sap_freq_list *freq_list,
				uint8_t vdev_id,
				enum ll_sap_csa_source csa_src);

/**
 * wlan_ll_lt_sap_override_freq() - Return frequency on which LL_LT_SAP can
 * be started
 * @psoc: Pointer to psoc object
 * @vdev_id: Vdev Id of LL_LT_SAP
 * @chan_freq: current frequency of ll_lt_sap
 *
 * This function checks if ll_lt_sap can come up on the given frequency, if it
 * can come up on given frequency then return same frequency else return a
 * different frequency on which ll_lt_sap can come up
 *
 * Return: valid ll_lt_sap frequency
 */
qdf_freq_t wlan_ll_lt_sap_override_freq(struct wlan_objmgr_psoc *psoc,
					uint32_t vdev_id,
					qdf_freq_t chan_freq);

/**
 * wlan_ll_lt_sap_extract_ll_sap_cap() - Extract LL LT SAP capability
 * @psoc: Pointer to psoc object
 *
 */
void wlan_ll_lt_sap_extract_ll_sap_cap(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_get_ll_lt_sap_restart_freq() - Get restart frequency on which LL_LT_SAP
 * can be re-started
 * @pdev: Pointer to pdev object
 * @chan_freq: current frequency of ll_lt_sap
 * @vdev_id: Vdev Id of LL_LT_SAP
 * @csa_reason: Reason for the CSA
 *
 * This function checks if ll_lt_sap needs to be restarted, if yes, it returns
 * new valid frequency on which ll_lt_sap can be restarted else return same
 * frequency.
 *
 * Return: valid ll_lt_sap frequency
 */
qdf_freq_t
wlan_get_ll_lt_sap_restart_freq(struct wlan_objmgr_pdev *pdev,
				qdf_freq_t chan_freq,
				uint8_t vdev_id,
				enum sap_csa_reason_code *csa_reason);

/**
 * wlan_ll_sap_fw_bearer_switch_req() - FW bearer switch request
 * @psoc: Pointer to psoc object
 * @req_type: Bearer switch request type
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_ll_sap_fw_bearer_switch_req(struct wlan_objmgr_psoc *psoc,
				 enum bearer_switch_req_type req_type);

/**
 * wlan_ll_sap_oob_connect_response() - FW response to OOB connect request
 * @psoc: Pointer to psoc object
 * @rsp: FW response to the OOB connect request
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_ll_sap_oob_connect_response(
			struct wlan_objmgr_psoc *psoc,
			struct ll_sap_oob_connect_response_event rsp);

/**
 * wlan_ll_lt_sap_get_mcs() - Get LL_LT_SAP MCS set
 * @psoc: Pointer to psoc object
 * @vdev_id: Vdev Id of LL_LT_SAP
 * @mcs_set: pointer in which mcs_set needs to be filled
 *
 * Return: None
 */
void wlan_ll_lt_sap_get_mcs(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
			    uint8_t *mcs_set);

/**
 * wlan_ll_lt_sap_continue_csa_after_tsf_rsp() - ll_sap process command function
 * @msg: scheduler msg
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_ll_lt_sap_continue_csa_after_tsf_rsp(struct scheduler_msg *msg);

/**
 * wlan_ll_sap_get_tsf_stats_before_csa() - Get tsf stats from fw for LL_LT_SAP
 * @psoc: psoc pointer
 * @vdev: vdev pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_ll_sap_get_tsf_stats_before_csa(struct wlan_objmgr_psoc *psoc,
						struct wlan_objmgr_vdev *vdev);

/**
 * wlan_ll_sap_reset_target_tsf_before_csa() - Reset target_tsf as 0 before CSA
 * @psoc: psoc pointer
 * @vdev: vdev pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_ll_sap_reset_target_tsf_before_csa(
					struct wlan_objmgr_psoc *psoc,
					struct wlan_objmgr_vdev *vdev);

/**
 * wlan_ll_sap_get_target_tsf() - Get target_tsf
 * @vdev: vdev pointer
 * @get_tsf: get target_tsf in different scenario
 *
 * Return: uint64_t
 */
uint64_t wlan_ll_sap_get_target_tsf(struct wlan_objmgr_vdev *vdev,
				    enum ll_sap_get_target_tsf get_tsf);

/**
 * wlan_ll_sap_get_target_tsf_for_vdev_restart() - Get target_tsf for vdev
 * restart
 * @vdev: vdev pointer
 *
 * Return: uint64_t
 */
uint64_t
wlan_ll_sap_get_target_tsf_for_vdev_restart(struct wlan_objmgr_vdev *vdev);

/**
 * wlan_ll_sap_is_bearer_switch_req_on_csa() - Check whether bearer switch
 * is required for LL_LT_SAP CSA or not
 * @psoc: psoc pointer
 * @vdev_id: vdev id
 */
bool wlan_ll_sap_is_bearer_switch_req_on_csa(struct wlan_objmgr_psoc *psoc,
					     uint8_t vdev_id);

/**
 * wlan_ll_lt_sap_is_freq_in_avoid_list() - Check whether given freq is present
 * in avoided list or not
 * @psoc: pointer to psoc
 * @freq: given freq
 *
 * Return: True/False
 */
bool wlan_ll_lt_sap_is_freq_in_avoid_list(struct wlan_objmgr_psoc *psoc,
					  qdf_freq_t freq);

/**
 * wlan_ll_lt_store_to_avoid_list_and_flush_old() - Store the current frequency
 * in void list and flush old/expired frequencies from avoid list
 * @psoc: pointer to psoc object
 * @freq: given frequency
 * @csa_src: Source for CSA
 *
 * Return: True/False
 */
void wlan_ll_lt_store_to_avoid_list_and_flush_old(
					struct wlan_objmgr_psoc *psoc,
					qdf_freq_t freq,
					enum ll_sap_csa_source csa_src);

/**
 * wlan_ll_sap_get_valid_freq_for_csa() - API to get valid frequency for
 * LL_LT_SAP
 * @psoc: Pointer to psoc object
 * @vdev_id: Vdev Id of ll_lt_sap
 * @curr_freq: current frequency
 * @csa_src: LL_SAP csa source
 *
 * Return: QDF_STATUS
 */

qdf_freq_t
wlan_ll_sap_get_valid_freq_for_csa(struct wlan_objmgr_psoc *psoc,
				   uint8_t vdev_id, qdf_freq_t curr_freq,
				   enum ll_sap_csa_source csa_src);
#else
static inline wlan_bs_req_id
wlan_ll_lt_sap_bearer_switch_get_id(struct wlan_objmgr_vdev *vdev)
{
	return 0;
}

static inline QDF_STATUS
wlan_ll_lt_sap_switch_bearer_to_ble(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_bearer_switch_request *bs_request)
{
	return QDF_STATUS_E_FAILURE;
}

static inline void
wlan_ll_lt_sap_extract_ll_sap_cap(struct wlan_objmgr_psoc *psoc)
{
}

static inline QDF_STATUS
wlan_ll_lt_sap_switch_bearer_to_wlan(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_bearer_switch_request *bs_request)
{
	return QDF_STATUS_E_FAILURE;
}

static inline QDF_STATUS
wlan_ll_sap_switch_bearer_on_sta_connect_start(struct wlan_objmgr_psoc *psoc,
					       qdf_list_t *scan_list,
					       uint8_t vdev_id,
					       wlan_cm_id cm_id)

{
	return QDF_STATUS_E_ALREADY;
}

static inline QDF_STATUS
wlan_ll_sap_switch_bearer_on_sta_connect_complete(struct wlan_objmgr_psoc *psoc,
						  uint8_t vdev_id)
{
	return QDF_STATUS_SUCCESS;
}

static inline
QDF_STATUS wlan_ll_sap_switch_bearer_on_ll_sap_csa(
					struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
QDF_STATUS wlan_ll_sap_switch_bearer_on_ll_sap_csa_complete(
						struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
QDF_STATUS wlan_ll_sap_switch_bearer_on_stop_ap(
					struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
QDF_STATUS wlan_ll_lt_sap_get_freq_list(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_ll_lt_sap_freq_list *freq_list,
				uint8_t vdev_id,
				enum ll_sap_csa_source csa_src)
{
	return QDF_STATUS_E_FAILURE;
}

static inline
qdf_freq_t wlan_ll_lt_sap_override_freq(struct wlan_objmgr_psoc *psoc,
					uint32_t vdev_id,
					qdf_freq_t chan_freq)
{
	return chan_freq;
}

static inline
qdf_freq_t wlan_get_ll_lt_sap_restart_freq(struct wlan_objmgr_pdev *pdev,
					   qdf_freq_t chan_freq,
					   uint8_t vdev_id,
					   enum sap_csa_reason_code *csa_reason)
{
	return 0;
}

static inline
void wlan_ll_lt_sap_get_mcs(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
			    uint8_t *mcs_set)
{
}

static inline
QDF_STATUS wlan_ll_lt_sap_continue_csa_after_tsf_rsp(struct scheduler_msg *msg)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
QDF_STATUS wlan_ll_sap_get_tsf_stats_before_csa(struct wlan_objmgr_psoc *psoc,
						struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
QDF_STATUS wlan_ll_sap_reset_target_tsf_before_csa(
					struct wlan_objmgr_psoc *psoc,
					struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
uint64_t wlan_ll_sap_get_target_tsf(struct wlan_objmgr_vdev *vdev,
				    enum ll_sap_get_target_tsf get_tsf)
{
	return 0;
}

static inline uint64_t
wlan_ll_sap_get_target_tsf_for_vdev_restart(struct wlan_objmgr_vdev *vdev)
{
	return 0;
}

static inline
bool wlan_ll_sap_is_bearer_switch_req_on_csa(struct wlan_objmgr_psoc *psoc,
					     uint8_t vdev_id)
{
	return false;
}

static inline
bool wlan_ll_lt_sap_is_freq_in_avoid_list(struct wlan_objmgr_psoc *psoc,
					  qdf_freq_t freq)
{
	return false;
}

static inline void
wlan_ll_lt_store_to_avoid_list_and_flush_old(struct wlan_objmgr_psoc *psoc,
					     qdf_freq_t freq,
					     enum ll_sap_csa_source csa_src);

static inline qdf_freq_t
wlan_ll_sap_get_valid_freq_for_csa(struct wlan_objmgr_psoc *psoc,
				   uint8_t vdev_id, qdf_freq_t curr_freq,
				   enum ll_sap_csa_source csa_src)
{
	return 0;
}

#endif /* WLAN_FEATURE_LL_LT_SAP */
#endif /* _WLAN_LL_LT_SAP_API_H_ */
