// SPDX-License-Identifier: GPL-2.0
/*
 *  Statistics of SamSung Generic I/O scheduler
 *
 *  Copyright (C) 2021 Changheun Lee <nanich.lee@samsung.com>
 */

#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/sbitmap.h>

#include "elevator.h"
#include "blk-mq.h"
#include "ssg.h"

#define IO_TYPES (REQ_OP_DISCARD + 1)

static unsigned int byte_table[] = {
	4096, // 4KB
	32768, // 32KB
	65536, // 64KB
	131072, // 128KB
	524288, // 512KB
	1048576, // 1MB

	UINT_MAX // should be last in this array
};
#define BYTE_TABLE_SIZE	(sizeof(byte_table)/sizeof(unsigned int))

static u64 nsec_table[] = {
	500000, // 0.5ms
	1000000, // 1ms
	2000000, // 2ms
	3000000, // 3ms
	4000000, // 4ms
	5000000, // 5ms
	10000000, // 10ms
	20000000, // 20ms

	ULLONG_MAX // should be last in this array
};
#define NSEC_TABLE_SIZE	(sizeof(nsec_table)/sizeof(u64))

struct ssg_stats {
	u64 io_latency_cnt[IO_TYPES][BYTE_TABLE_SIZE][NSEC_TABLE_SIZE];
};

struct ssg_bt_tags_iter_data {
	struct blk_mq_tags *tags;
	void *data;
	bool reserved;
};

typedef bool (ssg_tag_iter_fn)(struct sbitmap *, unsigned int, void *);

static unsigned int byte_to_index(unsigned int byte)
{
	unsigned int idx;

	for (idx = 0; idx < BYTE_TABLE_SIZE; idx++)
		if (byte <= byte_table[idx])
			return idx;

	return BYTE_TABLE_SIZE - 1;
}

static unsigned int nsec_to_index(u64 nsec)
{
	unsigned int idx;

	for (idx = 0; idx < NSEC_TABLE_SIZE; idx++)
		if (nsec <= nsec_table[idx])
			return idx;

	return NSEC_TABLE_SIZE - 1;
}

static void update_io_latency(struct ssg_data *ssg, struct request *rq,
		unsigned int data_size, u64 now)
{
	struct ssg_stats *stats;
	int type = req_op(rq);
	int byte_idx, ns_idx;

	if (type > REQ_OP_DISCARD)
		return;

	if (rq->io_start_time_ns > now)
		return;

	byte_idx = byte_to_index(data_size);
	ns_idx = nsec_to_index(now - rq->io_start_time_ns);

	stats = get_cpu_ptr(ssg->stats);
	stats->io_latency_cnt[type][byte_idx][ns_idx]++;
	put_cpu_ptr(stats);
}

void ssg_stat_account_io_done(struct ssg_data *ssg, struct request *rq,
		unsigned int data_size, u64 now)
{
	if (unlikely(!ssg->stats))
		return;

	update_io_latency(ssg, rq, data_size, now);
}

static int print_io_latency(struct ssg_stats __percpu *stats, int io_type,
		char *buf, int buf_size)
{
	u64 sum[BYTE_TABLE_SIZE][NSEC_TABLE_SIZE] = { 0, };
	int cpu;
	int len = 0;
	int byte_idx, ns_idx;

	for_each_possible_cpu(cpu) {
		struct ssg_stats *s = per_cpu_ptr(stats, cpu);

		for (byte_idx = 0; byte_idx < BYTE_TABLE_SIZE; byte_idx++)
			for (ns_idx = 0; ns_idx < NSEC_TABLE_SIZE; ns_idx++)
				sum[byte_idx][ns_idx] +=
					s->io_latency_cnt[io_type][byte_idx][ns_idx];
	}

	for (byte_idx = 0; byte_idx < BYTE_TABLE_SIZE; byte_idx++) {
		len += snprintf(buf + len, buf_size - len, "%u:",
				byte_table[byte_idx] / 1024);
		for (ns_idx = 0; ns_idx < NSEC_TABLE_SIZE; ns_idx++)
			len += snprintf(buf + len, buf_size - len, " %llu",
					sum[byte_idx][ns_idx]);
		len += snprintf(buf + len, buf_size - len, "\n");
	}

	return len;
}

#define IO_LATENCY_SHOW_FUNC(__FUNC, __IO_TYPE)		\
ssize_t __FUNC(struct elevator_queue *e, char *page)	\
{							\
	struct ssg_data *ssg = e->elevator_data;	\
	if (unlikely(!ssg->stats))			\
		return 0;				\
	return print_io_latency(ssg->stats,		\
			__IO_TYPE, page, PAGE_SIZE);	\
}
IO_LATENCY_SHOW_FUNC(ssg_stat_read_latency_show, REQ_OP_READ);
IO_LATENCY_SHOW_FUNC(ssg_stat_write_latency_show, REQ_OP_WRITE);
IO_LATENCY_SHOW_FUNC(ssg_stat_flush_latency_show, REQ_OP_FLUSH);
IO_LATENCY_SHOW_FUNC(ssg_stat_discard_latency_show, REQ_OP_DISCARD);

static void ssg_all_tag_iter(struct blk_mq_tags *tags, ssg_tag_iter_fn *fn, struct ssg_bt_tags_iter_data *iter_data)
{
	iter_data->tags = tags;

	if (tags->nr_reserved_tags) {
		iter_data->reserved = true;
		sbitmap_for_each_set(&tags->breserved_tags.sb, fn, iter_data);
	}

	iter_data->reserved = false;
	sbitmap_for_each_set(&tags->bitmap_tags.sb, fn, iter_data);
}

static bool ssg_count_inflight(struct sbitmap *bitmap, unsigned int bitnr, void *data)
{
	struct ssg_bt_tags_iter_data *iter_data = data;
	struct blk_mq_tags *tags = iter_data->tags;
	unsigned int *inflight = iter_data->data;
	bool reserved = iter_data->reserved;
	struct request *rq;

	if (!reserved)
		bitnr += tags->nr_reserved_tags;

	rq = tags->static_rqs[bitnr];

	if (!rq)
		return true;

	if (req_op(rq) < IO_TYPES)
		inflight[req_op(rq)]++;

	return true;
}

static void get_ssg_inflight(struct request_queue *q, unsigned int *inflight)
{
	struct blk_mq_hw_ctx *hctx;
	unsigned long i;
	struct ssg_bt_tags_iter_data iter_data = {
		.data = inflight,
	};

	if (blk_mq_is_shared_tags(q->tag_set->flags)) {
		ssg_all_tag_iter(q->sched_shared_tags, ssg_count_inflight, &iter_data);
	} else {
		queue_for_each_hw_ctx(q, hctx, i) {
			/*
			 * If no software queues are currently mapped to this
			 * hardware queue, there's nothing to check
			 */
			if (!blk_mq_hw_queue_mapped(hctx))
				continue;

			ssg_all_tag_iter(hctx->sched_tags, ssg_count_inflight, &iter_data);
		}
	}
}

ssize_t ssg_stat_inflight_show(struct elevator_queue *e, char *page)
{
	struct ssg_data *ssg = e->elevator_data;
	unsigned int inflight[IO_TYPES] = {0, };

	if (unlikely(!ssg->stats))
		return 0;

	get_ssg_inflight(ssg->queue, inflight);

	return snprintf(page, PAGE_SIZE, "%u %u %u\n", inflight[REQ_OP_READ],
			inflight[REQ_OP_WRITE], inflight[REQ_OP_DISCARD]);
}

static bool print_ssg_rq_info(struct sbitmap *bitmap, unsigned int bitnr, void *data)
{
	struct ssg_bt_tags_iter_data *iter_data = data;
	struct blk_mq_tags *tags = iter_data->tags;
	bool reserved = iter_data->reserved;
	char *page =  iter_data->data;
	struct request *rq;
	int len = strlen(page);

	if (!reserved)
		bitnr += tags->nr_reserved_tags;

	rq = tags->static_rqs[bitnr];

	if (!rq)
		return true;

	scnprintf(page + len, PAGE_SIZE - len, "%d %d %x %x %llu %u %llu %d\n",
					rq->tag, rq->internal_tag, req_op(rq), rq->rq_flags,
					blk_rq_pos(rq), blk_rq_bytes(rq), rq->start_time_ns, rq->state);

	return true;
}

static void print_ssg_rqs(struct request_queue *q, char *page)
{
	struct blk_mq_hw_ctx *hctx;
	unsigned long i;
	struct ssg_bt_tags_iter_data iter_data = {
		.data = page,
	};

	if (blk_mq_is_shared_tags(q->tag_set->flags)) {
		ssg_all_tag_iter(q->sched_shared_tags, print_ssg_rq_info, &iter_data);
	} else {
		queue_for_each_hw_ctx(q, hctx, i) {
			/*
			 * If no software queues are currently mapped to this
			 * hardware queue, there's nothing to check
			 */
			if (!blk_mq_hw_queue_mapped(hctx))
				continue;

			ssg_all_tag_iter(hctx->sched_tags, print_ssg_rq_info, &iter_data);
		}
	}
}

ssize_t ssg_stat_rqs_info_show(struct elevator_queue *e, char *page)
{
	struct ssg_data *ssg = e->elevator_data;

	if (unlikely(!ssg->stats))
		return 0;

	print_ssg_rqs(ssg->queue, page);

	return strlen(page);
}

int ssg_stat_init(struct ssg_data *ssg)
{
	ssg->stats = alloc_percpu_gfp(struct ssg_stats,
			GFP_KERNEL | __GFP_ZERO);
	if (!ssg->stats)
		return -ENOMEM;

	return 0;
}

void ssg_stat_exit(struct ssg_data *ssg)
{
	if (ssg->stats)
		free_percpu(ssg->stats);
}
