// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt)	"[sde_cesta:%s:%d]: " fmt, __func__, __LINE__

#include <linux/clk/qcom.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <soc/qcom/crm.h>

#include "sde_cesta.h"
#include "msm_drv.h"
#include "sde_trace.h"
#include "sde_dbg.h"

#define SDE_ERROR_CESTA(fmt, ...) pr_err("[sde error cesta]" fmt, ##__VA_ARGS__)
#define SDE_DEBUG_CESTA(fmt, ...) pr_debug("[sde debug cesta]" fmt, ##__VA_ARGS__)

#define DATA_BUS_HW_CLIENT_NAME "qcom,sde-data-bus-hw"
#define DATA_BUS_SW_CLIENT_0_NAME "qcom,sde-data-bus-sw-0"

#define MDP_CLK_LOWSVS_D1	156000000

#define SDE_CRM_BASE	0xAF21000
#define SDE_CRM_LEN 	0xA000

static struct sde_cesta *cesta_list[MAX_CESTA_COUNT] = {NULL, };

bool sde_cesta_is_enabled(u32 cesta_index)
{
	if (cesta_index >= MAX_CESTA_COUNT)
		return false;

	return cesta_list[cesta_index] ? true : false;
}

struct sde_power_handle *sde_cesta_get_phandle(u32 cesta_index)
{
	struct sde_cesta *cesta;

	if ((cesta_index >= MAX_CESTA_COUNT) || !sde_cesta_is_enabled(cesta_index))
		return NULL;

	cesta = cesta_list[cesta_index];

	return &cesta->phandle;
}

void sde_cesta_update_perf_config(u32 cesta_index, struct sde_cesta_perf_cfg *cfg)
{
	struct sde_cesta *cesta;

	if (!sde_cesta_is_enabled(cesta_index))
		return;

	if (!cfg) {
		SDE_ERROR_CESTA("invalid config\n");
		return;
	}
	cesta = cesta_list[cesta_index];

	memcpy(&cesta->perf_cfg, cfg, sizeof(struct sde_cesta_perf_cfg));
}

static struct clk *_sde_cesta_get_core_clk(struct sde_cesta *cesta)
{
	struct sde_power_handle *phandle;
	struct dss_module_power *mp;
	struct clk *core_clk = NULL;
	int i;

	phandle = &cesta->phandle;
	mp = &phandle->mp;

	for (i = 0; i < mp->num_clk; i++) {
		if (!strcmp(mp->clk_config[i].clk_name, "core_clk")) {
			core_clk = mp->clk_config[i].clk;
			break;
		}
	}

	return core_clk;
}

static void _sde_cesta_log_status(struct sde_cesta *cesta)
{
	struct sde_cesta_client *client;
	struct sde_cesta_scc_status status = {0, };
	u32 pwr_event, pwr_ctrl_status;

	if (!cesta->hw_ops.get_status || !cesta->hw_ops.get_pwr_event
			|| !cesta->hw_ops.get_rscc_pwr_ctrl_status)
		return;

	mutex_lock(&cesta->client_lock);
	list_for_each_entry(client, &cesta->client_list, list) {
		cesta->hw_ops.get_status(cesta, client->client_index, &status);

		SDE_DEBUG_CESTA("SCC[%d] status- frame_region:%d scc_hs:%d fsm_st:%d miss_cnt:%d\n",
				client->client_index, status.frame_region, status.sch_handshake,
				status.fsm_state, status.flush_missed_counter);
		SDE_EVT32(client->client_index, status.frame_region, status.sch_handshake,
				status.fsm_state, status.flush_missed_counter);
	}
	mutex_unlock(&cesta->client_lock);

	pwr_event = cesta->hw_ops.get_pwr_event(cesta);
	pwr_ctrl_status = cesta->hw_ops.get_rscc_pwr_ctrl_status(cesta);

	SDE_DEBUG_CESTA("pwr_event:0x%x, pwr_ctrl_status:0x%x\n", pwr_event, pwr_ctrl_status);
	SDE_EVT32(pwr_event, pwr_ctrl_status);
}

static int  _sde_cesta_check_mode2_entry_status(u32 cesta_index)
{
	struct sde_cesta *cesta;
	int pwr_ctrl_status, trial = 0, max_trials = 10;

	cesta = cesta_list[cesta_index];

	if (!cesta->hw_ops.get_rscc_pwr_ctrl_status)
		return 0;

	while (trial < max_trials) {
		pwr_ctrl_status = cesta->hw_ops.get_rscc_pwr_ctrl_status(cesta);
		if (pwr_ctrl_status & BIT(12)) /* GDSC power-collapse success */
			break;
		udelay(50);
		trial++;
	}

	if (trial == max_trials) {
		SDE_ERROR_CESTA("cesata:%d mode2 entry failed, trial:%d, status:0x%x\n",
				cesta_index, trial, pwr_ctrl_status);
		SDE_EVT32(cesta_index, trial, pwr_ctrl_status, SDE_EVTLOG_ERROR);
		_sde_cesta_log_status(cesta);
		return -EBUSY;
	}

	SDE_EVT32(cesta_index, trial, pwr_ctrl_status);
	_sde_cesta_log_status(cesta);

	return 0;
}

void sde_cesta_force_db_update(struct sde_cesta_client *client, bool en_auto_active,
		enum sde_cesta_ctrl_pwr_req_mode req_mode, bool en_hw_sleep, bool en_clk_gate)
{
	struct sde_cesta *cesta;

	if (!client || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid param, client:%d, cesta_index:%d\n",
					!!client, client ? client->cesta_index : -1);
		return;
	}

	cesta = cesta_list[client->cesta_index];

	SDE_EVT32(client->client_index, client->scc_index, en_auto_active, req_mode, en_hw_sleep,
			en_clk_gate);
	if (cesta->hw_ops.force_db_update)
		cesta->hw_ops.force_db_update(cesta, client->client_index,
				en_auto_active, req_mode, en_hw_sleep, en_clk_gate);
}

void sde_cesta_reset_ctrl(struct sde_cesta_client *client, bool en)
{
	struct sde_cesta *cesta;

	if (!client || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid param, client:%d, cesta_index:%d\n",
					!!client, client ? client->cesta_index : -1);
		return;
	}

	cesta = cesta_list[client->cesta_index];

	SDE_EVT32(client->client_index, client->scc_index, en);
	if (cesta->hw_ops.reset_ctrl)
		cesta->hw_ops.reset_ctrl(cesta, client->client_index, en);
}

void sde_cesta_override_ctrl(struct sde_cesta_client *client, u32 force_flags)
{
	struct sde_cesta *cesta;

	if (!client || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid param, client:%d, cesta_index:%d\n",
					!!client, client ? client->cesta_index : -1);
		return;
	}

	cesta = cesta_list[client->cesta_index];

	SDE_EVT32(client->client_index, client->scc_index, force_flags);
	if (cesta->hw_ops.override_ctrl_setup)
		cesta->hw_ops.override_ctrl_setup(cesta, client->client_index, force_flags);
}

void sde_cesta_splash_release(u32 cesta_index)
{
	struct sde_cesta *cesta;
	struct clk *core_clk;

	if (!sde_cesta_is_enabled(cesta_index))
		return;

	cesta = cesta_list[cesta_index];

	core_clk = _sde_cesta_get_core_clk(cesta);
	if (!core_clk) {
		SDE_ERROR_CESTA("core_clk not found\n");
		return;
	}

	clk_set_rate(core_clk, 19200000);

	SDE_EVT32(cesta_index);
}

void sde_cesta_ctrl_setup(struct sde_cesta_client *client, struct sde_cesta_ctrl_cfg *cfg)
{
	struct sde_cesta *cesta;

	if (!client || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid param, client:%d, cesta_index:%d\n",
					!!client, client ? client->cesta_index : -1);
		return;
	}

	cesta = cesta_list[client->cesta_index];

	if (cesta->hw_ops.ctrl_setup)
		cesta->hw_ops.ctrl_setup(cesta, client->client_index, cfg);

	if (cfg)
		SDE_EVT32(client->client_index, client->scc_index, cfg->enable, cfg->intf, cfg->wb,
			cfg->dual_dsi, cfg->avr_enable, cfg->auto_active_on_panic, cfg->req_mode,
			cfg->hw_sleep_enable);
	else
		SDE_EVT32(client->client_index, client->scc_index);
}

static bool _sde_cesta_is_update_required(struct sde_cesta_client *client,
		struct sde_cesta_params *params)
{
	struct sde_cesta_client_data *client_data = &client->hw;

	if ((!client->enabled && params->enable) || (client->enabled && !params->enable))
		return true;
	else if ((params->data.core_clk_rate_ab != client_data->core_clk_rate_ab)
			|| (params->data.core_clk_rate_ib != client_data->core_clk_rate_ib))
		return true;
	else if (params->data.bw_ab != client_data->bw_ab)
		return true;

	return false;
}

int sde_cesta_clk_bw_check(struct sde_cesta_client *client, struct sde_cesta_params *params)
{
	struct sde_cesta *cesta;
	struct sde_cesta_client *c;
	struct sde_cesta_client_data *client_data;
	struct dss_module_power *mp;
	u64 bw_ab, bw_ib = 0, core_clk_ab, core_clk_ib, core_clk_rate;
	int i, ret = 0;

	if (!client || !params || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_DEBUG_CESTA("invalid values - client:%d, param:%d, cesta_index:%d\n",
					!!client, !!params, client ? client->cesta_index : -1);
		return 0;
	}
	cesta = cesta_list[client->cesta_index];

	mutex_lock(&cesta->client_lock);

	if (!_sde_cesta_is_update_required(client, params))
		goto end;

	bw_ab = params->data.bw_ab;
	core_clk_ab = params->data.core_clk_rate_ab;
	core_clk_ib = params->data.core_clk_rate_ib;

	list_for_each_entry(c, &cesta->client_list, list) {
		/* skip for disabled paths & current ctl-path as its already accounted */
		if (!c->enabled || (c == client))
			continue;

		client_data = &c->hw;

		bw_ab += client_data->bw_ab;

		core_clk_ab += client_data->core_clk_rate_ab;
		core_clk_ib = max(core_clk_ib, client_data->core_clk_rate_ib);
	}

	/* IB vote calculation */
	if (cesta->perf_cfg.num_ddr_channels && cesta->perf_cfg.dram_efficiency)
		bw_ib = div_u64(div_u64(bw_ab, cesta->perf_cfg.num_ddr_channels) * 100,
				cesta->perf_cfg.dram_efficiency);

	/* BW validation */
	if (max(bw_ab, bw_ib) > (cesta->perf_cfg.max_bw_kbps * 1000ULL)) {
		SDE_ERROR_CESTA(
		  "client:%d/%d, BW exceeded - ab:%llu, ib:%llu, c_ab:%llu, max_kbps:%llu\n",
				client->client_index, client->scc_index, bw_ab, bw_ib,
				params->data.bw_ab, cesta->perf_cfg.max_bw_kbps);
		ret = -E2BIG;
		goto end;
	}

	/* Clk validation */
	core_clk_rate = max(core_clk_ab, core_clk_ib);
	if (core_clk_rate > cesta->perf_cfg.max_core_clk_rate) {
		SDE_ERROR_CESTA(
		  "client:%d/%d, Clk exceeded - ab:%llu, ib:%llu, c_ab:%llu, c_ib:%llu, max:%llu\n",
				client->client_index, client->scc_index, core_clk_ab, core_clk_ib,
				params->data.core_clk_rate_ab, params->data.core_clk_rate_ib,
				cesta->perf_cfg.max_core_clk_rate);
		ret = -E2BIG;
		goto end;
	}

	/* reserve core_clk in MMRM based on the consolidated clk value */
	mp = &cesta->phandle.mp;
	for (i = 0; i < mp->num_clk; i++) {
		if (mp->clk_config[i].type != DSS_CLK_MMRM)
			continue;

		core_clk_rate = clk_round_rate(mp->clk_config[i].clk, core_clk_rate);
		ret = sde_power_clk_set_rate(&cesta->phandle, mp->clk_config[i].clk_name,
				core_clk_rate, MMRM_CLIENT_DATA_FLAG_RESERVE_ONLY);
		if (ret) {
			SDE_ERROR_CESTA("client:%d/%d, cannot reserve core clk rate:%llu, ret:%d\n",
					client->client_index, client->scc_index,
					core_clk_rate, ret);
			ret = -E2BIG;
			goto end;
		}
	}

end:
	if (ret) {
		list_for_each_entry(c, &cesta->client_list, list) {
			client_data = &c->hw;
			SDE_EVT32(c->client_index, c->scc_index, c->enabled, c->pwr_st_override,
				client_data->core_clk_rate_ab, client_data->core_clk_rate_ib,
				client_data->bw_ab, client_data->bw_ib, SDE_EVTLOG_ERROR);

			SDE_ERROR_CESTA(
				"client:%d/%d, en:%d, ab:%llu, ib:%llu, c_ab:%llu, c_ib:%llu\n",
				       c->client_index, c->scc_index, c->enabled,
				       client_data->bw_ab, client_data->bw_ib,
				       client_data->core_clk_rate_ab,
				       client_data->core_clk_rate_ib);
		}
	}
	mutex_unlock(&cesta->client_lock);

	return ret;
}

static void _sde_cesta_clk_bw_vote(struct sde_cesta_client *client, bool pwr_st_override,
		u64 clk_ab, u64 clk_ib, u64 bw_ab, u64 bw_ib)
{
	struct sde_cesta *cesta = cesta_list[client->cesta_index];
	struct dss_module_power *mp;
	struct clk *core_clk = NULL;
	u64 idle_bw_ab = 0, idle_bw_ib = 0, idle_clk_ab = 0, idle_clk_ib = 0;
	int i, ret;

	mp = &cesta->phandle.mp;
	for (i = 0; i < mp->num_clk; i++) {
		if (!strcmp(mp->clk_config[i].clk_name, "core_clk")) {
			core_clk = mp->clk_config[i].clk;
			break;
		}
	}

	if (!core_clk || !cesta->bus_hdl[client->scc_index]) {
		SDE_ERROR_CESTA("core clk / bus handle not found - client:%d, scc:%d\n",
				client->client_index, client->scc_index);
		return;
	}

	/* avoid clk_round_rate during disable as it rounds to 19.2 Mhz */
	if (clk_ab || clk_ib) {
		clk_ab =  clk_round_rate(core_clk, clk_ab);
		clk_ib =  clk_round_rate(core_clk, clk_ib);
	}

	/*
	 * When hw-client vote has override flag set, both acive/idle vote for BW should be the same
	 * Clock vote remains the same for both active/idle as clk gating will be done during idle
	 */
	if (pwr_st_override) {
		idle_bw_ab = bw_ab;
		idle_bw_ib = bw_ib;

		idle_clk_ab = clk_ab;
		idle_clk_ib = clk_ib;
	} else {
		idle_clk_ab = 0;
		idle_clk_ib = MDP_CLK_LOWSVS_D1;
	}

	/* mdp-clk voting */
	ret = qcom_clk_crmb_set_rate(core_clk, CRM_HW_DRV, client->scc_index,
			0, CRM_PWR_STATE0, idle_clk_ab, idle_clk_ib);
	if (ret)
		SDE_ERROR_CESTA("clk active vote failed - ret:%d\n", ret);

	ret = qcom_clk_crmb_set_rate(core_clk, CRM_HW_DRV, client->scc_index,
			0, CRM_PWR_STATE1, clk_ab, clk_ib);
	if (ret)
		SDE_ERROR_CESTA("clk active vote failed - ret:%d\n", ret);

	/* bw voting */
	icc_set_tag(cesta->bus_hdl_idle[client->scc_index], QCOM_ICC_TAG_PWR_ST_0);
	ret = icc_set_bw(cesta->bus_hdl_idle[client->scc_index],
			Bps_to_icc(idle_bw_ab), Bps_to_icc(idle_bw_ib));
	if (ret)
		SDE_ERROR_CESTA("bw idle vote failed - ret:%d\n", ret);

	icc_set_tag(cesta->bus_hdl[client->scc_index], QCOM_ICC_TAG_PWR_ST_1);
	ret = icc_set_bw(cesta->bus_hdl[client->scc_index], Bps_to_icc(bw_ab), Bps_to_icc(bw_ib));
	if (ret)
		SDE_ERROR_CESTA("bw active vote failed - ret:%d\n", ret);

	/* pwr_state update for channel switch */
	ret = crm_write_pwr_states(cesta->crm_dev, client->scc_index);
	if (ret)
		SDE_ERROR_CESTA("crm_write_pwr_states failed - ret:%d\n", ret);

	SDE_EVT32(client->scc_index, clk_ab, clk_ib, idle_clk_ab, idle_clk_ib, bw_ab, bw_ib,
			idle_bw_ab, idle_bw_ib);
}

void sde_cesta_clk_bw_update(struct sde_cesta_client *client, struct sde_cesta_params *params)
{
	struct sde_cesta *cesta;
	struct sde_cesta_client *c;
	struct sde_cesta_client_data *client_data, *hw_data;
	u64 bw_ab = 0, bw_ib = 0, params_max, client_max;

	enum vote_state {
		NO_CHANGE,
		UPVOTE,
		DOWNVOTE,
	} clk_vote = NO_CHANGE, bw_vote = NO_CHANGE;

	if (!client || !params || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid values - client:%d, param:%d, cesta_index:%d\n",
					!!client, !!params, client ? client->cesta_index : -1);
		return;
	}
	cesta = cesta_list[client->cesta_index];

	mutex_lock(&cesta->client_lock);

	client_data = &client->hw;

	/*
	 * avoid checks & recalculation for post-commit as it will apply the bw/clk downvote
	 * calcuated and stored in client struct during the pre_commit
	 */
	if (params->post_commit) {
		if ((client->vote_state == SDE_CESTA_BW_UPVOTE_CLK_DOWNVOTE)
				|| (client->vote_state == SDE_CESTA_CLK_UPVOTE_BW_DOWNVOTE))
			goto skip_calc;
		else
			goto end;
	}

	if (!_sde_cesta_is_update_required(client, params)) {
		/* make an explicit upvote after the first override vote */
		if ((client->pwr_st_override && !params->pwr_st_override)
				|| params->pwr_st_override) {
			client->vote_state = SDE_CESTA_BW_CLK_UPVOTE;
			goto skip_calc;
		} else {
			client->vote_state = SDE_CESTA_BW_CLK_NOCHANGE;
			goto end;
		}
	}

	params_max = max(params->data.core_clk_rate_ab, params->data.core_clk_rate_ib);
	client_max = max(client_data->core_clk_rate_ab, client_data->core_clk_rate_ib);
	if (params_max > client_max)
		clk_vote = UPVOTE;
	else if (params_max < client_max)
		clk_vote = DOWNVOTE;

	bw_ab = params->data.bw_ab;
	list_for_each_entry(c, &cesta->client_list, list) {
		/* skip for disabled paths & current ctl-path as its already accounted */
		if (!c->enabled || (c == client))
			continue;

		hw_data = &c->hw;
		bw_ab += hw_data->bw_ab;
	}

	/* IB vote calculation */
	if (!params->max_vote && params->enable
			&& cesta->perf_cfg.num_ddr_channels && cesta->perf_cfg.dram_efficiency) {
		bw_ib = div_u64(div_u64(bw_ab, cesta->perf_cfg.num_ddr_channels) * 100,
				cesta->perf_cfg.dram_efficiency);
		params->data.bw_ib = bw_ib;
	}

	params_max = max(params->data.bw_ab, params->data.bw_ib);
	client_max = max(client_data->bw_ab, client_data->bw_ib);
	if (params_max > client_max)
		bw_vote = UPVOTE;
	else if (params_max < client_max)
		bw_vote = DOWNVOTE;

	if (((client->client_index == 0) || (client->client_index == 1)) &&
			(clk_vote == UPVOTE) && (bw_vote == DOWNVOTE)) {
		params->data.bw_ab = client_data->bw_ab;
		params->data.bw_ib = client_data->bw_ib;
		bw_vote = NO_CHANGE;
	}

	if (bw_vote == clk_vote)
		client->vote_state = (enum sde_cesta_vote_state) bw_vote;
	else if ((bw_vote == UPVOTE) && (clk_vote == DOWNVOTE))
		client->vote_state = SDE_CESTA_BW_UPVOTE_CLK_DOWNVOTE;
	else if ((bw_vote == DOWNVOTE) && (clk_vote == UPVOTE))
		client->vote_state = SDE_CESTA_CLK_UPVOTE_BW_DOWNVOTE;
	else if (!bw_vote || !clk_vote)
		client->vote_state = bw_vote ? (enum sde_cesta_vote_state) bw_vote
						: (enum sde_cesta_vote_state) clk_vote;

skip_calc:
	/* store power state override request from the caller */
	client->pwr_st_override = params->pwr_st_override;
	client->enabled = params->enable;

	if ((client->vote_state == SDE_CESTA_BW_CLK_UPVOTE)
			|| (client->vote_state == SDE_CESTA_BW_CLK_DOWNVOTE)) {
		_sde_cesta_clk_bw_vote(client, client->pwr_st_override,
				params->data.core_clk_rate_ab, params->data.core_clk_rate_ib,
				params->data.bw_ab, params->data.bw_ib);

	} else if (params->post_commit &&
			((client->vote_state == SDE_CESTA_BW_UPVOTE_CLK_DOWNVOTE)
				|| (client->vote_state == SDE_CESTA_CLK_UPVOTE_BW_DOWNVOTE))) {
		client->pwr_st_override = true;
		_sde_cesta_clk_bw_vote(client, client->pwr_st_override,
				client_data->core_clk_rate_ab, client_data->core_clk_rate_ib,
				client_data->bw_ab, client_data->bw_ib);

	} else if (client->vote_state == SDE_CESTA_CLK_UPVOTE_BW_DOWNVOTE) {
		/*
		 * pre-commit: vote for new clk upvote & old BW vote
		 * post-commit: vote for same clk upvote & new BW downvote with override pwr state
		 */
		_sde_cesta_clk_bw_vote(client, client->pwr_st_override,
				params->data.core_clk_rate_ab, params->data.core_clk_rate_ib,
				client_data->bw_ab, client_data->bw_ib);

	} else if (client->vote_state == SDE_CESTA_BW_UPVOTE_CLK_DOWNVOTE) {
		 /*
		  * pre-commit: vote for new BW upvote & old clk vote
		  * post-commit: vote for same BW upvote & new clk downvote with override pwr state
		  */
		_sde_cesta_clk_bw_vote(client, client->pwr_st_override,
				client_data->core_clk_rate_ab, client_data->core_clk_rate_ib,
				params->data.bw_ab, params->data.bw_ib);
	}

	/* update the client vote values */
	memcpy(client_data, &params->data, sizeof(struct sde_cesta_client_data));

	SDE_EVT32(client->client_index, client->scc_index, client->enabled, client->pwr_st_override,
			client->vote_state, client_data->core_clk_rate_ab,
			client_data->core_clk_rate_ib, client_data->bw_ab, client_data->bw_ib,
			params->pwr_st_override, params->post_commit);

end:
	mutex_unlock(&cesta->client_lock);
}

void sde_cesta_poll_handshake(struct sde_cesta_client *client)
{
	struct sde_cesta *cesta;
	int rc;
	ktime_t start, end;

	if (!client || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid param - client:%d, cesta_index:%d\n",
					!!client, client ? client->cesta_index : -1);
		return;
	}
	cesta = cesta_list[client->cesta_index];

	if (!cesta->hw_ops.poll_handshake)
		return;

	start = ktime_get();
	rc = cesta->hw_ops.poll_handshake(cesta, client->scc_index);
	end = ktime_get();

	SDE_EVT32(client->client_index, client->scc_index,
			rc ? SDE_EVTLOG_ERROR : ktime_us_delta(end, start));
}

void sde_cesta_get_status(struct sde_cesta_client *client, struct sde_cesta_scc_status *status)
{
	struct sde_cesta *cesta;

	if (!client || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid param - client:%d, cesta_index:%d\n",
					!!client, client ? client->cesta_index : -1);
		return;
	}
	cesta = cesta_list[client->cesta_index];

	if (cesta->hw_ops.get_status)
		cesta->hw_ops.get_status(cesta, client->scc_index, status);

	SDE_EVT32(client->client_index, client->scc_index, status->frame_region,
			status->sch_handshake, status->fsm_state, status->flush_missed_counter);
}

int sde_cesta_sw_client_update(u32 cesta_index, struct sde_cesta_sw_client_data *data,
			enum sde_cesta_sw_client_update_flag flag)
{
	struct sde_cesta *cesta;
	struct clk *core_clk = NULL;
	struct crm_cmd cmd = {0, };
	int ret = 0;

	if (!sde_cesta_is_enabled(cesta_index))
		return 0;

	cesta = cesta_list[cesta_index];

	SDE_EVT32(cesta_index, flag, data->data.core_clk_rate_ab, data->data.core_clk_rate_ib,
			data->data.bw_ab, data->data.bw_ib, data->aoss_cp_level);

	mutex_lock(&cesta->client_lock);

	if (flag & SDE_CESTA_SW_CLIENT_BW_UPDATE) {
		icc_set_tag(cesta->sw_client_bus_hdl, QCOM_ICC_TAG_AMC);
		ret = icc_set_bw(cesta->sw_client_bus_hdl, Bps_to_icc(data->data.bw_ab),
					Bps_to_icc(data->data.bw_ib));
		if (ret) {
			SDE_ERROR_CESTA("sw-client-0 BW vote failed ab:%llu, ib%llu, ret:%d\n",
				data->data.bw_ab, data->data.bw_ib, ret);
			goto end;
		}
		cesta->sw_client.data.bw_ab = data->data.bw_ab;
		cesta->sw_client.data.bw_ib = data->data.bw_ib;
	}

	if (flag & SDE_CESTA_SW_CLIENT_AOSS_UPDATE) {
		cmd.data = data->aoss_cp_level;
		cmd.resource_idx = 0;
		cmd.wait = true;

		cmd.pwr_state.sw = CRM_ACTIVE_STATE;
		ret = crm_write_perf_ol(cesta->crm_dev, CRM_SW_DRV, 0, &cmd);
		if (ret) {
			SDE_ERROR_CESTA("sw-client-0 AOSS vote failed - cp_level:%d, ret:%d\n",
				data->aoss_cp_level, ret);
			goto end;
		}
		cesta->sw_client.aoss_cp_level = data->aoss_cp_level;
	}

	if (flag & SDE_CESTA_SW_CLIENT_CLK_UPDATE) {
		core_clk = _sde_cesta_get_core_clk(cesta);
		if (!core_clk) {
			SDE_ERROR_CESTA("core_clk not found\n");
			ret = -EINVAL;
			goto end;
		}

		ret = qcom_clk_crmb_set_rate(core_clk, CRM_SW_DRV, 0, 0, CRM_PWR_STATE0,
					data->data.core_clk_rate_ab, data->data.core_clk_rate_ib);
		if (ret) {
			SDE_ERROR_CESTA("sw-client-0 Clk vote failed, ab:%llu, ib:%llu, ret:%d\n",
				data->data.core_clk_rate_ab, data->data.core_clk_rate_ib, ret);
			goto end;
		}
		cesta->sw_client.data.core_clk_rate_ab = data->data.core_clk_rate_ab;
		cesta->sw_client.data.core_clk_rate_ib = data->data.core_clk_rate_ib;
	}

end:
	mutex_unlock(&cesta->client_lock);
	return ret;
}

int sde_cesta_aoss_update(struct sde_cesta_client *client, enum sde_cesta_aoss_cp_level cp_level)
{
	struct sde_cesta *cesta;
	struct crm_cmd cmd = {0, };
	int ret = 0;

	if (!client || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid param - client:%d, cesta_index:%d\n",
					!!client, client ? client->cesta_index : -1);
		return -EINVAL;
	}
	cesta = cesta_list[client->cesta_index];

	cmd.data = cp_level;
	cmd.resource_idx = 0;
	cmd.wait = true;

	cmd.pwr_state.sw = CRM_ACTIVE_STATE;
	ret = crm_write_perf_ol(cesta->crm_dev, CRM_SW_DRV, client->scc_index, &cmd);
	if (ret)
		SDE_ERROR_CESTA("AOSS vote failed - client:%d, scc:%d, cp_level:%d, ret:%d\n",
				client->client_index, client->scc_index, cp_level, ret);
	else
		client->aoss_cp_level = cp_level;

	SDE_EVT32(client->client_index, client->scc_index, cp_level, ret);

	return ret;
}

u64 sde_cesta_get_core_clk_rate(u32 cesta_index)
{
	struct sde_cesta *cesta;
	struct sde_cesta_client *c;
	struct sde_cesta_client_data *client_data;
	struct clk *core_clk = NULL;
	u64 core_clk_ab = 0, core_clk_ib = 0, core_clk_rate = 0;

	if (!sde_cesta_is_enabled(cesta_index))
		return 0;

	cesta = cesta_list[cesta_index];

	core_clk = _sde_cesta_get_core_clk(cesta);
	if (!core_clk) {
		SDE_ERROR_CESTA("core_clk not found\n");
		return 0;
	}

	mutex_lock(&cesta->client_lock);

	list_for_each_entry(c, &cesta->client_list, list) {
		if (!c->enabled)
			continue;

		client_data = &c->hw;

		core_clk_ab += client_data->core_clk_rate_ab;
		core_clk_ib = max(core_clk_ib, client_data->core_clk_rate_ib);
	}
	core_clk_rate = max(core_clk_ab, core_clk_ib);
	core_clk_rate = clk_round_rate(core_clk, core_clk_rate);

	mutex_unlock(&cesta->client_lock);

	SDE_EVT32(cesta_index, core_clk_rate, core_clk_ab, core_clk_ib);

	return core_clk_rate;
}

struct sde_cesta_client *sde_cesta_create_client(u32 cesta_index, char *client_name)
{
	struct sde_cesta *cesta;
	struct sde_cesta_client *client;
	static int id;

	if (!sde_cesta_is_enabled(cesta_index))
		return NULL;

	cesta = cesta_list[cesta_index];
	client = devm_kzalloc(cesta->dev, sizeof(struct sde_cesta_client), GFP_KERNEL);
	if (!client)
		return ERR_PTR(-ENOMEM);

	strscpy(client->name, client_name, MAX_CESTA_CLIENT_NAME_LEN);
	client->cesta_index = cesta_index;
	client->client_index = id;
	client->scc_index = cesta->scc_index[id];
	SDE_DEBUG_CESTA("client:%s cesta_index:%d\n", client_name, cesta_index);

	mutex_lock(&cesta->client_lock);
	list_add(&client->list, &cesta->client_list);
	id++;
	mutex_unlock(&cesta->client_lock);

	return client;
}

void sde_cesta_destroy_client(struct sde_cesta_client *client)
{
	struct sde_cesta *cesta;

	if (!client || (client->cesta_index >= MAX_CESTA_COUNT)) {
		SDE_ERROR_CESTA("invalid params - client:%d, cesta_idx:%d\n",
				!!client, client ? client->cesta_index : -1);
	}

	cesta = cesta_list[client->cesta_index];
	if (!cesta)
		return;

	mutex_lock(&cesta->client_lock);
	list_del_init(&client->list);
	mutex_unlock(&cesta->client_lock);
}

int sde_cesta_resource_disable(u32 cesta_index)
{
	struct sde_cesta *cesta;
	struct sde_power_handle *phandle;
	struct dss_module_power *mp;
	struct sde_cesta_sw_client_data sw_data = {0, };
	u32 sw_update_flag = 0;
	int ret;

	if (!sde_cesta_is_enabled(cesta_index))
		return 0;

	cesta = cesta_list[cesta_index];

	SDE_EVT32(cesta_index, cesta->sw_fs_enabled, SDE_EVTLOG_FUNC_ENTRY);

	if (cesta->sw_fs_enabled) {
		/* remove the AOSS & BW votes placed during enable */
		sw_data.aoss_cp_level = SDE_CESTA_AOSS_CP_LEVEL_0;
		sw_update_flag = SDE_CESTA_SW_CLIENT_BW_UPDATE | SDE_CESTA_SW_CLIENT_AOSS_UPDATE;

		if (cesta->fs) {
			ret = regulator_set_mode(cesta->fs, REGULATOR_MODE_FAST);
			if (ret) {
				SDE_ERROR_CESTA("vdd reg fast mode set failed, ret:%d\n", ret);
				return ret;
			}
		} else if (cesta->pd_fs) {
			pm_runtime_put_sync(cesta->pd_fs);
		}

		cesta->sw_fs_enabled = false;
	}

	/* remove last minimum vote for GDSC to enter power-collapse */
	sw_update_flag |= SDE_CESTA_SW_CLIENT_BW_UPDATE;
	ret = sde_cesta_sw_client_update(cesta_index, &sw_data, sw_update_flag);
	if (ret) {
		SDE_ERROR_CESTA("sw-client voting failed, ret:%d", ret);
		return ret;
	}

	phandle = &cesta->phandle;
	mp = &phandle->mp;

	sde_power_mmrm_reserve(phandle);
	msm_dss_enable_clk(mp->clk_config, mp->num_clk, false);

	ret = _sde_cesta_check_mode2_entry_status(cesta_index);
	if (ret) {
		SDE_ERROR_CESTA("mode2 entry failed ret:%d\n", ret);
		return ret;
	}

	/*
	 * Add delay before disabling MMCX to allow HW to complete any pending operations.
	 * This avoids potential NOC issue.
	 */
	usleep_range(500, 510);

	return 0;
}

int sde_cesta_resource_enable(u32 cesta_index)
{
	struct sde_cesta *cesta;
	struct sde_power_handle *phandle;
	struct dss_module_power *mp;
	struct sde_cesta_sw_client_data sw_data = {0, };
	u32 sw_update_flag = 0;
	int ret = 0;

	if (!sde_cesta_is_enabled(cesta_index))
		return 0;

	cesta = cesta_list[cesta_index];
	phandle = &cesta->phandle;
	mp = &phandle->mp;

	SDE_EVT32(cesta_index, cesta->sw_fs_enabled);

	ret = msm_dss_enable_clk(mp->clk_config, mp->num_clk, true);
	if (ret) {
		SDE_ERROR_CESTA("clock enable failed, rc:%d\n", ret);
		return ret;
	}

	/*
	 * Add sw-client-0 AOSS & BW vote in first enable and remove it as part of
	 * first disable. This is to make sure, all CRMB_PT resource is voted and
	 * ready to send idle signal on disable.
	 * Core-clk is already voted as part of the clk_enable.
	 */
	if (cesta->sw_fs_enabled) {
		sw_data.aoss_cp_level = SDE_CESTA_AOSS_CP_LEVEL_4;
		sw_data.data.bw_ab = 0;
		sw_data.data.bw_ib = cesta->perf_cfg.max_bw_kbps * 1000ULL;
		sw_update_flag = SDE_CESTA_SW_CLIENT_BW_UPDATE | SDE_CESTA_SW_CLIENT_AOSS_UPDATE;

		ret = sde_cesta_sw_client_update(cesta_index, &sw_data, sw_update_flag);
		if (ret) {
			SDE_ERROR_CESTA("AOSS/BW sw-client voting failed, ret:%d", ret);
			return ret;
		}
	}

	return 0;
}

#if defined(CONFIG_DEBUG_FS)
static int _sde_debugfs_status_show(struct seq_file *s, void *data)
{
	struct sde_cesta *cesta;
	struct sde_cesta_client *client;
	struct sde_cesta_client_data *c_data;
	struct sde_cesta_scc_status status = {0, };
	bool enabled = false;
	u32 pwr_event;

	if (!s || !s->private)
		return -EINVAL;

	cesta = s->private;

	mutex_lock(&cesta->client_lock);

	list_for_each_entry(client, &cesta->client_list, list) {

		c_data = &client->hw;
		enabled |= client->enabled;

		seq_printf(s, "client%d:%s en:%d\n", client->client_index, client->name,
				client->enabled);
		seq_printf(s, "\t pwr_override:%d vote_state:%d aoss_cp_level:%d\n",
				client->pwr_st_override, client->vote_state, client->aoss_cp_level);

		seq_printf(s, "\t HW-Client - core_clk[ab,ib]:%llu,%llu ",
				c_data->core_clk_rate_ab, c_data->core_clk_rate_ib);
		seq_printf(s, "bw[ab,ib]:%llu,%llu ", c_data->bw_ab, c_data->bw_ib);
		seq_puts(s, "\n\n");
	}

	c_data = &cesta->sw_client.data;
	seq_printf(s, "SW-Client-0 - core_clk[ab,ib]:%llu,%llu ",
				c_data->core_clk_rate_ab, c_data->core_clk_rate_ib);
	seq_printf(s, "bw[ab,ib]:%llu,%llu ", c_data->bw_ab, c_data->bw_ib);
	seq_printf(s, "aoss_cp_level:%d\n", cesta->sw_client.aoss_cp_level);
	seq_puts(s, "\n\n");

	if (enabled && cesta->hw_ops.get_status && cesta->hw_ops.get_pwr_event) {
		pwr_event = cesta->hw_ops.get_pwr_event(cesta);
		seq_printf(s, "PWR_event:%d\n", pwr_event);

		list_for_each_entry(client, &cesta->client_list, list) {
			cesta->hw_ops.get_status(cesta, client->client_index, &status);

			seq_printf(s, "SCC[%d] status - ", client->client_index);
			seq_printf(s, "frame_region:%d scc_hshake:%d fsm_st:%d flush_miss_cnt:%d\n",
					status.frame_region, status.sch_handshake, status.fsm_state,
					status.flush_missed_counter);
		}
	}

	seq_puts(s, "\n");
	mutex_unlock(&cesta->client_lock);

	return 0;
}

static int _sde_debugfs_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, _sde_debugfs_status_show, inode->i_private);
}

static const struct file_operations debugfs_status_fops = {
	.open =		_sde_debugfs_status_open,
	.read =		seq_read,
	.llseek =	seq_lseek,
	.release =	single_release,
};

static void _sde_cesta_init_debugfs(struct sde_cesta *cesta, char *name)
{
	cesta->debugfs_root = debugfs_create_dir(name, NULL);
	if (!cesta->debugfs_root)
		return;

	debugfs_create_file("status", 0400, cesta->debugfs_root, cesta, &debugfs_status_fops);
	debugfs_create_x32("debug_mode", 0600, cesta->debugfs_root, &cesta->debug_mode);
}

#else
static void _sde_cesta_init_debugfs(struct sde_cesta *cesta, char *name)
{
}
#endif /* defined(CONFIG_DEBUG_FS) */

static int sde_cesta_get_io_resources(struct msm_io_res *io_res, void *data)
{
	int rc = 0;
	struct sde_cesta *sde_cesta = (struct sde_cesta *)data;
	struct platform_device *pdev = to_platform_device(sde_cesta->dev);

	rc = msm_dss_get_io_mem(pdev, &io_res->mem);
	if (rc) {
		SDE_ERROR_CESTA("failed to get cesta io mem, rc:%d\n", rc);
		return rc;
	}

	return 0;
}

int sde_cesta_bind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sde_cesta *cesta;
	int i;
	struct msm_vm_ops vm_event_ops = {
		.vm_get_io_resources = sde_cesta_get_io_resources,
	};

	if (!dev || !master) {
		SDE_ERROR_CESTA("invalid param(s), dev:%d, master:%d\n", !!dev, !!master);
		return -EINVAL;
	}

	cesta = platform_get_drvdata(pdev);
	if (!cesta) {
		SDE_ERROR_CESTA("invalid sde cesta\n");
		return -EINVAL;
	}

	sde_dbg_reg_register_base("sde_rsc_rscc", cesta->rscc_io.base,
			cesta->rscc_io.len, msm_get_phys_addr(pdev, "rscc"), SDE_DBG_RSC);

	sde_dbg_reg_register_base("sde_rsc_wrapper", cesta->wrapper_io.base,
			cesta->wrapper_io.len, msm_get_phys_addr(pdev, "wrapper"), SDE_DBG_RSC);

	for (i = 0; i < cesta->scc_count; i++) {
		char blk_name[32];

		snprintf(blk_name, sizeof(blk_name), "scc_%u", cesta->scc_index[i]);
		sde_dbg_reg_register_base(blk_name, cesta->scc_io[i].base,
			cesta->scc_io[i].len, msm_get_phys_addr(pdev, blk_name), SDE_DBG_RSC);
	}

	sde_dbg_reg_register_base("sde_crm", cesta->sde_crm_io.base,
			cesta->sde_crm_io.len, SDE_CRM_BASE, SDE_DBG_RSC);

	msm_register_vm_event(master, dev, &vm_event_ops, (void *)cesta);

	return 0;
}

void sde_cesta_unbind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sde_cesta *cesta;

	if (!dev || !master) {
		SDE_ERROR_CESTA("invalid param(s), dev:%d, master:%d\n", !!dev, !!master);
		return;
	}

	cesta = platform_get_drvdata(pdev);
	if (!cesta) {
		SDE_ERROR_CESTA("invalid sde cesta\n");
		return;
	}

	msm_unregister_vm_event(master, dev);
}

static const struct component_ops sde_cesta_comp_ops = {
	.bind = sde_cesta_bind,
	.unbind = sde_cesta_unbind,
};

static void sde_cesta_deinit(struct platform_device *pdev, struct sde_cesta *cesta)
{
	int i;

	if (!cesta)
		return;

	for (i = 0; i < cesta->scc_count; i++) {
		if (cesta->bus_hdl[i])
			icc_put(cesta->bus_hdl[i]);
		if (cesta->bus_hdl_idle[i])
			icc_put(cesta->bus_hdl_idle[i]);
	}

	if (cesta->sw_client_bus_hdl)
		icc_put(cesta->sw_client_bus_hdl);

	if (cesta->sw_fs_enabled) {
		if (cesta->pd_fs)
			pm_runtime_put_sync(cesta->pd_fs);
		else if (cesta->fs)
			regulator_disable(cesta->fs);
	}

	if (cesta->pd_fs)
		dev_pm_domain_detach(cesta->pd_fs, false);

	if (cesta->rscc_io.base)
		msm_dss_iounmap(&cesta->rscc_io);

	if (cesta->wrapper_io.base)
		msm_dss_iounmap(&cesta->wrapper_io);

	for (i = 0; i < cesta->scc_count; i++) {
		if (cesta->scc_io[i].base)
			msm_dss_iounmap(&cesta->scc_io[i]);
	}

	sde_power_resource_deinit(pdev, &cesta->phandle);
}

static int sde_cesta_probe(struct platform_device *pdev)
{
	struct sde_cesta *cesta;
	int ret, i, index;
	struct icc_path *path;
	char name[MAX_CESTA_CLIENT_NAME_LEN];

	cesta = devm_kzalloc(&pdev->dev, sizeof(struct sde_cesta), GFP_KERNEL);
	if (!cesta)
		return -ENOMEM;

	platform_set_drvdata(pdev, cesta);
	cesta->dev = &pdev->dev;

	ret = of_property_read_u32(pdev->dev.of_node, "cell-index", &index);
	if (ret)
		index = SDE_CESTA_INDEX;

	if (index >= MAX_CESTA_COUNT) {
		SDE_ERROR_CESTA("SDE CESTA supports probe till MAX_CESTA_COUNT:%d\n",
				MAX_CESTA_COUNT);
		return -EINVAL;
	}

	ret = sde_power_resource_init(pdev, &cesta->phandle);
	if (ret) {
		SDE_ERROR_CESTA("power resource init failed, ret:%d\n", ret);
		goto fail;
	}

	ret = msm_dss_ioremap_byname(pdev, &cesta->rscc_io, "rscc");
	if (ret) {
		SDE_ERROR_CESTA("rscc io data mapping failed, ret:%d\n", ret);
		goto fail;
	}

	ret = msm_dss_ioremap_byname(pdev, &cesta->wrapper_io, "wrapper");
	if (ret) {
		SDE_ERROR_CESTA("wrapper io data mapping failed, ret:%d\n", ret);
		goto fail;
	}

	cesta->sde_crm_io.base = ioremap(SDE_CRM_BASE, SDE_CRM_LEN);
	cesta->sde_crm_io.len = SDE_CRM_LEN;

	cesta->scc_count = 0;
	for (i = 0; i < MAX_SCC_BLOCK; i++) {
		char blk_name[32];

		snprintf(blk_name, sizeof(blk_name), "scc_%u", i);
		ret = msm_dss_ioremap_byname(pdev, &cesta->scc_io[cesta->scc_count], blk_name);
		if (ret) {
			SDE_DEBUG_CESTA("scc_%u io data mapping failed, ret:%d\n", i, ret);
			ret = 0;
		} else {
			cesta->scc_index[cesta->scc_count] = i;
			cesta->scc_count++;
		}
	}

	for (i = 0; i < cesta->scc_count; i++) {
		char bus_name[32];

		snprintf(bus_name, sizeof(bus_name), "%s-%d", DATA_BUS_HW_CLIENT_NAME, i);

		ret = of_property_match_string(pdev->dev.of_node, "interconnect-names", bus_name);
		if (ret < 0) {
			SDE_ERROR_CESTA("interconnect not found for %s, ret:%d\n", bus_name, ret);
			goto fail;
		}

		path = of_icc_get(&pdev->dev, bus_name);
		if (IS_ERR_OR_NULL(path)) {
			SDE_ERROR_CESTA("of_icc_get failed for %s, ret:%ld\n",
					bus_name, PTR_ERR(path));
			goto fail;
		}
		cesta->bus_hdl[i] = path;

		path = of_icc_get(&pdev->dev, bus_name);
		if (IS_ERR_OR_NULL(path)) {
			SDE_ERROR_CESTA("of_icc_get for idle failed for %s, ret:%ld\n",
					bus_name, PTR_ERR(path));
			goto fail;
		}
		cesta->bus_hdl_idle[i] = path;

	}

	ret = of_property_match_string(pdev->dev.of_node, "interconnect-names",
					DATA_BUS_SW_CLIENT_0_NAME);
	if (ret < 0) {
		SDE_ERROR_CESTA("interconnect not found for %s, ret:%d\n",
					DATA_BUS_SW_CLIENT_0_NAME, ret);
		goto fail;
	}

	path = of_icc_get(&pdev->dev, DATA_BUS_SW_CLIENT_0_NAME);
	if (IS_ERR_OR_NULL(path)) {
		SDE_ERROR_CESTA("of_icc_get failed for %s, ret:%ld\n",
					DATA_BUS_SW_CLIENT_0_NAME, PTR_ERR(path));
		goto fail;
	}
	cesta->sw_client_bus_hdl = path;

	cesta->crm_dev = crm_get_device("disp_crm");
	if (IS_ERR_OR_NULL(cesta->crm_dev)) {
		SDE_ERROR_CESTA("get crm_dev failed:%ld\n", PTR_ERR(cesta->crm_dev));
		cesta->crm_dev = NULL;
		goto fail;
	}

	if (pdev->dev.pm_domain) {
		pm_runtime_enable(&pdev->dev);
		cesta->pd_fs = &pdev->dev;
		cesta->fs = NULL;
	} else {
		cesta->fs = devm_regulator_get(&pdev->dev, "vdd");
		if (IS_ERR_OR_NULL(cesta->fs)) {
			SDE_ERROR_CESTA("regulator get failed, ret:%d\n", ret);
			cesta->fs = NULL;
		}
		cesta->pd_fs = NULL;
	}

	if (cesta->pd_fs)
		ret = pm_runtime_get_sync(cesta->pd_fs);
	else if (cesta->fs)
		ret = regulator_enable(cesta->fs);
	if (ret) {
		SDE_ERROR_CESTA("regulator enabled failed, ret:%d\n", ret);
		goto fail;
	}
	cesta->sw_fs_enabled = true;

	INIT_LIST_HEAD(&cesta->client_list);
	mutex_init(&cesta->client_lock);

	cesta_list[index] = cesta;

	sde_cesta_hw_init(cesta);
	cesta->hw_ops.init(cesta);

	snprintf(name, MAX_CESTA_CLIENT_NAME_LEN, "%s%d", "sde_cesta_", index);
	_sde_cesta_init_debugfs(cesta, name);

	pr_info("sde cesta index:%d probed successfully\n", index);

	ret = component_add(&pdev->dev, &sde_cesta_comp_ops);
	if (ret) {
		SDE_ERROR_CESTA("component add failed, ret:%d\n", ret);
		goto fail;
	}

	return 0;

fail:
	sde_cesta_deinit(pdev, cesta);

	return ret;
}

static int sde_cesta_remove(struct platform_device *pdev)
{
	struct sde_cesta *cesta = platform_get_drvdata(pdev);

	sde_cesta_deinit(pdev, cesta);

	return 0;
}

static const struct of_device_id dt_match[] = {
	{ .compatible = "qcom,sde-cesta"},
	{},
};

static struct platform_driver sde_cesta_platform_driver = {
	.probe      = sde_cesta_probe,
	.remove     = sde_cesta_remove,
	.driver     = {
		.name   = "sde_cesta",
		.of_match_table = dt_match,
		.suppress_bind_attrs = true,
	},
};

void __init sde_cesta_register(void)
{
	platform_driver_register(&sde_cesta_platform_driver);
}

void __exit sde_cesta_unregister(void)
{
	platform_driver_unregister(&sde_cesta_platform_driver);
}
