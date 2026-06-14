// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/kernel.h>

#include "sec_qc_summary.h"

/* NOTE: mock functions to replace sec_qc_summary_arm64_ap_context.c */
int __weak __qc_summary_arm64_ap_context_init(struct builder *bd)
{
	return 0;
}

void __weak __qc_summary_arm64_ap_context_exit(struct builder *bd)
{
}

/* NOTE: mock functions to replace 'sec_qc_summary_var_mon.c' */
int __weak __qc_summary_info_mon_init(struct builder *bd)
{
	return 0;
}

int __weak __qc_summary_var_mon_init_last_pet(struct builder *bd)
{
	return 0;
}

int __weak __qc_summary_var_mon_init_last_ns(struct builder *bd)
{
	return 0;
}

/* NOTE: mock functions to replace 'sec_qc_summary_coreinfo.c' */
void __weak __qc_summary_var_mon_exit_last_pet(struct builder *bd)
{
}

int __weak __qc_summary_coreinfo_init(struct builder *bd)
{
	return 0;
}

void __weak __qc_summary_coreinfo_exit(struct builder *bd)
{
}
