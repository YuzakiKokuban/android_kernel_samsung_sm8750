// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include "sec_qc_smem.h"

static char *lpddr_manufacture_name[] = {
	"NA",
	"SEC"/* Samsung */,
	"NA",
	"NA",
	"NA",
	"NAN" /* Nanya */,
	"HYN" /* SK hynix */,
	"NA",
	"WIN" /* Winbond */,
	"ESM" /* ESMT */,
	"NA",
	"NA",
	"NA",
	"NA",
	"NA",
	"MIC" /* Micron */,
};

static uint32_t __lpddr_get_ddr_info(uint32_t type)
{
	sec_smem_id_vendor0_v2_t *vendor0;

	if (!__qc_smem_is_probed())
		return 0;

	vendor0 = qc_smem->vendor0;
	if (IS_ERR_OR_NULL(vendor0)) {
		dev_warn(qc_smem->bd.dev, "SMEM_ID_VENDOR0 get entry error\n");
		return 0;
	}

	return (vendor0->ddr_vendor >> type) & 0xFF;
}

uint8_t sec_qc_smem_lpddr_get_revision_id1(void)
{
	return __lpddr_get_ddr_info(DDR_IFNO_REVISION_ID1);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_revision_id1);

uint8_t sec_qc_smem_lpddr_get_revision_id2(void)
{
	return __lpddr_get_ddr_info(DDR_IFNO_REVISION_ID2);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_revision_id2);

uint8_t sec_qc_smem_lpddr_get_total_density(void)
{
	return __lpddr_get_ddr_info(DDR_IFNO_TOTAL_DENSITY);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_total_density);

static const char *__lpddr_get_vendor_name(struct qc_smem_drvdata *drvdata)
{
	sec_smem_id_vendor0_v2_t *vendor0 = drvdata->vendor0;
	size_t manufacture;

	if (IS_ERR_OR_NULL(vendor0)) {
		dev_warn(qc_smem->bd.dev, "SMEM_ID_VENDOR0 get entry error\n");
		return "NA";
	}

	manufacture = (size_t)vendor0->ddr_vendor
			% ARRAY_SIZE(lpddr_manufacture_name);

	return lpddr_manufacture_name[manufacture];
}

const char *sec_qc_smem_lpddr_get_vendor_name(void)
{
	if (!__qc_smem_is_probed())
		return "NA";

	return __lpddr_get_vendor_name(qc_smem);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_vendor_name);

static ddr_train_t *__lpddr_get_ddr_train(struct qc_smem_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	void *vendor1 = drvdata->vendor1;
	ddr_train_t *ddr_training;

	if (IS_ERR_OR_NULL(vendor1)) {
		dev_warn(dev, "SMEM_ID_VENDOR1 get entry error\n");
		return ERR_PTR(-ENODEV);
	}

	ddr_training = __qc_smem_id_vendor1_get_ddr_training(drvdata);
	if (IS_ERR_OR_NULL(ddr_training)) {
		dev_warn(dev, "SMEM_ID_VENDOR1 is invalid or wrong version (%ld)\n",
				PTR_ERR(ddr_training));
		return ERR_PTR(-ENOENT);
	}

	return ddr_training;
}

static uint32_t __lpddr_get_DSF_version(struct qc_smem_drvdata *drvdata)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->version;
}

uint32_t sec_qc_smem_lpddr_get_DSF_version(void)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_DSF_version(qc_smem);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_DSF_version);


static uint8_t __lpddr_get_rcw_tDQSCK(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->rcw.tDQSCK[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_rcw_tDQSCK(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_rcw_tDQSCK(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_rcw_tDQSCK);

static uint8_t __lpddr_get_wr_coarseCDC(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->wr_dqdqs.coarse_cdc[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_wr_coarseCDC(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_wr_coarseCDC(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_wr_coarseCDC);

static uint8_t __lpddr_get_wr_fineCDC(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->wr_dqdqs.fine_cdc[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_wr_fineCDC(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_wr_fineCDC(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_wr_fineCDC);

static uint8_t __lpddr_get_wr_pr_width(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.wr_pr_width[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_wr_pr_width(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_wr_pr_width(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_wr_pr_width);

static uint8_t __lpddr_get_wr_min_eye_height(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.wr_min_eye_height[ch][cs];
}

uint8_t sec_qc_smem_lpddr_get_wr_min_eye_height(size_t ch, size_t cs)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_wr_min_eye_height(qc_smem, ch, cs);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_wr_min_eye_height);

static uint8_t __lpddr_get_wr_best_vref(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.wr_best_vref[ch][cs];
}

uint8_t sec_qc_smem_lpddr_get_wr_best_vref(size_t ch, size_t cs)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_wr_best_vref(qc_smem, ch, cs);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_wr_best_vref);

static uint8_t __lpddr_get_wr_vmax_to_vmid(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.wr_vmax_to_vmid[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_wr_vmax_to_vmid(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_wr_vmax_to_vmid(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_wr_vmax_to_vmid);

static uint8_t __lpddr_get_wr_vmid_to_vmin(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.wr_vmid_to_vmin[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_wr_vmid_to_vmin(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_wr_vmid_to_vmin(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_wr_vmid_to_vmin);

static uint8_t __lpddr_get_dqs_dcc_adj(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.dqs_dcc_adj[ch][dq];
}

uint8_t sec_qc_smem_lpddr_get_dqs_dcc_adj(size_t ch, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_dqs_dcc_adj(qc_smem, ch, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_dqs_dcc_adj);

static uint8_t __lpddr_get_rd_pr_width(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.rd_pr_width[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_rd_pr_width(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_rd_pr_width(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_rd_pr_width);

static uint8_t __lpddr_get_rd_min_eye_height(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.rd_min_eye_height[ch][cs];
}

uint8_t sec_qc_smem_lpddr_get_rd_min_eye_height(size_t ch, size_t cs)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_rd_min_eye_height(qc_smem, ch, cs);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_rd_min_eye_height);

static uint8_t __lpddr_get_rd_best_vref(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.rd_best_vref[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_rd_best_vref(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_rd_best_vref(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_rd_best_vref);

static uint8_t __lpddr_get_dq_dcc_abs(struct qc_smem_drvdata *drvdata,
		size_t ch, size_t cs, size_t dq)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.dq_dcc_abs[ch][cs][dq];
}

uint8_t sec_qc_smem_lpddr_get_dq_dcc_abs(size_t ch, size_t cs, size_t dq)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_dq_dcc_abs(qc_smem, ch, cs, dq);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_dq_dcc_abs);

static uint16_t __lpddr_get_small_eye_detected(struct qc_smem_drvdata *drvdata)
{
	ddr_train_t *ddr_training = __lpddr_get_ddr_train(drvdata);

	if  (IS_ERR_OR_NULL(ddr_training))
		return 0;

	return ddr_training->dqdqs_eye.small_eye_detected;
}

uint8_t sec_qc_smem_lpddr_get_small_eye_detected(void)
{
	if (!__qc_smem_is_probed())
		return 0;

	return __lpddr_get_small_eye_detected(qc_smem);
}
EXPORT_SYMBOL_GPL(sec_qc_smem_lpddr_get_small_eye_detected);
