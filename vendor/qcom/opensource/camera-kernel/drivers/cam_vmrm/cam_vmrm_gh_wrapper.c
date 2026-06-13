// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "cam_vmrm_gh_wrapper.h"

int cam_vmrm_gh_irq_lend_v2(enum gh_irq_label label, enum gh_vm_names name,
	int irq, gh_irq_handle_fn_v2 cb_handle, void *data)
{
	int rc = 0;

	rc = gh_irq_lend_v2(label, name, irq, cb_handle, data);
	if (rc) {
		CAM_ERR(CAM_VMRM, "IRQ lend failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "IRQ lent success for label: %d", label);

	return rc;
}

int cam_vmrm_gh_irq_lend_notify(enum gh_irq_label label)
{
	int rc = 0;

	rc = gh_irq_lend_notify(label);
	if (rc) {
		CAM_ERR(CAM_VMRM, "IRQ lend notify failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "IRQ lent notify success for label: %d", label);

	return rc;
}

int cam_vmrm_gh_irq_wait_for_lend_v2(enum gh_irq_label label,
	enum gh_vm_names name, gh_irq_handle_fn_v2 on_lend, void *data)
{
	int rc = 0;

	rc = gh_irq_wait_for_lend_v2(label, name, on_lend, data);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq wait for lend failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Irq wait for lend succeed for label: %d", label);

	return rc;
}

int cam_vmrm_gh_irq_accept(enum gh_irq_label label, int irq, int type)
{
	int rc = 0;

	rc = gh_irq_accept(label, irq, type);
	if (rc < 0) {
		CAM_ERR(CAM_VMRM, "Irq accept failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Irq accept succeed for label: %d", label);

	return 0;
}

int cam_vmrm_gh_irq_accept_notify(enum gh_irq_label label)
{
	int rc = 0;

	rc = gh_irq_accept_notify(label);
	if (rc < 0) {
		CAM_ERR(CAM_VMRM, "Irq accept notify failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Irq accept notify succeed for label: %d", label);

	return 0;
}

int cam_vmrm_gh_irq_release(enum gh_irq_label label)
{
	int rc = 0;

	rc = gh_irq_release(label);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq release failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Irq release succeed for label: %d", label);

	return rc;
}

int cam_vmrm_gh_irq_release_notify(enum gh_irq_label label)
{
	int rc = 0;

	rc = gh_irq_release_notify(label);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq release notify failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Irq release notify succeed for label: %d", label);

	return rc;
}

int cam_vmrm_gh_irq_reclaim(enum gh_irq_label label)
{
	int rc = 0;

	rc = gh_irq_reclaim(label);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq reclaim failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Irq reclaim succeed for label: %d", label);

	return 0;
}

int cam_vmrm_gh_mem_register_notifier(enum gh_mem_notifier_tag tag,
	gh_mem_notifier_handler handler, void *data,
	void **cookie)
{
	void *cookie_temp;
	int rc = 0;

	cookie_temp = gh_mem_notifier_register(tag, handler, data);
	if (IS_ERR(cookie_temp)) {
		rc = PTR_ERR(cookie_temp);
		CAM_ERR(CAM_VMRM, "Mem notifier register failed for tag: %d, rc: %d", tag, rc);
		return rc;
	}
	*cookie = cookie_temp;

	CAM_DBG(CAM_VMRM, "Mem notifier register success for tag: %d", tag);

	return rc;
}

void cam_vmrm_gh_mem_unregister_notifier(void *cookie)

{
	gh_mem_notifier_unregister(cookie);

	CAM_DBG(CAM_VMRM, "Mem notifier unregister success");
}

int cam_vmrm_gh_mem_lend(u8 mem_type, u8 flags, gh_label_t label,
	struct gh_acl_desc *acl_desc, struct gh_sgl_desc *sgl_desc,
	struct gh_mem_attr_desc *mem_attr_desc,
	gh_memparcel_handle_t *handle)
{
	int rc = 0, i;
	int n_sgl_entries = 0;

	n_sgl_entries = sgl_desc->n_sgl_entries;
	for (i = 0; i < n_sgl_entries; i++) {
		CAM_DBG(CAM_VMRM, "base 0x%x size 0x%x index %d",
			sgl_desc->sgl_entries[i].ipa_base, sgl_desc->sgl_entries[i].size, i);
	}

	rc = ghd_rm_mem_lend(mem_type, flags, label, acl_desc, sgl_desc, mem_attr_desc, handle);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem lend failed for label %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "mem lend succeed for label: %d", label);

	return rc;
}

int cam_vmrm_gh_mem_notify(gh_memparcel_handle_t handle, u8 flags,
	gh_label_t mem_info_tag, struct gh_notify_vmid_desc *vmid_desc)
{
	int rc = 0;

	rc = gh_rm_mem_notify(handle, flags, mem_info_tag, vmid_desc);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem notify failed for tag: %d, rc: %d", mem_info_tag, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Memory notify succeeded for tag : %d", mem_info_tag);

	return rc;
}

int cam_vmrm_gh_mem_accept(gh_memparcel_handle_t handle, u8 mem_type, u8 trans_type,
	u8 flags, gh_label_t label, struct gh_acl_desc *acl_desc, struct gh_sgl_desc *sgl_desc,
	struct gh_mem_attr_desc *mem_attr_desc, u16 map_vmid)
{
	int rc = 0;

	sgl_desc = gh_rm_mem_accept(handle, mem_type, trans_type, flags, label, acl_desc, sgl_desc,
		mem_attr_desc, map_vmid);
	if (IS_ERR_OR_NULL(sgl_desc)) {
		rc = PTR_ERR(sgl_desc);
		CAM_ERR(CAM_VMRM, "Mem accept failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Mem accept succeeded for label: %d", label);

	return rc;
}

int cam_vmrm_gh_mem_release(gh_memparcel_handle_t handle, u8 flags)
{
	int rc = 0;

	rc = gh_rm_mem_release(handle, flags);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem release failed, rc: %d", rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Mem release succeed");

	return rc;
}

int cam_vmrm_gh_mem_reclaim(gh_memparcel_handle_t handle, u8 flags)
{
	int rc = 0;

	rc = ghd_rm_mem_reclaim(handle, flags);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem reclaim failed, rc: %d", rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Mem reclaim succeed");

	return 0;
}

#ifdef CONFIG_SPECTRA_RESOURCE_REQUEST_ENABLE
int cam_vmrm_gh_resource_register_req_client(struct gh_resource_client *client)
{
	int rc = 0;

	rc = gh_resource_register_req_client(client);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Failed in resoure register req client, rc: %d", rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Gh resoure register req client succeed");

	return rc;
}

int cam_vmrm_gh_resource_unregister_req_client(struct gh_resource_client *client)
{
	int rc = 0;

	rc = gh_resource_unregister_req_client(client);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Failed in resource unregister req client, rc: %d", rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Gh resource unregister req client succeed");

	return rc;
}

int cam_vmrm_gh_resource_register_release_client(
	struct gh_resource_client *client)
{
	int rc = 0;

	rc = gh_resource_register_release_client(client);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Failed in resource register release cb, rc: %d", rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Gh resource register release cb succeed");

	return rc;
}

int cam_vmrm_gh_resource_unregister_release_client(
	struct gh_resource_client *client)
{
	int rc = 0;

	rc = gh_resource_unregister_release_client(client);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Failed in resource unregister release cb, rc: %d", rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Gh resource unregister release client succeed");

	return rc;
}

int cam_vmrm_gh_resource_request(gh_vmid_t target_vmid, const char *subsys_name,
	struct gh_res_request *req_resource, int res_cnt)
{
	int rc = 0;

	rc = gh_resource_request(target_vmid, subsys_name, req_resource, res_cnt);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Failed in gh resource request, rc: %d", rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Gh resource request succeed");

	return rc;
}

int cam_vmrm_gh_resource_release(gh_vmid_t target_vmid, const char *subsys_name,
	struct gh_res_request *release_resource, int res_cnt)
{
	int rc = 0;

	rc = gh_resource_release(target_vmid, subsys_name, release_resource, res_cnt);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Failed in gh resource release, rc: %d", rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Gh resource release succeed");

	return rc;
}
#else
int cam_vmrm_gh_resource_register_req_cb(const char *subsys_name, gh_res_cb cb)
{
	return -EOPNOTSUPP;
}

int cam_vmrm_gh_resource_unregister_req_cb(const char *subsys_name, gh_res_cb cb)
{
	return -EOPNOTSUPP;
}

int cam_vmrm_gh_resource_register_release_cb(const char *subsys_name, gh_res_cb cb)
{
	return -EOPNOTSUPP;
}

int cam_vmrm_gh_resource_unregister_release_cb(
	const char *subsys_name, gh_res_cb cb)
{
	return -EOPNOTSUPP;
}

int cam_vmrm_gh_resource_request(void)
{
	return -EOPNOTSUPP;
}

int cam_vmrm_gh_resource_release(void)
{
	return -EOPNOTSUPP;
}
#endif

int cam_vmrm_ghd_rm_get_vmid(enum gh_vm_names vm_name, gh_vmid_t *vmid)
{
	int rc = 0;

	rc = ghd_rm_get_vmid(vm_name, vmid);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Failed in gh get vmid for vm name: %d, rc: %d", vm_name, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Gh get vmid succeed for vm name: %d", vm_name);

	return rc;
}

int cam_vmrm_gh_rm_get_vm_name(gh_vmid_t vmid, enum gh_vm_names *vm_name)
{
	int rc = 0;

	rc = gh_rm_get_vm_name(vmid, vm_name);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Failed in gh get vm name for vmid: %d, rc: %d", vmid, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Gh get vmid succeed for vmid: %d", vmid);

	return rc;
}
