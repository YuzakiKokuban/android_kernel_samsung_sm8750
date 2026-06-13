// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kdebug.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/panic_notifier.h>
#include <linux/platform_device.h>

#include <trace/hooks/debug.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_riscv64_ap_context.h>
#include <linux/samsung/debug/sec_debug_region.h>

struct ap_context_drvdata {
	struct builder bd;
	const char *name;
	uint32_t unique_id;
	struct sec_dbg_region_client *client;
	struct sec_riscv64_ap_context *ctx;
	struct notifier_block nb_die;
	struct notifier_block nb_panic;
};

enum {
	TYPE_VH_IPI_STOP = 0,
	/* */
	TYPE_VH_MAX,
	TYPE_VH_UNKNOWN = -EINVAL,
};

struct sec_riscv64_ap_context *ap_context[TYPE_VH_MAX];

static void __always_inline __ap_context_save_core_regs_from_pt_regs(
		struct sec_riscv64_ap_context *ctx, struct pt_regs *regs)
{
	memcpy_toio(&ctx->core_regs, regs, sizeof(struct pt_regs));
}

/* FIXME: tempoary workaround to prevent linking errors */
void __naked __ap_context_save_core_regs_on_current(struct pt_regs *regs)
{
	asm volatile (
		/* NOTE: pc, ra, sp, fp are not saved at here. */
		"sd	gp,24(a0) \n\t"
		"sd	t0,40(a0) \n\t"
		"sd	t1,48(a0) \n\t"
		"sd	t2,56(a0) \n\t"
		"sd	s1,72(a0) \n\t"
		"sd	a0,80(a0) \n\t"
		"sd	a1,88(a0) \n\t"
		"sd	a2,96(a0) \n\t"
		"sd	a3,104(a0) \n\t"
		"sd	a4,112(a0) \n\t"
		"sd	a5,120(a0) \n\t"
		"sd	a6,128(a0) \n\t"
		"sd	a7,136(a0) \n\t"
		"sd	s2,144(a0) \n\t"
		"sd	s3,152(a0) \n\t"
		"sd	s4,160(a0) \n\t"
		"sd	s5,168(a0) \n\t"
		"sd	s6,176(a0) \n\t"
		"sd	s7,184(a0) \n\t"
		"sd	s8,192(a0) \n\t"
		"sd	s9,200(a0) \n\t"
		"sd	s10,208(a0) \n\t"
		"sd	s11,216(a0) \n\t"
		"sd	t3,224(a0) \n\t"
		"sd	t4,232(a0) \n\t"
		"sd	t5,240(a0) \n\t"
		"sd	t6,248(a0) \n\t"
		"ret \n\t"
	);
}

static void __always_inline __ap_context_save_core_extra_regs(
		struct sec_riscv64_ap_context *ctx)
{
	struct pt_regs *regs = &ctx->core_regs;

	regs->status = csr_read(CSR_STATUS);
}

static void __always_inline __ap_context_save_csr_regs(
		struct sec_riscv64_ap_context *ctx)
{
	uint64_t *csr_regs = &ctx->csr_regs[0];

	csr_regs[ID_RV64_CSR_SATP] = csr_read(CSR_SATP);
}

static ssize_t __ap_context_unique_id_to_type(uint32_t unique_id)
{
	ssize_t type;

	switch (unique_id) {
	case SEC_RISCV64_VH_IPI_STOP_MAGIC:
		type = TYPE_VH_IPI_STOP;
		break;
	default:
		type = TYPE_VH_UNKNOWN;
		break;
	}

	return type;
}

static noinline int __ap_context_parse_dt_name(struct builder *bd,
		struct device_node *np)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);

	return of_property_read_string(np, "sec,name", &drvdata->name);
}

static noinline int __ap_context_parse_dt_unique_id(struct builder *bd,
                struct device_node *np)
{
        struct ap_context_drvdata *drvdata =
                        container_of(bd, struct ap_context_drvdata, bd);
        u32 unique_id;
        int err;

        err = of_property_read_u32(np, "sec,unique_id", &unique_id);
        if (err)
                return -EINVAL;

        drvdata->unique_id = (uint32_t)unique_id;

        return 0;
}

static const struct dt_builder __ap_context_dt_builder[] = {
	DT_BUILDER(__ap_context_parse_dt_name),
	DT_BUILDER(__ap_context_parse_dt_unique_id),
};

static noinline int __ap_context_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __ap_context_dt_builder,
			ARRAY_SIZE(__ap_context_dt_builder));
}

static noinline int __ap_context_alloc_client(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	size_t size = sizeof(struct sec_riscv64_ap_context) * num_possible_cpus();
	struct sec_dbg_region_client *client;
	ssize_t type;

	type = __ap_context_unique_id_to_type(drvdata->unique_id);
	if (type >= TYPE_VH_MAX || type == TYPE_VH_UNKNOWN)
		return -ERANGE;

	if (ap_context[type])
		return -EBUSY;

	client = sec_dbg_region_alloc(drvdata->unique_id, size);
	if (PTR_ERR(client) == -EBUSY)
		return -EPROBE_DEFER;
	else if (IS_ERR_OR_NULL(client))
		return -ENOMEM;

	client->name = drvdata->name;
	drvdata->client = client;
	drvdata->ctx = (struct sec_riscv64_ap_context *)client->virt;

	ap_context[type] = drvdata->ctx;

	return 0;
}

static noinline void __ap_context_free_client(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	ssize_t type;

	type = __ap_context_unique_id_to_type(drvdata->unique_id);
	BUG_ON(type < 0 || type >= TYPE_VH_MAX);

	ap_context[type] = NULL;

	sec_dbg_region_free(drvdata->client);
}

static void __trace_android_vh_ipi_stop(void *unused, struct pt_regs *regs)
{
	struct sec_riscv64_ap_context *__ctx = ap_context[TYPE_VH_IPI_STOP];
	int cpu = smp_processor_id();
	struct sec_riscv64_ap_context *ctx = &__ctx[cpu];

	if (ctx->used)
		return;

	__ap_context_save_core_regs_from_pt_regs(ctx, regs);
	__ap_context_save_csr_regs(ctx);

	ctx->used = true;

	pr_emerg("context saved (CPU:%d)\n", cpu);
}

static noinline int __ap_context_register_vh(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	ssize_t type;
	int err;

	type = __ap_context_unique_id_to_type(drvdata->unique_id);
	if (type >= TYPE_VH_MAX || type == TYPE_VH_UNKNOWN)
		return -ERANGE;

	switch (type) {
	case TYPE_VH_IPI_STOP:
		err = register_trace_android_vh_ipi_stop(
				__trace_android_vh_ipi_stop, NULL);
		break;
	default:
		err = -EINVAL;
	}

	return err;
}

static noinline void __ap_context_unregister_vh(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	struct device *dev = bd->dev;
	ssize_t type;

	type = __ap_context_unique_id_to_type(drvdata->unique_id);
	if (type >= TYPE_VH_MAX || type == TYPE_VH_UNKNOWN) {
		dev_warn(dev, "invalid type number - %zd\n", type);
		return;
	}

	switch (type) {
	case TYPE_VH_IPI_STOP:
		unregister_trace_android_vh_ipi_stop(
				__trace_android_vh_ipi_stop, NULL);
		break;
	default:
		dev_warn(dev, "%zd is not a valid vendor hook\n", type);
	}
}

static __always_inline void __ap_context_hack_core_regs_for_panic(
		struct pt_regs *regs)
{
	uint16_t op_code_2bit_0;
	uint16_t op_code_2bit_2;

	regs->sp = (uintptr_t)__builtin_frame_address(4);
	regs->s0 = regs->sp;

	regs->ra = (uintptr_t)__builtin_return_address(3);
	op_code_2bit_2 = (*(uint16_t *)(regs->ra - 0x4)) & 0x3;
	op_code_2bit_0 = (*(uint16_t *)(regs->ra - 0x2)) & 0x3;
	if ((op_code_2bit_2 != 0x3) && (op_code_2bit_0 == 0x2))
		regs->ra -= 0x2;	/* compressed instruction */
	else
		regs->ra -= 0x4;	/* 32-bit instruction */

	regs->epc = regs->ra;
}

static int __used __sec_riscv64_ap_context_on_panic(struct pt_regs *regs)
{
	/* NOTE: a0 MUST BE SAVED before this function is called.
	 * see, 'sec_riscv64_ap_context_on_panic'.
	 */
	struct notifier_block *this = (void *)regs->a0;
	struct ap_context_drvdata *drvdata =
			container_of(this, struct ap_context_drvdata, nb_panic);
	struct sec_riscv64_ap_context *__ctx = drvdata->ctx;
	struct sec_riscv64_ap_context *ctx;
	int cpu;

	if (!__ctx)
		return NOTIFY_DONE;

	cpu = smp_processor_id();
	ctx = &__ctx[cpu];

	if (ctx->used)
		return NOTIFY_DONE;

	__ap_context_hack_core_regs_for_panic(regs);
	__ap_context_save_core_regs_from_pt_regs(ctx, regs);
	__ap_context_save_core_extra_regs(ctx);
	__ap_context_save_csr_regs(ctx);

	ctx->used = true;

	pr_emerg("context saved (CPU:%d)\n", cpu);

	return NOTIFY_OK;
}

static int __naked sec_riscv64_ap_context_on_panic(struct notifier_block *nb,
		unsigned long l, void *d)
{
	asm volatile (
		"addi	sp,sp,-0x10 \n\t"
		"sd	ra,0x8(sp) \n\t"
		"sd	a0,0x0(sp) \n\t"

		/* 'sp' indicates 'struct pt_regs' */
		"addi	sp,sp,-%0 \n\t"
		"mv	a0,sp \n\t"
		"jal	__ap_context_save_core_regs_on_current \n\t"

		/* save 'a0' on 'struct pt_regs' before calling
		 * '__sec_riscv64_ap_context_on_panic'
		 */
		"ld	a0,%0(sp) \n\t"
		"sd	a0,%1(sp) \n\n"

		/* concrete notifier */
		"mv	a0,sp \n\t"
		"jal	__sec_riscv64_ap_context_on_panic \n\t"

		"addi	sp,sp,%0 \n\t"
		"ld	ra,0x8(sp) \n\t"
		"addi	sp,sp,0x10 \n\t"
		"ret \n\t"
		:
		: "i"(sizeof(struct pt_regs)),
		  "i"(offsetof(struct pt_regs, a0))
		:
	);
}

static int __ap_context_register_panic_notifier(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	struct notifier_block *nb = &drvdata->nb_panic;

	nb->notifier_call = sec_riscv64_ap_context_on_panic;
	nb->priority = 0x7FFFFFFF;

	return atomic_notifier_chain_register(&panic_notifier_list, nb);
}

static void __ap_context_unregister_panic_notifier(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	struct notifier_block *nb = &drvdata->nb_panic;

	atomic_notifier_chain_unregister(&panic_notifier_list, nb);
}

static int sec_riscv64_ap_context_on_die(struct notifier_block *this,
		unsigned long l, void *data)
{
	struct ap_context_drvdata *drvdata =
			container_of(this, struct ap_context_drvdata, nb_die);
	struct die_args *args = data;
	struct pt_regs *regs = args->regs;
	struct sec_riscv64_ap_context *__ctx = drvdata->ctx;
	struct sec_riscv64_ap_context *ctx;
	int cpu;

	if (!__ctx)
		return NOTIFY_DONE;

	cpu = smp_processor_id();
	ctx = &__ctx[cpu];

	if (ctx->used)
		return NOTIFY_DONE;

	__ap_context_save_core_regs_from_pt_regs(ctx, regs);
	__ap_context_save_csr_regs(ctx);

	ctx->used = true;

	pr_emerg("context saved (CPU:%d)\n", cpu);

	return NOTIFY_OK;
}

static int __ap_context_register_die_notifier(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	struct notifier_block *nb = &drvdata->nb_die;

	nb->notifier_call = sec_riscv64_ap_context_on_die;

	return register_die_notifier(nb);
}

static void __ap_context_unregister_die_notifier(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	struct notifier_block *nb = &drvdata->nb_die;

	unregister_die_notifier(nb);
}

static noinline int __ap_context_probe_epilog(struct builder *bd)
{
	struct ap_context_drvdata *drvdata =
			container_of(bd, struct ap_context_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static int __ap_context_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct ap_context_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __ap_context_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct ap_context_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __ap_context_dev_builder[] = {
	DEVICE_BUILDER(__ap_context_parse_dt, NULL),
	DEVICE_BUILDER(__ap_context_alloc_client, __ap_context_free_client),
	DEVICE_BUILDER(__ap_context_register_vh, __ap_context_unregister_vh),
	DEVICE_BUILDER(__ap_context_register_panic_notifier,
		       __ap_context_unregister_panic_notifier),
	DEVICE_BUILDER(__ap_context_register_die_notifier,
		       __ap_context_unregister_die_notifier),
	DEVICE_BUILDER(__ap_context_probe_epilog, NULL),
};

static int sec_ap_context_probe(struct platform_device *pdev)
{
	return __ap_context_probe(pdev, __ap_context_dev_builder,
			ARRAY_SIZE(__ap_context_dev_builder));
}

static int sec_ap_context_remove(struct platform_device *pdev)
{
	return __ap_context_remove(pdev, __ap_context_dev_builder,
			ARRAY_SIZE(__ap_context_dev_builder));
}

static const struct of_device_id sec_ap_context_match_table[] = {
	{ .compatible = "samsung,riscv64-ap_context" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_ap_context_match_table);

static struct platform_driver sec_ap_context_driver = {
	.driver = {
		.name = "sec,riscv64-ap_context",
		.of_match_table = of_match_ptr(sec_ap_context_match_table),
	},
	.probe = sec_ap_context_probe,
	.remove = sec_ap_context_remove,
};

static int __init sec_ap_context_init(void)
{
	return platform_driver_register(&sec_ap_context_driver);
}
arch_initcall(sec_ap_context_init);

static void __exit sec_ap_context_exit(void)
{
	platform_driver_unregister(&sec_ap_context_driver);
}
module_exit(sec_ap_context_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("AP CORE/MMU context snaphot (RISC-V)");
MODULE_LICENSE("GPL v2");
