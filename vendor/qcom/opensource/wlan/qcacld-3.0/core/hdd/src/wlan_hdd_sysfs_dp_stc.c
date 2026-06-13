/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: wlan_hdd_sysfs_dp_stc.c
 *
 * implementation for creating sysfs files:
 *
 * dp_stc
 */

#include <wlan_hdd_includes.h>
#include "osif_psoc_sync.h"
#include <wlan_hdd_sysfs.h>
#include <wlan_hdd_sysfs_dp_stc.h>
#include "wlan_dp_ucfg_api.h"

static ssize_t __hdd_sysfs_dp_stc_logmask_show(struct hdd_context *hdd_ctx,
					       struct kobj_attribute *attr,
					       char *buf)
{
	int ret = 0;
	uint32_t mask;

	if (!wlan_hdd_validate_modules_state(hdd_ctx))
		return -EINVAL;

	mask = ucfg_dp_stc_get_logmask(hdd_ctx->psoc);
	hdd_debug("STC log mask: 0x%x", mask);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret,
			 "Current logmask: 0x%x\n", mask);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret,
			 "\nLogmask Details:\n");
	ret += scnprintf(buf + ret, PAGE_SIZE - ret,
			 "BIT(0) - Enable Flow stats logging\n");
	ret += scnprintf(buf + ret, PAGE_SIZE - ret,
			 "BIT(1) - Enable Classified Flow stats logging\n");

	return ret;
}

static ssize_t hdd_sysfs_dp_stc_logmask_show(struct kobject *kobj,
					     struct kobj_attribute *attr,
					     char *buf)
{
	struct osif_psoc_sync *psoc_sync;
	struct hdd_context *hdd_ctx = cds_get_context(QDF_MODULE_ID_HDD);
	ssize_t errno_size;
	int ret;

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (ret != 0)
		return ret;

	errno_size = osif_psoc_sync_op_start(wiphy_dev(hdd_ctx->wiphy),
					     &psoc_sync);
	if (errno_size)
		return errno_size;

	errno_size = __hdd_sysfs_dp_stc_logmask_show(hdd_ctx, attr, buf);

	osif_psoc_sync_op_stop(psoc_sync);

	return errno_size;
}

static ssize_t
__hdd_sysfs_dp_stc_logmask_store(struct hdd_context *hdd_ctx,
				 struct kobj_attribute *attr, const char *buf,
				 size_t count)
{
	char buf_local[MAX_SYSFS_USER_COMMAND_SIZE_LENGTH + 1];
	char *sptr, *token;
	uint32_t value;
	int ret;

	if (!wlan_hdd_validate_modules_state(hdd_ctx))
		return -EINVAL;

	ret = hdd_sysfs_validate_and_copy_buf(buf_local, sizeof(buf_local),
					      buf, count);
	if (ret) {
		hdd_err_rl("invalid input");
		return ret;
	}

	sptr = buf_local;
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &value))
		return -EINVAL;

	hdd_debug("STC logmask: 0x%x", value);

	ucfg_dp_stc_update_logmask(hdd_ctx->psoc, value);
	return count;
}

static ssize_t
hdd_sysfs_dp_stc_logmask_store(struct kobject *kobj,
			       struct kobj_attribute *attr,
			       char const *buf, size_t count)
{
	struct osif_psoc_sync *psoc_sync;
	struct hdd_context *hdd_ctx = cds_get_context(QDF_MODULE_ID_HDD);
	ssize_t errno_size;
	int ret;

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (ret != 0)
		return ret;

	errno_size = osif_psoc_sync_op_start(wiphy_dev(hdd_ctx->wiphy),
					     &psoc_sync);
	if (errno_size)
		return errno_size;

	errno_size = __hdd_sysfs_dp_stc_logmask_store(hdd_ctx, attr,
						      buf, count);

	osif_psoc_sync_op_stop(psoc_sync);

	return errno_size;
}

static struct kobj_attribute dp_stc_logmask_attribute =
	__ATTR(stc_logmask, 0664, hdd_sysfs_dp_stc_logmask_show,
	       hdd_sysfs_dp_stc_logmask_store);

int hdd_sysfs_dp_stc_logmask_create(struct kobject *driver_kobject)
{
	int error;

	if (!driver_kobject) {
		hdd_err("could not get driver kobject!");
		return -EINVAL;
	}

	error = sysfs_create_file(driver_kobject,
				  &dp_stc_logmask_attribute.attr);
	if (error)
		hdd_err("could not create stc_logmask sysfs file");

	return error;
}

void
hdd_sysfs_dp_stc_logmask_destroy(struct kobject *driver_kobject)
{
	if (!driver_kobject) {
		hdd_err("could not get driver kobject!");
		return;
	}

	sysfs_remove_file(driver_kobject, &dp_stc_logmask_attribute.attr);
}
