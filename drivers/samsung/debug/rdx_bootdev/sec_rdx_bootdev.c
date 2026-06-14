// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#include <uapi/linux/fiemap.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_debug.h>

struct rdx_bootdev_drvdata {
	struct builder bd;
	struct resource res;
	struct mutex lock;
	phys_addr_t paddr;
	phys_addr_t size;
	struct proc_dir_entry *proc;
};

static noinline int __rdx_bootdev_parse_dt_memory_region(struct builder *bd,
		struct device_node *np)
{
	struct rdx_bootdev_drvdata *drvdata =
			container_of(bd, struct rdx_bootdev_drvdata, bd);
	struct device_node *mem_np;
	int err;

	mem_np = of_parse_phandle(np, "memory-region", 0);
	if (!mem_np)
		return -EINVAL;

	err = of_address_to_resource(mem_np, 0, &drvdata->res);
	if (err)
		return err;

	drvdata->paddr = (phys_addr_t)drvdata->res.start;
	drvdata->size = (phys_addr_t)resource_size(&drvdata->res);

	return 0;
}

static const struct dt_builder __rdx_bootdev_dt_builder[] = {
	DT_BUILDER(__rdx_bootdev_parse_dt_memory_region),
};

static noinline int __rdx_bootdev_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __rdx_bootdev_dt_builder,
			ARRAY_SIZE(__rdx_bootdev_dt_builder));
}

#if IS_BUILTIN(CONFIG_SEC_RDX_BOOTDEV)
static __always_inline unsigned long __free_reserved_area(void *start, void *end, int poison, const char *s)
{
	return free_reserved_area(start, end, poison, s);
}
#else
/* FIXME: this is a copy of 'free_reserved_area' of 'page_alloc.c' */
static unsigned long __free_reserved_area(void *start, void *end, int poison, const char *s)
{
	void *pos;
	unsigned long pages = 0;

	start = (void *)PAGE_ALIGN((unsigned long)start);
	end = (void *)((unsigned long)end & PAGE_MASK);
	for (pos = start; pos < end; pos += PAGE_SIZE, pages++) {
		struct page *page = virt_to_page(pos);
		void *direct_map_addr;

		direct_map_addr = page_address(page);

		direct_map_addr = kasan_reset_tag(direct_map_addr);
		if ((unsigned int)poison <= 0xFF)
			memset(direct_map_addr, poison, PAGE_SIZE);

		free_reserved_page(page);
	}

	if (pages && s)
		pr_info("Freeing %s memory: %ldK\n",
			s, pages << (PAGE_SHIFT - 10));

	return pages;
}
#endif

static inline void ____rdx_bootdev_free(
		struct rdx_bootdev_drvdata *drvdata,
		phys_addr_t paddr, phys_addr_t size)
{
	uint8_t *vaddr = (uint8_t *)phys_to_virt(paddr);

	memset(vaddr, 0x00, size);

	__free_reserved_area(vaddr, vaddr + size, -1, "rdx_bootdev");

	if (drvdata->paddr == paddr)
		drvdata->paddr = 0;

	drvdata->size -= size;
}

static void __rdx_bootdev_free(struct rdx_bootdev_drvdata *drvdata,
		phys_addr_t paddr, phys_addr_t size)
{
	struct device *dev = drvdata->bd.dev;

	if (!drvdata->paddr) {
		dev_warn(dev, "reserved address is NULL\n");
		return;
	}

	if (!drvdata->size) {
		dev_warn(dev, "reserved size is zero\n");
		return;
	}

	if (paddr < drvdata->paddr) {
		dev_warn(dev, "paddr is not a valid reserved address\n");
		return;
	}

	if ((paddr + size) > (drvdata->paddr + drvdata->size)) {
		dev_warn(dev, "invalid reserved memory size\n");
		return;
	}

	____rdx_bootdev_free(drvdata, paddr, size);
}

static ssize_t __rdx_bootdev_free_entire(struct rdx_bootdev_drvdata *drvdata)
{
	__rdx_bootdev_free(drvdata, drvdata->paddr, drvdata->size);

	return -ENODEV;
}

#define SHA256_DIGEST_LENGTH	(256 / 8)

static ssize_t __rdx_bootdev_free_partial(struct rdx_bootdev_drvdata *drvdata,
		const char __user *buf, size_t count)
{
	struct device *dev = drvdata->bd.dev;
	struct {
		uint8_t sha256[SHA256_DIGEST_LENGTH];
		struct fiemap fiemap;
	} __packed *shared = phys_to_virt(drvdata->paddr);
	struct fiemap *pfiemap;
	phys_addr_t pa_to_be_freed;
	phys_addr_t sz_to_be_freed;
	phys_addr_t max_mapped_extents;

	if (copy_from_user(shared, buf, count)) {
		dev_warn(dev, "copy_from_user failed\n");
		return -EFAULT;
	}

	pfiemap = &shared->fiemap;

	max_mapped_extents = (drvdata->size - sizeof(*shared))
			/ sizeof(struct fiemap_extent);
	if (pfiemap->fm_mapped_extents > max_mapped_extents) {
		dev_warn(dev, "out of bound\n");
		return -ERANGE;
	}

	pa_to_be_freed = virt_to_phys(&pfiemap->fm_extents[0]);
	pa_to_be_freed += (pfiemap->fm_mapped_extents) * sizeof(struct fiemap_extent);
	pa_to_be_freed = ALIGN(pa_to_be_freed, PAGE_SIZE);

	sz_to_be_freed = drvdata->size -
			(pa_to_be_freed - drvdata->paddr);

	__rdx_bootdev_free(drvdata, pa_to_be_freed, sz_to_be_freed);

	return count;
}

static ssize_t __rdx_bootdev_drvdata_write(
		struct rdx_bootdev_drvdata *drvdata,
		const char __user *buf, size_t count)
{
	/* NOTE: according to the RDX protocol, free the etire reserved memroy */
	if (!buf && !count)
		return __rdx_bootdev_free_entire(drvdata);
	else
		return __rdx_bootdev_free_partial(drvdata, buf, count);
}

static ssize_t sec_rdx_bootdev_proc_write(struct file *file,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct rdx_bootdev_drvdata *drvdata = pde_data(file_inode(file));
	struct device *dev = drvdata->bd.dev;
	ssize_t ret = -ERANGE;

	mutex_lock(&drvdata->lock);

	if (!drvdata->paddr) {
		dev_warn(dev, "paddr is NULL\n");
		ret = -EFAULT;
		goto nothing_to_do;
	}

	if (count > drvdata->size) {
		dev_warn(dev, "size is wrong (%zu > %llu)\n", count,
				(unsigned long long)drvdata->size);
		ret = -EINVAL;
		goto nothing_to_do;
	}

	ret = __rdx_bootdev_drvdata_write(drvdata, buf, count);

nothing_to_do:
	mutex_unlock(&drvdata->lock);
	return ret;
}

static const struct proc_ops rdx_bootdev_pops = {
	.proc_write = sec_rdx_bootdev_proc_write,
};

static noinline int __rdx_bootdev_test_sec_debug(struct builder *bd)
{
	struct rdx_bootdev_drvdata *drvdata =
			container_of(bd, struct rdx_bootdev_drvdata, bd);
	struct device *dev = bd->dev;

	if (!sec_debug_is_enabled()) {
		dev_info(dev, "sec_debug is not enabled\n");
		__rdx_bootdev_free_entire(drvdata);
		/* NOTE: keepgoing. create dummy proc nodes */
	}

	return 0;
}

static int __rdx_bootdev_proc_init(struct builder *bd)
{
	struct rdx_bootdev_drvdata *drvdata =
			container_of(bd, struct rdx_bootdev_drvdata, bd);
	struct device *dev = bd->dev;
	const char *node_name = "rdx_bootdev";

	drvdata->proc = proc_create_data(node_name, 0220, NULL,
			&rdx_bootdev_pops, drvdata);
	if (!drvdata->proc) {
		dev_err(dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	return 0;
}

static void __rdx_bootdev_proc_exit(struct builder *bd)
{
	struct rdx_bootdev_drvdata *drvdata =
			container_of(bd, struct rdx_bootdev_drvdata, bd);

	proc_remove(drvdata->proc);
}

static noinline int __rdx_boodev_probe_epilog(struct builder *bd)
{
	struct rdx_bootdev_drvdata *drvdata =
			container_of(bd, struct rdx_bootdev_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static int __rdx_bootdev_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct rdx_bootdev_drvdata *drvdata;
	int err;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	err = sec_director_probe_dev(&drvdata->bd, builder, n);
	if (err)
		return err;

	return 0;
}

static int __rdx_bootdev_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct rdx_bootdev_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __rdx_bootdev_dev_builder[] = {
	DEVICE_BUILDER(__rdx_bootdev_parse_dt, NULL),
	DEVICE_BUILDER(__rdx_bootdev_test_sec_debug, NULL),
	DEVICE_BUILDER(__rdx_bootdev_proc_init,
		       __rdx_bootdev_proc_exit),
	DEVICE_BUILDER(__rdx_boodev_probe_epilog, NULL),
};

static int sec_rdx_bootdev_probe(struct platform_device *pdev)
{
	return __rdx_bootdev_probe(pdev, __rdx_bootdev_dev_builder,
			ARRAY_SIZE(__rdx_bootdev_dev_builder));
}

static int sec_rdx_bootdev_remove(struct platform_device *pdev)
{
	return __rdx_bootdev_remove(pdev, __rdx_bootdev_dev_builder,
			ARRAY_SIZE(__rdx_bootdev_dev_builder));
}

static const struct of_device_id sec_rdx_bootdev_match_table[] = {
	{ .compatible = "samsung,rdx_bootdev" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_rdx_bootdev_match_table);

static struct platform_driver sec_rdx_bootdev_driver = {
	.driver = {
		.name = "sec,rdx_bootdev",
		.of_match_table = of_match_ptr(sec_rdx_bootdev_match_table),
	},
	.probe = sec_rdx_bootdev_probe,
	.remove = sec_rdx_bootdev_remove,
};

static __init int sec_rdx_bootdev_init(void)
{
	return platform_driver_register(&sec_rdx_bootdev_driver);
}
module_init(sec_rdx_bootdev_init);

static __exit void sec_rdx_bootdev_exit(void)
{
	platform_driver_unregister(&sec_rdx_bootdev_driver);
}
module_exit(sec_rdx_bootdev_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("RDX Boot-Dev driver for Samsung Android devices");
MODULE_LICENSE("GPL v2");
