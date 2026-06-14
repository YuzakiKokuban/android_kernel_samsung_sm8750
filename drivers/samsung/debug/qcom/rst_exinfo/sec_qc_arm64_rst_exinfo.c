// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2022 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/kernel.h>

#include "sec_qc_rst_exinfo.h"

void __qc_arch_rst_exinfo_die_handler(struct rst_exinfo_drvdata *drvdata,
		enum die_val val, const struct die_args *args)
{
	struct pt_regs *regs = args->regs;

	__qc_rst_exinfo_fault_print_handler(drvdata, args->str);
	__qc_rst_exinfo_store_extc_idx(drvdata, false);
	__qc_rst_exinfo_save_dying_msg(drvdata, args->str,
			(void *)instruction_pointer(regs),
			(void *)regs->regs[30]);
}
