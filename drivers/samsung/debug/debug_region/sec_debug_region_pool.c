// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include "sec_debug_region.h"

static bool __dbg_region_pool_sanity_check(const struct dbg_region_pool *pool)
{
	return !(!pool->probe || !pool->remove || !pool->alloc || !pool->free);
}

static const struct dbg_region_pool *
		__dbg_region_pool_creator(struct dbg_region_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	unsigned int rmem_type = drvdata->rmem_type;
	const struct dbg_region_pool *pool;

	switch (rmem_type) {
	case RMEM_TYPE_NOMAP:
	case RMEM_TYPE_MAPPED:
		pool = __dbg_region_gen_pool_creator();
		break;
	case RMEM_TYPE_REUSABLE:
		pool = __dbg_region_cma_pool_creator();
		break;
	case RMEM_TYPE_SLAB:
		pool = __dbg_region_slab_pool_creator();
		break;
	default:
		dev_warn(dev, "%u is not a supported or deprecated rmem-type\n",
				rmem_type);
		pool = ERR_PTR(-ENOENT);
	}

	if (!__dbg_region_pool_sanity_check(pool))
		return ERR_PTR(-EINVAL);

	return pool;
}

static void __dbg_region_pool_factory(struct dbg_region_drvdata *drvdata)
{
	drvdata->pool = __dbg_region_pool_creator(drvdata);
}

int __dbg_region_pool_init(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);

	__dbg_region_pool_factory(drvdata);
	if (IS_ERR_OR_NULL(drvdata->pool))
		return -ENODEV;

	drvdata->pool->probe(drvdata);

	return 0;
}

void __dbg_region_pool_exit(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);

	drvdata->pool->remove(drvdata);
}
