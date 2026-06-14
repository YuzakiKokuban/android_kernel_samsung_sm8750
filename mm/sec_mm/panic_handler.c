// SPDX-License-Identifier: GPL-2.0
/*
 * sec_mm/
 *
 * Copyright (C) 2020 Samsung Electronics
 *
 */

#include <linux/mm.h>
#include <linux/panic_notifier.h>
#include <linux/sec_mm.h>

static int sec_mm_panic_handler(struct notifier_block *nb,
				unsigned long action, void *str_buf)
{
	WRITE_ONCE(dump_tasks_rs.interval, 0);
#ifdef CONFIG_SEC_MM_DUMP_DMABUF_TASKS
	WRITE_ONCE(dma_buf_tasks_rs.interval, 0);
#endif
	show_mem();

	return NOTIFY_DONE;
}

static struct notifier_block panic_block = {
	.notifier_call = sec_mm_panic_handler,
	.priority = 1 /* prior to priority 0 */
};

void init_panic_handler(void)
{
	atomic_notifier_chain_register(&panic_notifier_list, &panic_block);
}

void exit_panic_handler(void)
{
	atomic_notifier_chain_unregister(&panic_notifier_list, &panic_block);
}
