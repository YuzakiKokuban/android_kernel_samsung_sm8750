#ifndef __SEC_RISCV64_AP_CONTEXT_H__
#define __SEC_RISCV64_AP_CONTEXT_H__

#include <dt-bindings/samsung/debug/sec_riscv64_vh_ipi_stop.h>

enum {
	ID_RV64_CSR_SATP = 0,
	/* */
	IDX_RV64_NR_CSR_REG,
};

struct sec_riscv64_ap_context {
	struct pt_regs core_regs;
	uint64_t csr_regs[IDX_RV64_NR_CSR_REG];
	bool used;
};

#endif /* __SEC_RISCV64_AP_CONTEXT_H__ */
