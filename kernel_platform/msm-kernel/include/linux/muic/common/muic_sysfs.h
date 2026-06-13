#ifndef MUIC_SYSFS_H
#define MUIC_SYSFS_H

#include <linux/muic/common/muic.h>

#ifdef CONFIG_MUIC_COMMON_SYSFS
extern int muic_sysfs_init(struct muic_platform_data *pdata);
extern void muic_sysfs_deinit(struct muic_platform_data *pdata);
#else
static inline int muic_sysfs_init(struct muic_platform_data *pdata)
		{return 0; }
static inline void muic_sysfs_deinit(struct muic_platform_data *pdata) {}
#endif

#endif /* MUIC_SYSFS_H */
