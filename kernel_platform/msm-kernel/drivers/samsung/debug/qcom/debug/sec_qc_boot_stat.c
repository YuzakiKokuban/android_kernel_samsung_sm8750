// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2021-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/sched/clock.h>

#include "sec_qc_debug.h"

/* NOTE: see 'drivers/soc/qcom/boot_stat.c' */
struct boot_stats {
	uint32_t bootloader_start;
	uint32_t bootloader_end;
	uint32_t bootloader_display;
	uint32_t bootloader_load_kernel;
};

static uint32_t bootloader_start;
static uint32_t bootloader_end;
static uint32_t bootloader_display;
static uint32_t bootloader_load_kernel;
static uint32_t clock_freq;
static unsigned long long ktime_delta;

static int __boot_stat_init_boot_stats(void)
{
	struct device_node *np_imem;
	struct boot_stats __iomem *boot_stats;

	np_imem = of_find_compatible_node(NULL, NULL,
			"qcom,msm-imem-boot_stats");
	if (!np_imem) {
		pr_warn("can't find qcom,msm-imem node\n");
		return -ENODEV;
	}

	boot_stats = of_iomap(np_imem, 0);
	if (!boot_stats) {
		pr_warn("boot_stats: Can't map imem\n");
		return -ENXIO;
	}

	bootloader_start = readl_relaxed(&boot_stats->bootloader_start);
	bootloader_end = readl_relaxed(&boot_stats->bootloader_end);
	bootloader_display = readl_relaxed(&boot_stats->bootloader_display);
	bootloader_load_kernel = readl_relaxed(
			&boot_stats->bootloader_load_kernel);

	iounmap(boot_stats);

	return 0;
}

static unsigned int __boot_stat_counter_to_msec(unsigned int counter)
{
	/* (second / HZ) * 1000 */
	return counter * 1000 / clock_freq;
}

static int __boot_stat_init_mpm_data(void)
{
	struct device_node *np_mpm2;
	void __iomem *counter_base;
	uint32_t counter;
	unsigned long long ktime;

	np_mpm2 = of_find_compatible_node(NULL, NULL,
			"qcom,mpm2-sleep-counter");

	if (!np_mpm2) {
		pr_warn("mpm_counter: can't find DT node\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np_mpm2, "clock-frequency", &clock_freq)) {
		pr_warn("mpm_counter: can't read clock-frequncy\n");
		return -ENOENT;
	}

	if (!of_get_address(np_mpm2, 0, NULL, NULL)) {
		pr_warn("mpm_counter: can't find resource");
		return -ENOENT;
	}

	counter_base = of_iomap(np_mpm2, 0);
	if (!counter_base) {
		pr_warn("mpm_counter: cant map counter base\n");
		return -ENXIO;
	}

	counter = readl_relaxed(counter_base);
	ktime = local_clock();

	ktime_delta = __boot_stat_counter_to_msec(counter);
	ktime_delta *= 1000000ULL;
	ktime_delta -= ktime;

	return 0;
}

static unsigned long long sec_qc_boot_stat_ktime_to_time(unsigned long long ktime)
{
	return ktime + ktime_delta;
}

static void sec_qc_boot_stat_show_on_boot_stat(struct seq_file *m)
{
	seq_printf(m, "%-46s%11u%13u%13u\n", "!@Boot: Bootloader start",
			__boot_stat_counter_to_msec(bootloader_start), 0, 0);
	seq_printf(m, "%-46s%11u%13u%13u\n", "!@Boot: Bootloader display",
			__boot_stat_counter_to_msec(bootloader_display), 0, 0);
	seq_printf(m, "%-46s%11u%13u%13u\n", "!@Boot: Bootloader load kernel",
			__boot_stat_counter_to_msec(bootloader_load_kernel), 0, 0);
	seq_printf(m, "%-46s%11u%13u%13u\n", "!@Boot: Bootloader end",
			__boot_stat_counter_to_msec(bootloader_end), 0, 0);
}

static void sec_qc_enhanced_boot_stat_show_on_boot_stat(struct seq_file *m)
{
	seq_printf(m, "%-90s%7u%7u%7u\n", "!@Boot_EBS_B: Bootloader start",
			__boot_stat_counter_to_msec(bootloader_start), 0,
			__boot_stat_counter_to_msec(bootloader_start));
	seq_printf(m, "%-90s%7u%7u%7u\n", "!@Boot_EBS_B: Bootloader display",
			__boot_stat_counter_to_msec(bootloader_display), 0, 
			__boot_stat_counter_to_msec(bootloader_display - bootloader_start));
	seq_printf(m, "%-90s%7u%7u%7u\n", "!@Boot_EBS_B: Bootloader load kernel",
			__boot_stat_counter_to_msec(bootloader_load_kernel), 0,
			__boot_stat_counter_to_msec(bootloader_load_kernel - bootloader_display));
	seq_printf(m, "%-90s%7u%7u%7u\n", "!@Boot_EBS_B: Bootloader end",
			__boot_stat_counter_to_msec(bootloader_end), 0,
			__boot_stat_counter_to_msec(bootloader_end - bootloader_load_kernel));
}

int sec_qc_boot_stat_init(struct builder *bd)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);
	struct device *dev = bd->dev;
	struct sec_boot_stat_soc_operations *boot_stat_ops;
	int err;

	err = __boot_stat_init_boot_stats();
	if (err)
		/* NOTE: soc specific operations will be ignored */
		return 0;

	err = __boot_stat_init_mpm_data();
	if (err)
		/* NOTE: soc specific operations will be ignored */
		return 0;

	boot_stat_ops = devm_kzalloc(dev, sizeof(*boot_stat_ops), GFP_KERNEL);
	if (!boot_stat_ops)
		return -ENOMEM;

	boot_stat_ops->ktime_to_time = sec_qc_boot_stat_ktime_to_time;
	boot_stat_ops->show_on_boot_stat = sec_qc_boot_stat_show_on_boot_stat;
	boot_stat_ops->show_on_enh_boot_stat = sec_qc_enhanced_boot_stat_show_on_boot_stat;

	err = sec_boot_stat_register_soc_ops(boot_stat_ops);
	if (err == -EBUSY)
		return -EPROBE_DEFER;
	else if (err)
		return err;

	drvdata->boot_stat_ops = boot_stat_ops;

	return 0;
}

void sec_qc_boot_stat_exit(struct builder *bd)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);
	struct sec_boot_stat_soc_operations *boot_stat_ops =
			drvdata->boot_stat_ops;

	if (boot_stat_ops)
		sec_boot_stat_unregister_soc_ops(boot_stat_ops);
}
