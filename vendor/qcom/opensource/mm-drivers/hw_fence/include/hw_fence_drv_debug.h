/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __HW_FENCE_DRV_DEBUG
#define __HW_FENCE_DRV_DEBUG

#include "hw_fence_drv_ipc.h"

enum hw_fence_drv_prio {
	HW_FENCE_HIGH = 0x000001,	/* High density debug messages (noisy) */
	HW_FENCE_LOW = 0x000002,	/* Low density debug messages */
	HW_FENCE_INFO = 0x000004,	/* Informational prints */
	HW_FENCE_INIT = 0x00008,	/* Initialization logs */
	HW_FENCE_QUEUE = 0x000010,	/* Queue logs */
	HW_FENCE_LUT = 0x000020,	/* Look-up and algorithm logs */
	HW_FENCE_IRQ = 0x000040,	/* Interrupt-related messages */
	HW_FENCE_LOCK = 0x000080,	/* Lock-related messages */
	HW_FENCE_SSR = 0x0000100,       /* SSR-related messages */
	HW_FENCE_PRINTK = 0x010000,
};

extern u32 msm_hw_fence_debug_level;

#define dprintk(__level, __fmt, ...) \
	do { \
		if (msm_hw_fence_debug_level & __level) \
			if (msm_hw_fence_debug_level & HW_FENCE_PRINTK) \
				pr_err(__fmt, ##__VA_ARGS__); \
	} while (0)


#define HWFNC_ERR(fmt, ...) \
	pr_err("[hwfence_error:%s:%d][%pS] "fmt, __func__, __LINE__, \
	__builtin_return_address(0), ##__VA_ARGS__)

#define HWFNC_ERR_ONCE(fmt, ...) \
	pr_err_once("[hwfence_error:%s:%d][%pS] "fmt, __func__, __LINE__, \
	__builtin_return_address(0), ##__VA_ARGS__)

#define HWFNC_DBG_H(fmt, ...) \
	dprintk(HW_FENCE_HIGH, "[hwfence_dbgh:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_L(fmt, ...) \
	dprintk(HW_FENCE_LOW, "[hwfence_dbgl:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_INFO(fmt, ...) \
	dprintk(HW_FENCE_INFO, "[hwfence_dbgi:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_INIT(fmt, ...) \
	dprintk(HW_FENCE_INIT, "[hwfence_dbg:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_Q(fmt, ...) \
	dprintk(HW_FENCE_QUEUE, "[hwfence_dbgq:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_LUT(fmt, ...) \
	dprintk(HW_FENCE_LUT, "[hwfence_dbglut:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_IRQ(fmt, ...) \
	dprintk(HW_FENCE_IRQ, "[hwfence_dbgirq:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_LOCK(fmt, ...) \
	dprintk(HW_FENCE_LOCK, "[hwfence_dbglock:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_SSR(fmt, ...) \
	dprintk(HW_FENCE_SSR, "[hwfence_dbgssr:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_DBG_DUMP(prio, fmt, ...) \
	dprintk(prio, "[hwfence_dbgd:%s:%d]"fmt, __func__, __LINE__, ##__VA_ARGS__)

#define HWFNC_WARN(fmt, ...) \
	pr_warn("[hwfence_warn:%s:%d][%pS] "fmt, __func__, __LINE__, \
	__builtin_return_address(0), ##__VA_ARGS__)

int hw_fence_debug_debugfs_register(struct hw_fence_driver_data *drv_data);
void hw_fence_debug_dump_fence(enum hw_fence_drv_prio prio, struct msm_hw_fence *hw_fence, u64 hash,
	u32 count);

#if IS_ENABLED(CONFIG_DEBUG_FS)

int process_validation_client_loopback(struct hw_fence_driver_data *drv_data, int client_id);
int hw_fence_debug_wait_val(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, struct dma_fence *fence, u64 hash, u64 mask,
	u64 timeout_ms, u32 *error);

void hw_fence_debug_dump_queues(struct hw_fence_driver_data *drv_data, enum hw_fence_drv_prio prio,
	struct msm_hw_fence_client *hw_fence_client);
void hw_fence_debug_dump_table(enum hw_fence_drv_prio prio, struct hw_fence_driver_data *drv_data);
void hw_fence_debug_dump_events(enum hw_fence_drv_prio prio, struct hw_fence_driver_data *drv_data);

extern const struct file_operations hw_sync_debugfs_fops;

struct hw_fence_out_clients_map {
	int ipc_client_id_vid; /* ipc client virtual id for the hw fence client */
	int ipc_client_id_pid; /* ipc client physical id for the hw fence client */
	int ipc_signal_id; /* ipc signal id for the hw fence client */
};

/* These signals are the ones that the actual clients should be triggering, hw-fence driver
 * does not need to have knowledge of these signals. Adding them here for debugging purposes.
 * Only fence controller and the cliens know these id's, since these
 * are to trigger the ipcc from the 'client hw-core' to the 'hw-fence controller'
 * The index of this struct must match the enum hw_fence_client_id
 */
static const struct hw_fence_out_clients_map
			dbg_out_clients_signal_map_no_dpu[HW_FENCE_CLIENT_ID_VAL6 + 1] = {
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 0},  /* CTRL_LOOPBACK */
	{HW_FENCE_IPC_CLIENT_ID_GPU_VID, HW_FENCE_IPC_CLIENT_ID_GPU_VID, 0},  /* CTX0 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 2},  /* CTL0 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 4},  /* CTL1 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 6},  /* CTL2 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 8},  /* CTL3 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 10}, /* CTL4 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 12}, /* CTL5 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 21}, /* VAL0 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 22}, /* VAL1 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 23}, /* VAL2 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 24}, /* VAL3 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 25}, /* VAL4 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 26}, /* VAL5 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 27}, /* VAL6 */
};
#endif /* CONFIG_DEBUG_FS */

#endif /* __HW_FENCE_DRV_DEBUG */
