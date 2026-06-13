#ifndef __INTERNAL__SEC_ENH_BOOT_TIME_PROC_H__
#define __INTERNAL__SEC_ENH_BOOT_TIME_PROC_H__

#define BOOT_TIME_HASH_BITS		3

struct enh_boot_time_proc {
	struct proc_dir_entry *proc;
	struct mutex lock;
	bool is_finished;
	size_t nr_event;
	struct list_head boot_time_head;
	DECLARE_HASHTABLE(boot_time_htbl, BOOT_TIME_HASH_BITS);
};

#define MAX_LENGTH_OF_ENH_BOOT_TIME_LOG	90

struct enh_boot_time_entry {
	struct list_head list;
	struct hlist_node hlist;
	char buf[MAX_LENGTH_OF_ENH_BOOT_TIME_LOG];
	unsigned long long ktime;
};

#endif /* __INTERNAL__SEC_ENH_BOOT_TIME_PROC_H__ */
