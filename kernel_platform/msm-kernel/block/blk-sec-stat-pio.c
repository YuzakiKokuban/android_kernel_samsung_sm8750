// SPDX-License-Identifier: GPL-2.0
/*
 *  Samsung Block Statistics
 *
 *  Copyright (C) 2021 Manjong Lee <mj0123.lee@samsung.com>
 *  Copyright (C) 2021 Junho Kim <junho89.kim@samsung.com>
 *  Copyright (C) 2021 Changheun Lee <nanich.lee@samsung.com>
 *  Copyright (C) 2021 Seunghwan Hyun <seunghwan.hyun@samsung.com>
 *  Copyright (C) 2021 Tran Xuan Nam <nam.tx2@samsung.com>
 */

#include <linux/sysfs.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#include "blk-sec.h"

#define MAX_PIO_NODE_NUM	10000
#define SORT_PIO_NODE_NUM	100
#define PIO_HASH_SIZE		100
#define MAX_PIO_DURATION_MS	10000

#define GET_HASH_KEY(tgid) ((unsigned int)(tgid) % PIO_HASH_SIZE)
#define RESET_PIO_IO(pio) \
do { \
	atomic_set(&(pio)->kb[REQ_OP_READ], 0); \
	atomic_set(&(pio)->kb[REQ_OP_WRITE], 0); \
	atomic_set(&(pio)->kb[REQ_OP_FLUSH], 0); \
	atomic_set(&(pio)->kb[REQ_OP_DISCARD], 0); \
} while (0)
#define GET_PIO_PRIO(pio) \
	(atomic_read(&(pio)->kb[REQ_OP_READ]) + \
	 atomic_read(&(pio)->kb[REQ_OP_WRITE]) * 2)

LIST_HEAD(pio_list);
LIST_HEAD(inflight_pios);
static DEFINE_SPINLOCK(pio_list_lock);
static DEFINE_SPINLOCK(inflight_pios_lock);
static int pio_cnt;
static int pio_enabled;
static unsigned int pio_duration_ms = 5000;
static unsigned long pio_timeout;
static struct kmem_cache *pio_cache;
static struct pio_node *pio_hash[PIO_HASH_SIZE];
static struct pio_node others;

static struct pio_node *add_pio_node(struct request *rq,
		struct task_struct *gleader)
{
	struct pio_node *pio = NULL;
	unsigned int hash_key = 0;

	if (pio_cnt >= MAX_PIO_NODE_NUM) {
add_others:
		return &others;
	}

	pio = kmem_cache_alloc(pio_cache, GFP_NOWAIT);
	if (!pio)
		goto add_others;

	INIT_LIST_HEAD(&pio->list);

	pio->tgid = task_tgid_nr(gleader);
	strncpy(pio->name, gleader->comm, TASK_COMM_LEN - 1);
	pio->name[TASK_COMM_LEN - 1] = '\0';
	pio->start_time = gleader->start_time;

	RESET_PIO_IO(pio);
	atomic_set(&pio->ref_count, 1);

	hash_key = GET_HASH_KEY(pio->tgid);

	spin_lock(&pio_list_lock);
	list_add(&pio->list, &pio_list);
	pio->h_next = pio_hash[hash_key];
	pio_hash[hash_key] = pio;
	pio_cnt++;
	spin_unlock(&pio_list_lock);

	return pio;
}

static struct pio_node *find_pio_node(pid_t tgid, u64 tg_start_time)
{
	struct pio_node *pio;

	for (pio = pio_hash[GET_HASH_KEY(tgid)]; pio; pio = pio->h_next) {
		if (pio->tgid != tgid)
			continue;
		if (pio->start_time != tg_start_time)
			continue;
		return pio;
	}

	return NULL;
}

static void free_pio_nodes(struct list_head *remove_list)
{
	struct pio_node *pio;
	struct pio_node *pion;

	/* move previous inflight pios to remove_list */
	spin_lock(&inflight_pios_lock);
	list_splice_init(&inflight_pios, remove_list);
	spin_unlock(&inflight_pios_lock);

	list_for_each_entry_safe(pio, pion, remove_list, list) {
		list_del(&pio->list);
		if (atomic_read(&pio->ref_count)) {
			spin_lock(&inflight_pios_lock);
			list_add(&pio->list, &inflight_pios);
			spin_unlock(&inflight_pios_lock);
			continue;
		}
		kmem_cache_free(pio_cache, pio);
	}
}

struct pio_node *get_pio_node(struct request *rq)
{
	struct task_struct *gleader = current->group_leader;
	struct pio_node *pio;

	if (pio_enabled == 0)
		return NULL;
	if (time_after(jiffies, pio_timeout))
		return NULL;
	if (req_op(rq) > REQ_OP_DISCARD)
		return NULL;

	spin_lock(&pio_list_lock);
	pio = find_pio_node(task_tgid_nr(gleader), gleader->start_time);
	if (pio) {
		atomic_inc(&pio->ref_count);
		spin_unlock(&pio_list_lock);
		return pio;
	}
	spin_unlock(&pio_list_lock);

	return add_pio_node(rq, gleader);
}

void update_pio_node(struct request *rq,
		unsigned int data_size, struct pio_node *pio)
{
	if (!pio)
		return;

	/* transfer bytes to kbytes via '>> 10' */
	atomic_add((req_op(rq) == REQ_OP_FLUSH) ? 1 : data_size >> 10,
			&pio->kb[req_op(rq)]);
}

void put_pio_node(struct pio_node *pio)
{
	if (!pio)
		return;

	atomic_dec(&pio->ref_count);
}

static void sort_pios(struct list_head *pios)
{
	struct pio_node *max_pio = NULL;
	struct pio_node *pio;
	unsigned long long max = 0;
	LIST_HEAD(sorted_list);
	int i;

	for (i = 0; i < SORT_PIO_NODE_NUM; i++) {
		list_for_each_entry(pio, pios, list) {
			if (GET_PIO_PRIO(pio) > max) {
				max = GET_PIO_PRIO(pio);
				max_pio = pio;
			}
		}
		if (max_pio != NULL)
			list_move_tail(&max_pio->list, &sorted_list);

		max = 0;
		max_pio = NULL;
	}
	list_splice_init(&sorted_list, pios);
}

static ssize_t pio_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	LIST_HEAD(curr_pios);
	int curr_pio_cnt;
	struct pio_node curr_others;
	struct pio_node *pio;
	int len = 0;

	spin_lock(&pio_list_lock);
	list_replace_init(&pio_list, &curr_pios);
	curr_pio_cnt = pio_cnt;
	curr_others = others;
	memset(pio_hash, 0x0, sizeof(pio_hash));
	pio_cnt = 0;
	RESET_PIO_IO(&others);
	spin_unlock(&pio_list_lock);

	if (curr_pio_cnt > SORT_PIO_NODE_NUM)
		sort_pios(&curr_pios);

	list_for_each_entry(pio, &curr_pios, list) {
		if (PAGE_SIZE - len > 80) {
			len += scnprintf(buf + len, PAGE_SIZE - len,
					"%d %d %d %s\n",
					pio->tgid,
					atomic_read(&pio->kb[REQ_OP_READ]),
					atomic_read(&pio->kb[REQ_OP_WRITE]),
					(pio->name[0]) ? pio->name : "-");
			continue;
		}

		atomic_add(atomic_read(&pio->kb[REQ_OP_READ]),
				&curr_others.kb[REQ_OP_READ]);
		atomic_add(atomic_read(&pio->kb[REQ_OP_WRITE]),
				&curr_others.kb[REQ_OP_WRITE]);
		atomic_add(atomic_read(&pio->kb[REQ_OP_FLUSH]),
				&curr_others.kb[REQ_OP_FLUSH]);
		atomic_add(atomic_read(&pio->kb[REQ_OP_DISCARD]),
				&curr_others.kb[REQ_OP_DISCARD]);
	}

	if (atomic_read(&curr_others.kb[REQ_OP_READ]) +
	    atomic_read(&curr_others.kb[REQ_OP_WRITE]))
		len += scnprintf(buf + len, PAGE_SIZE - len,
				"%d %d %d %s\n",
				curr_others.tgid,
				atomic_read(&curr_others.kb[REQ_OP_READ]),
				atomic_read(&curr_others.kb[REQ_OP_WRITE]),
				curr_others.name);

	free_pio_nodes(&curr_pios);
	pio_timeout = jiffies + msecs_to_jiffies(pio_duration_ms);

	return len;
}

static ssize_t pio_enabled_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	LIST_HEAD(curr_pios);
	int enable = 0;
	int ret;

	ret = kstrtoint(buf, 10, &enable);
	if (ret)
		return ret;

	pio_enabled = (enable >= 1) ? 1 : 0;

	spin_lock(&pio_list_lock);
	list_replace_init(&pio_list, &curr_pios);
	memset(pio_hash, 0x0, sizeof(pio_hash));
	pio_cnt = 0;
	RESET_PIO_IO(&others);
	spin_unlock(&pio_list_lock);

	free_pio_nodes(&curr_pios);
	pio_timeout = jiffies + msecs_to_jiffies(pio_duration_ms);

	return count;
}

static ssize_t pio_enabled_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int len = 0;

	len = scnprintf(buf, PAGE_SIZE, "%d\n", pio_enabled);

	return len;
}

static ssize_t pio_duration_ms_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	int ret;

	ret = kstrtoint(buf, 10, &pio_duration_ms);
	if (ret)
		return ret;

	if (pio_duration_ms > MAX_PIO_DURATION_MS)
		pio_duration_ms = MAX_PIO_DURATION_MS;

	return count;
}

static ssize_t pio_duration_ms_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int len = 0;

	len = scnprintf(buf, PAGE_SIZE, "%u\n", pio_duration_ms);

	return len;
}

static struct kobj_attribute pios_attr = __ATTR(pios, 0444, pio_show,  NULL);
static struct kobj_attribute pios_enable_attr = __ATTR(pios_enable, 0644,
		pio_enabled_show,  pio_enabled_store);
static struct kobj_attribute pios_duration_ms_attr = __ATTR(pios_duration_ms, 0644,
		pio_duration_ms_show, pio_duration_ms_store);

static const struct attribute *blk_sec_stat_pio_attrs[] = {
	&pios_attr.attr,
	&pios_enable_attr.attr,
	&pios_duration_ms_attr.attr,
	NULL,
};

int blk_sec_stat_pio_init(struct kobject *kobj)
{
	int retval;

	if (!kobj)
		return -EINVAL;

	pio_cache = kmem_cache_create("pio_node", sizeof(struct pio_node), 0, 0, NULL);
	if (!pio_cache)
		return -ENOMEM;

	retval = sysfs_create_files(kobj, blk_sec_stat_pio_attrs);
	if (retval) {
		kmem_cache_destroy(pio_cache);
		return retval;
	}

	/* init others */
	INIT_LIST_HEAD(&others.list);
	others.tgid = INT_MAX;
	strncpy(others.name, "others", TASK_COMM_LEN - 1);
	others.name[TASK_COMM_LEN - 1] = '\0';
	others.start_time = 0;
	RESET_PIO_IO(&others);
	atomic_set(&others.ref_count, 1);
	others.h_next = NULL;

	return 0;
}

void blk_sec_stat_pio_exit(struct kobject *kobj)
{
	if (!kobj)
		return;

	sysfs_remove_files(kobj, blk_sec_stat_pio_attrs);
	kmem_cache_destroy(pio_cache);
}
