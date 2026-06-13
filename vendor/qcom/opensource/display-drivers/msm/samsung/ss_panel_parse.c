/*
 * =================================================================
 *
 *
 *	Description:  samsung display parsing file
 *
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2023, Samsung Electronics. All rights reserved.

 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "ss_dsi_panel_common.h"

/* disable pll ssc */
int vdd_pll_ssc_disabled;

#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE) || IS_ENABLED(CONFIG_UML)
#if IS_ENABLED(CONFIG_SDP)
static void ss_free_cmd_legoop_map(struct cmd_legoop_map *table)
{
	int r;

	if (!table)
		return;

	for (r = 0; r < table->row_size; r++)
		kvfree(table->cmds[r]);
	kvfree(table->cmds);
}
#endif

static int ss_parse_dyn_mipi_clk_timing_table(struct device_node *np,
		struct samsung_display_driver_data *vdd, char *keystring)
{
	int i = 0, ret = 0;
	struct cmd_legoop_map org_table;
	struct clk_timing_table *table = &vdd->dyn_mipi_clk.clk_timing_table;

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	/* Precondition = Clock Timing table has only single col (n x 1) */
	table->tab_size = org_table.row_size;

	table->clk_rate = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->clk_rate)
		return -ENOMEM;

	for (i = 0 ; i < table->tab_size; i++) {
		table->clk_rate[i] = org_table.cmds[i][0];
		LCD_DEBUG(vdd, "Clock[%d] = %d\n", i, table->clk_rate[i]);
	}

	LCD_INFO(vdd, "[SDE] %s tab_size (%d)\n", keystring, table->tab_size);

	return 0;
}

static int ss_parse_dyn_mipi_clk_sel_table(struct device_node *np,
		struct samsung_display_driver_data *vdd, char *keystring)
{
	int i = 0, ret = 0;
	struct cmd_legoop_map org_table;
	struct clk_sel_table *table = &vdd->dyn_mipi_clk.clk_sel_table;

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	/* translate to candela table: org_table -> table */
	table->tab_size = org_table.row_size;

	LCD_INFO(vdd, "[%s] tab_size(row_size): %d\n", keystring, table->tab_size);

	table->rat = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->rat)
		goto error;
	table->band = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->band)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;
	table->target_clk_idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->target_clk_idx)
		goto error;
	if (vdd->dyn_mipi_clk.osc_support) {
		table->target_osc_idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
		if (!table->target_osc_idx)
			goto error;
	}

	for (i = 0 ; i < table->tab_size; i++) {
		table->rat[i] = org_table.cmds[i][0];
		table->band[i] = org_table.cmds[i][1];
		table->from[i] = org_table.cmds[i][2];
		table->end[i] = org_table.cmds[i][3];
		table->target_clk_idx[i] = org_table.cmds[i][4];
		if (vdd->dyn_mipi_clk.osc_support)
			table->target_osc_idx[i] = org_table.cmds[i][5];

		LCD_DEBUG(vdd, "[%d]= %d %d %d %d %d %d\n",
			i, table->rat[i], table->band[i], table->from[i],
			table->end[i], table->target_clk_idx[i],
			table->target_osc_idx ? table->target_osc_idx[i] : 9999);
	}

	LCD_INFO(vdd, "%s tab_size (%d)\n", keystring, table->tab_size);

	return 0;

error:
	LCD_ERR(vdd, "Allocation Fail\n");
	kfree(table->rat);
	kfree(table->band);
	kfree(table->from);
	kfree(table->end);
	kfree(table->target_clk_idx);

	return -ENOMEM;
}

#if IS_ENABLED(CONFIG_SDP)
static int ss_parse_adaptive_mipi_v2_mipi_clk_sel_table(struct device_node *np,
		struct samsung_display_driver_data *vdd,
		struct adaptive_mipi_v2_table_element **ret_table,
		int *ret_table_size,
		char *keystring)
{
	struct adaptive_mipi_v2_table_element *table;
	int table_size;
	struct cmd_legoop_map org_table;
	int i, j;
	int ret;
	int mipi_clocks_rating_size;

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	/* translate to candela table: org_table -> table */
	mipi_clocks_rating_size = org_table.col_size - 4;
	if (mipi_clocks_rating_size > MAX_MIPI_FREQ_CNT || mipi_clocks_rating_size  <= 0) {
		LCD_ERR(vdd, "invalid mipi clocks ratings size (%d)\n", mipi_clocks_rating_size);
		ss_free_cmd_legoop_map(&org_table);
		return -EINVAL;
	}

	table_size = org_table.row_size;
	table = kvzalloc(table_size * sizeof(struct adaptive_mipi_v2_table_element), GFP_KERNEL);
	if (!table) {
		LCD_ERR(vdd, "fail to allocate table\n");
		ss_free_cmd_legoop_map(&org_table);
		return -ENOMEM;
	}


	for (i = 0 ; i < table_size; i++) {
		table[i].rat = org_table.cmds[i][0];
		table[i].band = org_table.cmds[i][1];
		table[i].from_ch = org_table.cmds[i][2];
		table[i].end_ch = org_table.cmds[i][3];

		for (j = 0; j < mipi_clocks_rating_size; j++)
			table[i].mipi_clocks_rating[j] = org_table.cmds[i][4 + j];

		LCD_DEBUG(vdd, "[%d] %d %d %d %d %d %d %d %d\n", i,
				table[i].rat, table[i].band, table[i].from_ch, table[i].end_ch,
				table[i].mipi_clocks_rating[0],
				table[i].mipi_clocks_rating[1],
				table[i].mipi_clocks_rating[2],
				table[i].mipi_clocks_rating[3]);
	}

	LCD_INFO(vdd, "%s table_size (%d)\n", keystring, table_size);

	ss_free_cmd_legoop_map(&org_table);

	*ret_table = table;
	*ret_table_size = table_size;

	return 0;
}

static int ss_parse_adaptive_mipi_v2_osc_clk_sel_table(struct device_node *np,
		struct samsung_display_driver_data *vdd,
		struct adaptive_mipi_v2_table_element **ret_table, int *ret_table_size,
		char *keystring)
{
	struct adaptive_mipi_v2_table_element *table;
	int table_size;
	struct cmd_legoop_map org_table;
	int i;
	int ret;

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	/* translate to candela table: org_table -> table */
	if (org_table.col_size != 5) {
		LCD_ERR(vdd, "invalid osc table col size (%d)\n", org_table.col_size);
		ss_free_cmd_legoop_map(&org_table);
		return -EINVAL;
	}

	table_size = org_table.row_size;
	table = kvzalloc(table_size * sizeof(struct adaptive_mipi_v2_table_element), GFP_KERNEL);
	if (!table) {
		LCD_ERR(vdd, "fail to allocate table\n");
		ss_free_cmd_legoop_map(&org_table);
		return -ENOMEM;
	}


	for (i = 0 ; i < table_size; i++) {
		table[i].rat = org_table.cmds[i][0];
		table[i].band = org_table.cmds[i][1];
		table[i].from_ch = org_table.cmds[i][2];
		table[i].end_ch = org_table.cmds[i][3];

		table[i].osc_idx = org_table.cmds[i][4];

		LCD_DEBUG(vdd, "[%d] %d %d %d %d %d\n", i,
				table[i].rat, table[i].band, table[i].from_ch, table[i].end_ch,
				table[i].osc_idx);
	}

	LCD_INFO(vdd, "%s table_size (%d)\n", keystring, table_size);

	ss_free_cmd_legoop_map(&org_table);

	*ret_table = table;
	*ret_table_size = table_size;

	return 0;
}

static int ss_parse_adaptive_mipi_v2_clk_sel_table(struct device_node *np,
		struct samsung_display_driver_data *vdd,
		struct adaptive_mipi_v2_info *info)
{
	int ret = 0;

	ret = ss_parse_adaptive_mipi_v2_mipi_clk_sel_table(np, vdd,
			&info->mipi_table[BANDWIDTH_10M_IDX],
			&info->mipi_table_size[BANDWIDTH_10M_IDX],
			"samsung,adaptive_mipi_v2_clk_sel_table_10m");
	if (ret) {
		LCD_ERR(vdd, "fail to parse table_10m, ret: %d\n", ret);
		return ret;
	}

	ret = ss_parse_adaptive_mipi_v2_mipi_clk_sel_table(np, vdd,
			&info->mipi_table[BANDWIDTH_20M_IDX],
			&info->mipi_table_size[BANDWIDTH_20M_IDX],
			"samsung,adaptive_mipi_v2_clk_sel_table_20m");
	if (ret) {
		LCD_ERR(vdd, "fail to parse table_20m, ret: %d\n", ret);
		return ret;
	}

	if (vdd->dyn_mipi_clk.osc_support) {
		ret = ss_parse_adaptive_mipi_v2_osc_clk_sel_table(np, vdd,
				&info->osc_table, &info->osc_table_size,
				"samsung,adaptive_mipi_v2_clk_sel_table_osc");
		if (ret) {
			LCD_ERR(vdd, "fail to parse table_osc, ret: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int ss_adaptive_mipi_v2_apply_freq(int mipi_clk_kbps, int osc_clk_khz, void *ctx)
{
	struct samsung_display_driver_data *vdd = (struct samsung_display_driver_data *)ctx;
	struct adaptive_mipi_v2_info *info = &vdd->dyn_mipi_clk.adaptive_mipi_v2_info;
	struct dsi_display *dsi_display = GET_DSI_DISPLAY(vdd);
	struct sde_connector *conn = GET_SDE_CONNECTOR(vdd);
	int osc_idx;
	int mipi_idx;

	if (vdd->dyn_mipi_clk.force_idx >= 0) {
		LCD_INFO(vdd, "test force_idx is on (%d), skip change\n", vdd->dyn_mipi_clk.force_idx);
		return 0;
	}

	if (vdd->dyn_mipi_clk.osc_support) {
		for (osc_idx = 0; osc_idx < info->osc_clocks_size; osc_idx++)
			if (osc_clk_khz == info->osc_clocks_khz[osc_idx])
				break;

		if (osc_idx == info->osc_clocks_size) {
			LCD_ERR(vdd, "invalid osc_clock_khz: %d\n", osc_clk_khz);
			return -EINVAL;
		}

		vdd->dyn_mipi_clk.requested_osc_idx = osc_idx;
		LCD_INFO(vdd, "osc clk_khz: %d, idx: %d\n", osc_clk_khz, osc_idx);
	}

	for (mipi_idx = 0; mipi_idx < info->mipi_clocks_size; mipi_idx++)
		if (mipi_clk_kbps == info->mipi_clocks_kbps[mipi_idx])
			break;

	if (mipi_idx == info->mipi_clocks_size) {
		LCD_ERR(vdd, "invalid mipi_clk_kbps: %d\n", mipi_clk_kbps);
		return -EINVAL;
	}

	vdd->dyn_mipi_clk.requested_clk_idx = mipi_idx;
	vdd->dyn_mipi_clk.requested_clk_rate = mipi_clk_kbps;

	if (dsi_display->panel->panel_mode == DSI_OP_VIDEO_MODE) {
		/* send drm_event */
		if (!conn) {
			LCD_ERR(vdd, "fail to get valid sde_connector\n");
		} else {
			LCD_INFO(vdd, "schedule_work - dyn_clk_work (%d)\n", vdd->dyn_mipi_clk.requested_clk_rate);
			conn->scheduled_dyn_clk_rate = vdd->dyn_mipi_clk.requested_clk_rate;
			schedule_work(&conn->dyn_clk_work.work); /* send drm_event */
		}
	}

	LCD_INFO(vdd, "mipi_clk_kbps: %d, idx: %d\n", mipi_clk_kbps, mipi_idx);

	return 0;
}

static struct adaptive_mipi_v2_adapter_funcs ss_adaptive_mipi_v2_adapter_funcs = {
	.apply_freq = ss_adaptive_mipi_v2_apply_freq,
};

static int ss_init_adaptive_mipi_v2(struct device_node *np, struct samsung_display_driver_data *vdd)
{
	struct adaptive_mipi_v2_info *info = &vdd->dyn_mipi_clk.adaptive_mipi_v2_info;
	struct cmd_legoop_map org_table;
	int i;
	int ret;

	memset(info, 0, sizeof(*info));

	info->ctx = vdd;

	// parse mipi clocks
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table,
			"samsung,adaptive_mipi_v2_mipi_clocks");
	if (ret) {
		LCD_ERR(vdd, "fail to parse mipi clocks, ret: %d\n", ret);
		return -EINVAL;
	}

	info->mipi_clocks_size = org_table.row_size;
	if (info->mipi_clocks_size > MAX_MIPI_FREQ_CNT) {
		LCD_ERR(vdd, "mipi_clocks_size(%d) is over than max (%d)\n",
				info->mipi_clocks_size, MAX_MIPI_FREQ_CNT);
		return -EINVAL;
	}

	for (i = 0; i < org_table.row_size; i++) {
		info->mipi_clocks_kbps[i] = org_table.cmds[i][0];
		LCD_DEBUG(vdd, "mipi clocks[%d] = %d\n", i, info->mipi_clocks_kbps[i]);
	}

	ss_free_cmd_legoop_map(&org_table);

	// parse osc clock table : ex) 96500khz, 94500khz
	if (vdd->dyn_mipi_clk.osc_support) {
		ret = ss_parse_panel_legoop_table(vdd, np, &org_table,
				"samsung,adaptive_mipi_v2_osc_clocks");
		if (ret) {
			LCD_ERR(vdd, "fail to parse osc clocks, ret: %d\n", ret);
			return -EINVAL;
		}

		info->osc_clocks_size = org_table.row_size;
		if (info->osc_clocks_size > MAX_OSC_FREQ_CNT) {
			LCD_ERR(vdd, "osc_clocks_size(%d) is over than max (%d)\n",
					info->osc_clocks_size, MAX_OSC_FREQ_CNT);
			return -EINVAL;
		}

		for (i = 0; i < org_table.row_size; i++) {
			info->osc_clocks_khz[i] = org_table.cmds[i][0];
			LCD_DEBUG(vdd, "osc clocks[%d] = %d\n", i, info->osc_clocks_khz[i]);
		}

		ss_free_cmd_legoop_map(&org_table);
	}

	// parse table_10m, table_20m, and table_osc
	ret = ss_parse_adaptive_mipi_v2_clk_sel_table(np, vdd, info);
	if (ret) {
		LCD_ERR(vdd, "fail to parse clk_sel_table, ret: %d\n", ret);
		return ret;
	}

	info->funcs = &ss_adaptive_mipi_v2_adapter_funcs;

	ret = sdp_init_adaptive_mipi_v2(info);
	if (ret) {
		LCD_ERR(vdd, "fail to init sdp adaptive mipi v2, ret: %d\n", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_SDP */
#endif

int ss_parse_panel_legoop_bool(struct samsung_display_driver_data *vdd,
		struct device_node *np,	char *keystring)
{
	const char *data_org;
	int len = 0;
	int ret = 0;

	data_org = of_get_property(np, keystring, &len);
	if (!data_org || len == 0) {
		LCD_DEBUG(vdd, "no prop (%s)\n", keystring);
		return -ENODEV;
	}

	data_org = ss_wrapper_parse_symbol(vdd, np, data_org, &len, keystring);
	if (len == 1) {
		LCD_DEBUG(vdd, "empty data [%s]\n", keystring);
		return -ENODEV;
	}

	if (!strcmp(data_org, "true") || !strcmp(data_org, "TRUE"))
		ret = true;
	else
		ret = false;

	LCD_INFO(vdd, "keystring: %s , len: %d , data: [%s], bool: %d\n", keystring, len, data_org, ret);

	return ret;
}

/* parse table from string.
 * 1) col size: check first valid data line. skip space and tab.
 * count values. and alaram if meets invalid input.
 * 2) row size: count lines except empty line
 * 3) alocates table memory with col * row size.
 * 4) parse iput items. check prefix, if 0x, handle it as hex, or deci.
 */
/* TBR: free table->cmds */
int ss_parse_panel_legoop_table(struct samsung_display_driver_data *vdd,
		struct device_node *np,	void *tbl, char *keystring)
{
	struct cmd_legoop_map *table = (struct cmd_legoop_map *) tbl;
	const char *data_org;
	char *data, *data_backup = NULL;
	char *data_line, *data_val;
	int  len = 0;
	int col, row;
	unsigned int base;
	int ret = 0;

	data_org = of_get_property(np, keystring, &len);
	if (!data_org || len == 0) {
		LCD_DEBUG(vdd, "no prop (%s)\n", keystring);
		return -ENODEV;
	}

	data_org = ss_wrapper_parse_symbol(vdd, np, data_org, &len, keystring);
	if (len == 1) {
		LCD_DEBUG(vdd, "empty data [%s]\n", keystring);
		return -ENODEV;
	}

	LCD_INFO(vdd, "keystring: %s , len: %d\n", keystring, len);

	/* 1) col size: check first valid data line. skip space and tab.
	 * count values. and alaram if meets invalid input.
	 */

	/* strsep modifies original data.
	 * To keep original data, copy to temprorary data block
	 */
	data = kvzalloc(len, GFP_KERNEL);
	if (!data) {
		LCD_ERR(vdd, "fail to allocate data\n");
		return -ENOMEM;
	}
	strlcpy(data, data_org, len);
	data_backup = data; /* to free data */

	/* skip empty data line */
	while ((data_line = strsep(&data, "\r\n")) != NULL &&
			strlen(data_line) == 0);

	if (!data_line) {
		LCD_ERR(vdd, "no valid data\n");
		return -EINVAL;
	}

	col = 0;
	while (*data_line) {
		/* skip empty characters */
		while (*data_line == CMD_STR_SPACE || *data_line == CMD_STR_TAB)
			++data_line;

		if (*data_line == CMD_STR_NUL)
			break;

		++col;
		++data_line;

		/* skip following valid characters */
		while (!(*data_line == CMD_STR_SPACE || *data_line == CMD_STR_TAB || *data_line == 0))
			++data_line;
	}

	if (col == 0) {
		LCD_ERR(vdd, "no valid input (col=0) [%s]\n", data_org);
		ret = -EINVAL;
		goto err_free_mem;
	}

	table->col_size = col;

	/* 2) row size: count lines except empty line */
	row = 1; /* first valid data_line is already parsed in 1) col_size step */
	while ((data_line = strsep(&data, "\r\n")) != NULL) {
		/* skip empty characters */
		while (*data_line == CMD_STR_SPACE || *data_line == CMD_STR_TAB)
			++data_line;

		if (strlen(data_line) == 0)
			continue;
		++row;
	}
	table->row_size = row;

	kvfree(data_backup);
	data_backup = NULL; /* prevent double free in unexpected case */

	/* 3) alocates table memory with col * row size */
	table->cmds = kvzalloc((sizeof(int *) * table->row_size), GFP_KERNEL);
	if (!table->cmds) {
		LCD_ERR(vdd, "fail to allocate cmds(%d)\n", table->row_size);
		return -ENOMEM;
	}

	for (row = 0 ; row < table->row_size; row++) {
		table->cmds[row] = kvzalloc((sizeof(int) * table->col_size), GFP_KERNEL);
		if (!table->cmds[row]) {
			LCD_ERR(vdd, "fail to allocate cmds[%d](%d)\n", row, table->col_size);
			ret = -ENOMEM;
			goto err_free_mem;
		}
	}

	LCD_INFO(vdd, "[%s] len: %d (%d X %d)\n", keystring, len,
			table->row_size, table->col_size);


	/* 4) parse iput items. check prefix, if 0x, handle it as hex, or deci */

	/* strsep modifies original data.
	 * To keep original data, copy to temprorary data block
	 */
	data = kvzalloc(len, GFP_KERNEL);
	if (!data) {
		LCD_ERR(vdd, "fail to allocate data\n");
		return -ENOMEM;
	}
	strlcpy(data, data_org, len);
	data_backup = data; /* to free data */

	row = 0;
	while ((data_line = strsep(&data, "\r\n")) != NULL) {
		/* skip empty characters */
		while (*data_line == CMD_STR_SPACE || *data_line == CMD_STR_TAB)
			++data_line;

		if (strlen(data_line) == 0)
			continue;

		if (row >= table->row_size) {
			LCD_ERR(vdd, "invalid input: row: %d >= %d, [%s]\n",
					row, table->row_size, data_line);
			ret = -EINVAL;
			goto err_free_mem;
		}

		col = 0;
		/* for ", (-80)" case, skip ' ', '(', ')', and '\t' */
		while ((data_val = strsep(&data_line, " (),\t")) != NULL) {
			if (!*data_val)
				continue;

			if (!strncasecmp(data_val, "0x", 2))
				base = 16;
			else
				base = 10;

			if (col >= table->col_size) {
				LCD_ERR(vdd, "invalid input: col: %d >= %d, [%s][%s]\n",
						col, table->col_size, data_val, data_line);
				ret = -EINVAL;
				goto err_free_mem;
			}

			if (kstrtoint(data_val, base, &table->cmds[row][col++])) {
				LCD_ERR(vdd, "fail to get value: [%s][%s]\n", data_val, data_line);
				ret = -EINVAL;
				goto err_free_mem;
			}
		}

		if (col != table->col_size) {
			LCD_ERR(vdd, "invalid input: col: %d != %d\n",
					col, table->col_size);
			ret = -EINVAL;
			goto err_free_mem;
		}

		++row;
	}

	kvfree(data_backup);

	return 0;

err_free_mem:
	for (row = 0 ; row < table->row_size; row++)
		kvfree(table->cmds[row]);

	kvfree(table->cmds);
	kvfree(data_backup);

	return ret;
}

static int ss_parse_panel_legoop_candela_table(struct samsung_display_driver_data *vdd,
		struct device_node *np,	void *tbl, char *keystring)
{
	struct candela_map_table *table = (struct candela_map_table *) tbl;
	int i, ret = 0;
	struct cmd_legoop_map org_table;

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	/* translate to candela table: org_table -> table */
	table->tab_size = org_table.row_size;

	LCD_INFO(vdd, "[%s] tab_size: %d\n", keystring, table->tab_size);

	/* legop candela table format: <platform> <gm2_wrdisbv> <candela> */
	table->cd_map = kvzalloc((sizeof(struct candela_map_data) * table->tab_size),
			GFP_KERNEL);
	if (!table->cd_map) {
		ret = -ENOMEM;
		goto end;
	}

	for (i = 0; i < table->tab_size; i++) {
		table->cd_map[i].bl_idx = i;
		table->cd_map[i].bl_level = org_table.cmds[i][0];
		table->cd_map[i].wrdisbv = org_table.cmds[i][1];
		table->cd_map[i].cd = org_table.cmds[i][2];
		if (org_table.col_size > 3)
			table->cd_map[i].abc_scale_idx = org_table.cmds[i][3];
	}

	table->min_lv = table->cd_map[0].bl_level;
	table->max_lv = table->cd_map[table->tab_size - 1].bl_level;
	if (table->max_lv > vdd->br_info.common_br.max_bl_level)
		vdd->br_info.common_br.max_bl_level = table->max_lv;

end:
	/* free original legoop table */
	for (i = 0; i < org_table.row_size; i++)
		kvfree(org_table.cmds[i]);
	kvfree(org_table.cmds);

	LCD_INFO(vdd, "tab_size [%d][%d], min/max lv (%d/%d), max_bl_level(%d)\n",
		table->tab_size, org_table.col_size, table->min_lv, table->max_lv, vdd->br_info.common_br.max_bl_level);

	return ret;
}

static int ss_parse_hbm_lux_table(struct samsung_display_driver_data *vdd,
		struct device_node *np,	void *tbl, char *keystring)
{
	struct enter_hbm_ce_table *table = (struct enter_hbm_ce_table *) tbl;
	const __be32 *data;
	int  data_offset, len = 0, i = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_ERR(vdd, "%d, Unable to read table %s\n", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_INFO(vdd, "Success to read table %s\n", keystring);

	if ((len % 2) != 0) {
		LCD_ERR(vdd, "%d, Incorrect table entries for %s",
					__LINE__, keystring);
		return -EINVAL;
	}

	table->size = len / (sizeof(int)*2);
	table->idx = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->idx)
		return -ENOMEM;

	table->lux = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->lux)
		goto error;

	data_offset = 0;
	for (i = 0 ; i < table->size; i++) {
		table->idx[i] = be32_to_cpup(&data[data_offset++]);
		table->lux[i] = be32_to_cpup(&data[data_offset++]);
	}

	return 0;
error:
	kfree(table->idx);

	return -ENOMEM;
}

static int ss_parse_hbm_lux_table_str(struct samsung_display_driver_data *vdd,
		struct device_node *np,	void *tbl, char *keystring)
{
	struct enter_hbm_ce_table *table = (struct enter_hbm_ce_table *) tbl;
	int  data_offset, i = 0, ret = 0;
	struct cmd_legoop_map org_table;

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	table->size = org_table.row_size;

	LCD_INFO(vdd, "[%s] table_size: %d\n", keystring, table->size);

	table->idx = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->idx)
		return -ENOMEM;

	table->lux = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->lux)
		goto error;

	data_offset = 0;
	for (i = 0 ; i < table->size; i++) {
		table->idx[i] = org_table.cmds[i][0];
		table->lux[i] = org_table.cmds[i][1];
	}

	return 0;
error:
	kfree(table->idx);

	return -ENOMEM;
}

static int ss_parse_temperature_table(struct samsung_display_driver_data *vdd,
		struct device_node *np,	void *tbl, char *keystring)
{
	struct temperature_table *table = (struct temperature_table *) tbl;
	const __be32 *data;
	int  data_offset, len = 0, i = 0;

	data = of_get_property(np, keystring, &len);
	if (!data)
		return -EINVAL;
	else
		LCD_INFO(vdd, "Success to read table %s\n", keystring);

	table->size = len / (sizeof(int));

	table->val = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->val)
		goto error;

	data_offset = 0;
	for (i = 0 ; i < table->size; i++) {
		table->val[i] = (int)be32_to_cpup(&data[data_offset++]);
   	}

	return 0;
error:
	kfree(table->val);

	return -ENOMEM;
}

static int ss_parse_temperature_table_str(struct samsung_display_driver_data *vdd,
		struct device_node *np,	void *tbl, char *keystring)
{
	int i = 0, ret = 0;
	struct cmd_legoop_map org_table;
	struct temperature_table *table = (struct temperature_table *) tbl;

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	/* Precondition = Temperature table has only single row (1 x n) */
	table->size = org_table.col_size;

	LCD_INFO(vdd, "[%s] table_size: %d\n", keystring, table->size);

	table->val = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->val)
		goto error;

	for (i = 0 ; i < table->size; i++) {
		table->val[i] = org_table.cmds[0][i];
		LCD_DEBUG(vdd, "table->val[%d] = %d\n", i, table->val[i]);
	}

	return 0;
error:
	kfree(table->val);

	return -ENOMEM;
}
int parse_dt_data(struct samsung_display_driver_data *vdd, struct device_node *np,
		 void *data, size_t size, char *cmd_string, char panel_rev,
		 int (*fnc)(struct samsung_display_driver_data *, struct device_node *, void *, char *))
{
	 char string[PARSE_STRING];
	 int ret = 0;

	 /* Generate string to parsing from DT */
	 snprintf(string, PARSE_STRING, "%s%c", cmd_string, 'A' + panel_rev);

	 ret = fnc(vdd, np, data, string);

	 /* If there is no dtsi data for panel rev B ~ T,
	  * use valid previous panel rev dtsi data.
	  * TODO: Instead of copying all data from previous panel rev,
	  * copy only the pointer...
	  */
	 if (ret && (panel_rev > 0))
		 memcpy(data, (u8 *) data - size, size);

	 return ret;
}

static void ss_panel_parse_dt_mapping_tables(struct device_node *np,
		 struct samsung_display_driver_data *vdd)
{
	 struct ss_brightness_info *info = &vdd->br_info;
	 int panel_rev;

	 for (panel_rev = 0; panel_rev < SUPPORT_PANEL_REVISION; panel_rev++) {
		 parse_dt_data(vdd, np, &info->candela_map_table[NORMAL][panel_rev],
				 sizeof(struct candela_map_table),
				 "samsung,ss_candela_map_table_rev", panel_rev,
				 ss_parse_panel_legoop_candela_table);

		/* Set defulat brightness level as normal max level */
		 /* If there is a specific default bl level, it should be implemented in the panel init. */
		 vdd->br_info.common_br.bl_level = info->candela_map_table[NORMAL][panel_rev].max_lv;

		 parse_dt_data(vdd, np, &info->candela_map_table[HBM][panel_rev],
				 sizeof(struct candela_map_table),
				 "samsung,ss_hbm_candela_map_table_rev", panel_rev,
				 ss_parse_panel_legoop_candela_table);

		 parse_dt_data(vdd, np, &info->candela_map_table[HMT][panel_rev],
				 sizeof(struct candela_map_table),
				 "samsung,ss_hmt_candela_map_table_rev", panel_rev,
				 ss_parse_panel_legoop_candela_table);

		 parse_dt_data(vdd, np, &info->candela_map_table[AOD][panel_rev],
				 sizeof(struct candela_map_table),
				 "samsung,ss_aod_candela_map_table_rev", panel_rev,
				 ss_parse_panel_legoop_candela_table);

		 parse_dt_data(vdd, np, &info->analog_offset_48hs[panel_rev],
				 sizeof(struct cmd_legoop_map),
				 "samsung,analog_offset_48hs_table_rev", panel_rev,
				 ss_parse_panel_legoop_table);

		 parse_dt_data(vdd, np, &info->analog_offset_60hs[panel_rev],
				 sizeof(struct cmd_legoop_map),
				 "samsung,analog_offset_60hs_table_rev", panel_rev,
				 ss_parse_panel_legoop_table);

		 parse_dt_data(vdd, np, &info->analog_offset_96hs[panel_rev],
				 sizeof(struct cmd_legoop_map),
				 "samsung,analog_offset_96hs_table_rev", panel_rev,
				 ss_parse_panel_legoop_table);

		 parse_dt_data(vdd, np, &info->analog_offset_120hs[panel_rev],
				 sizeof(struct cmd_legoop_map),
				 "samsung,analog_offset_120hs_table_rev", panel_rev,
				 ss_parse_panel_legoop_table);

		 parse_dt_data(vdd, np, &info->manual_aor_120hs[panel_rev],
				 sizeof(struct cmd_legoop_map),
				 "samsung,manual_aor_120hs_table_rev", panel_rev,
				 ss_parse_panel_legoop_table);

		 parse_dt_data(vdd, np, &info->manual_aor_96hs[panel_rev],
				 sizeof(struct cmd_legoop_map),
				 "samsung,manual_aor_96hs_table_rev", panel_rev,
				 ss_parse_panel_legoop_table);

		 parse_dt_data(vdd, np, &info->manual_aor_60hs[panel_rev],
				 sizeof(struct cmd_legoop_map),
				 "samsung,manual_aor_60hs_table_rev", panel_rev,
				 ss_parse_panel_legoop_table);

		 parse_dt_data(vdd, np, &info->manual_aor_48hs[panel_rev],
				 sizeof(struct cmd_legoop_map),
				 "samsung,manual_aor_48hs_table_rev", panel_rev,
				 ss_parse_panel_legoop_table);

	 	parse_dt_data(vdd, np, &info->acl_offset_map_table[panel_rev],
				sizeof(struct cmd_legoop_map),
				"samsung,ss_acl_offset_map_table_rev", panel_rev,
				ss_parse_panel_legoop_table);

		parse_dt_data(vdd, np, &info->irc_offset_map_table[panel_rev],
				sizeof(struct cmd_legoop_map),
				"samsung,ss_irc_offset_map_table_rev", panel_rev,
				ss_parse_panel_legoop_table);
	 }
}

static int ss_panel_parse_disable_vrr_modes(struct device_node *np,
		 struct samsung_display_driver_data *vdd)
{
	 const char *data_org;
	 char *data;
	 char *mode;
	 int fps;
	 bool hs, phs;
	 char *endp;
	 int len;

	 data_org = of_get_property(np, "samsung,disable_vrr_modes", &len);
	 if (!data_org)
		 return 0;

	 data = kvzalloc(len, GFP_KERNEL);
	 if (!data) {
		 LCD_ERR(vdd, "fail to allocate data(len: %d\n", len);
		 return -ENOMEM;
	 }

	/* Parse symbol to use PDF */
	data = ss_wrapper_parse_symbol(vdd, np, data_org, &len, "DISABLE_VRR_MODES");
	if (len == 1) {
		LCD_DEBUG(vdd, "empty data [DISABLE_VRR_MODES]\n");
		return -ENODEV;
	}

	 vdd->disable_vrr_modes_count = 0;
	 while ((mode = strsep(&data, ", ")) != NULL) {
		 if (!*mode)
			 continue;

		 fps = simple_strtol(mode, &endp, 10);
		 if (fps < 0 || !endp || strlen(endp) <= 0) {
			 /* correct format ex) 60HS */
			 LCD_ERR(vdd, "invalid input: [%s]\n", mode);
			 continue;
		 }

		 hs = phs = false;
		 if (!strncasecmp(endp, "HS", strlen(endp)))
			 hs = true;
		 else if (!strncasecmp(endp, "PHS", strlen(endp)))
			 hs = phs = true;

		 vdd->disable_vrr_modes[vdd->disable_vrr_modes_count].fps = fps;
		 vdd->disable_vrr_modes[vdd->disable_vrr_modes_count].hs = hs;
		 vdd->disable_vrr_modes[vdd->disable_vrr_modes_count].phs = phs;
		 vdd->disable_vrr_modes_count++;
		LCD_INFO(vdd, "disable vrr modes: %d%s, cnt:%d\n",
				fps, phs ? "PHS" : hs ? "HS" : "NS", vdd->disable_vrr_modes_count);
	 }

	 return 0;
}

static int ss_panel_parse_dt_resolution(struct device_node *np,
		 struct samsung_display_driver_data *vdd)
{
	 struct device_node *res_root, *child = NULL;
	 u32 data;
	 const char *data_str = NULL;
	 int rc;
	 int count = 0;

	 res_root = of_get_child_by_name(np, "samsung,resolution_info");
	 if (!res_root)
		 return -ENODEV;

	 for_each_child_of_node(res_root, child)
		 count++;
	 vdd->res = kvcalloc(count, sizeof(struct resolution_info), GFP_KERNEL);
	 if (!vdd->res) {
		 LCD_ERR(vdd, "fail to alloc res(%lu)\n",
				 count * sizeof(struct resolution_info));
		 return -ENOMEM;

	 }
	 vdd->res_count = count;

	 count = 0;
	 for_each_child_of_node(res_root, child) {
		 rc = of_property_read_u32(child, "samsung,panel_width", &data);
		 vdd->res[count].width = !rc ? data : 0;

		 rc = of_property_read_u32(child, "samsung,panel_height", &data);
		 vdd->res[count].height = !rc ? data : 0;

		 rc = of_property_read_string(child, "samsung,panel_resolution_name", &data_str);
		 if (!rc)
			 strlcpy(vdd->res[count].name, data_str, sizeof(vdd->res[count].name));

		 LCD_INFO(vdd, "[%d] [%s] %dx%d\n", count, vdd->res[count].name,
				 vdd->res[count].width, vdd->res[count].height);

		 count++;
	 }

	 return 0;
}

/*
 * parse flash table for the panel using GM2.
 * devide node ? : address could be not consecutive.
 * read/classify data ? : read/classify raw data in each panel functions.
 */
static int ss_panel_parse_dt_gm2_flash_table(struct device_node *np,
		 struct samsung_display_driver_data *vdd)
{
	 struct device_node *root_node = NULL;
	 struct device_node *node = NULL;
	 struct gm2_flash_table *gm2_flash_tbl;
	 int count = 0;
	 int rc, data;

	 root_node = of_get_child_by_name(np, "samsung,gm2_flash_table");
	 if (!root_node) {
		 /* Depends on DDI, some DDI (ex: HAD) doesn't support
		  * samsung,gm2_flash_table.
		  */
		 LCD_INFO(vdd, "no gm2_flash_table root node\n");
		 return 0;
	 }

	 /* count sub table */
	 for_each_child_of_node(root_node, node)
		 count++;

	 vdd->br_info.gm2_flash_tbl_cnt = count;
	 LCD_INFO(vdd, "total gm2 flash node count : %d\n", count);

	 gm2_flash_tbl = kzalloc((sizeof(struct gm2_flash_table) * count), GFP_KERNEL);
	 if (!gm2_flash_tbl)
		 return -ENOMEM;

	 vdd->br_info.gm2_flash_tbl = gm2_flash_tbl;

	 count = 0;
	 for_each_child_of_node(root_node, node) {

		 gm2_flash_tbl[count].idx = count;

		 rc = of_property_read_u32(node, "samsung,gm2_flash_read_start", &data);
		 if (!rc)
			 gm2_flash_tbl[count].start_addr = data;
		 rc = of_property_read_u32(node, "samsung,gm2_flash_read_end", &data);
		 if (!rc)
			 gm2_flash_tbl[count].end_addr = data;

		 gm2_flash_tbl[count].buf_len =
			 gm2_flash_tbl[count].end_addr - gm2_flash_tbl[count].start_addr + 1;

		 if (gm2_flash_tbl[count].buf_len <= 0) {
			 LCD_ERR(vdd, "wrong address is implemented.. please check!!\n");
		 } else {
			 gm2_flash_tbl[count].raw_buf = kzalloc(gm2_flash_tbl[count].buf_len, GFP_KERNEL | GFP_DMA);
			 if (!gm2_flash_tbl[count].raw_buf) {
				 LCD_ERR(vdd, "fail to alloc raw_buf\n");
				 return -ENOMEM;
			 }
		 }

		 LCD_INFO(vdd, "GM2 FLASH [%d] - addr : 0x%x ~ 0x%x (len %d)\n", count,
			 gm2_flash_tbl[count].start_addr, gm2_flash_tbl[count].end_addr, gm2_flash_tbl[count].buf_len);

		 count++;
	 }

	 return 0;
}

static int ss_parse_mafpc_dt(struct samsung_display_driver_data *vdd)
{
	int i = 0, ret = 0;
	struct device_node *np;
	struct cmd_legoop_map org_table;

	np = ss_get_mafpc_of(vdd);
	if (!np) {
		LCD_INFO(vdd, "No mAFPC np..\n");
		return -ENODEV;
	}

	ret = ss_parse_panel_legoop_bool(vdd, np, "samsung,support_mafpc");
	if (ret > 0)
		vdd->mafpc.is_support = true;
	else
		vdd->mafpc.is_support = false;

	LCD_INFO(vdd, "vdd->mafpc.is_support = %d\n", vdd->mafpc.is_support);

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, "samsung,mafpc_crc_pass_data");
	if (ret)
		return ret;

	/* Precondition = MAFPC(ABC) CRC Check pass value has only single row (1 x n) */
	for (i = 0 ; i < org_table.col_size && i < sizeof(vdd->mafpc.crc_pass_data); i++)
		vdd->mafpc.crc_pass_data[i] = org_table.cmds[0][i];

	vdd->mafpc.crc_size = sizeof(vdd->mafpc.crc_pass_data);

	LCD_INFO(vdd, "MAFPC(ABC) Check Pass Value: 0x%02X 0x%02X\n",
			vdd->mafpc.crc_pass_data[0], vdd->mafpc.crc_pass_data[1]);

	/* parse ABC cmd hdr size */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, "samsung,mafpc_cmd_header_size");
	if (ret)
		return ret;
	vdd->mafpc.header_cmd_size = org_table.cmds[0][0];

	/* parse ABC enable cmd size */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, "samsung,mafpc_cmd_enable_size");
	if (ret)
		return ret;
	vdd->mafpc.enable_cmd_size = org_table.cmds[0][0];

	LCD_INFO(vdd, "MAFPC(ABC) enable cmd : header (%d) / enable (%d)\n",
			vdd->mafpc.header_cmd_size, vdd->mafpc.enable_cmd_size);

	/* parse ABC default scale table */
	ret = ss_parse_panel_legoop_table(vdd, np, &vdd->mafpc_scale_table, "samsung,mafpc_scale_table");
	if (ret)
		LCD_ERR(vdd, "Failed to parse mafpc_scale_table data\n");
	else
		LCD_INFO(vdd, "Parsing mafpc_scale_table\n");

	return 0;

}
static int ss_parse_self_disp_dt(struct samsung_display_driver_data *vdd)
{
	int i = 0, ret = 0;
	struct device_node *np;
	struct cmd_legoop_map org_table;

	np = ss_get_self_disp_of(vdd);
	if (!np) {
		LCD_INFO(vdd, "No self display np\n");
		return -ENODEV;
	}

	/* support self_disp */
	ret = ss_parse_panel_legoop_bool(vdd, np, "samsung,support_self_display");
	if (ret > 0)
		vdd->self_disp.is_support = true;
	else
		vdd->self_disp.is_support = false;

	LCD_INFO(vdd, "vdd->self_disp.is_support = %d\n", vdd->self_disp.is_support);

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, "samsung,mask_crc_pass_data");
	if (ret)
		return ret;

	/* Precondition = SelfMask Check pass value has only single row (1 x n) */
	for (i = 0 ; i < org_table.col_size && i < sizeof(vdd->self_disp.mask_crc_pass_data); i++)
		vdd->self_disp.mask_crc_pass_data[i] = org_table.cmds[0][i];

	vdd->self_disp.mask_crc_size = sizeof(vdd->self_disp.mask_crc_pass_data);

	LCD_INFO(vdd, "SelfMask Check Pass Value: 0x%02X 0x%02X\n",
			vdd->self_disp.mask_crc_pass_data[0], vdd->self_disp.mask_crc_pass_data[1]);

	return 0;
}

static int ss_parse_gct_pass_val_str(struct device_node *np,
		struct samsung_display_driver_data *vdd, char *keystring)
{
	int i = 0, ret = 0;
	struct cmd_legoop_map org_table;

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	/* Precondition = GCT pass value has only single row (1 x n) */
	for (i = 0 ; i < org_table.col_size && i < sizeof(vdd->gct.valid_checksum); i++)
		vdd->gct.valid_checksum[i] = org_table.cmds[0][i];

	LCD_INFO(vdd, "GCT Pass Value: 0x%02X 0x%02X 0x%02X 0x%02X\n",
			vdd->gct.valid_checksum[0], vdd->gct.valid_checksum[1],
			vdd->gct.valid_checksum[2], vdd->gct.valid_checksum[3]);
	return 0;
}

static int ss_parse_test_pass_val(struct device_node *np,
		struct samsung_display_driver_data *vdd, char *keystring, struct ddi_test_mode *test_mode)
{
	int i = 0, ret = 0, len = 0;
	struct cmd_legoop_map org_table;
	char pBuf[256];

	memset(pBuf, 0x00, 256);

	/* get original table data */
	ret = ss_parse_panel_legoop_table(vdd, np, &org_table, keystring);
	if (ret)
		return ret;

	if (org_table.col_size) {
		test_mode->pass_val_size = org_table.col_size;
		test_mode->pass_val = (u8 *)kzalloc(sizeof(u8) * org_table.col_size, GFP_KERNEL);
		if (test_mode->pass_val == NULL) {
			LCD_ERR(vdd, "fail to alloc..\n");
			return -ENOMEM;
		}

		/* Precondition = flash test pass value has only single row (1 x n) */
		for (i = 0 ; i < test_mode->pass_val_size ; i++) {
			test_mode->pass_val[i] = (u8)org_table.cmds[0][i];
			len += snprintf(pBuf + len, 256 - len, " 0x%02X", test_mode->pass_val[i]);
		}

		LCD_INFO(vdd, "[%s] pass val[%d]:%s\n", keystring, org_table.col_size, pBuf);
	} else {
		LCD_ERR(vdd, "[%s] no pass_val\n", keystring);
	}

	return 0;
}

static void ss_parse_test_mode_dt(struct samsung_display_driver_data *vdd)
{
	struct device_node *np;
	u32 tmp[3];
	int rc;

	np = ss_get_panel_of(vdd);
	if (!np) {
		LCD_INFO(vdd, "No test_mode np..\n");
		return;
	}

	/* Gram Checksum Test */
	vdd->gct.is_support = of_property_read_bool(np, "samsung,support_gct");
	LCD_INFO(vdd, "vdd->gct.is_support = %d\n", vdd->gct.is_support);

	/* GCT pass Value */
	if (vdd->gct.is_support) {
		rc = ss_parse_gct_pass_val_str(np, vdd, "samsung,gct_pass_val");
		if (rc)
			LCD_ERR(vdd, "fail to parse gct_pass_val, rc: %d\n", rc);
	}

	/* ccd success,fail value */
	rc = of_property_read_u32(np, "samsung,ccd_pass_val", tmp);
	vdd->ccd_pass_val = (!rc ? tmp[0] : 0);
	rc = of_property_read_u32(np, "samsung,ccd_fail_val", tmp);
	vdd->ccd_fail_val = (!rc ? tmp[0] : 0);

	LCD_INFO(vdd, "CCD pass/fail value [%02x]/[%02x] \n", vdd->ccd_pass_val, vdd->ccd_fail_val);

	ss_parse_test_pass_val(np, vdd, "samsung,dsc_crc_pass_val", &vdd->ddi_test[DDI_TEST_DSC_CRC]);
	ss_parse_test_pass_val(np, vdd, "samsung,ecc_pass_val", &vdd->ddi_test[DDI_TEST_ECC]);
	ss_parse_test_pass_val(np, vdd, "samsung,ssr_pass_val", &vdd->ddi_test[DDI_TEST_SSR]);
	ss_parse_test_pass_val(np, vdd, "samsung,flash_test_pass_val", &vdd->ddi_test[DDI_TEST_FLASH_TEST]);
}

static int ss_dsi_panel_parse_cmd_sets(struct samsung_display_driver_data *vdd)
{
	struct ss_cmd_set *cmd_sets = vdd->dtsi_data.ss_cmd_sets;
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	int rc = 0;
	u32 i;
	struct dsi_parser_utils *utils;
	int ss_cmd_type;
	struct ss_cmd_set *ss_set;
	struct dsi_panel_cmd_set *qc_set;
	char *cmd_name;

	for (i = SS_DSI_CMD_SET_START; i < SS_DSI_CMD_SET_MAX ; i++) {
		cmd_name = ss_cmd_set_prop_map[i - SS_DSI_CMD_SET_START];
		ss_cmd_type = i - SS_DSI_CMD_SET_START;

		ss_set = &cmd_sets[ss_cmd_type];
		ss_set->ss_type = i;
		ss_set->count = 0;

		qc_set = &ss_set->base;
		qc_set->ss_cmd_type = i;
		qc_set->count = 0;

		/* Self display has different device node */
		if (i >= TX_SELF_DISP_CMD_START && i <= TX_SELF_DISP_CMD_END)
			utils = &panel->self_display_utils;
		/* mafpc has different device node */
		else if (i >= TX_MAFPC_CMD_START && i <= TX_MAFPC_CMD_END)
			utils = &panel->mafpc_utils;
		else
			utils = &panel->utils;

		LCD_INFO_IF(vdd, "parse ss cmd: [%s]\n", cmd_name);
		rc = ss_wrapper_parse_cmd_sets(vdd, ss_set, i, utils->data, NULL, cmd_name, true);
		if (rc)
			continue;

		rc = __ss_parse_ss_cmd_sets_revision(vdd, ss_set, i, utils->data, NULL, cmd_name, true);
		if (rc)
			pr_err("failed to parse subrev set %d, rc: %d\n", i, rc);
	}

	return rc;
}

__visible_for_testing int ss_panel_parse_dt_esd(struct device_node *np,
		struct samsung_display_driver_data *vdd)
{
	int rc = 0;
	const char *data = NULL;
	char esd_irq_gpio[] = "samsung,esd-irq-gpio1";
	char esd_irqflag[] = "samsung,esd-irq-trigger1";
	struct esd_recovery *esd = NULL;
	struct dsi_panel *panel = NULL;
	struct drm_panel_esd_config *esd_config = NULL;
	int ret = 0;
	u8 i = 0;

	panel = (struct dsi_panel *)vdd->msm_private;
	esd_config = &panel->esd_config;

	esd = &vdd->esd_recovery;
	esd->num_of_gpio = 0;

	esd_config->esd_enabled = of_property_read_bool(np,
		"qcom,esd-check-enabled");

	if (!esd_config->esd_enabled) {
		ret = -EPERM;
		goto end;
	}

	for (i = 0; i < MAX_ESD_GPIO; i++) {
		esd_irq_gpio[strlen(esd_irq_gpio) - 1] = '1' + i;
		esd->esd_gpio[esd->num_of_gpio] = ss_wrapper_of_get_named_gpio(np, esd_irq_gpio, 0);

		if (ss_gpio_is_valid(esd->esd_gpio[esd->num_of_gpio])) {
			LCD_INFO(vdd, "[ESD] gpio : %d, irq : %d\n",
					esd->esd_gpio[esd->num_of_gpio],
					ss_gpio_to_irq(esd->esd_gpio[esd->num_of_gpio]));
			esd->num_of_gpio++;
		}
	}

	if (vdd->is_factory_mode && ss_gpio_is_valid(vdd->fg_err_gpio)) {
		esd->esd_gpio[esd->num_of_gpio] = vdd->fg_err_gpio;
		esd->num_of_gpio++;
		LCD_INFO(vdd, "add fg_err esd gpio(%d), num_of_gpio: %d\n",
				vdd->fg_err_gpio, esd->num_of_gpio);
	}

	if (esd->num_of_gpio == 0) {
		LCD_ERR(vdd, "fail to find valid esd gpio\n");
		ret = -EINVAL;
		goto end;
	}

	rc = of_property_read_string(np, "qcom,mdss-dsi-panel-status-check-mode", &data);
	if (rc == 0 && !strcmp(data, "irq_check"))
		esd_config->status_mode = ESD_MODE_PANEL_IRQ;
	else
		LCD_ERR(vdd, "invalid panel-status-check-mode string(rc: %d)\n", rc);

	for (i = 0; i < esd->num_of_gpio; i++) {
		/* Without ONESHOT setting, it fails in request_threaded_irq()
		 * with 'handler=NULL' error log
		 */
		esd->irqflags[i] = IRQF_ONESHOT | IRQF_NO_SUSPEND; /* default setting */

		esd_irqflag[strlen(esd_irqflag) - 1] = '1' + i;
		rc = of_property_read_string(np, esd_irqflag, &data);
		if (!rc) {
			if (!strcmp(data, "rising"))
				esd->irqflags[i] |= IRQF_TRIGGER_RISING;
			else if (!strcmp(data, "falling"))
				esd->irqflags[i] |= IRQF_TRIGGER_FALLING;
			else if (!strcmp(data, "high"))
				esd->irqflags[i] |= IRQF_TRIGGER_HIGH;
			else if (!strcmp(data, "low"))
				esd->irqflags[i] |= IRQF_TRIGGER_LOW;
		} else {
			LCD_ERR(vdd, "[%d] fail to find ESD irq flag (gpio%d). set default as FALLING..\n",
					i, esd->esd_gpio[i]);
			esd->irqflags[i] |= IRQF_TRIGGER_FALLING;
		}
	}

end:
	LCD_INFO(vdd, "samsung esd %s mode (%d)\n",
		esd_config->esd_enabled ? "enabled" : "disabled",
		esd_config->status_mode);
	return ret;
}

static void ss_panel_parse_dt_ub_con(struct device_node *np,
		struct samsung_display_driver_data *vdd)
{
	struct ub_con_detect *ub_con_det;
	struct esd_recovery *esd = NULL;
	int ret, i;

	ub_con_det = &vdd->ub_con_det;

	ub_con_det->gpio = ss_wrapper_of_get_named_gpio(np,
			"samsung,ub-con-det", 0);

	/* Skip panel power off in user binary */
	ub_con_det->ub_con_ignore_user  = of_property_read_bool(np, "samsung,ub_con_ignore_user");
	LCD_INFO(vdd, "ub_con_ignore_user = %d\n", ub_con_det->ub_con_ignore_user);

	esd = &vdd->esd_recovery;
	if (vdd->is_factory_mode) {
		for (i = 0; i < esd->num_of_gpio; i++) {
			if (esd->esd_gpio[i] == ub_con_det->gpio) {
				LCD_INFO(vdd, "[ub_con_det] gpio : %d share esd gpio : %d\n",
					ub_con_det->gpio, esd->esd_gpio[i]);
				return;
			}
		}
	}

	if (ss_gpio_is_valid(ub_con_det->gpio)) {
		gpio_request(ub_con_det->gpio, "UB_CON_DET");
		LCD_INFO(vdd, "[ub_con_det] gpio : %d, irq : %d status : %d\n",
				ub_con_det->gpio, ss_gpio_to_irq(ub_con_det->gpio),
				ss_gpio_get_value(vdd, ub_con_det->gpio));
	} else {
		LCD_ERR(vdd, "ub-con-det gpio is not valid (%d) \n", ub_con_det->gpio);
		return;
	}

	ret = request_threaded_irq(
				ss_gpio_to_irq(ub_con_det->gpio),
				NULL,
				ss_ub_con_det_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"UB_CON_DET",
				(void *) vdd);
	if (ret)
		LCD_ERR(vdd, "Failed to request_irq, ret=%d\n", ret);
	else
		ub_con_det->ub_con_det_irq_enable = ss_ub_con_det_irq_enable;

	return;
}

void ss_panel_parse_dt(struct samsung_display_driver_data *vdd)
{
	int rc, i;
	u32 tmp[2];
	char backlight_tft_gpio[] = "samsung,panel-backlight-tft-gpio1";
	const __be32 *data_32;
	struct device_node *np;
	struct dsi_panel *panel;
	struct cmd_legoop_map org_table;

	np = ss_get_panel_of(vdd);
	if (!np) {
		LCD_ERR(vdd, "No panel np\n");
		return;
	}

	panel = GET_DSI_PANEL(vdd);

	/* Esync is the key to distinguish between Video & Video Hybrid Mode */
	vdd->esync_support = of_property_read_bool(np, "qcom,mdss-esync");
	LCD_INFO(vdd, "ESYNC support %s\n",
		vdd->esync_support ? "enabled" : "disabled");

	/* Set LP11 init flag */
	vdd->dtsi_data.samsung_lp11_init = of_property_read_bool(np, "samsung,dsi-lp11-init");
	LCD_INFO(vdd, "LP11 init %s\n",
		vdd->dtsi_data.samsung_lp11_init ? "enabled" : "disabled");

	rc = of_property_read_u32(np, "samsung,mdss-power-on-reset-delay-us", tmp);
	vdd->dtsi_data.samsung_power_on_reset_delay = (!rc ? tmp[0] : 0);

	/* Set esc clk 128M */
	vdd->dtsi_data.samsung_esc_clk_128M = of_property_read_bool(np, "samsung,esc-clk-128M");
	LCD_INFO(vdd, "ESC CLK 128M %s\n",
		vdd->dtsi_data.samsung_esc_clk_128M ? "enabled" : "disabled");

	vdd->panel_lpm.is_support = of_property_read_bool(np, "samsung,support_lpm");
	LCD_INFO(vdd, "alpm enable %s\n", vdd->panel_lpm.is_support? "enabled" : "disabled");

	vdd->skip_read_on_pre = of_property_read_bool(np, "samsung,skip_read_on_pre");
	LCD_INFO(vdd, "Skip read on pre %s\n", vdd->skip_read_on_pre ? "enabled" : "disabled");

	/* Set HMT flag */
	vdd->hmt.is_support = of_property_read_bool(np, "samsung,support_hmt");
	LCD_INFO(vdd, "support_hmt %s\n", vdd->hmt.is_support ? "enabled" : "disabled");

	/* TCON Clk On Support */
	vdd->dtsi_data.samsung_tcon_clk_on_support =
		of_property_read_bool(np, "samsung,tcon-clk-on-support");
	LCD_INFO(vdd, "tcon clk on support: %s\n",
			vdd->dtsi_data.samsung_tcon_clk_on_support ?
			"enabled" : "disabled");

	/* Set unicast flags for whole mipi cmds for dual DSI panel
	 * If display->ctrl_count is 2, it broadcasts.
	 * To prevent to send mipi cmd thru mipi dsi1, set unicast flag
	 */
	vdd->dtsi_data.samsung_cmds_unicast =
		of_property_read_bool(np, "samsung,cmds-unicast");
	LCD_INFO(vdd, "mipi cmds unicast flag: %d\n",
			vdd->dtsi_data.samsung_cmds_unicast);

	/* Set TFT flag */
	vdd->mdnie.tuning_enable_tft = of_property_read_bool(np,
				"samsung,mdnie-tuning-enable-tft");
	vdd->dtsi_data.tft_common_support  = of_property_read_bool(np,
		"samsung,tft-common-support");

	LCD_INFO(vdd, "tft_common_support %s\n",
	vdd->dtsi_data.tft_common_support ? "enabled" : "disabled");

	vdd->dtsi_data.tft_module_name = of_get_property(np,
		"samsung,tft-module-name", NULL);  /* for tft tablet */

	vdd->dtsi_data.panel_vendor = of_get_property(np,
		"samsung,panel-vendor", NULL);

	vdd->dtsi_data.disp_model = of_get_property(np,
		"samsung,disp-model", NULL);

	vdd->dtsi_data.backlight_gpio_config = of_property_read_bool(np,
		"samsung,backlight-gpio-config");

	LCD_INFO(vdd, "backlight_gpio_config %s\n",
	vdd->dtsi_data.backlight_gpio_config ? "enabled" : "disabled");

	/* Factory Panel Swap*/
	vdd->dtsi_data.samsung_support_factory_panel_swap = of_property_read_bool(np,
		"samsung,support_factory_panel_swap");

	vdd->support_window_color = of_property_read_bool(np, "samsung,support_window_color");
	LCD_INFO(vdd, "ndx(%d) support_window_color %s\n", vdd->ndx,
		vdd->support_window_color ? "support" : "not support");

	/* Set tft backlight gpio */
	for (i = 0; i < MAX_BACKLIGHT_TFT_GPIO; i++) {
		backlight_tft_gpio[strlen(backlight_tft_gpio) - 1] = '1' + i;
		vdd->dtsi_data.backlight_tft_gpio[i] =
				 ss_wrapper_of_get_named_gpio(np,
						backlight_tft_gpio, 0);
		if (ss_gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[i]))
			LCD_INFO(vdd, "[%d] backlight_tft_gpio num : %d\n", i,
					vdd->dtsi_data.backlight_tft_gpio[i]);
	}
	if (ss_gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[0]))
		ss_backlight_tft_request_gpios(vdd);

	/* Set Mdnie lite HBM_CE_TEXT_MDNIE mode used */
	vdd->dtsi_data.hbm_ce_text_mode_support = of_property_read_bool(np, "samsung,hbm_ce_text_mode_support");

	/* Set Backlight IC discharge time */
	rc = of_property_read_u32(np, "samsung,blic-discharging-delay-us", tmp);
	vdd->dtsi_data.blic_discharging_delay_tft = (!rc ? tmp[0] : 6);

	/* IRC */
	vdd->br_info.common_br.support_irc = of_property_read_bool(np, "samsung,support_irc");

	/* Gamma Mode 2 */
	vdd->br_info.common_br.gamma_mode2_support = of_property_read_bool(np, "samsung,support_gamma_mode2");
	LCD_INFO(vdd, "vdd->br.gamma_mode2_support = %d\n", vdd->br_info.common_br.gamma_mode2_support);

	if (vdd->br_info.common_br.gamma_mode2_support) {
		rc = of_property_read_u32(np, "samsung,flash_vbias_start_addr", tmp);
		vdd->br_info.gm2_table.ddi_flash_start_addr = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_tot_len", tmp);
		vdd->br_info.gm2_table.ddi_flash_raw_len = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_ns_temp_warm_offset", tmp);
		vdd->br_info.gm2_table.off_ns_temp_warm = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_ns_temp_cool_offset", tmp);
		vdd->br_info.gm2_table.off_ns_temp_cool = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_ns_temp_cold_offset", tmp);
		vdd->br_info.gm2_table.off_ns_temp_cold = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_hs_temp_warm_offset", tmp);
		vdd->br_info.gm2_table.off_hs_temp_warm = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_hs_temp_cool_offset", tmp);
		vdd->br_info.gm2_table.off_hs_temp_cool = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_hs_temp_cold_offset", tmp);
		vdd->br_info.gm2_table.off_hs_temp_cold = (!rc ? tmp[0] : 0);

		rc = of_property_read_u32(np, "samsung,flash_vbias_checksum_offset", tmp);
		vdd->br_info.gm2_table.off_gm2_flash_checksum = (!rc ? tmp[0] : 0);

		LCD_INFO(vdd, "flash: start: %x, tot len: %d, mode off={%x %x %x %x %x %x, checksum: %x}\n",
				vdd->br_info.gm2_table.ddi_flash_start_addr,
				vdd->br_info.gm2_table.ddi_flash_raw_len,
				vdd->br_info.gm2_table.off_ns_temp_warm,
				vdd->br_info.gm2_table.off_ns_temp_cool,
				vdd->br_info.gm2_table.off_ns_temp_cold,
				vdd->br_info.gm2_table.off_hs_temp_warm,
				vdd->br_info.gm2_table.off_hs_temp_cool,
				vdd->br_info.gm2_table.off_hs_temp_cold,
				vdd->br_info.gm2_table.off_gm2_flash_checksum);

		ss_panel_parse_dt_gm2_flash_table(np, vdd);
	}

	/* Smooth Dimming */
	vdd->br_info.common_br.ap_driven_smooth_dim = of_property_read_bool(np, "samsung,brightness_ap_driven_smooth_dim");
	rc = ss_parse_panel_legoop_table(vdd, np, &org_table, "samsung,brightness_smooth_ap_dimming_frame");
	vdd->br_info.common_br.smooth_dim_ap_frame_cnt = (!rc ? org_table.cmds[0][0] : 0);
	rc = ss_parse_panel_legoop_table(vdd, np, &org_table, "samsung,brightness_smooth_ddi_dimming_frame");
	vdd->br_info.common_br.smooth_dim_ddi_frame_cnt = (!rc ? org_table.cmds[0][0] : 0);
	LCD_INFO(vdd, "ap_driven_smooth_dim = %s, smooth_dim_ap_frame_cnt=%d smooth_dim_ddi_frame_cnt=%d\n",
		vdd->br_info.common_br.ap_driven_smooth_dim ? "Enable" : "Disable",
		vdd->br_info.common_br.smooth_dim_ap_frame_cnt,
		vdd->br_info.common_br.smooth_dim_ddi_frame_cnt);

	/* Global Para */
	vdd->gpara = of_property_read_bool(np, "samsung,support_gpara");
	LCD_INFO(vdd, "vdd->support_gpara = %d\n", vdd->gpara);
	vdd->pointing_gpara = of_property_read_bool(np, "samsung,pointing_gpara");
	LCD_INFO(vdd, "vdd->pointing_gpara = %d\n", vdd->pointing_gpara);
	vdd->two_byte_gpara = of_property_read_bool(np, "samsung,two_byte_gpara");
	LCD_INFO(vdd, "vdd->two_byte_gpara = %d\n", vdd->two_byte_gpara);

	/* Get temperature table */
	rc = ss_parse_temperature_table_str(vdd, np, &vdd->temp_table, "samsung,temperature_table_str");
	if (rc)
		ss_parse_temperature_table(vdd, np, &vdd->temp_table, "samsung,temperature_table");

	/* support mdnie */
	rc = ss_parse_panel_legoop_bool(vdd, np, "samsung,support_mdnie");
	if (rc > 0)
		vdd->mdnie.support_mdnie = true;
	else
		vdd->mdnie.support_mdnie = false;

	LCD_INFO(vdd, "vdd->mdnie.support_mdnie = %d\n", vdd->mdnie.support_mdnie);

	/* Set lux value for mdnie HBM */
	data_32 = of_get_property(np, "samsung,enter_hbm_ce_lux", NULL);
	if (data_32)
		vdd->mdnie.enter_hbm_ce_lux = (int)(be32_to_cpup(data_32));
	else
		vdd->mdnie.enter_hbm_ce_lux = MDNIE_HBM_CE_LUX;

	ss_parse_hbm_lux_table(vdd, np, &vdd->mdnie.hbm_ce_table, "samsung,enter_hbm_ce_lux_table");
	ss_parse_hbm_lux_table_str(vdd, np, &vdd->mdnie.hbm_ce_table, "samsung,enter_hbm_ce_lux_table_str");

	/* SAMSUNG_FINGERPRINT */
	vdd->support_optical_fingerprint = of_property_read_bool(np, "samsung,support-optical-fingerprint");
	LCD_INFO(vdd, "support_optical_fingerprint %s\n",
		vdd->support_optical_fingerprint ? "enabled" : "disabled");

	vdd->not_support_single_tx = of_property_read_bool(np, "samsung,not-support-single-tx");
	LCD_INFO(vdd, "not_support_single_tx %s\n", vdd->not_support_single_tx ? "enabled" : "disabled");

	/* To reduce DISPLAY ON time */
	vdd->dtsi_data.samsung_reduce_display_on_time = of_property_read_bool(np,"samsung,reduce_display_on_time");
	vdd->dtsi_data.samsung_dsi_force_clock_lane_hs = of_property_read_bool(np,"samsung,dsi_force_clock_lane_hs");
	rc = of_property_read_u32(np, "samsung,wait_after_reset_delay", tmp);
	vdd->dtsi_data.after_reset_delay = (!rc ? (s64)tmp[0] : 0);
	rc = of_property_read_u32(np, "samsung,sleep_out_to_on_delay", tmp);
	vdd->dtsi_data.sleep_out_to_on_delay = (!rc ? (s64)tmp[0] : 0);

	if (vdd->dtsi_data.samsung_reduce_display_on_time) {
		/*
			Please Check interrupt gpio is general purpose gpio
			1. pmic gpio or gpio-expender is not permitted.
			2. gpio should have unique interrupt number.
		*/

		if (ss_gpio_is_valid(ss_wrapper_of_get_named_gpio(np, "samsung,home_key_irq_gpio", 0))) {
				vdd->dtsi_data.samsung_home_key_irq_num =
					ss_gpio_to_irq(ss_wrapper_of_get_named_gpio(np,
								"samsung,home_key_irq_gpio", 0));
				LCD_INFO(vdd, "%s gpio : %d (irq : %d)\n",
					np->name, ss_wrapper_of_get_named_gpio(np, "samsung,home_key_irq_gpio", 0),
					vdd->dtsi_data.samsung_home_key_irq_num);
		}

		if (ss_gpio_is_valid(ss_wrapper_of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0))) {
			vdd->dtsi_data.samsung_finger_print_irq_num =
				ss_gpio_to_irq(ss_wrapper_of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0));
			LCD_INFO(vdd, "%s gpio : %d (irq : %d)\n",
				np->name, ss_wrapper_of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0),
				vdd->dtsi_data.samsung_finger_print_irq_num);
		}
	}

#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE)
	/* Dynamic MIPI Clock */
	vdd->dyn_mipi_clk.is_support = of_property_read_bool(np, "samsung,support_dynamic_mipi_clk");
#if IS_ENABLED(CONFIG_SDP)
	vdd->dyn_mipi_clk.is_adaptive_mipi_v2 = of_property_read_bool(np, "samsung,support_adaptive_mipi_v2");

	LCD_INFO(vdd, "dyn_mipi_clk.is_support = %d, is_adaptive_mipi_v2: %d\n",
			vdd->dyn_mipi_clk.is_support, vdd->dyn_mipi_clk.is_adaptive_mipi_v2);
#endif

	if (vdd->dyn_mipi_clk.is_support) {
		vdd->dyn_mipi_clk.osc_support = of_property_read_bool(np, "samsung,support_dynamic_mipi_clk_osc");

#if IS_ENABLED(CONFIG_SDP)
		if (vdd->dyn_mipi_clk.is_adaptive_mipi_v2) {
			ss_init_adaptive_mipi_v2(np, vdd);
		} else {
#endif
			ss_parse_dyn_mipi_clk_timing_table(np, vdd, "samsung,dynamic_mipi_clk_timing_table_str");
			ss_parse_dyn_mipi_clk_sel_table(np, vdd, "samsung,dynamic_mipi_clk_sel_table_str");

			vdd->dyn_mipi_clk.notifier.priority = 0;
			vdd->dyn_mipi_clk.notifier.notifier_call = ss_rf_info_notify_callback;
			register_dev_ril_bridge_event_notifier(&vdd->dyn_mipi_clk.notifier);
#if IS_ENABLED(CONFIG_SDP)
		}
#endif

		if (of_property_read_bool(np, "samsung,dynamic_mipi_clk_force_dft")) {
			vdd->dyn_mipi_clk.requested_clk_idx = 0;
			vdd->dyn_mipi_clk.requested_clk_rate =
				vdd->dyn_mipi_clk.clk_timing_table.clk_rate[0];
			LCD_INFO(vdd, "force to default dynamic mipi clock(%d)\n",
					vdd->dyn_mipi_clk.requested_clk_rate);
		}
	}
#endif

	ss_panel_parse_dt_mapping_tables(np, vdd);

	ss_panel_parse_disable_vrr_modes(np, vdd);

	rc = ss_parse_panel_legoop_table(vdd, np, &org_table, "samsung,ss_hbm_plus_level");
	vdd->br_info.hbm_plus_level = (!rc ? org_table.cmds[0][0] : 0);

	/* Set the following two properties to the max level of the brightness table (normal/hbm).
	 * - qcom,mdss-dsi-bl-max-level
	 * - qcom,mdss-brightness-max-level
	 */
	if (panel) {
		panel->bl_config.bl_max_level = panel->bl_config.brightness_max_level = vdd->br_info.common_br.max_bl_level;
		LCD_INFO(vdd, "set panel bl default (%d) / max (%d) / hbm+ (%d)\n",
			vdd->br_info.common_br.bl_level, panel->bl_config.bl_max_level, vdd->br_info.hbm_plus_level);
	}

	vdd->ss_cmd_dsc_long_write = of_property_read_bool(np, "samsung,ss_cmd_dsc_long_write");
	vdd->ss_cmd_dsc_short_write_param = of_property_read_bool(np, "samsung,ss_cmd_dsc_short_write_param");
	vdd->ss_cmd_always_last_command_set = of_property_read_bool(np, "samsung,ss_cmd_always_last_command_set");
	vdd->allow_level_key_once = of_property_read_bool(np, "samsung,allow_level_key_once");
	LCD_INFO(vdd, "ss_cmd_dsc_long_write: %d, "\
			"ss_cmd_dsc_short_write_param: %d, "\
			"ss_cmd_always_last_command_set: %d, "\
			"allow_level_key_once: %d\n",
			vdd->ss_cmd_dsc_long_write,
			vdd->ss_cmd_dsc_short_write_param,
			vdd->ss_cmd_always_last_command_set,
			vdd->allow_level_key_once);

	ss_dsi_panel_parse_cmd_sets(vdd);
	ss_prepare_otp(vdd);

	/* pll ssc */
	vdd_pll_ssc_disabled = of_property_read_bool(np, "samsung,pll_ssc_disabled");
	LCD_INFO(vdd, "vdd->pll_ssc_disabled = %d\n", vdd_pll_ssc_disabled);

	// LCD SELECT
	vdd->select_panel_gpio = ss_wrapper_of_get_named_gpio(np,
			"samsung,mdss_dsi_lcd_sel_gpio", 0);
	if (ss_gpio_is_valid(vdd->select_panel_gpio))
		gpio_request(vdd->select_panel_gpio, "lcd_sel_gpio");

	vdd->select_panel_use_expander_gpio = of_property_read_bool(np, "samsung,mdss_dsi_lcd_sel_use_expander_gpio");

	/* Panel LPM */
	rc = of_property_read_u32(np, "samsung,lpm_swire_delay", tmp);
	vdd->dtsi_data.lpm_swire_delay = (!rc ? tmp[0] : 0);

	rc = of_property_read_u32(np, "samsung,lpm-init-delay-ms", tmp);
	vdd->dtsi_data.samsung_lpm_init_delay = (!rc ? tmp[0] : 0);

	rc = of_property_read_u32(np, "samsung,delayed-display-on", tmp);
	vdd->dtsi_data.samsung_delayed_display_on = (!rc ? tmp[0] : 0);

	/* FG_ERR */
	if (vdd->is_factory_mode) {
		vdd->fg_err_gpio = ss_wrapper_of_get_named_gpio(np, "samsung,fg-err_gpio", 0);
		if (ss_gpio_is_valid(vdd->fg_err_gpio))
			LCD_INFO(vdd, "FG_ERR gpio: %d\n", vdd->fg_err_gpio);
	} else {
		vdd->fg_err_gpio = -1; /* default 0 is valid gpio... set invalid gpio.. */
	}

	vdd->support_lp_rx_err_recovery = of_property_read_bool(np, "samsung,support_lp_rx_err_recovery");
	LCD_INFO(vdd, "support_lp_rx_err_recovery : %d\n", vdd->support_lp_rx_err_recovery);

	ss_panel_parse_dt_esd(np, vdd);
	ss_panel_parse_dt_ub_con(np, vdd);

	rc = of_property_read_u32(np, "samsung,gamma_size", tmp);
			vdd->br_info.gamma_size = (!rc ? tmp[0] : 34);
	rc = of_property_read_u32(np, "samsung,aor_size", tmp);
			vdd->br_info.aor_size = (!rc ? tmp[0] : 2);
	rc = of_property_read_u32(np, "samsung,vint_size", tmp);
			vdd->br_info.vint_size = (!rc ? tmp[0] : 1);
	rc = of_property_read_u32(np, "samsung,elvss_size", tmp);
			vdd->br_info.elvss_size = (!rc ? tmp[0] : 3);
	rc = of_property_read_u32(np, "samsung,irc_size", tmp);
			vdd->br_info.irc_size = (!rc ? tmp[0] : 17);
	rc = of_property_read_u32(np, "samsung,dbv_size", tmp);
			vdd->br_info.dbv_size = (!rc ? tmp[0] : 17);

	LCD_INFO(vdd, "gamma_size gamma:%d aor:%d vint:%d elvss:%d irc:%d dbv:%d\n",
		vdd->br_info.gamma_size,
		vdd->br_info.aor_size,
		vdd->br_info.vint_size,
		vdd->br_info.elvss_size,
		vdd->br_info.irc_size,
		vdd->br_info.dbv_size);

	vdd->rsc_4_frame_idle = of_property_read_bool(np, "samsung,rsc_4_frame_idle");
	LCD_INFO(vdd, "rsc_4_frame_idle : %d\n", vdd->rsc_4_frame_idle);

	/* VRR: Variable Refresh Rate(MRR) */
	vdd->vrr.support_vrr_based_bl = of_property_read_bool(np, "samsung,support_vrr_based_bl");

	/* DVRR: Dynamic(Discrete) Variable Refresh Rate */
	vdd->dvrr.support_dvrr = of_property_read_bool(np, "samsung,support_dvrr");

	/* Low Frequency Driving mode for HOP display */
	vdd->vrr.lfd.support_lfd = of_property_read_bool(np, "samsung,support_lfd");

	LCD_INFO(vdd, "support: vrr(%d), dvrr(%d), lfd(%d)\n",
			vdd->vrr.support_vrr_based_bl, vdd->dvrr.support_dvrr, vdd->vrr.lfd.support_lfd);

	/* Need exclusive tx for Brightness */
	vdd->need_brightness_lock= of_property_read_bool(np, "samsung,need_brightness_lock");
	LCD_INFO(vdd, "need_brightness_lock : %s\n",
		vdd->need_brightness_lock ? "support brightness_lock" : "Not support brightness_lock");
	/* Not to send QCOM made PPS (A data type) */
	vdd->no_qcom_pps= of_property_read_bool(np, "samsung,no_qcom_pps");
	LCD_INFO(vdd, "no_qcom_pps : %s\n",
		vdd->no_qcom_pps ? "support no_qcom_pps" : "Not support no_qcom_pps");

	/* TDDI, touch notify esd when no esd gpio from ddi */
	vdd->esd_touch_notify = of_property_read_bool(np, "samsung,esd_touch_notify");
	LCD_INFO(vdd, "esd_touch_notify is %s\n",
			vdd->esd_touch_notify ? "enabled" : "Not supported");

	/* Read module ID at probe timing */
	vdd->support_early_id_read = of_property_read_bool(np, "samsung,support_early_id_read");
	LCD_INFO(vdd, "support_early_id_read : %s\n",
		vdd->support_early_id_read ? "support" : "doesn't support");

	/*
		panel dsi/csi mipi strength value
	*/
	for (i = 0; i < SS_PHY_CMN_MAX; i++) {
		clear_bit(i, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[i] = 0;
	}

	if (!of_property_read_u32(np, "samsung,phy_vreg_ctrl_0", tmp)) {
		set_bit(SS_PHY_CMN_VREG_CTRL_0, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_VREG_CTRL_0] = tmp[0];
		LCD_INFO(vdd, "phy_vreg_ctrl_0 : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_VREG_CTRL_0]);
	}

	if (!of_property_read_u32(np, "samsung,phy_ctrl2", tmp)) {
		set_bit(SS_PHY_CMN_CTRL_2, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_CTRL_2] = tmp[0];
		LCD_INFO(vdd, "phy_ctrl2 : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_CTRL_2]);
	}

	if (!of_property_read_u32(np, "samsung,phy_offset_top_ctrl", tmp)) {
		set_bit(SS_PHY_CMN_GLBL_RESCODE_OFFSET_TOP_CTRL, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_TOP_CTRL] = tmp[0];
		LCD_INFO(vdd, "phy_offset_top_ctrl : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_TOP_CTRL]);
	}

	if (!of_property_read_u32(np, "samsung,phy_offset_bot_ctrl", tmp)) {
		set_bit(SS_PHY_CMN_GLBL_RESCODE_OFFSET_BOT_CTRL, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_BOT_CTRL] = tmp[0];
		LCD_INFO(vdd, "phy_offset_bot_ctrl : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_BOT_CTRL]);
	}

	if (!of_property_read_u32(np, "samsung,phy_offset_mid_ctrl", tmp)) {
		set_bit(SS_PHY_CMN_GLBL_RESCODE_OFFSET_MID_CTRL, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_MID_CTRL] = tmp[0];
		LCD_INFO(vdd, "phy_offset_mid_ctrl : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_RESCODE_OFFSET_MID_CTRL]);
	}

	if (!of_property_read_u32(np, "samsung,phy_str_swi_cal_sel_ctrl", tmp)) {
		set_bit(SS_PHY_CMN_GLBL_STR_SWI_CAL_SEL_CTRL, vdd->ss_phy_ctrl_bit);
		vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_STR_SWI_CAL_SEL_CTRL] = tmp[0];
		LCD_INFO(vdd, "phy_str_swi_cal_sel_ctrl : 0x%x\n", vdd->ss_phy_ctrl_data[SS_PHY_CMN_GLBL_STR_SWI_CAL_SEL_CTRL]);
	}

	/* Send DSI_CMD_SET_ON durning splash booting */
	vdd->cmd_set_on_splash_enabled = of_property_read_bool(np, "samsung,cmd_set_on_splash_enabled");
	LCD_INFO(vdd, "cmd_set_on_splash_enabled [%s]\n",
		vdd->cmd_set_on_splash_enabled ? "enabled" : "disabled");

	/* (mafpc) (self_display) (test_mode) have dependent dtsi file */
	ss_parse_mafpc_dt(vdd);
	ss_parse_self_disp_dt(vdd);
	ss_parse_test_mode_dt(vdd);

	/* UDC */
	rc = of_property_read_u32(np, "samsung,udc_start_addr", tmp);
	vdd->udc.start_addr = (!rc ? tmp[0] : 0);
	rc = of_property_read_u32(np, "samsung,udc_data_size", tmp);
	vdd->udc.size = (!rc ? tmp[0] : 0);
	if (vdd->udc.size) {
		vdd->udc.data = kzalloc(vdd->udc.size, GFP_KERNEL);
		if (!vdd->udc.data)
			LCD_INFO(vdd, "[UDC] fail to alloc udc data buf..\n");
		else
			LCD_INFO(vdd, "[UDC] start_addr : %X, data_size : %d\n", vdd->udc.start_addr, vdd->udc.size);
	}

	/* use recovery when flash loading is failed */
	vdd->use_flash_done_recovery = of_property_read_bool(np, "samsung,use_flash_done_recovery");
	LCD_INFO(vdd, "use_flash_done_recovery [%s]\n", vdd->use_flash_done_recovery ? "enabled" : "disabled");

	/* get resolution name and size */
	ss_panel_parse_dt_resolution(np, vdd);
}
