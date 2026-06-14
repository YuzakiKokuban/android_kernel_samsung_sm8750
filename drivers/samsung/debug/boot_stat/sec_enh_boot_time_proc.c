// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/sched/clock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stringhash.h>

#include "sec_boot_stat.h"

static const char *h_line = "-----------------------------------------------------------------------------------";

static __always_inline struct enh_boot_time_entry *__enh_boot_time_find_entry_locked(
		struct enh_boot_time_proc *enh_boot_time,
		const char *message)
{
	struct enh_boot_time_entry *h;
	size_t msg_len = strlen(message);
	u32 key = full_name_hash(NULL, message, (unsigned int)msg_len);

	hash_for_each_possible(enh_boot_time->boot_time_htbl, h, hlist, key) {
		size_t len = strnlen(h->buf, msg_len + 1);

		if (len != msg_len)
			continue;

		if (!strncmp(h->buf, message, msg_len))
			return h;
	}

	return ERR_PTR(-ENOENT);
}

static __always_inline void __enh_boot_time_record_locked(
		struct enh_boot_time_proc *enh_boot_time,
		const char *message)
{
	struct enh_boot_time_entry *entry;
	struct device *dev = __enh_boot_time_proc_to_dev(enh_boot_time);
	struct enh_boot_time_entry *entry_in_hash;
	u32 key;

	entry = devm_kzalloc(dev, sizeof(*entry), GFP_KERNEL);
	if (unlikely(!entry))
		return;

	strlcpy(entry->buf, message, sizeof(entry->buf));
	entry_in_hash = __enh_boot_time_find_entry_locked(enh_boot_time,
			entry->buf);
	if (!IS_ERR(entry_in_hash)) {
		devm_kfree(dev, entry);
		return;
	}

	entry->ktime = local_clock();
	list_add(&entry->list, &enh_boot_time->boot_time_head);

	INIT_HLIST_NODE(&entry->hlist);
	key = full_name_hash(NULL, entry->buf, (unsigned int)strlen(entry->buf));
	hash_add(enh_boot_time->boot_time_htbl, &entry->hlist, key);
}

#define DELAY_KTIME_EBS	30000000000	/* 30 sec */
#define MAX_EVENTS_EBS	150

/* NOTE: BE CAREFUL!!. 2 mutexes are used in this function. */
static __always_inline void __enh_boot_time_update_is_finished(
		struct enh_boot_time_proc *enh_boot_time,
		struct boot_stat_proc *boot_stat)
{
	unsigned long long delay_ktime;

	mutex_lock(&boot_stat->lock);
	if (!boot_stat->is_completed) {
		mutex_unlock(&boot_stat->lock);
		return;
	}
	mutex_unlock(&boot_stat->lock);

	mutex_lock(&enh_boot_time->lock);
	if (enh_boot_time->is_finished) {
		mutex_unlock(&enh_boot_time->lock);
		return;
	}

	/* NOTE: after 'boot_stat->is_completed' is set, 'ktime_completed'
	 * is never changed anymore. So, at this point, lock in not needed.
	 */
	delay_ktime = local_clock() - boot_stat->ktime_completed;

	if (delay_ktime >= DELAY_KTIME_EBS)
		enh_boot_time->is_finished = true;

	mutex_unlock(&enh_boot_time->lock);
}

static __always_inline void __enh_boot_time_add_boot_event_locked(
		struct enh_boot_time_proc *enh_boot_time, const char *log)
{
	const struct {
		char pmsg_mark[2];	/* !@ */
		uint64_t boot_prefix;
		char colon;
	} __packed *log_head;

	if (enh_boot_time->is_finished ||
	    enh_boot_time->nr_event >= MAX_EVENTS_EBS)
		return;

	log_head = (void *)log;
	if (log_head->colon == ':') {
		const size_t offset = sizeof("!@Boot_EBS: ") - 1;
		__enh_boot_time_record_locked(enh_boot_time, &log[offset]);
	} else if (log_head->colon == '_') {
		__enh_boot_time_record_locked(enh_boot_time, log);
	} else {
		return;
	}

	enh_boot_time->nr_event++;
}

void sec_enh_boot_time_add_boot_event(
	struct boot_stat_drvdata *drvdata, const char *log)
{
	struct enh_boot_time_proc *enh_boot_time = &drvdata->enh_boot_time;
	struct boot_stat_proc *boot_stat = &drvdata->boot_stat;

	__enh_boot_time_update_is_finished(enh_boot_time, boot_stat);

	mutex_lock(&enh_boot_time->lock);
	__enh_boot_time_add_boot_event_locked(enh_boot_time, log);
	mutex_unlock(&enh_boot_time->lock);
}

static unsigned long long __enh_boot_time_show_framework_each_locked(
		struct seq_file *m, struct enh_boot_time_entry *entry,
		unsigned long long prev_ktime)
{
	unsigned long long msec;
	unsigned long long delta;
	unsigned long long curr_ktime = entry->ktime;
	unsigned long long time;

	msec = curr_ktime;
	do_div(msec, 1000000ULL);

	time = sec_boot_stat_ktime_to_time(curr_ktime);
	do_div(time, 1000000ULL);

	if (entry->buf[0] == '!') {
		delta = curr_ktime - prev_ktime;
		do_div(delta, 1000000ULL);

		seq_printf(m, "%-90s%7llu%7llu%7llu\n", entry->buf,
				time, msec, delta);
	} else {
		seq_printf(m, "%-90s%7llu%7llu\n", entry->buf,
				time, msec);
		curr_ktime = prev_ktime;
	}

	return curr_ktime;
}

static void __enh_boot_time_show_framework_locked(struct seq_file *m,
		struct enh_boot_time_proc *enh_boot_time)
{
	struct list_head *head = &enh_boot_time->boot_time_head;
	struct enh_boot_time_entry *entry;
	unsigned long long prev_ktime = 0;

	seq_printf(m, "%-90s%7s%7s%7s\n", "Boot Events", "time", "ktime", "delta");
	seq_printf(m, "%s\n", h_line);
	seq_puts(m, "BOOTLOADER\n");
	seq_printf(m, "%s\n", h_line);
	sec_boot_stat_bootloader_stat(m);

	seq_printf(m, "%s\n", h_line);
	seq_puts(m, "FRAMEWORK\n");
	seq_printf(m, "%s\n", h_line);

	prev_ktime = 0;
	list_for_each_entry_reverse (entry, head, list) {
		prev_ktime = __enh_boot_time_show_framework_each_locked(m,
				entry, prev_ktime);
	}
}

static void __enh_boot_time_show_framework(struct seq_file *m,
		struct enh_boot_time_proc *enh_boot_time)
{
	mutex_lock(&enh_boot_time->lock);
	__enh_boot_time_show_framework_locked(m, enh_boot_time);
	mutex_unlock(&enh_boot_time->lock);
}

static void __enh_boot_time_show_soc(struct seq_file *m,
		struct enh_boot_time_proc *enh_boot_time)
{
	struct boot_stat_drvdata *drvdata = container_of(enh_boot_time,
			struct boot_stat_drvdata, enh_boot_time);
	struct sec_boot_stat_soc_operations *soc_ops;

	mutex_lock(&drvdata->soc_ops_lock);

	soc_ops = drvdata->soc_ops;
	if (!soc_ops || !soc_ops->show_on_enh_boot_time) {
		mutex_unlock(&drvdata->soc_ops_lock);
		return;
	}

	soc_ops->show_on_enh_boot_time(m);
	mutex_unlock(&drvdata->soc_ops_lock);
}

static int sec_enh_boot_time_proc_show(struct seq_file *m, void *v)
{
	struct enh_boot_time_proc *enh_boot_time = m->private;

	__enh_boot_time_show_soc(m, enh_boot_time);
	__enh_boot_time_show_framework(m, enh_boot_time);

	return 0;
}

static int sec_enh_boot_time_proc_open(struct inode *inode, struct file *file)
{
	void *__enh_boot_time = pde_data(inode);

	return single_open(file, sec_enh_boot_time_proc_show, __enh_boot_time);
}

static const struct proc_ops enh_boot_time_pops = {
	.proc_open = sec_enh_boot_time_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int __enh_boot_time_procfs_init(struct device *dev,
		struct enh_boot_time_proc *enh_boot_time)
{
	struct proc_dir_entry *proc;
	const char *node_name = "enhanced_boot_stat";

	proc = proc_create_data(node_name, 0444, NULL, &enh_boot_time_pops,
			enh_boot_time);
	if (!proc) {
		dev_err(dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	enh_boot_time->proc = proc;

	return 0;
}

static void __enh_boot_time_procfs_exit(struct device *dev,
		struct enh_boot_time_proc *enh_boot_time)
{
	proc_remove(enh_boot_time->proc);
}

int sec_enh_boot_time_init(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *dev = bd->dev;
	struct enh_boot_time_proc *enh_boot_time = &drvdata->enh_boot_time;
	int err;

	mutex_init(&enh_boot_time->lock);
	INIT_LIST_HEAD(&enh_boot_time->boot_time_head);
	hash_init(enh_boot_time->boot_time_htbl);

	if (IS_MODULE(CONFIG_SEC_BOOT_STAT))
		sec_enh_boot_time_add_boot_event(drvdata,
				"!@Boot_EBS_F: FirstStageMain Init");

	err = __enh_boot_time_procfs_init(dev, enh_boot_time);
	if (err)
		return err;

	return 0;
}

void sec_enh_boot_time_exit(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *dev = bd->dev;
	struct enh_boot_time_proc *enh_boot_time = &drvdata->enh_boot_time;

	__enh_boot_time_procfs_exit(dev, enh_boot_time);
	mutex_destroy(&enh_boot_time->lock);
}
