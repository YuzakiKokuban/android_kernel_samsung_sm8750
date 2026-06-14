// SPDX-License-Identifier: GPL-2.0-only
/*
 * drivers/cpufreq/cpufreq_limit.c
 *
 * Remade according to cpufreq change
 * (refer to commit df0eea4488081e0698b0b58ccd1e8c8823e22841
 *                 18c49926c4bf4915e5194d1de3299c0537229f9f)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/cpufreq.h>
#include <linux/cpufreq_limit.h>
#include <linux/err.h>
#include <linux/suspend.h>
#include <linux/cpu.h>
#include <linux/kobject.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#endif
#include <trace/hooks/cpufreq.h>

static unsigned int __read_mostly lpcharge;
module_param(lpcharge, uint, 0444);

static DEFINE_MUTEX(cflm_mutex);
#define LIMIT_RELEASE	-1

/* boosted state */
#define BOOSTED	1
#define NOT_BOOSTED	0

#define NUM_CPUS	8
static unsigned int cflm_req_init[NUM_CPUS];
static struct freq_qos_request max_req[NUM_CPUS][CFLM_MAX_ITEM];
static struct freq_qos_request min_req[NUM_CPUS][CFLM_MAX_ITEM];
static struct kobject *cflm_kobj;

struct vfreq_table {
	unsigned int vfreq;
	unsigned int prime;
	unsigned int perf;
};

/* input info: freq, time(TBD) */
struct input_info {
	int boosted;
	int min;
	int max;
	u64 time_in_min_limit;
	u64 time_in_max_limit;
	u64 time_in_over_limit;
	ktime_t last_min_limit_time;
	ktime_t last_max_limit_time;
	ktime_t last_over_limit_time;
};
static struct input_info freq_input[CFLM_MAX_ITEM];

struct cflm_parameter {
	unsigned int table_size;

	struct vfreq_table *min_table;
	struct vfreq_table *max_table;
	struct vfreq_table *ab_low_table;

	/* cpu info: performance/prime core */
	unsigned int	perf_first;
	unsigned int	prime_first;

	/* TODO: not use for logic, just reference, remove?? */
	unsigned int	perf_fmin;
	unsigned int	perf_fmax;
	unsigned int	prime_fmin;
	unsigned int	prime_fmax;

	/* virtual table */
	unsigned int	virt_fmin;
	unsigned int	virt_fmax;

	/* current freq in virtual table */
	unsigned int	min_limit_val;
	unsigned int	max_limit_val;

	/* over limit */
	unsigned int	over_limit;

	/* adaptive boost */
	unsigned int	ab_enabled;

	/* adaptive boost level 1 */
	unsigned int	ab_l1_perf;
	unsigned int	ab_l1_prime;
};


/* TODO: move to dtsi? */
static struct cflm_parameter param = {
	.perf_first			= 0,
	.prime_first		= 6,

	.min_limit_val		= -1,
	.max_limit_val		= -1,

	.over_limit			= 0,

	.ab_enabled			= 0,

	.ab_l1_perf			= 0,
	.ab_l1_prime		= 0,
};

static int cflm_verify_table(void)
{
	int i = 0;

	param.virt_fmin = UINT_MAX;
	param.virt_fmax = 0;

	for (i = 0; i < param.table_size; i++) {
		if (param.min_table[i].vfreq != param.max_table[i].vfreq) {
			pr_err("%s: vfreq mismatch: min(%d), max(%d)\n",
				__func__, param.min_table[i].vfreq, param.max_table[i].vfreq);

			return -EINVAL;
		} else {
			if (param.min_table[i].vfreq < param.virt_fmin)
				param.virt_fmin = param.min_table[i].vfreq;

			if (param.min_table[i].vfreq > param.virt_fmax)
				param.virt_fmax = param.min_table[i].vfreq;
		}
	}

	pr_info("%s: vfreq fmax(%d), fmin(%d)\n", __func__, param.virt_fmax, param.virt_fmin);

	return 0;
}

/**
 * cflm_get_table - fill the cpufreq table to support HMP
 * @buf		a buf that has been requested to fill the cpufreq table
 */
static ssize_t cflm_get_table(char *buf)
{
	ssize_t len = 0;
	int i = 0;

	if (!param.table_size)
		return len;

	for (i = 0; i < param.table_size; i++)
		len += snprintf(buf + len, MAX_BUF_SIZE, "%u ",
					param.min_table[i].vfreq);

	len--;
	len += snprintf(buf + len, MAX_BUF_SIZE, "\n");

	pr_info("%s: %s\n", __func__, buf);

	return len;
}

static s32 cflm_freq_qos_read_value(struct freq_constraints *qos,
			enum freq_qos_req_type type)
{
	s32 ret;

	switch (type) {
	case FREQ_QOS_MIN:
		ret = IS_ERR_OR_NULL(qos) ?
			FREQ_QOS_MIN_DEFAULT_VALUE :
			READ_ONCE(qos->min_freq.target_value);
		break;
	case FREQ_QOS_MAX:
		ret = IS_ERR_OR_NULL(qos) ?
			FREQ_QOS_MAX_DEFAULT_VALUE :
			READ_ONCE(qos->max_freq.target_value);
		break;
	default:
		WARN_ON(1);
		ret = 0;
	}

	return ret;
}

static void cflm_current_qos(void)
{
	struct cpufreq_policy *policy;
	int perf_min = 0, perf_max = 0;
	int prime_min = 0, prime_max = 0;

	policy = cpufreq_cpu_get(param.perf_first);
	if (!policy) {
		pr_err("%s: no policy for cpu%d\n", __func__, param.perf_first);
		return;
	}
	perf_min = cflm_freq_qos_read_value(&policy->constraints, FREQ_QOS_MIN);
	perf_max = cflm_freq_qos_read_value(&policy->constraints, FREQ_QOS_MAX);
	cpufreq_cpu_put(policy);

	pr_cont("%s: perf[%d-%d]", __func__, perf_min, perf_max);

	policy = cpufreq_cpu_get(param.prime_first);
	if (!policy) {
		pr_err("%s: no policy for cpu%d\n", __func__, param.prime_first);
		return;
	}
	prime_min = cflm_freq_qos_read_value(&policy->constraints, FREQ_QOS_MIN);
	prime_max = cflm_freq_qos_read_value(&policy->constraints, FREQ_QOS_MAX);
	cpufreq_cpu_put(policy);

	pr_cont(", prime[%d-%d]", prime_min, prime_max);

	pr_cont("\n");
}

static bool cflm_max_lock_need_restore(void)
{
	if ((int)param.over_limit <= 0)
		return false;

	if (freq_input[CFLM_USERSPACE].min > 0) {
		if (freq_input[CFLM_USERSPACE].min > (int)param.virt_fmin) {
			pr_debug("%s: userspace minlock (%d) > virt fmin (%d)\n",
					__func__, freq_input[CFLM_USERSPACE].min, param.virt_fmin);
			return false;
		}
	}

	if (freq_input[CFLM_TOUCH].min > 0) {
		if (freq_input[CFLM_TOUCH].min > (int)param.virt_fmin) {
			pr_debug("%s: touch minlock (%d) > virt fmin (%d)\n",
					__func__, freq_input[CFLM_TOUCH].min, param.virt_fmin);
			return false;
		}
	}

	return true;
}

static bool cflm_high_pri_min_lock_required(void)
{
	if ((int)param.over_limit <= 0)
		return false;

	if (freq_input[CFLM_USERSPACE].min > 0) {
		if (freq_input[CFLM_USERSPACE].min > (int)param.virt_fmin) {
			pr_debug("%s: userspace minlock (%d) > virt fmin (%d)\n",
					__func__, freq_input[CFLM_USERSPACE].min, param.virt_fmin);
			return true;
		}
	}

	if (freq_input[CFLM_TOUCH].min > 0) {
		if (freq_input[CFLM_TOUCH].min > (int)param.virt_fmin) {
			pr_debug("%s: touch minlock (%d) > virt fmin (%d)\n",
					__func__, freq_input[CFLM_TOUCH].min, param.virt_fmin);
			return true;
		}
	}

	return false;
}

static int cflm_get_freq_index(int freq)
{
	int i;

	for (i = 0; i < param.table_size; i++)
		if (freq >= param.min_table[i].vfreq)
			return i;

	return -EINVAL;
}

static int cflm_adaptive_boost(int cpu, int boost_freq)
{
	struct cpufreq_policy *policy;
	int ret = 0;

	pr_debug("%s: cpu%d: %d\n", __func__, cpu, boost_freq);

	if (!param.ab_enabled)
		return -EINVAL;

	policy = cpufreq_cpu_get(cpu);
	if (!policy) {
		pr_err("%s: no policy for cpu%d\n", __func__, cpu);
		return -EFAULT;
	}

	if (!policy->governor || !policy->governor->name[0]) {
		pr_err("%s: no governor for cpu%d\n", __func__, cpu);
		return -EFAULT;
	}

	if (strcmp((policy->governor->name), "walt")) {
		pr_err("%s: not supported gov(%s)\n", __func__, policy->governor->name);
		return -EFAULT;
	}

	if (boost_freq > 0) {
		int idx = 0, ab_high = 0, ab_low = 0, ab_l1 = 0;

		idx = cflm_get_freq_index(boost_freq);

		if (cpu == param.perf_first) {
			ab_high = param.min_table[idx].perf;
			ab_low = param.ab_low_table[idx].perf;
			ab_l1 = param.ab_l1_perf;
		} else if (cpu == param.prime_first) {
			ab_high = param.min_table[idx].prime;
			ab_low = param.ab_low_table[idx].prime;
			ab_l1 = param.ab_l1_prime;
		}

		/* reset adaptive boost */
		if (ab_low == 0) {
			pr_debug("%s: not apply aboost: cpu%d\n", __func__, cpu);
			for_each_cpu(cpu, policy->related_cpus)
				if (cpu_online(cpu))
					cpufreq_walt_reset_adaptive_freq(cpu);

			cpufreq_cpu_put(policy);

			return -EINVAL;
		}

		for_each_cpu(cpu, policy->related_cpus) {
			if (cpu_online(cpu)) {
				pr_debug("%s: set aboost: cpu%d: %d, %d, %d\n", __func__, cpu, ab_l1, ab_low, ab_high);
				ret = cpufreq_walt_set_adaptive_freq(cpu, ab_l1, ab_low, ab_high);
				if (ret < 0) {
					pr_err("%s: fail to set adaptive freq\n", __func__);
					goto out;
				}
			}
		}
	} else {
		for_each_cpu(cpu, policy->related_cpus) {
			if (cpu_online(cpu)) {
				pr_debug("%s: clear aboost: cpu%d\n", __func__, cpu);
				ret = cpufreq_walt_reset_adaptive_freq(cpu);
				if (ret < 0) {
					pr_err("%s: fail to reset adaptive freq\n", __func__);
					goto out;
				}
			}
		}
	}

	cpufreq_cpu_put(policy);

out:
	return ret;
}

#if IS_ENABLED(CONFIG_SCHED_FLEX_BOOT)
#include <asm/timex.h>

static int __read_mostly max_freqs_perf;
static int __read_mostly max_freqs_prime;
static int __read_mostly max_freqs_time;

static int __read_mostly min_freqs_perf;
static int __read_mostly min_freqs_prime;
static int __read_mostly min_freqs_time;

module_param(max_freqs_perf, int, 0444);
module_param(max_freqs_prime, int, 0444);
module_param(max_freqs_time, int, 0444);

module_param(min_freqs_perf, int, 0444);
module_param(min_freqs_prime, int, 0444);
module_param(min_freqs_time, int, 0444);

static int flexboot = 0;
static void cpufreq_sec_limit_max(int type)
{
	flexboot = 0;

	pr_info("%s: max_freqs_perf(%u), max_freqs_prime(%u), max_freqs_time(%u) \n",
			__func__, max_freqs_perf, max_freqs_prime, max_freqs_time);

	if (get_cycles() < 19200000UL * max_freqs_time) {
		flexboot = 1;
		freq_qos_update_request(&max_req[param.perf_first][type], max_freqs_perf);
		pr_info("performance max limit to %u kHz\n", max_freqs_perf);

		freq_qos_update_request(&max_req[param.prime_first][type], max_freqs_prime);
		pr_info("prime max limit to %u kHz\n", max_freqs_prime);
	}
}

static void cpufreq_sec_limit_min(int type)
{
	flexboot = 0;

	pr_info("%s: min_freqs_perf(%u), min_freqs_prime(%u), min_freqs_time(%u) \n",
			__func__, min_freqs_perf, min_freqs_prime, min_freqs_time);

	if (get_cycles() < 19200000UL * min_freqs_time) {
		flexboot = 1;
		freq_qos_update_request(&min_req[param.perf_first][type], min_freqs_perf);
		pr_info("performance min limit to %u kHz\n", min_freqs_perf);

		freq_qos_update_request(&min_req[param.prime_first][type], min_freqs_prime);
		pr_info("prime min limit to %u kHz\n", min_freqs_prime);
	}
}
#endif

static void cflm_freq_decision(int type, int new_min, int new_max)
{
	int cpu = 0, idx = 0;
	bool need_update_user_max = false;
	int new_user_max = FREQ_QOS_MAX_DEFAULT_VALUE;
	int ret = 0;

#if IS_ENABLED(CONFIG_SCHED_FLEX_BOOT)
	cpufreq_sec_limit_max(type);
	cpufreq_sec_limit_min(type);

	if(flexboot)
		return;
#endif

	pr_info("%s: input: type(%d), min(%d), max(%d)\n",
			__func__, type, new_min, new_max);

	/* update input freq */
	if (new_min != 0) {
		freq_input[type].min = new_min;
		if (new_min == LIMIT_RELEASE && freq_input[type].last_min_limit_time != 0) {
			freq_input[type].time_in_min_limit += ktime_to_ms(ktime_get()-
				freq_input[type].last_min_limit_time);
			freq_input[type].last_min_limit_time = 0;
			freq_input[type].boosted = NOT_BOOSTED;
			pr_debug("%s: type(%d), released(%d)\n", __func__, type, freq_input[type].boosted);
		}
		if (new_min != LIMIT_RELEASE && freq_input[type].last_min_limit_time == 0) {
			freq_input[type].last_min_limit_time = ktime_get();
			freq_input[type].boosted = BOOSTED;
			pr_debug("%s: type(%d), boosted(%d)\n", __func__, type, freq_input[type].boosted);
		}
	}

	if (new_max != 0) {
		freq_input[type].max = new_max;
		if (new_max == LIMIT_RELEASE && freq_input[type].last_max_limit_time != 0) {
			freq_input[type].time_in_max_limit += ktime_to_ms(ktime_get() -
				freq_input[type].last_max_limit_time);
			freq_input[type].last_max_limit_time = 0;
		}
		if (new_max != LIMIT_RELEASE && freq_input[type].last_max_limit_time == 0) {
			freq_input[type].last_max_limit_time = ktime_get();
		}
	}

	if (new_min > 0) {
		if (new_min < param.virt_fmin) {
			pr_err("%s: too low freq(%d), set to %d\n",
				__func__, new_min, param.virt_fmin);
			new_min = param.virt_fmin;
		}

		pr_debug("%s: new_min=%d, virt_fmin=%d, over_limit=%d\n", __func__,
				new_min, param.virt_fmin, param.over_limit);
		if ((type == CFLM_USERSPACE || type == CFLM_TOUCH) &&
			cflm_high_pri_min_lock_required()) {
			if (freq_input[CFLM_USERSPACE].max > 0) {
				need_update_user_max = true;
				new_user_max = MAX((int)param.over_limit, freq_input[CFLM_USERSPACE].max);
				pr_debug("%s: override new_max %d => %d,  userspace_min=%d, touch_min=%d, virt_fmin=%d\n",
						__func__, freq_input[CFLM_USERSPACE].max, new_user_max, freq_input[CFLM_USERSPACE].min,
						freq_input[CFLM_TOUCH].min, param.virt_fmin);
			}
		}

		idx = cflm_get_freq_index(new_min);

		ret = cflm_adaptive_boost(param.perf_first, new_min);
		if (ret)
			freq_qos_update_request(&min_req[param.perf_first][type], param.min_table[idx].perf);

		ret = cflm_adaptive_boost(param.prime_first, new_min);
		if (ret)
			freq_qos_update_request(&min_req[param.prime_first][type], param.min_table[idx].prime);
	} else if (new_min == LIMIT_RELEASE) {
		for_each_possible_cpu(cpu) {
			freq_qos_update_request(&min_req[cpu][type],
						FREQ_QOS_MIN_DEFAULT_VALUE);
		}

		if (param.ab_enabled) {
			int i;
			int aggr_state = 0;

			for (i = 0; i < CFLM_MAX_ITEM; i++)
				aggr_state += freq_input[i].boosted;

			if (aggr_state == 0) {
				cflm_adaptive_boost(param.perf_first, 0);
				cflm_adaptive_boost(param.prime_first, 0);
				pr_debug("%s: aboost: clear\n", __func__);
			}
		}

		if ((type == CFLM_USERSPACE || type == CFLM_TOUCH) &&
			cflm_max_lock_need_restore()) { // if there is no high priority min lock and over limit is set
			if (freq_input[CFLM_USERSPACE].max > 0) {
				need_update_user_max = true;
				new_user_max = freq_input[CFLM_USERSPACE].max;
				pr_debug("%s: restore new_max => %d\n",
						__func__, new_user_max);
			}
		}
	}

	if (new_max > 0) {
		if (new_max > param.virt_fmax) {
			pr_err("%s: too high freq(%d), set to %d\n",
				__func__, new_max, param.virt_fmax);
			new_max = param.virt_fmax;
		}

		if ((type == CFLM_USERSPACE) && // if userspace maxlock is being set
			cflm_high_pri_min_lock_required()) {
			need_update_user_max = true;
			new_user_max = MAX((int)param.over_limit, freq_input[CFLM_USERSPACE].max);
			pr_debug("%s: force up new_max %d => %d, userspace_min=%d, touch_min=%d, virt_fmin=%d\n",
					__func__, new_max, new_user_max, freq_input[CFLM_USERSPACE].min,
					freq_input[CFLM_TOUCH].min, param.virt_fmin);
		}

		idx = cflm_get_freq_index(new_max);

		freq_qos_update_request(&max_req[param.perf_first][type], param.max_table[idx].perf);
		freq_qos_update_request(&max_req[param.prime_first][type], param.max_table[idx].prime);
	} else if (new_max == LIMIT_RELEASE) {
		for_each_possible_cpu(cpu)
			freq_qos_update_request(&max_req[cpu][type],
						FREQ_QOS_MAX_DEFAULT_VALUE);
	}

	if ((freq_input[type].min <= (int)param.virt_fmin || new_user_max != (int)param.over_limit) &&
		freq_input[type].last_over_limit_time != 0) {
		freq_input[type].time_in_over_limit += ktime_to_ms(ktime_get() -
			freq_input[type].last_over_limit_time);
		freq_input[type].last_over_limit_time = 0;
	}
	if (freq_input[type].min > (int)param.virt_fmin && new_user_max == (int)param.over_limit &&
		freq_input[type].last_over_limit_time == 0) {
		freq_input[type].last_over_limit_time = ktime_get();
	}

	if (need_update_user_max) {
		pr_debug("%s: update_user_max is true\n", __func__);
		if (new_user_max > param.virt_fmax) {
			pr_debug("%s: too high freq(%d), set to %d\n",
			__func__, new_user_max, param.virt_fmax);
			new_user_max = param.virt_fmax;
		}

		pr_info("%s: freq_update_request : new userspace max new_user_max %d\n", __func__, new_user_max);

		idx = cflm_get_freq_index(new_user_max);
		freq_qos_update_request(&max_req[param.perf_first][CFLM_USERSPACE], param.max_table[idx].perf);
		freq_qos_update_request(&max_req[param.prime_first][CFLM_USERSPACE], param.max_table[idx].prime);
	}

	cflm_current_qos();
}

static ssize_t cpufreq_table_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	ssize_t len = 0;

	len = cflm_get_table(buf);

	return len;
}

static ssize_t cpufreq_max_limit_show(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{
	return snprintf(buf, MAX_BUF_SIZE, "%d\n", param.max_limit_val);
}

static ssize_t cpufreq_max_limit_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	int freq;
	int ret = -EINVAL;

	ret = kstrtoint(buf, 10, &freq);
	if (ret < 0) {
		pr_err("%s: cflm: Invalid cpufreq format\n", __func__);
		goto out;
	}

	mutex_lock(&cflm_mutex);

	param.max_limit_val = freq;
	cflm_freq_decision(CFLM_USERSPACE, 0, freq);

	mutex_unlock(&cflm_mutex);
	ret = n;

out:
	return ret;
}

static ssize_t cpufreq_min_limit_show(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{
	return snprintf(buf, MAX_BUF_SIZE, "%d\n", param.min_limit_val);
}

static ssize_t cpufreq_min_limit_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	int freq;
	int ret = -EINVAL;

	ret = kstrtoint(buf, 10, &freq);
	if (ret < 0) {
		pr_err("%s: cflm: Invalid cpufreq format\n", __func__);
		goto out;
	}

	mutex_lock(&cflm_mutex);

	cflm_freq_decision(CFLM_USERSPACE, freq, 0);
	param.min_limit_val = freq;

	mutex_unlock(&cflm_mutex);
	ret = n;
out:
	return ret;
}

static ssize_t over_limit_show(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{
	return snprintf(buf, MAX_BUF_SIZE, "%d\n", param.over_limit);
}

static ssize_t over_limit_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	int freq;
	int ret = -EINVAL;

	ret = kstrtoint(buf, 10, &freq);
	if (ret < 0) {
		pr_err("%s: cflm: Invalid cpufreq format\n", __func__);
		goto out;
	}

	mutex_lock(&cflm_mutex);

	if (param.over_limit != freq) {
		param.over_limit = freq;
		if ((int)param.max_limit_val > 0)
			cflm_freq_decision(CFLM_USERSPACE, 0, param.max_limit_val);
	}

	mutex_unlock(&cflm_mutex);
	ret = n;
out:
	return ret;
}

static ssize_t limit_stat_show(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{

	ssize_t len = 0;
	int i, j = 0;

	mutex_lock(&cflm_mutex);
	for (i = 0; i < CFLM_MAX_ITEM; i++) {
		if (freq_input[i].last_min_limit_time != 0) {
			freq_input[i].time_in_min_limit += ktime_to_ms(ktime_get() -
				freq_input[i].last_min_limit_time);
			freq_input[i].last_min_limit_time = ktime_get();
		}

		if (freq_input[i].last_max_limit_time != 0) {
			freq_input[i].time_in_max_limit += ktime_to_ms(ktime_get() -
				freq_input[i].last_max_limit_time);
			freq_input[i].last_max_limit_time = ktime_get();
		}

		if (freq_input[i].last_over_limit_time != 0) {
			freq_input[i].time_in_over_limit += ktime_to_ms(ktime_get() -
				freq_input[i].last_over_limit_time);
			freq_input[i].last_over_limit_time = ktime_get();
		}
	}

	for (j = 0; j < CFLM_MAX_ITEM; j++) {
		len += snprintf(buf + len, MAX_BUF_SIZE - len, "%llu %llu %llu\n",
				freq_input[j].time_in_min_limit, freq_input[j].time_in_max_limit,
				freq_input[j].time_in_over_limit);
	}

	mutex_unlock(&cflm_mutex);
	return len;
}

static ssize_t vtable_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int i = 0;

	len += snprintf(buf + len, MAX_BUF_SIZE, "================min===============max=======\n");
	len += snprintf(buf + len, MAX_BUF_SIZE, "  virt   |  prime   perf   |  prime   perf\n");
	for (i = 0; i < param.table_size; i++) {
		len += snprintf(buf + len, MAX_BUF_SIZE, " %7u | %7u %7u | %7u %7u\n",
			param.min_table[i].vfreq,

			param.min_table[i].prime,
			param.min_table[i].perf,

			param.max_table[i].prime,
			param.max_table[i].perf);
	}
	len += snprintf(buf + len, MAX_BUF_SIZE, "============================================\n");

	pr_info("%s: %s\n", __func__, buf);

	return len;
}

/* sysfs in /sys/power */
static struct kobj_attribute cpufreq_table = {
	.attr	= {
		.name = "cpufreq_table",
		.mode = 0444
	},
	.show	= cpufreq_table_show,
	.store	= NULL,
};

static struct kobj_attribute cpufreq_min_limit = {
	.attr	= {
		.name = "cpufreq_min_limit",
		.mode = 0644
	},
	.show	= cpufreq_min_limit_show,
	.store	= cpufreq_min_limit_store,
};

static struct kobj_attribute cpufreq_max_limit = {
	.attr	= {
		.name = "cpufreq_max_limit",
		.mode = 0644
	},
	.show	= cpufreq_max_limit_show,
	.store	= cpufreq_max_limit_store,
};

static struct kobj_attribute over_limit = {
	.attr	= {
		.name = "over_limit",
		.mode = 0644
	},
	.show	= over_limit_show,
	.store	= over_limit_store,
};

static struct kobj_attribute limit_stat = {
	.attr	= {
		.name = "limit_stat",
		.mode = 0644
	},
	.show	= limit_stat_show,
};

static struct kobj_attribute vtable = {
	.attr	= {
		.name = "vtable",
		.mode = 0444
	},
	.show	= vtable_show,
	.store	= NULL,
};

int set_freq_limit(unsigned int id, unsigned int freq)
{
	if (lpcharge) {
		pr_err("%s: not allowed in LPM\n", __func__);
		return 0;
	}

	mutex_lock(&cflm_mutex);

	pr_info("%s: cflm: id(%d) freq(%d)\n", __func__, (int)id, freq);

	cflm_freq_decision(id, freq, 0);

	mutex_unlock(&cflm_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(set_freq_limit);

#define cflm_attr_rw(_name)		\
static struct kobj_attribute _name##_attr =	\
__ATTR(_name, 0644, show_##_name, store_##_name)

#define show_one(file_name)			\
static ssize_t show_##file_name			\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return scnprintf(buf, PAGE_SIZE, "%u\n", param.file_name);	\
}

#define store_one(file_name)					\
static ssize_t store_##file_name				\
(struct kobject *kobj, struct kobj_attribute *attr,		\
const char *buf, size_t count)					\
{								\
	int ret;						\
								\
	ret = sscanf(buf, "%u", &param.file_name);				\
	if (ret != 1)						\
		return -EINVAL;					\
								\
	return count;						\
}

/* adaptive boost */
show_one(ab_enabled);
store_one(ab_enabled);
cflm_attr_rw(ab_enabled);

static ssize_t show_cflm_info(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int i = 0;

	mutex_lock(&cflm_mutex);

	len += snprintf(buf, MAX_BUF_SIZE, "[basic info]\n");
	len += snprintf(buf + len, MAX_BUF_SIZE - len,
			"perf(%d ~ %d), prime(%d ~ %d), virt(%d ~ %d)\n",
			param.perf_fmin, param.perf_fmax,
			param.prime_fmin, param.prime_fmax,
			param.virt_fmin, param.virt_fmax);

	len += snprintf(buf + len, MAX_BUF_SIZE - len,
			"aboost(%d), l1_prime(%d), l1_perf(%d)\n\n",
			param.ab_enabled, param.ab_l1_prime, param.ab_l1_perf);

	len += snprintf(buf + len, MAX_BUF_SIZE - len, "[requested info]\n");
	for (i = 0; i < CFLM_MAX_ITEM; i++) {
		len += snprintf(buf + len, MAX_BUF_SIZE - len,
				"[%d] min(%d), max(%d)\n",
				i, freq_input[i].min, freq_input[i].max);
	}

	mutex_unlock(&cflm_mutex);

	return len;
}

static struct kobj_attribute cflm_info =
	__ATTR(info, 0444, show_cflm_info, NULL);

static struct attribute *cflm_attributes[] = {
	&cpufreq_table.attr,
	&cpufreq_min_limit.attr,
	&cpufreq_max_limit.attr,
	&over_limit.attr,
	&limit_stat.attr,
	&cflm_info.attr,
	&vtable.attr,
	&ab_enabled_attr.attr,
	NULL,
};

static struct attribute_group cflm_attr_group = {
	.attrs = cflm_attributes,
};

#ifdef CONFIG_OF
static void cflm_parse_dt(struct platform_device *pdev)
{
	int size = 0;
	int min_tbl_sz = 0, max_tbl_sz = 0, ab_tbl_sz = 0;

	if (!pdev->dev.of_node) {
		pr_info("%s: no device tree\n", __func__);
		return;
	}

	/* cpu number */
	of_property_read_u32(pdev->dev.of_node, "limit,perf_first", &param.perf_first);
	of_property_read_u32(pdev->dev.of_node, "limit,prime_first", &param.prime_first);
	pr_info("%s: param: cluster0(%d), cluster1(%d)\n, ",
					__func__, param.perf_first, param.prime_first);

	/* min table */
	size = 0;
	of_get_property(pdev->dev.of_node, "limit,min_table", &size);
	if (size) {
		param.min_table = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
		of_property_read_u32_array(pdev->dev.of_node, "limit,min_table",
				(u32 *)param.min_table, size / sizeof(u32));

		min_tbl_sz = size / sizeof(*param.min_table);
		pr_info("%s: param: min table size(%d)\n", __func__, min_tbl_sz);
	} else {
		pr_info("%s: param: no min table\n", __func__);
	}

	/* max table */
	size = 0;
	of_get_property(pdev->dev.of_node, "limit,max_table", &size);
	if (size) {
		param.max_table = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
		of_property_read_u32_array(pdev->dev.of_node, "limit,max_table",
				(u32 *)param.max_table, size / sizeof(u32));

		max_tbl_sz = size / sizeof(*param.max_table);
		pr_info("%s: param: max table size(%d)\n", __func__, max_tbl_sz);
	} else {
		pr_info("%s: param: no max table\n", __func__);
	}

	if (min_tbl_sz == max_tbl_sz) {
		param.table_size = min_tbl_sz;
		pr_info("%s: param: table size updated(%d)\n", __func__, param.table_size);
	} else {
		pr_info("%s: param: wrong table size(%d, %d)\n", __func__, min_tbl_sz, max_tbl_sz);
		param.table_size = 0;
	}

	/* adaptive low table */
	size = 0;
	of_get_property(pdev->dev.of_node, "limit,ab_low_table", &size);
	if (size) {
		param.ab_low_table = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
		of_property_read_u32_array(pdev->dev.of_node, "limit,ab_low_table",
				(u32 *)param.ab_low_table, size / sizeof(u32));

		ab_tbl_sz = size / sizeof(*param.ab_low_table);
		pr_info("%s: param: ab low table size(%d)\n", __func__, ab_tbl_sz);

		if (param.table_size == ab_tbl_sz) {
			param.ab_enabled = 1;
			pr_info("%s: param: adaptive boost is enabled\n", __func__);
		}
	} else {
		pr_info("%s: param: no ab low table\n", __func__);
	}

	/* adaptive l1 freq */
	of_property_read_u32(pdev->dev.of_node, "limit,ab_l1_perf", &param.ab_l1_perf);
	of_property_read_u32(pdev->dev.of_node, "limit,ab_l1_prime", &param.ab_l1_prime);
	pr_info("%s: param: ab_l1_perf(%d), ab_l1_prime(%d)\n, ",
					__func__, param.ab_l1_perf, param.ab_l1_prime);

	of_node_put(pdev->dev.of_node);
};
#endif

int cflm_add_qos(void)
{
	struct cpufreq_policy *policy;
	unsigned int i = 0;
	unsigned int j = 0;
	int ret = 0;

	for_each_possible_cpu(i) {
		policy = cpufreq_cpu_get(i);
		if (!policy) {
			pr_err("no policy for cpu%d\n", i);
			ret = -EPROBE_DEFER;
			break;
		}

		for (j = 0; j < CFLM_MAX_ITEM; j++) {
			ret = freq_qos_add_request(&policy->constraints,
					&min_req[i][j], FREQ_QOS_MIN, policy->cpuinfo.min_freq);
			if (ret < 0) {
				pr_err("%s: failed to add min req(%d)\n", __func__, ret);
				break;
			}
			cflm_req_init[i] |= BIT(j * 2);

			ret = freq_qos_add_request(&policy->constraints,
					&max_req[i][j], FREQ_QOS_MAX, policy->cpuinfo.max_freq);
			if (ret < 0) {
				pr_err("%s: failed to add max req(%d)\n", __func__, ret);
				break;
			}
			cflm_req_init[i] |= BIT(j * 2 + 1);
		}

		/* TODO: not use for logic, just reference, remove?? */
		if (i == param.perf_first) {
			if (!param.perf_fmin)
				param.perf_fmin = policy->cpuinfo.min_freq;
			if (!param.perf_fmax)
				param.perf_fmax = policy->cpuinfo.max_freq;
		}

		if (i == param.prime_first) {
			if (!param.prime_fmin)
				param.prime_fmin = policy->cpuinfo.min_freq;
			if (!param.prime_fmax)
				param.prime_fmax = policy->cpuinfo.max_freq;
		}

		cpufreq_cpu_put(policy);
	}

	return ret;
}

void cflm_remove_qos(void)
{
	unsigned int i = 0;
	unsigned int j = 0;
	int ret = 0;

	pr_info("%s\n", __func__);
	for_each_possible_cpu(i) {
		for (j = 0; j < CFLM_MAX_ITEM; j++) {
			if (cflm_req_init[i] & BIT(j * 2)) {
				pr_info("%s: try to remove min[%d][%d] req\n", __func__, i, j);
				ret = freq_qos_remove_request(&min_req[i][j]);
				if (ret < 0)
					pr_err("%s: failed to remove min_req (%d)\n", __func__, ret);
			}

			if (cflm_req_init[i] & BIT(j * 2 + 1)) {
				pr_info("%s: try to remove max[%d][%d] req\n", __func__, i, j);
				ret = freq_qos_remove_request(&max_req[i][j]);
				if (ret < 0)
					pr_err("%s: failed to remove max_req (%d)\n", __func__, ret);
			}
		}
		cflm_req_init[i] = 0U;
	}
}

int cflm_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev_root;

	pr_info("%s\n", __func__);

	if (lpcharge) {
		pr_info("%s: dummy for LPM\n", __func__);

		return 0;
	}

#ifdef CONFIG_OF
	cflm_parse_dt(pdev);
#endif

	/* verify vfreq table */
	ret = cflm_verify_table();
	if (ret < 0)
		goto policy_not_ready;

	ret = cflm_add_qos();
	if (ret < 0)
		goto policy_not_ready;

	dev_root = bus_get_dev_root(&cpu_subsys);
	if (dev_root) {
		cflm_kobj = kobject_create_and_add("cpufreq_limit", &dev_root->kobj);
		put_device(dev_root);
	}

	if (!cflm_kobj) {
		pr_err("Unable to cread cflm_kobj\n");
		goto object_create_failed;
	}

	ret = sysfs_create_group(cflm_kobj, &cflm_attr_group);
	if (ret) {
		pr_err("Unable to create cflm group\n");
		goto group_create_failed;
	}

	pr_info("%s done\n", __func__);
	return ret;

group_create_failed:
	kobject_put(cflm_kobj);
object_create_failed:
	cflm_kobj = NULL;
policy_not_ready:
	cflm_remove_qos();

	return ret;
}

static int cflm_remove(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);

	if (!lpcharge && cflm_kobj) {
		cflm_remove_qos();
		sysfs_remove_group(cflm_kobj, &cflm_attr_group);
		kobject_put(cflm_kobj);
		cflm_kobj = NULL;
	}

	return 0;
}

static const struct of_device_id cflm_match_table[] = {
	{ .compatible = "cpufreq_limit" },
	{}
};

static struct platform_driver cflm_driver = {
	.driver = {
		.name = "cpufreq_limit",
		.of_match_table = cflm_match_table,
	},
	.probe = cflm_probe,
	.remove = cflm_remove,
};

static int __init cflm_init(void)
{
	return platform_driver_register(&cflm_driver);
}

static void __exit cflm_exit(void)
{
	platform_driver_unregister(&cflm_driver);
}

MODULE_AUTHOR("Sangyoung Son <hello.son@samsung.com");
MODULE_DESCRIPTION("'cpufreq_limit' - A driver to limit cpu frequency");
MODULE_LICENSE("GPL");

late_initcall(cflm_init);
module_exit(cflm_exit);
