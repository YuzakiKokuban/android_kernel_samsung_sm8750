// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/io.h>
#include <linux/kernel.h>


#include "sec_qc_summary.h"
#include "sec_qc_summary_external.h"

static void __summary_set_lpm_info_runqueues(
		struct sec_qc_summary_aplpm *aplpm)
{
	/* TODO: there is no way to know this variable in module */
	aplpm->p_runqueues = virt_to_phys((void *)&runqueues);
}

int notrace __qc_summary_aplpm_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct sec_qc_summary_aplpm *aplpm =
			&(secdbg_apss(drvdata)->aplpm);

	__summary_set_lpm_info_runqueues(aplpm);

	sec_qc_summary_set_sched_walt_info(secdbg_apss(drvdata));

	return 0;
}
