// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/kernel.h>
#include <linux/sched.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_summary.h>

#include "sec_qc_user_reset.h"

static int __modem_reset_data_test_last_rst_reason(
		struct qc_user_reset_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	ap_health_t *health;
	int ret;

	health = __qc_ap_health_data_read(drvdata);
	if (IS_ERR_OR_NULL(health)) {
		ret = -ENODEV;
		goto err_read_ap_health_data;
	}

	switch (health->last_rst_reason) {
	case USER_UPLOAD_CAUSE_PANIC:
	case USER_UPLOAD_CAUSE_CP_CRASH:
		dev_info(dev, "modem reset data will be updated...\n");
		ret = 1;
		break;
	default:
		dev_info(dev, "reset reason is not panic or cp-crash\n");
		ret = 0;
		break;
	}

err_read_ap_health_data:
	return ret;
}

static int __modem_reset_data_restore(struct qc_user_reset_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	struct sec_qc_summary_data_modem *modem;
	bool result;

	modem = sec_qc_summary_get_modem();
	if (PTR_ERR(modem) == -EBUSY)
		return -EPROBE_DEFER;
	else if (IS_ERR_OR_NULL(modem)) {
		dev_warn(dev, "modem reset data is skipped.");
		return 0;
	}

	result = sec_qc_dbg_part_read(debug_index_modem_info, modem);
	if (!result)
		dev_warn(dev, "failed to read modem reset data!\n");

	return 0;
}

int __qc_modem_reset_data_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata = container_of(bd,
			struct qc_user_reset_drvdata, bd);
	int is_required;

	is_required = __modem_reset_data_test_last_rst_reason(drvdata);
	if (is_required < 0)
		return is_required;

	if (!is_required)
		return 0;

	return __modem_reset_data_restore(drvdata);
}
