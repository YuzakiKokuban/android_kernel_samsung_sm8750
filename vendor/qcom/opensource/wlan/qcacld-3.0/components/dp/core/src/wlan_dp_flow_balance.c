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

#include "wlan_dp_fisa_rx.h"
#include "wlan_dp_flow_balance.h"
#include <cdp_txrx_ctrl.h>
#include "wlan_dp_main.h"

/* units in percentage(%) */
#define FLOW_THRASH_DETECT_THRES 90
#define RING_OVERLOAD_THRESH 70
#define ALLOWED_BUFFER_WT 5

#define FLOW_WTG_TABEL_SIZE 10
#define WTG_TBL_SEARCH_SKID_LEN 2

static int wlan_dp_fb_sort_flow_map_tbl(const void *e1, const void *e2)
{
	struct wlan_dp_rx_ring_fm_tbl *elem1;
	struct wlan_dp_rx_ring_fm_tbl *elem2;

	elem1 = (struct wlan_dp_rx_ring_fm_tbl *)e1;
	elem2 = (struct wlan_dp_rx_ring_fm_tbl *)e2;

	if (elem1->ring_weightage > elem2->ring_weightage)
		return -1;
	else if (elem1->ring_weightage < elem2->ring_weightage)
		return 1;
	else
		return 0;
}

static int wlan_dp_fb_sort_targeted_wtg_dist(const void *e1, const void *e2)
{
	uint32_t *elem1 = (uint32_t *)e1;
	uint32_t *elem2 = (uint32_t *)e2;

	if (*elem1 > *elem2)
		return -1;
	else if (*elem1 < *elem2)
		return 1;
	else
		return 0;
}

static bool
wlan_dp_fb_check_flow_balance_is_needed(struct wlan_dp_rx_ring_fm_tbl *map_tbl,
					uint32_t *targeted_wtgs,
					uint8_t num_rings)
{
	struct wlan_dp_rx_ring_fm_tbl *ring;
	uint8_t cur_num_active_rings = 0;
	int i;
	bool do_balance = false;

	/* Check if the current ring weightages are +-5% of the targeted
	 * ring weightages. If yes, then no flow balance required
	 */
	for (i = 0; i < MAX_REO_DEST_RINGS; i++) {
		ring = &map_tbl[i];

		if (!ring->ring_weightage)
			break;

		cur_num_active_rings++;

		/* Check current ring weight is +-5% of the targeted weight */
		if ((targeted_wtgs[i] - ALLOWED_BUFFER_WT <=
		     ring->ring_weightage) &&
		    (targeted_wtgs[i] + ALLOWED_BUFFER_WT >=
		     ring->ring_weightage)) {
			continue;
		} else {
			do_balance = true;
			break;
		}
	}

	if ((num_rings == cur_num_active_rings) && !do_balance)
		return false;
	else
		return true;
}

static inline void
wlan_dp_fb_update_cur_ring_wtgs(struct wlan_dp_rx_ring_fm_tbl *map_tbl,
				struct wlan_dp_rx_ring_wtg *balanced_wtgs,
				uint8_t num_rings)
{
	struct wlan_dp_rx_ring_fm_tbl *ring;
	struct wlan_dp_rx_ring_wtg *ring_wtg;
	int i;

	for (i = 0; i < num_rings; i++) {
		ring = &map_tbl[i];
		ring_wtg = &balanced_wtgs[i];
		ring_wtg->weight = ring->ring_weightage;
		ring_wtg->ring_id = ring->ring_id;
	}
}

/**
 * wlan_dp_fb_add_flow_to_fwt() - Add element to the flow weightage table
 * @flow_wtg_tbl: pointer to the flow weightage table base
 * @flow: flow info
 * @napi_id: current napi id of the flow
 *
 * Return: none
 */
static void wlan_dp_fb_add_flow_to_fwt(struct wlan_dp_fwt_elem **flow_wtg_tbl,
				       struct wlan_dp_flow *flow,
				       uint8_t napi_id)
{
	struct wlan_dp_fwt_elem *fwt_elem;
	uint32_t index;

	fwt_elem = qdf_mem_malloc(sizeof(*fwt_elem));
	if (!fwt_elem) {
		dp_err("failed to allocate memory for flow wtg elem");
		return;
	}

	fwt_elem->flow_id = flow->flow_id;
	fwt_elem->weightage = flow->flow_wtg;
	fwt_elem->napi_id = napi_id;

	index = (flow->flow_wtg / FLOW_WTG_TABEL_SIZE);

	if (index == FLOW_WTG_TABEL_SIZE)
		index -= 1;

	if (flow_wtg_tbl[index]) {
		fwt_elem->next = flow_wtg_tbl[index];
		flow_wtg_tbl[index] = fwt_elem;
	} else {
		flow_wtg_tbl[index] = fwt_elem;
	}
}

/**
 * wlan_dp_fb_get_flow_from_fwt() - Get flow from the flow weightage table
 * @flow_wtg_tbl: pointer to the flow weightage table base
 * @weight: flow weight
 * @skid_len: how many entries can be searched above the index
 * @first_search: first weight search for the ring
 *
 * Return: flow element on success, NULL on search failure
 */
static struct wlan_dp_fwt_elem*
wlan_dp_fb_get_flow_from_fwt(struct wlan_dp_fwt_elem **flow_wtg_tbl,
			     int weight, int skid_len, bool first_search)
{
	struct wlan_dp_fwt_elem *fwt_elem = NULL;
	int index, tmp;
	int max_skid;

	index = weight / FLOW_WTG_TABEL_SIZE;
	if (index >= FLOW_WTG_TABEL_SIZE)
		index = FLOW_WTG_TABEL_SIZE - 1;

	/* check for lesser weights until 0th index */
	for (tmp = index; tmp >= 0; tmp--) {
		if (!flow_wtg_tbl[tmp])
			continue;

		goto found_element;
	}

	/* if the get flow called for the fist time for a ring then iterate
	 * through the entire table and find the best possible weight,
	 * else search until skid length above the index.
	 */
	max_skid = !first_search ? index + skid_len : FLOW_WTG_TABEL_SIZE;
	if (max_skid > FLOW_WTG_TABEL_SIZE)
		max_skid = FLOW_WTG_TABEL_SIZE;

	for (tmp = index + 1; tmp < max_skid; tmp++) {
		if (!flow_wtg_tbl[tmp])
			continue;

		goto found_element;
	}

	return fwt_elem;

found_element:
	fwt_elem = flow_wtg_tbl[tmp];
	flow_wtg_tbl[tmp] = fwt_elem->next;
	return fwt_elem;
}

/**
 * wlan_dp_fb_do_flow_balance() - Core API for flow balance
 * 1. Firstly check whether the number of current active rings are equal to the
 * number of rings decided for the flow balance, and the current ring wightages
 * are +-5% of the targeted ring weightages. flow balance is not required in
 * this case.
 * 2. Loop through the ring flow map table and add the flow weights grater
 * than the targeted ring weightages.
 * 3. Loop for the number of requested rings for the flow balance, get the
 * weightgs from the flow weightage table until targeted weightage reached
 * for each ring.
 * 4. Add flow info to the migrate list if the existing napi id is not equal
 * to the new napi id.
 * 5. Update the FST table for the migrated list.
 *
 * @dp_ctx: dp context
 * @map_tbl: per ring per flow map table
 * @targeted_wtgs: per ring targeted weightage for the decided number of rings
 * @num_rings: number of rings decided for the flow balance
 * @balanced_wtgs: ring weightages after flow balance
 *
 * Return: none
 */

static void
wlan_dp_fb_do_flow_balance(struct wlan_dp_psoc_context *dp_ctx,
			   struct wlan_dp_rx_ring_fm_tbl *map_tbl,
			   uint32_t *targeted_wtgs,
			   uint8_t num_rings,
			   struct wlan_dp_rx_ring_wtg *balanced_wtgs)
{
	struct dp_rx_fst *rx_fst = dp_ctx->rx_fst;
	struct wlan_dp_fwt_elem *flow_wtg_tbl[FLOW_WTG_TABEL_SIZE] = {NULL};
	struct wlan_dp_fwt_elem *fwt_elem;
	struct wlan_dp_mig_flow *mig_list;
	struct wlan_dp_flow *flow;
	struct wlan_dp_rx_ring_fm_tbl *ring;
	struct wlan_dp_rx_ring_wtg *ring_wtg;
	uint32_t flow_wtg_tbl_depth = 0;
	uint32_t mig_flow_idx = 0;
	int ring_idx = 0;
	int i;
	int delta;
	bool do_balance;
	bool first_search;

	/* Check the current ring weightages are already meeting the targeted
	 * weightages, update the current ring weightages in the balanced
	 * weightages and return in that case.
	 */
	do_balance = wlan_dp_fb_check_flow_balance_is_needed(map_tbl,
							     targeted_wtgs,
							     num_rings);
	if (!do_balance) {
		wlan_dp_fb_update_cur_ring_wtgs(map_tbl,
						balanced_wtgs, num_rings);
		return;
	}

	qdf_sort(map_tbl, MAX_REO_DEST_RINGS, sizeof(*map_tbl),
		 wlan_dp_fb_sort_flow_map_tbl, NULL);

	qdf_sort(targeted_wtgs, MAX_REO_DEST_RINGS, sizeof(uint32_t),
		 wlan_dp_fb_sort_targeted_wtg_dist, NULL);

	mig_list =
		(struct wlan_dp_mig_flow *)qdf_mem_malloc(rx_fst->max_entries *
							  sizeof(*mig_list));

	/* move the weightages more than then targeted weightage of the ring
	 * to the flow weightage table.
	 */
	for (i = 0; i < MAX_REO_DEST_RINGS; i++) {
		ring = &map_tbl[i];

		while (ring->ring_weightage >
		       (targeted_wtgs[i] + ALLOWED_BUFFER_WT)) {
			flow = &ring->flow_list[ring->num_flows - 1];
			ring->ring_weightage -= flow->flow_wtg;
			wlan_dp_fb_add_flow_to_fwt(flow_wtg_tbl, flow,
						   ring->ring_id);
			ring->num_flows--;
			flow_wtg_tbl_depth++;
		}
	}

	/* Get the weightages from flow weightage table and map the flows
	 * to the rings till the targeted ring weightages met.
	 */
	for (i = 0; i < num_rings; i++) {
get_next_ring:
		if (ring_idx >= MAX_REO_DEST_RINGS)
			break;

		ring = &map_tbl[ring_idx];
		ring_wtg = &balanced_wtgs[i];

		if (wlan_dp_check_is_ring_ipa_rx(dp_ctx->cdp_soc,
						 ring->ring_id) ||
		    dp_rx_is_ring_latency_sensitive_reo(ring->ring_id)) {
			ring_idx++;
			goto get_next_ring;
		}

		ring_idx++;
		delta = targeted_wtgs[i] - ring->ring_weightage;
		if (delta <= ALLOWED_BUFFER_WT) {
			ring_wtg->weight = ring->ring_weightage;
			ring_wtg->ring_id = ring->ring_id;
			continue;
		}

		first_search = true;

		while (delta > ALLOWED_BUFFER_WT && flow_wtg_tbl_depth) {
			fwt_elem =
			 wlan_dp_fb_get_flow_from_fwt(flow_wtg_tbl, delta,
						      WTG_TBL_SEARCH_SKID_LEN,
						      first_search);
			if (!fwt_elem)
				break;

			first_search = false;
			flow_wtg_tbl_depth--;
			ring->ring_weightage += fwt_elem->weightage;

			if (fwt_elem->napi_id != ring->ring_id) {
				mig_list[mig_flow_idx].flow_id =
							fwt_elem->flow_id;
				mig_list[mig_flow_idx].napi_id = ring->ring_id;
				mig_flow_idx++;
			}

			delta -= fwt_elem->weightage;
			qdf_mem_free(fwt_elem);
		}

		ring_wtg->weight = ring->ring_weightage;
		ring_wtg->ring_id = ring->ring_id;
	}

	if (mig_flow_idx)
		dp_fisa_update_fst_table(dp_ctx, mig_list, mig_flow_idx);

	qdf_mem_free(mig_list);
}

static inline int
wlan_dp_fb_get_num_rings_for_flow_balance(uint8_t num_rx_rings,
					  int total_num_flows)
{
	if (total_num_flows >= num_rx_rings)
		return num_rx_rings;
	else
		return total_num_flows;
}

/**
 * wlan_dp_fb_get_per_ring_targeted_weightage() - Gives the per ring targeted
 * weightages
 * Calculated the number of rings will going to assign to each cpu and
 * targeted weightage of each ring based on the allowed wlan load of the
 * respective CPU.
 * Example: num_cpus = 2, num_rings = 4, allowed_load_on_cpu1 = 60%,
 *	allowed_load_on_cpu2 = 40% then,
 * targeted_weightages will be ring0 = 30%, ring1 = 30%,
 *	ring2 = 20% and ring4 = 20%.
 *
 * @cpu_load_avg: per cpu load average in percentage
 * @num_cpus: num cpus available for balance
 * @num_rings: number of rings for flow balance
 * @targeted_weightages: targeted weightages result
 */
static void
wlan_dp_fb_get_per_ring_targeted_weightage(struct cpu_load *cpu_load_avg,
					   int num_cpus, int num_rings,
					   uint32_t *targeted_weightages)
{
	int index = 0;
	int num_rings_per_cpu;
	int i, j, rem;

	rem = num_rings % num_cpus;

	for (i = 0; i < num_cpus; i++) {
		num_rings_per_cpu = num_rings / num_cpus;
		if (rem) {
			num_rings_per_cpu++;
			rem--;
		}

		for (j = 0; j < num_rings_per_cpu; j++)
			targeted_weightages[index++] =
				cpu_load_avg[i].allowed_wtg / num_rings_per_cpu;
	}
}

/**
 * wlan_dp_fb_cal_per_flow_weightage() - calculate the per flow weightage in
 * percentage
 * weightage = flow throughput / total throughput.
 * @map_tbl: rx ring flow map table which holds the per ring per flow info
 * @total_flow_pkt_avg: sum of packets per second avg of all the flows
 *
 * Return: None
 */
static void
wlan_dp_fb_cal_per_flow_weightage(struct wlan_dp_rx_ring_fm_tbl *map_tbl,
				  uint32_t total_flow_pkt_avg)
{
	struct wlan_dp_rx_ring_fm_tbl *ring;
	struct wlan_dp_flow *flow_info;
	int i, flow;

	for (i = 0; i < MAX_REO_DEST_RINGS; i++) {
		ring = &map_tbl[i];

		if (!ring->total_avg_pkts)
			continue;

		for (flow = 0; flow < ring->num_flows; flow++) {
			flow_info = &ring->flow_list[flow];
			flow_info->flow_wtg = (flow_info->avg_pkt_cnt * 100) /
						total_flow_pkt_avg;
			ring->ring_weightage += flow_info->flow_wtg;
		}
	}
}

/**
 * wlan_dp_fb_handler() - handler for flow balance called from load balance
 * handler
 * 1. Builds the flow_map table which holds the per ring flow details
 * 2. Calculates the per flow weightage and detects the flow thrashing by
 *    comparing the flow map table ring weightage with the per ring packet
 *    average computed from per reo stats.
 * 3. Calculate the num rings required for flow balance based on the num flows
 * 4. Calculate the per ring targeted weightage, so that the flows will be
 *     moved across the rings based on this targeted weightage.
 * 5. Do the flow balance and update the balanced ring weightages.
 *
 * @dp_ctx: dp context
 * @per_ring_pkt_avg: per ring packet average per second
 * @total_avg_pkts: sum of all rings packet average per second
 * @cpu_load: cpu load details in percentage
 * @num_cpus: number of cpus available for load balance
 * @balanced_wtgs: ring weightage after flow balance
 *
 * Return: returns the wlan_dp_fb_status
 */
enum wlan_dp_fb_status
wlan_dp_fb_handler(struct wlan_dp_psoc_context *dp_ctx,
		   uint32_t *per_ring_pkt_avg,
		   uint32_t total_avg_pkts,
		   struct cpu_load *cpu_load,
		   uint32_t num_cpus,
		   struct wlan_dp_rx_ring_wtg *balanced_wtgs)
{
	struct dp_rx_fst *rx_fst = dp_ctx->rx_fst;
	struct wlan_dp_rx_ring_fm_tbl flow_map_tbl[MAX_REO_DEST_RINGS] = {0};
	struct wlan_dp_rx_ring_fm_tbl *ring;
	uint32_t targeted_weightages[MAX_REO_DEST_RINGS] = {0};
	uint32_t total_flow_pkt_avg = 0;
	uint32_t total_num_flows = 0;
	int i;
	uint8_t total_num_rings = dp_ctx->lb_data.num_wlan_used_rx_rings;
	uint8_t num_rings;
	bool flow_thrash = false;
	bool ring_overloaded = false;
	bool flow_balance_not_eligible = false;

	for (i = 0; i <  MAX_REO_DEST_RINGS; i++) {
		ring = &flow_map_tbl[i];
		ring->ring_id = i;
		ring->flow_list =
		(struct wlan_dp_flow *)qdf_mem_malloc(rx_fst->max_entries *
						      sizeof(struct wlan_dp_flow));
		if (!ring->flow_list) {
			dp_err("failed to allocate flow list for ring %d", i);
			flow_balance_not_eligible = true;
			goto free_flow_list;
		}
	}

	dp_fisa_flow_balance_build_flow_map_tbl(dp_ctx, &flow_map_tbl[0],
						&total_flow_pkt_avg,
						&total_num_flows);

	if (total_num_flows <= 1) {
		flow_balance_not_eligible = true;
		goto free_flow_list;
	}

	wlan_dp_fb_cal_per_flow_weightage(&flow_map_tbl[0], total_flow_pkt_avg);

	/* Do the flow balance only in the following cases,
	 * 1. No flow thrashing detected
	 * 2. Most of the flows are mapped to single ring
	 */
	for (i = 0; i <  MAX_REO_DEST_RINGS; i++) {
		ring = &flow_map_tbl[i];
		if (ring->num_flows &&
		    (ring->total_avg_pkts <
		     ((per_ring_pkt_avg[i] * FLOW_THRASH_DETECT_THRES) / 100))) {
			dp_info("flow thrash detected on ring %d FAP %u RAP %u",
				i, ring->total_avg_pkts, per_ring_pkt_avg[i]);
			flow_thrash = true;
			break;
		}

		if (ring->ring_weightage > RING_OVERLOAD_THRESH) {
			dp_info("ring%d is overloaded wtg %u",
				i, ring->ring_weightage);
			ring_overloaded = true;
		}
	}

	if (flow_thrash || !ring_overloaded) {
		dp_info("fb is not eligible FT %s RO %s",
			flow_thrash ? "true" : "false",
			ring_overloaded ? "true" : "false");
		flow_balance_not_eligible = true;
		goto free_flow_list;
	}

	num_rings = wlan_dp_fb_get_num_rings_for_flow_balance(total_num_rings,
							      total_num_flows);

	dp_info("fb: num_rings %u num_flows %u", num_rings, total_num_flows);

	wlan_dp_fb_get_per_ring_targeted_weightage(cpu_load,
						   num_cpus, num_rings,
						   targeted_weightages);

	wlan_dp_fb_do_flow_balance(dp_ctx, &flow_map_tbl[0],
				   targeted_weightages, num_rings,
				   balanced_wtgs);
free_flow_list:
	for (i = 0; i <  MAX_REO_DEST_RINGS; i++) {
		ring = &flow_map_tbl[i];
		if (ring->flow_list)
			qdf_mem_free(ring->flow_list);
	}

	if (flow_balance_not_eligible)
		return FLOW_BALANCE_NOT_ELIGIBLE;
	else
		return FLOW_BALANCE_SUCCESS;
}

void wlan_dp_fb_update_num_rx_rings(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_psoc_get_priv(psoc);
	struct wlan_dp_lb_data *lb_data = &dp_ctx->lb_data;
	cdp_config_param_type soc_param;
	QDF_STATUS status;
	int i;
	uint8_t num_rx_rings = 0;

	status = cdp_txrx_get_psoc_param(dp_ctx->cdp_soc,
					 CDP_CFG_REO_RINGS_MAPPING, &soc_param);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Unable to fetch rx rings mapping");
		return;
	}

	for (i = 0; i < MAX_REO_DEST_RINGS; i++) {
		if ((BIT(i) & soc_param.cdp_reo_rings_mapping) &&
		    !dp_rx_is_ring_latency_sensitive_reo(i) &&
		    !wlan_dp_check_is_ring_ipa_rx(dp_ctx->cdp_soc, i))
			num_rx_rings++;
	}

	lb_data->num_wlan_used_rx_rings = num_rx_rings;
	dp_info("fb: num_host_used_rx_rings %d",
		lb_data->num_wlan_used_rx_rings);
}
