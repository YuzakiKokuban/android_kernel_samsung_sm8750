// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2019-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/sched.h>

#include <asm/fpsimd.h>

#include <trace/hooks/fpsimd.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_debug.h>
#include <linux/samsung/sec_of.h>

struct fsimd_debug_drvdata {
	struct builder bd;
};

static noinline int __fsimd_debug_parse_dt_check_debug_level(struct builder *bd,
		struct device_node *np)
{
	struct device *dev = bd->dev;
	unsigned int sec_dbg_level = sec_debug_level();
	int err;

	err = sec_of_test_debug_level(np, "sec,debug_level", sec_dbg_level);
	if (err == -ENOENT) {
		dev_warn(dev, "%s will be enabled all sec debug levels!\n",
				dev_name(dev));
		return 0;
	} else if (err < 0)
		return -ENODEV;

	return 0;
}

static const struct dt_builder __fsimd_debug_dt_builder[] = {
	DT_BUILDER(__fsimd_debug_parse_dt_check_debug_level),
};

static noinline int __fsimd_debug_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __fsimd_debug_dt_builder,
			ARRAY_SIZE(__fsimd_debug_dt_builder));
}

#if IS_BUILTIN(CONFIG_SEC_ARM64_FSIMD_DEBUG)
static __always_inline void __fpsimd_save_state(struct user_fpsimd_state *state)
{
	fpsimd_save_state(state);
}
#else
/* NOTE: copied from arch/arm64/kernel/entry-fpsimd.S */
static void __naked __fpsimd_save_state(struct user_fpsimd_state *state)
{
	asm volatile (
		"stp	q0, q1, [x0] \n\t"
		"stp	q2, q3, [x0, #32] \n\t"
		"stp	q4, q5, [x0, #64] \n\t"
		"stp	q6, q7, [x0, #96] \n\t"
		"stp	q8, q9, [x0, #128] \n\t"
		"stp	q10, q11, [x0, #160] \n\t"
		"stp	q12, q13, [x0, #192] \n\t"
		"stp	q14, q15, [x0, #224] \n\t"
		"stp	q16, q17, [x0, #256] \n\t"
		"stp	q18, q19, [x0, #288] \n\t"
		"stp	q20, q21, [x0, #320] \n\t"
		"stp	q22, q23, [x0, #352] \n\t"
		"stp	q24, q25, [x0, #384] \n\t"
		"stp	q26, q27, [x0, #416] \n\t"
		"stp	q28, q29, [x0, #448] \n\t"
		"stp	q30, q31, [x0, #480]! \n\t"
		"mrs	x8, fpsr \n\t"
		"str	w8, [x0, #32] \n\t"
		"mrs	x8, fpcr \n\t"
		"str	w8, [x0, #36] \n\t"
		"ret \n\t"
	);
}
#endif

static void __trace_android_vh_is_fpsimd_save(void *unused,
		struct task_struct *prev, struct task_struct *next)
{
	struct user_fpsimd_state current_st;
	struct user_fpsimd_state *saved_st = &next->thread.uw.fpsimd_state;
	size_t i;

	if (test_tsk_thread_flag(next, TIF_FOREIGN_FPSTATE))
		return;

	__fpsimd_save_state(&current_st);

	for (i = 0; i < ARRAY_SIZE(current_st.vregs); i++) {
		/* NOTE: this driver will not be proved when the debug level is low. */
		BUG_ON(current_st.vregs[i] != saved_st->vregs[i]);
	}

	/* NOTE: this driver will not be proved when the debug level is low. */
	BUG_ON((current_st.fpsr != saved_st->fpsr) ||
	       (current_st.fpcr != saved_st->fpcr));
}

static int __fsimd_debug_install_vendor_hook(struct builder *bd)
{
	return register_trace_android_vh_is_fpsimd_save(
			__trace_android_vh_is_fpsimd_save,
			NULL);
}

static void __fsimd_debug_uninstall_vendor_hook(struct builder *bd)
{
	unregister_trace_android_vh_is_fpsimd_save(
			__trace_android_vh_is_fpsimd_save,
			NULL);
}

static noinline int __fsimd_debug_probe_epilog(struct builder *bd)
{
	struct fsimd_debug_drvdata *drvdata =
			container_of(bd, struct fsimd_debug_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static const struct dev_builder __fsimd_debug_dev_builder[] = {
	DEVICE_BUILDER(__fsimd_debug_parse_dt, NULL),
	DEVICE_BUILDER(__fsimd_debug_install_vendor_hook,
		       __fsimd_debug_uninstall_vendor_hook),
	DEVICE_BUILDER(__fsimd_debug_probe_epilog, NULL),
};

static int __fsimd_debug_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct fsimd_debug_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __fsimd_debug_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct fsimd_debug_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int sec_fsimd_debug_probe(struct platform_device *pdev)
{
	return __fsimd_debug_probe(pdev, __fsimd_debug_dev_builder,
			ARRAY_SIZE(__fsimd_debug_dev_builder));
}

static int sec_fsimd_debug_remove(struct platform_device *pdev)
{
	return __fsimd_debug_remove(pdev, __fsimd_debug_dev_builder,
			ARRAY_SIZE(__fsimd_debug_dev_builder));
}

static const struct of_device_id sec_fsimd_debug_match_table[] = {
	{ .compatible = "samsung,arm64-fsimd_debug" },
	{ .compatible = "samsung,fsimd_debug" },	/* TODP: should be removed in future */
	{},
};
MODULE_DEVICE_TABLE(of, sec_fsimd_debug_match_table);

static struct platform_driver sec_fsimd_debug_driver = {
	.driver = {
		.name = "sec,arm64-fsimd_debug",
		.of_match_table = of_match_ptr(sec_fsimd_debug_match_table),
	},
	.probe = sec_fsimd_debug_probe,
	.remove = sec_fsimd_debug_remove,
};

static int __init sec_fsimd_debug_init(void)
{
	return platform_driver_register(&sec_fsimd_debug_driver);
}
module_init(sec_fsimd_debug_init);

static void __exit sec_fsimd_debug_exit(void)
{
	return platform_driver_unregister(&sec_fsimd_debug_driver);
}
module_exit(sec_fsimd_debug_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Detecting fsimd register corruption");
MODULE_LICENSE("GPL v2");
