/*
 * Copyright (c) 2017-2019, 2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: Contains p2p public data structure definitions
 */

#ifndef _WLAN_P2P_PUBLIC_STRUCT_H_
#define _WLAN_P2P_PUBLIC_STRUCT_H_

#include <qdf_types.h>

#define P2P_MAX_NOA_DESC 4

#define P2P_MC_ADDR      "\x51\x6F\x9A\x02"
#define P2P_MC_ADDR_SIZE 4

#define HEADER_LEN_P2P_IE  6

#define P2P_1X1_WAR_OUI   "\x00\x50\xf2\x04"
#define P2P_1X1_OUI_LEN    4

#define MAX_CONFIG_METHODS_LEN   2
#define DEVICE_CATEGORY_MAX_LEN  1

#define P2P_OUI                  "\x50\x6f\x9a\x09"
#define P2P_OUI_SIZE             4

#define P2P2_OUI                 "\x50\x6f\x9a\x28"
#define P2P2_OUI_SIZE            4

/* Indicate if scan type is p2p or not */
#define BIT_P2P_SCAN_IN_STA_VDEV 0

#define P2P_SCAN_IN_STA_VDEV_FLAG BIT(BIT_P2P_SCAN_IN_STA_VDEV)

/**
 * struct p2p_ps_params - P2P powersave related params
 * @opp_ps: opportunistic power save
 * @ctwindow: CT window
 * @count: count
 * @duration: duration
 * @interval: interval
 * @single_noa_duration: single shot noa duration
 * @ps_selection: power save selection
 * @session_id: session id
 * @start: start time
 */
struct p2p_ps_params {
	uint8_t opp_ps;
	uint32_t ctwindow;
	uint8_t count;
	uint32_t duration;
	uint32_t interval;
	uint32_t single_noa_duration;
	uint8_t ps_selection;
	uint8_t session_id;
	uint32_t start;
};

/**
 * struct p2p_roc_req - P2P roc request
 * @vdev_id:     Vdev id on which this request has come
 * @chan_freq:   Chan frequency for which this RoC has been requested
 * @phy_mode:    PHY mode
 * @duration:    Duration for the RoC
 */
struct p2p_roc_req {
	uint32_t vdev_id;
	qdf_freq_t chan_freq;
	uint32_t phy_mode;
	uint32_t duration;
};

/**
 * enum p2p_roc_event - P2P RoC event
 * @ROC_EVENT_READY_ON_CHAN:  RoC has started now
 * @ROC_EVENT_COMPLETED:      RoC has been completed
 * @ROC_EVENT_INAVLID:        Invalid event
 */
enum p2p_roc_event {
	ROC_EVENT_READY_ON_CHAN = 0,
	ROC_EVENT_COMPLETED,
	ROC_EVENT_INAVLID,
};

/**
 * struct p2p_event - p2p event
 * @vdev_id:     Vdev id
 * @roc_event:   RoC event
 * @cookie:      Cookie which is given to supplicant for this roc req
 * @chan_freq:   Chan frequency for which this RoC has been requested
 * @duration:    Duration for the RoC
 * @opmode:      Interface type of RoC
 * @flag:        Indicate scan type. BIT[0] indicate whether
 *               scan type is for p2p or not when sta vdev gets
 *               use during p2p device mode operation
 */
struct p2p_event {
	uint32_t vdev_id;
	enum p2p_roc_event roc_event;
	uint64_t cookie;
	qdf_freq_t chan_freq;
	uint32_t duration;
	enum QDF_OPMODE opmode;
	uint32_t flag;
};

/**
 * struct p2p_rx_mgmt_frame - rx mgmt frame structure
 * @frame_len:   Frame length
 * @rx_freq:     RX Frequency
 * @vdev_id:     Vdev id
 * @frm_type:    Frame type
 * @rx_rssi:     RX rssi
 * @buf:         Buffer address
 */
struct p2p_rx_mgmt_frame {
	uint32_t frame_len;
	uint32_t rx_freq;
	uint32_t vdev_id;
	uint32_t frm_type;
	uint32_t rx_rssi;
	QDF_FLEX_ARRAY(uint8_t, buf);
};

/**
 * struct p2p_tx_cnf - tx confirm structure
 * @vdev_id:        Vdev id
 * @action_cookie:  TX cookie for this action frame
 * @buf_len:        Frame length
 * @status:         TX status
 * @buf:            Buffer address
 * @opmode:         interface type on which tx mgmt frame came
 */
struct p2p_tx_cnf {
	uint32_t vdev_id;
	uint64_t action_cookie;
	uint32_t buf_len;
	uint32_t status;
	uint8_t *buf;
	enum QDF_OPMODE opmode;
};

/**
 * struct p2p_mgmt_tx - p2p mgmt tx structure
 * @vdev_id:             Vdev id
 * @chan_freq:           Chan frequency for which this RoC has been requested
 * @wait:                Duration for the RoC
 * @len:                 Length of tx buffer
 * @no_cck:              Required cck or not
 * @dont_wait_for_ack:   Wait for ack or not
 * @off_chan:            Off channel tx or not
 * @buf:                 TX buffer
 * @opmode:              Interface type on which mgmt tx came
 * @flag:        Indicate scan type. BIT[0] indicate whether
 *               scan type is for p2p or not when sta vdev gets
 *               use during p2p device mode operation
 */
struct p2p_mgmt_tx {
	uint32_t vdev_id;
	qdf_freq_t chan_freq;
	uint32_t wait;
	uint32_t len;
	uint32_t no_cck;
	uint32_t dont_wait_for_ack;
	uint32_t off_chan;
	const uint8_t *buf;
	enum QDF_OPMODE opmode;
	uint32_t flag;
};

/**
 * struct p2p_set_mac_filter_evt
 * @vdev_id: Vdev id
 * @status: target reported result of set mac addr filter
 */
struct p2p_set_mac_filter_evt {
	uint32_t vdev_id;
	uint32_t status;
};

/**
 * struct p2p_ps_config
 * @vdev_id:               Vdev id
 * @opp_ps:                Opportunistic power save
 * @ct_window:             CT window
 * @count:                 Count
 * @duration:              Duration
 * @interval:              Interval
 * @single_noa_duration:   Single shot noa duration
 * @ps_selection:          power save selection
 * @start:                 Start time
 */
struct p2p_ps_config {
	uint32_t vdev_id;
	uint32_t opp_ps;
	uint32_t ct_window;
	uint32_t count;
	uint32_t duration;
	uint32_t interval;
	uint32_t single_noa_duration;
	uint32_t ps_selection;
	uint32_t start;
};

/**
 * struct p2p_lo_start - p2p listen offload start
 * @vdev_id:            Vdev id
 * @ctl_flags:          Control flag
 * @freq:               P2P listen frequency
 * @period:             Listen offload period
 * @interval:           Listen offload interval
 * @count:              Number listen offload intervals
 * @dev_types_len:      Device types length
 * @probe_resp_len:     Probe response template length
 * @device_types:       Device types
 * @probe_resp_tmplt:   Probe response template
 */
struct p2p_lo_start {
	uint32_t vdev_id;
	uint32_t ctl_flags;
	uint32_t freq;
	uint32_t period;
	uint32_t interval;
	uint32_t count;
	uint32_t dev_types_len;
	uint32_t probe_resp_len;
	uint8_t  *device_types;
	uint8_t  *probe_resp_tmplt;
};

/**
 * struct p2p_lo_event
 * @vdev_id:        vdev id
 * @reason_code:    reason code
 */
struct p2p_lo_event {
	uint32_t vdev_id;
	uint32_t reason_code;
};

/**
 * struct noa_descriptor - noa descriptor
 * @type_count:     255: continuous schedule, 0: reserved
 * @duration:       Absent period duration in micro seconds
 * @interval:       Absent period interval in micro seconds
 * @start_time:     32 bit tsf time when in starts
 */
struct noa_descriptor {
	uint32_t type_count;
	uint32_t duration;
	uint32_t interval;
	uint32_t start_time;
};

/**
 * struct p2p_noa_info - p2p noa information
 * @index:             identifies instance of NOA su element
 * @opps_ps:           opps ps state of the AP
 * @ct_window:         ct window in TUs
 * @vdev_id:           vdev id
 * @num_desc:          number of NOA descriptors
 * @noa_desc:          noa descriptors
 */
struct p2p_noa_info {
	uint32_t index;
	uint32_t opps_ps;
	uint32_t ct_window;
	uint32_t vdev_id;
	uint32_t num_desc;
	struct noa_descriptor noa_desc[P2P_MAX_NOA_DESC];
};

/**
 * struct p2p_chan_switch_req_params - Channel switch parameters for P2P GO/CLI
 * @p2p_soc_obj: P2P soc priv object
 * @vdev_id: VDEV ID of the p2p entity
 * @channel: Channel number of channel switch request
 * @op_class: operating class of channel switch request
 */
struct p2p_chan_switch_req_params {
	struct p2p_soc_priv_obj *p2p_soc_obj;
	uint8_t vdev_id;
	uint8_t channel;
	uint8_t op_class;
};

/**
 * struct p2p_ap_assist_dfs_group_params - Params of assisted AP for DFS
 * P2P group
 * @vdev_id: VDEV ID of p2p entity
 * @bssid: BSSID of the assisted infra BSS
 * @non_tx_bssid: Non-TxBSSID of the assisted infra BSS
 */
struct p2p_ap_assist_dfs_group_params {
	uint8_t vdev_id;
	struct qdf_mac_addr bssid;
	struct qdf_mac_addr non_tx_bssid;
};

/**
 * struct p2p_protocol_callbacks - callback to non-converged driver
 * @is_mgmt_protected: func to get 11w mgmt protection status
 * @ap_assist_dfs_group_bmiss_notify: Received BMISS event from FW for DFS group
 * which is in infra BSS assisted mode.
 * @p2p_group_chan_switch_req: Channel switch request params for
 * P2P group entity
 * @ap_assist_dfs_group_fw_monitor_update: Update FW monitoring status for the
 * assisted infra BSS in DFS operation
 */
struct p2p_protocol_callbacks {
	bool (*is_mgmt_protected)(uint32_t vdev_id, const uint8_t *peer_addr);
	void (*ap_assist_dfs_group_bmiss_notify)(uint8_t vdev_id);
	void (*p2p_group_chan_switch_req)(uint8_t vdev_id, uint8_t chan,
					  uint8_t op_class);
	void (*ap_assist_dfs_group_fw_monitor_update)(uint8_t vdev_id,
						      bool val);
};

/**
 * enum p2p_attr_id - enum for P2P attributes ID in P2P IE
 * @P2P_ATTR_STATUS: Attribute Status none
 * @P2P_ATTR_MINOR_REASON_CODE: Minor reason code attribute
 * @P2P_ATTR_CAPABILITY: Capability attribute
 * @P2P_ATTR_DEVICE_ID: device ID attribute
 * @P2P_ATTR_GROUP_OWNER_INTENT: Group owner intent attribute
 * @P2P_ATTR_CONFIGURATION_TIMEOUT: Config timeout attribute
 * @P2P_ATTR_LISTEN_CHANNEL: listen channel attribute
 * @P2P_ATTR_GROUP_BSSID: Group BSSID attribute
 * @P2P_ATTR_EXT_LISTEN_TIMING: Listen timing attribute
 * @P2P_ATTR_INTENDED_INTERFACE_ADDR: Intended interface address attribute
 * @P2P_ATTR_MANAGEABILITY:  Manageability attribute
 * @P2P_ATTR_CHANNEL_LIST: Channel list attribute
 * @P2P_ATTR_NOTICE_OF_ABSENCE: Notice of Absence attribute
 * @P2P_ATTR_DEVICE_INFO: Device Info attribute
 * @P2P_ATTR_GROUP_INFO: Group Info attribute
 * @P2P_ATTR_GROUP_ID: Group ID attribute
 * @P2P_ATTR_INTERFACE: Interface attribute
 * @P2P_ATTR_OPERATING_CHANNEL: Operating channel attribute
 * @P2P_ATTR_INVITATION_FLAGS: Invitation flags attribute
 * @P2P_ATTR_OOB_GO_NEG_CHANNEL: GO neg channel attribute
 * @P2P_ATTR_SERVICE_HASH: Service HASH attribute
 * @P2P_ATTR_SESSION_INFORMATION_DATA: Session Info data attribute
 * @P2P_ATTR_CONNECTION_CAPABILITY: Connection capability attribute
 * @P2P_ATTR_ADVERTISEMENT_ID: Advertisement ID attribute
 * @P2P_ATTR_ADVERTISED_SERVICE: Advertised Service attribute
 * @P2P_ATTR_SESSION_ID: Session ID attribute
 * @P2P_ATTR_FEATURE_CAPABILITY: Feature capability attribute
 * @P2P_ATTR_PERSISTENT_GROUP: Persistent group attribute
 * @P2P_ATTR_EXT_CAPABILITY: P2P extended capability attribute in P2P2 IE
 * @P2P_ATTR_WLAN_AP_INFO: WLAN AP info attribute in P2P2 IE
 * @P2P_ATTR_VENDOR_SPECIFIC: Vendor specific attribute
 */
enum p2p_attr_id {
	P2P_ATTR_STATUS = 0,
	P2P_ATTR_MINOR_REASON_CODE = 1,
	P2P_ATTR_CAPABILITY = 2,
	P2P_ATTR_DEVICE_ID = 3,
	P2P_ATTR_GROUP_OWNER_INTENT = 4,
	P2P_ATTR_CONFIGURATION_TIMEOUT = 5,
	P2P_ATTR_LISTEN_CHANNEL = 6,
	P2P_ATTR_GROUP_BSSID = 7,
	P2P_ATTR_EXT_LISTEN_TIMING = 8,
	P2P_ATTR_INTENDED_INTERFACE_ADDR = 9,
	P2P_ATTR_MANAGEABILITY = 10,
	P2P_ATTR_CHANNEL_LIST = 11,
	P2P_ATTR_NOTICE_OF_ABSENCE = 12,
	P2P_ATTR_DEVICE_INFO = 13,
	P2P_ATTR_GROUP_INFO = 14,
	P2P_ATTR_GROUP_ID = 15,
	P2P_ATTR_INTERFACE = 16,
	P2P_ATTR_OPERATING_CHANNEL = 17,
	P2P_ATTR_INVITATION_FLAGS = 18,
	P2P_ATTR_OOB_GO_NEG_CHANNEL = 19,
	P2P_ATTR_SERVICE_HASH = 21,
	P2P_ATTR_SESSION_INFORMATION_DATA = 22,
	P2P_ATTR_CONNECTION_CAPABILITY = 23,
	P2P_ATTR_ADVERTISEMENT_ID = 24,
	P2P_ATTR_ADVERTISED_SERVICE = 25,
	P2P_ATTR_SESSION_ID = 26,
	P2P_ATTR_FEATURE_CAPABILITY = 27,
	P2P_ATTR_PERSISTENT_GROUP = 28,
	P2P_ATTR_EXT_CAPABILITY = 29,
	P2P_ATTR_WLAN_AP_INFO = 30,
	P2P_ATTR_VENDOR_SPECIFIC = 221
};

#ifdef FEATURE_WLAN_SUPPORT_USD
#define P2P_USD_SERVICE_LEN                    6
#define P2P_USD_SSI_LEN                        1400
#define P2P_USD_FRAME_LEN                      1024
#define P2P_USD_CHAN_CONFIG_FREQ_LIST_MAX_SIZE 10

/**
 * enum p2p_usd_op_type: OP type for P2P USD
 * @P2P_USD_OP_TYPE_FLUSH: Indicates USD tear down of all active publish and
 * subscribe sessions.
 *
 * @P2P_USD_OP_TYPE_PUBLISH: Indicates USD solicited publish operation that
 * enables to offer a service for other devices based on given parameters.
 *
 * @P2P_USD_OP_TYPE_SUBSCRIBE: Indicates USD active subscribe operation that
 * requests for a given service with given parameters from other devices that
 * offer the service.
 *
 * @P2P_USD_OP_TYPE_UPDATE_PUBLISH: Indicates update of an instance of the
 * publish function of given publish id.
 *
 * @P2P_USD_OP_TYPE_CANCEL_PUBLISH: Indicates cancellation of an instance of
 * the publish function.
 *
 * @P2P_USD_OP_TYPE_CANCEL_SUBSCRIBE: Indicates cancellation of an instance of
 * the subscribe function.
 */
enum p2p_usd_op_type {
	P2P_USD_OP_TYPE_FLUSH = 0,
	P2P_USD_OP_TYPE_PUBLISH = 1,
	P2P_USD_OP_TYPE_SUBSCRIBE = 2,
	P2P_USD_OP_TYPE_UPDATE_PUBLISH = 3,
	P2P_USD_OP_TYPE_CANCEL_PUBLISH = 4,
	P2P_USD_OP_TYPE_CANCEL_SUBSCRIBE = 5,
};

/**
 * enum p2p_usd_service_protocol_type: USD service protocol type for P2P
 * @P2P_USD_SERVICE_PROTOCOL_TYPE_BONJOUR: Indicates SSI info is of type Bonjour
 * @P2P_USD_SERVICE_PROTOCOL_TYPE_GENERIC: Indicates SSI info is of type generic
 * @P2P_USD_SERVICE_PROTOCOL_TYPE_CSA_MATTER: Indicates SSI info is of type
 * CSA/Matter
 */
enum p2p_usd_service_protocol_type {
	P2P_USD_SERVICE_PROTOCOL_TYPE_BONJOUR = 1,
	P2P_USD_SERVICE_PROTOCOL_TYPE_GENERIC = 2,
	P2P_USD_SERVICE_PROTOCOL_TYPE_CSA_MATTER = 3,
};

/**
 * struct p2p_usd_data - containing frame data and length
 * @data: array for frame data
 * @len: frame length
 */
struct p2p_usd_data {
	uint8_t *data;
	uint32_t len;
};

/**
 * struct p2p_usd_freq_list - containing frequency list and length
 * @freq: list of frequency
 * @len: total bytes in frequency list
 */
struct p2p_usd_freq_list {
	uint8_t *freq;
	uint32_t len;
};

/**
 * struct p2p_usd_freq_config - USd channel configuration
 * @default_freq: default frequency
 * @freq_list: frequency list structure
 */
struct p2p_usd_freq_config {
	uint32_t default_freq;
	struct p2p_usd_freq_list freq_list;
};

/**
 * struct p2p_usd_service_info - containing service information
 * @service_id: 6 bytes of service ID
 * @protocol_type: protocol type
 * @len: service ID length
 */
struct p2p_usd_service_info {
	uint8_t *service_id;
	enum p2p_usd_service_protocol_type protocol_type;
	uint8_t len;
};

/**
 * struct p2p_usd_ssi - containing SSI information
 * @data: array for SSI data
 * @len: SSI length
 */
struct p2p_usd_ssi {
	uint8_t *data;
	uint32_t len;
};

/**
 * struct p2p_usd_attr_params - containing USD attributes parameters
 * @vdev_id: VDEV ID
 * @op_type: OP type
 * @p2p_mac_addr: P2P MAC address
 * @instance_id: instance ID
 * @service_info: Service information structure
 * @ssi: SSI information structure
 * @freq_config: frequency configuration structure
 * @frame: Frame information structure
 * @ttl: (Time to live) Timeout for each request
 */
struct p2p_usd_attr_params {
	uint32_t vdev_id;
	enum p2p_usd_op_type op_type;
	struct qdf_mac_addr p2p_mac_addr;
	uint8_t instance_id;
	struct p2p_usd_service_info service_info;
	struct p2p_usd_ssi ssi;
	struct p2p_usd_freq_config freq_config;
	struct p2p_usd_data frame;
	uint16_t ttl;
};
#endif /* FEATURE_WLAN_SUPPORT_USD */
#endif /* _WLAN_P2P_PUBLIC_STRUCT_H_ */
