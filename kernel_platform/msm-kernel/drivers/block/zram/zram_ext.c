// SPDX-License-Identifier: GPL-2.0
/*
 * zram_ext
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 */

#include <linux/bio.h>
#include <linux/bitops.h>
#include <linux/blkdev.h>
#include <linux/freezer.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/loop.h>
#include <linux/statfs.h>
#include <linux/sec_mm.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <uapi/linux/falloc.h>
#include <trace/hooks/mm.h>

#include "zram_ext.h"

#ifdef CONFIG_ZRAM_EXT
static unsigned long alloc_chunk_bdev(struct zram *zram)
{
	unsigned long chunk_idx = 1;
	unsigned long max_idx = blk_to_chunk_idx(zram->nr_pages);

retry:
	/* skip 0 bit to confuse zram.handle = 0 */
	chunk_idx = find_next_zero_bit(zram->chunk_bitmap, max_idx, chunk_idx);
	if (chunk_idx == max_idx)
		return 0;

	if (test_and_set_bit(chunk_idx, zram->chunk_bitmap))
		goto retry;

	atomic64_add(NR_ZWBS, &zram->stats.bd_count);
	return chunk_idx;
}

static void free_chunk_bdev(struct zram *zram, unsigned long chunk_idx)
{
	unsigned long flags;

	spin_lock_irqsave(&zram->refcount_lock, flags);
	if (zram->refcount_table && !--zram->refcount_table[chunk_idx]) {
		clear_bit(chunk_idx, zram->chunk_bitmap);
		atomic64_sub(NR_ZWBS, &zram->stats.bd_count);
	}
	spin_unlock_irqrestore(&zram->refcount_lock, flags);
}

void free_block_bdev(struct zram *zram, unsigned long handle)
{
	int size = handle & (PAGE_SIZE - 1) ? : PAGE_SIZE;

	if (size) {
		atomic64_sub(size, &zram->stats.bd_size);
		atomic64_dec(&zram->stats.bd_objcnt);
	}
	free_chunk_bdev(zram, blk_to_chunk_idx(handle >> IDX_SHIFT));
}

static void pin_chunk_bdev(struct zram *zram, unsigned long chunk_idx)
{
	unsigned long flags;

	spin_lock_irqsave(&zram->refcount_lock, flags);
	if (zram->refcount_table)
		zram->refcount_table[chunk_idx]++;
	spin_unlock_irqrestore(&zram->refcount_lock, flags);
}

static void unpin_chunk_bdev(struct zram *zram, unsigned long chunk_idx)
{
	free_chunk_bdev(zram, chunk_idx);
}

static void copy_to_buf(struct page **pages, void *dst,
			int idx, int offset, int size)
{
	int sizes[2];
	u8 *src;

	sizes[0] = min_t(int, size, PAGE_SIZE - offset);
	sizes[1] = size - sizes[0];

	if (sizes[0]) {
		src = kmap_atomic(pages[idx]);
		memcpy(dst, src + offset, sizes[0]);
		kunmap_atomic(src);
	}
	if (sizes[1]) {
		src = kmap_atomic(pages[idx + 1]);
		memcpy(dst + sizes[0], src, sizes[1]);
		kunmap_atomic(src);
	}
}

static void print_hex_dump_pages(struct page **src_page, int idx)
{
	const char * const keywords[] = {"Prev", "This", "Next"};
	int min_idx = max_t(int, idx - 1, 0);
	int max_idx = min_t(int, idx + 1, NR_ZWBS - 1);
	int i;
	void *src;

	for (i = min_idx; i <= max_idx; i++) {
		pr_err("%s page\n", keywords[i - idx + 1]);
		src = kmap_atomic(src_page[i]);
		print_hex_dump_fmt(src, PAGE_SIZE);
		kunmap_atomic(src);
	}
}

static void check_marker(struct zram *zram, struct page **pages, int idx,
			void *addr, int size, u32 index)
{
	unsigned char lzo_marker[4] = {0x11, 0x00, 0x00};
	u32 prio;

	if (size == PAGE_SIZE)
		return;

	prio = zram_get_priority(zram, index);
	if (!strncmp(zram->comps[prio]->name, "lzo-rle", 7))
		return;
	if (!memcmp(addr + size - 3, lzo_marker, 3))
		return;
	pr_err("%ps marker error, addr=0x%px len=%u\n", (void *)_RET_IP_, addr, size);
	if (pages)
		print_hex_dump_pages(pages, idx);
	else
		print_hex_dump_fmt(addr, size);
	zram_error_count_store(zram, ERR_TYPE1);
}

static int zram_decomp_page(struct zram *zram, struct page **src_page,
		struct page *dst_page, unsigned long handle)
{
	struct zram_wb_header *zhdr;
	struct zcomp_strm *zstrm;
	int page_idx = (handle >> IDX_SHIFT) & ~ZWBS_ALIGN_MASK;
	int offset = (handle >> PAGE_SHIFT) & (PAGE_SIZE - 1);
	int size = handle & (PAGE_SIZE - 1) ? : PAGE_SIZE;
	int ret = 0, header_sz = sizeof(struct zram_wb_header);
	u32 index, prio;
	u8 *src, *dst, *addr;

	src = kmap_atomic(src_page[page_idx]);
	zhdr = (struct zram_wb_header *)(src + offset);
	index = zhdr->index;
	if (zhdr->size != size) {
		pr_err("%s zhdr error, size should be %u but was %u src=0x%px offset=%u\n",
			__func__, size, zhdr->size, src, offset);
		print_hex_dump_pages(src_page, page_idx);
		zram_error_count_store(zram, ERR_TYPE2);
		kunmap_atomic(src);
		return -EINVAL;
	}
	dst = kmap_atomic(dst_page);
	if (size == PAGE_SIZE) {
		copy_to_buf(src_page, dst, page_idx, offset + header_sz, size);
		goto out;
	}
	prio = zram_get_priority(zram, index);
	zstrm = zcomp_stream_get(zram->comps[prio]);
	if (offset + header_sz + size > PAGE_SIZE) {
		addr = zstrm->tmpbuf;
		copy_to_buf(src_page, addr, page_idx, offset + header_sz, size);
	} else {
		addr = src + offset + header_sz;
	}
	ret = zcomp_decompress(zstrm, addr, size, dst);
	zcomp_stream_put(zram->comps[prio]);
	if (unlikely(ret)) {
		pr_err("%s Decompression failed! err=%d, index=%u, len=%u, vaddr=0x%px\n",
			zram->comps[prio]->name, ret, index, size, addr);
		print_hex_dump_fmt(addr, size);
		zram_error_count_store(zram, ERR_TYPE1);
	}
out:
	kunmap_atomic(dst);
	kunmap_atomic(src);

	return ret;
}

static void zram_free_read_work(struct zram *zram,
		struct zram_wb_work *zw, unsigned int chunk_idx)
{
	int i;

	spin_lock(&zram->read_work_lock);
	if (--zw->nr_waits > 0) {
		spin_unlock(&zram->read_work_lock);
		return;
	}
	zram->read_work[chunk_idx] = NULL;
	spin_unlock(&zram->read_work_lock);
	for (i = 0; i < NR_ZWBS; i++)
		__free_page(zw->src_page[i]);
	kvfree(zw);
}

static int zram_hook_read_work(struct zram *zram, struct page *page,
		unsigned long handle, unsigned long chunk_idx)
{
	struct zram_wb_work *zw;
	int ret;

	if (!page)
		return -EBUSY;

	spin_lock(&zram->read_work_lock);
	zw = zram->read_work[chunk_idx];
	if (!zw) {
		spin_unlock(&zram->read_work_lock);
		return -EBUSY;
	}
	zw->nr_waits++;
	spin_unlock(&zram->read_work_lock);
	ret = zram_decomp_page(zram, zw->src_page, page, handle);
	zram_free_read_work(zram, zw, chunk_idx);

	return ret;
}

static void zram_move_entry(struct zram *zram, struct page **pages,
		int idx, unsigned long blk_idx, int offset, int size, u32 index)
{
	unsigned long handle = -ENOMEM;
	unsigned long expected = to_handle(blk_idx + idx, offset, size);
	unsigned long alloced_pages;
	int header_sz = sizeof(struct zram_wb_header);
	u8 *dst;
	gfp_t gfp_mask = __GFP_HIGHMEM | __GFP_MOVABLE | __GFP_CMA;

retry:
	zram_slot_lock(zram, index);
	if (!zram_allocated(zram, index) ||
			!zram_test_flag(zram, index, ZRAM_WB) ||
			zram_get_element(zram, index) != expected) {
		zram_slot_unlock(zram, index);
		if (!IS_ERR_VALUE(handle))
			zs_free(zram->mem_pool, handle);
		return;
	}

	if (IS_ERR_VALUE(handle))
		handle = zs_malloc(zram->mem_pool, size,
				__GFP_KSWAPD_RECLAIM | __GFP_NOWARN | gfp_mask);
	if (IS_ERR_VALUE(handle)) {
		zram_slot_unlock(zram, index);
		handle = zs_malloc(zram->mem_pool, size, GFP_KERNEL | gfp_mask);
		goto retry;
	}
	alloced_pages = zs_get_total_pages(zram->mem_pool);
	update_used_max(zram, alloced_pages);

	dst = zs_map_object(zram->mem_pool, handle, ZS_MM_WO);
	copy_to_buf(pages, dst, idx, offset + header_sz, size);
	zs_unmap_object(zram->mem_pool, handle);

	atomic64_add(size, &zram->stats.compr_data_size);
	zram_free_page(zram, index);
	zram_set_element(zram, index, handle);
	zram_set_obj_size(zram, index, size);
	if (size == PAGE_SIZE) {
		zram_set_flag(zram, index, ZRAM_HUGE);
		atomic64_inc(&zram->stats.huge_pages);
	}
	zram_slot_unlock(zram, index);
	atomic64_inc(&zram->stats.pages_stored);
}

static void zram_move_to_zspool(struct zram *zram, struct page **pages,
				unsigned long blk_idx)
{
	struct zram_wb_header *zhdr;
	int size, offset = 0, idx = 0;
	int header_sz = sizeof(struct zram_wb_header);
	u32 index, max_index = zram->disksize >> PAGE_SHIFT;
	u8 *mem;

	while (idx < NR_ZWBS) {
		mem = kmap_atomic(pages[idx]);
		zhdr = (struct zram_wb_header *)(mem + offset);
		index = zhdr->index;
		size = zhdr->size;
		kunmap_atomic(mem);

		/* last object */
		if (index == UINT_MAX && size == 0) {
			idx++;
			offset = 0;
			continue;
		}

		/* corrupted page */
		if (index >= max_index || size > PAGE_SIZE)
			break;

		/* store entry in zspool */
		zram_move_entry(zram, pages, idx, blk_idx, offset, size, index);

		offset += (size + header_sz);
		idx += (offset / PAGE_SIZE);
		offset %= PAGE_SIZE;
		/* check next offset again */
		if (offset + header_sz > PAGE_SIZE) {
			idx++;
			offset = 0;
		}
	}
}

static void zram_read_done_work(struct work_struct *work)
{
	struct zram_wb_work *zw = container_of(work, struct zram_wb_work, work);
	struct zram *zram = zw->zram;
	struct page **src_page = zw->src_page;
	struct page *dst_page = zw->dst_page;
	struct bio *bio = zw->bio;
	struct bio *bio_chain = zw->bio_chain;
	unsigned long blk_idx = (zw->handle >> IDX_SHIFT) & ZWBS_ALIGN_MASK;
	unsigned long chunk_idx = blk_to_chunk_idx(blk_idx);
	int errno = blk_status_to_errno(bio->bi_status);

	if (dst_page) {
		if (errno)
			bio_chain->bi_status = bio->bi_status;
		else if (zram_decomp_page(zram, src_page, dst_page, zw->handle))
			bio_chain->bi_status = BLK_STS_IOERR;
		bio_endio(bio_chain);
	}

	if (!errno)
		zram_move_to_zspool(zram, zw->src_page, blk_idx);
	clear_bit(chunk_idx, zram->read_bitmap);
	zram_free_read_work(zram, zw, chunk_idx);
	unpin_chunk_bdev(zram, chunk_idx);
	bio_put(bio);
}

static void zram_read_end_io(struct bio *bio)
{
	struct page *page = bio->bi_io_vec[0].bv_page;
	struct zram_wb_work *zw = (struct zram_wb_work *)page_private(page);
	struct zram *zram = zw->zram;
	unsigned long chunk_idx = blk_to_chunk_idx(zw->handle >> IDX_SHIFT);
	int errno = blk_status_to_errno(bio->bi_status);

	if (errno)
		pr_err("%s read bio returned errno %d\n", __func__, errno);
	else if (!zram->read_work[chunk_idx])
		zram->read_work[chunk_idx] = zw;

	INIT_WORK(&zw->work, zram_read_done_work);
	schedule_work(&zw->work);
}

static bool zram_read_from_bdev(struct zram *zram, struct page *page,
		unsigned long handle, struct bio *parent)
{
	struct zram_wb_work *zw;
	struct bio *bio;
	unsigned long blk_idx = (handle >> IDX_SHIFT) & ZWBS_ALIGN_MASK;
	int i;

	atomic64_add(NR_ZWBS, &zram->stats.bd_reads);
	bio = bio_alloc(zram->bdev, NR_ZWBS, REQ_OP_READ, GFP_NOIO);
	if (!bio)
		return false;
	zw = kvzalloc(sizeof(struct zram_wb_work), GFP_NOIO);
	if (!zw) {
		bio_put(bio);
		return false;
	}
	zw->dst_page = page;
	zw->zram = zram;
	zw->bio = bio;
	zw->handle = handle;
	zw->nr_waits = 1;
	bio->bi_end_io = zram_read_end_io;
	bio->bi_iter.bi_sector = blk_idx * (PAGE_SIZE >> 9);
	for (i = 0; i < NR_ZWBS; i++) {
		zw->src_page[i] = alloc_page(GFP_NOIO | __GFP_HIGHMEM);
		if (!zw->src_page[i])
			goto free_pages;
		__bio_add_page(bio, zw->src_page[i], PAGE_SIZE, 0);
	}
	set_page_private(zw->src_page[0], (unsigned long)zw);
	if (parent) {
		zw->bio_chain = bio_alloc(zram->bdev, 1, REQ_OP_READ, GFP_NOIO);
		if (!zw->bio_chain)
			goto free_pages;
		zw->bio_chain->bi_opf = parent->bi_opf;
		bio_chain(zw->bio_chain, parent);
	}
	submit_bio(bio);

	return true;
free_pages:
	for (i = 0; i < NR_ZWBS; i++) {
		if (!zw->src_page[i])
			break;
		__free_page(zw->src_page[i]);
	}
	kvfree(zw);
	bio_put(bio);

	return false;
}

int try_read_from_bdev(struct zram *zram, struct page *page,
		u32 index, struct bio *parent, bool wait)
{
	unsigned long handle;
	unsigned long chunk_idx;
	int ret;

	zram_slot_lock(zram, index);
	if (!zram_test_flag(zram, index, ZRAM_WB)) {
		zram_slot_unlock(zram, index);
		return -ENOENT;
	}
	handle = zram_get_element(zram, index);
	chunk_idx = blk_to_chunk_idx(handle >> IDX_SHIFT);
	pin_chunk_bdev(zram, chunk_idx);
	if (test_and_set_bit(chunk_idx, zram->read_bitmap) && wait) {
		ret = zram_hook_read_work(zram, page, handle, chunk_idx);
		unpin_chunk_bdev(zram, chunk_idx);
		zram_slot_unlock(zram, index);
		return ret;
	}
	zram_slot_unlock(zram, index);
	if (!zram_read_from_bdev(zram, page, handle, parent)) {
		clear_bit(chunk_idx, zram->read_bitmap);
		unpin_chunk_bdev(zram, chunk_idx);
		return -EBUSY;
	}
	if (page)
		atomic64_inc(&zram->stats.bd_objreads);
	return 0;
}

static int zram_prefetchd(void *data)
{
	struct zram *zram = data;
	struct zram_request *req;
	struct list_head *list = &zram->prefetch_list;
	u32 index;

	set_freezable();

	while (!kthread_should_stop()) {
		wait_event_freezable(zram->prefetch_wait,
				atomic_read(&zram->nr_prefetch) ||
				kthread_should_stop());
		while (atomic_read(&zram->nr_prefetch)) {
			if (try_to_freeze() || kthread_should_stop())
				break;
			spin_lock(&zram->prefetch_lock);
			req = list_last_entry(list, struct zram_request, list);
			while (req->first < req->last) {
				index = req->index[req->first++];
				atomic_dec(&zram->nr_prefetch);
				spin_unlock(&zram->prefetch_lock);
				try_read_from_bdev(zram, NULL, index, NULL, true);
				spin_lock(&zram->prefetch_lock);
			}
			spin_unlock(&zram->prefetch_lock);
			if (req->first == MAX_REQ_IDX) {
				spin_lock(&zram->prefetch_lock);
				list_del_init(&req->list);
				spin_unlock(&zram->prefetch_lock);
				kvfree(req);
			}
		}
	}
	return 0;
}

static bool zram_wb_available(struct zram *zram)
{
	struct loop_device *lo;
	struct inode *inode;
	struct dentry *root;
	struct kstatfs statbuf;
	u64 min_free_blocks;
	int ret;

	if (!zram->backing_dev || !zram->bdev || !zram->bdev->bd_disk)
		return false;

	lo = zram->bdev->bd_disk->private_data;
	if (!lo || !lo->lo_backing_file)
		return false;

	inode = lo->lo_backing_file->f_mapping->host;
	root = inode->i_sb->s_root;
	if (!root->d_sb->s_op->statfs)
		return false;

	ret = root->d_sb->s_op->statfs(root, &statbuf);
	if (ret)
		return false;
	/*
	 * To guarantee "reserved block(133MB on Q-os)" for system,
	 * SQZR is triggered only when devices have enough storage free space
	 * more than SZ_1G or reserved block * 2.
	 */
	min_free_blocks = max_t(u64, SZ_1G / statbuf.f_bsize,
			(statbuf.f_bfree - statbuf.f_bavail) * 2);
	if (statbuf.f_bavail < min_free_blocks)
		return false;

	spin_lock(&zram->wb_limit_lock);
	if (zram->wb_limit_enable && !zram->bd_wb_limit) {
		spin_unlock(&zram->wb_limit_lock);
		return false;
	}
	spin_unlock(&zram->wb_limit_lock);

	if (atomic64_read(&zram->stats.bd_count) == zram->nr_pages)
		return false;
	if (atomic64_read(&zram->stats.bd_objcnt) >= zram->nr_pages * 4)
		return false;

	return true;
}

static void fallocate_block(struct zram *zram, unsigned long blk_idx)
{
	struct block_device *bdev = zram->bdev;

	if (!bdev)
		return;

	mutex_lock(&zram->falloc_lock);
	/* check 2MB block bitmap. if unset, fallocate 2MB block at once */
	if (!test_and_set_bit(blk_idx / NR_FALLOC_PAGES, zram->falloc_bitmap)) {
		struct loop_device *lo = bdev->bd_disk->private_data;
		struct file *file = lo->lo_backing_file;
		loff_t pos = (blk_idx & FALLOC_ALIGN_MASK) << PAGE_SHIFT;
		loff_t len = NR_FALLOC_PAGES << PAGE_SHIFT;
		int mode = FALLOC_FL_KEEP_SIZE;
		int ret;

		file_start_write(file);
		ret = file->f_op->fallocate(file, mode, pos, len);
		if (ret)
			pr_err("%s pos %lx failed %d\n", __func__, (unsigned long)pos, ret);
		file_end_write(file);
	}
	mutex_unlock(&zram->falloc_lock);
}

static void free_writeback_buffer(struct zram_wb_buffer *buf)
{
	int i;

	for (i = 0; i < NR_ZWBS; i++)
		if (buf->page[i])
			__free_page(buf->page[i]);
	kvfree(buf);
}

static struct zram_wb_buffer *alloc_writeback_buffer(void)
{
	struct zram_wb_buffer *buf;
	int i;

	buf = kvzalloc(sizeof(struct zram_wb_buffer), GFP_KERNEL);
	if (!buf)
		return NULL;

	for (i = 0; i < NR_ZWBS; i++) {
		buf->page[i] = alloc_page(GFP_KERNEL);
		if (!buf->page[i])
			goto out;
	}
	return buf;

out:
	free_writeback_buffer(buf);
	return NULL;
}

static void mark_end_of_page(struct zram_wb_buffer *buf)
{
	struct zram_wb_header *zhdr;
	struct page *page = buf->page[buf->idx];
	int offset = buf->off[buf->idx];
	void *mem;

	if (offset + sizeof(struct zram_wb_header) <= PAGE_SIZE) {
		mem = kmap_atomic(page);
		zhdr = (struct zram_wb_header *)(mem + offset);
		zhdr->index = UINT_MAX;
		zhdr->size = 0;
		kunmap_atomic(mem);
	}
}

static int zram_writeback_fill_page(struct zram *zram,
		struct zram_wb_buffer *buf, u32 index)
{
	struct zram_wb_header *zhdr;
	unsigned long handle;
	int idx = buf->idx;
	int offset = buf->off[idx];
	int size, sizes[2];
	int header_sz = sizeof(struct zram_wb_header);
	void *src, *dst;

	zram_slot_lock(zram, index);
	if (!zram_allocated(zram, index) ||
			!zram_test_flag(zram, index, ZRAM_IDLE) ||
			zram_test_flag(zram, index, ZRAM_WB) ||
			zram_test_flag(zram, index, ZRAM_SAME) ||
			zram_test_flag(zram, index, ZRAM_UNDER_WB)) {
		zram_slot_unlock(zram, index);
		return 0;
	}
	size = zram_get_obj_size(zram, index);
	if ((idx == NR_ZWBS - 1 && offset + header_sz + size > PAGE_SIZE) ||
			offset + header_sz > PAGE_SIZE) {
		zram_slot_unlock(zram, index);
		return -ENOSPC;
	}
	/*
	 * Clearing ZRAM_UNDER_WB is duty of caller.
	 * IOW, zram_free_page never clear it.
	 */
	zram_set_flag(zram, index, ZRAM_UNDER_WB);
	/* Need for hugepage writeback racing */
	zram_set_flag(zram, index, ZRAM_IDLE);

	handle = zram_get_element(zram, index);
	if (!handle) {
		zram_clear_flag(zram, index, ZRAM_UNDER_WB);
		zram_clear_flag(zram, index, ZRAM_IDLE);
		zram_slot_unlock(zram, index);
		return -ENOENT;
	}
	src = zs_map_object(zram->mem_pool, handle, ZS_MM_RO);
	dst = kmap_atomic(buf->page[idx]);
	zhdr = (struct zram_wb_header *)(dst + offset);
	zhdr->index = index;
	zhdr->size = size;
	dst = (u8 *)(zhdr + 1);

	if (offset + header_sz + size > PAGE_SIZE) {
		sizes[0] = PAGE_SIZE - (offset + header_sz);
		sizes[1] = size - sizes[0];
		memcpy(dst, src, sizes[0]);
		kunmap_atomic(dst);
		dst = kmap_atomic(buf->page[idx + 1]);
		memcpy(dst, src + sizes[0], sizes[1]);
		buf->off[idx + 1] = sizes[1];
	} else {
		memcpy(dst, src, size);
	}
	kunmap_atomic(dst);
	check_marker(zram, NULL, 0, src, size, index);
	zs_unmap_object(zram->mem_pool, handle);
	zram_slot_unlock(zram, index);

	return size;
}

static void zram_clear_flags(struct zram *zram, struct zram_wb_buffer *buf)
{
	int i, j;
	u32 index;

	for (i = 0; i < NR_ZWBS; i++) {
		for (j = 0; j < buf->cnt[i]; j++) {
			index = buf->entry[i][j].index;
			zram_slot_lock(zram, index);
			if (zram_allocated(zram, index)) {
				zram_clear_flag(zram, index, ZRAM_UNDER_WB);
				zram_clear_flag(zram, index, ZRAM_IDLE);
			}
			zram_slot_unlock(zram, index);
		}
	}
}

static void zram_writeback_complete(struct zram_wb_work *zw)
{
	if (!atomic_dec_return(zw->refcount))
		wake_up(zw->wq);
	free_writeback_buffer(zw->buf);
	bio_put(zw->bio);
	kvfree(zw);
}

static void zram_update_max_stats(struct zram *zram)
{
	unsigned long bd_count, bd_size;

	bd_count = atomic64_read(&zram->stats.bd_count);
	if (bd_count <= atomic64_read(&zram->stats.bd_max_count))
		return;

	bd_size = atomic64_read(&zram->stats.bd_size);
	atomic64_set(&zram->stats.bd_max_count, bd_count);
	atomic64_set(&zram->stats.bd_max_size, bd_size);
}

static void zram_writeback_done(struct zram *zram, struct zram_wb_entry *entry,
				unsigned long blk_idx)
{
	u32 index = entry->index;
	int offset = entry->offset;
	int size = entry->size;

	zram_slot_lock(zram, index);
	if (!zram_allocated(zram, index) ||
			!zram_test_flag(zram, index, ZRAM_IDLE)) {
		zram_clear_flag(zram, index, ZRAM_UNDER_WB);
		zram_clear_flag(zram, index, ZRAM_IDLE);
		atomic64_dec(&zram->stats.bd_objcnt);
		free_chunk_bdev(zram, blk_to_chunk_idx(blk_idx));
		zram_slot_unlock(zram, index);
		return;
	}
	zram_free_page(zram, index);
	zram_clear_flag(zram, index, ZRAM_UNDER_WB);
	zram_set_flag(zram, index, ZRAM_WB);
	atomic64_add(size, &zram->stats.bd_size);

	zram_set_element(zram, index, to_handle(blk_idx, offset, size));
	zram_slot_unlock(zram, index);
	atomic64_inc(&zram->stats.pages_stored);
}

static void zram_writeback_done_work(struct work_struct *work)
{
	struct zram_wb_work *zw = container_of(work, struct zram_wb_work, work);
	struct zram_wb_buffer *buf = zw->buf;
	struct zram *zram = zw->zram;
	unsigned long blk_idx = zw->handle;
	unsigned long flags;
	int count = 0;
	int i, j;

	for (i = 0; i < NR_ZWBS; i++)
		count += buf->cnt[i];

	spin_lock_irqsave(&zram->refcount_lock, flags);
	if (!zram->refcount_table) {
		spin_unlock_irqrestore(&zram->refcount_lock, flags);
		return;
	}
	zram->refcount_table[blk_to_chunk_idx(blk_idx)] = count;
	spin_unlock_irqrestore(&zram->refcount_lock, flags);
	atomic64_add(count, &zram->stats.bd_objwrites);
	atomic64_add(count, &zram->stats.bd_objcnt);

	for (i = 0; i < NR_ZWBS; blk_idx++, i++)
		for (j = 0; j < buf->cnt[i]; j++)
			zram_writeback_done(zram, &buf->entry[i][j], blk_idx);

	zram_update_max_stats(zram);
	atomic64_add(NR_ZWBS, &zram->stats.bd_writes);
	spin_lock(&zram->wb_limit_lock);
	if (zram->wb_limit_enable)
		zram->bd_wb_limit -= min_t(u64, NR_ZWBS, zram->bd_wb_limit);
	spin_unlock(&zram->wb_limit_lock);
	zram_writeback_complete(zw);
}

static void zram_writeback_end_io(struct bio *bio)
{
	struct page *page = bio->bi_io_vec[0].bv_page;
	struct zram_wb_work *zw = (struct zram_wb_work *)page_private(page);
	int errno = blk_status_to_errno(bio->bi_status);

	if (errno) {
		pr_err("%s write bio returned errno %d\n", __func__, errno);
		zram_clear_flags(zw->zram, zw->buf);
		zram_writeback_complete(zw);
		return;
	}
	INIT_WORK(&zw->work, zram_writeback_done_work);
	schedule_work(&zw->work);
}

static int zram_writeback_page(struct zram *zram, struct zram_wb_buffer *buf,
				wait_queue_head_t *wq, atomic_t *refcount)
{
	struct zram_wb_work *zw;
	struct bio *bio;
	unsigned long chunk_idx;
	unsigned long blk_idx;
	int i;

	chunk_idx = alloc_chunk_bdev(zram);
	if (!chunk_idx) {
		zram_clear_flags(zram, buf);
		free_writeback_buffer(buf);
		return -ENOMEM;
	}
	blk_idx = chunk_to_blk_idx(chunk_idx);
	/* fallocate 2MB block if not allocated yet */
	fallocate_block(zram, blk_idx);

	bio = bio_alloc(zram->bdev, NR_ZWBS, REQ_OP_WRITE, GFP_KERNEL);
	zw = kvzalloc(sizeof(struct zram_wb_work), GFP_KERNEL);
	zw->zram = zram;
	zw->bio = bio;
	zw->handle = blk_idx;
	zw->buf = buf;
	zw->wq = wq;
	zw->refcount = refcount;
	atomic_inc(refcount);
	set_page_private(buf->page[0], (unsigned long)zw);
	bio->bi_end_io = zram_writeback_end_io;
	bio->bi_iter.bi_sector = blk_idx * (PAGE_SIZE >> 9);
	for (i = 0; i < NR_ZWBS; i++)
		__bio_add_page(bio, buf->page[i], PAGE_SIZE, 0);
	submit_bio(bio);
	return 0;
}

static int zram_writeback_index(struct zram *zram,
		struct zram_wb_buffer **bufptr, u32 index,
		wait_queue_head_t *wq, atomic_t *refcount)
{
	struct zram_wb_buffer *buf = *bufptr;
	struct zram_wb_entry *entry;
	int size, i, ret = 0;

retry:
	/* allocate new buffer for writeback */
	if (buf == NULL) {
		buf = alloc_writeback_buffer();
		if (buf == NULL)
			return -ENOMEM;
	}
	i = buf->idx;

	size = zram_writeback_fill_page(zram, buf, index);
	if (size > 0) {
		entry = &buf->entry[i][buf->cnt[i]];
		entry->index = index;
		entry->offset = buf->off[i];
		entry->size = size;
		buf->off[i] += (size + sizeof(struct zram_wb_header));
		buf->cnt[i]++;
	}
	/* writeback if page is full/entry is full */
	if (size == -ENOSPC || buf->cnt[i] == ZRAM_WB_THRESHOLD) {
		mark_end_of_page(buf);
		if (++buf->idx == NR_ZWBS) {
			ret = zram_writeback_page(zram, buf, wq, refcount);
			buf = NULL;
		}
		if (ret == 0)
			goto retry;
	}
	*bufptr = buf;
	return ret;
}

int zram_writeback_list(struct zram *zram, struct list_head *list)
{
	struct zram_request *req;
	struct zram_wb_buffer *buf = NULL;
	DECLARE_WAIT_QUEUE_HEAD_ONSTACK(wq);
	atomic_t refcount = ATOMIC_INIT(1);
	int ret = 0;
	u32 index;

	while (!list_empty(list)) {
		req = list_last_entry(list, struct zram_request, list);
		while (req->first < req->last) {
			index = req->index[req->first++];
			if (ret || am_app_launch || !zram_wb_available(zram)) {
				ret = -EBUSY;
				zram_slot_lock(zram, index);
				zram_clear_flag(zram, index, ZRAM_IDLE);
				zram_slot_unlock(zram, index);
				continue;
			}
			zram_writeback_index(zram, &buf, index, &wq, &refcount);
		}
		list_del_init(&req->list);
		kvfree(req);
	}

	if (!ret && buf && buf->cnt[0] && zram_wb_available(zram)) {
		/* mark end of pages */
		for (; buf->idx < NR_ZWBS; buf->idx++)
			mark_end_of_page(buf);
		zram_writeback_page(zram, buf, &wq, &refcount);
	} else if (buf) {
		zram_clear_flags(zram, buf);
		free_writeback_buffer(buf);
	}
	atomic_dec(&refcount);
	/* wait until all writeback requests are completed */
	while (atomic_read(&refcount) > 0)
		wait_event_timeout(wq, !atomic_read(&refcount), HZ);
	return ret;
}

void zram_request_prefetch(struct zram *zram, u32 index)
{
	struct zram_request *req;
	struct list_head *list = &zram->prefetch_list;
	unsigned long handle;
	unsigned long blk_idx;
	unsigned long chunk_idx;
	static unsigned long prev_idx;

	if (!zram_slot_trylock(zram, index))
		return;
	if (!zram_allocated(zram, index) ||
			!zram_test_flag(zram, index, ZRAM_WB)) {
		zram_slot_unlock(zram, index);
		return;
	}
	handle = zram_get_element(zram, index);
	blk_idx = handle >> IDX_SHIFT;
	chunk_idx = blk_to_chunk_idx(blk_idx);
	if (chunk_idx == prev_idx ||
			test_bit(chunk_idx, zram->read_bitmap)) {
		zram_slot_unlock(zram, index);
		return;
	}
	prev_idx = chunk_idx;
	zram_slot_unlock(zram, index);

	spin_lock(&zram->prefetch_lock);
	req = list_first_entry_or_null(list, struct zram_request, list);
	if (!req || req->last == MAX_REQ_IDX) {
		spin_unlock(&zram->prefetch_lock);
		req = kvzalloc(sizeof(struct zram_request), GFP_NOIO);
		if (!req)
			return;
		INIT_LIST_HEAD(&req->list);
		spin_lock(&zram->prefetch_lock);
		list_add(&req->list, list);
	}
	req->index[req->last++] = index;
	atomic_inc(&zram->nr_prefetch);
	spin_unlock(&zram->prefetch_lock);
	wake_up(&zram->prefetch_wait);
}

void zram_request_writeback(struct zram *zram,
		struct list_head *list, u32 index)
{
	struct zram_request *req;

	if (!zram_wb_available(zram))
		return;
	if (index >= (zram->disksize >> PAGE_SHIFT))
		return;
	if (!zram_slot_trylock(zram, index))
		return;
	if (!zram_allocated(zram, index) ||
			zram_test_flag(zram, index, ZRAM_IDLE) ||
			zram_test_flag(zram, index, ZRAM_WB) ||
			zram_test_flag(zram, index, ZRAM_SAME) ||
			zram_test_flag(zram, index, ZRAM_UNDER_WB)) {
		zram_slot_unlock(zram, index);
		return;
	}
	zram_set_flag(zram, index, ZRAM_IDLE);
	zram_slot_unlock(zram, index);

	req = list_first_entry_or_null(list, struct zram_request, list);
	if (!req || req->last == MAX_REQ_IDX) {
		req = kvzalloc(sizeof(struct zram_request), GFP_NOIO);
		if (!req)
			return;
		INIT_LIST_HEAD(&req->list);
		list_add(&req->list, list);
	}
	req->index[req->last++] = index;
}

static void zram_count_shared(void *data, unsigned long *swap_shared)
{
	(*swap_shared)++;
}

static void zram_show_shared(void *data, struct seq_file *m,
		unsigned long swap_shared)
{
	char str[100];
	char *p = str;

	p += sprintf(p, "SwapShared:     %8lu kB\n", K(swap_shared));
	seq_puts(m, str);

}

static void zram_count_entry_type(void *data, swp_entry_t swpent,
		unsigned long *writeback,
		unsigned long *same, unsigned long *huge)
{
	struct zram *zram = (struct zram *)data;
	u32 index = swp_offset(swpent);

	if (index >= (zram->disksize >> PAGE_SHIFT))
		return;

	zram_slot_lock(zram, index);
	if (zram_allocated(zram, index)) {
		if (zram_test_flag(zram, index, ZRAM_WB))
			(*writeback)++;
		else if (zram_test_flag(zram, index, ZRAM_SAME))
			(*same)++;
		else if (zram_test_flag(zram, index, ZRAM_HUGE))
			(*huge)++;
	}
	zram_slot_unlock(zram, index);
}

static void zram_show_entry_type(void *data, struct seq_file *m,
		unsigned long writeback,
		unsigned long same, unsigned long huge)
{
	char str[100];
	char *p = str;

	p += sprintf(p, "Writeback:      %8lu kB\n", K(writeback));
	p += sprintf(p, "Same:           %8lu kB\n", K(same));
	p += sprintf(p, "Huge:           %8lu kB\n", K(huge));
	seq_puts(m, str);
}

#define FOUR_K(x) ((x) * (1 << (PAGE_SHIFT - 12)))
ssize_t bd_stat_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct zram *zram = dev_to_zram(dev);
	ssize_t ret;

	down_read(&zram->init_lock);
	ret = scnprintf(buf, PAGE_SIZE,
			"%8d %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8d "
			"%8d %8d %8d %8d %8d %8d %8llu %8llu\n",
			0,
			FOUR_K((u64)atomic64_read(&zram->stats.bd_count)),
			FOUR_K((u64)atomic64_read(&zram->stats.bd_reads)),
			FOUR_K((u64)atomic64_read(&zram->stats.bd_writes)),
			FOUR_K((u64)atomic64_read(&zram->stats.bd_objcnt)),
			(u64)(atomic64_read(&zram->stats.bd_size) >> PAGE_SHIFT),
			FOUR_K((u64)atomic64_read(&zram->stats.bd_max_count)),
			(u64)(atomic64_read(&zram->stats.bd_max_size) >> PAGE_SHIFT),
			0, 0, 0, 0, 0, 0, 0,
			FOUR_K((u64)atomic64_read(&zram->stats.bd_objreads)),
			FOUR_K((u64)atomic64_read(&zram->stats.bd_objwrites)));
	up_read(&zram->init_lock);

	return ret;
}

ssize_t bd_stat_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t len)
{
	struct zram *zram = dev_to_zram(dev);

	atomic64_set(&zram->stats.bd_max_count, 0);
	atomic64_set(&zram->stats.bd_max_size, 0);

	return len;
}

static long get_zram_total_kbytes(struct zram *zram)
{
	unsigned long kbytes;

	if (!zram || !down_read_trylock(&zram->init_lock))
		return 0;

	if (!init_done(zram) || !zram->mem_pool)
		kbytes = 0;
	else
		kbytes = zs_get_total_pages(zram->mem_pool) << (PAGE_SHIFT - 10);
	up_read(&zram->init_lock);

	return kbytes;
}

static void zram_show_mem(void *data, unsigned int filter, nodemask_t *nodemask)
{
	struct zram *zram = data;
	long total_kbytes = get_zram_total_kbytes(zram);

	if (total_kbytes == 0)
		return;

	pr_info("%s: %ld kB\n", zram->disk->disk_name, total_kbytes);
}

static void zram_meminfo(void *data, struct seq_file *m)
{
	struct zram *zram = data;
	long total_kbytes = get_zram_total_kbytes(zram);

	if (total_kbytes == 0)
		return;

	show_val_meminfo(m, zram->disk->disk_name, total_kbytes);
}

void zram_error_count_store(struct zram *zram, int type)
{
	atomic64_inc(&zram->stats.error_count[type]);
	pr_info("zram error count: %llu %llu\n",
			(u64)atomic64_read(&zram->stats.error_count[0]),
			(u64)atomic64_read(&zram->stats.error_count[1]));
	BUG_ON(force_upload_mode());
}

ssize_t zram_error_count_show(struct zram *zram, char *buf, ssize_t size)
{
	return scnprintf(buf, size, "%8llu %8llu\n",
			(u64)atomic64_read(&zram->stats.error_count[0]),
			(u64)atomic64_read(&zram->stats.error_count[1]));
}

void deinit_zram_ext(struct zram *zram)
{
	unsigned long flags;
	u16 *refcount_table = zram->refcount_table;

	if (zram->read_work) {
		kvfree(zram->read_work);
		zram->read_work = NULL;
	}
	if (zram->read_bitmap) {
		kvfree(zram->read_bitmap);
		zram->read_bitmap = NULL;
	}
	if (zram->chunk_bitmap) {
		kvfree(zram->chunk_bitmap);
		zram->chunk_bitmap = NULL;
	}
	if (zram->falloc_bitmap) {
		kvfree(zram->falloc_bitmap);
		zram->falloc_bitmap = NULL;
	}
	spin_lock_irqsave(&zram->refcount_lock, flags);
	zram->refcount_table = NULL;
	spin_unlock_irqrestore(&zram->refcount_lock, flags);

	kvfree(refcount_table);
	exit_zram_madvise();
	unregister_trace_android_vh_smaps_swap_shared(zram_count_shared, zram);
	unregister_trace_android_vh_show_smap_swap_shared(zram_show_shared, zram);
	unregister_trace_android_vh_smaps_pte_entry(zram_count_entry_type, zram);
	unregister_trace_android_vh_show_smap(zram_show_entry_type, zram);
	unregister_trace_android_vh_show_mem(zram_show_mem, zram);
	unregister_trace_android_vh_meminfo_proc_show(zram_meminfo, zram);
}

int init_zram_ext(struct zram *zram, unsigned long nr_pages, unsigned int size)
{
	/* backing dev should be large enough */
	if (size < max_t(int, NR_FALLOC_PAGES, NR_ZWBS))
		return -EINVAL;

	zram->falloc_bitmap = kvzalloc(size / NR_FALLOC_PAGES, GFP_KERNEL);
	if (!zram->falloc_bitmap)
		goto out;
	zram->chunk_bitmap = kvzalloc(size / NR_ZWBS, GFP_KERNEL);
	if (!zram->chunk_bitmap)
		goto out;
	zram->read_bitmap = kvzalloc(size / NR_ZWBS, GFP_KERNEL);
	if (!zram->read_bitmap)
		goto out;
	size = nr_pages * sizeof(void *) / NR_ZWBS;
	zram->read_work = kvzalloc(size, GFP_KERNEL);
	if (!zram->read_work)
		goto out;
	size = nr_pages * sizeof(*zram->refcount_table);
	zram->refcount_table = kvzalloc(size, GFP_KERNEL);
	if (!zram->refcount_table)
		goto out;
	mutex_init(&zram->falloc_lock);
	spin_lock_init(&zram->bitmap_lock);
	spin_lock_init(&zram->prefetch_lock);
	spin_lock_init(&zram->read_work_lock);
	INIT_LIST_HEAD(&zram->prefetch_list);
	init_waitqueue_head(&zram->prefetch_wait);
	zram->prefetchd = kthread_run(zram_prefetchd, zram,
			"%s_prefetchd", zram->disk->disk_name);
	if (IS_ERR(zram->prefetchd))
		goto out;
	zram->wb_limit_enable = true;
	init_zram_madvise(zram);
	register_trace_android_vh_smaps_swap_shared(zram_count_shared, zram);
	register_trace_android_vh_show_smap_swap_shared(zram_show_shared, zram);
	register_trace_android_vh_smaps_pte_entry(zram_count_entry_type, zram);
	register_trace_android_vh_show_smap(zram_show_entry_type, zram);
	register_trace_android_vh_show_mem(zram_show_mem, zram);
	register_trace_android_vh_meminfo_proc_show(zram_meminfo, zram);

	return 0;
out:
	deinit_zram_ext(zram);
	return -ENOMEM;
}
#endif
