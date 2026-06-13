/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _WLAN_DP_FLOW_BALANCE_H_
#define _WLAN_DP_FLOW_BALANCE_H_

#include "wlan_dp_load_balance.h"

/* packets per second */
#define FLOW_BALANCE_THRESH 180000

/**
 * struct wlan_dp_mig_flow - migrate flow details
 * @flow_id: flow id
 * @napi_id: napi id to which flow is going to migrate
 */
struct wlan_dp_mig_flow {
	uint16_t flow_id;
	uint8_t napi_id;
};

/**
 * struct wlan_dp_fwt_elem - flow weightage table element information
 * @flow_id: flow id
 * @weightage: weightage of the flow in percentage
 * @napi_id: napi id to which flow is mapped currently
 * @next: pointer to the next flow weightage table element
 */
struct wlan_dp_fwt_elem {
	uint16_t flow_id;
	uint8_t weightage;
	uint8_t napi_id;
	struct wlan_dp_fwt_elem *next;
};

/**
 * struct wlan_dp_flow - structure holding flow information
 * @avg_pkt_cnt: per second average packet count received on this flow
 * @flow_id: flow id
 * @flow_wtg: weightage of the flow
 */
struct wlan_dp_flow {
	uint32_t avg_pkt_cnt;
	uint16_t flow_id;
	uint8_t flow_wtg;
};

/**
 * struct wlan_dp_rx_ring_fm_tbl - structure holding the per ring per flow info
 * @flow_list: flow information of all the flows
 * @num_flows: number of active flows mapped to this ring
 * @total_avg_pkts: total average packets per second received on this ring
 * @ring_weightage: weightage in percentage of the ring
 * @ring_id: ring id
 */
struct wlan_dp_rx_ring_fm_tbl {
	struct wlan_dp_flow *flow_list;
	uint32_t num_flows;
	uint32_t total_avg_pkts;
	uint8_t ring_weightage;
	uint8_t ring_id;
};

/**
 * enum wlan_dp_fb_status - flow balance status
 * @FLOW_BALANCE_SUCCESS: flow balance is success
 * @FLOW_BALANCE_NOT_ELIGIBLE: flow balance is not eligible
 */
enum wlan_dp_fb_status {
	FLOW_BALANCE_SUCCESS,
	FLOW_BALANCE_NOT_ELIGIBLE,
};

#ifdef WLAN_FEATURE_LATENCY_SENSITIVE_REO
static inline
bool wlan_dp_rx_is_latency_sensitive_reo_enabled(void)
{
	return true;
}
#else
static inline
bool wlan_dp_rx_is_latency_sensitive_reo_enabled(void)
{
	return false;
}
#endif

#ifdef WLAN_DP_FLOW_BALANCE_SUPPORT
enum wlan_dp_fb_status
wlan_dp_fb_handler(struct wlan_dp_psoc_context *dp_ctx,
		   uint32_t *per_ring_pkt_avg,
		   uint32_t total_avg_pkts,
		   struct cpu_load *cpu_load,
		   uint32_t num_cpus,
		   struct wlan_dp_rx_ring_wtg *balanced_wtgs);

static inline bool wlan_dp_fb_enabled(struct wlan_dp_psoc_context *dp_ctx)
{
	return dp_ctx->dp_cfg.is_flow_balance_enabled;
}

void wlan_dp_fb_update_num_rx_rings(struct wlan_objmgr_psoc *psoc);
#else
static inline enum wlan_dp_fb_status
wlan_dp_fb_handler(struct wlan_dp_psoc_context *dp_ctx,
		   uint32_t *per_ring_pkt_avg,
		   uint32_t total_avg_pkts,
		   struct cpu_load *cpu_load,
		   uint32_t num_cpus,
		   struct wlan_dp_rx_ring_wtg *balanced_wtgs)
{
	return FLOW_BALANCE_NOT_ELIGIBLE;
}

static inline bool wlan_dp_fb_enabled(struct wlan_dp_psoc_context *dp_ctx)
{
	return false;
}
#endif
#endif /*_WLAN_DP_FLOW_BALANCE_H_ */
