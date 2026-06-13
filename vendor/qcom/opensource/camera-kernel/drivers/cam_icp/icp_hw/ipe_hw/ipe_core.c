// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/videodev2.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/iopoll.h>
#include "cam_io_util.h"
#include "cam_hw.h"
#include "cam_hw_intf.h"
#include "ipe_core.h"
#include "ipe_soc.h"
#include "cam_soc_util.h"
#include "cam_io_util.h"
#include "cam_ipe_hw_intf.h"
#include "cam_icp_hw_mgr_intf.h"
#include "cam_cpas_api.h"
#include "cam_debug_util.h"
#include "hfi_reg.h"
#include "cam_common_util.h"

static int cam_ipe_cpas_vote(struct cam_ipe_device_core_info *core_info,
	struct cam_icp_cpas_vote *cpas_vote)
{
	int rc = 0;

	if (cpas_vote->ahb_vote_valid)
		rc = cam_cpas_update_ahb_vote(core_info->cpas_handle,
			&cpas_vote->ahb_vote);
	if (cpas_vote->axi_vote_valid)
		rc = cam_cpas_update_axi_vote(core_info->cpas_handle,
			&cpas_vote->axi_vote);

	if (rc)
		CAM_ERR(CAM_PERF, "cpas vote is failed: %d", rc);

	return rc;
}

int cam_ipe_init_hw(void *device_priv,
	void *init_hw_args, uint32_t arg_size)
{
	struct cam_hw_info *ipe_dev = device_priv;
	struct cam_hw_soc_info *soc_info = NULL;
	struct cam_ipe_device_core_info *core_info = NULL;
	struct cam_icp_cpas_vote cpas_vote;
	unsigned long flags;
	int rc = 0;

	if (!device_priv) {
		CAM_ERR(CAM_ICP, "Invalid cam_dev_info");
		return -EINVAL;
	}

	soc_info = &ipe_dev->soc_info;
	core_info = (struct cam_ipe_device_core_info *)ipe_dev->core_info;

	if ((!soc_info) || (!core_info)) {
		CAM_ERR(CAM_ICP, "soc_info = %pK core_info = %pK",
			soc_info, core_info);
		return -EINVAL;
	}

	spin_lock_irqsave(&ipe_dev->hw_lock, flags);
	if (ipe_dev->hw_state == CAM_HW_STATE_POWER_UP) {
		core_info->power_on_cnt++;
		spin_unlock_irqrestore(&ipe_dev->hw_lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&ipe_dev->hw_lock, flags);

	rc = cam_vmrm_soc_acquire_resources(CAM_HW_ID_IPE + core_info->ipe_hw_info->hw_idx);
	if (rc) {
		CAM_ERR(CAM_ICP, "IPE hw id %x acquire ownership failed",
			CAM_HW_ID_IPE + core_info->ipe_hw_info->hw_idx);
		return rc;
	}

	cpas_vote.ahb_vote.type = CAM_VOTE_ABSOLUTE;
	cpas_vote.ahb_vote.vote.level = CAM_LOWSVS_D1_VOTE;
	cpas_vote.axi_vote.num_paths = 1;
	cpas_vote.axi_vote.axi_path[0].path_data_type =
		CAM_IPE_DEFAULT_AXI_PATH;
	cpas_vote.axi_vote.axi_path[0].transac_type =
		CAM_IPE_DEFAULT_AXI_TRANSAC;
	cpas_vote.axi_vote.axi_path[0].camnoc_bw =
		CAM_CPAS_DEFAULT_AXI_BW;
	cpas_vote.axi_vote.axi_path[0].mnoc_ab_bw =
		CAM_CPAS_DEFAULT_AXI_BW;
	cpas_vote.axi_vote.axi_path[0].mnoc_ib_bw =
		CAM_CPAS_DEFAULT_AXI_BW;

	rc = cam_cpas_start(core_info->cpas_handle,
		&cpas_vote.ahb_vote, &cpas_vote.axi_vote);
	if (rc) {
		CAM_ERR(CAM_ICP, "cpas start failed: %d", rc);
		goto error;
	}
	core_info->cpas_start = true;

	rc = cam_ipe_enable_soc_resources(soc_info);
	if (rc) {
		CAM_ERR(CAM_ICP, "soc enable is failed : %d", rc);
		if (cam_cpas_stop(core_info->cpas_handle))
			CAM_ERR(CAM_ICP, "cpas stop is failed");
		else
			core_info->cpas_start = false;
	} else {
		core_info->clk_enable = true;
	}

	spin_lock_irqsave(&ipe_dev->hw_lock, flags);
	ipe_dev->hw_state = CAM_HW_STATE_POWER_UP;
	core_info->power_on_cnt++;
	spin_unlock_irqrestore(&ipe_dev->hw_lock, flags);
	CAM_DBG(CAM_ICP, "IPE%u powered on (refcnt: %u)",
		soc_info->index, core_info->power_on_cnt);

error:
	return rc;
}

int cam_ipe_deinit_hw(void *device_priv,
	void *init_hw_args, uint32_t arg_size)
{
	struct cam_hw_info *ipe_dev = device_priv;
	struct cam_hw_soc_info *soc_info = NULL;
	struct cam_ipe_device_core_info *core_info = NULL;
	unsigned long flags;
	int rc = 0;

	if (!device_priv) {
		CAM_ERR(CAM_ICP, "Invalid cam_dev_info");
		return -EINVAL;
	}

	soc_info = &ipe_dev->soc_info;
	core_info = (struct cam_ipe_device_core_info *)ipe_dev->core_info;
	if ((!soc_info) || (!core_info)) {
		CAM_ERR(CAM_ICP, "soc_info = %pK core_info = %pK",
			soc_info, core_info);
		return -EINVAL;
	}

	spin_lock_irqsave(&ipe_dev->hw_lock, flags);
	if (ipe_dev->hw_state == CAM_HW_STATE_POWER_DOWN) {
		spin_unlock_irqrestore(&ipe_dev->hw_lock, flags);
		return 0;
	}

	core_info->power_on_cnt--;
	if (core_info->power_on_cnt) {
		spin_unlock_irqrestore(&ipe_dev->hw_lock, flags);
		CAM_DBG(CAM_ICP, "IPE%u power on reference still held %u",
			soc_info->index, core_info->power_on_cnt);
		return 0;
	}
	spin_unlock_irqrestore(&ipe_dev->hw_lock, flags);

	rc = cam_ipe_disable_soc_resources(soc_info, core_info->clk_enable);
	if (rc)
		CAM_ERR(CAM_ICP, "soc disable is failed : %d", rc);
	core_info->clk_enable = false;

	if (core_info->cpas_start) {
		if (cam_cpas_stop(core_info->cpas_handle))
			CAM_ERR(CAM_ICP, "cpas stop is failed");
		else
			core_info->cpas_start = false;
	}

	spin_lock_irqsave(&ipe_dev->hw_lock, flags);
	ipe_dev->hw_state = CAM_HW_STATE_POWER_DOWN;
	spin_unlock_irqrestore(&ipe_dev->hw_lock, flags);

	CAM_DBG(CAM_ICP, "IPE%u powered off (refcnt: %u)",
		soc_info->index, core_info->power_on_cnt);

	rc = cam_vmrm_soc_release_resources(CAM_HW_ID_IPE + core_info->ipe_hw_info->hw_idx);
	if (rc) {
		CAM_ERR(CAM_ICP, "IPE hw id %x release ownership failed",
			CAM_HW_ID_IPE + core_info->ipe_hw_info->hw_idx);
		return rc;
	}

	return rc;
}

static int cam_ipe_handle_pc(struct cam_hw_info *ipe_dev)
{
	struct cam_hw_soc_info *soc_info = NULL;
	struct cam_ipe_device_core_info *core_info = NULL;
	struct cam_ipe_device_hw_info *hw_info = NULL;
	int rc = 0;
	int pwr_ctrl;
	int pwr_status;

	soc_info = &ipe_dev->soc_info;
	core_info = (struct cam_ipe_device_core_info *)ipe_dev->core_info;
	hw_info = core_info->ipe_hw_info;

	if (!core_info->cpas_start) {
		CAM_DBG(CAM_ICP, "CPAS IPE client not started");
		return 0;
	}

	rc = cam_cpas_reg_read(core_info->cpas_handle,
			CAM_CPAS_REGBASE_CPASTOP, hw_info->pwr_ctrl,
			true, &pwr_ctrl);
	if (rc) {
		CAM_ERR(CAM_ICP, "power ctrl read failed rc=%d", rc);
		return rc;
	}

	if (!(pwr_ctrl & IPE_COLLAPSE_MASK)) {
		rc = cam_cpas_reg_read(core_info->cpas_handle,
				CAM_CPAS_REGBASE_CPASTOP, hw_info->pwr_status,
				true, &pwr_status);
		if (rc) {
			CAM_ERR(CAM_ICP, "power status read failed rc=%d", rc);
			return rc;
		}

		cam_cpas_reg_write(core_info->cpas_handle,
			CAM_CPAS_REGBASE_CPASTOP,
			hw_info->pwr_ctrl, true, 0x1);

		if (pwr_status >> IPE_PWR_ON_MASK)
			CAM_WARN(CAM_PERF, "BPS: pwr_status(%x):pwr_ctrl(%x)",
				pwr_status, pwr_ctrl);

	}

	rc = cam_ipe_get_gdsc_control(soc_info);
	if (rc) {
		CAM_ERR(CAM_ICP, "failed to get gdsc control rc=%d", rc);
		return rc;
	}

	rc = cam_cpas_reg_read(core_info->cpas_handle,
			CAM_CPAS_REGBASE_CPASTOP, hw_info->pwr_ctrl,
			true, &pwr_ctrl);
	if (rc) {
		CAM_ERR(CAM_ICP, "power ctrl read failed rc=%d", rc);
		return rc;
	}

	rc = cam_cpas_reg_read(core_info->cpas_handle,
			CAM_CPAS_REGBASE_CPASTOP, hw_info->pwr_status,
			true, &pwr_status);
	if (rc) {
		CAM_ERR(CAM_ICP, "power status read failed rc=%d", rc);
		return rc;
	}

	CAM_DBG(CAM_PERF, "pwr_ctrl=%x pwr_status=%x", pwr_ctrl, pwr_status);

	return 0;
}

static int cam_ipe_handle_resume(struct cam_hw_info *ipe_dev)
{
	struct cam_hw_soc_info *soc_info = NULL;
	struct cam_ipe_device_core_info *core_info = NULL;
	struct cam_ipe_device_hw_info *hw_info = NULL;
	int pwr_ctrl;
	int pwr_status;
	int rc = 0;

	soc_info = &ipe_dev->soc_info;
	core_info = (struct cam_ipe_device_core_info *)ipe_dev->core_info;
	hw_info = core_info->ipe_hw_info;

	if (!core_info->cpas_start) {
		CAM_DBG(CAM_ICP, "CPAS IPE client not started");
		return 0;
	}

	rc = cam_cpas_reg_read(core_info->cpas_handle,
			CAM_CPAS_REGBASE_CPASTOP, hw_info->pwr_ctrl,
			true, &pwr_ctrl);
	if (rc) {
		CAM_ERR(CAM_ICP, "power ctrl read failed rc=%d", rc);
		return rc;
	}

	if (pwr_ctrl & IPE_COLLAPSE_MASK) {
		CAM_DBG(CAM_PERF, "IPE pwr_ctrl set(%x)", pwr_ctrl);
		cam_cpas_reg_write(core_info->cpas_handle,
			CAM_CPAS_REGBASE_CPASTOP,
			hw_info->pwr_ctrl, true, 0);
	}

	rc = cam_ipe_transfer_gdsc_control(soc_info);
	if (rc) {
		CAM_ERR(CAM_ICP, "failed to transfer gdsc control rc=%d", rc);
		return rc;
	}

	rc = cam_cpas_reg_read(core_info->cpas_handle,
			CAM_CPAS_REGBASE_CPASTOP, hw_info->pwr_ctrl,
			true, &pwr_ctrl);
	if (rc) {
		CAM_ERR(CAM_ICP, "power ctrl read failed rc=%d", rc);
		return rc;
	}

	rc = cam_cpas_reg_read(core_info->cpas_handle,
			CAM_CPAS_REGBASE_CPASTOP, hw_info->pwr_status,
			true, &pwr_status);
	if (rc) {
		CAM_ERR(CAM_ICP, "power status read failed rc=%d", rc);
		return rc;
	}

	CAM_DBG(CAM_PERF, "pwr_ctrl=%x pwr_status=%x", pwr_ctrl, pwr_status);

	return rc;
}

int cam_ipe_process_cmd(void *device_priv, uint32_t cmd_type,
	void *cmd_args, uint32_t arg_size)
{
	struct cam_hw_info *ipe_dev = device_priv;
	struct cam_hw_soc_info *soc_info = NULL;
	struct cam_ipe_device_core_info *core_info = NULL;
	struct cam_ipe_device_hw_info *hw_info = NULL;
	int rc = 0;

	if (!device_priv) {
		CAM_ERR(CAM_ICP, "Invalid arguments");
		return -EINVAL;
	}

	if (cmd_type >= CAM_ICP_DEV_CMD_MAX) {
		CAM_ERR(CAM_ICP, "Invalid command : %x", cmd_type);
		return -EINVAL;
	}

	soc_info = &ipe_dev->soc_info;
	core_info = (struct cam_ipe_device_core_info *)ipe_dev->core_info;
	hw_info = core_info->ipe_hw_info;

	switch (cmd_type) {
	case CAM_ICP_DEV_CMD_VOTE_CPAS: {
		struct cam_icp_cpas_vote *cpas_vote = cmd_args;

		if (!cmd_args)
			return -EINVAL;

		cam_ipe_cpas_vote(core_info, cpas_vote);
		break;
	}

	case CAM_ICP_DEV_CMD_CPAS_START: {
		struct cam_icp_cpas_vote *cpas_vote = cmd_args;

		if (!cmd_args)
			return -EINVAL;

		if (!core_info->cpas_start) {
			rc = cam_cpas_start(core_info->cpas_handle,
				&cpas_vote->ahb_vote, &cpas_vote->axi_vote);
			core_info->cpas_start = true;
		}
		break;
	}

	case CAM_ICP_DEV_CMD_CPAS_STOP:
		if (core_info->cpas_start) {
			cam_cpas_stop(core_info->cpas_handle);
			core_info->cpas_start = false;
		}
		break;
	case CAM_ICP_DEV_CMD_POWER_COLLAPSE:
		rc = cam_ipe_handle_pc(ipe_dev);
		break;
	case CAM_ICP_DEV_CMD_POWER_RESUME:
		rc = cam_ipe_handle_resume(ipe_dev);
		break;
	case CAM_ICP_DEV_CMD_UPDATE_CLK: {
		struct cam_icp_dev_clk_update_cmd *clk_upd_cmd = cmd_args;
		struct cam_ahb_vote ahb_vote;
		uint32_t clk_rate = clk_upd_cmd->curr_clk_rate;
		int32_t clk_level  = 0, err = 0;

		CAM_DBG(CAM_PERF, "ipe_src_clk rate = %d", (int)clk_rate);
		if (!core_info->clk_enable) {
			if (clk_upd_cmd->dev_pc_enable) {
				cam_ipe_handle_pc(ipe_dev);
				cam_cpas_reg_write(core_info->cpas_handle,
					CAM_CPAS_REGBASE_CPASTOP,
					hw_info->pwr_ctrl, true, 0x0);
			}
			rc = cam_ipe_toggle_clk(soc_info, true);
			if (rc)
				CAM_ERR(CAM_ICP, "Enable failed");
			else
				core_info->clk_enable = true;
			if (clk_upd_cmd->dev_pc_enable) {
				rc = cam_ipe_handle_resume(ipe_dev);
				if (rc)
					CAM_ERR(CAM_ICP, "bps resume failed");
			}
		}
		CAM_DBG(CAM_PERF, "clock rate %d", clk_rate);

		rc = cam_ipe_update_clk_rate(soc_info, &clk_rate);
		if (rc)
			CAM_ERR(CAM_PERF, "Failed to update clk %d", clk_rate);

		err = cam_soc_util_get_clk_level(soc_info,
			clk_rate, soc_info->src_clk_idx,
			&clk_level);

		if (!err) {
			clk_upd_cmd->clk_level = clk_level;
			ahb_vote.type = CAM_VOTE_ABSOLUTE;
			ahb_vote.vote.level = clk_level;

			rc = cam_cpas_update_ahb_vote(core_info->cpas_handle, &ahb_vote);
			if (rc) {
				CAM_ERR(CAM_PERF, "failed at updating ahb vote level rc: %d",
					rc);
				return rc;
			}

			rc = cam_cpas_update_axi_floor_lvl(core_info->cpas_handle,
				clk_level);
			if (rc) {
				CAM_ERR(CAM_PERF, "failed at updating axi vote level clk_level: %d rc: %d",
					clk_level, rc);
				return rc;
			}
		}
		break;
	}
	case CAM_ICP_DEV_CMD_DISABLE_CLK:
		if (core_info->clk_enable == true)
			cam_ipe_toggle_clk(soc_info, false);
		core_info->clk_enable = false;
		break;
	default:
		CAM_ERR(CAM_ICP, "Invalid Cmd Type:%u", cmd_type);
		rc = -EINVAL;
		break;
	}
	return rc;
}

irqreturn_t cam_ipe_irq(int irq_num, void *data)
{
	return IRQ_HANDLED;
}
