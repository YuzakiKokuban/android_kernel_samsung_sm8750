#ifndef __INTERNAL__SEC_BOOT_STAT_H__
#define __INTERNAL__SEC_BOOT_STAT_H__

#include <linux/hashtable.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_boot_stat.h>

#include "sec_boot_stat_proc.h"
#include "sec_enh_boot_time_proc.h"

struct boot_stat_drvdata {
	struct builder bd;
	struct device *bsp_dev;
	struct mutex soc_ops_lock;
	struct sec_boot_stat_soc_operations *soc_ops;
	struct boot_stat_proc boot_stat;
	struct enh_boot_time_proc enh_boot_time;
};

static __always_inline struct device *__boot_stat_proc_to_dev(
		struct boot_stat_proc *boot_stat)
{
	struct boot_stat_drvdata *drvdata = container_of(boot_stat,
			struct boot_stat_drvdata, boot_stat);

	return drvdata->bd.dev;
}

static __always_inline struct device *__enh_boot_time_proc_to_dev(
		struct enh_boot_time_proc *enh_boot_time)
{
	struct boot_stat_drvdata *drvdata = container_of(enh_boot_time,
			struct boot_stat_drvdata, enh_boot_time);

	return drvdata->bd.dev;
}

/* sec_boot_stata_main.c */
extern unsigned long long sec_boot_stat_ktime_to_time(unsigned long long ktime);
extern void sec_boot_stat_bootloader_stat(struct seq_file *m);

/* sec_boot_stat_proc.c */
extern void sec_boot_stat_add_boot_event(struct boot_stat_drvdata *drvdata, const char *log);
extern int sec_boot_stat_proc_init(struct builder *bd);
extern void sec_boot_stat_proc_exit(struct builder *bd);

/* sec_enh_boot_time_proc.c */
extern void sec_enh_boot_time_add_boot_event(struct boot_stat_drvdata *drvdata, const char *log);
extern int sec_enh_boot_time_init(struct builder *bd);
extern void sec_enh_boot_time_exit(struct builder *bd);

#endif /* __INTERNAL__SEC_BOOT_STAT_H__ */
