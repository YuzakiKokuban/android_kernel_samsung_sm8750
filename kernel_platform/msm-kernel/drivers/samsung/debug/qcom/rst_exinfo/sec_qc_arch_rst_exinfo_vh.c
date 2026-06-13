// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022 Samsung Electronics Co., Ltd. All Right Reserved.
 */

/* This file contains '__weak' function for the UML based testing. */

#include <linux/kernel.h>

#include <linux/samsung/builder_pattern.h>

int __weak __qc_arch_rst_exinfo_vh_init(struct builder *bd)
{
	return 0;
}

int __weak __qc_arch_rst_exinfo_register_rvh_die_kernel_fault(struct builder *bd)
{
	return 0;
}

int __weak __qc_arch_rst_exinfo_register_rvh_do_sp_pc_abort(struct builder *bd)
{
	return 0;
}

int __weak __qc_arch_rst_exinfo_register_rvh_report_bug(struct builder *bd)
{
	return 0;
}
