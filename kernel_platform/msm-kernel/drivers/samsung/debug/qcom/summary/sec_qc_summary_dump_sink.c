// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/samsung/debug/sec_debug.h>

#include "sec_qc_summary.h"

int notrace __qc_summary_dump_sink_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);

	secdbg_apss(drvdata)->dump_sink_paddr = sec_debug_get_dump_sink_phys();

	return 0;
}
