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

#ifndef _WLAN_LL_LT_SAP_BEARER_SWITCH_H_
#define _WLAN_LL_LT_SAP_BEARER_SWITCH_H_

#include "wlan_ll_sap_public_structs.h"
#include <qdf_atomic.h>
#include "wlan_cmn.h"
#include "wlan_ll_sap_main.h"
#include "wlan_sm_engine.h"
#include "qdf_event.h"

/**
 * enum wlan_bearer_switch_sm_state - Bearer switch states
 * @BEARER_NON_WLAN: Default state, Bearer non wlan state
 * @BEARER_NON_WLAN_REQUESTED: State when bearer switch requested to non-wlan
 * @BEARER_WLAN_REQUESTED: State when bearer switch requested to wlan
 * @BEARER_WLAN: Bearer non wlan state
 * @BEARER_SWITCH_MAX: Max state
 */
enum wlan_bearer_switch_sm_state {
	BEARER_NON_WLAN = 0,
	BEARER_NON_WLAN_REQUESTED = 1,
	BEARER_WLAN_REQUESTED = 2,
	BEARER_WLAN = 3,
	BEARER_SWITCH_MAX = 4,
};

/**
 * enum wlan_bearer_switch_sm_evt - Bearer switch related events, if any new
 * enum is added to this enum, then please update bs_sm_event_names
 * @WLAN_BS_SM_EV_SWITCH_TO_WLAN: Bearer switch request to WLAN
 * @WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN: Bearer switch request to NON WLAN
 * @WLAN_BS_SM_EV_SWITCH_TO_WLAN_TIMEOUT: Bearer switch request to WLA
 * timeout
 * @WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_TIMEOUT: Bearer switch request to NON-WLAN
 * timeout
 * @WLAN_BS_SM_EV_SWITCH_TO_WLAN_COMPLETED: Bearer switch request to WLAN
 * completed
 * @WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_COMPLETED: Bearer switch request to
 * NON-WLAN completed
 * @WLAN_BS_SM_EV_SWITCH_TO_WLAN_FAILURE: Bearer switch request to WLAN
 * failure
 * @WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_FAILURE: Bearer switch request to NON-WLAN
 * failure
 */
enum wlan_bearer_switch_sm_evt {
	WLAN_BS_SM_EV_SWITCH_TO_WLAN = 0,
	WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN = 1,
	WLAN_BS_SM_EV_SWITCH_TO_WLAN_TIMEOUT = 2,
	WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_TIMEOUT = 3,
	WLAN_BS_SM_EV_SWITCH_TO_WLAN_COMPLETED = 4,
	WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_COMPLETED = 5,
	WLAN_BS_SM_EV_SWITCH_TO_WLAN_FAILURE = 6,
	WLAN_BS_SM_EV_SWITCH_TO_NON_WLAN_FAILURE = 7,
};

/**
 * struct bs_state_sm - Bearer switch state machine
 * @bs_sm_lock: sm lock
 * @sm_hdl: sm handlers
 * @bs_state: bearer switch state
 * @is_non_wlan_requested: Is non-wlan bearer switch is requested or not
 */
struct bs_state_sm {
	qdf_mutex_t bs_sm_lock;
	struct wlan_sm *sm_hdl;
	enum wlan_bearer_switch_sm_state bs_state;
	bool is_non_wlan_requested;
};

/**
 * struct bearer_switch_info - Data structure to store the bearer switch
 * requests and related information
 * @vdev: Pointer to the ll lt sap vdev
 * @request_id: Last allocated request id
 * @sm: state machine context
 * @ref_count: Reference count corresponding to each vdev and requester
 * @fw_ref_count: Reference counts for the firmware requests
 * @total_ref_count: Total reference counts
 * @last_status:status of the last bearer switch request
 * @requests: Array of bearer_switch_requests to cache the request information
 * @bs_request_timer: Bearer switch request timer, this timer monitors the
 * bearer switch response
 * @bs_wlan_request_timer: Bearer switch wlan request timer, host driver waits
 * for this timer to expire before sending the bearer switch request to wlan,
 * this is used to avoid the back to back bearer switch requests to userspace.
 * @p2p_go_bs_copletion_event: Bearer switch request for P2P GO is completed
 */
struct bearer_switch_info {
	struct wlan_objmgr_vdev *vdev;
	qdf_atomic_t request_id;
	struct bs_state_sm sm;
	qdf_atomic_t ref_count[WLAN_UMAC_PSOC_MAX_VDEVS][BEARER_SWITCH_REQ_MAX];
	qdf_atomic_t fw_ref_count;
	qdf_atomic_t total_ref_count;
	QDF_STATUS last_status;
	struct wlan_bearer_switch_request requests[MAX_BEARER_SWITCH_REQUESTERS];
	qdf_mc_timer_t bs_request_timer;
	qdf_mc_timer_t bs_wlan_request_timer;
	qdf_event_t p2p_go_bs_copletion_event;
};

/**
 * ll_lt_sap_bearer_switch_get_id() - Get the request id for bearer switch
 * request
 * @psoc: Pointer to psoc
 * Return: Bearer switch request id
 */
wlan_bs_req_id ll_lt_sap_bearer_switch_get_id(struct wlan_objmgr_psoc *psoc);

/**
 * bs_sm_create() - Invoke SM creation for bearer switch
 * @bs_ctx:  Bearer switch context
 *
 * Return: SUCCESS on successful creation
 *         FAILURE, if creation fails
 */
QDF_STATUS bs_sm_create(struct bearer_switch_info *bs_ctx);

/**
 * bs_sm_destroy() - Invoke SM deletion for bearer switch
 * @bs_ctx:  Bearer switch context
 *
 * Return: SUCCESS on successful deletion
 *         FAILURE, if deletion fails
 */
QDF_STATUS bs_sm_destroy(struct bearer_switch_info *bs_ctx);

/**
 * bs_lock_create() - Create BS SM mutex
 * @bs_ctx:  Bearer switch ctx
 *
 * Creates Bearer switch state machine mutex
 *
 * Return: void
 */
static inline void bs_lock_create(struct bearer_switch_info *bs_ctx)
{
	qdf_mutex_create(&bs_ctx->sm.bs_sm_lock);
}

/**
 * bs_lock_destroy() - Create BS SM mutex
 * @bs_ctx:  Bearer switch ctx
 *
 * Deatroys Bearer switch state machine mutex
 *
 * Return: void
 */
static inline void bs_lock_destroy(struct bearer_switch_info *bs_ctx)
{
	qdf_mutex_destroy(&bs_ctx->sm.bs_sm_lock);
}

/**
 * bs_lock_acquire() - Acquires BS SM mutex
 * @bs_ctx:  Bearer switch ctx
 *
 * Acquire Bearer switch state machine mutex
 *
 * Return: void
 */
static inline void bs_lock_acquire(struct bearer_switch_info *bs_ctx)
{
	qdf_mutex_acquire(&bs_ctx->sm.bs_sm_lock);
}

/**
 * bs_lock_release() - Release BS SM mutex
 * @bs_ctx:  Bearer switch ctx
 *
 * Releases Bearer switch state machine mutex
 *
 * Return: void
 */
static inline void bs_lock_release(struct bearer_switch_info *bs_ctx)
{
	qdf_mutex_release(&bs_ctx->sm.bs_sm_lock);
}

/**
 * bs_sm_transition_to() - invokes state transition
 * @bs_ctx:  Bearer switch ctx
 * @state: new cm state
 *
 * API to invoke SM API to move to new state
 *
 * Return: void
 */
static inline void bs_sm_transition_to(struct bearer_switch_info *bs_ctx,
				       enum wlan_bearer_switch_sm_state state)
{
	wlan_sm_transition_to(bs_ctx->sm.sm_hdl, state);
}

/**
 * bs_get_state() - Get current state of the bearer switch state machine
 * @bearer_switch_ctx: Bearer switch context
 *
 * Return: Current state of the bearer switch state machine
 */
enum wlan_bearer_switch_sm_state
bs_get_state(struct bearer_switch_info *bearer_switch_ctx);

/**
 * bs_sm_deliver_event() - Delivers event to Bearer switch SM
 * @psoc: Pointer to psoc
 * @event: BS event
 * @data_len: data size
 * @data: event data
 *
 * API to dispatch event to Bearer switch state machine. To be used while
 * posting events from API called from public API.
 *
 * Return: SUCCESS: on handling event
 *         FAILURE: If event not handled
 */
QDF_STATUS bs_sm_deliver_event(struct wlan_objmgr_psoc *psoc,
			       enum wlan_bearer_switch_sm_evt event,
			       uint16_t data_len, void *data);

/**
 * ll_lt_sap_is_bs_ctx_valid() - Check if bearer switch context is valid or not
 * @bs_ctx: Bearer switch context
 *
 * Return: True if bearer switch context is valid else return false
 */
#define ll_lt_sap_is_bs_ctx_valid(bs_ctx) \
	__ll_lt_sap_is_bs_ctx_valid(bs_ctx, __func__)

bool __ll_lt_sap_is_bs_ctx_valid(struct bearer_switch_info *bs_ctx,
				 const char *func);

/**
 * ll_lt_sap_is_bs_req_valid() - Check if bearer switch request is valid or not
 * @bs_req: Bearer switch request
 *
 * Return: True if bearer switch request is valid else return false
 */
#define ll_lt_sap_is_bs_req_valid(bs_req) \
	__ll_lt_sap_is_bs_req_valid(bs_req, __func__)

bool __ll_lt_sap_is_bs_req_valid(struct wlan_bearer_switch_request *bs_req,
				 const char *func);

/**
 * ll_lt_sap_switch_bearer_to_ble() - Switch audio transport to BLE
 * @psoc: Pointer to psoc
 * @bs_request: Pointer to bearer switch request
 * Return: QDF_STATUS_SUCCESS on successful bearer switch else failure
 */
QDF_STATUS
ll_lt_sap_switch_bearer_to_ble(struct wlan_objmgr_psoc *psoc,
			       struct wlan_bearer_switch_request *bs_request);

/**
 * ll_lt_sap_extract_ll_sap_cap() - Extract LL LT SAP capabilities
 * @psoc: Pointer to psoc object
 *
 */
void ll_lt_sap_extract_ll_sap_cap(struct wlan_objmgr_psoc *psoc);

/**
 * ll_lt_sap_switch_bearer_to_wlan() - Switch audio transport to BLE
 * @psoc: Pointer to psoc
 * @bs_request: Pointer to bearer switch request
 * Return: QDF_STATUS_SUCCESS on successful bearer switch else failure
 */
QDF_STATUS
ll_lt_sap_switch_bearer_to_wlan(struct wlan_objmgr_psoc *psoc,
				struct wlan_bearer_switch_request *bs_request);

/**
 * ll_lt_sap_request_for_audio_transport_switch() - Handls audio transport
 * switch request from userspace
 * @vdev: Vdev on which the request is received
 * @req_type: requested transport switch type
 *
 * Return: True/False
 */
QDF_STATUS
ll_lt_sap_request_for_audio_transport_switch(struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_req_type req_type);

/**
 * ll_lt_sap_deliver_audio_transport_switch_resp() - Deliver audio
 * transport switch response
 * @vdev: Vdev on which the request is received
 * @req_type: Transport switch type for which the response is received
 * @status: Status of the response
 *
 * Return: None
 */
void
ll_lt_sap_deliver_audio_transport_switch_resp(struct wlan_objmgr_vdev *vdev,
					enum bearer_switch_req_type req_type,
					enum bearer_switch_status status);

/**
 * ll_lt_sap_switch_bearer_for_p2p_go_start() - Switch the bearer if P2P GO is
 * coming up in mcc
 * @psoc: Pointer to psoc
 * @vdev_id: vdev id of the current vdev
 * @oper_freq: Frequency on which P2P_GO wants to come up
 * @device_mode: Device mode of the current vdev
 *
 * This API checks if LL_LT_SAP is present and P2P_GO is coming up in MCC with
 * LL_LT_SAP, then it switches the bearer as P2P GO vdev start takes some time
 * and till then LL_LT_SAP will not be able to stay on that channel continuously
 *
 * Return: None
 */
void ll_lt_sap_switch_bearer_for_p2p_go_start(struct wlan_objmgr_psoc *psoc,
					      uint8_t vdev_id,
					      qdf_freq_t oper_freq,
					      enum QDF_OPMODE device_mode);

/**
 * ll_lt_sap_switch_bearer_on_p2p_go_complete() - Switch the bearer if P2P GO is
 * started
 * @psoc: Pointer to psoc
 * @vdev_id: vdev id of the current vdev
 * @device_mode: Device mode of the current vdev
 *
 * This API switches the bearer back to wlan, once P2P GO is started
 *
 * Return: None
 */

void ll_lt_sap_switch_bearer_on_p2p_go_complete(struct wlan_objmgr_psoc *psoc,
						uint8_t vdev_id,
						enum QDF_OPMODE device_mode);

/**
 * ll_lt_sap_switch_bearer() - LL_LT_SAP bearer switch request
 * @psoc: Pointer to psoc
 * @vdev_id: vdev id of the current vdev
 * @req_type: request type
 * @source: source of bearer switch req
 * @requester_cb: requester callback
 * @arg_value: arg value
 * @arg: argument
 */
QDF_STATUS
ll_lt_sap_switch_bearer(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
			enum bearer_switch_req_type req_type,
			enum bearer_switch_req_source source,
			bearer_switch_requester_cb requester_cb,
			uint32_t arg_value, void *arg);
#endif /* _WLAN_LL_LT_SAP_BEARER_SWITCH_H_ */
