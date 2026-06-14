/*
 * sb_sysfs.h
 * Samsung Mobile SysFS Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SB_SYSFS_H
#define __SB_SYSFS_H __FILE__

#include <linux/err.h>
#include <linux/device.h>

#define SB_SYSFS_DISABLE	(-3662)
#if IS_ENABLED(CONFIG_SB_SYSFS)
int sb_sysfs_add_attrs(const char *name, void *pdata, struct device_attribute *attr, unsigned long size);
int sb_sysfs_remove_attrs(const char *name);
void *sb_sysfs_get_pdata(const char *name);
int sb_sysfs_create_attrs(struct device *dev);
#else
static inline int sb_sysfs_add_attrs(const char *name, void *pdata, struct device_attribute *attr, unsigned long size)
{ return SB_SYSFS_DISABLE; }
static inline int sb_sysfs_remove_attrs(const char *name)
{ return SB_SYSFS_DISABLE; }
static inline void *sb_sysfs_get_pdata(const char *name)
{ return ERR_PTR(SB_SYSFS_DISABLE); }
static inline int sb_sysfs_create_attrs(struct device *dev)
{ return SB_SYSFS_DISABLE; }
#endif

#endif /* __SB_SYSFS_H */

