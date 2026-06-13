// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/of_platform.h>
#include "hw_fence_drv_priv.h"
#include "hw_fence_drv_utils.h"
#include "hw_fence_drv_ipc.h"
#include "hw_fence_drv_debug.h"

/*
 * Max size of base table with ipc mappings, with one mapping per client type with configurable
 * number of subclients
 */
#define HW_FENCE_IPC_MAP_MAX (HW_FENCE_MAX_STATIC_CLIENTS_INDEX + \
	HW_FENCE_MAX_CLIENT_TYPE_CONFIGURABLE)

/**
 * HW_FENCE_IPCC_MAX_LOOPS:
 * Max number of times HW Fence Driver can read interrupt information
 */
#define HW_FENCE_IPCC_MAX_LOOPS 100

/**
 * struct hw_fence_client_ipc_map - map client id with ipc signal for trigger.
 * @ipc_client_id_virt: virtual ipc client id for the hw-fence client.
 * @ipc_client_id_phys: physical ipc client id for the hw-fence client.
 * @ipc_signal_id: ipc signal id for the hw-fence client.
 * @update_rxq: bool to indicate if client requires rx queue update in general signal case
 *              (e.g. if dma-fence is signaled)
 * @signaled_update_rxq: bool to indicate if client requires rx queue update when registering to
 *                     wait on an already signaled fence
 * @signaled_send_ipc: bool to indicate if client requires ipc interrupt for already signaled fences
 * @txq_update_send_ipc: bool to indicate if client requires ipc interrupt for signaled fences
 */
struct hw_fence_client_ipc_map {
	int ipc_client_id_virt;
	int ipc_client_id_phys;
	int ipc_signal_id;
	bool update_rxq;
	bool signaled_update_rxq;
	bool signaled_send_ipc;
	bool txq_update_send_ipc;
};

/**
 * struct hw_fence_clients_ipc_map - Table makes the 'client to signal' mapping, which is
 *		used by the hw fence driver to trigger ipc signal when hw fence is already
 *		signaled.
 *		This version is for targets that support dpu client id.
 *
 * Note that the index of this struct must match the enum hw_fence_client_id
 */
struct hw_fence_client_ipc_map hw_fence_clients_ipc_map[HW_FENCE_IPC_MAP_MAX] = {
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 1, true, true, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_GPU_VID,  HW_FENCE_IPC_CLIENT_ID_GPU_VID, 0, true, false, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_VID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_VID, 1, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_VID, 2, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_VID, 3, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_VID, 4, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_VID, 5, false, false, true,
		false},
#if IS_ENABLED(CONFIG_DEBUG_FS)
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 21, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 22, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 23, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 24, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 25, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 26, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_VID, 27, true, true, false,
		true},
#else
	{0, 0, 0, false, false, false, false}, /* val0 */
	{0, 0, 0, false, false, false, false}, /* val1 */
	{0, 0, 0, false, false, false, false}, /* val2 */
	{0, 0, 0, false, false, false, false}, /* val3 */
	{0, 0, 0, false, false, false, false}, /* val4 */
	{0, 0, 0, false, false, false, false}, /* val5 */
	{0, 0, 0, false, false, false, false}, /* val6 */
#endif /* CONFIG_DEBUG_FS */
	{HW_FENCE_IPC_CLIENT_ID_IPE_VID, HW_FENCE_IPC_CLIENT_ID_IPE_VID, 0, true, true, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_VPU_VID, HW_FENCE_IPC_CLIENT_ID_VPU_VID, 0, true, true, true,
		false},
};

/**
 * struct hw_fence_clients_ipc_map_v2 - Table makes the 'client to signal' mapping, which is
 *		used by the hw fence driver to trigger ipc signal when hw fence is already
 *		signaled.
 *		This version is for targets that support dpu client id and IPC v2.
 *
 * Note that the index of this struct must match the enum hw_fence_client_id for clients ids less
 * than HW_FENCE_MAX_STATIC_CLIENTS_INDEX.
 * For clients with configurable sub-clients, the index of this struct matches
 * HW_FENCE_MAX_STATIC_CLIENTS_INDEX + (client type index - HW_FENCE_MAX_CLIENT_TYPE_STATIC).
 */
struct hw_fence_client_ipc_map hw_fence_clients_ipc_map_v2[HW_FENCE_IPC_MAP_MAX] = {
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 1, true, true, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_GPU_VID,  HW_FENCE_IPC_CLIENT_ID_GPU_PID, 0, true, false, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID, 1, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID, 2, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID, 3, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID, 4, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID, 5, false, false, true,
		false},
#if IS_ENABLED(CONFIG_DEBUG_FS)
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 21, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 22, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 23, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 24, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 25, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 26, true, true, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 27, true, true, false,
		true},
#else
	{0, 0, 0, false, false, false, false}, /* val0 */
	{0, 0, 0, false, false, false, false}, /* val1 */
	{0, 0, 0, false, false, false, false}, /* val2 */
	{0, 0, 0, false, false, false, false}, /* val3 */
	{0, 0, 0, false, false, false, false}, /* val4 */
	{0, 0, 0, false, false, false, false}, /* val5 */
	{0, 0, 0, false, false, false, false}, /* val6 */
#endif /* CONFIG_DEBUG_FS */
	{HW_FENCE_IPC_CLIENT_ID_IPE_VID, HW_FENCE_IPC_CLIENT_ID_IPE_PID, 0, true, true, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_VPU_VID, HW_FENCE_IPC_CLIENT_ID_VPU_PID, 0, true, true, true,
		false},
	{0, 0, 0, false, false, false, false}, /* ipa */
	{HW_FENCE_IPC_CLIENT_ID_IFE0_VID, HW_FENCE_IPC_CLIENT_ID_IFE0_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE1_VID, HW_FENCE_IPC_CLIENT_ID_IFE1_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE2_VID, HW_FENCE_IPC_CLIENT_ID_IFE2_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE3_VID, HW_FENCE_IPC_CLIENT_ID_IFE3_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE4_VID, HW_FENCE_IPC_CLIENT_ID_IFE4_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE5_VID, HW_FENCE_IPC_CLIENT_ID_IFE5_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE6_VID, HW_FENCE_IPC_CLIENT_ID_IFE6_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE7_VID, HW_FENCE_IPC_CLIENT_ID_IFE7_PID, 0, false, false, true,
		false},
};

/**
 * struct hw_fence_clients_ipc_map_sun - Table makes the 'client to signal' mapping, which is
 *		used by the hw fence driver to trigger ipc signal when hw fence is already
 *		signaled.
 *		This version is for sun target.
 *
 * Note that the index of this struct must match the enum hw_fence_client_id for clients ids less
 * than HW_FENCE_MAX_STATIC_CLIENTS_INDEX.
 * For clients with configurable sub-clients, the index of this struct matches
 * HW_FENCE_MAX_STATIC_CLIENTS_INDEX + (client type index - HW_FENCE_MAX_CLIENT_TYPE_STATIC).
 */
struct hw_fence_client_ipc_map hw_fence_clients_ipc_map_sun[HW_FENCE_IPC_MAP_MAX] = {
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 0, true, true, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_GPU_VID,  HW_FENCE_IPC_CLIENT_ID_GPU_PID, 0, true, false, false,
		true},
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_SUN, 0, false, false, true,
		false}, /* ctl0 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_SUN, 1, false, false, true,
		false}, /* ctl1 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_SUN, 2, false, false, true,
		false}, /* ctl2 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_SUN, 3, false, false, true,
		false}, /* ctl3 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_SUN, 4, false, false, true,
		false}, /* ctl4 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_SUN, 5, false, false, true,
		false}, /* ctl5 */
#if IS_ENABLED(CONFIG_DEBUG_FS)
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 21, true, true, true,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 22, true, true, true,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 23, true, true, true,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 24, true, true, true,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 25, true, true, true,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 26, true, true, true,
		true},
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID, 27, true, true, true,
		true},
#else
	{0, 0, 0, false, false, false, false}, /* val0 */
	{0, 0, 0, false, false, false, false}, /* val1 */
	{0, 0, 0, false, false, false, false}, /* val2 */
	{0, 0, 0, false, false, false, false}, /* val3 */
	{0, 0, 0, false, false, false, false}, /* val4 */
	{0, 0, 0, false, false, false, false}, /* val5 */
	{0, 0, 0, false, false, false, false}, /* val6 */
#endif /* CONFIG_DEBUG_FS */
	{HW_FENCE_IPC_CLIENT_ID_IPE_VID, HW_FENCE_IPC_CLIENT_ID_IPE_PID_SUN, 0, true, true, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_VPU_VID, HW_FENCE_IPC_CLIENT_ID_VPU_PID, 0, true, true, true,
		false},
	{0, 0, 0, false, false, false, false}, /* ipa */
	{HW_FENCE_IPC_CLIENT_ID_IFE0_VID, HW_FENCE_IPC_CLIENT_ID_IFE0_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE1_VID, HW_FENCE_IPC_CLIENT_ID_IFE1_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE2_VID, HW_FENCE_IPC_CLIENT_ID_IFE2_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE3_VID, HW_FENCE_IPC_CLIENT_ID_IFE3_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE4_VID, HW_FENCE_IPC_CLIENT_ID_IFE4_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE5_VID, HW_FENCE_IPC_CLIENT_ID_IFE5_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE6_VID, HW_FENCE_IPC_CLIENT_ID_IFE6_PID, 0, false, false, true,
		false},
	{HW_FENCE_IPC_CLIENT_ID_IFE7_VID, HW_FENCE_IPC_CLIENT_ID_IFE7_PID, 0, false, false, true,
		false},
};

/**
 * struct hw_fence_clients_ipc_map_niobe - Table makes the 'client to signal' mapping, which is
 *		used by the hw fence driver to trigger ipc signal when hw fence is already
 *		signaled.
 *		This version is for niobe target.
 *
 * Note that the index of this struct must match the enum hw_fence_client_id for clients ids less
 * than HW_FENCE_MAX_STATIC_CLIENTS_INDEX.
 * For clients with configurable sub-clients, the index of this struct matches
 * HW_FENCE_MAX_STATIC_CLIENTS_INDEX + (client type index - HW_FENCE_MAX_CLIENT_TYPE_STATIC).
 */
struct hw_fence_client_ipc_map hw_fence_clients_ipc_map_niobe[HW_FENCE_IPC_MAP_MAX] = {
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE, 0, true, true,
		true, false}, /* ctrlq */
	{HW_FENCE_IPC_CLIENT_ID_GPU_VID,  HW_FENCE_IPC_CLIENT_ID_GPU_PID_NIOBE, 0, true, false,
		false, true}, /* gfx */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_NIOBE, 0, false, false,
		true, false}, /* ctl0 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_NIOBE, 1, false, false,
		true, false}, /* ctl1 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_NIOBE, 2, false, false,
		true, false}, /* ctl2 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_NIOBE, 3, false, false,
		true, false}, /* ctl3 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_NIOBE, 4, false, false,
		true, false}, /* ctl4 */
	{HW_FENCE_IPC_CLIENT_ID_DPU_VID,  HW_FENCE_IPC_CLIENT_ID_DPU_PID_NIOBE, 5, false, false,
		true, false}, /* ctl5 */
#if IS_ENABLED(CONFIG_DEBUG_FS)
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE, 21, true, true,
		true, true}, /* val0 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE, 22, true, true,
		true, true}, /* val1 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE, 23, true, true,
		true, true}, /* val2 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE, 24, true, true,
		true, true}, /* val3 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE, 25, true, true,
		true, true}, /* val4 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE, 26, true, true,
		true, true}, /* val5 */
	{HW_FENCE_IPC_CLIENT_ID_APPS_VID, HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE, 27, true, true,
		true, true}, /* val6 */
#else
	{0, 0, 0, false, false, false, false}, /* val0 */
	{0, 0, 0, false, false, false, false}, /* val1 */
	{0, 0, 0, false, false, false, false}, /* val2 */
	{0, 0, 0, false, false, false, false}, /* val3 */
	{0, 0, 0, false, false, false, false}, /* val4 */
	{0, 0, 0, false, false, false, false}, /* val5 */
	{0, 0, 0, false, false, false, false}, /* val6 */
#endif /* CONFIG_DEBUG_FS */
	{HW_FENCE_IPC_CLIENT_ID_IPE_VID, HW_FENCE_IPC_CLIENT_ID_IPE_PID_NIOBE, 0, true, true, true,
		false}, /* ipe */
	{HW_FENCE_IPC_CLIENT_ID_VPU_VID, HW_FENCE_IPC_CLIENT_ID_VPU_PID_NIOBE, 0, true, true, true,
		false}, /* vpu */
	{HW_FENCE_IPC_CLIENT_ID_IPA_VID, HW_FENCE_IPC_CLIENT_ID_IPA_PID_NIOBE, 0, true, true, true,
		false}, /* ipa */
	{HW_FENCE_IPC_CLIENT_ID_IFE0_VID, HW_FENCE_IPC_CLIENT_ID_IFE0_PID_NIOBE, 0, false, false,
		true, false}, /* ife0 */
	{HW_FENCE_IPC_CLIENT_ID_IFE1_VID, HW_FENCE_IPC_CLIENT_ID_IFE1_PID_NIOBE, 0, false, false,
		true, false}, /* ife1 */
	{HW_FENCE_IPC_CLIENT_ID_IFE2_VID, HW_FENCE_IPC_CLIENT_ID_IFE2_PID_NIOBE, 0, false, false,
		true, false}, /* ife2 */
	{HW_FENCE_IPC_CLIENT_ID_IFE3_VID, HW_FENCE_IPC_CLIENT_ID_IFE3_PID_NIOBE, 0, false, false,
		true, false}, /* ife3 */
	{HW_FENCE_IPC_CLIENT_ID_IFE4_VID, HW_FENCE_IPC_CLIENT_ID_IFE4_PID_NIOBE, 0, false, false,
		true, false}, /* ife4 */
	{HW_FENCE_IPC_CLIENT_ID_IFE5_VID, HW_FENCE_IPC_CLIENT_ID_IFE5_PID_NIOBE, 0, false, false,
		true, false}, /* ife5 */
	{HW_FENCE_IPC_CLIENT_ID_IFE6_VID, HW_FENCE_IPC_CLIENT_ID_IFE6_PID_NIOBE, 0, false, false,
		true, false}, /* ife6 */
	{HW_FENCE_IPC_CLIENT_ID_IFE7_VID, HW_FENCE_IPC_CLIENT_ID_IFE7_PID_NIOBE, 0, false, false,
		true, false}, /* ife7 */
	{HW_FENCE_IPC_CLIENT_ID_IFE8_VID, HW_FENCE_IPC_CLIENT_ID_IFE8_PID_NIOBE, 0, false, false,
		true, false}, /* ife8 */
	{HW_FENCE_IPC_CLIENT_ID_IFE9_VID, HW_FENCE_IPC_CLIENT_ID_IFE9_PID_NIOBE, 0, false, false,
		true, false}, /* ife9 */
	{HW_FENCE_IPC_CLIENT_ID_IFE10_VID, HW_FENCE_IPC_CLIENT_ID_IFE10_PID_NIOBE, 0, false, false,
		true, false}, /* ife10 */
	{HW_FENCE_IPC_CLIENT_ID_IFE11_VID, HW_FENCE_IPC_CLIENT_ID_IFE11_PID_NIOBE, 0, false, false,
		true, false}, /* ife11 */

};

int hw_fence_ipcc_get_client_virt_id(struct hw_fence_driver_data *drv_data, u32 client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num)
		return -EINVAL;

	return drv_data->ipc_clients_table[client_id].ipc_client_id_virt;
}

int hw_fence_ipcc_get_client_phys_id(struct hw_fence_driver_data *drv_data, u32 client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num)
		return -EINVAL;

	return drv_data->ipc_clients_table[client_id].ipc_client_id_phys;
}

int hw_fence_ipcc_get_signal_id(struct hw_fence_driver_data *drv_data, u32 client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num)
		return -EINVAL;

	return drv_data->ipc_clients_table[client_id].ipc_signal_id;
}

bool hw_fence_ipcc_needs_rxq_update(struct hw_fence_driver_data *drv_data, int client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num)
		return false;

	return drv_data->ipc_clients_table[client_id].update_rxq;
}

bool hw_fence_ipcc_signaled_needs_rxq_update(struct hw_fence_driver_data *drv_data,
	int client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num)
		return false;

	return drv_data->ipc_clients_table[client_id].signaled_update_rxq;
}

bool hw_fence_ipcc_signaled_needs_ipc_irq(struct hw_fence_driver_data *drv_data, int client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num)
		return false;

	return drv_data->ipc_clients_table[client_id].signaled_send_ipc;
}

bool hw_fence_ipcc_txq_update_needs_ipc_irq(struct hw_fence_driver_data *drv_data, int client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num)
		return false;

	return drv_data->ipc_clients_table[client_id].txq_update_send_ipc;
}

/**
 * _get_ipc_phys_client_name() - Returns ipc client name from its physical id, used for debugging.
 */
static inline char *_get_ipc_phys_client_name(u32 client_id)
{
	switch (client_id) {
	case HW_FENCE_IPC_CLIENT_ID_APPS_PID:
		return "APPS_PID";
	case HW_FENCE_IPC_CLIENT_ID_GPU_PID:
		return "GPU_PID";
	case HW_FENCE_IPC_CLIENT_ID_DPU_PID:
		return "DPU_PID";
	case HW_FENCE_IPC_CLIENT_ID_IPE_PID:
		return "IPE_PID";
	case HW_FENCE_IPC_CLIENT_ID_VPU_PID:
		return "VPU_PID";
	case HW_FENCE_IPC_CLIENT_ID_IFE0_PID:
		return "IFE0_PID";
	case HW_FENCE_IPC_CLIENT_ID_IFE1_PID:
		return "IFE1_PID";
	case HW_FENCE_IPC_CLIENT_ID_IFE2_PID:
		return "IFE2_PID";
	case HW_FENCE_IPC_CLIENT_ID_IFE3_PID:
		return "IFE3_PID";
	case HW_FENCE_IPC_CLIENT_ID_IFE4_PID:
		return "IFE4_PID";
	case HW_FENCE_IPC_CLIENT_ID_IFE5_PID:
		return "IFE5_PID";
	case HW_FENCE_IPC_CLIENT_ID_IFE6_PID:
		return "IFE6_PID";
	case HW_FENCE_IPC_CLIENT_ID_IFE7_PID:
		return "IFE7_PID";
	}

	return "UNKNOWN_PID";
}

/**
 * _get_ipc_virt_client_name() - Returns ipc client name from its virtual id, used for debugging.
 */
static inline char *_get_ipc_virt_client_name(u32 client_id)
{
	switch (client_id) {
	case HW_FENCE_IPC_CLIENT_ID_APPS_VID:
		return "APPS_VID";
	case HW_FENCE_IPC_CLIENT_ID_GPU_VID:
		return "GPU_VID";
	case HW_FENCE_IPC_CLIENT_ID_DPU_VID:
		return "DPU_VID";
	case HW_FENCE_IPC_CLIENT_ID_IPE_VID:
		return "IPE_VID";
	case HW_FENCE_IPC_CLIENT_ID_VPU_VID:
		return "VPU_VID";
	case HW_FENCE_IPC_CLIENT_ID_IFE0_VID:
		return "IFE0_VID";
	case HW_FENCE_IPC_CLIENT_ID_IFE1_VID:
		return "IFE1_VID";
	case HW_FENCE_IPC_CLIENT_ID_IFE2_VID:
		return "IFE2_VID";
	case HW_FENCE_IPC_CLIENT_ID_IFE3_VID:
		return "IFE3_VID";
	case HW_FENCE_IPC_CLIENT_ID_IFE4_VID:
		return "IFE4_VID";
	case HW_FENCE_IPC_CLIENT_ID_IFE5_VID:
		return "IFE5_VID";
	case HW_FENCE_IPC_CLIENT_ID_IFE6_VID:
		return "IFE6_VID";
	case HW_FENCE_IPC_CLIENT_ID_IFE7_VID:
		return "IFE7_VID";
	}

	return "UNKNOWN_VID";
}

void hw_fence_ipcc_trigger_signal(struct hw_fence_driver_data *drv_data,
	u32 tx_client_pid, u32 rx_client_vid, u32 signal_id)
{
	void __iomem *ptr;
	u32 val;

	/* Send signal */
	ptr = IPC_PROTOCOLp_CLIENTc_SEND(drv_data->ipcc_io_mem, drv_data->protocol_id,
		tx_client_pid);
	val = (rx_client_vid << 16) | signal_id;

	HWFNC_DBG_IRQ("Sending ipcc from %s (%d) to %s (%d) signal_id:%d [wr:0x%x to off:0x%pK]\n",
		_get_ipc_phys_client_name(tx_client_pid), tx_client_pid,
		_get_ipc_virt_client_name(rx_client_vid), rx_client_vid,
		signal_id, val, ptr);
	HWFNC_DBG_H("Write:0x%x to RegOffset:0x%pK\n", val, ptr);
	writel_relaxed(val, ptr);

	/* Make sure value is written */
	wmb();
}

static int _hw_fence_ipcc_init_map_with_configurable_clients(struct hw_fence_driver_data *drv_data,
	struct hw_fence_client_ipc_map *base_table)
{
	int i, j, map_idx;
	size_t size;

	size = drv_data->clients_num * sizeof(struct hw_fence_client_ipc_map);
	drv_data->ipc_clients_table = kzalloc(size, GFP_KERNEL);

	if (!drv_data->ipc_clients_table)
		return -ENOMEM;

	/* copy mappings for static hw fence clients */
	size = HW_FENCE_MAX_STATIC_CLIENTS_INDEX * sizeof(struct hw_fence_client_ipc_map);
	memcpy(drv_data->ipc_clients_table, base_table, size);

	/* initialize mappings for ipc clients with configurable number of hw fence clients */
	map_idx = HW_FENCE_MAX_STATIC_CLIENTS_INDEX;
	for (i = 0; i < HW_FENCE_MAX_CLIENT_TYPE_CONFIGURABLE; i++) {
		int client_type = HW_FENCE_MAX_CLIENT_TYPE_STATIC + i;
		int clients_num = drv_data->hw_fence_client_types[client_type].clients_num;

		for (j = 0; j < clients_num; j++) {
			/* this should never happen if drv_data->clients_num is correct */
			if (map_idx >= drv_data->clients_num) {
				HWFNC_ERR("%s clients_num:%d exceeds drv_data->clients_num:%u\n",
					drv_data->hw_fence_client_types[client_type].name,
					clients_num, drv_data->clients_num);
				return -EINVAL;
			}
			drv_data->ipc_clients_table[map_idx] =
				base_table[HW_FENCE_MAX_STATIC_CLIENTS_INDEX + i];
			drv_data->ipc_clients_table[map_idx].ipc_signal_id = j;
			map_idx++;
		}
	}

	return 0;
}

/**
 * _hw_fence_ipcc_hwrev_init() - Initializes internal driver struct with corresponding ipcc data,
 *		according to the ipcc hw revision.
 * @drv_data: driver data.
 * @hwrev: ipcc hw revision.
 */
static int _hw_fence_ipcc_hwrev_init(struct hw_fence_driver_data *drv_data, u32 hwrev)
{
	int ret = 0;

	switch (hwrev) {
	case HW_FENCE_IPCC_HW_REV_170:
		drv_data->ipcc_client_vid = HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_client_pid = HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_fctl_vid = HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_fctl_pid = HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->protocol_id = HW_FENCE_IPC_COMPUTE_L1_PROTOCOL_ID_KALAMA;
		drv_data->ipc_clients_table = hw_fence_clients_ipc_map;
		HWFNC_DBG_INIT("ipcc protocol_id: Kalama\n");
		break;
	case HW_FENCE_IPCC_HW_REV_203:
		drv_data->ipcc_client_vid = HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_client_pid = HW_FENCE_IPC_CLIENT_ID_APPS_PID;
		drv_data->ipcc_fctl_vid = HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_fctl_pid = HW_FENCE_IPC_CLIENT_ID_APPS_PID;
		drv_data->protocol_id = HW_FENCE_IPC_FENCE_PROTOCOL_ID_PINEAPPLE; /* Fence */
		ret = _hw_fence_ipcc_init_map_with_configurable_clients(drv_data,
			hw_fence_clients_ipc_map_v2);
		HWFNC_DBG_INIT("ipcc protocol_id: Pineapple\n");
		break;
	case HW_FENCE_IPCC_HW_REV_2A2:
		drv_data->ipcc_client_vid = HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_client_pid = HW_FENCE_IPC_CLIENT_ID_APPS_PID;
		drv_data->ipcc_fctl_vid = drv_data->has_soccp ? HW_FENCE_IPC_CLIENT_ID_SOCCP_VID :
			HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_fctl_pid = drv_data->has_soccp ? HW_FENCE_IPC_CLIENT_ID_SOCCP_PID :
			HW_FENCE_IPC_CLIENT_ID_APPS_PID;
		drv_data->protocol_id = HW_FENCE_IPC_FENCE_PROTOCOL_ID_SUN; /* Fence */
		ret = _hw_fence_ipcc_init_map_with_configurable_clients(drv_data,
			hw_fence_clients_ipc_map_sun);
		HWFNC_DBG_INIT("ipcc protocol_id: Sun\n");
		break;
	case HW_FENCE_IPCC_HW_REV_2B4:
		drv_data->ipcc_client_vid = HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_client_pid = HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE;
		drv_data->ipcc_fctl_vid = drv_data->has_soccp ? HW_FENCE_IPC_CLIENT_ID_SOCCP_VID :
			HW_FENCE_IPC_CLIENT_ID_APPS_VID;
		drv_data->ipcc_fctl_pid = drv_data->has_soccp ?
			HW_FENCE_IPC_CLIENT_ID_SOCCP_PID_NIOBE :
			HW_FENCE_IPC_CLIENT_ID_APPS_PID_NIOBE;
		drv_data->protocol_id = HW_FENCE_IPC_FENCE_PROTOCOL_ID_NIOBE; /* Fence */
		ret = _hw_fence_ipcc_init_map_with_configurable_clients(drv_data,
			hw_fence_clients_ipc_map_niobe);
		HWFNC_DBG_INIT("ipcc protocol_id: Niobe\n");
		break;
	default:
		HWFNC_ERR("unrecognized ipcc hw-rev:0x%x\n", hwrev);
		return -1;
	}

	return ret;
}

static int _enable_client_signal_pair(struct hw_fence_driver_data *drv_data,
	u32 rx_client_id_phys, u32 tx_client_id_vid, u32 signal_id)
{
	void __iomem *ptr;
	u32 val;

	if (!drv_data || !drv_data->ipcc_io_mem || !drv_data->protocol_id) {
		HWFNC_ERR("invalid drv_data:0x%pK ipcc_io_mem:0x%pK protocol:%d\n",
			drv_data, drv_data ? drv_data->ipcc_io_mem : NULL,
			drv_data ? drv_data->protocol_id : -1);
		return -EINVAL;
	}

	val = ((tx_client_id_vid) << 16) | ((signal_id) & 0xFFFF);
	ptr = IPC_PROTOCOLp_CLIENTc_RECV_SIGNAL_ENABLE(drv_data->ipcc_io_mem, drv_data->protocol_id,
		rx_client_id_phys);
	HWFNC_DBG_H("Write:0x%x to RegOffset:0x%pK\n", val, ptr);
	writel_relaxed(val, ptr);

	return 0;
}

int hw_fence_ipcc_enable_signaling(struct hw_fence_driver_data *drv_data)
{
	u32 val;
	int ret;

	HWFNC_DBG_H("enable ipc +\n");

	ret = of_property_read_u32(drv_data->dev->of_node, "qcom,hw-fence-ipc-ver", &val);
	if (ret || !val) {
		HWFNC_ERR("missing hw fences ipc-ver entry or invalid ret:%d val:%d\n", ret, val);
		return -EINVAL;
	}

	if (_hw_fence_ipcc_hwrev_init(drv_data, val)) {
		HWFNC_ERR("ipcc protocol id not supported\n");
		return -EINVAL;
	}

	/* Enable protocol for ctrl queue */
	hw_fence_ipcc_enable_protocol(drv_data, 0);

	/* Enable Client-Signal pairs from FCTL (SOCCP or APSS(NS)) to APPS(NS) (0x8) */
	ret = _enable_client_signal_pair(drv_data, drv_data->ipcc_client_pid,
		drv_data->ipcc_fctl_vid, 0);

	HWFNC_DBG_H("enable ipc -\n");

	return 0;
}

int hw_fence_ipcc_enable_protocol(struct hw_fence_driver_data *drv_data, u32 client_id)
{
	void __iomem *ptr;
	u32 val;

	if (!drv_data || !drv_data->protocol_id || !drv_data->ipc_clients_table ||
		client_id >= drv_data->clients_num) {
		HWFNC_ERR("drv_data:0x%pK protocol:%d ipc_table:0x%pK client_id:%u max:%u\n",
			drv_data, drv_data ? drv_data->protocol_id : -1,
			drv_data ? drv_data->ipc_clients_table : NULL, client_id,
			drv_data ? drv_data->clients_num : -1);
		return -EINVAL;
	}

	/* Sets bit(1) to clear when RECV_ID is read */
	val = 0x00000001;
	ptr = IPC_PROTOCOLp_CLIENTc_CONFIG(drv_data->ipcc_io_mem, drv_data->protocol_id,
		drv_data->ipc_clients_table[client_id].ipc_client_id_phys);
	HWFNC_DBG_H("Write:0x%x to RegOffset:0x%llx\n", val, (u64)ptr);
	writel_relaxed(val, ptr);

	return 0;
}

int hw_fence_ipcc_enable_client_signal_pairs(struct hw_fence_driver_data *drv_data,
	u32 start_client)
{
	struct hw_fence_client_ipc_map *hw_fence_client;
	int i, ipc_client_vid;

	HWFNC_DBG_H("enable ipc for client signal pairs +\n");

	if (!drv_data || !drv_data->protocol_id || !drv_data->ipc_clients_table ||
		start_client >= drv_data->clients_num) {
		HWFNC_ERR("drv_data:0x%pK protocol:%d ipc_table:0x%pK start_client:%u max:%u\n",
			drv_data, drv_data ? drv_data->protocol_id : -1,
			drv_data ? drv_data->ipc_clients_table : NULL, start_client,
			drv_data ? drv_data->clients_num : -1);
		return -EINVAL;
	}
	ipc_client_vid = drv_data->ipc_clients_table[start_client].ipc_client_id_virt;

	HWFNC_DBG_H("ipcc_io_mem:0x%llx\n", (u64)drv_data->ipcc_io_mem);

	HWFNC_DBG_H("Initialize %s ipc signals\n", _get_ipc_virt_client_name(ipc_client_vid));
	/* Enable Client-Signal pairs from Client to APPS(NS) (8) */
	for (i = start_client; i < drv_data->clients_num; i++) {
		hw_fence_client = &drv_data->ipc_clients_table[i];

		/*
		 * Stop after enabling signals for all clients with the same ipcc client id as the
		 * given client.
		 */
		if (hw_fence_client->ipc_client_id_virt != ipc_client_vid)
			break;

		/* Enable signals for given client */
		HWFNC_DBG_H("%s client:%d vid:%d pid:%d signal:%d has_soccp:%d\n",
			_get_ipc_virt_client_name(ipc_client_vid), i,
			hw_fence_client->ipc_client_id_virt, hw_fence_client->ipc_client_id_phys,
			hw_fence_client->ipc_signal_id, drv_data->has_soccp);

		/* Enable input signal from driver to client */
		if (drv_data->has_soccp || ipc_client_vid != drv_data->ipcc_client_vid)
			_enable_client_signal_pair(drv_data, hw_fence_client->ipc_client_id_phys,
				drv_data->ipcc_client_vid, hw_fence_client->ipc_signal_id);

		/* If fctl separate from driver, enable separate input fctl-signal for client */
		if (drv_data->ipcc_client_vid != drv_data->ipcc_fctl_vid)
			_enable_client_signal_pair(drv_data, hw_fence_client->ipc_client_id_phys,
				drv_data->ipcc_fctl_vid, hw_fence_client->ipc_signal_id);
	}

	HWFNC_DBG_H("enable %s ipc for start:%d end:%d -\n",
		_get_ipc_virt_client_name(ipc_client_vid), start_client, i);

	return 0;
}

static bool _is_invalid_signaling_client(struct hw_fence_driver_data *drv_data, u32 client_id)
{
#if IS_ENABLED(CONFIG_DEBUG_FS)
	return client_id != drv_data->ipcc_fctl_vid && client_id != drv_data->ipcc_client_vid;
#else
	return client_id != drv_data->ipcc_fctl_vid;
#endif
}

u64 hw_fence_ipcc_get_signaled_clients_mask(struct hw_fence_driver_data *drv_data)
{
	u32 client_id, signal_id, reg_val;
	u64 mask = 0;
	int i;

	if (!drv_data || !drv_data->protocol_id || !drv_data->ipcc_client_pid ||
			!drv_data->ipcc_fctl_vid || !drv_data->has_soccp) {
		HWFNC_ERR("invalid drv_data:0x%pK protocol:%d drv_pid:%d fctl_vid:%d\n",
			drv_data, drv_data ? drv_data->protocol_id : -1,
			drv_data ? drv_data->ipcc_client_pid : -1,
			drv_data ? drv_data->ipcc_fctl_vid : -1);
		return -1;
	}

	/* read recv_id until done processing all clients signals */
	for (i = 0; i < HW_FENCE_IPCC_MAX_LOOPS; i++) {
		mb(); /* make sure memory is updated */
		reg_val = readl_relaxed(IPC_PROTOCOLp_CLIENTc_RECV_ID(drv_data->ipcc_io_mem,
			drv_data->protocol_id, drv_data->ipcc_client_pid));

		/* finished reading clients */
		if (reg_val == HW_FENCE_IPC_RECV_ID_NONE)
			return mask;

		client_id = (reg_val >> 16) & 0xFFFF;
		signal_id = reg_val & 0xFFFF;
		HWFNC_DBG_IRQ("read recv_id value:0x%x client:%u signal:%u\n", reg_val, client_id,
			signal_id);

		if (_is_invalid_signaling_client(drv_data, client_id)) {
			HWFNC_ERR("Received client:%u signal:%u expected client:%u\n",
				client_id, signal_id, drv_data->ipcc_fctl_vid);
			continue;
		}

#if IS_ENABLED(CONFIG_DEBUG_FS)
		/* received signals from SOCCP for validation clients */
		if (signal_id >= hw_fence_ipcc_get_signal_id(drv_data, HW_FENCE_CLIENT_ID_VAL0)
				&& signal_id <= hw_fence_ipcc_get_signal_id(drv_data,
				HW_FENCE_CLIENT_ID_VAL6))
			signal_id = signal_id - hw_fence_ipcc_get_signal_id(drv_data,
				HW_FENCE_CLIENT_ID_VAL0) + HW_FENCE_CLIENT_ID_VAL0;
#endif /* CONFIG_DEBUG_FS*/

		mask |= BIT(signal_id);
	}

	HWFNC_ERR("irq_handler has too many loops i=%d max:%d\n", i, HW_FENCE_IPCC_MAX_LOOPS);

	return mask;
}
