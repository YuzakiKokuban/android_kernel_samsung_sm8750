// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ADAPTIVE_MIPI_V2_H__
#define __ADAPTIVE_MIPI_V2_H__

/* Adaptive MIPI v2 interfaces for display drivers */
#define MAX_MIPI_FREQ_CNT	(4)
#define INV_OSC_CLK		(0)

enum {
	DEFAULT_OSC_ID = 0,
	ALTERNATIVE_OSC_ID,
	MAX_OSC_FREQ_CNT,
};

enum {
	BANDWIDTH_10M_IDX = 0,
	BANDWIDTH_20M_IDX,
	MAX_BANDWIDTH_IDX,
};

struct adaptive_mipi_v2_table_element {
	int rat;
	int band;
	int from_ch;
	int end_ch;

	union {
		int mipi_clocks_rating[MAX_MIPI_FREQ_CNT];
		int osc_idx;
	};
};

struct adaptive_mipi_v2_adapter_funcs {
	int (*apply_freq)(int mipi_clk_kbps, int osc_clk_khz, void *ctx);
};

struct adaptive_mipi_v2_info {
	void *ctx;

	int mipi_clocks_kbps[MAX_MIPI_FREQ_CNT];
	int mipi_clocks_size;
	int osc_clocks_khz[MAX_OSC_FREQ_CNT];
	int osc_clocks_size;

	struct adaptive_mipi_v2_table_element *mipi_table[MAX_BANDWIDTH_IDX];
	int mipi_table_size[MAX_BANDWIDTH_IDX];

	struct adaptive_mipi_v2_table_element *osc_table;
	int osc_table_size;

	struct adaptive_mipi_v2_adapter_funcs *funcs;

	struct notifier_block ril_nb;
};

extern int sdp_init_adaptive_mipi_v2(struct adaptive_mipi_v2_info *info);
extern int sdp_cleanup_adaptive_mipi_v2(struct adaptive_mipi_v2_info *info);

#endif /* __ADAPTIVE_MIPI_V2_H__ */
