// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2017-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/genalloc.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/log2.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>

#include <linux/samsung/sec_of.h>
#include <linux/samsung/debug/sec_debug.h>
#include <linux/samsung/debug/sec_debug_region.h>

#include "sec_debug_region.h"

static struct dbg_region_drvdata *dbg_region;

static __always_inline bool __dbg_region_is_probed(void)
{
	return !!dbg_region;
}

static void *__dbg_region_alloc(struct dbg_region_drvdata *drvdata,
		size_t size, phys_addr_t *phys)
{
	const struct dbg_region_pool *pool = drvdata->pool;

	return pool->alloc(drvdata, size, phys);
}

static void __dbg_region_free(struct dbg_region_drvdata *drvdata,
		size_t size, void *virt, phys_addr_t phys)
{
	const struct dbg_region_pool *pool = drvdata->pool;

	return pool->free(drvdata, size, virt, phys);
}

static struct sec_dbg_region_client *__dbg_region_find_locked(
		struct dbg_region_drvdata *drvdata, uint32_t unique_id)
{
	struct sec_dbg_region_client *client;

	list_for_each_entry(client, &drvdata->root->clients, list) {
		if (client->unique_id == unique_id)
			return client;
	}

	return ERR_PTR(-ENOENT);
}

static struct sec_dbg_region_client *__dbg_region_alloc_client_data(
		struct dbg_region_drvdata *drvdata)
{
	struct sec_dbg_region_client *client;
	phys_addr_t __client;

	client = __dbg_region_alloc(drvdata,
			sizeof(struct sec_dbg_region_client), &__client);
	if (IS_ERR_OR_NULL(client))
		return ERR_PTR(-ENOMEM);

	client->__client = __client;

	return client;
}

static void __dbg_region_free_client_data(struct dbg_region_drvdata *drvdata,
		struct sec_dbg_region_client *client)
{
	__dbg_region_free(drvdata, sizeof(*client), client, client->__client);
}

static void *__dbg_region_alloc_client_memory(struct dbg_region_drvdata *drvdata,
		size_t size, phys_addr_t *phys)
{
	void *virt;

	virt = __dbg_region_alloc(drvdata, size, phys);
	if (IS_ERR_OR_NULL(virt))
		return ERR_PTR(-ENOMEM);

	memset_io(virt, 0x0, size);

	return virt;
}

static void __dbg_region_free_client_memory(struct dbg_region_drvdata *drvdata,
		struct sec_dbg_region_client *client)
{
	__dbg_region_free(drvdata, client->size,
			(void *)client->virt,  client->phys);
}

static inline struct sec_dbg_region_client *__dbg_region_alloc_client(
		struct dbg_region_drvdata *drvdata,
		uint32_t unique_id, size_t size)
{
	struct sec_dbg_region_client *client;
	void *virt;
	int err;

	mutex_lock(&drvdata->lock);

	client = __dbg_region_find_locked(drvdata, unique_id);
	if (!IS_ERR(client)) {
		err = -EINVAL;
		goto err_duplicated_unique_id;
	}

	client = __dbg_region_alloc_client_data(drvdata);
	if (IS_ERR_OR_NULL(client)) {
		err = PTR_ERR(client);
		goto err_client_data;
	}

	virt = __dbg_region_alloc_client_memory(drvdata, size, &client->phys);
	if (IS_ERR_OR_NULL(virt)) {
		err = -ENOMEM;
		goto err_client_memory;
	}

	client->virt = (unsigned long)virt;
	client->magic = SEC_DBG_REGION_CLIENT_MAGIC;
	client->unique_id = unique_id;
	client->size = size;
	client->name = NULL;	/* optional for each client */

	list_add(&client->list, &drvdata->root->clients);

	mutex_unlock(&drvdata->lock);
	return client;

err_client_memory:
	__dbg_region_free_client_data(drvdata, client);
err_client_data:
err_duplicated_unique_id:
	mutex_unlock(&drvdata->lock);
	return ERR_PTR(err);
}

struct sec_dbg_region_client *sec_dbg_region_alloc(uint32_t unique_id,
		size_t size)
{
	if (!__dbg_region_is_probed())
		return ERR_PTR(-EBUSY);

	return __dbg_region_alloc_client(dbg_region, unique_id, size);
}
EXPORT_SYMBOL_GPL(sec_dbg_region_alloc);

static inline int __dbg_region_free_client(struct dbg_region_drvdata *drvdata,
		struct sec_dbg_region_client *client)
{
	mutex_lock(&drvdata->lock);

	if (client->magic != SEC_DBG_REGION_CLIENT_MAGIC) {
		mutex_unlock(&drvdata->lock);
		/* NOTE: should be detected during developing */
		BUG_ON(sec_debug_is_enabled());
	} else {
		list_del(&client->list);
		client->magic = 0x0;
		mutex_unlock(&drvdata->lock);

		__dbg_region_free_client_memory(drvdata, client);
		__dbg_region_free_client_data(drvdata, client);
	}

	return 0;
}

int sec_dbg_region_free(struct sec_dbg_region_client *client)
{
	if (!__dbg_region_is_probed())
		return -EBUSY;

	return __dbg_region_free_client(dbg_region, client);
}
EXPORT_SYMBOL_GPL(sec_dbg_region_free);

static inline const struct sec_dbg_region_client *__dbg_region_find(
		struct dbg_region_drvdata *drvdata, uint32_t unique_id)
{
	const struct sec_dbg_region_client *client;

	mutex_lock(&drvdata->lock);
	client = __dbg_region_find_locked(drvdata, unique_id);
	mutex_unlock(&drvdata->lock);

	return client;
}

const struct sec_dbg_region_client *sec_dbg_region_find(uint32_t unique_id)
{
	if (!__dbg_region_is_probed())
		return ERR_PTR(-EBUSY);

	return __dbg_region_find(dbg_region, unique_id);
}
EXPORT_SYMBOL_GPL(sec_dbg_region_find);

static noinline int __dbg_region_parse_dt_memory_region(struct builder *bd,
		struct device_node *np)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);
	struct device *dev = bd->dev;
	struct device_node *mem_np;
	struct reserved_mem *rmem;

	mem_np = of_parse_phandle(np, "memory-region", 0);
	if (!mem_np)
		return -EINVAL;

	rmem = of_reserved_mem_lookup(mem_np);
	if (!rmem) {
		dev_warn(dev, "failed to get a reserved memory (%s)\n",
				mem_np->name);
		return -EFAULT;
	}

	drvdata->rmem = rmem;

	return 0;
}

static bool __dbg_region_is_in_reserved_mem_bound(
		const struct reserved_mem *rmem,
		phys_addr_t base, phys_addr_t size)
{
	phys_addr_t rmem_base = rmem->base;
	phys_addr_t rmem_end = rmem_base + rmem->size - 1;
	phys_addr_t end = base + size - 1;

	if ((base >= rmem_base) && (end <= rmem_end))
		return true;

	return false;
}

static int __dbg_region_use_partial_reserved_mem(
	struct dbg_region_drvdata *drvdata, struct device_node *np)
{
	struct reserved_mem *rmem = drvdata->rmem;
	phys_addr_t base;
	phys_addr_t size;
	int err;

	err = sec_of_parse_reg_prop(np, &base, &size);
	if (err)
		return err;

	if (!__dbg_region_is_in_reserved_mem_bound(rmem, base, size))
		return -ERANGE;

	drvdata->phys = base;
	drvdata->size = size;

	return 0;
}

static int __dbg_region_use_entire_reserved_mem(
	struct dbg_region_drvdata *drvdata)
{
	struct reserved_mem *rmem = drvdata->rmem;

	drvdata->phys = rmem->base;
	drvdata->size = rmem->size;

	return 0;
}

static noinline int __dbg_region_parse_dt_partial_reserved_mem(struct builder *bd,
		struct device_node *np)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);
	int err;

	if (of_property_read_bool(np, "sec,use-partial_reserved_mem"))
		err = __dbg_region_use_partial_reserved_mem(drvdata, np);
	else
		err = __dbg_region_use_entire_reserved_mem(drvdata);

	if (err)
		return -EFAULT;

	return 0;
}

static noinline int __dbg_region_parse_dt_set_rmem_type(struct builder *bd,
		struct device_node *np)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);
	struct device_node *mem_np;

	mem_np = of_parse_phandle(np, "memory-region", 0);
	if (!mem_np)
		return -EINVAL;

	if (of_property_read_bool(mem_np, "sec,type-slab"))
		drvdata->rmem_type = RMEM_TYPE_SLAB;
	else if (of_property_read_bool(mem_np, "no-map"))
		drvdata->rmem_type = RMEM_TYPE_NOMAP;
	else if (of_property_read_bool(mem_np, "reusable"))
		drvdata->rmem_type = RMEM_TYPE_REUSABLE;
	else
		drvdata->rmem_type = RMEM_TYPE_MAPPED;

	return 0;
}

static const struct dt_builder __dbg_region_dt_builder[] = {
	DT_BUILDER(__dbg_region_parse_dt_memory_region),
	DT_BUILDER(__dbg_region_parse_dt_partial_reserved_mem),
	DT_BUILDER(__dbg_region_parse_dt_set_rmem_type),
};

static noinline int __dbg_region_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __dbg_region_dt_builder,
			ARRAY_SIZE(__dbg_region_dt_builder));
}

static noinline int __dbg_region_probe_prolog(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);

	mutex_init(&drvdata->lock);

	return 0;
}

static noinline void __dbg_region_remove_epilog(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);

	mutex_destroy(&drvdata->lock);
}

static noinline int __dbg_region_create_root(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);
	struct dbg_region_root *root;
	phys_addr_t __root;

	if (drvdata->rmem_type != RMEM_TYPE_SLAB)
		root = __dbg_region_alloc(drvdata,
				sizeof(struct dbg_region_root), &__root);
	else {
		root = phys_to_virt(drvdata->phys);
		__root = drvdata->phys;
	}
	pr_debug("root = %px\n", root);
	if (IS_ERR_OR_NULL(root))
		return -ENOMEM;

	root->__root = __root;
	root->magic = SEC_DBG_REGION_ROOT_MAGIC;
	INIT_LIST_HEAD(&root->clients);

	drvdata->root = root;

	return 0;
}

static noinline void __dbg_region_delete_root(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);
	struct dbg_region_root *root = drvdata->root;

	if (drvdata->rmem_type != RMEM_TYPE_SLAB)
		__dbg_region_free(drvdata, sizeof(struct dbg_region_root),
				root, root->__root);
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int sec_dbg_region_dbgfs_show_all(struct seq_file *m, void *unsed)
{
	struct dbg_region_drvdata *drvdata = m->private;
	struct dbg_region_root *root = drvdata->root;
	const struct sec_dbg_region_client *client;
	size_t sz_used = sizeof(*root);

	seq_printf(m, "%pa++%pa - %s\n",
			&drvdata->phys, &drvdata->size,
			dev_name(drvdata->bd.dev));

	seq_puts(m, "\nclients:\n");

	mutex_lock(&drvdata->lock);
	list_for_each_entry(client, &root->clients, list) {
		seq_printf(m, "%pa++%pa %7zu - %s (0x%08X)\n",
				&client->phys, &client->size, client->size,
				client->name, client->unique_id);
		sz_used += sizeof(*client);
		sz_used += client->size;
	}
	mutex_unlock(&drvdata->lock);

	seq_puts(m, "\n");
	seq_printf(m, " - Total : %zu\n", drvdata->size);
	seq_printf(m, " - Used  : %zu\n", sz_used);
	seq_puts(m, "\n");

	return 0;
}

static int sec_dbg_region_dbgfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_dbg_region_dbgfs_show_all,
			inode->i_private);
}

static const struct file_operations sec_dbg_region_dgbfs_fops = {
	.open = sec_dbg_region_dbgfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int __dbg_region_debugfs_create(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);

	drvdata->dbgfs = debugfs_create_file("sec_debug_region", 0440,
			NULL, drvdata, &sec_dbg_region_dgbfs_fops);

	return 0;
}

static void __dbg_region_debugfs_remove(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);

	debugfs_remove(drvdata->dbgfs);
}
#else
static int __dbg_region_debugfs_create(struct builder *bd) { return 0; }
static void __dbg_region_debugfs_remove(struct builder *bd) {}
#endif

static noinline int __dbg_region_probe_epilog(struct builder *bd)
{
	struct dbg_region_drvdata *drvdata =
			container_of(bd, struct dbg_region_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	dbg_region = drvdata;	/* set a singleton */

	return 0;
}

static noinline void __dbg_region_remove_prolog(struct builder *bd)
{
	/* FIXME: This is not a graceful exit. */
	dbg_region = NULL;
}

static int __dbg_region_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct dbg_region_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __dbg_region_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct dbg_region_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __dbg_region_dev_builder[] = {
	DEVICE_BUILDER(__dbg_region_parse_dt, NULL),
	DEVICE_BUILDER(__dbg_region_probe_prolog, __dbg_region_remove_epilog),
	DEVICE_BUILDER(__dbg_region_pool_init, __dbg_region_pool_exit),
	DEVICE_BUILDER(__dbg_region_create_root, __dbg_region_delete_root),
	DEVICE_BUILDER(__dbg_region_debugfs_create,
		       __dbg_region_debugfs_remove),
	DEVICE_BUILDER(__dbg_region_probe_epilog, __dbg_region_remove_prolog),
};

static int sec_dbg_region_probe(struct platform_device *pdev)
{
	return __dbg_region_probe(pdev, __dbg_region_dev_builder,
			ARRAY_SIZE(__dbg_region_dev_builder));
}

static int sec_dbg_region_remove(struct platform_device *pdev)
{
	return __dbg_region_remove(pdev, __dbg_region_dev_builder,
			ARRAY_SIZE(__dbg_region_dev_builder));
}

static const struct of_device_id sec_dbg_region_match_table[] = {
	{ .compatible = "samsung,debug_region" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_dbg_region_match_table);

static struct platform_driver sec_dbg_region_driver = {
	.driver = {
		.name = "sec,debug_region",
		.of_match_table = of_match_ptr(sec_dbg_region_match_table),
	},
	.probe = sec_dbg_region_probe,
	.remove = sec_dbg_region_remove,
};

static __init int sec_dbg_region_init(void)
{
	return platform_driver_register(&sec_dbg_region_driver);
}
core_initcall_sync(sec_dbg_region_init);

static __exit void sec_dbg_region_exit(void)
{
	platform_driver_unregister(&sec_dbg_region_driver);
}
module_exit(sec_dbg_region_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Memory pool for debugging features");
MODULE_LICENSE("GPL v2");
