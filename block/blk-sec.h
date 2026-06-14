/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BLK_SEC_H
#define BLK_SEC_H

enum {
	WB_REQ_IOSCHED = 0,
	WB_REQ_USER,

	NR_WB_REQ_TYPE
};

#if IS_ENABLED(CONFIG_BLK_SEC_COMMON)
extern struct device *blk_sec_dev;
extern struct workqueue_struct *blk_sec_common_wq;

extern struct gendisk *blk_sec_internal_disk(void);
#else
static struct gendisk *blk_sec_internal_disk(void)
{
	return NULL;
}
#endif

#if IS_ENABLED(CONFIG_BLK_SEC_STATS)
struct pio_node {
	struct list_head list;

	pid_t tgid;
	char name[TASK_COMM_LEN];
	u64 start_time;

	atomic_t kb[REQ_OP_DISCARD + 1];

	atomic_t ref_count;
	struct pio_node *h_next; /* next pio_node for hash */
};

extern void blk_sec_stat_account_init(struct request_queue *q);
extern void blk_sec_stat_account_exit(struct elevator_queue *eq);
extern void blk_sec_stat_account_io_prepare(struct request *rq,
		void *ptr_pio);
extern void blk_sec_stat_account_io_complete(struct request *rq,
		unsigned int data_size, void *pio);
extern void blk_sec_stat_account_io_finish(struct request *rq,
		void *ptr_pio);
#else
static inline void blk_sec_stat_account_init(struct request_queue *q)
{
}

static inline void blk_sec_stat_account_exit(struct elevator_queue *eq)
{
}

static inline void blk_sec_stat_account_io_prepare(struct request *rq,
		void *ptr_pio)
{
}

static inline void blk_sec_stat_account_io_complete(struct request *rq,
		unsigned int data_size, void *pio)
{
}

static inline void blk_sec_stat_account_io_finish(struct request *rq,
		void *ptr_pio)
{
}
#endif

#if IS_ENABLED(CONFIG_BLK_SEC_WB)
extern int blk_sec_wb_ctrl(bool enable, int req_type);
extern int blk_sec_wb_ctrl_async(bool enable, int req_type);
extern bool blk_sec_wb_is_supported(struct gendisk *gd);
#else
static inline int blk_sec_wb_ctrl(bool enable, int req_type)
{
	return 0;
}

static inline int blk_sec_wb_ctrl_async(bool enable, int req_type)
{
	return 0;
}

static inline bool blk_sec_wb_is_supported(struct gendisk *gd)
{
	return false;
}
#endif

#endif // BLK_SEC_H
