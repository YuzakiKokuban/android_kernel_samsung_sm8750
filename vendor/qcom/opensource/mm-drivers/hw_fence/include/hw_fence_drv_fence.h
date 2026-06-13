/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __HW_FENCE_DRV_HW_DMA_FENCE
#define __HW_FENCE_DRV_HW_DMA_FENCE

#define HW_FENCE_NAME_SIZE 64

/**
 * struct hw_dma_fence - fences internally created by hw-fence driver.
 * @base: base dma-fence structure, this must remain at beginning of the struct.
 * @name: name of each fence.
 * @client_handle: handle for the client owner of this fence, this is returned by the hw-fence
 *                 driver after a successful registration of the client and used by this fence
 *                 during release.
 * @data: internal data to process the fence ops.
 * @dma_fence_key: key for the dma-fence hash table.
 * @is_internal: true if this fence is initialized internally by hw-fence driver, false otherwise
 * @signal_cb: drv_data, hash, and signal_cb of hw_fence
 * @node: node for fences held in the dma-fences hash table linked lists
 */
struct hw_dma_fence {
	struct dma_fence base;
	char name[HW_FENCE_NAME_SIZE];
	void *client_handle;
	u32 dma_fence_key;
	bool is_internal;
	struct hw_fence_signal_cb signal_cb;
	struct hlist_node node;
};

static inline struct hw_dma_fence *to_hw_dma_fence(struct dma_fence *fence)
{
	return container_of(fence, struct hw_dma_fence, base);
}

static const char *hw_fence_dbg_get_driver_name(struct dma_fence *fence)
{
	struct hw_dma_fence *hw_dma_fence = to_hw_dma_fence(fence);

	return hw_dma_fence->name;
}

static const char *hw_fence_dbg_get_timeline_name(struct dma_fence *fence)
{
	struct hw_dma_fence *hw_dma_fence = to_hw_dma_fence(fence);

	return hw_dma_fence->name;
}

static bool hw_fence_dbg_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static void _hw_fence_release(struct hw_dma_fence *hw_dma_fence)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(hw_dma_fence->client_handle) || (hw_dma_fence->is_internal &&
			IS_ERR_OR_NULL(hw_dma_fence->signal_cb.drv_data))) {
		HWFNC_ERR("invalid hwfence data %pK %pK, won't release hw_fence!\n",
			hw_dma_fence->client_handle, hw_dma_fence->signal_cb.drv_data);
		return;
	}

	/* release hw-fence */
	if (hw_dma_fence->is_internal) /* internally owned hw_dma_fence has its own refcount */
		ret = hw_fence_destroy_refcount(hw_dma_fence->signal_cb.drv_data,
			hw_dma_fence->signal_cb.hash, HW_FENCE_DMA_FENCE_REFCOUNT);
	else /* externally owned hw_dma_fence uses standard hlos refcount */
		ret = msm_hw_fence_destroy(hw_dma_fence->client_handle, &hw_dma_fence->base);

	if (ret)
		HWFNC_ERR("failed to release hw_fence!\n");
}

static void hw_fence_dbg_release(struct dma_fence *fence)
{
	struct hw_dma_fence *hw_dma_fence;

	if (!fence)
		return;

	HWFNC_DBG_H("release backing fence %pK\n", fence);
	hw_dma_fence = to_hw_dma_fence(fence);

	if (test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags))
		_hw_fence_release(hw_dma_fence);

	kfree(fence->lock);
	kfree(hw_dma_fence);
}

static struct dma_fence_ops hw_fence_dbg_ops = {
	.get_driver_name = hw_fence_dbg_get_driver_name,
	.get_timeline_name = hw_fence_dbg_get_timeline_name,
	.enable_signaling = hw_fence_dbg_enable_signaling,
	.wait = dma_fence_default_wait,
	.release = hw_fence_dbg_release,
};

static inline bool dma_fence_is_hw_dma(struct dma_fence *fence)
{
	return fence->ops == &hw_fence_dbg_ops;
}

#endif /* __HW_FENCE_DRV_HW_DMA_FENCE */
