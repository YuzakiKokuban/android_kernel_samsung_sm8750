// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/kallsyms.h>
#include <linux/kernel.h>

#include <debug_kinfo.h>

#include "sec_qc_summary.h"

static inline void __summary_ksyms_init_builtin(
		struct qc_summary_drvdata *drvdata)
{
	struct sec_qc_summary_ksyms *ksyms =
			&(secdbg_apss(drvdata)->ksyms);

	if (!IS_ENABLED(CONFIG_KALLSYMS_BASE_RELATIVE)) {
		ksyms->addresses_pa = __pa(kallsyms_addresses);
		ksyms->relative_base = 0x0;
		ksyms->offsets_pa = 0x0;
	} else {
		ksyms->addresses_pa = 0x0;
		ksyms->relative_base = (uint64_t)kallsyms_relative_base;
		ksyms->offsets_pa = __pa(kallsyms_offsets);
	}

	ksyms->names_pa = __pa(kallsyms_names);
	ksyms->num_syms = kallsyms_num_syms;
	ksyms->token_table_pa = __pa(kallsyms_token_table);
	ksyms->token_index_pa = __pa(kallsyms_token_index);
	ksyms->markers_pa = __pa(kallsyms_markers);

	ksyms->sect.sinittext = (uintptr_t)_sinittext;
	ksyms->sect.einittext = (uintptr_t)_einittext;
	ksyms->sect.stext = (uintptr_t)_stext;
	ksyms->sect.etext = (uintptr_t)_etext;
	ksyms->sect.end = (uintptr_t)_end;

	ksyms->kallsyms_all = !!IS_ENABLED(CONFIG_KALLSYMS_ALL);
	ksyms->magic = SEC_DEBUG_SUMMARY_MAGIC1;
}

static inline void __summary_ksyms_init_module(
		struct qc_summary_drvdata *drvdata)
{
	struct sec_qc_summary_ksyms *ksyms =
			&(secdbg_apss(drvdata)->ksyms);
	const struct kernel_all_info *all_kinfo =
			drvdata->debug_kinfo_rmem->priv;
	const struct kernel_info *kinfo = &(all_kinfo->info);

	if (kinfo->enabled_absolute_percpu) {
		ksyms->addresses_pa = kinfo->_addresses_pa;
		ksyms->relative_base = 0x0;
		ksyms->offsets_pa = 0x0;
	} else {
		ksyms->addresses_pa = 0x0;
		ksyms->relative_base = __phys_to_kimg(kinfo->_relative_pa);
		ksyms->offsets_pa = kinfo->_offsets_pa;
	}

	ksyms->names_pa = kinfo->_names_pa;
	ksyms->num_syms = kinfo->num_syms;
	ksyms->token_table_pa = kinfo->_token_table_pa;
	ksyms->token_index_pa = kinfo->_token_index_pa;
	ksyms->markers_pa = kinfo->_markers_pa;

	ksyms->sect.sinittext = __phys_to_kimg(kinfo->_sinittext_pa);
	ksyms->sect.einittext = __phys_to_kimg(kinfo->_einittext_pa);
	ksyms->sect.stext = __phys_to_kimg(kinfo->_stext_pa);
	ksyms->sect.etext = __phys_to_kimg(kinfo->_etext_pa);
	ksyms->sect.end = __phys_to_kimg(kinfo->_end_pa);

	ksyms->kallsyms_all = !!kinfo->enabled_all;
	ksyms->magic = SEC_DEBUG_SUMMARY_MAGIC1;
}

int notrace __qc_summary_ksyms_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);

	if (IS_BUILTIN(CONFIG_SEC_QC_SUMMARY))
		__summary_ksyms_init_builtin(drvdata);
	else if (!IS_ENABLED(CONFIG_ANDROID_DEBUG_KINFO))
		return 0;
	else
		__summary_ksyms_init_module(drvdata);

	return 0;
}
