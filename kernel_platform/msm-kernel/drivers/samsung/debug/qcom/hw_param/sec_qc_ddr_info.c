// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include <linux/samsung/debug/qcom/sec_qc_smem.h>

#include "sec_qc_hw_param.h"

#define for_each_dq(dq, max_dq) \
	for (dq = 0; dq < max_dq; dq++)

#define for_each_cs(cs, max_cs) \
	for (cs = 0; cs < max_cs; cs++)

#define for_each_cs_dq(cs, max_cs, dq, max_dq) \
	for_each_cs(cs, max_cs) \
		for_each_dq(dq, max_dq)

#define for_each_ch(ch, max_ch) \
	for (ch = 0; ch < max_ch; ch++)

#define for_each_ch_dq(ch, max_ch, dq, max_dq) \
	for_each_ch(ch, max_ch) \
		for_each_dq(dq, max_dq)

#define for_each_ch_cs(ch, max_ch, cs, max_cs) \
	for_each_ch(ch, max_ch) \
		for_each_cs(cs, max_cs)

#define for_each_ch_cs_dq(ch, max_ch, cs, max_cs, dq, max_dq) \
	for_each_ch(ch, max_ch) \
		for_each_cs_dq(cs, max_cs, dq, max_dq)

static ssize_t __ddr_info_report_header(char *buf,
		const ssize_t sz_buf, ssize_t info_size)
{
	union {
		uint32_t raw;
		struct {
			uint32_t lv2:16;
			uint32_t lv1:8;
			uint32_t lv0:8;
		};
	} dsf;

	dsf.raw = sec_qc_smem_lpddr_get_DSF_version();

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"DDRV\":\"%s\",",
			sec_qc_smem_lpddr_get_vendor_name());
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"DSF\":\"%u.%u.%u\",",
			dsf.lv0, dsf.lv1, dsf.lv2);

	return info_size;
}

static ssize_t ddr_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = DEFAULT_LEN_STR;
	size_t ch, cs, dq;

	info_size = __ddr_info_report_header(buf, sz_buf, info_size);

	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RW_%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq, sec_qc_smem_lpddr_get_rcw_tDQSCK(ch, cs, dq));
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"WC_%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq, sec_qc_smem_lpddr_get_wr_coarseCDC(ch, cs, dq));
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"WF_%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq, sec_qc_smem_lpddr_get_wr_fineCDC(ch, cs, dq));
	}

	/* remove the last ',' character */
	info_size--;

	__qc_hw_param_clean_format(buf, &info_size, sz_buf);

	return info_size;
}
DEVICE_ATTR_RO(ddr_info);

static ssize_t __ddr_info_report_revision(char *buf,
		const ssize_t sz_buf, ssize_t info_size)
{
        __qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"REV1\":\"%02x\",",
			sec_qc_smem_lpddr_get_revision_id1());
        __qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"REV2\":\"%02x\",",
			sec_qc_smem_lpddr_get_revision_id2());
        __qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"SIZE\":\"%02x\",",
			sec_qc_smem_lpddr_get_total_density());

	return info_size;
}

static ssize_t __ddr_info_report_eye_rd_info(char *buf,
		const ssize_t sz_buf, ssize_t info_size)
{
	size_t ch, cs, dq;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"CNT\":\"%d\",",
			sec_qc_smem_lpddr_get_small_eye_detected());

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"rd_width\":\"R1\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RW%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq,
				sec_qc_smem_lpddr_get_rd_pr_width(ch, cs, dq));
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"rd_height\":\"R2\",");
	for_each_ch_cs(ch, NUM_CH, cs, NUM_CS) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RH%zu_%zu_x\":\"%d\",",
				ch, cs,
				sec_qc_smem_lpddr_get_rd_min_eye_height(ch, cs));
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"rd_vref\":\"Rx\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RV%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq,
				sec_qc_smem_lpddr_get_rd_best_vref(ch, cs, dq));
	}

	return info_size;
}

static ssize_t eye_rd_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = DEFAULT_LEN_STR;

	info_size = __ddr_info_report_header(buf, sz_buf, info_size);
	info_size = __ddr_info_report_revision(buf, sz_buf, info_size);

	if (!qc_hw_param->use_ddr_eye_info)
		goto early_exit_no_eye_info;

	info_size = __ddr_info_report_eye_rd_info(buf, sz_buf, info_size);

early_exit_no_eye_info:
	/* remove the last ',' character */
	info_size--;

	__qc_hw_param_clean_format(buf, &info_size, sz_buf);

	return info_size;
}
DEVICE_ATTR_RO(eye_rd_info);

/* TODO: only for 'use_ddr_eye_info' */
static ssize_t eye_dcc_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = DEFAULT_LEN_STR;
	size_t ch, cs, dq;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dqs_dcc\":\"D3\",");
	for_each_ch_dq(ch, NUM_CH, dq, NUM_DQ_PCH) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"DS%zu_x_%zu\":\"%d\",",
				ch, dq,
				sec_qc_smem_lpddr_get_dqs_dcc_adj(ch, dq));
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dq_dcc\":\"D5\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"DQ%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq,
				sec_qc_smem_lpddr_get_dq_dcc_abs(ch, cs, dq));
	}

	/* remove the last ',' character */
	info_size--;

	__qc_hw_param_clean_format(buf, &info_size, sz_buf);

	return info_size;
}
DEVICE_ATTR_RO(eye_dcc_info);

/* TODO: only for 'use_ddr_eye_info' */
static ssize_t eye_wr1_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = DEFAULT_LEN_STR;
	size_t ch, cs, dq;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"wr_width\":\"W1\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"WW%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq,
				sec_qc_smem_lpddr_get_wr_pr_width(ch, cs, dq));
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"wr_height\":\"W2\",");
	for_each_ch_cs(ch, NUM_CH, cs, NUM_CS) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"WH%zu_%zu_x\":\"%d\",",
				ch, cs,
				sec_qc_smem_lpddr_get_wr_min_eye_height(ch, cs));
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"wr_vref\":\"Wx\",");
	for_each_ch_cs(ch, NUM_CH, cs, NUM_CS) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"WV%zu_%zu_x\":\"%d\",",
				ch, cs,
				sec_qc_smem_lpddr_get_wr_best_vref(ch, cs));
	}

	/* remove the last ',' character */
	info_size--;

	__qc_hw_param_clean_format(buf, &info_size, sz_buf);

	return info_size;
}
DEVICE_ATTR_RO(eye_wr1_info);

/* TODO: only for 'use_ddr_eye_info' */
static ssize_t eye_wr2_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = DEFAULT_LEN_STR;
	size_t ch, cs, dq;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"wr_topw\":\"W4\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"WT%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq,
				sec_qc_smem_lpddr_get_wr_vmax_to_vmid(ch, cs, dq));
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"wr_botw\":\"W4\",");
	for_each_ch_cs_dq(ch, NUM_CH, cs, NUM_CS, dq, NUM_DQ_PCH) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"WB%zu_%zu_%zu\":\"%d\",",
				ch, cs, dq,
				sec_qc_smem_lpddr_get_wr_vmid_to_vmin(ch, cs, dq));
	}

	/* remove the last ',' character */
	info_size--;

	__qc_hw_param_clean_format(buf, &info_size, sz_buf);

	return info_size;
}
DEVICE_ATTR_RO(eye_wr2_info);
