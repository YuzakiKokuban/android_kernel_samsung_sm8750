/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

#ifndef _WLAN_DP_RESOURCE_MGR_H_
#define _WLAN_DP_RESOURCE_MGR_H_

#include "wlan_dp_priv.h"

#ifdef	DP_DYNAMIC_RESOURCE_MGMT_DEBUG_ENABLE
#define dp_rsrc_mgr_debug dp_info
#else
#define dp_rsrc_mgr_debug(params...)
#endif

/*Max throughput(Mbps) supported in different resource levels*/
#define RESOURCE_LVL_1_TPUT_MBPS  2400
#define RESOURCE_LVL_2_TPUT_MBPS  5900

#if defined(QCA_WIFI_WCN7750) || defined(QCA_WIFI_QCA6750) || \
    defined(QCA_WIFI_WCN6450)
#define MAX_MAC_RESOURCES 1
/*RX buffers required in different resource levels*/
#define RESOURCE_LVL_1_RX_BUFFERS (6 * 1024)
#else
#define MAX_MAC_RESOURCES 2
/*RX buffers required in different resource levels*/
#define RESOURCE_LVL_1_RX_BUFFERS (10 * 1024)
#endif
#define RESOURCE_LVL_2_RX_BUFFERS ((16 * 1024) - 1)
#define UNKNOWN_MAC_ID 0xf

#define DP_RSRC_MGR_TIMER_MS	1000

/**
 * enum wlan_dp_resource_level - DP Resource levels
 * @RESOURCE_LVL_1: Default Resource level
 * @RESOURCE_LVL_2: Higher resource level in peak tput
 * @RESOURCE_LVL_MAX: MAX resource level undefined
 */
enum wlan_dp_resource_level {
	RESOURCE_LVL_1,
	RESOURCE_LVL_2,
	RESOURCE_LVL_MAX
};

/**
 * struct wlan_dp_resource_map - DP Resource map config
 * @resource_level: Resource level
 * @max_tput: MAX throughput current level supports
 * @num_rx_buffers: MAX number of RX buffers required
 */
struct wlan_dp_resource_map {
	enum wlan_dp_resource_level resource_level;
	uint16_t max_tput;
	uint16_t num_rx_buffers;
};

/*DP resource MAP used in resource level selection*/
static struct wlan_dp_resource_map dp_resource_map[] = {
	{RESOURCE_LVL_1, RESOURCE_LVL_1_TPUT_MBPS, RESOURCE_LVL_1_RX_BUFFERS},
	{RESOURCE_LVL_2, RESOURCE_LVL_2_TPUT_MBPS, RESOURCE_LVL_2_RX_BUFFERS},
};

/**
 * struct wlan_dp_resource_ml_vote_info - DP Resource ML vote
 * common info
 * @ref_cnt: ref count of active links
 * @num_links: number of links in ML connections
 */
struct wlan_dp_resource_ml_vote_info {
	qdf_atomic_t ref_cnt;
	uint8_t num_links;
};

/**
 * struct wlan_dp_resource_vote_node - DP Resource vote node context
 * @priv_ctx: DP peer priv context ref
 * @peer: OBJMGR peer reference
 * @self_mac: Self MAC address
 * @remote_mac: Remote MAC address
 * @ml_vote_info: ML vote common info
 * @phymode: phymode it can operate
 * @phymode0: phymode on MAC-0
 * @phymode1: phymode on MAC-1
 * @opmode: Operating mode of vote node
 * @node: List node entry
 * @tput: Max throughput of phymode
 * @tput0: Max throughput on MAC-0
 * @tput1: Max throughput on MAC-1
 * @vdev_id: Vdev id
 * @mac_id: HW MAC id
 */
struct wlan_dp_resource_vote_node {
	struct wlan_dp_peer_priv_context *priv_ctx;
	struct wlan_objmgr_peer *peer;
	struct qdf_mac_addr self_mac;
	struct qdf_mac_addr remote_mac;
	struct wlan_dp_resource_ml_vote_info *ml_vote_info;
	enum wlan_phymode phymode;
	enum wlan_phymode phymode0;
	enum wlan_phymode phymode1;
	enum QDF_OPMODE opmode;
	qdf_list_node_t node;
	uint64_t tput;
	uint64_t tput0;
	uint64_t tput1;
	uint32_t vdev_id;
	uint32_t mac_id;
};

/**
 * struct wlan_dp_resource_mgr_ctx - DP Resource manager context
 * @dp_ctx: DP context reference
 * @cur_rsrc_map: Current DP resource map
 * @cur_resource_level: current resource level for active connections
 * @monitor_vdev_vote: Monitor vdev present
 * @cur_max_tput: Current MAX throughput
 * @mac_n_list: MAC-n list for connections mac_id not fixed
 * @mac_list: MAC based vote nodes list
 * @nan_list: NAN connections vote node list
 * @mac_count: total number of MACs supported
 * @timer: Resource manager timer handle
 * @max_phymode_nodes: max phymodes selected across system
 * @rsrc_mgr_lock: lock to protect operations from parallel contexts
 */
struct wlan_dp_resource_mgr_ctx {
	struct wlan_dp_psoc_context *dp_ctx;
	struct wlan_dp_resource_map cur_rsrc_map[RESOURCE_LVL_MAX];
	enum wlan_dp_resource_level cur_resource_level;
	bool monitor_vdev_vote;
	uint64_t cur_max_tput;
	qdf_list_t mac_n_list;
	qdf_list_t mac_list[MAX_MAC_RESOURCES];
	qdf_list_t nan_list;
	uint32_t mac_count;
	qdf_timer_t timer;
	struct wlan_dp_resource_vote_node *max_phymode_nodes[MAX_MAC_RESOURCES];
	qdf_spinlock_t rsrc_mgr_lock;
};

#ifdef WLAN_DP_DYNAMIC_RESOURCE_MGMT

/*RX buffers replenished based on batch manner*/
#define RX_RESOURCE_ALLOC_BATCH_COUNT			64

#define  WLAN_PHYMODE_11A_TPUT_MBPS			54
#define  WLAN_PHYMODE_11B_TPUT_MBPS			12
#define  WLAN_PHYMODE_11G_TPUT_MBPS			54
#define  WLAN_PHYMODE_11G_ONLY_TPUT_MBPS		54
#define  WLAN_PHYMODE_11NA_HT20_TPUT_MBPS		150
#define  WLAN_PHYMODE_11NG_HT20_TPUT_MBPS		150
#define  WLAN_PHYMODE_11NA_HT40_TPUT_MBPS		300
#define  WLAN_PHYMODE_11NG_HT40PLUS_TPUT_MBPS		300
#define  WLAN_PHYMODE_11NG_HT40MINUS_TPUT_MBPS		300
#define  WLAN_PHYMODE_11NG_HT40_TPUT_MBPS		300
#define  WLAN_PHYMODE_11AC_VHT20_TPUT_MBPS		275
#define  WLAN_PHYMODE_11AC_VHT20_2G_TPUT_MBPS		275
#define  WLAN_PHYMODE_11AC_VHT40_TPUT_MBPS		550
#define  WLAN_PHYMODE_11AC_VHT40PLUS_2G_TPUT_MBPS	550
#define  WLAN_PHYMODE_11AC_VHT40MINUS_2G_TPUT_MBPS	550
#define  WLAN_PHYMODE_11AC_VHT40_2G_TPUT_MBPS		550
#define  WLAN_PHYMODE_11AC_VHT80_TPUT_MBPS		1100
#define  WLAN_PHYMODE_11AC_VHT80_2G_TPUT_MBPS		1100
#define  WLAN_PHYMODE_11AC_VHT160_TPUT_MBPS		2200
#define  WLAN_PHYMODE_11AC_VHT80_80_TPUT_MBPS		2200
#define  WLAN_PHYMODE_11AXA_HE20_TPUT_MBPS		300
#define  WLAN_PHYMODE_11AXG_HE20_TPUT_MBPS		300
#define  WLAN_PHYMODE_11AXA_HE40_TPUT_MBPS		600
#define  WLAN_PHYMODE_11AXG_HE40PLUS_TPUT_MBPS		600
#define  WLAN_PHYMODE_11AXG_HE40MINUS_TPUT_MBPS		600
#define  WLAN_PHYMODE_11AXG_HE40_TPUT_MBPS		600
#define  WLAN_PHYMODE_11AXA_HE80_TPUT_MBPS		1200
#define  WLAN_PHYMODE_11AXG_HE80_TPUT_MBPS		1200
#define  WLAN_PHYMODE_11AXA_HE160_TPUT_MBPS		2400
#define  WLAN_PHYMODE_11AXA_HE80_80_TPUT_MBPS		2400
#ifdef WLAN_FEATURE_11BE
#define  WLAN_PHYMODE_11BEA_EHT20_TPUT_MBPS		368
#define  WLAN_PHYMODE_11BEG_EHT20_TPUT_MBPS		368
#define  WLAN_PHYMODE_11BEA_EHT40_TPUT_MBPS		737
#define  WLAN_PHYMODE_11BEG_EHT40PLUS_TPUT_MBPS		737
#define  WLAN_PHYMODE_11BEG_EHT40MINUS_TPUT_MBPS	737
#define  WLAN_PHYMODE_11BEG_EHT40_TPUT_MBPS		737
#define  WLAN_PHYMODE_11BEA_EHT80_TPUT_MBPS		1475
#define  WLAN_PHYMODE_11BEG_EHT80_TPUT_MBPS		1475
#define  WLAN_PHYMODE_11BEA_EHT160_TPUT_MBPS		2950
#define  WLAN_PHYMODE_11BEA_EHT320_TPUT_MBPS		5900
#endif
#define  WLAN_PHYMODE_DEFAULT_TPUT_MBPS			2400

/**
 * wlan_dp_resource_mgr_max_of_two_vote_nodes() - Max of two phymodes is
 * selected based on throughput requirement
 * @node1: vote node1 reference
 * @node2: vote node2 reference
 *
 * Return: Max vote node out of both
 */
static inline struct wlan_dp_resource_vote_node*
wlan_dp_resource_mgr_max_of_two_vote_nodes(
				struct wlan_dp_resource_vote_node *node1,
				struct wlan_dp_resource_vote_node *node2)

{
	if (node1 && node2)
		return node1->tput > node2->tput ? node1 : node2;
	else if (node1)
		return node1;
	else if (node2)
		return node2;

	return NULL;
}

/**
 * wlan_dp_resource_mgr_detect_tput_level_increase() - Detect tput level
 * increase for current vote node
 * @rsrc_ctx: DP resource context
 * @vote_node: vote node
 * @prev_tput: previous throughput of current vote node
 *
 * Return: True if throughput increase for current vote node
 */
static inline bool
wlan_dp_resource_mgr_detect_tput_level_increase(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_dp_resource_vote_node *vote_node,
				uint64_t prev_tput)
{
	struct wlan_dp_resource_vote_node *max_vote_node;
	int i;

	for (i = 0; i < MAX_MAC_RESOURCES; i++) {
		max_vote_node = rsrc_ctx->max_phymode_nodes[i];
		if ((max_vote_node == vote_node) &&
		    (prev_tput > vote_node->tput))
			return true;
	}

	return false;
}

/**
 * wlan_dp_resource_mgr_attach() - Attach DP resource manager
 * @dp_ctx: pointer to the DP context
 *
 * Return: None
 */
void wlan_dp_resource_mgr_attach(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_resource_mgr_detach: Detach DP resource manager
 * @dp_ctx: DP context pointer
 *
 * Return: None
 */
void wlan_dp_resource_mgr_detach(struct wlan_dp_psoc_context *dp_ctx);

/**
 * wlan_dp_resource_mgr_upscale_resources: Upscale DP resources in refill
 * thread context
 * @refill_thread: Refill thread context
 *
 * Return: None
 */
void
wlan_dp_resource_mgr_upscale_resources(
				struct dp_rx_refill_thread *refill_thread);

/**
 * wlan_dp_resource_mgr_downscale_resources: Downscale DP resources in refill
 * thread context
 *
 * Return: None
 */
void wlan_dp_resource_mgr_downscale_resources(void);

/**
 * wlan_dp_resource_mgr_vote_node_free: Free DP resource vote node
 * @peer: Peer reference
 *
 * Return: None
 */
void
wlan_dp_resource_mgr_vote_node_free(struct wlan_objmgr_peer *peer);

/**
 * wlan_dp_resource_mgr_notify_ndp_channel_info: Notify NDP channel info
 * to DP resource manager
 * @rsrc_ctx: DP resource context
 * @peer: peer reference for channel info update
 * @ch_info: NDP channel info
 * @num_channels: number of channels NDP peer operate
 *
 * Return: None
 */
void
wlan_dp_resource_mgr_notify_ndp_channel_info(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_peer *peer,
				struct nan_datapath_channel_info *ch_info,
				uint32_t num_channels);

/**
 * wlan_dp_resource_mgr_notify_vdev_mac_id_migration: Notify vdev mac-id migration
 * to DP resource manager
 * @rsrc_ctx: DP resource context
 * @vdev: objmgr vdev reference
 * @old_mac_id: Old mac id vdev present
 * @new_mac_id: New mac id vdev moved
 *
 * Return: None
 */
void wlan_dp_resource_mgr_notify_vdev_mac_id_migration(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_vdev *vdev,
				uint32_t old_mac_id,
				uint32_t new_mac_id);

/**
 * wlan_dp_resource_mgr_set_req_resources: Set Required resources for
 * current driver operations
 * @rsrc_ctx: DP resource context reference
 *
 * Return: None
 */
void
wlan_dp_resource_mgr_set_req_resources(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx);

/**
 * wlan_dp_resource_mgr_notify_vdev_creation: notify vdev creation to
 * DP resource manager
 * @rsrc_ctx: DP resource context reference
 * @vdev: vdev objmgr reference
 *
 * Return: None
 */
void
wlan_dp_resource_mgr_notify_vdev_creation(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_vdev *vdev);

/**
 * wlan_dp_resource_mgr_notify_vdev_deletion: notify vdev deletion to
 * DP resource manager
 * @rsrc_ctx: DP resource context reference
 * @vdev: vdev objmgr reference
 *
 * Return: None
 */
void
wlan_dp_resource_mgr_notify_vdev_deletion(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_vdev *vdev);
#else

static inline
void wlan_dp_resource_mgr_attach(struct wlan_dp_psoc_context *dp_ctx)
{
}

static inline
void wlan_dp_resource_mgr_detach(struct wlan_dp_psoc_context *dp_ctx)
{
}

static inline void
wlan_dp_resource_mgr_vote_node_free(struct wlan_objmgr_peer *peer)
{
}

static inline void
wlan_dp_resource_mgr_upscale_resources(
				struct dp_rx_refill_thread *refill_thread)
{
}

static inline void
wlan_dp_resource_mgr_downscale_resources(void)
{
}

static inline void
wlan_dp_resource_mgr_set_req_resources(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx)
{
}

static inline void
wlan_dp_resource_mgr_notify_vdev_creation(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_vdev *vdev)
{
}

static inline void
wlan_dp_resource_mgr_notify_vdev_deletion(
				struct wlan_dp_resource_mgr_ctx *rsrc_ctx,
				struct wlan_objmgr_vdev *vdev)
{
}
#endif /*WLAN_DP_DYNAMIC_RESOURCE_MGMT*/

#endif /*_WLAN_DP_RESOURCE_MGR_H_*/
