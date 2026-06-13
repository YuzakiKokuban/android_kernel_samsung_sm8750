// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#if (KERNEL_VERSION(6, 1, 25) <= LINUX_VERSION_CODE)
#include <linux/remoteproc/qcom_rproc.h>
#endif
#include <linux/kthread.h>

#include "hw_fence_drv_priv.h"
#include "hw_fence_drv_utils.h"
#include "hw_fence_drv_debug.h"
#include "hw_fence_drv_ipc.h"
#include "hw_fence_drv_fence.h"

struct hw_fence_driver_data *hw_fence_drv_data;
#if IS_ENABLED(CONFIG_QTI_ENABLE_HW_FENCE_DEFAULT)
bool hw_fence_driver_enable = true;
#else
bool hw_fence_driver_enable;
#endif

static int _set_power_vote_if_needed(struct hw_fence_driver_data *drv_data,
	u32 client_id, bool state)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (drv_data->has_soccp && client_id >= HW_FENCE_CLIENT_ID_VAL0 &&
			client_id <= HW_FENCE_CLIENT_ID_VAL6) {
		ret = hw_fence_utils_set_power_vote(drv_data, state);
	}
#endif /* CONFIG_DEBUG_FS */

	return ret;
}

static void msm_hw_fence_client_destroy(struct kref *kref)
{
	struct msm_hw_fence_client *hw_fence_client = container_of(kref,
		struct msm_hw_fence_client, kref);
	hw_fence_cleanup_client(hw_fence_drv_data, hw_fence_client);
}

void *msm_hw_fence_register(enum hw_fence_client_id client_id_ext,
	struct msm_hw_fence_mem_addr *mem_descriptor)
{
	struct msm_hw_fence_client *hw_fence_client;
	enum hw_fence_client_id client_id;
	int ret;

	if (!hw_fence_driver_enable)
		return ERR_PTR(-ENODEV);

	HWFNC_DBG_H("++ client_id_ext:%d\n", client_id_ext);

	ret = hw_fence_check_hw_fence_driver(hw_fence_drv_data);
	if (ret)
		return ERR_PTR(ret);

	if (client_id_ext >= HW_FENCE_CLIENT_MAX) {
		HWFNC_ERR("Invalid client_id_ext:%d\n", client_id_ext);
		return ERR_PTR(-EINVAL);
	}

	client_id = hw_fence_utils_get_client_id_priv(hw_fence_drv_data, client_id_ext);
	if (client_id >= HW_FENCE_CLIENT_MAX) {
		HWFNC_ERR("Invalid params: client_id:%d client_id_ext:%d\n",
			client_id, client_id_ext);
		return ERR_PTR(-EINVAL);
	}

	/* Alloc client handle */
	hw_fence_client =  kzalloc(sizeof(*hw_fence_client), GFP_KERNEL);
	if (!hw_fence_client)
		return ERR_PTR(-ENOMEM);
	kref_init(&hw_fence_client->kref);

	/* Avoid race condition if multiple-threads request same client at same time */
	mutex_lock(&hw_fence_drv_data->clients_register_lock);
	if (hw_fence_drv_data->clients[client_id] &&
			kref_get_unless_zero(&hw_fence_drv_data->clients[client_id]->kref)) {
		mutex_unlock(&hw_fence_drv_data->clients_register_lock);
		HWFNC_DBG_INIT("client with id %d already registered\n", client_id);
		kfree(hw_fence_client);

		/* Client already exists, return the pointer to the client and populate mem desc */
		hw_fence_client = hw_fence_drv_data->clients[client_id];

		/* Init client memory descriptor */
		if (!IS_ERR_OR_NULL(mem_descriptor))
			memcpy(mem_descriptor, &hw_fence_client->mem_descriptor,
				sizeof(struct msm_hw_fence_mem_addr));
		else
			HWFNC_DBG_L("null mem descriptor, skipping copy\n");

		return hw_fence_client;
	}

	/* Mark client as registered */
	hw_fence_drv_data->clients[client_id] = hw_fence_client;
	mutex_unlock(&hw_fence_drv_data->clients_register_lock);

	hw_fence_client->client_id = client_id;
	hw_fence_client->client_id_ext = client_id_ext;
	hw_fence_client->ipc_client_vid =
		hw_fence_ipcc_get_client_virt_id(hw_fence_drv_data, client_id);
	hw_fence_client->ipc_client_pid =
		hw_fence_ipcc_get_client_phys_id(hw_fence_drv_data, client_id);

	if (hw_fence_client->ipc_client_vid <= 0 || hw_fence_client->ipc_client_pid <= 0) {
		HWFNC_ERR("Failed to find client:%d ipc vid:%d pid:%d\n", client_id,
			hw_fence_client->ipc_client_vid, hw_fence_client->ipc_client_pid);
		ret = -EINVAL;
		goto error;
	}

	hw_fence_client->ipc_signal_id = hw_fence_ipcc_get_signal_id(hw_fence_drv_data, client_id);
	if (hw_fence_client->ipc_signal_id < 0) {
		HWFNC_ERR("Failed to find client:%d signal\n", client_id);
		ret = -EINVAL;
		goto error;
	}

	hw_fence_client->update_rxq = hw_fence_ipcc_needs_rxq_update(hw_fence_drv_data, client_id);
	hw_fence_client->signaled_update_rxq =
		hw_fence_ipcc_signaled_needs_rxq_update(hw_fence_drv_data, client_id);
	hw_fence_client->signaled_send_ipc = hw_fence_ipcc_signaled_needs_ipc_irq(hw_fence_drv_data,
		client_id);
	hw_fence_client->txq_update_send_ipc =
		hw_fence_ipcc_txq_update_needs_ipc_irq(hw_fence_drv_data, client_id);

	hw_fence_client->queues_num = hw_fence_utils_get_queues_num(hw_fence_drv_data, client_id);
	if (!hw_fence_client->queues_num) {
		HWFNC_ERR("client:%d invalid q_num:%d\n", client_id, hw_fence_client->queues_num);
		ret = -EINVAL;
		goto error;
	}
	if (hw_fence_client->queues_num < HW_FENCE_CLIENT_QUEUES) {
		hw_fence_client->update_rxq = false;
		hw_fence_client->signaled_update_rxq = false;
	}

	hw_fence_client->skip_fctl_ref = hw_fence_utils_get_skip_fctl_ref(hw_fence_drv_data,
		client_id);

	/* Alloc Client HFI Headers and Queues */
	ret = hw_fence_alloc_client_resources(hw_fence_drv_data,
		hw_fence_client, mem_descriptor);
	if (ret)
		goto error;

	/* Initialize signal for communication with FenceCTL */
	ret = hw_fence_init_controller_signal(hw_fence_drv_data, hw_fence_client);
	if (ret)
		goto error;

	/*
	 * Update Fence Controller with the address of the Queues and
	 * the Fences Tables for this client
	 */
	ret = hw_fence_init_controller_resources(hw_fence_client);
	if (ret)
		goto error;

	hw_fence_client->context_id = dma_fence_context_alloc(1);
	mutex_init(&hw_fence_client->error_cb_lock);

	HWFNC_DBG_INIT("Initialized ptr:0x%p client_id:%d q_num:%d ipc signal:%d vid:%d pid:%d\n",
		hw_fence_client, hw_fence_client->client_id, hw_fence_client->queues_num,
		hw_fence_client->ipc_signal_id, hw_fence_client->ipc_client_vid,
		hw_fence_client->ipc_client_pid);

	HWFNC_DBG_INIT("update_rxq:%s signaled update_rxq:%s send_ipc:%s txq_update_send_ipc:%s\n",
		hw_fence_client->update_rxq ? "true" : "false",
		hw_fence_client->signaled_update_rxq ? "true" : "false",
		hw_fence_client->signaled_send_ipc ? "true" : "false",
		hw_fence_client->txq_update_send_ipc ? "true" : "false");

#if IS_ENABLED(CONFIG_DEBUG_FS)
	init_waitqueue_head(&hw_fence_client->wait_queue);
#endif /* CONFIG_DEBUG_FS */

	ret = _set_power_vote_if_needed(hw_fence_drv_data, hw_fence_client->client_id_ext, true);
	if (ret) {
		HWFNC_ERR("set soccp power vote failed, fail client:%u registration ret:%d\n",
			hw_fence_client->client_id_ext, ret);
		goto error;
	}

	return (void *)hw_fence_client;
error:

	/* Free all the allocated resources */
	kref_put(&hw_fence_client->kref, msm_hw_fence_client_destroy);

	HWFNC_ERR("failed with error:%d\n", ret);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(msm_hw_fence_register);

int msm_hw_fence_deregister(void *client_handle)
{
	struct msm_hw_fence_client *hw_fence_client;
	bool destroyed_client;
	u32 client_id;
	int ret = 0;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;
	client_id = hw_fence_client->client_id_ext;

	if (hw_fence_client->client_id >= hw_fence_drv_data->clients_num) {
		HWFNC_ERR("Invalid client_id:%d\n", hw_fence_client->client_id);
		return -EINVAL;
	}

	HWFNC_DBG_H("+\n");

	/* Free all the allocated resources */
	destroyed_client = kref_put(&hw_fence_client->kref, msm_hw_fence_client_destroy);

	if (destroyed_client)
		ret = _set_power_vote_if_needed(hw_fence_drv_data, client_id, false);
	if (ret)
		HWFNC_ERR("remove soccp power vote failed, fail client:%u deregistration ret:%d\n",
			hw_fence_client->client_id_ext, ret);

	HWFNC_DBG_H("-\n");

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_deregister);

int msm_hw_fence_create(void *client_handle,
	struct msm_hw_fence_create_params *params)
{
	struct msm_hw_fence_client *hw_fence_client;
	struct dma_fence_array *array;
	struct dma_fence *fence;
	int ret;

	ret = hw_fence_check_valid_fctl(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	if (!params || !params->handle) {
		HWFNC_ERR("Invalid input\n");
		return -EINVAL;
	}

	HWFNC_DBG_H("+\n");

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;
	fence = (struct dma_fence *)params->fence;

	/* if not provided, create a dma-fence */
	if (!fence) {
		fence = hw_fence_internal_dma_fence_create(hw_fence_drv_data, hw_fence_client,
			params->handle);
		if (IS_ERR_OR_NULL(fence)) {
			HWFNC_ERR("failed to create internal dma-fence for client:%d err:%ld\n",
				hw_fence_client->client_id, PTR_ERR(fence));
			return PTR_ERR(fence);
		}

		return 0;
	}

	/* Block any Fence-Array, we should only get individual fences */
	array = to_dma_fence_array(fence);
	if (array) {
		HWFNC_ERR("HW Fence must be created for individual fences\n");
		return -EINVAL;
	}

	/* This Fence is already a HW-Fence */
	if (test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags)) {
		HWFNC_ERR("DMA Fence already has HW Fence Flag set\n");
		return -EINVAL;
	}

	/* Create the HW Fence, i.e. add entry in the Global Table for this Fence */
	ret = hw_fence_create(hw_fence_drv_data, hw_fence_client, fence->context,
		fence->seqno, params->handle);
	if (ret) {
		HWFNC_ERR("Error creating HW fence\n");
		return ret;
	}

	ret = hw_fence_add_callback(hw_fence_drv_data, fence, *params->handle);
	if (ret) {
		HWFNC_ERR("Fail to add dma-fence signal cb client:%d ctx:%llu seq:%llu ret:%d\n",
			hw_fence_client->client_id, fence->context, fence->seqno, ret);
		/* release both refs, one held by fctl and one held by creating client */
		hw_fence_destroy_refcount(hw_fence_drv_data, *params->handle,
			HW_FENCE_FCTL_REFCOUNT);
		hw_fence_destroy_with_hash(hw_fence_drv_data, hw_fence_client, *params->handle);

		return ret;
	}

	/* If no error, set the HW Fence Flag in the dma-fence */
	set_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags);

	HWFNC_DBG_H("-\n");

	return ret;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_create);

int msm_hw_fence_destroy(void *client_handle,
	struct dma_fence *fence)
{
	struct msm_hw_fence_client *hw_fence_client;
	struct dma_fence_array *array;
	int ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	if (!fence) {
		HWFNC_ERR("Invalid data\n");
		return -EINVAL;
	}
	hw_fence_client = (struct msm_hw_fence_client *)client_handle;

	HWFNC_DBG_H("+\n");

	/* Block any Fence-Array, we should only get individual fences */
	array = to_dma_fence_array(fence);
	if (array) {
		HWFNC_ERR("HW Fence must be destroy for individual fences\n");
		return -EINVAL;
	}

	/* This Fence not a HW-Fence */
	if (!test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags)) {
		HWFNC_ERR("DMA Fence is not a HW Fence flags:0x%lx\n", fence->flags);
		return -EINVAL;
	}

	if (dma_fence_is_hw_dma(fence)) {
		HWFNC_ERR("deprecated api cannot destroy hw_dma_fence ctx:%llu seq:%llu\n",
			fence->context, fence->seqno);
		return -EINVAL;
	}

	/* Destroy the HW Fence, i.e. remove entry in the Global Table for the Fence */
	ret = hw_fence_destroy(hw_fence_drv_data, hw_fence_client,
		fence->context, fence->seqno);
	if (ret) {
		HWFNC_ERR("Error destroying the HW fence\n");
		return ret;
	}

	/* Clear the HW Fence Flag in the dma-fence */
	clear_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags);

	HWFNC_DBG_H("-\n");

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_destroy);

int msm_hw_fence_destroy_with_handle(void *client_handle, u64 handle)
{
	struct msm_hw_fence_client *hw_fence_client;
	int ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;

	if (hw_fence_client->client_id >= hw_fence_drv_data->clients_num) {
		HWFNC_ERR("Invalid client_id:%d\n", hw_fence_client->client_id);
		return -EINVAL;
	}

	HWFNC_DBG_H("+\n");

	/* Destroy the HW Fence, i.e. remove entry in the Global Table for the Fence */
	ret = hw_fence_destroy_with_hash(hw_fence_drv_data, hw_fence_client, handle);
	if (ret) {
		HWFNC_ERR("Error destroying the HW fence handle:%llu client_id:%d\n", handle,
			hw_fence_client->client_id);
		return ret;
	}

	HWFNC_DBG_H("-\n");

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_destroy_with_handle);

int msm_hw_fence_wait_update_v2(void *client_handle,
	struct dma_fence **fence_list, u64 *handles, u64 *client_data_list, u32 num_fences,
	bool create)
{
	struct msm_hw_fence_client *hw_fence_client;
	struct dma_fence_array *array;
	int i, j, destroy_ret, ret = 0;
	enum hw_fence_client_data_id data_id;

	ret = hw_fence_check_valid_fctl(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	if (!fence_list || !*fence_list) {
		HWFNC_ERR("Invalid data\n");
		return -EINVAL;
	}

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;
	data_id = hw_fence_get_client_data_id(hw_fence_client->client_id_ext);
	if (client_data_list && data_id >= HW_FENCE_MAX_CLIENTS_WITH_DATA) {
		HWFNC_ERR("Populating non-NULL client_data_list with invalid client_id_ext:%d\n",
			hw_fence_client->client_id_ext);
		return -EINVAL;
	}

	HWFNC_DBG_H("+\n");

	/* Process all the list of fences */
	for (i = 0; i < num_fences; i++) {
		struct dma_fence *fence = fence_list[i];
		u64 hash, client_data = 0;

		if (client_data_list)
			client_data = client_data_list[i];

		/* Process a Fence-Array */
		array = to_dma_fence_array(fence);
		if (array) {
			ret = hw_fence_process_fence_array(hw_fence_drv_data, hw_fence_client,
				array, &hash, client_data);
			if (ret) {
				HWFNC_ERR("Failed to process FenceArray\n");
				goto error;
			}
		} else {
			/* Process individual Fence */
			ret = hw_fence_process_fence(hw_fence_drv_data, hw_fence_client, fence,
				&hash, client_data);
			if (ret) {
				HWFNC_ERR("Failed to process Fence\n");
				goto error;
			}
		}

		if (handles)
			handles[i] = hash;
	}

	HWFNC_DBG_H("-\n");

	return 0;
error:
	for (j = 0; j < i; j++) {
		destroy_ret = hw_fence_destroy_with_hash(hw_fence_drv_data, hw_fence_client,
			handles[j]);
		if (destroy_ret)
			HWFNC_ERR("Failed decr fence ref ctx:%llu seq:%llu h:%llu idx:%d ret:%d\n",
				fence_list[j] ? fence_list[j]->context : -1, fence_list[j] ?
				fence_list[j]->seqno : -1, handles[j], j, destroy_ret);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_wait_update_v2);

int msm_hw_fence_wait_update(void *client_handle,
	struct dma_fence **fence_list, u32 num_fences, bool create)
{
	u64 handle;
	int i, ret = 0;

	for (i = 0; i < num_fences; i++) {
		ret = msm_hw_fence_wait_update_v2(client_handle, &fence_list[i], &handle, NULL,
			1, create);

		if (ret) {
			HWFNC_ERR("Failed reg for wait on fence ctx:%llu seq:%llu idx:%d ret:%d\n",
				fence_list[i] ? fence_list[i]->context : -1,
				fence_list[i] ? fence_list[i]->seqno : -1, i, ret);
			return ret;
		}

		/* decrement reference on hw-fence acquired by msm_hw_fence_wait_update_v2 call */
		ret = msm_hw_fence_destroy_with_handle(client_handle, handle);
		if (ret) {
			HWFNC_ERR("Failed decr fence ref ctx:%llu seq:%llu h:%llu idx:%d ret:%d\n",
				fence_list[i] ? fence_list[i]->context : -1,
				fence_list[i] ? fence_list[i]->seqno : -1, handle, i, ret);
			return ret;
		}
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_wait_update);

int msm_hw_fence_reset_client(void *client_handle, u32 reset_flags)
{
	struct msm_hw_fence_client *hw_fence_client;
	struct msm_hw_fence *hw_fences_tbl;
	int i, ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;
	hw_fences_tbl = hw_fence_drv_data->hw_fences_tbl;

	HWFNC_DBG_L("reset fences and queues for client:%d\n", hw_fence_client->client_id);
	for (i = 0; i < hw_fence_drv_data->hw_fences_tbl_cnt; i++)
		hw_fence_utils_cleanup_fence(hw_fence_drv_data, hw_fence_client,
			&hw_fences_tbl[i], i, reset_flags);

	hw_fence_utils_reset_queues(hw_fence_drv_data, hw_fence_client);

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_reset_client);

int msm_hw_fence_reset_client_by_id(enum hw_fence_client_id client_id_ext, u32 reset_flags)
{
	enum hw_fence_client_id client_id;
	int ret;

	ret = hw_fence_check_hw_fence_driver(hw_fence_drv_data);
	if (ret)
		return ret;

	if (client_id_ext >= HW_FENCE_CLIENT_MAX) {
		HWFNC_ERR("Invalid client_id_ext:%d\n", client_id_ext);
		return -EINVAL;
	}

	client_id = hw_fence_utils_get_client_id_priv(hw_fence_drv_data, client_id_ext);

	if (client_id >= HW_FENCE_CLIENT_MAX) {
		HWFNC_ERR("Invalid client_id:%d client_id_ext:%d\n", client_id, client_id_ext);
		return -EINVAL;
	}

	return msm_hw_fence_reset_client(hw_fence_drv_data->clients[client_id],
		reset_flags);
}
EXPORT_SYMBOL_GPL(msm_hw_fence_reset_client_by_id);

int msm_hw_fence_update_txq(void *client_handle, u64 handle, u64 flags, u32 error)
{
	struct msm_hw_fence_client *hw_fence_client;
	int ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	if (handle >= hw_fence_drv_data->hw_fences_tbl_cnt) {
		HWFNC_ERR("Invalid handle:%llu max:%d\n", handle,
			hw_fence_drv_data->hw_fences_tbl_cnt);
		return -EINVAL;
	}
	hw_fence_client = (struct msm_hw_fence_client *)client_handle;

	/* Write to Tx queue */
	hw_fence_update_queue(hw_fence_drv_data, hw_fence_client,
		hw_fence_drv_data->hw_fences_tbl[handle].ctx_id,
		hw_fence_drv_data->hw_fences_tbl[handle].seq_id, handle,
		flags, 0, error, HW_FENCE_TX_QUEUE - 1);

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_update_txq);


int msm_hw_fence_update_txq_error(void *client_handle, u64 handle, u32 error, u32 update_flags)
{
	struct msm_hw_fence_client *hw_fence_client;
	int ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	if ((handle >= hw_fence_drv_data->hw_fences_tbl_cnt) || !error) {
		HWFNC_ERR("Invalid fence handle:%llu max:%d or error:%d\n",
			handle, hw_fence_drv_data->hw_fences_tbl_cnt, error);
		return -EINVAL;
	}

	if (update_flags != MSM_HW_FENCE_UPDATE_ERROR_WITH_MOVE) {
		HWFNC_ERR("invalid flags:0x%x expected:0x%lx no support of in-place error update\n",
			update_flags, MSM_HW_FENCE_UPDATE_ERROR_WITH_MOVE);
		return -EINVAL;
	}
	hw_fence_client = (struct msm_hw_fence_client *)client_handle;

	/* Write to Tx queue */
	hw_fence_update_existing_txq_payload(hw_fence_drv_data, hw_fence_client,
		handle, error);

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_update_txq_error);

/* tx client has to be the physical, rx client virtual id*/
int msm_hw_fence_trigger_signal(void *client_handle,
	u32 tx_client_pid, u32 rx_client_vid,
	u32 signal_id)
{
	struct msm_hw_fence_client *hw_fence_client;
	int ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;

	HWFNC_DBG_H("sending ipc for client:%d\n", hw_fence_client->client_id);
	hw_fence_ipcc_trigger_signal(hw_fence_drv_data, tx_client_pid,
		rx_client_vid, signal_id);

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_trigger_signal);

int msm_hw_fence_register_error_cb(void *client_handle, msm_hw_fence_error_cb_t cb, void *data)
{
	struct msm_hw_fence_client *hw_fence_client;
	int ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	if (IS_ERR_OR_NULL(cb) || IS_ERR_OR_NULL(data)) {
		HWFNC_ERR("Invalid params cb_func:0x%pK data:0x%pK\n", cb, data);
		return -EINVAL;
	}

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;
	if (hw_fence_client->fence_error_cb) {
		HWFNC_ERR("client_id:%d client_id_ext:%d already registered cb_func:%pK data:%pK\n",
			hw_fence_client->client_id, hw_fence_client->client_id_ext,
			hw_fence_client->fence_error_cb, hw_fence_client->fence_error_cb_userdata);
		return -EINVAL;
	}

	hw_fence_client->fence_error_cb_userdata = data;
	hw_fence_client->fence_error_cb = cb;

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_register_error_cb);

int msm_hw_fence_deregister_error_cb(void *client_handle)
{
	struct msm_hw_fence_client *hw_fence_client;
	int ret = 0;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;
	if (!mutex_trylock(&hw_fence_client->error_cb_lock)) {
		HWFNC_ERR("client_id:%d is modifying or using fence_error_cb:0x%pK data:0x%pK\n",
			hw_fence_client->client_id, hw_fence_client->fence_error_cb,
			hw_fence_client->fence_error_cb_userdata);
		return -EAGAIN;
	}

	if (!hw_fence_client->fence_error_cb) {
		HWFNC_ERR("client_id:%d client_id_ext:%d did not register cb:%pK data:%pK\n",
			hw_fence_client->client_id, hw_fence_client->client_id_ext,
			hw_fence_client->fence_error_cb, hw_fence_client->fence_error_cb_userdata);
		ret = -EINVAL;
		goto exit;
	}

	hw_fence_client->fence_error_cb = NULL;
	hw_fence_client->fence_error_cb_userdata = NULL;

exit:
	mutex_unlock(&hw_fence_client->error_cb_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_deregister_error_cb);

#if IS_ENABLED(CONFIG_DEBUG_FS)
int msm_hw_fence_dump_debug_data(void *client_handle, u32 dump_flags, u32 dump_clients_mask)
{
	struct msm_hw_fence_client *hw_fence_client;
	int client_id, ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	hw_fence_client = (struct msm_hw_fence_client *)client_handle;

	if (dump_flags & MSM_HW_FENCE_DBG_DUMP_QUEUES) {
		hw_fence_debug_dump_queues(hw_fence_drv_data, HW_FENCE_PRINTK, hw_fence_client);

		if (dump_clients_mask)
			for (client_id = 0; client_id < HW_FENCE_CLIENT_MAX; client_id++)
				if ((dump_clients_mask & (1 << client_id)) &&
						hw_fence_drv_data->clients[client_id])
					hw_fence_debug_dump_queues(hw_fence_drv_data,
						HW_FENCE_PRINTK,
						hw_fence_drv_data->clients[client_id]);
	}

	if (dump_flags & MSM_HW_FENCE_DBG_DUMP_TABLE)
		hw_fence_debug_dump_table(HW_FENCE_PRINTK, hw_fence_drv_data);

	if (dump_flags & MSM_HW_FENCE_DBG_DUMP_EVENTS)
		hw_fence_debug_dump_events(HW_FENCE_PRINTK, hw_fence_drv_data);

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_dump_debug_data);

int msm_hw_fence_dump_fence(void *client_handle, struct dma_fence *fence)
{
	struct msm_hw_fence_client *hw_fence_client;
	struct msm_hw_fence *hw_fence;
	u64 hash;
	int ret;

	ret = hw_fence_check_valid_client(hw_fence_drv_data, client_handle);
	if (ret)
		return ret;

	if (!test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags)) {
		HWFNC_ERR("DMA Fence is not a HW Fence ctx:%llu seqno:%llu flags:0x%lx\n",
			fence->context, fence->seqno, fence->flags);
		return -EINVAL;
	}
	hw_fence_client = (struct msm_hw_fence_client *)client_handle;

	hw_fence = msm_hw_fence_find(hw_fence_drv_data, hw_fence_client, fence->context,
		fence->seqno, &hash);
	if (!hw_fence) {
		HWFNC_ERR("failed to find hw-fence client_id:%d fence:0x%pK ctx:%llu seqno:%llu\n",
			hw_fence_client->client_id, fence, fence->context, fence->seqno);
		return -EINVAL;
	}
	hw_fence_debug_dump_fence(HW_FENCE_PRINTK, hw_fence, hash, 0);
	/* release refcount acquired by finding fence */
	msm_hw_fence_destroy_with_handle(client_handle, hash);

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_dump_fence);
#endif /* CONFIG_DEBUG_FS */

/* Function used for simulation purposes only. */
int msm_hw_fence_driver_doorbell_sim(u64 db_mask)
{
	int ret;

	ret = hw_fence_check_hw_fence_driver(hw_fence_drv_data);
	if (ret)
		return ret;

	HWFNC_DBG_IRQ("db callback sim-mode flags:0x%llx qtime:%llu\n",
		db_mask, hw_fence_get_qtime(hw_fence_drv_data));

	hw_fence_utils_process_signaled_clients_mask(hw_fence_drv_data, db_mask);

	return 0;
}
EXPORT_SYMBOL_GPL(msm_hw_fence_driver_doorbell_sim);

static int msm_hw_fence_probe_init(struct platform_device *pdev)
{
	int rc;

	HWFNC_DBG_H("+\n");

	hw_fence_drv_data = kzalloc(sizeof(*hw_fence_drv_data), GFP_KERNEL);
	if (!hw_fence_drv_data)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, hw_fence_drv_data);
	hw_fence_drv_data->dev = &pdev->dev;

	if (hw_fence_driver_enable) {
		/* Initialize HW Fence Driver resources */
		rc = hw_fence_init(hw_fence_drv_data);
		if (rc)
			goto error;

		mutex_init(&hw_fence_drv_data->clients_register_lock);

		/* set ready value so clients can register */
		hw_fence_drv_data->resources_ready = true;
	} else {
		/* check for presence of soccp */
		hw_fence_drv_data->has_soccp =
			of_property_read_bool(hw_fence_drv_data->dev->of_node, "soccp_controller");

		/* Allocate hw fence driver mem pool and share it with HYP */
		rc = hw_fence_utils_alloc_mem(hw_fence_drv_data);
		if (rc) {
			HWFNC_ERR_ONCE("failed to alloc base memory\n");
			goto error;
		}

		HWFNC_DBG_INFO("hw fence driver not enabled\n");
	}

	HWFNC_DBG_H("-\n");

	return rc;

error:
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(hw_fence_drv_data->ipc_clients_table);
	kfree(hw_fence_drv_data->hw_fence_client_queue_size);
	if (hw_fence_drv_data->cpu_addr_cookie)
		dma_free_attrs(hw_fence_drv_data->dev, hw_fence_drv_data->size,
			hw_fence_drv_data->cpu_addr_cookie, hw_fence_drv_data->res.start,
			DMA_ATTR_NO_KERNEL_MAPPING);
	kfree(hw_fence_drv_data);
	hw_fence_drv_data = (void *) -EPROBE_DEFER;

	HWFNC_ERR_ONCE("error %d\n", rc);

	return rc;
}

static int msm_hw_fence_probe(struct platform_device *pdev)
{
	int rc = -EINVAL;

	HWFNC_DBG_H("+\n");

	if (!pdev) {
		HWFNC_ERR("null platform dev\n");
		return -EINVAL;
	}

	if (of_device_is_compatible(pdev->dev.of_node, "qcom,msm-hw-fence"))
		rc = msm_hw_fence_probe_init(pdev);
	if (rc)
		goto err_exit;

	HWFNC_DBG_H("-\n");

	return 0;

err_exit:
	HWFNC_ERR_ONCE("error %d\n", rc);
	return rc;
}

static int msm_hw_fence_remove(struct platform_device *pdev)
{
	struct hw_fence_soccp *soccp_props;

	HWFNC_DBG_H("+\n");

	if (!pdev) {
		HWFNC_ERR("null platform dev\n");
		return -EINVAL;
	}

	hw_fence_drv_data = dev_get_drvdata(&pdev->dev);
	if (!hw_fence_drv_data) {
		HWFNC_ERR("null driver data\n");
		return -EINVAL;
	}
	soccp_props = &hw_fence_drv_data->soccp_props;
	if (soccp_props->ssr_notifier) {
		if (qcom_unregister_ssr_notifier(soccp_props->ssr_notifier,
				&soccp_props->ssr_nb))
			HWFNC_ERR("failed to unregister soccp ssr notifier\n");
	}

	/* indicate listener thread should stop listening for interrupts from soccp */
	hw_fence_drv_data->has_soccp = false;
	if (hw_fence_drv_data->soccp_listener_thread)
		kthread_stop(hw_fence_drv_data->soccp_listener_thread);

	dev_set_drvdata(&pdev->dev, NULL);

	/* free memory allocations as part of hw_fence_drv_data */
	kfree(hw_fence_drv_data->ipc_clients_table);
	kfree(hw_fence_drv_data->hw_fence_client_queue_size);
	if (hw_fence_drv_data->cpu_addr_cookie)
		dma_free_attrs(hw_fence_drv_data->dev, hw_fence_drv_data->size,
			hw_fence_drv_data->cpu_addr_cookie, hw_fence_drv_data->res.start,
			DMA_ATTR_NO_KERNEL_MAPPING);
	kfree(hw_fence_drv_data);
	hw_fence_drv_data = (void *) -EPROBE_DEFER;

	HWFNC_DBG_H("-\n");

	return 0;
}

static const struct of_device_id msm_hw_fence_dt_match[] = {
	{.compatible = "qcom,msm-hw-fence"},
	{}
};

static struct platform_driver msm_hw_fence_driver = {
	.probe = msm_hw_fence_probe,
	.remove = msm_hw_fence_remove,
	.driver = {
		.name = "msm-hw-fence",
		.of_match_table = of_match_ptr(msm_hw_fence_dt_match),
	},
};

static int __init msm_hw_fence_init(void)
{
	int rc = 0;

	HWFNC_DBG_H("+\n");

	rc = platform_driver_register(&msm_hw_fence_driver);
	if (rc) {
		HWFNC_ERR("%s: failed to register platform driver\n",
			__func__);
		return rc;
	}

	HWFNC_DBG_H("-\n");

	return 0;
}

static void __exit msm_hw_fence_exit(void)
{
	HWFNC_DBG_H("+\n");

	platform_driver_unregister(&msm_hw_fence_driver);

	HWFNC_DBG_H("-\n");
}

module_param_named(enable, hw_fence_driver_enable, bool, 0600);
MODULE_PARM_DESC(enable, "Enable hardware fences");

module_init(msm_hw_fence_init);
module_exit(msm_hw_fence_exit);

MODULE_DESCRIPTION("QTI HW Fence Driver");
MODULE_LICENSE("GPL v2");
