/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_VMRM_GH_WRAPPER_H_
#define _CAM_VMRM_GH_WRAPPER_H_

#include <linux/gunyah/gh_irq_lend.h>
#include <linux/gunyah/gh_mem_notifier.h>
#include "cam_debug_util.h"

typedef int (*gh_res_cb) (void);

/**
 * cam_vmrm_gh_irq_lend_v2()
 *
 * @brief:        Gh interrupt lend api wrapper
 *
 * @label:        vIRQ high-level label
 * @name:         VM name to send interrupt to
 * @irq:          Linux IRQ number to lend
 * @cb_handle:    Callback to invoke when other VM release or accept the interrupt
 * @data:         Argument to pass to cb_handle
 *
 */
int cam_vmrm_gh_irq_lend_v2(enum gh_irq_label label, enum gh_vm_names name,
	int irq, gh_irq_handle_fn_v2 cb_handle, void *data);

/**
 * cam_vmrm_gh_irq_lend_notify()
 *
 * @brief:        Gh interrupt lend notify api wrapper
 *
 * @label:        vIRQ high-level label
 *
 */
int cam_vmrm_gh_irq_lend_notify(enum gh_irq_label label);

/**
 * cam_vmrm_gh_irq_wait_for_lend_v2()
 *
 * @brief:        Gh interrupt wait for lend api wrapper
 *
 * @label:        vIRQ high-level label
 * @name:         VM name to send interrupt to
 * @on_lend:      Callback to invoke when other VM release or accept the interrupt
 * @data:         Argument to pass to cb_handle
 *
 */
int cam_vmrm_gh_irq_wait_for_lend_v2(enum gh_irq_label label,
	enum gh_vm_names name, gh_irq_handle_fn_v2 on_lend, void *data);

/**
 * cam_vmrm_gh_irq_accept()
 *
 * @brief:        Gh interrupt accept api wrapper
 *
 * @label:        vIRQ high-level label
 * @irq:          Linux IRQ# to associate vIRQ with. If don't care, use -1
 * @type          IRQ flags to use when allowing RM to choose the IRQ.
 *                If irq parameter is specified, then type is unused
 *
 */
int cam_vmrm_gh_irq_accept(enum gh_irq_label label, int irq, int type);

/**
 * cam_vmrm_gh_irq_accept_notify()
 *
 * @brief:        Gh interrupt accept notify api wrapper
 *
 * @label:        vIRQ high-level label
 *
 */
int cam_vmrm_gh_irq_accept_notify(enum gh_irq_label label);

/**
 * cam_vmrm_gh_irq_release()
 *
 * @brief:        Gh interrupt release api wrapper
 *
 * @label:        vIRQ high-level label
 *
 */
int cam_vmrm_gh_irq_release(enum gh_irq_label label);

/**
 * cam_vmrm_gh_irq_release_notify()
 *
 * @brief:        Gh interrupt release notify api wrapper
 *
 * @label:        vIRQ high-level label
 *
 */
int cam_vmrm_gh_irq_release_notify(enum gh_irq_label label);

/**
 * cam_vmrm_gh_irq_reclaim()
 *
 * @brief:        Gh interrupt reclaim api wrapper
 *
 * @label:        vIRQ high-level label
 *
 */
int cam_vmrm_gh_irq_reclaim(enum gh_irq_label label);

/**
 * cam_vmrm_gh_mem_register_notifier()
 *
 * @brief:                Gh memory register notifier api wrapper
 *
 * @tag:                  The tag for which the caller would like to receive MEM_SHARED
 *                        and MEM_RELEASED notifications for
 * @handler:              The handler that will be invoked when a MEM_SHARED or MEM_RELEASED
 *                        notification pertaining to @tag arrives at the VM. The handler will
 *                        also be invoked with a pointer to caller specific data, as well as
 *                        the original MEM_SHARED/MEM_RELEASED payload from the resource manager.
 * @data:                 The data that should be passed to @handler when it is invoked.
 * @cookie:               Save the original api return value.
 *
 */
int cam_vmrm_gh_mem_register_notifier(enum gh_mem_notifier_tag tag,
	gh_mem_notifier_handler handler, void *data, void **cookie);

/**
 * cam_vmrm_gh_mem_unregister_notifier()
 *
 * @brief:         Gh memory unregister notifier api wrapper
 *
 * @cookie:        The cookie returned by gh_mem_notifier_register
 *
 */
void cam_vmrm_gh_mem_unregister_notifier(void *cookie);

/**
 * cam_vmrm_gh_mem_lend()
 *
 * @brief:                Gh memory lend api wrapper
 *
 * @mem_type:             The type of memory being lent (i.e. normal or I/O)
 * @flags:                Bitmask of values to influence the behavior of the RM
 *                        when it lends the memory
 * @label:                The label to assign to the memparcel that the RM will create
 * @acl_desc:             Describes the number of ACL entries and VMID and permission
 *                        pairs that the resource manager should consider when lending the memory
 * @sgl_desc:             Describes the number of SG-List entries as well as
 *                        the location of the memory in the IPA space of the owner
 * @mem_attr_desc:        Describes the number of memory attribute entries and the
 *                        memory attribute and VMID pairs that the RM should consider
 *                        when lending the memory
 * @handle:               Pointer to where the memparcel handle should be stored
 *
 */
int cam_vmrm_gh_mem_lend(u8 mem_type, u8 flags, gh_label_t label,
	struct gh_acl_desc *acl_desc, struct gh_sgl_desc *sgl_desc,
	struct gh_mem_attr_desc *mem_attr_desc,
	gh_memparcel_handle_t *handle);

/**
 * cam_vmrm_gh_mem_notify()
 *
 * @brief:         Gh memory notify api wrapper
 *
 * @handle:        The handle of the memparcel for which a notification should be sent out
 * @flags:         Flags to determine if the notification is for notifying that memory
 *                 has been shared to another VM, or that a VM has released memory
 * @mem_info_tag:  A 32-bit value that is attached to the
 *                 MEM_SHARED/MEM_RELEASED/MEM_ACCEPTED notifications to aid in
 *                 distinguishing different resources from one another.
 * @vmid_desc:     A list of VMIDs to notify that memory has been shared with them.
 *                 This parameter should only be non-NULL if other VMs are being
 *                 notified (i.e. it is invalid to specify this parameter when the
.*                 operation is a release notification)
 *
 */
int cam_vmrm_gh_mem_notify(gh_memparcel_handle_t handle, u8 flags,
	gh_label_t mem_info_tag, struct gh_notify_vmid_desc *vmid_desc);

/**
 * cam_vmrm_gh_mem_accept()
 *
 * @brief:                Gh memory accept api wrapper
 *
 * @handle:               The memparcel handle associated with the memory
 * @mem_type:             The type of memory being lent (i.e. normal or I/O)
 * @trans_type:           The type of memory transfer
 * @flags:                Bitmask of values to influence the behavior of the RM
 *                        when it lends the memory
 * @label:                The label to validate against the label maintained by the RM
 * @acl_desc:             Describes the number of ACL entries and VMID and permission
 *                        pairs that the resource manager should validate against for AC
 *                        regarding the memparcel
 * @sgl_desc:             Describes the number of SG-List entries as well as
 *                        where the memory should be mapped in the IPA space of the VM
 *                        denoted by @map_vmid. If this parameter is left NULL, then the
 *                        RM will map the memory at an arbitrary location
 * @mem_attr_desc:        Describes the number of memory attribute entries and the
 *                        memory attribute and VMID pairs that the RM should validate
 *                        against regarding the memparcel.
 * @map_vmid:             The VMID which RM will map the memory for. VMID 0 corresponds
 *                        to mapping the memory for the current VM
 *
 */
int cam_vmrm_gh_mem_accept(gh_memparcel_handle_t handle, u8 mem_type, u8 trans_type,
	u8 flags, gh_label_t label, struct gh_acl_desc *acl_desc, struct gh_sgl_desc *sgl_desc,
	struct gh_mem_attr_desc *mem_attr_desc, u16 map_vmid);

/**
 * cam_vmrm_gh_mem_release()
 *
 * @brief:                Gh memory release api wrapper
 *
 * @handle:               The memparcel handle associated with the memory
 * @flags:                Bitmask of values to influence the behavior of the RM when it unmap
 *                        the memory.
 *
 */
int cam_vmrm_gh_mem_release(gh_memparcel_handle_t handle, u8 flags);

/**
 * cam_vmrm_gh_mem_reclaim()
 *
 * @brief:                Gh memory reclaim api wrapper
 *
 * @handle:               The memparcel handle associated with the memory
 * @flags:                Bitmask of values to influence the behavior of the RM when it unmap
 *                        the memory.
 *
 */
int cam_vmrm_gh_mem_reclaim(gh_memparcel_handle_t handle, u8 flags);

#ifdef CONFIG_SPECTRA_RESOURCE_REQUEST_ENABLE
/**
 * cam_vmrm_gh_resource_register_req_client()
 *
 * @brief:                Gh resource register request client api wrapper
 *
 * @client:               Client
 *
 */
int cam_vmrm_gh_resource_register_req_client(struct gh_resource_client *client);

/**
 * cam_vmrm_gh_resource_unregister_req_client()
 *
 * @brief:                Gh resource unregister request client api wrapper
 *
 * @client:               Client
 *
 */
int cam_vmrm_gh_resource_unregister_req_client(struct gh_resource_client *client);


/**
 * cam_vmrm_gh_resource_register_release_client()
 *
 * @brief:                Gh resource register release client api wrapper
 *
 * @client:               Client
 *
 */
int cam_vmrm_gh_resource_register_release_client(
	struct gh_resource_client *client);


/**
 * cam_vmrm_gh_resource_unregister_release_client()
 *
 * @brief:                Gh resource register release client api wrapper
 *
 * @client:               Client
 *
 */
int cam_vmrm_gh_resource_unregister_release_client(
	struct gh_resource_client *client);

/**
 * cam_vmrm_gh_resource_request()
 *
 * @brief:                Gh resource request api wrapper
 *
 * @target_vmid:          Target vm id
 * @subsys_name:          Subsystem name
 * @req_resource:         Request resource
 * @res_cnt:              Request resource count
 *
 */
int cam_vmrm_gh_resource_request(gh_vmid_t target_vmid, const char *subsys_name,
	struct gh_res_request *req_resource, int res_cnt);

/**
 * cam_vmrm_gh_resource_release()
 *
 * @brief:                Gh resource release api wrapper
 *
 * @target_vmid:          Target vm id
 * @subsys_name:          Subsystem name
 * @release_resource:     Release resource
 * @res_cnt:              Release resource count
 *
 */
int cam_vmrm_gh_resource_release(gh_vmid_t target_vmid, const char *subsys_name,
	struct gh_res_request *release_resource, int res_cnt);
#else
/**
 * cam_vmrm_gh_resource_register_req_cb()
 *
 * @brief:                Gh resource register request api wrapper
 * @subsys_name:          Subsystem name
 * @cb:                   Callback
 */
int cam_vmrm_gh_resource_register_req_cb(
	const char *subsys_name, gh_res_cb cb);

/**
 * cam_vmrm_gh_resource_unregister_req_cb()
 *
 * @brief:                Gh resource unregister request api wrapper
 * @subsys_name:          Subsystem name
 * @cb:                   Callback
 */
int cam_vmrm_gh_resource_unregister_req_cb(
	const char *subsys_name, gh_res_cb cb);

/**
 * cam_vmrm_gh_resource_register_release_cb()
 *
 * @brief:                Gh resource register release api wrapper
 * @subsys_name:          Subsystem name
 * @cb:                   Callback
 */
int cam_vmrm_gh_resource_register_release_cb(
	const char *subsys_name, gh_res_cb cb);

/**
 * cam_vmrm_gh_resource_unregister_release_cb()
 *
 * @brief:                Gh resource register release api wrapper
 * @subsys_name:          Subsystem name
 * @cb:                   Callback
 */
int cam_vmrm_gh_resource_unregister_release_cb(
	const char *subsys_name, gh_res_cb cb);

/**
 * cam_vmrm_gh_resource_request()
 *
 * @brief:                Gh resource request api wrapper
 *
 */
int cam_vmrm_gh_resource_request(void);

/**
 * cam_vmrm_gh_resource_release()
 *
 * @brief:                Gh resource release api wrapper
 *
 */
int cam_vmrm_gh_resource_release(void);
#endif

/**
 * cam_vmrm_ghd_rm_get_vmid()
 *
 * @brief:                Gh translate VM name to vmid api wrapper
 *
 * @vm_name:              VM name to lookup
 * @vmid:                 Out pointer to store found vmid if VM is found
 *
 */
int cam_vmrm_ghd_rm_get_vmid(enum gh_vm_names vm_name, gh_vmid_t *vmid);

/**
 * cam_vmrm_gh_rm_get_vm_name()
 *
 * @brief:                Gh translate vmid to vm name api wrapper
 *
 * @vmid:                 Vmid to lookup
 * @vm_name:              Out pointer to store found VM name if vmid is found
 *
 */
int cam_vmrm_gh_rm_get_vm_name(gh_vmid_t vmid, enum gh_vm_names *vm_name);

#endif /* _CAM_VMRM_GH_WRAPPER_H_ */
