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
 * DOC: contains ll_lt_sap declarations specific to the bearer
 * switch functionalities
 */

#ifndef TARGET_IF_LL_SAP_H
#define TARGET_IF_LL_SAP_H

#include "wlan_ll_sap_public_structs.h"

/**
 * target_if_ll_sap_register_tx_ops() - register ll_lt_sap tx ops
 * @tx_ops: pointer to ll_sap tx ops
 *
 * Return: None
 */
void target_if_ll_sap_register_tx_ops(struct wlan_ll_sap_tx_ops *tx_ops);

/**
 * target_if_ll_sap_register_rx_ops() - register ll_lt_sap rx ops
 * @rx_ops: pointer to ll_sap rx ops
 *
 * Return: None
 */
void target_if_ll_sap_register_rx_ops(struct wlan_ll_sap_rx_ops *rx_ops);

/**
 * target_if_ll_sap_register_events() - register ll_lt_sap fw event handlers
 * @psoc: pointer to psoc
 *
 * Return: QDF_STATUS
 */
QDF_STATUS target_if_ll_sap_register_events(struct wlan_objmgr_psoc *psoc);

/**
 * target_if_ll_sap_deregister_events() - deregister ll_lt_sap fw event handlers
 * @psoc: pointer to psoc
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
target_if_ll_sap_deregister_events(struct wlan_objmgr_psoc *psoc);

/**
 * target_if_ll_sap_continue_csa_after_tsf_rsp() - Continue CSA after getting
 * rsp of tsf stats from fw
 * @psoc: psoc object
 * @twt_params: pointer to twt_session_stats_info
 *
 * Return: QDF_STATUS
 */
QDF_STATUS target_if_ll_sap_continue_csa_after_tsf_rsp(
				struct wlan_objmgr_psoc *psoc,
				struct twt_session_stats_info *twt_params);

/**
 * target_if_ll_sap_is_twt_event_type_query_rsp() - Check if twt event type is
 * query response or not
 * @psoc: psoc object
 * @evt_buf: event buffer:
 * @params: pointer to twt_session_stats_event_param
 * @twt_params: pointer to twt_session_stats_info
 */
bool target_if_ll_sap_is_twt_event_type_query_rsp(
				struct wlan_objmgr_psoc *psoc,
				uint8_t *evt_buf,
				struct twt_session_stats_event_param *params,
				struct twt_session_stats_info *twt_params);
#endif /* TARGET_IF_LL_SAP_H */
