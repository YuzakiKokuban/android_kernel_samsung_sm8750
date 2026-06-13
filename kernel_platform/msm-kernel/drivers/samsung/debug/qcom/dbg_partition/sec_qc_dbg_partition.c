// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/uio.h>

#include <linux/samsung/bsp/sec_blk_helper.h>
#include <linux/samsung/debug/qcom/sec_qc_summary.h>
#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/sec_kunit.h>

#include <block/blk.h>

#include "sec_qc_dbg_partition.h"

static struct qc_dbg_part_drvdata *qc_dbg_part;

static __always_inline bool __dbg_part_is_probed(void)
{
	return !!qc_dbg_part;
}

__ss_static struct qc_dbg_part_info dbg_part_info[DEBUG_PART_MAX_TABLE] __ro_after_init = {
	QC_DBG_PART_INFO(debug_index_reset_header,
			DEBUG_PART_OFFSET_FROM_DT,
			sizeof(struct debug_reset_header),
			O_RDWR),
	QC_DBG_PART_INFO(debug_index_reset_ex_info,
			DEBUG_PART_OFFSET_FROM_DT,
			sizeof(rst_exinfo_t),
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_ap_health,
			DEBUG_PART_OFFSET_FROM_DT,
			sizeof(ap_health_t),
			O_RDWR),
	QC_DBG_PART_INFO(debug_index_lcd_debug_info,
			DEBUG_PART_OFFSET_FROM_DT,
			sizeof(struct lcd_debug_t),
			O_RDWR),
	QC_DBG_PART_INFO(debug_index_reset_history,
			DEBUG_PART_OFFSET_FROM_DT,
			SEC_DEBUG_RESET_HISTORY_SIZE,
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_onoff_history,
			DEBUG_PART_OFFSET_FROM_DT,
			sizeof(onoff_history_t),
			O_RDWR),
	QC_DBG_PART_INFO(debug_index_reset_tzlog,
			DEBUG_PART_OFFSET_FROM_DT,
			DEBUG_PART_SIZE_FROM_DT,
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_reset_extrc_info,
			DEBUG_PART_OFFSET_FROM_DT,
			SEC_DEBUG_RESET_EXTRC_SIZE,
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_auto_comment,
			DEBUG_PART_OFFSET_FROM_DT,
			SEC_DEBUG_AUTO_COMMENT_SIZE,
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_reset_rkplog,
			DEBUG_PART_OFFSET_FROM_DT,
			SEC_DEBUG_RESET_ETRM_SIZE,
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_modem_info,
			DEBUG_PART_OFFSET_FROM_DT,
			sizeof(struct sec_qc_summary_data_modem),
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_reset_klog,
			DEBUG_PART_OFFSET_FROM_DT,
			SEC_DEBUG_RESET_KLOG_SIZE,
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_reset_lpm_klog,
			DEBUG_PART_OFFSET_FROM_DT,
			SEC_DEBUG_RESET_LPM_KLOG_SIZE,
			O_WRONLY),
	QC_DBG_PART_INFO(debug_index_reset_summary,
			DEBUG_PART_OFFSET_FROM_DT,
			DEBUG_PART_SIZE_FROM_DT,
			O_RDONLY),
	QC_DBG_PART_INFO(debug_index_reset_last_kmsg,
			DEBUG_PART_OFFSET_FROM_DT,
			SEC_DEBUG_RESET_LAST_KMSG_SIZE,
			O_RDWR),
};

__ss_static bool ____dbg_part_is_valid_index(size_t index,
		const struct qc_dbg_part_info *info)
{
	size_t size;

	if (index >= DEBUG_PART_MAX_TABLE)
		return false;

	size = info[index].size;
	if (!size || size >= DEBUG_PART_SIZE_FROM_DT)
		return false;

	return true;
}

static bool __dbg_part_is_valid_index(size_t index)
{
	return ____dbg_part_is_valid_index(index, dbg_part_info);
}

__ss_static ssize_t __dbg_part_get_size(size_t index,
		const struct qc_dbg_part_info *info)
{
	if (!____dbg_part_is_valid_index(index, info))
		return -EINVAL;

	return info[index].size;
}

ssize_t sec_qc_dbg_part_get_size(size_t index)
{
	if (!__dbg_part_is_probed())
		return -EBUSY;

	return __dbg_part_get_size(index, dbg_part_info);
}
EXPORT_SYMBOL_GPL(sec_qc_dbg_part_get_size);

static bool __dbg_part_read(struct qc_dbg_part_drvdata *drvdata,
		size_t index, void *value)
{
	struct device *dev = drvdata->bd.dev;
	struct qc_dbg_part_info *info;
	ssize_t read;

	info = &dbg_part_info[index];
	if (info->flags != O_RDONLY && info->flags != O_RDWR) {
		dev_warn(dev, "read operation is not permitted for idx:%zu\n",
				index);
		return false;
	}

	read = sec_blk_read(drvdata->bdev,
			value, info->size, info->offset);
	if (read < 0) {
		dev_warn(dev, "read faield (idx:%zu, err:%zd)\n", index, read);
		return false;
	} else if (read != info->size) {
		dev_warn(dev, "wrong size (idx:%zu) - requested(%zu) != read(%zd)\n",
				index, info->size, read);
		return false;
	}

	return true;
}

bool sec_qc_dbg_part_read(size_t index, void *value)
{
	if (!__dbg_part_is_probed())
		return false;

	if (!__dbg_part_is_valid_index(index))
		return false;

	return __dbg_part_read(qc_dbg_part, index, value);
}
EXPORT_SYMBOL_GPL(sec_qc_dbg_part_read);

static bool __dbg_part_write(struct qc_dbg_part_drvdata *drvdata,
		size_t index, const void *value)
{
	struct device *dev = drvdata->bd.dev;
	struct qc_dbg_part_info *info;
	ssize_t write;

	info = &dbg_part_info[index];
	if (info->flags != O_WRONLY && info->flags != O_RDWR) {
		dev_warn(dev, "write operation is not permitted for idx:%zu\n",
				index);
		return false;
	}

	write = sec_blk_write(drvdata->bdev,
			value, info->size, info->offset);
	if (write < 0) {
		dev_warn(dev, "write faield (idx:%zu, err:%zd)\n", index, write);
		return false;
	} else if (write != info->size) {
		dev_warn(dev, "wrong size (idx:%zu) - requested(%zu) != write(%zd)\n",
				index, info->size, write);
		return false;
	}

	return true;
}

bool sec_qc_dbg_part_write(size_t index, const void *value)
{
	if (!__dbg_part_is_probed())
		return false;

	if (!__dbg_part_is_valid_index(index))
		return false;

	return __dbg_part_write(qc_dbg_part, index, value);
}
EXPORT_SYMBOL_GPL(sec_qc_dbg_part_write);

static const char *sec_part_table = "sec,part_table";

static noinline int __dbg_part_parse_dt_part_table_entry(struct device *dev,
		struct device_node *np, u32 idx, u32 *offsetp, u32 *sizep)
{
	u32 offset, size;
	int err;

	err = of_property_read_u32_index(np, sec_part_table, idx * 2, &offset);
	if (err) {
		dev_err(dev, "%s %d offset read error - %d\n",
				sec_part_table, idx, err);
		return -EINVAL;
	}

	err = of_property_read_u32_index(np, sec_part_table, idx * 2 + 1, &size);
	if (err) {
		dev_err(dev, "%s %d size read error - %d\n",
				sec_part_table, idx, err);
		return -EINVAL;
	}

	if (offset + size > SEC_DEBUG_PARTITION_SIZE) {
		dev_err(dev, "%s oversize 0x%x\n",
				sec_part_table, offset + size);
		return -EINVAL;
	}

	*offsetp = offset;
	*sizep = size;

	return 0;
}

static void __dbg_part_info_set_offset(struct qc_dbg_part_info *info,
		u32 offset)
{
	if (info->offset == DEBUG_PART_OFFSET_FROM_DT)
		info->offset = offset;
}

static void __dbg_part_info_set_size(struct qc_dbg_part_info *info,
		u32 size)
{
	if (info->size == DEBUG_PART_SIZE_FROM_DT)
		info->size = size;
}

__ss_static noinline int ____dbg_part_parse_dt_part_table(struct builder *bd,
		struct device_node *np, struct qc_dbg_part_info *info)
{
	int len;
	u32 i, offset, size;
	struct device *dev = bd->dev;
	int err;

	of_get_property(np, sec_part_table, &len);
	if (!len) {
		dev_err(dev, "part-table node is not in device tree\n");
		return -ENODEV;
	}

	if (len != DEBUG_PART_MAX_TABLE * 2 * sizeof(u32)) {
		dev_err(dev, "%s has wrong size\n", sec_part_table);
		return -EINVAL;
	}

	for (i = 0; i < DEBUG_PART_MAX_TABLE; i++) {
		err = __dbg_part_parse_dt_part_table_entry(dev,
				np, i, &offset, &size);
		if (err)
			return err;

		__dbg_part_info_set_offset(&info[i], offset);
		__dbg_part_info_set_size(&info[i], size);
	}

	return 0;
}

static noinline int __dbg_part_parse_dt_part_table(struct builder *bd,
		struct device_node *np)
{
	return ____dbg_part_parse_dt_part_table(bd, np, dbg_part_info);
}

__ss_static noinline int __dbg_part_parse_dt_bdev_path(struct builder *bd,
		struct device_node *np)
{
	struct qc_dbg_part_drvdata *drvdata =
			container_of(bd, struct qc_dbg_part_drvdata, bd);

	return of_property_read_string(np, "sec,bdev_path",
			&drvdata->bdev_path);
}

static const struct dt_builder __dbg_part_dt_builder[] = {
	DT_BUILDER(__dbg_part_parse_dt_bdev_path),
	DT_BUILDER(__dbg_part_parse_dt_part_table),
};

static noinline int __dbg_part_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __dbg_part_dt_builder,
			ARRAY_SIZE(__dbg_part_dt_builder));
}

static noinline int __dbg_part_probe_prolog(struct builder *bd)
{
	struct qc_dbg_part_drvdata *drvdata =
			container_of(bd, struct qc_dbg_part_drvdata, bd);
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

	return 0;

err_blkdev:
	dev_err(dev, "can't find a block device - %s\n",
			drvdata->bdev_path);
	return err;
}

static noinline int __dbg_part_init_reset_header(struct builder *bd)
{
	struct qc_dbg_part_drvdata *drvdata =
			container_of(bd, struct qc_dbg_part_drvdata, bd);
	struct debug_reset_header __reset_header;
	struct debug_reset_header *reset_header = &__reset_header;
	bool valid;

	valid = __dbg_part_read(drvdata,
			debug_index_reset_summary_info, reset_header);
	if (!valid)
		return -EINVAL;

	if (reset_header->magic == DEBUG_PARTITION_MAGIC)
		/* OK, already initialized, skip this */
		return 0;

	/* NOTE: debug partition is not initialized. */
	memset(reset_header, 0, sizeof(*reset_header));
	reset_header->magic = DEBUG_PARTITION_MAGIC;
	valid = __dbg_part_write(drvdata,
			debug_index_reset_summary_info, reset_header);
	if (!valid)
		return -EINVAL;

	return 0;
}

static noinline void __dbg_part_remove_epilog(struct builder *bd)
{
	struct qc_dbg_part_drvdata *drvdata =
			container_of(bd, struct qc_dbg_part_drvdata, bd);

	blkdev_put(drvdata->bdev, NULL);
}

static noinline int __dbg_part_probe_epilog(struct builder *bd)
{
	struct qc_dbg_part_drvdata *drvdata =
			container_of(bd, struct qc_dbg_part_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	qc_dbg_part = drvdata;

	return 0;
}

static noinline void __dbg_part_remove_prolog(struct builder *bd)
{
	/* FIXME: This is not a graceful exit. */
	qc_dbg_part = NULL;
}

static int __dbg_part_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_dbg_part_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __dbg_part_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_dbg_part_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static void __dbg_part_dbgfs_show_bdev(struct seq_file *m)
{
	struct qc_dbg_part_drvdata *drvdata = m->private;
	struct block_device *bdev = drvdata->bdev;

	seq_puts(m, "* Block Device :\n");
	seq_printf(m, "  - bdevname : %pg\n", bdev);
	seq_printf(m, "  - uuid     : %s\n", bdev->bd_meta_info->uuid);
	seq_printf(m, "  - volname  : %s\n", bdev->bd_meta_info->volname);
	seq_puts(m, "\n");
}

static void __dbg_part_dbgfs_show_each(struct seq_file *m, size_t index)
{
	struct qc_dbg_part_info *info = &dbg_part_info[index];
	uint8_t *buf;

	if (!info->size)
		return;

	seq_printf(m, "[%zu] = %s\n", index, info->name);
	seq_printf(m, "  - offset : %zu\n", (size_t)info->offset);
	seq_printf(m, "  - size   : %zu\n", info->size);

	buf = kvmalloc(info->size, GFP_KERNEL);
	if (!sec_qc_dbg_part_read(index, buf)) {
		seq_puts(m, "  - failed to read debug partition!\n");
		goto warn_read_fail;
	}

	seq_hex_dump(m, " + ", DUMP_PREFIX_OFFSET, 16, 1,
			buf, info->size, true);

warn_read_fail:
	seq_puts(m, "\n");
	kvfree(buf);
}

static int sec_qc_dbg_part_dbgfs_show_all(struct seq_file *m, void *unsed)
{
	size_t i;

	__dbg_part_dbgfs_show_bdev(m);

	for (i = 0; i < DEBUG_PART_MAX_TABLE; i++)
		__dbg_part_dbgfs_show_each(m, i);

	return 0;
}

static int sec_qc_dbg_part_dbgfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_qc_dbg_part_dbgfs_show_all,
			inode->i_private);
}

static const struct file_operations sec_qc_dbg_part_dgbfs_fops = {
	.open = sec_qc_dbg_part_dbgfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int __dbg_part_debugfs_create(struct builder *bd)
{
	struct qc_dbg_part_drvdata *drvdata =
			container_of(bd, struct qc_dbg_part_drvdata, bd);

	drvdata->dbgfs = debugfs_create_file("sec_qc_dbg_part", 0440,
			NULL, drvdata, &sec_qc_dbg_part_dgbfs_fops);

	return 0;
}

static void __dbg_part_debugfs_remove(struct builder *bd)
{
	struct qc_dbg_part_drvdata *drvdata =
			container_of(bd, struct qc_dbg_part_drvdata, bd);

	debugfs_remove(drvdata->dbgfs);
}
#else
static int __dbg_part_debugfs_create(struct builder *bd) { return 0; }
static void __dbg_part_debugfs_remove(struct builder *bd) {}
#endif

static const struct dev_builder __dbg_part_dev_builder[] = {
	DEVICE_BUILDER(__dbg_part_parse_dt, NULL),
	DEVICE_BUILDER(__dbg_part_probe_prolog, __dbg_part_remove_epilog),
	DEVICE_BUILDER(__dbg_part_init_reset_header, NULL),
	DEVICE_BUILDER(__dbg_part_debugfs_create,
		       __dbg_part_debugfs_remove),
	DEVICE_BUILDER(__dbg_part_probe_epilog, __dbg_part_remove_prolog),
};

static int sec_qc_dbg_part_probe(struct platform_device *pdev)
{
	return __dbg_part_probe(pdev, __dbg_part_dev_builder,
			ARRAY_SIZE(__dbg_part_dev_builder));
}

static int sec_qc_dbg_part_remove(struct platform_device *pdev)
{
	return __dbg_part_remove(pdev, __dbg_part_dev_builder,
			ARRAY_SIZE(__dbg_part_dev_builder));
}

static const struct of_device_id sec_qc_dbg_part_match_table[] = {
	{ .compatible = "samsung,qcom-debug_partition" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_dbg_part_match_table);

static struct platform_driver sec_qc_dbg_part_driver = {
	.driver = {
		.name = "sec,qc-debug_partition",
		.of_match_table = of_match_ptr(sec_qc_dbg_part_match_table),
	},
	.probe = sec_qc_dbg_part_probe,
	.remove = sec_qc_dbg_part_remove,
};

static int __init sec_qc_dbg_part_init(void)
{
	return platform_driver_register(&sec_qc_dbg_part_driver);
}
module_init(sec_qc_dbg_part_init);

static void __exit sec_qc_dbg_part_exit(void)
{
	platform_driver_unregister(&sec_qc_dbg_part_driver);
}
module_exit(sec_qc_dbg_part_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("SEC debug partition & Qualcomm based devices");
MODULE_LICENSE("GPL v2");

