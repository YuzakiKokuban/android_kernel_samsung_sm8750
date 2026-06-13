#ifndef __INTERNAL__SEC_QC_LOG_BUF_H__
#define __INTERNAL__SEC_QC_LOG_BUF_H__

#include <linux/console.h>
#include <linux/debugfs.h>
#include <linux/crypto.h>
#include <linux/kmsg_dump.h>
#include <linux/kprobes.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_log_buf.h>

struct last_kmsg_data {
	char *buf;
	size_t size;
	bool use_compression;
	const char *compressor;
	struct mutex lock;
	unsigned int ref_cnt;
	struct crypto_comp *tfm;
	char *buf_comp;
	size_t size_comp;
	struct proc_dir_entry *proc;
};

struct log_buf_drvdata;

struct log_buf_logger {
	int (*probe)(struct log_buf_drvdata *);
	void (*remove)(struct log_buf_drvdata *);
};

struct ap_klog_proc {
	char *buf;
	size_t size;
	struct mutex lock;
	unsigned int ref_cnt;
	struct proc_dir_entry *proc;
	struct proc_dir_entry *symlink;
};

struct log_buf_drvdata {
	struct builder bd;
	struct reserved_mem *rmem;
	phys_addr_t paddr;
	size_t size;
	struct kmsg_dump_iter iter;
	unsigned int strategy;
	union {
		struct console con;
		struct kprobe probe;
	};
	const struct log_buf_logger *logger;
	struct atomic_notifier_head sync_list;
	struct last_kmsg_data last_kmsg;
	struct ap_klog_proc ap_klog;
	struct notifier_block nb_restart;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbgfs;
#endif
};

extern struct log_buf_drvdata *sec_log_buf;

static __always_inline bool __log_buf_is_probed(void)
{
	return !!sec_log_buf;
}

static __always_inline size_t __log_buf_print_time(u64 ts, char *buf)
{
	unsigned long rem_nsec = do_div(ts, 1000000000);

	return sprintf(buf, "\n[%5lu.%06lu]",
		       (unsigned long)ts, rem_nsec / 1000);
}

static __always_inline size_t __log_buf_print_process(unsigned int cpu,
		char *buf, size_t sz_buf)
{
	return scnprintf(buf, sz_buf, " %c[%2u:%15s:%5u] ",
			in_interrupt() ? 'I' : ' ',
			cpu, current->comm, (unsigned int)current->pid);
}

/* sec_log_buf_main.c */
extern bool __log_buf_is_acceptable(const char *s, size_t count);
extern void __log_buf_write(const char *s, size_t count);
extern void __log_buf_store_from_kmsg_dumper(void);
extern const struct sec_log_buf_head *__log_buf_get_header(void);
extern ssize_t __log_buf_get_buf_size(void);
extern size_t __log_buf_copy_to_buffer(void *buf);

/* sec_log_buf_last_kmsg.c */
extern int __last_kmsg_alloc_buffer(struct builder *bd);
extern void __last_kmsg_free_buffer(struct builder *bd);
extern int __last_kmsg_pull_last_log(struct builder *bd);
extern int __last_kmsg_procfs_create(struct builder *bd);
extern void __last_kmsg_procfs_remove(struct builder *bd);
extern int __last_kmsg_init_compression(struct builder *bd);
extern void __last_kmsg_exit_compression(struct builder *bd);

/* sec_log_buf_debugfs.c */
#if IS_ENABLED(CONFIG_DEBUG_FS)
extern int __log_buf_debugfs_create(struct builder *bd);
extern void __log_buf_debugfs_remove(struct builder *bd);
#endif

/* sec_log_buf_logger.c */
extern int __log_buf_logger_init(struct builder *bd);
extern void __log_buf_logger_exit(struct builder *bd);

/* sec_log_buf_builtin.c */
extern const struct log_buf_logger *__log_buf_logger_builtin_creator(void);

/* sec_log_buf_tp_console.c */
#if IS_ENABLED(CONFIG_SEC_LOG_BUF_USING_TP_CONSOLE)
extern const struct log_buf_logger *__log_buf_logger_tp_console_creator(void);
#else
static const inline struct log_buf_logger *__log_buf_logger_tp_console_creator(void) { return ERR_PTR(-ENODEV); }
#endif

/* sec_log_buf_kprobe.c */
#if IS_ENABLED(CONFIG_SEC_LOG_BUF_USING_KPROBE)
extern const struct log_buf_logger *__log_buf_logger_kprobe_creator(void);
#else
static const inline struct log_buf_logger *__log_buf_logger_kprobe_creator(void) { return ERR_PTR(-ENODEV); }
#endif

/* sec_log_buf_console.c */
extern const struct log_buf_logger *__log_buf_logger_console_creator(void);

/* sec_log_buf_vh_log_buf.c */
#if IS_ENABLED(CONFIG_SEC_LOG_BUF_USING_VH_LOGBUF)
extern const struct log_buf_logger *__log_buf_logger_vh_logbuf_creator(void);
#else
static const inline struct log_buf_logger *__log_buf_logger_vh_logbuf_creator(void) { return ERR_PTR(-ENODEV); }
#endif

/* sec_log_buf_ap_klog.c */
extern int __ap_klog_proc_init(struct builder *bd);
extern void __ap_klog_proc_exit(struct builder *bd);
extern int __ap_klog_proc_create_symlink(struct builder *bd);
extern void __ap_klog_proc_remove_symlink(struct builder *bd);

#endif /* __INTERNAL__SEC_QC_LOG_BUF_H__ */
