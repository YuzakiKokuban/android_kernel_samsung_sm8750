// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2015-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include "sec_qc_smem.h"

#define __smem_id_vendor1_get_member_ptr(__drvdata, __member) \
({ \
	const struct vendor1_operations *ops = __drvdata->vendor1_ops; \
	void *member_ptr; \
	member_ptr = ops->__member ? \
			ops->__member(__drvdata->vendor1) : ERR_PTR(-ENOENT); \
	member_ptr; \
})

sec_smem_header_t *__qc_smem_id_vendor1_get_header(struct qc_smem_drvdata *drvdata)
{
	return __smem_id_vendor1_get_member_ptr(drvdata, header);
}

sec_smem_id_vendor1_v2_t *__qc_smem_id_vendor1_get_ven1_v2(
		struct qc_smem_drvdata *drvdata)
{
	return __smem_id_vendor1_get_member_ptr(drvdata, ven1_v2);
}

sec_smem_id_vendor1_share_t *__qc_smem_id_vendor1_get_share(
		struct qc_smem_drvdata *drvdata)
{
	return __smem_id_vendor1_get_member_ptr(drvdata, share);
}

smem_ddr_stat_t *__qc_smem_id_vendor1_get_ddr_stat(struct qc_smem_drvdata *drvdata)
{
	return __smem_id_vendor1_get_member_ptr(drvdata, ddr_stat);
}

void **__qc_smem_id_vendor1_get_ap_health(struct qc_smem_drvdata *drvdata)
{
	return __smem_id_vendor1_get_member_ptr(drvdata, ap_health);
}

smem_apps_stat_t *__qc_smem_id_vendor1_get_apps_stat(
		struct qc_smem_drvdata *drvdata)
{
	return __smem_id_vendor1_get_member_ptr(drvdata, apps_stat);
}

smem_vreg_stat_t *__qc_smem_id_vendor1_get_vreg_stat(
		struct qc_smem_drvdata *drvdata)
{
	return __smem_id_vendor1_get_member_ptr(drvdata, vreg_stat);
}

ddr_train_t *__qc_smem_id_vendor1_get_ddr_training(
		struct qc_smem_drvdata *drvdata)
{
	return __smem_id_vendor1_get_member_ptr(drvdata, ddr_training);
}

static const struct vendor1_operations *
		__smem_vendor1_ops_creator(struct qc_smem_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	const struct vendor1_operations *ops;
	unsigned int vendor1_ver = drvdata->vendor1_ver;

	switch (vendor1_ver) {
	case 5:
		ops = __qc_smem_vendor1_v5_ops_creator();
		break;
	case 6:
		ops = __qc_smem_vendor1_v6_ops_creator();
		break;
	case 7:
		ops = __qc_smem_vendor1_v7_ops_creator();
		break;
	default:
		ops = __qc_smem_vendor1_v7_ops_creator();
		dev_warn(dev, "v%u is not supported or deprecated. use default\n",
				vendor1_ver);
		break;
	}

	return ops;
}

static void __smem_vendor1_operations_factory(struct qc_smem_drvdata *drvdata)
{
	drvdata->vendor1_ops = __smem_vendor1_ops_creator(drvdata);
}

int __qc_smem_id_vendor1_init(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	void **ap_health;

	__smem_vendor1_operations_factory(drvdata);

	ap_health = __qc_smem_id_vendor1_get_ap_health(drvdata);
	if (!IS_ERR_OR_NULL(ap_health))
		*ap_health = (void *)virt_to_phys(drvdata->ap_health);

	return 0;
}
