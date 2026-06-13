/*
 * Copyright (c) 2011-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <wlan_hdd_main.h>
#include <wlan_hdd_sysfs_ipa.h>
#include <wlan_ipa_ucfg_api.h>
#include <wlan_hdd_sysfs.h>
#include "osif_sync.h"
#include "wlan_dp_ucfg_api.h"

#ifdef IPA_OFFLOAD
#define MAX_USER_COMMAND_SIZE_IPAUCSTAT 4
#define MAX_OPT_DP_CTRL_FLT_ADD_CMD_SIZE 250
#define MAX_OPT_DP_CTRL_FLT_DEL_CMD_SIZE 50
#define IPV6ARRAY 4
#define IPV6ARRAY_U8 16
#define IPV6ARRAY_U16 8

static ssize_t __hdd_sysfs_ipaucstate_store(struct net_device *net_dev,
					    const char __user *buf,
					    size_t count)
{
	struct hdd_adapter *adapter = WLAN_HDD_GET_PRIV_PTR(net_dev);
	struct hdd_context *hdd_ctx;
	uint8_t cmd[MAX_USER_COMMAND_SIZE_IPAUCSTAT] = {0};
	int ret;
	char *sptr, *token;
	uint8_t set_value = 0;

	hdd_enter();

	if (hdd_validate_adapter(adapter))
		return -EINVAL;

	hdd_ctx = WLAN_HDD_GET_CTX(adapter);
	ret = wlan_hdd_validate_context(hdd_ctx);
	if (ret)
		return ret;

	if (!wlan_hdd_validate_modules_state(hdd_ctx))
		return -EINVAL;

	if (!ucfg_ipa_is_enabled())
		return -EINVAL;

	ret = hdd_sysfs_validate_and_copy_buf(cmd, sizeof(cmd),
					      buf, count);
	if (ret) {
		hdd_err_rl("invalid input");
		return ret;
	}

	sptr = cmd;
	/* Get set_value */
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &set_value))
		return -EINVAL;

	/* If input value is non-zero get stats */
	switch (set_value) {
	case IPA_UC_STAT:
		ucfg_ipa_uc_stat(hdd_ctx->pdev);
		break;
	case IPA_UC_INFO:
		ucfg_ipa_uc_info(hdd_ctx->pdev);
		break;
	case IPA_UC_RT_DEBUG_HOST_DUMP:
		ucfg_ipa_uc_rt_debug_host_dump(hdd_ctx->pdev);
		break;
	case IPA_DUMP_INFO:
		ucfg_ipa_dump_info(hdd_ctx->pdev);
		break;
	default:
		/* place holder for stats clean up
		 * Stats clean not implemented yet on FW and IPA
		 */
		break;
	}
	hdd_exit();
	return count;
}

static ssize_t hdd_sysfs_ipaucstate_store(struct device *dev,
					  struct device_attribute *attr,
					  char const *buf, size_t count)
{
	struct net_device *net_dev = container_of(dev, struct net_device, dev);
	struct osif_vdev_sync *vdev_sync;
	ssize_t err_size;

	err_size = osif_vdev_sync_op_start(net_dev, &vdev_sync);
	if (err_size)
		return err_size;

	err_size = __hdd_sysfs_ipaucstate_store(net_dev, buf, count);

	osif_vdev_sync_op_stop(vdev_sync);
	return err_size;
}

static uint32_t convert_ip(char *sptr)
{
	uint8_t var[4], i = 0;
	char *token;
	uint32_t ip;

	token = strsep(&sptr, ".");
	while (token) {
		if (!token)
			break;
		if (kstrtou8(token, 0, &var[i]))
			return -EINVAL;
		i++;
		token = strsep(&sptr, ".");
	}

	ip = (var[0] << 24) | (var[1] << 16) | (var[2] << 8) | (var[3]);
	ipa_debug("ip %u", ip);

	return ip;
}

static int parse_ipv6(char *str_ptr, uint32_t *ipv6_addr)
{
	uint8_t temp_addr[IPV6ARRAY_U8];
	uint8_t *curr_ptr = temp_addr;
	int value = 0;
	int digits = 0;
	uint16_t digit_value;
	int i;

	for (i = 0; i < IPV6ARRAY; i++)
		ipv6_addr[i] = 0;

	for (i = 0; i < IPV6ARRAY_U16; i++) {
		if (*str_ptr == ':')
			str_ptr++;

		value = 0;
		digits = 0;
		while (*str_ptr && *str_ptr != ':' && digits < 4) {
			if (kstrtou16(str_ptr, 16, &digit_value) &&
			    digit_value < 0)
				return -EINVAL;

			value = (value << 4) | digit_value;
			digits++;
			str_ptr++;
		}

		*curr_ptr++ = (value >> 8) & 0xff;
		*curr_ptr++ = value & 0xff;
	}

	/* Convert the uint8_t array to uint32_t array */
	for (i = 0; i < IPV6ARRAY; ++i) {
		ipv6_addr[i] = (temp_addr[i * 4] << 24) |
				(temp_addr[i * 4 + 1] << 16) |
				(temp_addr[i * 4 + 2] << 8) |
				temp_addr[i * 4 + 3];
		ipa_debug("opt_dp_ctrl, ipv6_array[%d] %d", i, ipv6_addr[i]);
	}

	return 0;
}

static ssize_t __hdd_sysfs_ipaoptdpctrl_store(struct hdd_context *hdd_ctx,
					      struct kobj_attribute *attr,
					      const char __user *buf,
					      size_t count)
{
	char cmd[MAX_OPT_DP_CTRL_FLT_ADD_CMD_SIZE];
	int ret;
	char *sptr, *token;
	struct ipa_wdi_opt_dpath_flt_add_cb_params ipa_flt_add_params;
	uint16_t sport, dport;
	uint32_t sipv4, dipv4;
	char *sipv6, *dipv6;
	int i = 0;

	hdd_enter();

	if (!ucfg_dp_ipa_ctrl_debug_supported(hdd_ctx->psoc)) {
		hdd_err_rl("opt_dp_ctrl, ipa debug is not supported");
		return -EINVAL;
	}

	ret = hdd_sysfs_validate_and_copy_buf(cmd, sizeof(cmd),
					      buf, count);
	if (ret) {
		hdd_err_rl("invalid input");
		return ret;
	}

	sptr = cmd;
	/* Get num_tuples */
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;

	if (kstrtou8(token, 0, &ipa_flt_add_params.num_tuples))
		return -EINVAL;

	for (i = 0; i < ipa_flt_add_params.num_tuples; i++) {
		/* Get version */
		token = strsep(&sptr, " ");
		if (!token)
			return -EINVAL;

		if (kstrtou8(token, 0, &ipa_flt_add_params.flt_info[i].version))
			return -EINVAL;

		/* Get IPV4 */
		if (ipa_flt_add_params.flt_info[i].version == 0) {
			token = strsep(&sptr, " ");
			if (!token)
				return -EINVAL;

			sipv4 = convert_ip(token);
			ipa_flt_add_params.flt_info[i].ipv4_addr.ipv4_saddr =
								sipv4;
			token = strsep(&sptr, " ");
			if (!token)
				return -EINVAL;

			dipv4 = convert_ip(token);
			ipa_flt_add_params.flt_info[i].ipv4_addr.ipv4_daddr =
								dipv4;
		} else {
			token = strsep(&sptr, " ");
			if (!token)
				return -EINVAL;

			sipv6 = token;
			parse_ipv6(sipv6,
				   ipa_flt_add_params.flt_info[i].ipv6_addr.
				   ipv6_saddr);

			token = strsep(&sptr, " ");
			if (!token)
				return -EINVAL;

			dipv6 = token;
			parse_ipv6(dipv6,
				   ipa_flt_add_params.flt_info[i].ipv6_addr.
				   ipv6_daddr);
		}

		/* Get sport */
		token = strsep(&sptr, " ");
		if (!token)
			return -EINVAL;

		if (kstrtou16(token, 0, &sport))
			return -EINVAL;

		ipa_flt_add_params.flt_info[i].sport = sport;

		/* Get dport */
		token = strsep(&sptr, " ");
		if (!token)
			return -EINVAL;

		if (kstrtou16(token, 0, &dport))
			return -EINVAL;

		ipa_flt_add_params.flt_info[i].dport = dport;
		ipa_flt_add_params.flt_info[i].protocol = 17;
	}

	ucfg_ipa_set_opt_dp_ctrl_flt(hdd_ctx->pdev, &ipa_flt_add_params);
	hdd_exit();

	return count;
}

static ssize_t hdd_sysfs_ipaoptdpctrl_store(struct kobject *kobj,
					    struct kobj_attribute *attr,
					    char const *buf, size_t count)
{
	struct osif_psoc_sync *psoc_sync;
	struct hdd_context *hdd_ctx = cds_get_context(QDF_MODULE_ID_HDD);
	ssize_t errno_size = 0;
	int ret;

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (ret != 0)
		return ret;

	if (!wlan_hdd_validate_modules_state(hdd_ctx))
		return -EINVAL;

	if (!ucfg_ipa_is_enabled())
		return -EINVAL;

	if (!hdd_get_adapter(hdd_ctx, QDF_STA_MODE) &&
	    !hdd_get_adapter(hdd_ctx, QDF_SAP_MODE)) {
		hdd_err("device mode not supporting opt_dp_ctrl");
		return errno_size;
	}

	errno_size = osif_psoc_sync_op_start(wiphy_dev(hdd_ctx->wiphy),
					     &psoc_sync);
	if (errno_size)
		return errno_size;

	errno_size = __hdd_sysfs_ipaoptdpctrl_store(hdd_ctx, attr, buf, count);
	osif_psoc_sync_op_stop(psoc_sync);

	return errno_size;
}

static ssize_t __hdd_sysfs_ipaoptdpctrlrm_store(struct hdd_context *hdd_ctx,
						struct kobj_attribute *attr,
						const char __user *buf,
						size_t count)
{
	char cmd[MAX_OPT_DP_CTRL_FLT_DEL_CMD_SIZE];
	int ret;
	char *sptr, *token;
	struct ipa_wdi_opt_dpath_flt_rem_cb_params ipa_flt_rm_params;
	int i = 0;

	hdd_enter();

	if (!ucfg_dp_ipa_ctrl_debug_supported(hdd_ctx->psoc)) {
		hdd_err_rl("opt_dp_ctrl, ipa debug is not supported");
		return -EINVAL;
	}

	ret = hdd_sysfs_validate_and_copy_buf(cmd, sizeof(cmd),
					      buf, count);
	if (ret) {
		hdd_err_rl("invalid input");
		return ret;
	}

	sptr = cmd;
	/* Get num_tuples */
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;

	if (kstrtou8(token, 0, &ipa_flt_rm_params.num_tuples))
		return -EINVAL;

	for (i = 0; i < ipa_flt_rm_params.num_tuples; i++) {
		token = strsep(&sptr, " ");
		if (!token)
			return -EINVAL;

		if (kstrtou32(token, 0, &ipa_flt_rm_params.hdl_info[i]))
			return -EINVAL;
	}

	ucfg_ipa_set_opt_dp_ctrl_flt_rm(hdd_ctx->pdev, &ipa_flt_rm_params);
	hdd_exit();

	return count;
}

static ssize_t hdd_sysfs_ipaoptdpctrlrm_store(struct kobject *kobj,
					      struct kobj_attribute *attr,
					      char const *buf, size_t count)
{
	struct osif_psoc_sync *psoc_sync;
	struct hdd_context *hdd_ctx = cds_get_context(QDF_MODULE_ID_HDD);
	ssize_t errno_size = 0;
	int ret;

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (ret != 0)
		return ret;

	if (!wlan_hdd_validate_modules_state(hdd_ctx))
		return -EINVAL;

	if (!ucfg_ipa_is_enabled())
		return -EINVAL;

	if (!hdd_get_adapter(hdd_ctx, QDF_STA_MODE) &&
	    !hdd_get_adapter(hdd_ctx, QDF_SAP_MODE)) {
		hdd_err("device mode not supporting opt_dp_ctrl");
		return errno_size;
	}

	errno_size = osif_psoc_sync_op_start(wiphy_dev(hdd_ctx->wiphy),
					     &psoc_sync);
	if (errno_size)
		return errno_size;

	errno_size = __hdd_sysfs_ipaoptdpctrlrm_store(hdd_ctx, attr,
						      buf, count);
	osif_psoc_sync_op_stop(psoc_sync);

	return errno_size;
}

static DEVICE_ATTR(ipaucstat, 0220,
		   NULL, hdd_sysfs_ipaucstate_store);

static struct kobj_attribute ipaoptdpctrl_attribute =
	__ATTR(ipaoptdpctrl, 0220, NULL, hdd_sysfs_ipaoptdpctrl_store);

static struct kobj_attribute ipaoptdpctrlrm_attribute =
	__ATTR(ipaoptdpctrlrm, 0220, NULL, hdd_sysfs_ipaoptdpctrlrm_store);

void hdd_sysfs_ipa_create(struct hdd_adapter *adapter)
{
	if (device_create_file(&adapter->dev->dev, &dev_attr_ipaucstat))
		hdd_err("could not create wlan sysfs file");
}

void hdd_sysfs_ipa_destroy(struct hdd_adapter *adapter)
{
	device_remove_file(&adapter->dev->dev, &dev_attr_ipaucstat);
}

void hdd_sysfs_ipa_opt_dp_ctrl_create(struct kobject *driver_kobject)
{
	int error;

	if (!driver_kobject) {
		hdd_err("could not get wifi kobject!");
		return;
	}

	error = sysfs_create_file(driver_kobject,
				  &ipaoptdpctrl_attribute.attr);
	if (error)
		hdd_err("could not create ipaoptdpctrl sysfs file");
}

void hdd_sysfs_ipa_opt_dp_ctrl_destroy(struct kobject *driver_kobject)
{
	if (!driver_kobject) {
		hdd_err("could not get wifi kobject!");
		return;
	}

	sysfs_remove_file(driver_kobject, &ipaoptdpctrl_attribute.attr);
}

void hdd_sysfs_ipa_opt_dp_ctrl_rm_create(struct kobject *driver_kobject)
{
	int error;

	if (!driver_kobject) {
		hdd_err("could not get wifi kobject!");
		return;
	}

	error = sysfs_create_file(driver_kobject,
				  &ipaoptdpctrlrm_attribute.attr);
	if (error)
		hdd_err("could not create ipaoptdpctrlrm sysfs file");
}

void hdd_sysfs_ipa_opt_dp_ctrl_rm_destroy(struct kobject *driver_kobject)
{
	if (!driver_kobject) {
		hdd_err("could not get wifi kobject!");
		return;
	}

	sysfs_remove_file(driver_kobject, &ipaoptdpctrlrm_attribute.attr);
}
#endif
