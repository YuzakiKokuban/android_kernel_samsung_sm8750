/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __SDE_CESTA_H__
#define __SDE_CESTA_H__

#include <soc/qcom/crm.h>

#include "sde_power_handle.h"

#define MAX_SCC_BLOCK		6
#define MAX_CESTA_COUNT		1
#define SDE_CESTA_INDEX		0

#define MAX_CESTA_CLIENT_NAME_LEN	32

/**
 * SCC override ctrl flags
 * SDE_CESTA_OVERRIDE_FORCE_DB_UPDATE: resets the state machines within SCC
 * SDE_CESTA_OVERRIDE_FORCE_IDLE: forces HW to signal chn_update
 * SDE_CESTA_OVERRIDE_FORCE_ACTIVE: forces HW to trigger ACTIVE vote and hold the state
 * SDE_CESTA_OVERRIDE_FORCE_CHN_UPDATE: forces HW to trigger IDLE vote and hold the state
 */
#define	SDE_CESTA_OVERRIDE_FORCE_DB_UPDATE	BIT(0)
#define	SDE_CESTA_OVERRIDE_FORCE_IDLE		BIT(1)
#define	SDE_CESTA_OVERRIDE_FORCE_ACTIVE		BIT(2)
#define	SDE_CESTA_OVERRIDE_FORCE_CHN_UPDATE	BIT(3)

struct sde_cesta;

/**
 * sde_cesta_vote_state - voting states for clock & bandwidth
 * SDE_CESTA_BW_CLK_NOCHANGE: no change in clk & bw vote
 * SDE_CESTA_BW_CLK_UPVOTE: both clk & bw increase
 * SDE_CESTA_BW_CLK_DOWNVOTE: both clk & bw decrease
 * SDE_CESTA_BW_UPVOTE_CLK_DOWNVOTE: clk decrease & bw increase
 * SDE_CESTA_CLK_UPVOTE_BW_DOWNVOTE: clk increase & bw decrease
 */
enum sde_cesta_vote_state {
	SDE_CESTA_BW_CLK_NOCHANGE,
	SDE_CESTA_BW_CLK_UPVOTE,
	SDE_CESTA_BW_CLK_DOWNVOTE,
	SDE_CESTA_BW_UPVOTE_CLK_DOWNVOTE,
	SDE_CESTA_CLK_UPVOTE_BW_DOWNVOTE,
};

/**
 * sde_cesta_client_data - sde cesta resource voting values
 * @bw_ab: CRMB_PT bandwidth AB in bytes
 * @bw_ib: CRMB_PT bandwidth IB in bytes
 * @core_clk_rate_ab: CRMB core clk AB in Hz frequency
 * @core_clk_rate_ib: CRMB core clk IB in Hz frequency
 */
struct sde_cesta_client_data {
	u64 bw_ab;
	u64 bw_ib;
	u64 core_clk_rate_ab;
	u64 core_clk_rate_ib;
};

/**
 * sde_cesta_client - sde cesta client information
 * @client_index: client index
 * @cesta_index: sde cesta instance
 * @scc_index: sde cesta control index
 * @name: client name
 * @enabled: client cesta status
 * @pwr_st_override: cesta override request
 * @vote_state: cesta resources voting status
 * @hw: cesta resources voting data
 * @aoss_cp_level: current AOSS CP level
 * @list: client list ponter
 */
struct sde_cesta_client {
	u32 client_index;
	u32 cesta_index;
	u32 scc_index;
	char name[MAX_CESTA_CLIENT_NAME_LEN];
	bool enabled;
	bool pwr_st_override;
	enum sde_cesta_vote_state vote_state;
	struct sde_cesta_client_data hw;
	u32 aoss_cp_level;
	struct list_head list;
};

/**
 * sde_cesta_perf_cfg - storage struct to save all the perf params at bootup
 * @min_bw_kbps: minimum bandwidth in kbps
 * @max_bw_kpbs: maximum bandwidth in kbps
 * @num_ddr_channels: number of DDR channels
 * @drm_efficiency: DRAM efficiency value, based on which the bandwidth IB value is calculated
 * @max_core_clk_rate: maximum core clock rate
 */
struct sde_cesta_perf_cfg {
	u64 min_bw_kbps;
	u64 max_bw_kbps;
	u32 num_ddr_channels;
	u32 dram_efficiency;
	u64 max_core_clk_rate;
};

/**
 * sde_cesta_ctrl_pwr_req_mode - power request mode to SCC
 * SDE_CESTA_CTRL_REQ_IMMEDIATE: apply the vote immediately
 * SDE_CESTA_CTRL_REQ_PANIC_REGION: apply the vote during the panic region as set by INTF
 * SDE_CESTA_CTRL_REQ_ON_PANIC: apply the vote on panic signal from INTF
 */
enum sde_cesta_ctrl_pwr_req_mode {
	SDE_CESTA_CTRL_REQ_IMMEDIATE,
	SDE_CESTA_CTRL_REQ_PANIC_REGION,
	SDE_CESTA_CTRL_REQ_ON_PANIC,
};

/**
 * sde_cesta_ctrl_cfg - SCC configuration
 * @enable: boolean to indicate enable/disable scc
 * @intf: interface mux
 * @wb: indicate if the interface is of type Writeback
 * @dual_dsi: indicate if the interface is of type dual-dsi
 * @avr_enable: avr freature is enabled
 * @auto_active_on_panic: set auto active signal on panic signal from INTF
 * @hw_sleep_enable: enable hardware sleep during idle
 * req_mode: mode based on which scc initiates the voting
 */
struct sde_cesta_ctrl_cfg {
	bool enable;
	u32 intf;
	bool wb;
	bool dual_dsi;
	bool avr_enable;
	bool auto_active_on_panic;
	bool hw_sleep_enable;
	enum sde_cesta_ctrl_pwr_req_mode req_mode;
};

/**
 * sde_cesta_scc_status - SCC status
 * @frame_region: indicates the current frame-region
 * @sch_handshake: handshake status between scc & cesta
 * @fsm_state: active/idle vote state
 * @flush_missed_counter: missed flush counters
 *
 */
struct sde_cesta_scc_status {
	u32 frame_region;
	u32 sch_handshake;
	u32 fsm_state;
	u32 flush_missed_counter;
};

enum sde_cesta_sw_client_update_flag {
	SDE_CESTA_SW_CLIENT_CLK_UPDATE = BIT(0),
	SDE_CESTA_SW_CLIENT_BW_UPDATE = BIT(1),
	SDE_CESTA_SW_CLIENT_AOSS_UPDATE = BIT(2),
};

/**
 * sde_cesta_sw_client_data - Software Client data; currntly supports only sw-client-0
 * @data: cesta resources voting data for clk & bw
 * @aoss_cp_level: current AOSS CP level
 */
struct sde_cesta_sw_client_data {
	struct sde_cesta_client_data data;
	u32 aoss_cp_level;
};

/**
 * sde_cesta_hw_ops - ops for sde cesta
 * @init: initialize sde cesta
 * @ctrl_setup: configure SCC
 * @poll_handshake: poll for handshake to occur
 * @get_status: get current sde cesta status
 * @get_pwr_event: get all the power states which can used for debugging
 * @override_ctrl_setup: configure the SCC override ctrl
 * @reset_ctrl: reset SCC ctrl
 * @force_db_update: change SCC ctrl setting and force db-update
 * @get_rscc_pwr_ctrl_status: get sde rscc power control status
 */
struct sde_cesta_hw_ops {
	void (*init)(struct sde_cesta *cesta);
	void (*ctrl_setup)(struct sde_cesta *cesta, u32 idx, struct sde_cesta_ctrl_cfg *cfg);
	int (*poll_handshake)(struct sde_cesta *cesta, u32 idx);
	void (*get_status)(struct sde_cesta *cesta, u32 idx, struct sde_cesta_scc_status *status);
	u32 (*get_pwr_event)(struct sde_cesta *cesta);
	void (*override_ctrl_setup)(struct sde_cesta *cesta, u32 idx, u32 force_flags);
	void (*reset_ctrl)(struct sde_cesta *cesta, u32 idx, bool en);
	void (*force_db_update)(struct sde_cesta *cesta, u32 idx, bool en_auto_active,
			enum sde_cesta_ctrl_pwr_req_mode req_mode, bool en_hw_sleep,
			bool en_clk_gate);
	u32 (*get_rscc_pwr_ctrl_status)(struct sde_cesta *cesta);
};

/**
 * sde_cesta - sde cesta base struct holding all cesta resources for a single instance
 * @dev: cesta device
 * @phandle: power handle object to control the resources
 * @fs: MDSS GDSC regulator handle
 * @pd_fs: MDSS GDSC power domain handle
 * @scc_io: scc instances io data mapping
 * @scc_index: stores the SCC index
 * @scc_count: number of SCC instances
 * @rscc_io: sde rscc io data mapping
 * @wrapper_io: wrapper io data mapping
 * @client_list: link list maintaing all the clients
 * @hw_ops: sde ceseta hardware operations
 * @sw_fs_enabled: track MDSS GDSC sw vote during probe
 * @bus_hdl: context structure for data bus control for each cesta HW client active path
 * @bus_hdl_idle: context structure for data bus control for each cesta HW client idle-path
 * @sw_client_bus_hdl: context structure for data bus control for cesta SW client-0
 * @sw_client: object to store the sw-client vote data
 * @crm_dev: CRM device pointer, used to communicate with CRM driver
 * @perf_cfg: object to store all the performance params set by sde_kms during bootup
 * @debug_mode: enables the logging for each register read/write
 * @debugfs_root: pointer to cesta debugfs root
 */
struct sde_cesta {
	struct device *dev;
	struct sde_power_handle phandle;
	struct regulator *fs;
	struct device *pd_fs;

	struct dss_io_data scc_io[MAX_SCC_BLOCK];
	u32 scc_index[MAX_SCC_BLOCK];
	u32 scc_count;

	struct dss_io_data rscc_io;
	struct dss_io_data wrapper_io;
	struct dss_io_data sde_crm_io;

	struct list_head client_list;
	struct mutex client_lock;

	struct sde_cesta_hw_ops hw_ops;

	bool sw_fs_enabled;

	struct icc_path *bus_hdl[MAX_SCC_BLOCK];
	struct icc_path *bus_hdl_idle[MAX_SCC_BLOCK];
	struct icc_path *sw_client_bus_hdl;
	struct sde_cesta_sw_client_data sw_client;
	const struct device *crm_dev;

	struct sde_cesta_perf_cfg perf_cfg;
	u32 debug_mode;
	struct dentry *debugfs_root;
};

/**
 * sde_cesta_params - struct used to pass the configs required for cesta client voting
 * @enable: enable/disable cesta
 * @data: contains the values for voting resources - clock & bandwidth
 * @pwr_st_override: request to place and override vote to take effect immediately
 * @post_commit: indicating if the request is coming at the beginning or end of the commit
 * @max_vote: flag indicating request to place max voting
 */
struct sde_cesta_params {
	bool enable;
	struct sde_cesta_client_data data;
	bool pwr_st_override;
	bool post_commit;
	bool max_vote;
};

/**
 * sde_cesta_aoss_cp_level - various CP levels supported by AOSS VCD
 */
enum sde_cesta_aoss_cp_level {
	SDE_CESTA_AOSS_CP_LEVEL_0,
	SDE_CESTA_AOSS_CP_LEVEL_1,
	SDE_CESTA_AOSS_CP_LEVEL_2,
	SDE_CESTA_AOSS_CP_LEVEL_3,
	SDE_CESTA_AOSS_CP_LEVEL_4,
	SDE_CESTA_AOSS_CP_LEVEL_MAX,
};

#if IS_ENABLED(CONFIG_DRM_SDE_CESTA)

/**
 * sde_cesta_is_enabled - checks if sde cesta is enbaled
 * @cesta_index: cesta instance being checked
 */
bool sde_cesta_is_enabled(u32 cesta_index);

/**
 * sde_cesta_hw_init - initialize sde cesta hardware
 * @cesta_index: cesta instance used
 */
void sde_cesta_hw_init(struct sde_cesta *cesta);

/**
 * sde_cesta_create_client - create a cesta client. Each client is tied to a SCC
 * @cesta_index: cesta instance used
 * @client_name: cesta client name
 */
struct sde_cesta_client *sde_cesta_create_client(u32 cesta_index, char *client_name);

/**
 * sde_cesta_resource_enable - enable the resources controlled by cesta
 * @cesta_index: cesta instance used
 */
int sde_cesta_resource_enable(u32 cesta_index);

/**
 * sde_cesta_resource_disable - disable the resources controlled by cesta
 * @cesta_index: cesta instance used
 */
int sde_cesta_resource_disable(u32 cesta_index);

/**
 * sde_cesta_update_perf_config - cache all the perforamnce related configs during bootup
 * @cesta_index: cesta instance used
 * @cfg: contains all the performance configurations
 */
void sde_cesta_update_perf_config(u32 cesta_index, struct sde_cesta_perf_cfg *cfg);

/**
 * sde_cesta_clk_bw_check - used to check the incoming frame's clock & bandwidth against the
 *                          currently active cesta clients
 * @client: pointer to sde cesta client
 * @params: configs related to clock/bandwidth and voting system
 */
int sde_cesta_clk_bw_check(struct sde_cesta_client *client, struct sde_cesta_params *params);

/**
 * sde_cesta_clk_bw_update - used to vote for the requested clock & bandwidth for the client
 * @client: pointer to sde cesta client
 * @params: configs related to clock/bandwidth and voting system
 */
void sde_cesta_clk_bw_update(struct sde_cesta_client *client, struct sde_cesta_params *params);

/**
 * sde_cesta_aoss_update - used to update the AOSS VCD
 * @client: pointer to sde cesta client
 * @cp_level: power level requested
 */
int sde_cesta_aoss_update(struct sde_cesta_client *client, enum sde_cesta_aoss_cp_level cp_level);

/**
 * sde_cesta_ctrl_setup - configure the sde cesta control SCC
 * @client: pointer to sde cesta client
 * @cfg: SCC configs indicating the type of INTF, mux, etc
 */
void sde_cesta_ctrl_setup(struct sde_cesta_client *client, struct sde_cesta_ctrl_cfg *cfg);

/**
 * sde_cesta_poll_handshake - poll for SCC to switch to process vote request
 * @client: pointer to sde cesta client
 */
void sde_cesta_poll_handshake(struct sde_cesta_client *client);

/**
 * sde_cesta_get_status - get current status of SCC for a client
 * @client: pointer to sde cesta client
 * @status: pointer containing the SCC client status information
 */
void sde_cesta_get_status(struct sde_cesta_client *client, struct sde_cesta_scc_status *status);

/**
 * sde_cesta_get_phandle - retrieves the power handle context for the requested cesta instance
 * @cesta_index: cesta instance used
 */
struct sde_power_handle *sde_cesta_get_phandle(u32 cesta_index);

/**
 * sde_cesta_splash_release - release the cesta related resources used for cont-splash
 * @cesta_index: cesta instance used
 */
void sde_cesta_splash_release(u32 cesta_index);

/**
 * sde_cesta_sw_client_update - updates clk/bw/aoss for cesta software client 0 based on the flag
 * @cesta_index: cesta instance used
 * @data: resource voting data
 * @flag: flag to indicate which resources to update
 */
int sde_cesta_sw_client_update(u32 cesta_index, struct sde_cesta_sw_client_data *data,
			enum sde_cesta_sw_client_update_flag flag);

/**
 * sde_cesta_get_core_clk_rate - get the consolidated core-clk rate
 * @cesta_index: cesta instance used
 */
u64 sde_cesta_get_core_clk_rate(u32 cesta_index);

/**
 * sde_cesta_override_ctrl - configure the scc override ctrl for the client
 * @client: pointer to sde cesta client
 */
void sde_cesta_override_ctrl(struct sde_cesta_client *client, u32 force_flags);

/**
 * sde_cesta_reset_ctrl - reset SCC ctrl
 * @client: pointer to sde cesta client
 * @en: flag to reset/unset the bits
 */
void sde_cesta_reset_ctrl(struct sde_cesta_client *client, bool en);

/**
 * sde_cesta_force_db_update - update SCC settings and do a force-db update
 * @client: pointer to sde cesta client
 * @en_auto_active: boolean to enable/disable auto_active
 * @req_mode: power req mode
 * @en_hw_sleep: boolean to enable/disable hw_sleep
 * @en_clk_gate: boolean to enable/disable clk_gate
 */
void sde_cesta_force_db_update(struct sde_cesta_client *client, bool en_auto_active,
		enum sde_cesta_ctrl_pwr_req_mode req_mode, bool en_hw_sleep, bool en_clk_gate);

#else
static inline bool sde_cesta_is_enabled(u32 cesta_index)
{
	return false;
}

static inline void sde_cesta_hw_init(struct sde_cesta *cesta)
{
}

static inline struct sde_cesta_client *sde_cesta_create_client(u32 cesta_index, char *client_name)
{
	return NULL;
}

static inline int sde_cesta_resource_enable(u32 cesta_index)
{
	return 0;
}

static inline int sde_cesta_resource_disable(u32 cesta_index)
{
	return 0;
}

static inline void sde_cesta_update_perf_config(u32 cesta_index, struct sde_cesta_perf_cfg *cfg)
{
}

static inline int sde_cesta_clk_bw_check(struct sde_cesta_client *client,
		struct sde_cesta_params *params)
{
	return 0;
}

static inline void sde_cesta_clk_bw_update(struct sde_cesta_client *client,
		struct sde_cesta_params *params)
{
}

static inline int sde_cesta_aoss_update(struct sde_cesta_client *client,
		enum sde_cesta_aoss_cp_level cp_level)
{
	return 0;
}

static inline void sde_cesta_ctrl_setup(struct sde_cesta_client *client,
		struct sde_cesta_ctrl_cfg *cfg)
{
}

static inline void sde_cesta_poll_handshake(struct sde_cesta_client *client)
{
}

static inline void sde_cesta_get_status(struct sde_cesta_client *client,
		struct sde_cesta_scc_status *status)
{
}

static inline struct sde_power_handle *sde_cesta_get_phandle(u32 cesta_index)
{
	return NULL;
}

static inline void sde_cesta_splash_release(u32 cesta_index)
{
}

static inline int sde_cesta_sw_client_update(u32 cesta_index,
		struct sde_cesta_sw_client_data *data, enum sde_cesta_sw_client_update_flag flag)
{
	return 0;
}

static inline u64 sde_cesta_get_core_clk_rate(u32 cesta_index)
{
	return 0;
}

static inline void sde_cesta_override_ctrl(struct sde_cesta_client *client, u32 force_flags)
{
}

static inline void sde_cesta_reset_ctrl(struct sde_cesta_client *client, bool en)
{
}

static inline void sde_cesta_force_db_update(struct sde_cesta_client *client,
		bool en_auto_active, enum sde_cesta_ctrl_pwr_req_mode req_mode, bool en_hw_sleep,
		bool en_clk_gate)
{
}
#endif /* CONFIG_DRM_SDE_CESTA */

#endif /* __SDE_CESTA_H__ */
