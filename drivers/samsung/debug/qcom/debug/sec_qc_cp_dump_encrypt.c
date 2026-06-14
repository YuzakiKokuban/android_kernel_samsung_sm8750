// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <linux/samsung/debug/sec_debug.h>

#include "sec_qc_debug.h"

#define ENCRYPT_CP_REGION	0x1
#define CLEAR_CP_REGION		0x0

/* kernel.sec_qc_debug.sec_cp_dbg_level */
static unsigned int sec_cp_dbg_level __ro_after_init;
module_param_named(cp_debug_level, sec_cp_dbg_level, uint, 0440);

static unsigned int sec_qc_cp_dump_policy;
module_param_named(cp_dump_policy, sec_qc_cp_dump_policy, uint, 0440);

static bool sec_cp_debug_is_on(void)
{
	if (sec_cp_dbg_level == 0x55FF)
		return false;
	return true;
}

static int __cp_dump_encrypt_init_each(
		const struct cp_dump_encrypt_entry *entry,
		struct device *dev)
{
	struct device_node *np_imem;
	void __iomem *cp_dump_encrypt;
	unsigned int debug_level;
	unsigned int policy;

	np_imem = of_find_compatible_node(NULL, NULL, entry->compatible);
	if (!np_imem) {
		pr_warn("can't find qcom,msm-imem node\n");
		return -ENODEV;
	}

	cp_dump_encrypt = of_iomap(np_imem, 0);
	if (!cp_dump_encrypt) {
		dev_err(dev, "Can't map imem\n");
		return -ENXIO;
	}

	debug_level = sec_debug_level();
	if ((debug_level == SEC_DEBUG_LEVEL_MID ||
		debug_level == SEC_DEBUG_LEVEL_HIGH) && sec_cp_debug_is_on())
		policy = entry->mid_high;
	else
		policy = entry->low;

	__raw_writel(policy, cp_dump_encrypt);
	iounmap(cp_dump_encrypt);

	sec_qc_cp_dump_policy = policy;

	return 0;
}

static int __cp_dump_encrypt_init(struct cp_dump_encrypt *encrypt,
		struct device *dev)
{
	int i;

	for (i = 0; i < encrypt->nr_entries; i++) {
		int err = __cp_dump_encrypt_init_each(&encrypt->entry[i], dev);
		if (err)
			return err;
	}

	return 0;
}

int sec_qc_cp_dump_encrypt_init(struct builder *bd)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);

	if (!drvdata->use_cp_dump_encrypt)
		return 0;

	return __cp_dump_encrypt_init(&drvdata->cp_dump_encrypt, bd->dev);
}
