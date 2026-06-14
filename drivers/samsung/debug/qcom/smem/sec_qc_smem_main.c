// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2015-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>

#include <linux/soc/qcom/smem.h>

#include <linux/samsung/debug/qcom/sec_qc_smem.h>

#include "sec_qc_smem.h"

struct qc_smem_drvdata *qc_smem;

static noinline int __smem_test_ap_health(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	ap_health_t *health = sec_qc_ap_health_data_read();

	if (PTR_ERR(health) == -EBUSY)
		return -EPROBE_DEFER;
	else if (IS_ERR_OR_NULL(health))
		return -ENODEV;

	drvdata->ap_health = health;

	return 0;
}

static void *__smem_get_ddr_smem_entry(struct qc_smem_drvdata *drvdata,
		unsigned int id)
{
	struct device *dev = drvdata->bd.dev;
	void *entry;
	size_t size = 0;

	entry = qcom_smem_get(QCOM_SMEM_HOST_ANY, id, &size);
	if (!size)
		dev_warn(dev, "entry size is zero\n");

	return entry;
}

static noinline int __smem_test_vendor0(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	char *vendor0;

	vendor0 = __smem_get_ddr_smem_entry(drvdata, SMEM_ID_VENDOR0);
	if (PTR_ERR(vendor0) == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (IS_ERR_OR_NULL(vendor0)) {
		dev_err(bd->dev, "SMEM_ID_VENDOR0 get entry error(%ld)\n",
				PTR_ERR(vendor0));
		return -EINVAL;
	}

	drvdata->vendor0 = (sec_smem_id_vendor0_v2_t *)(&vendor0[drvdata->smem_offset_vendor0]);

	return 0;
}

static noinline int __smem_test_vendor1(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	char *vendor1;

	vendor1 = __smem_get_ddr_smem_entry(drvdata, SMEM_ID_VENDOR1);
	if (PTR_ERR(vendor1) == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (IS_ERR_OR_NULL(vendor1)) {
		dev_err(bd->dev, "SMEM_ID_VENDOR1 get entry error(%ld)\n",
				PTR_ERR(vendor1));
		return -EINVAL;
	}

	drvdata->vendor1 = (void *)(&vendor1[drvdata->smem_offset_vendor1]);

	return 0;
}

static noinline int __smem_parse_dt_smem_offset_vendor0(struct builder *bd, struct device_node *np)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	u32 smem_offset;
	int err;

	/* NOTE:
	 * android13-5.15.y and before : optional. if not found assign '0'.
	 * android14-6.1.y and after : mandatory.
	 */
	err = of_property_read_u32(np, "sec,smem_offset_vendor0", &smem_offset);
	if (err)
		return -EINVAL;

	drvdata->smem_offset_vendor0 = (size_t)smem_offset;

	return 0;
}

static noinline int __smem_parse_dt_smem_offset_vendor1(struct builder *bd, struct device_node *np)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	u32 smem_offset;
	int err;

	/* NOTE:
	 * android13-5.15.y and before : optional. if not found assign '0'.
	 * android14-6.1.y and after : mandatory.
	 */
	err = of_property_read_u32(np, "sec,smem_offset_vendor1", &smem_offset);
	if (err)
		return -EINVAL;

	drvdata->smem_offset_vendor1 = (size_t)smem_offset;

	return 0;
}

static noinline int __smem_parse_dt_vendor0_ver(struct builder *bd,
		struct device_node *np)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	u32 vendor0_ver;
	int err;

	err = of_property_read_s32(np, "sec,vendor0_ver", &vendor0_ver);
	if (err)
		return -EINVAL;

	drvdata->vendor0_ver = (unsigned int)vendor0_ver;

	return 0;
}

static noinline int __smem_parse_dt_vendor1_ver(struct builder *bd,
		struct device_node *np)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	u32 vendor1_ver;
	int err;

	err = of_property_read_s32(np, "sec,vendor1_ver", &vendor1_ver);
	if (err)
		return -EINVAL;

	drvdata->vendor1_ver = (unsigned int)vendor1_ver;

	return 0;
}

static const struct dt_builder __smem_dt_builder[] = {
	DT_BUILDER(__smem_parse_dt_smem_offset_vendor0),
	DT_BUILDER(__smem_parse_dt_smem_offset_vendor1),
	DT_BUILDER(__smem_parse_dt_vendor0_ver),
	DT_BUILDER(__smem_parse_dt_vendor1_ver),
};

static noinline int __smem_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __smem_dt_builder,
			ARRAY_SIZE(__smem_dt_builder));
}

static noinline int __smem_probe_epilog(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata =
			container_of(bd, struct qc_smem_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	qc_smem = drvdata;

	return 0;
}

static noinline void __smem_remove_prolog(struct builder *bd)
{
	qc_smem = NULL;
}

static int __smem_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_smem_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __smem_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_smem_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __smem_dev_builder[] = {
	DEVICE_BUILDER(__smem_test_ap_health, NULL),
	DEVICE_BUILDER(__smem_test_vendor0, NULL),
	DEVICE_BUILDER(__smem_test_vendor1, NULL),
	/* TODO: deferrable concrete builders should be before here. */
	DEVICE_BUILDER(__smem_parse_dt, NULL),
	DEVICE_BUILDER(__qc_smem_id_vendor1_init, NULL),
	DEVICE_BUILDER(__qc_smem_clk_osm_probe, NULL),
	DEVICE_BUILDER(__qc_smem_register_nb_cpuclk_log,
		       __qc_smem_unregister_nb_cpuclk_log),
	DEVICE_BUILDER(__qc_smem_register_nb_l3clk_log,
		       __qc_smem_unregister_nb_l3clk_log),
	DEVICE_BUILDER(__smem_probe_epilog, __smem_remove_prolog),
};

static int sec_qc_smem_probe(struct platform_device *pdev)
{
	return __smem_probe(pdev, __smem_dev_builder,
			ARRAY_SIZE(__smem_dev_builder));
}

static int sec_qc_smem_remove(struct platform_device *pdev)
{
	return __smem_remove(pdev, __smem_dev_builder,
			ARRAY_SIZE(__smem_dev_builder));
}

#define SUSPEND		0x1
#define RESUME		0x0

static int sec_qc_smem_suspend(struct device *dev)
{
	struct qc_smem_drvdata *drvdata = dev_get_drvdata(dev);
	sec_smem_id_vendor1_v2_t *ven1_v2;
	sec_smem_id_vendor1_share_t *share;

	ven1_v2 = __qc_smem_id_vendor1_get_ven1_v2(drvdata);
	if (!IS_ERR_OR_NULL(ven1_v2))
		ven1_v2->ap_suspended = SUSPEND;

	share = __qc_smem_id_vendor1_get_share(drvdata);
	if (!IS_ERR_OR_NULL(share))
		share->ap_suspended = SUSPEND;

	dev_info(dev, "smem_vendor1 ap_suspended - SUSPEND\n");

	return 0;
}

static int sec_qc_smem_resume(struct device *dev)
{
	struct qc_smem_drvdata *drvdata = dev_get_drvdata(dev);
	sec_smem_id_vendor1_v2_t *ven1_v2;
	sec_smem_id_vendor1_share_t *share;

	ven1_v2 = __qc_smem_id_vendor1_get_ven1_v2(drvdata);
	if (!IS_ERR_OR_NULL(ven1_v2))
		ven1_v2->ap_suspended = RESUME;

	share = __qc_smem_id_vendor1_get_share(drvdata);
	if (!IS_ERR_OR_NULL(share))
		share->ap_suspended = RESUME;

	dev_info(dev, "smem_vendor1 ap_suspended - RESUME\n");

	return 0;
}

static const struct dev_pm_ops sec_qc_smem_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sec_qc_smem_suspend, sec_qc_smem_resume)
};

static const struct of_device_id sec_qc_smem_match_table[] = {
	{ .compatible = "samsung,qcom-smem" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_smem_match_table);

static struct platform_driver sec_qc_smem_driver = {
	.driver = {
		.name = "sec,qc-smem",
		.of_match_table = of_match_ptr(sec_qc_smem_match_table),
		.pm = &sec_qc_smem_pm_ops,
	},
	.probe = sec_qc_smem_probe,
	.remove = sec_qc_smem_remove,
};

static int __init sec_qcom_smem_init(void)
{
	return platform_driver_register(&sec_qc_smem_driver);
}
module_init(sec_qcom_smem_init);

static void __exit sec_qcom_smem_exit(void)
{
	platform_driver_unregister(&sec_qc_smem_driver);
}
module_exit(sec_qcom_smem_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("SMEM for Qualcomm based devices");
MODULE_LICENSE("GPL v2");
