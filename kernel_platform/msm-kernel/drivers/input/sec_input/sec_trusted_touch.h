/* SPDX-License-Identifier: GPL-2.0-only */
/* drivers/input/sec_input/sec_trusted_touch.h
 *
 * Core file for Samsung input device driver for multi device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SEC_TRUSTED_TOUCH_H_
#define _SEC_TRUSTED_TOUCH_H_

#include "sec_input.h"
#if IS_ENABLED(CONFIG_INPUT_SEC_TRUSTED_TOUCH)
#include <linux/gpio.h>
#include <linux/pinctrl/qcom-pinctrl.h>
#include <linux/gunyah/gh_msgq.h>
#include <linux/gunyah/gh_rm_drv.h>
#include <linux/gunyah/gh_irq_lend.h>
#include <linux/gunyah/gh_mem_notifier.h>
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/sort.h>

#define TRUSTED_TOUCH_MEM_LABEL 0x7
enum trusted_touch_mode_config {
	TRUSTED_TOUCH_VM_MODE,
	TRUSTED_TOUCH_MODE_NONE
};

enum trusted_touch_pvm_states {
	TRUSTED_TOUCH_PVM_INIT,
	PVM_I2C_RESOURCE_ACQUIRED,
	PVM_INTERRUPT_DISABLED,
	PVM_IOMEM_LENT,
	PVM_IOMEM_LENT_NOTIFIED,
	PVM_IRQ_LENT,
	PVM_IRQ_LENT_NOTIFIED,
	PVM_IOMEM_RELEASE_NOTIFIED,
	PVM_IRQ_RELEASE_NOTIFIED,
	PVM_ALL_RESOURCES_RELEASE_NOTIFIED,
	PVM_IRQ_RECLAIMED,
	PVM_IOMEM_RECLAIMED,
	PVM_INTERRUPT_ENABLED,
	PVM_I2C_RESOURCE_RELEASED,
	TRUSTED_TOUCH_PVM_STATE_MAX
};

enum trusted_touch_tvm_states {
	TRUSTED_TOUCH_TVM_INIT,
	TVM_IOMEM_LENT_NOTIFIED,
	TVM_IRQ_LENT_NOTIFIED,
	TVM_ALL_RESOURCES_LENT_NOTIFIED,
	TVM_IOMEM_ACCEPTED,
	TVM_I2C_SESSION_ACQUIRED,
	TVM_IRQ_ACCEPTED,
	TVM_INTERRUPT_ENABLED,
	TVM_INTERRUPT_DISABLED,
	TVM_IRQ_RELEASED,
	TVM_I2C_SESSION_RELEASED,
	TVM_IOMEM_RELEASED,
	TRUSTED_TOUCH_TVM_STATE_MAX
};

#define TOUCH_INTR_GPIO_BASE 0xF12E000
#define TOUCH_INTR_GPIO_SIZE 0x1000
#define TOUCH_INTR_GPIO_OFFSET 0x8

#define TRUSTED_TOUCH_EVENT_LEND_FAILURE -1
#define TRUSTED_TOUCH_EVENT_LEND_NOTIFICATION_FAILURE -2
#define TRUSTED_TOUCH_EVENT_ACCEPT_FAILURE -3
#define TRUSTED_TOUCH_EVENT_FUNCTIONAL_FAILURE -4
#define TRUSTED_TOUCH_EVENT_RELEASE_FAILURE -5
#define TRUSTED_TOUCH_EVENT_RECLAIM_FAILURE -6
#define TRUSTED_TOUCH_EVENT_I2C_FAILURE -7
#define TRUSTED_TOUCH_EVENT_NOTIFICATIONS_PENDING 5

struct trusted_touch_vm_info {
	enum gh_irq_label irq_label;
	enum gh_mem_notifier_tag mem_tag;
	enum gh_vm_names vm_name;
	const char *trusted_touch_type;
	u32 hw_irq;
	gh_memparcel_handle_t vm_mem_handle;
	u32 *iomem_bases;
	u32 *iomem_sizes;
	u32 iomem_list_size;
	void *mem_cookie;
	atomic_t vm_state;
};

struct sec_trusted_touch {
	struct device *dev;
	struct trusted_touch_vm_info *vm_info;
	struct mutex clk_io_ctrl_mutex;
	struct mutex transition_lock;
	const char *touch_environment;
	struct completion trusted_touch_powerdown;
	struct clk *core_clk;
	struct clk *iface_clk;
	atomic_t trusted_touch_initialized;
	atomic_t trusted_touch_enabled;
	atomic_t trusted_touch_transition;
	atomic_t trusted_touch_event;
	atomic_t trusted_touch_abort_status;
	atomic_t delayed_vm_probe_pending;
	atomic_t trusted_touch_mode;
	int (*pre_func)(struct device *dev, int mode);
	int (*post_func)(struct device *dev, int mode);
};
int sec_trusted_touch_init(struct device *dev);
#else
static inline int sec_trusted_touch_init(struct device *dev)
{
	return 0;
}
#endif

#endif