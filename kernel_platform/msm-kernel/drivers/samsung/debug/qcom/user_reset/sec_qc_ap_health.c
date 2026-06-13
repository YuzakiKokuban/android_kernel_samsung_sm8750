// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#include <linux/samsung/debug/qcom/sec_qc_summary.h>

#include "sec_qc_user_reset.h"

ap_health_t *__qc_ap_health_data_read(struct qc_user_reset_drvdata *drvdata)
{
	return drvdata->health;
}

ap_health_t *sec_qc_ap_health_data_read(void)
{
	if (!__qc_user_reset_is_probed())
		return ERR_PTR(-EBUSY);

	return __qc_ap_health_data_read(qc_user_reset);
}
EXPORT_SYMBOL_GPL(sec_qc_ap_health_data_read);

int __qc_ap_health_data_write(struct qc_user_reset_drvdata *drvdata,
		ap_health_t *data)
{
	ap_health_t *health = drvdata->health;
	bool valid;

	if (health != data)
		memcpy(health, data, sizeof(*health));

	valid = sec_qc_dbg_part_write(debug_index_ap_health, health);
	if (!valid)
		return -ENXIO;

	return 0;
}

int sec_qc_ap_health_data_write(ap_health_t *data)
{
	if (!__qc_user_reset_is_probed())
		return -EBUSY;

	return __qc_ap_health_data_write(qc_user_reset, data);
}
EXPORT_SYMBOL_GPL(sec_qc_ap_health_data_write);

static void sec_qc_ap_health_data_write_fn(struct work_struct *work)
{
	struct delayed_work *health_work = to_delayed_work(work);
	struct qc_user_reset_drvdata *drvdata = container_of(health_work,
			struct qc_user_reset_drvdata, health_work);
	struct device *dev = drvdata->bd.dev;
	int err;

	atomic_set(&drvdata->health_work_offline, 1);

	err = __qc_ap_health_data_write(drvdata, drvdata->health);
	if (err)
		dev_warn(dev, "failed to write ap_health data (%d)\n", err);
}

void __qc_ap_health_data_write_delayed(struct qc_user_reset_drvdata *drvdata,
		ap_health_t *data)
{
	if (atomic_dec_if_positive(&drvdata->health_work_offline) < 0)
		return;	/* already queued */

	queue_delayed_work(system_unbound_wq, &drvdata->health_work, 0);
}

int sec_qc_ap_health_data_write_delayed(ap_health_t *data)
{
	if (!__qc_user_reset_is_probed())
		return -EBUSY;

	__qc_ap_health_data_write_delayed(qc_user_reset, data);

	return 0;
}
EXPORT_SYMBOL_GPL(sec_qc_ap_health_data_write_delayed);

static inline void __init_lcd_debug_data(void)
{
	struct lcd_debug_t lcd_debug;

	memset(&lcd_debug, 0, sizeof(struct lcd_debug_t));

	sec_qc_dbg_part_write(debug_index_lcd_debug_info, &lcd_debug);
}

static inline void __init_ap_health_data(ap_health_t *health)
{
	memset(health, 0, sizeof(ap_health_t));

	health->header.magic = AP_HEALTH_MAGIC;
	health->header.version = AP_HEALTH_VER;
	health->header.size = sizeof(ap_health_t);
	health->spare_magic1 = AP_HEALTH_MAGIC;
	health->spare_magic2 = AP_HEALTH_MAGIC;
	health->spare_magic3 = AP_HEALTH_MAGIC;

	sec_qc_dbg_part_write(debug_index_ap_health, health);
}

static bool __ap_health_is_initialized(ap_health_t *health)
{
	if (health->header.magic != AP_HEALTH_MAGIC ||
	    health->header.version != AP_HEALTH_VER ||
	    health->header.size != sizeof(ap_health_t) ||
	    health->spare_magic1 != AP_HEALTH_MAGIC ||
	    health->spare_magic2 != AP_HEALTH_MAGIC ||
	    health->spare_magic3 != AP_HEALTH_MAGIC)
		return false;

	return true;
}

static unsigned int is_boot_recovery __ro_after_init;
module_param_named(boot_recovery, is_boot_recovery, uint, 0444);

int __qc_ap_health_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct device *dev = drvdata->bd.dev;
	ap_health_t *health;
	bool valid;

	health = devm_kzalloc(dev, sizeof(ap_health_t), GFP_KERNEL);
	if (!health)
		return -ENOMEM;

	valid = sec_qc_dbg_part_read(debug_index_ap_health, health);
	if (!valid)
		return -ENXIO;

	atomic_set(&drvdata->health_work_offline, 1);
	INIT_DELAYED_WORK(&drvdata->health_work,
			sec_qc_ap_health_data_write_fn);

	if (!__ap_health_is_initialized(health) || is_boot_recovery) {
		__init_ap_health_data(health);
		__init_lcd_debug_data();
	}

	drvdata->health = health;

	return 0;
}

void __qc_ap_health_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);

	cancel_delayed_work_sync(&drvdata->health_work);
}
