// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2017-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/stringhash.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_force_err.h>

#include "sec_debug.h"

static struct force_err_handle *__force_err_find_handle_locked(
		struct force_err *force_err, const char *val)
{
	struct force_err_handle *h;
	size_t val_len = strlen(val);
	u32 key = full_name_hash(NULL, val, (unsigned int)val_len);

	hash_for_each_possible(force_err->htbl, h, node, key) {
		size_t len = strnlen(h->val, val_len + 1);

		if (len != val_len)
			continue;

		if (!strncmp(h->val, val, val_len))
			return h;
	}

	return ERR_PTR(-ENOENT);
}

static inline int __force_err_add_custom_handle(struct force_err *force_err,
		struct force_err_handle *h)
{
	struct force_err_handle *h_old;
	u32 key = full_name_hash(NULL, h->val, (unsigned int)strlen(h->val));
	int ret = 0;

	mutex_lock(&force_err->lock);

	if (hash_empty(force_err->htbl)) {
		ret = -EBUSY;
		goto not_initialized;
	}

	if (hash_hashed(&h->node)) {
		pr_warn("The node is aready added! (%s)\n", h->val);
		goto already_added;
	}

	h_old = __force_err_find_handle_locked(force_err, h->val);
	if (!IS_ERR(h_old)) {
		pr_warn("A same handler for %s is regitered before. I'll be removed.\n",
				h->val);
		hash_del(&h_old->node);
	}

	hash_add(force_err->htbl, &h->node, key);

already_added:
not_initialized:
	mutex_unlock(&force_err->lock);
	return ret;
}

int sec_force_err_add_custom_handle(struct force_err_handle *h)
{
	if (!__debug_is_probed())
		return -EBUSY;

	return __force_err_add_custom_handle(&sec_debug->force_err, h);
}
EXPORT_SYMBOL_GPL(sec_force_err_add_custom_handle);

static inline int __force_err_del_custom_handle(struct force_err *force_err,
		struct force_err_handle *h)
{
	int ret = 0;

	mutex_lock(&force_err->lock);

	if (hash_empty(force_err->htbl)) {
		ret = -EBUSY;
		goto not_initialized;
	}

	if (!hash_hashed(&h->node))
		goto already_removed;

	hash_del(&h->node);

already_removed:
not_initialized:
	mutex_unlock(&force_err->lock);
	return ret;
}

int sec_force_err_del_custom_handle(struct force_err_handle *h)
{
	if (!__debug_is_probed())
		return -EBUSY;

	return __force_err_del_custom_handle(&sec_debug->force_err, h);
}
EXPORT_SYMBOL_GPL(sec_force_err_del_custom_handle);

/* timeout for dog bark/bite */
#define DELAY_TIME 20000

static void __simulate_apps_wdog_bark(struct force_err_handle *h)
{
	unsigned long time_out_jiffies;

	pr_emerg("Simulating apps watch dog bark\n");
	local_irq_disable();

	time_out_jiffies = jiffies + msecs_to_jiffies(DELAY_TIME);
	while (time_is_after_jiffies(time_out_jiffies))
		udelay(1);

	local_irq_enable();
	/* if we reach here, simulation failed */
	pr_emerg("Simulation of apps watch dog bark failed\n");
}

static void __simulate_bug(struct force_err_handle *h)
{
	/* NOTE: force error is not enabled for end-users. */
	BUG();
}

static void __simulate_bug_on(struct force_err_handle *h)
{
	/* NOTE: force error is not enabled for end-users. */
	BUG_ON(true);
}

static void __simulate_panic(struct force_err_handle *h)
{
	/* NOTE: force error is not enabled for end-users. */
	panic("%s", __func__);
}

static void __simulate_apps_wdog_bite(struct force_err_handle *h)
{
	unsigned long time_out_jiffies;

#if IS_ENABLED(CONFIG_HOTPLUG_CPU)
	int cpu;

	for_each_online_cpu(cpu) {
		if (cpu == 0)
			continue;
		remove_cpu(cpu);
	}
#endif

	pr_emerg("Simulating apps watch dog bite\n");
	local_irq_disable();

	time_out_jiffies = jiffies + msecs_to_jiffies(DELAY_TIME);
	while (time_is_after_jiffies(time_out_jiffies))
		udelay(1);

	local_irq_enable();
	/* if we reach here, simulation had failed */
	pr_emerg("Simualtion of apps watch dog bite failed\n");
}

static void __simulate_bus_hang(struct force_err_handle *h)
{
	void __iomem *p = NULL;

	pr_emerg("Generating Bus Hang!\n");

	p = ioremap_wt(0xFC4B8000, 32);
	*(unsigned int *)p = *(unsigned int *)p;
	mb();	/* memory barriar to generate bus hang */

	pr_info("*p = %x\n", *(unsigned int *)p);

	pr_emerg("Clk may be enabled.Try again if it reaches here!\n");
}

unsigned long *__dabort_buf;

static void __simulate_dabort(struct force_err_handle *h)
{
	*__dabort_buf = 0;
}

static void __simulate_pabort(struct force_err_handle *h)
{
	((void (*)(void))NULL)();
}

static void __simulate_dblfree(struct force_err_handle *h)
{
	unsigned int *ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);

	kfree(ptr);
	msleep(1000);
	kfree(ptr);
}

static void __simulate_danglingref(struct force_err_handle *h)
{
	unsigned int *ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);

	kfree(ptr);

	*ptr = 0x1234;
}

static void __simulate_lowmem(struct force_err_handle *h)
{
	size_t i;

	for (i = 0; kmalloc(128 * 1024, GFP_KERNEL); i++)
		;

	pr_emerg("Allocated %zu KB!\n", i * 128);
}

static void __simulate_memcorrupt(struct force_err_handle *h)
{
	unsigned int *ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);

	*ptr++ = 4;
	*ptr = 2;

	/* NOTE: force error is not enabled for end-users. */
	panic("MEMORY CORRUPTION");
}

static struct force_err_handle __force_err_default[] = {
	FORCE_ERR_HANDLE("appdogbark", "Generating an apps wdog bark!",
			__simulate_apps_wdog_bark),
	FORCE_ERR_HANDLE("appdogbite", "Generating an apps wdog bite!",
			__simulate_apps_wdog_bite),
	FORCE_ERR_HANDLE("dabort", "Generating a data abort exception!",
			__simulate_dabort),
	FORCE_ERR_HANDLE("pabort", "Generating a data abort exception!",
			__simulate_pabort),
	FORCE_ERR_HANDLE("bushang", "Generating a Bus Hang!",
			__simulate_bus_hang),
	FORCE_ERR_HANDLE("dblfree", NULL,
			__simulate_dblfree),
	FORCE_ERR_HANDLE("danglingref", NULL,
			__simulate_danglingref),
	FORCE_ERR_HANDLE("lowmem", "Allocating memory until failure!",
			__simulate_lowmem),
	FORCE_ERR_HANDLE("memcorrupt", NULL,
			__simulate_memcorrupt),
	FORCE_ERR_HANDLE("KP", "Generating a data abort exception!",
			__simulate_dabort),
	FORCE_ERR_HANDLE("DP", NULL,
			__simulate_apps_wdog_bark),
	FORCE_ERR_HANDLE("bug", "call BUG()",
			__simulate_bug),
	FORCE_ERR_HANDLE("bug_on", "call BUG_ON()",
			__simulate_bug_on),
	FORCE_ERR_HANDLE("panic", "call panic()",
			__simulate_panic),
};

static long __force_error(struct force_err *force_err, const char *val)
{
	struct force_err_handle *h;
	long err = 0;

	pr_emerg("!!!WARN forced error : %s\n", val);

	mutex_lock(&force_err->lock);

	h = __force_err_find_handle_locked(force_err, val);
	if (IS_ERR(h)) {
		pr_warn("%s is not supported!\n", val);
		mutex_unlock(&force_err->lock);
		return 0;
	}

	h->func(h);
	pr_emerg("No such error defined for now!\n");

	mutex_unlock(&force_err->lock);
	return err;
}

static int force_error(const char *val, const struct kernel_param *kp)
{
	char *__trimed_val, *trimed_val;
	int err;

	if (!__debug_is_probed())
		return -EBUSY;

	__trimed_val = kstrdup(val, GFP_KERNEL);
	if (!__trimed_val) {
		pr_err("Not enough memory!\n");
		return 0;
	}
	trimed_val = strim(__trimed_val);

	err = (int)__force_error(&sec_debug->force_err, trimed_val);

	kfree(__trimed_val);

	return err;
}
module_param_call(force_error, force_error, NULL, NULL, 0644);

int sec_force_err_probe_prolog(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata =
			container_of(bd, struct sec_debug_drvdata, bd);
	struct force_err *force_err = &drvdata->force_err;

	mutex_init(&force_err->lock);
	hash_init(force_err->htbl);

	return 0;
}

void sec_force_err_remove_epilog(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata =
			container_of(bd, struct sec_debug_drvdata, bd);
	struct force_err *force_err = &drvdata->force_err;

	mutex_destroy(&force_err->lock);
}

int sec_force_err_build_htbl(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata =
			container_of(bd, struct sec_debug_drvdata, bd);
	struct force_err *force_err = &drvdata->force_err;
	struct force_err_handle *h;
	u32 key;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(__force_err_default); i++) {
		h = &__force_err_default[i];

		INIT_HLIST_NODE(&h->node);

		key = full_name_hash(NULL, h->val, (unsigned int)strlen(h->val));
		hash_add(force_err->htbl, &h->node, key);
	}

	return 0;
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static void __force_err_dbgfs_show_each_locked(struct seq_file *m,
		struct force_err_handle *h)
{
	seq_printf(m, "[<%p>] %s\n", h, h->val);
	seq_printf(m, "  - msg  : %s\n", h->msg);
	seq_printf(m, "  - func : [<%p>] %ps\n", h->func, h->func);

	seq_puts(m, "\n");
}

static int sec_force_err_dbgfs_show_all(struct seq_file *m, void *unsed)
{
	struct force_err *force_err = m->private;
	struct force_err_handle *h;
	int bkt;

	mutex_lock(&force_err->lock);
	hash_for_each(force_err->htbl, bkt, h, node) {
		__force_err_dbgfs_show_each_locked(m, h);
	}
	mutex_unlock(&force_err->lock);

	return 0;
}

static int sec_force_err_dbgfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_force_err_dbgfs_show_all,
			inode->i_private);
}

static const struct file_operations sec_force_err_dgbfs_fops = {
	.open = sec_force_err_dbgfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int sec_force_err_debugfs_create(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata =
			container_of(bd, struct sec_debug_drvdata, bd);
	struct force_err *force_err = &drvdata->force_err;

	force_err->dbgfs = debugfs_create_file("sec_force_err", 0440,
			NULL, force_err, &sec_force_err_dgbfs_fops);

	return 0;
}

void sec_force_err_debugfs_remove(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata =
			container_of(bd, struct sec_debug_drvdata, bd);
	struct force_err *force_err = &drvdata->force_err;

	debugfs_remove(force_err->dbgfs);
}
#else
int sec_force_err_debugfs_create(struct builder *bd) { return 0; }
void sec_force_err_debugfs_remove(struct builder *bd) {}
#endif /* IS_ENABLED(CONFIG_DEBUG_FS) */
