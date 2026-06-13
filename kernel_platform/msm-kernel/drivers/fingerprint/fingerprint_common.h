#ifndef _FINGERPRINT_COMMON_H_
#define _FINGERPRINT_COMMON_H_

#include "fingerprint.h"
#include <linux/pm_qos.h>
#include <linux/delay.h>
#include <linux/pm_wakeup.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/version.h>

#define DEBUG_TIMER_SEC (10 * HZ)

struct boosting_config {
	unsigned int min_cpufreq_limit;
	struct pm_qos_request pm_qos;
};

struct spi_clk_setting {
	bool enabled_clk;
	u32 spi_speed;
	struct wakeup_source *spi_wake_lock;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	struct clk *fp_spi_pclk;
	struct clk *fp_spi_sclk;
#endif
};

struct debug_logger {
	struct work_struct work_debug;
	struct workqueue_struct *wq_dbg;
	struct timer_list dbg_timer;
	struct device *dev;
};

extern struct debug_logger *g_logger;

void spi_get_ctrldata(struct spi_device *spi);
void set_sensor_type(const int type_value, int *sensortype);
int spi_clk_register(struct spi_clk_setting *clk_setting, struct device *dev);
int spi_clk_unregister(struct spi_clk_setting *clk_setting);
int spi_clk_enable(struct spi_clk_setting *clk_setting);
int spi_clk_disable(struct spi_clk_setting *clk_setting);
int cpu_speedup_enable(struct boosting_config *boosting);
int cpu_speedup_disable(struct boosting_config *boosting);
void enable_fp_debug_timer(struct debug_logger *logger);
void disable_fp_debug_timer(struct debug_logger *logger);
int set_fp_debug_timer(struct debug_logger *logger,
			void (*func)(struct work_struct *work));
void set_delay_in_spi_transfer(struct spi_transfer *xfer, unsigned int usec);

#endif /* _FINGERPRINT_COMMON_H_ */
