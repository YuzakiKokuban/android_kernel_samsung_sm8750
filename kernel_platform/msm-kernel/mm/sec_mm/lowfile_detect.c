// SPDX-License-Identifier: GPL-2.0
/*
 * sec_mm/
 *
 * Copyright (C) 2020 Samsung Electronics
 *
 */

#include <linux/mm.h>
#include <linux/ratelimit.h>
#include <linux/sec_mm.h>

#define MIN_FILE_SIZE_HIGH	300
#define MIN_FILE_SIZE_LOW	200
#define MIN_FILE_SIZE_THR_GB	3

static unsigned long min_file;

static unsigned long lowfile_count(struct shrinker *s,
				struct shrink_control *sc)
{
	unsigned long inactive_file, active_file, file;
	static DEFINE_RATELIMIT_STATE(mm_debug_rs, 10 * HZ, 1);

	inactive_file = global_node_page_state(NR_INACTIVE_FILE);
	active_file = global_node_page_state(NR_ACTIVE_FILE);
	file = inactive_file + active_file;
	if (file < min_file && __ratelimit(&mm_debug_rs)) {
		pr_info("low file detected : %lukB < %luKB\n", K(file),
			K(min_file));
		show_mem();
	}
	return 0; /* return 0 not to call to scan_objects */
}

static struct shrinker mm_debug_shrinker = {
	.count_objects = lowfile_count,
};

void init_lowfile_detect(void)
{
	if (totalram_pages() > GB_TO_PAGES(MIN_FILE_SIZE_THR_GB))
		min_file = MB_TO_PAGES(MIN_FILE_SIZE_HIGH);
	else
		min_file = MB_TO_PAGES(MIN_FILE_SIZE_LOW);

	register_shrinker(&mm_debug_shrinker, "mm_debug_shrinker");
}

void exit_lowfile_detect(void)
{
	unregister_shrinker(&mm_debug_shrinker);
}
