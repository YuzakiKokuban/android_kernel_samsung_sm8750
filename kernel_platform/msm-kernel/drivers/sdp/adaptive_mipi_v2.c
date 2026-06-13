// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/sdp/adaptive_mipi_v2.h>
#include <linux/sdp/adaptive_mipi_v2_cp_info.h>
#include <linux/sdp/sdp_debug.h>
#include <linux/notifier.h>

static struct adaptive_mipi_v2_table_element *get_matched_table_element(
				struct band_info *band_info,
				struct adaptive_mipi_v2_table_element *table,
				int table_size)
{
	struct adaptive_mipi_v2_table_element *element;
	int i;

	for (i = 0; i < table_size; i++) {
		element = &table[i];
		if (band_info->rat == element->rat &&
				band_info->band == element->band &&
				band_info->channel >= element->from_ch &&
				band_info->channel <= element->end_ch)
			return element;
	}

	return NULL;
}

static int get_optimal_osc_clock(struct cp_info *cp_msg,
				 struct adaptive_mipi_v2_info *info)
{
	struct adaptive_mipi_v2_table_element *element;
	struct band_info *band_msg;
	int i;

	if (info->osc_table_size == 0)
		return INV_OSC_CLK;

	for (i = 0; i < cp_msg->cell_count; i++) {
		band_msg = &cp_msg->infos[i];
		element = get_matched_table_element(band_msg,
						info->osc_table,
						info->osc_table_size);
		if (element)
			return info->osc_clocks_khz[ALTERNATIVE_OSC_ID];
	}

	return info->osc_clocks_khz[DEFAULT_OSC_ID];
}

static int get_status_weight(int connection_status)
{
	if (connection_status == STATUS_PRIMARY_SERVING)
		return WEIGHT_PRIMARY_SERVING;
	else if (connection_status == STATUS_SECONDARY_SERVING)
		return WEIGHT_SECONDARY_SERVING;

	sdp_err(NULL, "invalid connection_status: %d\n",
				connection_status);

	return -EINVAL;
}

static int get_signal_weight(int sinr)
{
	if (sinr == DEFAULT_WEAK_SIGNAL)
		return WEIGHT_WEAK_SIGNAL;

	if (sinr < JUDGE_STRONG_SIGNAL)
		return WEIGHT_WEAK_SIGNAL;

	return WEIGHT_STRONG_SIGNAL;
}

static int update_ratings(int connection_status, int sinr,
			  int *ratings, int ratings_size)
{
	int status_weight = get_status_weight(connection_status);
	int signal_weight = get_signal_weight(sinr);
	int i;

	if (status_weight < 0 || signal_weight < 0)
		return -EINVAL;

	for (i = 0; i < ratings_size; i++)
		ratings[i] *= status_weight * signal_weight;

	return 0;
}

static int get_band_id(u32 bandwidth_khz)
{
	return bandwidth_khz <= 10000 ? BANDWIDTH_10M_IDX : BANDWIDTH_20M_IDX;
}

static int get_min_score_clock_id(int *total_score, int score_size)
{
	int min_id = 0;
	int min_score = total_score[0];
	int i;

	for (i = 1; i < score_size; i++) {
		if (min_score > total_score[i]) {
			min_score = total_score[i];
			min_id = i;
		}
	}

	return min_id;
}

static int get_optimal_mipi_clock(struct cp_info *cp_msg,
				  struct adaptive_mipi_v2_info *info)
{
	struct adaptive_mipi_v2_table_element *element;
	struct band_info *band_msg;
	int band_id;
	int ratings[MAX_MIPI_FREQ_CNT] = {0, };
	int total_score[MAX_MIPI_FREQ_CNT] = {0, };
	int min_id;
	int i, j;
	int ret;

	sdp_info(info->ctx, "cell_count: %d\n", cp_msg->cell_count);
	for (i = 0; i < cp_msg->cell_count; i++) {
		band_msg = &cp_msg->infos[i];
		band_id = get_band_id(band_msg->bandwidth);
		element = get_matched_table_element(band_msg,
						info->mipi_table[band_id],
						info->mipi_table_size[band_id]);
		if (!element)
			continue;

		memcpy(ratings, element->mipi_clocks_rating, sizeof(ratings));
		ret = update_ratings(band_msg->connection_status, band_msg->sinr,
					ratings, info->mipi_clocks_size);
		if (ret < 0)
			continue;

		for (j = 0; j < info->mipi_clocks_size; j++)
			total_score[j] += ratings[j];

		sdp_info(info->ctx, "[%d] band_info: {%d,%d,%d,%d,%d,%d}, score: {%d,%d,%d,%d} \n",
				i, band_msg->rat, band_msg->band, band_msg->channel,
				band_msg->connection_status, band_msg->bandwidth, band_msg->sinr,
				total_score[0], total_score[1], total_score[2], total_score[3]);
	}

	min_id = get_min_score_clock_id(total_score, info->mipi_clocks_size);

	return info->mipi_clocks_kbps[min_id];
}

static int sdp_adaptive_mipi_v2_ril_notifier_callback(struct notifier_block *nb,
						unsigned long size, void *buf)
{
	struct adaptive_mipi_v2_info *info =
			container_of(nb, struct adaptive_mipi_v2_info, ril_nb);
	struct dev_ril_bridge_msg *ril_msg = buf;
	struct cp_info *cp_msg;

	int mipi_clk_kbps;
	int osc_clk_khz;

	int ret;

	if (!nb || !ril_msg) {
		sdp_err(NULL, "null arg\n");
		return NOTIFY_BAD;
	}

	if (ril_msg->dev_id != IPC_SYSTEM_CP_ADAPTIVE_MIPI_INFO) {
		sdp_dbg(info->ctx, "unmatched sub cmd: %d\n", ril_msg->dev_id);
		return NOTIFY_DONE;
	}

	cp_msg = (struct cp_info *)ril_msg->data;

	if (ril_msg->data_len < sizeof(cp_msg->cell_count)) {
		return NOTIFY_BAD;
	}

	if (cp_msg->cell_count > MAX_BAND || cp_msg->cell_count <= 0) {
		sdp_err(info->ctx, "invalid cell_count (%d)\n", cp_msg->cell_count);
		return NOTIFY_BAD;
	}

	if (ril_msg->data_len < sizeof(cp_msg->cell_count) + cp_msg->cell_count * sizeof(*cp_msg->infos)) {
		sdp_err(info->ctx, "invalid data_len (%d) - cell_count(%d)\n", ril_msg->data_len, cp_msg->cell_count);
		return NOTIFY_BAD;
	}

	mipi_clk_kbps = get_optimal_mipi_clock(cp_msg, info);
	if (mipi_clk_kbps < 0)
		return NOTIFY_DONE;

	osc_clk_khz = get_optimal_osc_clock(cp_msg, info);

	sdp_info(info->ctx, "mipi_clk_kbps: %d, osc_clk_khz: %d\n", mipi_clk_kbps, osc_clk_khz);

	ret = info->funcs->apply_freq(mipi_clk_kbps, osc_clk_khz, info->ctx);
	if (ret < 0) {
		sdp_err(info->ctx, "fail to set freq: ret=%d\n", ret);
		return NOTIFY_DONE;
	}

	return NOTIFY_DONE;
}

static int validate_info(struct adaptive_mipi_v2_info *info)
{
	int i;

	if (!info) {
		sdp_err(NULL, "null info\n");
		return -ENODEV;
	}

	if (info->mipi_clocks_size == 0 ||
			info->mipi_clocks_size > MAX_MIPI_FREQ_CNT) {
		sdp_err(info->ctx, "invalid mipi_clocks_size (%d)\n",
				info->mipi_clocks_size);
		return -EINVAL;
	}

	if (!info->funcs || !info->funcs->apply_freq) {
		sdp_err(info->ctx, "no callback\n");
		return -EINVAL;
	}

	for (i = 0; i < MAX_BANDWIDTH_IDX; i++) {
		if (!info->mipi_table[i]) {
			sdp_err(info->ctx, "no mipi table[%d]\n", i);
			return -EINVAL;
		}
	}

	return 0;
}

int sdp_init_adaptive_mipi_v2(struct adaptive_mipi_v2_info *info)
{
	int ret;

	if (validate_info(info) < 0)
		return -EINVAL;

	info->ril_nb.notifier_call = sdp_adaptive_mipi_v2_ril_notifier_callback;
	ret = register_dev_ril_bridge_event_notifier(&info->ril_nb);
	if (ret < 0) {
		sdp_err(info->ctx, "failed to register ril notifier(%d)\n", ret);
		return ret;
	}

	sdp_info(info->ctx, "initialized sdp adaptive mipi v2\n");

	return 0;
}
EXPORT_SYMBOL(sdp_init_adaptive_mipi_v2);

int sdp_cleanup_adaptive_mipi_v2(struct adaptive_mipi_v2_info *info)
{
	int ret;

	if (!info) {
		sdp_err(NULL, "null info\n");
		return -ENODEV;
	}

	ret = unregister_dev_ril_bridge_event_notifier(&info->ril_nb);
	if (ret < 0) {
		sdp_err(info->ctx, "failed to unregister ril notifier(%d)\n", ret);
		return ret;
	}

	sdp_info(info->ctx, "cleanup sdp adaptive mipi v2\n");

	return 0;
}
EXPORT_SYMBOL(sdp_cleanup_adaptive_mipi_v2);

MODULE_DESCRIPTION("sdp adaptive mipi v2");
MODULE_LICENSE("GPL");
