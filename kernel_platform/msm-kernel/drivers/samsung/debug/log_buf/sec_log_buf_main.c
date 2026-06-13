// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2010-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/cacheflush.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/samsung/sec_of.h>

#include "sec_log_buf.h"

struct log_buf_drvdata *sec_log_buf __read_mostly;

static struct sec_log_buf_head *s_log_buf __read_mostly;
static size_t sec_log_buf_size __read_mostly;
static struct atomic_notifier_head *sync_list __read_mostly;

const char *block_str[] = {
	"init: Loading module",
};

static void (*__log_buf_memcpy_fromio)(void *, const void *, size_t) __read_mostly;
static void (*__log_buf_memcpy_toio)(void *, const void *, size_t) __read_mostly;

static void notrace ____log_buf_memcpy_fromio(void *dst, const void *src, size_t cnt)
{
	memcpy_fromio(dst, src, cnt);
}

static void notrace ____log_buf_memcpy_toio(void *dst, const void *src, size_t cnt)
{
	memcpy_toio(dst, src, cnt);
}

static void notrace ____log_buf_memcpy(void *dst, const void *src, size_t cnt)
{
	memcpy(dst, src, cnt);
}

const struct sec_log_buf_head *__log_buf_get_header(void)
{
	return s_log_buf;
}

const struct sec_log_buf_head *sec_log_buf_get_header(void)
{
	if (!__log_buf_is_probed())
		return ERR_PTR(-EBUSY);

	return __log_buf_get_header();
}
EXPORT_SYMBOL_GPL(sec_log_buf_get_header);

ssize_t __log_buf_get_buf_size(void)
{
	return sec_log_buf_size;
}

ssize_t sec_log_buf_get_buf_size(void)
{
	if (!__log_buf_is_probed())
		return -EBUSY;

	return __log_buf_get_buf_size();
}
EXPORT_SYMBOL_GPL(sec_log_buf_get_buf_size);

static int __log_buf_register_sync_handler(struct log_buf_drvdata *drvdata,
		struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&drvdata->sync_list, nb);
}

int sec_log_buf_register_sync_handler(struct notifier_block *nb)
{
	if (!__log_buf_is_probed())
		return -EBUSY;

	return __log_buf_register_sync_handler(sec_log_buf, nb);
}
EXPORT_SYMBOL_GPL(sec_log_buf_register_sync_handler);

static int __log_buf_unregister_sync_handler(struct log_buf_drvdata *drvdata,
		struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&drvdata->sync_list, nb);
}

int sec_log_buf_unregister_sync_handler(struct notifier_block *nb)
{
	if (!__log_buf_is_probed())
		return -EBUSY;

	return __log_buf_unregister_sync_handler(sec_log_buf, nb);
}
EXPORT_SYMBOL_GPL(sec_log_buf_unregister_sync_handler);

bool __log_buf_is_acceptable(const char *s, size_t count)
{
	static bool filter_en = !!IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP);
	const char *magic_str = "init: init second stage started!";
	size_t i;

	if (likely(!filter_en))
		return true;

	if (strnstr(s, magic_str, count)) {
		filter_en = false;
		return true;
	}

	for (i = 0; i < ARRAY_SIZE(block_str); i++) {
		if (strnstr(s, block_str[i], count))
			return false;
	}

	return true;
}

void notrace __log_buf_write(const char *s, size_t count)
{
	size_t f_len, s_len, remain_space;
	size_t idx;

	idx = s_log_buf->idx % sec_log_buf_size;
	remain_space = sec_log_buf_size - idx;
	f_len = min(count, remain_space);
	__log_buf_memcpy_toio(&(s_log_buf->buf[idx]), s, f_len);

	s_len = count - f_len;
	if (unlikely(s_len))
		__log_buf_memcpy_toio(s_log_buf->buf, &s[f_len], s_len);

	s_log_buf->idx += (uint32_t)count;

	atomic_notifier_call_chain(sync_list, s_log_buf->idx, s_log_buf);
}

static __always_inline size_t __log_buf_print_kmsg(unsigned int cpu,
		char *buf, size_t sz_buf)
{
	struct kmsg_dump_iter *iter = &sec_log_buf->iter;
	size_t len;

	if (kmsg_dump_get_line(iter, true, buf, sz_buf, &len))
		return len;

	return 0;
}

#define SZ_TASK_BUF		32
#define SZ_KMSG_BUF		256

struct log_buf_kmsg_ctx {
	char task[SZ_TASK_BUF];
	size_t task_len;
	char head[SZ_KMSG_BUF];
	size_t head_len;
	char *tail;
	size_t tail_len;
};

static DEFINE_PER_CPU(struct log_buf_kmsg_ctx, kmsg_ctx);
static DEFINE_PER_CPU(struct log_buf_kmsg_ctx, kmsg_ctx_irq);

static bool __log_buf_kmsg_check_level_text(struct log_buf_kmsg_ctx *ctx)
{
	char *head = ctx->head;
	char *endp;
	long l;

	if (head[0] != '<')
		return false;

	/* NOTE: simple_strto{?} fucnctions are not recommended for normal cases.
	 * Because the position of next token is requried, kstrto{?} functions
	 * can not be used or more complex implementation is needed.
	 */
	l = simple_strtol(&head[1], &endp, 10);
	if (!endp || endp[0] != '>')
		return false;

	return true;
}

static void __log_buf_kmsg_split(struct log_buf_kmsg_ctx *ctx)
{
	char *head = ctx->head;
	char *tail;
	const char *delim = "] ";
	size_t head_len;

	tail = strnstr(head, delim, SZ_KMSG_BUF);
	if (!tail) {
		ctx->tail = NULL;
		return;
	}

	tail = &tail[2];
	head_len = tail - head - 1;
	head[head_len] = '\0';

	ctx->tail = tail;
	ctx->tail_len = ctx->head_len - head_len - 1;

	ctx->head_len = head_len;
}

static __always_inline void __log_buf_kmg_print(struct log_buf_kmsg_ctx *ctx)
{
	if (__log_buf_kmsg_check_level_text(ctx))
		__log_buf_kmsg_split(ctx);

	__log_buf_write(ctx->head, ctx->head_len);
	if (ctx->tail) {
		__log_buf_write(ctx->task, ctx->task_len);
		__log_buf_write(ctx->tail, ctx->tail_len);
	}
}

size_t __log_buf_copy_to_buffer(void *__buf)
{
	char *buf = (char *)__buf;
	const struct sec_log_buf_head *log_buf_head = __log_buf_get_header();
	const size_t log_buf_size = __log_buf_get_buf_size();
	const size_t max_size = log_buf_size;
	size_t head;
	size_t total;

	if (log_buf_head->idx > max_size) {
		head = (size_t)log_buf_head->idx % log_buf_size;
		__log_buf_memcpy_fromio(buf, &log_buf_head->buf[head],
				log_buf_size - head);
		if (head != 0)
			__log_buf_memcpy_fromio(&buf[log_buf_size - head],
					log_buf_head->buf, head);
		total = max_size;
	} else {
		__log_buf_memcpy_fromio(buf, log_buf_head->buf, log_buf_head->idx);
		total = log_buf_head->idx;
	}

	return total;
}

static noinline int __log_buf_parse_dt_strategy(struct builder *bd,
		struct device_node *np)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device *dev = bd->dev;
	u32 strategy;
	int err;

	err = of_property_read_u32(np, "sec,strategy", &strategy);
	if (err)
		return -EINVAL;

	if (IS_MODULE(CONFIG_SEC_LOG_BUF) &&
			(strategy == SEC_LOG_BUF_STRATEGY_BUILTIN)) {
		dev_err(dev, "BUILTIN strategy can't be used in the kernel module!\n");
		return -EINVAL;
	}

	if (strategy >= SEC_LOG_BUF_NR_STRATEGIES) {
		dev_err(dev, "invalid strategy (%u)!\n", strategy);
		return -EINVAL;
	}

	drvdata->strategy = (unsigned int)strategy;

	return 0;
}

static noinline int __log_buf_parse_dt_memory_region(struct builder *bd,
		struct device_node *np)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device *dev = bd->dev;
	struct device_node *mem_np;
	struct reserved_mem *rmem;

	mem_np = of_parse_phandle(np, "memory-region", 0);
	if (!mem_np)
		return -EINVAL;

	rmem = of_reserved_mem_lookup(mem_np);
	if (!rmem) {
		dev_warn(dev, "failed to get a reserved memory (%s)\n",
				mem_np->name);
		return -EFAULT;
	}

	drvdata->rmem = rmem;

	return 0;
}

static bool __log_buf_is_in_reserved_mem_bound(
		const struct reserved_mem *rmem,
		phys_addr_t base, phys_addr_t size)
{
	phys_addr_t rmem_base = rmem->base;
	phys_addr_t rmem_end = rmem_base + rmem->size - 1;
	phys_addr_t end = base + size - 1;

	if ((base >= rmem_base) && (end <= rmem_end))
		return true;

	return false;
}

static int __log_buf_use_partial_reserved_mem(
	struct log_buf_drvdata *drvdata, struct device_node *np)
{
	struct reserved_mem *rmem = drvdata->rmem;
	phys_addr_t base;
	phys_addr_t size;
	int err;

	err = sec_of_parse_reg_prop(np, &base, &size);
	if (err)
		return err;

	if (!__log_buf_is_in_reserved_mem_bound(rmem, base, size))
		return -ERANGE;

	drvdata->paddr = base;
	drvdata->size = size;

	return 0;
}

static int __log_buf_use_entire_reserved_mem(
	struct log_buf_drvdata *drvdata)
{
	struct reserved_mem *rmem = drvdata->rmem;

	drvdata->paddr = rmem->base;
	drvdata->size = rmem->size;

	return 0;
}

static noinline int __log_buf_parse_dt_partial_reserved_mem(struct builder *bd,
		struct device_node *np)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	int err;

	if (of_property_read_bool(np, "sec,use-partial_reserved_mem"))
		err = __log_buf_use_partial_reserved_mem(drvdata, np);
	else
		err = __log_buf_use_entire_reserved_mem(drvdata);

	if (err)
		return -EFAULT;

	return 0;
}

static noinline int __log_buf_parse_dt_test_no_map(struct builder *bd,
		struct device_node *np)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device_node *mem_np;

	mem_np = of_parse_phandle(np, "memory-region", 0);
	if (!mem_np)
		return -EINVAL;

	if (!of_property_read_bool(mem_np, "no-map")) {
		s_log_buf = phys_to_virt(drvdata->paddr);
		__log_buf_memcpy_fromio = ____log_buf_memcpy;
		__log_buf_memcpy_toio = ____log_buf_memcpy;
	} else {
		__log_buf_memcpy_fromio = ____log_buf_memcpy_fromio;
		__log_buf_memcpy_toio = ____log_buf_memcpy_toio;
	}

	return 0;
}

static noinline int __last_kmsg_parse_dt_use_last_kmsg_compression(struct builder *bd,
		struct device_node *np)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;

	last_kmsg->use_compression =
			of_property_read_bool(np, "sec,use-last_kmsg_compression");

	return 0;
}

static noinline int __last_kmsg_parse_dt_last_kmsg_compressor(struct builder *bd,
		struct device_node *np)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;

	if (!last_kmsg->use_compression)
		return 0;

	return of_property_read_string(np, "sec,last_kmsg_compressor",
			&last_kmsg->compressor);
}

static const struct dt_builder __log_buf_dt_builder[] = {
	DT_BUILDER(__log_buf_parse_dt_strategy),
	DT_BUILDER(__log_buf_parse_dt_memory_region),
	DT_BUILDER(__log_buf_parse_dt_partial_reserved_mem),
	DT_BUILDER(__log_buf_parse_dt_test_no_map),
	DT_BUILDER(__last_kmsg_parse_dt_use_last_kmsg_compression),
	DT_BUILDER(__last_kmsg_parse_dt_last_kmsg_compressor),
};

static noinline int __log_buf_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __log_buf_dt_builder,
			ARRAY_SIZE(__log_buf_dt_builder));
}

static void __iomem *__log_buf_ioremap(struct log_buf_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;

	if (s_log_buf)
		return s_log_buf;

	return devm_ioremap(dev, drvdata->paddr, drvdata->size);
}

static noinline int __log_buf_probe_prolog(struct builder * bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);

	ATOMIC_INIT_NOTIFIER_HEAD(&drvdata->sync_list);
	sync_list = &drvdata->sync_list;

	return 0;
}

static noinline void __log_buf_prepare_buffer_raw(struct log_buf_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;

	dev_warn(dev, "sec_log_magic is not valid : 0x%x at 0x%p\n",
			s_log_buf->magic, &(s_log_buf->magic));

	s_log_buf->magic = SEC_LOG_MAGIC;
	s_log_buf->idx = 0;
	s_log_buf->prev_idx = 0;
}

static noinline int __log_buf_prepare_buffer(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);

	s_log_buf = __log_buf_ioremap(drvdata);
	if (!s_log_buf)
		return -EFAULT;

	if (s_log_buf->magic != SEC_LOG_MAGIC)
		__log_buf_prepare_buffer_raw(drvdata);

	sec_log_buf_size = drvdata->size -
			offsetof(struct sec_log_buf_head, buf);

	return 0;
}

static ssize_t __pull_early_buffer(struct log_buf_drvdata *drvdata, char *buf)
{
	struct kmsg_dump_iter *iter = &drvdata->iter;
	ssize_t copied;
	char *line;
	size_t len;

	line = kvmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!line)
		return -ENOMEM;

	memset(buf, 0x0, drvdata->size);
	copied = 0;
	kmsg_dump_rewind(iter);
	while (kmsg_dump_get_line(iter, true, line, PAGE_SIZE, &len)) {
		memcpy_fromio(&buf[copied], line, len);
		copied += len;
	}

	kvfree(line);

	return copied;
}

static size_t __remove_till_end_of_line(char *substr)
{
	size_t i = 0;

	while (substr[i] != '\n' && substr[i] != '\0')
		substr[i++] = ' ';

	return i;
}

static void ____remove_block_str(char *buf, size_t len, const char *keyword)
{
	size_t offset = 0;
	char *substr;

	while (offset < len) {
		substr = strnstr(&buf[offset], keyword, len);
		if (!substr)
			break;

		offset = substr - buf;
		offset += __remove_till_end_of_line(substr);
	}
}

static void __remove_block_str(char *buf, size_t len)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(block_str); i++)
		____remove_block_str(buf, len, block_str[i]);
}

static noinline int __log_buf_pull_early_buffer(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	char *buf;
	ssize_t copied;

	buf = kvmalloc(drvdata->size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	copied = __pull_early_buffer(drvdata, buf);
	if (copied < 0) {
		kvfree(buf);
		return copied;
	}

	if (IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP))
		__remove_block_str(buf, copied);

	__log_buf_write(buf, copied);

	kvfree(buf);

	return 0;
}

void notrace __log_buf_store_from_kmsg_dumper(void)
{
	unsigned int cpu;
	struct log_buf_kmsg_ctx *ctx_pcpu;

	cpu = get_cpu();

	if (in_irq())
		ctx_pcpu = this_cpu_ptr(&kmsg_ctx_irq);
	else
		ctx_pcpu = this_cpu_ptr(&kmsg_ctx);

	ctx_pcpu->head_len = __log_buf_print_kmsg(cpu,
			ctx_pcpu->head, SZ_KMSG_BUF);
	if (!ctx_pcpu->head_len)
		goto print_nothing;

	ctx_pcpu->task_len = __log_buf_print_process(cpu,
			ctx_pcpu->task, SZ_TASK_BUF);

	do {
		__log_buf_kmg_print(ctx_pcpu);

		ctx_pcpu->head_len = __log_buf_print_kmsg(cpu,
				ctx_pcpu->head, SZ_KMSG_BUF);
	} while (ctx_pcpu->head_len);

print_nothing:
	put_cpu();
}

static int sec_log_buf_restart_handler(struct notifier_block *nb,
		unsigned long l, void *msg)
{
	struct log_buf_drvdata *drvdata =
			container_of(nb, struct log_buf_drvdata, nb_restart);
	const unsigned long pfn = PHYS_PFN(drvdata->paddr);
	const unsigned long nr_pages = (unsigned long)(drvdata->size >> PAGE_SHIFT);
	unsigned long i;

	flush_cache_all();	/* NOTE: may not work on arm64 architecture */

	for (i = 0; i < nr_pages; i++) {
		struct page *page = pfn_to_page(pfn + i);
		flush_dcache_page(page);
	}

	return NOTIFY_OK;
}

static int __log_buf_register_restart_handler(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct notifier_block *nb = &drvdata->nb_restart;

	nb->priority = 255;
	nb->notifier_call = sec_log_buf_restart_handler;

	return register_restart_handler(nb);
}

static void __log_buf_unregister_restart_handler(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct notifier_block *nb = &drvdata->nb_restart;

	unregister_restart_handler(nb);
}

static noinline int __log_buf_probe_epilog(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	sec_log_buf = drvdata;	/* set a singleton */

	pr_debug("buf base virtual addrs 0x%p phy=%pa\n", s_log_buf,
			&sec_log_buf->paddr);

	return 0;
}

static noinline void __log_buf_remove_prolog(struct builder *bd)
{
	/* FIXME: This is not a graceful exit.
	 * 'sec_log_buf' can be used in some calling 'printk'.
	 */
	sec_log_buf = NULL;
}

static int __log_buf_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct log_buf_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __log_buf_probe_threaded(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct log_buf_drvdata *drvdata = platform_get_drvdata(pdev);

	return sec_director_probe_dev_threaded(&drvdata->bd, builder, n,
			"log_buf");
}

static int __log_buf_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct log_buf_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int __log_buf_remove_threaded(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct log_buf_drvdata *drvdata = platform_get_drvdata(pdev);
	struct director_threaded *drct = drvdata->bd.drct;

	sec_director_destruct_dev_threaded(drct);

	return 0;
}

static const struct dev_builder __log_buf_dev_builder[] = {
	DEVICE_BUILDER(__log_buf_parse_dt, NULL),
	DEVICE_BUILDER(__log_buf_probe_prolog, NULL),
	DEVICE_BUILDER(__log_buf_prepare_buffer, NULL),
	DEVICE_BUILDER(__last_kmsg_alloc_buffer, __last_kmsg_free_buffer),
	DEVICE_BUILDER(__last_kmsg_pull_last_log, NULL),
	DEVICE_BUILDER(__last_kmsg_procfs_create, __last_kmsg_procfs_remove),
	DEVICE_BUILDER(__log_buf_pull_early_buffer, NULL),
	DEVICE_BUILDER(__log_buf_logger_init, __log_buf_logger_exit),
	DEVICE_BUILDER(__ap_klog_proc_init, __ap_klog_proc_exit),
	DEVICE_BUILDER(__ap_klog_proc_create_symlink, __ap_klog_proc_remove_symlink),
	DEVICE_BUILDER(__log_buf_register_restart_handler, __log_buf_unregister_restart_handler),
	DEVICE_BUILDER(__log_buf_probe_epilog, __log_buf_remove_prolog),
};

static const struct dev_builder __log_buf_dev_builder_threaded[] = {
#if IS_ENABLED(CONFIG_DEBUG_FS)
	DEVICE_BUILDER(__log_buf_debugfs_create, __log_buf_debugfs_remove),
#endif
	DEVICE_BUILDER(__last_kmsg_init_compression, __last_kmsg_exit_compression),
};

static int sec_log_buf_probe(struct platform_device *pdev)
{
	int err;

	err = __log_buf_probe(pdev, __log_buf_dev_builder,
			ARRAY_SIZE(__log_buf_dev_builder));
	if (err)
		return err;

	return __log_buf_probe_threaded(pdev, __log_buf_dev_builder_threaded,
			ARRAY_SIZE(__log_buf_dev_builder_threaded));
}

static int sec_log_buf_remove(struct platform_device *pdev)
{
	__log_buf_remove_threaded(pdev, __log_buf_dev_builder_threaded,
			ARRAY_SIZE(__log_buf_dev_builder_threaded));

	__log_buf_remove(pdev, __log_buf_dev_builder,
			ARRAY_SIZE(__log_buf_dev_builder));

	return 0;
}

static const struct of_device_id sec_log_buf_match_table[] = {
	{ .compatible = "samsung,kernel_log_buf" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_log_buf_match_table);

static struct platform_driver sec_log_buf_driver = {
	.driver = {
		.name = "sec,log_buf",
		.of_match_table = of_match_ptr(sec_log_buf_match_table),
	},
	.probe = sec_log_buf_probe,
	.remove = sec_log_buf_remove,
};

static int __init sec_log_buf_init(void)
{
	return platform_driver_register(&sec_log_buf_driver);
}
/* NOTE: all compression algorithms are registered in 'subsys_initcall' stage. */
subsys_initcall_sync(sec_log_buf_init);

static void __exit sec_log_buf_exit(void)
{
	platform_driver_unregister(&sec_log_buf_driver);
}
module_exit(sec_log_buf_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Kernel log buffer shared with boot loader");
MODULE_LICENSE("GPL v2");
