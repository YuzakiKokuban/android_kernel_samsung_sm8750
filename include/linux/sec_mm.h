/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SEC_MM_H
#define _SEC_MM_H

#include <linux/mm.h>
#include <linux/page-isolation.h>
#include <linux/seq_file.h>

#define GB_TO_PAGES(x) ((x) << (30 - PAGE_SHIFT))
#define MB_TO_PAGES(x) ((x) << (20 - PAGE_SHIFT))
#define K(x) ((x) << (PAGE_SHIFT-10))

static inline void show_val_meminfo(struct seq_file *m,
			const char *str, long size)
{
	char name[17];
	int len = strlen(str);

	if (len <= 15) {
		sprintf(name, "%s:", str);
	} else {
		strncpy(name, str, 15);
		name[15] = ':';
		name[16] = '\0';
	}

	seq_printf(m, "%-16s%8ld kB\n", name, size);
}

static inline bool is_migrate_cma_or_isolate_page(struct page *page)
{
	int migratetype = get_pageblock_migratetype(page);

	return is_migrate_cma(migratetype) || is_migrate_isolate(migratetype);
}

extern struct ratelimit_state dump_tasks_rs;
#ifdef CONFIG_SEC_MM_DUMP_DMABUF_TASKS
extern struct ratelimit_state dma_buf_tasks_rs;
#endif
extern struct atomic_notifier_head am_app_launch_notifier;
extern bool am_app_launch;
extern unsigned int mmap_readaround_limit;

#ifdef CONFIG_RBIN
void wake_dmabuf_rbin_heap_prereclaim(void);
#endif

static inline bool file_is_tiny(unsigned long low_threshold)
{
	return (global_node_page_state(NR_ACTIVE_FILE) +
		global_node_page_state(NR_INACTIVE_FILE)) < low_threshold;
}

static inline unsigned long get_low_threshold(void)
{
	if (totalram_pages() > GB_TO_PAGES(4))
		return MB_TO_PAGES(500);
	else if (totalram_pages() > GB_TO_PAGES(3))
		return MB_TO_PAGES(400);
	else if (totalram_pages() > GB_TO_PAGES(2))
		return MB_TO_PAGES(300);
	else
		return MB_TO_PAGES(200);
}

bool mem_boost_mode_high(void);

void mm_debug_dump_tasks(void);
#ifdef CONFIG_SEC_MM_DUMP_DMABUF_TASKS
void mm_debug_dump_dma_buf_tasks(void);
#endif

void init_lowfile_detect(void);
void exit_lowfile_detect(void);

void init_panic_handler(void);
void exit_panic_handler(void);

void init_sec_mm_debug(void);
void exit_sec_mm_debug(void);

void init_sec_mm_tune(void);
void exit_sec_mm_tune(void);

void init_sec_mm_sysfs(void);
void exit_sec_mm_sysfs(void);

#endif /* _SEC_MM_H */
