// SPDX-License-Identifier: GPL-2.0
/*
 * sec_mm/
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 */

#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/sec_mm.h>
#include <linux/sysfs.h>
#include <linux/vmstat.h>

ATOMIC_NOTIFIER_HEAD(am_app_launch_notifier);
EXPORT_SYMBOL_GPL(am_app_launch_notifier);

bool am_app_launch;
EXPORT_SYMBOL_GPL(am_app_launch);

#ifdef CONFIG_SYSFS
#define MEM_BOOST_MAX_TIME (5 * HZ) /* 5 sec */
/* mem_boost throttles only kswapd's behavior */
enum mem_boost {
	NO_BOOST,
	BOOST_MID = 1,
	BOOST_HIGH = 2,
	BOOST_KILL = 3,
};

static int mem_boost_mode = NO_BOOST;
static unsigned long last_mode_change;

bool mem_boost_mode_high(void)
{
	if (time_after(jiffies, last_mode_change + MEM_BOOST_MAX_TIME))
		mem_boost_mode = NO_BOOST;
	return mem_boost_mode >= BOOST_HIGH;
}
EXPORT_SYMBOL_GPL(mem_boost_mode_high);

static ssize_t mem_boost_mode_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	if (time_after(jiffies, last_mode_change + MEM_BOOST_MAX_TIME))
		mem_boost_mode = NO_BOOST;
	return sprintf(buf, "%d\n", mem_boost_mode);
}

static ssize_t mem_boost_mode_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int mode, err;

	err = kstrtoint(buf, 10, &mode);
	if (err || mode > BOOST_KILL || mode < NO_BOOST)
		return -EINVAL;
	mem_boost_mode = mode;
	last_mode_change = jiffies;
#ifdef CONFIG_RBIN
	if (mem_boost_mode >= BOOST_HIGH)
		wake_dmabuf_rbin_heap_prereclaim();
#endif

	return count;
}

static struct kobj_attribute mem_boost_mode_attr = __ATTR_RW(mem_boost_mode);

static ssize_t am_app_launch_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", am_app_launch ? 1 : 0);
}

static ssize_t am_app_launch_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int mode, err;
	bool am_app_launch_new;

	err = kstrtoint(buf, 10, &mode);
	if (err || (mode != 0 && mode != 1))
		return -EINVAL;

	am_app_launch_new = mode ? true : false;
	if (am_app_launch != am_app_launch_new)
		atomic_notifier_call_chain(&am_app_launch_notifier, mode, NULL);
	am_app_launch = am_app_launch_new;

	return count;
}

static struct kobj_attribute am_app_launch_attr = __ATTR_RW(am_app_launch);

static ssize_t mmap_readaround_limit_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", mmap_readaround_limit);
}

static struct kobj_attribute mmap_readaround_limit_attr = __ATTR_RO(mmap_readaround_limit);

static struct attribute *sec_mm_attrs[] = {
	&mem_boost_mode_attr.attr,
	&am_app_launch_attr.attr,
	&mmap_readaround_limit_attr.attr,
	NULL,
};

static struct attribute_group sec_mm_attr_group = {
	.attrs = sec_mm_attrs,
	.name = "sec_mm",
};

void init_sec_mm_sysfs(void)
{
	if (sysfs_create_group(kernel_kobj, &sec_mm_attr_group))
		pr_err("sec_mm_sysfs: failed to create\n");
}

void exit_sec_mm_sysfs(void)
{
}
#else
void init_sec_mm_sysfs(void)
{
}

void exit_sec_mm_sysfs(void);
{
}
#endif

