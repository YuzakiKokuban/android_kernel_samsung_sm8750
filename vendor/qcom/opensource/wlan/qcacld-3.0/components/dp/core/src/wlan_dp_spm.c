/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#include <dp_types.h>
#include <dp_internal.h>
#include <wlan_dp_spm.h>
#include "dp_htt.h"
#include "wlan_dp_fim.h"
#include "wlan_dp_stc.h"

#define DP_SPM_FLOW_FLAG_IN_USE         BIT(0)
#define DP_SPM_FLOW_FLAG_SVC_METADATA   BIT(1)

#ifdef WLAN_FEATURE_SAWFISH
/**
 * wlan_dp_spm_get_context(): Get SPM context from DP PSOC interface
 *
 * Return: SPM context handle
 */
static inline
struct wlan_dp_spm_context *wlan_dp_spm_get_context(void)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();

	return dp_ctx ? dp_ctx->spm_ctx : NULL;
}

/**
 * wlan_dp_spm_update_svc_metadata(): Update service metadata for a service
 *                                    class
 * @svc_class: Service class for which metadata is required
 * @peer_id: Peer ID
 *
 * Return: Service metadata
 */
static inline uint8_t
wlan_dp_spm_update_svc_metadata(struct wlan_dp_spm_svc_class *svc_class,
				uint16_t peer_id)
{
	/**
	 * TBD: Will be implemented when message interface is set for SAWFISH
	 * (peer_id != svc_class->queue_info.peer_id ||
	 *   !svc_class->queue_info.metadata) {
	 * dp_htt_send_svc_map_msg()
	 */

	return WLAN_DP_SPM_INVALID_METADATA;
}

/**
 * wlan_dp_spm_match_flow_to_policy(): Match flow to a policy's parameters
 * @flow: Flow to be matched
 * @policy: Policy which needs to be matched with
 *
 * Return: True/False
 */
static bool wlan_dp_spm_match_flow_to_policy(struct flow_info *flow,
					struct wlan_dp_spm_policy_info *policy)
{
	struct flow_info *policy_flow = &policy->flow;

	if (flow->flags & DP_FLOW_TUPLE_FLAGS_IPV4) {
		if (flow->flags & DP_FLOW_TUPLE_FLAGS_SRC_IP)
			if (flow->src_ip.ipv4_addr !=
				policy_flow->src_ip.ipv4_addr)
				return false;
		if (flow->flags & DP_FLOW_TUPLE_FLAGS_DST_IP)
			if (flow->dst_ip.ipv4_addr !=
				policy_flow->dst_ip.ipv4_addr)
				return false;
	} else if (flow->flags & DP_FLOW_TUPLE_FLAGS_IPV6) {
		if (flow->flags & DP_FLOW_TUPLE_FLAGS_SRC_IP)
			if (qdf_mem_cmp(flow->src_ip.ipv6_addr,
					policy_flow->src_ip.ipv6_addr,
					sizeof(struct in6_addr)))
				return false;
		if (flow->flags & DP_FLOW_TUPLE_FLAGS_DST_IP)
			if (qdf_mem_cmp(flow->dst_ip.ipv6_addr,
					policy_flow->dst_ip.ipv6_addr,
					sizeof(struct in6_addr)))
				return false;
	}

	if (!policy->is_5tuple)
		return true;

	if (flow->flags & DP_FLOW_TUPLE_FLAGS_PROTO)
		if (flow->proto != policy_flow->proto)
			return false;

	if (flow->flags & DP_FLOW_TUPLE_FLAGS_SRC_PORT)
		if (flow->src_port != policy_flow->src_port)
			return false;
	if (flow->flags & DP_FLOW_TUPLE_FLAGS_DST_PORT)
		if (flow->dst_port != policy_flow->dst_port)
			return false;

	return true;
}

/**
 * wlan_dp_spm_flow_get_service(): Find service for an incoming flow
 * @new_flow: Flow info
 *
 * Return: None
 */
static void wlan_dp_spm_flow_get_service(struct wlan_dp_spm_flow_info *new_flow)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_svc_class **svc_class;
	struct wlan_dp_spm_policy_info *policy;
	int i;

	svc_class = spm_ctx->svc_class_db;
	for (i = 0; i < spm_ctx->max_supported_svc_class; i++) {
		if (!svc_class[i] || !svc_class[i]->policy_list.count)
			continue;

		qdf_list_for_each(&svc_class[i]->policy_list, policy, node) {
			if (wlan_dp_spm_match_flow_to_policy(&new_flow->info,
							     policy)) {
				new_flow->svc_metadata =
				wlan_dp_spm_update_svc_metadata(svc_class[i],
							new_flow->peer_id);
				/* If svc metadata is not present, this loop
				 * will be repeated again, hence skipping here
				 */
				if (new_flow->svc_metadata ==
						WLAN_DP_SPM_INVALID_METADATA)
					return;
				new_flow->flags |=
						DP_SPM_FLOW_FLAG_SVC_METADATA;
				new_flow->svc_id = i;
				policy->flows_attached++;
				return;
			}
		}
	}
}

/**
 * wlan_dp_spm_unmap_svc_to_flow(): Unmap flow from a service
 * @svc_id: Service ID
 * @flow: Flow info to be checked against policies
 *
 * Return: None
 */
static inline
void wlan_dp_spm_unmap_svc_to_flow(uint16_t svc_id, struct flow_info *flow)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_svc_class *svc_class = spm_ctx->svc_class_db[svc_id];
	struct wlan_dp_spm_policy_info *policy;

	if (svc_id == WLAN_DP_SPM_INVALID_METADATA)
		return;

	qdf_list_for_each(&svc_class->policy_list, policy, node) {
		if (wlan_dp_spm_match_flow_to_policy(flow, policy)) {
			policy->flows_attached--;
			return;
		}
	}
}

/**
 * wlan_dp_spm_flow_retire(): Retire flows in active flow table
 * @spm_intf: SPM interface
 * @clear_tbl: Set to clear the entire table
 *
 * Return: None
 */
static void wlan_dp_spm_flow_retire(struct wlan_dp_spm_intf_context *spm_intf,
				    bool clear_tbl)
{
	struct wlan_dp_spm_flow_info *cursor;
	uint64_t curr_ts = qdf_sched_clock();
	int i;

	qdf_spinlock_acquire(&spm_intf->flow_list_lock);
	for (i = 0; i < WLAN_DP_SPM_FLOW_REC_TBL_MAX; i++, cursor++) {
		cursor = spm_intf->origin_aft[i];
		if (!cursor)
			continue;

		if (clear_tbl) {
			qdf_list_insert_back(&spm_intf->o_flow_rec_freelist,
					     &cursor->node);
		} else if ((curr_ts - cursor->active_ts) >
				WLAN_DP_SPM_FLOW_RETIREMENT_TIMEOUT) {
			wlan_dp_spm_unmap_svc_to_flow(cursor->svc_id,
						      &cursor->info);
			qdf_mem_zero(cursor,
				     sizeof(struct wlan_dp_spm_flow_info));
			cursor->id = i;
			qdf_list_insert_back(&spm_intf->o_flow_rec_freelist,
					     &cursor->node);
			spm_intf->o_stats.active--;
			spm_intf->o_stats.deleted++;
		}
	}
	qdf_spinlock_release(&spm_intf->flow_list_lock);
}

/**
 * wlan_dp_spm_match_policy_to_active_flows(): Match policy with active flows
 * @policy: Policy parameters
 * @is_add: Indicates if policy is being added or deleted
 *
 * Return: None
 */
static void
wlan_dp_spm_match_policy_to_active_flows(struct wlan_dp_spm_policy_info *policy,
					 bool is_add)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_flow_info **o_aft;
	int i;

	if (!spm_ctx || !spm_ctx->spm_intf) {
		dp_info("Feature not supported");
		return;
	}

	o_aft = spm_ctx->spm_intf->origin_aft;

	for (i = 0; i < WLAN_DP_SPM_FLOW_REC_TBL_MAX; i++) {
		if (!o_aft[i])
			continue;

		if (wlan_dp_spm_match_flow_to_policy(&o_aft[i]->info, policy)) {
			if (is_add) {
				o_aft[i]->svc_metadata =
				wlan_dp_spm_update_svc_metadata(
					spm_ctx->svc_class_db[policy->svc_id],
					o_aft[i]->peer_id);
				/* If svc metadata is not present, this loop
				 * will be repeated again, hence skipping here
				 */
				if (o_aft[i]->svc_metadata ==
						WLAN_DP_SPM_INVALID_METADATA)
					return;
				o_aft[i]->flags |=
						DP_SPM_FLOW_FLAG_SVC_METADATA;
				o_aft[i]->svc_id = policy->svc_id;
				policy->flows_attached++;
			} else {
				o_aft[i]->svc_id =
						WLAN_DP_SPM_INVALID_METADATA;
				o_aft[i]->svc_metadata =
						WLAN_DP_SPM_INVALID_METADATA;
				policy->flows_attached--;
			}
		}
	}
}

/**
 * wlan_dp_spm_svc_map(): Map flows to a queue when service is attached to a
 *                        queue
 * @svc_class: Service class
 *
 * Return: None
 */
void wlan_dp_spm_svc_map(struct wlan_dp_spm_svc_class *svc_class)
{
	struct wlan_dp_spm_policy_info *policy;

	qdf_list_for_each(&svc_class->policy_list, policy, node) {
		wlan_dp_spm_match_policy_to_active_flows(policy, true);
	}
}

/**
 * wlan_dp_spm_svc_delete(): Delete service and unmap all attached flows
 * @svc_class: Service class
 *
 * Return: None
 */
void wlan_dp_spm_svc_delete(struct wlan_dp_spm_svc_class *svc_class)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_policy_info *policy;

	if (!spm_ctx) {
		dp_info("Feature not supported");
		return;
	}

	while ((policy = qdf_list_first_entry_or_null(&svc_class->policy_list,
						struct wlan_dp_spm_policy_info,
						node))) {
		qdf_list_remove_node(&svc_class->policy_list, &policy->node);
		wlan_dp_spm_match_policy_to_active_flows(policy, false);
	}
	qdf_list_destroy(&svc_class->policy_list);

	/**
	 * TBD: Will be implemented when message interface is set for SAWFISH
	 * if (svc_class->queue_info.metadata != WLAN_DP_SPM_INVALID_METADATA) {
	 * dp_htt_send_svc_map_msg()
	 */

	dp_info("Deleted svc class %u: TID: %u MSDU loss rate %u Policies attached %u Metadata: %u",
		svc_class->id, svc_class->tid, svc_class->msdu_loss_rate,
		svc_class->policy_list.count, svc_class->queue_info.metadata);

	spm_ctx->svc_class_db[svc_class->id] = NULL;
	qdf_mem_free(svc_class);
}

/**
 * wlan_dp_spm_work_handler(): SPM work handler
 * @arg: NULL
 *
 * Return: None
 */
static void wlan_dp_spm_work_handler(void *arg)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_spm_context *spm_ctx;
	struct wlan_dp_spm_event *evt;

	if (!dp_ctx || dp_ctx->spm_ctx) {
		dp_info("Feature not supported");
		return;
	}

	spm_ctx = dp_ctx->spm_ctx;

	qdf_spinlock_acquire(&spm_ctx->evt_lock);
	while ((evt = qdf_list_first_entry_or_null(&spm_ctx->evt_list,
						   struct wlan_dp_spm_event,
						   node))) {
		qdf_list_remove_node(&spm_ctx->evt_list, &evt->node);
		qdf_spinlock_release(&spm_ctx->evt_lock);

		switch (evt->type) {
		case WLAN_DP_SPM_EVENT_ACTIVE_FLOW_ADD:
			wlan_dp_spm_flow_get_service(
				(struct wlan_dp_spm_flow_info *)evt->data);
			break;
		case WLAN_DP_SPM_EVENT_ACTIVE_FLOW_RETIRE:
			wlan_dp_spm_flow_retire(
			  (struct wlan_dp_spm_intf_context *)evt->data, false);
			break;
		case WLAN_DP_SPM_EVENT_POLICY_ADD:
			wlan_dp_spm_match_policy_to_active_flows(
			    (struct wlan_dp_spm_policy_info *)evt->data, true);
			break;
		case WLAN_DP_SPM_EVENT_POLICY_DELETE:
			wlan_dp_spm_match_policy_to_active_flows(
			   (struct wlan_dp_spm_policy_info *)evt->data, false);
			qdf_mem_free(evt->data);
			break;
		case WLAN_DP_SPM_EVENT_SERVICE_MAP:
			wlan_dp_spm_svc_map(
				(struct wlan_dp_spm_svc_class *)evt->data);
			break;
		case WLAN_DP_SPM_EVENT_SERVICE_DELETE:
			wlan_dp_spm_svc_delete(
				(struct wlan_dp_spm_svc_class *)evt->data);
			break;
		default:
			dp_info("Unknown %u event type", evt->type);
		}
		qdf_mem_free(evt);
		qdf_spinlock_acquire(&spm_ctx->evt_lock);
	}
	qdf_spinlock_release(&spm_ctx->evt_lock);
}

/**
 * wlan_dp_spm_event_post(): Post event to SPM work
 * @type: Event type
 * @data: Related Data
 *
 * Return: none
 */
static inline
void wlan_dp_spm_event_post(enum wlan_dp_spm_event_type type, void *data)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_event *evt = qdf_mem_malloc(sizeof(*evt));

	if (!evt) {
		dp_err("unable to alloc mem for type %u", type);
		return;
	}

	evt->type = type;
	evt->data = data;

	qdf_spinlock_acquire(&spm_ctx->evt_lock);
	qdf_list_insert_back(&spm_ctx->evt_list, &evt->node);
	qdf_spinlock_release(&spm_ctx->evt_lock);

	qdf_queue_work(0, spm_ctx->spm_wq, &spm_ctx->spm_work);
}

QDF_STATUS wlan_dp_spm_ctx_init(struct wlan_dp_psoc_context *dp_ctx,
				uint32_t queues_per_tid, uint32_t num_tids)
{
	QDF_STATUS status;
	struct wlan_dp_spm_context *ctx;

	if (dp_ctx->spm_ctx) {
		dp_err("SPM module already initialized");
		return QDF_STATUS_E_ALREADY;
	}

	ctx = (struct wlan_dp_spm_context *)qdf_mem_malloc(sizeof(*ctx));
	if (!ctx) {
		dp_err("Unable to allocate spm ctx");
		goto fail;
	}

	/* Move this below creation of DB to WMI ready msg or new HTT msg */
	ctx->max_supported_svc_class = queues_per_tid * num_tids;

	if (ctx->max_supported_svc_class >
		WLAN_DP_SPM_MAX_SERVICE_CLASS_SUPPORT) {
		dp_err("Wrong config recvd queues per tid: %u, num tids: %u",
		       queues_per_tid, num_tids);
		ctx->max_supported_svc_class =
					WLAN_DP_SPM_MAX_SERVICE_CLASS_SUPPORT;
	}

	ctx->svc_class_db = (struct wlan_dp_spm_svc_class **)
			qdf_mem_malloc(sizeof(struct wlan_dp_spm_svc_class *) *
				       queues_per_tid * num_tids);
	if (!ctx->svc_class_db) {
		dp_err("Unable to allocate SPM service class database");
		goto fail_svc_db_alloc;
	}

	qdf_list_create(&ctx->evt_list, 0);

	status = qdf_create_work(NULL, &ctx->spm_work,
				 wlan_dp_spm_work_handler, NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Work creation failed");
		goto fail_work_creation;
	}

	ctx->spm_wq = qdf_alloc_unbound_workqueue("spm_wq");
	if (!ctx->spm_wq) {
		dp_err("SPM Workqueue allocation failed");
		goto fail_workqueue_alloc;
	}

	dp_ctx->spm_ctx = ctx;
	dp_info("Initialized SPM context!");

	return QDF_STATUS_SUCCESS;

fail_workqueue_alloc:
	qdf_destroy_work(NULL, &ctx->spm_work);

fail_work_creation:
	qdf_mem_free(ctx->svc_class_db);

fail_svc_db_alloc:
	qdf_mem_free(ctx);

fail:
	return QDF_STATUS_E_FAILURE;
}

void wlan_dp_spm_ctx_deinit(struct wlan_dp_psoc_context *dp_ctx)
{
	struct wlan_dp_spm_context *ctx;
	struct wlan_dp_spm_svc_class **svc_class;
	int i;

	if (!dp_ctx->spm_ctx) {
		dp_err("SPM module not present!");
		return;
	}

	ctx = dp_ctx->spm_ctx;

	if (ctx->spm_intf)
		wlan_dp_spm_intf_ctx_deinit(NULL);

	svc_class = ctx->svc_class_db;
	for (i = 0; i < ctx->max_supported_svc_class; i++) {
		if (svc_class[i])
			wlan_dp_spm_svc_class_delete(i);
	}

	qdf_flush_workqueue(NULL, ctx->spm_wq);
	qdf_destroy_workqueue(NULL, ctx->spm_wq);
	qdf_flush_work(&ctx->spm_work);
	qdf_destroy_work(NULL, &ctx->spm_work);
	qdf_mem_free(ctx);
	dp_ctx->spm_ctx = NULL;
	dp_info("Deinitialized SPM context!");
}

QDF_STATUS wlan_dp_spm_svc_class_create(struct dp_svc_data *data)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_svc_class *svc_class_new;

	if (!spm_ctx) {
		dp_info("Feature not supported");
		return QDF_STATUS_E_NOSUPPORT;
	}

	if (data->svc_id > spm_ctx->max_supported_svc_class) {
		dp_err("Invalid svc ID %u", data->svc_id);
		return QDF_STATUS_E_FAILURE;
	}

	if (spm_ctx->svc_class_db[data->svc_id]) {
		dp_err("Svc already exists for id: %u", data->svc_id);
		return QDF_STATUS_E_ALREADY;
	}

	svc_class_new = (struct wlan_dp_spm_svc_class *)
				qdf_mem_malloc(sizeof(*svc_class_new));
	if (!svc_class_new) {
		dp_err("Unable to allocate svc class");
		return QDF_STATUS_E_NOMEM;
	}

	svc_class_new->id = data->svc_id;
	svc_class_new->tid = data->tid;
	svc_class_new->msdu_loss_rate = data->msdu_loss_rate;
	qdf_list_create(&svc_class_new->policy_list, 0);

	spm_ctx->svc_class_db[data->svc_id] = svc_class_new;
	/* TBD: send service class WMI cmd to FW */

	dp_info("New service class %u: TID: %u, MSDU loss rate: %u",
		svc_class_new->id, svc_class_new->tid,
		svc_class_new->msdu_loss_rate);

	return QDF_STATUS_SUCCESS;
}

void wlan_dp_spm_svc_class_delete(uint32_t svc_id)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_svc_class *svc_class;

	if (!spm_ctx) {
		dp_info("Feature not supported");
		return;
	}

	svc_class = spm_ctx->svc_class_db[svc_id];
	if (!spm_ctx->svc_class_db[svc_id]) {
		dp_err("Service %u not present", svc_id);
		return;
	}

	if (!svc_class->policy_list.count) {
		dp_info("No policies attached for service: %u", svc_id);
	/**
	 * TBD: Will be implemented when message interface is set for SAWFISH
	 * if (svc_class->queue_info.metadata != WLAN_DP_SPM_INVALID_METADATA) {
	 * dp_htt_send_svc_map_msg()
	 */
		qdf_mem_free(svc_class);
		spm_ctx->svc_class_db[svc_id] = NULL;
		dp_info("Deleted svc class %u: TID: %u MSDU loss rate %u Policies attached %u Metadata: %u",
			svc_class->id, svc_class->tid,
			svc_class->msdu_loss_rate,
			svc_class->policy_list.count,
			svc_class->queue_info.metadata);
	} else {
		wlan_dp_spm_event_post(WLAN_DP_SPM_EVENT_SERVICE_DELETE,
				       (void *)svc_class);
	}
}

uint8_t wlan_dp_spm_svc_get(uint8_t svc_id, struct dp_svc_data *svc_table,
			    uint16_t table_size)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_svc_class *svc_class = NULL;
	int i = 0;

	if (!spm_ctx) {
		dp_info("Feature not supported");
		return i;
	}

	if (svc_id != WLAN_DP_SPM_INVALID_METADATA) {
		if (svc_id < spm_ctx->max_supported_svc_class) {
			svc_class = spm_ctx->svc_class_db[svc_id];
			svc_table[0].svc_id = svc_class->id;
			svc_table[0].tid = svc_class->tid;
			svc_table[0].msdu_loss_rate = svc_class->msdu_loss_rate;
			i++;
		}
	} else {
		for (i = 0; i < spm_ctx->max_supported_svc_class; i++) {
			if (spm_ctx->svc_class_db[svc_id]) {
				svc_class = spm_ctx->svc_class_db[svc_id];
				svc_table[i].svc_id = svc_class->id;
				svc_table[i].tid = svc_class->tid;
				svc_table[i].msdu_loss_rate =
						svc_class->msdu_loss_rate;
			}
		}
	}

	return i;
}

void wlan_dp_spm_svc_set_queue_info(uint32_t *msg_word, qdf_nbuf_t htt_t2h_msg)
{
	/* TBD: Will be implemented when message interface is set for SAWFISH */
}

uint16_t wlan_dp_spm_svc_get_metadata(struct wlan_dp_intf *dp_intf,
				      qdf_nbuf_t skb, uint16_t flow_id,
				      uint64_t cookie)
{
	struct wlan_dp_spm_context *spm_ctx = dp_intf->spm_intf_ctx;
	struct wlan_dp_spm_flow_info *flow;

	flow = spm_intf->origin_aft[flow_id];
	/* Flow can be NULL when evicted or retired */
	if (qdf_unlikely(!flow))
		return WLAN_DP_SPM_FLOW_REC_TBL_MAX;

	if (qdf_unlikely(flow->cookie != cookie)) {
		dp_info("Flow cookie %lu mismatch against table %lu", cookie,
			flow->cookie);
		return QDF_STATUS_E_INVAL;
	}

	flow->active_ts = qdf_sched_clock();
	skb->mark = flow->svc_metadata;

	wlan_dp_stc_check_n_track_tx_flow_features(dp_intf->dp_ctx, skb,
						   flow->track_flow_stats,
						   flow->id,
						   dp_intf->def_link->link_id,
						   flow->peer_id, flow->guid);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_dp_spm_policy_add(struct dp_policy *policy)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_svc_class *svc_class;
	struct wlan_dp_spm_policy_info *new_policy;

	if (!spm_ctx) {
		dp_info("Feature not supported");
		return QDF_STATUS_E_NOSUPPORT;
	}

	if (!spm_ctx->svc_class_db[policy->svc_id]) {
		dp_info("Service class not present");
		return QDF_STATUS_E_EMPTY;
	}
	svc_class = spm_ctx->svc_class_db[policy->svc_id];

	new_policy = (struct wlan_dp_spm_policy_info *)
			qdf_mem_malloc(sizeof(*new_policy));
	if (!new_policy) {
		dp_info("Unable to allocate policy");
		return QDF_STATUS_E_NOMEM;
	}

	qdf_mem_copy(&new_policy->flow, &policy->flow,
		     sizeof(struct flow_info));
	new_policy->id = policy->policy_id;

	if ((policy->flags & DP_FLOW_TUPLE_FLAGS_SRC_PORT) &&
	    (policy->flags & DP_FLOW_TUPLE_FLAGS_DST_PORT))
		new_policy->is_5tuple = true;

	new_policy->svc_id = policy->svc_id;

	qdf_list_insert_back(&svc_class, &new_policy->node);

	wlan_dp_spm_event_post(WLAN_DP_SPM_EVENT_POLICY_ADD,
			       (void *)new_policy);

	spm_ctx->policy_cnt++;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_dp_spm_policy_delete(uint32_t policy_id)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_svc_class **svc_class;
	struct wlan_dp_spm_policy_info *policy, *delete_policy = NULL;
	int i;

	if (!spm_ctx) {
		dp_info("Feature not supported");
		return QDF_STATUS_E_NOSUPPORT;
	}

	svc_class = spm_ctx->svc_class_db;
	for (i = 0; i < spm_ctx->max_supported_svc_class; i++) {
		if (!svc_class[i] || !svc_class[i]->policy_list.count)
			continue;

		qdf_list_for_each(&svc_class[i]->policy_list, policy, node) {
			if (policy->id == policy_id) {
				qdf_list_remove_node(&svc_class[i]->policy_list,
						     &policy->node);
				delete_policy = policy;
				break;
			}
		}
	}

	dp_info("Deleting policy %u attached to svc id: %u", policy_id,
		delete_policy->svc_id);

	if (delete_policy) {
		if (delete_policy->flows_attached) {
			/* policy is deleted in the workqueue context*/
			wlan_dp_spm_event_post(WLAN_DP_SPM_EVENT_POLICY_DELETE,
					       (void *)delete_policy);
		} else {
			qdf_mem_free(delete_policy);
			delete_policy = NULL;
		}
	} else {
		return QDF_STATUS_E_INVAL;
	}

	spm_ctx->policy_cnt--;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_dp_spm_policy_update(struct dp_policy *policy)
{
	if (wlan_dp_spm_policy_delete(policy->policy_id))
		return wlan_dp_spm_policy_add(policy);
	else
		return QDF_STATUS_E_INVAL;
}
#else
/**
 * wlan_dp_spm_get_context(): Get SPM context from DP PSOC interface
 *
 * Return: SPM context handle
 */
static inline
struct wlan_dp_spm_context *wlan_dp_spm_get_context(void)
{
	return NULL;
}

static inline
void wlan_dp_spm_event_post(enum wlan_dp_spm_event_type type, void *data)
{
}

static void wlan_dp_spm_flow_retire(struct wlan_dp_spm_intf_context *spm_intf,
				    bool clear_tbl)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_get_context();
	struct wlan_dp_spm_flow_info *cursor;
	uint64_t curr_ts = qdf_sched_clock();
	int i;

	qdf_spinlock_acquire(&spm_intf->flow_list_lock);
	for (i = 0; i < WLAN_DP_SPM_FLOW_REC_TBL_MAX; i++, cursor++) {
		cursor = spm_intf->origin_aft[i];
		if (!cursor)
			continue;

		if (clear_tbl) {
			wlan_dp_stc_tx_flow_retire_ind(dp_ctx,
						       cursor->classified,
						       cursor->c_flow_id);
			qdf_mem_zero(cursor,
				     sizeof(struct wlan_dp_spm_flow_info));
			qdf_list_insert_back(&spm_intf->o_flow_rec_freelist,
					     &cursor->node);
			spm_intf->origin_aft[i] = NULL;
		} else if ((curr_ts - cursor->active_ts) >
				WLAN_DP_SPM_FLOW_RETIREMENT_TIMEOUT) {
			wlan_dp_stc_tx_flow_retire_ind(dp_ctx,
						       cursor->classified,
						       cursor->c_flow_id);
			qdf_mem_zero(cursor,
				     sizeof(struct wlan_dp_spm_flow_info));
			cursor->id = i;
			qdf_list_insert_back(&spm_intf->o_flow_rec_freelist,
					     &cursor->node);
			spm_intf->o_stats.active--;
			spm_intf->o_stats.deleted++;
			spm_intf->origin_aft[i] = NULL;
		}
	}
	qdf_spinlock_release(&spm_intf->flow_list_lock);
}

uint16_t wlan_dp_spm_svc_get_metadata(struct wlan_dp_intf *dp_intf,
				      qdf_nbuf_t nbuf, uint16_t flow_id,
				      uint64_t cookie)
{
	struct wlan_dp_spm_intf_context *spm_intf = dp_intf->spm_intf_ctx;
	struct wlan_dp_spm_flow_info *flow;

	flow = spm_intf->origin_aft[flow_id];
	/* Flow can be NULL when evicted or retired */
	if (qdf_unlikely(!flow))
		return WLAN_DP_SPM_FLOW_REC_TBL_MAX;

	if (qdf_unlikely(flow->cookie != cookie)) {
		dp_info("Flow cookie %lu mismatch against table %lu", cookie,
			flow->cookie);
		return WLAN_DP_SPM_FLOW_REC_TBL_MAX;
	}

	flow->active_ts = qdf_sched_clock();

	wlan_dp_stc_check_n_track_tx_flow_features(dp_intf->dp_ctx, nbuf,
						   flow->track_flow_stats,
						   flow->id,
						   dp_intf->def_link->link_id,
						   flow->peer_id, flow->guid);

	return QDF_STATUS_SUCCESS;
}
#endif

/* Flow Unique ID generator */
static uint32_t flow_guid_gen;

QDF_STATUS wlan_dp_spm_intf_ctx_init(struct wlan_dp_intf *dp_intf)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_psoc_context *dp_ctx = dp_intf->dp_ctx;
	struct wlan_dp_spm_intf_context *spm_intf;
	struct wlan_dp_spm_flow_info *flow_rec;
	int i;

	if (dp_intf->device_mode != QDF_STA_MODE)
		return QDF_STATUS_E_NOSUPPORT;

	if (dp_intf->spm_intf_ctx) {
		dp_info("Module already initialized!");
		return QDF_STATUS_E_ALREADY;
	}

	spm_intf = (struct wlan_dp_spm_intf_context *)
					qdf_mem_malloc(sizeof(*spm_intf));
	if (!spm_intf) {
		dp_err("Unable to allocate spm interface");
		goto fail_intf_alloc;
	}

	qdf_list_create(&spm_intf->o_flow_rec_freelist,
			WLAN_DP_SPM_FLOW_REC_TBL_MAX);
	qdf_spinlock_create(&spm_intf->flow_list_lock);

	if (!dp_ctx->gl_flow_recs)
		goto fail_flow_rec_freelist;

	spm_intf->flow_rec_base =
	     &dp_ctx->gl_flow_recs[dp_intf->id * WLAN_DP_SPM_FLOW_REC_TBL_MAX];
	if (!spm_intf->flow_rec_base) {
		dp_err("Unable to allocate origin freelist");
		goto fail_flow_rec_freelist;
	}

	flow_rec = spm_intf->flow_rec_base;
	for (i = 1; i < WLAN_DP_SPM_FLOW_REC_TBL_MAX; i++) {
		qdf_mem_zero(flow_rec, sizeof(struct wlan_dp_spm_flow_info));
		flow_rec->id = i;
		qdf_list_insert_back(&spm_intf->o_flow_rec_freelist,
				     &flow_rec->node);
		flow_rec++;
	}

	dp_intf->spm_intf_ctx = spm_intf;
	if (spm_ctx)
		spm_ctx->spm_intf = spm_intf;
	dp_info("SPM interface created");

	return QDF_STATUS_SUCCESS;

fail_flow_rec_freelist:
	qdf_mem_free(spm_intf);
	spm_intf = NULL;

fail_intf_alloc:
	return QDF_STATUS_E_FAILURE;
}

void wlan_dp_spm_intf_ctx_deinit(struct wlan_dp_intf *dp_intf)
{
	struct wlan_dp_spm_context *spm_ctx = wlan_dp_spm_get_context();
	struct wlan_dp_spm_intf_context *spm_intf;

	if (dp_intf->device_mode != QDF_STA_MODE)
		return;

	if (!dp_intf->spm_intf_ctx) {
		dp_info("Module already uninitialized!");
		return;
	}

	spm_intf = dp_intf->spm_intf_ctx;

	wlan_dp_spm_flow_retire(spm_intf, true);

	qdf_spinlock_destroy(&spm_intf->flow_list_lock);
	qdf_mem_zero(spm_intf->flow_rec_base, WLAN_DP_SPM_FLOW_REC_TBL_MAX *
					sizeof(struct wlan_dp_spm_flow_info));

	qdf_mem_free(spm_intf);
	dp_intf->spm_intf_ctx = NULL;

	if (spm_ctx)
		spm_ctx->spm_intf = NULL;
	dp_info("SPM interface deinitialized!");
}

#ifdef WLAN_DP_FEATURE_STC
static inline
void wlan_dp_spm_update_tx_flow_hash(struct wlan_dp_psoc_context *dp_ctx,
				     struct wlan_dp_spm_flow_info *flow_rec)
{
	struct flow_info flow_info_reverse = {0};
	struct flow_info *flow_info = &flow_rec->info;

	/* Switching direction to match Rx flow hash for bi-di flows*/
	if (flow_info->flags & FLOW_INFO_PRESENT_IPV4_SRC_IP) {
		flow_info_reverse.src_ip.ipv4_addr =
						flow_info->dst_ip.ipv4_addr;
		flow_info_reverse.dst_ip.ipv4_addr =
						flow_info->src_ip.ipv4_addr;
		flow_rec->is_ipv4 = true;
	} else if (flow_info->flags & FLOW_INFO_PRESENT_IPV6_SRC_IP) {
		flow_info_reverse.src_ip.ipv6_addr[0] =
						flow_info->dst_ip.ipv6_addr[0];
		flow_info_reverse.src_ip.ipv6_addr[1] =
						flow_info->dst_ip.ipv6_addr[1];
		flow_info_reverse.src_ip.ipv6_addr[2] =
						flow_info->dst_ip.ipv6_addr[2];
		flow_info_reverse.src_ip.ipv6_addr[3] =
						flow_info->dst_ip.ipv6_addr[3];

		flow_info_reverse.dst_ip.ipv6_addr[0] =
						flow_info->src_ip.ipv6_addr[0];
		flow_info_reverse.dst_ip.ipv6_addr[1] =
						flow_info->src_ip.ipv6_addr[1];
		flow_info_reverse.dst_ip.ipv6_addr[2] =
						flow_info->src_ip.ipv6_addr[2];
		flow_info_reverse.dst_ip.ipv6_addr[3] =
						flow_info->src_ip.ipv6_addr[3];
	}

	flow_info_reverse.src_port = flow_info->dst_port;
	flow_info_reverse.dst_port = flow_info->src_port;
	flow_info_reverse.proto =
				wlan_dp_ip_proto_to_stc_proto(flow_info->proto);
	flow_info_reverse.flags = 0;

	flow_rec->flow_tuple_hash = wlan_dp_get_flow_hash(dp_ctx,
							  &flow_info_reverse);
}
#else
static inline
void wlan_dp_spm_update_tx_flow_hash(struct wlan_dp_psoc_context *dp_ctx,
				     struct wlan_dp_spm_flow_info *flow_rec);
{
}
#endif

#if defined(WLAN_FEATURE_SAWFISH) || defined(WLAN_DP_FEATURE_STC)
void wlan_dp_spm_flow_table_attach(struct wlan_dp_psoc_context *dp_ctx)
{
	dp_ctx->gl_flow_recs =
		qdf_mem_malloc(sizeof(struct wlan_dp_spm_flow_info) *
			       WLAN_DP_SPM_FLOW_REC_TBL_MAX * WLAN_DP_INTF_MAX);
	if (!dp_ctx->gl_flow_recs)
		dp_err("Failed to SPM Tx flow table");
}

void wlan_dp_spm_flow_table_detach(struct wlan_dp_psoc_context *dp_ctx)
{
	qdf_mem_free(dp_ctx->gl_flow_recs);
}
#endif

QDF_STATUS wlan_dp_spm_get_flow_id_origin(struct wlan_dp_intf *dp_intf,
					  uint16_t *flow_id,
					  struct flow_info *flow_info,
					  uint64_t cookie_sk, uint16_t peer_id)
{
	struct wlan_dp_spm_intf_context *spm_intf;
	struct wlan_dp_spm_flow_info *flow_rec = NULL;
	struct wlan_dp_psoc_context *dp_ctx = dp_intf->dp_ctx;

	if (!dp_intf->spm_intf_ctx) {
		dp_info("Feature not supported");
		return QDF_STATUS_E_NOSUPPORT;
	}

	spm_intf = dp_intf->spm_intf_ctx;

	qdf_spinlock_acquire(&spm_intf->flow_list_lock);
	qdf_list_remove_front(&spm_intf->o_flow_rec_freelist,
			      (qdf_list_node_t **)&flow_rec);
	qdf_spinlock_release(&spm_intf->flow_list_lock);

	if (!flow_rec) {
		*flow_id = WLAN_DP_SPM_INVALID_FLOW_ID;
		dp_info_rl("records freelist size: %u, Active flow table full!",
			   spm_intf->o_flow_rec_freelist.count);
		return QDF_STATUS_E_EMPTY;
	}

	/* Copy data to flow record */
	flow_rec->flow_add_ts = qdf_sched_clock();
	flow_rec->guid = flow_guid_gen++;
	flow_rec->peer_id = peer_id;
	flow_rec->vdev_id = dp_intf->def_link->link_id;
	qdf_mem_copy(&flow_rec->info, flow_info, sizeof(struct flow_info));
	flow_rec->svc_id = WLAN_DP_SPM_INVALID_METADATA;
	flow_rec->svc_metadata = WLAN_DP_SPM_INVALID_METADATA;
	flow_rec->flags |= DP_SPM_FLOW_FLAG_IN_USE;
	flow_rec->cookie = cookie_sk;

	wlan_dp_spm_update_tx_flow_hash(dp_ctx, flow_rec);
	wlan_dp_indicate_rx_flow_add(dp_ctx);

	/* put the flow record in table and fill stats */
	spm_intf->origin_aft[flow_rec->id] = flow_rec;
	spm_intf->o_stats.active++;

	*flow_id = flow_rec->id;
	flow_rec->is_populated = 1;

	/* Trigger flow retiring event at threshold */
	if (qdf_unlikely(spm_intf->o_flow_rec_freelist.count <
				WLAN_DP_SPM_LOW_AVAILABLE_FLOWS_WATERMARK)) {
		if (!wlan_dp_spm_get_context())
			wlan_dp_spm_flow_retire(dp_intf->spm_intf_ctx, false);
		else
			wlan_dp_spm_event_post(
					WLAN_DP_SPM_EVENT_ACTIVE_FLOW_RETIRE,
					dp_intf->spm_intf_ctx);
	}

	return QDF_STATUS_SUCCESS;
}

void wlan_dp_spm_set_flow_active(struct wlan_dp_spm_intf_context *spm_intf,
				 uint16_t flow_id, uint64_t flow_guid)
{
	struct wlan_dp_spm_flow_info *flow_rec = NULL;

	if (flow_id >= WLAN_DP_SPM_FLOW_REC_TBL_MAX)
		return;

	flow_rec = spm_intf->origin_aft[flow_id];

	if (flow_rec && flow_rec->guid == flow_guid)
		flow_rec->active_ts = qdf_sched_clock();
	else
		dp_info("Flow %u with guid %lu not found", flow_id, flow_guid);
}
