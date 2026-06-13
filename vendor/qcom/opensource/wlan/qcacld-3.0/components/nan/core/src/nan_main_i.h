/*
 * Copyright (c) 2012-2021 The Linux Foundation. All rights reserved.
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
 * DOC: contains declaration of common utility APIs and private structs to be
 * used in NAN modules
 */

#ifdef WLAN_FEATURE_NAN
#ifndef _WLAN_NAN_MAIN_I_H_
#define _WLAN_NAN_MAIN_I_H_

#include "qdf_types.h"
#include "qdf_status.h"
#include "nan_public_structs.h"
#include "wlan_objmgr_cmn.h"
#include "cfg_nan.h"
#include "sir_mac_prot_def.h"

struct wlan_objmgr_vdev;
struct wlan_objmgr_psoc;
struct scheduler_msg;

#define nan_alert(params...) \
	QDF_TRACE_FATAL(QDF_MODULE_ID_NAN, params)
#define nan_err(params...) \
	QDF_TRACE_ERROR(QDF_MODULE_ID_NAN, params)
#define nan_warn(params...) \
	QDF_TRACE_WARN(QDF_MODULE_ID_NAN, params)
#define nan_notice(params...) \
	QDF_TRACE_INFO(QDF_MODULE_ID_NAN, params)
#define nan_info(params...) \
	QDF_TRACE_INFO(QDF_MODULE_ID_NAN, params)
#define nan_debug(params...) \
	QDF_TRACE_DEBUG(QDF_MODULE_ID_NAN, params)

#define nan_nofl_alert(params...) \
	QDF_TRACE_FATAL_NO_FL(QDF_MODULE_ID_NAN, params)
#define nan_nofl_err(params...) \
	QDF_TRACE_ERROR_NO_FL(QDF_MODULE_ID_NAN, params)
#define nan_nofl_warn(params...) \
	QDF_TRACE_WARN_NO_FL(QDF_MODULE_ID_NAN, params)
#define nan_nofl_info(params...) \
	QDF_TRACE_INFO_NO_FL(QDF_MODULE_ID_NAN, params)
#define nan_nofl_debug(params...) \
	QDF_TRACE_DEBUG_NO_FL(QDF_MODULE_ID_NAN, params)

#define nan_alert_rl(params...) \
	QDF_TRACE_FATAL_RL(QDF_MODULE_ID_NAN, params)
#define nan_err_rl(params...) \
	QDF_TRACE_ERROR_RL(QDF_MODULE_ID_NAN, params)
#define nan_warn_rl(params...) \
	QDF_TRACE_WARN_RL(QDF_MODULE_ID_NAN, params)
#define nan_info_rl(params...) \
	QDF_TRACE_INFO_RL(QDF_MODULE_ID_NAN, params)
#define nan_debug_rl(params...) \
	QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_NAN, params)

/**
 * enum nan_disc_state - NAN Discovery states
 * @NAN_DISC_DISABLED: NAN Discovery is disabled
 * @NAN_DISC_ENABLE_IN_PROGRESS: NAN Discovery enable is in progress
 * @NAN_DISC_ENABLED: NAN Discovery is enabled
 * @NAN_DISC_DISABLE_IN_PROGRESS: NAN Discovery disable is in progress
 */
enum nan_disc_state {
	NAN_DISC_DISABLED,
	NAN_DISC_ENABLE_IN_PROGRESS,
	NAN_DISC_ENABLED,
	NAN_DISC_DISABLE_IN_PROGRESS,
};

/**
 * struct nan_cfg_params - NAN INI config params
 * @enable: NAN feature enable
 * @dp_enable: NAN Datapath feature enable
 * @ndi_mac_randomize: Randomize NAN datapath interface MAC
 * @nan_separate_iface_support: To supports separate iface creation for NAN
 * @support_mp0_discovery: To support discovery of NAN cluster with Master
 * Preference (MP) as 0 when a new device is enabling NAN
 * @disable_6g_nan: Disable NAN in 6GHz frequency band
 * @enable_nan_eht_cap: Enable(1)/Disable(0) NAN EHT capability
 * @support_sta_sap_ndp: support STA + SAP + NDP
 * @support_sta_p2p_ndp: support STA + P2P + NDP
 * @prefer_nan_chan_for_p2p: Prefer NAN social channels for P2P PCL
 * @reserved: Bits reserved for future use
 * @ndp_inactivity_timeout: NDP inactivity timeout
 * @ndp_keep_alive_period: To configure duration of how many seconds to
 * wait to kickout peer if peer is not reachable
 * @max_ndp_sessions: max ndp sessions host supports
 * @max_ndi: max number of ndi host supports
 * @nan_feature_config: Bitmap to enable/disable a particular NAN feature
 *                      configuration in firmware. It's sent to firmware through
 *                      wmi_vdev_param_enable_disable_nan_config_features
 */
struct nan_cfg_params {
	uint32_t enable:1;
	uint32_t dp_enable:1;
	uint32_t ndi_mac_randomize:1;
	uint32_t nan_separate_iface_support:1;
	uint32_t support_mp0_discovery:1;
	uint32_t disable_6g_nan:1;
	uint32_t enable_nan_eht_cap:1;
	uint32_t support_sta_sap_ndp:1;
	uint32_t support_sta_p2p_ndp:1;
	uint32_t prefer_nan_chan_for_p2p:1;
	uint32_t reserved:22;
	uint16_t ndp_inactivity_timeout;
	uint16_t ndp_keep_alive_period;
	uint32_t max_ndp_sessions;
	uint32_t max_ndi;
	uint32_t nan_feature_config;
};

/**
 * struct nan_psoc_priv_obj - nan private psoc obj
 * @lock: lock to be acquired before reading or writing to object
 * @cb_obj: struct containing callback pointers
 * @cfg_param: NAN Config parameters in INI
 * @nan_caps: NAN Target capabilities
 * @tx_ops: Tx ops registered with Target IF interface
 * @rx_ops: Rx  ops registered with Target IF interface
 * @disc_state: Present NAN Discovery state
 * @nan_social_ch_2g_freq: NAN 2G Social channel for discovery
 * @nan_social_ch_5g_freq: NAN 5G Social channel for discovery
 * @nan_disc_mac_id: MAC id used for NAN Discovery
 * @is_explicit_disable: Flag to indicate that NAN is being explicitly
 * disabled by driver or user-space
 * @ndp_request_ctx: NDP request context
 * @nan_disc_request_ctx: NAN discovery enable/disable request context
 * @nan_pairing_create_ctx: NAN Pairing create context
 * @nan_pairing_delete_ctx: NAN Pairing delete context
 * @nan_delete_all_peer_ctx: Delete all peer context
 * @fw_nan_addr: NAN MAC address which is randomized by target
 */
struct nan_psoc_priv_obj {
	qdf_spinlock_t lock;
	struct nan_callbacks cb_obj;
	struct nan_cfg_params cfg_param;
	struct nan_tgt_caps nan_caps;
	struct wlan_nan_tx_ops tx_ops;
	struct wlan_nan_rx_ops rx_ops;
	enum nan_disc_state disc_state;
	uint32_t nan_social_ch_2g_freq;
	uint32_t nan_social_ch_5g_freq;
	uint8_t nan_disc_mac_id;
	bool is_explicit_disable;
	void *ndp_request_ctx;
	void *nan_disc_request_ctx;
	void *nan_pairing_create_ctx;
	void *nan_pairing_delete_ctx;
	void *nan_delete_all_peer_ctx;
	struct qdf_mac_addr fw_nan_addr;
};

#define MAX_NAN_MIGRATED_PEERS 5
/**
 * struct nan_vdev_priv_obj - nan private vdev obj
 * @lock: lock to be acquired before reading or writing to object
 * @state: Current state of NDP
 * @active_ndp_peers: number of active ndp peers
 * @ndp_create_transaction_id: transaction id for create req
 * @ndp_delete_transaction_id: transaction id for delete req
 * @ndi_delete_rsp_reason: reason code for ndi_delete rsp
 * @ndi_delete_rsp_status: status for ndi_delete rsp
 * @primary_peer_mac: Primary NDP Peer mac address for the vdev
 * @disable_context: Disable all NDP's operation context
 * @ndp_init_done: Flag to indicate NDP initialization complete after first peer
 *		   connection.
 * @peer_mc_addr_list: Peer multicast address list
 * @num_pasn_peers: Number of NAN PASN peers
 * @is_delete_all_pasn_peer_in_progress: flag to track the deletion of all
 * pasn peers
 * @num_peer_migrated: Number of peers migrated
 * @peer_migrated_addr_list: list containing migrated peer mac address
 */
struct nan_vdev_priv_obj {
	qdf_spinlock_t lock;
	enum nan_datapath_state state;
	uint32_t active_ndp_peers;
	uint16_t ndp_create_transaction_id;
	uint16_t ndp_delete_transaction_id;
	uint32_t ndi_delete_rsp_reason;
	uint32_t ndi_delete_rsp_status;
	struct qdf_mac_addr primary_peer_mac;
	void *disable_context;
	bool ndp_init_done;
	struct qdf_mac_addr peer_mc_addr_list[MAX_NDP_SESSIONS];
	uint8_t num_pasn_peers;
	bool is_delete_all_pasn_peer_in_progress;
	uint8_t num_peer_migrated;
	struct qdf_mac_addr peer_migrated_addr_list[MAX_NAN_MIGRATED_PEERS];
};

/**
 * struct nan_peer_priv_obj - nan private peer obj
 * @lock: lock to be acquired before reading or writing to object
 * @active_ndp_sessions: number of active ndp sessions for this peer
 * @home_chan_info: Home channel info for the NDP associated with the Peer
 * @ndi_vdev_id: NDI vdev ID
 */
struct nan_peer_priv_obj {
	qdf_spinlock_t lock;
	uint32_t active_ndp_sessions;
	struct nan_datapath_channel_info home_chan_info;
	uint8_t ndi_vdev_id;
};

/**
 * nan_release_cmd: frees resources for NAN command.
 * @in_req: pointer to msg buffer to be freed
 * @req_type: type of request
 *
 * Return: None
 */
void nan_release_cmd(void *in_req, uint32_t req_type);

/**
 * nan_scheduled_msg_handler: callback pointer to be called when scheduler
 * starts executing enqueued NAN command.
 * @msg: pointer to msg
 *
 * Return: status of operation
 */
QDF_STATUS nan_scheduled_msg_handler(struct scheduler_msg *msg);

/**
 * nan_discovery_flush_callback: callback to flush the NAN scheduler msg
 * @msg: pointer to msg
 *
 * Return: QDF_STATUS
 */
QDF_STATUS nan_discovery_flush_callback(struct scheduler_msg *msg);

/**
 * nan_discovery_scheduled_handler: callback pointer to be called when scheduler
 * starts executing enqueued NAN command.
 * @msg: pointer to msg
 *
 * Return: status of operation
 */
QDF_STATUS nan_discovery_scheduled_handler(struct scheduler_msg *msg);

/*
 * nan_discovery_event_handler: function to process NAN events from firmware
 * @msg: message received from Target IF
 *
 * Return: status of operation
 */
QDF_STATUS nan_discovery_event_handler(struct scheduler_msg *msg);

/*
 * nan_datapath_event_handler: function to process NDP events from firmware
 * @msg: message received from Target IF
 *
 * Return: status of operation
 */
QDF_STATUS nan_datapath_event_handler(struct scheduler_msg *msg);

/*
 * nan_set_discovery_state: Attempts to set NAN Discovery state as the given one
 * @psoc: PSOC object
 * @new_state: Attempting to this NAN discovery state
 *
 * Return: status of operation
 */
QDF_STATUS nan_set_discovery_state(struct wlan_objmgr_psoc *psoc,
				   enum nan_disc_state new_state);

/*
 * nan_discovery_pre_enable: Takes steps before sending NAN Enable to Firmware
 * @pdev: pdev object
 * @nan_ch_freq: Primary social channel for NAN Discovery
 *
 * Return: status of operation
 */
QDF_STATUS nan_discovery_pre_enable(struct wlan_objmgr_pdev *pdev,
				    uint32_t nan_ch_freq);

/*
 * nan_get_discovery_state: Returns the current NAN Discovery state
 * @psoc: PSOC object
 *
 * Return: Current NAN Discovery state
 */
enum nan_disc_state nan_get_discovery_state(struct wlan_objmgr_psoc *psoc);

/*
 * nan_is_enable_allowed: Queries whether NAN Discovery is allowed
 * @psoc: PSOC object
 * @nan_ch_freq: Possible primary social channel for NAN Discovery
 * @vdev_id: Vdev Id
 *
 * Return: True if NAN Enable is allowed on given channel, False otherwise
 */
bool nan_is_enable_allowed(struct wlan_objmgr_psoc *psoc, uint32_t nan_ch_freq,
			   uint8_t vdev_id);

/*
 * nan_is_disc_active: Queries whether NAN Discovery is active
 * @psoc: PSOC object
 *
 * Return: True if NAN Disc is active, False otherwise
 */
bool nan_is_disc_active(struct wlan_objmgr_psoc *psoc);

/*
 * nan_get_connection_info: Gets connection info of the NAN Discovery interface
 * @psoc: PSOC object
 * @chan: NAN Social channel to be returned
 * @mac_if: MAC ID associated with NAN Discovery
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
nan_get_connection_info(struct wlan_objmgr_psoc *psoc, uint8_t *chan,
			uint8_t *mac_id);

/**
 * nan_get_vdev_id_from_bssid() -get NAN vdev_id for NAN BSSID
 * @pdev: PDEV object
 * @bssid: BSSID present in mgmt frame
 * @dbg_id: Object Manager ref debug id
 *
 * API to get NAN vdev_id for only NAN BSSID.
 *
 * Return: NAN vdev_id
 */
uint8_t nan_get_vdev_id_from_bssid(struct wlan_objmgr_pdev *pdev,
				   tSirMacAddr bssid,
				   wlan_objmgr_ref_dbgid dbg_id);

/*
 * nan_is_sta_sta_concurrency_present: Queries whether STA + STA concurrency
 * present
 * @psoc: PSOC object
 *
 * Return: True if concurrency is present, False otherwise
 */
bool nan_is_sta_sta_concurrency_present(struct wlan_objmgr_psoc *psoc);

/**
 * nan_is_pairing_allowed() - check NAN pairing capability
 * @psoc: pointer to psoc object
 *
 * Return: Boolean flag indicating whether the NAN pairing allowed or not
 */
bool nan_is_pairing_allowed(struct wlan_objmgr_psoc *psoc);

/**
 * nan_is_peer_exist_for_opmode() - check whether peer is exist or not for given
 * opmode.
 * @psoc: pointer to psoc object
 * @peer_mac_addr: peer mac address
 * @opmode: OP mode
 *
 * Return: Boolean flag indicating whether the peer exists or not
 */
bool nan_is_peer_exist_for_opmode(struct wlan_objmgr_psoc *psoc,
				  struct qdf_mac_addr *peer_mac_addr,
				  enum QDF_OPMODE opmode);

/**
 * nan_update_pasn_peer_count() - Increment or Decrement pasn peer count
 * @vdev: Pointer to vdev object
 * @is_increment: flag to indicate if peer count needs to be incremented
 *
 * Return: None
 */
void nan_update_pasn_peer_count(struct wlan_objmgr_vdev *vdev,
				bool is_increment);

/*
 * nan_pasn_flush_callback: callback to flush the NAN PASN scheduler msg
 * @msg: pointer to msg
 *
 * Return: QDF_STATUS
 */
QDF_STATUS nan_pasn_flush_callback(struct scheduler_msg *msg);

/**
 * nan_pasn_scheduled_handler: callback pointer to be called when scheduler
 * starts executing enqueued NAN command for PASN
 * @msg: pointer to msg
 *
 * Return: status of operation
 */
QDF_STATUS nan_pasn_scheduled_handler(struct scheduler_msg *msg);

/*
 * nan_handle_pasn_peer_create_rsp: This API handle pasn peer create response
 * by clearing wait timer.
 * @psoc: PSOC object
 * @vdev_id: vdev id
 * @peer_mac: peer mac address
 * @peer_create_status: peer create status
 *
 * Return: None
 */
void nan_handle_pasn_peer_create_rsp(struct wlan_objmgr_psoc *psoc,
				     uint8_t vdev_id,
				     struct qdf_mac_addr *peer_mac,
				     uint8_t peer_create_status);
/**
 * nan_pasn_peer_handle_del_rsp: handle psan peer delete response
 * @psoc: pointer to psoc object
 * @peer_mac: address of peer
 * @vdev_id: vdev id
 *
 * Return: None
 */
void nan_pasn_peer_handle_del_rsp(struct wlan_objmgr_psoc *psoc,
				  uint8_t *peer_mac, uint8_t vdev_id);
/**
 * nan_handle_delete_all_pasn_peers: handle response for all PASN peers delete
 * cmd for NAN
 * @psoc: pointer to psoc object
 * @vdev_id: vdev id
 *
 * Return: Success when handled response, otherwise error
 */
QDF_STATUS nan_handle_delete_all_pasn_peers(struct wlan_objmgr_psoc *psoc,
					    uint8_t vdev_id);

/**
 * nan_cleanup_pasn_peers() - Delete all PASN peer objects for given vdev
 * @psoc: Pointer to psoc object
 *
 * Return: QDF_STATUS
 */
QDF_STATUS nan_cleanup_pasn_peers(struct wlan_objmgr_psoc *psoc);

/*
 * ndi_add_pasn_peer_to_nan(): This API will add PASN peer to NAN VDEV on
 * peer migration
 * @psoc: pointer to PSOC object
 * @nan_vdev_id: VDEV ID
 * @peer_mac: address of peer
 *
 * Return: Success when handled response, otherwise error
 */
QDF_STATUS
ndi_add_pasn_peer_to_nan(struct wlan_objmgr_psoc *psoc, uint8_t nan_vdev_id,
			 struct qdf_mac_addr *peer_mac);

/**
 * nan_get_fw_addr() - get NAN MAC address randomized by target.
 * @psoc: pointer to psoc object
 *
 * Return: NAN MAC address
 */
struct qdf_mac_addr *nan_get_fw_addr(struct wlan_objmgr_psoc *psoc);
#endif /* _WLAN_NAN_MAIN_I_H_ */
#endif /* WLAN_FEATURE_NAN */
