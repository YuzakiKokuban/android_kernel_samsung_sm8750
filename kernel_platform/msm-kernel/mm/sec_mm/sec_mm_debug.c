// SPDX-License-Identifier: GPL-2.0
/*
 * sec_mm/
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 */

#include <linux/dma-heap.h>
#include <linux/psi.h>
#include <linux/mm.h>
#include <linux/ratelimit.h>
#include <linux/vmalloc.h>
#include <linux/sec_mm.h>
#include <linux/sched/cputime.h>
#include <trace/events/mmap.h>
#include <trace/events/samsung.h>
#include <trace/hooks/dmabuf.h>
#include <trace/hooks/mm.h>
#include <trace/hooks/psi.h>

DEFINE_RATELIMIT_STATE(dump_tasks_rs, 5 * HZ, 1);
#ifdef CONFIG_SEC_MM_DUMP_DMABUF_TASKS
DEFINE_RATELIMIT_STATE(dma_buf_tasks_rs, 10 * HZ, 1);
#endif

static void sec_mm_alloc_contig_range_not_isolated(void *data,
		unsigned long start, unsigned end)
{
	pr_info_ratelimited("alloc_contig_range: [%lx, %x) PFNs busy\n",
			start, end);
}

static void sec_mm_alloc_pages_slowpath_start(void *data, u64 *stime)
{
	u64 utime;

	task_cputime(current, &utime, stime);
}

static void sec_mm_alloc_pages_slowpath_end(void *data, gfp_t *gfp,
		unsigned int order, unsigned long jiffies_start, u64 start,
		unsigned long did_some_progress, unsigned long pages_reclaimed,
		int retry)
{
	u64 utime, cputime, end;

	task_cputime(current, &utime, &end);
	cputime = (end - start) / NSEC_PER_MSEC;
	if (cputime < 256)
		return;

	pr_info("alloc stall: timeJS(ms):%u|%llu rec:%lu|%lu ret:%d o:%d gfp:%#x(%pGg) AaiFai:%lukB|%lukB|%lukB|%lukB\n",
			jiffies_to_msecs(jiffies - jiffies_start), cputime,
			did_some_progress, pages_reclaimed, retry, order, *gfp, gfp,
			K(global_node_page_state(NR_ACTIVE_ANON)),
			K(global_node_page_state(NR_INACTIVE_ANON)),
			K(global_node_page_state(NR_ACTIVE_FILE)),
			K(global_node_page_state(NR_INACTIVE_FILE)));
}

static void sec_mm_cma_debug_show_areas(void *data, bool *show)
{
	*show = true;
}

static void sec_mm_dma_heap_buffer_alloc_start(void *data,
		const char *name, size_t len, u32 fd_flags, u64 heap_flags)
{
	tracing_mark_begin("%s(%s, %zu, 0x%x, 0x%llx)", "dma-buf_alloc",
			name, len, fd_flags, heap_flags);
}

static void sec_mm_dma_heap_buffer_alloc_end(void *data,
		const char *name, size_t len)
{
	tracing_mark_end();
}

static void sec_mm_tracing_mark_begin(struct file *file, pgoff_t pgoff,
		unsigned int size, bool sync)
{
	char buf[TRACING_MARK_BUF_SIZE], *path;

	if (!trace_tracing_mark_write_enabled())
		return;

	path = file_path(file, buf, TRACING_MARK_BUF_SIZE);
	if (IS_ERR(path)) {
		sprintf(buf, "file_path failed(%ld)", PTR_ERR(path));
		path = buf;
	}
	tracing_mark_begin("%d , %s , %lu , %d", sync, path, pgoff, size);
}

static void sec_mm_filemap_fault_start(void *data,
		struct file *file, pgoff_t pgoff)
{
	sec_mm_tracing_mark_begin(file, pgoff, 1, true);
}

static void sec_mm_filemap_fault_end(void *data,
		struct file *file, pgoff_t pgoff)
{
	tracing_mark_end();
}

static void sec_mm_page_cache_readahead_start(void *data,
		struct file *file, pgoff_t pgoff, unsigned int size, bool sync)
{
	sec_mm_tracing_mark_begin(file, pgoff, size, sync);
}

static void sec_mm_page_cache_readahead_end(void *data,
		struct file *file, pgoff_t pgoff)
{
	tracing_mark_end();
}

static void sec_mm_show_mem(void *data, unsigned int filter, nodemask_t *nodes)
{
	long dma_heap_pool_size_kb = dma_heap_try_get_pool_size_kb();

	pr_info("%s: %lu kB\n", "VmallocUsed", K(vmalloc_nr_pages()));
	if (dma_heap_pool_size_kb >= 0)
		pr_info("%s: %ld kB\n", "DmaHeapPool", dma_heap_pool_size_kb);

	if (in_interrupt())
		return;

	if (__ratelimit(&dump_tasks_rs))
		mm_debug_dump_tasks();
#ifdef CONFIG_SEC_MM_DUMP_DMABUF_TASKS
	if (__ratelimit(&dma_buf_tasks_rs))
		mm_debug_dump_dma_buf_tasks();
#endif
}

static void sec_mm_meminfo(void *data, struct seq_file *m)
{
	long dma_heap_pool_size_kb = dma_heap_try_get_pool_size_kb();

	if (dma_heap_pool_size_kb >= 0)
		show_val_meminfo(m, "DmaHeapPool", dma_heap_pool_size_kb);
}

#define WINDOW_MIN_NS 1000000000 /* 1s */
#define THRESHOLD_MIN_NS 100000000 /* 100ms */

static void sec_mm_psi_monitor(void *data,
		struct psi_trigger *t, u64 now, u64 growth)
{
	if (t->win.size >= WINDOW_MIN_NS && t->threshold >= THRESHOLD_MIN_NS)
		printk_deferred("psi: %s %llu %llu %d %llu %llu\n",
				"update_triggers", now, t->last_event_time,
				t->state, t->threshold, growth);
}

static void sec_mm_warn_alloc_show_mem_bypass(void *data, bool *bypass)
{
	static DEFINE_RATELIMIT_STATE(show_mem_rs, HZ, 1);

	if (!__ratelimit(&show_mem_rs))
		*bypass = true;
}

static void sec_mm_warn_alloc_tune_ratelimit(void *data,
		struct ratelimit_state *rs)
{
	rs->interval = 5*HZ;
	rs->burst = 2;
}

static void sec_mm_vm_unmapped_area(void *data,
		unsigned long addr, struct vm_unmapped_area_info *info)
{
	if (!IS_ERR_VALUE(addr))
		return;

	pr_warn_ratelimited("%s err:%ld total_vm:0x%lx flags:0x%lx len:0x%lx low:0x%lx high:0x%lx mask:0x%lx offset:0x%lx\n",
			__func__, addr, current->mm->total_vm, info->flags,
			info->length, info->low_limit, info->high_limit,
			info->align_mask, info->align_offset);
}

void init_sec_mm_debug(void)
{
	register_trace_android_vh_alloc_contig_range_not_isolated(
			sec_mm_alloc_contig_range_not_isolated, NULL);
	register_trace_android_vh_alloc_pages_slowpath_start(
			sec_mm_alloc_pages_slowpath_start, NULL);
	register_trace_android_vh_alloc_pages_slowpath_end(
			sec_mm_alloc_pages_slowpath_end, NULL);
	register_trace_android_vh_cma_debug_show_areas(
			sec_mm_cma_debug_show_areas, NULL);
	register_trace_android_vh_dma_heap_buffer_alloc_start(
			sec_mm_dma_heap_buffer_alloc_start, NULL);
	register_trace_android_vh_dma_heap_buffer_alloc_end(
			sec_mm_dma_heap_buffer_alloc_end, NULL);
	register_trace_android_vh_filemap_fault_start(
			sec_mm_filemap_fault_start, NULL);
	register_trace_android_vh_filemap_fault_end(
			sec_mm_filemap_fault_end, NULL);
	register_trace_android_vh_page_cache_readahead_start(
			sec_mm_page_cache_readahead_start, NULL);
	register_trace_android_vh_page_cache_readahead_end(
			sec_mm_page_cache_readahead_end, NULL);
	register_trace_android_vh_meminfo_proc_show(sec_mm_meminfo, NULL);
	register_trace_android_vh_psi_update_triggers(sec_mm_psi_monitor, NULL);
	register_trace_android_vh_warn_alloc_show_mem_bypass(
			sec_mm_warn_alloc_show_mem_bypass, NULL);
	register_trace_android_vh_warn_alloc_tune_ratelimit(
			sec_mm_warn_alloc_tune_ratelimit, NULL);
	register_trace_prio_android_vh_show_mem(sec_mm_show_mem, NULL, 0);
	register_trace_vm_unmapped_area(sec_mm_vm_unmapped_area, NULL);
}

void exit_sec_mm_debug(void)
{
	unregister_trace_android_vh_alloc_contig_range_not_isolated(
			sec_mm_alloc_contig_range_not_isolated, NULL);
	unregister_trace_android_vh_alloc_pages_slowpath_start(
			sec_mm_alloc_pages_slowpath_start, NULL);
	unregister_trace_android_vh_alloc_pages_slowpath_end(
			sec_mm_alloc_pages_slowpath_end, NULL);
	unregister_trace_android_vh_cma_debug_show_areas(
			sec_mm_cma_debug_show_areas, NULL);
	unregister_trace_android_vh_dma_heap_buffer_alloc_start(
			sec_mm_dma_heap_buffer_alloc_start, NULL);
	unregister_trace_android_vh_dma_heap_buffer_alloc_end(
			sec_mm_dma_heap_buffer_alloc_end, NULL);
	unregister_trace_android_vh_filemap_fault_start(
			sec_mm_filemap_fault_start, NULL);
	unregister_trace_android_vh_filemap_fault_end(
			sec_mm_filemap_fault_end, NULL);
	unregister_trace_android_vh_page_cache_readahead_start(
			sec_mm_page_cache_readahead_start, NULL);
	unregister_trace_android_vh_page_cache_readahead_end(
			sec_mm_page_cache_readahead_end, NULL);
	unregister_trace_android_vh_show_mem(sec_mm_show_mem, NULL);
	unregister_trace_android_vh_meminfo_proc_show(sec_mm_meminfo, NULL);
	unregister_trace_android_vh_psi_update_triggers(
			sec_mm_psi_monitor, NULL);
	unregister_trace_android_vh_warn_alloc_show_mem_bypass(
			sec_mm_warn_alloc_show_mem_bypass, NULL);
	unregister_trace_android_vh_warn_alloc_tune_ratelimit(
			sec_mm_warn_alloc_tune_ratelimit, NULL);
	unregister_trace_vm_unmapped_area(sec_mm_vm_unmapped_area, NULL);
}
