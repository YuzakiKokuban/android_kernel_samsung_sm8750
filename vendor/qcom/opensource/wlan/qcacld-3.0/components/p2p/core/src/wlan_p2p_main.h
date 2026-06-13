/*
 * Copyright (c) 2017-2020 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: Defines main P2P functions & structures
 */

#ifndef _WLAN_P2P_MAIN_H_
#define _WLAN_P2P_MAIN_H_

#include <qdf_trace.h>
#include <qdf_types.h>
#include <qdf_event.h>
#include <qdf_list.h>
#include <qdf_lock.h>
#include <qdf_idr.h>
#include <qdf_mc_timer.h>
#include <wlan_scan_public_structs.h>
#include "wlan_p2p_ucfg_api.h"

#define MAX_QUEUE_LENGTH 20
#define P2P_NOA_ATTR_IND 0x1090
#define P2P_MODULE_NAME  "P2P"
#define P2P_INVALID_VDEV_ID 0xFFFFFFFF
#define MAX_RANDOM_MAC_ADDRS 4

#define p2p_debug(params ...) \
	QDF_TRACE_DEBUG(QDF_MODULE_ID_P2P, params)
#define p2p_info(params ...) \
	QDF_TRACE_INFO(QDF_MODULE_ID_P2P, params)
#define p2p_warn(params ...) \
	QDF_TRACE_WARN(QDF_MODULE_ID_P2P, params)
#define p2p_err(params ...) \
	QDF_TRACE_ERROR(QDF_MODULE_ID_P2P, params)
#define p2p_debug_rl(params...) \
	QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_P2P, params)
#define p2p_info_rl(params...) \
		QDF_TRACE_INFO_RL(QDF_MODULE_ID_P2P, params)

#define p2p_alert(params ...) \
	QDF_TRACE_FATAL(QDF_MODULE_ID_P2P, params)

#define p2p_nofl_debug(params ...) \
	QDF_TRACE_DEBUG_NO_FL(QDF_MODULE_ID_P2P, params)
#define p2p_nofl_info(params ...) \
	QDF_TRACE_INFO_NO_FL(QDF_MODULE_ID_P2P, params)
#define p2p_nofl_warn(params ...) \
	QDF_TRACE_WARN_NO_FL(QDF_MODULE_ID_P2P, params)
#define p2p_nofl_err(params ...) \
	QDF_TRACE_ERROR_NO_FL(QDF_MODULE_ID_P2P, params)
#define p2p_nofl_alert(params ...) \
	QDF_TRACE_FATAL_NO_FL(QDF_MODULE_ID_P2P, params)

struct scheduler_msg;
struct p2p_tx_cnf;
struct p2p_rx_mgmt_frame;
struct p2p_lo_event;
struct p2p_start_param;
struct p2p_noa_info;
struct tx_action_context;

/**
 * enum p2p_cmd_type - P2P request type
 * @P2P_ROC_REQ:            P2P roc request
 * @P2P_CANCEL_ROC_REQ:     Cancel P2P roc request
 * @P2P_MGMT_TX:            P2P tx action frame request
 * @P2P_MGMT_TX_CANCEL:     Cancel tx action frame request
 * @P2P_CLEANUP_ROC:        Cleanup roc queue
 * @P2P_CLEANUP_TX:         Cleanup tx mgmt queue
 * @P2P_SET_RANDOM_MAC: Set Random MAC addr filter request
 * @P2P_GROUP_CHAN_SWITCH_CMD: Channel switch request on P2P device
 */
enum p2p_cmd_type {
	P2P_ROC_REQ = 0,
	P2P_CANCEL_ROC_REQ,
	P2P_MGMT_TX,
	P2P_MGMT_TX_CANCEL,
	P2P_CLEANUP_ROC,
	P2P_CLEANUP_TX,
	P2P_SET_RANDOM_MAC,
	P2P_GROUP_CHAN_SWITCH_CMD,
};

/**
 * enum p2p_event_type - P2P event type
 * @P2P_EVENT_SCAN_EVENT:        P2P scan event
 * @P2P_EVENT_MGMT_TX_ACK_CNF:   P2P mgmt tx confirm frame
 * @P2P_EVENT_RX_MGMT:           P2P rx mgmt frame
 * @P2P_EVENT_LO_STOPPED:        P2P listen offload stopped event
 * @P2P_EVENT_NOA:               P2P noa event
 * @P2P_EVENT_ADD_MAC_RSP: Set Random MAC addr event
 * @P2P_EVENT_AP_ASSIST_DFS_GROUP_BMISS_IND: P2P AP assisted DFS group bmiss
 * indication from FW.
 */
enum p2p_event_type {
	P2P_EVENT_SCAN_EVENT = 0,
	P2P_EVENT_MGMT_TX_ACK_CNF,
	P2P_EVENT_RX_MGMT,
	P2P_EVENT_LO_STOPPED,
	P2P_EVENT_NOA,
	P2P_EVENT_ADD_MAC_RSP,
	P2P_EVENT_AP_ASSIST_DFS_GROUP_BMISS_IND,
};

/**
 * struct p2p_tx_conf_event - p2p tx confirm event
 * @p2p_soc_obj:        p2p soc private object
 * @nbuf:               buffer address
 * @status:             tx status
 */
struct p2p_tx_conf_event {
	struct p2p_soc_priv_obj *p2p_soc_obj;
	qdf_nbuf_t nbuf;
	uint32_t status;
};

/**
 * struct p2p_rx_mgmt_event - p2p rx mgmt frame event
 * @p2p_soc_obj:        p2p soc private object
 * @rx_mgmt:            p2p rx mgmt frame structure
 */
struct p2p_rx_mgmt_event {
	struct p2p_soc_priv_obj *p2p_soc_obj;
	struct p2p_rx_mgmt_frame *rx_mgmt;
};

/**
 * struct p2p_lo_stop_event - p2p listen offload stop event
 * @p2p_soc_obj:        p2p soc private object
 * @lo_event:           p2p lo stop structure
 */
struct p2p_lo_stop_event {
	struct p2p_soc_priv_obj *p2p_soc_obj;
	struct p2p_lo_event *lo_event;
};

/**
 * struct p2p_noa_event - p2p noa event
 * @p2p_soc_obj:        p2p soc private object
 * @noa_info:           p2p noa information structure
 */
struct p2p_noa_event {
	struct p2p_soc_priv_obj *p2p_soc_obj;
	struct p2p_noa_info *noa_info;
};

/**
 * struct p2p_mac_filter_rsp - p2p set mac filter respone
 * @p2p_soc_obj: p2p soc private object
 * @vdev_id: vdev id
 * @status: successfully(1) or not (0)
 */
struct p2p_mac_filter_rsp {
	struct p2p_soc_priv_obj *p2p_soc_obj;
	uint32_t vdev_id;
	uint32_t status;
};

/**
 * struct p2p_ap_assist_dfs_group_bmiss - P2P AP assisted DFS group bmiss
 * notify params
 * @p2p_soc_obj: P2P soc priv object.
 * @vdev_id: VDEV ID of bmiss
 */
struct p2p_ap_assist_dfs_group_bmiss {
	struct p2p_soc_priv_obj *p2p_soc_obj;
	uint8_t vdev_id;
};

#ifdef WLAN_FEATURE_P2P_DEBUG
/**
 * enum p2p_connection_status - p2p connection status
 * @P2P_NOT_ACTIVE:                P2P not active status
 * @P2P_GO_NEG_PROCESS:            P2P GO negotiation in process
 * @P2P_GO_NEG_COMPLETED:          P2P GO negotiation complete
 * @P2P_CLIENT_CONNECTING_STATE_1: P2P client connecting state 1
 * @P2P_GO_COMPLETED_STATE:        P2P GO complete state
 * @P2P_CLIENT_CONNECTED_STATE_1:  P2P client connected state 1
 * @P2P_CLIENT_DISCONNECTED_STATE: P2P client disconnected state
 * @P2P_CLIENT_CONNECTING_STATE_2: P2P client connecting state 2
 * @P2P_CLIENT_COMPLETED_STATE:    P2P client complete state
 */
enum p2p_connection_status {
	P2P_NOT_ACTIVE,
	P2P_GO_NEG_PROCESS,
	P2P_GO_NEG_COMPLETED,
	P2P_CLIENT_CONNECTING_STATE_1,
	P2P_GO_COMPLETED_STATE,
	P2P_CLIENT_CONNECTED_STATE_1,
	P2P_CLIENT_DISCONNECTED_STATE,
	P2P_CLIENT_CONNECTING_STATE_2,
	P2P_CLIENT_COMPLETED_STATE
};

/**
 * p2p_status_update() - Update p2p connection status
 * @p2p_soc_obj: p2p priv object
 * @status: p2p connection status
 *
 * Return: void
 */
void p2p_status_update(struct p2p_soc_priv_obj *p2p_soc_obj,
		       enum p2p_connection_status status);
#endif

/**
 * struct p2p_param - p2p parameters to be used
 * @go_keepalive_period:            P2P GO keep alive period
 * @go_link_monitor_period:         period where link is idle and
 *                                  where we send NULL frame
 * @p2p_device_addr_admin:          enable/disable to derive the P2P
 *                                  MAC address from the primary MAC address
 * @skip_dfs_channel_p2p_search:    skip DFS Channel in case of P2P Search
 * @is_random_seq_num_enabled:      Flag to generate random sequence numbers
 * @indoor_channel_support:         support to allow GO in indoor channels
 * @go_ignore_non_p2p_probe_req:    P2P GO ignore non-P2P probe req
 * @sta_vdev_for_p2p_device:        Use sta vdev for p2p device operation
 * @sta_vdev_for_p2p_device_upon_vdev_exhaust: Use sta vdev for p2p device
 * operation when maximum vdev creation reaches to limit
 */
struct p2p_param {
	uint32_t go_keepalive_period;
	uint32_t go_link_monitor_period;
	uint32_t p2p_device_addr_admin:1;
	uint32_t is_random_seq_num_enabled:1;
	uint32_t indoor_channel_support:1;
	uint32_t go_ignore_non_p2p_probe_req:1;
	uint32_t sta_vdev_for_p2p_device:1;
	uint32_t sta_vdev_for_p2p_device_upon_vdev_exhaust:1;
};

/**
 * struct p2p_soc_priv_obj - Per SoC p2p private object
 * @soc:              Pointer to SoC context
 * @roc_q:            Queue for pending roc requests
 * @tx_q_roc:         Queue for tx frames waiting for RoC
 * @tx_q_ack:         Queue for tx frames waiting for ack
 * @scan_req_id:      Scan requestor id
 * @start_param:      Start parameters, include callbacks and user
 *                    data to HDD
 * @cancel_roc_done:  Cancel roc done event
 * @cleanup_roc_done: Cleanup roc done event
 * @cleanup_tx_done:  Cleanup tx done event
 * @roc_runtime_lock: Runtime lock for roc request
 * @p2p_cb: Callbacks to protocol stack
 * @cur_roc_vdev_id:  Vdev id of current roc
 * @p2p_idr:          p2p idr
 * @param:            p2p parameters to be used
 * @connection_status:Global P2P connection status
 * @mcc_quota_ev_os_if_cb:  callback to OS IF to indicate mcc quota event
 * @mgmt_frm_registration_update: mgmt frame registration update
 * @sta_vdev_for_p2p_dev_operations: Use sta vdev for p2p device operations
 * @sta_vdev_id: store sta vdev_id to use it for p2p device operation.
 */
struct p2p_soc_priv_obj {
	struct wlan_objmgr_psoc *soc;
	qdf_list_t roc_q;
	qdf_list_t tx_q_roc;
	qdf_list_t tx_q_ack;
	wlan_scan_requester scan_req_id;
	struct p2p_start_param *start_param;
	qdf_event_t cleanup_roc_done;
	qdf_event_t cleanup_tx_done;
	qdf_runtime_lock_t roc_runtime_lock;
	struct p2p_protocol_callbacks p2p_cb;
	uint32_t cur_roc_vdev_id;
	qdf_idr p2p_idr;
	struct p2p_param param;
#ifdef WLAN_FEATURE_P2P_DEBUG
	enum p2p_connection_status connection_status;
#endif
#ifdef WLAN_FEATURE_MCC_QUOTA
	mcc_quota_event_callback mcc_quota_ev_os_if_cb;
#endif
	uint32_t mgmt_frm_registration_update;
	bool sta_vdev_for_p2p_dev_operations;
	uint32_t sta_vdev_id;
};

/**
 * struct action_frame_cookie - Action frame cookie item in cookie list
 * @cookie_node: qdf_list_node
 * @cookie: Cookie value
 */
struct action_frame_cookie {
	qdf_list_node_t cookie_node;
	uint64_t cookie;
};

/**
 * struct action_frame_random_mac - Action Frame random mac addr &
 * related attrs
 * @p2p_vdev_obj: p2p vdev private obj ptr
 * @in_use: Checks whether random mac is in use
 * @addr: Contains random mac addr
 * @freq: Channel frequency
 * @clear_timer: timer to clear random mac filter
 * @cookie_list: List of cookies tied with random mac
 */
struct action_frame_random_mac {
	struct p2p_vdev_priv_obj *p2p_vdev_obj;
	bool in_use;
	uint8_t addr[QDF_MAC_ADDR_SIZE];
	uint32_t freq;
	qdf_mc_timer_t clear_timer;
	qdf_list_t cookie_list;
};

/**
 * typedef p2p_request_mgr_callback_t() - callback to process set mac filter
 *                                        result
 * @result: bool
 * @context: callback context.
 *
 * Return: void
 */
typedef void (*p2p_request_mgr_callback_t)(bool result, void *context);

/**
 * struct random_mac_priv - request private data struct
 * @result: result of request.
 */
struct random_mac_priv {
	bool result;
};

/**
 * struct p2p_set_mac_filter_req - set mac addr filter cmd data structure
 * @soc: soc object
 * @vdev_id: vdev id
 * @mac: mac address to be set
 * @freq: frequency
 * @set: set or clear
 * @cb: callback func to be called when the request completion
 * @req_cookie: cookie to be used when request completed
 */
struct p2p_set_mac_filter_req {
	struct wlan_objmgr_psoc *soc;
	uint32_t vdev_id;
	uint8_t mac[QDF_MAC_ADDR_SIZE];
	uint32_t freq;
	bool set;
	p2p_request_mgr_callback_t cb;
	void *req_cookie;
};

#define WLAN_P2P_MAX_WLAN_AP_INFO 10
/**
 * struct p2p_ap_assist_dfs_ap_info - Struct to hold WLAN per AP info in P2P2 IE
 * @is_connected: Is connected bit set in WLAN AP info attr
 * @is_valid: Is WLAN AP info is valid
 * @ap_bssid: BSSID of the WLAN AP
 * @op_class: Operating class of the WLAN AP
 * @chan: Channel number of the WLAN AP
 */
struct p2p_ap_assist_dfs_ap_info {
	bool is_connected;
	bool is_valid;
	struct qdf_mac_addr ap_bssid;
	uint8_t op_class;
	uint8_t chan;
};

/**
 * struct p2p_ap_assist_dfs_group_info - Extracted info from P2P2 IE related
 * to DFS owner capability and AP assisted params
 * @is_dfs_owner: Is DFS owner
 * @is_client_csa: Can client send CSA request
 * @extn_cap_attr_found: Is extended cap attr found
 * @wlan_ap_info_attr_found: Is WLAN AP info attr found
 * @is_valid_ap_assist: Is assisted AP params valid
 * @num_ap_info: Number of APs in WLAN AP info attr
 * @ap_info: List of WLAN AP extracted from WLAN AP info attr
 */
struct p2p_ap_assist_dfs_group_info {
	bool is_dfs_owner;
	bool is_client_csa;
	bool extn_cap_attr_found;
	bool wlan_ap_info_attr_found;
	bool is_valid_ap_assist;
	uint8_t num_ap_info;
	struct p2p_ap_assist_dfs_ap_info ap_info[WLAN_P2P_MAX_WLAN_AP_INFO];
};

/**
 * struct p2p_vdev_priv_obj - Per vdev p2p private object
 * @vdev:               Pointer to vdev context
 * @noa_info:           NoA information
 * @noa_status:         NoA status i.e. Enabled / Disabled (TRUE/FALSE)
 * @non_p2p_peer_count: Number of legacy stations connected to this GO
 * @random_mac_lock:    lock for random_mac list
 * @random_mac:         active random mac filter lists
 * @pending_req:        pending set mac filter request.
 * @prev_action_frame_addr2: Address2 field (TA) of the last transmitted
 *                           action frame.
 * @ap_assist_dfs:      AP assisted DFS group operation info
 */
struct p2p_vdev_priv_obj {
	struct   wlan_objmgr_vdev *vdev;
	struct   p2p_noa_info *noa_info;
	bool     noa_status;
	uint16_t non_p2p_peer_count;

	/* random address management for management action frames */
	qdf_spinlock_t random_mac_lock;
	struct action_frame_random_mac random_mac[MAX_RANDOM_MAC_ADDRS];
	struct p2p_set_mac_filter_req pending_req;
	uint8_t prev_action_frame_addr2[QDF_MAC_ADDR_SIZE];

	struct p2p_ap_assist_dfs_group_info ap_assist_dfs;
};

/**
 * struct p2p_noa_attr - p2p noa attribute
 * @rsvd1:             reserved bits 1
 * @opps_ps:           opps ps state of the AP
 * @ct_win:            ct window in TUs
 * @index:             identifies instance of NOA su element
 * @rsvd2:             reserved bits 2
 * @noa1_count:        interval count of noa1
 * @noa1_duration:     absent period duration of noa1
 * @noa1_interval:     absent period interval of noa1
 * @noa1_start_time:   32 bit tsf time of noa1
 * @rsvd3:             reserved bits 3
 * @noa2_count:        interval count of noa2
 * @noa2_duration:     absent period duration of noa2
 * @noa2_interval:     absent period interval of noa2
 * @noa2_start_time:   32 bit tsf time of noa2
 */
struct p2p_noa_attr {
	uint32_t rsvd1:16;
	uint32_t ct_win:7;
	uint32_t opps_ps:1;
	uint32_t index:8;
	uint32_t rsvd2:24;
	uint32_t noa1_count:8;
	uint32_t noa1_duration;
	uint32_t noa1_interval;
	uint32_t noa1_start_time;
	uint32_t rsvd3:24;
	uint32_t noa2_count:8;
	uint32_t noa2_duration;
	uint32_t noa2_interval;
	uint32_t noa2_start_time;
};

/**
 * p2p_component_init() - P2P component initialization
 *
 * This function registers psoc/vdev create/delete handler.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_component_init(void);

/**
 * p2p_component_deinit() - P2P component de-init
 *
 * This function deregisters psoc/vdev create/delete handler.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_component_deinit(void);

/**
 * p2p_psoc_object_open() - Open P2P component
 * @soc: soc context
 *
 * This function initialize p2p psoc object
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_psoc_object_open(struct wlan_objmgr_psoc *soc);

/**
 * p2p_psoc_object_close() - Close P2P component
 * @soc: soc context
 *
 * This function de-init p2p psoc object.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_psoc_object_close(struct wlan_objmgr_psoc *soc);

/**
 * p2p_psoc_start() - Start P2P component
 * @soc: soc context
 * @req: P2P start parameters
 *
 * This function sets up layer call back in p2p psoc object
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_psoc_start(struct wlan_objmgr_psoc *soc,
	struct p2p_start_param *req);

/**
 * p2p_psoc_stop() - Stop P2P component
 * @soc: soc context
 *
 * This function clears up layer call back in p2p psoc object.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_psoc_stop(struct wlan_objmgr_psoc *soc);

/**
 * p2p_process_cmd() - Process P2P messages in OS interface queue
 * @msg: message information
 *
 * This function is main handler for P2P messages in OS interface
 * queue, it gets called by message scheduler.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_process_cmd(struct scheduler_msg *msg);

/**
 * p2p_process_evt() - Process P2P messages in target interface queue
 * @msg: message information
 *
 * This function is main handler for P2P messages in target interface
 * queue, it gets called by message scheduler.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_process_evt(struct scheduler_msg *msg);

/**
 * p2p_msg_flush_callback() - Callback used to flush P2P messages
 * @msg: message information
 *
 * This callback will be called when scheduler flush some of P2P messages.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_msg_flush_callback(struct scheduler_msg *msg);

/**
 * p2p_event_flush_callback() - Callback used to flush P2P events
 * @msg: event information
 *
 * This callback will be called when scheduler flush some of P2P events.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_event_flush_callback(struct scheduler_msg *msg);

/**
 * p2p_check_oui_and_force_1x1() - Function to get P2P client device
 * attributes from assoc request frame IE passed in.
 * @assoc_ie:     Pointer to the IEs in the association req frame
 * @assoc_ie_len: Total length of the IE in association req frame
 *
 * Return: true if the OUI is present else false
 */
bool p2p_check_oui_and_force_1x1(uint8_t *assoc_ie, uint32_t assoc_ie_len);

#ifdef FEATURE_P2P_LISTEN_OFFLOAD
/**
 * p2p_process_lo_stop() - Process lo stop event
 * @lo_stop_event: listen offload stop event information
 *
 * This function handles listen offload stop event and deliver this
 * event to HDD layer by registered callback.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_process_lo_stop(
	struct p2p_lo_stop_event *lo_stop_event);
#else
static inline QDF_STATUS p2p_process_lo_stop(
	struct p2p_lo_stop_event *lo_stop_event)
{
	return QDF_STATUS_SUCCESS;
}
#endif
/**
 * p2p_process_noa() - Process noa event
 * @noa_event: noa event information
 *
 * This function handles noa event and save noa information in p2p
 * vdev object.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_process_noa(struct p2p_noa_event *noa_event);

#ifdef WLAN_FEATURE_P2P_DEBUG
/**
 * p2p_status_scan() - Update P2P connection status
 * @vdev: vdev context
 *
 * This function updates P2P connection status when scanning
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_status_scan(struct wlan_objmgr_vdev *vdev);

/**
 * p2p_status_connect() - Update P2P connection status
 * @vdev:        vdev context
 *
 * This function updates P2P connection status when connecting.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_status_connect(struct wlan_objmgr_vdev *vdev);

/**
 * p2p_status_disconnect() - Update P2P connection status
 * @vdev:        vdev context
 *
 * This function updates P2P connection status when disconnecting.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_status_disconnect(struct wlan_objmgr_vdev *vdev);

/**
 * p2p_status_start_bss() - Update P2P connection status
 * @vdev:        vdev context
 *
 * This function updates P2P connection status when starting BSS.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_status_start_bss(struct wlan_objmgr_vdev *vdev);

/**
 * p2p_status_stop_bss() - Update P2P connection status
 * @vdev:        vdev context
 *
 * This function updates P2P connection status when stopping BSS.
 *
 * Return: QDF_STATUS_SUCCESS - in case of success
 */
QDF_STATUS p2p_status_stop_bss(struct wlan_objmgr_vdev *vdev);
#else
static inline QDF_STATUS p2p_status_scan(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS p2p_status_connect(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS p2p_status_disconnect(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS p2p_status_start_bss(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS p2p_status_stop_bss(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_FEATURE_P2P_DEBUG */
#ifdef WLAN_FEATURE_P2P_P2P_STA
/**
 * p2p_check_and_force_scc_go_plus_go() - Check and do force scc for
 * go plus go
 * @psoc: psoc object
 * @vdev: vdev object
 *
 * This function checks whether force scc is enabled or not. If it
 * is enabled then it will do force scc to remaining p2p go vdev if
 * user has initiated CSA to current vdev.
 *
 * Return: status
 */

QDF_STATUS
p2p_check_and_force_scc_go_plus_go(struct wlan_objmgr_psoc *psoc,
				   struct wlan_objmgr_vdev *vdev);
#endif /* WLAN_FEATURE_P2P_P2P_STA */

/**
 * p2p_set_mgmt_frm_registration_update() - Set mgmt registration update
 * @psoc: pointer to psoc object
 * @mgmt_frm_registration_update: mgmt frame registration update value
 *
 * Return: None
 */
void
p2p_set_mgmt_frm_registration_update(struct wlan_objmgr_psoc *psoc,
				     uint32_t mgmt_frm_registration_update);

/**
 * p2p_get_mgmt_frm_registration_update() - Get mgmt registration update
 * @psoc: pointer to psoc object
 *
 * Return: uint32_t
 */
uint32_t
p2p_get_mgmt_frm_registration_update(struct wlan_objmgr_psoc *psoc);

/**
 * p2p_parse_assoc_ie_for_device_info() - This function finds P2P interface
 * address from assocaition IE
 * @assoc_ie: Association request IE
 * @assoc_ie_len: Association IE length
 *
 * Return: pointer to P2P address
 */
const uint8_t *p2p_parse_assoc_ie_for_device_info(const uint8_t *assoc_ie,
						  uint32_t assoc_ie_len);
#ifdef FEATURE_WLAN_SUPPORT_USD
/**
 * p2p_send_usd_params() - Sent USD parameters to target
 * @psoc: pointer to PSOC object
 * @param: pointer to USD attributes parameters structure
 *
 * Return: QDF status
 */
QDF_STATUS p2p_send_usd_params(struct wlan_objmgr_psoc *psoc,
			       struct p2p_usd_attr_params *param);
#endif /* FEATURE_WLAN_SUPPORT_USD */

/**
 * p2p_extract_ap_assist_dfs_params() - Extract P2P2 IE for assisted AP
 * operation info
 * @vdev: VDEV object manager pointer
 * @ie: Buffer pointer to IE
 * @ie_len: Length of bytes pointer by @ie
 * @is_connected: If set to %true, only connected AP info is extracted from
 * WLAN AP info attr in P2P2 IE
 * @freq: Frequency to filter from the available APs in WLAN AP info attr in
 * P2P@ IE
 * @is_self: If set to %true, the extracted info is saved in VDEV priv
 *
 * Return: QDF_STATUS
 */
QDF_STATUS p2p_extract_ap_assist_dfs_params(struct wlan_objmgr_vdev *vdev,
					    const uint8_t *ie, uint16_t ie_len,
					    bool is_connected, qdf_freq_t freq,
					    bool is_self);

/**
 * p2p_get_ap_assist_dfs_params() - Get the parameters of attributes in P2P2 IE
 * @vdev: VDEV object manager pointer of P2P entity
 * @is_dfs_owner: Pointer to get DFS owner capability in extended cap of P2P2 IE
 * @is_valid_ap_assist: Is assisted AP params valid
 * @ap_bssid: Pointer to get AP BSSID of assisted AP in DFS oper extracted from
 * wlan ap info attribute
 * @opclass: Operating class of the AP pointed in @ap_bssid
 * @chan: Channel number of the AP pointed in @ap_bssid
 *
 * Return: QDF_STATUS
 */
QDF_STATUS p2p_get_ap_assist_dfs_params(struct wlan_objmgr_vdev *vdev,
					bool *is_dfs_owner,
					bool *is_valid_ap_assist,
					struct qdf_mac_addr *ap_bssid,
					uint8_t *opclass, uint8_t *chan);

/**
 * p2p_fw_support_ap_assist_dfs_group() - API to return the FW capability of
 * AP assisted DFS P2P group
 * @psoc: PSOC object manager
 *
 * This API checks the FW capability attribute to support for AP assisted
 * DFS P2P group operation
 *
 * Return: bool
 */
bool p2p_fw_support_ap_assist_dfs_group(struct wlan_objmgr_psoc *psoc);

/**
 * p2p_check_ap_assist_dfs_group_cli() - API to check the status of P2P CLI
 * for operation in DFS channel under assisted AP mode
 * @vdev: VDEV object manager pointer of P2P_CLI entity
 *
 * Checks the conditions for P2P CLI to operate in DFS channel in AP assisted
 * mode and sends command to FW to either monitor the assisted or not.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS p2p_check_ap_assist_dfs_group_cli(struct wlan_objmgr_vdev *vdev);

/**
 * p2p_check_ap_assist_dfs_group_go() - API to check the status of P2P GO
 * for operation in DFS channel under assisted AP mode
 * @vdev: VDEV object manager pointer of P2P_GO entity
 *
 * Checks the requirements for P2P GO to operate in DFS channel in AP assisted
 * mode.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS p2p_check_ap_assist_dfs_group_go(struct wlan_objmgr_vdev *vdev);

/**
 * p2p_check_ap_assist_dfs_group_go_with_csa() - API to check the status of
 * P2P GO for operation in DFS channel
 * @vdev: VDEV object manager pointer of P2P_GO entity
 *
 * Checks the requirements for P2P GO to operate in DFS channel in AP assisted
 * mode and triggers CSA incase any condition fails.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
p2p_check_ap_assist_dfs_group_go_with_csa(struct wlan_objmgr_vdev *vdev);

/**
 * p2p_validate_ap_assist_dfs_group() - Validate the params of extracted
 * assisted AP params from P2P2 IE
 * @vdev: VDEV object manager pointer of P2P device
 *
 * Validates the info parsed from the P2P2 IE related to DFS operation under
 * assisted AP mode.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS p2p_validate_ap_assist_dfs_group(struct wlan_objmgr_vdev *vdev);

/**
 * p2p_get_sta_vdev_for_p2p_dev_cap() - Check fw and host capability
 * @psoc: pointer to psoc
 *
 * This API checks if STA vdev for P2P device operation is supported by both
 * host and firmware.
 *
 * Return: True/False
 */
bool p2p_get_sta_vdev_for_p2p_dev_cap(struct wlan_objmgr_psoc *psoc);

/**
 * p2p_get_sta_vdev_for_p2p_dev_upon_vdev_exhaust_cap()
 * @psoc: pointer to psoc
 *
 * This api will check if STA vdev for P2P device operation is supported or
 * not. If it's supported then host will use STA vdev for P2P device operation
 * whenever the new interface tries to comes up but there is no more vdev
 * available to create.
 * In this case, host will try to accommodate the new interface by destroying
 * the p2p device vdev if it's present and it will redirect the p2p device
 * operation on STA vdev itself.
 *
 * Return: True/False
 */
bool p2p_get_sta_vdev_for_p2p_dev_upon_vdev_exhaust_cap(
					struct wlan_objmgr_psoc *psoc);

/**
 * p2p_set_sta_vdev_for_p2p_dev_operations() - Allow current p2p device to
 *						    use sta vdev
 * @psoc: pointer to psoc
 * @val: value
 *
 * This is called with value true when firmware and the host INI params support
 * the feature "use STA vdev for P2P" and STA vdev is available
 *
 * Return: None
 */
void p2p_set_sta_vdev_for_p2p_dev_operations(struct wlan_objmgr_psoc *psoc,
					     bool val);

/**
 * p2p_is_sta_vdev_usage_allowed_for_p2p_dev() - Check whether sta vdev
 *					can be used for current P2P device
 * @psoc: pointer to psoc
 *
 * Return: True/False
 */
bool p2p_is_sta_vdev_usage_allowed_for_p2p_dev(struct wlan_objmgr_psoc *psoc);

/**
 * p2p_psoc_priv_set_sta_vdev_id() - Cache STA vdev id
 * @psoc: pointer to psoc
 * @vdev_id: vdev id to set
 *
 * Cache STA vdev_id in psoc p2p priv object.
 *
 * Return: None
 */
void p2p_psoc_priv_set_sta_vdev_id(struct wlan_objmgr_psoc *psoc,
				   uint8_t vdev_id);

/**
 * p2p_psoc_priv_get_sta_vdev_id() - Get cached STA vdev id
 * @psoc: pointer to psoc
 *
 * Return: uint8_t
 */
uint8_t p2p_psoc_priv_get_sta_vdev_id(struct wlan_objmgr_psoc *psoc);

/**
 * p2p_set_rand_mac_for_p2p_dev() - set P2P device mac addr to rx filters
 * @soc: pointer to psoc
 * @vdev_id: vdev id to fetch p2p_vdev_priv_obj
 * @freq: frequency on which the filtering(allow) is expected
 * @rnd_cookie: cookie value
 * @duration: duration of the filter validity. p2p_mac_clear_timeout() is called
 *            and filter would be removed upon timeout, if not removed already
 *
 * Return: None
 */
QDF_STATUS
p2p_set_rand_mac_for_p2p_dev(struct wlan_objmgr_psoc *soc,
			     uint32_t vdev_id, uint32_t freq,
			     uint64_t rnd_cookie, uint32_t duration);

#endif /* _WLAN_P2P_MAIN_H_ */
