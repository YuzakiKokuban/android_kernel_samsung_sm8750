// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/panic_notifier.h>

#include <linux/samsung/debug/sec_debug_region.h>

#include "sec_qc_summary.h"

struct sec_debug_mmu_reg_t {
	uint64_t TTBR0_EL1;
	uint64_t TTBR1_EL1;
	uint64_t TCR_EL1;
	uint64_t MAIR_EL1;
	uint64_t ATCR_EL1;
	uint64_t AMAIR_EL1;

	uint64_t HSTR_EL2;
	uint64_t HACR_EL2;
	uint64_t TTBR0_EL2;
	uint64_t VTTBR_EL2;
	uint64_t TCR_EL2;
	uint64_t VTCR_EL2;
	uint64_t MAIR_EL2;
	uint64_t ATCR_EL2;

	uint64_t TTBR0_EL3;
	uint64_t MAIR_EL3;
	uint64_t ATCR_EL3;
};

/* ARM CORE regs mapping structure */
struct sec_debug_core_t {
	/* COMMON */
	uint64_t x0;
	uint64_t x1;
	uint64_t x2;
	uint64_t x3;
	uint64_t x4;
	uint64_t x5;
	uint64_t x6;
	uint64_t x7;
	uint64_t x8;
	uint64_t x9;
	uint64_t x10;
	uint64_t x11;
	uint64_t x12;
	uint64_t x13;
	uint64_t x14;
	uint64_t x15;
	uint64_t x16;
	uint64_t x17;
	uint64_t x18;
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29;		/* sp */
	uint64_t x30;		/* lr */

	uint64_t pc;		/* pc */
	uint64_t cpsr;		/* cpsr */

	/* EL0 */
	uint64_t sp_el0;

	/* EL1 */
	uint64_t sp_el1;
	uint64_t elr_el1;
	uint64_t spsr_el1;

	/* EL2 */
	uint64_t sp_el2;
	uint64_t elr_el2;
	uint64_t spsr_el2;

	/* EL3 */
	/* uint64_t sp_el3; */
	/* uint64_t elr_el3; */
	/* uint64_t spsr_el3; */
};

DEFINE_PER_CPU(struct sec_debug_core_t, sec_debug_core_reg);
DEFINE_PER_CPU(struct sec_debug_mmu_reg_t, sec_debug_mmu_reg);

static inline void __summary_ap_context_core(
		struct qc_summary_ap_context *qc_ap_context, int cpu)
{
	uint64_t pstate, which_el;
	struct sec_arm64_ap_context *ap_context =
			&qc_ap_context->ap_context[cpu];
	struct sec_debug_core_t *core_reg = &per_cpu(sec_debug_core_reg, cpu);

	core_reg->x0 = ap_context->core_regs.regs[0];
	core_reg->x1 = ap_context->core_regs.regs[1];
	core_reg->x2 = ap_context->core_regs.regs[2];
	core_reg->x3 = ap_context->core_regs.regs[3];
	core_reg->x4 = ap_context->core_regs.regs[4];
	core_reg->x5 = ap_context->core_regs.regs[5];
	core_reg->x6 = ap_context->core_regs.regs[6];
	core_reg->x7 = ap_context->core_regs.regs[7];
	core_reg->x8 = ap_context->core_regs.regs[8];
	core_reg->x9 = ap_context->core_regs.regs[9];
	core_reg->x10 = ap_context->core_regs.regs[10];
	core_reg->x11 = ap_context->core_regs.regs[11];
	core_reg->x12 = ap_context->core_regs.regs[12];
	core_reg->x13 = ap_context->core_regs.regs[13];
	core_reg->x14 = ap_context->core_regs.regs[14];
	core_reg->x15 = ap_context->core_regs.regs[15];
	core_reg->x16 = ap_context->core_regs.regs[16];
	core_reg->x17 = ap_context->core_regs.regs[17];
	core_reg->x18 = ap_context->core_regs.regs[18];
	core_reg->x19 = ap_context->core_regs.regs[19];
	core_reg->x20 = ap_context->core_regs.regs[20];
	core_reg->x21 = ap_context->core_regs.regs[21];
	core_reg->x22 = ap_context->core_regs.regs[22];
	core_reg->x23 = ap_context->core_regs.regs[23];
	core_reg->x24 = ap_context->core_regs.regs[24];
	core_reg->x25 = ap_context->core_regs.regs[25];
	core_reg->x26 = ap_context->core_regs.regs[26];
	core_reg->x27 = ap_context->core_regs.regs[27];
	core_reg->x28 = ap_context->core_regs.regs[28];
	core_reg->x29 = ap_context->core_regs.regs[29];
	core_reg->x30 = ap_context->core_regs.regs[30];

	core_reg->sp_el1 = ap_context->core_regs.sp;
	core_reg->pc = ap_context->core_regs.pc;
	core_reg->cpsr = ap_context->core_regs.pstate;

	core_reg->sp_el0 = ap_context->core_extra_regs[IDX_CORE_EXTRA_SP_EL0];

	pstate = ap_context->core_regs.pstate;
	which_el = pstate & PSR_MODE_MASK;

	if (which_el >= PSR_MODE_EL2t) {
		core_reg->sp_el1 = ap_context->core_extra_regs[IDX_CORE_EXTRA_SP_EL1];
		core_reg->elr_el1 = ap_context->core_extra_regs[IDX_CORE_EXTRA_ELR_EL1];
		core_reg->spsr_el1 = ap_context->core_extra_regs[IDX_CORE_EXTRA_SPSR_EL1];
		core_reg->sp_el2 = ap_context->core_extra_regs[IDX_CORE_EXTRA_SP_EL2];
		core_reg->elr_el2 = ap_context->core_extra_regs[IDX_CORE_EXTRA_ELR_EL2];
		core_reg->spsr_el2 = ap_context->core_extra_regs[IDX_CORE_EXTRA_SPSR_EL2];
	}
}

static inline void __summary_ap_context_mmu(
		struct qc_summary_ap_context *qc_ap_context, int cpu)
{
	struct sec_arm64_ap_context *ap_context =
			&qc_ap_context->ap_context[cpu];
	struct sec_debug_mmu_reg_t *mmu_reg = &per_cpu(sec_debug_mmu_reg, cpu);

	mmu_reg->TTBR0_EL1 = ap_context->mmu_regs[IDX_MMU_TTBR0_EL1];
	mmu_reg->TTBR1_EL1 = ap_context->mmu_regs[IDX_MMU_TTBR1_EL1];
	mmu_reg->TCR_EL1 = ap_context->mmu_regs[IDX_MMU_TCR_EL1];
	mmu_reg->MAIR_EL1 = ap_context->mmu_regs[IDX_MMU_MAIR_EL1];
	mmu_reg->AMAIR_EL1 = ap_context->mmu_regs[IDX_MMU_AMAIR_EL1];
}

static inline int sec_qc_summary_ap_context_notifier(
		struct notifier_block *this, unsigned long val, void *data)
{
	struct qc_summary_ap_context *qc_ap_context =
			container_of(this, struct qc_summary_ap_context, nb);
	int cpu;

	for (cpu = 0; cpu < num_possible_cpus(); cpu++) {
		__summary_ap_context_core(qc_ap_context, cpu);
		__summary_ap_context_mmu(qc_ap_context, cpu);
	}

	return NOTIFY_OK;
}

int notrace __qc_summary_arm64_ap_context_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct sec_qc_summary_cpu_context *cpu_reg =
			&(secdbg_apss(drvdata)->cpu_reg);
	struct qc_summary_ap_context *qc_ap_context = &drvdata->ap_context;
	const struct sec_dbg_region_client *client;
	int err;

	client = sec_dbg_region_find(SEC_ARM64_VH_IPI_STOP_MAGIC);
	if (IS_ERR(client)) {
		err = -EPROBE_DEFER;
		goto err_find_ref_data;
	}

	qc_ap_context->ap_context = (void *)client->virt;

	qc_ap_context->nb.notifier_call = sec_qc_summary_ap_context_notifier;
	/* NOTE: should be called just after 'sec_arm64_ap_context_on_panic' */
	qc_ap_context->nb.priority = 0x7FFFFFFE;

	err = atomic_notifier_chain_register(&panic_notifier_list,
			&qc_ap_context->nb);
	if (err < 0) {
		err = -EINVAL;
		goto err_register_panic_notifier;
	}

	/* setup sec debug core reg info */
	cpu_reg->sec_debug_core_reg_paddr = virt_to_phys(&sec_debug_core_reg);

	return 0;

err_register_panic_notifier:
err_find_ref_data:
	return err;
}

void __qc_summary_arm64_ap_context_exit(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct qc_summary_ap_context *qc_ap_context = &drvdata->ap_context;

	atomic_notifier_chain_unregister(&panic_notifier_list,
			&qc_ap_context->nb);
}
