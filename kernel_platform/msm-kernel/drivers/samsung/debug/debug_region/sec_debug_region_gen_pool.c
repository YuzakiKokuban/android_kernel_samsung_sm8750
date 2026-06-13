// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/genalloc.h>
#include <linux/kernel.h>

#include "sec_debug_region.h"

static int __dbg_region_prepare_pool(struct dbg_region_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	unsigned int rmem_type = drvdata->rmem_type;
	void __iomem *virt;

	if (rmem_type == RMEM_TYPE_NOMAP)
		virt = devm_ioremap_wc(dev, drvdata->phys, drvdata->size);
	else
		virt = phys_to_virt(drvdata->phys);

	if (!virt)
		return -EFAULT;

	drvdata->virt = (unsigned long)virt;

	return 0;
}

static int __dbg_region_gen_pool_create(struct dbg_region_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	const int min_alloc_order = ilog2(cache_line_size());
	struct gen_pool *pool;
	int err;

	pool = devm_gen_pool_create(dev, min_alloc_order, -1, "sec_dbg_region");
	if (IS_ERR(pool)) {
		err = PTR_ERR(pool);
		goto err_gen_pool_create;
	}

	err = gen_pool_add_virt(pool, drvdata->virt, drvdata->phys,
			drvdata->size, -1);
	if (err) {
		err = -ENOMEM;
		goto err_gen_pool_add_virt;
	}

	drvdata->private = pool;

	return 0;

err_gen_pool_add_virt:
	gen_pool_destroy(pool);
err_gen_pool_create:
	return err;
}

static int dbg_region_gen_pool_probe(struct dbg_region_drvdata *drvdata)
{
	int err;

	err = __dbg_region_prepare_pool(drvdata);
	if (err)
		return err;

	err = __dbg_region_gen_pool_create(drvdata);
	if (err)
		return err;

	return 0;
}

static void dbg_region_gen_pool_remove(struct dbg_region_drvdata *drvdata)
{
	struct gen_pool *pool = drvdata->private;

	gen_pool_destroy(pool);
}

static void *dbg_region_gen_pool_alloc(struct dbg_region_drvdata *drvdata,
		size_t size, phys_addr_t *phys)
{
	struct gen_pool *pool = drvdata->private;
	unsigned long virt;

	virt = gen_pool_alloc(pool, size);
	if (!virt)
		return ERR_PTR(-ENOMEM);

	*phys = gen_pool_virt_to_phys(pool, virt);

	return (void *)virt;
}

static void dbg_region_gen_pool_free(struct dbg_region_drvdata *drvdata,
		size_t size, void *virt, phys_addr_t phys)
{
	struct gen_pool *pool = drvdata->private;

	gen_pool_free(pool, (unsigned long)virt, size);
}

static const struct dbg_region_pool dbg_region_gen_pool = {
	.probe = dbg_region_gen_pool_probe,
	.remove = dbg_region_gen_pool_remove,
	.alloc = dbg_region_gen_pool_alloc,
	.free = dbg_region_gen_pool_free,
};

const struct dbg_region_pool *__dbg_region_gen_pool_creator(void)
{
	return &dbg_region_gen_pool;
}
