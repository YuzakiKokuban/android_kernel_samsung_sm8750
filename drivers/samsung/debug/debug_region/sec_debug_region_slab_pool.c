// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2024 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/io.h>
#include <linux/slab.h>

#include "sec_debug_region.h"

static int dbg_region_slab_pool_probe(struct dbg_region_drvdata *drvdata)
{
	return 0;
}

static void dbg_region_slab_pool_remove(struct dbg_region_drvdata *drvdata)
{
}

static void *dbg_region_slab_pool_alloc(struct dbg_region_drvdata *drvdata,
		size_t size, phys_addr_t *__phys)
{
	void *vaddr;

	vaddr = kzalloc(size, GFP_KERNEL);
	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	*__phys = virt_to_phys(vaddr);

	return vaddr;
}

static void dbg_region_slab_pool_free(struct dbg_region_drvdata *drvdata,
		size_t size, void *vaddr, phys_addr_t phys)
{
	kfree(vaddr);
}

static const struct dbg_region_pool dbg_region_slab_pool = {
	.probe = dbg_region_slab_pool_probe,
	.remove = dbg_region_slab_pool_remove,
	.alloc = dbg_region_slab_pool_alloc,
	.free = dbg_region_slab_pool_free,
};

const struct dbg_region_pool *__dbg_region_slab_pool_creator(void)
{
	return &dbg_region_slab_pool;
}
