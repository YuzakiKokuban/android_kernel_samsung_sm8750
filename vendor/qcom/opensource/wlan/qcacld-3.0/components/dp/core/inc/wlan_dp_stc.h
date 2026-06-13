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
  * DOC: wlan_dp_stc.h
  *
  *
  */
#ifndef __WLAN_DP_STC_H__
#define __WLAN_DP_STC_H__

#include "wlan_dp_main.h"
#include "wlan_dp_spm.h"

#define DP_STC_UPDATE_MIN_MAX_STATS(__field, __val)			\
	do {								\
		if (__field##_min == 0 || __field##_min > __val)	\
			__field##_min = __val;				\
		if (__field##_max < __val)				\
			__field##_max = __val;				\
	} while (0)

#define DP_STC_UPDATE_MIN_MAX_SUM_STATS(__field, __val)			\
	do {								\
		__field##_sum += __val;					\
		if (__field##_min == 0 || __field##_min > __val)	\
			__field##_min = __val;				\
		if (__field##_max < __val)				\
			__field##_max = __val;				\
	} while (0)

#define DP_STC_UPDATE_WIN_MIN_MAX_STATS(__field, __val)			\
	do {								\
		if (__field##_min == 0 || __field##_min > __val)	\
			__field##_min = __val;				\
		if (__field##_max < __val)				\
			__field##_max = __val;				\
	} while (0)

/* TODO - This macro needs to be same as the max peers in CMN DP */
#define DP_STC_MAX_PEERS 64

#define BURST_START_TIME_THRESHOLD_NS 10000000
#define BURST_START_BYTES_THRESHOLD 5000
#define BURST_END_TIME_THRESHOLD_NS 1500000000
#define FLOW_CLASSIFY_WAIT_TIME_NS 10000000000

#define WLAN_DP_STC_TX_FLOW_ID_INTF_ID_SHIFT 6
#define WLAN_DP_STC_TX_FLOW_ID_INTF_ID_MASK 0x3
#define WLAN_DP_STC_TX_FLOW_ID_MASK 0x3f

/**
 * struct wlan_dp_stc_peer_ping_info - Active ping information table with
 *				       per peer records
 * @mac_addr: mac address of the peer
 * @last_ping_ts: timestamp when the last ping packet was traced
 */
struct wlan_dp_stc_peer_ping_info {
	struct qdf_mac_addr mac_addr;
	uint64_t last_ping_ts;
};

/**
 * enum wlan_dp_stc_periodic_work_state - Periodic work states
 * @WLAN_DP_STC_WORK_INIT: INIT state
 * @WLAN_DP_STC_WORK_STOPPED: Stopped state
 * @WLAN_DP_STC_WORK_STARTED: Started state
 * @WLAN_DP_STC_WORK_RUNNING: Running state
 */
enum wlan_dp_stc_periodic_work_state {
	WLAN_DP_STC_WORK_INIT,
	WLAN_DP_STC_WORK_STOPPED,
	WLAN_DP_STC_WORK_STARTED,
	WLAN_DP_STC_WORK_RUNNING,
};

/**
 * enum wlan_dp_stc_burst_state - Burst detection state
 * @BURST_DETECTION_INIT: Burst detection not started
 * @BURST_DETECTION_START: Burst detection has started
 * @BURST_DETECTION_BURST_START: Burst start is detected and burst has started
 */
enum wlan_dp_stc_burst_state {
	BURST_DETECTION_INIT,
	BURST_DETECTION_START,
	BURST_DETECTION_BURST_START,
};

#define DP_STC_SAMPLE_FLOWS_MAX 32
#define DP_STC_SAMPLE_BIDI_FLOW_MAX 16
#define DP_STC_SAMPLE_RX_FLOW_MAX 8
#define DP_STC_SAMPLE_TX_FLOW_MAX 8

#define DP_STC_LONG_WINDOW_MS 30000
#define DP_STC_TIMER_THRESH_MS 600

/**
 * enum wlan_stc_sampling_state - Sampling state
 * @WLAN_DP_SAMPLING_STATE_INIT: init state
 * @WLAN_DP_SAMPLING_STATE_FLOW_ADDED: flow added for sampling
 * @WLAN_DP_SAMPLING_STATE_SAMPLING_START: sampling started
 * @WLAN_DP_SAMPLING_STATE_SAMPLING_BURST_STATS: sampling burst stats
 * @WLAN_DP_SAMPLING_STATE_SAMPLING_DONE: sampling completed
 * @WLAN_DP_SAMPLING_STATE_SAMPLING_FAIL: sampling failed
 * @WLAN_DP_SAMPLING_STATE_SAMPLES_SENT: samples sent
 * @WLAN_DP_SAMPLING_STATE_CLASSIFIED: flow classified
 */
enum wlan_stc_sampling_state {
	WLAN_DP_SAMPLING_STATE_INIT,
	WLAN_DP_SAMPLING_STATE_FLOW_ADDED,
	WLAN_DP_SAMPLING_STATE_SAMPLING_START,
	WLAN_DP_SAMPLING_STATE_SAMPLING_BURST_STATS,
	WLAN_DP_SAMPLING_STATE_SAMPLING_DONE,
	WLAN_DP_SAMPLING_STATE_SAMPLING_FAIL,
	WLAN_DP_SAMPLING_STATE_SAMPLES_SENT,
	WLAN_DP_SAMPLING_STATE_CLASSIFIED,
};

/* Bit-fields used for "flags" in struct wlan_dp_stc_sampling_candidate */
#define WLAN_DP_SAMPLING_CANDIDATE_VALID BIT(0)
#define WLAN_DP_SAMPLING_CANDIDATE_TX_FLOW_VALID BIT(1)
#define WLAN_DP_SAMPLING_CANDIDATE_RX_FLOW_VALID BIT(2)

enum wlan_dp_flow_dir {
	WLAN_DP_FLOW_DIR_INVALID,
	WLAN_DP_FLOW_DIR_TX,
	WLAN_DP_FLOW_DIR_RX,
	WLAN_DP_FLOW_DIR_BIDI,
};

/**
 * struct wlan_dp_stc_sampling_candidate - Sampling candidate
 * @peer_id: Peer ID
 * @flags: flags
 * @tx_flow_id: TX flow ID
 * @rx_flow_id: RX flow ID
 * @tx_flow_metadata: TX flow metadata
 * @rx_flow_metadata: RX flow metadata
 * @flow_tuple: flow tuple
 * @dir: flow direction
 */
struct wlan_dp_stc_sampling_candidate {
	uint16_t peer_id;
	uint32_t flags;
	uint16_t tx_flow_id;
	uint16_t rx_flow_id;
	uint32_t tx_flow_metadata;
	uint32_t rx_flow_metadata;
	struct flow_info flow_tuple;
	enum wlan_dp_flow_dir dir;
};

/* bit-fields used for "flags" in struct wlan_dp_stc_sampling_table_entry */
#define WLAN_DP_SAMPLING_FLAGS_TX_FLOW_VALID BIT(0)
#define WLAN_DP_SAMPLING_FLAGS_RX_FLOW_VALID BIT(1)
#define WLAN_DP_SAMPLING_FLAGS_TXRX_SAMPLES_READY BIT(2)
#define WLAN_DP_SAMPLING_FLAGS_BURST_SAMPLES_READY BIT(3)

#define WLAN_DP_SAMPLING_FLAGS1_TXRX_SAMPLES_SENT BIT(0)
#define WLAN_DP_SAMPLING_FLAGS1_BURST_SAMPLES_SENT BIT(1)
#define WLAN_DP_SAMPLING_FLAGS1_FLOW_REPORT_SENT BIT(2)

/*
 * struct wlan_dp_stc_sampling_table_entry - Sampling table entry
 * @state: State of sampling for this flow
 * @dir: direction of flow
 * @burst_stats_report_ts: Burst stats reported timestamp
 * @flags: flags set by timer
 * @flags1: flags set by periodic work
 * @next_sample_idx: next sample index to fill min/max stats in per-packet path
 * @next_win_idx: next window index to fill min/max stats in per-packet path
 * @max_num_sample_attempts: max number of sampling_timer runs to collect
 *			     txrx and burst state
 * @tx_flow_id: tx flow ID
 * @rx_flow_id: rx flow ID
 * @tx_flow_metadata: tx flow metadata
 * @rx_flow_metadata: rx flow metadata
 * @tuple_hash: Flow tuple hash
 * @flow_tuple: Flow tuple info
 * @tx_stats_ref: tx window stats reference
 * @rx_stats_ref: rx window stats reference
 * @flow_samples: flow samples
 */
struct wlan_dp_stc_sampling_table_entry {
	enum wlan_stc_sampling_state state;
	enum wlan_dp_flow_dir dir;
	uint64_t burst_stats_report_ts;
	uint32_t flags;
	uint32_t flags1;
	uint8_t next_sample_idx;
	uint8_t next_win_idx;
	uint8_t max_num_sample_attempts;
	uint8_t traffic_type;
	uint16_t peer_id;
	uint16_t tx_flow_id;
	uint16_t rx_flow_id;
	uint32_t tx_flow_metadata;
	uint32_t rx_flow_metadata;
	uint64_t tuple_hash;
	struct flow_info flow_tuple;
	struct wlan_dp_stc_txrx_stats tx_stats_ref;
	struct wlan_dp_stc_txrx_stats rx_stats_ref;
	struct wlan_dp_stc_flow_samples flow_samples;
};

/*
 * struct wlan_dp_stc_sampling_table - Sampling table
 * @num_valid_entries: Number of valid flows added to sampling flow table
 * @num_bidi_flows: number of Bi-Di flows added to sampling flow table
 * @num_tx_only_flows: number of TX only flows added to sampling flow table
 * @num_rx_only_flows: number of RX only flows added to sampling flow table
 * @entries: records added to sampling table
 */
struct wlan_dp_stc_sampling_table {
	qdf_atomic_t num_valid_entries;
	qdf_atomic_t num_bidi_flows;
	qdf_atomic_t num_tx_only_flows;
	qdf_atomic_t num_rx_only_flows;
	struct wlan_dp_stc_sampling_table_entry entries[DP_STC_SAMPLE_FLOWS_MAX];
};

/**
 * struct wlan_dp_stc_flow_table_entry - Flow table maintained in per pkt path
 * @prev_pkt_arrival_ts: previous packet arrival time
 * @win_transition_complete: Window transition completion flag.
 * @metadata: flow metadata
 * @burst_state: burst state
 * @burst_start_time: burst start time
 * @burst_start_detect_bytes: current snapshot of total bytes during burst
 *			      start detection phase
 * @cur_burst_bytes: total bytes accumulated in current burst
 * @idx: union for storing sample & window index
 * @idx.sample_win_idx: sample and window index
 * @idx.s.win_idx: window index
 * @idx.s.sample_idx: sample index
 * @txrx_stats: txrx stats
 * @txrx_min_max_stats: placeholder for stats which needs per window
 *			min/max values
 * @burst_stats: burst stats
 */
struct wlan_dp_stc_flow_table_entry {
	uint64_t prev_pkt_arrival_ts;
	uint8_t win_transition_complete;
	uint32_t metadata;
	enum wlan_dp_stc_burst_state burst_state;
	uint64_t burst_start_time;
	uint32_t burst_start_detect_bytes;
	uint32_t cur_burst_bytes;
	/* Can be atomic.! Decide based on the accuracy during test */
	union {
		uint32_t sample_win_idx;
		struct {
			uint32_t win_idx:16,
				 sample_idx:16;
		} s;
	} idx;
	struct wlan_dp_stc_txrx_stats txrx_stats;
	struct wlan_dp_stc_txrx_min_max_stats txrx_min_max_stats[DP_STC_TXRX_SAMPLES_MAX][DP_TXRX_SAMPLES_WINDOW_MAX];
	struct wlan_dp_stc_burst_stats burst_stats;
};

#define DP_STC_FLOW_TABLE_ENTRIES_MAX 384
/**
 * struct wlan_dp_stc_rx_flow_table - RX flow table
 * @entries: RX flow table records
 */
struct wlan_dp_stc_rx_flow_table {
	struct wlan_dp_stc_flow_table_entry entries[DP_STC_FLOW_TABLE_ENTRIES_MAX];
};

/**
 * struct wlan_dp_stc_tx_flow_table - TX flow table
 * @entries: TX flow table records
 */
struct wlan_dp_stc_tx_flow_table {
	struct wlan_dp_stc_flow_table_entry entries[DP_STC_FLOW_TABLE_ENTRIES_MAX];
};

/**
 * enum wlan_dp_stc_timer_state - Sampling timer state
 * @WLAN_DP_STC_TIMER_INIT: Init state
 * @WLAN_DP_STC_TIMER_STOPPED: timer stopped state
 * @WLAN_DP_STC_TIMER_STARTED: timer started state
 * @WLAN_DP_STC_TIMER_RUNNING: timer running state
 */
enum wlan_dp_stc_timer_state {
	WLAN_DP_STC_TIMER_INIT,
	WLAN_DP_STC_TIMER_STOPPED,
	WLAN_DP_STC_TIMER_STARTED,
	WLAN_DP_STC_TIMER_RUNNING,
};

/**
 * struct wlan_dp_stc_peer_traffic_context - peer traffic context
 * @mac_addr: peer mac address
 * @valid: context valid flag
 * @vdev_id: vdev_id for the peer
 * @peer_id: peer_id assigned to this peer
 * @last_ping_ts: Last seen ping pkt timestamp
 * @num_streaming: Number of streaming flows on this peer
 * @num_gaming: Number of gaming flows on this peer
 * @num_voice_call: Number of voice call flows on this peer
 * @num_video_call: Number of video call flows on this peer
 * @active_bk_traffic: Flag to indicate if background traffic is
 *		       active on this peer
 * @active_ping: Flag to indicate if ping is active on this peer
 * @send_fw_ind: Flag to mark if traffic_map indication is to be sent to FW
 */
struct wlan_dp_stc_peer_traffic_context {
	struct qdf_mac_addr mac_addr;
	uint8_t valid;
	uint8_t vdev_id;
	uint16_t peer_id;
	uint64_t last_ping_ts;
	qdf_atomic_t num_streaming;
	qdf_atomic_t num_gaming;
	qdf_atomic_t num_voice_call;
	qdf_atomic_t num_video_call;
	qdf_atomic_t active_bk_traffic;
	qdf_atomic_t active_ping;
	qdf_atomic_t send_fw_ind;
};

#define DP_STC_CLASSIFIED_TABLE_FLOW_MAX 256
/**
 * struct wlan_dp_stc_classified_flow_entry - Classified flow entry
 * @flow_active: flag to indicate if the flow is active
 * @vdev_id: vdev_id on which the flow is active
 * @tx_flow_id: TX flow table index
 * @rx_flow_id: RX flow table index
 * @peer_id: ID of the peer on which the flow is running
 * @prev_tx_pkts: Last snapshot for TX pkts count
 * @prev_rx_pkts: Last snapshot for RX pkts count
 * @flags: flags bitmap
 * @del_flags: delete flags bitmap
 * @traffic_type: Traffic type identified
 * @state: STATE
 */
struct wlan_dp_stc_classified_flow_entry {
	uint8_t flow_active;
	uint8_t vdev_id;
	uint16_t tx_flow_id;
	uint16_t rx_flow_id;
	uint16_t peer_id;
	uint32_t prev_tx_pkts;
	uint32_t prev_rx_pkts;
	unsigned long flags;
	unsigned long del_flags;
	enum qca_traffic_type traffic_type;
	qdf_atomic_t state;
};

/**
 * struct wlan_dp_stc_classified_flow_table - Classified flow table
 * @num_valid_entries: Number of valid entries
 * @entries: classified flow table entries
 */
struct wlan_dp_stc_classified_flow_table {
	qdf_atomic_t num_valid_entries;
	struct wlan_dp_stc_classified_flow_entry entries[DP_STC_CLASSIFIED_TABLE_FLOW_MAX];
};

/**
 * struct wlan_dp_stc - Smart traffic classifier context
 * @dp_ctx: DP component global context
 * @send_flow_stats: Flag to indicate whether flow stats are to be reported
 * @send_classified_flow_stats: Flag to indicate whether the classified
 *				flow stats are to be reported
 * @flow_monitor_work: periodic work to process all the misc work for STC
 * @flow_monitor_interval: periodic flow monitor work interval
 * @logmask: mask indicating which logs are enabled
 * @periodic_work_state: States of the periodic flow monitor work
 * @flow_sampling_timer: timer to sample all the short-listed flows
 * @sample_timer_state: sampling timer state
 * @peer_tc: per peer active traffic context
 * @peer_ping_info: Ping tracking per peer
 * @sampling_flow_table: Sampling flow table
 * @rx_flow_table: RX flow table
 * @tx_flow_table: TX flow table
 * @classified_flow_table: Flow table of all the classified flows
 * @candidates: Sampling candidate selection table
 */
struct wlan_dp_stc {
	struct wlan_dp_psoc_context *dp_ctx;
	uint8_t send_flow_stats;
	uint8_t send_classified_flow_stats;
	struct qdf_periodic_work flow_monitor_work;
	uint32_t flow_monitor_interval;
	uint32_t logmask;
	enum wlan_dp_stc_periodic_work_state periodic_work_state;
	qdf_timer_t flow_sampling_timer;
	enum wlan_dp_stc_timer_state sample_timer_state;
	struct wlan_dp_stc_peer_traffic_context peer_tc[DP_STC_MAX_PEERS];
	struct wlan_dp_stc_sampling_table *sampling_flow_table;
	struct wlan_dp_stc_rx_flow_table *rx_flow_table;
	struct wlan_dp_stc_tx_flow_table *tx_flow_table;
	struct wlan_dp_stc_classified_flow_table *classified_flow_table;
	struct wlan_dp_stc_sampling_candidate candidates[DP_STC_SAMPLE_FLOWS_MAX];
};

/* Function Declaration - START */

#ifdef WLAN_DP_FEATURE_STC
/**
 * dp_stc_get_timestamp() - Get current timestamp to be used in STC
 *			    critical per packet path and its related calculation
 *
 * Return: current timestamp in ns
 */
static inline uint64_t dp_stc_get_timestamp(void)
{
	return qdf_sched_clock();
}

static inline uint8_t wlan_dp_ip_proto_to_stc_proto(uint16_t ip_proto)
{
	switch (ip_proto) {
	case IPPROTO_TCP:
		return QCA_WLAN_VENDOR_FLOW_POLICY_PROTO_TCP;
	case IPPROTO_UDP:
		return QCA_WLAN_VENDOR_FLOW_POLICY_PROTO_UDP;
	default:
		break;
	}

	return -EINVAL;
}

/**
 * wlan_dp_stc_populate_flow_tuple() - Populate the STC flow tuple using the
 *				       flow tuple in active flow table
 * @flow_tuple: STC flow tuple
 * @flow_tuple_info: flow tuple info
 *
 * Return: None
 */
static inline void
wlan_dp_stc_populate_flow_tuple(struct flow_info *flow_tuple,
				struct cdp_rx_flow_tuple_info *flow_tuple_info)
{
	uint8_t is_ipv4_flow = 1;	//TODO - Get from fisa flow table entry
	uint8_t is_ipv6_flow = 0;
	uint8_t proto = flow_tuple_info->l4_protocol;

	if (is_ipv4_flow) {
		flow_tuple->src_ip.ipv4_addr = flow_tuple_info->src_ip_31_0;
		flow_tuple->dst_ip.ipv4_addr = flow_tuple_info->dest_ip_31_0;
	} else if (is_ipv6_flow) {
		flow_tuple->src_ip.ipv6_addr[0] = flow_tuple_info->src_ip_31_0;
		flow_tuple->src_ip.ipv6_addr[1] = flow_tuple_info->src_ip_63_32;
		flow_tuple->src_ip.ipv6_addr[2] = flow_tuple_info->src_ip_95_64;
		flow_tuple->src_ip.ipv6_addr[3] =
						flow_tuple_info->src_ip_127_96;

		flow_tuple->dst_ip.ipv6_addr[0] = flow_tuple_info->dest_ip_31_0;
		flow_tuple->dst_ip.ipv6_addr[1] =
						flow_tuple_info->dest_ip_63_32;
		flow_tuple->dst_ip.ipv6_addr[2] =
						flow_tuple_info->dest_ip_95_64;
		flow_tuple->dst_ip.ipv6_addr[3] =
						flow_tuple_info->dest_ip_127_96;
	}
	flow_tuple->src_port = flow_tuple_info->src_port;
	flow_tuple->dst_port = flow_tuple_info->dest_port;
	flow_tuple->proto = wlan_dp_ip_proto_to_stc_proto(proto);
	flow_tuple->flags = 0;
}

static inline void
wlan_dp_stc_inc_traffic_type(struct wlan_dp_stc_peer_traffic_context *peer_tc,
			     enum qca_traffic_type traffic_type)
{
	uint32_t val = 0;

	switch (traffic_type) {
	case QCA_TRAFFIC_TYPE_STREAMING:
		val = qdf_atomic_inc_return(&peer_tc->num_streaming);
		break;
	case QCA_TRAFFIC_TYPE_GAMING:
		val = qdf_atomic_inc_return(&peer_tc->num_gaming);
		break;
	case QCA_TRAFFIC_TYPE_VOICE_CALL:
		val = qdf_atomic_inc_return(&peer_tc->num_voice_call);
		break;
	case QCA_TRAFFIC_TYPE_VIDEO_CALL:
		val = qdf_atomic_inc_return(&peer_tc->num_video_call);
		break;
	default:
		break;
	}

	if (val == 1)
		qdf_atomic_set(&peer_tc->send_fw_ind, 1);
}

static inline void
wlan_dp_stc_dec_traffic_type(struct wlan_dp_stc_peer_traffic_context *peer_tc,
			     enum qca_traffic_type traffic_type)
{
	uint32_t val = 0;

	switch (traffic_type) {
	case QCA_TRAFFIC_TYPE_STREAMING:
		val = qdf_atomic_dec_and_test(&peer_tc->num_streaming);
		break;
	case QCA_TRAFFIC_TYPE_GAMING:
		val = qdf_atomic_dec_and_test(&peer_tc->num_gaming);
		break;
	case QCA_TRAFFIC_TYPE_VOICE_CALL:
		val = qdf_atomic_dec_and_test(&peer_tc->num_voice_call);
		break;
	case QCA_TRAFFIC_TYPE_VIDEO_CALL:
		val = qdf_atomic_dec_and_test(&peer_tc->num_video_call);
		break;
	default:
		break;
	}

	if (val)
		qdf_atomic_set(&peer_tc->send_fw_ind, 1);
}

enum wlan_dp_stc_classfied_flow_state {
	WLAN_DP_STC_CLASSIFIED_FLOW_STATE_INIT,
	WLAN_DP_STC_CLASSIFIED_FLOW_STATE_ADDING,
	WLAN_DP_STC_CLASSIFIED_FLOW_STATE_ADDED,
};

#define WLAN_DP_CLASSIFIED_FLAGS_TX_FLOW_VALID 0
#define WLAN_DP_CLASSIFIED_FLAGS_RX_FLOW_VALID 1

#define WLAN_DP_CLASSIFIED_DEL_FLAGS_TX_DEL 0
#define WLAN_DP_CLASSIFIED_DEL_FLAGS_RX_DEL 1

static inline void
wlan_dp_stc_tx_flow_retire_ind(struct wlan_dp_psoc_context *dp_ctx,
			       uint8_t classified,
			       uint8_t c_flow_id)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_classified_flow_table *c_table;
	struct wlan_dp_stc_classified_flow_entry *c_entry;

	if (!dp_stc)
		return;

	if (!classified)
		return;

	c_table = dp_stc->classified_flow_table;
	c_entry = &c_table->entries[c_flow_id];
	if (qdf_atomic_read(&c_entry->state) ==
					WLAN_DP_STC_CLASSIFIED_FLOW_STATE_INIT)
		return;

	if (qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_FLAGS_TX_FLOW_VALID,
				&c_entry->flags))
		qdf_atomic_set_bit(WLAN_DP_CLASSIFIED_DEL_FLAGS_TX_DEL,
				   &c_entry->del_flags);
}

static inline void
wlan_dp_stc_rx_flow_retire_ind(struct wlan_dp_psoc_context *dp_ctx,
			       uint8_t classified,
			       uint8_t c_flow_id)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_classified_flow_table *c_table;
	struct wlan_dp_stc_classified_flow_entry *c_entry;

	if (!dp_stc)
		return;

	if (!classified)
		return;

	c_table = dp_stc->classified_flow_table;
	c_entry = &c_table->entries[c_flow_id];
	if (qdf_atomic_read(&c_entry->state) ==
					WLAN_DP_STC_CLASSIFIED_FLOW_STATE_INIT)
		return;

	if (qdf_atomic_test_bit(WLAN_DP_CLASSIFIED_FLAGS_RX_FLOW_VALID,
				&c_entry->flags))
		qdf_atomic_set_bit(WLAN_DP_CLASSIFIED_DEL_FLAGS_RX_DEL,
				   &c_entry->del_flags);
}

/**
 * wlan_dp_stc_mark_ping_ts() - Mark the last ping timestamp in STC table
 * @dp_ctx: DP global psoc context
 * @peer_id: peer id
 *
 * Return: void
 */
static inline void
wlan_dp_stc_mark_ping_ts(struct wlan_dp_psoc_context *dp_ctx,
			 uint16_t peer_id)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_peer_traffic_context *peer_tc;
	bool send_fw_indication = false;

	if (!dp_stc)
		return;

	if (peer_id == 0xFFFF)
		return;

	peer_tc = &dp_stc->peer_tc[peer_id];
	if (!peer_tc->valid)
		return;

	if (peer_tc->last_ping_ts == 0)
		send_fw_indication = true;

	peer_tc->last_ping_ts = dp_stc_get_timestamp();
	qdf_atomic_set(&peer_tc->active_ping, 1);

	if (send_fw_indication)
		qdf_atomic_set(&peer_tc->send_fw_ind, 1);
}

/**
 * wlan_dp_indicate_rx_flow_add() - Indication to STC when rx flow is added
 * @dp_ctx: Global DP psoc context
 *
 * Return: None
 */
static inline void
wlan_dp_indicate_rx_flow_add(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;

	/* RCU or atomic variable?? */
	if (dp_stc && dp_stc->periodic_work_state < WLAN_DP_STC_WORK_STARTED) {
		qdf_periodic_work_start(&dp_stc->flow_monitor_work,
					dp_stc->flow_monitor_interval);
		dp_stc->periodic_work_state = WLAN_DP_STC_WORK_STARTED;
	}
}

/**
 * wlan_dp_stc_track_flow_features() - Track flow features
 * @dp_stc: STC context
 * @nbuf: packet handle
 * @flow_entry: flow entry (to which the current pkt belongs)
 * @vdev_id: ID of vdev to which the packet belongs
 * @peer_id: ID of peer to which the packet belongs
 * @pkt_len: length of the packet (excluding ethernet header)
 * @metadata: RX flow metadata
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_dp_stc_track_flow_features(struct wlan_dp_stc *dp_stc, qdf_nbuf_t nbuf,
				struct wlan_dp_stc_flow_table_entry *flow_entry,
				uint8_t vdev_id, uint16_t peer_id,
				uint16_t pkt_len, uint32_t metadata);

static inline QDF_STATUS
wlan_dp_stc_check_n_track_rx_flow_features(struct wlan_dp_psoc_context *dp_ctx,
					   qdf_nbuf_t nbuf)
{
	struct wlan_dp_stc_flow_table_entry *flow_entry;
	struct wlan_dp_stc *dp_stc;
	uint8_t vdev_id;
	uint16_t peer_id;
	uint16_t flow_id;
	uint32_t metadata;
	uint16_t pkt_len;

	if (qdf_likely(!QDF_NBUF_CB_RX_TRACK_FLOW(nbuf)))
		return QDF_STATUS_SUCCESS;

	/* Do not update flow feature stats for TCP pure acks */
	if (qdf_unlikely(QDF_NBUF_CB_RX_TCP_PURE_ACK(nbuf)))
		return QDF_STATUS_SUCCESS;

	dp_stc = dp_ctx->dp_stc;
	vdev_id = QDF_NBUF_CB_RX_VDEV_ID(nbuf);
	peer_id = QDF_NBUF_CB_RX_PEER_ID(nbuf);
	flow_id = QDF_NBUF_CB_EXT_RX_FLOW_ID(nbuf);
	metadata = QDF_NBUF_CB_RX_FLOW_METADATA(nbuf);
	pkt_len = qdf_nbuf_len(nbuf);

	flow_entry = &dp_stc->rx_flow_table->entries[flow_id];

	return wlan_dp_stc_track_flow_features(dp_stc, nbuf, flow_entry,
					       vdev_id, peer_id, pkt_len,
					       metadata);
}

/**
 * wlan_dp_stc_check_n_track_tx_flow_features() - Update the stats if flow is
 *						  tracking enabled
 * @dp_ctx: DP global psoc context
 * @nbuf: Packet nbuf
 * @flow_track_enabled: Flow tracking enabled
 * @flow_id: Flow id of flow
 * @vdev_id: Vdev of the flow
 * @peer_id: Peer_id of the flow
 * @metadata: Metadata of the flow
 *
 * Return: QDF_STATUS
 */
static inline QDF_STATUS
wlan_dp_stc_check_n_track_tx_flow_features(struct wlan_dp_psoc_context *dp_ctx,
					   qdf_nbuf_t nbuf,
					   uint32_t flow_track_enabled,
					   uint16_t flow_id, uint8_t vdev_id,
					   uint16_t peer_id, uint32_t metadata)
{
	struct wlan_dp_stc *dp_stc = dp_ctx->dp_stc;
	struct wlan_dp_stc_flow_table_entry *flow_entry;
	uint16_t pkt_len;

	if (qdf_likely(!flow_track_enabled))
		return QDF_STATUS_SUCCESS;

	/* Do not update flow feature stats for TCP pure acks */
	if (qdf_unlikely(QDF_NBUF_CB_GET_PACKET_TYPE(nbuf) ==
					QDF_NBUF_CB_PACKET_TYPE_TCP_ACK))
		return QDF_STATUS_SUCCESS;

	flow_entry = &dp_stc->tx_flow_table->entries[flow_id];
	pkt_len = qdf_nbuf_len(nbuf) - sizeof(qdf_ether_header_t);

	return wlan_dp_stc_track_flow_features(dp_stc, nbuf, flow_entry,
					       vdev_id, peer_id, pkt_len,
					       metadata);
}

QDF_STATUS
wlan_dp_stc_handle_flow_stats_policy(enum qca_async_stats_type type,
				     enum qca_async_stats_action);
/**
 * wlan_dp_stc_handle_flow_classify_result() - Handle flow classify result
 * @flow_classify_result:
 *
 * Return: none
 */
void
wlan_dp_stc_handle_flow_classify_result(struct wlan_dp_stc_flow_classify_result *flow_classify_result);

/**
 * wlan_dp_stc_peer_event_notify() - Handle the peer map/unmap events
 * @soc: CDP soc
 * @event: Peer event
 * @peer_id: Peer ID
 * @vdev_id: VDEV ID
 * @peer_mac_addr: mac address of the Peer
 */
QDF_STATUS wlan_dp_stc_peer_event_notify(ol_txrx_soc_handle soc,
					 enum cdp_peer_event event,
					 uint16_t peer_id, uint8_t vdev_id,
					 uint8_t *peer_mac_addr);

/**
 * wlan_dp_stc_cfg_init() - CFG init for STC
 * @config: SoC CFG config
 * @psoc: Objmgr PSoC handle
 *
 * Return: None
 */
void wlan_dp_stc_cfg_init(struct wlan_dp_psoc_cfg *config,
			  struct wlan_objmgr_psoc *psoc);

/**
 * wlan_dp_cfg_is_stc_enabled() - Helper function to check if STC is enabled
 * @dp_cfg: SoC CFG config
 *
 * Return: true if STC is enabled, false if STC is disabled.
 */
static inline bool wlan_dp_cfg_is_stc_enabled(struct wlan_dp_psoc_cfg *dp_cfg)
{
	return dp_cfg->stc_enable;
}

/**
 * wlan_dp_stc_get_logmask() - Get STC log mask
 * @dp_ctx: DP global psoc context
 *
 * Return: logmask configured in STC
 */
uint32_t wlan_dp_stc_get_logmask(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_stc_update_logmask() - Set STC log mask
 * @dp_ctx: DP global psoc context
 * @mask: new log mask to be set
 *
 * Return: None
 */
void wlan_dp_stc_update_logmask(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t mask);

/**
 * wlan_dp_stc_attach() - STC attach
 * @dp_ctx: DP global psoc context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_stc_detach() - STC detach
 * @dp_ctx: DP global psoc context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx);
#else
static inline void
wlan_dp_stc_populate_flow_tuple(struct flow_info *flow_tuple,
				struct cdp_rx_flow_tuple_info *flow_tuple_info)
{
}

static inline void
wlan_dp_indicate_rx_flow_add(struct wlan_dp_psoc_context *dp_ctx)
{
}

static inline void
wlan_dp_stc_tx_flow_retire_ind(struct wlan_dp_psoc_context *dp_ctx,
			       uint8_t classified,
			       uint8_t c_flow_id)
{
}

static inline void
wlan_dp_stc_rx_flow_retire_ind(struct wlan_dp_psoc_context *dp_ctx,
			       uint8_t classified,
			       uint8_t c_flow_id)
{
}

static inline void
wlan_dp_stc_mark_ping_ts(struct wlan_dp_psoc_context *dp_ctx,
			 uint16_t peer_id)
{
}

static inline QDF_STATUS
wlan_dp_stc_check_n_track_rx_flow_features(struct wlan_dp_psoc_context *dp_ctx,
					   qdf_nbuf_t nbuf)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
wlan_dp_stc_check_n_track_tx_flow_features(struct wlan_dp_psoc_context *dp_ctx,
					   qdf_nbuf_t nbuf,
					   uint32_t flow_track_enabled,
					   uint16_t flow_id, uint8_t vdev_id,
					   uint16_t peer_id, uint32_t metadata)
{
	return QDF_STATUS_SUCCESS;
}

static inline void wlan_dp_stc_cfg_init(struct wlan_dp_psoc_cfg *config,
					struct wlan_objmgr_psoc *psoc)
{
}

static inline bool wlan_dp_cfg_is_stc_enabled(struct wlan_dp_psoc_cfg *dp_cfg)
{
	return false;
}

static inline
uint32_t wlan_dp_stc_get_logmask(struct wlan_dp_psoc_context *dp_ctx)
{
	return 0;
}

static inline
void wlan_dp_stc_update_logmask(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t mask)
{
}

static inline
QDF_STATUS wlan_dp_stc_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS wlan_dp_stc_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	return QDF_STATUS_SUCCESS;
}
#endif
/* Function Declaration - END */
#endif
