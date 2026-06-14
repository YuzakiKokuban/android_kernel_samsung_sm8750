#include "fingerprint_common.h"
#include <linux/version.h>
#if defined(CONFIG_CPU_FREQ_LIMIT) || defined(CONFIG_CPU_FREQ_LIMIT_USERSPACE)
#if (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)
#include <linux/cpufreq.h>
#else
#include <linux/cpufreq_limit.h>
extern int set_freq_limit(unsigned int id, unsigned int freq);
#endif
#endif

#define FINGER_ID 2
#define LIMIT_RELEASE -1


void spi_get_ctrldata(struct spi_device *spi)
{

}

int spi_clk_register(struct spi_clk_setting *clk_setting, struct device *dev)
{
	return 0;
}

int spi_clk_unregister(struct spi_clk_setting *clk_setting)
{
	return 0;
}

int spi_clk_enable(struct spi_clk_setting *clk_setting)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	if (!clk_setting->enabled_clk) {
		__pm_stay_awake(clk_setting->spi_wake_lock);
		clk_setting->enabled_clk = true;
	}
#endif

	return 0;
}

int spi_clk_disable(struct spi_clk_setting *clk_setting)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	if (clk_setting->enabled_clk) {
		__pm_relax(clk_setting->spi_wake_lock);
		clk_setting->enabled_clk = false;
	}
#endif

	return 0;
}

int cpu_speedup_enable(struct boosting_config *boosting)
{
	int retval = 0;
#if defined(CONFIG_CPU_FREQ_LIMIT) || defined(CONFIG_CPU_FREQ_LIMIT_USERSPACE)
	if (boosting->min_cpufreq_limit) {
		pr_debug("%s\n", __func__);
		pm_qos_add_request(&boosting->pm_qos, PM_QOS_CPU_DMA_LATENCY, 0);
		retval = set_freq_limit(FINGER_ID, boosting->min_cpufreq_limit);
		if (retval)
			pr_err("booster start failed. (%d)\n", retval);
	}
#else
	pr_debug("FP_CPU_SPEEDUP does not supported\n");
#endif

	return retval;
}

int cpu_speedup_disable(struct boosting_config *boosting)
{
	int retval = 0;
#if defined(CONFIG_CPU_FREQ_LIMIT) || defined(CONFIG_CPU_FREQ_LIMIT_USERSPACE)
	if (boosting->min_cpufreq_limit) {
		pr_debug("%s\n", __func__);
		retval = set_freq_limit(FINGER_ID, LIMIT_RELEASE);
		if (retval)
			pr_err("booster stop failed. (%d)\n", retval);
		pm_qos_remove_request(&boosting->pm_qos);
	}
#else
	pr_debug("FP_CPU_SPEEDUP does not supported\n");
#endif

	return retval;
}
