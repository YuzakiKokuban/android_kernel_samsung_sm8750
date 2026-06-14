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

#ifndef __LINUX_CPUFREQ_LIMIT_H__
#define __LINUX_CPUFREQ_LIMIT_H__

#define MAX_BUF_SIZE	1024
#define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#define MAX(a, b)     (((a) > (b)) ? (a) : (b))

/* adaptive boost from walt */
extern int cpufreq_walt_set_adaptive_freq(unsigned int cpu,
				unsigned int adaptive_level_1,
				unsigned int adaptive_low_freq,
				unsigned int adaptive_high_freq);
extern int cpufreq_walt_get_adaptive_freq(unsigned int cpu,
				unsigned int *adaptive_level_1,
				unsigned int *adaptive_low_freq,
				unsigned int *adaptive_high_freq);
extern int cpufreq_walt_reset_adaptive_freq(unsigned int cpu);

enum {
	CFLM_USERSPACE		= 0,	/* user(/sys/power/cpufreq*limit) */
	CFLM_TOUCH		= 1,	/* touch */
	CFLM_FINGER		= 2,	/* fingerprint */
	CFLM_ARGOS		= 3,	/* argos */

	CFLM_MAX_ITEM
};

#endif /* __LINUX_CPUFREQ_LIMIT_H__ */
