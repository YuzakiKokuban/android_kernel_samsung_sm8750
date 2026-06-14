// SPDX-License-Identifier: GPL-2.0
/*
 *  Write Booster of SamSung Generic I/O scheduler
 *
 *  Copyright (C) 2022 Jisoo Oh <jisoo2146.oh@samsung.com>
 *  Copyright (C) 2023 Changheun Lee <nanich.lee@samsung.com>
 */

#include <linux/blkdev.h>
#include <linux/sbitmap.h>
#include <linux/blk-mq.h>

#include "elevator.h"
#include "blk-mq.h"
#include "blk-sec.h"
#include "ssg.h"

struct wb_params {
	int on_rqs;
	int off_rqs;
	int on_dirty_bytes;
	int off_dirty_bytes;
	int on_sync_write_bytes;
	int off_sync_write_bytes;
	int on_dirty_busy_written_pages;
	int on_dirty_busy_jiffies;
	int off_delay_jiffies;
	unsigned long dirty_busy_start_jiffies;
	unsigned long dirty_busy_start_written_pages;
};

struct ssg_wb_data {
	struct wb_params params;
	struct request_queue *queue;
	struct delayed_work wb_ctrl_work;
	struct delayed_work wb_deferred_off_work;

	atomic_t wb_triggered;
};

struct io_amount_data {
	unsigned int allocated_rqs;
	unsigned int sync_write_bytes;
	unsigned long dirty_bytes;
};

struct ssg_wb_iter_data {
	struct blk_mq_tags *tags;
	void *data;
	bool reserved;
};

static const int _on_rqs_ratio = 90;
static const int _off_rqs_ratio = 40;
static const int _on_dirty_bytes = 50*1024*1024;
static const int _off_dirty_bytes = 25*1024*1024;
static const int _on_sync_write_bytes = 2*1024*1024;
static const int _off_sync_write_bytes = 1*1024*1024;
static const int _on_dirty_busy_written_bytes = 100*1024*1024;
static const int _on_dirty_busy_msecs = 1000;
static const int _off_delay_msecs = 5000;

#define may_wb_on(io_amount, params) \
	((io_amount).allocated_rqs >= params.on_rqs || \
	(io_amount).dirty_bytes >= params.on_dirty_bytes || \
	(io_amount).sync_write_bytes >= params.on_sync_write_bytes || \
	(params.dirty_busy_start_written_pages && \
	(global_node_page_state(NR_WRITTEN) - params.dirty_busy_start_written_pages) \
	> params.on_dirty_busy_written_pages))

#define may_wb_off(io_amount, params) \
	((io_amount).allocated_rqs < params.off_rqs && \
	(io_amount).dirty_bytes < params.off_dirty_bytes && \
	(io_amount).sync_write_bytes < params.off_sync_write_bytes)

static void trigger_wb_on(struct ssg_wb_data *ssg_wb)
{
	cancel_delayed_work_sync(&ssg_wb->wb_deferred_off_work);
	blk_sec_wb_ctrl(true, WB_REQ_IOSCHED);
	atomic_set(&ssg_wb->wb_triggered, true);
}

static void wb_off_work(struct work_struct *work)
{
	blk_sec_wb_ctrl(false, WB_REQ_IOSCHED);
}

static void trigger_wb_off(struct ssg_wb_data *ssg_wb)
{
	queue_delayed_work(blk_sec_common_wq,
				&ssg_wb->wb_deferred_off_work, ssg_wb->params.off_delay_jiffies);

	atomic_set(&ssg_wb->wb_triggered, false);
}

static void init_wb_params(struct ssg_wb_data *ssg_wb)
{
	ssg_wb->params.on_rqs = ssg_wb->queue->nr_requests * _on_rqs_ratio / 100U;
	ssg_wb->params.off_rqs = ssg_wb->queue->nr_requests * _off_rqs_ratio / 100U;
	ssg_wb->params.on_dirty_bytes = _on_dirty_bytes;
	ssg_wb->params.off_dirty_bytes = _off_dirty_bytes;
	ssg_wb->params.on_sync_write_bytes = _on_sync_write_bytes;
	ssg_wb->params.off_sync_write_bytes = _off_sync_write_bytes;
	ssg_wb->params.on_dirty_busy_written_pages = _on_dirty_busy_written_bytes / PAGE_SIZE;
	ssg_wb->params.on_dirty_busy_jiffies = msecs_to_jiffies(_on_dirty_busy_msecs);
	ssg_wb->params.off_delay_jiffies = msecs_to_jiffies(_off_delay_msecs);
	ssg_wb->params.dirty_busy_start_written_pages = 0;
	ssg_wb->params.dirty_busy_start_jiffies = 0;
}

static bool wb_count_io(struct sbitmap *bitmap, unsigned int bitnr, void *data)
{
	struct ssg_wb_iter_data *iter_data = data;
	struct blk_mq_tags *tags = iter_data->tags;
	struct io_amount_data *io_amount = iter_data->data;
	bool reserved = iter_data->reserved;
	struct request *rq;

	if (!reserved)
		bitnr += tags->nr_reserved_tags;

	rq = tags->static_rqs[bitnr];
	if (!rq)
		return true;

	io_amount->allocated_rqs++;
	if (req_op(rq) == REQ_OP_WRITE && rq->cmd_flags & REQ_SYNC)
		io_amount->sync_write_bytes += blk_rq_bytes(rq);

	return true;
}

static void wb_all_tag_iter(struct blk_mq_tags *tags, struct io_amount_data *io_amount)
{
	struct ssg_wb_iter_data iter_data = {
		.data = io_amount,
		.tags = tags,
	};

	if (tags->nr_reserved_tags) {
		iter_data.reserved = true;
		sbitmap_for_each_set(&tags->breserved_tags.sb, wb_count_io, &iter_data);
	}

	iter_data.reserved = false;
	sbitmap_for_each_set(&tags->bitmap_tags.sb, wb_count_io, &iter_data);
}

static void wb_get_io_amount(struct request_queue *q, struct io_amount_data *io_amount)
{
	struct blk_mq_hw_ctx *hctx;
	unsigned long i;

	if (blk_mq_is_shared_tags(q->tag_set->flags)) {
		wb_all_tag_iter(q->sched_shared_tags, io_amount);
	} else {
		queue_for_each_hw_ctx(q, hctx, i) {
			/*
			 * If no software queues are currently mapped to this
			 * hardware queue, there's nothing to check
			 */
			if (!blk_mq_hw_queue_mapped(hctx))
				continue;

			wb_all_tag_iter(hctx->sched_tags, io_amount);
		}
	}

	io_amount->dirty_bytes = (global_node_page_state(NR_FILE_DIRTY) +
			global_node_page_state(NR_WRITEBACK)) * PAGE_SIZE;
}

static void update_dirty_busy_status(struct ssg_wb_data *ssg_wb, struct io_amount_data *io_amount)
{
	if (time_after(jiffies, ssg_wb->params.dirty_busy_start_jiffies + ssg_wb->params.on_dirty_busy_jiffies)) {
		ssg_wb->params.dirty_busy_start_jiffies = 0;
		ssg_wb->params.dirty_busy_start_written_pages = 0;
	}

	if (!ssg_wb->params.dirty_busy_start_jiffies && io_amount->dirty_bytes >= ssg_wb->params.off_dirty_bytes) {
		ssg_wb->params.dirty_busy_start_jiffies = jiffies;
		ssg_wb->params.dirty_busy_start_written_pages = global_node_page_state(NR_WRITTEN);
	}
}

static void wb_ctrl_work(struct work_struct *work)
{
	struct ssg_wb_data *ssg_wb = container_of(to_delayed_work(work),
						struct ssg_wb_data, wb_ctrl_work);
	struct io_amount_data io_amount = {
		.allocated_rqs = 0,
		.sync_write_bytes = 0,
	};

	wb_get_io_amount(ssg_wb->queue, &io_amount);
	update_dirty_busy_status(ssg_wb, &io_amount);

	if (atomic_read(&ssg_wb->wb_triggered)) {
		if (may_wb_off(io_amount, ssg_wb->params))
			trigger_wb_off(ssg_wb);
	} else {
		if (may_wb_on(io_amount, ssg_wb->params))
			trigger_wb_on(ssg_wb);
	}

	if (atomic_read(&ssg_wb->wb_triggered))
		queue_delayed_work(blk_sec_common_wq, &ssg_wb->wb_ctrl_work,
				ssg_wb->params.off_delay_jiffies);
}

void ssg_wb_run_ctrl_work(struct ssg_data *ssg, struct request *rq)
{
	struct ssg_wb_data *ssg_wb = ssg->wb_data;

	if (!ssg_wb)
		return;

	if (atomic_read(&ssg_wb->wb_triggered))
		return;

	if (((rq->cmd_flags & REQ_OP_MASK) == REQ_OP_READ)
			&& atomic_read(&ssg->allocated_rqs) < ssg_wb->params.on_rqs)
		return;

	if (!work_busy(&ssg_wb->wb_ctrl_work.work))
		queue_delayed_work(blk_sec_common_wq, &ssg_wb->wb_ctrl_work, 0);
}

void ssg_wb_depth_updated(struct blk_mq_hw_ctx *hctx)
{
	struct request_queue *q = hctx->queue;
	struct ssg_data *ssg = q->elevator->elevator_data;
	struct ssg_wb_data *ssg_wb = ssg->wb_data;
	int nr_rqs;

	if (!ssg_wb)
		return;

	nr_rqs = hctx->sched_tags->bitmap_tags.sb.depth;
	ssg_wb->params.on_rqs = nr_rqs * _on_rqs_ratio / 100U;
	ssg_wb->params.off_rqs = nr_rqs * _off_rqs_ratio / 100U;
}

void ssg_wb_init(struct ssg_data *ssg)
{
	struct ssg_wb_data *ssg_wb;
	struct gendisk *gd = ssg->queue->disk;

	if (!gd)
		return;

	if (!blk_sec_wb_is_supported(gd))
		return;

	ssg_wb = kzalloc(sizeof(*ssg_wb), GFP_KERNEL);
	if (!ssg_wb)
		return;

	ssg_wb->queue = ssg->queue;

	INIT_DELAYED_WORK(&ssg_wb->wb_ctrl_work, wb_ctrl_work);
	INIT_DELAYED_WORK(&ssg_wb->wb_deferred_off_work, wb_off_work);

	init_wb_params(ssg_wb);

	atomic_set(&ssg_wb->wb_triggered, false);
	ssg->wb_data = ssg_wb;
}

void ssg_wb_exit(struct ssg_data *ssg)
{
	struct ssg_wb_data *ssg_wb = ssg->wb_data;

	if (!ssg_wb)
		return;

	cancel_delayed_work_sync(&ssg_wb->wb_ctrl_work);
	cancel_delayed_work_sync(&ssg_wb->wb_deferred_off_work);

	if (atomic_read(&ssg_wb->wb_triggered))
		blk_sec_wb_ctrl(false, WB_REQ_IOSCHED);

	ssg->wb_data = NULL;
	kfree(ssg_wb);
}

/* sysfs */
#define SHOW_FUNC(__NAME, __VAR, __CONV)				\
ssize_t ssg_wb_##__NAME##_show(struct elevator_queue *e, char *page)	\
{									\
	struct ssg_data *ssg = e->elevator_data;			\
	struct ssg_wb_data *ssg_wb = ssg->wb_data;			\
	int val;							\
									\
	if (!ssg_wb)							\
		return 0;						\
									\
	if (__CONV == 1)						\
		val = jiffies_to_msecs(__VAR);				\
	else if (__CONV == 2)						\
		val = __VAR * PAGE_SIZE;				\
	else								\
		val = __VAR;						\
									\
	return snprintf(page, PAGE_SIZE, "%d\n", val);			\
}
SHOW_FUNC(on_rqs, ssg_wb->params.on_rqs, 0);
SHOW_FUNC(off_rqs, ssg_wb->params.off_rqs, 0);
SHOW_FUNC(on_dirty_bytes, ssg_wb->params.on_dirty_bytes, 0);
SHOW_FUNC(off_dirty_bytes, ssg_wb->params.off_dirty_bytes, 0);
SHOW_FUNC(on_sync_write_bytes, ssg_wb->params.on_sync_write_bytes, 0);
SHOW_FUNC(off_sync_write_bytes, ssg_wb->params.off_sync_write_bytes, 0);
SHOW_FUNC(on_dirty_busy_written_bytes, ssg_wb->params.on_dirty_busy_written_pages, 2);
SHOW_FUNC(on_dirty_busy_msecs, ssg_wb->params.on_dirty_busy_jiffies, 1);
SHOW_FUNC(off_delay_msecs, ssg_wb->params.off_delay_jiffies, 1);
#undef SHOW_FUNC

#define STORE_FUNC(__NAME, __PTR, __VAR, __COND, __CONV)	\
ssize_t ssg_wb_##__NAME##_store(struct elevator_queue *e,	\
				const char *page, size_t count)	\
{								\
	struct ssg_data *ssg = e->elevator_data;		\
	struct ssg_wb_data *ssg_wb = ssg->wb_data;		\
	int __VAR;						\
								\
	if (!ssg_wb)						\
		return count;					\
								\
	if (kstrtoint(page, 10, &__VAR))			\
		return count;					\
								\
	if (!(__COND))						\
		return count;					\
								\
	if (__CONV == 1)					\
		*(__PTR) = msecs_to_jiffies(__VAR);		\
	else if (__CONV == 2)					\
		*(__PTR) = __VAR / PAGE_SIZE;			\
	else							\
		*(__PTR) = __VAR;				\
								\
	return count;						\
}
STORE_FUNC(on_rqs, &ssg_wb->params.on_rqs, val,
		val >= ssg_wb->params.off_rqs, 0);
STORE_FUNC(off_rqs, &ssg_wb->params.off_rqs, val,
		val >= 0 && val <= ssg_wb->params.on_rqs, 0);
STORE_FUNC(on_dirty_bytes, &ssg_wb->params.on_dirty_bytes, val,
		val >= ssg_wb->params.off_dirty_bytes, 0);
STORE_FUNC(off_dirty_bytes, &ssg_wb->params.off_dirty_bytes, val,
		val >= 0 && val <= ssg_wb->params.on_dirty_bytes, 0);
STORE_FUNC(on_sync_write_bytes, &ssg_wb->params.on_sync_write_bytes, val,
		val >= ssg_wb->params.off_sync_write_bytes, 0);
STORE_FUNC(off_sync_write_bytes, &ssg_wb->params.off_sync_write_bytes, val,
		val >= 0 && val <= ssg_wb->params.on_sync_write_bytes, 0);
STORE_FUNC(on_dirty_busy_written_bytes, &ssg_wb->params.on_dirty_busy_written_pages, val,
		val >= 0, 2);
STORE_FUNC(on_dirty_busy_msecs, &ssg_wb->params.on_dirty_busy_jiffies, val,
		val >= 0, 1);
STORE_FUNC(off_delay_msecs, &ssg_wb->params.off_delay_jiffies, val, val >= 0, 1);
#undef STORE_FUNC

ssize_t ssg_wb_triggered_show(struct elevator_queue *e, char *page)
{
	struct ssg_data *ssg = e->elevator_data;
	struct ssg_wb_data *ssg_wb = ssg->wb_data;

	if (!ssg_wb)
		return 0;

	return snprintf(page, PAGE_SIZE, "%d\n", atomic_read(&ssg_wb->wb_triggered));
}
