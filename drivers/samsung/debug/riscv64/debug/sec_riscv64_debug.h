#ifndef __INTERNAL__SEC_RISCV64_DEBUG_H__
#define __INTERNAL__SEC_RISCV64_DEBUG_H__

#include <linux/debugfs.h>
#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>

struct riscv64_debug_drvdata {
	struct builder bd;
};

/* sec_riscv64_force_err.c */
extern int sec_riscv64_force_err_init(struct builder *bd);
extern void sec_riscv64_force_err_exit(struct builder *bd);

#endif /* __INTERNAL__SEC_RISCV64_DEBUG_H__ */
