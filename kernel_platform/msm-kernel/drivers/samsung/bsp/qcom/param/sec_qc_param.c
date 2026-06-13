// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2011-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/uio.h>

#include <linux/samsung/sec_kunit.h>
#include <linux/samsung/bsp/sec_blk_helper.h>
#include <linux/samsung/bsp/sec_class.h>
#include <linux/samsung/debug/sec_debug.h>

#include <block/blk.h>
#include "sec_qc_param.h"

static struct qc_param_drvdata *qc_param;

static __always_inline bool __qc_param_is_probed(void)
{
	return !!qc_param;
}

__ss_static bool __qc_param_verify_debuglevel(const struct qc_param_info *info,
		const void *value)
{
	const unsigned int debuglevel = *(const unsigned int *)value;
	bool ret;

	switch (debuglevel) {
	case SEC_DEBUG_LEVEL_LOW:
	case SEC_DEBUG_LEVEL_MID:
	case SEC_DEBUG_LEVEL_HIGH:
		ret = true;
		break;
	default:
		ret = false;
		break;
	}

	return ret;
}

__ss_static bool __qc_param_verify_sapa(const struct qc_param_info *info,
		const void *value)
{
	const unsigned int sapa = *(const unsigned int *)value;

	if (sapa == SAPA_KPARAM_MAGIC || !sapa)
		return true;

	return false;
}

__ss_static bool __qc_param_verify_afc_disable(const struct qc_param_info *info,
		const void *value)
{
	const char mode = *(const char *)value;

	if (mode == '0' || mode == '1')
		return true;

	return false;
}

__ss_static bool __qc_param_verify_pd_disable(const struct qc_param_info *info,
		const void *value)
{
	const char mode = *(const char *)value;

	if (mode == '0' || mode == '1')
		return true;

	return false;
}

__ss_static bool __qc_param_verify_cp_reserved_mem(const struct qc_param_info *info,
		const void *value)
{
	const unsigned int cp_reserved_mem = *(const unsigned int *)value;
	bool ret;

	switch (cp_reserved_mem) {
	case CP_MEM_RESERVE_OFF:
	case CP_MEM_RESERVE_ON_1:
	case CP_MEM_RESERVE_ON_2:
		ret = true;
		break;
	default:
		ret = false;
		break;
	}

	return ret;
}

__ss_static bool __qc_param_verify_FMM_lock(const struct qc_param_info *info,
		const void *value)
{
	const unsigned int fmm_lock_magic = *(const unsigned int *)value;

	if (fmm_lock_magic == FMMLOCK_MAGIC_NUM || !fmm_lock_magic)
		return true;

	return false;
}

__ss_static bool __qc_param_verify_fiemap_update(const struct qc_param_info *info,
		const void *value)
{
	const unsigned int edtbo_fiemap_magic = *(const unsigned int *)value;

	if (edtbo_fiemap_magic == EDTBO_FIEMAP_MAGIC || !edtbo_fiemap_magic)
		return true;

	return false;
}

static const struct qc_param_info qc_param_info[] = {
	QC_PARAM_INFO(param_index_debuglevel, debuglevel, __qc_param_verify_debuglevel),
	QC_PARAM_INFO(param_index_uartsel, uartsel, NULL),
	QC_PARAM_INFO(param_index_product_device, product_device, NULL),
	QC_PARAM_INFO(param_rory_control, rory_control, NULL),
	QC_PARAM_INFO(param_cp_debuglevel, cp_debuglevel, NULL),
	QC_PARAM_INFO(param_index_sapa, sapa, __qc_param_verify_sapa),
	QC_PARAM_INFO(param_index_normal_poweroff, normal_poweroff, NULL),
	QC_PARAM_INFO(param_index_wireless_ic, wireless_ic, NULL),
	QC_PARAM_INFO(param_index_wireless_charging_mode, wireless_charging_mode, NULL),
	QC_PARAM_INFO(param_index_afc_disable, afc_disable, __qc_param_verify_afc_disable),
	QC_PARAM_INFO(param_index_cp_reserved_mem, cp_reserved_mem, __qc_param_verify_cp_reserved_mem),
	QC_PARAM_INFO(param_index_api_gpio_test, api_gpio_test, NULL),
	QC_PARAM_INFO(param_index_api_gpio_test_result, api_gpio_test_result, NULL),
	QC_PARAM_INFO(param_index_reboot_recovery_cause, reboot_recovery_cause, NULL),
	QC_PARAM_INFO(param_index_user_partition_flashed, user_partition_flashed, NULL),
	QC_PARAM_INFO(param_index_force_upload_flag, force_upload_flag, NULL),
	// FIXME: QC_PARAM_INFO(param_index_cp_reserved_mem_backup, cp_reserved_mem_backup, NULL),
	QC_PARAM_INFO(param_index_FMM_lock, FMM_lock, __qc_param_verify_FMM_lock),
	QC_PARAM_INFO(param_index_dump_sink, dump_sink, NULL),
	QC_PARAM_INFO(param_index_fiemap_update, fiemap_update, __qc_param_verify_fiemap_update),
	QC_PARAM_INFO(param_index_fiemap_result, fiemap_result, NULL),
	QC_PARAM_INFO(param_index_window_color, window_color, NULL),
	QC_PARAM_INFO(param_index_VrrStatus, VrrStatus, NULL),
	QC_PARAM_INFO(param_index_pd_hv_disable, pd_disable, __qc_param_verify_pd_disable),
	QC_PARAM_INFO(param_vib_le_est, vib_le_est, NULL),
};

static inline bool __qc_param_is_param_data(loff_t pos)
{
	if ((pos >= qc_param->offset) ||
	    (pos < qc_param->offset + sizeof(struct sec_qc_param_data)))
		return true;

	return false;
}

ssize_t sec_qc_param_read_raw(void *buf, size_t len, loff_t pos)
{
	if (!__qc_param_is_probed())
		return -EBUSY;

	if (__qc_param_is_param_data(pos))
		return -ENXIO;

	return sec_blk_read(qc_param->bdev, buf, len, pos);
}
EXPORT_SYMBOL_GPL(sec_qc_param_read_raw);

__ss_static bool __qc_param_is_valid_index(size_t index)
{
	size_t size;

	if (index >= ARRAY_SIZE(qc_param_info))
		return false;

	size = qc_param_info[index].size;
	if (!size)
		return false;

	return true;
}

static bool __qc_param_read(struct qc_param_drvdata *drvdata,
		size_t index, void *value)
{
	struct device *dev = drvdata->bd.dev;
	const struct qc_param_info *info;
	loff_t offset;
	ssize_t read;

	info = &qc_param_info[index];
	offset = info->offset + drvdata->offset;

	read = sec_blk_read(drvdata->bdev,
			value, info->size, offset);
	if (read < 0) {
		dev_warn(dev, "read failed (idx:%zu, err:%zd)\n", index, read);
		return false;
	} else if (read != info->size) {
		dev_warn(dev, "wrong size (idx:%zu)- requested(%zu) != read(%zd)\n",
				index, info->size, read);
		return false;
	}

	return true;
}

static bool sec_qc_param_read(size_t index, void *value)
{
	if (!__qc_param_is_probed())
		return false;

	if (!__qc_param_is_valid_index(index)) {
		dev_warn(qc_param->bd.dev, "invalid index (%zu)\n", index);
		return false;
	}

	return __qc_param_read(qc_param, index, value);
}

ssize_t sec_qc_param_write_raw(const void *buf, size_t len, loff_t pos)
{
	if (!__qc_param_is_probed())
		return -EBUSY;

	if (__qc_param_is_param_data(pos))
		return -ENXIO;

	return sec_blk_write(qc_param->bdev, buf, len, pos);
}
EXPORT_SYMBOL_GPL(sec_qc_param_write_raw);

static bool __qc_param_write(struct qc_param_drvdata *drvdata,
		size_t index, const void *value)
{
	struct device *dev = drvdata->bd.dev;
	loff_t offset;
	ssize_t written;
	const struct qc_param_info *info;

	info = &qc_param_info[index];
	offset = info->offset + drvdata->offset;

	if (info->verify_input && !info->verify_input(info, value)) {
		dev_warn(dev, "wrong data %pS\n",
				__builtin_return_address(0));
		print_hex_dump_bytes(" + ", DUMP_PREFIX_OFFSET,
				value, info->size);
		return false;
	}

	written = sec_blk_write(drvdata->bdev,
			value, info->size, offset);
	if (written < 0) {
		dev_warn(dev, "write failed (idx:%zu, err:%zd)\n",
				index, written);
		return false;
	} else if (written != info->size) {
		dev_warn(dev, "wrong size (idx:%zu) - requested(%zu) != written(%zd)\n",
				index, info->size, written);
		return false;
	}

	return true;
}

static bool sec_qc_param_write(size_t index, const void *value)
{
	if (!__qc_param_is_probed())
		return false;

	if (!__qc_param_is_valid_index(index)) {
		dev_warn(qc_param->bd.dev, "invalid index (%zu)\n", index);
		return false;
	}

	return __qc_param_write(qc_param, index, value);
}

__ss_static noinline int __qc_param_parse_dt_bdev_path(struct builder *bd,
		struct device_node *np)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);

	return of_property_read_string(np, "sec,bdev_path",
			&drvdata->bdev_path);
}

__ss_static noinline int __qc_param_parse_dt_negative_offset(struct builder *bd,
		struct device_node *np)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	u32 negative_offset;
	int err;

	err = of_property_read_u32(np, "sec,negative_offset", &negative_offset);
	if (err)
		return -EINVAL;

	drvdata->negative_offset = (loff_t)negative_offset;

	return 0;
}

static const struct dt_builder __qc_param_dt_builder[] = {
	DT_BUILDER(__qc_param_parse_dt_bdev_path),
	DT_BUILDER(__qc_param_parse_dt_negative_offset),
};

static noinline int __qc_param_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __qc_param_dt_builder,
			ARRAY_SIZE(__qc_param_dt_builder));
}

static int __qc_param_sec_class_create(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	struct device *param_dev;

	param_dev = sec_device_create(NULL, "sec_param");
	if (IS_ERR(param_dev))
		return PTR_ERR(param_dev);

	dev_set_drvdata(param_dev, drvdata);

	drvdata->param_dev = param_dev;

	return 0;
}

static void __qc_param_sec_class_remove(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	struct device *param_dev = drvdata->param_dev;

	if (!param_dev)
		return;

	sec_device_destroy(param_dev->devt);
}

static noinline int __qc_param_init_blkdev(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	struct device *dev = bd->dev;
	fmode_t mode = FMODE_READ | FMODE_WRITE;
	struct block_device *bdev;
	sector_t nr_sects;
	int err;

	bdev = blkdev_get_by_path(drvdata->bdev_path, mode, NULL, NULL);
	if (IS_ERR(bdev)) {
		dev_t devt;

		if (sec_devt_from_partuuid(drvdata->bdev_path, &devt)) {
			dev_warn(dev, "'sec_devt_from_partuuid' failed!\n");
			err = -EPROBE_DEFER;
			goto err_blkdev;
		}

		bdev = blkdev_get_by_dev(devt, mode, NULL, NULL);
		if (IS_ERR(bdev)) {
			dev_warn(dev, "'blkdev_get_by_dev' failed! (%ld)\n",
					PTR_ERR(bdev));
			err = -EPROBE_DEFER;
			goto err_blkdev;
		}
	}

	nr_sects = bdev_nr_sectors(bdev);
	if (!nr_sects) {
		dev_err(dev, "not enough space for %s\n",
				drvdata->bdev_path);
		blkdev_put(bdev, NULL);
		return -ENOSPC;
	}

	drvdata->bdev = bdev;
	drvdata->offset = (loff_t)(nr_sects << SECTOR_SHIFT)
			- drvdata->negative_offset;

	return 0;

err_blkdev:
	dev_err(dev, "can't find a block device - %s\n",
			drvdata->bdev_path);
	return err;
}

static noinline void __qc_param_exit_blkdev(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);

	blkdev_put(drvdata->bdev, NULL);
}

static int __qc_param_register_operations(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	struct sec_param_operations *ops = &drvdata->ops;
	int err;

	ops->read = sec_qc_param_read;
	ops->write = sec_qc_param_write;

	err = sec_param_register_operations(ops);
	if (err == -EBUSY)
		return -EPROBE_DEFER;

	return err;
}

static void __qc_param_unregister_operations(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	struct sec_param_operations *ops = &drvdata->ops;

	sec_param_unregister_operations(ops);
}

static ssize_t __used __qc_param_show_simple_uint(struct device *sec_class_dev,
		struct device_attribute *attr, char *buf,
		size_t index)
{
	struct qc_param_drvdata *drvdata = dev_get_drvdata(sec_class_dev);
	unsigned int value;

	if (!__qc_param_read(drvdata, index, &value))
		return -EINVAL;

	return scnprintf(buf, PAGE_SIZE, "%u", value);
}

static ssize_t __used __qc_param_show_simple_str(struct device *sec_class_dev,
		struct device_attribute *attr, char *buf,
		size_t index, size_t len)
{
	struct qc_param_drvdata *drvdata = dev_get_drvdata(sec_class_dev);
	char *context;
	int ret;

	/* NOTE: I use a boucing buffer to prevent 'buf' is not corrupted,
	 * when '__qc_paam_read' is failed.
	 */
	context = kmalloc(len, GFP_KERNEL);
	if (!context) {
		ret = -ENOMEM;
		goto __err_nomem;
	}

	if (!__qc_param_read(drvdata, index, context)) {
		ret = -EINVAL;
		goto __err_read_fail;
	}

	ret = scnprintf(buf, PAGE_SIZE, "%s", context);

__err_read_fail:
	kfree(context);
__err_nomem:
	return ret;
}

static ssize_t api_gpio_test_show(struct device *sec_class_dev,
		struct device_attribute *attr, char *buf)
{
	return __qc_param_show_simple_uint(sec_class_dev, attr, buf,
			param_index_api_gpio_test);
}
DEVICE_ATTR_RO(api_gpio_test);

static ssize_t api_gpio_test_result_show(struct device *sec_class_dev,
		struct device_attribute *attr, char *buf)
{
	return __qc_param_show_simple_str(sec_class_dev, attr, buf,
			param_index_api_gpio_test_result,
			QC_PARAM_SIZE(api_gpio_test_result));
}
DEVICE_ATTR_RO(api_gpio_test_result);

static inline void  __api_gpio_test_clear(struct qc_param_drvdata *drvdata)
{
	unsigned int zero = 0;

	__qc_param_write(drvdata, param_index_api_gpio_test, &zero);
}

static inline void __api_gpio_test_result_clear(
		struct qc_param_drvdata *drvdata)
{
	char empty[QC_PARAM_SIZE(api_gpio_test_result)] = { '\0', };

	__qc_param_write(drvdata, param_index_api_gpio_test_result, &empty);
}

static ssize_t api_gpio_test_clear_store(struct device *sec_class_dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct qc_param_drvdata *drvdata = dev_get_drvdata(sec_class_dev);
	struct device *dev = drvdata->bd.dev;
	int written;
	int err;

	err = kstrtoint(buf, 10, &written);
	if (err < 0) {
		dev_warn(dev, "requested written code is malformed or wrong\n");
		print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 16, 1,
				buf, count, 1);
		return err;
	}

	if (written != 1)
		return count;

	__api_gpio_test_clear(drvdata);
	__api_gpio_test_result_clear(drvdata);

	return count;
}
DEVICE_ATTR_WO(api_gpio_test_clear);

static struct attribute *sec_qc_param_attrs[] = {
	&dev_attr_api_gpio_test.attr,
	&dev_attr_api_gpio_test_result.attr,
	&dev_attr_api_gpio_test_clear.attr,
	NULL,
};

static const struct attribute_group sec_qc_param_attr_group = {
	.attrs = sec_qc_param_attrs,
};

static noinline int __qc_param_sysfs_create(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	struct device *dev = drvdata->param_dev;
	int err;

	err = sysfs_create_group(&dev->kobj, &sec_qc_param_attr_group);
	if (err)
		return err;

	return 0;
}

static noinline void __qc_param_sysfs_remove(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	struct device *dev = drvdata->param_dev;

	sysfs_remove_group(&dev->kobj, &sec_qc_param_attr_group);
}

static noinline int __qc_param_probe_epilog(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	qc_param = drvdata;

	return 0;
}

static noinline void __qc_param_remove_prolog(struct builder *bd)
{
	/* FIXME: This is not a graceful exit. */
	qc_param = NULL;
}

static int __qc_param_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_param_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __qc_param_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_param_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static void __qc_param_dbgfs_show_bdev(struct seq_file *m)
{
	struct qc_param_drvdata *drvdata = m->private;
	struct block_device *bdev = drvdata->bdev;

	seq_puts(m, "* Block Device :\n");
	seq_printf(m, "  - bdevname : %pg\n", bdev);
	seq_printf(m, "  - uuid     : %s\n", bdev->bd_meta_info->uuid);
	seq_printf(m, "  - volname  : %s\n", bdev->bd_meta_info->volname);
	seq_puts(m, "\n");
}

static void __qc_param_dbgfs_show_each(struct seq_file *m, size_t index)
{
	const struct qc_param_info *info = &qc_param_info[index];
	uint8_t *buf;

	if (!info->size)
		return;

	seq_printf(m, "[%zu] = %s\n", index, info->name);
	seq_printf(m, "  - offset : %zu\n", (size_t)info->offset);
	seq_printf(m, "  - size   : %zu\n", info->size);

	buf = kmalloc(info->size, GFP_KERNEL);
	if (!sec_qc_param_read(index, buf)) {
		seq_puts(m, "  - failed to read param!\n");
		goto warn_read_fail;
	}

	seq_hex_dump(m, " + ", DUMP_PREFIX_OFFSET, 16, 1,
			buf, info->size, true);

warn_read_fail:
	seq_puts(m, "\n");
	kfree(buf);
}

static int sec_qc_param_dbgfs_show_all(struct seq_file *m, void *unsed)
{
	size_t i;

	__qc_param_dbgfs_show_bdev(m);

	for (i = 0; i < ARRAY_SIZE(qc_param_info); i++)
		__qc_param_dbgfs_show_each(m, i);

	return 0;
}

static int sec_qc_param_dbgfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_qc_param_dbgfs_show_all,
			inode->i_private);
}

static const struct file_operations sec_qc_param_dgbfs_fops = {
	.open = sec_qc_param_dbgfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static noinline int __qc_param_debugfs_create(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);

	drvdata->dbgfs = debugfs_create_file("sec_qc_param", 0440,
			NULL, drvdata, &sec_qc_param_dgbfs_fops);

	return 0;
}

static noinline void __qc_param_debugfs_remove(struct builder *bd)
{
	struct qc_param_drvdata *drvdata =
			container_of(bd, struct qc_param_drvdata, bd);

	debugfs_remove(drvdata->dbgfs);
}
#else
static noinline int __qc_param_debugfs_create(struct builder *bd) { return 0; }
static noinline void __qc_param_debugfs_remove(struct builder *bd) {}
#endif

static const struct dev_builder __qc_param_dev_builder[] = {
	DEVICE_BUILDER(__qc_param_parse_dt, NULL),
	DEVICE_BUILDER(__qc_param_sec_class_create,
		       __qc_param_sec_class_remove),
	DEVICE_BUILDER(__qc_param_init_blkdev, __qc_param_exit_blkdev),
	DEVICE_BUILDER(__qc_param_register_operations,
		       __qc_param_unregister_operations),
	DEVICE_BUILDER(__qc_param_sysfs_create, __qc_param_sysfs_remove),
	DEVICE_BUILDER(__qc_param_debugfs_create, __qc_param_debugfs_remove),
	DEVICE_BUILDER(__qc_param_probe_epilog, __qc_param_remove_prolog),
};

static int sec_qc_param_probe(struct platform_device *pdev)
{
	return __qc_param_probe(pdev, __qc_param_dev_builder,
			ARRAY_SIZE(__qc_param_dev_builder));
}

static int sec_qc_param_remove(struct platform_device *pdev)
{
	return __qc_param_remove(pdev, __qc_param_dev_builder,
			ARRAY_SIZE(__qc_param_dev_builder));
}

static const struct of_device_id sec_qc_param_match_table[] = {
	{ .compatible = "samsung,qcom-param" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_param_match_table);

static struct platform_driver sec_qc_param_driver = {
	.driver = {
		.name = "sec,qc-param",
		.of_match_table = of_match_ptr(sec_qc_param_match_table),
	},
	.probe = sec_qc_param_probe,
	.remove = sec_qc_param_remove,
};

static int __init sec_qc_param_init(void)
{
	return platform_driver_register(&sec_qc_param_driver);
}
module_init(sec_qc_param_init);

static void __exit sec_qc_param_exit(void)
{
	platform_driver_unregister(&sec_qc_param_driver);
}
module_exit(sec_qc_param_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("SEC PARAM driver for RAW Partion & Qualcomm based devices");
MODULE_LICENSE("GPL v2");
