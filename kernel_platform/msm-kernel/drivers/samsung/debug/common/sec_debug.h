#ifndef __INTERNAL__SEC_DEBUG_H__
#define __INTERNAL__SEC_DEBUG_H__

#include <linux/debugfs.h>
#include <linux/hashtable.h>
#include <linux/mutex.h>
#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>

struct svc_ap_node {
	struct kobject *svc_kobj;
	struct device *ap_dev;
};

#define FORCE_ERR_HASH_BITS	3

struct force_err {
	struct mutex lock;
	DECLARE_HASHTABLE(htbl, FORCE_ERR_HASH_BITS);
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbgfs;
#endif
};

struct sec_debug_drvdata {
	struct builder bd;
	struct svc_ap_node svc_ap;
	bool in_panic;
	struct notifier_block nb_panic;
	struct notifier_block nb_restart;
#if IS_ENABLED(CONFIG_SEC_FORCE_ERR)
	struct force_err force_err;
#endif
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbgfs_panic;
#endif
};

extern struct sec_debug_drvdata *sec_debug;

static __always_inline bool __debug_is_probed(void)
{
	return !!sec_debug;
}

/* sec_user_fault.c */
extern int sec_user_fault_init(struct builder *bd);
extern void sec_user_fault_exit(struct builder *bd);

/* sec_ap_serial.c */
extern int sec_ap_serial_sysfs_init(struct builder *bd);
extern void sec_ap_serial_sysfs_exit(struct builder *bd);

/* sec_force_err.c */
extern int sec_force_err_probe_prolog(struct builder *bd);
extern void sec_force_err_remove_epilog(struct builder *bd);
extern int sec_force_err_build_htbl(struct builder *bd);
extern int sec_force_err_debugfs_create(struct builder *bd);
extern void sec_force_err_debugfs_remove(struct builder *bd);

/* sec_debug_show_stat.c */
extern void sec_debug_show_stat(const char *msg);

/* sec_debug_node.c */
extern int sec_debug_node_init_dump_sink(struct builder *bd);

/* sec_panic_with_reason.c */
extern int sec_panic_with_reason_init(struct builder *bd);
extern void sec_panic_with_reason_exit(struct builder *bd);

#endif /* __INTERNAL__SEC_DEBUG_H__ */
