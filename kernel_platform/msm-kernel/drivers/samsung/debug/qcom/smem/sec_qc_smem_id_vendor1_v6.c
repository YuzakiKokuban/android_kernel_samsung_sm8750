// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2015-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include "sec_qc_smem.h"

#define member_ptr_of_v6(__vendor1, __member) \
	&((sec_smem_id_vendor1_v6_t *)(__vendor1))->__member

static sec_smem_header_t *vendor1_v6_header(void *vendor1)
{
	return member_ptr_of_v6(vendor1, header);
}

static sec_smem_id_vendor1_v2_t *vendor1_v6_ven1_v2(void *vendor1)
{
	return member_ptr_of_v6(vendor1, ven1_v2);
}

static smem_ddr_stat_t *vendor1_v6_ddr_stat(void *vendor1)
{
	return member_ptr_of_v6(vendor1, ddr_stat);
}

static void **vendor1_v6_ap_health(void *vendor1)
{
	return member_ptr_of_v6(vendor1, ap_health);
}

static smem_apps_stat_t *vendor1_v6_apps_stat(void *vendor1)
{
	return member_ptr_of_v6(vendor1, apps_stat);
}

static smem_vreg_stat_t *vendor1_v6_vreg_stat(void *vendor1)
{
	return member_ptr_of_v6(vendor1, vreg_stat);
}

static ddr_train_t *vendor1_v6_ddr_training(void *vendor1)
{
	return member_ptr_of_v6(vendor1, ddr_training);
}

static const struct vendor1_operations smem_id_vendor1_v6_ops = {
	.header = vendor1_v6_header,
	.ven1_v2 = vendor1_v6_ven1_v2,
	.ddr_stat = vendor1_v6_ddr_stat,
	.ap_health = vendor1_v6_ap_health,
	.apps_stat = vendor1_v6_apps_stat,
	.vreg_stat = vendor1_v6_vreg_stat,
	.ddr_training = vendor1_v6_ddr_training,
};

const struct vendor1_operations *__qc_smem_vendor1_v6_ops_creator(void)
{
	return &smem_id_vendor1_v6_ops;
}
