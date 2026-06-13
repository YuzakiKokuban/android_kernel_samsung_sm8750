/* SPDX-License-Identifier: GPL-2.0-only */
/* drivers/input/sec_input/sec_trusted_touch.c
 *
 * Core file for Samsung input device driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_trusted_touch.h"

static int sec_trusted_touch_get_pvm_driver_state(struct trusted_touch_vm_info *vm_info)
{
	return atomic_read(&vm_info->vm_state);
}

static void sec_trusted_touch_set_pvm_driver_state(struct trusted_touch_vm_info *vm_info,
							int state)
{
	atomic_set(&vm_info->vm_state, state);
}

static void sec_trusted_touch_event_notify(struct sec_ts_plat_data *pdata, int event)
{
	atomic_set(&pdata->pvm->trusted_touch_event, event);
	sysfs_notify(&pdata->input_dev->dev.kobj, NULL, "trusted_touch_event");
}

static void sec_trusted_clk_disable_unprepare(struct sec_trusted_touch *pvm)
{
	clk_disable_unprepare(pvm->core_clk);
	clk_disable_unprepare(pvm->iface_clk);
}

static void sec_trusted_bus_put(struct sec_ts_plat_data *pdata)
{
	struct sec_trusted_touch *pvm = pdata->pvm;
	struct device *dev = NULL;

	dev = pdata->bus_master->parent;

	mutex_lock(&pvm->clk_io_ctrl_mutex);
	if (pvm->core_clk != NULL && pvm->iface_clk != NULL)
		sec_trusted_clk_disable_unprepare(pvm);
	pm_runtime_put_sync(dev);
	mutex_unlock(&pvm->clk_io_ctrl_mutex);
}

static void sec_trusted_touch_abort_handler(struct sec_ts_plat_data *pdata, int error)
{
	atomic_set(&pdata->pvm->trusted_touch_abort_status, error);
	input_info(true, pdata->dev, "TUI session aborted with failure:%d\n", error);
	sec_trusted_touch_event_notify(pdata, error);
}


static struct gh_acl_desc *sec_trusted_vm_get_acl(enum gh_vm_names vm_name)
{
	struct gh_acl_desc *acl_desc;
	gh_vmid_t vmid;

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	ghd_rm_get_vmid(vm_name, &vmid);
#else
	gh_rm_get_vmid(vm_name, &vmid);
#endif

	acl_desc = kzalloc(offsetof(struct gh_acl_desc, acl_entries[1]),
			GFP_KERNEL);
	if (!acl_desc)
		return ERR_PTR(ENOMEM);

	acl_desc->n_acl_entries = 1;
	acl_desc->acl_entries[0].vmid = vmid;
	acl_desc->acl_entries[0].perms = GH_RM_ACL_R | GH_RM_ACL_W;

	return acl_desc;
}

static struct gh_sgl_desc *sec_trusted_vm_get_sgl(
				struct trusted_touch_vm_info *vm_info)
{
	struct gh_sgl_desc *sgl_desc;
	int i;

	sgl_desc = kzalloc(offsetof(struct gh_sgl_desc,
			sgl_entries[vm_info->iomem_list_size]), GFP_KERNEL);
	if (!sgl_desc)
		return ERR_PTR(ENOMEM);

	sgl_desc->n_sgl_entries = vm_info->iomem_list_size;

	for (i = 0; i < vm_info->iomem_list_size; i++) {
		sgl_desc->sgl_entries[i].ipa_base = vm_info->iomem_bases[i];
		sgl_desc->sgl_entries[i].size = vm_info->iomem_sizes[i];
	}

	return sgl_desc;
}

static int sec_trusted_populate_vm_info(struct device *dev)
{
	int i, gpio, rc = 0;
	struct trusted_touch_vm_info *vm_info;
	struct device_node *np = dev->of_node;
	int num_regs, num_sizes, num_gpios, list_size;
	struct resource res;
	struct sec_ts_plat_data *pdata;

	pdata = (struct sec_ts_plat_data *)dev_get_platdata(dev);

	vm_info = kzalloc(sizeof(struct trusted_touch_vm_info), GFP_KERNEL);
	if (!vm_info) {
		rc = -ENOMEM;
		goto error;
	}

	pdata->pvm->vm_info = vm_info;
	vm_info->vm_name = GH_TRUSTED_VM;

	rc = of_property_read_u32(np, "trusted-touch-spi-irq", &vm_info->hw_irq);
	if (rc) {
		input_err(true, pdata->dev, "Failed to read trusted touch SPI irq:%d\n", rc);
		goto vm_error;
	}

	num_regs = of_property_count_u32_elems(np, "trusted-touch-io-bases");
	if (num_regs < 0) {
		input_err(true, pdata->dev, "Invalid number of IO regions specified\n");
		rc = -EINVAL;
		goto vm_error;
	}

	num_sizes = of_property_count_u32_elems(np, "trusted-touch-io-sizes");
	if (num_sizes < 0) {
		input_err(true, pdata->dev, "Invalid number of IO regions specified\n");
		rc = -EINVAL;
		goto vm_error;
	}

	if (num_regs != num_sizes) {
		input_err(true, pdata->dev, "IO bases and sizes doe not match\n");
		rc = -EINVAL;
		goto vm_error;
	}

	num_gpios = of_count_phandle_with_args(np, "trusted-touch-vm-gpio-list", "#gpio-cells");
	if (num_gpios < 0) {
		input_err(true, pdata->dev, "Ignoring invalid trusted gpio list: %d\n", num_gpios);
		num_gpios = 0;
	}

	list_size = num_regs + num_gpios;
	vm_info->iomem_list_size = list_size;

	vm_info->iomem_bases = devm_kcalloc(pdata->dev, list_size, sizeof(*vm_info->iomem_bases), GFP_KERNEL);
	if (!vm_info->iomem_bases) {
		rc = -ENOMEM;
		goto vm_error;
	}

	vm_info->iomem_sizes = devm_kcalloc(pdata->dev, list_size, sizeof(*vm_info->iomem_sizes), GFP_KERNEL);
	if (!vm_info->iomem_sizes) {
		rc = -ENOMEM;
		goto io_bases_error;
	}

	for (i = 0; i < num_gpios; ++i) {
		gpio = of_get_named_gpio(np, "trusted-touch-vm-gpio-list", i);
		if (gpio < 0 || !gpio_is_valid(gpio)) {
			input_err(true, pdata->dev, "Invalid gpio %d at position %d\n", gpio, i);
			return gpio;
		}

		if (!msm_gpio_get_pin_address(gpio, &res)) {
			input_err(true, pdata->dev, "Failed to retrieve gpio-%d resource\n", gpio);
			return -ENODATA;
		}

		vm_info->iomem_bases[i] = res.start;
		vm_info->iomem_sizes[i] = resource_size(&res);
	}

	rc = of_property_read_u32_array(np, "trusted-touch-io-bases",
			&vm_info->iomem_bases[i], list_size - i);
	if (rc) {
		input_err(true, pdata->dev, "Failed to read trusted touch io bases:%d\n", rc);
		goto io_bases_error;
	}

	rc = of_property_read_u32_array(np, "trusted-touch-io-sizes",
			&vm_info->iomem_sizes[i], list_size - i);
	if (rc) {
		input_err(true, pdata->dev, "Failed to read trusted touch io sizes:%d\n", rc);
		goto io_sizes_error;
	}

	rc = of_property_read_string(np, "trusted-touch-type",
						&vm_info->trusted_touch_type);
	if (rc) {
		input_err(true, pdata->dev, "%s: No trusted touch type selection made\n", __func__);
		vm_info->mem_tag = GH_MEM_NOTIFIER_TAG_TOUCH_PRIMARY;
		vm_info->irq_label = GH_IRQ_LABEL_TRUSTED_TOUCH_PRIMARY;
		rc = 0;
	} else if (!strcmp(vm_info->trusted_touch_type, "primary")) {
		vm_info->mem_tag = GH_MEM_NOTIFIER_TAG_TOUCH_PRIMARY;
		vm_info->irq_label = GH_IRQ_LABEL_TRUSTED_TOUCH_PRIMARY;
	} else if (!strcmp(vm_info->trusted_touch_type, "secondary")) {
		vm_info->mem_tag = GH_MEM_NOTIFIER_TAG_TOUCH_SECONDARY;
		vm_info->irq_label = GH_IRQ_LABEL_TRUSTED_TOUCH_SECONDARY;
	}

	return rc;

io_sizes_error:
	kfree(vm_info->iomem_sizes);
io_bases_error:
	kfree(vm_info->iomem_bases);
vm_error:
	kfree(vm_info);
error:
	return rc;
}

static void sec_trusted_destroy_vm_info(struct trusted_touch_vm_info *vm_info)
{
	kfree(vm_info->iomem_sizes);
	kfree(vm_info->iomem_bases);
	kfree(vm_info);
}

static void sec_trusted_vm_deinit(struct trusted_touch_vm_info *vm_info)
{
	if (vm_info->mem_cookie)
		gh_mem_notifier_unregister(vm_info->mem_cookie);
	sec_trusted_destroy_vm_info(vm_info);
}

static void sec_trusted_touch_abort_pvm(struct sec_ts_plat_data *pdata)
{
	struct sec_trusted_touch *pvm = pdata->pvm;
	struct trusted_touch_vm_info *vm_info = pvm->vm_info;
	int rc = 0;
	int vm_state = sec_trusted_touch_get_pvm_driver_state(vm_info);

	if (vm_state >= TRUSTED_TOUCH_PVM_STATE_MAX) {
		input_err(true, pdata->dev, "Invalid driver state: %d\n", vm_state);
		return;
	}

	switch (vm_state) {
	case PVM_IRQ_RELEASE_NOTIFIED:
	case PVM_ALL_RESOURCES_RELEASE_NOTIFIED:
	case PVM_IRQ_LENT:
	case PVM_IRQ_LENT_NOTIFIED:
		rc = gh_irq_reclaim(vm_info->irq_label);
		if (rc)
			input_err(true, pdata->dev, "failed to reclaim irq on pvm rc:%d\n", rc);
		fallthrough;
	case PVM_IRQ_RECLAIMED:
	case PVM_IOMEM_LENT:
	case PVM_IOMEM_LENT_NOTIFIED:
	case PVM_IOMEM_RELEASE_NOTIFIED:
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
		rc = ghd_rm_mem_reclaim(vm_info->vm_mem_handle, 0);
#else
		rc = gh_rm_mem_reclaim(vm_info->vm_mem_handle, 0);
#endif
		if (rc)
			input_err(true, pdata->dev, "failed to reclaim iomem on pvm rc:%d\n", rc);
		vm_info->vm_mem_handle = 0;
		fallthrough;
	case PVM_IOMEM_RECLAIMED:
	case PVM_INTERRUPT_DISABLED:
		enable_irq(pdata->irq);
		fallthrough;
	case PVM_I2C_RESOURCE_ACQUIRED:
	case PVM_INTERRUPT_ENABLED:
		sec_trusted_bus_put(pdata);
		fallthrough;
	case TRUSTED_TOUCH_PVM_INIT:
	case PVM_I2C_RESOURCE_RELEASED:
		atomic_set(&pvm->trusted_touch_enabled, 0);
		atomic_set(&pvm->trusted_touch_transition, 0);
		complete_all(&pvm->trusted_touch_powerdown);
		break;
	}

	atomic_set(&pvm->trusted_touch_abort_status, 0);

	sec_trusted_touch_set_pvm_driver_state(vm_info, TRUSTED_TOUCH_PVM_INIT);
}

static int sec_trusted_clk_prepare_enable(struct sec_ts_plat_data *pdata)
{
	struct sec_trusted_touch *pvm = pdata->pvm;
	int ret;

	ret = clk_prepare_enable(pvm->iface_clk);
	if (ret) {
		input_err(true, pdata->dev, "%s: error on clk_prepare_enable(iface_clk): %d\n", __func__, ret);
		return ret;
	}

	ret = clk_prepare_enable(pvm->core_clk);
	if (ret) {
		clk_disable_unprepare(pvm->iface_clk);
		input_err(true, pdata->dev, "%s: error clk_prepare_enable(core_clk): %d\n", __func__, ret);
	}
	return ret;
}

static int sec_trusted_bus_get(struct sec_ts_plat_data *pdata)
{
	struct sec_trusted_touch *pvm = pdata->pvm;

	int rc = 0;
	struct device *dev = NULL;

	dev = pdata->bus_master->parent;

	mutex_lock(&pvm->clk_io_ctrl_mutex);
	rc = pm_runtime_get_sync(dev);
	if (rc >= 0 &&  pvm->core_clk != NULL &&
				pvm->iface_clk != NULL) {
		rc = sec_trusted_clk_prepare_enable(pdata);
		if (rc)
			pm_runtime_put_sync(dev);
	}

	mutex_unlock(&pvm->clk_io_ctrl_mutex);
	return rc;
}

static struct gh_notify_vmid_desc *sec_trusted_vm_get_vmid(gh_vmid_t vmid)
{
	struct gh_notify_vmid_desc *vmid_desc;

	vmid_desc = kzalloc(offsetof(struct gh_notify_vmid_desc,
				vmid_entries[1]), GFP_KERNEL);
	if (!vmid_desc)
		return ERR_PTR(ENOMEM);

	vmid_desc->n_vmid_entries = 1;
	vmid_desc->vmid_entries[0].vmid = vmid;
	return vmid_desc;
}

static void stm_trusted_touch_pvm_vm_mode_disable(struct sec_ts_plat_data *pdata)
{
	struct sec_trusted_touch *pvm = pdata->pvm;
	struct trusted_touch_vm_info *vm_info = pvm->vm_info;
	int rc = 0;

	atomic_set(&pvm->trusted_touch_transition, 1);

	if (atomic_read(&pvm->trusted_touch_abort_status)) {
		sec_trusted_touch_abort_pvm(pdata);
		return;
	}

	if (sec_trusted_touch_get_pvm_driver_state(vm_info) !=
					PVM_ALL_RESOURCES_RELEASE_NOTIFIED)
		input_err(true, pdata->dev, "all release notifications are not received yet\n");

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	rc = ghd_rm_mem_reclaim(vm_info->vm_mem_handle, 0);
#else
	rc = gh_rm_mem_reclaim(vm_info->vm_mem_handle, 0);
#endif
	if (rc) {
		input_err(true, pdata->dev, "Trusted touch VM mem reclaim failed rc:%d\n", rc);
		goto error;
	}
	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_IOMEM_RECLAIMED);
	vm_info->vm_mem_handle = 0;
	input_info(true, pdata->dev, "vm mem reclaim succeded!\n");

	rc = gh_irq_reclaim(vm_info->irq_label);
	if (rc) {
		input_err(true, pdata->dev, "failed to reclaim irq on pvm rc:%d\n", rc);
		goto error;
	}
	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_IRQ_RECLAIMED);
	input_info(true, pdata->dev, "vm irq reclaim succeded!\n");

	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_INTERRUPT_ENABLED);
	sec_trusted_bus_put(pdata);
	atomic_set(&pvm->trusted_touch_transition, 0);
	sec_trusted_touch_set_pvm_driver_state(vm_info,
						PVM_I2C_RESOURCE_RELEASED);
	sec_trusted_touch_set_pvm_driver_state(vm_info,
						TRUSTED_TOUCH_PVM_INIT);

	sec_delay(250);

	atomic_set(&pvm->trusted_touch_enabled, 0);
	complete_all(&pvm->trusted_touch_powerdown);
	input_info(true, pdata->dev, "trusted touch disabled\n");

	enable_irq(pdata->irq);
	return;
error:
	sec_trusted_touch_abort_handler(pdata,
			TRUSTED_TOUCH_EVENT_RECLAIM_FAILURE);
}

static void sec_trusted_vm_irq_on_release_callback(void *data,
					unsigned long notif_type,
					enum gh_irq_label label)
{
	struct sec_ts_plat_data *pdata = (struct sec_ts_plat_data *)data;
	struct trusted_touch_vm_info *vm_info = pdata->pvm->vm_info;
	if (notif_type != GH_RM_NOTIF_VM_IRQ_RELEASED) {
		input_err(true, pdata->dev, "invalid notification type\n");
		return;
	}
	if (sec_trusted_touch_get_pvm_driver_state(vm_info) == PVM_IOMEM_RELEASE_NOTIFIED)
		sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_ALL_RESOURCES_RELEASE_NOTIFIED);
	else
		sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_IRQ_RELEASE_NOTIFIED);
}

static void sec_trusted_vm_mem_on_release_handler(enum gh_mem_notifier_tag tag,
		unsigned long notif_type, void *entry_data, void *notif_msg)
{
	struct sec_ts_plat_data *pdata;
	struct trusted_touch_vm_info *vm_info;
	struct gh_rm_notif_mem_released_payload *release_payload;

	if (!entry_data) {
		pr_err("[sec_input] %s: Invalid entry_data\n", __func__);
		return;
	}

	pdata = (struct sec_ts_plat_data *)entry_data;

	vm_info = pdata->pvm->vm_info;
	if (!vm_info) {
		input_err(true, pdata->dev, "Invalid vm_info\n");
		return;
	}

	if (notif_type != GH_RM_NOTIF_MEM_RELEASED) {
		input_err(true, pdata->dev, "Invalid notification type\n");
		return;
	}

	if (tag != vm_info->mem_tag) {
		input_err(true, pdata->dev, "Invalid tag\n");
		return;
	}

	if (!notif_msg) {
		input_err(true, pdata->dev, "Invalid data or notification message\n");
		return;
	}

	release_payload = (struct gh_rm_notif_mem_released_payload  *)notif_msg;
	if (release_payload->mem_handle != vm_info->vm_mem_handle) {
		input_err(true, pdata->dev, "Invalid mem handle detected\n");
		return;
	}

	input_info(true, pdata->dev, "received mem lend request with handle:\n");
	if (sec_trusted_touch_get_pvm_driver_state(vm_info) ==
				PVM_IRQ_RELEASE_NOTIFIED) {
		sec_trusted_touch_set_pvm_driver_state(vm_info,
			PVM_ALL_RESOURCES_RELEASE_NOTIFIED);
	} else {
		sec_trusted_touch_set_pvm_driver_state(vm_info,
			PVM_IOMEM_RELEASE_NOTIFIED);
	}
}

static int sec_trusted_vm_mem_lend(struct sec_ts_plat_data *pdata)
{
	struct gh_acl_desc *acl_desc;
	struct gh_sgl_desc *sgl_desc;
	struct gh_notify_vmid_desc *vmid_desc;
	gh_memparcel_handle_t mem_handle;
	gh_vmid_t trusted_vmid;
	struct trusted_touch_vm_info *vm_info = pdata->pvm->vm_info;
	int rc = 0;

	acl_desc = sec_trusted_vm_get_acl(GH_TRUSTED_VM);
	if (IS_ERR(acl_desc)) {
		input_err(true, pdata->dev, "Failed to get acl of IO memories for Trusted touch(%ld)\n",
			PTR_ERR(acl_desc));
		return -EINVAL;
	}

	sgl_desc = sec_trusted_vm_get_sgl(vm_info);
	if (IS_ERR(sgl_desc)) {
		input_err(true, pdata->dev, "Failed to get sgl of IO memories for Trusted touch(%ld)\n",
			PTR_ERR(sgl_desc));
		rc = -EINVAL;
		goto sgl_error;
	}

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	rc = ghd_rm_mem_lend(GH_RM_MEM_TYPE_IO, 0, TRUSTED_TOUCH_MEM_LABEL,
			acl_desc, sgl_desc, NULL, &mem_handle);
#else
	rc = gh_rm_mem_lend(GH_RM_MEM_TYPE_IO, 0, TRUSTED_TOUCH_MEM_LABEL,
			acl_desc, sgl_desc, NULL, &mem_handle);
#endif
	if (rc) {
		input_err(true, pdata->dev, "Failed to lend IO memories for Trusted touch rc:%d\n",
							rc);
		goto error;
	}

	input_info(true, pdata->dev, "vm mem lend succeded\n");

	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_IOMEM_LENT);

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	ghd_rm_get_vmid(GH_TRUSTED_VM, &trusted_vmid);
#else
	gh_rm_get_vmid(GH_TRUSTED_VM, &trusted_vmid);
#endif

	vmid_desc = sec_trusted_vm_get_vmid(trusted_vmid);

	rc = gh_rm_mem_notify(mem_handle, GH_RM_MEM_NOTIFY_RECIPIENT_SHARED,
			vm_info->mem_tag, vmid_desc);
	if (rc) {
		input_err(true, pdata->dev, "Failed to notify mem lend to hypervisor rc:%d\n", rc);
		goto vmid_error;
	}

	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_IOMEM_LENT_NOTIFIED);

	vm_info->vm_mem_handle = mem_handle;
vmid_error:
	kfree(vmid_desc);
error:
	kfree(sgl_desc);
sgl_error:
	kfree(acl_desc);

	return rc;
}

static int sec_trusted_touch_pvm_vm_mode_enable(struct sec_ts_plat_data *pdata)
{
	struct sec_trusted_touch *pvm = pdata->pvm;
	int rc = 0;
	struct trusted_touch_vm_info *vm_info = pvm->vm_info;

	atomic_set(&pvm->trusted_touch_transition, 1);
	mutex_lock(&pvm->transition_lock);

	if (atomic_read(&pdata->enabled) == false) {
		input_err(true, pdata->dev, "Invalid power state for operation\n");
		atomic_set(&pvm->trusted_touch_transition, 0);
		rc =  -EPERM;
		goto error;
	}

	/* i2c session start and resource acquire */
	if (sec_trusted_bus_get(pdata) < 0) {
		input_err(true, pdata->dev, "sec_trusted_bus_get failed\n");
		rc = -EIO;
		goto error;
	}

	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_I2C_RESOURCE_ACQUIRED);
	/* flush pending interurpts from FIFO */
	disable_irq(pdata->irq);
	reinit_completion(&pvm->trusted_touch_powerdown);
	atomic_set(&pvm->trusted_touch_enabled, 1);
	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_INTERRUPT_DISABLED);
	sec_input_release_all_finger(pdata->dev);

	rc = sec_trusted_vm_mem_lend(pdata);
	if (rc) {
		input_err(true, pdata->dev, "Failed to lend memory\n");
		goto abort_handler;
	}
	input_info(true, pdata->dev, "[sec_input] vm mem lend succeded\n");
	rc = gh_irq_lend_v2(vm_info->irq_label, vm_info->vm_name,
		pdata->irq, &sec_trusted_vm_irq_on_release_callback, pdata);
	if (rc) {
		input_err(true, pdata->dev, "Failed to lend irq\n");
		goto abort_handler;
	}

	input_info(true, pdata->dev, "vm irq lend succeded for irq:%d\n", pdata->irq);
	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_IRQ_LENT);

	rc = gh_irq_lend_notify(vm_info->irq_label);
	if (rc) {
		input_info(true, pdata->dev, "Failed to notify irq\n");
		goto abort_handler;
	}
	sec_trusted_touch_set_pvm_driver_state(vm_info, PVM_IRQ_LENT_NOTIFIED);

	mutex_unlock(&pvm->transition_lock);
	atomic_set(&pvm->trusted_touch_transition, 0);
	input_info(true, pdata->dev, "trusted touch enabled\n");
	return rc;

abort_handler:
	atomic_set(&pvm->trusted_touch_enabled, 0);
	complete_all(&pvm->trusted_touch_powerdown);
	sec_trusted_touch_abort_handler(pdata, TRUSTED_TOUCH_EVENT_LEND_FAILURE);

error:
	mutex_unlock(&pvm->transition_lock);
	return rc;
}

int sec_trusted_handle_trusted_touch_pvm(struct sec_ts_plat_data *pdata, int value)
{
	struct sec_trusted_touch *pvm = pdata->pvm;
	int err = 0;

	if (pvm->pre_func)
		pvm->pre_func(pdata->dev, value);

	switch (value) {
	case 0:
		if (atomic_read(&pvm->trusted_touch_enabled) == 0 &&
			(atomic_read(&pvm->trusted_touch_abort_status) == 0)) {
			input_err(true, pdata->dev, "Trusted touch is already disabled\n");
			break;
		}
		if (atomic_read(&pvm->trusted_touch_mode) ==
				TRUSTED_TOUCH_VM_MODE) {
			stm_trusted_touch_pvm_vm_mode_disable(pdata);
		} else {
			input_err(true, pdata->dev, "Unsupported trusted touch mode\n");
		}
		break;

	case 1:
		if (atomic_read(&pvm->trusted_touch_enabled)) {
			input_err(true, pdata->dev, "Trusted touch usecase underway\n");
			err = -EBUSY;
			break;
		}
		if (atomic_read(&pvm->trusted_touch_mode) ==
				TRUSTED_TOUCH_VM_MODE) {
			err = sec_trusted_touch_pvm_vm_mode_enable(pdata);
		} else {
			input_err(true, pdata->dev, "Unsupported trusted touch mode\n");
		}
		break;

	default:
		input_err(true, pdata->dev, "unsupported value: %d\n", value);
		err = -EINVAL;
		break;
	}

	if (pvm->post_func)
		pvm->post_func(pdata->dev, value);

	return err;
}

static int sec_trusted_vm_init(struct device *dev)
{
	int rc = 0;
	struct trusted_touch_vm_info *vm_info;
	struct sec_ts_plat_data *pdata = (struct sec_ts_plat_data *)dev_get_platdata(dev);
	struct sec_trusted_touch  *pvm;
	void *mem_cookie;

	pvm = pdata->pvm;

	rc = sec_trusted_populate_vm_info(dev);
	if (rc) {
		input_err(true, pdata->dev, "Cannot setup vm pipeline\n");
		rc = -EINVAL;
		goto fail;
	}

	vm_info = pvm->vm_info;
	mem_cookie = gh_mem_notifier_register(vm_info->mem_tag,
			sec_trusted_vm_mem_on_release_handler, pdata);
	if (!mem_cookie) {
		input_err(true, pdata->dev, "Failed to register on release mem notifier\n");
		rc = -EINVAL;
		goto init_fail;
	}

	vm_info->mem_cookie = mem_cookie;
	sec_trusted_touch_set_pvm_driver_state(vm_info, TRUSTED_TOUCH_PVM_INIT);
	return rc;
init_fail:
	sec_trusted_vm_deinit(vm_info);
fail:
	return rc;
}

static void sec_trusted_dt_parse_touch_info(struct device *dev, struct sec_trusted_touch *pvm)
{
	struct device_node *np = dev->of_node;
	struct sec_ts_plat_data *pdata = dev->platform_data;

	int rc = 0;
	const char *selection;
	const char *environment;

	rc = of_property_read_string(np, "trusted-touch-mode",
								&selection);
	if (rc) {
		input_err(true, pdata->dev, "%s: No trusted touch mode selection made\n", __func__);
		atomic_set(&pvm->trusted_touch_mode,
						TRUSTED_TOUCH_MODE_NONE);
		return;
	}

	if (!strcmp(selection, "vm_mode")) {
		atomic_set(&pvm->trusted_touch_mode,
						TRUSTED_TOUCH_VM_MODE);
		input_err(true, pdata->dev, "Selected trusted touch mode to VM mode\n");
	} else {
		atomic_set(&pvm->trusted_touch_mode,
						TRUSTED_TOUCH_MODE_NONE);
		input_err(true, pdata->dev, "Invalid trusted_touch mode\n");
	}

	rc = of_property_read_string(np, "touch-environment",
						&environment);
	if (rc)
		input_err(true, pdata->dev, "%s: No trusted touch mode environment\n", __func__);

	pvm->touch_environment = environment;
	input_info(true, pdata->dev, "Trusted touch environment:%s\n",
			pvm->touch_environment);
}


static ssize_t trusted_touch_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct sec_ts_plat_data *pdata = input_dev->dev.parent->platform_data;
	struct sec_trusted_touch *pvm = pdata->pvm;

	input_info(true, pdata->dev, "%s\n", __func__);
	return scnprintf(buf, PAGE_SIZE, "%d", atomic_read(&pvm->trusted_touch_enabled));
}

static ssize_t trusted_touch_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct sec_ts_plat_data *pdata = input_dev->dev.parent->platform_data;
	struct sec_trusted_touch *pvm = pdata->pvm;
	unsigned long value;
	int ret;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret < 0)
		return ret;

	if (!atomic_read(&pvm->trusted_touch_initialized))
		return -EIO;

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)//TODO: check for call back from core driver.
	if (value == 1)
		sec_input_notify(NULL, NOTIFIER_SECURE_TOUCH_ENABLE, NULL);
#endif
	input_info(true, pdata->dev, "%s: value: %ld\n", __func__, value);

	ret = sec_trusted_handle_trusted_touch_pvm(pdata, value);

	if (ret) {
		input_err(true, pdata->dev, "Failed to handle touch vm\n");
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)//TODO: check for call back from core driver.
		sec_input_notify(NULL, NOTIFIER_SECURE_TOUCH_DISABLE, NULL);
#endif
		return -EINVAL;
	}

	if (value == 1)
		reinit_completion(&pdata->secure_powerdown);
	else
		complete_all(&pdata->secure_powerdown);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)//TODO: check for call back from core driver.
	if (value == 0)
		sec_input_notify(NULL, NOTIFIER_SECURE_TOUCH_DISABLE, NULL);
#endif

	return count;
}

static ssize_t trusted_touch_event_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct sec_ts_plat_data *pdata = input_dev->dev.parent->platform_data;

	input_err(true, pdata->dev, "%s\n", __func__);
	return scnprintf(buf, PAGE_SIZE, "%d", atomic_read(&pdata->pvm->trusted_touch_event));
}

static ssize_t trusted_touch_event_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct sec_ts_plat_data *pdata = input_dev->dev.parent->platform_data;
	unsigned long value;
	int ret;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret < 0)
		return ret;

	if (!atomic_read(&pdata->pvm->trusted_touch_initialized))
		return -EIO;

	input_info(true, pdata->dev, "%s: value: %ld\n", __func__, value);

	atomic_set(&pdata->pvm->trusted_touch_event, value);

	return count;
}

static ssize_t trusted_touch_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct sec_ts_plat_data *pdata = input_dev->dev.parent->platform_data;
	input_info(true, pdata->dev, "%s\n", __func__);
	return scnprintf(buf, PAGE_SIZE, "%s", pdata->pvm->vm_info->trusted_touch_type);
}

static DEVICE_ATTR_RW(trusted_touch_enable);
static DEVICE_ATTR_RW(trusted_touch_event);
static DEVICE_ATTR_RO(trusted_touch_type);

static struct attribute *trusted_attr[] = {
	&dev_attr_trusted_touch_enable.attr,
	&dev_attr_trusted_touch_event.attr,
	&dev_attr_trusted_touch_type.attr,
	NULL,
};

static struct attribute_group trusted_attr_group = {//TODO: Move it back to core driver
	.attrs = trusted_attr,
};

int sec_trusted_touch_init(struct device *dev)
{
	int rc = 0;
	int ret = 0;
	struct sec_trusted_touch *pvm;
	struct sec_ts_plat_data *pdata = dev->platform_data;

	pvm = devm_kzalloc(pdata->dev, sizeof(struct sec_trusted_touch), GFP_KERNEL);
	if (!pvm) {
		ret = -ENOMEM;
		input_err(true, pdata->dev, "%s: fail to allocate memory for sec_trusted_touch\n", __func__);
		goto error_allocate_pvm;
	}
	pdata->pvm = pvm;

	atomic_set(&pvm->trusted_touch_initialized, 0);
	sec_trusted_dt_parse_touch_info(pdata->dev, pvm);

	if (atomic_read(&pvm->trusted_touch_mode) == TRUSTED_TOUCH_MODE_NONE)
		return -1;

	init_completion(&pvm->trusted_touch_powerdown);

	/* Get clocks */

	pvm->core_clk = devm_clk_get(pdata->bus_master->parent, "m-ahb");
	if (IS_ERR(pvm->core_clk)) {
		pvm->core_clk = NULL;
		input_err(true, pdata->dev, "%s: core_clk is not defined\n", __func__);
	}

	pvm->iface_clk = devm_clk_get(pdata->bus_master->parent, "se-clk");
	if (IS_ERR(pvm->iface_clk)) {
		pvm->iface_clk = NULL;
		input_err(true, pdata->dev, "%s: iface_clk is not defined\n", __func__);
	}

	if (atomic_read(&pvm->trusted_touch_mode) ==
						TRUSTED_TOUCH_VM_MODE) {
		rc = sec_trusted_vm_init(pdata->dev);
		if (rc)
			input_err(true, pdata->dev, "Failed to init VM\n");
	}
	atomic_set(&pvm->trusted_touch_initialized, 1);

	if (sysfs_create_group(&pdata->input_dev->dev.kobj, &trusted_attr_group) < 0)
		input_err(true, pdata->dev, "%s: do not make secure group\n", __func__);

	mutex_init(&pvm->clk_io_ctrl_mutex);
	mutex_init(&pvm->transition_lock);

error_allocate_pvm:
	return ret;
}
EXPORT_SYMBOL(sec_trusted_touch_init);

MODULE_DESCRIPTION("Samsung input trusted PVM touch");
MODULE_LICENSE("GPL");