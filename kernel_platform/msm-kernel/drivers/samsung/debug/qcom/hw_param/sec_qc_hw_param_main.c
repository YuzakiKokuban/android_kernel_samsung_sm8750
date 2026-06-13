// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/sched.h>

#include <linux/samsung/bsp/sec_class.h>

#include "sec_qc_hw_param.h"

struct qc_hw_param_drvdata *qc_hw_param;

bool __qc_hw_param_is_valid_reset_reason(unsigned int reset_reason)
{
	if (reset_reason < USER_UPLOAD_CAUSE_MIN ||
	    reset_reason > USER_UPLOAD_CAUSE_MAX)
		return false;

	if (reset_reason == USER_UPLOAD_CAUSE_MANUAL_RESET ||
	    reset_reason == USER_UPLOAD_CAUSE_REBOOT ||
	    reset_reason == USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT ||
	    reset_reason == USER_UPLOAD_CAUSE_POWER_ON)
		return false;

	return true;
}

void __qc_hw_param_clean_format(char *buf, ssize_t *size, int max_len_str)
{
	int i = 0, cnt = 0, pos = 0;

	if (!buf || *size <= 0)
		return;

	if (*size >= max_len_str)
		*size = max_len_str - 1;

	while (i < *size && buf[i]) {
		if (buf[i] == '"') {
			cnt++;
			pos = i;
		}

		if ((buf[i] < 0x20) || (buf[i] == 0x5C) || (buf[i] > 0x7E))
			buf[i] = ' ';
		i++;
	}

	if (cnt % 2) {
		if (pos == *size - 1) {
			buf[*size - 1] = '\0';
		} else {
			buf[*size - 1] = '"';
			buf[*size] = '\0';
		}
	}
}

int __qc_hw_param_read_param0(const char *name)
{
	struct qc_ap_info *ap_info = &qc_hw_param->ap_info;
	struct device *dev = qc_hw_param->bd.dev;
	u32 val;
	int ret;

	if (!ap_info->np_hw_param)
		return -PARAM0_IVALID;

	ret = of_property_read_u32(ap_info->np_hw_param, name, &val);
	if (ret) {
		dev_warn(dev, "failed to get %s from node\n", name);
		return -PARAM0_LESS_THAN_0;
	}

	return (int)val;
}

static noinline int __hw_param_test_ap_health(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	ap_health_t *health = sec_qc_ap_health_data_read();

	if (PTR_ERR(health) == -EBUSY)
		return -EPROBE_DEFER;
	else if (IS_ERR_OR_NULL(health))
		return -ENODEV;

	drvdata->ap_health = health;

	return 0;
}

static noinline int __hw_param_parse_dt_use_ddr_eye_info(struct builder *bd,
		struct device_node *np)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);

	drvdata->use_ddr_eye_info =
			of_property_read_bool(np, "sec,use-ddr_eye_info");

	return 0;
}

static const struct dt_builder __hw_param_dt_builder[] = {
	DT_BUILDER(__hw_param_parse_dt_use_ddr_eye_info),
};

static noinline int __hw_param_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __hw_param_dt_builder,
			ARRAY_SIZE(__hw_param_dt_builder));
}

static noinline int __hw_param_init_sec_class(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct device *sec_hw_param_dev;

	sec_hw_param_dev = sec_device_create(NULL, "sec_hw_param");
	if (IS_ERR(sec_hw_param_dev)) {
		dev_err(bd->dev, "failed to create device for sec_debug\n");
		return PTR_ERR(sec_hw_param_dev);
	}

	drvdata->sec_hw_param_dev = sec_hw_param_dev;

	return 0;
}

static noinline int __hw_param_find_sec_hw_param_node(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct device *dev = drvdata->bd.dev;
	struct qc_ap_info *ap_info = &drvdata->ap_info;
	struct device_node *np = of_find_node_by_path("/soc/sec_hw_param");

	if (!np) {
		dev_warn(dev, "sec_hw_param node is not found\n");
		return 0;
	}

	ap_info->np_hw_param = np;

	return 0;
}

static noinline void __hw_param_exit_sec_class(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct device *sec_hw_param_dev = drvdata->sec_hw_param_dev;

	sec_device_destroy(sec_hw_param_dev->devt);
}

static struct attribute *hw_param_attrs[] = {
	&dev_attr_ap_info.attr,
	&dev_attr_ap_health.attr,
	&dev_attr_last_dcvs.attr,
	&dev_attr_extra_info.attr,
	&dev_attr_extrb_info.attr,
	&dev_attr_extrc_info.attr,
	&dev_attr_extrt_info.attr,
	&dev_attr_extrm_info.attr,
	&dev_attr_ddr_info.attr,
	&dev_attr_eye_rd_info.attr,
	NULL,
};

static const struct attribute_group hw_param_attr_group = {
	.attrs = hw_param_attrs,
};

static struct attribute *hw_param_attrs_for_ddr_eye_info[] = {
	&dev_attr_eye_dcc_info.attr,
	&dev_attr_eye_wr1_info.attr,
	&dev_attr_eye_wr2_info.attr,
	NULL,
};

static const struct attribute_group hw_param_attr_group_for_ddr_eye_info = {
	.attrs = hw_param_attrs_for_ddr_eye_info,
};

static int __hw_param_sysfs_create_group(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct device *sec_hw_param_dev = drvdata->sec_hw_param_dev;
	int err;

	err = sysfs_create_group(&sec_hw_param_dev->kobj,
			&hw_param_attr_group);
	if (err) {
		dev_err(bd->dev, "failed to create device files. (%d)", err);
		return err;
	}

	return 0;
}

static void __hw_param_sysfs_remove_group(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct device *sec_hw_param_dev = drvdata->sec_hw_param_dev;

	sysfs_remove_group(&sec_hw_param_dev->kobj,
			&hw_param_attr_group);
}

static int __hw_param_sysfs_create_group_ddr_eye_info(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct device *sec_hw_param_dev = drvdata->sec_hw_param_dev;
	int err;

	if (!drvdata->use_ddr_eye_info)
		return 0;

	err = sysfs_create_group(&sec_hw_param_dev->kobj,
			&hw_param_attr_group_for_ddr_eye_info);
	if (err) {
		dev_err(bd->dev, "failed to create device files. (%d)", err);
		return err;
	}

	return 0;
}

static void __hw_param_sysfs_remove_group_ddr_eye_info(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct device *sec_hw_param_dev = drvdata->sec_hw_param_dev;

	if (!drvdata->use_ddr_eye_info)
		return;

	sysfs_remove_group(&sec_hw_param_dev->kobj,
			&hw_param_attr_group_for_ddr_eye_info);
}

static noinline int __hw_param_probe_epilog(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	qc_hw_param = drvdata;

	return 0;
}

static noinline void __hw_param_remove_prolog(struct builder *bd)
{
	qc_hw_param = NULL;
}

static const struct dev_builder __hw_param_dev_builder[] = {
	DEVICE_BUILDER(__hw_param_test_ap_health, NULL),
	/* TODO: deferrable concrete builders should be before here. */
	DEVICE_BUILDER(__hw_param_parse_dt, NULL),
	DEVICE_BUILDER(__hw_param_init_sec_class, __hw_param_exit_sec_class),
	DEVICE_BUILDER(__hw_param_find_sec_hw_param_node, NULL),
	DEVICE_BUILDER(__qc_ap_info_init, NULL),
	DEVICE_BUILDER(__qc_last_dcvs_init, NULL),
	DEVICE_BUILDER(__hw_param_sysfs_create_group,
		       __hw_param_sysfs_remove_group),
	DEVICE_BUILDER(__hw_param_sysfs_create_group_ddr_eye_info,
		       __hw_param_sysfs_remove_group_ddr_eye_info),
	DEVICE_BUILDER(__qc_errp_extra_init, __qc_errp_extra_exit),
	DEVICE_BUILDER(__hw_param_probe_epilog, __hw_param_remove_prolog),
};

static int __hw_param_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_hw_param_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __hw_param_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_hw_param_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int sec_qc_hw_param_probe(struct platform_device *pdev)
{
	return __hw_param_probe(pdev, __hw_param_dev_builder,
			ARRAY_SIZE(__hw_param_dev_builder));
}

static int sec_qc_hw_param_remove(struct platform_device *pdev)
{
	return __hw_param_remove(pdev, __hw_param_dev_builder,
			ARRAY_SIZE(__hw_param_dev_builder));
}

static const struct of_device_id sec_qc_hw_param_match_table[] = {
	{ .compatible = "samsung,qcom-hw_param" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_hw_param_match_table);

static struct platform_driver sec_qc_hw_param_driver = {
	.driver = {
		.name = "sec,qc-hw_param",
		.of_match_table = of_match_ptr(sec_qc_hw_param_match_table),
	},
	.probe = sec_qc_hw_param_probe,
	.remove = sec_qc_hw_param_remove,
};

static int __init sec_qc_hw_param_init(void)
{
	return platform_driver_register(&sec_qc_hw_param_driver);
}
module_init(sec_qc_hw_param_init);

static void __exit sec_qc_hw_param_exit(void)
{
	platform_driver_unregister(&sec_qc_hw_param_driver);
}
module_exit(sec_qc_hw_param_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("HW Paramter Driver for Qualcomm based devices");
MODULE_LICENSE("GPL v2");
