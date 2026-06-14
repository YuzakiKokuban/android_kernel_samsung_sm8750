// SPDX-License-Identifier: GPL-2.0
/*
 * linux/mm/hpp.c
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 */
#include <linux/suspend.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/ratelimit.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <linux/memblock.h>
#include <linux/mm.h>
#include <linux/compaction.h>
#include <linux/sec_mm.h>
#include <uapi/linux/sched/types.h>
#include <trace/hooks/mm.h>
#include <trace/hooks/dmabuf.h>

#define HUGEPAGE_ORDER	HPAGE_PMD_ORDER
#define HPP_FPI_MAGIC	((__force int __bitwise)BIT(31))

struct task_struct *khppd_task;

enum hpp_state_enum {
	HPP_OFF,
	HPP_ON,
	HPP_ACTIVATED,
	HPP_STATE_MAX
};

static unsigned int hpp_state;
static bool hpp_debug;
static bool app_launch;
static int khppd_wakeup = 1;
static unsigned long last_wakeup_stamp;

DECLARE_WAIT_QUEUE_HEAD(khppd_wait);
static struct list_head hugepage_list[MAX_NR_ZONES];
static struct list_head hugepage_nonzero_list[MAX_NR_ZONES];
int nr_hugepages_quota[MAX_NR_ZONES];
int nr_hugepages_limit[MAX_NR_ZONES];
int nr_hugepages_to_fill[MAX_NR_ZONES];
int nr_hugepages[MAX_NR_ZONES];
int nr_hugepages_nonzero[MAX_NR_ZONES];
int nr_hugepages_tried[MAX_NR_ZONES];
int nr_hugepages_alloced[MAX_NR_ZONES];
int nr_hugepages_fill_tried[MAX_NR_ZONES];
int nr_hugepages_fill_done[MAX_NR_ZONES];
static spinlock_t hugepage_list_lock[MAX_NR_ZONES];
static spinlock_t hugepage_nonzero_list_lock[MAX_NR_ZONES];

/* free pool if available memory is below this value */
static unsigned long hugepage_avail_low[MAX_NR_ZONES];
/* fill pool if available memory is above this value */
static unsigned long hugepage_avail_high[MAX_NR_ZONES];

static unsigned long get_zone_nr_hugepages(int zidx)
{
	return nr_hugepages[zidx] + nr_hugepages_nonzero[zidx];
}

static unsigned long total_hugepage_pool_pages(void)
{
	unsigned long total_nr_hugepages = 0;
	int zidx;

	if (hpp_state == HPP_OFF)
		return 0;

	for (zidx = MAX_NR_ZONES - 1; zidx >= 0; zidx--)
		total_nr_hugepages += get_zone_nr_hugepages(zidx);

	return total_nr_hugepages << HUGEPAGE_ORDER;
}

static inline unsigned long zone_available_simple(int zidx)
{
	struct pglist_data *pgdat = &contig_page_data;
	struct zone *zone = &pgdat->node_zones[zidx];

	return zone_page_state(zone, NR_FREE_PAGES) +
		zone_page_state(zone, NR_ZONE_INACTIVE_FILE) +
		zone_page_state(zone, NR_ZONE_ACTIVE_FILE);
}

/*
 * adjust limits depending on available memory
 * then return total limits in #pages under the specified zone.
 * If ratelimited, it returns -1. Caller should check returned value.
 */
static void hugepage_calculate_limits_under_zone(void)
{
	int zidx, prev_limit;
	bool print_debug_log = false;
	/* calculate only after 100ms passed */
	static DEFINE_RATELIMIT_STATE(rs, HZ/10, 1);
	static DEFINE_RATELIMIT_STATE(log_rs, HZ, 1);

	ratelimit_set_flags(&rs, RATELIMIT_MSG_ON_RELEASE);
	if (!__ratelimit(&rs))
		return;

	if (unlikely(hpp_debug) && __ratelimit(&log_rs)) {
		print_debug_log = true;
		pr_err("%s: zidx curavail d_avail  curpool curlimit newlimit\n", __func__);
	}

	for (zidx = 0; zidx < MAX_NR_ZONES; zidx++) {
		long avail_pages = zone_available_simple(zidx);
		long delta_avail = 0;
		long current_pool_pages = get_zone_nr_hugepages(zidx) << HUGEPAGE_ORDER;

		prev_limit = nr_hugepages_limit[zidx];
		if (avail_pages < hugepage_avail_low[zidx]) {
			delta_avail = hugepage_avail_low[zidx] - avail_pages;
			if (current_pool_pages - delta_avail < 0)
				delta_avail = current_pool_pages;
			nr_hugepages_limit[zidx] = (current_pool_pages - delta_avail) >> HUGEPAGE_ORDER;
		} else {
			nr_hugepages_limit[zidx] = nr_hugepages_quota[zidx];
		}

		if (print_debug_log)
			pr_err("%s: %4d %8ld %8ld %8ld %8d %8d\n",
				__func__, zidx, avail_pages, delta_avail,
				current_pool_pages, prev_limit, nr_hugepages_limit[zidx]);
	}
}

static inline void __try_to_wake_up_khppd(enum zone_type zidx)
{
	bool do_wakeup = false;

	if (app_launch || mem_boost_mode_high())
		return;

	if (time_is_after_jiffies(last_wakeup_stamp + 10 * HZ))
		return;

	if (khppd_wakeup)
		return;

	if (nr_hugepages_limit[zidx]) {
		if (nr_hugepages[zidx] * 2 < nr_hugepages_limit[zidx] ||
				nr_hugepages_nonzero[zidx])
			do_wakeup = true;
	} else if (zone_available_simple(zidx) > hugepage_avail_high[zidx]) {
		do_wakeup = true;
	}

	if (do_wakeup) {
		khppd_wakeup = 1;
		if (unlikely(hpp_debug))
			pr_info("khppd: woken up\n");
		wake_up(&khppd_wait);
	}
}

static void try_to_wake_up_khppd(void)
{
	int zidx;

	for (zidx = MAX_NR_ZONES - 1; zidx >= 0; zidx--)
		__try_to_wake_up_khppd(zidx);
}

static inline gfp_t get_gfp(enum zone_type zidx)
{
	gfp_t ret;

	if (zidx == ZONE_MOVABLE)
		ret = __GFP_MOVABLE | __GFP_HIGHMEM;
#ifdef CONFIG_ZONE_DMA
	else if (zidx == ZONE_DMA)
		ret = __GFP_DMA;
#elif defined(CONFIG_ZONE_DMA32)
	else if (zidx == ZONE_DMA32)
		ret = __GFP_DMA32;
#endif
	else
		ret = 0;
	return ret & ~__GFP_RECLAIM;
}

bool insert_hugepage_pool(struct page *page)
{
	enum zone_type zidx = page_zonenum(page);

	if (hpp_state == HPP_OFF)
		return false;

	if (get_zone_nr_hugepages(zidx) >= nr_hugepages_quota[zidx])
		return false;

	/*
	 * note that, at this point, the page is in the free page state except
	 * it is not in buddy. need prep_new_hpage before going to hugepage list.
	 */
	spin_lock(&hugepage_nonzero_list_lock[zidx]);
	list_add(&page->lru, &hugepage_nonzero_list[zidx]);
	nr_hugepages_nonzero[zidx]++;
	spin_unlock(&hugepage_nonzero_list_lock[zidx]);

	return true;
}

static void zeroing_nonzero_list(enum zone_type zidx)
{
	if (!nr_hugepages_nonzero[zidx])
		return;

	spin_lock(&hugepage_nonzero_list_lock[zidx]);
	while (!list_empty(&hugepage_nonzero_list[zidx])) {
		struct page *page = list_first_entry(&hugepage_nonzero_list[zidx],
						     struct page, lru);
		list_del(&page->lru);
		nr_hugepages_nonzero[zidx]--;
		spin_unlock(&hugepage_nonzero_list_lock[zidx]);

		if (nr_hugepages[zidx] < nr_hugepages_quota[zidx]) {
			prep_new_hpage(page, __GFP_ZERO, 0);
			spin_lock(&hugepage_list_lock[zidx]);
			list_add(&page->lru, &hugepage_list[zidx]);
			nr_hugepages[zidx]++;
			spin_unlock(&hugepage_list_lock[zidx]);
		} else {
			free_hpage(page, HPP_FPI_MAGIC);
		}
		spin_lock(&hugepage_nonzero_list_lock[zidx]);
	}
	spin_unlock(&hugepage_nonzero_list_lock[zidx]);
}

/*
 * this function should be called within hugepage_poold context, only.
 */
static void prepare_hugepage_alloc(void)
{
#ifdef CONFIG_COMPACTION
	struct sched_param param_normal = { .sched_priority = 0 };
	struct sched_param param_idle = { .sched_priority = 0 };
	static DEFINE_RATELIMIT_STATE(rs, 60 * 60 * HZ, 1);
	static int compact_count;

	if (!__ratelimit(&rs))
		return;

	if (!sched_setscheduler(current, SCHED_NORMAL, &param_normal)) {
		pr_info("khppd: compact start\n");
		compact_node_async(0);
		pr_info("khppd: compact end (%d done)\n", ++compact_count);
		if (sched_setscheduler(current, SCHED_IDLE, &param_idle))
			pr_err("khppd: fail to set sched_idle\n");
	}
#endif
}

static void calculate_nr_hugepages_to_fill(void)
{
	int zidx;

	if (unlikely(hpp_debug))
		pr_err("%s: zidx curavail d_avail curpool tofill\n", __func__);

	for (zidx = 0; zidx < MAX_NR_ZONES; zidx++) {
		long avail_pages = zone_available_simple(zidx);
		long delta_avail = 0;
		long current_pool_pages = get_zone_nr_hugepages(zidx) << HUGEPAGE_ORDER;
		long quota_pages = ((long)nr_hugepages_quota[zidx]) << HUGEPAGE_ORDER;

		if (avail_pages > hugepage_avail_high[zidx]) {
			delta_avail = avail_pages - hugepage_avail_high[zidx];
			if (current_pool_pages + delta_avail > quota_pages)
				delta_avail = quota_pages - current_pool_pages;
			nr_hugepages_to_fill[zidx] = delta_avail >> HUGEPAGE_ORDER;
		} else {
			nr_hugepages_to_fill[zidx] = 0;
		}

		if (unlikely(hpp_debug))
			pr_err("%s: %4d %8ld %8ld %8ld %8d\n",
				__func__, zidx, avail_pages, delta_avail,
				current_pool_pages, nr_hugepages_to_fill[zidx]);
	}
}

static void fill_hugepage_pool(enum zone_type zidx)
{
	struct page *page;
	int trial = nr_hugepages_to_fill[zidx];

	prepare_hugepage_alloc();

	nr_hugepages_fill_tried[zidx] += trial;
	while (trial--) {
		if (nr_hugepages[zidx] >= nr_hugepages_quota[zidx])
			break;

		page = alloc_pages(get_gfp(zidx) | __GFP_ZERO |
				__GFP_NOWARN, HUGEPAGE_ORDER);

		/* if alloc fails, future requests may fail also. stop here. */
		if (!page)
			break;

		if (page_zonenum(page) != zidx) {
			/* Note that we should use __free_pages to call to free_pages_prepare */
			__free_pages(page, HUGEPAGE_ORDER);

			/*
			 * if page is from the lower zone, future requests may
			 * also get the lower zone pages. stop here.
			 */
			break;
		}
		nr_hugepages_fill_done[zidx]++;
		spin_lock(&hugepage_list_lock[zidx]);
		list_add(&page->lru, &hugepage_list[zidx]);
		nr_hugepages[zidx]++;
		spin_unlock(&hugepage_list_lock[zidx]);
	}
}

static struct page *alloc_zeroed_hugepage(gfp_t gfp,
		enum zone_type highest_zoneidx)
{
	struct page *page = NULL;
	int zidx;

	if (hpp_state != HPP_ACTIVATED)
		return NULL;
	if (current == khppd_task)
		return NULL;

	nr_hugepages_tried[highest_zoneidx]++;
	for (zidx = highest_zoneidx; zidx >= 0; zidx--) {
		__try_to_wake_up_khppd(zidx);
		if (!nr_hugepages[zidx])
			continue;
		if (unlikely(!spin_trylock(&hugepage_list_lock[zidx])))
			continue;

		if (!list_empty(&hugepage_list[zidx])) {
			page = list_first_entry(&hugepage_list[zidx],
					struct page, lru);
			list_del(&page->lru);
			nr_hugepages[zidx]--;
		}
		spin_unlock(&hugepage_list_lock[zidx]);

		if (page)
			goto got_page;
	}

	for (zidx = highest_zoneidx; zidx >= 0; zidx--) {
		if (!nr_hugepages_nonzero[zidx])
			continue;
		if (unlikely(!spin_trylock(&hugepage_nonzero_list_lock[zidx])))
			continue;

		if (!list_empty(&hugepage_nonzero_list[zidx])) {
			page = list_first_entry(&hugepage_nonzero_list[zidx],
					struct page, lru);
			list_del(&page->lru);
			nr_hugepages_nonzero[zidx]--;
		}
		spin_unlock(&hugepage_nonzero_list_lock[zidx]);

		if (page) {
			prep_new_hpage(page, __GFP_ZERO, 0);
			goto got_page;
		}
	}
	return NULL;

got_page:
	nr_hugepages_alloced[zidx]++;
	if (gfp & __GFP_COMP)                       
		prep_compound_page(page, HUGEPAGE_ORDER);
	return page;
}

static int khppd(void *p)
{
	while (!kthread_should_stop()) {
		int zidx;

		wait_event_freezable(khppd_wait, khppd_wakeup ||
				kthread_should_stop());

		khppd_wakeup = 0;
		last_wakeup_stamp = jiffies;

		calculate_nr_hugepages_to_fill();
		for (zidx = 0; zidx < MAX_NR_ZONES; zidx++) {
			if (app_launch || mem_boost_mode_high())
				break;

			zeroing_nonzero_list(zidx);
			fill_hugepage_pool(zidx);
		}
	}
	return 0;
}

static bool is_hugepage_avail_low_ok(void)
{
	long total_avail_pages = 0;
	long total_avail_low_pages = 0;
	int zidx;

	for (zidx = 0; zidx < MAX_NR_ZONES; zidx++) {
		total_avail_pages += zone_available_simple(zidx);
		total_avail_low_pages += hugepage_avail_low[zidx];
	}
	return total_avail_pages >= total_avail_low_pages;
}

static unsigned long hugepage_pool_count(struct shrinker *shrink,
					struct shrink_control *sc)
{
	long count, total_count = 0;
	int zidx;
	static DEFINE_RATELIMIT_STATE(log_rs, HZ, 1);

	if (!current_is_kswapd())
		return 0;

	if (is_hugepage_avail_low_ok())
		return 0;

	hugepage_calculate_limits_under_zone();
	for (zidx = MAX_NR_ZONES - 1; zidx >= 0; zidx--) {
		count = get_zone_nr_hugepages(zidx) - nr_hugepages_limit[zidx];
		if (count > 0)
			total_count += (count << HUGEPAGE_ORDER);
	}

	if (unlikely(hpp_debug) && __ratelimit(&log_rs))
		pr_err("%s returned %ld\n", __func__, total_count);

	return total_count;
}

static unsigned long hugepage_pool_scan(struct shrinker *shrink,
					struct shrink_control *sc)
{
	struct page *page;
	unsigned long total_freed = 0;
	int zidx, nr_to_scan, nr_freed;
	bool print_debug_log = false;
	static DEFINE_RATELIMIT_STATE(log_rs, HZ, 1);

	if (!current_is_kswapd())
		return SHRINK_STOP;

	if (unlikely(hpp_debug) && __ratelimit(&log_rs)) {
		print_debug_log = true;
		pr_err("%s was requested %lu\n", __func__, sc->nr_to_scan);
	}

	hugepage_calculate_limits_under_zone();
	for (zidx = 0; zidx < MAX_NR_ZONES; zidx++) {
		nr_to_scan = get_zone_nr_hugepages(zidx) - nr_hugepages_limit[zidx];
		if (nr_to_scan <= 0)
			continue;
		nr_freed = 0;
		spin_lock(&hugepage_nonzero_list_lock[zidx]);
		while (!list_empty(&hugepage_nonzero_list[zidx]) &&
				nr_freed < nr_to_scan) {
			page = list_first_entry(&hugepage_nonzero_list[zidx],
					struct page, lru);
			list_del(&page->lru);
			free_hpage(page, HPP_FPI_MAGIC);
			nr_hugepages_nonzero[zidx]--;
			nr_freed++;
		}
		spin_unlock(&hugepage_nonzero_list_lock[zidx]);

		spin_lock(&hugepage_list_lock[zidx]);
		while (!list_empty(&hugepage_list[zidx]) &&
				nr_freed < nr_to_scan) {
			page = list_first_entry(&hugepage_list[zidx],
					struct page, lru);
			list_del(&page->lru);
			free_hpage(page, HPP_FPI_MAGIC);
			nr_hugepages[zidx]--;
			nr_freed++;
		}
		spin_unlock(&hugepage_list_lock[zidx]);
		total_freed += nr_freed;
	}

	if (print_debug_log)
		pr_err("%s freed %lu hugepages(%luK)\n",
			__func__, total_freed, K(total_freed << HUGEPAGE_ORDER));

	return total_freed ? total_freed << HUGEPAGE_ORDER : SHRINK_STOP;
}

static struct shrinker hugepage_pool_shrinker_info = {
	.scan_objects = hugepage_pool_scan,
	.count_objects = hugepage_pool_count,
	.seeks = DEFAULT_SEEKS,
};

module_param_array(nr_hugepages, int, NULL, 0444);
module_param_array(nr_hugepages_nonzero, int, NULL, 0444);
module_param_array(nr_hugepages_alloced, int, NULL, 0444);
module_param_array(nr_hugepages_tried, int, NULL, 0444);
module_param_array(nr_hugepages_fill_tried, int, NULL, 0444);
module_param_array(nr_hugepages_fill_done, int, NULL, 0444);
module_param_array(nr_hugepages_quota, int, NULL, 0644);
module_param_array(nr_hugepages_limit, int, NULL, 0444);

module_param_array(hugepage_avail_low, ulong, NULL, 0644);
module_param_array(hugepage_avail_high, ulong, NULL, 0644);

static int khppd_app_launch_notifier(struct notifier_block *nb,
				 unsigned long action, void *data)
{
	bool prev_launch;

	if (hpp_state == HPP_OFF)
		return 0;

	prev_launch = app_launch;
	app_launch = action ? true : false;

	if (prev_launch && !app_launch)
		try_to_wake_up_khppd();

	return 0;
}

static struct notifier_block khppd_app_launch_nb = {
	.notifier_call = khppd_app_launch_notifier,
};

static void hpp_meminfo(void *data, struct seq_file *m)
{
	show_val_meminfo(m, "HugepagePool", K(total_hugepage_pool_pages()));
}

static void hpp_show_mem(void *data, unsigned int filter, nodemask_t *nodemask)
{
	pr_info("%s: %lu kB\n", "HugepagePool", K(total_hugepage_pool_pages()));
}

static void hpp_meminfo_adjust(void *data, unsigned long *totalram, unsigned long *freeram)
{
	*freeram += total_hugepage_pool_pages();
}

static void hpp_try_alloc_pages_gfp(void *data, struct page **page,
		unsigned int order, gfp_t gfp, enum zone_type highest_zoneidx)
{
	if (order == HUGEPAGE_ORDER && !in_atomic())
		*page = alloc_zeroed_hugepage(gfp, highest_zoneidx);
}

static void hpp_free_pages_prepare_bypass(void *data, struct page *page,
		unsigned int order, int __bitwise fpi_flags, bool *bypass)
{
	if (fpi_flags != HPP_FPI_MAGIC)
		return;
	if (page_ref_count(page))
		put_page_testzero(page);
	else
		*bypass = true;
}

static void hpp_free_pages_ok_bypass(void *data, struct page *page,
		unsigned int order, int __bitwise fpi_flags, bool *bypass)
{
	if (fpi_flags == HPP_FPI_MAGIC)
		return;
	if (is_migrate_cma_or_isolate_page(page))
		return;
	if (order == HUGEPAGE_ORDER && insert_hugepage_pool(page))
		*bypass = true;
}

static void hpp_dmabuf_page_pool_free_bypass(void *data,
		struct page *page, bool *bypass)
{
	static bool did_check;
	static bool is_huge_dram;

	if (unlikely(!did_check)) {
		is_huge_dram = totalram_pages() > GB_TO_PAGES(6);
		did_check = true;
	}
	if (is_huge_dram && compound_order(page) == HUGEPAGE_ORDER) {
		__free_pages(page, HUGEPAGE_ORDER);
		*bypass = true;
	}
}

static void hpp_split_large_folio_bypass(void *data, bool *bypass)
{
	*bypass = is_hugepage_avail_low_ok();
}

static int __init init_hugepage_pool(void)
{
	struct pglist_data *pgdat = &contig_page_data;
	unsigned long managed_pages;
	long hugepage_quota, avail_low, avail_high;
	uint32_t totalram_pages_uint = totalram_pages();
	u64 num_pages;
	int zidx;

	if (totalram_pages_uint > GB_TO_PAGES(10)) {
		hugepage_quota = GB_TO_PAGES(1);
		avail_low = MB_TO_PAGES(2560);
	} else if (totalram_pages_uint > GB_TO_PAGES(6)) {
		hugepage_quota = GB_TO_PAGES(1);
		avail_low = MB_TO_PAGES(1100);
	} else {
		return -EINVAL;
	}
	avail_high = avail_low + (avail_low >> 2);

	for (zidx = 0; zidx < MAX_NR_ZONES; zidx++) {
		managed_pages = zone_managed_pages(&pgdat->node_zones[zidx]);
		/*
		 * calculate without zone lock as we assume managed_pages of
		 * zones do not change at runtime
		 */
		num_pages = (u64)hugepage_quota * managed_pages;
		do_div(num_pages, totalram_pages_uint);
		nr_hugepages_quota[zidx] = (num_pages >> HUGEPAGE_ORDER);
		nr_hugepages_limit[zidx] = nr_hugepages_quota[zidx];

		hugepage_avail_low[zidx] = (u64)avail_low * managed_pages;
		do_div(hugepage_avail_low[zidx], totalram_pages_uint);

		hugepage_avail_high[zidx] = (u64)avail_high * managed_pages;
		do_div(hugepage_avail_high[zidx], totalram_pages_uint);

		spin_lock_init(&hugepage_list_lock[zidx]);
		spin_lock_init(&hugepage_nonzero_list_lock[zidx]);
		INIT_LIST_HEAD(&hugepage_list[zidx]);
		INIT_LIST_HEAD(&hugepage_nonzero_list[zidx]);
	}
	return 0;
}

static int __init hpp_init(void)
{
	struct sched_param param = { .sched_priority = 0 };
	int ret;

	if (init_hugepage_pool())
		goto skip_all;

	khppd_task = kthread_run(khppd, NULL, "khppd");
	if (IS_ERR(khppd_task)) {
		pr_err("Failed to start khppd\n");
		khppd_task = NULL;
		goto skip_all;
	}
	try_to_wake_up_khppd();
	sched_setscheduler(khppd_task, SCHED_IDLE, &param);

	atomic_notifier_chain_register(&am_app_launch_notifier,
			&khppd_app_launch_nb);
	ret = register_shrinker(&hugepage_pool_shrinker_info, "hugepage_pool");
	if (ret) {
		kthread_stop(khppd_task);
		goto skip_all;
	}
	register_trace_android_vh_meminfo_proc_show(hpp_meminfo, NULL);
	register_trace_android_vh_show_mem(hpp_show_mem, NULL);
	register_trace_android_vh_si_meminfo_adjust(hpp_meminfo_adjust, NULL);
	register_trace_android_vh_free_pages_prepare_bypass(
			hpp_free_pages_prepare_bypass, NULL);
	register_trace_android_vh_free_pages_ok_bypass(
			hpp_free_pages_ok_bypass, NULL);
	register_trace_android_vh_dmabuf_page_pool_free_bypass(
			hpp_dmabuf_page_pool_free_bypass, NULL);
	register_trace_android_vh_split_large_folio_bypass(
			hpp_split_large_folio_bypass, NULL);
	register_trace_android_rvh_try_alloc_pages_gfp(
			hpp_try_alloc_pages_gfp, NULL);

	hpp_state = HPP_ACTIVATED;
skip_all:
	return 0;
}

static void __exit hpp_exit(void)
{
	if (!IS_ERR_OR_NULL(khppd_task))
		kthread_stop(khppd_task);
	atomic_notifier_chain_unregister(&am_app_launch_notifier,
			&khppd_app_launch_nb);
	unregister_shrinker(&hugepage_pool_shrinker_info);
	unregister_trace_android_vh_meminfo_proc_show(hpp_meminfo, NULL);
	unregister_trace_android_vh_show_mem(hpp_show_mem, NULL);
	unregister_trace_android_vh_si_meminfo_adjust(hpp_meminfo_adjust, NULL);
	unregister_trace_android_vh_free_pages_prepare_bypass(
			hpp_free_pages_prepare_bypass, NULL);
	unregister_trace_android_vh_free_pages_ok_bypass(
			hpp_free_pages_ok_bypass, NULL);
	unregister_trace_android_vh_dmabuf_page_pool_free_bypass(
			hpp_dmabuf_page_pool_free_bypass, NULL);
	unregister_trace_android_vh_split_large_folio_bypass(
			hpp_split_large_folio_bypass, NULL);
}

static int hpp_debug_param_set(const char *val, const struct kernel_param *kp)
{
	return param_set_bool(val, kp);
}

static const struct kernel_param_ops hpp_debug_param_ops = {
	.set =	hpp_debug_param_set,
	.get =	param_get_bool,
};
module_param_cb(debug, &hpp_debug_param_ops, &hpp_debug, 0644);

static int hpp_state_param_set(const char *val, const struct kernel_param *kp)
{
	return param_set_uint_minmax(val, kp, 0, 2);
}

static const struct kernel_param_ops hpp_state_param_ops = {
	.set =	hpp_state_param_set,
	.get =	param_get_uint,
};
module_param_cb(state, &hpp_state_param_ops, &hpp_state, 0644);
module_init(hpp_init)
module_exit(hpp_exit);
MODULE_LICENSE("GPL");
