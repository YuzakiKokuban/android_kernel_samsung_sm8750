/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#include <wlan_objmgr_pdev_obj.h>
#include <wlan_dp_main.h>
#include <wlan_dp_priv.h>
#include <wlan_dp_resource_mgr.h>
#include "wlan_objmgr_vdev_obj_i.h"
#include "wlan_mlo_mgr_peer.h"
#include "wlan_mlme_main.h"
#include <nan_ucfg_api.h>
#include <wma.h>
#include <cdp_txrx_cmn.h>
#include <cdp_txrx_ctrl.h>
#include "cfg_ucfg_api.h"

static uint64_t
wlan_dp_resource_mgr_phymode_to_tput(enum wlan_phymode phymode)
{
	switch (phymode) {
	case WLAN_PHYMODE_11A:
		return WLAN_PHYMODE_11A_TPUT_MBPS;
	case WLAN_PHYMODE_11B:
		return WLAN_PHYMODE_11B_TPUT_MBPS;
	case WLAN_PHYMODE_11G:
		return WLAN_PHYMODE_11G_TPUT_MBPS;
	case WLAN_PHYMODE_11G_ONLY:
		return WLAN_PHYMODE_11G_ONLY_TPUT_MBPS;
	case WLAN_PHYMODE_11NA_HT20:
		return WLAN_PHYMODE_11NA_HT20_TPUT_MBPS;
	case WLAN_PHYMODE_11NG_HT20:
		return WLAN_PHYMODE_11NG_HT20_TPUT_MBPS;
	case WLAN_PHYMODE_11NA_HT40:
		return WLAN_PHYMODE_11NA_HT40_TPUT_MBPS;
	case WLAN_PHYMODE_11NG_HT40PLUS:
		return WLAN_PHYMODE_11NG_HT40PLUS_TPUT_MBPS;
	case WLAN_PHYMODE_11NG_HT40MINUS:
		return WLAN_PHYMODE_11NG_HT40MINUS_TPUT_MBPS;
	case WLAN_PHYMODE_11NG_HT40:
		return WLAN_PHYMODE_11NG_HT40_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT20:
		return WLAN_PHYMODE_11AC_VHT20_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT20_2G:
		return WLAN_PHYMODE_11AC_VHT20_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT40:
		return WLAN_PHYMODE_11AC_VHT40_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT40PLUS_2G:
		return WLAN_PHYMODE_11AC_VHT40PLUS_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT40MINUS_2G:
		return WLAN_PHYMODE_11AC_VHT40MINUS_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT40_2G:
		return WLAN_PHYMODE_11AC_VHT40_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT80:
		return WLAN_PHYMODE_11AC_VHT80_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT80_2G:
		return WLAN_PHYMODE_11AC_VHT80_2G_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT160:
		return WLAN_PHYMODE_11AC_VHT160_TPUT_MBPS;
	case WLAN_PHYMODE_11AC_VHT80_80:
		return WLAN_PHYMODE_11AC_VHT80_80_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE20:
		return WLAN_PHYMODE_11AXA_HE20_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE20:
		return WLAN_PHYMODE_11AXG_HE20_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE40:
		return WLAN_PHYMODE_11AXA_HE40_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE40PLUS:
		return WLAN_PHYMODE_11AXG_HE40PLUS_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE40MINUS:
		return WLAN_PHYMODE_11AXG_HE40MINUS_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE40:
		return WLAN_PHYMODE_11AXG_HE40_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE80:
		return WLAN_PHYMODE_11AXA_HE80_TPUT_MBPS;
	case WLAN_PHYMODE_11AXG_HE80:
		return WLAN_PHYMODE_11AXG_HE80_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE160:
		return WLAN_PHYMODE_11AXA_HE160_TPUT_MBPS;
	case WLAN_PHYMODE_11AXA_HE80_80:
		return WLAN_PHYMODE_11AXA_HE80_80_TPUT_MBPS;
#ifdef WLAN_FEATURE_11BE
	case WLAN_PHYMODE_11BEA_EHT20:
		return WLAN_PHYMODE_11BEA_EHT20_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT20:
		return WLAN_PHYMODE_11BEG_EHT20_TPUT_MBPS;
	case WLAN_PHYMODE_11BEA_EHT40:
		return WLAN_PHYMODE_11BEA_EHT40_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT40PLUS:
		return WLAN_PHYMODE_11BEG_EHT40PLUS_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT40MINUS:
		return WLAN_PHYMODE_11BEG_EHT40MINUS_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT40:
		return WLAN_PHYMODE_11BEG_EHT40_TPUT_MBPS;
	case WLAN_PHYMODE_11BEA_EHT80:
		return WLAN_PHYMODE_11BEA_EHT80_TPUT_MBPS;
	case WLAN_PHYMODE_11BEG_EHT80:
		return WLAN_PHYMODE_11BEG_EHT80_TPUT_MBPS;
	case WLAN_PHYMODE_11BEA_EHT160:
		return WLAN_PHYMODE_11BEA_EHT160_TPUT_MBPS;
	case WLAN_PHYMODE_11BEA_EHT320:
		return WLAN_PHYMODE_11BEA_EHT320_TPUT_MBPS;
#endif
	default:
		return WLAN_PHYMODE_DEFAULT_TPUT_MBPS;
	}
}

static void
wlan_dp_resource_mgr_post_upscale_resource_req(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	ol_txrx_soc_handle cdp_soc = rsrc_ctx->dp_ctx->cdp_soc;
	struct dp_rx_refill_thread *refill_thread;
	struct dp_txrx_handle *dp_ext_hdl;

	dp_rsrc_mgr_debug("Posting Upscale resource");
	dp_ext_hdl = cdp_soc_get_dp_txrx_handle(cdp_soc);

	refill_thread = &dp_ext_hdl->refill_thread;

	/*Clear any previous resource downscale event*/
	qdf_atomic_clear_bit(RX_RESOURCE_DOWNSCALE_EVENT,
			     &refill_thread->event_flag);
	qdf_set_bit(RX_RESOURCE_UPSCALE_EVENT,
		    &refill_thread->event_flag);
	qdf_set_bit(RX_REFILL_POST_EVENT, &refill_thread->event_flag);
	qdf_wake_up_interruptible(&refill_thread->wait_q);
}

static void
wlan_dp_resource_mgr_post_downscale_resource_req(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				bool delay_req)
{
	ol_txrx_soc_handle cdp_soc = rsrc_ctx->dp_ctx->cdp_soc;
	struct dp_rx_refill_thread *refill_thread;
	struct dp_txrx_handle *dp_ext_hdl;

	if (delay_req) {
		dp_rsrc_mgr_debug("DP rsrc mgr downscale req dealyed to timer ctx");
		qdf_timer_mod(&rsrc_ctx->timer,
			      DP_RSRC_MGR_TIMER_MS);
		return;
	}

	dp_rsrc_mgr_debug("Posting Downscale resource");
	dp_ext_hdl = cdp_soc_get_dp_txrx_handle(cdp_soc);

	refill_thread = &dp_ext_hdl->refill_thread;

	/*clear any previous resource upscale event*/
	qdf_atomic_clear_bit(RX_RESOURCE_UPSCALE_EVENT,
			     &refill_thread->event_flag);
	qdf_set_bit(RX_RESOURCE_DOWNSCALE_EVENT,
		    &refill_thread->event_flag);
	qdf_set_bit(RX_REFILL_POST_EVENT, &refill_thread->event_flag);
	qdf_wake_up_interruptible(&refill_thread->wait_q);
}

static void
wlan_dp_resource_mgr_timer_handler(void *arg)
{
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx =
		(struct wlan_dp_resource_mgr_ctx *)arg;
	struct cdp_soc_t *cdp_soc = rsrc_ctx->dp_ctx->cdp_soc;
	enum wlan_dp_resource_level rsrc_level;
	uint64_t cur_req_rx_buff_descs;
	uint64_t req_rx_buff_descs;
	uint64_t in_use_rx_buff_descs;
	QDF_STATUS status;

	dp_rsrc_mgr_debug("DP rsrc mgr timer handler scheduled");
	status = cdp_get_num_buff_descs_info(cdp_soc,
					     &cur_req_rx_buff_descs,
					     &in_use_rx_buff_descs, 0);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Unable to fetch DP rx desc info");
		return;
	}

	qdf_spin_lock_bh(&rsrc_ctx->rsrc_mgr_lock);
	rsrc_level = rsrc_ctx->cur_resource_level;
	req_rx_buff_descs = rsrc_ctx->cur_rsrc_map[rsrc_level].num_rx_buffers;

	if (cur_req_rx_buff_descs > req_rx_buff_descs) {
		dp_info("Downscale required cur_rx_buf:%u to req_rx_buf:%u",
			cur_req_rx_buff_descs, req_rx_buff_descs);
		wlan_dp_resource_mgr_post_downscale_resource_req(rsrc_ctx,
								 false);
	}
	qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
}

void wlan_dp_resource_mgr_downscale_resources(void)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx = dp_ctx->rsrc_mgr_ctx;
	struct cdp_soc_t *cdp_soc = dp_ctx->cdp_soc;
	enum wlan_dp_resource_level rsrc_level;
	uint64_t cur_req_rx_buff_descs;
	uint64_t req_rx_buff_descs;
	uint64_t in_use_rx_buff_descs;
	QDF_STATUS status;

	if (!rsrc_ctx)
		return;

	dp_rsrc_mgr_debug("Downscale resource called");
	status = cdp_get_num_buff_descs_info(cdp_soc,
					     &cur_req_rx_buff_descs,
					     &in_use_rx_buff_descs, 0);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Unable to fetch DP rx desc info");
		return;
	}

	rsrc_level = rsrc_ctx->cur_resource_level;
	req_rx_buff_descs = rsrc_ctx->cur_rsrc_map[rsrc_level].num_rx_buffers;
	dp_rsrc_mgr_debug("rsrc_level:%u cur:%u in_use:%u req:%u",
			  rsrc_level, cur_req_rx_buff_descs,
			  in_use_rx_buff_descs, req_rx_buff_descs);

	if (cur_req_rx_buff_descs > req_rx_buff_descs) {
		dp_info("Downscaling cur_rx_buf:%u to req_rx_buf:%u",
			cur_req_rx_buff_descs, req_rx_buff_descs);
		status = cdp_set_req_buff_descs(cdp_soc, req_rx_buff_descs, 0);
		if (QDF_IS_STATUS_ERROR(status))
			dp_err("Unable to set req DP rx desc");
	}
}

void wlan_dp_resource_mgr_upscale_resources(
				struct dp_rx_refill_thread *refill_thread)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx = dp_ctx->rsrc_mgr_ctx;
	struct cdp_soc_t *cdp_soc = dp_ctx->cdp_soc;
	enum wlan_dp_resource_level rsrc_level;
	uint64_t cur_req_rx_buff_descs;
	uint64_t req_rx_buff_descs;
	uint64_t in_use_rx_buff_descs;
	uint64_t additional_rx_buffers = 0, batch_count = 0;
	uint32_t num_alloc_buff;
	QDF_STATUS status;

	if (!rsrc_ctx)
		return;

	dp_rsrc_mgr_debug("Upscale resource called");
	status = cdp_get_num_buff_descs_info(cdp_soc,
					     &cur_req_rx_buff_descs,
					     &in_use_rx_buff_descs, 0);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Unable to fetch DP rx desc info");
		return;
	}

	rsrc_level = rsrc_ctx->cur_resource_level;
	req_rx_buff_descs = rsrc_ctx->cur_rsrc_map[rsrc_level].num_rx_buffers;

	if (cur_req_rx_buff_descs != req_rx_buff_descs) {

		cdp_set_req_buff_descs(cdp_soc, req_rx_buff_descs, 0);
		if (QDF_IS_STATUS_ERROR(status)) {
			dp_err("Unable to set REQ DP rx desc");
			return;
		}
		if (req_rx_buff_descs > cur_req_rx_buff_descs &&
		    req_rx_buff_descs > in_use_rx_buff_descs)
			additional_rx_buffers = (req_rx_buff_descs -
				in_use_rx_buff_descs);

		dp_info("Upscaling cur_rx_buf:%u req_rx_buf:%u additional:%u",
			cur_req_rx_buff_descs, req_rx_buff_descs,
			additional_rx_buffers);
	}

	dp_rsrc_mgr_debug("cur:%u req:%u in_use:%u additional_req:%u",
			  cur_req_rx_buff_descs, req_rx_buff_descs,
			  in_use_rx_buff_descs, additional_rx_buffers);
	batch_count = RX_RESOURCE_ALLOC_BATCH_COUNT;
	while (additional_rx_buffers) {
		if (additional_rx_buffers < RX_RESOURCE_ALLOC_BATCH_COUNT)
			batch_count = additional_rx_buffers;
		num_alloc_buff = cdp_buffers_replenish_on_demand(cdp_soc,
							 batch_count, 0);
		if (!num_alloc_buff) {
			dp_err("failed to allocate rx buffers, count:%u",
			       batch_count);
			break;
		}
		if (qdf_atomic_test_and_clear_bit(
					RX_RESOURCE_DOWNSCALE_EVENT,
					&refill_thread->event_flag)) {
			dp_info("DOWNSCALE request posted, stopping uspcale");
			wlan_dp_resource_mgr_downscale_resources();
			break;
		}
		additional_rx_buffers -= num_alloc_buff;
	}

	if (additional_rx_buffers)
		dp_err("Upscale request deficit:%u", additional_rx_buffers);
}

static uint32_t
wlan_dp_resource_mgr_get_mac_id(struct wlan_objmgr_vdev *vdev)
{
	struct mlme_legacy_priv *mlme_priv;
	uint32_t mac_id = 0;

	mlme_priv = wlan_vdev_mlme_get_ext_hdl(vdev);
	if (!mlme_priv) {
		dp_err("Unable to get mlme priv");
		return mac_id;
	}

	if (mlme_priv->mac_id < MAX_MAC_RESOURCES)
		mac_id = mlme_priv->mac_id;

	return mac_id;
}

static void
wlan_dp_resource_mgr_find_nan_max_phymodes(
		struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
		struct wlan_dp_resource_vote_node **nan_max_vote_node)
{
	qdf_list_t *nan_list = &rsrc_ctx->nan_list;
	struct wlan_dp_resource_vote_node *first_max_node = NULL;
	struct wlan_dp_resource_vote_node *second_max_node = NULL;
	struct wlan_dp_resource_vote_node *first_max_mac0_node = NULL;
	struct wlan_dp_resource_vote_node *second_max_mac0_node = NULL;
	struct wlan_dp_resource_vote_node *vote_node_cur;
	qdf_list_node_t *cur_node = NULL, *next_node = NULL;
	bool ndi_dbs_support;
	QDF_STATUS status;

	dp_rsrc_mgr_debug("NAN find MAX phymodes called");
	/*NO active peers for NAN*/
	if (qdf_list_empty(nan_list))
		return;

	/*
	 * NAN single peer can have operating slots in both MACs,
	 * but at a time same peer can operate on single MAC.
	 * Multiple peers can operate simultaneously on both MACs.
	 * So pick max two phymodes of different peers and able to
	 * support simultaneous operation on different peers.
	 */
	ndi_dbs_support = ucfg_is_ndi_dbs_supported(rsrc_ctx->dp_ctx->psoc);
	status = qdf_list_peek_front(nan_list, &cur_node);
	while (QDF_IS_STATUS_SUCCESS(status)) {
		vote_node_cur = qdf_container_of(cur_node,
				struct wlan_dp_resource_vote_node, node);

		if (!first_max_node) {
			first_max_mac0_node = vote_node_cur;
			if (!ndi_dbs_support) {
				first_max_mac0_node->tput =
					first_max_mac0_node->tput0;
				first_max_mac0_node->phymode =
					first_max_mac0_node->phymode0;
				nan_max_vote_node[0] = first_max_mac0_node;
				dp_rsrc_mgr_debug("Single MAC NAN phymode");
				return;
			}
			first_max_node = vote_node_cur;
			goto get_next_node;
		}
		if (!second_max_node) {
			second_max_node = vote_node_cur;
			second_max_mac0_node = vote_node_cur;
			if (first_max_node->tput1 < second_max_node->tput1) {
				second_max_node = first_max_node;
				first_max_node = vote_node_cur;
			}
			goto get_next_node;
		}

		if (vote_node_cur->tput1 > first_max_node->tput1) {
			second_max_node = first_max_node;
			first_max_node = vote_node_cur;
		} else if (vote_node_cur->tput1 > second_max_node->tput1) {
			second_max_node = vote_node_cur;
		}

get_next_node:
		status = qdf_list_peek_next(nan_list, cur_node, &next_node);
		cur_node = next_node;
		next_node = NULL;
	}

	if (!second_max_node) {
		/*Single NAN peer case in DBS*/
		if (first_max_node->tput1 > first_max_node->tput0) {
			first_max_node->tput = first_max_node->tput1;
			first_max_node->phymode =
				first_max_node->phymode1;
			nan_max_vote_node[1] = first_max_node;
		} else {
			first_max_node->tput = first_max_node->tput0;
			first_max_node->phymode =
				first_max_node->phymode0;
			nan_max_vote_node[0] = first_max_node;
		}
		dp_rsrc_mgr_debug("Single NAN peer present in DBS case");
		return;
	}

	if (first_max_mac0_node == first_max_node) {
		dp_rsrc_mgr_debug("NAN Same peer have both MACs vote");
		/*
		 * MAC-0 max phymode tput and MAC-1 max phymode tput are of
		 * same peer, ideally same peer cannot have simultaneous rx on both macs.
		 * so involve other peer in max phymode tput selection
		 */
		if ((first_max_node->tput0 + second_max_node->tput1) >
		    (first_max_node->tput1 + second_max_mac0_node->tput0)) {
			first_max_node->tput = first_max_node->tput0;
			first_max_node->phymode = first_max_node->phymode0;
			second_max_node->tput = second_max_node->tput1;
			second_max_node->phymode = second_max_node->phymode1;
			nan_max_vote_node[0] = first_max_node;
			nan_max_vote_node[1] = second_max_node;
		} else {
			first_max_node->tput = first_max_node->tput1;
			first_max_node->phymode = first_max_node->phymode1;
			second_max_mac0_node->tput =
				second_max_mac0_node->tput0;
			second_max_mac0_node->phymode =
				second_max_mac0_node->phymode0;
			nan_max_vote_node[0] = second_max_mac0_node;
			nan_max_vote_node[1] = first_max_node;
		}
		return;
	}

	dp_rsrc_mgr_debug("NAN multi peer MACs votes selected");
	first_max_mac0_node->tput = first_max_mac0_node->tput0;
	first_max_mac0_node->phymode = first_max_mac0_node->phymode0;
	first_max_node->tput = second_max_node->tput1;
	first_max_node->phymode = second_max_node->phymode1;

	nan_max_vote_node[0] = first_max_mac0_node;
	nan_max_vote_node[1] = second_max_node;
}

static void
wlan_dp_resource_mgr_find_max_mac_n_phymodes(
			struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
			struct wlan_dp_resource_vote_node **max_mac_n_phymodes)
{
	struct wlan_dp_resource_vote_node *vote_node_cur;
	qdf_list_node_t *cur_node = NULL, *next_node = NULL;
	QDF_STATUS status;
	int i;

	status = qdf_list_peek_front(&rsrc_ctx->mac_n_list, &cur_node);
	for (i = 0; i < rsrc_ctx->mac_count; i++) {
		if (QDF_IS_STATUS_ERROR(status))
			break;
		vote_node_cur = qdf_container_of(cur_node,
				struct wlan_dp_resource_vote_node, node);
		max_mac_n_phymodes[i] = vote_node_cur;

		status = qdf_list_peek_next(&rsrc_ctx->mac_n_list,
					    cur_node, &next_node);
		cur_node = next_node;
		next_node = NULL;
	}
}

static void
wlan_dp_resource_mgr_find_max_mac_phymodes(
			struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
			struct wlan_dp_resource_vote_node **max_mac_phymodes)
{
	struct wlan_dp_resource_vote_node *vote_node_cur;
	qdf_list_node_t *cur_node = NULL;
	QDF_STATUS status;
	int i;

	for (i = 0; i < rsrc_ctx->mac_count; i++) {
		status = qdf_list_peek_front(&rsrc_ctx->mac_list[i], &cur_node);
		if (QDF_IS_STATUS_SUCCESS(status)) {
			vote_node_cur = qdf_container_of(cur_node,
				struct wlan_dp_resource_vote_node, node);
			max_mac_phymodes[i] = vote_node_cur;
		}
	}
}

static void
wlan_dp_resource_mgr_sort_phymodes(struct wlan_dp_resource_vote_node **phymodes,
				   uint32_t mac_count)
{
	struct wlan_dp_resource_vote_node *vote_node;
	int i = 0, j;

	for (i = 0; i < mac_count; i++) {
		for (j = i + 1; j < mac_count; j++) {
			if (phymodes[i] && phymodes[j]) {
				/*Both vote nodes present normal sort*/
				if (phymodes[i]->tput > phymodes[j]->tput) {
					vote_node =  phymodes[i];
					phymodes[i] = phymodes[j];
					phymodes[j] = vote_node;
				}
			} else if (!phymodes[i]) {
				/*No vote node on this entry sort*/
				phymodes[i] = phymodes[j];
				phymodes[j] = NULL;
			}
		}
	}
}

static void
wlan_dp_resource_mgr_select_resource_level(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	struct wlan_dp_resource_vote_node *vote_node;
	uint64_t total_tput = 0;
	enum wlan_dp_resource_level prev_level;
	int i;

	dp_rsrc_mgr_debug("Select resource level called");
	if (rsrc_ctx->monitor_vdev_vote)
		total_tput =
			rsrc_ctx->cur_rsrc_map[RESOURCE_LVL_MAX - 1].max_tput;

	for (i = 0; i < rsrc_ctx->mac_count; i++) {
		vote_node = rsrc_ctx->max_phymode_nodes[i];
		if (vote_node)
			total_tput += vote_node->tput;
	}

	for (i = 0; i < RESOURCE_LVL_MAX; i++)
		if (total_tput <= rsrc_ctx->cur_rsrc_map[i].max_tput)
			break;

	/*Level overflow reset to max supported*/
	if (i >= RESOURCE_LVL_MAX)
		i = (RESOURCE_LVL_MAX - 1);

	prev_level = rsrc_ctx->cur_resource_level;
	rsrc_ctx->cur_resource_level = i;
	rsrc_ctx->cur_max_tput = total_tput;
	if (prev_level < rsrc_ctx->cur_resource_level)
		wlan_dp_resource_mgr_post_upscale_resource_req(rsrc_ctx);
	else if (prev_level > rsrc_ctx->cur_resource_level)
		wlan_dp_resource_mgr_post_downscale_resource_req(rsrc_ctx,
								 true);
	else
		dp_info("Resource level change not required");

	dp_info("Resource level:%u selected for tput:%u req_tput:%u",
		rsrc_ctx->cur_resource_level, rsrc_ctx->cur_max_tput,
		total_tput);
}

static void
wlan_dp_resource_mgr_select_max_phymodes(
			struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	struct wlan_dp_resource_vote_node *max_mac_phymodes[MAX_MAC_RESOURCES] = {NULL};
	struct wlan_dp_resource_vote_node *max_mac_n_phymodes[MAX_MAC_RESOURCES] = {NULL};
	struct wlan_dp_resource_vote_node *max_nan_phymodes[MAX_MAC_RESOURCES] = {NULL};
	uint32_t mac_count = rsrc_ctx->mac_count;
	int i, j, k;

	dp_rsrc_mgr_debug("Select max phymode called");

	wlan_dp_resource_mgr_find_nan_max_phymodes(rsrc_ctx, max_nan_phymodes);
	wlan_dp_resource_mgr_find_max_mac_phymodes(rsrc_ctx, max_mac_phymodes);
	wlan_dp_resource_mgr_find_max_mac_n_phymodes(rsrc_ctx,
						     max_mac_n_phymodes);

	qdf_mem_set(&rsrc_ctx->max_phymode_nodes, sizeof(rsrc_ctx->max_phymode_nodes), 0);
	/*MAX MAC based phymodes including NAN peers*/
	for (i = 0; i < mac_count; i++)
		max_mac_phymodes[i] =
			wlan_dp_resource_mgr_max_of_two_vote_nodes(
							max_mac_phymodes[i],
							max_nan_phymodes[i]);
	wlan_dp_resource_mgr_sort_phymodes(max_mac_phymodes, mac_count);
	if (qdf_list_empty(&rsrc_ctx->mac_n_list)) {
		qdf_mem_copy(&rsrc_ctx->max_phymode_nodes, &max_mac_phymodes,
			     (sizeof(rsrc_ctx->max_phymode_nodes[0]) * mac_count));
		dp_rsrc_mgr_debug("ML STA list empty");
		goto select_resource_level;
	}

	/*ML-STA connections present include them in resource selection*/
	i = j = k = 0;
	while (i < mac_count && j < mac_count) {
		/*If both nodes are present normal merge sort scenario*/
		if (max_mac_phymodes[i] && max_mac_n_phymodes[j]) {
			if (max_mac_phymodes[i]->tput > max_mac_n_phymodes[j]->tput) {
				rsrc_ctx->max_phymode_nodes[k] =
					max_mac_phymodes[i];
				i++;
			} else {
				rsrc_ctx->max_phymode_nodes[k] =
					max_mac_n_phymodes[j];
				j++;
			}
			k++;
			continue;
		} else if (max_mac_phymodes[i]) {
			/*Fetch entries from non null array*/
			qdf_mem_copy(&rsrc_ctx->max_phymode_nodes[k],
				     &max_mac_phymodes[i],
				sizeof(rsrc_ctx->max_phymode_nodes[k]) * (mac_count - k));
		} else if (max_mac_n_phymodes[j]) {
			qdf_mem_copy(&rsrc_ctx->max_phymode_nodes[k],
				     &max_mac_n_phymodes[j],
				sizeof(rsrc_ctx->max_phymode_nodes[k]) * (mac_count - k));
		}
		break;
	}
	dp_rsrc_mgr_debug("ML STA phymodes considered i:%u j:%u k:%u", i, j, k);

select_resource_level:
	wlan_dp_resource_mgr_select_resource_level(rsrc_ctx);
}

static void
wlan_dp_resource_mgr_list_insert_vote_node(qdf_list_t *mac_list,
				struct wlan_dp_resource_vote_node *vote_node,
				enum QDF_OPMODE opmode)
{
	struct wlan_dp_resource_vote_node *vote_node_cur;
	qdf_list_node_t *cur_node = NULL, *next_node = NULL;
	QDF_STATUS status;

	status = qdf_list_peek_front(mac_list, &cur_node);
	while (QDF_IS_STATUS_SUCCESS(status)) {
		vote_node_cur = qdf_container_of(cur_node,
				struct wlan_dp_resource_vote_node, node);

		if (opmode == QDF_NDI_MODE) {
			/*NAN case sort based on MAC0 tput*/
			if (vote_node->tput0 > vote_node_cur->tput0) {
				qdf_list_insert_before(mac_list,
						       &vote_node->node,
						       &vote_node_cur->node);
				break;
			}
		} else {
			if (vote_node->tput > vote_node_cur->tput) {
				qdf_list_insert_before(mac_list,
						       &vote_node->node,
						       &vote_node_cur->node);
				break;
			}
		}

		status = qdf_list_peek_next(mac_list, cur_node, &next_node);
		cur_node = next_node;
		next_node = NULL;
	}

	if (!cur_node)
		qdf_list_insert_back(mac_list, &vote_node->node);
	dp_rsrc_mgr_debug("vote node inserted to list for opmode:%u", opmode);
}

static struct wlan_dp_resource_vote_node *
wlan_dp_resource_mgr_add_vote_node(struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				   struct wlan_objmgr_peer *peer,
				   enum wlan_phymode phymode,
				   enum QDF_OPMODE opmode)
{
	struct wlan_dp_resource_vote_node *vote_node;
	qdf_list_t *mac_list;
	uint32_t mac_id;

	vote_node = qdf_mem_malloc(sizeof(*vote_node));
	if (!vote_node) {
		dp_err("Failed to allocate memory for resource vote node");
		return NULL;
	}

	vote_node->phymode = phymode;
	vote_node->tput = wlan_dp_resource_mgr_phymode_to_tput(phymode);
	mac_id = wlan_dp_resource_mgr_get_mac_id(wlan_peer_get_vdev(peer));
	if (mac_id > MAX_MAC_RESOURCES) {
		dp_err("MAC id is not valid");
		QDF_BUG(0);
	}
	vote_node->mac_id = mac_id;
	mac_list = &rsrc_ctx->mac_list[mac_id];
	wlan_dp_resource_mgr_list_insert_vote_node(mac_list, vote_node, opmode);
	dp_info("New vote node added to list mac_id:%u len:%u phymode:%u",
		vote_node->mac_id, qdf_list_size(mac_list),
		vote_node->phymode);

	return vote_node;
}

void
wlan_dp_resource_mgr_notify_ndp_channel_info(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_peer *peer,
				struct nan_datapath_channel_info *ch_info,
				uint32_t num_channels)
{
	struct wlan_dp_peer_priv_context *priv_ctx;
	struct wlan_dp_resource_vote_node *vote_node;
	enum QDF_OPMODE opmode = QDF_NDI_MODE;
	enum wlan_phymode mac0_phymode = 0, mac1_phymode = 0;
	uint64_t mac0_tput = 0, mac1_tput = 0;
	enum wlan_phymode host_phymode;
	int i;

	if (!rsrc_ctx)
		return;

	dp_rsrc_mgr_debug("NDP notify channel info called");
	for (i = 0; i < num_channels; i++) {
		host_phymode = wma_fw_to_host_phymode(ch_info->phymode);
		if (!ch_info->mac_id &&
		    (wlan_dp_resource_mgr_phymode_to_tput(host_phymode) >
		     mac0_tput)) {
			mac0_phymode = host_phymode;
			mac0_tput =
			wlan_dp_resource_mgr_phymode_to_tput(mac0_phymode);
		} else if (ch_info->mac_id &&
			   (wlan_dp_resource_mgr_phymode_to_tput(host_phymode) >
			    mac1_tput)) {
			mac1_phymode = host_phymode;
			mac1_tput = wlan_dp_resource_mgr_phymode_to_tput(mac1_phymode);
		}
		ch_info++;
	}

	qdf_spin_lock_bh(&rsrc_ctx->rsrc_mgr_lock);
	priv_ctx = dp_get_peer_priv_obj(peer);
	if (priv_ctx->vote_node) {
		vote_node = priv_ctx->vote_node;
		if (vote_node->phymode0 == mac0_phymode &&
		    vote_node->phymode1 == mac1_phymode) {
			qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
			dp_info("NDP Max phymode did not change");
			return;
		}

		vote_node->phymode0 = mac0_phymode;
		vote_node->tput0 = mac0_tput;
		vote_node->phymode1 = mac1_phymode;
		vote_node->tput1 = mac1_tput;

		/*
		 * Removing from the sorted list, since phymode changed.
		 * Adding to list in based on new phymode throughput value.
		 */
		qdf_list_remove_node(&rsrc_ctx->nan_list,
				     &vote_node->node);
		wlan_dp_resource_mgr_list_insert_vote_node(
			&rsrc_ctx->nan_list, vote_node, opmode);
		dp_rsrc_mgr_debug("NDP resource vote node re-insert phymode0:%u phymode1:%u list_len:%u",
				  vote_node->phymode0, vote_node->phymode1,
				  qdf_list_size(&rsrc_ctx->nan_list));
		goto select_max_phymodes;
	}

	vote_node = qdf_mem_malloc(sizeof(*vote_node));
	if (!vote_node) {
		qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
		dp_err("Failed to allocate memory for resource vote node");
		return;
	}
	priv_ctx->vote_node = vote_node;
	vote_node->phymode0 = mac0_phymode;
	vote_node->tput0 = mac0_tput;
	vote_node->phymode1 = mac1_phymode;
	vote_node->tput1 = mac1_tput;
	wlan_dp_resource_mgr_list_insert_vote_node(&rsrc_ctx->nan_list,
						   vote_node, opmode);
	dp_info("NDP new node phymode0:%u tput0:%u phymode1:%u tput1:%u list_len:%u",
		vote_node->phymode0, vote_node->tput0,
		vote_node->phymode1, vote_node->tput1,
		qdf_list_size(&rsrc_ctx->nan_list));
select_max_phymodes:
	wlan_dp_resource_mgr_select_max_phymodes(rsrc_ctx);
	qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
}

static struct wlan_dp_resource_vote_node*
wlan_dp_resource_mgr_find_ml_vote_node(qdf_list_t *mac_n_list,
				       uint8_t *self_mac, uint8_t *remote_mac)
{
	struct wlan_dp_resource_vote_node *vote_node_cur;
	qdf_list_node_t *cur_node = NULL, *next_node = NULL;
	QDF_STATUS status;

	status = qdf_list_peek_front(mac_n_list, &cur_node);
	while (QDF_IS_STATUS_SUCCESS(status)) {
		vote_node_cur = qdf_container_of(cur_node,
				struct wlan_dp_resource_vote_node, node);

		if ((qdf_mem_cmp(self_mac, &vote_node_cur->self_mac,
			       QDF_MAC_ADDR_SIZE) == 0) &&
		   (qdf_mem_cmp(remote_mac, &vote_node_cur->remote_mac,
			       QDF_MAC_ADDR_SIZE) == 0)) {
			return vote_node_cur;
		}

		status = qdf_list_peek_next(mac_n_list, cur_node, &next_node);
		cur_node = next_node;
		next_node = NULL;
	}

	return NULL;
}

static void
wlan_dp_resource_mgr_handle_ml_sta_phymode_update(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_peer *peer)
{
	qdf_list_t *mac_n_list = &rsrc_ctx->mac_n_list;
	struct wlan_dp_resource_vote_node *vote_node;
	struct wlan_dp_resource_ml_vote_info *ml_vote_info = NULL;
	uint8_t *self_mac =
		wlan_vdev_mlme_get_macaddr(wlan_peer_get_vdev(peer));
	uint8_t *remote_mac = wlan_peer_get_macaddr(peer);
	enum wlan_phymode phymode;
	struct mlo_link_info *link_info;
	uint8_t link_info_iter;

	/*vote is based on connected BSS peer*/
	if (wlan_peer_get_peer_type(peer) != WLAN_PEER_AP)
		return;

	qdf_spin_lock_bh(&rsrc_ctx->rsrc_mgr_lock);
	/*Vote node is part of list, update existing phymode*/
	vote_node = wlan_dp_resource_mgr_find_ml_vote_node(mac_n_list, self_mac,
							   remote_mac);
	if (vote_node) {
		/*Increment refcount only for first time set phymode*/
		if (vote_node->vdev_id == WLAN_INVALID_VDEV_ID) {
			qdf_atomic_inc(&vote_node->ml_vote_info->ref_cnt);
			vote_node->vdev_id = wlan_vdev_get_id(wlan_peer_get_vdev(peer));
			dp_info("Partner link assoc ref_cnt:%u",
				qdf_atomic_read(&vote_node->ml_vote_info->ref_cnt));
		}

		phymode = wlan_peer_get_phymode(peer);
		if (vote_node->phymode == phymode) {
			qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
			return;
		}
		vote_node->phymode = phymode;
		vote_node->tput =
			wlan_dp_resource_mgr_phymode_to_tput(phymode);

		/*
		 * Removing from the sorted list, since phymode changed.
		 * Adding to list in based on new phymode throughput value.
		 */
		qdf_list_remove_node(mac_n_list, &vote_node->node);
		wlan_dp_resource_mgr_list_insert_vote_node(mac_n_list,
							   vote_node,
							   QDF_STA_MODE);
		goto select_max_phymodes;
	}

	/*
	 * vote node is not part of list, first link assoc happening
	 * fetch partner links and add vote nodes for all the links.
	 */
	link_info = mlo_mgr_get_ap_link(wlan_peer_get_vdev(peer));
	if (!link_info) {
		qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
		dp_err("Resource mgr not able to get STA ML link info");
		return;
	}

	for (link_info_iter = 0; link_info_iter < WLAN_MAX_ML_BSS_LINKS;
	     link_info_iter++) {
		if (qdf_is_macaddr_zero(&link_info->ap_link_addr))
			break;

		vote_node = qdf_mem_malloc(sizeof(*vote_node));
		if (!vote_node) {
			dp_err("Failed to allocate memory for resource vote node");
			break;
		}

		/*Allocate common STA ML info*/
		if (!ml_vote_info) {
			ml_vote_info = qdf_mem_malloc(sizeof(struct wlan_dp_resource_ml_vote_info));
			if (!ml_vote_info) {
				qdf_mem_free(vote_node);
				dp_err("Failed to allocate memory for resource vote node");
					break;
			}
		}

		ml_vote_info->num_links++;
		vote_node->ml_vote_info = ml_vote_info;
		vote_node->phymode = link_info->link_chan_info->ch_phymode;
		vote_node->tput = wlan_dp_resource_mgr_phymode_to_tput(vote_node->phymode);
		vote_node->mac_id = UNKNOWN_MAC_ID;
		vote_node->vdev_id = WLAN_INVALID_VDEV_ID;
		qdf_copy_macaddr(&vote_node->self_mac, &link_info->link_addr);
		qdf_copy_macaddr(&vote_node->remote_mac, &link_info->ap_link_addr);
		if((qdf_mem_cmp(self_mac, &vote_node->self_mac,
				QDF_MAC_ADDR_SIZE) == 0) &&
				(qdf_mem_cmp(remote_mac, &vote_node->remote_mac,
				QDF_MAC_ADDR_SIZE) == 0)) {
			/*Main assoc link vote node ref count*/
			vote_node->vdev_id = wlan_vdev_get_id(wlan_peer_get_vdev(peer));
			qdf_atomic_inc(&ml_vote_info->ref_cnt);
			dp_rsrc_mgr_debug("Main assoc link node added ref_cnt:%u", qdf_atomic_read(&ml_vote_info->ref_cnt));
		}

		wlan_dp_resource_mgr_list_insert_vote_node(mac_n_list,
							   vote_node,
							   QDF_STA_MODE);
		link_info++;
	}

	dp_info("ML STA vote nodes added num_links:%u", link_info_iter);
	if (!ml_vote_info) {
		qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
		dp_err("Resource mgr ML STA links not added");
		return;
	}

select_max_phymodes:
	wlan_dp_resource_mgr_select_max_phymodes(rsrc_ctx);
	qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
}

static void
wlan_dp_resource_mgr_handle_ml_sta_remove_phymode(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_peer *peer)
{
	qdf_list_t *mac_n_list = &rsrc_ctx->mac_n_list;
	struct wlan_dp_resource_ml_vote_info *ml_vote_info = NULL;
	uint8_t *self_mac =
		wlan_vdev_mlme_get_macaddr(wlan_peer_get_vdev(peer));
	uint8_t *remote_mac = wlan_peer_get_macaddr(peer);
	struct wlan_dp_resource_vote_node *vote_node_cur;
	qdf_list_node_t *cur_node = NULL, *next_node = NULL;
	QDF_STATUS status;

	dp_rsrc_mgr_debug("ML STA remove phymode called");
	vote_node_cur = wlan_dp_resource_mgr_find_ml_vote_node(mac_n_list, self_mac,
							       remote_mac);
	if (!vote_node_cur) {
		dp_rsrc_mgr_debug("ML STA vote node not present to remove");
		return;
	}
	ml_vote_info = vote_node_cur->ml_vote_info;

	if (vote_node_cur->vdev_id == WLAN_INVALID_VDEV_ID)
		return;
	vote_node_cur->vdev_id = WLAN_INVALID_VDEV_ID;
	/*ML STA peer delete as part of link switch*/
	if (!qdf_atomic_dec_and_test(&ml_vote_info->ref_cnt))
		return;

	/*ML STA disconnection, remove all link vote nodes*/
	status = qdf_list_peek_front(mac_n_list, &cur_node);
	while (QDF_IS_STATUS_SUCCESS(status)) {
		vote_node_cur = qdf_container_of(cur_node,
						 struct wlan_dp_resource_vote_node, node);

		status = qdf_list_peek_next(mac_n_list, cur_node, &next_node);
		if(vote_node_cur->ml_vote_info == ml_vote_info) {
			qdf_list_remove_node(mac_n_list, &vote_node_cur->node);
			qdf_mem_free(vote_node_cur);
			ml_vote_info->num_links--;

		}

		cur_node = next_node;
		next_node = NULL;
	}

	if (ml_vote_info->num_links)
		dp_err("DP resource mgr ML link vote nodes are not in sync");
	qdf_mem_free(ml_vote_info);

	wlan_dp_resource_mgr_select_max_phymodes(rsrc_ctx);
}

static void
wlan_dp_resource_mgr_phymode_update(struct wlan_objmgr_peer *peer, void *arg)
{
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx =
		(struct wlan_dp_resource_mgr_ctx *)arg;
	struct wlan_dp_peer_priv_context *priv_ctx;
	struct wlan_dp_resource_vote_node *vote_node;
	struct wlan_objmgr_peer *vote_peer = peer;
	enum wlan_phymode vote_phymode = wlan_peer_get_phymode(peer);
	struct  wlan_objmgr_vdev *vdev;
	struct wlan_channel *desc_chan;
	enum QDF_OPMODE opmode;
	uint64_t prev_tput;

	vdev = wlan_peer_get_vdev(peer);
	opmode = wlan_vdev_mlme_get_opmode(vdev);

	dp_info("update phymode called for opmode:%u", opmode);

	/*NAN peers handled by NDP events*/
	if (opmode == QDF_NDI_MODE)
		return;

	if ((opmode == QDF_STA_MODE) && wlan_peer_is_mlo(peer))
		return wlan_dp_resource_mgr_handle_ml_sta_phymode_update(
							rsrc_ctx, peer);

	if ((opmode == QDF_SAP_MODE) || (opmode == QDF_P2P_GO_MODE)) {
		/*
		 * For SAP resource vote node is attached to self peer,
		 * And also fetch phymode from vdev desc_chan.
		 */
		vote_peer = wlan_vdev_get_selfpeer(peer->peer_objmgr.vdev);
		if (!vote_peer) {
			dp_err("SAP self peer not present");
			return;
		}
		desc_chan = wlan_vdev_mlme_get_des_chan(vdev);
		vote_phymode = desc_chan->ch_phymode;
	}

	qdf_spin_lock_bh(&rsrc_ctx->rsrc_mgr_lock);
	priv_ctx = dp_get_peer_priv_obj(vote_peer);
	if (priv_ctx->vote_node) {
		vote_node = priv_ctx->vote_node;
		if (vote_node->phymode == vote_phymode) {
			qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
			dp_info("same phymode update for opmode:%u", opmode);
			return;
		}

		prev_tput = vote_node->tput;
		vote_node->phymode = vote_phymode;
		vote_node->tput =
			wlan_dp_resource_mgr_phymode_to_tput(vote_phymode);

		/*
		 * Removing from the sorted list, since phymode changed.
		 * Adding to list in based on new phymode throughput value.
		 */
		qdf_list_remove_node(&rsrc_ctx->mac_list[vote_node->mac_id],
				     &vote_node->node);
		wlan_dp_resource_mgr_list_insert_vote_node(
			&rsrc_ctx->mac_list[vote_node->mac_id],
			vote_node, opmode);

		dp_info("phymode update for existing vote node phymode:%u", vote_node->phymode);
		/*
		 * Check if current resource vote node change increase
		 * overall system tput requirement.
		 */
		if (wlan_dp_resource_mgr_detect_tput_level_increase(rsrc_ctx,
								vote_node,
								prev_tput)) {
			wlan_dp_resource_mgr_select_resource_level(rsrc_ctx);
			qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
			return;
		}
		goto select_max_phymodes;
	}

	priv_ctx->vote_node =
		wlan_dp_resource_mgr_add_vote_node(rsrc_ctx,
						   vote_peer, vote_phymode,
						   opmode);
select_max_phymodes:
	wlan_dp_resource_mgr_select_max_phymodes(rsrc_ctx);
	qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
}

void
wlan_dp_resource_mgr_vote_node_free(struct wlan_objmgr_peer *peer)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx = dp_ctx->rsrc_mgr_ctx;
	struct wlan_dp_peer_priv_context *priv_ctx;
	struct wlan_dp_resource_vote_node *vote_node;
	struct wlan_objmgr_peer *vote_peer = peer;
	struct  wlan_objmgr_vdev *vdev;
	enum QDF_OPMODE opmode;

	if (!rsrc_ctx)
		return;

	vdev = wlan_peer_get_vdev(peer);
	opmode = wlan_vdev_mlme_get_opmode(vdev);

	dp_rsrc_mgr_debug("vote node free called for opmode:%u", opmode);

	if ((opmode == QDF_SAP_MODE) || (opmode == QDF_P2P_GO_MODE)) {
		/* Check for active clients connected to SAP*/
		if (wlan_vdev_get_peer_count(vdev) > 2)
			return;
		vote_peer = wlan_vdev_get_selfpeer(peer->peer_objmgr.vdev);
		if (!vote_peer) {
			dp_err("SAP self peer not present");
			return;
		}
	}

	qdf_spin_lock_bh(&rsrc_ctx->rsrc_mgr_lock);
	priv_ctx = dp_get_peer_priv_obj(vote_peer);
	if (priv_ctx->vote_node) {
		vote_node = priv_ctx->vote_node;
		if (opmode == QDF_NDI_MODE)
			qdf_list_remove_node(&rsrc_ctx->nan_list,
					     &vote_node->node);
		else
			qdf_list_remove_node(
				&rsrc_ctx->mac_list[vote_node->mac_id],
				&vote_node->node);

		dp_info("vote node freed on vdev_id:%u mac_id:%u list_len:%u",
			wlan_vdev_get_id(vdev), vote_node->mac_id,
			qdf_list_size(&rsrc_ctx->mac_list[vote_node->mac_id]));
		qdf_mem_free(vote_node);
		priv_ctx->vote_node = NULL;
		wlan_dp_resource_mgr_select_max_phymodes(rsrc_ctx);
	} else if (opmode == QDF_STA_MODE)
		wlan_dp_resource_mgr_handle_ml_sta_remove_phymode(rsrc_ctx,
								  peer);

	qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
}

void wlan_dp_resource_mgr_notify_vdev_mac_id_migration(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_vdev *vdev,
				uint32_t old_mac_id,
				uint32_t new_mac_id)
{
	enum QDF_OPMODE opmode = wlan_vdev_mlme_get_opmode(vdev);
	struct wlan_dp_resource_vote_node *vote_node;
	struct wlan_dp_peer_priv_context *priv_ctx;
	struct wlan_objmgr_peer *peer;
	bool list_update = false;

	if (!rsrc_ctx)
		return;

	dp_rsrc_mgr_debug("vdev migration came for opmode:%u", opmode);
	/*ML-STA connections are mac_n_list no handling required*/
	if (((opmode == QDF_STA_MODE) &&
	    wlan_vdev_mlme_is_mlo_vdev(vdev)) ||
			(opmode == QDF_NDI_MODE))
		return;

	qdf_spin_lock_bh(&rsrc_ctx->rsrc_mgr_lock);
	wlan_objmgr_for_each_vdev_peer(vdev, peer) {
		wlan_objmgr_peer_get_ref(peer, WLAN_DP_ID);
		/*If vote node is present then no active connection*/
		priv_ctx = dp_get_peer_priv_obj(peer);
		if (priv_ctx->vote_node) {
			dp_info("vote_node found for vdev_id:%u migration prev:%u new:%u",
				wlan_vdev_get_id(vdev), old_mac_id, new_mac_id);
			vote_node = priv_ctx->vote_node;
			if (vote_node->mac_id != old_mac_id) {
				dp_err("old macid is not matching to vote macid old:%u new:%u vote_mac:%u",
				       old_mac_id, new_mac_id, vote_node->mac_id);
			}

			/*
			 * Removing from mac list based on old mac_id.
			 * Adding to mac list based on new mac_id.
			 */
			qdf_list_remove_node(&rsrc_ctx->mac_list[vote_node->mac_id],
					     &vote_node->node);
			vote_node->mac_id = new_mac_id;
			wlan_dp_resource_mgr_list_insert_vote_node(
					&rsrc_ctx->mac_list[vote_node->mac_id],
					vote_node, opmode);
			if (!list_update)
				list_update = true;
		}

		wlan_objmgr_peer_release_ref(peer, WLAN_DP_ID);
	}

	if (list_update)
		wlan_dp_resource_mgr_select_max_phymodes(rsrc_ctx);

	qdf_spin_unlock_bh(&rsrc_ctx->rsrc_mgr_lock);
}

static void
wlan_dp_resource_mgr_list_attach(struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	int i;

	qdf_list_create(&rsrc_ctx->mac_n_list, 0);
	qdf_list_create(&rsrc_ctx->nan_list, 0);
	for (i = 0; i < MAX_MAC_RESOURCES; i++)
		qdf_list_create(&rsrc_ctx->mac_list[i], 0);
}

static void
wlan_dp_resource_mgr_list_detach(struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	int i;

	qdf_list_destroy(&rsrc_ctx->mac_n_list);
	qdf_list_destroy(&rsrc_ctx->nan_list);
	for (i = 0; i < MAX_MAC_RESOURCES; i++)
		qdf_list_destroy(&rsrc_ctx->mac_list[i]);
}

void
wlan_dp_resource_mgr_notify_vdev_creation(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_vdev *vdev)
{
	if (!rsrc_ctx) {
		dp_info("DP resource mgr context not present");
		return;
	}

	if (wlan_vdev_mlme_get_opmode(vdev) != QDF_MONITOR_MODE)
		return;

	rsrc_ctx->monitor_vdev_vote = true;
	wlan_dp_resource_mgr_select_resource_level(rsrc_ctx);
}

void
wlan_dp_resource_mgr_notify_vdev_deletion(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_vdev *vdev)
{
	if (!rsrc_ctx) {
		dp_info("DP resource mgr context not present");
		return;
	}

	if (wlan_vdev_mlme_get_opmode(vdev) != QDF_MONITOR_MODE)
		return;

	rsrc_ctx->monitor_vdev_vote = false;
	wlan_dp_resource_mgr_select_resource_level(rsrc_ctx);
}

void
wlan_dp_resource_mgr_set_req_resources(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
	enum wlan_dp_resource_level rsrc_level;
	uint64_t req_rx_buff_descs;
	QDF_STATUS status;

	if (!rsrc_ctx) {
		dp_info("DP resource mgr context not present");
		return;
	}

	rsrc_level = rsrc_ctx->cur_resource_level;
	req_rx_buff_descs = rsrc_ctx->cur_rsrc_map[rsrc_level].num_rx_buffers;

	status = cdp_set_req_buff_descs(rsrc_ctx->dp_ctx->cdp_soc,
					req_rx_buff_descs, 0);
	if (QDF_IS_STATUS_ERROR(status)) {
		wlan_dp_resource_mgr_detach(rsrc_ctx->dp_ctx);
		dp_err("Unable to set req DP rx desc");
	}
	dp_rsrc_mgr_debug("req_rx_buffer_desc:%u set", req_rx_buff_descs);
}

void wlan_dp_resource_mgr_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_resource_mgr_ctx *rsrc_ctx;
	struct cdp_soc_t *cdp_soc = dp_ctx->cdp_soc;
	cdp_config_param_type val = {0};
	QDF_STATUS status;
	int i;

	if (!cfg_get(dp_ctx->psoc, CFG_DP_DYNAMIC_RESOURCE_MGMT_ENABLE)) {
		dp_info("DP dynamic resourec mgmt cfg disabled");
		return;
	}

	rsrc_ctx = qdf_mem_malloc(sizeof(*rsrc_ctx));
	if (!rsrc_ctx) {
		dp_err("Failed to create DP resource mgr context");
		return;
	}
	qdf_mem_copy(rsrc_ctx->cur_rsrc_map, dp_resource_map,
		     (sizeof(struct wlan_dp_resource_map) * RESOURCE_LVL_MAX));

	status = cdp_txrx_get_psoc_param(cdp_soc,
					 CDP_CFG_RXDMA_REFILL_RING_SIZE, &val);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Failed to fetch Refill ring config status:%u", status);
		goto attach_err;
	}

	/*
	 * Config value is less than default resource map, then
	 * do not enable resource mgr, since resource mgr cannot
	 * operated on less than default resources.
	 * If config value greater than default resources, than
	 * use that value for max reource level selection.
	 */
	if (val.cdp_rxdma_refill_ring_size <=
	    dp_resource_map[RESOURCE_LVL_1].num_rx_buffers) {
		dp_info("Unsupported RX buffers config:%u, disabling rsrc mgr",
			val.cdp_rxdma_refill_ring_size);
		goto attach_err;
	} else if ((val.cdp_rxdma_refill_ring_size - 1) !=
		   dp_resource_map[RESOURCE_LVL_2].num_rx_buffers) {
		dp_info("Adjusting Resource lvl_2 rx buffers map_val:%u cfg_val:%u",
			dp_resource_map[RESOURCE_LVL_2].num_rx_buffers,
			val.cdp_rxdma_refill_ring_size);
		rsrc_ctx->cur_rsrc_map[RESOURCE_LVL_2].num_rx_buffers =
			(val.cdp_rxdma_refill_ring_size - 1);
	}

	for (i = 0; i < RESOURCE_LVL_MAX; i++) {
		dp_info("DP resource level:%u MAX_TPUT:%u NUM_RX_BUF:%u",
			i, rsrc_ctx->cur_rsrc_map[i].max_tput,
			rsrc_ctx->cur_rsrc_map[i].num_rx_buffers);
	}

	status = wlan_objmgr_register_peer_phymode_change_notify_handler(
					WLAN_COMP_DP,
					wlan_dp_resource_mgr_phymode_update,
					rsrc_ctx);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Failed to register for peer phymode change notif");
		goto attach_err;
	}

	qdf_spinlock_create(&rsrc_ctx->rsrc_mgr_lock);
	wlan_dp_resource_mgr_list_attach(rsrc_ctx);

	rsrc_ctx->mac_count = MAX_MAC_RESOURCES;

	qdf_timer_init(NULL, &rsrc_ctx->timer,
		       wlan_dp_resource_mgr_timer_handler,
		       rsrc_ctx, QDF_TIMER_TYPE_SW);

	rsrc_ctx->dp_ctx = dp_ctx;
	dp_ctx->rsrc_mgr_ctx = rsrc_ctx;

	dp_info("DP resource mgr attach done mac_count:%u",
		rsrc_ctx->mac_count);
	return;

attach_err:
	dp_err("DP resource mgr attach failure");
	qdf_mem_free(rsrc_ctx);
}

void wlan_dp_resource_mgr_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	QDF_STATUS status;

	dp_info("DP resource mgr detaching %s",
		dp_ctx->rsrc_mgr_ctx ? "true" : "false");
	if (dp_ctx->rsrc_mgr_ctx) {
		status =
		wlan_objmgr_unregister_peer_phymode_change_notify_handler(
					WLAN_COMP_DP,
					wlan_dp_resource_mgr_phymode_update,
					dp_ctx->rsrc_mgr_ctx);
		if (QDF_IS_STATUS_ERROR(status)) {
			dp_err("Failed to unregister peer phymode notification handler");
			return;
		}

		qdf_timer_free(&dp_ctx->rsrc_mgr_ctx->timer);
		wlan_dp_resource_mgr_list_detach(dp_ctx->rsrc_mgr_ctx);
		qdf_spinlock_destroy(&dp_ctx->rsrc_mgr_ctx->rsrc_mgr_lock);
		qdf_mem_free(dp_ctx->rsrc_mgr_ctx);
		dp_ctx->rsrc_mgr_ctx = NULL;
	}
}
