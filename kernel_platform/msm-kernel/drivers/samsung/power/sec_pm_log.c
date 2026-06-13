// SPDX-License-Identifier: GPL-2.0
/*
 *  sec_pm_log.c - SAMSUNG Power/Thermal logging.
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/suspend.h>
#include <linux/sched/clock.h>

#include <linux/sec_pm_log.h>

/* log for pm team */
#define PROC_PM_DIR		"sec_pm_log"
static struct proc_dir_entry *procfs_pm_dir;

/* power/thermal log */
#define POWER_LOG_NAME		"power_log"
#define THERMAL_LOG_NAME	"thermal_log"
#define DCVSH_LOG_NAME		"dcvsh_log"
#define	PM_LOG_BUF_SIZE		SZ_128K
#define MAX_STR_LEN		256
static bool proc_init_done __read_mostly;

/* monitoring log */
static struct delayed_work power_log_work;
static DEFINE_MUTEX(power_log_lock);

/* proc log implementation */
#define SEC_PM_LOG(name)						\
static char name##_log_buf[PM_LOG_BUF_SIZE];				\
static unsigned int name##_buf_pos;					\
static unsigned int name##_buf_full;					\
									\
void ss_##name##_print(const char *fmt, ...)				\
{									\
	char buf[MAX_STR_LEN] = {0, };					\
	unsigned int len = 0;						\
	va_list args;							\
	u64 time;							\
	unsigned long nsec;						\
									\
	if (!proc_init_done)						\
		return;							\
									\
	/* timestamp */							\
	time = local_clock();						\
	nsec = do_div(time, NSEC_PER_SEC);				\
	len = snprintf(buf, sizeof(buf), "[%6lu.%06ld] ",		\
			(unsigned long)time, nsec / NSEC_PER_USEC);	\
									\
	/* append log */						\
	va_start(args, fmt);						\
	len += vsnprintf(buf + len, MAX_STR_LEN - len, fmt, args);	\
	va_end(args);							\
									\
	/* buffer full, reset position */				\
	if (name##_buf_pos + len + 1 > PM_LOG_BUF_SIZE) {		\
		name##_buf_pos = 0;					\
		name##_buf_full++;					\
	}								\
									\
	name##_buf_pos += scnprintf(&name##_log_buf[name##_buf_pos],	\
						len + 1, "%s", buf);	\
}									\
EXPORT_SYMBOL(ss_##name##_print);					\
									\
static ssize_t ss_##name##_log_read(struct file *file, char __user *buf,	\
						size_t len, loff_t *offset)	\
{										\
	loff_t pos = *offset;							\
	ssize_t count = 0;							\
	size_t size = (name##_buf_full > 0) ? PM_LOG_BUF_SIZE : (size_t)name##_buf_pos;	\
										\
	pr_info("%s: pos(%d), full(%d), size(%lu)\n", __func__,			\
				name##_buf_pos, name##_buf_full, size);		\
										\
	if (pos >= size)							\
		return 0;							\
										\
	count = min(len, size);						\
										\
	if ((pos + count) > size)						\
		count = size - pos;						\
										\
	if (copy_to_user(buf, name##_log_buf + pos, count))			\
		return -EFAULT;							\
										\
	*offset += count;							\
										\
	pr_info("%s: done\n", __func__);					\
										\
	return count;								\
}										\
										\
static const struct proc_ops proc_ss_##name##_log_ops = {	\
	.proc_read = ss_##name##_log_read,			\
};

/* all-in-one define */
SEC_PM_LOG(power);
SEC_PM_LOG(thermal);
SEC_PM_LOG(dcvsh);


/* monitoring log implementation */
#define POLLING_DELAY	(HZ * 5)
static void __ref power_log_print(struct work_struct *work)
{
	char power_log[MAX_STR_LEN];

	mutex_lock(&power_log_lock);
	pm_get_active_wakeup_sources(power_log, MAX_STR_LEN);
	mutex_unlock(&power_log_lock);

	ss_power_print("pmon: %s\n", power_log);

	schedule_delayed_work(&power_log_work, POLLING_DELAY);
}

void sec_pm_proc_log_init(void)
{
	struct proc_dir_entry *entry;

	procfs_pm_dir = proc_mkdir(PROC_PM_DIR, NULL);
	if (unlikely(!procfs_pm_dir)) {
		pr_err("%s: failed to make %s\n", __func__, PROC_PM_DIR);
		return;
	}

	/* proc power */
	entry = proc_create(POWER_LOG_NAME, 0444,
			procfs_pm_dir, &proc_ss_power_log_ops);
	if (unlikely(!entry)) {
		pr_err("%s: proc power log fail\n", __func__);
	} else {
		proc_set_size(entry, PM_LOG_BUF_SIZE);
		ss_power_print("%s done\n", __func__);
		proc_init_done = true;
	}

	/* proc thermal */
	entry = proc_create(THERMAL_LOG_NAME, 0444,
				procfs_pm_dir,
				&proc_ss_thermal_log_ops);
	if (unlikely(!entry)) {
		pr_err("%s: proc thermal log fail\n", __func__);
	} else {
		proc_set_size(entry, PM_LOG_BUF_SIZE);
		ss_thermal_print("%s done\n", __func__);
		proc_init_done = true;
	}

	/* proc dcvsh */
	entry = proc_create(DCVSH_LOG_NAME, 0444,
				procfs_pm_dir,
				&proc_ss_dcvsh_log_ops);
	if (unlikely(!entry)) {
		pr_err("%s: proc dcvsh log fail\n", __func__);
	} else {
		proc_set_size(entry, PM_LOG_BUF_SIZE);
		ss_dcvsh_print("%s done\n", __func__);
		proc_init_done = true;
	}

	if (!proc_init_done)
		remove_proc_subtree(PROC_PM_DIR, NULL);
	else
		pr_info("%s: proc init done\n", __func__);
}

static int sec_pm_log_notify(struct notifier_block *nb,
			     unsigned long mode, void *_unused)
{
	struct timespec64 ts;
	struct rtc_time tm;

	ktime_get_real_ts64(&ts);
	rtc_time64_to_tm(ts.tv_sec, &tm);

	switch (mode) {
	case PM_SUSPEND_PREPARE:
		ss_power_print("soc: suspend(%d-%02d-%02d %02d:%02d:%02d.%03lu)\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
		/* TEMP: remarked
		 * cancel_delayed_work(&power_log_work);
		 */
		break;
	case PM_POST_SUSPEND:
		/* TEMP: remarked
		 * schedule_delayed_work(&power_log_work, 0);
		 */
		ss_power_print("soc: resume(%d-%02d-%02d %02d:%02d:%02d.%03lu)\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block sec_pm_log_nb = {
	.notifier_call = sec_pm_log_notify,
};

static int __init sec_pm_log_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	/* proc log */
	sec_pm_proc_log_init();

	/* monitoring log */
	INIT_DELAYED_WORK(&power_log_work, power_log_print);

	/* TEMP: remarked
	 * schedule_delayed_work(&power_log_work, 0);
	 */

	pr_info("%s: monitor init done\n", __func__);

	/* pm notifier */
	ret = register_pm_notifier(&sec_pm_log_nb);
	if (ret)
		pr_err("sec_pm_log: pm notifier register fail(%d).\n", ret);

	return 0;
}

static void  __exit sec_pm_log_exit(void)
{
	remove_proc_subtree(POWER_LOG_NAME, procfs_pm_dir);
	remove_proc_subtree(THERMAL_LOG_NAME, procfs_pm_dir);
	remove_proc_subtree(DCVSH_LOG_NAME, procfs_pm_dir);
	remove_proc_subtree(PROC_PM_DIR, NULL);
}

module_init(sec_pm_log_init);
module_exit(sec_pm_log_exit);

MODULE_ALIAS("platform:sec_pm_log");
MODULE_DESCRIPTION("Samsung Power/Thermal Log driver");
MODULE_LICENSE("GPL v2");
