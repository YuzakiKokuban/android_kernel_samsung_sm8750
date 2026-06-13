// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/pinctrl/qcom-pinctrl.h>
#include "camera_main.h"
#include "cam_debug_util.h"
#include "cam_vmrm.h"
#include "cam_common_util.h"
#include "cam_vmrm_interface.h"
#include "cam_soc_util.h"
#include "cam_req_mgr_dev.h"
#include "cam_mem_mgr_api.h"

struct cam_vmrm_intf_dev *g_vmrm_intf_dev;

static const struct of_device_id msm_cam_vmrm_intf_dt_match[] = {
	{
		.compatible = "qcom,cam-vmrm-intf",
	},
	{},
};

struct cam_vmrm_intf_dev *cam_vmrm_get_intf_dev(void)
{
	return g_vmrm_intf_dev;
}

static enum gh_vm_names cam_vmid_to_gh_name(uint32_t cam_vmid)
{
	enum gh_vm_names gh_name = GH_VM_MAX;

	switch (cam_vmid) {
	case CAM_PVM:
		gh_name = GH_PRIMARY_VM;
		break;
	case CAM_SVM1:
		gh_name = GH_TRUSTED_VM;
		break;
	default:
		CAM_ERR(CAM_VMRM, "Invalid camera vmid: %d", cam_vmid);
		break;
	}

	return gh_name;
}

#ifdef CONFIG_SPECTRA_RESOURCE_REQUEST_ENABLE
static uint32_t cam_gh_name_to_vmid(enum gh_vm_names vm_name)
{
	uint32_t vmid = CAM_VM_MAX;

	switch (vm_name) {
	case GH_PRIMARY_VM:
		vmid = CAM_PVM;
		break;
	case GH_TRUSTED_VM:
		vmid = CAM_SVM1;
		break;
	default:
		CAM_ERR(CAM_VMRM, "Invalid gh name: %d", vm_name);
		break;
	}

	return vmid;
}
#endif

static enum gh_irq_label cam_irq_label_convert_gh_label(uint32_t label)
{
	int diff;
	enum gh_irq_label gh_label = GH_IRQ_LABEL_MAX;

	if ((label >= CAM_IRQ_LABEL_BASE) && (label <= CAM_IRQ_LABEL_MAX)) {
		diff = label - CAM_IRQ_LABEL_BASE;
		gh_label = GH_IRQ_LABEL_CAM_BASE + diff;
	} else {
		CAM_ERR(CAM_VMRM, "Invalid cam irq label %d", label);
	}

	return gh_label;
}

static uint32_t cam_gh_irq_label_convert_cam_irq_label(enum gh_irq_label label)
{
	int diff;
	uint32_t cam_irq_label = CAM_IRQ_LABEL_MAX + 1;

	if ((label >= GH_IRQ_LABEL_CAM_BASE) &&
		(label < GH_IRQ_LABEL_CAM_BASE + CAM_IRQ_LABEL_MAX)) {
		diff = label - GH_IRQ_LABEL_CAM_BASE;
		cam_irq_label = CAM_IRQ_LABEL_BASE + diff;
	} else {
		CAM_ERR(CAM_VMRM, "Invalid gh irq label %d", label);
	}

	return cam_irq_label;
}

static enum gh_mem_notifier_tag cam_mem_tag_convert_gh_tag(uint32_t tag)
{
	int diff;
	enum gh_mem_notifier_tag gh_tag = GH_MEM_NOTIFIER_TAG_MAX;

	if ((tag >= CAM_MEM_TAG_BASE) && (tag <= CAM_MEM_TAG_MAX)) {
		diff = tag - CAM_MEM_TAG_BASE;
		gh_tag = GH_MEM_NOTIFIER_TAG_CAM_BASE + diff;
	} else {
		CAM_ERR(CAM_VMRM, "Invalid cam mem tag %d", tag);
	}

	return gh_tag;
}

static uint32_t cam_gh_tag_convert_cam_mem_tag(enum gh_mem_notifier_tag tag)
{
	int diff;
	uint32_t cam_tag = CAM_MEM_TAG_MAX + 1;

	if ((tag >= GH_MEM_NOTIFIER_TAG_CAM_BASE) &&
		(tag < GH_MEM_NOTIFIER_TAG_CAM_BASE + CAM_MEM_TAG_MAX)) {
		diff = tag - GH_MEM_NOTIFIER_TAG_CAM_BASE;
		cam_tag = CAM_MEM_TAG_BASE + diff;
	} else {
		CAM_ERR(CAM_VMRM, "Invalid gh mem tag %d", tag);
	}

	return cam_tag;
}

static bool cam_validate_gh_mem_notifier_tag(enum gh_mem_notifier_tag tag)
{
	bool valid = false;

	if ((tag >= GH_MEM_NOTIFIER_TAG_CAM_BASE) &&
		(tag < GH_MEM_NOTIFIER_TAG_CAM_BASE + CAM_MEM_TAG_MAX))
		valid = true;
	else {
		CAM_ERR(CAM_VMRM, "Invalid gh mem tag %d", tag);
	}

	return valid;
}

static bool cam_validate_cam_mem_label(uint32_t label)
{
	bool valid = false;

	if ((label >= CAM_MEM_LABEL_BASE) && (label <= CAM_MEM_TAG_MAX))
		valid = true;
	else
		CAM_ERR(CAM_VMRM, "Invalid cam mem label %d", label);

	return valid;
}

static bool cam_validate_gh_irq_label(enum gh_irq_label label)
{
	bool valid = false;

	if ((label >= GH_IRQ_LABEL_CAM_BASE) &&
		(label < GH_IRQ_LABEL_CAM_BASE + CAM_IRQ_LABEL_MAX))
		valid = true;
	else {
		CAM_ERR(CAM_VMRM, "Invalid gh irq label %d", label);
	}

	return valid;
}

static struct gh_acl_desc *cam_populate_acl(enum gh_vm_names vm_name)
{
	struct gh_acl_desc *acl_desc;
	gh_vmid_t vmid;

	cam_vmrm_ghd_rm_get_vmid(vm_name, &vmid);

	acl_desc = CAM_MEM_ZALLOC(offsetof(struct gh_acl_desc, acl_entries[1]), GFP_KERNEL);
	if (!acl_desc) {
		CAM_ERR(CAM_VMRM, "Could not reserve memory for ACL entries");
		return ERR_PTR(-ENOMEM);
	}

	acl_desc->n_acl_entries = 1;
	acl_desc->acl_entries[0].vmid = vmid;
	acl_desc->acl_entries[0].perms = GH_RM_ACL_R | GH_RM_ACL_W;

	return acl_desc;
}

static struct gh_notify_vmid_desc *cam_populate_vmid_desc(enum gh_vm_names vm_name)
{
	struct gh_notify_vmid_desc *vmid_desc;
	gh_vmid_t vmid;

	cam_vmrm_ghd_rm_get_vmid(vm_name, &vmid);

	vmid_desc = CAM_MEM_ZALLOC(offsetof(struct gh_notify_vmid_desc,
			vmid_entries[1]), GFP_KERNEL);
	if (!vmid_desc) {
		CAM_ERR(CAM_VMRM, "Could not reserve memory for vmid entries");
		return ERR_PTR(-ENOMEM);
	}

	vmid_desc->n_vmid_entries = 1;
	vmid_desc->vmid_entries[0].vmid = vmid;

	return vmid_desc;
}

static int cam_mem_entry_is_duplicate(phys_addr_t mem_base, uint32_t mem_block_size)
{
	struct cam_io_mem_entry *mem, *mem_temp;
	int rc = 0;

	if (!list_empty(&g_vmrm_intf_dev->io_res.mem)) {
		list_for_each_entry_safe(mem, mem_temp, &g_vmrm_intf_dev->io_res.mem, list) {
			if (mem->base == mem_base) {
				if (mem->size != mem_block_size) {
					CAM_DBG(CAM_VMRM,
						"Duplicate mem size mismatch previous mem base 0x%x size 0x%x current size 0x%x",
						mem_base, mem->size, mem_block_size);
					if (mem->size < mem_block_size) {
						CAM_DBG(CAM_VMRM,
							"update mem size from 0x%x to 0x%x",
							mem->size, mem_block_size);
						mem->size = mem_block_size;
					}
					rc = 2;
					break;
				}
				rc = 1;
				break;
			}
		}
	}

	return rc;
}

static struct cam_io_irq_entry *cam_irq_lookup(enum gh_irq_label label, uint32_t irq_num)
{
	struct cam_io_irq_entry *irq = NULL, *irq_temp;
	bool found = false;

	if (!list_empty(&g_vmrm_intf_dev->io_res.irq)) {
		list_for_each_entry_safe(irq, irq_temp, &g_vmrm_intf_dev->io_res.irq, list) {
			if (label) {
				if (cam_irq_label_convert_gh_label(irq->label) == label) {
					found = true;
					break;
				}
			}
			if (irq_num) {
				if (irq->irq_num == irq_num) {
					found = true;
					break;
				}
			}
		}
	}
	if (!found) {
		CAM_ERR(CAM_VMRM, "Not found the irq num %d gh label %d in list", irq_num, label);
		irq = NULL;
	}

	return irq;
}

struct cam_hw_instance *cam_hw_instance_lookup(uint32_t hw_id, uint32_t mem_tag,
	uint32_t irq_label, uint32_t gpio_num)
{
	struct cam_hw_instance *hw_pos = NULL, *hw_temp;
	int i;

	if (!list_empty(&g_vmrm_intf_dev->hw_instance)) {
		list_for_each_entry_safe(hw_pos, hw_temp, &g_vmrm_intf_dev->hw_instance, list) {
			if (hw_id && (hw_pos->hw_id == hw_id)) {
				CAM_DBG(CAM_VMRM, "found hw id 0x%x", hw_id);
				goto end;
			}

			if (mem_tag && (hw_pos->resources.mem_tag == mem_tag)) {
				CAM_DBG(CAM_VMRM, "found hw id 0x%x mem tag %d", hw_pos->hw_id,
					hw_pos->resources.mem_tag);
				goto end;
			}

			if (irq_label && ((hw_pos->resources.irq_label[0] == irq_label) ||
				(hw_pos->resources.irq_label[1] == irq_label))) {
				CAM_DBG(CAM_VMRM, "found hw id 0x%x irq label %d",
					hw_pos->hw_id, irq_label);
				goto end;
			}

			if (gpio_num) {
				for (i = 0; i < hw_pos->resources.gpio_count; i++) {
					if (gpio_num == hw_pos->resources.gpio_num[i]) {
						CAM_DBG(CAM_VMRM, "found hw id 0x%x gpio num %d",
							hw_pos->hw_id, gpio_num);
						goto end;
					}
				}
			}
		}
		hw_pos = NULL;
		CAM_ERR(CAM_VMRM, "Not found hw id 0x%x mem tag %d irq label %d gpio num %d",
			hw_id, mem_tag, irq_label, gpio_num);
	} else {
		CAM_WARN(CAM_VMRM, "hw instance empty");
	}

end:
	return hw_pos;
}

struct cam_driver_node *cam_driver_node_lookup(uint32_t driver_id)
{
	struct cam_driver_node *driver_pos = NULL, *driver_temp;

	if (!list_empty(&g_vmrm_intf_dev->driver_node)) {
		list_for_each_entry_safe(driver_pos, driver_temp,
			&g_vmrm_intf_dev->driver_node, list) {
			if (driver_pos->driver_id == driver_id) {
				CAM_DBG(CAM_VMRM, "found driver id 0x%x", driver_id);
				goto end;
			}
		}
		driver_pos = NULL;
		CAM_ERR(CAM_VMRM, "Not found driver id 0x%x", driver_id);
	} else {
		CAM_WARN(CAM_VMRM, "driver node empty");
	}

end:
	return driver_pos;
}

static void cam_set_hw_mem_owner(struct cam_hw_instance *hw_pos, uint32_t vm_owner)
{
	int i;
	struct cam_io_mem_entry *mem, *mem_temp;

	if (hw_pos->resources.num_mem_block) {
		for (i = 0; i < hw_pos->resources.num_mem_block; i++) {
			if (!list_empty(&g_vmrm_intf_dev->io_res.mem)) {
				list_for_each_entry_safe(mem, mem_temp,
					&g_vmrm_intf_dev->io_res.mem, list) {
					if (mem->base == hw_pos->resources.mem_block_addr[i]) {
						mem->vm_owner = vm_owner;
						mem->lend_in_progress = false;
						break;
					}
				}
			}
		}
	}

	if (hw_pos->resources.gpio_count) {
		for (i = 0; i < hw_pos->resources.gpio_count; i++) {
			if (!list_empty(&g_vmrm_intf_dev->io_res.mem)) {
				list_for_each_entry_safe(mem, mem_temp,
					&g_vmrm_intf_dev->io_res.mem, list) {
					if (mem->base == hw_pos->resources.gpio_mem_addr[i]) {
						mem->vm_owner = vm_owner;
						mem->lend_in_progress = false;
						break;
					}
				}
			}
		}
	}
}

static void cam_set_hw_irq_owner(uint32_t irq_num, uint32_t vm_owner)
{
	struct cam_io_irq_entry *irq, *irq_temp;

	if (!list_empty(&g_vmrm_intf_dev->io_res.irq)) {
		list_for_each_entry_safe(irq, irq_temp, &g_vmrm_intf_dev->io_res.irq, list) {
			if (irq->irq_num == irq_num) {
				irq->vm_owner = vm_owner;
				break;
			}
		}
	}

}

static void cam_mem_notification_pvm_handler(
	enum gh_mem_notifier_tag tag, unsigned long notify_type,
	void *entry_data, void *notif_msg)
{
	int rc = 0;
	uint32_t mem_hdl_gh;
	uint32_t cam_mem_tag;
	struct gh_rm_notif_mem_accepted_payload *accepted_payload;
	struct gh_rm_notif_mem_released_payload *released_payload;
	struct cam_hw_instance *hw;

	if (!cam_validate_gh_mem_notifier_tag(tag)) {
		CAM_ERR(CAM_VMRM, "Invalid tag: %d", tag);
		return;
	}

	if (!notif_msg) {
		CAM_ERR(CAM_VMRM, "Invalid msg");
		return;
	}

	if (notify_type == GH_RM_NOTIF_MEM_ACCEPTED) {
		accepted_payload = (struct gh_rm_notif_mem_accepted_payload *)notif_msg;
		mem_hdl_gh = accepted_payload->mem_handle;
		cam_mem_tag = cam_gh_tag_convert_cam_mem_tag(tag);

		mutex_lock(&g_vmrm_intf_dev->lock);
		hw = cam_hw_instance_lookup(0, cam_mem_tag, 0, 0);
		if (!hw) {
			CAM_ERR(CAM_VMRM, "Look up hw mem tag failed %d", cam_mem_tag);
			mutex_unlock(&g_vmrm_intf_dev->lock);
			return;
		}
		mutex_unlock(&g_vmrm_intf_dev->lock);

		CAM_DBG(CAM_VMRM, "Receive hw id 0x%x mem accept notification tag: %d",
			hw->hw_id, cam_mem_tag);

		spin_lock(&hw->spin_lock);
		hw->resources.ready_mask |= BIT(0);
		cam_set_hw_mem_owner(hw, CAM_SVM1);
		if (hw->resources.ready_mask == hw->resources.valid_mask) {
			hw->vm_owner = CAM_SVM1;
			hw->is_using = true;
			hw->resources.ready_mask = 0;
			atomic_set(&hw->lend_in_progress, 0);
			CAM_DBG(CAM_VMRM, "hw 0x%x is acquired by vm %d", hw->hw_id, CAM_SVM1);
		}
		spin_unlock(&hw->spin_lock);
	} else if (notify_type == GH_RM_NOTIF_MEM_RELEASED) {
		released_payload = (struct gh_rm_notif_mem_released_payload *)notif_msg;
		mem_hdl_gh = released_payload->mem_handle;
		cam_mem_tag = cam_gh_tag_convert_cam_mem_tag(tag);

		mutex_lock(&g_vmrm_intf_dev->lock);
		hw = cam_hw_instance_lookup(0, cam_mem_tag, 0, 0);
		if (!hw) {
			CAM_ERR(CAM_VMRM, "Look up hw mem tag failed %d", cam_mem_tag);
			mutex_unlock(&g_vmrm_intf_dev->lock);
			return;
		}

		rc = cam_vmrm_gh_mem_reclaim(mem_hdl_gh, 0);
		if (rc) {
			CAM_ERR(CAM_VMRM, "Reclaim mem failed for tag: %d, rc: %d",
				cam_mem_tag, rc);
			mutex_unlock(&g_vmrm_intf_dev->lock);
			return;
		}

		CAM_DBG(CAM_VMRM, "Reclaim hw id 0x%x mem succeed for tag: %d",
			hw->hw_id, cam_mem_tag);

		hw->resources.ready_mask |= BIT(0);
		cam_set_hw_mem_owner(hw, CAM_PVM);
		if (hw->resources.ready_mask == hw->resources.valid_mask) {
			hw->vm_owner = CAM_PVM;
			hw->is_using = false;
			hw->resources.ready_mask = 0;
			CAM_DBG(CAM_VMRM, "Reclaim hw succeed 0x%x", hw->hw_id);
		}
		mutex_unlock(&g_vmrm_intf_dev->lock);
	} else {
		CAM_ERR(CAM_VMRM, "Invalid notification type: %lld", notify_type);
		return;
	}
}

static void cam_mem_notification_svm_handler(
	enum gh_mem_notifier_tag tag, unsigned long notify_type,
	void *entry_data, void *notif_msg)
{
	int rc = 0;
	enum gh_vm_names gh_vm_name;
	uint32_t mem_hdl = 0, mem_hdl_gh, label, flags_accept;
	uint32_t cam_mem_tag, trans_type, vm_name;
	struct cam_hw_instance *hw;
	struct gh_acl_desc *acl_desc = NULL;
	struct gh_rm_notif_mem_shared_payload *payload;
	uint8_t flags = GH_RM_MEM_NOTIFY_OWNER_ACCEPTED;

	CAM_DBG(CAM_VMRM, "notify type: %lld tag: %d", notify_type, tag);

	if (notify_type != GH_RM_NOTIF_MEM_SHARED) {
		CAM_ERR(CAM_VMRM, "Invalid notification type: %lld", notify_type);
		return;
	}

	if (!cam_validate_gh_mem_notifier_tag(tag)) {
		CAM_ERR(CAM_VMRM, "Invalid tag: %d", tag);
		return;
	}

	if (!notif_msg) {
		CAM_ERR(CAM_VMRM, "Invalid msg");
		return;
	}

	payload = (struct gh_rm_notif_mem_shared_payload *)notif_msg;
	label = payload->label;
	trans_type = payload->trans_type;
	mem_hdl_gh = payload->mem_handle;
	if (!cam_validate_cam_mem_label(label)) {
		CAM_ERR(CAM_VMRM, "Invalid label: %d", label);
		return;
	}

	if (trans_type != GH_RM_TRANS_TYPE_LEND) {
		CAM_ERR(CAM_VMRM, "Invalid transfer type: %d", trans_type);
		return;
	}

	CAM_DBG(CAM_VMRM, "Trans type: %d label: %d handle: %d",
		trans_type, label, mem_hdl);

	vm_name = cam_vmrm_intf_get_vmid();
	cam_mem_tag = cam_gh_tag_convert_cam_mem_tag(tag);
	gh_vm_name = cam_vmid_to_gh_name(vm_name);
	acl_desc = cam_populate_acl(gh_vm_name);
	if (IS_ERR(acl_desc)) {
		CAM_ERR(CAM_VMRM, "Populate acl failed %d", PTR_ERR(acl_desc));
		return;
	}

	flags_accept = GH_RM_MEM_ACCEPT_VALIDATE_ACL_ATTRS|
		GH_RM_MEM_ACCEPT_VALIDATE_LABEL|
		GH_RM_MEM_ACCEPT_DONE;

	hw = cam_hw_instance_lookup(0, cam_mem_tag, 0, 0);
	if (!hw) {
		CAM_ERR(CAM_VMRM, "Look up hw mem tag failed %d", cam_mem_tag);
		CAM_MEM_FREE(acl_desc);
		return;
	}
	rc = cam_vmrm_gh_mem_accept(mem_hdl_gh, GH_RM_MEM_TYPE_IO,
		trans_type, flags_accept, label, acl_desc, NULL, NULL, 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem accept failed for tag: %d, rc: %d", cam_mem_tag, rc);
		CAM_MEM_FREE(acl_desc);
		return;
	}

	CAM_DBG(CAM_VMRM, "Mem accept succeed for tag: %d handle: %d hw id 0x%x",
		cam_mem_tag, mem_hdl_gh, hw->hw_id);
	CAM_MEM_FREE(acl_desc);

	spin_lock(&hw->spin_lock);
	hw->resources.ready_mask |= BIT(0);
	hw->handle = mem_hdl_gh;
	cam_set_hw_mem_owner(hw, cam_vmrm_intf_get_vmid());
	if (hw->resources.ready_mask == hw->resources.valid_mask) {
		hw->vm_owner = cam_vmrm_intf_get_vmid();
		hw->is_using = true;
		hw->resources.ready_mask = 0;
		complete_all(&hw->wait_response);
		CAM_DBG(CAM_VMRM, "Acquire hw succeed 0x%x", hw->hw_id);
	}
	spin_unlock(&hw->spin_lock);

	rc = cam_vmrm_gh_mem_notify(mem_hdl_gh, flags, tag, 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem notify failed for tag: %d, rc: %d", tag, rc);
		return;
	}

	CAM_DBG(CAM_VMRM, "Mem accept notify succeed for tag: %d hw id 0x%x", tag, hw->hw_id);
}

static void cam_irq_notification_pvm_handler(void *data,
	unsigned long notify_type, enum gh_irq_label label)
{
	int rc = 0, i;
	uint32_t irq_num;
	uint32_t cam_irq_label;
	struct cam_io_irq_entry *irq;
	struct cam_hw_instance *hw;

	CAM_DBG(CAM_VMRM, "notify type: %lld gh irq label: %d", notify_type, label);

	if ((notify_type != GH_RM_NOTIF_VM_IRQ_ACCEPTED) &&
		(notify_type != GH_RM_NOTIF_VM_IRQ_RELEASED)) {
		CAM_ERR(CAM_VMRM, "Invalid notification type %lld", notify_type);
		return;
	}

	if (!cam_validate_gh_irq_label(label)) {
		CAM_ERR(CAM_VMRM, "Invalid label %d", label);
		return;
	}

	if (notify_type == GH_RM_NOTIF_VM_IRQ_ACCEPTED) {
		mutex_lock(&g_vmrm_intf_dev->lock);
		irq = cam_irq_lookup(label, 0);
		if (!irq) {
			CAM_ERR(CAM_VMRM, "Lookup irq label failed %d", label);
			mutex_unlock(&g_vmrm_intf_dev->lock);
			return;
		}

		cam_irq_label = cam_gh_irq_label_convert_cam_irq_label(label);
		hw = cam_hw_instance_lookup(0, 0, cam_irq_label, 0);
		if (!hw) {
			CAM_ERR(CAM_VMRM, "Look up hw cam irq label failed %d", cam_irq_label);
			mutex_unlock(&g_vmrm_intf_dev->lock);
			return;
		}
		mutex_unlock(&g_vmrm_intf_dev->lock);

		CAM_DBG(CAM_VMRM, "Receive irq accept notification camera label: %d",
			cam_irq_label);

		spin_lock(&hw->spin_lock);
		for (i = 0; i < hw->resources.irq_count; i++) {
			if (cam_irq_label == hw->resources.irq_label[i])
				break;
		}
		hw->resources.ready_mask |= BIT(i + CAM_VMRM_RESOURCE_IRQ_BIT_MAP_OFFSET);
		cam_set_hw_irq_owner(hw->resources.irq_num[i], CAM_SVM1);
		if (hw->resources.ready_mask == hw->resources.valid_mask) {
			hw->vm_owner = CAM_SVM1;
			hw->is_using = true;
			hw->resources.ready_mask = 0;
			atomic_set(&hw->lend_in_progress, 0);
			CAM_DBG(CAM_VMRM, "hw 0x%x is acquired by vm %d", hw->hw_id, CAM_SVM1);
		}
		spin_unlock(&hw->spin_lock);
	} else if (notify_type == GH_RM_NOTIF_VM_IRQ_RELEASED) {
		CAM_DBG(CAM_VMRM, "Receive irq release notification for gh label: %d", label);
		mutex_lock(&g_vmrm_intf_dev->lock);
		irq = cam_irq_lookup(label, 0);
		if (!irq) {
			CAM_ERR(CAM_VMRM, "Lookup irq label failed %d", label);
			mutex_unlock(&g_vmrm_intf_dev->lock);
			return;
		}

		irq_num = irq->irq_num;
		rc = cam_vmrm_gh_irq_reclaim(label);
		if (rc) {
			CAM_ERR(CAM_VMRM, "Irq reclaim %d failed for irq num: %d camera label: %d",
				irq_num, irq->label);
			mutex_unlock(&g_vmrm_intf_dev->lock);
			return;
		}

		cam_irq_label = cam_gh_irq_label_convert_cam_irq_label(label);
		hw = cam_hw_instance_lookup(0, 0, cam_irq_label, 0);
		if (!hw) {
			CAM_ERR(CAM_VMRM, "Look up hw cam irq label failed %d", cam_irq_label);
			mutex_unlock(&g_vmrm_intf_dev->lock);
			return;
		}

		CAM_DBG(CAM_VMRM, "Reclaim irq succeed for label: %d", cam_irq_label);

		for (i = 0; i < hw->resources.irq_count; i++) {
			if (cam_irq_label == hw->resources.irq_label[i])
				break;
		}
		hw->resources.ready_mask |= BIT(i + CAM_VMRM_RESOURCE_IRQ_BIT_MAP_OFFSET);
		cam_set_hw_irq_owner(hw->resources.irq_num[i], CAM_PVM);
		if (hw->resources.ready_mask == hw->resources.valid_mask) {
			hw->vm_owner = CAM_PVM;
			hw->is_using = false;
			hw->resources.ready_mask = 0;
			CAM_DBG(CAM_VMRM, "Reclaim hw succeed 0x%x", hw->hw_id);
		}
		mutex_unlock(&g_vmrm_intf_dev->lock);

		return;
	}
}

static void cam_irq_notification_svm_handler(void *data,
	unsigned long notify_type, enum gh_irq_label label)
{
	int rc = 0, accepted_irq, i;
	struct cam_io_irq_entry *irq;
	struct cam_hw_instance *hw;
	uint32_t cam_irq_label;

	if (notify_type != GH_RM_NOTIF_VM_IRQ_LENT) {
		CAM_ERR(CAM_VMRM, "Invalid notification type %lld", notify_type);
		return;
	}

	if (!cam_validate_gh_irq_label(label)) {
		CAM_ERR(CAM_VMRM, "Invalid gh label %d", label);
		return;
	}

	irq = cam_irq_lookup(label, 0);
	if (!irq) {
		CAM_ERR(CAM_VMRM, "Lookup irq label failed %d", label);
		return;
	}

	accepted_irq = cam_vmrm_gh_irq_accept(label, -1, IRQ_TYPE_EDGE_RISING);
	if (accepted_irq < 0) {
		CAM_ERR(CAM_VMRM, "Accept irq failed for gh label: %d error: %d",
			label, accepted_irq);
		return;
	}
	CAM_DBG(CAM_VMRM, "Accept irq succeed for camera label: %d", irq->label);

	cam_irq_label = cam_gh_irq_label_convert_cam_irq_label(label);
	hw = cam_hw_instance_lookup(0, 0, cam_irq_label, 0);
	if (!hw) {
		CAM_ERR(CAM_VMRM, "Look up hw cam irq label failed %d", cam_irq_label);
		return;
	}

	spin_lock(&hw->spin_lock);
	for (i = 0; i < hw->resources.irq_count; i++) {
		if (cam_irq_label == hw->resources.irq_label[i])
			break;
	}
	hw->resources.ready_mask |= BIT(i + CAM_VMRM_RESOURCE_IRQ_BIT_MAP_OFFSET);
	cam_set_hw_irq_owner(hw->resources.irq_num[i], cam_vmrm_intf_get_vmid());
	if (hw->resources.ready_mask == hw->resources.valid_mask) {
		hw->vm_owner = cam_vmrm_intf_get_vmid();
		hw->is_using = true;
		hw->resources.ready_mask = 0;
		complete_all(&hw->wait_response);
		CAM_DBG(CAM_VMRM, "Acquire hw succeed 0x%x", hw->hw_id);
	}
	spin_unlock(&hw->spin_lock);

	rc = cam_vmrm_gh_irq_accept_notify(label);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq accept notify failed for label: %d error: %d", label, rc);
		return;
	}
	CAM_DBG(CAM_VMRM, "Irq accept notify succeed for gh label: %d", label);
}

static int cam_vmrm_dt_parse(struct platform_device *pdev,
	struct cam_vmrm_intf_dev *vmrm_intf_dev)
{
	int rc = 0;

	if (!pdev || !vmrm_intf_dev) {
		CAM_ERR(CAM_VMRM, "Invalid Input Params. pdev: %s, vmrm_intf_dev: %s",
			CAM_IS_NULL_TO_STR(pdev), CAM_IS_NULL_TO_STR(vmrm_intf_dev));
		return -EINVAL;
	}

	rc = of_property_read_u32(pdev->dev.of_node, "vmid", &g_vmrm_intf_dev->cam_vmid);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Cannot read the VM id. rc: %d", rc);
		return rc;
	}

	g_vmrm_intf_dev->gh_rr_enable = of_property_read_bool(pdev->dev.of_node, "gh_rr_enable");
	g_vmrm_intf_dev->no_register_read_on_bind = of_property_read_bool(pdev->dev.of_node,
		"no_register_read_on_bind");

	rc = of_property_read_u32(pdev->dev.of_node, "proxy_voting_enable",
		&g_vmrm_intf_dev->proxy_voting_enable);

	if (rc) {
		CAM_DBG(CAM_VMRM, "No found proxy_voting_enable");
		rc = 0;
	}

	CAM_DBG(CAM_VMRM,
		"VM-name: %d proxy voting enable 0x%x gh request resource enable %d no register read on bind %d",
		g_vmrm_intf_dev->cam_vmid, g_vmrm_intf_dev->proxy_voting_enable,
		g_vmrm_intf_dev->gh_rr_enable, g_vmrm_intf_dev->no_register_read_on_bind);

	return rc;
}

static int cam_vmrm_set_proxy_voting_enable(void *data, u64 val)
{
	g_vmrm_intf_dev->proxy_voting_enable = val;
	CAM_INFO(CAM_VMRM, "Set proxy_voting_enable value :%lld", val);

	return 0;
}

static int cam_vmrm_get_proxy_voting_enable(void *data, u64 *val)
{
	*val = g_vmrm_intf_dev->proxy_voting_enable;
	CAM_INFO(CAM_VMRM, "Get proxy_voting_enable value :%lld",
		g_vmrm_intf_dev->proxy_voting_enable);

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(cam_vmrm_proxy_voting_enable,
	cam_vmrm_get_proxy_voting_enable,
	cam_vmrm_set_proxy_voting_enable, "%16llu");

static int cam_vmrm_irq_lend_and_notify(uint32_t label, uint32_t vm_name,
	int irq, gh_irq_handle_fn_v2 handler, void *data)
{
	int rc = 0;
	enum gh_irq_label gh_label;
	enum gh_vm_names gh_vm;

	gh_label = cam_irq_label_convert_gh_label(label);
	gh_vm = cam_vmid_to_gh_name(vm_name);

	rc = cam_vmrm_gh_irq_lend_v2(gh_label, gh_vm, irq, handler, data);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq lend failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	rc = cam_vmrm_gh_irq_lend_notify(gh_label);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq lend notify failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Irq lend and notify label %d succeed", label);

	return rc;
}

static int cam_vmrm_irq_release_and_notify(uint32_t label)
{
	int rc = 0;
	enum gh_irq_label gh_label;

	gh_label = cam_irq_label_convert_gh_label(label);

	rc = cam_vmrm_gh_irq_release(gh_label);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq release failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	rc = cam_vmrm_gh_irq_release_notify(gh_label);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Irq release notify failed for label: %d, rc: %d", label, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Irq release succeed for label: %d", label);

	return rc;
}

static int cam_vmrm_mem_lend_and_notify(u8 mem_type, u8 flags, uint32_t label,
	struct gh_sgl_desc *sgl_desc, struct gh_mem_attr_desc *mem_attr_desc,
	gh_memparcel_handle_t *handle, uint32_t vm_name, uint32_t tag)

{
	int rc = 0;
	enum gh_vm_names gh_vm;
	enum gh_mem_notifier_tag gh_tag;
	struct gh_notify_vmid_desc *vmid_desc = NULL;
	struct gh_acl_desc *acl_desc = NULL;

	gh_vm = cam_vmid_to_gh_name(vm_name);
	gh_tag = cam_mem_tag_convert_gh_tag(tag);

	acl_desc = cam_populate_acl(gh_vm);
	if (IS_ERR(acl_desc)) {
		CAM_ERR(CAM_VMRM, "populate acl failed %d", PTR_ERR(acl_desc));
		return -EINVAL;
	}

	rc = cam_vmrm_gh_mem_lend(mem_type, flags, label,
		acl_desc, sgl_desc, mem_attr_desc, handle);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem lend failed label: %d, rc: %d", label, rc);
		goto free_acl_desc;
	}

	vmid_desc = cam_populate_vmid_desc(gh_vm);
	if (IS_ERR(vmid_desc)) {
		CAM_ERR(CAM_VMRM, "populate vmid desc failed %d", PTR_ERR(vmid_desc));
		rc = -EINVAL;
		goto free_acl_desc;
	}

	rc = cam_vmrm_gh_mem_notify(*handle, GH_RM_MEM_NOTIFY_RECIPIENT_SHARED, gh_tag, vmid_desc);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem lend notify failed tag: %d, rc: %d", tag, rc);
		goto free_vmid_desc;
	}

	CAM_DBG(CAM_VMRM, "Mem lend and notify label: %d succeed", label);

free_vmid_desc:
	CAM_MEM_FREE(vmid_desc);
free_acl_desc:
	CAM_MEM_FREE(acl_desc);
	return rc;
}

static int cam_vmrm_mem_release_and_notify(
	gh_memparcel_handle_t *handle, int32_t tag)

{
	int rc = 0;
	enum gh_mem_notifier_tag gh_tag;

	gh_tag = cam_mem_tag_convert_gh_tag(tag);

	rc = cam_vmrm_gh_mem_release(*handle, 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem release failed tag: %d, rc: %d", tag, rc);
		return rc;
	}

	rc = cam_vmrm_gh_mem_notify(*handle, GH_RM_MEM_NOTIFY_OWNER_RELEASED, gh_tag, 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Mem release notify tag: %d failed rc: %d", tag, rc);
		return rc;
	}

	CAM_DBG(CAM_VMRM, "Mem release and notify tag: %d succeed", tag);

	return rc;
}

struct cam_hw_instance *cam_check_hw_instance_available(
	uint32_t hw_id)
{
	struct cam_hw_instance *hw_pos;

	hw_pos = cam_hw_instance_lookup(hw_id, 0, 0, 0);
	if (!hw_pos) {
		CAM_ERR(CAM_VMRM, "do not find the hw instance 0x%x", hw_id);
		return hw_pos;
	}

	if (hw_pos->is_using && (hw_pos->vm_owner != cam_vmrm_intf_get_vmid())) {
		CAM_WARN(CAM_VMRM, "hw instance 0x%x is using by other vm %d current vm %d",
			hw_pos->hw_id, hw_pos->vm_owner, cam_vmrm_intf_get_vmid());
		return NULL;
	} else {
		return hw_pos;
	}
}

static void cam_clean_io_mem(struct list_head *mem_list)
{
	struct cam_io_mem_entry *pos, *tmp;

	if (!list_empty(mem_list)) {
		list_for_each_entry_safe(pos, tmp, mem_list, list) {
			list_del(&pos->list);
			CAM_MEM_FREE(pos);
		}
	}
}

static void cam_clean_io_irq(struct list_head *irq_list)
{
	struct cam_io_irq_entry *pos, *tmp;

	if (!list_empty(irq_list)) {
		list_for_each_entry_safe(pos, tmp, irq_list, list) {
			list_del(&pos->list);
			CAM_MEM_FREE(pos);
		}
	}
}

static void cam_clean_hw_instance(struct list_head *hw_list)
{
	struct cam_hw_instance *pos, *tmp;

	if (!list_empty(hw_list)) {
		list_for_each_entry_safe(pos, tmp, hw_list, list) {
			list_del(&pos->list);
			CAM_MEM_FREE(pos);
		}
	}
}

static void cam_clean_driver_node(struct list_head *driver_list)
{
	struct cam_driver_node *pos, *tmp;

	if (!list_empty(driver_list)) {
		list_for_each_entry_safe(pos, tmp, driver_list, list) {
			list_del(&pos->list);
			kfree(pos);
		}
	}
}

int cam_populate_irq_resource_info(struct cam_hw_instance *hw_pos)
{
	int i;
	struct cam_io_irq_entry *irq_entry;
	uint32_t irq_label;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	for (i = 0; i < hw_pos->resources.irq_count; i++) {
		irq_entry = CAM_MEM_ZALLOC(sizeof(*irq_entry), GFP_KERNEL);
		if (!irq_entry) {
			CAM_ERR(CAM_VMRM, "irq entry allocate memory failed");
			return -ENOMEM;
		}
		irq_entry->irq_num = hw_pos->resources.irq_num[i];
		irq_label = hw_pos->resources.irq_label[i];
		irq_entry->label = irq_label;
		irq_entry->vm_owner = CAM_PVM;

		INIT_LIST_HEAD(&irq_entry->list);
		mutex_lock(&vmrm_intf_dev->lock);
		list_add_tail(&irq_entry->list, &vmrm_intf_dev->io_res.irq);
		mutex_unlock(&vmrm_intf_dev->lock);
		CAM_DBG(CAM_VMRM, "hw name %s, hw id 0x%x irq label %d irq num %d",
			hw_pos->hw_name, hw_pos->hw_id, irq_entry->label, irq_entry->irq_num);
	}

	return 0;
}

int cam_populate_mem_resource_info(struct cam_hw_instance *hw_pos)
{
	int i, rc = 0;
	struct cam_io_mem_entry *mem_entry;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	mutex_lock(&vmrm_intf_dev->lock);
	for (i = 0; i < hw_pos->resources.num_mem_block; i++) {
		if (hw_pos->hw_id == CAM_HW_ID_CPAS) {
			if ((!strcmp(hw_pos->resources.mem_block_name[i], "cam_rpmh")) ||
				(!strcmp(hw_pos->resources.mem_block_name[i], "cam_cesta"))) {
				CAM_DBG(CAM_VMRM, "%s is other hw access range can not lend",
					hw_pos->resources.mem_block_name[i]);
				continue;
			}
		}
		rc = cam_mem_entry_is_duplicate(hw_pos->resources.mem_block_addr[i],
			hw_pos->resources.mem_block_size[i]);
		if (rc == 1) {
			CAM_DBG(CAM_VMRM, "mem entry 0x%x has been added",
				hw_pos->resources.mem_block_addr[i]);
			rc = 0;
			continue;
		} else if (rc == 2) {
			CAM_DBG(CAM_VMRM,
				"mem entry 0x%x size mismatch with previous and update mem size to max",
				hw_pos->resources.mem_block_addr[i]);
			rc = 0;
			continue;
		} else {
			CAM_DBG(CAM_VMRM, "hw id 0x%x new mem entry 0x%x", hw_pos->hw_id,
				hw_pos->resources.mem_block_addr[i]);
		}

		mem_entry = CAM_MEM_ZALLOC(sizeof(*mem_entry), GFP_KERNEL);
		if (!mem_entry) {
			CAM_ERR(CAM_VMRM, "io mem entry allocate memory failed");
			mutex_unlock(&vmrm_intf_dev->lock);
			return -ENOMEM;
		}
		mem_entry->base = hw_pos->resources.mem_block_addr[i];
		mem_entry->size = hw_pos->resources.mem_block_size[i];
		mem_entry->vm_owner = CAM_PVM;
		mem_entry->lend_in_progress = false;

		INIT_LIST_HEAD(&mem_entry->list);
		list_add_tail(&mem_entry->list, &vmrm_intf_dev->io_res.mem);
		CAM_DBG(CAM_VMRM, "hw name %s, mem address 0x%x size 0x%x",
			hw_pos->hw_name, mem_entry->base, mem_entry->size);
	}
	mutex_unlock(&vmrm_intf_dev->lock);

	return 0;
}

int cam_populate_gpio_resource_info(struct cam_hw_instance *hw_pos)
{
	int rc = 0, i;
	struct cam_io_mem_entry *mem_entry;
	struct resource res;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	mutex_lock(&vmrm_intf_dev->lock);
	if (hw_pos->resources.gpio_count) {
		for (i = 0; i < hw_pos->resources.gpio_count; i++) {
			rc = msm_gpio_get_pin_address(hw_pos->resources.gpio_num[i], &res);
			if (!rc) {
				CAM_ERR(CAM_VMRM, "invalid gpio %d",
					hw_pos->resources.gpio_num[i]);
				goto end;
			}
			rc = 0;

			rc = cam_mem_entry_is_duplicate(res.start, resource_size(&res));
			if (rc == 1) {
				CAM_DBG(CAM_VMRM, "gpio num %x mem entry 0x%x has been added",
					hw_pos->resources.gpio_num[i], res.start);
				rc = 0;
				continue;
			} else if (rc == 2) {
				CAM_DBG(CAM_VMRM,
					"mem entry 0x%x size mismatch with previous and update mem size to max",
					hw_pos->resources.mem_block_addr[i]);
				rc = 0;
				continue;
			} else {
				CAM_DBG(CAM_VMRM, "hw id 0x%x new gpio mem entry 0x%x",
					hw_pos->hw_id, hw_pos->resources.gpio_num[i]);
			}

			mem_entry = CAM_MEM_ZALLOC(sizeof(*mem_entry), GFP_KERNEL);
			if (!mem_entry) {
				rc = -ENOMEM;
				CAM_ERR(CAM_VMRM, "gpio mem allocate memory failed");
				goto end;
			}

			mem_entry->base = res.start;
			mem_entry->size = resource_size(&res);
			mem_entry->vm_owner = CAM_PVM;
			mem_entry->lend_in_progress = false;
			hw_pos->resources.gpio_mem_addr[i] = mem_entry->base;
			hw_pos->resources.gpio_mem_size[i] = mem_entry->size;

			INIT_LIST_HEAD(&mem_entry->list);
			list_add_tail(&mem_entry->list, &vmrm_intf_dev->io_res.mem);
			CAM_DBG(CAM_VMRM, "hw name %s, gpio num %d mem address 0x%x size 0x%x",
				hw_pos->hw_name, hw_pos->resources.gpio_num[i], mem_entry->base,
				mem_entry->size);
		}
	}

end:
	mutex_unlock(&vmrm_intf_dev->lock);
	return rc;
}

void cam_free_io_resource_info(void)
{
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	mutex_lock(&vmrm_intf_dev->lock);
	if (!list_empty(&vmrm_intf_dev->io_res.mem))
		cam_clean_io_mem(&vmrm_intf_dev->io_res.mem);

	if (!list_empty(&vmrm_intf_dev->io_res.irq))
		cam_clean_io_irq(&vmrm_intf_dev->io_res.irq);
	mutex_unlock(&vmrm_intf_dev->lock);
}

static int cam_vmrm_lend_irq_resources(struct cam_hw_instance *hw_pos, uint32_t vmid)
{
	int rc = 0, i;
	struct cam_io_irq_entry *irq, *irq_temp;

	for (i = 0; i < hw_pos->resources.irq_count; i++) {
		if (!list_empty(&g_vmrm_intf_dev->io_res.irq)) {
			list_for_each_entry_safe(irq, irq_temp,
				&g_vmrm_intf_dev->io_res.irq, list) {
				if (irq->irq_num == hw_pos->resources.irq_num[i] &&
					irq->vm_owner == CAM_PVM) {
					rc = cam_vmrm_irq_lend_and_notify(irq->label, vmid,
						irq->irq_num, cam_irq_notification_pvm_handler,
						NULL);
					if (rc) {
						CAM_ERR(CAM_VMRM,
							"irq lend and notify irq failed label %d rc: %d",
							irq->label, rc);
						goto end;
					}
					break;
				}
			}
		}
	}

end:
	return rc;
}

static int cam_vmrm_lend_mem_resources(struct cam_hw_instance *hw_pos, uint32_t vmid)
{
	int rc = 0, i = 0, j = 0;
	struct gh_sgl_desc *sgl_desc = NULL;
	struct cam_io_mem_entry *mem, *mem_temp;

	sgl_desc = CAM_MEM_ZALLOC(offsetof(struct gh_sgl_desc,
		sgl_entries[hw_pos->resources.num_mem_block + hw_pos->resources.gpio_count]),
		GFP_KERNEL);
	if (!sgl_desc) {
		CAM_ERR(CAM_VMRM, "Could not reserve memory for SGL entries");
		rc = -ENOMEM;
		goto end;
	}

	mutex_lock(&g_vmrm_intf_dev->lock);
	if (hw_pos->resources.num_mem_block) {
		for (i = 0; i < hw_pos->resources.num_mem_block; i++) {
			if (!list_empty(&g_vmrm_intf_dev->io_res.mem)) {
				list_for_each_entry_safe(mem, mem_temp,
					&g_vmrm_intf_dev->io_res.mem, list) {
					if (mem->base == hw_pos->resources.mem_block_addr[i] &&
						mem->vm_owner == CAM_PVM &&
						(!mem->lend_in_progress)) {
						sgl_desc->n_sgl_entries = j + 1;
						sgl_desc->sgl_entries[j].ipa_base =
							ALIGN_DOWN(mem->base, PAGE_SIZE);
						sgl_desc->sgl_entries[j].size =
							ALIGN(mem->size, PAGE_SIZE);
						mem->lend_in_progress = true;
						CAM_DBG(CAM_VMRM, "base 0x%x size 0x%x index %d",
							sgl_desc->sgl_entries[j].ipa_base,
							sgl_desc->sgl_entries[j].size, j);
						j = j + 1;
						break;
					}
				}
			}
		}
	}

	if (hw_pos->resources.gpio_count) {
		for (i = 0; i < hw_pos->resources.gpio_count; i++) {
			if (!list_empty(&g_vmrm_intf_dev->io_res.mem)) {
				list_for_each_entry_safe(mem, mem_temp,
					&g_vmrm_intf_dev->io_res.mem, list) {
					if ((mem->base == hw_pos->resources.gpio_mem_addr[i]) &&
						mem->vm_owner == CAM_PVM &&
						(!mem->lend_in_progress)) {
						sgl_desc->n_sgl_entries = j + 1;
						sgl_desc->sgl_entries[j].ipa_base = mem->base;
						sgl_desc->sgl_entries[j].size = mem->size;
						mem->lend_in_progress = true;
						CAM_DBG(CAM_VMRM, "base 0x%x size 0x%x index %d",
							sgl_desc->sgl_entries[j].ipa_base,
							sgl_desc->sgl_entries[j].size, j);
						j = j + 1;
						break;
					}
				}
			}
		}
	}
	mutex_unlock(&g_vmrm_intf_dev->lock);

	if (!sgl_desc->n_sgl_entries) {
		CAM_DBG(CAM_VMRM, "Not mem need to lend hw id 0x%x", hw_pos->hw_id);
		CAM_MEM_FREE(sgl_desc);
		return 0;
	}

	rc = cam_vmrm_mem_lend_and_notify(CAM_MEM_TYPE_IO, 0, hw_pos->resources.mem_label,
		sgl_desc, NULL, &hw_pos->handle, vmid, hw_pos->resources.mem_tag);
	if (rc)
		CAM_ERR(CAM_VMRM,
			"hw_id 0x%x mem lend and notify mem label %d rc: %d",
			hw_pos->hw_id, hw_pos->resources.mem_label, rc);

	CAM_MEM_FREE(sgl_desc);
end:
	return rc;
}

static int cam_vmrm_lend_resources(uint32_t hw_id, uint32_t vmid)
{
	int rc = 0;
	struct cam_hw_instance *hw_pos;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	hw_pos = cam_hw_instance_lookup(hw_id, 0, 0, 0);
	if (!hw_pos) {
		CAM_ERR(CAM_VMRM, "Do not find hw instance %x", hw_id);
		return -EINVAL;
	}

	spin_lock(&hw_pos->spin_lock);
	if (atomic_read(&hw_pos->lend_in_progress)) {
		CAM_ERR(CAM_VMRM, "Hw lend is in progress 0x%x", hw_pos->hw_id);
		spin_unlock(&hw_pos->spin_lock);
		return -EINVAL;
	}

	atomic_set(&hw_pos->lend_in_progress, 1);
	spin_unlock(&hw_pos->spin_lock);

	rc = cam_vmrm_lend_irq_resources(hw_pos, vmid);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Lend irq resources failed hw instance %x vmid %d rc %d",
			hw_id, vmid, rc);
		goto end;
	}

	rc = cam_vmrm_lend_mem_resources(hw_pos, vmid);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Lend mem resources failed hw instance %x vmid %d rc %d",
			hw_id, vmid, rc);
		goto end;
	}

	return rc;
end:
	spin_lock(&hw_pos->spin_lock);
	atomic_set(&hw_pos->lend_in_progress, 0);
	spin_unlock(&hw_pos->spin_lock);
	return rc;
}

static int cam_vmrm_set_resource_state(uint32_t msg_dst_id, bool state)
{
	struct cam_hw_instance *hw_pos;

	mutex_lock(&g_vmrm_intf_dev->lock);
	hw_pos = cam_hw_instance_lookup(msg_dst_id, 0, 0, 0);
	if (!hw_pos) {
		CAM_ERR(CAM_VMRM, "do not find the hw instance 0x%x", msg_dst_id);
		mutex_unlock(&g_vmrm_intf_dev->lock);
		return -EINVAL;
	}
	hw_pos->is_using = state;
	mutex_unlock(&g_vmrm_intf_dev->lock);

	return 0;
}

static int cam_vmrm_release_irq_resources(struct cam_hw_instance *hw_pos)
{
	int rc = 0, i;
	struct cam_io_irq_entry *irq, *irq_temp;

	for (i = 0; i < hw_pos->resources.irq_count; i++) {
		if (!list_empty(&g_vmrm_intf_dev->io_res.irq)) {
			list_for_each_entry_safe(irq, irq_temp,
				&g_vmrm_intf_dev->io_res.irq, list) {
				if ((irq->irq_num == hw_pos->resources.irq_num[i]) &&
					(irq->vm_owner != CAM_PVM)) {
					rc = cam_vmrm_irq_release_and_notify(irq->label);
					if (rc) {
						CAM_ERR(CAM_VMRM,
							"irq release and notify irq failed label %d rc: %d",
							irq->label, rc);
						goto end;
					}
					cam_set_hw_irq_owner(irq->irq_num, CAM_PVM);
					break;
				}
			}
		}
	}

end:
	return rc;
}

static int cam_vmrm_release_mem_resources(struct cam_hw_instance *hw_pos)
{
	int rc = 0;
	uint32_t handle, tag, hw_id;

	handle = hw_pos->handle;
	tag = hw_pos->resources.mem_tag;
	hw_id = hw_pos->hw_id;

	rc = cam_vmrm_mem_release_and_notify(&handle, tag);
	if (rc)
		CAM_ERR(CAM_VMRM, "hw_id mem release and notify mem tag %d rc: %d",
			hw_id, tag, rc);

	return rc;
}

int cam_vmrm_release_resources(uint32_t hw_id)
{
	int rc = 0;
	struct cam_hw_instance *hw_pos;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	hw_pos = cam_hw_instance_lookup(hw_id, 0, 0, 0);
	if (!hw_pos) {
		CAM_ERR(CAM_VMRM, "Do not find hw instance %x", hw_id);
		return -EINVAL;
	}

	rc = cam_vmrm_release_irq_resources(hw_pos);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Release irq resources failed hw instance %x vmid %d rc %d",
			hw_id, cam_vmrm_intf_get_vmid(), rc);
		goto end;
	}

	rc = cam_vmrm_release_mem_resources(hw_pos);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Release mem resources failed hw instance %x vmid %d rc %d",
			hw_id, cam_vmrm_intf_get_vmid(), rc);
		goto end;
	}

	spin_lock(&hw_pos->spin_lock);
	cam_set_hw_mem_owner(hw_pos, CAM_PVM);
	hw_pos->vm_owner = CAM_PVM;
	hw_pos->is_using = false;
	spin_unlock(&hw_pos->spin_lock);

	CAM_DBG(CAM_VMRM, "Release resources succeed hw instance 0x%x vmid %d", hw_id,
		cam_vmrm_intf_get_vmid());
end:
	return rc;
}

static int cam_vmrm_hw_instance_generic_callback(void *cb_data, void *msg,
	uint32_t size)
{
	int rc = 0;
	uint32_t msg_type, hw_id;
	struct cam_hw_soc_info            *soc_info = NULL;
	unsigned long cesta_clk_rate_high = 0, cesta_clk_rate_low = 0;
	int cesta_client_idx = -1;
	struct cam_msg_set_clk_rate *msg_set_clk_rate = NULL;
	struct cam_vmrm_msg *hw_msg = NULL;

	hw_msg = msg;
	hw_id = hw_msg->msg_dst_id;

	soc_info = (struct cam_hw_soc_info *)cb_data;

	msg_type = hw_msg->msg_type;

	CAM_DBG(CAM_VMRM, "hw id:0x%x, msg type %d", hw_id, msg_type);

	switch (msg_type) {
	case CAM_SOC_ENABLE_RESOURCE:
		rc = cam_soc_util_enable_platform_resource(soc_info, CAM_CLK_SW_CLIENT_IDX,
			true, soc_info->lowest_clk_level, false);
		if (rc)
			CAM_ERR(CAM_VMRM, "hw id:0x%x soc enable resource failed", hw_id);
		break;

	case CAM_SOC_DISABLE_RESOURCE:
		rc = cam_soc_util_disable_platform_resource(soc_info, CAM_CLK_SW_CLIENT_IDX,
			true, false);
		if (rc)
			CAM_ERR(CAM_VMRM, "hw id:0x%x soc disable resource failed", hw_id);
		break;

	case CAM_CLK_SET_RATE:
		msg_set_clk_rate = (struct cam_msg_set_clk_rate *)&hw_msg->data[0];
		cesta_client_idx = msg_set_clk_rate->cesta_client_idx;
		cesta_clk_rate_high = msg_set_clk_rate->clk_rate_high;
		cesta_clk_rate_low = msg_set_clk_rate->clk_rate_low;
		CAM_DBG(CAM_VMRM, "hw id:0x%x, client idx %d, clk[high low]=[%lu %lu]", hw_id,
			cesta_client_idx, cesta_clk_rate_high, cesta_clk_rate_low);
		rc = cam_soc_util_set_src_clk_rate(soc_info, cesta_client_idx,
			cesta_clk_rate_high, cesta_clk_rate_low);
		if (rc)
			CAM_ERR(CAM_VMRM, "hw id:0x%x clk set rate failed", hw_id);
		break;

	default:
		rc = -EINVAL;
		CAM_ERR(CAM_VMRM, "hw id:0x%x Error, Invalid msg type:%d", hw_id, msg_type);
		break;
	}

	return rc;
}

void cam_vmrm_msg_handle(void *msg, size_t size, struct cam_intervm_response *res)
{
	int rc = 0;
	uint32_t data_size, msg_type, msg_dst_type, msg_size;
	struct cam_vmrm_msg *vm_msg, *res_msg_local;
	struct cam_hw_instance *hw_pos;
	struct cam_driver_node *driver_pos;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;
	bool response_msg;
	bool need_response;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	vm_msg = msg;
	data_size = vm_msg->data_size;
	res->send_response = false;
	CAM_DBG(CAM_VMRM,
		"msg_size %d data_size %d source_vmid %d des_vmid %d dst_type %d dst_id %x msg_type %d response msg %d need response %d",
		size, vm_msg->data_size, vm_msg->source_vmid, vm_msg->des_vmid,
		vm_msg->msg_dst_type, vm_msg->msg_dst_id, vm_msg->msg_type, vm_msg->response_msg,
		vm_msg->need_response);

	if (offsetof(struct cam_vmrm_msg, data[data_size]) != size) {
		CAM_ERR(CAM_VMRM, "received msg size not match");
		return;
	}

	vm_msg = kmemdup(msg, size, GFP_ATOMIC);
	if (!vm_msg) {
		CAM_ERR(CAM_VMRM, "msg mem allocate failed");
		return;
	}

	msg_type = vm_msg->msg_type;
	msg_dst_type = vm_msg->msg_dst_type;
	response_msg = vm_msg->response_msg;
	need_response = vm_msg->need_response;

	/* actual message */
	if (!response_msg) {
		if (msg_dst_type == CAM_MSG_DST_TYPE_HW_INSTANCE) {
			mutex_lock(&vmrm_intf_dev->lock);
			hw_pos = cam_hw_instance_lookup(vm_msg->msg_dst_id, 0, 0, 0);
			if (hw_pos) {
				if (hw_pos->hw_msg_callback)
					rc = hw_pos->hw_msg_callback(hw_pos->hw_msg_callback_data,
						vm_msg, size);
				else
					rc = cam_vmrm_hw_instance_generic_callback(
						hw_pos->hw_msg_callback_data, vm_msg, size);
				if (!rc)
					CAM_DBG(CAM_VMRM, "hw id 0x%x msg type %d handle succeed",
						vm_msg->msg_dst_id, vm_msg->msg_type);
				else
					CAM_ERR(CAM_VMRM,
						"hw id 0x%x msg type %d handle failed %d",
						vm_msg->msg_dst_id, vm_msg->msg_type, rc);
			} else {
				CAM_ERR(CAM_VMRM, "not found the hw %x", vm_msg->msg_dst_id);
				mutex_unlock(&vmrm_intf_dev->lock);
				goto free_recv_msg;
			}
			mutex_unlock(&vmrm_intf_dev->lock);
		} else if (msg_dst_type ==
			CAM_MSG_DST_TYPE_DRIVER_NODE) {
			mutex_lock(&vmrm_intf_dev->lock);
			driver_pos = cam_driver_node_lookup(vm_msg->msg_dst_id);
			if (driver_pos) {
				rc = driver_pos->driver_msg_callback(
					driver_pos->driver_msg_callback_data, vm_msg, size);
				if (!rc)
					CAM_DBG(CAM_VMRM,
						"driver id 0x%x msg type %d handle succeed",
						vm_msg->msg_dst_id, vm_msg->msg_type);
				else
					CAM_ERR(CAM_VMRM,
						"hw id 0x%x msg type %d handle failed %d",
						vm_msg->msg_dst_id, vm_msg->msg_type, rc);
			} else {
				CAM_ERR(CAM_VMRM, "not found the driver %x", vm_msg->msg_dst_id);
				mutex_unlock(&vmrm_intf_dev->lock);
				goto free_recv_msg;
			}
			mutex_unlock(&vmrm_intf_dev->lock);
		} else if (msg_dst_type ==
			CAM_MSG_DST_TYPE_VMRM) {
			if (msg_type == CAM_HW_RESOURCE_ACQUIRE) {
				rc = cam_vmrm_lend_resources(vm_msg->msg_dst_id,
					vm_msg->source_vmid);
				if (!rc)
					CAM_DBG(CAM_VMRM,
						"lend resources succeed hw id 0x%x vmid %d",
						vm_msg->msg_dst_id, vm_msg->source_vmid);
				else
					CAM_ERR(CAM_VMRM,
						"lend resources failed %d hw id 0x%x vmid %d",
						rc, vm_msg->msg_dst_id, vm_msg->source_vmid);
			} else if (msg_type == CAM_HW_RESOURCE_SET_ACQUIRE) {
				rc = cam_vmrm_set_resource_state(vm_msg->msg_dst_id, true);
				if (!rc)
					CAM_DBG(CAM_VMRM,
						"vmrm set acquire state succeed hw id 0x%x vmid %d",
						vm_msg->msg_dst_id, cam_vmrm_intf_get_vmid());
				else
					CAM_ERR(CAM_VMRM,
						"mrm set acquire state failed %d hw id 0x%x vmid %d",
						rc, vm_msg->msg_dst_id, cam_vmrm_intf_get_vmid());
				goto free_recv_msg;
			} else if (msg_type == CAM_HW_RESOURCE_SET_RELEASE) {
				rc = cam_vmrm_set_resource_state(vm_msg->msg_dst_id, false);
				if (!rc)
					CAM_DBG(CAM_VMRM,
						"vmrm set release state succeed hw id 0x%x vmid %d",
						vm_msg->msg_dst_id, cam_vmrm_intf_get_vmid());
				else
					CAM_ERR(CAM_VMRM,
						"mrm set release state failed %d hw id 0x%x vmid %d",
						rc, vm_msg->msg_dst_id, cam_vmrm_intf_get_vmid());
				goto free_recv_msg;
			} else {
				CAM_ERR(CAM_VMRM, "Not supported msg type %d", msg_type);
				goto free_recv_msg;
			}
		} else {
			CAM_ERR(CAM_VMRM, "Not supported msg dst type", msg_dst_type);
			goto free_recv_msg;
		}
		/* this memory is freed inside qrtr layer */
		if (need_response) {
			msg_size = offsetof(struct cam_vmrm_msg, data[1]);
			res_msg_local = CAM_MEM_ZALLOC((msg_size), GFP_KERNEL);
			if (!res_msg_local) {
				CAM_ERR(CAM_VMRM, "res msg mem allocate failed");
				goto free_recv_msg;
			}
			res_msg_local->response_result = rc;
			res_msg_local->msg_dst_type = msg_dst_type;
			res_msg_local->msg_dst_id = vm_msg->msg_dst_id;
			res_msg_local->msg_type = msg_type;
			res_msg_local->data_size = 1;
			res_msg_local->response_msg = true;
			res_msg_local->need_response = false;
			res->send_response = true;
			res->res_msg = res_msg_local;
			res->msg_size = msg_size;
			CAM_DBG(CAM_VMRM,
				"send response msg dst type %d dst id 0x%x msg type %d result %d",
				res_msg_local->msg_dst_type, res_msg_local->msg_dst_id,
				res_msg_local->msg_type, res_msg_local->response_result);
		}
	} else {
		if (msg_dst_type == CAM_MSG_DST_TYPE_HW_INSTANCE) {
			mutex_lock(&vmrm_intf_dev->lock);
			hw_pos = cam_hw_instance_lookup(vm_msg->msg_dst_id, 0, 0, 0);
			if (!hw_pos) {
				CAM_ERR(CAM_VMRM, "not found the hw %x", vm_msg->msg_dst_id);
				mutex_unlock(&vmrm_intf_dev->lock);
				goto free_recv_msg;
			}
			mutex_unlock(&vmrm_intf_dev->lock);
			spin_lock(&hw_pos->spin_lock);
			hw_pos->response_result = vm_msg->response_result;
			complete_all(&hw_pos->wait_response);
			spin_unlock(&hw_pos->spin_lock);
		} else if (msg_dst_type == CAM_MSG_DST_TYPE_DRIVER_NODE) {
			mutex_lock(&vmrm_intf_dev->lock);
			driver_pos = cam_driver_node_lookup(vm_msg->msg_dst_id);
			if (driver_pos) {
				complete_all(&driver_pos->wait_response);
				driver_pos->response_result = vm_msg->response_result;
			} else {
				CAM_ERR(CAM_VMRM, "not found the driver id %x", vm_msg->msg_dst_id);
				mutex_unlock(&vmrm_intf_dev->lock);
				goto free_recv_msg;
			}
			mutex_unlock(&vmrm_intf_dev->lock);
		} else if (msg_dst_type == CAM_MSG_DST_TYPE_VMRM) {
			hw_pos = cam_hw_instance_lookup(vm_msg->msg_dst_id, 0, 0, 0);
			if (!hw_pos) {
				CAM_ERR(CAM_VMRM, "not found the hw %x", vm_msg->msg_dst_id);
				goto free_recv_msg;
			}
			/*
			 * when result failure signal completion or result succeed tvm callback
			 * to signal
			 */
			spin_lock(&hw_pos->spin_lock);
			hw_pos->response_result = vm_msg->response_result;
			if (hw_pos->response_result)
				complete_all(&hw_pos->wait_response);
			spin_unlock(&hw_pos->spin_lock);
		} else {
			CAM_ERR(CAM_VMRM, "Not supported msg dst type", msg_dst_type);
		}
	}

free_recv_msg:
	kfree(vm_msg);
}

#ifdef CONFIG_SPECTRA_RESOURCE_REQUEST_ENABLE
int cam_vmrm_resource_cb(gh_vmid_t source_vmid, bool is_req,
	struct gh_res_request *resource, int resource_cnt)
{
	int rc = 0, i;
	struct cam_io_irq_entry *irq;
	enum gh_vm_names vm_name;
	uint32_t cam_vmid;
	struct cam_hw_instance *hw;
	bool resource_include_irq = false;
	int resource_irq_index = 0;
	int resource_gpio_index = 0;

	rc = cam_vmrm_gh_rm_get_vm_name(source_vmid, &vm_name);
	if (rc) {
		CAM_ERR(CAM_VMRM, "get vm name failed %d", source_vmid);
		return rc;
	}

	cam_vmid = cam_gh_name_to_vmid(vm_name);
	if (cam_vmid == CAM_VM_MAX) {
		CAM_ERR(CAM_VMRM, "get cam vmid failed %d", vm_name);
		return -EINVAL;
	}

	for (i = 0; i < resource_cnt; i++) {
		if (resource[i].resource_type == GH_RESOURCE_IRQ) {
			resource_include_irq = true;
			resource_irq_index = i;
			CAM_DBG(CAM_VMRM, "resource include irq index %d", resource_irq_index);
			break;
		}
	}

	if (!resource_include_irq) {
		for (i = 0; i < resource_cnt; i++) {
			if (resource[i].resource_type == GH_RESOURCE_GPIO) {
				resource_gpio_index = i;
				CAM_DBG(CAM_VMRM, "resource include gpio index %d",
					resource_gpio_index);
				break;
			}
		}
	}

	if (resource_include_irq) {
		irq = cam_irq_lookup(0, resource[resource_irq_index].resource.irq_num);
		if (!irq) {
			CAM_ERR(CAM_VMRM, "lookup irq %d failed",
				resource[resource_irq_index].resource.irq_num);
			return -EINVAL;
		}
		hw = cam_hw_instance_lookup(0, 0, irq->label, 0);
		if (!hw) {
			CAM_ERR(CAM_VMRM, "Do not find hw instance irq label %d", irq->label);
			return -EINVAL;
		}
	} else {
		hw = cam_hw_instance_lookup(0, 0, 0,
			resource[resource_gpio_index].resource.gpio_num);
		if (!hw) {
			CAM_ERR(CAM_VMRM, "Do not find hw instance gpio num %d",
				resource[resource_gpio_index].resource.gpio_num);
			return -EINVAL;
		}
	}

	/* request from svm or release from pvm*/
	if (is_req) {
		rc = cam_vmrm_lend_resources(hw->hw_id, cam_vmid);
		if (!rc)
			CAM_DBG(CAM_VMRM,
				"lend resources succeed hw id 0x%x vmid %d",
				hw->hw_id, cam_vmid);
		else
			CAM_ERR(CAM_VMRM,
				"lend resources failed %d hw id 0x%x vmid %d",
				rc, hw->hw_id, cam_vmid);
	} else {
		rc = cam_vmrm_release_resources(hw->hw_id);
		if (!rc)
			CAM_DBG(CAM_VMRM,
				"release resources succeed hw id 0x%x vmid %d",
				hw->hw_id, cam_vmid);
		else
			CAM_ERR(CAM_VMRM,
				"release resources failed %d hw id 0x%x vmid %d",
				rc, hw->hw_id, cam_vmid);
	}

	return rc;
}

int cam_vmrm_resource_req(struct cam_hw_instance *hw_pos, bool is_req)
{
	int rc = 0;
	int i = 0, j;
	int resource_cnt;
	struct gh_res_request *resource;
	gh_vmid_t vmid;
	enum gh_vm_names vmname;

	resource_cnt = hw_pos->resources.resource_cnt;
	resource = CAM_ZALLOC_ARRAY(resource_cnt, sizeof(*resource), GFP_KERNEL);
	if (!resource) {
		CAM_ERR(CAM_VMRM, "no memory for hw id 0x%x", hw_pos->hw_id);
		return -ENOMEM;
	}

	for (j = 0; j < hw_pos->resources.gpio_count; j++, i++) {
		resource[i].resource_type = GH_RESOURCE_GPIO;
		resource[i].resource.gpio_num = hw_pos->resources.gpio_num[j];
		CAM_DBG(CAM_VMRM, "hw id 0x%x index %d gpio num %d", hw_pos->hw_id, i,
			resource[i].resource.gpio_num);
	}

	for (j = 0; j < hw_pos->resources.num_mem_block ; j++, i++) {
		resource[i].resource_type = GH_RESOURCE_IOMEM;
		resource[i].resource.sgl_entry.ipa_base = hw_pos->resources.mem_block_addr[j];
		resource[i].resource.sgl_entry.size = hw_pos->resources.mem_block_size[j];
		CAM_DBG(CAM_VMRM, "hw id 0x%x index %d mem addr 0x%x size 0x%x", hw_pos->hw_id, i,
			resource[i].resource.sgl_entry.ipa_base,
			resource[i].resource.sgl_entry.size);
	}

	for (j = 0; j < hw_pos->resources.irq_count; j++, i++) {
		resource[i].resource_type = GH_RESOURCE_IRQ;
		resource[i].resource.irq_num = hw_pos->resources.irq_num[j];
		CAM_DBG(CAM_VMRM, "hw id 0x%x index %d irq num %d", hw_pos->hw_id, i,
			resource[i].resource.irq_num);
	}

	vmname = cam_vmid_to_gh_name(CAM_PVM);
	if (vmname == GH_VM_MAX) {
		CAM_ERR(CAM_VMRM, "get gh vm name failed");
		rc = -EINVAL;
		goto free_res;
	}

	rc = cam_vmrm_ghd_rm_get_vmid(vmname, &vmid);
	if (rc) {
		CAM_ERR(CAM_VMRM, "get vmid failed %d", rc);
		goto free_res;
	}

	if (is_req) {
		rc = cam_vmrm_gh_resource_request(vmid, CAM_RESOURCE_REQ_CLIENT_NAME,
			resource, resource_cnt);
		if (rc)
			CAM_ERR(CAM_VMRM, "svm call resource request failed %d", rc);
	} else {
		rc = cam_vmrm_gh_resource_release(vmid, CAM_RESOURCE_REQ_CLIENT_NAME,
			resource, resource_cnt);
		if (rc)
			CAM_ERR(CAM_VMRM, "pvm call resource request failed %d", rc);
	}

free_res:
	CAM_MEM_FREE(resource);

	return rc;
}

int cam_register_gh_res_callback(void)
{
	int rc = 0;
	struct gh_resource_client *client;

	client = CAM_MEM_ZALLOC(sizeof(*client), GFP_KERNEL);
	if (!client) {
		CAM_ERR(CAM_VMRM, " gh resource client allocate memory failed");
		return -ENOMEM;
	}

	client->cb = cam_vmrm_resource_cb;
	scnprintf(client->subsys_name, sizeof(client->subsys_name), "%s",
		CAM_RESOURCE_REQ_CLIENT_NAME);

	if (CAM_IS_PRIMARY_VM()) {
		rc = cam_vmrm_gh_resource_register_req_client(client);
		if (rc) {
			CAM_ERR(CAM_VMRM, "PVM resource register req cb failed rc: %d", rc);
			CAM_MEM_FREE(client);
			goto end;
		}
		CAM_DBG(CAM_VMRM, "PVM resource register req cb succeed");
	} else {
		rc = cam_vmrm_gh_resource_register_release_client(client);
		if (rc) {
			CAM_ERR(CAM_VMRM, "SVM resource register release cb failed rc: %d", rc);
			CAM_MEM_FREE(client);
			goto end;
		}
		CAM_DBG(CAM_VMRM, "SVM resource register release cb succeed");
	}

	g_vmrm_intf_dev->gh_res_client = client;

end:
	return rc;
}

int cam_unregister_gh_res_callback(void)
{
	int rc = 0;
	struct gh_resource_client *client;

	client = g_vmrm_intf_dev->gh_res_client;
	if (!client) {
		CAM_ERR(CAM_VMRM, "client is invalid");
		return -EINVAL;
	}

	if (CAM_IS_PRIMARY_VM()) {
		rc = cam_vmrm_gh_resource_unregister_req_client(client);
		if (rc) {
			CAM_ERR(CAM_VMRM, "PVM resource unregister req failed rc: %d", rc);
			goto end;
		}
		CAM_DBG(CAM_VMRM, "PVM resource unregister req succeed");
	} else {
		rc = cam_vmrm_gh_resource_unregister_release_client(client);
		if (rc) {
			CAM_ERR(CAM_VMRM, "SVM resource unregister release failed rc: %d", rc);
			goto end;
		}
		CAM_DBG(CAM_VMRM, "SVM resource unregister release succeed");
	}

end:
	CAM_MEM_FREE(client);
	return rc;
}
#else
int cam_vmrm_resource_cb(void)
{
	return -EOPNOTSUPP;
}

int cam_vmrm_resource_req(struct cam_hw_instance *hw_pos, bool is_req)
{
	return -EOPNOTSUPP;
}

int cam_register_gh_res_callback(void)
{
	return -EOPNOTSUPP;
}

int cam_unregister_gh_res_callback(void)
{
	return -EOPNOTSUPP;
}
#endif

void cam_unregister_gh_mem_callback(void)
{
	struct cam_hw_instance *hw, *hw_temp;

	if (!list_empty(&g_vmrm_intf_dev->hw_instance)) {
		list_for_each_entry_safe(hw, hw_temp,
			&g_vmrm_intf_dev->hw_instance, list) {
			if (hw->mem_notify_handle) {
				cam_vmrm_gh_mem_unregister_notifier(hw->mem_notify_handle);
				CAM_DBG(CAM_VMRM, "RM unregister mem tag %d notifier succeed",
					hw->resources.mem_tag);
			}
		}
	}
}

int cam_register_gh_mem_callback(void)
{
	int rc = 0;
	struct cam_hw_instance *hw, *hw_temp;
	uint32_t cam_mem_tag;
	enum gh_mem_notifier_tag gh_mem_tag;
	gh_mem_notifier_handler handler;

	if (CAM_IS_PRIMARY_VM())
		handler = cam_mem_notification_pvm_handler;
	else
		handler = cam_mem_notification_svm_handler;

	if (!list_empty(&g_vmrm_intf_dev->hw_instance)) {
		list_for_each_entry_safe(hw, hw_temp,
			&g_vmrm_intf_dev->hw_instance, list) {
			cam_mem_tag = hw->resources.mem_tag;
			gh_mem_tag = cam_mem_tag_convert_gh_tag(cam_mem_tag);
			if ((gh_mem_tag >= GH_MEM_NOTIFIER_TAG_CAM_BASE) &&
				(gh_mem_tag < GH_MEM_NOTIFIER_TAG_MAX)) {
				rc = cam_vmrm_gh_mem_register_notifier(gh_mem_tag, handler, NULL,
					&hw->mem_notify_handle);
				if (rc) {
					CAM_ERR(CAM_VMRM,
						"VM %d register mem %d notifier failed rc: %d",
						cam_vmrm_intf_get_vmid(), cam_mem_tag, rc);
					goto end;
				}
				CAM_DBG(CAM_VMRM,
					"VM %d register hw_id 0x%x mem tag %d notifier succeed",
					cam_vmrm_intf_get_vmid(), hw->hw_id, cam_mem_tag);
			} else {
				CAM_ERR(CAM_VMRM, "invalid cam mem tag %d", cam_mem_tag);
				rc = -EINVAL;
				goto end;
			}
		}
	}

end:
	return rc;
}

int cam_register_gh_irq_callback(void)
{
	int rc = 0;
	enum gh_vm_names vmname;
	enum gh_irq_label gh_irq_label_local;
	struct cam_io_irq_entry *irq, *irq_temp;

	if (CAM_IS_SECONDARY_VM()) {
		vmname = cam_vmid_to_gh_name(CAM_PVM);
		if (vmname == GH_VM_MAX) {
			CAM_ERR(CAM_VMRM, "get vmid failed");
			rc = -EINVAL;
			goto end;
		}

		if (!list_empty(&g_vmrm_intf_dev->io_res.irq)) {
			list_for_each_entry_safe(irq, irq_temp,
				&g_vmrm_intf_dev->io_res.irq, list) {
				gh_irq_label_local = cam_irq_label_convert_gh_label(irq->label);
				if ((gh_irq_label_local >= GH_IRQ_LABEL_CAM_BASE) &&
					(gh_irq_label_local < GH_IRQ_LABEL_MAX)) {
					rc = cam_vmrm_gh_irq_wait_for_lend_v2(gh_irq_label_local,
						vmname, cam_irq_notification_svm_handler, NULL);
					if (rc) {
						CAM_ERR(CAM_VMRM,
							"SVM RM register irq %d wait lend failed rc: %d",
							irq->label, rc);
						goto end;
					}
					CAM_DBG(CAM_VMRM,
						"SVM RM register irq number %d irq %d wait lend succeed",
						irq->irq_num, irq->label);
				} else {
					CAM_ERR(CAM_VMRM, "invalid cam irq label %d", irq->label);
					rc = -EINVAL;
					goto end;
				}
			}
		}
	}

end:
	return rc;
}

void cam_vmrm_print_soc_resources(struct cam_hw_instance *hw)
{
	int i;

	CAM_DBG(CAM_VMRM,
		"hw id %d, hw name %s vm owner %d is using %d response result %d irq count %d mem count %d gpio count %d resource count %d valid mask %d ready mask %d",
		hw->hw_id, hw->hw_name, hw->vm_owner, hw->is_using, hw->response_result,
		hw->resources.irq_count, hw->resources.num_mem_block, hw->resources.gpio_count,
		hw->resources.resource_cnt, hw->resources.valid_mask, hw->resources.ready_mask);

	for (i = 0; i < hw->resources.irq_count; i++)
		CAM_DBG(CAM_VMRM, "index %d, irq name %s irq num %d irq label %d", i,
			hw->resources.irq_name[i], hw->resources.irq_num[i],
			hw->resources.irq_label[i]);
	for (i = 0; i < hw->resources.num_mem_block; i++)
		CAM_DBG(CAM_VMRM,
			"index %d, mem name %s mem base 0x%x mem size 0x%x mem label mem tag", i,
			hw->resources.mem_block_name[i], hw->resources.mem_block_addr[i],
			hw->resources.mem_block_size[i], hw->resources.mem_label,
			hw->resources.mem_tag);
	for (i = 0; i < hw->resources.gpio_count; i++)
		CAM_DBG(CAM_VMRM,
			"index %d, gpio num %d gpio base 0x%x gpio size 0x%x mem label mem tag", i,
			hw->resources.gpio_num[i], hw->resources.gpio_mem_addr[i],
			hw->resources.gpio_mem_size[i], hw->resources.mem_label,
			hw->resources.mem_tag);
}

void cam_vmrm_print_hw_instances(struct cam_vmrm_intf_dev *vmrm_intf_dev)
{
	struct cam_hw_instance *hw_pos, *hw_temp;

	if (!list_empty(&vmrm_intf_dev->hw_instance)) {
		list_for_each_entry_safe(hw_pos,
			hw_temp, &vmrm_intf_dev->hw_instance, list) {
			cam_vmrm_print_soc_resources(hw_pos);
		}
	}
}

void cam_vmrm_print_driver_nodes(struct cam_vmrm_intf_dev *vmrm_intf_dev)
{
	struct cam_driver_node *driver_pos, *driver_temp;

	if (!list_empty(&vmrm_intf_dev->driver_node)) {
		list_for_each_entry_safe(driver_pos,
			driver_temp, &vmrm_intf_dev->driver_node, list) {
			CAM_DBG(CAM_VMRM, "driver id %d, driver name %s response result %d",
				driver_pos->driver_id, driver_pos->driver_name,
				driver_pos->response_result);
		}
	}
}

void cam_vmrm_print_io_res(struct cam_vmrm_intf_dev *vmrm_intf_dev)
{
	int i;
	struct cam_io_irq_entry *irq, *irq_temp;
	struct cam_io_mem_entry *mem, *mem_temp;

	if (!list_empty(&vmrm_intf_dev->io_res.irq)) {
		list_for_each_entry_safe(irq, irq_temp,
			&vmrm_intf_dev->io_res.irq, list) {
			i = 0;
			CAM_DBG(CAM_VMRM, "index %d irq label %d, num %d vm_owner %d",
				i, irq->label, irq->irq_num, irq->vm_owner);
			i++;
		}
	}

	if (!list_empty(&vmrm_intf_dev->io_res.mem)) {
		list_for_each_entry_safe(mem, mem_temp,
			&vmrm_intf_dev->io_res.mem, list) {
			i = 0;
			CAM_DBG(CAM_VMRM,
				"index %d mem base 0x%x, size 0x%x vm_owner %d lend_in_progress %d",
				i, mem->base, mem->size, mem->vm_owner, mem->lend_in_progress);
			i++;
		}
	}
}

void cam_vmrm_print_state(void)
{
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	CAM_DBG(CAM_VMRM,
		"VM resource manager vmid %d is_initialized %d gh_rr_enable %d proxy_voting_enable 0x%x",
		vmrm_intf_dev->cam_vmid, vmrm_intf_dev->is_initialized,
		vmrm_intf_dev->gh_rr_enable, vmrm_intf_dev->proxy_voting_enable);

	cam_vmrm_print_hw_instances(vmrm_intf_dev);
	cam_vmrm_print_driver_nodes(vmrm_intf_dev);
	cam_vmrm_print_io_res(vmrm_intf_dev);
}

static int cam_vmrm_set_lend_all_resources_test(void *data, u64 val)
{
	int rc = 0;
	struct cam_hw_instance *hw_pos, *hw_temp;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	CAM_INFO(CAM_VMRM, "lend all resources test starting");

	if (cam_vmrm_intf_get_vmid() != CAM_PVM) {
		CAM_WARN(CAM_VMRM, "lend all resources test only for pvm");
		return 0;
	}

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	if (!list_empty(&vmrm_intf_dev->hw_instance)) {
		list_for_each_entry_safe(hw_pos,
			hw_temp, &vmrm_intf_dev->hw_instance, list) {
			rc = cam_vmrm_lend_resources(hw_pos->hw_id, CAM_SVM1);
			if (rc) {
				CAM_ERR(CAM_VMRM,
					"lend resources test for hw id failed: 0x%x, rc: %d",
					hw_pos->hw_id, rc);
				return 0;
			}
		}
	}

	CAM_INFO(CAM_VMRM, "lend all resources test succeed");

	return 0;
}

static int cam_vmrm_get_lend_all_resources_test(void *data, u64 *val)
{
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(cam_vmrm_lend_all_resources_test,
	cam_vmrm_get_lend_all_resources_test,
	cam_vmrm_set_lend_all_resources_test, "%16llu");

static ssize_t cam_vmrm_get_acquire_resources_test(struct file *file,
	char __user *ubuf, size_t size, loff_t *loff_t)
{
	return 0;
}

static ssize_t cam_vmrm_set_acquire_resources_test(struct file *file,
	const char __user *ubuf, size_t size, loff_t *loff_t)
{
	int rc = 0;
	char input_buf[CAM_BUF_SIZE_MAX] = {'\0'};
	uint32_t hw_id;

	if (size >= CAM_BUF_SIZE_MAX) {
		CAM_ERR(CAM_VMRM, "%d size more than max size %d", size, CAM_BUF_SIZE_MAX);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, ubuf, sizeof(input_buf))) {
		CAM_ERR(CAM_VMRM, "copy_from_user failed");
		return -EFAULT;
	}

	if (kstrtou32(input_buf, CAM_HEXADECIMAL, &hw_id)) {
		CAM_ERR(CAM_VMRM, "kstrtou32 failed");
		return -EINVAL;
	}

	CAM_INFO(CAM_VMRM, "acquire resources test for hw id 0x%x", hw_id);

	rc = cam_vmrm_soc_acquire_resources(hw_id);
	if (rc) {
		CAM_ERR(CAM_VMRM, "acquire resources test for hw id failed: 0x%x, rc: %d",
			hw_id, rc);
		return rc;
	}

	CAM_INFO(CAM_VMRM, "hw id 0x%x acquire resources test succeed", hw_id);

	return size;
}

static const struct file_operations cam_vmrm_acquire_resources_test = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = cam_vmrm_get_acquire_resources_test,
	.write = cam_vmrm_set_acquire_resources_test,
};

static ssize_t cam_vmrm_get_release_resources_test(struct file *file,
	char __user *ubuf, size_t size, loff_t *loff_t)
{
	return 0;
}

static ssize_t cam_vmrm_set_release_resources_test(struct file *file,
	const char __user *ubuf, size_t size, loff_t *loff_t)
{
	int rc = 0;
	char input_buf[CAM_BUF_SIZE_MAX] = {'\0'};
	uint32_t hw_id;

	if (size >= CAM_BUF_SIZE_MAX) {
		CAM_ERR(CAM_VMRM, "%d size more than max size %d", size, CAM_BUF_SIZE_MAX);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, ubuf, sizeof(input_buf))) {
		CAM_ERR(CAM_VMRM, "copy_from_user failed");
		return -EFAULT;
	}

	if (kstrtou32(input_buf, CAM_HEXADECIMAL, &hw_id)) {
		CAM_ERR(CAM_VMRM, "kstrtou32 failed");
		return -EINVAL;
	}

	CAM_INFO(CAM_VMRM, "release resources test for hw id 0x%x", hw_id);

	rc = cam_vmrm_soc_release_resources(hw_id);
	if (rc) {
		CAM_ERR(CAM_VMRM, "release resources test for hw id failed: 0x%x, rc: %d",
			hw_id, rc);
		return rc;
	}

	CAM_INFO(CAM_VMRM, "hw id 0x%x release resources test succeed", hw_id);

	return size;
}

static const struct file_operations cam_vmrm_release_resources_test = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = cam_vmrm_get_release_resources_test,
	.write = cam_vmrm_set_release_resources_test,
};

static ssize_t cam_vmrm_get_gh_lend_resources_test(
	struct file *file, char __user *ubuf,
	size_t size, loff_t *loff_t)
{
	return 0;
}

static ssize_t cam_vmrm_set_gh_lend_resources_test(struct file *file,
	const char __user *ubuf, size_t size, loff_t *loff_t)
{
	int rc = 0;
	char input_buf[CAM_BUF_SIZE_MAX] = {'\0'};
	uint32_t hw_id;

	if (size >= CAM_BUF_SIZE_MAX) {
		CAM_ERR(CAM_VMRM, "%d size more than max size %d", size, CAM_BUF_SIZE_MAX);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, ubuf, sizeof(input_buf))) {
		CAM_ERR(CAM_VMRM, "copy_from_user failed");
		return -EFAULT;
	}

	if (kstrtou32(input_buf, CAM_HEXADECIMAL, &hw_id)) {
		CAM_ERR(CAM_VMRM, "kstrtou32 failed");
		return -EINVAL;
	}

	CAM_INFO(CAM_VMRM, "lend resources test for hw id 0x%x", hw_id);

	if (cam_vmrm_intf_get_vmid() != CAM_PVM) {
		CAM_WARN(CAM_VMRM, "gh lend resources test only for pvm");
		return size;
	}

	rc = cam_vmrm_lend_resources(hw_id, CAM_SVM1);
	if (rc) {
		CAM_ERR(CAM_VMRM, "lend resources test for hw id failed: 0x%x, rc: %d",
			hw_id, rc);
		return rc;
	}

	CAM_INFO(CAM_VMRM, "hw id 0x%x lend resources test succeed", hw_id);

	return size;
}

static const struct file_operations cam_vmrm_gh_lend_resources_test = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = cam_vmrm_get_gh_lend_resources_test,
	.write = cam_vmrm_set_gh_lend_resources_test,
};

static ssize_t cam_vmrm_get_gh_release_resources_test(struct file *file,
	char __user *ubuf, size_t size, loff_t *loff_t)
{
	return 0;
}

static ssize_t cam_vmrm_set_gh_release_resources_test(struct file *file,
	const char __user *ubuf, size_t size, loff_t *loff_t)
{
	int rc = 0;
	char input_buf[CAM_BUF_SIZE_MAX] = {'\0'};
	uint32_t hw_id;

	if (size >= CAM_BUF_SIZE_MAX) {
		CAM_ERR(CAM_VMRM, "%d size more than max size %d", size, CAM_BUF_SIZE_MAX);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, ubuf, sizeof(input_buf))) {
		CAM_ERR(CAM_VMRM, "copy_from_user failed");
		return -EFAULT;
	}

	if (kstrtou32(input_buf, CAM_HEXADECIMAL, &hw_id)) {
		CAM_ERR(CAM_VMRM, "kstrtou32 failed");
		return -EINVAL;
	}

	CAM_INFO(CAM_VMRM, "release resources test for hw id 0x%x", hw_id);

	if (cam_vmrm_intf_get_vmid() == CAM_PVM) {
		CAM_WARN(CAM_VMRM, "gh release resources test only for svm");
		return size;
	}

	rc = cam_vmrm_release_resources(hw_id);
	if (rc) {
		CAM_ERR(CAM_VMRM, "release resources test for hw id failed: 0x%x, rc: %d",
			hw_id, rc);
		return rc;
	}

	CAM_INFO(CAM_VMRM, "hw id 0x%x release resources test succeed", hw_id);

	return size;
}

static const struct file_operations cam_vmrm_gh_release_resources_test = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = cam_vmrm_get_gh_release_resources_test,
	.write = cam_vmrm_set_gh_release_resources_test,
};

static ssize_t cam_vmrm_get_soc_resources_enable_test(
	struct file *file, char __user *ubuf,
	size_t size, loff_t *loff_t)
{
	return 0;
}

static ssize_t cam_vmrm_set_soc_resources_enable_test(struct file *file,
	const char __user *ubuf, size_t size, loff_t *loff_t)
{
	int rc = 0;
	char input_buf[CAM_BUF_SIZE_MAX] = {'\0'};
	uint32_t hw_id;

	if (size >= CAM_BUF_SIZE_MAX) {
		CAM_ERR(CAM_VMRM, "%d size more than max size %d", size, CAM_BUF_SIZE_MAX);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, ubuf, sizeof(input_buf))) {
		CAM_ERR(CAM_VMRM, "copy_from_user failed");
		return -EFAULT;
	}

	if (kstrtou32(input_buf, CAM_HEXADECIMAL, &hw_id)) {
		CAM_ERR(CAM_VMRM, "kstrtou32 failed");
		return -EINVAL;
	}

	CAM_INFO(CAM_VMRM, "soc resources enable request test for hw id 0x%x", hw_id);

	if (cam_vmrm_intf_get_vmid() == CAM_PVM) {
		CAM_WARN(CAM_VMRM, "soc resources enable test only for svm");
		return size;
	}

	rc = cam_vmrm_soc_enable_disable_resources(hw_id, true);
	if (rc) {
		CAM_ERR(CAM_VMRM, "soc enable for hw id failed: 0x%x, rc: %d", hw_id, rc);
		return rc;
	}

	CAM_INFO(CAM_VMRM, "hw id 0x%x soc resources enable request test succeed", hw_id);

	return size;
}

static const struct file_operations cam_vmrm_soc_resources_enable_test = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = cam_vmrm_get_soc_resources_enable_test,
	.write = cam_vmrm_set_soc_resources_enable_test,
};

static ssize_t cam_vmrm_get_soc_resources_disable_test(struct file *file,
	char __user *ubuf, size_t size, loff_t *loff_t)
{
	return 0;
}

static ssize_t cam_vmrm_set_soc_resources_disable_test(struct file *file,
	const char __user *ubuf, size_t size, loff_t *loff_t)
{
	int rc = 0;
	char input_buf[CAM_BUF_SIZE_MAX] = {'\0'};
	uint32_t hw_id;

	if (size >= CAM_BUF_SIZE_MAX) {
		CAM_ERR(CAM_VMRM, "%d size more than max size %d", size, CAM_BUF_SIZE_MAX);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, ubuf, sizeof(input_buf))) {
		CAM_ERR(CAM_VMRM, "copy_from_user failed");
		return -EFAULT;
	}

	if (kstrtou32(input_buf, CAM_HEXADECIMAL, &hw_id)) {
		CAM_ERR(CAM_VMRM, "kstrtou32 failed");
		return -EINVAL;
	}

	CAM_INFO(CAM_VMRM, "soc resources disable request test for hw id 0x%x", hw_id);

	if (cam_vmrm_intf_get_vmid() == CAM_PVM) {
		CAM_WARN(CAM_VMRM, "soc resources disable test only for svm");
		return size;
	}

	rc = cam_vmrm_soc_enable_disable_resources(hw_id, false);
	if (rc) {
		CAM_ERR(CAM_VMRM, "soc disable for hw id failed: 0x%x, rc: %d", hw_id, rc);
		return rc;
	}

	CAM_INFO(CAM_VMRM, "hw id 0x%x soc resources disable request test succeed", hw_id);

	return size;
}

static const struct file_operations cam_vmrm_soc_resources_disable_test = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = cam_vmrm_get_soc_resources_disable_test,
	.write = cam_vmrm_set_soc_resources_disable_test,
};

static ssize_t cam_vmrm_get_driver_node_msg_test(
	struct file *file, char __user *ubuf,
	size_t size, loff_t *loff_t)
{
	return 0;
}

static ssize_t cam_vmrm_set_driver_node_msg_test(struct file *file,
	const char __user *ubuf, size_t size, loff_t *loff_t)
{
	int rc = 0;
	char input_buf[CAM_BUF_SIZE_MAX] = {'\0'};
	uint32_t driver_id;

	if (size >= CAM_BUF_SIZE_MAX) {
		CAM_ERR(CAM_VMRM, "%d size more than max size %d", size, CAM_BUF_SIZE_MAX);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, ubuf, sizeof(input_buf))) {
		CAM_ERR(CAM_VMRM, "copy_from_user failed");
		return -EFAULT;
	}

	if (kstrtou32(input_buf, CAM_HEXADECIMAL, &driver_id)) {
		CAM_ERR(CAM_VMRM, "kstrtou32 failed");
		return -EINVAL;
	}

	CAM_INFO(CAM_VMRM, "test for driver id 0x%x", driver_id);

	if (cam_vmrm_intf_get_vmid() == CAM_PVM) {
		CAM_WARN(CAM_VMRM, "driver node msg test only for svm");
		return size;
	}

	rc = cam_vmrm_send_driver_msg_wrapper(CAM_PVM, driver_id, CAM_MSG_TYPE_MAX, false, true,
		NULL, 1, 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "send msg failed for driver node: 0x%x, rc: %d", driver_id, rc);
		return rc;
	}

	return size;
}

static const struct file_operations cam_vmrm_driver_node_msg_test = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = cam_vmrm_get_driver_node_msg_test,
	.write = cam_vmrm_set_driver_node_msg_test,
};

static ssize_t cam_vmrm_get_clk_rate_set_test(struct file *file,
	char __user *ubuf, size_t size, loff_t *loff_t)
{
	return 0;
}

static ssize_t cam_vmrm_set_clk_rate_set_test(struct file *file,
	const char __user *ubuf, size_t size, loff_t *loff_t)
{
	int rc = 0;
	char input_buf[CAM_BUF_SIZE_MAX] = {'\0'};
	uint32_t hw_id;
	uint32_t clk_rate = 0;
	char *msg = NULL;
	char *token = NULL;

	if (size >= CAM_BUF_SIZE_MAX) {
		CAM_ERR(CAM_VMRM, "%d size more than max size %d", size, CAM_BUF_SIZE_MAX);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, ubuf, size)) {
		CAM_ERR(CAM_VMRM, "copy_from_user failed");
		return -EFAULT;
	}

	msg = input_buf;
	token = strsep(&msg, ":");
	if (token != NULL) {
		if (kstrtou32(token, CAM_HEXADECIMAL, &hw_id)) {
			CAM_ERR(CAM_VMRM, "kstrtou32 failed");
			return -EINVAL;
		}
	}

	if (msg != NULL) {
		if (kstrtou32(msg, 0, &clk_rate)) {
			CAM_ERR(CAM_VMRM, "kstrtou32 failed");
			return -EINVAL;
		}
	}

	clk_rate = clk_rate * CAM_MHZ;

	CAM_INFO(CAM_VMRM, "clk rate set test hw id %x clk_rate %d", hw_id, clk_rate);

	if (cam_vmrm_intf_get_vmid() == CAM_PVM) {
		CAM_WARN(CAM_VMRM, "clk rate set test only for svm");
		return size;
	}

	rc = cam_vmrm_set_src_clk_rate(hw_id, -1, clk_rate, clk_rate);
	if (rc) {
		CAM_ERR(CAM_VMRM, "set src clk rate for hw id failed: 0x%x, rc: %d", hw_id, rc);
		return rc;
	}

	CAM_INFO(CAM_VMRM, "hw id 0x%x clk rate set test succeed", hw_id);

	return size;
}

static const struct file_operations cam_vmrm_clk_rate_set_test = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = cam_vmrm_get_clk_rate_set_test,
	.write = cam_vmrm_set_clk_rate_set_test,
};

static int cam_vmrm_debugfs_init(struct cam_vmrm_intf_dev *vmrm_dev)
{
	struct dentry *dbgfileptr = NULL;
	int rc = 0;

	if (!cam_debugfs_available())
		return 0;

	rc = cam_debugfs_create_subdir("vmrm", &dbgfileptr);
	if (rc) {
		CAM_ERR(CAM_VMRM, "DebugFS could not create directory!");
		return rc;
	}

	vmrm_dev->dentry = dbgfileptr;

	debugfs_create_file("proxy_voting_enable", 0644, vmrm_dev->dentry, NULL,
		&cam_vmrm_proxy_voting_enable);

	debugfs_create_file("cam_vmrm_lend_all_resources_test", 0644, vmrm_dev->dentry, NULL,
		&cam_vmrm_lend_all_resources_test);

	debugfs_create_file("cam_vmrm_acquire_resources_test", 0644,
		vmrm_dev->dentry, NULL, &cam_vmrm_acquire_resources_test);

	debugfs_create_file("cam_vmrm_release_resources_test", 0644,
		vmrm_dev->dentry, NULL, &cam_vmrm_release_resources_test);

	debugfs_create_file("cam_vmrm_gh_lend_resources_test", 0644,
		vmrm_dev->dentry, NULL, &cam_vmrm_gh_lend_resources_test);

	debugfs_create_file("cam_vmrm_gh_release_resources_test", 0644,
		vmrm_dev->dentry, NULL, &cam_vmrm_gh_release_resources_test);

	debugfs_create_file("cam_vmrm_soc_resources_enable_test", 0644,
		vmrm_dev->dentry, NULL, &cam_vmrm_soc_resources_enable_test);

	debugfs_create_file("cam_vmrm_soc_resources_disable_test", 0644,
		vmrm_dev->dentry, NULL, &cam_vmrm_soc_resources_disable_test);

	debugfs_create_file("cam_vmrm_driver_node_msg_test", 0644,
		vmrm_dev->dentry, NULL, &cam_vmrm_driver_node_msg_test);

	debugfs_create_file("cam_vmrm_clk_rate_set_test", 0644,
		vmrm_dev->dentry, NULL, &cam_vmrm_clk_rate_set_test);

	return 0;
}

static int cam_vmrm_intf_bind(struct device *dev,
	struct device *parent_dev, void *data)
{
	int                       rc = 0;
	struct platform_device   *pdev;
	bool                      is_server_vm = false;
	struct timespec64         ts_start, ts_end;
	long                      microsec = 0;

	CAM_GET_TIMESTAMP(ts_start);
	pdev = to_platform_device(dev);

	g_vmrm_intf_dev = CAM_MEM_ZALLOC(sizeof(struct cam_vmrm_intf_dev), GFP_KERNEL);
	if (!g_vmrm_intf_dev) {
		CAM_ERR(CAM_VMRM, "VM resource manager device allocate failed");
		rc = -ENOMEM;
		return rc;
	}

	rc = cam_vmrm_dt_parse(pdev, g_vmrm_intf_dev);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Device tree parsing failed rc: %d", rc);
		return rc;
	}

	mutex_init(&g_vmrm_intf_dev->lock);
	INIT_LIST_HEAD(&g_vmrm_intf_dev->hw_instance);
	INIT_LIST_HEAD(&g_vmrm_intf_dev->driver_node);
	INIT_LIST_HEAD(&g_vmrm_intf_dev->io_res.irq);
	INIT_LIST_HEAD(&g_vmrm_intf_dev->io_res.mem);

	g_vmrm_intf_dev->ops_table = CAM_MEM_ZALLOC(sizeof(struct cam_inter_vm_comms_ops),
							GFP_KERNEL);
	if (!g_vmrm_intf_dev->ops_table) {
		CAM_ERR(CAM_VMRM, "allocate qrtr function table mem failed");
		rc = -ENOMEM;
		goto free_vmrm_intf_dev;
	}
	rc = cam_get_inter_vm_comms_function_table(g_vmrm_intf_dev->ops_table);
	if (rc) {
		CAM_ERR(CAM_VMRM, "get qrtr function table failed %d", rc);
		goto free_ops_table;
	}

	if (CAM_IS_PRIMARY_VM())
		is_server_vm = true;
	rc = g_vmrm_intf_dev->ops_table->init(&g_vmrm_intf_dev->vm_handle,
		CAM_VMRM_RECV_MSG_BUF_SIZE, cam_vmrm_msg_handle, is_server_vm);
	if (rc) {
		CAM_ERR(CAM_VMRM, "qrtr initialize failed %d", rc);
		goto free_ops_table;
	}

	cam_vmrm_debugfs_init(g_vmrm_intf_dev);
	g_vmrm_intf_dev->is_initialized = true;

	CAM_DBG(CAM_VMRM, "VMRM %d name %s driver bind succeed", g_vmrm_intf_dev->cam_vmid,
		CAM_GET_VM_NAME());
	CAM_GET_TIMESTAMP(ts_end);
	CAM_GET_TIMESTAMP_DIFF_IN_MICRO(ts_start, ts_end, microsec);
	cam_record_bind_latency(pdev->name, microsec);

	return rc;
free_ops_table:
	CAM_MEM_FREE(g_vmrm_intf_dev->ops_table);
free_vmrm_intf_dev:
	CAM_MEM_FREE(g_vmrm_intf_dev);
	return rc;
}

static void cam_vmrm_intf_unbind(struct device *dev,
	struct device *parent_dev, void *data)
{
	int rc = 0;

	CAM_DBG(CAM_VMRM, "Unbinding cam_vmrm_intf driver vmid %d", g_vmrm_intf_dev->cam_vmid);

	if (!list_empty(&g_vmrm_intf_dev->io_res.mem))
		cam_clean_io_mem(&g_vmrm_intf_dev->io_res.mem);
	if (!list_empty(&g_vmrm_intf_dev->io_res.irq))
		cam_clean_io_irq(&g_vmrm_intf_dev->io_res.irq);
	if (!list_empty(&g_vmrm_intf_dev->driver_node))
		cam_clean_driver_node(&g_vmrm_intf_dev->driver_node);
	if (!list_empty(&g_vmrm_intf_dev->hw_instance))
		cam_clean_hw_instance(&g_vmrm_intf_dev->hw_instance);

	rc = g_vmrm_intf_dev->ops_table->deinit(g_vmrm_intf_dev->vm_handle);
	if (rc)
		CAM_ERR(CAM_VMRM, "deinit internal vm communication failed");

	mutex_destroy(&g_vmrm_intf_dev->lock);
	CAM_MEM_FREE(g_vmrm_intf_dev->ops_table);
	CAM_MEM_FREE(g_vmrm_intf_dev);
}

static const struct component_ops cam_vmrm_intf_ops = {
	.bind = cam_vmrm_intf_bind,
	.unbind = cam_vmrm_intf_unbind,
};

static int cam_vmrm_intf_probe(struct platform_device *pdev)
{
	int rc;

	CAM_DBG(CAM_VMRM, "Probe for vmrm_intf driver");

	rc = component_add(&pdev->dev, &cam_vmrm_intf_ops);
	if (rc) {
		CAM_ERR(CAM_VMRM, "Adding vmrm_intf Driver to the component list failed. rc: %d",
			rc);
		return rc;
	}

	return rc;
}

static int cam_vmrm_intf_remove(struct platform_device *pdev)
{
	CAM_DBG(CAM_VMRM, "Removing the vmrm_intf driver");
	component_del(&pdev->dev, &cam_vmrm_intf_ops);
	return 0;
}

struct platform_driver cam_vmrm_intf_driver = {
	.probe  = cam_vmrm_intf_probe,
	.remove = cam_vmrm_intf_remove,
	.driver = {
		.name = "msm_cam_vmrm_intf",
		.owner = THIS_MODULE,
		.of_match_table = msm_cam_vmrm_intf_dt_match,
		.suppress_bind_attrs = true,
	},
};

int cam_vmrm_module_init(void)
{
	return platform_driver_register(&cam_vmrm_intf_driver);
}

void cam_vmrm_module_exit(void)
{
	platform_driver_unregister(&cam_vmrm_intf_driver);
}

MODULE_DESCRIPTION("MSM Camera VM Resource Manager Driver");
MODULE_LICENSE("GPL");
