/*
 * @file hdm.c
 * @brief HDM Support
 * Copyright (c) 2020, Samsung Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#if defined(CONFIG_HDM_QCOM)
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/resource.h>
#include <linux/sched/signal.h>
#include <soc/qcom/secure_buffer.h>
#include <linux/qtee_shmbridge.h>
#include <linux/scatterlist.h>
#endif
#include <linux/hdm.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/sec_class.h>


#include "hdm_log.h"

#ifdef CONFIG_HDM_UH
#include "uh.h"
#endif


int hdm_log_level = HDM_LOG_LEVEL;

static int is_hdm_initialized;
static u64 supported_subsystem;

static char *status = "NONE";
module_param(status, charp, 0444);
MODULE_PARM_DESC(status, "HDM status");

static uint32_t hdm_blocked_bit;

bool hdm_is_applied(uint32_t bit)
{
	if ((hdm_blocked_bit & bit)!= 0)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(hdm_is_applied);

void hdm_printk(int level, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (hdm_log_level < level)
		return;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk(KERN_INFO "%s %pV", TAG, &vaf);

	va_end(args);
}


static int __init hdm_flag_setup(void)
{
	char tmp_hdm_status[100] = {0};
	char *tmp_p = NULL;
	char *token = NULL;
	int cnt = 0;
	long val = 0;
	int err = 0;

	hdm_blocked_bit = 0;

	hdm_info("%s hdm.status = %s\n", __func__, status);

	if (status) {
		snprintf(tmp_hdm_status, sizeof(tmp_hdm_status), "%s", status);

		tmp_p = tmp_hdm_status;

		token = strsep(&tmp_p, "&|");

		while (token) {
			//even = hdm applied bit
			if (cnt++%2) {
				err = kstrtol(token, 16, &val);
				if (err)
					return err;
				hdm_blocked_bit = hdm_blocked_bit | val;
			}
			token = strsep(&tmp_p, "&|");
		}
	}

	hdm_info("%s hdm_blocked_status = 0x%x\n", __func__, hdm_blocked_bit);

	return 0;
}

static ssize_t hdm_policy_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long mode = HDM_CMD_MAX;
	int c, p;

	if (count == 0) {
		hdm_err("%s count = 0\n", __func__);
		goto error;
	}

	if (kstrtoul(buf, 0, &mode)) {
		goto error;
	};

	if (mode > HDM_CMD_MAX) {
		hdm_err("%s command size max fail. %d\n", __func__, mode);
		goto error;
	}
	hdm_info("%s: command id: 0x%x\n", __func__, (int)mode);

	c = (int)(mode & HDM_C_BITMASK);
	p = (int)(mode & HDM_P_BITMASK);

	hdm_info("%s m:0x%x c:0x%x p:0x%x\n", __func__, (int)mode, c, p);
	switch (c) {
	case HDM_FLAG_UPDATE:
		hdm_info("%s HDM_FLAG_UPDATE 0x%x\n", __func__, p);
		hdm_blocked_bit = p;
		hdm_info("hdm_blocked_status = 0x%x\n", hdm_blocked_bit);
		break;
#if defined(CONFIG_HDM_QCOM)
#ifdef CONFIG_HDM_UH
	case HDM_HYP_CALL:
		hdm_info("%s HDM_HYP_CALL\n", __func__);
		uh_call(UH_APP_HDM, 9, 0, p, 0, 0);
		break;
	case HDM_HYP_CALLP:
		hdm_info("%s HDM_HYP_CALLP\n", __func__);
		uh_call(UH_APP_HDM, 2, 0, p, 0, 0);
		break;
	case HDM_HYP_INIT:
		hdm_info("%s HDM_HYP_INIT\n", __func__);
		uh_call(UH_APP_HDM, 3, 0, 0, 0, 0);
		break;
	case HDM_HYP_CLEAR:
		hdm_info("%s HDM_HYP_CLEAR\n", __func__);
		uh_call(UH_APP_HDM, 4, 0, 0, 0, 0);
		break;
#endif
#endif
	default:
		goto error;
	}
error:
	return count;
}

static void get_supported_subsystem(void)
{
	if (is_hdm_initialized != true) {
#ifdef CONFIG_HDM_UH
		uh_call(UH_APP_HDM, HDM_GET_SUPPORTED_SUBSYSTEM, (u64)&supported_subsystem, 0, 0, 0);
#endif
		hdm_info("supported_subsystem = %012llx\n", supported_subsystem);
		is_hdm_initialized = true;
	}
}

static ssize_t hdm_subsystem_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 hdm_version = 0;

	hdm_info("%s\n", __func__);

	get_supported_subsystem();

	hdm_version = (supported_subsystem >> 12) & 0xFFF;
	hdm_info("hdm_version = %03x\n", hdm_version);

	return snprintf(buf, 7, "0x%03x\n", hdm_version);
}

static ssize_t pad_subsystem_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 pad_version = 0;

	hdm_info("%s\n", __func__);

	get_supported_subsystem();

	pad_version = supported_subsystem & 0xFFF;
	hdm_info("pad_version = %03x\n", pad_version);

	return snprintf(buf, 7, "0x%03x\n", pad_version);
}

static ssize_t bt_block_sub_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 bt_block_sub = 0;

	hdm_info("%s\n", __func__);

	get_supported_subsystem();

	bt_block_sub = (supported_subsystem >> 24) & 0xFFF;
	hdm_info("bt_block_sub = %03x\n", bt_block_sub);

	return snprintf(buf, 7, "0x%03x\n", bt_block_sub);
}
static ssize_t bt_unblock_sub_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 bt_unblock_sub = 0;

	hdm_info("%s\n", __func__);

	get_supported_subsystem();

	bt_unblock_sub = (supported_subsystem >> 36) & 0xFFF;
	hdm_info("bt_unblock_sub = %03x\n", bt_unblock_sub);

	return snprintf(buf, 7, "0x%03x\n", bt_unblock_sub);
}
static DEVICE_ATTR_WO(hdm_policy);
static DEVICE_ATTR_RO(hdm_subsystem);
static DEVICE_ATTR_RO(pad_subsystem);
static DEVICE_ATTR_RO(bt_block_sub);
static DEVICE_ATTR_RO(bt_unblock_sub);

#if defined(CONFIG_HDM_QCOM)
int hyp_assign_phys(phys_addr_t addr, u64 size, u32 *source_vm_list,
			int source_nelems, int *dest_vmids,
			int *dest_perms, int dest_nelems)
{
	struct sg_table table;
	int ret;

	ret = sg_alloc_table(&table, 1, GFP_KERNEL);
	if (ret)
		return ret;

	sg_set_page(table.sgl, phys_to_page(addr), size, 0);

	ret = hyp_assign_table(&table, source_vm_list, source_nelems,
			       dest_vmids, dest_perms, dest_nelems);

	sg_free_table(&table);
	return ret;
}


static uint64_t qseelog_shmbridge_handle;
static int __init __hdm_init_of(void)
{
	struct device_node *node;
	struct resource r;
	int ret;
	phys_addr_t addr;
	u64 size;

	uint32_t ns_vmids[] = {VMID_HLOS_FREE};
	uint32_t ns_vm_perms[] = {PERM_READ | PERM_WRITE};
	uint32_t ns_vm_nums = 1;

	int src_vmids[1] = {VMID_HLOS};
	int dest_vmids[1] = {VMID_HLOS_FREE};
	int dest_perms[1] = {PERM_READ | PERM_WRITE};

	hdm_info("%s start\n", __func__);

	node = of_find_node_by_name(NULL, "samsung,sec_hdm");
	if (!node) {
		hdm_err("failed of_find_node_by_name\n");
		return -ENODEV;
	}

	node = of_parse_phandle(node, "memory-region", 0);
	if (!node) {
		hdm_err("no memory-region specified\n");
		return -EINVAL;
	}


	ret = of_address_to_resource(node, 0, &r);
	if (ret) {
		hdm_err("failed of_address_to_resource\n");
		return ret;
	}

	addr = r.start;
	size = resource_size(&r);

	ret = hyp_assign_phys(addr, size, src_vmids, 1, dest_vmids, dest_perms, 1);
	if (ret) {
		hdm_err("%s: failed for %pa address of size %zx rc:%d\n",
				__func__, &addr, size, ret);
	}

	ret = qtee_shmbridge_register(addr, size, ns_vmids, ns_vm_perms,
			ns_vm_nums, PERM_READ | PERM_WRITE, &qseelog_shmbridge_handle);
	if (ret)
		hdm_err("failed to create bridge for qsee_log buffer\n");


	hdm_info("%s done\n", __func__);
	return 0;
}
#endif

static int __init hdm_test_init(void)
{
	struct device *dev;
#if defined(CONFIG_HDM_QCOM)
	int err;
#endif

	supported_subsystem = 0;
	hdm_flag_setup();

#if defined(CONFIG_HDM_QCOM)
	err = __hdm_init_of();
	if (err)
		return err;
#endif

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	dev = sec_device_create(NULL, "hdm");
#else
	hdm_err("%s sec_classs not supported\n", __func__);
	dev = NULL;
#endif
	if (IS_ERR_OR_NULL(dev)) {
		hdm_err("%s Failed to create devce\n", __func__);
		return 0;
	}

	if (device_create_file(dev, &dev_attr_hdm_policy) < 0) {
		hdm_err("%s Failed to create device file\n", __func__);
		return 0;
	}

	if (device_create_file(dev, &dev_attr_hdm_subsystem) < 0) {
		hdm_err("%s Failed to create device file\n", __func__);
		return 0;
	}

	if (device_create_file(dev, &dev_attr_pad_subsystem) < 0) {
		hdm_err("%s Failed to create device file\n", __func__);
		return 0;
	}

	if (device_create_file(dev, &dev_attr_bt_block_sub) < 0) {
		hdm_err("%s Failed to create device file\n", __func__);
		return 0;
	}

	if (device_create_file(dev, &dev_attr_bt_unblock_sub) < 0) {
		hdm_err("%s Failed to create device file\n", __func__);
		return 0;
	}

	hdm_info("%s end\n", __func__);
	return 0;
}

module_init(hdm_test_init);

MODULE_AUTHOR("Taeho Kim <taeho81.kim@samsung.com>");
MODULE_AUTHOR("Shinjae Lee <shinjae1.lee@samsung.com>");
MODULE_DESCRIPTION("HDM driver");
MODULE_LICENSE("GPL");
