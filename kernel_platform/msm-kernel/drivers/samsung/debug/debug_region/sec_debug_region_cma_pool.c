// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/of_reserved_mem.h>

#include "sec_debug_region.h"

static int dbg_region_cma_pool_probe(struct dbg_region_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	struct device_node *np = dev_of_node(dev);
	int err;

	err = of_reserved_mem_device_init_by_idx(dev, np, 0);
	if (err) {
		dev_err(dev, "failed to initialize reserved mem (%d)\n", err);
		return err;
	}

	return 0;
}

static void dbg_region_cma_pool_remove(struct dbg_region_drvdata *drvdata)
{
}

static void *dbg_region_cma_pool_alloc(struct dbg_region_drvdata *drvdata,
		size_t size, phys_addr_t *__phys)
{
	struct device *dev = drvdata->bd.dev;
	void *vaddr;
	dma_addr_t phys;

	vaddr = dma_alloc_wc(dev, PAGE_ALIGN(size), &phys, GFP_KERNEL);
	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	*__phys = (phys_addr_t)phys;

	return vaddr;
}

static void dbg_region_cma_pool_free(struct dbg_region_drvdata *drvdata,
		size_t size, void *vaddr, phys_addr_t phys)
{
	struct device *dev = drvdata->bd.dev;

	dma_free_wc(dev, PAGE_ALIGN(size), vaddr, (dma_addr_t)phys);
}

static const struct dbg_region_pool dbg_region_cma_pool = {
	.probe = dbg_region_cma_pool_probe,
	.remove = dbg_region_cma_pool_remove,
	.alloc = dbg_region_cma_pool_alloc,
	.free = dbg_region_cma_pool_free,
};

const struct dbg_region_pool *__dbg_region_cma_pool_creator(void)
{
	return &dbg_region_cma_pool;
}
