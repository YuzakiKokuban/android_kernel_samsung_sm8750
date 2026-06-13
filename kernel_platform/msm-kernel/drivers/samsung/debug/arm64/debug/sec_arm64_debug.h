#ifndef __INTERNAL__SEC_ARM64_DEBUG_H__
#define __INTERNAL__SEC_ARM64_DEBUG_H__

#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>

struct arm64_debug_drvdata {
	struct builder bd;
};

/* sec_arm64_force_err.c */
extern int sec_fsimd_debug_init_random_pi_work(struct builder *bd);
extern int sec_arm64_force_err_init(struct builder *bd);
extern void sec_arm64_force_err_exit(struct builder *bd);

#endif /* __INTERNAL__SEC_ARM64_DEBUG_H__ */
