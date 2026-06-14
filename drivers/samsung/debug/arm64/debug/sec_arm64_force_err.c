// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

#include <linux/samsung/debug/sec_force_err.h>

#include "sec_arm64_debug.h"

static void __arm64_simulate_undef(struct force_err_handle *h)
{
	asm volatile(".word 0xDEADBEEF");
}

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

static noinline unsigned long __fsimd_random_pi(unsigned long seed)
{
	unsigned long pi_int;

	pi_int = (unsigned long)(M_PI * seed);

	return pi_int;
}

static void sec_arm64_fsimd_random_pi(struct work_struct *work)
{
	unsigned long pi_int, seed;
	size_t i;

	for (i = 0; i < 80; i++) {
		seed = get_random_long() % 100UL;
		pi_int = __fsimd_random_pi(seed);
		pr_info("int(M_PI * %lu) = %lu\n",
				seed, pi_int);
		msleep(20);
	}
}

static struct work_struct random_pi_work[10];

int sec_fsimd_debug_init_random_pi_work(struct builder *bd)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(random_pi_work); i++)
		INIT_WORK(&random_pi_work[i], sec_arm64_fsimd_random_pi);

	return 0;
}

static void __arm64_simulate_fsimd_error(struct force_err_handle *h)
{
	size_t i;

	pr_emerg("Simulating fsimd error in kernel space\n");

	for (i = 0; i < ARRAY_SIZE(random_pi_work); i++)
		queue_work(system_long_wq, &random_pi_work[i]);

	ssleep(1);

	/* if we reach here, simulation failed */
	pr_emerg("Simulation of fsimd error failed\n");
}

static void __naked __arm64_simulate_pabort(struct force_err_handle *h)
{
	asm volatile ("mov x0, %0 \n\t"
		      "blr x0\n\t"
		      "ret \n\t"
		      :: "r" (PAGE_OFFSET - 0x8));
}

static void __naked __arm64_simulate_unaligned_pc(struct force_err_handle *h)
{
	asm volatile ("mov x1, %0 \n\t"
		      "mov x0, x30 \n\t"
		      "add x0, x0, 0x1 \n\t"
		      "orr x0, x0, x1 \n\n"
		      "blr x0\n\t"
		      "ret \n\t"
		      :: "r" PAGE_OFFSET);
}

static void __naked __arm64_simulate_bti(struct force_err_handle *h)
{
	asm volatile ("adr x0, . \n\t"
		      "add x0, x0, 0x10 \n\t"
		      "br x0 \n\t"
		      "eor x0, x0, x0 \n\t"
		      "eor x0, x0, x0 \n\t"
		      "eor x0, x0, x0 \n\t"
		      "eor x0, x0, x0 \n\t"
		      "ret \n\t");
}

static struct force_err_handle __arm64_force_err_default[] = {
	FORCE_ERR_HANDLE("undef", "Generating a undefined instruction exception!",
			__arm64_simulate_undef),
	FORCE_ERR_HANDLE("fsimd_err", "Generating an fsimd error!",
			__arm64_simulate_fsimd_error),
	FORCE_ERR_HANDLE("pabort", "Generating a data abort exception!",
			__arm64_simulate_pabort),
	FORCE_ERR_HANDLE("unaligned_pc", "Generating an unaligned pc exception!",
			__arm64_simulate_unaligned_pc),
	FORCE_ERR_HANDLE("bti", "Generating an bti exception!",
			__arm64_simulate_bti),
};

static ssize_t __arm64_force_err_add_handlers(ssize_t begin)
{
	struct force_err_handle *h;
	int err = 0;
	ssize_t n = ARRAY_SIZE(__arm64_force_err_default);
	ssize_t i;

	for (i = begin; i < n; i++) {
		h = &__arm64_force_err_default[i];

		INIT_HLIST_NODE(&h->node);

		err = sec_force_err_add_custom_handle(h);
		if (err) {
			pr_err("failed to add a handler - [%zu] %ps (%d)\n",
					i, h->func, err);
			return -i;
		}
	}

	return n;
}

static void __arm64_force_err_del_handlers(ssize_t last_failed)
{
	struct force_err_handle *h;
	int err = 0;
	ssize_t n = ARRAY_SIZE(__arm64_force_err_default);
	ssize_t i;

	/* NOTE: force error is not enabled for end-users. */
	BUG_ON((last_failed < 0) || (last_failed > n));

	for (i = last_failed - 1; i >= 0; i--) {
		h = &__arm64_force_err_default[i];

		err = sec_force_err_del_custom_handle(h);
		if (err)
			pr_warn("failed to delete a handler - [%zu] %ps (%d)\n",
					i, h->func, err);
	}
}

int sec_arm64_force_err_init(struct builder *bd)
{
	ssize_t last_failed;

	last_failed = __arm64_force_err_add_handlers(0);
	if (last_failed <= 0) {
		dev_warn(bd->dev, "force err is disabled. ignored.\n");
		goto err_add_handlers;
	}

	return 0;

err_add_handlers:
	__arm64_force_err_del_handlers(-last_failed);
	return 0;
}

void sec_arm64_force_err_exit(struct builder *bd)
{
	__arm64_force_err_del_handlers(ARRAY_SIZE(__arm64_force_err_default));
}
