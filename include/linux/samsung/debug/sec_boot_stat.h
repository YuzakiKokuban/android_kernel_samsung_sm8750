#ifndef __SEC_BOOT_STATH_H__
#define __SEC_BOOT_STATH_H__

#include <linux/seq_file.h>

struct sec_boot_stat_soc_operations {
	unsigned long long (*ktime_to_time)(unsigned long long ktime);
	void (*show_on_boot_stat)(struct seq_file *m);
	void (*show_on_enh_boot_time)(struct seq_file *m);
	void (*show_on_enh_boot_stat)(struct seq_file *m);
};

#if IS_ENABLED(CONFIG_SEC_BOOT_STAT)
extern void sec_boot_stat_add(const char *log);
extern int sec_boot_stat_register_soc_ops(struct sec_boot_stat_soc_operations *soc_ops);
extern int sec_boot_stat_unregister_soc_ops(struct sec_boot_stat_soc_operations *soc_ops);
#else
static inline void sec_boot_stat_add(const char *log) {}
static inline int sec_boot_stat_register_soc_ops(struct sec_boot_stat_soc_operations *soc_ops) { return -ENODEV; }
static inline int sec_boot_stat_unregister_soc_ops(struct sec_boot_stat_soc_operations *soc_ops) { return -ENODEV; }
#endif

#endif /* __SEC_BOOT_STATH_H__ */
