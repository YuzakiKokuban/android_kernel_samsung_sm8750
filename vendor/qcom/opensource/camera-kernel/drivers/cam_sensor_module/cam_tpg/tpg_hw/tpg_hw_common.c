// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "tpg_hw_common.h"

int tpg_hw_write_settings(struct tpg_hw *hw,
		struct tpg_settings_config_t *config, struct tpg_reg_settings *settings)
{
		struct cam_hw_soc_info *soc_info = NULL;
		int setting_index;
		phys_addr_t size;
		uint32_t tpg_reg_offset;

		if (!hw || !config || !settings) {
			CAM_ERR(CAM_TPG, "invalid params");
			return -EINVAL;
		}

		soc_info = hw->soc_info;
		size = resource_size(soc_info->mem_block[0]);

		for (setting_index = 0; setting_index < config->active_count; setting_index++) {
			tpg_reg_offset = settings->reg_offset;
			if (tpg_reg_offset >= size) {
				CAM_ERR(CAM_TPG, "settings reg_offset error");
				return -EINVAL;
			}
			cam_io_w_mb(settings->reg_value, soc_info->reg_map[0].mem_base +
						tpg_reg_offset);
			settings++;
		}

		return 0;
}
