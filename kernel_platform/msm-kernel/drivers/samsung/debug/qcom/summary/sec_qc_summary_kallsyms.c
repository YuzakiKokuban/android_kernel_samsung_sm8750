// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>

#include <debug_kinfo.h>

#include "sec_qc_summary.h"

/* NOTE: copy of 'kallsyms_expand_symbol' */
static unsigned int __kallsyms_expand_symbol(
		const struct qc_summary_kallsyms *kallsyms,
		unsigned int off, char *result, size_t maxlen)
{
	int len, skipped_first = 0;
	const char *tptr;
	const u8 *data;

	data = &kallsyms->names[off];
	len = *data;
	data++;

	off += len + 1;

	while (len) {
		tptr = &kallsyms->token_table[kallsyms->token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				if (maxlen <= 1)
					goto tail;
				*result = *tptr;
				result++;
				maxlen--;
			} else
				skipped_first = 1;
			tptr++;
		}
	}

tail:
	if (maxlen)
		*result = '\0';

	return off;
}

/* NOTE: copy of 'kallsyms_sym_address' */
static unsigned long __kallsyms_sym_address(
		const struct qc_summary_kallsyms *kallsyms,
		int idx)
{
	if (kallsyms->addresses)
		return kallsyms->addresses[idx];

	/* values are unsigned offsets if --absolute-percpu is not in effect */
	if (kallsyms->relative_base)
		return kallsyms->relative_base + (u32)kallsyms->offsets[idx];

	/* ...otherwise, positive offsets are absolute values */
	if (kallsyms->offsets[idx] >= 0)
		return kallsyms->offsets[idx];

	/* ...and negative offsets are relative to kallsyms_relative_base - 1 */
	return kallsyms->relative_base - 1 - kallsyms->offsets[idx];
}

/* NOTE: copy of 'cleanup_symbol_name' */
#if defined(CONFIG_CFI_CLANG) && defined(CONFIG_LTO_CLANG_THIN)
static inline char *__cleanup_symbol_name(char *s)
{
	char *res = NULL;

	res = strrchr(s, '$');
	if (res)
		*res = '\0';

	return res;
}
#else
static inline char *__cleanup_symbol_name(char *s) { return NULL; }
#endif

static unsigned long __kallsyms_lookup_name(
		const struct qc_summary_kallsyms *kallsyms,
		const char *name)
{
	char namebuf[KSYM_NAME_LEN];
	unsigned long i;
	unsigned int off;

	if (!kallsyms->names)
		return 0;

	for (i = 0, off = 0; i < kallsyms->num_syms; i++) {
		off = __kallsyms_expand_symbol(kallsyms,
				off, namebuf, ARRAY_SIZE(namebuf));

		if (strcmp(namebuf, name) == 0)
			return __kallsyms_sym_address(kallsyms, i);

		if (__cleanup_symbol_name(namebuf) && strcmp(namebuf, name) == 0)
			return __kallsyms_sym_address(kallsyms, i);
	}

	return 0;
}

static inline int __kallsyms_init(struct qc_summary_drvdata *drvdata)
{
	struct qc_summary_kallsyms *kallsyms = &drvdata->kallsyms;
	const struct kernel_all_info *all_kinfo =
			drvdata->debug_kinfo_rmem->priv;
	const struct kernel_info *kinfo = &(all_kinfo->info);

	if (!kinfo->enabled_all)
		return 0;

	if (kinfo->enabled_absolute_percpu) {
		kallsyms->addresses = (unsigned long *)__phys_to_kimg(kinfo->_addresses_pa);
		kallsyms->relative_base = 0x0;
		kallsyms->offsets = 0x0;
	} else {
		kallsyms->addresses = 0x0;
		kallsyms->relative_base = __phys_to_kimg(kinfo->_relative_pa);
		kallsyms->offsets = (int *)__phys_to_kimg(kinfo->_offsets_pa);
	}

	kallsyms->names = (u8 *)__phys_to_kimg(kinfo->_names_pa);
	kallsyms->num_syms = kinfo->num_syms;
	kallsyms->token_table = (char *)__phys_to_kimg(kinfo->_token_table_pa);
	kallsyms->token_index = (u16 *)__phys_to_kimg(kinfo->_token_index_pa);
	kallsyms->markers = (unsigned int *)__phys_to_kimg(kinfo->_markers_pa);

	return 0;
}

int __qc_summary_kallsyms_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);

	if (!IS_ENABLED(CONFIG_ANDROID_DEBUG_KINFO))
		return 0;

	if (IS_BUILTIN(CONFIG_SEC_QC_SUMMARY))
		return 0;

	return __kallsyms_init(drvdata);
}

/* TODO: DO NOT EXPORT THIS FUNCTION!!! */
unsigned long __qc_summary_kallsyms_lookup_name(
		struct qc_summary_drvdata *drvdata,
		const char *name)
{
	const struct qc_summary_kallsyms *kallsyms = &drvdata->kallsyms;
	unsigned long addr;

	if (!IS_ENABLED(CONFIG_ANDROID_DEBUG_KINFO))
		return 0;

	if (IS_BUILTIN(CONFIG_SEC_QC_SUMMARY))
		return kallsyms_lookup_name(name);

	addr = __kallsyms_lookup_name(kallsyms, name);

	return addr;
}
