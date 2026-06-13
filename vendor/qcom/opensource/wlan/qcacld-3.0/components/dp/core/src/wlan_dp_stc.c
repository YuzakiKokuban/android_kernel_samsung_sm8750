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
 /**
  * DOC: wlan_dp_stc.c
  *
  *
  */

#include "wlan_dp_stc.h"
#include "wlan_dp_fisa_rx.h"
#include "target_if.h"
#include "dp_internal.h"
#include "wlan_dp_prealloc.h"

#ifdef WLAN_DP_FEATURE_STC

#define FLOW_TRACK_ELIGIBLE_THRESH_NS 3000000000
#define WLAN_DP_STC_PING_INACTIVE_TIMEOUT_NS 10000000000
#define FLOW_INACTIVE_TIME_THRESH_NS 20000000000
#define FLOW_RESUME_TIME_THRESH_NS 500000000
#define FLOW_SHORTLIST_PKT_RATE_PER_SEC_THRESH 15

/* Macros used by STC logmask */
#define WLAN_DP_STC_LOGMASK_FLOW_STATS BIT(0)
#define WLAN_DP_STC_LOGMASK_CLASSIFIED_FLOW_STATS BIT(1)

QDF_STATUS
wlan_dp_stc_track_flow_features(struct wlan_dp_stc *dp_stc, qdf_nbuf_t nbuf,
				struct wlan_dp_stc_flow_table_entry *flow_entry,
				uint8_t vdev_id, uint16_t peer_id,
				uint16_t pkt_len, uint32_t metadata)
{
	uint64_t curr_pkt_ts = dp_stc_get_timestamp();
	uint64_t pkt_iat = 0;
	uint16_t s, w;

	if (flow_entry->metadata != metadata) {
		/* Clear the flow entry */
		qdf_mem_zero(flow_entry, sizeof(*flow_entry));
		flow_entry->metadata = metadata;
	}

	/* TxRx Stats - Start */
	if (flow_entry->prev_pkt_arrival_ts) {
		pkt_iat = curr_pkt_ts - flow_entry->prev_pkt_arrival_ts;
		DP_STC_UPDATE_MIN_MAX_SUM_STATS(flow_entry->txrx_stats.pkt_iat,
						pkt_iat);
	}

	flow_entry->txrx_stats.bytes += pkt_len;
	flow_entry->txrx_stats.pkts++;
	DP_STC_UPDATE_MIN_MAX_STATS(flow_entry->txrx_stats.pkt_size,
				    pkt_len);

	s = flow_entry->idx.s.sample_idx;
	w = flow_entry->idx.s.win_idx;
	if (s < DP_STC_TXRX_SAMPLES_MAX && w < DP_TXRX_SAMPLES_WINDOW_MAX) {
		struct wlan_dp_stc_txrx_min_max_stats *txrx_min_max_stats;

		/* Copy win-1 min-max to win-2 */
		if (w != 0 && !flow_entry->win_transition_complete) {
			struct wlan_dp_stc_txrx_min_max_stats *win_1;
			struct wlan_dp_stc_txrx_min_max_stats *win_2;

			win_1 = &flow_entry->txrx_min_max_stats[s][0];
			win_2 = &flow_entry->txrx_min_max_stats[s][w];

			win_2->pkt_size_min = win_1->pkt_size_min;
			win_2->pkt_size_max = win_1->pkt_size_max;
			win_2->pkt_iat_min = win_1->pkt_iat_min;
			win_2->pkt_iat_max = win_1->pkt_iat_max;
			flow_entry->win_transition_complete = 1;
		} else {
			flow_entry->win_transition_complete = 0;
		}

		txrx_min_max_stats = &flow_entry->txrx_min_max_stats[s][w];
		DP_STC_UPDATE_WIN_MIN_MAX_STATS(txrx_min_max_stats->pkt_size,
						pkt_len);
		DP_STC_UPDATE_WIN_MIN_MAX_STATS(txrx_min_max_stats->pkt_iat,
						pkt_iat);
	}
	/* TxRx Stats - End */

check_burst:
	/* Burst stats - START */
	switch (flow_entry->burst_state) {
	case BURST_DETECTION_INIT:
		/*
		 * We should move to BURST_DETECTION_START, since this piece of
		 * code will be executed only if the sampling is going on.
		 */
		flow_entry->burst_start_time = curr_pkt_ts;
		flow_entry->burst_state = BURST_DETECTION_START;
		fallthrough;
	case BURST_DETECTION_START:
	{
		uint32_t time_delta = curr_pkt_ts -
						flow_entry->burst_start_time;
		/*
		 * If time_delta < threshold then for the burst start detection
		 * current pkt bytes also should be considered.
		 */
		uint32_t bytes = flow_entry->burst_start_detect_bytes;

		if (time_delta < BURST_START_TIME_THRESHOLD_NS &&
		    (bytes + pkt_len) < BURST_START_BYTES_THRESHOLD) {
			/* BURST not started, but continue detection */
			flow_entry->burst_start_detect_bytes += pkt_len;
		} else if (time_delta > BURST_START_TIME_THRESHOLD_NS &&
			   bytes < BURST_START_BYTES_THRESHOLD) {
			/* Burst start failed, move to new burst detection */
			flow_entry->burst_state = BURST_DETECTION_INIT;
			flow_entry->burst_start_detect_bytes = 0;
			flow_entry->cur_burst_bytes = 0;
			goto check_burst;
		} else if (time_delta < BURST_START_TIME_THRESHOLD_NS &&
			   (bytes + pkt_len) > BURST_START_BYTES_THRESHOLD) {
			/* Valid burst start */
			flow_entry->burst_start_detect_bytes += pkt_len;
			flow_entry->burst_state = BURST_DETECTION_BURST_START;
			flow_entry->burst_stats.burst_count++;
			flow_entry->cur_burst_bytes +=
					flow_entry->burst_start_detect_bytes;
		} else {
			/*
			 * (time_delta > BURST_START_TIME_THRESHOLD_NS &&
			 *     bytes > BURST_START_BYTES_THRESHOLD)
			 */
			/* Burst start failed, move to new burst detection */
			flow_entry->burst_start_time = curr_pkt_ts;
			flow_entry->burst_state = BURST_DETECTION_START;
			flow_entry->burst_start_detect_bytes = pkt_len;
			flow_entry->cur_burst_bytes = 0;
		}
		break;
	}
	case BURST_DETECTION_BURST_START:
	{
		if (pkt_iat > BURST_END_TIME_THRESHOLD_NS) {
			struct wlan_dp_stc_burst_stats *burst_stats;
			uint32_t burst_dur, burst_size;

			flow_entry->burst_state = BURST_DETECTION_INIT;
			burst_stats = &flow_entry->burst_stats;

			/*
			 * Burst ended at the last packet, so calculate
			 * burst duration using the last pkt timestamp
			 */
			burst_dur = flow_entry->prev_pkt_arrival_ts -
						flow_entry->burst_start_time;
			burst_size = flow_entry->cur_burst_bytes;

			DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_duration,
							burst_dur);
			DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_size,
							burst_size);
			flow_entry->burst_start_time = curr_pkt_ts;
			flow_entry->burst_state = BURST_DETECTION_INIT;
			flow_entry->burst_start_detect_bytes = 0;
			flow_entry->cur_burst_bytes = 0;
			pkt_iat = 0;
			goto check_burst;
		}

		flow_entry->cur_burst_bytes += pkt_len;
		break;
	}
	default:
		break;
	}
	/* Burst stats - END */
	flow_entry->prev_pkt_arrival_ts = curr_pkt_ts;

	return QDF_STATUS_SUCCESS;
}

static inline int
wlan_dp_stc_get_valid_sampling_flow_count(struct wlan_dp_stc *dp_stc)
{
	return qdf_atomic_read(&dp_stc->sampling_flow_table->num_valid_entries);
}

static inline void
wlan_dp_stc_get_avail_flow_quota(struct wlan_dp_stc *dp_stc, uint8_t *bidi,
				 uint8_t *tx, uint8_t *rx)
{
	struct wlan_dp_stc_sampling_table *s_table;

	s_table = dp_stc->sampling_flow_table;

	*bidi = DP_STC_SAMPLE_BIDI_FLOW_MAX -
			qdf_atomic_read(&s_table->num_bidi_flows);
	*tx = DP_STC_SAMPLE_TX_FLOW_MAX -
			qdf_atomic_read(&s_table->num_tx_only_flows);
	*rx = DP_STC_SAMPLE_RX_FLOW_MAX -
			qdf_atomic_read(&s_table->num_rx_only_flows);
}

static inline void
wlan_dp_stc_fill_rx_flow_candidate(struct wlan_dp_stc *dp_stc,
				   struct wlan_dp_stc_sampling_candidate *candidate,
				   uint16_t rx_flow_id,
				   uint32_t rx_flow_metadata)
{
	candidate->rx_flow_id = rx_flow_id;
	candidate->rx_flow_metadata = rx_flow_metadata;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_VALID;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_RX_FLOW_VALID;
}

static inline struct dp_fisa_rx_sw_ft *
wlan_dp_get_rx_flow_hdl(struct wlan_dp_psoc_context *dp_ctx, uint8_t flow_id)
{
	struct dp_rx_fst *fisa_hdl = dp_ctx->rx_fst;

	return (&(((struct dp_fisa_rx_sw_ft *)fisa_hdl->base)[flow_id]));
}

static inline struct wlan_dp_spm_flow_info *
wlan_dp_get_tx_flow_hdl(struct wlan_dp_psoc_context *dp_ctx, uint8_t flow_id)
{
	if (!dp_ctx->gl_flow_recs)
		return NULL;

	return &dp_ctx->gl_flow_recs[flow_id];
}

static inline void
wlan_dp_stc_fill_bidi_flow_candidate(struct wlan_dp_stc *dp_stc,
				     struct wlan_dp_stc_sampling_candidate *candidate,
				     uint16_t tx_flow_id,
				     uint32_t tx_flow_metadata,
				     uint16_t rx_flow_id,
				     uint32_t rx_flow_metadata)
{
	candidate->tx_flow_id = tx_flow_id;
	candidate->tx_flow_metadata = tx_flow_metadata;
	candidate->rx_flow_id = rx_flow_id;
	candidate->rx_flow_metadata = rx_flow_metadata;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_VALID;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_TX_FLOW_VALID;
	candidate->flags |= WLAN_DP_SAMPLING_CANDIDATE_RX_FLOW_VALID;
}

static inline void
wlan_dp_move_candidate_to_sample_table(struct wlan_dp_stc *dp_stc,
				       struct wlan_dp_stc_sampling_candidate *candidate,
				       struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_stc_sampling_table *s_table =
					dp_stc->sampling_flow_table;

	qdf_atomic_inc(&dp_stc->sampling_flow_table->num_valid_entries);
	s_entry->dir = candidate->dir;
	switch (candidate->dir) {
	case WLAN_DP_FLOW_DIR_TX:
		qdf_atomic_inc(&s_table->num_tx_only_flows);
		break;
	case WLAN_DP_FLOW_DIR_RX:
		qdf_atomic_inc(&s_table->num_rx_only_flows);
		break;
	case WLAN_DP_FLOW_DIR_BIDI:
		qdf_atomic_inc(&s_table->num_bidi_flows);
		break;
	default:
		break;
	}

	s_entry->peer_id = candidate->peer_id;
	if (candidate->flags & WLAN_DP_SAMPLING_CANDIDATE_TX_FLOW_VALID) {
		s_entry->flags |= WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID;
		s_entry->tx_flow_id = candidate->tx_flow_id;
		s_entry->tx_flow_metadata = candidate->tx_flow_metadata;
	}

	if (candidate->flags & WLAN_DP_SAMPLING_CANDIDATE_RX_FLOW_VALID) {
		struct dp_fisa_rx_sw_ft *rx_flow;

		rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx,
						  candidate->rx_flow_id);
		s_entry->flags |= WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID;
		s_entry->rx_flow_id = candidate->rx_flow_id;
		s_entry->rx_flow_metadata = candidate->rx_flow_metadata;
		wlan_dp_stc_populate_flow_tuple(&s_entry->flow_samples.flow_tuple,
						&rx_flow->rx_flow_tuple_info);
		s_entry->tuple_hash = rx_flow->flow_tuple_hash;
	}

	s_entry->state = WLAN_DP_SAMPLING_STATE_FLOW_ADDED;
	candidate->flags = 0;
}

static inline struct wlan_dp_stc_sampling_table_entry *
wlan_dp_get_free_sampling_table_entry(struct wlan_dp_stc *dp_stc)
{
	int i;
	struct wlan_dp_stc_sampling_table *s_table =
						dp_stc->sampling_flow_table;

	for (i = 0; i < DP_STC_SAMPLE_FLOWS_MAX; i++) {
		if (s_table->entries[i].state == WLAN_DP_SAMPLING_STATE_INIT)
			return &s_table->entries[i];
	}

	return NULL;
}

static inline bool
wlan_dp_txrx_samples_ready(struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TXRX_SAMPLES_READY)
		return true;

	return false;
}

static inline bool
wlan_dp_txrx_samples_reported(struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	if (s_entry->flags1 & WLAN_DP_SAMPLING_FLAGS1_TXRX_SAMPLES_SENT)
		return true;

	return false;
}

static inline bool
wlan_dp_burst_samples_ready(struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_BURST_SAMPLES_READY)
		return true;

	return false;
}

static inline bool
wlan_dp_burst_samples_reported(struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	if (s_entry->flags1 & WLAN_DP_SAMPLING_FLAGS1_BURST_SAMPLES_SENT)
		return true;

	return false;
}

static inline QDF_STATUS
wlan_dp_stc_find_ul_flow(struct wlan_dp_stc *dp_stc, uint16_t rx_flow_id,
			 uint64_t rx_flow_tuple_hash, uint16_t *tx_flow_id,
			 uint64_t *tx_flow_metadata)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	uint16_t max_tx_flow = WLAN_DP_INTF_MAX *
				WLAN_DP_SPM_FLOW_REC_TBL_MAX;
	uint16_t flow_id;

	if (!dp_ctx->gl_flow_recs)
		return QDF_STATUS_E_INVAL;

	for (flow_id = 0; flow_id < max_tx_flow; flow_id++) {
		struct wlan_dp_spm_flow_info *flow;

		flow = wlan_dp_get_tx_flow_hdl(dp_ctx, flow_id);
		if (!flow->is_populated || flow->selected_to_sample ||
		    flow->classified)
			continue;

		if (flow->flow_tuple_hash != rx_flow_tuple_hash)
			continue;

		dp_info("STC: Found Bi-Di flow tx %d (mdata 0x%x) rx %d",
			flow_id, flow->guid, rx_flow_id);
		*tx_flow_id = flow_id;
		*tx_flow_metadata = flow->guid;
		return QDF_STATUS_SUCCESS;
	}

	return QDF_STATUS_E_INVAL;
}

static inline QDF_STATUS
wlan_dp_send_txrx_sample(struct wlan_dp_stc *dp_stc,
			 struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_objmgr_psoc *psoc = dp_ctx->psoc;
	uint32_t flags = WLAN_DP_TXRX_SAMPLES_READY;

	if (dp_stc->logmask & WLAN_DP_STC_LOGMASK_FLOW_STATS)
		flags |= WLAN_DP_LOG_ENABLE;

	if (dp_ctx->dp_ops.send_flow_stats_event)
		return dp_ctx->dp_ops.send_flow_stats_event(psoc,
						&s_entry->flow_samples, flags);

	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
wlan_dp_send_flow_report(struct wlan_dp_stc *dp_stc,
			 struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_objmgr_psoc *psoc = dp_ctx->psoc;
	uint32_t flags = 0;

	dp_info("STC: Send NL msg flow report: %d",
		dp_stc->send_classified_flow_stats);
	if (!dp_stc->send_classified_flow_stats)
		return QDF_STATUS_SUCCESS;

	if (wlan_dp_txrx_samples_ready(s_entry))
		flags |= WLAN_DP_TXRX_SAMPLES_READY;
	if (wlan_dp_burst_samples_ready(s_entry))
		flags |= WLAN_DP_BURST_SAMPLES_READY;

	if (dp_stc->logmask & WLAN_DP_STC_LOGMASK_CLASSIFIED_FLOW_STATS)
		flags |= WLAN_DP_LOG_ENABLE;

	if (dp_ctx->dp_ops.send_flow_report_event)
		return dp_ctx->dp_ops.send_flow_report_event(psoc,
						&s_entry->flow_samples, flags);

	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
wlan_dp_send_burst_sample(struct wlan_dp_stc *dp_stc,
			  struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_objmgr_psoc *psoc = dp_ctx->psoc;
	uint32_t flags = WLAN_DP_BURST_SAMPLES_READY;

	if (dp_stc->logmask & WLAN_DP_STC_LOGMASK_FLOW_STATS)
		flags |= WLAN_DP_LOG_ENABLE;

	if (dp_ctx->dp_ops.send_flow_stats_event)
		return dp_ctx->dp_ops.send_flow_stats_event(psoc,
						&s_entry->flow_samples, flags);

	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
wlan_dp_stc_remove_sampling_table_entry(struct wlan_dp_stc *dp_stc,
					struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_stc_sampling_table *s_table =
					dp_stc->sampling_flow_table;
	struct dp_fisa_rx_sw_ft *rx_flow;

	if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
		struct wlan_dp_spm_flow_info *tx_flow;
		uint16_t tx_flow_id = s_entry->tx_flow_id;

		tx_flow = wlan_dp_get_tx_flow_hdl(dp_ctx, tx_flow_id);
		if (tx_flow->guid == s_entry->tx_flow_metadata)
			tx_flow->track_flow_stats = 0;
	}

	if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
		uint16_t flow_id = s_entry->rx_flow_id;

		rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx, flow_id);
		if (s_entry->rx_flow_metadata == rx_flow->metadata)
			rx_flow->track_flow_stats = 0;
	}

	switch (s_entry->dir) {
	case WLAN_DP_FLOW_DIR_TX:
		qdf_atomic_dec(&s_table->num_tx_only_flows);
		break;
	case WLAN_DP_FLOW_DIR_RX:
		qdf_atomic_dec(&s_table->num_rx_only_flows);
		break;
	case WLAN_DP_FLOW_DIR_BIDI:
		qdf_atomic_dec(&s_table->num_bidi_flows);
		break;
	default:
		break;
	}

	qdf_atomic_dec(&dp_stc->sampling_flow_table->num_valid_entries);
	qdf_mem_zero(s_entry, sizeof(*s_entry));
	s_entry->state = WLAN_DP_SAMPLING_STATE_INIT;

	return QDF_STATUS_SUCCESS;
}

static inline void
wlan_dp_stc_check_ping_activity(struct wlan_dp_stc *dp_stc,
				uint16_t peer_id)
{
	struct wlan_dp_stc_peer_traffic_context *peer_tc;
	uint64_t cur_ts = dp_stc_get_timestamp();

	peer_tc = &dp_stc->peer_tc[peer_id];
	if (!peer_tc->valid)
		return;

	if ((qdf_atomic_read(&peer_tc->active_ping) == 1) &&
	    (cur_ts - peer_tc->last_ping_ts >
				WLAN_DP_STC_PING_INACTIVE_TIMEOUT_NS)) {
		peer_tc->last_ping_ts = 0;
		qdf_atomic_set(&peer_tc->active_ping, 0);
		qdf_atomic_set(&peer_tc->send_fw_ind, 1);
	}
}

static inline void
wlan_dp_stc_purge_classified_flow(struct wlan_dp_stc *dp_stc,
				  struct wlan_dp_stc_classified_flow_entry *c_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_stc_peer_traffic_context *peer_tc;
	struct wlan_dp_spm_flow_info *tx_flow;
	struct dp_fisa_rx_sw_ft *rx_flow;

	if (qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_DEL_FLAGS_TX_DEL,
				&c_entry->del_flags) &&
	    qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_FLAGS_RX_FLOW_VALID,
				&c_entry->flags)) {
		/* TX flow retired, mark corresponding rx flow as inactive */
		rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx, c_entry->rx_flow_id);
		rx_flow->classified = 0;
		/*
		 * Classified flow entry is cleared at the end,
		 * so no need to clear the del_flags
		 */
	}

	if (qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_DEL_FLAGS_RX_DEL,
				&c_entry->del_flags) &&
	    qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_FLAGS_TX_FLOW_VALID,
				&c_entry->flags)) {
		/* RX flow retired, mark corresponding tx flow as inactive */
		tx_flow = wlan_dp_get_tx_flow_hdl(dp_ctx, c_entry->tx_flow_id);
		tx_flow->classified = 0;
		/*
		 * Classified flow entry is cleared at the end,
		 * so no need to clear the del_flags
		 */
	}

	peer_tc = &dp_stc->peer_tc[c_entry->peer_id];
	if (peer_tc->valid && c_entry->flow_active)
		wlan_dp_stc_dec_traffic_type(peer_tc,
					     c_entry->traffic_type);

	qdf_mem_zero(c_entry, sizeof(*c_entry));
	qdf_atomic_set(&c_entry->state, WLAN_DP_STC_CLASSIFIED_FLOW_STATE_INIT);
}

static inline void
wlan_dp_stc_check_and_retire_flow(struct wlan_dp_stc *dp_stc,
				  struct wlan_dp_stc_classified_flow_entry *c_entry)
{
	if (qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_DEL_FLAGS_TX_DEL,
				&c_entry->del_flags) ||
	    qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_DEL_FLAGS_RX_DEL,
				&c_entry->del_flags)) {
		/* trigger a detach of this c_entry */
		wlan_dp_stc_purge_classified_flow(dp_stc, c_entry);
	}
}

static inline void
wlan_dp_stc_check_flow_inactivity(struct wlan_dp_stc *dp_stc,
				  struct wlan_dp_stc_classified_flow_entry *c_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_stc_peer_traffic_context *peer_tc;
	struct wlan_dp_spm_flow_info *tx_flow;
	struct dp_fisa_rx_sw_ft *rx_flow;
	uint64_t cur_ts = dp_stc_get_timestamp();
	int tx_flow_valid, rx_flow_valid;

	tx_flow_valid = qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_FLAGS_TX_FLOW_VALID,
					    &c_entry->flags);
	rx_flow_valid = qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_FLAGS_RX_FLOW_VALID,
					    &c_entry->flags);

	if (tx_flow_valid) {
		uint64_t flow_active_ts;

		tx_flow = wlan_dp_get_tx_flow_hdl(dp_ctx, c_entry->tx_flow_id);
		flow_active_ts = tx_flow->active_ts;
		cur_ts = dp_stc_get_timestamp();
		if (cur_ts - flow_active_ts > FLOW_INACTIVE_TIME_THRESH_NS)
			goto flow_inactive;
	}

	if (rx_flow_valid) {
		uint64_t flow_active_ts;

		rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx, c_entry->rx_flow_id);
		flow_active_ts = rx_flow->last_accessed_ts;
		cur_ts = dp_stc_get_timestamp();
		if (cur_ts - flow_active_ts > FLOW_INACTIVE_TIME_THRESH_NS)
			goto flow_inactive;
	}

	return;

flow_inactive:
	/*
	 * Set inactive flag for the flow, but then how will it be added back
	 * when the same flow starts again ?
	 * How about we just keep it in the classified flow table, and move the
	 * state to tracking inactive flows (which are still in the table)
	 */
	/*
	 * 1) Mark the flow as inactive
	 * 2) Update active traffic map
	 * 3) Send wmi command
	 * 4) Remove from classified flow table ??
	 */
	c_entry->flow_active = 0;

	peer_tc = &dp_stc->peer_tc[c_entry->peer_id];
	if (!peer_tc->valid)
		return;

	wlan_dp_stc_dec_traffic_type(peer_tc, c_entry->traffic_type);
}

static inline void
wlan_dp_stc_check_flow_resumption(struct wlan_dp_stc *dp_stc,
				  struct wlan_dp_stc_classified_flow_entry *c_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_stc_peer_traffic_context *peer_tc;
	struct wlan_dp_spm_flow_info *tx_flow;
	struct dp_fisa_rx_sw_ft *rx_flow;
	uint64_t cur_ts = dp_stc_get_timestamp();
	int tx_flow_valid, rx_flow_valid;

	tx_flow_valid = qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_FLAGS_TX_FLOW_VALID,
					    &c_entry->flags);
	rx_flow_valid = qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_FLAGS_RX_FLOW_VALID,
					    &c_entry->flags);

	if (tx_flow_valid) {
		tx_flow = wlan_dp_get_tx_flow_hdl(dp_ctx, c_entry->tx_flow_id);
		if (cur_ts - tx_flow->active_ts < FLOW_RESUME_TIME_THRESH_NS)
			goto flow_active;
	}

	if (rx_flow_valid) {
		rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx, c_entry->rx_flow_id);
		if (cur_ts - rx_flow->last_accessed_ts <
						FLOW_RESUME_TIME_THRESH_NS)
			goto flow_active;
	}

	return;

flow_active:
	/*
	 * Set inactive flag for the flow, but then how will it be added back
	 * when the same flow starts again ?
	 * How about we just keep it in the classified flow table, and move the
	 * state to tracking inactive flows (which are still in the table)
	 */
	/*
	 * 1) Mark the flow as inactive
	 * 2) Update active traffic map
	 * 3) Send wmi command
	 * 4) Remove from classified flow table ??
	 */
	c_entry->flow_active = 1;

	peer_tc = &dp_stc->peer_tc[c_entry->peer_id];
	if (!peer_tc->valid)
		return;

	wlan_dp_stc_inc_traffic_type(peer_tc, c_entry->traffic_type);
}

static inline void
wlan_dp_stc_process_add_classified_flow(struct wlan_dp_stc *dp_stc,
					struct wlan_dp_stc_classified_flow_entry *c_entry)
{
	struct wlan_dp_stc_peer_traffic_context *peer_tc;
	enum qca_traffic_type traffic_type = c_entry->traffic_type;
	uint16_t peer_id = c_entry->peer_id;

	peer_tc = &dp_stc->peer_tc[peer_id];
	if (!peer_tc->valid)
		return;

	wlan_dp_stc_inc_traffic_type(peer_tc, traffic_type);
}

static inline QDF_STATUS
wlan_dp_stc_send_active_traffic_map_ind(struct wlan_dp_stc *dp_stc,
					uint16_t peer_id)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_psoc_sb_ops *sb_ops = &dp_ctx->sb_ops;
	struct wlan_dp_stc_peer_traffic_context *peer_tc;
	struct dp_active_traffic_map_params req_buf;
	uint32_t wmi_active_traffic_map = 0;
	QDF_STATUS status;

	peer_tc = &dp_stc->peer_tc[peer_id];
	if (!peer_tc->valid)
		return QDF_STATUS_SUCCESS;

	if (qdf_atomic_read(&peer_tc->send_fw_ind) == 0)
		return QDF_STATUS_SUCCESS;

	qdf_atomic_set(&peer_tc->send_fw_ind, 0);

	if (qdf_atomic_read(&peer_tc->num_streaming))
		wmi_active_traffic_map |=
				WMI_PEER_ACTIVE_TRAFFIC_TYPE_STREAMING_M;
	if (qdf_atomic_read(&peer_tc->active_ping))
		wmi_active_traffic_map |= WMI_PEER_ACTIVE_TRAFFIC_TYPE_PING_M;
	if (qdf_atomic_read(&peer_tc->active_bk_traffic))
		wmi_active_traffic_map |=
				WMI_PEER_ACTIVE_TRAFFIC_TYPE_BACKGROUND_M;

	if (qdf_atomic_read(&peer_tc->num_gaming))
		wmi_active_traffic_map |= WMI_PEER_ACTIVE_TRAFFIC_TYPE_GAMING_M;
	if (qdf_atomic_read(&peer_tc->num_voice_call))
		wmi_active_traffic_map |= WMI_PEER_ACTIVE_TRAFFIC_TYPE_VOIP_M;
	if (qdf_atomic_read(&peer_tc->num_video_call))
		wmi_active_traffic_map |=
				WMI_PEER_ACTIVE_TRAFFIC_TYPE_VIDEO_CONF_M;

	req_buf.vdev_id = peer_tc->vdev_id;
	qdf_mem_copy(&req_buf.mac.bytes,
		     peer_tc->mac_addr.bytes,
		     QDF_MAC_ADDR_SIZE);
	req_buf.active_traffic_map = wmi_active_traffic_map;
	status = sb_ops->dp_send_active_traffic_map(dp_ctx->psoc,
						    &req_buf);
	if (!QDF_IS_STATUS_SUCCESS(status)) {
		dp_err("STC: Active traffic map ind failed for peer %d",
		       peer_id);
		return status;
	}

	return status;
}

static inline void
wlan_dp_stc_process_classified_flow(struct wlan_dp_stc *dp_stc,
				    struct wlan_dp_stc_classified_flow_entry *c_entry)
{
	uint32_t state;

	state = qdf_atomic_read(&c_entry->state);

	switch (state) {
	case WLAN_DP_STC_CLASSIFIED_FLOW_STATE_INIT:
		/* Nothing to check in this entry, return */
		return;
	case WLAN_DP_STC_CLASSIFIED_FLOW_STATE_ADDED:
		/*
		 * 1) Check if the flow flags indicate flow retirement
		 */
		wlan_dp_stc_check_and_retire_flow(dp_stc, c_entry);

		/*
		 * 1) Monitor the activity of the flow
		 */
		if (c_entry->flow_active)
			wlan_dp_stc_check_flow_inactivity(dp_stc, c_entry);
		else
			wlan_dp_stc_check_flow_resumption(dp_stc, c_entry);
		break;
	default:
		break;
	}
}

static inline bool
wlan_dp_stc_is_traffic_type_known(enum qca_traffic_type traffic_type)
{
	if (traffic_type == QCA_TRAFFIC_TYPE_UNKNOWN ||
	    traffic_type == QCA_TRAFFIC_TYPE_INVALID)
		return false;

	return true;
}

/*
 * This function should just mark something in the sampling entry.
 * The periodic work can take care of moving this entry to classified table
 */
static void
wlan_dp_stc_move_to_classified_table(struct wlan_dp_stc *dp_stc,
				     struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct wlan_dp_stc_classified_flow_table *c_table =
						dp_stc->classified_flow_table;
	struct wlan_dp_stc_classified_flow_entry *c_entry;
	struct wlan_dp_spm_flow_info *tx_flow;
	struct dp_fisa_rx_sw_ft *rx_flow;
	uint16_t c_id;

	/*
	 * 1) Move sampling flow to classified flow table
	 * 2) Mark STATE = ADDED
	 * 3) Mark classify result in Tx/Rx flows
	 * 4) Mark classified flow table IDX in Tx/Rx flows
	 */
	if (qdf_atomic_read(&c_table->num_valid_entries) ==
					DP_STC_CLASSIFIED_TABLE_FLOW_MAX) {
		/* No free space in classified flow table */
		dp_info("STC: No space available in classified flow table");
		return;
	}

	/* Should this indication be done from flow classify handler ? */
	if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
		tx_flow = wlan_dp_get_tx_flow_hdl(dp_ctx, s_entry->tx_flow_id);
		tx_flow->classified = 1;
		tx_flow->selected_to_sample = 0;
	}

	if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
		rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx, s_entry->rx_flow_id);
		rx_flow->classified = 1;
		rx_flow->selected_to_sample = 0;
	}

	if (!wlan_dp_stc_is_traffic_type_known(s_entry->traffic_type)) {
		wlan_dp_stc_remove_sampling_table_entry(dp_stc, s_entry);
		return;
	}

	/* Move to classified flow table if its not unknown traffic type */
	for (c_id = 0; c_id < DP_STC_CLASSIFIED_TABLE_FLOW_MAX; c_id++) {
		uint32_t state;

		c_entry = &c_table->entries[c_id];
		state = qdf_atomic_read(&c_entry->state);
		if (state > WLAN_DP_STC_CLASSIFIED_FLOW_STATE_INIT)
			continue;

		dp_info("STC: Move flow to classified flow %d for peer %d",
			c_id, s_entry->peer_id);
		/* Got a free entry */
		qdf_atomic_set(&c_entry->state,
			       WLAN_DP_STC_CLASSIFIED_FLOW_STATE_ADDED);
		c_entry->traffic_type = s_entry->traffic_type;
		c_entry->peer_id = s_entry->peer_id;
		qdf_atomic_inc(&c_table->num_valid_entries);

		if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
			c_entry->tx_flow_id = s_entry->tx_flow_id;
			qdf_atomic_set_bit(WLAN_DP_CLASSIFIED_FLAGS_TX_FLOW_VALID,
					   &c_entry->flags);
			tx_flow->classified = 1;
			tx_flow->c_flow_id = c_id;
		}

		if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
			c_entry->rx_flow_id = s_entry->rx_flow_id;
			qdf_atomic_set_bit(WLAN_DP_CLASSIFIED_FLAGS_RX_FLOW_VALID,
					   &c_entry->flags);
			rx_flow->classified = 1;
			rx_flow->c_flow_id = c_id;
		}

		c_entry->flow_active = 1;
		wlan_dp_stc_remove_sampling_table_entry(dp_stc, s_entry);
		wlan_dp_stc_process_add_classified_flow(dp_stc, c_entry);
		break;
	}
}

static void wlan_dp_stc_flow_monitor_work_handler(void *arg)
{
	struct wlan_dp_stc *dp_stc = (struct wlan_dp_stc *)arg;
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;
	struct dp_rx_fst *fst = dp_ctx->rx_fst;
	uint32_t candidate_idx = 0;
	uint8_t bidi, tx, rx;
	uint16_t rx_flow_id, tx_flow_id, flow_id;
	uint64_t rx_flow_tuple_hash;
	struct dp_fisa_rx_sw_ft *rx_flow;
	struct wlan_dp_stc_sampling_candidate *candidates =
							dp_stc->candidates;
	struct wlan_dp_stc_sampling_table_entry *s_entry;
	struct wlan_dp_stc_classified_flow_table *c_table =
						dp_stc->classified_flow_table;
	uint64_t cur_ts = dp_stc_get_timestamp();
	uint64_t tx_flow_metadata;
	QDF_STATUS status;
	int i;
	uint16_t peer_id;
	bool start_timer = false, candidate_selected = false;

	/*
	 * 1) Monitor TX/RX flows
	 * 2) Add long flow to Sampling flow table
	 */

	if (wlan_dp_stc_get_valid_sampling_flow_count(dp_stc) ==
						DP_STC_SAMPLE_FLOWS_MAX)
		goto other_checks;

	wlan_dp_stc_get_avail_flow_quota(dp_stc, &bidi, &tx, &rx);

	for (rx_flow_id = 0; rx_flow_id < fst->max_entries; rx_flow_id++) {
		uint64_t pkt_rate;

		if (!rx && !bidi)
			break;
		/* loop through entire FISA table */
		rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx, rx_flow_id);
		if (!rx_flow->is_populated ||
		    rx_flow->selected_to_sample ||
		    rx_flow->classified)
			continue;

		if (cur_ts - rx_flow->flow_init_ts <
						FLOW_TRACK_ELIGIBLE_THRESH_NS)
			continue;

		pkt_rate = (rx_flow->num_pkts * QDF_NSEC_PER_SEC) /
				(cur_ts - rx_flow->flow_init_ts);
		if (pkt_rate < FLOW_SHORTLIST_PKT_RATE_PER_SEC_THRESH)
			continue;

		rx_flow_tuple_hash = wlan_dp_fisa_get_flow_tuple_hash(rx_flow);
		/*
		 * This function should search and mark the flow id in
		 * tx_flow for cross referencing
		 */
		status = wlan_dp_stc_find_ul_flow(dp_stc,
						  rx_flow_id,
						  rx_flow_tuple_hash,
						  &tx_flow_id,
						  &tx_flow_metadata);
		if (QDF_IS_STATUS_ERROR(status)) {
			if (rx) {
				/* This is a RX only flow. */
				wlan_dp_stc_fill_rx_flow_candidate(dp_stc,
						&candidates[candidate_idx],
						rx_flow_id,
						rx_flow->metadata);
				candidates[candidate_idx].peer_id =
							rx_flow->peer_id;
				candidates[candidate_idx].dir = WLAN_DP_FLOW_DIR_RX;
				candidate_idx++;
				rx--;
				candidate_selected = true;
			}
			continue;
		}

		if (bidi) {
			wlan_dp_stc_fill_bidi_flow_candidate(dp_stc,
					&candidates[candidate_idx],
					tx_flow_id, tx_flow_metadata,
					rx_flow_id, rx_flow->metadata);
			candidates[candidate_idx].peer_id = rx_flow->peer_id;
			candidates[candidate_idx].dir = WLAN_DP_FLOW_DIR_BIDI;
			candidate_idx++;
			bidi--;
			candidate_selected = true;
			continue;
		}
	}

	if (!candidate_selected)
		goto other_checks;

	for (i = 0; i < DP_STC_SAMPLE_FLOWS_MAX; i++) {
		if (!(candidates[i].flags & WLAN_DP_SAMPLING_CANDIDATE_VALID))
			continue;

		/* Optimize this function to return array of pointers */
		s_entry = wlan_dp_get_free_sampling_table_entry(dp_stc);
		if (!s_entry) {
			/*
			 * Quota is present but entry is not free.
			 * This cannot happen
			 */
			qdf_assert_always(0);
			break;
		}

		wlan_dp_move_candidate_to_sample_table(dp_stc, &candidates[i],
						       s_entry);
		/*
		 * Set the indication in tx & rx flows,
		 * for us to not shortlist them again.
		 */
		if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
			struct wlan_dp_spm_flow_info *tx_flow;
			uint16_t tx_flow_id = s_entry->tx_flow_id;

			tx_flow = wlan_dp_get_tx_flow_hdl(dp_ctx, tx_flow_id);
			tx_flow->selected_to_sample = 1;
		}

		if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
			uint16_t rx_flow_id = s_entry->rx_flow_id;

			rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx, rx_flow_id);
			rx_flow->selected_to_sample = 1;
		}

		start_timer = true;
	}

	if (start_timer &&
	    dp_stc->sample_timer_state < WLAN_DP_STC_TIMER_STARTED) {
		qdf_timer_mod(&dp_stc->flow_sampling_timer,
			      DP_STC_TIMER_THRESH_MS);
	}

other_checks:
	/* 3) Check if flow samples are ready to be sent to userspace */
	for (flow_id = 0; flow_id < DP_STC_SAMPLE_FLOWS_MAX; flow_id++) {
		s_entry = &dp_stc->sampling_flow_table->entries[flow_id];

		if (s_entry->state == WLAN_DP_SAMPLING_STATE_INIT)
			continue;

		if (s_entry->state == WLAN_DP_SAMPLING_STATE_SAMPLING_FAIL)
			wlan_dp_stc_remove_sampling_table_entry(dp_stc,
								s_entry);

		if (wlan_dp_txrx_samples_ready(s_entry) &&
		    !wlan_dp_txrx_samples_reported(s_entry)) {
			/* TXRX sample is ready to be sent */
			s_entry->flags1 |=
				WLAN_DP_SAMPLING_FLAGS1_TXRX_SAMPLES_SENT;
			wlan_dp_send_txrx_sample(dp_stc, s_entry);
			/*
			 * Set flag to indicate that the txrx samples
			 * have been reported
			 */
		}

		if (wlan_dp_burst_samples_ready(s_entry) &&
		    !wlan_dp_burst_samples_reported(s_entry)) {
			/* Burst samples are ready to be sent */
			s_entry->flags1 |=
				WLAN_DP_SAMPLING_FLAGS1_BURST_SAMPLES_SENT;
			wlan_dp_send_burst_sample(dp_stc, s_entry);
			s_entry->burst_stats_report_ts = cur_ts;
			/*
			 * Set flag to indicate that the burst sample
			 * has been reported
			 */
		}

		if (wlan_dp_burst_samples_reported(s_entry) &&
		    s_entry->burst_stats_report_ts &&
		    cur_ts - s_entry->burst_stats_report_ts >
						FLOW_CLASSIFY_WAIT_TIME_NS) {
			wlan_dp_stc_remove_sampling_table_entry(dp_stc,
								s_entry);
		}

		if (s_entry->state == WLAN_DP_SAMPLING_STATE_CLASSIFIED) {
			if (!(s_entry->flags1 &
			      WLAN_DP_SAMPLING_FLAGS1_FLOW_REPORT_SENT)) {
				wlan_dp_send_flow_report(dp_stc, s_entry);
				s_entry->flags1 |=
				       WLAN_DP_SAMPLING_FLAGS1_FLOW_REPORT_SENT;
			}
			wlan_dp_stc_move_to_classified_table(dp_stc, s_entry);
		}
	}

	if (!qdf_atomic_read(&c_table->num_valid_entries))
		goto skip_classified_table_check;

	for (flow_id = 0; flow_id < DP_STC_CLASSIFIED_TABLE_FLOW_MAX;
								flow_id++) {
		struct wlan_dp_stc_classified_flow_entry *c_entry;
		/*
		 * 1) Check if c_entry is valid
		 * 2) Check if state > ADDED
		 * 3) Check if stats is reported to collector
		 * 4) Check if flow is inactive
		 * 5) Process active traffic mapping
		 *		(based on flow classify result)
		 * 6) Check Ping TS
		 */
		c_entry = &c_table->entries[flow_id];

		wlan_dp_stc_process_classified_flow(dp_stc, c_entry);
	}

skip_classified_table_check:
	for (peer_id = 0; peer_id < DP_STC_MAX_PEERS; peer_id++) {
		/*
		 * 1) Check ping inactivity
		 */
		wlan_dp_stc_check_ping_activity(dp_stc, peer_id);

		/*
		 * 2) Send wmi command to FW if required
		 */
		wlan_dp_stc_send_active_traffic_map_ind(dp_stc, peer_id);
	}
}

static inline QDF_STATUS
wlan_dp_stc_trigger_sampling(struct wlan_dp_stc *dp_stc, uint16_t flow_id,
			     uint8_t track, enum qdf_proto_dir dir)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_stc->dp_ctx;

	switch (dir) {
	case QDF_TX:
	{
		struct wlan_dp_spm_flow_info *tx_flow;

		tx_flow = wlan_dp_get_tx_flow_hdl(dp_ctx, flow_id);
		tx_flow->track_flow_stats = track;
		break;
	}
	case QDF_RX:
	{
		struct dp_fisa_rx_sw_ft *rx_flow;

		rx_flow = wlan_dp_get_rx_flow_hdl(dp_ctx, flow_id);
		rx_flow->track_flow_stats = track;
		dp_info("STC: RX flow %d track %d", flow_id, track);
		break;
	}
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

static inline void
wlan_dp_stc_save_delta_stats(struct wlan_dp_stc_txrx_stats *dst,
			     struct wlan_dp_stc_txrx_stats *cur,
			     struct wlan_dp_stc_txrx_stats *ref)
{
	dst->bytes = cur->bytes - ref->bytes;
	dst->pkts = cur->pkts - ref->pkts;
	dst->pkt_iat_sum = cur->pkt_iat_sum - ref->pkt_iat_sum;
}

static inline void
wlan_dp_stc_save_min_max_state(struct wlan_dp_stc_txrx_stats *dst,
			       struct wlan_dp_stc_txrx_min_max_stats *src)
{
	dst->pkt_size_min = src->pkt_size_min;
	dst->pkt_size_max = src->pkt_size_max;
	dst->pkt_iat_min = src->pkt_iat_min;
	dst->pkt_iat_max = src->pkt_iat_max;
}

static inline void
wlan_dp_stc_save_burst_samples(struct wlan_dp_stc *dp_stc,
			       struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	struct wlan_dp_stc_flow_table_entry *flow;
	struct wlan_dp_stc_burst_samples *burst_sample;
	uint32_t burst_dur, burst_size;

	burst_sample = &s_entry->flow_samples.burst_sample;
	if (!(s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID))
		goto save_rx_flow_samples;

	flow = &dp_stc->tx_flow_table->entries[s_entry->tx_flow_id];
	qdf_mem_copy(&burst_sample->txrx_samples.tx, &flow->txrx_stats,
		     sizeof(burst_sample->txrx_samples.tx));
	qdf_mem_copy(&burst_sample->tx, &flow->burst_stats,
		     sizeof(burst_sample->tx));
	if (flow->burst_state == BURST_DETECTION_BURST_START) {
		struct wlan_dp_stc_burst_stats *burst_stats;

		burst_stats = &burst_sample->tx;

		/*
		 * Burst ended at the last packet, so calculate
		 * burst duration using the last pkt timestamp
		 */
		burst_dur = flow->prev_pkt_arrival_ts -
					flow->burst_start_time;
		burst_size = flow->cur_burst_bytes;

		DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_duration,
						burst_dur);
		DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_size,
						burst_size);
	}

save_rx_flow_samples:
	if (!(s_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID))
		return;

	flow = &dp_stc->rx_flow_table->entries[s_entry->rx_flow_id];
	qdf_mem_copy(&burst_sample->txrx_samples.rx, &flow->txrx_stats,
		     sizeof(burst_sample->txrx_samples.rx));
	burst_sample->txrx_samples.win_size = DP_STC_LONG_WINDOW_MS;
	qdf_mem_copy(&burst_sample->rx, &flow->burst_stats,
		     sizeof(burst_sample->rx));
	if (flow->burst_state == BURST_DETECTION_BURST_START) {
		struct wlan_dp_stc_burst_stats *burst_stats;

		burst_stats = &burst_sample->rx;

		/*
		 * Burst ended at the last packet, so calculate
		 * burst duration using the last pkt timestamp
		 */
		burst_dur = flow->prev_pkt_arrival_ts -
					flow->burst_start_time;
		burst_size = flow->cur_burst_bytes;

		DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_duration,
						burst_dur);
		DP_STC_UPDATE_MIN_MAX_SUM_STATS(burst_stats->burst_size,
						burst_size);

	}
}

static inline void
wlan_dp_stc_stop_flow_tracking(struct wlan_dp_stc *dp_stc,
			       struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	uint32_t flow_id;

	if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
		flow_id = s_entry->tx_flow_id;
		wlan_dp_stc_trigger_sampling(dp_stc, flow_id, 0, QDF_TX);
	}

	if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
		flow_id = s_entry->rx_flow_id;
		wlan_dp_stc_trigger_sampling(dp_stc, flow_id, 0, QDF_RX);
	}
}

static inline bool
wlan_dp_stc_sample_flow(struct wlan_dp_stc *dp_stc,
			struct wlan_dp_stc_sampling_table_entry *s_entry)
{
	struct wlan_dp_stc_txrx_stats tx_stats = {0};
	struct wlan_dp_stc_txrx_stats rx_stats = {0};
	struct wlan_dp_stc_txrx_samples *txrx_samples;
	struct wlan_dp_stc_flow_samples *flow_samples = &s_entry->flow_samples;
	uint8_t sample_idx, win_idx;
	bool sampling_pending = false;
	struct wlan_dp_stc_flow_table_entry *flow;
	uint32_t flow_id;

	switch (s_entry->state) {
	case WLAN_DP_SAMPLING_STATE_INIT:
		/* Skip this entry, since no flow is added here */
		break;
	case WLAN_DP_SAMPLING_STATE_FLOW_ADDED:
	{
		struct wlan_dp_stc_burst_samples *burst_sample;

		burst_sample = &flow_samples->burst_sample;
		/* Send an indication to TX and RX flow to start tracking */
		/*
		 * Flow is just added, so take the stats snapshot and
		 * move it to WLAN_DP_SAMPLING_STATE_SAMPLING_START
		 */
		s_entry->next_sample_idx = 0;
		s_entry->next_win_idx = 0;

		if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
			flow_id = s_entry->tx_flow_id;
			wlan_dp_stc_trigger_sampling(dp_stc, flow_id, 1, QDF_TX);
			flow = &dp_stc->tx_flow_table->entries[flow_id];
#ifdef METADATA_CHECK_NEEDED_DURING_ADD
			if (s_entry->tx_flow_metadata !=
							flow->guid) {
				qdf_assert_always(0);
				goto rx_flow_sample;
			}
#endif
			flow->metadata = s_entry->tx_flow_metadata;

			flow->idx.sample_win_idx =
					(s_entry->next_sample_idx << 16) |
					(s_entry->next_win_idx);
			qdf_mem_copy(&s_entry->tx_stats_ref,
				     &flow->txrx_stats,
				     sizeof(s_entry->tx_stats_ref));
			/*
			 * Push these ref stats to burst_stats->txrx_stats,
			 * since it can be used as ref for 30-sec window
			 */
			qdf_mem_copy(&burst_sample->txrx_samples.tx,
				     &s_entry->tx_stats_ref,
				     sizeof(struct wlan_dp_stc_txrx_stats));
		}

#ifdef METADATA_CHECK_NEEDED_DURING_ADD
sample:
#endif
		if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
			flow_id = s_entry->rx_flow_id;
			wlan_dp_stc_trigger_sampling(dp_stc, flow_id, 1, QDF_RX);
			flow = &dp_stc->rx_flow_table->entries[flow_id];
#ifdef METADATA_CHECK_NEEDED_DURING_ADD
			if (s_entry->rx_flow_metadata != flow->metadata) {
				qdf_assert_always(0);
				goto fail;
			}
#endif

			flow->metadata = s_entry->rx_flow_metadata;
			flow->idx.sample_win_idx =
					(s_entry->next_sample_idx << 16) |
					(s_entry->next_win_idx);
			qdf_mem_copy(&s_entry->rx_stats_ref,
				     &flow->txrx_stats,
				     sizeof(s_entry->rx_stats_ref));

			/*
			 * Push these ref stats to burst_stats->txrx_stats,
			 * since it can be used as ref for 30-sec window
			 */
			qdf_mem_copy(&burst_sample->txrx_samples.rx,
				     &s_entry->rx_stats_ref,
				     sizeof(struct wlan_dp_stc_txrx_stats));
		}

		s_entry->max_num_sample_attempts =
				DP_STC_LONG_WINDOW_MS / DP_STC_TIMER_THRESH_MS;
		s_entry->state = WLAN_DP_SAMPLING_STATE_SAMPLING_START;
		sampling_pending = true;
		break;
	}
	case WLAN_DP_SAMPLING_STATE_SAMPLING_START:
		sample_idx = s_entry->next_sample_idx;
		win_idx = s_entry->next_win_idx;
		txrx_samples = &flow_samples->txrx_samples[sample_idx][win_idx];
		if ((win_idx + 1) == DP_TXRX_SAMPLES_WINDOW_MAX) {
			s_entry->next_win_idx = 0;
			s_entry->next_sample_idx++;
			txrx_samples->win_size = DP_STC_TIMER_THRESH_MS +
						 DP_STC_TIMER_THRESH_MS;
		} else {
			s_entry->next_win_idx++;
			txrx_samples->win_size = DP_STC_TIMER_THRESH_MS;
		}

		s_entry->max_num_sample_attempts--;
		/*
		 * 1) wlan_dp_stc_get_txrx_stats for Tx and Rx flow
		 * 2) Calculate delta stats
		 * 3) Push to the sampling flow table
		 * 4) Update next_sample_idx & next_win_idx
		 * 5) If next_sample_idx & next_win_idx = (5/0),
		 *	mark sampling done
		 * 6) Incr some global atomic variable ?? to indicate to
		 *	periodic work.
		 * There should be no further access to this index.
		 */

		if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID) {
			flow_id = s_entry->tx_flow_id;
			flow = &dp_stc->tx_flow_table->entries[flow_id];
			if (s_entry->tx_flow_metadata != flow->metadata) {
				s_entry->state =
					WLAN_DP_SAMPLING_STATE_SAMPLING_FAIL;
				/* continue to next flow */
				break;
			}

			flow->idx.sample_win_idx =
					((s_entry->next_sample_idx) << 16) |
					(s_entry->next_win_idx);
			qdf_mem_copy(&tx_stats,
				     &flow->txrx_stats,
				     sizeof(tx_stats));
			wlan_dp_stc_save_delta_stats(&txrx_samples->tx,
						     &tx_stats,
						     &s_entry->tx_stats_ref);
			wlan_dp_stc_save_min_max_state(&txrx_samples->tx,
						       &flow->txrx_min_max_stats[sample_idx][win_idx]);
		}

		if (s_entry->flags & WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID) {
			flow_id = s_entry->rx_flow_id;
			flow = &dp_stc->rx_flow_table->entries[flow_id];
			if (s_entry->rx_flow_metadata != flow->metadata) {
				s_entry->state =
					WLAN_DP_SAMPLING_STATE_SAMPLING_FAIL;
				/* continue to next flow */
				break;
			}

			flow->idx.sample_win_idx =
					((s_entry->next_sample_idx) << 16) |
					(s_entry->next_win_idx);
			qdf_mem_copy(&rx_stats,
				     &flow->txrx_stats,
				     sizeof(rx_stats));
			wlan_dp_stc_save_delta_stats(&txrx_samples->rx,
						     &rx_stats,
						     &s_entry->rx_stats_ref);
			wlan_dp_stc_save_min_max_state(&txrx_samples->rx,
						       &flow->txrx_min_max_stats[sample_idx][win_idx]);
		}

		if (s_entry->next_win_idx == 0) {
			qdf_mem_copy(&s_entry->tx_stats_ref,
				     &tx_stats,
				     sizeof(s_entry->tx_stats_ref));
			qdf_mem_copy(&s_entry->rx_stats_ref,
				     &rx_stats,
				     sizeof(s_entry->rx_stats_ref));
		}

		if (s_entry->next_sample_idx == DP_STC_TXRX_SAMPLES_MAX) {
			s_entry->flags |=
				WLAN_DP_SAMPLING_FLAGS_TXRX_SAMPLES_READY;
			s_entry->state =
				WLAN_DP_SAMPLING_STATE_SAMPLING_BURST_STATS;
		}
		sampling_pending = true;
		break;
	case WLAN_DP_SAMPLING_STATE_SAMPLING_BURST_STATS:
		s_entry->max_num_sample_attempts--;
		if (qdf_unlikely(!s_entry->max_num_sample_attempts)) {
			/* Burst samples are ready, push to Global */
			wlan_dp_stc_save_burst_samples(dp_stc, s_entry);
			s_entry->flags |=
				WLAN_DP_SAMPLING_FLAGS_BURST_SAMPLES_READY;
			s_entry->state = WLAN_DP_SAMPLING_STATE_SAMPLING_DONE;
			/* Set some indication to periodic work */
			wlan_dp_stc_stop_flow_tracking(dp_stc, s_entry);
			break;
		}
		sampling_pending = true;
		break;
	case WLAN_DP_SAMPLING_STATE_SAMPLING_DONE:
		/*
		 * Nothing to be done.
		 * Periodic work will take care of NL packing.
		 * The periodic work will remove this entry as well
		 * from the sampling table.
		 */
		/* Maybe mark done in original flow table,
		 * so that code execution does not reach here
		 */
		break;
	case WLAN_DP_SAMPLING_STATE_SAMPLING_FAIL:
		fallthrough;
	case WLAN_DP_SAMPLING_STATE_SAMPLES_SENT:
		fallthrough;
	case WLAN_DP_SAMPLING_STATE_CLASSIFIED:
		fallthrough;
	default:
		break;
	}

	return sampling_pending;
}

static void wlan_dp_stc_flow_sampling_timer(void *arg)
{
	struct wlan_dp_stc *dp_stc = (struct wlan_dp_stc *)arg;
	struct wlan_dp_stc_sampling_table_entry *s_entry;
	bool sampling_pending = false;
	int i;

	for (i = 0; i < DP_STC_SAMPLE_FLOWS_MAX; i++) {
		s_entry = &dp_stc->sampling_flow_table->entries[i];
		sampling_pending |= wlan_dp_stc_sample_flow(dp_stc, s_entry);
	}

	if (sampling_pending)
		qdf_timer_mod(&dp_stc->flow_sampling_timer,
			      DP_STC_TIMER_THRESH_MS);
	else
		dp_stc->sample_timer_state = WLAN_DP_STC_TIMER_STOPPED;

	return;
}

QDF_STATUS
wlan_dp_stc_handle_flow_stats_policy(enum qca_async_stats_type type,
				     enum qca_async_stats_action action)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;

	if (!dp_stc)
		return QDF_STATUS_E_NOSUPPORT;

	dp_info("STC: type %d action %d", type, action);
	switch (type) {
	case QCA_ASYNC_STATS_TYPE_FLOW_STATS:
		if (action == QCA_ASYNC_STATS_ACTION_START)
			dp_stc->send_flow_stats = 1;
		else if (action == QCA_ASYNC_STATS_ACTION_STOP)
			dp_stc->send_flow_stats = 0;
		else
			return QDF_STATUS_E_INVAL;
		break;
	case QCA_ASYNC_STATS_TYPE_CLASSIFIED_FLOW_STATS:
		if (action == QCA_ASYNC_STATS_ACTION_START)
			dp_stc->send_classified_flow_stats = 1;
		else if (action == QCA_ASYNC_STATS_ACTION_STOP)
			dp_stc->send_classified_flow_stats = 0;
		else
			return QDF_STATUS_E_INVAL;
		break;
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

static inline bool wlan_dp_is_flow_exact_match(struct flow_info *tuple1,
					       struct flow_info *tuple2)
{
	if ((tuple1->src_port ^ tuple2->src_port) ||
	    (tuple1->dst_port ^ tuple2->dst_port) ||
	    (tuple1->proto ^ tuple2->proto))
		return false;

	if ((tuple1->flags & DP_FLOW_TUPLE_FLAGS_IPV4) &&
	    ((tuple1->src_ip.ipv4_addr ^ tuple2->src_ip.ipv4_addr) ||
	     (tuple1->dst_ip.ipv4_addr ^ tuple2->dst_ip.ipv4_addr)))
		return false;

	if ((tuple1->flags & DP_FLOW_TUPLE_FLAGS_IPV6) &&
	    ((tuple1->src_ip.ipv6_addr[0] ^ tuple2->src_ip.ipv6_addr[0]) ||
	     (tuple1->src_ip.ipv6_addr[1] ^ tuple2->src_ip.ipv6_addr[1]) ||
	     (tuple1->src_ip.ipv6_addr[2] ^ tuple2->src_ip.ipv6_addr[2]) ||
	     (tuple1->src_ip.ipv6_addr[3] ^ tuple2->src_ip.ipv6_addr[3]) ||
	     (tuple1->dst_ip.ipv6_addr[0] ^ tuple2->dst_ip.ipv6_addr[0]) ||
	     (tuple1->dst_ip.ipv6_addr[1] ^ tuple2->dst_ip.ipv6_addr[1]) ||
	     (tuple1->dst_ip.ipv6_addr[2] ^ tuple2->dst_ip.ipv6_addr[2]) ||
	     (tuple1->dst_ip.ipv6_addr[3] ^ tuple2->dst_ip.ipv6_addr[3])))
		return false;

	return true;
}

void
wlan_dp_stc_handle_flow_classify_result(struct wlan_dp_stc_flow_classify_result *flow_classify_result)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_sampling_table *s_table;
	struct flow_info *flow_tuple = &flow_classify_result->flow_tuple;
	struct wlan_dp_stc_sampling_table_entry *s_entry;
	uint64_t hash;
	int i;

	hash = wlan_dp_get_flow_hash(dp_ctx, flow_tuple);
	s_table = dp_stc->sampling_flow_table;

	for (i = 0; i < DP_STC_SAMPLE_FLOWS_MAX; i++) {
		s_entry = &s_table->entries[i];
		if (s_entry->state == WLAN_DP_SAMPLING_STATE_INIT)
			continue;

		/* Neither samples have been reported, so skip this flow */
		if (!(wlan_dp_txrx_samples_reported(s_entry) ||
		      wlan_dp_burst_samples_reported(s_entry)))
			continue;

		if (s_entry->tuple_hash != hash)
			continue;

		if (!wlan_dp_is_flow_exact_match(flow_tuple,
						 &s_entry->flow_samples.flow_tuple))
			continue;

		/*
		 * Flow is an exact match.
		 * The classification result is for this flow only.
		 */
		s_entry->traffic_type = flow_classify_result->traffic_type;
		dp_info("STC: sampling flow %d result %d burst_reported %d",
			i, flow_classify_result->traffic_type,
			wlan_dp_burst_samples_reported(s_entry));
		/*
		 * 1) Indicate to TX and RX flow
		 * 2) Change state to classified,
		 *	timer will remove it from table
		 * The risk in changing state here is that, if the flow is
		 * classified using txrx samples, then timer may change the
		 * state to BURST_SAMPLING. This is write from 2 contexts.
		 */
		/*
		 * Sampling is done under below cases
		 * 1) txrx stats reported and flow classified as valid type
		 * 2) burst stats reported and flow classification attempted
		 */
		if (wlan_dp_stc_is_traffic_type_known(s_entry->traffic_type) ||
		    wlan_dp_burst_samples_reported(s_entry))
			s_entry->state = WLAN_DP_SAMPLING_STATE_CLASSIFIED;

		break;
	}

	/*
	 * Currently there is no case of sampling table entry being removed
	 * without the classification result. So assert if we are unable to
	 * find the flow which has been classified.
	 */
	if (i == DP_STC_SAMPLE_FLOWS_MAX) {
		dp_info("STC: Could not find the classified flow in table");
	}
}

QDF_STATUS wlan_dp_stc_peer_event_notify(ol_txrx_soc_handle soc,
					 enum cdp_peer_event event,
					 uint16_t peer_id, uint8_t vdev_id,
					 uint8_t *peer_mac_addr)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_peer_traffic_context *peer_tc;

	if (!dp_stc)
		return QDF_STATUS_E_NOSUPPORT;

	if (peer_id >= DP_STC_MAX_PEERS)
		return QDF_STATUS_E_INVAL;

	peer_tc = &dp_stc->peer_tc[peer_id];

	dp_info("STC: notify for peer %d, event %d, valid %d",
		peer_id, event, peer_tc->valid);
	switch (event) {
	case CDP_PEER_EVENT_MAP:
		if (peer_tc->valid) {
			dp_info("STC: Peer map notify for active peer");
			qdf_assert_always(0);
			return QDF_STATUS_E_BUSY;
		}

		peer_tc->vdev_id = vdev_id;
		peer_tc->peer_id = peer_id;
		qdf_mem_copy(peer_tc->mac_addr.bytes,
			     peer_mac_addr, QDF_MAC_ADDR_SIZE);
		peer_tc->valid = 1;
		break;
	case CDP_PEER_EVENT_UNMAP:
		if (!peer_tc->valid) {
			dp_info("STC: Peer unmap notify for inactive peer");
			qdf_assert_always(0);
			return QDF_STATUS_E_BUSY;
		}

		if (qdf_mem_cmp(peer_tc->mac_addr.bytes,
				peer_mac_addr, QDF_MAC_ADDR_SIZE) != 0) {
			dp_err("STC: peer unmap notify: mac addr mismatch");
			return QDF_STATUS_E_INVAL;
		}
		peer_tc->valid = 0;
		break;
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

uint32_t wlan_dp_stc_get_logmask(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;

	if (!dp_stc) {
		dp_info("STC: module not initialized");
		return 0;
	}

	return dp_stc->logmask;
}

void wlan_dp_stc_update_logmask(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t mask)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;

	if (!dp_stc) {
		dp_info("STC: module not initialized");
		return;
	}

	dp_stc->logmask = mask;
}

static bool
wlan_dp_stc_is_traffic_conext_supported(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		dp_err("Invalid WMI handle");
		return false;
	}

	return wmi_service_enabled(wmi_handle,
				   wmi_service_traffic_context_support);
}

static bool wlan_dp_stc_clients_available(struct wlan_dp_psoc_context *dp_ctx)
{
	if (wlan_dp_stc_is_traffic_conext_supported(dp_ctx->psoc)) {
		dp_info("STC: TCAM client available");
		return true;
	}

	return false;
}

void wlan_dp_stc_cfg_init(struct wlan_dp_psoc_cfg *config,
			  struct wlan_objmgr_psoc *psoc)
{
	config->stc_enable = cfg_get(psoc, CFG_DP_STC_ENABLE);
}

QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct dp_soc *soc = (struct dp_soc *)dp_ctx->cdp_soc;
	struct wlan_dp_stc *dp_stc;
	struct wlan_dp_stc_sampling_table *s_table;
	struct wlan_dp_stc_rx_flow_table *rx_flow_table;
	struct wlan_dp_stc_tx_flow_table *tx_flow_table;
	struct wlan_dp_stc_classified_flow_table *c_table;
	QDF_STATUS status;

	if (!wlan_dp_cfg_is_stc_enabled(&dp_ctx->dp_cfg)) {
		dp_info("STC: feature not enabled via cfg");
		return QDF_STATUS_SUCCESS;
	}

	if (!wlan_dp_stc_clients_available(dp_ctx)) {
		dp_info("STC: No clients available, skip attach");
		dp_ctx->dp_stc = NULL;
		return QDF_STATUS_SUCCESS;
	}

	dp_info("STC: attach");
	dp_stc = dp_context_alloc_mem(soc, DP_STC_CONTEXT_TYPE,
				      sizeof(*dp_stc));
	if (!dp_stc) {
		dp_err("STC context alloc failed");
		return QDF_STATUS_E_NOMEM;
	}

	s_table = dp_context_alloc_mem(soc, DP_STC_SAMPLING_TABLE_TYPE,
				       sizeof(*s_table));
	if (!s_table) {
		dp_err("STC sampling flow table alloc failed");
		status = QDF_STATUS_E_NOMEM;
		goto sampling_table_alloc_fail;
	}
	dp_stc->sampling_flow_table = s_table;

	rx_flow_table = dp_context_alloc_mem(soc, DP_STC_RX_FLOW_TABLE_TYPE,
					     sizeof(*rx_flow_table));
	if (!rx_flow_table) {
		dp_err("STC RX flow table alloc failed");
		status = QDF_STATUS_E_NOMEM;
		goto rx_flow_table_alloc_fail;
	}
	dp_stc->rx_flow_table = rx_flow_table;

	tx_flow_table = dp_context_alloc_mem(soc, DP_STC_TX_FLOW_TABLE_TYPE,
					     sizeof(*tx_flow_table));
	if (!tx_flow_table) {
		dp_err("STC TX flow table alloc failed");
		status = QDF_STATUS_E_NOMEM;
		goto tx_flow_table_alloc_fail;
	}
	dp_stc->tx_flow_table = tx_flow_table;

	c_table = dp_context_alloc_mem(soc, DP_STC_CLASSIFIED_FLOW_TABLE_TYPE,
				       sizeof(*c_table));
	if (!c_table) {
		dp_err("STC classified flow table alloc failed");
		status = QDF_STATUS_E_NOMEM;
		goto classified_table_alloc_fail;
	}
	dp_stc->classified_flow_table = c_table;

	dp_ctx->dp_stc = dp_stc;
	dp_stc->dp_ctx = dp_ctx;

	/* Init periodic work */
	status = qdf_periodic_work_create(&dp_stc->flow_monitor_work,
					  wlan_dp_stc_flow_monitor_work_handler,
					  dp_stc);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_info("STC flow monitor work creation failed");
		goto periodic_work_creation_fail;
	}

	dp_stc->flow_monitor_interval = 100;
	dp_stc->periodic_work_state = WLAN_DP_STC_WORK_INIT;

	/* Init timer */
	status = qdf_timer_init(dp_ctx->qdf_dev, &dp_stc->flow_sampling_timer,
				wlan_dp_stc_flow_sampling_timer, dp_stc,
				QDF_TIMER_TYPE_SW);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_info("STC flow sampling timer init failed");
		goto timer_init_fail;
	}

	dp_stc->sample_timer_state = WLAN_DP_STC_TIMER_INIT;
	dp_fisa_rx_add_tcp_flow_to_fst(dp_ctx);

	return QDF_STATUS_SUCCESS;

timer_init_fail:
	qdf_periodic_work_destroy(&dp_stc->flow_monitor_work);

periodic_work_creation_fail:
	dp_context_free_mem(soc, DP_STC_CLASSIFIED_FLOW_TABLE_TYPE, c_table);
classified_table_alloc_fail:
	dp_context_free_mem(soc, DP_STC_TX_FLOW_TABLE_TYPE, tx_flow_table);
tx_flow_table_alloc_fail:
	dp_context_free_mem(soc, DP_STC_RX_FLOW_TABLE_TYPE, rx_flow_table);
rx_flow_table_alloc_fail:
	dp_context_free_mem(soc, DP_STC_SAMPLING_TABLE_TYPE, s_table);
sampling_table_alloc_fail:
	dp_context_free_mem(soc, DP_STC_CONTEXT_TYPE, dp_stc);
	dp_ctx->dp_stc = NULL;

	return status;
}

QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct dp_soc *soc = (struct dp_soc *)dp_ctx->cdp_soc;

	if (!dp_stc) {
		dp_info("STC: module not initialized, skip detach");
		return QDF_STATUS_SUCCESS;
	}

	dp_info("STC: detach");
	qdf_timer_sync_cancel(&dp_stc->flow_sampling_timer);
	qdf_periodic_work_stop_sync(&dp_stc->flow_monitor_work);
	qdf_periodic_work_destroy(&dp_stc->flow_monitor_work);
	dp_context_free_mem(soc, DP_STC_CLASSIFIED_FLOW_TABLE_TYPE,
			    dp_stc->classified_flow_table);
	dp_context_free_mem(soc, DP_STC_TX_FLOW_TABLE_TYPE,
			    dp_stc->tx_flow_table);
	dp_context_free_mem(soc, DP_STC_RX_FLOW_TABLE_TYPE,
			    dp_stc->rx_flow_table);
	dp_context_free_mem(soc, DP_STC_SAMPLING_TABLE_TYPE,
			    dp_stc->sampling_flow_table);
	dp_context_free_mem(soc, DP_STC_CONTEXT_TYPE, dp_stc);

	dp_ctx->dp_stc = NULL;

	return QDF_STATUS_SUCCESS;
}
#endif
