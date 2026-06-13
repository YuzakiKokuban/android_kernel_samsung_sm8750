// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifdef CONFIG_SPECTRA_VMRM
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/pinctrl/qcom-pinctrl.h>
#include <linux/gpio.h>
#include "camera_main.h"
#include "cam_debug_util.h"
#include "cam_common_util.h"
#endif
#include "cam_vmrm.h"
#include "cam_vmrm_interface.h"
#include "cam_cpas_api.h"
#include "cam_req_mgr_dev.h"
#include "cam_mem_mgr_api.h"

#ifdef CONFIG_SPECTRA_VMRM
extern struct cam_vmrm_intf_dev *g_vmrm_intf_dev;

bool cam_vmrm_is_supported(void)
{
	return true;
}

uint32_t cam_vmrm_intf_get_vmid(void)
{
	return g_vmrm_intf_dev->cam_vmid;
}

bool cam_vmrm_proxy_clk_rgl_voting_enable(void)
{
	struct cam_vmrm_intf_dev *vmrm_intf_dev;
	bool enable = false;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	if (!vmrm_intf_dev) {
		CAM_ERR(CAM_VMRM, "vmrm dev is not ready");
		return false;
	}

	if (!vmrm_intf_dev->is_initialized) {
		CAM_ERR(CAM_VMRM, "vmrm is not initialized");
		return false;
	}

	if (vmrm_intf_dev->proxy_voting_enable & CAM_PROXY_CLK_RGL_VOTING)
		enable = true;

	return enable;
}

bool cam_vmrm_proxy_icc_voting_enable(void)
{
	struct cam_vmrm_intf_dev *vmrm_intf_dev;
	bool enable = false;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	if (!vmrm_intf_dev) {
		CAM_ERR(CAM_VMRM, "vmrm dev is not ready");
		return false;
	}

	if (!vmrm_intf_dev->is_initialized) {
		CAM_ERR(CAM_VMRM, "vmrm is not initialized");
		return false;
	}

	if (vmrm_intf_dev->proxy_voting_enable & CAM_PROXY_ICC_VOTING)
		enable = true;

	return enable;
}

bool cam_vmrm_no_register_read_on_bind(void)
{
	struct cam_vmrm_intf_dev *vmrm_intf_dev;
	struct timespec64         ts_start, ts_end;
	long                      microsec = 0;

	CAM_GET_TIMESTAMP(ts_start);
	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	if (!vmrm_intf_dev) {
		CAM_ERR(CAM_VMRM, "vmrm dev is not ready");
		return false;
	}

	if (!vmrm_intf_dev->is_initialized) {
		CAM_ERR(CAM_VMRM, "vmrm is not initialized");
		return false;
	}

	CAM_GET_TIMESTAMP(ts_end);
	CAM_GET_TIMESTAMP_DIFF_IN_MICRO(ts_start, ts_end, microsec);
	cam_record_bind_latency(pdev->name, microsec);
	return vmrm_intf_dev->no_register_read_on_bind;
}

int cam_vmvm_populate_hw_instance_info(struct cam_hw_soc_info *soc_info,
	msg_cb_func hw_msg_callback, void *hw_msg_callback_data)
{
	int i;
	struct cam_hw_instance *hw_instance_temp;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;
	struct cam_soc_gpio_data *gpio_data;

	if (!soc_info) {
		CAM_ERR(CAM_VMRM, "Invalid hw info");
		return -EINVAL;
	}

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	if (!vmrm_intf_dev) {
		CAM_ERR(CAM_VMRM, "vmrm dev is not ready");
		return -EINVAL;
	}

	if (!vmrm_intf_dev->is_initialized) {
		CAM_ERR(CAM_VMRM, "vmrm is not initialized");
		return -EINVAL;
	}

	hw_instance_temp = CAM_MEM_ZALLOC(sizeof(*hw_instance_temp), GFP_KERNEL);
	if (!hw_instance_temp) {
		CAM_ERR(CAM_VMRM, " hw instance allocate memory failed");
		return -EINVAL;
	}
	hw_instance_temp->hw_id = soc_info->hw_id;
	scnprintf(hw_instance_temp->hw_name,
		sizeof(hw_instance_temp->hw_name), "%s", soc_info->dev_name);

	for (i = 0; i < soc_info->irq_count; i++) {
		scnprintf(hw_instance_temp->resources.irq_name[i],
			sizeof(hw_instance_temp->resources.irq_name[i]), "%s",
				soc_info->irq_name[i]);
		hw_instance_temp->resources.irq_num[i] = soc_info->irq_num[i];
		hw_instance_temp->resources.irq_label[i] =
			soc_info->vmrm_resource_ids[i + CAM_VMRM_RESOURCE_IRQ_OFFSET];
		hw_instance_temp->resources.valid_mask |=
			BIT(i + CAM_VMRM_RESOURCE_IRQ_BIT_MAP_OFFSET);
		CAM_DBG(CAM_VMRM,
			"hw name %s hw id %x irq name %s irq count %d irq num %d irq label %d valid mask %d",
			hw_instance_temp->hw_name, hw_instance_temp->hw_id,
			hw_instance_temp->resources.irq_name[i], i,
			hw_instance_temp->resources.irq_num[i],
			hw_instance_temp->resources.irq_label[i],
			hw_instance_temp->resources.valid_mask);
	}
	hw_instance_temp->resources.irq_count = soc_info->irq_count;
	hw_instance_temp->resources.resource_cnt = hw_instance_temp->resources.irq_count;

	for (i = 0; i < soc_info->num_mem_block; i++) {
		scnprintf(hw_instance_temp->resources.mem_block_name[i],
			sizeof(hw_instance_temp->resources.mem_block_name[i]),
			"%s", soc_info->mem_block_name[i]);
		hw_instance_temp->resources.mem_block_addr[i] = soc_info->mem_block[i]->start;
		hw_instance_temp->resources.mem_block_size[i] =
			resource_size(soc_info->mem_block[i]);

		CAM_DBG(CAM_VMRM,
			"hw name %s hw id %x mem name %s mem count %d mem address %lx mem size %x",
			hw_instance_temp->hw_name, hw_instance_temp->hw_id,
			hw_instance_temp->resources.mem_block_name[i], i,
			hw_instance_temp->resources.mem_block_addr[i],
			hw_instance_temp->resources.mem_block_size[i]);
	}
	hw_instance_temp->resources.num_mem_block = soc_info->num_mem_block;
	hw_instance_temp->resources.resource_cnt += hw_instance_temp->resources.num_mem_block;

	gpio_data = soc_info->gpio_data;
	if (gpio_data) {
		for (i = 0; i < gpio_data->cam_gpio_common_tbl_size; i++) {
			hw_instance_temp->resources.gpio_num[i] =
				gpio_data->cam_gpio_common_tbl[i].gpio;

			CAM_DBG(CAM_VMRM, "hw name %s hw id %x gpio count %d gpio num %d",
				hw_instance_temp->hw_name, hw_instance_temp->hw_id, i,
				hw_instance_temp->resources.gpio_num[i]);
		}
		hw_instance_temp->resources.gpio_count = gpio_data->cam_gpio_common_tbl_size;
		hw_instance_temp->resources.resource_cnt += hw_instance_temp->resources.gpio_count;
	}

	hw_instance_temp->resources.mem_label = soc_info->vmrm_resource_ids[0];
	hw_instance_temp->resources.mem_tag = soc_info->vmrm_resource_ids[1];
	hw_instance_temp->hw_msg_callback = hw_msg_callback;

	if (hw_instance_temp->hw_msg_callback)
		hw_instance_temp->hw_msg_callback_data = hw_msg_callback_data;
	else
		hw_instance_temp->hw_msg_callback_data = soc_info;

	/*Mem resource is allways there*/
	hw_instance_temp->resources.valid_mask |= BIT(0);

	/* Default owner is PVM*/
	hw_instance_temp->vm_owner = CAM_PVM;
	hw_instance_temp->is_using = false;
	hw_instance_temp->ref_count = 0;

	INIT_LIST_HEAD(&hw_instance_temp->list);
	init_completion(&hw_instance_temp->wait_response);
	spin_lock_init(&hw_instance_temp->spin_lock);
	mutex_init(&hw_instance_temp->msg_comm_lock);
	mutex_lock(&vmrm_intf_dev->lock);
	list_add_tail(&hw_instance_temp->list, &vmrm_intf_dev->hw_instance);
	mutex_unlock(&vmrm_intf_dev->lock);

	CAM_DBG(CAM_VMRM,
		"hw name %s hw id %x mem count %d irq count %d gpio count %d mem label %d mem tag %d valid mask %d",
		hw_instance_temp->hw_name, hw_instance_temp->hw_id,
		hw_instance_temp->resources.num_mem_block, hw_instance_temp->resources.irq_count,
		hw_instance_temp->resources.gpio_count, hw_instance_temp->resources.mem_label,
		hw_instance_temp->resources.mem_tag, hw_instance_temp->resources.valid_mask);

	return 0;
}

int cam_vmrm_populate_driver_node_info(struct cam_driver_node *driver_node)
{
	struct cam_driver_node *driver_node_temp;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	if (!vmrm_intf_dev) {
		CAM_ERR(CAM_VMRM, "vmrm dev is not ready");
		return -EINVAL;
	}

	if (!vmrm_intf_dev->is_initialized) {
		CAM_ERR(CAM_VMRM, "vmrm is not initialized");
		return -EINVAL;
	}

	driver_node_temp = kmemdup(driver_node, sizeof(*driver_node_temp), GFP_ATOMIC);
	if (!driver_node_temp) {
		CAM_ERR(CAM_VMRM, "driver allocate memory failed");
		return -ENOMEM;
	}

	CAM_DBG(CAM_VMRM, "driver name %s, driver id 0x%x",
		driver_node_temp->driver_name, driver_node_temp->driver_id);

	INIT_LIST_HEAD(&driver_node_temp->list);
	init_completion(&driver_node_temp->wait_response);
	mutex_init(&driver_node_temp->msg_comm_lock);
	mutex_lock(&vmrm_intf_dev->lock);
	list_add_tail(&driver_node_temp->list, &vmrm_intf_dev->driver_node);
	mutex_unlock(&vmrm_intf_dev->lock);

	return 0;
}

int cam_vmrm_populate_io_resource_info(void)
{
	int rc = 0;
	struct cam_hw_instance *hw_pos, *hw_temp;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	if (!list_empty(&vmrm_intf_dev->hw_instance)) {
		list_for_each_entry_safe(hw_pos,
			hw_temp, &vmrm_intf_dev->hw_instance, list) {
			rc = cam_populate_irq_resource_info(hw_pos);
			if (rc) {
				CAM_ERR(CAM_VMRM, "Populate irq resource info failed %d",
					hw_pos->hw_id);
				goto free_resources;
			}
			rc = cam_populate_mem_resource_info(hw_pos);
			if (rc) {
				CAM_ERR(CAM_VMRM, "Populate mem resource info failed %d",
					hw_pos->hw_id);
				goto free_resources;
			}
			rc = cam_populate_gpio_resource_info(hw_pos);
			if (rc) {
				CAM_ERR(CAM_VMRM, "Populate gpio resource info failed %d",
					hw_pos->hw_id);
				goto free_resources;
			}
		}
	}

	CAM_DBG(CAM_VMRM, "populate io resource succeed");

	return rc;

free_resources:
	cam_free_io_resource_info();
	return rc;
}

int cam_vmrm_register_gh_callback(void)
{
	int rc = 0;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	mutex_lock(&vmrm_intf_dev->lock);
	if (vmrm_intf_dev->gh_rr_enable) {
		rc = cam_register_gh_res_callback();
		if (rc) {
			CAM_ERR(CAM_VMRM, "Register gh resource callback failed %d", rc);
			mutex_unlock(&vmrm_intf_dev->lock);
			return rc;
		}
	}

	rc = cam_register_gh_mem_callback();
	if (rc) {
		CAM_ERR(CAM_VMRM, "Register gh mem callback failed %d", rc);
		goto unregister_gh_mem_callback;
	}

	rc = cam_register_gh_irq_callback();
	if (rc) {
		CAM_ERR(CAM_VMRM, "Register gh irq callback failed %d", rc);
		goto unregister_gh_mem_callback;
	}
	mutex_unlock(&vmrm_intf_dev->lock);

	CAM_DBG(CAM_VMRM, "register gh callback succeed");

	return rc;

unregister_gh_mem_callback:
	cam_unregister_gh_mem_callback();
	cam_unregister_gh_res_callback();
	mutex_unlock(&vmrm_intf_dev->lock);
	return rc;
}

int cam_vmrm_unregister_gh_callback(void)
{
	int rc = 0;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();

	mutex_lock(&vmrm_intf_dev->lock);
	cam_unregister_gh_mem_callback();

	if (vmrm_intf_dev->gh_rr_enable) {
		rc = cam_unregister_gh_res_callback();
		if (rc)
			CAM_ERR(CAM_VMRM, "Unregister gh resource callback failed %d", rc);
	}
	mutex_unlock(&vmrm_intf_dev->lock);

	return rc;
}

int cam_vmrm_send_msg(uint32_t source_vmid, uint32_t des_vmid, uint32_t msg_dst_type,
	uint32_t msg_dst_id, uint32_t msg_type, bool response_msg, bool need_response,
	void *msg_data, uint32_t data_size, struct completion *complete, uint32_t timeout)
{
	int rc = 0;
	struct cam_vmrm_msg *vm_msg;
	uint32_t msg_size;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;
	unsigned long rem_jiffies = 0;

	msg_size = offsetof(struct cam_vmrm_msg, data[data_size]);
	vm_msg = CAM_MEM_ZALLOC((msg_size), GFP_KERNEL);
	if (!vm_msg) {
		CAM_ERR(CAM_VMRM, "msg mem allocate failed");
		return -ENOMEM;
	}

	if (!timeout)
		timeout = CAM_VMRM_INTER_VM_MSG_TIMEOUT;

	vm_msg->source_vmid = source_vmid;
	vm_msg->des_vmid = des_vmid;
	vm_msg->msg_dst_type = msg_dst_type;
	vm_msg->msg_dst_id = msg_dst_id;
	vm_msg->msg_type = msg_type;
	vm_msg->response_msg = response_msg;
	vm_msg->need_response = need_response;
	vm_msg->data_size = data_size;

	CAM_DBG(CAM_VMRM,
		"msg_size %d data_size %d source_vmid %d des_vmid %d dst_type %d dst_id %x msg_type %d response msg %d need response %d",
		msg_size, vm_msg->data_size, vm_msg->source_vmid, vm_msg->des_vmid,
		vm_msg->msg_dst_type, vm_msg->msg_dst_id, vm_msg->msg_type, vm_msg->response_msg,
		vm_msg->need_response);

	if (msg_size > CAM_VMRM_RECV_MSG_BUF_SIZE) {
		CAM_ERR(CAM_VMRM, "send msg size %d more than allocated receive buffer size %d",
			msg_size, CAM_VMRM_RECV_MSG_BUF_SIZE);
		return -EINVAL;
	}

	if (msg_data)
		memcpy(vm_msg->data, msg_data, data_size);

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	rc = vmrm_intf_dev->ops_table->send_message(vmrm_intf_dev->vm_handle,
		(void *)vm_msg, msg_size);
	if (rc) {
		if (rc == -EAGAIN) {
			rc = 0;
			CAM_WARN(CAM_VMRM, "connection is not ready skip send msg");
		} else {
			CAM_ERR(CAM_VMRM, "msg send failed");
		}
		CAM_MEM_FREE(vm_msg);
		return rc;
	}
	CAM_MEM_FREE(vm_msg);

	CAM_DBG(CAM_VMRM, "msg send succeed");

	if (need_response) {
		rem_jiffies = cam_common_wait_for_completion_timeout(complete,
			msecs_to_jiffies(timeout));
		if (rem_jiffies == 0) {
			CAM_ERR(CAM_VMRM, "hw 0x%x wait for response timeout", msg_dst_id);
			rc = -EINVAL;
		}
	}

	return rc;
}

int cam_vmrm_soc_acquire_resources(uint32_t hw_id)
{
	int rc = 0;
	struct cam_hw_instance *hw_pos, *cpas_hw_pos;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	mutex_lock(&vmrm_intf_dev->lock);
	/*
	 * Check if the hw resource is using by other vm firstly
	 * to handle concurrency acquire from different vm
	 */
	hw_pos = cam_check_hw_instance_available(hw_id);
	if (!hw_pos) {
		CAM_WARN(CAM_VMRM, "hw instance 0x%x is not available", hw_id);
		rc = -EBUSY;
		mutex_unlock(&vmrm_intf_dev->lock);
		goto end;
	}

	CAM_DBG(CAM_VMRM, "hw 0x%x ownership %d is_using %d ref count %d", hw_id, hw_pos->vm_owner,
		hw_pos->is_using, hw_pos->ref_count);

	if ((hw_pos->vm_owner == cam_vmrm_intf_get_vmid()) && hw_pos->ref_count) {
		hw_pos->ref_count++;
		CAM_DBG(CAM_VMRM, "hw 0x%x ownership has been acquired %d ref count %d",
			hw_id, cam_vmrm_intf_get_vmid(), hw_pos->ref_count);
		mutex_unlock(&vmrm_intf_dev->lock);
		goto end;
	}

	/*
	 * When the hw resource is idle, check the ownership then.
	 * If ownership is own just return and set using flag and
	 * send the msg to other vm to ensure other vm know the info.
	 * So when other vm try to acquire the same resource will
	 * get busy failing.
	 * For PVM, need to mask the using flag and send msg to other VMs.
	 * For SVM, if gh new api enable, svm call new gh request api.
	 * And if gh not enable, svm send acquire msg to pvm.
	 */
	if (CAM_IS_PRIMARY_VM()) {
		hw_pos->is_using = true;
		hw_pos->ref_count++;
		mutex_unlock(&vmrm_intf_dev->lock);
		rc = cam_vmrm_send_msg(cam_vmrm_intf_get_vmid(), CAM_SVM1, CAM_MSG_DST_TYPE_VMRM,
			hw_id, CAM_HW_RESOURCE_SET_ACQUIRE, false, false, NULL, 1, NULL, 0);
		if (rc) {
			CAM_ERR(CAM_VMRM, "vm rm send msg failed %d", rc);
			goto end;
		}
		CAM_DBG(CAM_VMRM, "hw 0x%x is pvm own and send msg to SVM ref count %d",
			hw_id, hw_pos->ref_count);
	} else {
		mutex_unlock(&vmrm_intf_dev->lock);
		if (vmrm_intf_dev->gh_rr_enable) {
			rc = cam_vmrm_resource_req(hw_pos, true);
			if (rc) {
				CAM_ERR(CAM_VMRM, "svm %d resource request call failed %d",
					cam_vmrm_intf_get_vmid(), rc);
				goto end;
			}
		} else {
			/*
			 * Due to cpas is basic resource, when acquire any hw, need to
			 * acquire cpas firstly.
			 */
			mutex_lock(&vmrm_intf_dev->lock);
			if (!vmrm_intf_dev->lend_cnt) {
				cpas_hw_pos = cam_hw_instance_lookup(CAM_HW_ID_CPAS, 0, 0, 0);
				if (!cpas_hw_pos) {
					CAM_ERR(CAM_VMRM, "Do not find cpas hw instance %x");
					rc = -EINVAL;
					mutex_unlock(&vmrm_intf_dev->lock);
					goto end;
				}
				mutex_lock(&cpas_hw_pos->msg_comm_lock);
				reinit_completion(&cpas_hw_pos->wait_response);
				rc = cam_vmrm_send_msg(cam_vmrm_intf_get_vmid(), CAM_PVM,
					CAM_MSG_DST_TYPE_VMRM, CAM_HW_ID_CPAS,
					CAM_HW_RESOURCE_ACQUIRE, false, true, NULL, 1,
					&cpas_hw_pos->wait_response, 0);
				if (rc) {
					CAM_ERR(CAM_VMRM, "vm rm send msg failed %d", rc);
					mutex_unlock(&cpas_hw_pos->msg_comm_lock);
					mutex_unlock(&vmrm_intf_dev->lock);
					goto end;
				}

				if (!rc && !cpas_hw_pos->response_result) {
					vmrm_intf_dev->lend_cnt++;
					CAM_DBG(CAM_VMRM,
						"acquire succeed hw id 0x%x, lend_cnt %d",
						cpas_hw_pos->hw_id, vmrm_intf_dev->lend_cnt);
				} else if (!rc && cpas_hw_pos->response_result) {
					CAM_ERR(CAM_VMRM, "failure happen in pvm lend hw id 0x%x",
						cpas_hw_pos->hw_id);
					rc = -EINVAL;
				} else {
					CAM_ERR(CAM_VMRM,
						"failure happen in tvm accept hw id 0x%x",
						cpas_hw_pos->hw_id);
				}
				mutex_unlock(&cpas_hw_pos->msg_comm_lock);
			}
			mutex_unlock(&vmrm_intf_dev->lock);

			mutex_lock(&hw_pos->msg_comm_lock);
			reinit_completion(&hw_pos->wait_response);
			rc = cam_vmrm_send_msg(cam_vmrm_intf_get_vmid(), CAM_PVM,
				CAM_MSG_DST_TYPE_VMRM, hw_id, CAM_HW_RESOURCE_ACQUIRE, false, true,
				NULL, 1, &hw_pos->wait_response, 0);
			if (rc) {
				CAM_ERR(CAM_VMRM, "vm rm send msg failed %d", rc);
				mutex_unlock(&hw_pos->msg_comm_lock);
				goto end;
			}
			/*
			 * Three results here, one is pvm lend failure, pvm send failure response
			 * message to tvm, when tvm receive the message and signal. And then check
			 * the response result to get the error happen in pvm.
			 * Another is pvm lend succeed, but tvm accept failure, pvm send succeed
			 * response message to tvm, when tvm receive the message and save the info,
			 * and do not signal. So completion will timeout.
			 * Last one is pvm lend and tvm accept succeed, pvm send succeed response
			 * message to tvm, when tvm receive the message and save the info. After
			 * tvm accept succeed to signal.
			 */
			if (!rc && !hw_pos->response_result) {
				mutex_lock(&vmrm_intf_dev->lock);
				vmrm_intf_dev->lend_cnt++;
				hw_pos->ref_count++;
				CAM_DBG(CAM_VMRM,
					"acquire succeed hw id 0x%x lend_cnt %d ref count %d",
					hw_pos->hw_id, vmrm_intf_dev->lend_cnt, hw_pos->ref_count);
				mutex_unlock(&vmrm_intf_dev->lock);
			} else if (!rc && hw_pos->response_result) {
				CAM_ERR(CAM_VMRM, "failure happen in pvm lend hw id 0x%x",
					hw_pos->hw_id);
				rc = -EINVAL;
			} else {
				CAM_ERR(CAM_VMRM, "failure happen in tvm accept hw id 0x%x",
					hw_pos->hw_id);
			}
			mutex_unlock(&hw_pos->msg_comm_lock);
		}
	}

end:
	return rc;

}

int cam_vmrm_soc_release_resources(uint32_t hw_id)
{
	int rc = 0;
	struct cam_hw_instance *hw_pos;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	mutex_lock(&vmrm_intf_dev->lock);
	hw_pos = cam_hw_instance_lookup(hw_id, 0, 0, 0);
	if (!hw_pos) {
		CAM_ERR(CAM_VMRM, "Do not find hw instance %x", hw_id);
		rc = -EINVAL;
		mutex_unlock(&vmrm_intf_dev->lock);
		goto end;
	}

	/*
	 * For svm need to release resource to pvm.
	 * For pvm reset the using flag and send msg to svm.
	 */
	if (!hw_pos->is_using) {
		CAM_DBG(CAM_VMRM, "hw %x using flag has been reset", hw_id);
		mutex_unlock(&vmrm_intf_dev->lock);
		goto end;
	}

	hw_pos->ref_count--;

	if (hw_pos->ref_count) {
		CAM_DBG(CAM_VMRM, "hw %x ref count %d", hw_id, hw_pos->ref_count);
		mutex_unlock(&vmrm_intf_dev->lock);
		goto end;
	}

	if (CAM_IS_PRIMARY_VM())
		hw_pos->is_using = false;
	mutex_unlock(&vmrm_intf_dev->lock);

	if (CAM_IS_PRIMARY_VM()) {
		rc = cam_vmrm_send_msg(cam_vmrm_intf_get_vmid(), CAM_SVM1, CAM_MSG_DST_TYPE_VMRM,
			hw_id, CAM_HW_RESOURCE_SET_RELEASE, false, false, NULL, 1, NULL, 0);
		if (rc) {
			CAM_ERR(CAM_VMRM, "vmrm send msg failed %d", rc);
			goto end;
		}
	} else {
		rc = cam_vmrm_release_resources(hw_id);
		if (rc) {
			CAM_ERR(CAM_VMRM, "vmrm release resources failed %d hw id 0x%x vmid %d",
				rc, hw_id, cam_vmrm_intf_get_vmid());
			goto end;
		}
		/*
		 * When release resource, need to decrease lend count and check if count is 1
		 * if it is 1, means all resource have been released, so need to release cpas.
		 */
		mutex_lock(&vmrm_intf_dev->lock);
		vmrm_intf_dev->lend_cnt--;
		CAM_DBG(CAM_VMRM, "lend_cnt %d", vmrm_intf_dev->lend_cnt);
		if (vmrm_intf_dev->lend_cnt == 1) {
			rc = cam_vmrm_release_resources(CAM_HW_ID_CPAS);
			if (rc) {
				CAM_ERR(CAM_VMRM,
					"vmrm release resources failed %d hw id 0x%x vmid %d",
					rc, CAM_HW_ID_CPAS, cam_vmrm_intf_get_vmid());
				mutex_unlock(&vmrm_intf_dev->lock);
				goto end;
			}
			vmrm_intf_dev->lend_cnt = 0;
		}
		mutex_unlock(&vmrm_intf_dev->lock);
	}

end:
	return rc;
}

int cam_vmrm_send_hw_msg_wrapper(uint32_t dest_vm, uint32_t hw_id, uint32_t msg_type,
	bool response_msg, bool need_response, void *msg, uint32_t msg_size, uint32_t timeout)
{
	int rc = 0;
	struct cam_hw_instance *hw_pos;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	mutex_lock(&vmrm_intf_dev->lock);
	hw_pos = cam_hw_instance_lookup(hw_id, 0, 0, 0);
	if (!hw_pos) {
		CAM_ERR(CAM_VMRM, "hw instance 0x%x look up failed", hw_id);
		mutex_unlock(&vmrm_intf_dev->lock);
		return -EINVAL;
	}
	mutex_unlock(&vmrm_intf_dev->lock);

	mutex_lock(&hw_pos->msg_comm_lock);
	reinit_completion(&hw_pos->wait_response);

	rc = cam_vmrm_send_msg(cam_vmrm_intf_get_vmid(), dest_vm,
		CAM_MSG_DST_TYPE_HW_INSTANCE, hw_id, msg_type, response_msg, need_response, msg,
		msg_size, &hw_pos->wait_response, timeout);
	if (rc) {
		CAM_ERR(CAM_VMRM, "send msg for hw id failed: 0x%x, rc: %d", hw_id, rc);
		mutex_unlock(&hw_pos->msg_comm_lock);
		return rc;
	}

	rc = hw_pos->response_result;
	if (!rc)
		CAM_DBG(CAM_VMRM, "send hw message succeed for hw_id:0x%x", hw_id);
	else
		CAM_ERR(CAM_VMRM, "send hw message failed for hw_id:0x%x", hw_id);

	mutex_unlock(&hw_pos->msg_comm_lock);

	return rc;

}

int cam_vmrm_send_driver_msg_wrapper(uint32_t dest_vm, uint32_t driver_id, uint32_t msg_type,
	bool response_msg, bool need_response, void *msg, uint32_t msg_size, uint32_t timeout)
{
	int rc = 0;
	struct cam_driver_node *driver_pos;
	struct cam_vmrm_intf_dev *vmrm_intf_dev;

	vmrm_intf_dev = cam_vmrm_get_intf_dev();
	mutex_lock(&vmrm_intf_dev->lock);
	driver_pos = cam_driver_node_lookup(driver_id);
	if (!driver_pos) {
		CAM_ERR(CAM_VMRM, "driver instance 0x%x look up failed", driver_id);
		mutex_unlock(&vmrm_intf_dev->lock);
		return -EINVAL;
	}
	mutex_unlock(&vmrm_intf_dev->lock);

	mutex_lock(&driver_pos->msg_comm_lock);
	reinit_completion(&driver_pos->wait_response);

	rc = cam_vmrm_send_msg(cam_vmrm_intf_get_vmid(), dest_vm,
		CAM_MSG_DST_TYPE_DRIVER_NODE, driver_id, msg_type, response_msg, need_response,
		msg, msg_size, &driver_pos->wait_response, timeout);
	if (rc) {
		CAM_ERR(CAM_VMRM, "send msg for driver id failed: 0x%x, rc: %d", driver_id, rc);
		mutex_unlock(&driver_pos->msg_comm_lock);
		return rc;
	}

	rc = driver_pos->response_result;
	if (!rc)
		CAM_DBG(CAM_VMRM, "send driver message succeed for hw_id:0x%x", driver_id);
	else
		CAM_ERR(CAM_VMRM, "send driver message failed for driver_id:0x%x", driver_id);

	mutex_unlock(&driver_pos->msg_comm_lock);

	return rc;

}

int cam_vmrm_set_clk_rate_level(uint32_t hw_id, int cesta_client_idx,
	enum cam_vote_level clk_level_high, enum cam_vote_level clk_level_low,
	bool do_not_set_src_clk, unsigned long clk_rate)
{
	int rc = 0;
	struct cam_msg_set_clk_rate_level msg_set_clk_rate_level;


	msg_set_clk_rate_level.cesta_client_idx = cesta_client_idx;
	msg_set_clk_rate_level.clk_level_high = clk_level_high;
	msg_set_clk_rate_level.clk_level_low = clk_level_low;
	msg_set_clk_rate_level.do_not_set_src_clk = do_not_set_src_clk;
	msg_set_clk_rate_level.clk_rate = clk_rate;


	rc = cam_vmrm_send_hw_msg_wrapper(CAM_PVM, hw_id, CAM_CLK_SET_RATE_LEVEL, false, true,
		&msg_set_clk_rate_level, sizeof(msg_set_clk_rate_level), 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "send msg for hw id failed: 0x%x, rc: %d", hw_id, rc);
	}

	return rc;
}

int cam_vmrm_soc_enable_disable_resources(uint32_t hw_id, bool flag)
{
	int rc = 0;
	uint32_t msg_type;

	if (flag)
		msg_type = CAM_SOC_ENABLE_RESOURCE;
	else
		msg_type = CAM_SOC_DISABLE_RESOURCE;

	rc = cam_vmrm_send_hw_msg_wrapper(CAM_PVM, hw_id, msg_type,
		false, true, NULL, 1, 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "send msg for hw id:0x%x msg_type:0x%x failed rc: %d",
			hw_id, msg_type, rc);
		return rc;
	}

	return rc;
}

int cam_vmrm_set_src_clk_rate(uint32_t hw_id, int cesta_client_idx,
	unsigned long clk_rate_high, unsigned long clk_rate_low)
{
	int rc = 0;
	struct cam_msg_set_clk_rate msg_set_clk_rate;

	msg_set_clk_rate.cesta_client_idx = cesta_client_idx;
	msg_set_clk_rate.clk_rate_high = clk_rate_high;
	msg_set_clk_rate.clk_rate_low = clk_rate_low;


	rc = cam_vmrm_send_hw_msg_wrapper(CAM_PVM, hw_id, CAM_CLK_SET_RATE, false, true,
		&msg_set_clk_rate, sizeof(msg_set_clk_rate), 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "send msg for hw id failed: 0x%x, rc: %d", hw_id, rc);
		return rc;
	}

	return rc;
}

int cam_vmrm_icc_vote(const char *name, uint64_t ab, uint64_t ib)
{
	int rc = 0;
	struct cam_msg_icc_vote msg_icc_vote;

	CAM_DBG(CAM_VMRM, "name: %s icc vote [%llu] ib[%llu]", name, ab, ib);

	scnprintf(msg_icc_vote.name, sizeof(msg_icc_vote.name), "%s", name);
	msg_icc_vote.ab = ab;
	msg_icc_vote.ib = ib;

	rc = cam_vmrm_send_hw_msg_wrapper(CAM_PVM, CAM_HW_ID_CPAS, CAM_ICC_VOTE, false, true,
		&msg_icc_vote, sizeof(msg_icc_vote), 0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "send msg for name %s failed rc: %d", name, rc);
		return rc;
	}

	return rc;
}

int cam_vmrm_sensor_power_up(uint32_t hw_id)
{
	int rc = 0;

	rc = cam_vmrm_send_hw_msg_wrapper(CAM_PVM, hw_id, CAM_HW_POWER_UP, false, true, NULL, 1,
		0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "hw id power up failed: 0x%x, rc: %d", hw_id, rc);
		return rc;
	}

	return rc;
}

int cam_vmrm_sensor_power_down(uint32_t hw_id)
{
	int rc = 0;

	rc = cam_vmrm_send_hw_msg_wrapper(CAM_PVM, hw_id, CAM_HW_POWER_DOWN, false, true, NULL, 1,
		0);
	if (rc) {
		CAM_ERR(CAM_VMRM, "hw id power down failed: 0x%x, rc: %d", hw_id, rc);
		return rc;
	}

	return rc;
}

int cam_vmrm_icp_send_msg(uint32_t dest_vm, uint32_t hw_mgr_id, uint32_t msg_type, bool need_ack,
	void *msg, uint32_t msg_size, uint32_t timeout)
{
	int rc = 0;

	rc = cam_vmrm_send_driver_msg_wrapper(dest_vm, CAM_DRIVER_ID_ICP + hw_mgr_id,
		msg_type, false, need_ack, msg, msg_size, timeout);
	if (rc) {
		CAM_ERR(CAM_VMRM, "ICP%d Failed in sending msg dest_driver:%d  rc %d",
			hw_mgr_id, CAM_DRIVER_ID_ICP + hw_mgr_id, rc);
		return rc;
	}


	return rc;
}

#else
bool cam_vmrm_is_supported(void)
{
	return false;
}

uint32_t cam_vmrm_intf_get_vmid(void)
{
	return CAM_PVM;
}

bool cam_vmrm_proxy_clk_rgl_voting_enable(void)
{
	return false;
}

bool cam_vmrm_proxy_icc_voting_enable(void)
{
	return false;
}

bool cam_vmrm_no_register_read_on_bind(void)
{
	return false;
}

int cam_vmvm_populate_hw_instance_info(struct cam_hw_soc_info *soc_info,
	msg_cb_func hw_msg_callback, void *hw_msg_callback_data)
{
	return 0;
}

int cam_vmrm_populate_driver_node_info(struct cam_driver_node *driver_node)
{
	return 0;
}

int cam_vmrm_populate_io_resource_info(void)
{
	return 0;
}

int cam_vmrm_register_gh_callback(void)
{
	return 0;
}

int cam_vmrm_unregister_gh_callback(void)
{
	return 0;
}

int cam_vmrm_send_msg(uint32_t source_vmid, uint32_t des_vmid, uint32_t msg_dst_type,
	uint32_t msg_dst_id, uint32_t msg_type, bool response_msg, bool need_response,
	void *msg_data, uint32_t data_size, struct completion *complete, uint32_t timeout)
{
	return 0;
}

int cam_vmrm_soc_acquire_resources(uint32_t hw_id)
{
	return 0;
}

int cam_vmrm_soc_release_resources(uint32_t hw_id)
{
	return 0;
}

int cam_vmrm_soc_enable_disable_resources(uint32_t hw_id, bool flag)
{
	return 0;
}

int cam_vmrm_set_src_clk_rate(uint32_t hw_id, int cesta_client_idx,
	unsigned long clk_rate_high, unsigned long clk_rate_low)
{
	return 0;
}

int cam_vmrm_set_clk_rate_level(uint32_t hw_id, int cesta_client_idx,
	enum cam_vote_level clk_level_high, enum cam_vote_level clk_level_low,
	bool do_not_set_src_clk, unsigned long clk_rate)
{
	return 0;
}

int cam_vmrm_icc_vote(const char *name, uint64_t ab, uint64_t ib)
{
	return 0;
}

int cam_vmrm_sensor_power_up(uint32_t hw_id)
{
	return 0;
}

int cam_vmrm_sensor_power_down(uint32_t hw_id)
{
	return 0;
}
#endif
