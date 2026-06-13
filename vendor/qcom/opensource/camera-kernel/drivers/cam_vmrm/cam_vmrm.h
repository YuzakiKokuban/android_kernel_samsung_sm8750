/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "cam_soc_util.h"
#include "cam_vmrm_gh_wrapper.h"
#include "cam_vmrm_msg_handler.h"
#include "cam_inter_vm_comms.h"
#include "cam_qrtr_comms.h"
#include <dt-bindings/msm-camera.h>

#ifndef _CAM_VMRM_H_
#define _CAM_VMRM_H_

#define CAM_GET_VM_NAME() ((cam_vmrm_intf_get_vmid() == CAM_PVM) ? "PVM" : "TVM")

#define CAM_BUF_SIZE_MAX      32
#define CAM_HEXADECIMAL       16
#define CAM_MHZ               1000000

/* message destination id for hw instance */
#define CAM_HW_ID_BASE          0x3000
#define CAM_HW_ID_CSIPHY0       (CAM_HW_ID_BASE + 0)
#define CAM_HW_ID_CCI0          (CAM_HW_ID_BASE + 0x100)
#define CAM_HW_ID_CPAS          (CAM_HW_ID_BASE + 0x200)
#define CAM_HW_ID_CDM0          (CAM_HW_ID_BASE + 0x300)
#define CAM_HW_ID_SFE0          (CAM_HW_ID_BASE + 0x400)
#define CAM_HW_ID_CSID0         (CAM_HW_ID_BASE + 0x500)
#define CAM_HW_ID_IFE0          (CAM_HW_ID_BASE + 0x600)
#define CAM_HW_ID_ICP           (CAM_HW_ID_BASE + 0x700)
#define CAM_HW_ID_IPE           (CAM_HW_ID_BASE + 0x800)
#define CAM_HW_ID_BPS           (CAM_HW_ID_BASE + 0x900)
#define CAM_HW_ID_OFE           (CAM_HW_ID_BASE + 0xa00)
#define CAM_HW_ID_SENSOR0       (CAM_HW_ID_BASE + 0xb00)
#define CAM_HW_ID_EEPROM0       (CAM_HW_ID_BASE + 0xc00)

/* message destination id for driver node*/
#define CAM_DRIVER_ID_BASE          0xa000
#define CAM_DRIVER_ID_ISP           (CAM_DRIVER_ID_BASE + 0)
#define CAM_DRIVER_ID_ICP           (CAM_DRIVER_ID_BASE + 0x100)
#define CAM_DRIVER_ID_CPAS          (CAM_DRIVER_ID_BASE + 0x200)

/*
 * camera irq label, there is a similar irq label defined in
 * gh side need to map the camera irq label with gh irq label
 * gh use irq label to manager the irq lend and release. Each
 * irq need to define label in dtsi.
 */
#define CAM_IRQ_LABEL_BASE       1
#define CAM_IRQ_LABEL_MAX        50

/*
 * camera io mem notifier tag, there is a similar mem tag defined
 * in gh side need to map the camera mem tag with gh mem tag
 * gh use mem tag to notify lend/release/accept mem. Currently
 * define one mem tag for each hw instance in dtsi.
 */
#define CAM_MEM_TAG_BASE      1
#define CAM_MEM_TAG_MAX       50

/*
 * camera io mem label, gh use camera mem label to lend mem.
 * Currently define one mem label for each hw instance in dtsi.
 */
#define CAM_MEM_LABEL_BASE       1
#define CAM_MEM_LABEL_MAX        50

/* camera memory type */
#define CAM_MEM_TYPE_NORMAL 0
#define CAM_MEM_TYPE_IO     1

/* message receive buffer size */
#define CAM_VMRM_RECV_MSG_BUF_SIZE        256

/* default inter vm messaging timeout*/
#define CAM_VMRM_INTER_VM_MSG_TIMEOUT     500

/* resource request client name*/
#define CAM_RESOURCE_REQ_CLIENT_NAME "cam_vmrm"

/* message callback */
typedef int (*msg_cb_func) (void *cb_data, void *msg, uint32_t size);

/**
 * struct cam_soc_resources - camera soc resources object for the hw
 *
 * @irq_count:                  Interrupt count
 * @irq_name:                   Interrupt name
 * @irq_num:                    Interrupt number
 * @num_mem_block:              Io memory block count
 * @mem_block_name:             Io memory block name
 * @mem_block_addr:             Io memory block base address
 * @mem_block_size:             Io memory block size
 * @gpio_count:                 Gpio count
 * @gpio_num:                   Gpio number
 * @gpio_mem_addr:              Gpio mem address array
 * @gpio_mem_size:              Gpio mem size array
 * @resource_cnt:               Resource counter
 * @mem_label:                  Mem label
 * @mem_tag:                    Mem tag
 * @irq_label:                  Irq label array
 * @valid_mask:                 Valid resource mask, bit map 0/1/2 is mem/irq1/irq2
 * @ready_mask:                 Resource ready mask, when the reource lent or reclaim
 */
struct cam_soc_resources {
	uint32_t   irq_count;
	char       irq_name[CAM_SOC_MAX_IRQ_LINES_PER_DEV][CAM_SOC_MAX_LENGTH_NAME];
	uint32_t   irq_num[CAM_SOC_MAX_IRQ_LINES_PER_DEV];
	uint32_t   num_mem_block;
	char       mem_block_name[CAM_SOC_MAX_BLOCK][CAM_SOC_MAX_LENGTH_NAME];
	uint32_t   mem_block_addr[CAM_SOC_MAX_BLOCK];
	uint32_t   mem_block_size[CAM_SOC_MAX_BLOCK];
	uint32_t   gpio_count;
	uint32_t   gpio_num[CAM_SOC_MAX_GPIO];
	uint32_t   gpio_mem_addr[CAM_SOC_MAX_GPIO];
	uint32_t   gpio_mem_size[CAM_SOC_MAX_GPIO];
	uint32_t   resource_cnt;
	uint32_t   mem_label;
	uint32_t   mem_tag;
	uint32_t   irq_label[CAM_SOC_MAX_IRQ_LINES_PER_DEV];
	uint32_t   valid_mask;
	uint32_t   ready_mask;
};

/**
 * struct cam_hw_instance - camera hw instance
 *
 * @hw_id:                      Hw id
 * @hw_name:                    Hw name
 * @resources:                  Camera soc resources
 * @handle:                     Memory handle
 * @mem_notify_handle:          Memory notify handle
 * @hw_msg_callback:            Hw message callback
 * @hw_msg_callback_data:       Data pass to callback
 * @registered_rr_callback:     Flag for registered request or release resource callback or not
 * @vm_owner                    VM owner
 * @is_using:                   Hw is using or not
 * @wait_response:              Completion info
 * @response_result:            Response result
 * @lend_in_progress            Whether lend is in progress
 * @spin_lock:                  Spin lock
 * @msg_comm_lock:              Message communication lock
 * @ref_count:                  Hw acquired ref count
 * @list:                       Link list entry
 */
struct cam_hw_instance {
	uint32_t                 hw_id;
	char                     hw_name[CAM_SOC_MAX_LENGTH_NAME];
	struct cam_soc_resources resources;
	uint32_t                 handle;
	void                    *mem_notify_handle;
	msg_cb_func              hw_msg_callback;
	void                    *hw_msg_callback_data;
	bool                     registered_rr_callback;
	uint32_t                 vm_owner;
	bool                     is_using;
	struct completion        wait_response;
	int                      response_result;
	atomic_t                 lend_in_progress;
	spinlock_t               spin_lock;
	struct mutex             msg_comm_lock;
	int32_t                  ref_count;
	struct list_head         list;
};

/**
 * struct cam_driver_node -    camera driver node
 *
 * @driver_id:                 Driver id
 * @driver_name:               Driver name
 * @driver_msg_callback:       Driver message callback
 * @driver_msg_callback_data:  Data pass to callback
 * @wait_response:             Completion info
 * @response_result:           Response result
 * @msg_comm_lock:             Message communication lock
 * @list:                      Link list entry
 */
struct cam_driver_node {
	uint32_t          driver_id;
	char              driver_name[CAM_SOC_MAX_LENGTH_NAME];
	msg_cb_func       driver_msg_callback;
	void             *driver_msg_callback_data;
	struct completion wait_response;
	int               response_result;
	struct mutex      msg_comm_lock;
	struct list_head  list;
};

/**
 * struct cam_io_irq_entry - camera io interrupt entry
 *
 * @label:                       Interrupt label for gh using
 * @irq_num:                     Interrupt number
 * @vm_owner                     VM owner
 * @list:                        Link list entry
 */
struct cam_io_irq_entry {
	uint32_t         label;
	uint32_t         irq_num;
	uint32_t         vm_owner;
	struct list_head list;
};

/**
 * struct cam_io_mem_entry - camera io memory entry
 *
 * @base:                        Memory base address
 * @size:                        Memory size
 * @vm_owner                     VM owner
 * @lend_in_progress             Lend in progress
 * @list:                        Link list entry
 */
struct cam_io_mem_entry {
	phys_addr_t      base;
	phys_addr_t      size;
	uint32_t         vm_owner;
	bool             lend_in_progress;
	struct list_head list;
};

/**
 * struct cam_io_res - camera io resource entry
 *
 * @irq:                   Interrupt resource list
 * @mem:                   Io memory resource list
 */
struct cam_io_res {
	struct list_head irq;
	struct list_head mem;
};

/**
 * struct cam_vmrm_intf_dev - camera vm resource manager
 *
 * @hw_instance:              Hw instance registered list
 * @driver_node:              Driver node registered list
 * @io_res:                   Io resource list
 * @is_initialized:           Flag for vm resource manager initialized or not
 * @gh_rr_enable:             Gh request resource enable or not
 * @gh_res_client:            Gh resource client
 * @proxy_voting_enable:      Proxy voting enable mask, support clk/icc proxy voting.
 * @no_register_read_on_bind: Workaround no register read on bind for tvm.
 * @lock:                     Mutex lock
 * @cam_vmid:                 Camera vm id, such as PVM, SVM1 etc
 * @dentry:                   Debugfs entry
 * @vm_handle:                Vm internal communication handle
 * @lend_cnt:                 Lend count
 * @ops_table:                Operations table
 */
struct cam_vmrm_intf_dev {
	struct list_head                  hw_instance;
	struct list_head                  driver_node;
	struct cam_io_res                 io_res;
	bool                              is_initialized;
	bool                              gh_rr_enable;
	struct gh_resource_client        *gh_res_client;
	uint32_t                          proxy_voting_enable;
	bool                              no_register_read_on_bind;
	struct mutex                      lock;
	uint32_t                          cam_vmid;
	struct dentry                    *dentry;
	void                             *vm_handle;
	int                               lend_cnt;
	struct cam_inter_vm_comms_ops    *ops_table;
};

/**
 * cam_vmrm_msg_handle()
 *
 * @brief:        When receive message, handle message in vm resource manager
 *
 * @msg:          Received message address
 * @size:         Receive message size
 * @res:          Inter vm response address
 */
void cam_vmrm_msg_handle(void *msg, size_t size, struct cam_intervm_response *res);

/**
 * cam_vmrm_module_init()
 *
 * @brief:        Vmrm module initialize
 *
 */
int cam_vmrm_module_init(void);

/**
 * cam_vmrm_module_exit()
 *
 * @brief:        Vmrm module exit
 *
 */
void cam_vmrm_module_exit(void);


/**
 * cam_vmrm_get_intf_dev()
 *
 * @brief:        Get vmrm interface device
 *
 */
struct cam_vmrm_intf_dev *cam_vmrm_get_intf_dev(void);

/**
 * cam_register_gh_res_callback()
 *
 * @brief:        Register gh resource callback
 *
 */
int cam_register_gh_res_callback(void);

/**
 * cam_register_gh_mem_callback()
 *
 * @brief:        Register gh mem callback
 *
 */
int cam_register_gh_mem_callback(void);

/**
 * cam_register_gh_irq_callback()
 *
 * @brief:        Register gh irq callback
 *
 */
int cam_register_gh_irq_callback(void);

/**
 * cam_unregister_gh_mem_callback()
 *
 * @brief:        Unregister gh mem callback
 *
 */
void cam_unregister_gh_mem_callback(void);

/**
 * cam_unregister_gh_res_callback()
 *
 * @brief:        Unregister gh res callback
 *
 */
int cam_unregister_gh_res_callback(void);

/**
 * cam_check_hw_instance_available()
 *
 * @brief:        Check hw instance if available
 *
 * @hw_id:        Hw id
 */
struct cam_hw_instance *cam_check_hw_instance_available(uint32_t hw_id);

/**
 * cam_vmrm_intf_get_vmid()
 *
 * @brief:        Get camera vm id
 *
 */
uint32_t cam_vmrm_intf_get_vmid(void);

/**
 * cam_vmrm_resource_req()
 *
 * @brief:        Resource request
 *
 * @hw_pos:        Hw instance
 */
int cam_vmrm_resource_req(struct cam_hw_instance *hw_pos, bool is_req);

/**
 * cam_vmrm_release_resources()
 *
 * @brief:        Release resources
 *
 * @hw_id:        Hw id
 */
int cam_vmrm_release_resources(uint32_t hw_id);

/**
 * cam_hw_instance_lookup()
 *
 * @brief:        Lookup hw instance
 *
 * @hw_id:        Hw id
 * @mem_tag:      Mem tag
 */
struct cam_hw_instance *cam_hw_instance_lookup(uint32_t hw_id, uint32_t mem_tag,
	uint32_t irq_label, uint32_t gpio_num);

/**
 * cam_driver_node_lookup()
 *
 * @brief:        Lookup driver node
 *
 * @driver_id:    Driver node id
 */
struct cam_driver_node *cam_driver_node_lookup(uint32_t driver_id);

/**
 * cam_populate_irq_resource_info()
 *
 * @brief:        Populate irq resource info
 *
 * @hw_pos:       Hw instance
 */
int cam_populate_irq_resource_info(struct cam_hw_instance *hw_pos);

/**
 * cam_populate_mem_resource_info()
 *
 * @brief:        Populate mem resource info
 *
 * @hw_pos:       Hw instance
 */
int cam_populate_mem_resource_info(struct cam_hw_instance *hw_pos);

/**
 * cam_populate_gpio_resource_info()
 *
 * @brief:        Populate gpio resource info
 *
 * @hw_pos:       Hw instance
 */
int cam_populate_gpio_resource_info(struct cam_hw_instance *hw_pos);

/**
 * cam_free_io_resource_info()
 *
 * @brief:        Free io resources
 *
 */
void cam_free_io_resource_info(void);

#endif /* _CAM_VMRM_H_ */
