// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/slab.h>
#include <synx_interop.h>
#include "msm_hw_fence.h"
#include "hw_fence_drv_priv.h"
#include "hw_fence_drv_debug.h"
#include "hw_fence_drv_interop.h"

/**
 * HW_FENCE_SYNX_FENCE_CLIENT_ID:
 * ClientID for fences created to back synx fences
 */
#define HW_FENCE_SYNX_FENCE_CLIENT_ID (~(u32)1)

/**
 * HW_FENCE_SYNX_FENCE_CLIENT_ID:
 * ClientID for fences created to back fences with native dma-fence producers
 */
#define HW_FENCE_NATIVE_FENCE_CLIENT_ID (~(u32)2)

struct synx_hwfence_interops synx_interops = {
	.share_handle_status = NULL,
	.get_fence = NULL,
	.notify_recover = NULL,
};

int hw_fence_interop_to_synx_status(int hw_fence_status_code)
{
	int synx_status_code;

	switch (hw_fence_status_code) {
	case 0:
		synx_status_code = SYNX_SUCCESS;
		break;
	case -ENOMEM:
		synx_status_code = -SYNX_NOMEM;
		break;
	case -EPERM:
		synx_status_code = -SYNX_NOPERM;
		break;
	case -ETIMEDOUT:
		synx_status_code = -SYNX_TIMEOUT;
		break;
	case -EALREADY:
		synx_status_code = -SYNX_ALREADY;
		break;
	case -ENOENT:
		synx_status_code = -SYNX_NOENT;
		break;
	case -EINVAL:
		synx_status_code = -SYNX_INVALID;
		break;
	case -EBUSY:
		synx_status_code = -SYNX_BUSY;
		break;
	case -EAGAIN:
		synx_status_code = -SYNX_EAGAIN;
		break;
	default:
		synx_status_code = hw_fence_status_code;
		break;
	}

	return synx_status_code;
}

u32 hw_fence_interop_to_synx_signal_status(u32 flags, u32 error)
{
	u32 status;

	if (!(flags & MSM_HW_FENCE_FLAG_SIGNAL)) {
		status = SYNX_STATE_ACTIVE;
		goto end;
	}

	switch (error) {
	case 0:
		status = SYNX_STATE_SIGNALED_SUCCESS;
		break;
	case MSM_HW_FENCE_ERROR_RESET:
		status = SYNX_STATE_SIGNALED_SSR;
		break;
	default:
		status = error;
		break;
	}

end:
	HWFNC_DBG_L("fence flags:%u err:%u status:%u\n", flags, error, status);

	return status;
}

u32 hw_fence_interop_to_hw_fence_error(u32 status)
{
	u32 error;

	switch (status) {
	case SYNX_STATE_INVALID:
		HWFNC_ERR("converting error status for invalid fence\n");
		error = SYNX_INVALID;
		break;
	case SYNX_STATE_ACTIVE:
		HWFNC_ERR("converting error status for unsignaled fence\n");
		error = 0;
		break;
	case SYNX_STATE_SIGNALED_SUCCESS:
		error = 0;
		break;
	case SYNX_STATE_SIGNALED_SSR:
		error = MSM_HW_FENCE_ERROR_RESET;
		break;
	default:
		error = status;
		break;
	}
	HWFNC_DBG_L("fence status:%u err:%u\n", status, error);

	return error;
}

static int _update_interop_fence(struct synx_import_indv_params *params, u64 handle)
{
	u32 signal_status;
	int ret, error;

	if (!params->new_h_synx || !synx_interops.share_handle_status) {
		HWFNC_ERR("invalid new_h_synx:0x%pK share_handle_status:0x%pK\n",
			params->new_h_synx, synx_interops.share_handle_status);
		return -EINVAL;
	}

	ret = synx_interops.share_handle_status(params, handle, &signal_status);
	if (ret || signal_status == SYNX_STATE_INVALID) {
		HWFNC_ERR("failed to share handle and signal status handle:%llu ret:%d\n",
			handle, ret);
		/* destroy reference held by signal*/
		hw_fence_destroy_refcount(hw_fence_drv_data, handle, HW_FENCE_FCTL_REFCOUNT);

		return ret;
	}
	if (signal_status != SYNX_STATE_ACTIVE) {
		error = hw_fence_interop_to_hw_fence_error(signal_status);
		ret = hw_fence_signal_fence(hw_fence_drv_data, NULL, handle, error, true);
		if (ret) {
			HWFNC_ERR("Failed to signal hwfence handle:%llu error:%u\n", handle, error);
			return ret;
		}
	}

	/* store h_synx for debugging purposes */
	ret = hw_fence_update_hsynx(hw_fence_drv_data, handle, *params->new_h_synx, false);
	if (ret)
		HWFNC_ERR("Failed to update hwfence handle:%llu h_synx:%u\n", handle,
			*params->new_h_synx);

	return ret;
}

int hw_fence_interop_create_fence_from_import(struct synx_import_indv_params *params)
{
	struct msm_hw_fence_client dummy_client;
	struct dma_fence *fence;
	int destroy_ret, ret;
	unsigned long flags;
	bool is_synx;
	u64 handle;

	if (IS_ERR_OR_NULL(params) || IS_ERR_OR_NULL(params->fence)) {
		HWFNC_ERR("invalid params:0x%pK fence:0x%pK\n",
			params, IS_ERR_OR_NULL(params) ? NULL : params->fence);
		return -SYNX_INVALID;
	}

	fence = (struct dma_fence *)params->fence;
	spin_lock_irqsave(fence->lock, flags);

	/* hw-fence already present, so no need to create new hw-fence */
	if (test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags)) {
		spin_unlock_irqrestore(fence->lock, flags);
		return SYNX_SUCCESS;
	}
	is_synx = test_bit(SYNX_NATIVE_FENCE_FLAG_ENABLED_BIT, &fence->flags);

	/* only synx clients can signal synx fences; no one can signal sw dma-fence from fw */
	dummy_client.client_id = is_synx ? HW_FENCE_SYNX_FENCE_CLIENT_ID :
		HW_FENCE_NATIVE_FENCE_CLIENT_ID;
	ret = hw_fence_create(hw_fence_drv_data, &dummy_client, fence->context,
		fence->seqno, &handle);
	if (ret) {
		HWFNC_ERR("failed create fence client:%d ctx:%llu seq:%llu is_synx:%s ret:%d\n",
			dummy_client.client_id, fence->context, fence->seqno,
			is_synx ? "true" : "false", ret);
		spin_unlock_irqrestore(fence->lock, flags);
		return hw_fence_interop_to_synx_status(ret);
	}
	set_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags);
	spin_unlock_irqrestore(fence->lock, flags);

	if (is_synx)
		/* exchange handles and register fence controller for wait on synx fence */
		ret = _update_interop_fence(params, handle);
	else
		/* native dma-fences do not have a signaling client, remove ref for fctl signal */
		ret = hw_fence_destroy_refcount(hw_fence_drv_data, handle, HW_FENCE_FCTL_REFCOUNT);

	if (ret) {
		HWFNC_ERR("failed to update for signaling client handle:%llu is_synx:%s ret:%d\n",
			handle, is_synx ? "true" : "false", ret);
		goto error;
	}

	ret = hw_fence_add_callback(hw_fence_drv_data, fence, handle);
	if (ret)
		HWFNC_ERR("failed to add signal callback for fence handle:%llu is_synx:%s ret:%d\n",
			handle, is_synx ? "true" : "false", ret);

error:
	/* destroy reference held by creator of fence */
	destroy_ret = hw_fence_destroy_with_hash(hw_fence_drv_data, &dummy_client,
		handle);
	if (destroy_ret) {
		HWFNC_ERR("failed destroy fence client:%d handle:%llu is_synx:%s ret:%d\n",
			dummy_client.client_id, handle, is_synx ? "true" : "false", ret);
		ret = destroy_ret;
	}

	return hw_fence_interop_to_synx_status(ret);
}

int hw_fence_interop_share_handle_status(struct synx_import_indv_params *params, u32 h_synx,
	u32 *signal_status)
{
	struct msm_hw_fence *hw_fence;
	int destroy_ret, ret = 0;
	struct dma_fence *fence;
	u64 flags, handle;
	bool is_signaled;
	u32 error;

	ret = hw_fence_check_hw_fence_driver(hw_fence_drv_data);
	if (ret)
		return hw_fence_interop_to_synx_status(ret);

	if (!hw_fence_drv_data->fctl_ready) {
		HWFNC_ERR("fctl in invalid state, cannot perform operation\n");
		return -SYNX_EAGAIN;
	}

	if (IS_ERR_OR_NULL(params) || IS_ERR_OR_NULL(params->new_h_synx) ||
			!(params->flags & SYNX_IMPORT_DMA_FENCE) ||
			(params->flags & SYNX_IMPORT_SYNX_FENCE) || IS_ERR_OR_NULL(params->fence)) {
		HWFNC_ERR("invalid params:0x%pK h_synx:0x%pK flags:0x%x fence:0x%pK\n",
			params, IS_ERR_OR_NULL(params) ? NULL : params->new_h_synx,
			IS_ERR_OR_NULL(params) ? 0 : params->flags,
			IS_ERR_OR_NULL(params) ? NULL : params->fence);
		return -SYNX_INVALID;
	}
	fence = params->fence;
	if (!test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags)) {
		HWFNC_ERR("invalid hwfence ctx:%llu seqno:%llu flags:%lx\n", fence->context,
			fence->seqno, fence->flags);
		return -SYNX_INVALID;
	}

	hw_fence = hw_fence_find_with_dma_fence(hw_fence_drv_data, NULL, fence, &handle,
		&is_signaled, false);

	if (is_signaled) {
		*signal_status = dma_fence_get_status(fence);
		return SYNX_SUCCESS;
	}
	if (!hw_fence) {
		HWFNC_ERR("failed to find hw-fence for ctx:%llu seq:%llu\n", fence->context,
			fence->seqno);
		return -SYNX_INVALID;
	}

	ret = hw_fence_get_flags_error(hw_fence_drv_data, handle, &flags, &error);
	if (ret) {
		HWFNC_ERR("Failed to get flags and error hwfence handle:%llu\n", handle);
		goto end;
	}

	*signal_status = hw_fence_interop_to_synx_signal_status(flags, error);
	if (*signal_status >= SYNX_STATE_SIGNALED_SUCCESS)
		goto end;

	/* update h_synx to register the synx framework as a waiter on the hw-fence */
	ret = hw_fence_update_hsynx(hw_fence_drv_data, handle, h_synx, true);
	if (ret) {
		HWFNC_ERR("failed to set h_synx for hw-fence handle:%llu\n", handle);
		goto end;
	}
	*params->new_h_synx = (u32)handle;

end:
	/* release reference held to find hw-fence */
	destroy_ret = hw_fence_destroy_with_hash(hw_fence_drv_data, NULL, handle);
	if (destroy_ret) {
		HWFNC_ERR("Failed to decrement refcount on hw-fence handle:%llu\n", handle);
		ret = destroy_ret;
	}

	return hw_fence_interop_to_synx_status(ret);
}

void *hw_fence_interop_get_fence(u32 h_synx)
{
	struct dma_fence *fence;
	int ret;

	ret = hw_fence_check_hw_fence_driver(hw_fence_drv_data);
	if (ret)
		return ERR_PTR(hw_fence_interop_to_synx_status(ret));

	if (!(h_synx & SYNX_HW_FENCE_HANDLE_FLAG)) {
		HWFNC_ERR("invalid h_synx:%u does not have hw-fence handle bit set:%lu\n",
			h_synx, SYNX_HW_FENCE_HANDLE_FLAG);
		return ERR_PTR(-SYNX_INVALID);
	}

	h_synx &= HW_FENCE_HANDLE_INDEX_MASK;
	fence = hw_fence_dma_fence_find(hw_fence_drv_data, h_synx, true);
	if (!fence) {
		HWFNC_ERR("failed to find dma-fence for hw-fence idx:%u\n", h_synx);
		return ERR_PTR(-SYNX_INVALID);
	}

	return (void *)fence;
}

int synx_hwfence_init_interops(struct synx_hwfence_interops *synx_ops,
	struct synx_hwfence_interops *hwfence_ops)
{
	if (IS_ERR_OR_NULL(synx_ops) || IS_ERR_OR_NULL(hwfence_ops)) {
		HWFNC_ERR("invalid params synx_ops:0x%pK hwfence_ops:0x%pK\n", synx_ops,
			hwfence_ops);
		return -EINVAL;
	}

	synx_interops.share_handle_status = synx_ops->share_handle_status;
	synx_interops.get_fence = synx_ops->get_fence;
	synx_interops.notify_recover = synx_ops->notify_recover;
	hwfence_ops->share_handle_status = hw_fence_interop_share_handle_status;
	hwfence_ops->get_fence = hw_fence_interop_get_fence;

	return 0;
}
EXPORT_SYMBOL_GPL(synx_hwfence_init_interops);
