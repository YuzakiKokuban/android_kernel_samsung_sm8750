// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2019-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_debug.h>

#include "sec_debug.h"

static long *g_allocated_phys_mem;
static long *g_allocated_virt_mem;

static int sec_alloc_virtual_mem(const char *val, const struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t size = (size_t)memparse(str, &str);

	if (size) {
		mem = vmalloc(size);
		if (mem) {
			pr_info("Allocated virtual memory of size: 0x%zx bytes\n",
					size);
			*mem = (long)g_allocated_virt_mem;
			g_allocated_virt_mem = mem;

			return 0;
		}

		pr_err("Failed to allocate virtual memory of size: 0x%zx bytes\n",
				size);

		return -ENOMEM;
	}

	pr_info("Invalid size: %s bytes\n", val);

	return -EAGAIN;
}
module_param_call(alloc_virtual_mem, &sec_alloc_virtual_mem, NULL, NULL, 0644);

static int sec_free_virtual_mem(const char *val, const struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t free_count = (size_t)memparse(str, &str);

	if (!free_count) {
		if (strncmp(val, "all", 4)) {
			free_count = 10;
		} else {
			pr_err("Invalid free count: %s\n", val);
			return -EAGAIN;
		}
	}

	if (free_count > 10)
		free_count = 10;

	if (!g_allocated_virt_mem) {
		pr_err("No virtual memory chunk to free.\n");
		return 0;
	}

	while (g_allocated_virt_mem && free_count--) {
		mem = (long *) *g_allocated_virt_mem;
		vfree(g_allocated_virt_mem);
		g_allocated_virt_mem = mem;
	}

	pr_info("Freed previously allocated virtual memory chunks.\n");

	if (g_allocated_virt_mem)
		pr_info("Still, some virtual memory chunks are not freed. Try again.\n");

	return 0;
}
module_param_call(free_virtual_mem, &sec_free_virtual_mem, NULL, NULL, 0644);

static int sec_alloc_physical_mem(const char *val,
				  const struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t size = (size_t)memparse(str, &str);

	if (size) {
		mem = kmalloc(size, GFP_KERNEL);
		if (mem) {
			pr_info("Allocated physical memory of size: 0x%zx bytes\n",
				size);
			*mem = (long) g_allocated_phys_mem;
			g_allocated_phys_mem = mem;

			return 0;
		}

		pr_err("Failed to allocate physical memory of size: 0x%zx bytes\n",
			       size);

		return -ENOMEM;
	}

	pr_info("Invalid size: %s bytes\n", val);

	return -EAGAIN;
}
module_param_call(alloc_physical_mem, &sec_alloc_physical_mem,
		NULL, NULL, 0644);

static int sec_free_physical_mem(const char *val, const struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t free_count = (size_t)memparse(str, &str);

	if (!free_count) {
		if (strncmp(val, "all", 4)) {
			free_count = 10;
		} else {
			pr_info("Invalid free count: %s\n", val);
			return -EAGAIN;
		}
	}

	if (free_count > 10)
		free_count = 10;

	if (!g_allocated_phys_mem) {
		pr_info("No physical memory chunk to free.\n");
		return 0;
	}

	while (g_allocated_phys_mem && free_count--) {
		mem = (long *) *g_allocated_phys_mem;
		kfree(g_allocated_phys_mem);
		g_allocated_phys_mem = mem;
	}

	pr_info("Freed previously allocated physical memory chunks.\n");

	if (g_allocated_phys_mem)
		pr_info("Still, some physical memory chunks are not freed. Try again.\n");

	return 0;
}
module_param_call(free_physical_mem, &sec_free_physical_mem, NULL, NULL, 0644);

#if IS_BUILTIN(CONFIG_SEC_DEBUG)
static int dbg_set_cpu_affinity(const char *val, const struct kernel_param *kp)
{
	char *endptr;
	pid_t pid;
	int cpu;
	struct cpumask mask;
	long ret;

	pid = (pid_t)memparse(val, &endptr);
	if (*endptr != '@') {
		pr_info("invalid input strin: %s\n", val);
		return -EINVAL;
	}

	cpu = (int)memparse(++endptr, &endptr);
	cpumask_clear(&mask);
	cpumask_set_cpu(cpu, &mask);
	pr_info("Setting %d cpu affinity to cpu%d\n", pid, cpu);

	ret = sched_setaffinity(pid, &mask);
	pr_info("sched_setaffinity returned %ld\n", ret);

	return 0;
}
module_param_call(setcpuaff, &dbg_set_cpu_affinity, NULL, NULL, 0644);
#endif

/* FIXME: backward compatibility. This value is always 1 */
static unsigned int reboot_multicmd = 1;
module_param_named(reboot_multicmd, reboot_multicmd, uint, 0644);

static unsigned int dump_sink;
static unsigned int *dump_sink_virt;

static int sec_debug_set_dump_sink(const char *val,
		const struct kernel_param *kp)
{
	int ret = param_set_uint(val, kp);

	if (dump_sink_virt)
		*dump_sink_virt = dump_sink;

	return ret;
}

static int sec_debug_get_dump_sink(char *buffer, const struct kernel_param *kp)
{
	return param_get_uint(buffer, kp);
}

phys_addr_t sec_debug_get_dump_sink_phys(void)
{
	return virt_to_phys(dump_sink_virt);
}
EXPORT_SYMBOL_GPL(sec_debug_get_dump_sink_phys);

module_param_call(dump_sink, sec_debug_set_dump_sink, sec_debug_get_dump_sink,
		&dump_sink, 0644);

int sec_debug_node_init_dump_sink(struct builder *bd)
{
	struct device *dev = bd->dev;

	dump_sink_virt = devm_kmalloc(dev, sizeof(*dump_sink_virt), GFP_KERNEL);
	if (!dump_sink_virt)
		return -ENOMEM;

	*dump_sink_virt = dump_sink;

	return 0;
}
