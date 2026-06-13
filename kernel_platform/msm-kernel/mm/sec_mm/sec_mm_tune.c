// SPDX-License-Identifier: GPL-2.0
/*
 * sec_mm/
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 */

#include <linux/swap.h>
#include <linux/sec_mm.h>
#include <linux/jiffies.h>
#include <trace/hooks/mm.h>
#include <trace/hooks/vmscan.h>

static void sec_mm_cma_alloc_set_max_retries(void *data, int *max_retries)
{
	*max_retries = 10;
}

static void sec_mm_drain_all_pages_bypass(void *data, gfp_t gfp_mask,
		unsigned int order, unsigned long alloc_flags, int migratetype,
		unsigned long did_some_progress, bool *bypass)
{
	*bypass = mem_boost_mode_high();
}

static void sec_mm_rebalance_anon_lru_bypass(void *data, bool *bypass)
{
	*bypass = mem_boost_mode_high();
}

static void sec_mm_shrink_slab_bypass(void *data, gfp_t gfp_mask,
		int nid, struct mem_cgroup *memcg, int priority, bool *bypass)
{
	/*
	 * Allow shrink_slab only for kswapd, d.reclaim with priority == 0 and
	 * drop caches.
	 */
	if (!current_is_kswapd() && priority > 0)
		*bypass = true;
}

static void sec_mm_suitable_migration_target_bypass(void *data,
		struct page *page, bool *bypass)
{
	if (is_migrate_cma_or_isolate_page(page))
		*bypass = true;
}

#if CONFIG_MMAP_READAROUND_LIMIT == 0
unsigned int mmap_readaround_limit = VM_READAHEAD_PAGES;
#else
unsigned int mmap_readaround_limit = CONFIG_MMAP_READAROUND_LIMIT;
#endif

static void sec_mm_tune_mmap_readaround(void *data,
		unsigned int ra_pages, pgoff_t pgoff, pgoff_t *start,
		unsigned int *size, unsigned int *async_size)
{
	unsigned int new_ra_pages = mmap_readaround_limit;

	if (mem_boost_mode_high())
		new_ra_pages = min_t(unsigned int, new_ra_pages, 8);
	if (ra_pages <= new_ra_pages)
		return;
	*start = max_t(long, 0, pgoff - new_ra_pages / 2);
	*size = new_ra_pages;
	*async_size = new_ra_pages / 4;
}

enum scan_balance {
	SCAN_EQUAL,
	SCAN_FRACT,
	SCAN_ANON,
	SCAN_FILE,
};

static void sec_mm_tune_scan_control(void *data, bool *skip_swap)
{
	*skip_swap = true;
}

static unsigned long low_threshold;

static void sec_mm_tune_scan_type(void *data, enum scan_balance *scan_type)
{
	if (*scan_type == SCAN_FRACT && current_is_kswapd() &&
			mem_boost_mode_high() && !file_is_tiny(low_threshold))
		*scan_type = SCAN_FILE;
}

static void sec_mm_use_vm_swappiness(void *data, bool *use_vm_swappiness)
{
	*use_vm_swappiness = true;
}

static void sec_mm_set_balance_anon_file_reclaim(void *data,
		bool *balance_anon_file_reclaim)
{
	*balance_anon_file_reclaim = true;
}

#define ZS_SHRINKER_THRESHOLD	1024
#define ZS_SHRINKER_INTERVAL	(10 * HZ)

static unsigned long time_stamp;

static void sec_mm_zs_shrinker_adjust(void *data, unsigned long *pages_to_free)
{
	if (*pages_to_free > ZS_SHRINKER_THRESHOLD)
		time_stamp = jiffies + ZS_SHRINKER_INTERVAL;
	else
		*pages_to_free = 0;
}

static void sec_mm_zs_shrinker_bypass(void *data, bool *bypass)
{
	if (!current_is_kswapd() || time_is_after_jiffies(time_stamp))
		*bypass = true;
}

void init_sec_mm_tune(void)
{
	low_threshold = get_low_threshold();

	register_trace_android_vh_cma_alloc_set_max_retries(
			sec_mm_cma_alloc_set_max_retries, NULL);
	register_trace_android_vh_drain_all_pages_bypass(
			sec_mm_drain_all_pages_bypass, NULL);
	register_trace_android_vh_rebalance_anon_lru_bypass(
			sec_mm_rebalance_anon_lru_bypass, NULL);
	register_trace_android_vh_shrink_slab_bypass(
			sec_mm_shrink_slab_bypass, NULL);
	register_trace_android_vh_suitable_migration_target_bypass(
			sec_mm_suitable_migration_target_bypass, NULL);
	register_trace_android_vh_tune_mmap_readaround(
			sec_mm_tune_mmap_readaround, NULL);
	register_trace_android_vh_tune_scan_control(
			sec_mm_tune_scan_control, NULL);
	register_trace_android_vh_tune_scan_type(sec_mm_tune_scan_type, NULL);
	register_trace_android_vh_use_vm_swappiness(
			sec_mm_use_vm_swappiness, NULL);
	register_trace_android_vh_zs_shrinker_adjust(
			sec_mm_zs_shrinker_adjust, NULL);
	register_trace_android_vh_zs_shrinker_bypass(
			sec_mm_zs_shrinker_bypass, NULL);
	register_trace_android_rvh_set_balance_anon_file_reclaim(
			sec_mm_set_balance_anon_file_reclaim, NULL);
}

void exit_sec_mm_tune(void)
{
	unregister_trace_android_vh_cma_alloc_set_max_retries(
			sec_mm_cma_alloc_set_max_retries, NULL);
	unregister_trace_android_vh_drain_all_pages_bypass(
			sec_mm_drain_all_pages_bypass, NULL);
	unregister_trace_android_vh_rebalance_anon_lru_bypass(
			sec_mm_rebalance_anon_lru_bypass, NULL);
	unregister_trace_android_vh_shrink_slab_bypass(
			sec_mm_shrink_slab_bypass, NULL);
	unregister_trace_android_vh_suitable_migration_target_bypass(
			sec_mm_suitable_migration_target_bypass, NULL);
	unregister_trace_android_vh_tune_mmap_readaround(
			sec_mm_tune_mmap_readaround, NULL);
	unregister_trace_android_vh_tune_scan_control(
			sec_mm_tune_scan_control, NULL);
	unregister_trace_android_vh_tune_scan_type(sec_mm_tune_scan_type, NULL);
	unregister_trace_android_vh_use_vm_swappiness(
			sec_mm_use_vm_swappiness, NULL);
	unregister_trace_android_vh_zs_shrinker_adjust(
			sec_mm_zs_shrinker_adjust, NULL);
	unregister_trace_android_vh_zs_shrinker_bypass(
			sec_mm_zs_shrinker_bypass, NULL);
}
