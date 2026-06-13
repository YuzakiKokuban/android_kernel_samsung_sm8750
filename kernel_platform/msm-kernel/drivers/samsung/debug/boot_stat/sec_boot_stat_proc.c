// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/sched/clock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/stringhash.h>

#include <linux/samsung/sec_kunit.h>

#include "sec_boot_stat.h"

static const char *h_line = "-----------------------------------------------------------------------------------";

#define BOOT_PREFIX(__idx, __head) \
	[__idx] = { \
		.head = __head, \
		.head_len = sizeof(__head) - 1, \
	}

static const struct boot_prefix boot_prefixes[] = {
	BOOT_PREFIX(EVT_PLATFORM, "!@Boot: "),
	BOOT_PREFIX(EVT_RIL, "!@Boot_SVC : "),
	BOOT_PREFIX(EVT_DEBUG, "!@Boot_DEBUG: "),
	BOOT_PREFIX(EVT_SYSTEMSERVER, "!@Boot_SystemServer: "),
	BOOT_PREFIX(EVT_INVALID, ""),
};

enum {
	SYSTEM_START_INIT_PROCESS,
	PLATFORM_START_PRELOAD,
	PLATFORM_END_PRELOAD,
	PLATFORM_START_INIT_AND_LOOP,
	PLATFORM_START_PACKAGEMANAGERSERVICE,
	PLATFORM_END_PACKAGEMANAGERSERVICE,
	PLATFORM_START_NETWORK,
	PLATFORM_END_NETWORK,
	PLATFORM_END_INIT_AND_LOOP,
	PLATFORM_PERFORMENABLESCREEN,
	PLATFORM_ENABLE_SCREEN,
	PLATFORM_BOOT_COMPLETE,
	PLATFORM_FINISH_USER_UNLOCKED_COMPLETED,
	PLATFORM_SET_ICON_VISIBILITY,
	PLATFORM_LAUNCHER_ONCREATE,
	PLATFORM_LAUNCHER_ONRESUME,
	PLATFORM_LAUNCHER_LOADERTASK_RUN,
	PLATFORM_LAUNCHER_FINISHFIRSTBIND,
	PLATFORM_VOICE_SVC,
	PLATFORM_DATA_SVC,
	PLATFORM_PHONEAPP_ONCREATE,
	RIL_UNSOL_RIL_CONNECTED,
	RIL_SETRADIOPOWER_ON,
	RIL_SETUICCSUBSCRIPTION,
	RIL_SIM_RECORDSLOADED,
	RIL_RUIM_RECORDSLOADED,
	RIL_SETUPDATA_RECORDSLOADED,
	RIL_SETUPDATACALL,
	RIL_RESPONSE_SETUPDATACALL,
	RIL_DATA_CONNECTION_ATTACHED,
	RIL_DCT_IMSI_READY,
	RIL_COMPLETE_CONNECTION,
	RIL_CS_REG,
	RIL_GPRS_ATTACH,
	FACTORY_BOOT_COMPLETE,
	NUM_BOOT_EVENTS,
};

#define BOOT_EVENT(__idx, __prefix_idx, __message) \
	[__idx] = { \
		.prefix_idx = __prefix_idx, \
		.message = __message, \
		.message_len = sizeof(__message) - 1, \
	}

__ss_static struct boot_event boot_events[] = {
	BOOT_EVENT(SYSTEM_START_INIT_PROCESS, EVT_PLATFORM, "start init process"),
	BOOT_EVENT(PLATFORM_START_PRELOAD, EVT_PLATFORM, "Begin of preload()"),
	BOOT_EVENT(PLATFORM_END_PRELOAD, EVT_PLATFORM, "End of preload()"),
	BOOT_EVENT(PLATFORM_START_INIT_AND_LOOP, EVT_PLATFORM, "Entered the Android system server!"),
	BOOT_EVENT(PLATFORM_START_PACKAGEMANAGERSERVICE, EVT_PLATFORM, "Start PackageManagerService"),
	BOOT_EVENT(PLATFORM_END_PACKAGEMANAGERSERVICE, EVT_PLATFORM, "End PackageManagerService"),
	BOOT_EVENT(PLATFORM_START_NETWORK, EVT_DEBUG, "start networkManagement"),
	BOOT_EVENT(PLATFORM_END_NETWORK, EVT_DEBUG, "end networkManagement"),
	BOOT_EVENT(PLATFORM_END_INIT_AND_LOOP, EVT_PLATFORM, "Loop forever"),
	BOOT_EVENT(PLATFORM_PERFORMENABLESCREEN, EVT_PLATFORM, "performEnableScreen"),
	BOOT_EVENT(PLATFORM_ENABLE_SCREEN, EVT_PLATFORM, "Enabling Screen!"),
	BOOT_EVENT(PLATFORM_BOOT_COMPLETE, EVT_PLATFORM, "bootcomplete"),
	BOOT_EVENT(PLATFORM_FINISH_USER_UNLOCKED_COMPLETED, EVT_DEBUG, "finishUserUnlockedCompleted"),
	BOOT_EVENT(PLATFORM_SET_ICON_VISIBILITY, EVT_PLATFORM, "setIconVisibility: ims_volte: [SHOW]"),
	BOOT_EVENT(PLATFORM_LAUNCHER_ONCREATE, EVT_DEBUG, "Launcher.onCreate()"),
	BOOT_EVENT(PLATFORM_LAUNCHER_ONRESUME, EVT_DEBUG, "Launcher.onResume()"),
	BOOT_EVENT(PLATFORM_LAUNCHER_LOADERTASK_RUN, EVT_DEBUG, "Launcher.LoaderTask.run() start"),
	BOOT_EVENT(PLATFORM_LAUNCHER_FINISHFIRSTBIND, EVT_DEBUG, "Launcher - FinishFirstBind"),
	BOOT_EVENT(PLATFORM_VOICE_SVC, EVT_PLATFORM, "Voice SVC is acquired"),
	BOOT_EVENT(PLATFORM_DATA_SVC, EVT_PLATFORM, "Data SVC is acquired"),
	BOOT_EVENT(PLATFORM_PHONEAPP_ONCREATE, EVT_RIL, "PhoneApp OnCrate"),
	BOOT_EVENT(RIL_UNSOL_RIL_CONNECTED, EVT_RIL, "RIL_UNSOL_RIL_CONNECTED"),
	BOOT_EVENT(RIL_SETRADIOPOWER_ON, EVT_RIL, "setRadioPower on"),
	BOOT_EVENT(RIL_SETUICCSUBSCRIPTION, EVT_RIL, "setUiccSubscription"),
	BOOT_EVENT(RIL_SIM_RECORDSLOADED, EVT_RIL, "SIM onAllRecordsLoaded"),
	BOOT_EVENT(RIL_RUIM_RECORDSLOADED, EVT_RIL, "RUIM onAllRecordsLoaded"),
	BOOT_EVENT(RIL_SETUPDATA_RECORDSLOADED, EVT_RIL, "SetupDataRecordsLoaded"),
	BOOT_EVENT(RIL_SETUPDATACALL, EVT_RIL, "setupDataCall"),
	BOOT_EVENT(RIL_RESPONSE_SETUPDATACALL, EVT_RIL, "Response setupDataCall"),
	BOOT_EVENT(RIL_DATA_CONNECTION_ATTACHED, EVT_RIL, "onDataConnectionAttached"),
	BOOT_EVENT(RIL_DCT_IMSI_READY, EVT_RIL, "IMSI Ready"),
	BOOT_EVENT(RIL_COMPLETE_CONNECTION, EVT_RIL, "completeConnection"),
	BOOT_EVENT(RIL_CS_REG, EVT_RIL, "CS Registered"),
	BOOT_EVENT(RIL_GPRS_ATTACH, EVT_RIL, "GPRS Attached"),
	BOOT_EVENT(FACTORY_BOOT_COMPLETE, EVT_PLATFORM, "Factory Process [Boot Completed]"),
};

__ss_static __ss_always_inline bool __boot_stat_is_boot_event(const char *log)
{
	const union {
		uint64_t raw;
		char text[8];
	} boot_prefix = {
		.text = { '!', '@', 'B', 'o', 'o', 't', '\0', '\0' },
	};
	/* NOTE: this is only valid on the 'little endian' system */
	const uint64_t boot_prefix_mask = 0x0000FFFFFFFFFFFF;
	uint64_t log_prefix;

	log_prefix = (*(uint64_t *)log) & boot_prefix_mask;
	if (log_prefix == boot_prefix.raw)
		return true;

	return false;
}

__ss_static __ss_always_inline ssize_t __boot_stat_get_message_offset_from_plog(
		const char *log, size_t *offset)
{
	ssize_t i;

	for (i = 0; i < NUM_OF_BOOT_PREFIX; i++) {
		const struct boot_prefix *prefix = &boot_prefixes[i];

		if (unlikely(!strncmp(log, prefix->head, prefix->head_len))) {
			*offset = prefix->head_len;
			return i;
		}
	}

	return -EINVAL;
}

static __always_inline struct boot_event *__boot_stat_find_event_locked(
		struct boot_stat_proc *boot_stat,
		const char *message)
{
	struct boot_event *h;
	size_t msg_len = strlen(message);
	u32 key = full_name_hash(NULL, message, (unsigned int)msg_len);

	hash_for_each_possible(boot_stat->event_htbl, h, hlist, key) {
		if (h->message_len != msg_len)
			continue;

		if (!strncmp(h->message, message, msg_len))
			return h;
	}

	return ERR_PTR(-ENOENT);
}

__ss_static __ss_always_inline void __boot_stat_record_boot_event_locked(
		struct boot_stat_proc *boot_stat, const char *message)
{
	struct boot_event *event =
			__boot_stat_find_event_locked(boot_stat, message);

	if (IS_ERR_OR_NULL(event))
		return;

	if (event->ktime)
		return;

	event->ktime = local_clock();

	list_add_tail(&event->list, &boot_stat->boot_event_head);
	boot_stat->nr_event++;
}

#define MAX_LENGTH_OF_SYSTEMSERVER_LOG 90

struct systemserver_init_time_entry {
	struct list_head list;
	char buf[MAX_LENGTH_OF_SYSTEMSERVER_LOG];
};

static __always_inline void __boot_stat_record_systemserver_init_time_locked(
		struct boot_stat_proc *boot_stat, const char *message)
{
	struct systemserver_init_time_entry *entry;
	struct device *dev = __boot_stat_proc_to_dev(boot_stat);

	if (likely(boot_stat->is_completed))
		return;

	entry = devm_kzalloc(dev, sizeof(*entry), GFP_KERNEL);
	if (unlikely(!entry))
		return;

	strlcpy(entry->buf, message, sizeof(entry->buf));
	list_add(&entry->list, &boot_stat->systemserver_init_time_head);
}

static __always_inline void __boot_stat_add_boot_event_locked(
		struct boot_stat_proc *boot_stat,
		const char *log)
{
	ssize_t prefix_idx;
	size_t offset;
	const char *message;

	prefix_idx = __boot_stat_get_message_offset_from_plog(log, &offset);
	message = &log[offset];

	switch (prefix_idx) {
	case EVT_PLATFORM:
		if (unlikely(!boot_stat->is_completed &&
			     !strcmp(message, "bootcomplete"))) {
			boot_stat->ktime_completed = local_clock();
			boot_stat->is_completed = true;
		}
		__boot_stat_record_boot_event_locked(boot_stat, message);
		break;
	case EVT_RIL:
	case EVT_DEBUG:
		__boot_stat_record_boot_event_locked(boot_stat, message);
		break;
	case EVT_SYSTEMSERVER:
		__boot_stat_record_systemserver_init_time_locked(boot_stat,
				message);
		break;
	default:
		return;
	}
}

void sec_boot_stat_add_boot_event(struct boot_stat_drvdata *drvdata,
		const char *log)
{
	struct boot_stat_proc *boot_stat;

	if (!__boot_stat_is_boot_event(log))
		return;

	boot_stat = &drvdata->boot_stat;

	mutex_lock(&boot_stat->lock);
	__boot_stat_add_boot_event_locked(boot_stat, log);
	mutex_unlock(&boot_stat->lock);
}

static unsigned long long __boot_stat_show_boot_event_each_locked(
		struct seq_file *m,
		struct boot_stat_proc *boot_stat,
		struct boot_event *event, unsigned long long prev_ktime)
{
	char *log;
	unsigned long long msec;
	unsigned long long delta;
	unsigned long long time;

	log = kasprintf(GFP_KERNEL, "%s%s",
			boot_prefixes[event->prefix_idx].head, event->message);

	msec = event->ktime;
	do_div(msec, 1000000ULL);

	delta = event->ktime - prev_ktime;
	do_div(delta, 1000000ULL);

	time = sec_boot_stat_ktime_to_time(event->ktime);
	do_div(time, 1000000ULL);

	seq_printf(m, "%-46s%11llu%13llu%13llu\n", log, time, msec, delta);

	kfree(log);

	return event->ktime;
}

static void __boot_stat_show_soc(struct seq_file *m,
		struct boot_stat_proc *boot_stat)
{
	struct boot_stat_drvdata *drvdata = container_of(boot_stat,
			struct boot_stat_drvdata, boot_stat);
	struct sec_boot_stat_soc_operations *soc_ops;

	mutex_lock(&drvdata->soc_ops_lock);

	soc_ops = drvdata->soc_ops;
	if (!soc_ops || !soc_ops->show_on_boot_stat) {
		mutex_unlock(&drvdata->soc_ops_lock);
		return;
	}

	soc_ops->show_on_boot_stat(m);
	mutex_unlock(&drvdata->soc_ops_lock);
}

static void __boot_stat_show_boot_event_head(struct seq_file *m,
		struct boot_stat_proc *boot_stat)
{
	seq_printf(m, "%-47s%11s%13s%13s\n", "Boot Events",
			"time", "ktime", "delta");
	seq_printf(m, "%s\n", h_line);

	__boot_stat_show_soc(m, boot_stat);
}

static void __boot_stat_show_boot_event_locked(struct seq_file *m,
		struct boot_stat_proc *boot_stat)
{
	struct list_head *boot_event_head = &boot_stat->boot_event_head;
	struct boot_event *event;
	unsigned long long prev_ktime = 0ULL;

	list_for_each_entry(event, boot_event_head, list)
		prev_ktime = __boot_stat_show_boot_event_each_locked(
				m, boot_stat, event, prev_ktime);
}

static void __boot_stat_show_systemserver_init_time_locked(struct seq_file *m,
		struct boot_stat_proc *boot_stat)
{
	struct list_head *systemserver_init_time_head =
			&boot_stat->systemserver_init_time_head;
	struct systemserver_init_time_entry *init_time;

	seq_printf(m, "%s\n", h_line);
	seq_puts(m, "SystemServer services that took long time\n\n");

	list_for_each_entry(init_time, systemserver_init_time_head, list)
		seq_printf(m, "%s\n", init_time->buf);
}

static int sec_boot_stat_proc_show(struct seq_file *m, void *v)
{
	struct boot_stat_proc *boot_stat = m->private;

	__boot_stat_show_boot_event_head(m, boot_stat);

	mutex_lock(&boot_stat->lock);
	__boot_stat_show_boot_event_locked(m, boot_stat);
	__boot_stat_show_systemserver_init_time_locked(m, boot_stat);
	mutex_unlock(&boot_stat->lock);

	return 0;
}

static int sec_boot_stat_proc_open(struct inode *inode, struct file *file)
{
	void *__boot_stat = pde_data(inode);

	return single_open(file, sec_boot_stat_proc_show, __boot_stat);
}

static const struct proc_ops boot_stat_pops = {
	.proc_open = sec_boot_stat_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int __boot_stat_procfs_init(struct device *dev,
		struct boot_stat_proc *boot_stat)
{
	struct proc_dir_entry *proc;
	const char *node_name = "boot_stat";

	proc = proc_create_data(node_name, 0444, NULL, &boot_stat_pops,
			boot_stat);
	if (!proc) {
		dev_err(dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	boot_stat->proc = proc;

	return 0;
}

static void __boot_stat_procfs_exit(struct device *dev,
		struct boot_stat_proc *boot_stat)
{
	proc_remove(boot_stat->proc);
}

__ss_static int __boot_stat_init_boot_events(struct boot_stat_proc *boot_stat)
{
	size_t i;

	hash_init(boot_stat->event_htbl);

	for (i = 0; i < ARRAY_SIZE(boot_events); i++) {
		struct boot_event *event = &boot_events[i];
		u32 key = full_name_hash(NULL,
				event->message, (unsigned int)strlen(event->message));

		INIT_HLIST_NODE(&event->hlist);
		hash_add(boot_stat->event_htbl, &event->hlist, key);
		event->ktime = 0;
	}

	return 0;
}

int sec_boot_stat_proc_init(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *dev = bd->dev;
	struct boot_stat_proc *boot_stat = &drvdata->boot_stat;
	int err;

	mutex_init(&boot_stat->lock);
	boot_stat->total_event = ARRAY_SIZE(boot_events);
	INIT_LIST_HEAD(&boot_stat->boot_event_head);
	INIT_LIST_HEAD(&boot_stat->systemserver_init_time_head);

	__boot_stat_init_boot_events(boot_stat);

	if (IS_MODULE(CONFIG_SEC_BOOT_STAT))
		sec_boot_stat_add_boot_event(drvdata,
				"!@Boot: start init process");

	err = __boot_stat_procfs_init(dev, boot_stat);
	if (err)
		return err;

	return 0;
}

void sec_boot_stat_proc_exit(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *dev = bd->dev;
	struct boot_stat_proc *boot_stat = &drvdata->boot_stat;

	__boot_stat_procfs_exit(dev, boot_stat);
	mutex_destroy(&boot_stat->lock);
}
