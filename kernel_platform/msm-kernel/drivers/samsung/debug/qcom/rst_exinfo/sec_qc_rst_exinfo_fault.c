// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/kernel.h>
#include <linux/stringhash.h>

#include <linux/samsung/sec_kunit.h>

#include "sec_qc_rst_exinfo.h"

#define MSG_TO_FAULT_HANDLER(__msg, __handler) \
	{ \
		.msg = __msg, \
		.msg_len = sizeof(__msg) - 1, \
		.handler_name = #__handler, \
		.handler_code = FAULT_HANDLER_ ## __handler, \
	}

/* TODO: cat arch/arm64/mm/fault.c | grep -E "\{[ \t]*(do_.+|early_brk64)" | perl -pe 's/^.*{\s*(\S+),.+(\".+\").+$/\tMSG_TO_FAULT_HANDLER($2, $1),/' */
static struct msg_to_fault_handler __msg_to_fault_handler[] __ro_after_init = {
        MSG_TO_FAULT_HANDLER("ttbr address size fault", do_bad),
        MSG_TO_FAULT_HANDLER("level 1 address size fault", do_bad),
        MSG_TO_FAULT_HANDLER("level 2 address size fault", do_bad),
        MSG_TO_FAULT_HANDLER("level 3 address size fault", do_bad),
        MSG_TO_FAULT_HANDLER("level 0 translation fault", do_translation_fault),
        MSG_TO_FAULT_HANDLER("level 1 translation fault", do_translation_fault),
        MSG_TO_FAULT_HANDLER("level 2 translation fault", do_translation_fault),
        MSG_TO_FAULT_HANDLER("level 3 translation fault", do_translation_fault),
        MSG_TO_FAULT_HANDLER("unknown 8", do_bad),
        MSG_TO_FAULT_HANDLER("level 1 access flag fault", do_page_fault),
        MSG_TO_FAULT_HANDLER("level 2 access flag fault", do_page_fault),
        MSG_TO_FAULT_HANDLER("level 3 access flag fault", do_page_fault),
        MSG_TO_FAULT_HANDLER("unknown 12", do_bad),
        MSG_TO_FAULT_HANDLER("level 1 permission fault", do_page_fault),
        MSG_TO_FAULT_HANDLER("level 2 permission fault", do_page_fault),
        MSG_TO_FAULT_HANDLER("level 3 permission fault", do_page_fault),
        MSG_TO_FAULT_HANDLER("synchronous external abort", do_sea),
        MSG_TO_FAULT_HANDLER("synchronous tag check fault", do_tag_check_fault),
        MSG_TO_FAULT_HANDLER("unknown 18", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 19", do_bad),
        MSG_TO_FAULT_HANDLER("level 0 (translation table walk)", do_sea),
        MSG_TO_FAULT_HANDLER("level 1 (translation table walk)", do_sea),
        MSG_TO_FAULT_HANDLER("level 2 (translation table walk)", do_sea),
        MSG_TO_FAULT_HANDLER("level 3 (translation table walk)", do_sea),
        MSG_TO_FAULT_HANDLER("synchronous parity or ECC error", do_sea),
        MSG_TO_FAULT_HANDLER("unknown 25", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 26", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 27", do_bad),
        MSG_TO_FAULT_HANDLER("level 0 synchronous parity error (translation table walk)", do_sea),
        MSG_TO_FAULT_HANDLER("level 1 synchronous parity error (translation table walk)", do_sea),
        MSG_TO_FAULT_HANDLER("level 2 synchronous parity error (translation table walk)", do_sea),
        MSG_TO_FAULT_HANDLER("level 3 synchronous parity error (translation table walk)", do_sea),
        MSG_TO_FAULT_HANDLER("unknown 32", do_bad),
        MSG_TO_FAULT_HANDLER("alignment fault", do_alignment_fault),
        MSG_TO_FAULT_HANDLER("unknown 34", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 35", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 36", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 37", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 38", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 39", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 40", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 41", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 42", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 43", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 44", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 45", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 46", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 47", do_bad),
        MSG_TO_FAULT_HANDLER("TLB conflict abort", do_bad),
        MSG_TO_FAULT_HANDLER("Unsupported atomic hardware update fault", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 50", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 51", do_bad),
        MSG_TO_FAULT_HANDLER("implementation fault (lockdown abort)", do_bad),
        MSG_TO_FAULT_HANDLER("implementation fault (unsupported exclusive)", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 54", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 55", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 56", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 57", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 58", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 59", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 60", do_bad),
        MSG_TO_FAULT_HANDLER("section domain fault", do_bad),
        MSG_TO_FAULT_HANDLER("page domain fault", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 63", do_bad),
        MSG_TO_FAULT_HANDLER("hardware breakpoint", do_bad),
        MSG_TO_FAULT_HANDLER("hardware single-step", do_bad),
        MSG_TO_FAULT_HANDLER("hardware watchpoint", do_bad),
        MSG_TO_FAULT_HANDLER("unknown 3", do_bad),
        MSG_TO_FAULT_HANDLER("aarch32 BKPT", do_bad),
        MSG_TO_FAULT_HANDLER("aarch32 vector catch", do_bad),
        MSG_TO_FAULT_HANDLER("aarch64 BRK", early_brk64),
        MSG_TO_FAULT_HANDLER("unknown 7", do_bad),
};

__ss_static struct msg_to_fault_handler *__rst_exinfo_find_handler(
		struct rst_exinfo_drvdata *drvdata, const char *msg)
{
	struct msg_to_fault_handler *h;
	size_t msg_len = strlen(msg);
	u32 key = full_name_hash(NULL, msg, (unsigned int)msg_len);

	hash_for_each_possible(drvdata->die_handler_htbl, h, node, key) {
		if (h->msg_len != msg_len)
			continue;

		if (!strncmp(h->msg, msg, msg_len))
			return h;
	}

	return ERR_PTR(-ENOENT);
}

void __qc_rst_exinfo_fault_print_handler(struct rst_exinfo_drvdata *drvdata,
		const char *msg)
{
	struct device *dev = drvdata->bd.dev;
	struct msg_to_fault_handler *h = __rst_exinfo_find_handler(drvdata, msg);

	if (IS_ERR_OR_NULL(h)) {
		dev_info(dev, "fault handler : unknown\n");
		return;
	}

	dev_info(dev, "fault hanlder : %s (%u)\n", h->handler_name, h->handler_code);
}

int __qc_rst_exinfo_fault_init(struct builder *bd)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);
	size_t i;

	hash_init(drvdata->die_handler_htbl);

	for (i = 0; i < ARRAY_SIZE(__msg_to_fault_handler); i++) {
		struct msg_to_fault_handler *h = &__msg_to_fault_handler[i];
		u32 key = full_name_hash(NULL, h->msg, (unsigned int)strlen(h->msg));

		INIT_HLIST_NODE(&h->node);
		hash_add(drvdata->die_handler_htbl, &h->node, key);
	}

	return 0;
}
