// SPDX-License-Identifier: GPL-2.0
/*
 * PROCA task descriptors table
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Hryhorii Tur, <hryhorii.tur@partner.samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/hashtable.h>
#include <linux/string.h>

#include "proca_table.h"
#include "proca_log.h"

int proca_table_init(struct proca_table *table)
{
	PROCA_BUG_ON(!table);

	memset(table, 0, sizeof(*table));

	spin_lock_init(&table->maps_lock);
	hash_init(table->pid_map);
	hash_init(table->app_name_map);

	table->hash_tables_shift = PROCA_TASKS_TABLE_SHIFT;
	return 0;
}

/*
 * Following hash functions and constants were taken from 4.9.59 kernel
 * in order to simplify porting to new devices.
 */

#define GOLDEN_RATIO_32 0x61C88647

static inline u32 proca_hash_32(u32 val)
{
	return val * GOLDEN_RATIO_32;
}

/* Hash courtesy of the R5 hash in reiserfs modulo sign bits */
#define proca_init_name_hash(salt)		(unsigned long)(salt)

/* partial hash update function. Assume roughly 4 bits per character */
static inline unsigned long
proca_partial_name_hash(unsigned long c, unsigned long prevhash)
{
	return (prevhash + (c << 4) + (c >> 4)) * 11;
}

/*
 * Finally: cut down the number of bits to a int value (and try to avoid
 * losing bits).  This also has the property (wanted by the dcache)
 * that the msbits make a good hash table index.
 */
static inline unsigned long proca_end_name_hash(unsigned long hash)
{
	return proca_hash_32((unsigned int)hash);
}

static unsigned long calculate_app_name_hash(struct proca_table *table,
					     const char *app_name,
					     size_t app_name_size)
{
	size_t i;
	unsigned long hash = proca_init_name_hash(0);

	if (!app_name)
		return proca_end_name_hash(hash);

	for (i = 0; i < app_name_size; ++i)
		hash = proca_partial_name_hash(app_name[i], hash);
	return proca_end_name_hash(hash) % (1 << table->hash_tables_shift);
}

static unsigned long calculate_pid_hash(struct proca_table *table, pid_t pid)
{
	return proca_hash_32(pid) >> (32 - table->hash_tables_shift);
}

int proca_table_add_task_descr(struct proca_table *table,
				struct proca_task_descr *descr)
{
	unsigned long pid_hash_key;
	unsigned long app_hash_key;
	unsigned long irqsave_flags;
	struct proca_identity *identity;

	PROCA_BUG_ON(!table || !descr);

	identity = &descr->proca_identity;

	pid_hash_key = calculate_pid_hash(table, descr->task->pid);
	if (identity->certificate)
		app_hash_key = calculate_app_name_hash(table,
			identity->parsed_cert.app_name,
			identity->parsed_cert.app_name_size);

	spin_lock_irqsave(&table->maps_lock, irqsave_flags);
	hlist_add_head(&descr->pid_map_node, &table->pid_map[pid_hash_key]);

	if (identity->certificate)
		hlist_add_head(&descr->app_name_map_node,
			&table->app_name_map[app_hash_key]);

	spin_unlock_irqrestore(&table->maps_lock, irqsave_flags);
	return 0;
}

void proca_table_remove_task_descr(struct proca_table *table,
				struct proca_task_descr *descr)
{
	unsigned long irqsave_flags;

	if (!descr)
		return;

	spin_lock_irqsave(&table->maps_lock, irqsave_flags);
	hash_del(&descr->pid_map_node);
	hash_del(&descr->app_name_map_node);
	spin_unlock_irqrestore(&table->maps_lock, irqsave_flags);
}

struct proca_task_descr *proca_table_get_by_task(
					struct proca_table *table,
					const struct task_struct *task)
{
	struct proca_task_descr *descr;
	struct proca_task_descr *target_task_descr = NULL;
	unsigned long hash_key;
	unsigned long irqsave_flags;

	hash_key = calculate_pid_hash(table, task->pid);

	spin_lock_irqsave(&table->maps_lock, irqsave_flags);
	hlist_for_each_entry(descr, &table->pid_map[hash_key], pid_map_node) {
		if (task == descr->task) {
			target_task_descr = descr;
			break;
		}
	}
	spin_unlock_irqrestore(&table->maps_lock, irqsave_flags);

	return target_task_descr;
}

struct proca_task_descr *proca_table_remove_by_task(
				struct proca_table *table,
				const struct task_struct *task)
{
	struct proca_task_descr *target_task_descr = NULL;

	target_task_descr = proca_table_get_by_task(table, task);
	proca_table_remove_task_descr(table, target_task_descr);

	return target_task_descr;
}

#if defined(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_GPL(proca_table_remove_by_task);
EXPORT_SYMBOL_GPL(proca_table_init);
EXPORT_SYMBOL_GPL(proca_table_get_by_task);
EXPORT_SYMBOL_GPL(proca_table_add_task_descr);
EXPORT_SYMBOL_GPL(compare_with_five_signature);
#endif
