// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>

#include "sec_qc_user_reset.h"

static int sec_qc_kryo_arm64_edac_handler(struct notifier_block *this,
		unsigned long l, void *__ctx)
{
	struct qc_user_reset_drvdata *drvdata = container_of(this,
			struct qc_user_reset_drvdata, nb_kryo_arm64_edac);
	ap_health_t *health = drvdata->health;

	sec_qc_kryo_arm64_edac_update(health,
			(struct kryo_arm64_edac_err_ctx *)__ctx);

	__qc_ap_health_data_write_delayed(drvdata, health);

	return NOTIFY_OK;
}

static int ____qc_kryo_arm64_edac_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata = container_of(bd,
			struct qc_user_reset_drvdata, bd);
	struct notifier_block *nb = &drvdata->nb_kryo_arm64_edac;
	ap_health_t *health = drvdata->health;
	ap_health_t *health_early;

	nb->notifier_call = sec_qc_kryo_arm64_edac_handler;
	health_early = qcom_kryo_arm64_edac_error_register_notifier(nb);
	if (IS_ERR_OR_NULL(health_early))
		return PTR_ERR(health_early);

	memcpy(&health->cache, &health_early->cache, sizeof(health->cache));
	memcpy(&health->daily_cache, &health_early->daily_cache,
			sizeof(health->daily_cache));

	__qc_ap_health_data_write(drvdata, health);

	return 0;
}

int __qc_kryo_arm64_edac_init(struct builder *bd)
{
	if (!IS_ENABLED(CONFIG_EDAC_KRYO_ARM64))
		return 0;	/* NOTE: do NOT block a proving */

	return ____qc_kryo_arm64_edac_init(bd);
}

static void ____qc_kryo_arm64_edac_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata = container_of(bd,
			struct qc_user_reset_drvdata, bd);
	struct notifier_block *nb = &drvdata->nb_kryo_arm64_edac;

	qcom_kryo_arm64_edac_error_unregister_notifier(nb);
}

void __qc_kryo_arm64_edac_exit(struct builder *bd)
{
	if (!IS_ENABLED(CONFIG_EDAC_KRYO_ARM64))
		return;

	____qc_kryo_arm64_edac_exit(bd);
}
