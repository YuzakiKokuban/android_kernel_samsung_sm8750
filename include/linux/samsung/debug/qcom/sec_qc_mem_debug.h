#ifndef __SEC_QC_MEM_DEBUG_H__
#define __SEC_QC_MEM_DEBUG_H__

#include <linux/kernel.h>
#include <linux/uaccess.h>

#if IS_ENABLED(CONFIG_ARM64)
/* In line with aarch64_insn_read from arch/arm64/kernel/insn.c */
static inline int ___qc_memdbg_instruction_read(void *addr, u32 *insnp)
{
	int ret;
	__le32 val;

	ret = copy_from_kernel_nofault(&val, addr, AARCH64_INSN_SIZE);
	if (!ret)
		*insnp = le32_to_cpu(val);

	return ret;
}

/* In line with dump_kernel_instr from arch/arm64/kernel/traps.c */
static inline void sec_qc_memdbg_dump_instr(const char *rname, u64 instr)
{
	/* 2 instructions + 16 instructions (cache line size) + 2 instructions + "()" + null */
	char str[sizeof("00000000 ") * 20 + 2 + 1], *p = str;
	u64 i;
	const u64 start = ALIGN_DOWN(instr, cache_line_size()) - 0x8;
	const u64 end = ALIGN(instr, cache_line_size()) + 0x8;
	const u64 nr_entries = (end - start) / AARCH64_INSN_SIZE;
	const u64 current_idx = (instr - start) / AARCH64_INSN_SIZE;

	for (i = 0; i < nr_entries; i++) {
		unsigned int val, bad;

		bad = ___qc_memdbg_instruction_read(&((u32 *)start)[i], &val);
		if (!bad)
			p += scnprintf(p, sizeof(str) - (p - str),
					i == current_idx ? "(%08x) " : "%08x ", val);
		else {
			p += scnprintf(p, sizeof(str) - (p - str),
					i == current_idx ? "(********) " : "******** ");
		}
	}

	pr_emerg("%s Code: %s\n", rname, str);
}

/* In line with __show_regs from arch/arm64/kernel/process.c */
static inline void sec_qc_memdbg_show_regs_min(struct pt_regs *regs)
{
	ssize_t i = 29;

	pr_emerg("pc : %pS\n", (void *)regs->pc);
	pr_emerg("lr : %pS\n", (void *)ptrauth_strip_kernel_insn_pac(regs->regs[30]));
	pr_emerg("sp : %016llx\n", regs->sp);

	while (i >= 0) {
		pr_emerg("x%-2zd: %016llx ", i, regs->regs[i]);
		i--;

		if (i % 2 == 0) {
			pr_cont("x%-2zd: %016llx ", i, regs->regs[i]);
			i--;
		}

		pr_cont("\n");
	}
}
#else
static inline int ___qc_memdbg_instruction_read(void *addr, u32 *insnp) { return -ENODEV; }
static inline void sec_qc_memdbg_dump_instr(const char *rname, u64 instr) {}
static inline void sec_qc_memdbg_show_regs_min(struct pt_regs *regs) {}
#endif

#endif /* __SEC_QC_MEM_DEBUG_H__ */
