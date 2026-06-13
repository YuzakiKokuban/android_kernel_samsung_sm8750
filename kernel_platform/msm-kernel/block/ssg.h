/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SSG_H
#define SSG_H

#include "blk-cgroup.h"

struct ssg_request_info {
	pid_t tgid;

	sector_t sector;
	unsigned int data_size;
	unsigned int blkcg_boost;

	struct blkcg_gq *blkg;

	void *pio;
};

struct ssg_data {
	struct request_queue *queue;

	/*
	 * requests are present on both sort_list and fifo_list
	 */
	struct rb_root sort_list[3];
	struct list_head fifo_list[3];

	/*
	 * next in sort order. read, write or both are NULL
	 */
	struct request *next_rq[3];
	unsigned int starved_writes;	/* times reads have starved writes */
	unsigned int starved_normal;	/* times boosts have starved normal reads or writes */

	/*
	 * settings that change how the i/o scheduler behaves
	 */
	int fifo_expire[3];
	int max_write_starvation;
	int max_normal_starvation;
	int front_merges;

	/*
	 * to control request allocation
	 */
	atomic_t allocated_rqs;
	atomic_t async_write_rqs;
	int congestion_threshold_rqs;
	int max_tgroup_rqs;
	int max_async_write_rqs;
	unsigned int tgroup_shallow_depth;	/* thread group shallow depth for each tag map */
	unsigned int async_write_shallow_depth;	/* async write shallow depth for each tag map */

	/*
	 * I/O context information for each request
	 */
	struct ssg_request_info *rq_info;

	/*
	 * Statistics
	 */
	void __percpu *stats;

	spinlock_t lock;
	struct list_head dispatch;

	/*
	 * Write booster
	 */
	void *wb_data;
};

static inline struct cgroup_subsys_state *curr_css(void)
{
	return task_css(current, io_cgrp_id);
}

/* ssg-stat.c */
extern int ssg_stat_init(struct ssg_data *ssg);
extern void ssg_stat_exit(struct ssg_data *ssg);
extern void ssg_stat_account_io_done(struct ssg_data *ssg,
		struct request *rq, unsigned int data_size, u64 now);
extern ssize_t ssg_stat_read_latency_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_stat_write_latency_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_stat_flush_latency_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_stat_discard_latency_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_stat_inflight_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_stat_rqs_info_show(struct elevator_queue *e, char *page);

/* ssg-cgroup.c */
#if IS_ENABLED(CONFIG_MQ_IOSCHED_SSG_CGROUP)
struct ssg_blkcg {
	struct blkcg_policy_data cpd; /* must be the first member */

	int max_available_ratio;
	int boost_on;
};

struct ssg_blkg {
	struct blkg_policy_data pd; /* must be the first member */

	atomic_t current_rqs;
	int max_available_rqs;
	unsigned int shallow_depth; /* shallow depth for each tag map to get sched tag */
};

extern int ssg_blkcg_init(void);
extern void ssg_blkcg_exit(void);
extern int ssg_blkcg_activate(struct request_queue *q);
extern void ssg_blkcg_deactivate(struct request_queue *q);
extern unsigned int ssg_blkcg_shallow_depth(struct request_queue *q);
extern void ssg_blkcg_depth_updated(struct blk_mq_hw_ctx *hctx);
extern void ssg_blkcg_inc_rq(struct blkcg_gq *blkg);
extern void ssg_blkcg_dec_rq(struct blkcg_gq *blkg);
extern int ssg_blkcg_check_boost(struct blkcg_gq *blkg);
#else
static inline int ssg_blkcg_init(void)
{
	return 0;
}

static inline void ssg_blkcg_exit(void)
{
}

static inline int ssg_blkcg_activate(struct request_queue *q)
{
	return 0;
}

static inline void ssg_blkcg_deactivate(struct request_queue *q)
{
}

static inline unsigned int ssg_blkcg_shallow_depth(struct request_queue *q)
{
	return 0;
}

static inline void ssg_blkcg_depth_updated(struct blk_mq_hw_ctx *hctx)
{
}

static inline void ssg_blkcg_inc_rq(struct blkcg_gq *blkg)
{
}

static inline void ssg_blkcg_dec_rq(struct blkcg_gq *blkg)
{
}
static inline int ssg_blkcg_check_boost(struct blkcg_gq *blkg)
{
	return 0;
}
#endif

/* ssg-wb.c */
#if IS_ENABLED(CONFIG_MQ_IOSCHED_SSG_WB)
extern void ssg_wb_run_ctrl_work(struct ssg_data *ssg, struct request *rq);
extern void ssg_wb_depth_updated(struct blk_mq_hw_ctx *hctx);
extern void ssg_wb_init(struct ssg_data *ssg);
extern void ssg_wb_exit(struct ssg_data *ssg);
extern ssize_t ssg_wb_on_rqs_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_on_rqs_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_off_rqs_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_off_rqs_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_on_dirty_bytes_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_on_dirty_bytes_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_off_dirty_bytes_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_off_dirty_bytes_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_on_sync_write_bytes_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_on_sync_write_bytes_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_off_sync_write_bytes_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_off_sync_write_bytes_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_on_dirty_busy_written_bytes_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_on_dirty_busy_written_bytes_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_on_dirty_busy_msecs_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_on_dirty_busy_msecs_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_off_delay_msecs_show(struct elevator_queue *e, char *page);
extern ssize_t ssg_wb_off_delay_msecs_store(struct elevator_queue *e, const char *page, size_t count);
extern ssize_t ssg_wb_triggered_show(struct elevator_queue *e, char *page);
#else
static inline void ssg_wb_run_ctrl_work(struct ssg_data *ssg, struct request *rq)
{
}

static inline void ssg_wb_depth_updated(struct blk_mq_hw_ctx *hctx)
{
}

static inline void ssg_wb_init(struct ssg_data *ssg)
{
}

static inline void ssg_wb_exit(struct ssg_data *ssg)
{
}
#endif
#endif // SSG_H
