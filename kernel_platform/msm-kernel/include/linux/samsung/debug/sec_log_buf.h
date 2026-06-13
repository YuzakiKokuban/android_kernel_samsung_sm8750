#ifndef __SEC_LOG_BUF_H__
#define __SEC_LOG_BUF_H__

#include <linux/notifier.h>

#include <dt-bindings/samsung/debug/sec_log_buf.h>

#define SEC_LOG_MAGIC		0x4d474f4c	/* "LOGM" */

struct sec_log_buf_head {
	uint32_t boot_cnt;
	uint32_t magic;
	uint32_t idx;
	uint32_t prev_idx;
	char buf[];
};

#if IS_ENABLED(CONFIG_SEC_LOG_BUF)
extern void sec_log_buf_store_on_vprintk_emit(void);
extern const struct sec_log_buf_head *sec_log_buf_get_header(void);
extern ssize_t sec_log_buf_get_buf_size(void);
extern int sec_log_buf_register_sync_handler(struct notifier_block *nb);
extern int sec_log_buf_unregister_sync_handler(struct notifier_block *nb);
#else
static inline void sec_log_buf_store_on_vprintk_emit(void) {}
static inline struct sec_log_buf_head *sec_log_buf_get_header(void) { return ERR_PTR(-ENODEV); }
static inline ssize_t sec_log_buf_get_buf_size(void) { return -ENODEV; }
static inline int sec_log_buf_register_sync_handler(struct notifier_block *nb) { return -ENODEV; }
static inline int sec_log_buf_unregister_sync_handler(struct notifier_block *nb) { return -ENODEV; }
#endif

#endif /* __SEC_LOG_BUF_H__ */
