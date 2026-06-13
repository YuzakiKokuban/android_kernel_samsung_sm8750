// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_crashkey_long.h>
#include <linux/samsung/debug/sec_upload_cause.h>
#include <linux/samsung/debug/qcom/sec_qc_upload_cause.h>

struct qc_upldc_drvdata {
	struct builder bd;
};

static struct qc_upldc_drvdata *qc_upldc;

/* This is shared with msm-power off module. */
static void __iomem *qcom_upload_cause;

DEFINE_PER_CPU(unsigned long, sec_debug_upload_cause);

static __always_inline bool __qc_upldc_is_probed(void)
{
	return !!qc_upldc;
}

static void __qc_upldc_write_cause(unsigned int type)
{
	__raw_writel(type, qcom_upload_cause);
}

static unsigned int __qc_upldc_read_cause(void)
{
	return readl(qcom_upload_cause);
}

void sec_qc_upldc_write_cause(unsigned int type)
{
	int cpu;

	if (!__qc_upldc_is_probed()) {
		pr_warn("upload cause address unmapped.\n");
		return;
	}

	cpu = get_cpu();
	per_cpu(sec_debug_upload_cause, cpu) = type;
	__qc_upldc_write_cause(type);
	put_cpu();

	pr_emerg("%x\n", type);
}
EXPORT_SYMBOL_GPL(sec_qc_upldc_write_cause);

unsigned int sec_qc_upldc_read_cause(void)
{
	if (!__qc_upldc_is_probed()) {
		pr_warn("upload cause address unmapped.\n");
		return UPLOAD_CAUSE_INIT;
	}

	return __qc_upldc_read_cause();
}
EXPORT_SYMBOL_GPL(sec_qc_upldc_read_cause);

void sec_qc_upldc_type_to_cause(unsigned int type, char *cause, size_t len)
{
	if (type == UPLOAD_CAUSE_KERNEL_PANIC)
		strlcpy(cause, "kernel_panic", len);
	else if (type == UPLOAD_CAUSE_INIT)
		strlcpy(cause, "unknown_error", len);
	else if (type == UPLOAD_CAUSE_NON_SECURE_WDOG_BARK)
		strlcpy(cause, "watchdog_bark", len);
	else
		sec_upldc_type_to_cause(type, cause, len);
}
EXPORT_SYMBOL_GPL(sec_qc_upldc_type_to_cause);

#define SEC_QC_UPLC(__cause, __type, __func) \
{ \
	.cause = __cause, \
	.type = __type, \
	.func = __func, \
}

#define SEC_QC_UPLC_STRNCMP(__cause, __type) \
	SEC_QC_UPLC(__cause, __type, sec_qc_upldc_strncmp)

#define SEC_QC_UPLC_STRNSTR(__cause, __type) \
	SEC_QC_UPLC(__cause, __type, sec_qc_upldc_strnstr)

#define SEC_QC_UPLC_STRNCASECMP(__cause, __type) \
	SEC_QC_UPLC(__cause, __type, sec_qc_upldc_strncasecmp)

static int __used sec_qc_upldc_strncmp(const struct sec_upload_cause *uc,
		const char *cause)
{
	if (strncmp(cause, uc->cause, strlen(uc->cause)))
		return SEC_UPLOAD_CAUSE_HANDLE_DONE;

	sec_qc_upldc_write_cause(uc->type);

	return SEC_UPLOAD_CAUSE_HANDLE_OK;
}

static int __used sec_qc_upldc_strnstr(const struct sec_upload_cause *uc,
		const char *cause)
{
	if (!strnstr(cause, uc->cause, strlen(cause)))
		return SEC_UPLOAD_CAUSE_HANDLE_DONE;

	sec_qc_upldc_write_cause(uc->type);

	return SEC_UPLOAD_CAUSE_HANDLE_OK;
}

static int __used sec_qc_upldc_strncasecmp(const struct sec_upload_cause *uc,
		const char *cause)
{
	if (strncasecmp(cause, uc->cause, strlen(uc->cause)))
		return SEC_UPLOAD_CAUSE_HANDLE_DONE;

	sec_qc_upldc_write_cause(uc->type);

	return SEC_UPLOAD_CAUSE_HANDLE_OK;
}

static struct sec_upload_cause __qc_upldc_strncmp[] = {
	SEC_QC_UPLC_STRNCMP("User Fault",
			    UPLOAD_CAUSE_USER_FAULT),
	SEC_QC_UPLC_STRNCMP("Crash Key",
			    UPLOAD_CAUSE_FORCED_UPLOAD),
	SEC_QC_UPLC_STRNCMP("User Crash Key",
			    UPLOAD_CAUSE_USER_FORCED_UPLOAD),
	SEC_QC_UPLC_STRNCMP("Long Key Press",
			    UPLOAD_CAUSE_POWER_LONG_PRESS),
	SEC_QC_UPLC_STRNCMP("CP Crash",
			    UPLOAD_CAUSE_CP_ERROR_FATAL),
	SEC_QC_UPLC_STRNCMP("MDM Crash",
			    UPLOAD_CAUSE_MDM_ERROR_FATAL),
	SEC_QC_UPLC_STRNCMP("watchdog_bark",
			    UPLOAD_CAUSE_NON_SECURE_WDOG_BARK)
};

static struct sec_upload_cause __qc_upldc_strnstr[] = {
	SEC_QC_UPLC_STRNSTR("mss",
			    UPLOAD_CAUSE_MODEM_RST_ERR),
	SEC_QC_UPLC_STRNSTR("esoc0 crashed",
			    UPLOAD_CAUSE_MDM_ERROR_FATAL),
	SEC_QC_UPLC_STRNSTR("modem",
			    UPLOAD_CAUSE_MODEM_RST_ERR),
	SEC_QC_UPLC_STRNSTR("external_modem",
			    UPLOAD_CAUSE_MDM_ERROR_FATAL),
	SEC_QC_UPLC_STRNSTR("unrecoverable external_modem",
			    UPLOAD_CAUSE_MDM_CRITICAL_FATAL),
	SEC_QC_UPLC_STRNSTR("riva",
			    UPLOAD_CAUSE_RIVA_RST_ERR),
	SEC_QC_UPLC_STRNSTR("lpass",
			    UPLOAD_CAUSE_LPASS_RST_ERR),
	SEC_QC_UPLC_STRNSTR("dsps",
			    UPLOAD_CAUSE_DSPS_RST_ERR),
	SEC_QC_UPLC_STRNSTR("SMPL",
			    UPLOAD_CAUSE_SMPL),
	SEC_QC_UPLC_STRNSTR("adsp",
			    UPLOAD_CAUSE_ADSP_ERROR_FATAL),
	SEC_QC_UPLC_STRNSTR("slpi",
			    UPLOAD_CAUSE_SLPI_ERROR_FATAL),
	SEC_QC_UPLC_STRNSTR("spss",
			    UPLOAD_CAUSE_SPSS_ERROR_FATAL),
	SEC_QC_UPLC_STRNSTR("npu",
			    UPLOAD_CAUSE_NPU_ERROR_FATAL),
	SEC_QC_UPLC_STRNSTR("cdsp",
			    UPLOAD_CAUSE_CDSP_ERROR_FATAL),
	SEC_QC_UPLC_STRNSTR("taking too long",
			    UPLOAD_CAUSE_SUBSYS_IF_TIMEOUT),
	SEC_QC_UPLC_STRNSTR("Software Watchdog Timer expired",
			    UPLOAD_CAUSE_PF_WD_BITE),
};

struct sec_upload_cause_suite {
	struct sec_upload_cause *uc;
	size_t nr_cause;
};

static struct sec_upload_cause_suite qc_upldc_suite[] = {
	{
		.uc = __qc_upldc_strncmp,
		.nr_cause = ARRAY_SIZE(__qc_upldc_strncmp),
	},
	{
		.uc = __qc_upldc_strnstr,
		.nr_cause = ARRAY_SIZE(__qc_upldc_strnstr),
	},
};

static void __qc_upldc_del_for_each(struct sec_upload_cause *uc, ssize_t n);

static int __qc_upldc_add_for_each(struct sec_upload_cause *uc, ssize_t n)
{
	int err;
	ssize_t last_failed;
	ssize_t i;

	for (i = 0; i < n; i++) {
		err = sec_upldc_add_cause(&uc[i]);
		if (err) {
			last_failed = i;
			goto err_add_cause;
		}
	}

	return 0;

err_add_cause:
	__qc_upldc_del_for_each(uc, last_failed);
	if (err == -EBUSY)
		return -EPROBE_DEFER;

	return err;
}

static void __qc_upldc_del_suite(struct sec_upload_cause_suite *suite,
		ssize_t n);

static int __qc_upldc_add_suite(struct sec_upload_cause_suite *suite,
		ssize_t n)
{
	int err;
	ssize_t last_failed;
	ssize_t i;

	for (i = 0; i < n; i++) {
		err = __qc_upldc_add_for_each(suite[i].uc, suite[i].nr_cause);
		if (err) {
			last_failed = i;
			goto err_add_for_each;
		}
	}

	return 0;

err_add_for_each:
	__qc_upldc_del_suite(suite, last_failed);
	return err;
}

static void __qc_upldc_del_for_each(struct sec_upload_cause * uc, ssize_t n)
{
	ssize_t i;

	for (i = n - 1; i >= 0; i--)
		sec_upldc_del_cause(&uc[i]);
}

static void __qc_upldc_del_suite(struct sec_upload_cause_suite *suite,
		ssize_t n)
{
	ssize_t i;

	for (i = n - 1; i >= 0; i--)
		__qc_upldc_del_for_each(suite[i].uc, suite[i].nr_cause);
}


static int __qc_upldc_crashkey_long_on_matched(void)
{
	if (sec_qc_upldc_read_cause() == UPLOAD_CAUSE_INIT)
		sec_qc_upldc_write_cause(UPLOAD_CAUSE_POWER_LONG_PRESS);

	return NOTIFY_OK;
}

static int __qc_upldc_crashkey_long_on_unmatched(void)
{
	if (sec_qc_upldc_read_cause() == UPLOAD_CAUSE_POWER_LONG_PRESS)
		sec_qc_upldc_write_cause(UPLOAD_CAUSE_INIT);

	return NOTIFY_OK;
}

static int sec_qc_upldc_crashkey_long_call(struct notifier_block *this,
		unsigned long type, void *v)
{
	int ret;

	switch (type) {
	case SEC_CRASHKEY_LONG_NOTIFY_TYPE_MATCHED:
		ret = __qc_upldc_crashkey_long_on_matched();
		break;
	case SEC_CRASHKEY_LONG_NOTIFY_TYPE_UNMATCHED:
		ret = __qc_upldc_crashkey_long_on_unmatched();
		break;
	case SEC_CRASHKEY_LONG_NOTIFY_TYPE_EXPIRED:
	default:
		ret = NOTIFY_DONE;
	}

	return ret;
}

static int sec_qc_upldc_default_cause(const struct sec_upload_cause *uc,
		const char *cause)
{
	sec_qc_upldc_write_cause(UPLOAD_CAUSE_KERNEL_PANIC);

	return SEC_UPLOAD_CAUSE_HANDLE_OK;
}

static struct sec_upload_cause sec_qc_upldc_default_handle = {
	.func = sec_qc_upldc_default_cause,
};

static int __qc_upldc_set_default_cause(struct builder *bd)
{
	int err = sec_upldc_set_default_cause(&sec_qc_upldc_default_handle);

	if (err == -EBUSY)
		return -EPROBE_DEFER;

	return err;
}

static void __qc_upldc_unset_default_cause(struct builder *bd)
{
	sec_upldc_unset_default_cause(&sec_qc_upldc_default_handle);
}

static struct notifier_block sec_qc_upldc_crashkey_long_handle = {
	.notifier_call = sec_qc_upldc_crashkey_long_call,
};

static int __qc_upldc_register_crashkey_long_handle(struct builder *bd)
{
	int err = sec_crashkey_long_add_preparing_panic(
			&sec_qc_upldc_crashkey_long_handle);

	if (err == -EBUSY)
		return -EPROBE_DEFER;

	return err;
}

static void __qc_upldc_unregister_crashkey_long_handle(struct builder *bd)
{
	sec_crashkey_long_del_preparing_panic(
			&sec_qc_upldc_crashkey_long_handle);
}

static int __qc_upldc_ioremap_qcom_upload_cause(struct builder *bd)
{
	struct device_node *mem_np;
	struct device *dev;

	dev = bd->dev;

	mem_np = of_find_compatible_node(NULL, NULL,
			"qcom,msm-imem-upload_cause");
	if (!mem_np) {
		dev_err(dev, "unable to find DT imem upload cause node\n");
		return -ENODEV;
	}

	qcom_upload_cause = of_iomap(mem_np, 0);
	if (unlikely(!qcom_upload_cause)) {
		dev_err(dev, "unable to map imem upload cause offset\n");
		return -ENOMEM;
	}

	dev_err(dev, "upload_cause addr : 0x%p(0x%llx)\n",
			qcom_upload_cause,
			(unsigned long long)virt_to_phys(qcom_upload_cause));

	sec_qc_upldc_write_cause(UPLOAD_CAUSE_INIT);

	return 0;
}

static void __qc_upldc_iounmap_qcom_upload_cause(struct builder *bd)
{
	iounmap(qcom_upload_cause);
}

static int __qc_upldc_add_upload_cause_suite(struct builder *bd)
{
	return __qc_upldc_add_suite(qc_upldc_suite, ARRAY_SIZE(qc_upldc_suite));
}

static void __qc_upldc_del_upload_cause_suite(struct builder *bd)
{
	__qc_upldc_del_suite(qc_upldc_suite,
			       ARRAY_SIZE(qc_upldc_suite));
}

static noinline int __qc_upldc_set_drvdata(struct builder *bd)
{
	struct qc_upldc_drvdata *drvdata =
			container_of(bd, struct qc_upldc_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static noinline int __qc_upldc_probe_epilog(struct builder *bd)
{
	struct qc_upldc_drvdata *drvdata =
			container_of(bd, struct qc_upldc_drvdata, bd);

	qc_upldc = drvdata;

	return 0;
}

static noinline void __qc_upldc_remove_prolog(struct builder *bd)
{
	qc_upldc = NULL;
}

static int __qc_upldc_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_upldc_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __qc_upldc_probe_threaded(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_upldc_drvdata *drvdata = platform_get_drvdata(pdev);

	return sec_director_probe_dev_threaded(&drvdata->bd, builder, n,
			"qc_upldc");
}

static int __qc_upldc_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_upldc_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int __qc_upldc_remove_threaded(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_upldc_drvdata *drvdata = platform_get_drvdata(pdev);
	struct director_threaded *drct = drvdata->bd.drct;

	sec_director_destruct_dev_threaded(drct);

	return 0;
}

static const struct dev_builder __qc_upldc_dev_builder[] = {
	DEVICE_BUILDER(__qc_upldc_ioremap_qcom_upload_cause,
		       __qc_upldc_iounmap_qcom_upload_cause),
	DEVICE_BUILDER(__qc_upldc_set_drvdata, NULL),
};

static const struct dev_builder __qc_upldc_dev_builder_threaded[] = {
	DEVICE_BUILDER(__qc_upldc_add_upload_cause_suite,
		       __qc_upldc_del_upload_cause_suite),
	DEVICE_BUILDER(__qc_upldc_set_default_cause,
		       __qc_upldc_unset_default_cause),
	DEVICE_BUILDER(__qc_upldc_register_crashkey_long_handle,
		       __qc_upldc_unregister_crashkey_long_handle),
	DEVICE_BUILDER(__qc_upldc_probe_epilog, __qc_upldc_remove_prolog),
};

static int sec_qc_upldc_probe(struct platform_device *pdev)
{
	int err;

	err = __qc_upldc_probe(pdev, __qc_upldc_dev_builder,
			ARRAY_SIZE(__qc_upldc_dev_builder));
	if (err)
		return err;

	return __qc_upldc_probe_threaded(pdev, __qc_upldc_dev_builder_threaded,
			ARRAY_SIZE(__qc_upldc_dev_builder_threaded));
}

static int sec_qc_upldc_remove(struct platform_device *pdev)
{
	__qc_upldc_remove_threaded(pdev, __qc_upldc_dev_builder_threaded,
			ARRAY_SIZE(__qc_upldc_dev_builder_threaded));

	__qc_upldc_remove(pdev, __qc_upldc_dev_builder,
			ARRAY_SIZE(__qc_upldc_dev_builder));

	return 0;
}

static const struct of_device_id sec_qc_upldc_match_table[] = {
	{ .compatible = "samsung,qcom-upload_cause" },
	{ },
};
MODULE_DEVICE_TABLE(of, sec_qc_upldc_match_table);

static struct platform_driver sec_qc_upldc_driver = {
	.driver = {
		.name = "sec,qc-upload_cause",
		.of_match_table = of_match_ptr(sec_qc_upldc_match_table),
	},
	.probe = sec_qc_upldc_probe,
	.remove = sec_qc_upldc_remove,
};

static int __init sec_qc_upldc_init(void)
{
	return platform_driver_register(&sec_qc_upldc_driver);
}
arch_initcall(sec_qc_upldc_init);

static void __exit sec_qc_upldc_exit(void)
{
	platform_driver_unregister(&sec_qc_upldc_driver);
}
module_exit(sec_qc_upldc_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Panic Notifier Updating Upload Cause for Qualcomm based devices");
MODULE_LICENSE("GPL v2");
