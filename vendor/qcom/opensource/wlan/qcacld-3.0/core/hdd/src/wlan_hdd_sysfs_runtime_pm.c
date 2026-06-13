/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved..
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

#include <wlan_hdd_includes.h>
#include "osif_psoc_sync.h"
#include <wlan_hdd_sysfs.h>
#include <wlan_hdd_sysfs_runtime_pm.h>
#include "hif.h"
#include "hif_runtime_pm.h"

static ssize_t hdd_sysfs_runtime_pm_show(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 char *buf)
{
	return hif_rtpm_log_debug_stats(buf, HIF_RTPM_FILL_TYPE_SYSFS);
}

static struct kobj_attribute runtime_pm_attribute =
	__ATTR(runtime_pm, 0440, hdd_sysfs_runtime_pm_show,
	       NULL);

int hdd_sysfs_runtime_pm_create(struct kobject *driver_kobject)
{
	int error;

	if (!driver_kobject) {
		hdd_err("could not get driver kobject!");
		return -EINVAL;
	}

	error = sysfs_create_file(driver_kobject,
				  &runtime_pm_attribute.attr);
	if (error)
		hdd_err("could not create runtime_pm sysfs file");

	return error;
}

void
hdd_sysfs_runtime_pm_destroy(struct kobject *driver_kobject)
{
	if (!driver_kobject) {
		hdd_err("could not get driver kobject!");
		return;
	}
	sysfs_remove_file(driver_kobject, &runtime_pm_attribute.attr);
}

static ssize_t
__hdd_sysfs_rtpm_store(struct hdd_context *hdd_ctx,
		       struct kobj_attribute *attr,
		       char const *buf, size_t count)
{
	char buf_local[MAX_SYSFS_USER_COMMAND_SIZE_LENGTH + 1];
	char *sptr, *token;
	int delay, ret;

	if (!wlan_hdd_validate_modules_state(hdd_ctx))
		return -EINVAL;

	ret = hdd_sysfs_validate_and_copy_buf(buf_local, sizeof(buf_local),
					      buf, count);
	if (ret) {
		hdd_err_rl("invalid input");
		return ret;
	}

	if (!hdd_is_any_sta_connected(hdd_ctx)) {
		hdd_err_rl("No STA is connected");
		return -EINVAL;
	}

	sptr = buf_local;
	/* Get value */
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtos32(token, 0, &delay))
		return -EINVAL;

	hdd_debug_rl("rtpm %d", delay);

	/* If delay is set as 0, then disable the rtpm */
	if (!delay)
		hif_rtpm_set_autosuspend_delay(-1);

	/* If delay is set as -1, then enable the rtpm with INI configuration */
	else if (delay == -1)
		hif_rtpm_restore_autosuspend_delay();
	else
		hif_rtpm_set_autosuspend_delay(delay);

	return count;
}

static ssize_t hdd_sysfs_rtpm_store(struct kobject *kobj,
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

	errno_size = __hdd_sysfs_rtpm_store(hdd_ctx, attr, buf, count);

	osif_psoc_sync_op_stop(psoc_sync);

	return errno_size;
}

static struct kobj_attribute rtpm_attribute =
	__ATTR(runtime_pm, 0220, NULL,
	       hdd_sysfs_rtpm_store);

int hdd_sysfs_create_rtpm_interface(struct kobject *wifi_kobject)
{
	int error;

	if (!wifi_kobject) {
		hdd_err("could not get wifi kobject!");
		return -EINVAL;
	}

	error = sysfs_create_file(wifi_kobject,
				  &rtpm_attribute.attr);
	if (error)
		hdd_err("could not create runtime_pm sysfs file");

	return error;
}

void hdd_sysfs_destroy_rtpm_interface(struct kobject *wifi_kobject)
{
	if (!wifi_kobject) {
		hdd_err("could not get wifi kobject!");
		return;
	}
	sysfs_remove_file(wifi_kobject, &rtpm_attribute.attr);
}
