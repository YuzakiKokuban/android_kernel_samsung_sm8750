// SPDX-License-Identifier: GPL-2.0

#include <linux/libfdt.h>
#include <linux/of.h>
#include <linux/slab.h>

#include <linux/samsung/sec_kunit.h>
#include <linux/samsung/sec_of_kunit.h>

#include "of_private.h"

static int __of_kunit_prepare_device_tree(struct sec_of_kunit_data *testdata,
		const char *compatible, struct sec_of_dtb_info *info)
{
	if (!compatible || !info)
		return 0;

	testdata->root = sec_of_kunit_dtb_to_fdt(info);
	if (!testdata->root)
		return -ENODEV;

	testdata->of_node = of_find_compatible_node(testdata->root, NULL,
						    compatible);
	if (IS_ERR_OR_NULL(testdata->of_node))
		return -ENOENT;

	return 0;
}

int sec_of_kunit_data_init(struct sec_of_kunit_data *testdata,
		const char *name, struct builder *bd,
		const char *compatible, struct sec_of_dtb_info *info)
{
	struct miscdevice *misc = &testdata->misc;
	int err;

	err = sec_kunit_init_miscdevice(misc, name);
	if (err)
		return err;

	err = __of_kunit_prepare_device_tree(testdata, compatible, info);
	if (err) {
		sec_kunit_exit_miscdevice(misc);
		return err;
	}

	bd->dev = misc->this_device;
	testdata->bd = bd;

	return 0;
}
EXPORT_SYMBOL_GPL(sec_of_kunit_data_init);

void sec_of_kunit_data_exit(struct sec_of_kunit_data *testdata)
{
	sec_kunit_exit_miscdevice(&testdata->misc);
	kfree_sensitive(testdata->root);
}
EXPORT_SYMBOL_GPL(sec_of_kunit_data_exit);

static void *__dt_alloc(u64 __size, u64 align)
{
	u64 size = ALIGN(__size, align);
	void *ptr = kmalloc(size, GFP_KERNEL);

	if (!ptr)
		panic("%s: Failed to allocate %llu bytes align=0x%llx\n",
		      __func__, size, align);

	return ptr;
}

/* NOTE: Inspired from 'drivers/of/unittest.c'. */
struct device_node *sec_of_kunit_dtb_to_fdt(struct sec_of_dtb_info *info)
{
	struct device_node *root;
	u32 data_size;
	u32 size;

	data_size = info->dtb_end - info->dtb_begin;
	if (!data_size) {
		pr_err("No dtb 'overlay_base' to attach\n");
		return ERR_PTR(-ENOENT);
	}

	size = fdt_totalsize(info->dtb_begin);
	if (size != data_size) {
		pr_err("dtb 'overlay_base' header totalsize != actual size");
		return ERR_PTR(-EINVAL);
	}

	__unflatten_device_tree((void *)info->dtb_begin, NULL, &root,
				__dt_alloc, true);

	return root;
}
EXPORT_SYMBOL_GPL(sec_of_kunit_dtb_to_fdt);

int __init sec_of_kunit_init(void)
{
	return 0;
}
module_init(sec_of_kunit_init);

void __exit sec_of_kunit_exit(void)
{
}
module_exit(sec_of_kunit_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Samsung additional facilites for of / kunit testing");
MODULE_LICENSE("GPL v2");
