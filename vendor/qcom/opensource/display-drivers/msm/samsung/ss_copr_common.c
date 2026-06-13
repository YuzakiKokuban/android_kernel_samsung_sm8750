/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * COPR :
 * Author: QC LCD driver <kr0124.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ss_dsi_panel_common.h"
#include "ss_copr_common.h"

void ss_set_copr_sum(struct samsung_display_driver_data *vdd, enum COPR_CD_INDEX idx)
{
	s64 delta;

	mutex_lock(&vdd->copr.copr_val_lock);
	vdd->copr.copr_cd[idx].last_t = vdd->copr.copr_cd[idx].cur_t;
	vdd->copr.copr_cd[idx].cur_t = ktime_get();
	delta = ktime_ms_delta(vdd->copr.copr_cd[idx].cur_t, vdd->copr.copr_cd[idx].last_t);
	vdd->copr.copr_cd[idx].total_t += delta;
	if (vdd->br_info.common_br.gamma_mode2_support)
		vdd->copr.copr_cd[idx].cd_sum += (vdd->br_info.common_br.cd_level * delta);

	mutex_unlock(&vdd->copr.copr_val_lock);

	LCD_DEBUG(vdd, "[%d] cd (%d) delta (%lld) cd_sum (%lld) total_t (%lld)\n",
			idx,
			vdd->br_info.common_br.cd_level,
			delta,
			vdd->copr.copr_cd[idx].cd_sum,
			vdd->copr.copr_cd[idx].total_t);
}

int ss_copr_init(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd,   "vdd is null or error\n");
		return -ENODEV;
	}

	mutex_init(&vdd->copr.copr_val_lock);
	mutex_init(&vdd->copr.copr_lock);

	LCD_INFO(vdd,  "Success to initialized copr\n");

	return 0;
}
