/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef _WLAN_DP_SPM_H_
#define _WLAN_DP_SPM_H_

#include <qdf_status.h>
#include <qdf_nbuf.h>
#include "wlan_dp_main.h"

#define WLAN_DP_SPM_FLOW_REC_TBL_MAX 64
#define WLAN_DP_SPM_INVALID_FLOW_ID 0xFF
#define WLAN_DP_SPM_MAX_SERVICE_CLASS_SUPPORT 32
#define WLAN_DP_SPM_INVALID_METADATA 0xFF

/* Timeout in nano seconds */
#define WLAN_DP_SPM_FLOW_RETIREMENT_TIMEOUT 10000000000
#define WLAN_DP_SPM_LOW_AVAILABLE_FLOWS_WATERMARK 5

/**
 * enum wlan_dp_spm_event_type - SPM event type
 * @WLAN_DP_SPM_EVENT_ACTIVE_FLOW_ADD: Addition of a new active flow
 * @WLAN_DP_SPM_EVENT_ACTIVE_FLOW_RETIRE: Flow retiring from active flow tables
 *                                   operation
 * @WLAN_DP_SPM_EVENT_POLICY_ADD: Addition of a new policy
 * @WLAN_DP_SPM_EVENT_POLICY_DELETE: Deletion of policy
 * @WLAN_DP_SPM_EVENT_SERVICE_MAP: Map event from FW for service
 * @WLAN_DP_SPM_EVENT_SERVICE_DELETE: Deletion of service
 */
enum wlan_dp_spm_event_type {
	WLAN_DP_SPM_EVENT_ACTIVE_FLOW_ADD,
	WLAN_DP_SPM_EVENT_ACTIVE_FLOW_RETIRE,
	WLAN_DP_SPM_EVENT_POLICY_ADD,
	WLAN_DP_SPM_EVENT_POLICY_DELETE,
	WLAN_DP_SPM_EVENT_SERVICE_MAP,
	WLAN_DP_SPM_EVENT_SERVICE_DELETE,
};

/**
 * struct wlan_dp_spm_event - Service policy manager workqueue event structure
 * @node: node used for list operations
 * @type: Operation type
 * @data: Pointer to event data
 */
struct wlan_dp_spm_event {
	qdf_list_node_t node;
	enum wlan_dp_spm_event_type type;
	void *data;
};

/**
 * struct wlan_dp_spm_flow_tbl_stats - Flow tables statistics
 * @total: Total flows tracked count
 * @active: Current active flows count
 * @deleted: Number of flows deleted
 * @mapped: Number of flows currently mapped
 */
struct wlan_dp_spm_flow_tbl_stats {
	uint32_t total;
	uint32_t active;
	uint32_t deleted;
	uint32_t mapped;
};

/**
 * struct wlan_dp_spm_flow_info - Record of flow which will be tracked
 * @node: List node
 * @id: Flow ID
 * @is_populated: Is flow valid
 * @info: Flow details
 * @is_ipv4: Is flow IPV4
 * @guid: Global unique identifier
 * @peer_id: Peer ID
 * @vdev_id: Vdev ID
 * @svc_id: Service ID
 * @svc_metadata: Service metadata
 * @cookie: sock address or skb hash
 * @flags: flow flags
 * @flow_add_ts: Flow add timestamp
 * @active_ts: last active timestamp
 * @c_flow_id: STC classification table ID
 * @track_flow_stats: is stats tracking enabled for flow
 * @selected_to_sample: Selected for classification stats collection
 * @classified: Classification done
 * @reserved: unused
 * @flow_tuple_hash: flow_tuple_hash to identify bi-directional flow
 */
struct wlan_dp_spm_flow_info {
	qdf_list_node_t node;
	uint16_t id;
	bool is_populated;
	struct flow_info info;
	bool is_ipv4;
	uint64_t guid;
	uint16_t peer_id;
	uint8_t vdev_id;
	uint16_t svc_id;
	uint8_t svc_metadata;
	uint64_t cookie;
	uint32_t flags;
	uint64_t flow_add_ts;
	uint64_t active_ts;
#ifdef WLAN_DP_FEATURE_STC
	uint8_t c_flow_id;
	uint8_t track_flow_stats;
	uint8_t selected_to_sample;
	uint8_t classified;
	uint64_t flow_tuple_hash;
#endif
};

/**
 * struct wlan_dp_spm_intf_context - SPM context per dp interface
 * @origin_aft: Active flow table for originating traffic
 * @flow_rec_base: Flow records base address
 * @o_flow_rec_freelist: Flow records freelist
 * @flow_list_lock: Flow list operation lock
 * @o_stats: Flow table stats for originating traffic
 */
struct wlan_dp_spm_intf_context {
	struct wlan_dp_spm_flow_info *origin_aft[WLAN_DP_SPM_FLOW_REC_TBL_MAX];
	struct wlan_dp_spm_flow_info *flow_rec_base;
	qdf_list_t o_flow_rec_freelist;
	qdf_spinlock_t flow_list_lock;
	struct wlan_dp_spm_flow_tbl_stats o_stats;
};

/**
 * struct wlan_dp_spm_policy_info: Policy information
 * @node: List node
 * @flow: Policy flow details
 * @id: policy ID
 * @is_5tuple: 2 tuple or 5 tuple
 * @svc_id: Service ID for this policy
 * @flows_attached: Number of flows attached
 */
struct wlan_dp_spm_policy_info {
	qdf_list_node_t node;
	struct flow_info flow;
	uint64_t id;
	bool is_5tuple;
	uint16_t svc_id;
	uint16_t flows_attached;
};

/**
 * struct wlan_dp_spm_svc_queue_info - Metadata of msdu queue per tid per peer
 * @node: List node
 * @peer_id: peer ID
 * @metadata: Queue metadata per peer
 */
struct wlan_dp_spm_svc_queue_info {
	qdf_list_node_t node;
	uint16_t peer_id;
	uint8_t metadata;
};

/**
 * struct wlan_dp_spm_svc_class - Service class information
 * @id: Service id
 * @tid: Traffic identifier
 * @msdu_loss_rate: Loss rate per ppm of this flow
 * @policy_list: list of Policies mapped to the service
 * @queue_info: MSDU queue metadata for the service
 */
struct wlan_dp_spm_svc_class {
	uint16_t id;
	uint8_t tid;
	uint32_t msdu_loss_rate;
	qdf_list_t policy_list;
	/* TBD: Converted to qdf_list_t when concurrency is supported. */
	struct wlan_dp_spm_svc_queue_info queue_info;
};

/**
 * struct wlan_dp_spm_context - SPM context
 * @svc_class_db: Service database
 * @max_supported_svc_class: Supported service classes
 * @spm_wq: SPM Workqueue
 * @spm_work: Work context of SPM
 * @evt_list: Event list
 * @evt_lock: SPM Event list lock
 * @flow_guid_gen: Ever incrementing count to give unique ID to flows
 * @spm_intf: Interface level SPM context containing flow tables
 * @svc_cnt: Number of Service classes present
 * @policy_cnt: Number of policies attached
 */
struct wlan_dp_spm_context {
	struct wlan_dp_spm_svc_class **svc_class_db;
	uint32_t max_supported_svc_class;
	qdf_workqueue_t *spm_wq;
	qdf_work_t spm_work;
	qdf_list_t evt_list;
	qdf_spinlock_t evt_lock;
	/* TBD: Converted to TAILQ when concurrency is supported. */
	struct wlan_dp_spm_intf_context *spm_intf;
	uint32_t svc_cnt;
	uint32_t policy_cnt;
};

/**
 * wlan_dp_spm_svc_get_metadata() - Get service metadata for flow:
 * @dp_intf: DP interface
 * @nbuf: Nbuf Packet
 * @flow_id: Flow ID
 * @cookie: sock address or hash value depending on traffic
 *
 * Return: Service metadata for the flow
 */
uint16_t wlan_dp_spm_svc_get_metadata(struct wlan_dp_intf *dp_intf,
				      qdf_nbuf_t nbuf, uint16_t flow_id,
				      uint64_t cookie);

#ifdef WLAN_FEATURE_SAWFISH
/**
 * wlan_dp_spm_ctx_init() - Initialize SPM context
 * @dp_ctx: PSOC DP context
 * @queues_per_tid: Number of queues per tid
 * @num_tids: number of TIDs
 *
 * Return: Success if initialized successfully
 */
QDF_STATUS wlan_dp_spm_ctx_init(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t queues_per_tid, uint32_t num_tids);

/**
 * wlan_dp_spm_ctx_deinit(): Deinitialize SPM context
 * @dp_ctx: PSOC DP context
 *
 * Return: None
 */
void wlan_dp_spm_ctx_deinit(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_spm_svc_class_create(): Create new service class and attach to
 *                                 database
 * @data: Service class parameters
 *
 * Return: Success if created, else failure code
 */
QDF_STATUS wlan_dp_spm_svc_class_create(struct dp_svc_data *data);

/**
 * wlan_dp_spm_svc_class_delete(): Delete service class and detach from database
 * @svc_id: Service class ID
 *
 * Return: None
 */
void wlan_dp_spm_svc_class_delete(uint32_t svc_id);

/**
 * wlan_dp_spm_svc_get(): Send service info to middleware
 * @svc_id: If specific svc id is required
 * @svc_table: service table to be filled
 * @table_size: Table size
 *
 * Return: Number of services being sent
 */
uint8_t wlan_dp_spm_svc_get(uint8_t svc_id, struct dp_svc_data *svc_table,
			    uint16_t table_size);

/**
 * wlan_dp_spm_svc_set_queue_info(): Set queue info for service class
 * @msg_word: Pointer to htt msg word.
 * @htt_t2h_msg: HTT message nbuf
 *
 * Return: None
 */
void wlan_dp_spm_svc_set_queue_info(uint32_t *msg_word, qdf_nbuf_t htt_t2h_msg);

/**
 * wlan_dp_spm_policy_add(): Add policy to a service
 * @policy: Policy parameters
 *
 * Return: Success if added, else failure code
 */
QDF_STATUS wlan_dp_spm_policy_add(struct dp_policy *policy);

/**
 * wlan_dp_spm_policy_delete(): Delete policy to a service
 * @policy_id: Policy ID to be deleted
 *
 * Return: Success if deleted, else failure code
 */
QDF_STATUS wlan_dp_spm_policy_delete(uint32_t policy_id);

/**
 * wlan_dp_spm_policy_update(): Update an existing policy
 * @policy: Policy parameters
 *
 * Return: Success if updated, else failure code
 */
QDF_STATUS wlan_dp_spm_policy_update(struct dp_policy *policy);
#else
static inline
QDF_STATUS wlan_dp_spm_ctx_init(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t queues_per_tid, uint32_t num_tids)
{
	return QDF_STATUS_SUCCESS;
}

static inline
QDF_STATUS wlan_spm_ctx_deinit(struct wlan_dp_psoc_context *dp_ctx)
{
	return QDF_STATUS_SUCCESS;
}

static inline
QDF_STATUS dp_spm_svc_class_create(struct dp_svc_data *data)
{
	return QDF_STATUS_SUCCESS;
}

static inline
void wlan_dp_spm_svc_class_delete(uint32_t svc_id)
{
}

static inline
uint8_t wlan_dp_spm_svc_get(uint8_t svc_id, struct dp_svc_data *svc_table,
			    uint16_t table_size)
{
	return 0;
}

static inline
void wlan_dp_spm_svc_set_queue_info(uint32_t *msg_word, qdf_nbuf_t htt_t2h_msg)
{
}

static inline
QDF_STATUS wlan_dp_spm_policy_add(struct dp_policy *policy)
{
	return QDF_STATUS_SUCCESS;
}

static inline
QDF_STATUS wlan_dp_spm_policy_delete(uint32_t policy_id)
{
	return QDF_STATUS_SUCCESS;
}

static inline
QDF_STATUS wlan_dp_spm_policy_update(struct dp_policy *policy)
{
	return QDF_STATUS_SUCCESS;
}
#endif

#if defined(WLAN_FEATURE_SAWFISH) || defined(WLAN_DP_FEATURE_STC)
/**
 * wlan_dp_spm_flow_table_attach() - SPM flow table attach
 * @dp_ctx: Global DP component context
 *
 * Return: None
 */
void wlan_dp_spm_flow_table_attach(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_spm_flow_table_detach() - SPM flow table detach
 * @dp_ctx: Global DP component context
 *
 * Return: None
 */
void wlan_dp_spm_flow_table_detach(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_spm_intf_ctx_init(): Initialize per interface SPM context
 * @dp_intf: DP interface
 *
 * Return: Success if initialized, else failure code
 */
QDF_STATUS wlan_dp_spm_intf_ctx_init(struct wlan_dp_intf *dp_intf);

/**
 * wlan_dp_spm_intf_ctx_deinit(): De-initialize per interface SPM context
 * @dp_intf: DP interface
 *
 * Return: None
 */
void wlan_dp_spm_intf_ctx_deinit(struct wlan_dp_intf *dp_intf);

/**
 * wlan_dp_spm_get_flow_id_origin() - Get flow ID for a new flow
 * @dp_intf: DP interface
 * @flow_id: ID place holder.
 * @flow_info: Flow related information
 * @cookie_sk: Sock structure pointer to be used as cookie
 * @peer_id: Peer ID
 *
 * Return: Success if flow is successfully added to flow table
 */
QDF_STATUS wlan_dp_spm_get_flow_id_origin(struct wlan_dp_intf *dp_intf,
					  uint16_t *flow_id,
					  struct flow_info *flow_info,
					  uint64_t cookie_sk, uint16_t peer_id);

/**
 * wlan_dp_spm_set_flow_active(): Set active ts for a flow
 * @spm_intf: SPM interface
 * @flow_id: Flow ID
 * @flow_guid: Flow Unique ID
 *
 * Return: None
 */
void wlan_dp_spm_set_flow_active(struct wlan_dp_spm_intf_context *spm_intf,
				 uint16_t flow_id, uint64_t flow_guid);
#else
static inline void
wlan_dp_spm_flow_table_attach(struct wlan_dp_psoc_context *dp_ctx)
{
}

static inline void
wlan_dp_spm_flow_table_detach(struct wlan_dp_psoc_context *dp_ctx)
{
}

static inline
QDF_STATUS wlan_dp_spm_intf_ctx_init(struct wlan_dp_intf *dp_intf)
{
	return QDF_STATUS_SUCCESS;
}

static inline
void wlan_dp_spm_intf_ctx_deinit(struct wlan_dp_intf *dp_intf)
{
}

static inline
QDF_STATUS wlan_dp_spm_get_flow_id_origin(struct wlan_dp_intf *dp_intf,
					  uint16_t *flow_id,
					  struct flow_info *flow_info,
					  uint64_t cookie_sk, uint16_t peer_id)
{
	return QDF_STATUS_SUCCESS;
}

static inline
void wlan_dp_spm_set_flow_active(struct wlan_dp_spm_intf_context *spm_intf,
				 uint16_t flow_id, uint64_t flow_guid)
{
}
#endif
#endif
