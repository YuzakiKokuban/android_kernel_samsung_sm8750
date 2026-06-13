/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ZRAM_EXT_H_
#define _ZRAM_EXT_H_

#include "zram_drv.h"

#ifdef CONFIG_ZRAM_EXT
#define ZRAM_WB_THRESHOLD 64
#define NR_ZWBS 64
#define NR_FALLOC_PAGES 512
#define FALLOC_ALIGN_MASK (~(NR_FALLOC_PAGES - 1))
#define ZWBS_ALIGN_MASK (~(NR_ZWBS - 1))
#define IDX_SHIFT (PAGE_SHIFT * 2)
#define MAX_REQ_IDX 1018
#define MADV_WRITEBACK	29
#define MADV_PREFETCH	30

#define chunk_to_blk_idx(idx) ((idx) * NR_ZWBS)
#define blk_to_chunk_idx(idx) ((idx) / NR_ZWBS)

static inline unsigned long to_handle(unsigned long blk_idx,
		int offset, int size)
{
	/* use handle format as "blk_idx|offset|size" */
	return (blk_idx << IDX_SHIFT) |
		(offset << PAGE_SHIFT) | (size % PAGE_SIZE);
}

struct zram;

struct zram_wb_header {
	u32 index;
	int size;
};

struct zram_wb_work {
	struct work_struct work;
	struct page *src_page[NR_ZWBS];
	struct page *dst_page;
	struct bio *bio;
	struct bio *bio_chain;
	struct zram_wb_buffer *buf;
	struct zram *zram;
	unsigned long handle;
	wait_queue_head_t *wq;
	atomic_t *refcount;
	u16 nr_waits;
};

struct zram_wb_entry {
	u32 index;
	int offset;
	int size;
};

struct zram_wb_buffer {
	struct zram_wb_entry entry[NR_ZWBS][ZRAM_WB_THRESHOLD];
	struct page *page[NR_ZWBS];
	int cnt[NR_ZWBS];
	int off[NR_ZWBS];
	int idx;
};

/* 4kB */
struct zram_request {
	struct list_head list;
	int first;
	int last;
	u32 index[MAX_REQ_IDX];
};

/* madvise.c -> zram_ext.c */
int zram_writeback_list(struct zram *zram, struct list_head *list);
void zram_request_prefetch(struct zram *zram, u32 index);
void zram_request_writeback(struct zram *zram,
		struct list_head *list, u32 index);

/* zram_ext.c -> zram_drv.c */
int try_read_from_bdev(struct zram *zram, struct page *page,
		u32 index, struct bio *parent, bool wait);

/* zram_ext.c -> madvise.c */
void init_zram_madvise(struct zram *zram);
void exit_zram_madvise(void);

/* zram_drv.c -> zram_ext.c */
void free_block_bdev(struct zram *zram, unsigned long handle);
ssize_t bd_stat_show(struct device *dev,
		struct device_attribute *attr, char *buf);
ssize_t bd_stat_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len);
void zram_error_count_store(struct zram *zram, int type);
ssize_t zram_error_count_show(struct zram *zram, char *buf, ssize_t ret);
void deinit_zram_ext(struct zram *zram);
int init_zram_ext(struct zram *zram, unsigned long nr_pages, unsigned int size);

/* QC only */
#include <linux/samsung/debug/sec_debug.h>
static inline bool force_upload_mode(void)
{
	return sec_debug_is_enabled();
}

#endif
#endif
