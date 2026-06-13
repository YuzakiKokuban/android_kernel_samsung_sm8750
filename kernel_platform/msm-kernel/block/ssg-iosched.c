// SPDX-License-Identifier: GPL-2.0
/*
 *  SamSung Generic I/O scheduler
 *  for the blk-mq scheduling framework
 *
 *  Copyright (C) 2021 Jisoo Oh <jisoo2146.oh@samsung.com>
 *  Copyright (C) 2021 Manjong Lee <mj0123.lee@samsung.com>
 *  Copyright (C) 2021 Changheun Lee <nanich.lee@samsung.com>
 */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/compiler.h>
#include <linux/rbtree.h>
#include <linux/sbitmap.h>

#include <trace/events/block.h>

#include "blk.h"
#include "elevator.h"
#include "blk-mq.h"
#include "blk-mq-debugfs.h"
#include "blk-mq-sched.h"
#include "ssg.h"
#include "blk-sec.h"

#define MAX_ASYNC_WRITE_RQS	8

/* ssg data direction definitions */
#define BOOST 2	/* read 0, write 1 */

static const int read_expire = HZ / 2;		/* max time before a read is submitted. */
static const int write_expire = 5 * HZ;		/* ditto for writes, these limits are SOFT! */
static const int boost_expire = HZ / 2;		/* max time before a boost is submitted. */
static const int max_write_starvation = 2;	/* max times reads can starve a write */
static const int max_normal_starvation = 2;	/* max times boosts can starve normal reads or writes */
static const int congestion_threshold = 90;	/* percentage of congestion threshold */
static const int max_tgroup_io_ratio = 50;	/* maximum service ratio for each thread group */
static const int max_async_write_ratio = 25;	/* maximum service ratio for async write */

static inline struct ssg_request_info *ssg_rq_info(struct ssg_data *ssg,
		struct request *rq)
{
	if (unlikely(!ssg->rq_info))
		return NULL;

	if (unlikely(!rq))
		return NULL;

	if (unlikely(rq->internal_tag < 0))
		return NULL;

	if (unlikely(rq->internal_tag >= rq->q->nr_requests))
		return NULL;

	return &ssg->rq_info[rq->internal_tag];
}

static inline int ssg_rq_data_dir(struct ssg_data *ssg, struct request *rq)
{
	struct ssg_request_info *rqi = ssg_rq_info(ssg, rq);

	if (likely(rqi) && rqi->blkcg_boost)
		return BOOST;

	return rq_data_dir(rq);
}

static inline struct rb_root *ssg_rb_root(struct ssg_data *ssg, struct request *rq)
{
	return &ssg->sort_list[ssg_rq_data_dir(ssg, rq)];
}

/*
 * get the request after `rq' in sector-sorted order
 */
static inline struct request *ssg_latter_request(struct request *rq)
{
	struct rb_node *node = rb_next(&rq->rb_node);

	if (node)
		return rb_entry_rq(node);

	return NULL;
}

static void ssg_add_rq_rb(struct ssg_data *ssg, struct request *rq)
{
	struct rb_root *root = ssg_rb_root(ssg, rq);

	elv_rb_add(root, rq);
}

static inline void ssg_del_rq_rb(struct ssg_data *ssg, struct request *rq)
{
	const int data_dir = ssg_rq_data_dir(ssg, rq);

	if (ssg->next_rq[data_dir] == rq)
		ssg->next_rq[data_dir] = ssg_latter_request(rq);

	elv_rb_del(ssg_rb_root(ssg, rq), rq);
}

/*
 * remove rq from rbtree and fifo.
 */
static void ssg_remove_request(struct request_queue *q, struct request *rq)
{
	struct ssg_data *ssg = q->elevator->elevator_data;

	list_del_init(&rq->queuelist);

	/*
	 * We might not be on the rbtree, if we are doing an insert merge
	 */
	if (!RB_EMPTY_NODE(&rq->rb_node))
		ssg_del_rq_rb(ssg, rq);

	elv_rqhash_del(q, rq);
	if (q->last_merge == rq)
		q->last_merge = NULL;
}

static void ssg_request_merged(struct request_queue *q, struct request *req,
			      enum elv_merge type)
{
	struct ssg_data *ssg = q->elevator->elevator_data;

	/*
	 * if the merge was a front merge, we need to reposition request
	 */
	if (type == ELEVATOR_FRONT_MERGE) {
		elv_rb_del(ssg_rb_root(ssg, req), req);
		ssg_add_rq_rb(ssg, req);
	}
}

static void ssg_merged_requests(struct request_queue *q, struct request *req,
			       struct request *next)
{
	/*
	 * if next expires before rq, assign its expire time to rq
	 * and move into next position (next will be deleted) in fifo
	 */
	if (!list_empty(&req->queuelist) && !list_empty(&next->queuelist)) {
		if (time_before((unsigned long)next->fifo_time,
				(unsigned long)req->fifo_time)) {
			list_move(&req->queuelist, &next->queuelist);
			req->fifo_time = next->fifo_time;
		}
	}

	/*
	 * kill knowledge of next, this one is a goner
	 */
	ssg_remove_request(q, next);
}

/*
 * move an entry to dispatch queue
 */
static void ssg_move_request(struct ssg_data *ssg, struct request *rq)
{
	const int data_dir = ssg_rq_data_dir(ssg, rq);

	ssg->next_rq[READ] = NULL;
	ssg->next_rq[WRITE] = NULL;
	ssg->next_rq[BOOST] = NULL;
	ssg->next_rq[data_dir] = ssg_latter_request(rq);

	/*
	 * take it off the sort and fifo list
	 */
	ssg_remove_request(rq->q, rq);
}

/*
 * ssg_check_fifo returns 0 if there are no expired requests on the fifo,
 * 1 otherwise. Requires !list_empty(&ssg->fifo_list[data_dir])
 */
static inline int ssg_check_fifo(struct ssg_data *ssg, int ddir)
{
	struct request *rq = rq_entry_fifo(ssg->fifo_list[ddir].next);

	/*
	 * rq is expired!
	 */
	if (time_after_eq(jiffies, (unsigned long)rq->fifo_time))
		return 1;

	return 0;
}

/*
 * For the specified data direction, return the next request to
 * dispatch using arrival ordered lists.
 */
static struct request *ssg_fifo_request(struct ssg_data *ssg, int data_dir)
{
	if (WARN_ON_ONCE(data_dir != READ && data_dir != WRITE && data_dir != BOOST))
		return NULL;

	if (list_empty(&ssg->fifo_list[data_dir]))
		return NULL;

	return rq_entry_fifo(ssg->fifo_list[data_dir].next);
}

/*
 * For the specified data direction, return the next request to
 * dispatch using sector position sorted lists.
 */
static struct request *ssg_next_request(struct ssg_data *ssg, int data_dir)
{
	if (WARN_ON_ONCE(data_dir != READ && data_dir != WRITE && data_dir != BOOST))
		return NULL;

	return ssg->next_rq[data_dir];
}

/*
 * ssg_dispatch_requests selects the best request according to
 * read/write expire, etc
 */
static struct request *__ssg_dispatch_request(struct ssg_data *ssg)
{
	struct request *rq, *next_rq;
	bool reads, writes, boosts;
	int data_dir;

	if (!list_empty(&ssg->dispatch)) {
		rq = list_first_entry(&ssg->dispatch, struct request, queuelist);
		list_del_init(&rq->queuelist);
		goto done;
	}

	reads = !list_empty(&ssg->fifo_list[READ]);
	writes = !list_empty(&ssg->fifo_list[WRITE]);
	boosts = !list_empty(&ssg->fifo_list[BOOST]);

	if (boosts) {
		if ((reads || writes) &&
		    (ssg->starved_normal++ >= ssg->max_normal_starvation))
			goto dispatch_normal;

		data_dir = BOOST;

		goto dispatch_find_request;
	}

dispatch_normal:
	/*
	 * select the appropriate data direction (read / write)
	 */
	if (reads) {
		BUG_ON(RB_EMPTY_ROOT(&ssg->sort_list[READ]));

		if (writes &&
		    (ssg->starved_writes++ >= ssg->max_write_starvation))
			goto dispatch_writes;

		ssg->starved_normal = 0;
		data_dir = READ;

		goto dispatch_find_request;
	}

	/*
	 * there are either no reads or writes have been starved
	 */

	if (writes) {
dispatch_writes:
		BUG_ON(RB_EMPTY_ROOT(&ssg->sort_list[WRITE]));

		ssg->starved_normal = 0;
		ssg->starved_writes = 0;

		data_dir = WRITE;

		goto dispatch_find_request;
	}

	return NULL;

dispatch_find_request:
	/*
	 * we are not running a batch, find best request for selected data_dir
	 */
	next_rq = ssg_next_request(ssg, data_dir);
	if (ssg_check_fifo(ssg, data_dir) || !next_rq) {
		/*
		 * A deadline has expired, the last request was in the other
		 * direction, or we have run out of higher-sectored requests.
		 * Start again from the request with the earliest expiry time.
		 */
		rq = ssg_fifo_request(ssg, data_dir);
	} else {
		/*
		 * The last req was the same dir and we have a next request in
		 * sort order. No expired requests so continue on from here.
		 */
		rq = next_rq;
	}

	if (!rq)
		return NULL;

	/*
	 * rq is the selected appropriate request.
	 */
	ssg_move_request(ssg, rq);
done:
	rq->rq_flags |= RQF_STARTED;
	return rq;
}

/*
 * One confusing aspect here is that we get called for a specific
 * hardware queue, but we may return a request that is for a
 * different hardware queue. This is because ssg-iosched has shared
 * state for all hardware queues, in terms of sorting, FIFOs, etc.
 */
static struct request *ssg_dispatch_request(struct blk_mq_hw_ctx *hctx)
{
	struct ssg_data *ssg = hctx->queue->elevator->elevator_data;
	struct request *rq;
	struct ssg_request_info *rqi;

	spin_lock(&ssg->lock);
	rq = __ssg_dispatch_request(ssg);
	spin_unlock(&ssg->lock);

	rqi = ssg_rq_info(ssg, rq);
	if (likely(rqi)) {
		rqi->sector = blk_rq_pos(rq);
		rqi->data_size = blk_rq_bytes(rq);
	}

	return rq;
}

static void ssg_completed_request(struct request *rq, u64 now)
{
	struct ssg_data *ssg = rq->q->elevator->elevator_data;
	struct ssg_request_info *rqi;

	rqi = ssg_rq_info(ssg, rq);
	if (likely(rqi && rqi->sector == blk_rq_pos(rq))) {
		ssg_stat_account_io_done(ssg, rq, rqi->data_size, now);
		blk_sec_stat_account_io_complete(rq, rqi->data_size, rqi->pio);
	}
}

static void ssg_set_shallow_depth(struct ssg_data *ssg, struct blk_mq_tags *tags)
{
	unsigned int depth = tags->bitmap_tags.sb.depth;
	unsigned int map_nr = tags->bitmap_tags.sb.map_nr;

	ssg->max_async_write_rqs =
		max_t(int, depth * max_async_write_ratio / 100U, 1);
	ssg->max_async_write_rqs =
		min_t(int, ssg->max_async_write_rqs, MAX_ASYNC_WRITE_RQS);
	ssg->async_write_shallow_depth =
		max_t(unsigned int, ssg->max_async_write_rqs / map_nr, 1);

	ssg->max_tgroup_rqs =
		max_t(int, depth * max_tgroup_io_ratio / 100U, 1);
	ssg->tgroup_shallow_depth =
		max_t(unsigned int, ssg->max_tgroup_rqs / map_nr, 1);
}

static void ssg_depth_updated(struct blk_mq_hw_ctx *hctx)
{
	struct request_queue *q = hctx->queue;
	struct ssg_data *ssg = q->elevator->elevator_data;
	struct blk_mq_tags *tags = hctx->sched_tags;
	unsigned int depth = tags->bitmap_tags.sb.depth;

	ssg->congestion_threshold_rqs = depth * congestion_threshold / 100U;

	kfree(ssg->rq_info);
	ssg->rq_info = kmalloc_array(depth, sizeof(struct ssg_request_info),
			GFP_KERNEL | __GFP_ZERO);
	if (ZERO_OR_NULL_PTR(ssg->rq_info))
		ssg->rq_info = NULL;

	ssg_set_shallow_depth(ssg, tags);
	sbitmap_queue_min_shallow_depth(&tags->bitmap_tags,
			ssg->async_write_shallow_depth);

	ssg_blkcg_depth_updated(hctx);
	ssg_wb_depth_updated(hctx);
}

static inline bool ssg_op_is_async_write(unsigned int op)
{
	return (op & REQ_OP_MASK) == REQ_OP_WRITE && !op_is_sync(op);
}

static unsigned int ssg_async_write_shallow_depth(unsigned int op,
		struct blk_mq_alloc_data *data)
{
	struct ssg_data *ssg = data->q->elevator->elevator_data;

	if (!ssg_op_is_async_write(op))
		return 0;

	if (atomic_read(&ssg->async_write_rqs) < ssg->max_async_write_rqs)
		return 0;

	return ssg->async_write_shallow_depth;
}

static unsigned int ssg_tgroup_shallow_depth(struct blk_mq_alloc_data *data)
{
	struct ssg_data *ssg = data->q->elevator->elevator_data;
	pid_t tgid = task_tgid_nr(current->group_leader);
	int nr_requests = data->q->nr_requests;
	int tgroup_rqs = 0;
	int i;

	if (unlikely(!ssg->rq_info))
		return 0;

	for (i = 0; i < nr_requests; i++)
		if (tgid == ssg->rq_info[i].tgid)
			tgroup_rqs++;

	if (tgroup_rqs < ssg->max_tgroup_rqs)
		return 0;

	return ssg->tgroup_shallow_depth;
}

static void ssg_limit_depth(unsigned int op, struct blk_mq_alloc_data *data)
{
	struct ssg_data *ssg = data->q->elevator->elevator_data;
	unsigned int shallow_depth = ssg_blkcg_shallow_depth(data->q);

	shallow_depth = min_not_zero(shallow_depth,
			ssg_async_write_shallow_depth(op, data));

	if (atomic_read(&ssg->allocated_rqs) > ssg->congestion_threshold_rqs)
		shallow_depth = min_not_zero(shallow_depth,
				ssg_tgroup_shallow_depth(data));

	data->shallow_depth = shallow_depth;
}

static int ssg_init_hctx(struct blk_mq_hw_ctx *hctx, unsigned int hctx_idx)
{
	struct ssg_data *ssg = hctx->queue->elevator->elevator_data;
	struct blk_mq_tags *tags = hctx->sched_tags;

	ssg_set_shallow_depth(ssg, tags);
	sbitmap_queue_min_shallow_depth(&tags->bitmap_tags,
			ssg->async_write_shallow_depth);

	return 0;
}

static void ssg_exit_queue(struct elevator_queue *e)
{
	struct ssg_data *ssg = e->elevator_data;

	ssg_blkcg_deactivate(ssg->queue);

	BUG_ON(!list_empty(&ssg->fifo_list[READ]));
	BUG_ON(!list_empty(&ssg->fifo_list[WRITE]));
	BUG_ON(!list_empty(&ssg->fifo_list[BOOST]));

	ssg_stat_exit(ssg);
	ssg_wb_exit(ssg);
	blk_sec_stat_account_exit(e);
	blk_stat_disable_accounting(ssg->queue);

	kfree(ssg->rq_info);
	kfree(ssg);
}

/*
 * initialize elevator private data (ssg_data).
 */
static int ssg_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct ssg_data *ssg;
	struct elevator_queue *eq;

	if (blk_queue_is_zoned(q))
		return -EINVAL;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	ssg = kzalloc_node(sizeof(*ssg), GFP_KERNEL, q->node);
	if (!ssg) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = ssg;

	ssg->queue = q;
	INIT_LIST_HEAD(&ssg->fifo_list[READ]);
	INIT_LIST_HEAD(&ssg->fifo_list[WRITE]);
	INIT_LIST_HEAD(&ssg->fifo_list[BOOST]);
	ssg->sort_list[READ] = RB_ROOT;
	ssg->sort_list[WRITE] = RB_ROOT;
	ssg->sort_list[BOOST] = RB_ROOT;
	ssg->fifo_expire[READ] = read_expire;
	ssg->fifo_expire[WRITE] = write_expire;
	ssg->fifo_expire[BOOST] = boost_expire;
	ssg->max_normal_starvation = max_normal_starvation;
	ssg->max_write_starvation = max_write_starvation;
	ssg->front_merges = 1;

	atomic_set(&ssg->allocated_rqs, 0);
	atomic_set(&ssg->async_write_rqs, 0);
	ssg->congestion_threshold_rqs =
		q->nr_requests * congestion_threshold / 100U;
	ssg->rq_info = kmalloc_array(q->nr_requests,
			sizeof(struct ssg_request_info),
			GFP_KERNEL | __GFP_ZERO);
	if (ZERO_OR_NULL_PTR(ssg->rq_info))
		ssg->rq_info = NULL;

	spin_lock_init(&ssg->lock);
	INIT_LIST_HEAD(&ssg->dispatch);

	ssg_blkcg_activate(q);

	q->elevator = eq;

	ssg_stat_init(ssg);
	blk_stat_enable_accounting(q);
	blk_sec_stat_account_init(q);
	ssg_wb_init(ssg);

	return 0;
}

static int ssg_request_merge(struct request_queue *q, struct request **rq,
			    struct bio *bio)
{
	struct ssg_data *ssg = q->elevator->elevator_data;
	sector_t sector = bio_end_sector(bio);
	struct request *__rq;

	if (!ssg->front_merges)
		return ELEVATOR_NO_MERGE;

	__rq = elv_rb_find(&ssg->sort_list[bio_data_dir(bio)], sector);
	if (__rq) {
		BUG_ON(sector != blk_rq_pos(__rq));

		if (elv_bio_merge_ok(__rq, bio)) {
			*rq = __rq;
			return ELEVATOR_FRONT_MERGE;
		}
	}

	return ELEVATOR_NO_MERGE;
}

static bool ssg_bio_merge(struct request_queue *q, struct bio *bio,
		unsigned int nr_segs)
{
	struct ssg_data *ssg = q->elevator->elevator_data;
	struct request *free = NULL;
	bool ret;

	spin_lock(&ssg->lock);
	ret = blk_mq_sched_try_merge(q, bio, nr_segs, &free);
	spin_unlock(&ssg->lock);

	if (free)
		blk_mq_free_request(free);

	return ret;
}

/*
 * add rq to rbtree and fifo
 */
static void ssg_insert_request(struct blk_mq_hw_ctx *hctx, struct request *rq,
			      blk_insert_t flags)
{
	struct request_queue *q = hctx->queue;
	struct ssg_data *ssg = q->elevator->elevator_data;
	const int data_dir = ssg_rq_data_dir(ssg, rq);

	LIST_HEAD(free);

	if (blk_mq_sched_try_insert_merge(q, rq, &free)) {
		blk_mq_free_requests(&free);
		return;
	}

	trace_block_rq_insert(rq);

	if ((flags & BLK_MQ_INSERT_AT_HEAD) || blk_rq_is_passthrough(rq)) {
		if (flags & BLK_MQ_INSERT_AT_HEAD)
			list_add(&rq->queuelist, &ssg->dispatch);
		else
			list_add_tail(&rq->queuelist, &ssg->dispatch);
	} else {
		ssg_add_rq_rb(ssg, rq);

		if (rq_mergeable(rq)) {
			elv_rqhash_add(q, rq);
			if (!q->last_merge)
				q->last_merge = rq;
		}

		/*
		 * set expire time and add to fifo list
		 */
		rq->fifo_time = jiffies + ssg->fifo_expire[data_dir];
		list_add_tail(&rq->queuelist, &ssg->fifo_list[data_dir]);
	}
}

static void ssg_insert_requests(struct blk_mq_hw_ctx *hctx,
			       struct list_head *list, blk_insert_t flags)
{
	struct request_queue *q = hctx->queue;
	struct ssg_data *ssg = q->elevator->elevator_data;

	spin_lock(&ssg->lock);
	while (!list_empty(list)) {
		struct request *rq;

		rq = list_first_entry(list, struct request, queuelist);
		list_del_init(&rq->queuelist);
		ssg_insert_request(hctx, rq, flags);
	}
	spin_unlock(&ssg->lock);
}

/*
 * Nothing to do here. This is defined only to ensure that .finish_request
 * method is called upon request completion.
 */
static void ssg_prepare_request(struct request *rq)
{
	struct ssg_data *ssg = rq->q->elevator->elevator_data;
	struct ssg_request_info *rqi;

	atomic_inc(&ssg->allocated_rqs);

	ssg_wb_run_ctrl_work(ssg, rq);

	rqi = ssg_rq_info(ssg, rq);
	if (likely(rqi)) {
		rqi->tgid = task_tgid_nr(current->group_leader);

		rcu_read_lock();
		rqi->blkg = blkg_lookup(css_to_blkcg(curr_css()), rq->q);
		ssg_blkcg_inc_rq(rqi->blkg);

		if (ssg_blkcg_check_boost(rqi->blkg))
			rqi->blkcg_boost = 1;

		rcu_read_unlock();

		blk_sec_stat_account_io_prepare(rq, &rqi->pio);
	}

	if (ssg_op_is_async_write(rq->cmd_flags))
		atomic_inc(&ssg->async_write_rqs);
}

static void ssg_finish_request(struct request *rq)
{
	struct request_queue *q = rq->q;
	struct ssg_data *ssg = q->elevator->elevator_data;
	struct ssg_request_info *rqi;

	if (unlikely(!(rq->rq_flags & RQF_USE_SCHED)))
		return;

	atomic_dec(&ssg->allocated_rqs);

	rqi = ssg_rq_info(ssg, rq);
	if (likely(rqi)) {
		rqi->tgid = 0;

		ssg_blkcg_dec_rq(rqi->blkg);
		rqi->blkg = NULL;

		rqi->blkcg_boost = 0;

		blk_sec_stat_account_io_finish(rq, &rqi->pio);
	}

	if (ssg_op_is_async_write(rq->cmd_flags))
		atomic_dec(&ssg->async_write_rqs);
}

static bool ssg_has_work(struct blk_mq_hw_ctx *hctx)
{
	struct ssg_data *ssg = hctx->queue->elevator->elevator_data;

	return !list_empty_careful(&ssg->dispatch) ||
		!list_empty_careful(&ssg->fifo_list[READ]) ||
		!list_empty_careful(&ssg->fifo_list[WRITE]) ||
		!list_empty_careful(&ssg->fifo_list[BOOST]);
}

/*
 * sysfs parts below
 */
static ssize_t ssg_var_show(int var, char *page)
{
	return sprintf(page, "%d\n", var);
}

static void ssg_var_store(int *var, const char *page)
{
	long val;

	if (!kstrtol(page, 10, &val))
		*var = val;
}

#define SHOW_FUNCTION(__FUNC, __VAR, __CONV)				\
static ssize_t __FUNC(struct elevator_queue *e, char *page)		\
{									\
	struct ssg_data *ssg = e->elevator_data;			\
	int __data = __VAR;						\
	if (__CONV)							\
		__data = jiffies_to_msecs(__data);			\
	return ssg_var_show(__data, (page));				\
}
SHOW_FUNCTION(ssg_read_expire_show, ssg->fifo_expire[READ], 1);
SHOW_FUNCTION(ssg_write_expire_show, ssg->fifo_expire[WRITE], 1);
SHOW_FUNCTION(ssg_max_write_starvation_show, ssg->max_write_starvation, 0);
SHOW_FUNCTION(ssg_front_merges_show, ssg->front_merges, 0);
SHOW_FUNCTION(ssg_max_tgroup_rqs_show, ssg->max_tgroup_rqs, 0);
SHOW_FUNCTION(ssg_max_async_write_rqs_show, ssg->max_async_write_rqs, 0);
SHOW_FUNCTION(ssg_tgroup_shallow_depth_show, ssg->tgroup_shallow_depth, 0);
SHOW_FUNCTION(ssg_async_write_shallow_depth_show, ssg->async_write_shallow_depth, 0);
#undef SHOW_FUNCTION

#define STORE_FUNCTION(__FUNC, __PTR, MIN, MAX, __CONV)			\
static ssize_t __FUNC(struct elevator_queue *e, const char *page, size_t count)	\
{									\
	struct ssg_data *ssg = e->elevator_data;			\
	int __data;							\
	ssg_var_store(&__data, (page));					\
	if (__data < (MIN))						\
		__data = (MIN);						\
	else if (__data > (MAX))					\
		__data = (MAX);						\
	if (__CONV)							\
		*(__PTR) = msecs_to_jiffies(__data);			\
	else								\
		*(__PTR) = __data;					\
	return count;							\
}
STORE_FUNCTION(ssg_read_expire_store, &ssg->fifo_expire[READ], 0, INT_MAX, 1);
STORE_FUNCTION(ssg_write_expire_store, &ssg->fifo_expire[WRITE], 0, INT_MAX, 1);
STORE_FUNCTION(ssg_max_write_starvation_store, &ssg->max_write_starvation, INT_MIN, INT_MAX, 0);
STORE_FUNCTION(ssg_front_merges_store, &ssg->front_merges, 0, 1, 0);
#undef STORE_FUNCTION

#define SSG_ATTR(name) \
	__ATTR(name, 0644, ssg_##name##_show, ssg_##name##_store)
#define SSG_ATTR_RO(name) \
	__ATTR(name, 0444, ssg_##name##_show, NULL)
#define SSG_STAT_ATTR_RO(name) \
	__ATTR(name, 0444, ssg_stat_##name##_show, NULL)

static struct elv_fs_entry ssg_attrs[] = {
	SSG_ATTR(read_expire),
	SSG_ATTR(write_expire),
	SSG_ATTR(max_write_starvation),
	SSG_ATTR(front_merges),
	SSG_ATTR_RO(max_tgroup_rqs),
	SSG_ATTR_RO(max_async_write_rqs),
	SSG_ATTR_RO(tgroup_shallow_depth),
	SSG_ATTR_RO(async_write_shallow_depth),

	SSG_STAT_ATTR_RO(read_latency),
	SSG_STAT_ATTR_RO(write_latency),
	SSG_STAT_ATTR_RO(flush_latency),
	SSG_STAT_ATTR_RO(discard_latency),
	SSG_STAT_ATTR_RO(inflight),
	SSG_STAT_ATTR_RO(rqs_info),

#if IS_ENABLED(CONFIG_MQ_IOSCHED_SSG_WB)
	SSG_ATTR(wb_on_rqs),
	SSG_ATTR(wb_off_rqs),
	SSG_ATTR(wb_on_dirty_bytes),
	SSG_ATTR(wb_off_dirty_bytes),
	SSG_ATTR(wb_on_sync_write_bytes),
	SSG_ATTR(wb_off_sync_write_bytes),
	SSG_ATTR(wb_on_dirty_busy_written_bytes),
	SSG_ATTR(wb_on_dirty_busy_msecs),
	SSG_ATTR(wb_off_delay_msecs),
	SSG_ATTR_RO(wb_triggered),
#endif

	__ATTR_NULL
};

static struct elevator_type ssg_iosched = {
	.ops = {
		.insert_requests = ssg_insert_requests,
		.dispatch_request = ssg_dispatch_request,
		.completed_request = ssg_completed_request,
		.prepare_request = ssg_prepare_request,
		.finish_request = ssg_finish_request,
		.next_request = elv_rb_latter_request,
		.former_request = elv_rb_former_request,
		.bio_merge = ssg_bio_merge,
		.request_merge = ssg_request_merge,
		.requests_merged = ssg_merged_requests,
		.request_merged = ssg_request_merged,
		.has_work = ssg_has_work,
		.limit_depth = ssg_limit_depth,
		.depth_updated = ssg_depth_updated,
		.init_hctx = ssg_init_hctx,
		.init_sched = ssg_init_queue,
		.exit_sched = ssg_exit_queue,
	},

	.elevator_attrs = ssg_attrs,
	.elevator_name = "ssg",
	.elevator_alias = "ssg",
	.elevator_features = ELEVATOR_F_ZBD_SEQ_WRITE,
	.elevator_owner = THIS_MODULE,
};
MODULE_ALIAS("ssg");

static int __init ssg_iosched_init(void)
{
	int ret;

	ret = elv_register(&ssg_iosched);
	if (ret)
		return ret;

	ret = ssg_blkcg_init();
	if (ret) {
		elv_unregister(&ssg_iosched);
		return ret;
	}

	return ret;
}

static void __exit ssg_iosched_exit(void)
{
	ssg_blkcg_exit();
	elv_unregister(&ssg_iosched);
}

module_init(ssg_iosched_init);
module_exit(ssg_iosched_exit);

MODULE_AUTHOR("Jisoo Oh");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSG IO Scheduler");
