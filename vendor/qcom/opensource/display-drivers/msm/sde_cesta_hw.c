// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt)	"[sde_cesta_hw:%s:%d]: " fmt, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/iopoll.h>

#include "sde_cesta.h"
#include "sde_dbg.h"

#define RSCC_SEQ_PWR_CTRL_STATUS	0x2d0

#define RSCC_WRAPPER_CTRL		0x0
#define RSCC_WRAPPER_DEBUG_BUS		0x10
#define RSCC_WRAPPER_DEBUG_BUS_DATA	0x14
#define RSCC_PWR_CTRL			0x24
#define RSCC_WRAPPER_SCC_CLK_GATE_ALLOW	0x40

#define SCC_CTRL			0x0
#define SCC_OVERRIDE_CTRL		0x4
#define SCC_CLK_GATE_SEL		0x8
#define SCC_HW_STATE_READBACK		0x10
#define SCC_DEBUG_FLUSH_MISSED		0x20
#define DEBUG_FLUSH_MISSED_CLEAR	0x24

void _sde_cesta_hw_init(struct sde_cesta *cesta)
{
	int i;

	for (i = 0; i < cesta->scc_count; i++) {
		dss_reg_w(&cesta->scc_io[i], SCC_CLK_GATE_SEL, 0x1, cesta->debug_mode);
		dss_reg_w(&cesta->wrapper_io, RSCC_WRAPPER_SCC_CLK_GATE_ALLOW + (0x4 * i),
				0x1, cesta->debug_mode);
	}
}

void _sde_cesta_hw_force_db_update(struct sde_cesta *cesta, u32 idx,
		bool en_auto_active, enum sde_cesta_ctrl_pwr_req_mode req_mode, bool en_hw_sleep,
		bool en_clk_gate)
{
	u32 ctl_val, override_val;

	ctl_val = dss_reg_r(&cesta->scc_io[idx], SCC_CTRL, cesta->debug_mode);
	override_val = dss_reg_r(&cesta->scc_io[idx], SCC_OVERRIDE_CTRL, cesta->debug_mode);

	if (en_auto_active)
		ctl_val |= BIT(3); /* set auto-active-on-panic */
	else
		ctl_val &= ~BIT(3);

	if (en_hw_sleep)
		ctl_val |= BIT(0); /* set hw sleep enable */
	else
		ctl_val &= ~BIT(0);

	if (en_clk_gate)
		ctl_val |= BIT(8); /* set clk gate enable */
	else
		ctl_val &= ~BIT(8);

	/* clear & set the pwr_req mode */
	ctl_val &= ~(BIT(1) | BIT(2));
	ctl_val |= (req_mode << 1);

	override_val |= BIT(0); /* set override force-db-update */

	dss_reg_w(&cesta->scc_io[idx], SCC_CTRL, ctl_val, cesta->debug_mode);
	dss_reg_w(&cesta->scc_io[idx], SCC_OVERRIDE_CTRL, override_val, cesta->debug_mode);
	wmb(); /* for reset to be applied immediately */

	SDE_EVT32(idx, ctl_val, override_val);
}

void _sde_cesta_hw_reset(struct sde_cesta *cesta, u32 idx, bool en)
{
	dss_reg_w(&cesta->scc_io[idx], SCC_OVERRIDE_CTRL, en ? BIT(31) : 0, cesta->debug_mode);
	wmb(); /* for reset to be applied immediately */
}

void _sde_cesta_hw_override_ctrl_setup(struct sde_cesta *cesta, u32 idx, u32 force_flags)
{
	u32 val = 0;

	if (force_flags & SDE_CESTA_OVERRIDE_FORCE_DB_UPDATE)
		val |= BIT(0);
	if (force_flags & SDE_CESTA_OVERRIDE_FORCE_IDLE)
		val |= BIT(1);
	if (force_flags & SDE_CESTA_OVERRIDE_FORCE_ACTIVE)
		val |= BIT(2);
	if (force_flags & SDE_CESTA_OVERRIDE_FORCE_CHN_UPDATE)
		val |= BIT(3);

	dss_reg_w(&cesta->scc_io[idx], SCC_OVERRIDE_CTRL, val, cesta->debug_mode);
	wmb(); /* for force votes to be applied immediately */
}

void _sde_cesta_hw_ctrl_setup(struct sde_cesta *cesta, u32 idx, struct sde_cesta_ctrl_cfg *cfg)
{
	u32 val = 0;

	if (!cfg || !cfg->enable) {
		dss_reg_w(&cesta->scc_io[idx], SCC_CTRL, 0xf0, cesta->debug_mode);
		_sde_cesta_hw_override_ctrl_setup(cesta, idx, SDE_CESTA_OVERRIDE_FORCE_DB_UPDATE);
		SDE_EVT32(idx, 0xf0);
		return;
	}

	if (cfg->avr_enable)
		val |= BIT(9);

	val |= BIT(8);

	val |= (cfg->req_mode << 1);

	if (cfg->wb)
		val |= (0xE << 4);
	else if (cfg->dual_dsi)
		val |= (0xD << 4);
	else
		val |= (cfg->intf << 4);

	if (cfg->auto_active_on_panic)
		val |= BIT(3);

	if (cfg->hw_sleep_enable)
		val |= BIT(0);

	dss_reg_w(&cesta->scc_io[idx], SCC_CTRL, val, cesta->debug_mode);
	SDE_EVT32(idx, val);
}

int _sde_cesta_hw_poll_handshake(struct sde_cesta *cesta, u32 idx)
{
	void __iomem *addr = cesta->scc_io[idx].base + SCC_HW_STATE_READBACK;
	u32 handshake_mask = BIT(4) | BIT(5);
	u32 handshake_vote_req = 0x1 << 4;
	u32 val;

	return readl_relaxed_poll_timeout(addr, val,
			(val & handshake_mask) != handshake_vote_req,
			100, 1000);
}

void _sde_cesta_hw_get_status(struct sde_cesta *cesta, u32 idx, struct sde_cesta_scc_status *status)
{
	u32 val;
	u32 debug1 = 0xc, debug2 = 0xd;
	u32 debug_val1, debug_val2;

	val = dss_reg_r(&cesta->scc_io[idx], SCC_HW_STATE_READBACK, cesta->debug_mode);

	status->frame_region = (val >> 8) & 0x3;
	status->sch_handshake = (val >> 4) & 0x3;
	status->fsm_state = val & 0x3;

	val = dss_reg_r(&cesta->scc_io[idx], SCC_DEBUG_FLUSH_MISSED, cesta->debug_mode);
	status->flush_missed_counter = val;

	/* clear flush_missed counter */
	dss_reg_w(&cesta->scc_io[idx], DEBUG_FLUSH_MISSED_CLEAR, 0x1, cesta->debug_mode);

	debug_val1 = (debug1 << 1) | BIT(0);
	dss_reg_w(&cesta->wrapper_io, 0x10, debug_val1, cesta->debug_mode);
	wmb();
	debug_val1 = dss_reg_r(&cesta->wrapper_io, 0x14, cesta->debug_mode);

	dss_reg_w(&cesta->wrapper_io, 0x10, 0x0, cesta->debug_mode);
	wmb();

	debug_val2 = (debug2 << 1) | BIT(0);
	dss_reg_w(&cesta->wrapper_io, 0x10, debug_val2, cesta->debug_mode);
	wmb();
	debug_val2 = dss_reg_r(&cesta->wrapper_io, 0x14, cesta->debug_mode);

	dss_reg_w(&cesta->wrapper_io, 0x10, 0x0, cesta->debug_mode);
	wmb();

	SDE_EVT32(idx, debug1, debug_val1, debug2, debug_val2);
}

u32 _sde_cesta_hw_get_pwr_event(struct sde_cesta *cesta)
{
	return dss_reg_r(&cesta->wrapper_io, RSCC_PWR_CTRL, cesta->debug_mode);
}

u32 _sde_get_rscc_pwr_ctrl_status(struct sde_cesta *cesta)
{
	return dss_reg_r(&cesta->rscc_io, RSCC_SEQ_PWR_CTRL_STATUS, cesta->debug_mode);
}

void sde_cesta_hw_init(struct sde_cesta *cesta)
{
	cesta->hw_ops.init = _sde_cesta_hw_init;
	cesta->hw_ops.ctrl_setup = _sde_cesta_hw_ctrl_setup;
	cesta->hw_ops.poll_handshake = _sde_cesta_hw_poll_handshake;
	cesta->hw_ops.get_status = _sde_cesta_hw_get_status;
	cesta->hw_ops.get_pwr_event = _sde_cesta_hw_get_pwr_event;
	cesta->hw_ops.override_ctrl_setup = _sde_cesta_hw_override_ctrl_setup;
	cesta->hw_ops.reset_ctrl = _sde_cesta_hw_reset;
	cesta->hw_ops.force_db_update = _sde_cesta_hw_force_db_update;
	cesta->hw_ops.get_rscc_pwr_ctrl_status = _sde_get_rscc_pwr_ctrl_status;
}
