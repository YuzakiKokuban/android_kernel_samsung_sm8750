// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/samsung/debug/sec_force_err.h>

#include "sec_riscv64_debug.h"

static void __riscv64_simulate_undef(struct force_err_handle *h)
{
	asm volatile(".word 0x00000000");
}

static void __riscv64_simulate_pabort(struct force_err_handle *h)
{
	asm volatile ("mv x0, %0 \n\t"
		      "jalr x0\n\t"
		      :: "r" (PAGE_OFFSET - 0x8));
}

static struct force_err_handle __riscv64_force_err_default[] = {
	FORCE_ERR_HANDLE("undef", "Generating a undefined instruction exception!",
			__riscv64_simulate_undef),
	FORCE_ERR_HANDLE("pabort", "Generating a data abort exception!",
			__riscv64_simulate_pabort),
};

static ssize_t __riscv64_force_err_add_handlers(ssize_t begin)
{
	struct force_err_handle *h;
	int err = 0;
	ssize_t n = ARRAY_SIZE(__riscv64_force_err_default);
	ssize_t i;

	for (i = begin; i < n; i++) {
		h = &__riscv64_force_err_default[i];

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

static void __riscv64_force_err_del_handlers(ssize_t last_failed)
{
	struct force_err_handle *h;
	int err = 0;
	ssize_t n = ARRAY_SIZE(__riscv64_force_err_default);
	ssize_t i;

	BUG_ON((last_failed < 0) || (last_failed > n));

	for (i = last_failed - 1; i >= 0; i--) {
		h = &__riscv64_force_err_default[i];

		err = sec_force_err_del_custom_handle(h);
		if (err)
			pr_warn("failed to delete a handler - [%zu] %ps (%d)\n",
					i, h->func, err);
	}
}

int sec_riscv64_force_err_init(struct builder *bd)
{
	ssize_t last_failed;

	last_failed = __riscv64_force_err_add_handlers(0);
	if (last_failed <= 0) {
		dev_warn(bd->dev, "force err is disabled. ignored.\n");
		goto err_add_handlers;
	}

	return 0;

err_add_handlers:
	__riscv64_force_err_del_handlers(-last_failed);
	return 0;
}

void sec_riscv64_force_err_exit(struct builder *bd)
{
	__riscv64_force_err_del_handlers(ARRAY_SIZE(__riscv64_force_err_default));
}
