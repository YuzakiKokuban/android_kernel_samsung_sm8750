// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/kernel.h>
#include <linux/sched/debug.h>
#include <linux/sched/signal.h>
#include <linux/sched/stat.h>

static bool __debug_is_platform_lockup_suspected(const char *msg)
{
	static const char *expected[] = {
		"Crash Key",
		"User Crash Key",
		"Long Key Press",
		"Software Watchdog Timer expired",
	};
	size_t i;

	for (i = 0; i < ARRAY_SIZE(expected); i++) {
		if (!strncmp(msg, expected[i], strlen(expected[i])))
			return true;
	}

	return false;
}

/* NOTE: see 'state_filter_match' function */
static bool __debug_is_task_uninterruptible(struct task_struct *p)
{
	if (!(p->__state & TASK_UNINTERRUPTIBLE))
		return false;

	if (p->__state == TASK_IDLE)
		return false;

	return true;
}

/* NOTE: see 'show_state_filter' function */
static void ____debug_show_task_uninterruptible(void)
{
	struct task_struct *g, *p;

	for_each_process_thread(g, p) {
		if (__debug_is_task_uninterruptible(p))
			sched_show_task(p);
	}
}

static void __debug_show_task_uninterruptible(void)
{
	pr_info("\n");
	pr_info(" ---------------------------------------------------------------------------------------\n");

	if (IS_BUILTIN(CONFIG_SEC_DEBUG))
		show_state_filter(TASK_UNINTERRUPTIBLE);
	else
		____debug_show_task_uninterruptible();

	pr_info(" ---------------------------------------------------------------------------------------\n");
}

/* TODO: this is a modified version of 'show_stat' in 'fs/proc/stat.c'
 * this function should be adapted for each kernel version
 */
#ifndef arch_irq_stat_cpu
#define arch_irq_stat_cpu(cpu) 0
#endif
#ifndef arch_irq_stat
#define arch_irq_stat() 0
#endif

static void __debug_show_cpu_stat(void)
{
	int i, j;
	u64 user, nice, system, idle, iowait, irq, softirq, steal;
	u64 guest, guest_nice;
	u64 sum = 0;
	u64 sum_softirq = 0;
	unsigned int per_softirq_sums[NR_SOFTIRQS] = {0};
	struct timespec64 boottime;

	user = nice = system = idle = iowait =
		irq = softirq = steal = 0;
	guest = guest_nice = 0;
	getboottime64(&boottime);

	for_each_possible_cpu(i) {
		user += kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice += kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system += kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle += kcpustat_cpu(i).cpustat[CPUTIME_IDLE];
		iowait += kcpustat_cpu(i).cpustat[CPUTIME_IOWAIT];
		irq += kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq += kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal += kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest += kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice += kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
		sum += kstat_cpu_irqs_sum(i);
		sum += arch_irq_stat_cpu(i);

		for (j = 0; j < NR_SOFTIRQS; j++) {
			unsigned int softirq_stat = kstat_softirqs_cpu(j, i);

			per_softirq_sums[j] += softirq_stat;
			sum_softirq += softirq_stat;
		}
	}
	sum += arch_irq_stat();

	pr_info("\n");
	pr_info(" ---------------------------------------------------------------------------------------\n");
	pr_info("      %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s\n",
			"user", "nice", "system", "idle", "iowait", "irq",
			"softirq", "steal", "guest", "guest_nice");
	pr_info("cpu   %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu\n",
			nsec_to_clock_t(user),
			nsec_to_clock_t(nice),
			nsec_to_clock_t(system),
			nsec_to_clock_t(idle),
			nsec_to_clock_t(iowait),
			nsec_to_clock_t(irq),
			nsec_to_clock_t(softirq),
			nsec_to_clock_t(steal),
			nsec_to_clock_t(guest),
			nsec_to_clock_t(guest_nice));

	for_each_online_cpu(i) {
		/* Copy values here to work around gcc-2.95.3, gcc-2.96 */
		user = kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice = kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system = kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle = kcpustat_cpu(i).cpustat[CPUTIME_IDLE];
		iowait = kcpustat_cpu(i).cpustat[CPUTIME_IOWAIT];
		irq = kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq = kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal = kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest = kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice = kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
		pr_info("cpu%-2d %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu %8llu\n",
				i,
				nsec_to_clock_t(user),
				nsec_to_clock_t(nice),
				nsec_to_clock_t(system),
				nsec_to_clock_t(idle),
				nsec_to_clock_t(iowait),
				nsec_to_clock_t(irq),
				nsec_to_clock_t(softirq),
				nsec_to_clock_t(steal),
				nsec_to_clock_t(guest),
				nsec_to_clock_t(guest_nice));
	}

#if 0
#if IS_BUILTIN(CONFIG_SEC_DEBUG)
	pr_info(" ---------------------------------------------------------------------------------------\n");
	pr_info("intr %llu\n", (unsigned long long)sum);

	/* sum again ? it could be updated? */
	for_each_irq_nr(j)
		if (kstat_irqs(j))
			pr_info(" irq-%d : %u\n", j, kstat_irqs(j));

	pr_info(" ---------------------------------------------------------------------------------------\n");
	pr_info("\nctxt %llu\n"
		"btime %llu\n"
		"processes %lu\n"
		"procs_running %lu\n"
		"procs_blocked %lu\n",
		nr_context_switches(),
		(unsigned long long)boottime.tv_sec,
		total_forks,
		nr_running(),
		nr_iowait());

	pr_info(" ---------------------------------------------------------------------------------------\n");
	pr_info("softirq %llu\n", (unsigned long long)sum_softirq);

	for (i = 0; i < NR_SOFTIRQS; i++)
		pr_info(" softirq-%d : %u\n", i, per_softirq_sums[i]);
#endif
#endif

	pr_info("\n");

	pr_info(" ---------------------------------------------------------------------------------------\n");
}

void sec_debug_show_stat(const char *msg)
{
	if (!__debug_is_platform_lockup_suspected(msg))
		return;

	__debug_show_task_uninterruptible();
	__debug_show_cpu_stat();
}
