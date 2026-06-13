// SPDX-License-Identifier: GPL-2.0
/*
 *  Samsung Block Write Booster
 *
 *  Copyright (C) 2023 Jisoo Oh <jisoo2146.oh@samsung.com>
 *  Copyright (C) 2023 Changheun Lee <nanich.lee@samsung.com>
 */

#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/part_stat.h>
#include <linux/timer.h>

#include "blk-sec.h"
#include "../drivers/ufs/host/ufs-sec-feature.h"

#define MIN_ENABLE_MS 100
#define MAX_ENABLE_MS 5000

struct blk_sec_wb {
	struct mutex lock;

	volatile unsigned long request;
	unsigned int state;

	struct work_struct ctrl_work;
	struct timer_list user_wb_off_timer;
};

static struct blk_sec_wb wb;

static void notify_wb_change(bool enabled)
{
#define BUF_SIZE 16
	char buf[BUF_SIZE];
	char *envp[] = { "NAME=BLK_SEC_WB", buf, NULL, };
	int ret;

	if (IS_ERR(blk_sec_dev))
		return;

	memset(buf, 0, BUF_SIZE);
	snprintf(buf, BUF_SIZE, "ENABLED=%d", enabled);
	ret = kobject_uevent_env(&blk_sec_dev->kobj, KOBJ_CHANGE, envp);
	if (ret)
		pr_err("%s: couldn't send uevent (%d)", __func__, ret);
}

/*
 * don't call this function in interrupt context,
 * it will be sleep when ufs_sec_wb_ctrl() is called
 *
 * Context: can sleep
 */
static int wb_ctrl(bool enable)
{
	int ret = 0;

	might_sleep();

	mutex_lock(&wb.lock);

	if (enable && (wb.state == WB_ON))
		goto out;

	if (!enable && (wb.state == WB_OFF))
		goto out;

	ret = ufs_sec_wb_ctrl(enable);
	if (ret)
		goto out;

	if (enable)
		wb.state = WB_ON;
	else
		wb.state = WB_OFF;

	notify_wb_change(enable);

out:
	mutex_unlock(&wb.lock);
	return ret;
}

static void wb_ctrl_work(struct work_struct *work)
{
	wb_ctrl(!!wb.request);
}

static void user_wb_off_handler(struct timer_list *timer)
{
	clear_bit(WB_REQ_USER, &wb.request);
	queue_work(blk_sec_common_wq, &wb.ctrl_work);
}

static void ufs_reset_notify(void)
{
	queue_work(blk_sec_common_wq, &wb.ctrl_work);
}

int blk_sec_wb_ctrl(bool enable, int req_type)
{
	if (req_type < 0 || req_type >= NR_WB_REQ_TYPE)
		return -EINVAL;

	if (enable)
		set_bit(req_type, &wb.request);
	else
		clear_bit(req_type, &wb.request);

	return wb_ctrl(!!wb.request);
}
EXPORT_SYMBOL(blk_sec_wb_ctrl);

int blk_sec_wb_ctrl_async(bool enable, int req_type)
{
	if (req_type < 0 || req_type >= NR_WB_REQ_TYPE)
		return -EINVAL;

	if (enable)
		set_bit(req_type, &wb.request);
	else
		clear_bit(req_type, &wb.request);

	queue_work(blk_sec_common_wq, &wb.ctrl_work);
	return 0;
}
EXPORT_SYMBOL(blk_sec_wb_ctrl_async);

bool blk_sec_wb_is_supported(struct gendisk *gd)
{
	if (blk_sec_internal_disk() != gd)
		return false;

	return ufs_sec_is_wb_supported();
}
EXPORT_SYMBOL(blk_sec_wb_is_supported);

static ssize_t request_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lx\n", wb.request);
}

static ssize_t state_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", wb.state);
}

static ssize_t enable_ms_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned long expire_jiffies = wb.user_wb_off_timer.expires;
	unsigned long current_jiffies = jiffies;

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			time_after(expire_jiffies, current_jiffies) ?
			jiffies_to_msecs(expire_jiffies - current_jiffies) : 0);
}

static ssize_t enable_ms_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int wb_on_duration = 0;
	unsigned long expire_jiffies = 0;
	int ret;

	ret = kstrtoint(buf, 10, &wb_on_duration);
	if (ret)
		return ret;

	if (wb_on_duration <= 0)
		return count;

	if (wb_on_duration < MIN_ENABLE_MS)
		wb_on_duration = MIN_ENABLE_MS;
	if (wb_on_duration > MAX_ENABLE_MS)
		wb_on_duration = MAX_ENABLE_MS;

	expire_jiffies = jiffies + msecs_to_jiffies(wb_on_duration);
	if (time_after(expire_jiffies, wb.user_wb_off_timer.expires))
		mod_timer(&wb.user_wb_off_timer, expire_jiffies);

	blk_sec_wb_ctrl(true, WB_REQ_USER);

	return count;
}

static struct kobj_attribute request_attr = __ATTR_RO(request);
static struct kobj_attribute state_attr = __ATTR_RO(state);
static struct kobj_attribute enable_ms_attr =
__ATTR(enable_ms, 0644, enable_ms_show, enable_ms_store);

static const struct attribute *blk_sec_wb_attrs[] = {
	&request_attr.attr,
	&state_attr.attr,
	&enable_ms_attr.attr,
	NULL,
};

static struct kobject *blk_sec_wb_kobj;

static int __init blk_sec_wb_init(void)
{
	int retval;

	blk_sec_wb_kobj = kobject_create_and_add("blk_sec_wb", kernel_kobj);
	if (!blk_sec_wb_kobj)
		return -ENOMEM;

	retval = sysfs_create_files(blk_sec_wb_kobj, blk_sec_wb_attrs);
	if (retval) {
		kobject_put(blk_sec_wb_kobj);
		return retval;
	}

	mutex_init(&wb.lock);
	wb.state = WB_OFF;
	INIT_WORK(&wb.ctrl_work, wb_ctrl_work);
	timer_setup(&wb.user_wb_off_timer, user_wb_off_handler, 0);
	ufs_sec_wb_register_reset_notify(&ufs_reset_notify);

	return 0;
}

static void __exit blk_sec_wb_exit(void)
{
	del_timer_sync(&wb.user_wb_off_timer);
	sysfs_remove_files(blk_sec_wb_kobj, blk_sec_wb_attrs);
	kobject_put(blk_sec_wb_kobj);
}

module_init(blk_sec_wb_init);
module_exit(blk_sec_wb_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jisoo Oh <jisoo2146.oh@samsung.com>");
MODULE_DESCRIPTION("Samsung write booster module in block layer");
MODULE_VERSION("1.0");
