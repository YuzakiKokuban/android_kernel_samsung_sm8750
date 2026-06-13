// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/slab.h>
#include <synx_api.h>
#include <synx_hwfence.h>
#include <synx_interop.h>
#include "msm_hw_fence.h"
#include "hw_fence_drv_priv.h"
#include "hw_fence_drv_utils.h"
#include "hw_fence_drv_debug.h"
#include "hw_fence_drv_interop.h"

/**
 * MAX_SUPPORTED_DPU0: Maximum number of dpu clients supported
 * MAX_SUPPORTED_TEST: Maximum number of validation clients supported
 */
#define MAX_SUPPORTED_DPU0 (HW_FENCE_CLIENT_ID_CTL5 - HW_FENCE_CLIENT_ID_CTL0)
#define MAX_SUPPORTED_TEST (HW_FENCE_CLIENT_ID_VAL6 - HW_FENCE_CLIENT_ID_VAL0)

static enum hw_fence_client_id _get_hw_fence_client_id(enum synx_client_id synx_client_id)
{
	enum hw_fence_client_id hw_fence_client_id;

	switch ((int)synx_client_id) {
	case SYNX_CLIENT_HW_FENCE_GFX_CTX0:
		hw_fence_client_id = HW_FENCE_CLIENT_ID_CTX0;
		break;
	case SYNX_CLIENT_HW_FENCE_IPE_CTX0 ... SYNX_CLIENT_HW_FENCE_IPE_CTX0 +
			SYNX_MAX_SIGNAL_PER_CLIENT - 1:
		hw_fence_client_id = synx_client_id - SYNX_CLIENT_HW_FENCE_IPE_CTX0 +
			HW_FENCE_CLIENT_ID_IPE;
		break;
	case SYNX_CLIENT_HW_FENCE_VID_CTX0 ... SYNX_CLIENT_HW_FENCE_VID_CTX0 +
			SYNX_MAX_SIGNAL_PER_CLIENT - 1:
		hw_fence_client_id = synx_client_id - SYNX_CLIENT_HW_FENCE_VID_CTX0 +
			HW_FENCE_CLIENT_ID_VPU;
		break;
	case SYNX_CLIENT_HW_FENCE_DPU0_CTL0 ... SYNX_CLIENT_HW_FENCE_DPU0_CTL0 + MAX_SUPPORTED_DPU0:
		hw_fence_client_id = synx_client_id - SYNX_CLIENT_HW_FENCE_DPU0_CTL0 +
			HW_FENCE_CLIENT_ID_CTL0;
		break;
	case SYNX_CLIENT_HW_FENCE_IPA_CTX0 ... SYNX_CLIENT_HW_FENCE_IPA_CTX0 +
			SYNX_MAX_SIGNAL_PER_CLIENT - 1:
		hw_fence_client_id = synx_client_id - SYNX_CLIENT_HW_FENCE_IPA_CTX0 +
			HW_FENCE_CLIENT_ID_IPA;
		break;
	case SYNX_CLIENT_HW_FENCE_IFE0_CTX0 ... SYNX_CLIENT_HW_FENCE_IFE11_CTX0 +
			SYNX_MAX_SIGNAL_PER_CLIENT - 1:
		hw_fence_client_id = synx_client_id - SYNX_CLIENT_HW_FENCE_IFE0_CTX0 +
			HW_FENCE_CLIENT_ID_IFE0;
		break;
	case SYNX_CLIENT_HW_FENCE_TEST_CTX0 ... SYNX_CLIENT_HW_FENCE_TEST_CTX0 + MAX_SUPPORTED_TEST:
		hw_fence_client_id = synx_client_id - SYNX_CLIENT_HW_FENCE_TEST_CTX0 +
			HW_FENCE_CLIENT_ID_VAL0;
		break;
	default:
		HWFNC_ERR("Unsupported hw-fence client for synx_id:%d\n", synx_client_id);
		hw_fence_client_id = HW_FENCE_CLIENT_MAX;
		break;
	}

	return hw_fence_client_id;
}

static bool is_hw_fence_client(enum synx_client_id synx_client_id)
{
	return synx_client_id >= SYNX_HW_FENCE_CLIENT_START
		&& synx_client_id < SYNX_HW_FENCE_CLIENT_END;
}

struct synx_session *synx_hwfence_initialize(struct synx_initialization_params *params)
{
	struct synx_session *session = NULL;
	enum hw_fence_client_id client_id;
	void *client_handle;

	if (!hw_fence_driver_enable)
		return ERR_PTR(-SYNX_INVALID);

	if (IS_ERR_OR_NULL(params)) {
		HWFNC_ERR("invalid params:0x%pK\n", params);
		return ERR_PTR(-SYNX_INVALID);
	}

	client_id = _get_hw_fence_client_id(params->id);
	if (!is_hw_fence_client(params->id) || client_id == HW_FENCE_CLIENT_MAX) {
		HWFNC_ERR("Initializing session for invalid synx_id:%d\n", params->id);
		return ERR_PTR(-SYNX_INVALID);
	}

	session = kzalloc(sizeof(struct synx_session), GFP_KERNEL);
	if (!session)
		return ERR_PTR(-SYNX_NOMEM);

	client_handle = msm_hw_fence_register(client_id,
		(struct msm_hw_fence_mem_addr *)params->ptr);
	if (IS_ERR_OR_NULL(client_handle)) {
		kfree(session);
		HWFNC_ERR("failed to initialize synx_id:%d ret:%ld\n", params->id,
			PTR_ERR(client_handle));
		return ERR_PTR(hw_fence_interop_to_synx_status(PTR_ERR(client_handle)));
	}
	session->client = client_handle;
	session->type = params->id;
	HWFNC_DBG_INIT("initialized session synx_id:%d hw_fence_id:%d\n", params->id, client_id);

	return session;
}
EXPORT_SYMBOL_GPL(synx_hwfence_initialize);

static int synx_hwfence_uninitialize(struct synx_session *session)
{
	int ret;

	if (IS_ERR_OR_NULL(session) || !is_hw_fence_client(session->type)) {
		HWFNC_ERR("invalid session:0x%pK synx_id:%d\n", session,
			IS_ERR_OR_NULL(session) ? -1 : session->type);
		return -SYNX_INVALID;
	}

	ret = msm_hw_fence_deregister(session->client);
	if (ret)
		HWFNC_ERR("Failed to deregister synx_id:%d ret:%d\n", session->type, ret);
	else
		kfree(session);

	return hw_fence_interop_to_synx_status(ret);
}

static int synx_hwfence_create(struct synx_session *session, struct synx_create_params *params)
{
	int ret = 0;
	struct msm_hw_fence_create_params hwfence_params;
	u64 handle;

	if (IS_ERR_OR_NULL(session) || !is_hw_fence_client(session->type) ||
			IS_ERR_OR_NULL(params)) {
		HWFNC_ERR("invalid session:0x%pK synx_id:%d params:0x%pK\n", session,
			IS_ERR_OR_NULL(session) ? -1 : session->type, params);
		return -SYNX_INVALID;
	}

	if (IS_ERR_OR_NULL(params->h_synx) || (params->flags > SYNX_CREATE_MAX_FLAGS) ||
			(params->flags & SYNX_CREATE_CSL_FENCE)) {
		HWFNC_ERR("synx_id:%d invalid create params h_synx:0x%pK flags:0x%x\n",
			session->type, params->h_synx, params->flags);
		return -SYNX_INVALID;
	}

	/* if SYNX_CREATE_DMA_FENCE specified and no dma-fence, fail */
	if (!params->fence && (params->flags & SYNX_CREATE_DMA_FENCE)) {
		HWFNC_ERR("synx_id:%d invalid fence:%pK params flags:0x%x\n",
			session->type, params->fence, params->flags);
		return -SYNX_INVALID;
	}

	hwfence_params.fence = params->fence;
	hwfence_params.handle = &handle;
	ret = msm_hw_fence_create(session->client, &hwfence_params);
	if (ret) {
		HWFNC_ERR("synx_id:%d failed create fence:0x%pK flags:0x%x ret:%d\n", session->type,
			params->fence, params->flags, ret);
		return hw_fence_interop_to_synx_status(ret);
	}
	if (handle > U32_MAX) {
		HWFNC_ERR("synx_id:%d fence handle:%llu would overflow h_synx\n", session->type,
			handle);
		hw_fence_destroy_refcount(hw_fence_drv_data, handle, HW_FENCE_FCTL_REFCOUNT);
		msm_hw_fence_destroy_with_handle(session->client, handle);
		return -SYNX_INVALID;
	}
	*params->h_synx = SYNX_HW_FENCE_HANDLE_FLAG | handle;

	return SYNX_SUCCESS;
}

static int synx_hwfence_release(struct synx_session *session, u32 h_synx)
{
	int ret;

	if (IS_ERR_OR_NULL(session) || !is_hw_fence_client(session->type) ||
			!(h_synx & SYNX_HW_FENCE_HANDLE_FLAG)) {
		HWFNC_ERR("invalid session:0x%pK synx_id:%d\n", session,
			IS_ERR_OR_NULL(session) ? -1 : session->type);
		return -SYNX_INVALID;
	}

	h_synx &= HW_FENCE_HANDLE_INDEX_MASK;
	ret = msm_hw_fence_destroy_with_handle(session->client, h_synx);
	if (ret)
		HWFNC_ERR("synx_id:%d failed to destroy fence h_synx:%u ret:%d\n", session->type,
			h_synx, ret);

	return hw_fence_interop_to_synx_status(ret);
}

static int synx_hwfence_signal(struct synx_session *session, u32 h_synx,
	enum synx_signal_status status)
{
	struct msm_hw_fence_client *hw_fence_client;
	u32 error;
	int ret;

	if (IS_ERR_OR_NULL(session) || !is_hw_fence_client(session->type) || !session->client ||
			!(h_synx & SYNX_HW_FENCE_HANDLE_FLAG) ||
			!(status == SYNX_STATE_SIGNALED_SUCCESS ||
			status == SYNX_STATE_SIGNALED_CANCEL ||
			status > SYNX_STATE_SIGNALED_MAX)) {
		HWFNC_ERR("invalid session:0x%pK synx_id:%d client:0x%pK h_synx:%u status:%u\n",
			session, IS_ERR_OR_NULL(session) ? -1 : session->type,
			IS_ERR_OR_NULL(session) ? NULL : session->client, h_synx, status);
		return -SYNX_INVALID;
	}

	error = hw_fence_interop_to_hw_fence_error(status);
	h_synx &= HW_FENCE_HANDLE_INDEX_MASK;
	ret = msm_hw_fence_update_txq(session->client, h_synx, 0, error);
	if (ret) {
		HWFNC_ERR("synx_id:%d failed to signal fence h_synx:%u status:%d ret:%d\n",
			session->type, h_synx, status, ret);
		goto error;
	}

	hw_fence_client = (struct msm_hw_fence_client *)session->client;
	if (hw_fence_client->txq_update_send_ipc)
		hw_fence_ipcc_trigger_signal(hw_fence_drv_data,
			hw_fence_client->ipc_client_pid, hw_fence_drv_data->ipcc_fctl_vid,
			hw_fence_client->ipc_signal_id);

error:
	return hw_fence_interop_to_synx_status(ret);
}

static int synx_hwfence_wait(struct synx_session *session, u32 h_synx, u64 timeout_ms)
{
	int ret = -EINVAL;
	u32 error;

	if (IS_ERR_OR_NULL(session) || !is_hw_fence_client(session->type) ||
			!(h_synx & SYNX_HW_FENCE_HANDLE_FLAG)) {
		HWFNC_ERR("invalid session:0x%pK synx_id:%d\n", session,
			IS_ERR_OR_NULL(session) ? -1 : session->type);
		return -SYNX_INVALID;
	}

#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (session->type >= SYNX_CLIENT_HW_FENCE_TEST_CTX0
			&& session->type <= SYNX_CLIENT_HW_FENCE_TEST_CTX0 + MAX_SUPPORTED_TEST)
		ret = hw_fence_debug_wait_val(hw_fence_drv_data, session->client, NULL, h_synx,
			HW_FENCE_HANDLE_INDEX_MASK, timeout_ms, &error);
#endif /* CONFIG_DEBUG_FS */

	if (ret) {
		HWFNC_ERR("synx_id:%d failed to wait on fence h_synx:%u timeout_ms:%llu\n",
			session->type, h_synx, timeout_ms);
		return hw_fence_interop_to_synx_status(ret);
	}

	return hw_fence_interop_to_synx_signal_status(MSM_HW_FENCE_FLAG_SIGNAL, error);
}

int synx_hwfence_recover(enum synx_client_id id)
{
	int ret;

	if (!is_hw_fence_client(id)) {
		HWFNC_ERR("invalid synx_id:%d\n", id);
		return -SYNX_INVALID;
	}

	ret = msm_hw_fence_reset_client_by_id(_get_hw_fence_client_id(id),
		MSM_HW_FENCE_RESET_WITHOUT_DESTROY);
	if (ret)
		HWFNC_ERR("synx_id:%d failed to recover ret:%d\n", id, ret);

	return hw_fence_interop_to_synx_status(ret);
}
EXPORT_SYMBOL_GPL(synx_hwfence_recover);

static void *synx_hwfence_get_fence(struct synx_session *session, u32 h_synx)
{
	if (IS_ERR_OR_NULL(session) || !is_hw_fence_client(session->type) ||
			!(h_synx & SYNX_HW_FENCE_HANDLE_FLAG)) {
		HWFNC_ERR("invalid session:0x%pK synx_id:%d h_synx:%u\n", session,
			IS_ERR_OR_NULL(session) ? -1 : session->type, h_synx);
		return ERR_PTR(-SYNX_INVALID);
	}

	return (void *)hw_fence_interop_get_fence(h_synx);
}

static int synx_hwfence_get_status(struct synx_session *session, u32 h_synx)
{
	u64 flags;
	u32 error;
	int ret;

	if (IS_ERR_OR_NULL(session) || !is_hw_fence_client(session->type) ||
			!(h_synx & SYNX_HW_FENCE_HANDLE_FLAG)) {
		HWFNC_ERR("invalid session:0x%pK synx_id:%d h_synx:%u\n", session,
			IS_ERR_OR_NULL(session) ? -1 : session->type, h_synx);
		return SYNX_STATE_INVALID;
	}

	h_synx &= HW_FENCE_HANDLE_INDEX_MASK;
	ret = hw_fence_get_flags_error(hw_fence_drv_data, h_synx, &flags, &error);
	if (ret) {
		HWFNC_ERR("Failed to get status for client:%d h_synx:%u\n", session->type, h_synx);
		return SYNX_STATE_INVALID;
	}

	return hw_fence_interop_to_synx_signal_status(flags, error);
}

static int synx_hwfence_import_fence(void *client, struct synx_import_indv_params *params)
{
	struct dma_fence_array *array;
	struct dma_fence *fence;
	u64 handle;
	int ret, i;

	ret = hw_fence_check_valid_fctl(hw_fence_drv_data, client);
	if (ret)
		return hw_fence_interop_to_synx_status(ret);

	fence = (struct dma_fence *)params->fence;
	array = to_dma_fence_array(fence);
	if (array) {
		for (i = 0; i < array->num_fences; i++) {
			if (dma_fence_is_array(array->fences[i])) {
				HWFNC_ERR("nested fence arrays not supported idx:%d fence:0x%pK\n",
					i, array->fences[i]);
				ret = -SYNX_INVALID;
				break;
			}

			params->fence = array->fences[i];
			ret = hw_fence_interop_create_fence_from_import(params);
			if (ret) {
				HWFNC_ERR("failed to back dma_fence_array idx:%d fence:0x%pK\n",
					i, array->fences[i]);
				params->fence = fence;
				break;
			}
		}
		params->fence = fence;
	} else {
		ret = hw_fence_interop_create_fence_from_import(params);
	}

	if (ret) {
		HWFNC_ERR("failed to back dma-fence:0x%pK with hw-fence(s) ret:%d\n",
			params->fence, ret);
		return ret;
	}

	ret = msm_hw_fence_wait_update_v2(client, (struct dma_fence **)&params->fence, &handle,
		NULL, 1, true);
	if (ret) {
		HWFNC_ERR("failed to import fence:0x%pK flags:0x%x ret:%d\n", params->fence,
			params->flags, ret);
		goto error;
	}
	if (handle > U32_MAX) {
		HWFNC_ERR("fence handle:%llu would overflow new_h_synx\n", handle);
		msm_hw_fence_wait_update_v2(client, (struct dma_fence **)&params->fence, &handle,
			NULL, 1, false);
		return -SYNX_INVALID;
	}
	*params->new_h_synx = SYNX_HW_FENCE_HANDLE_FLAG | handle;

error:
	return hw_fence_interop_to_synx_status(ret);
}

static int synx_hwfence_import_handle(void *client, struct synx_import_indv_params *params)
{
	struct synx_import_indv_params fence_params;
	u32 h_synx;
	int ret;

	if (!synx_interops.get_fence) {
		HWFNC_ERR("invalid synx_get_fence:0x%pK\n", synx_interops.get_fence);
		return -SYNX_INVALID;
	}
	h_synx = *(u32 *)params->fence;
	if (h_synx & SYNX_HW_FENCE_HANDLE_FLAG)
		fence_params.fence = hw_fence_interop_get_fence(h_synx);
	else
		fence_params.fence = synx_interops.get_fence(h_synx);
	if (IS_ERR_OR_NULL(fence_params.fence)) {
		HWFNC_ERR("failed to get native fence h_synx:%u ret:0x%pK\n", h_synx,
			fence_params.fence);
		return -SYNX_INVALID;
	}
	fence_params.new_h_synx = params->new_h_synx;
	fence_params.flags = SYNX_IMPORT_DMA_FENCE;
	ret = synx_hwfence_import_fence(client, &fence_params);
	dma_fence_put(fence_params.fence); /* release dma-fence ref acquired by get_fence */

	return ret;
}

static int synx_hwfence_import_indv(void *client, struct synx_import_indv_params *params)
{
	if (IS_ERR_OR_NULL(client) || IS_ERR_OR_NULL(params) ||
			IS_ERR_OR_NULL(params->new_h_synx) ||
			!((params->flags & SYNX_IMPORT_DMA_FENCE) ||
			(params->flags & SYNX_IMPORT_SYNX_FENCE)) ||
			IS_ERR_OR_NULL(params->fence)) {
		HWFNC_ERR("invalid client:0x%pK params:0x%pK h_synx:0x%pK flags:0x%x fence:0x%pK\n",
			client, params, IS_ERR_OR_NULL(params) ? NULL : params->new_h_synx,
			IS_ERR_OR_NULL(params) ? 0 : params->flags,
			IS_ERR_OR_NULL(params) ? NULL : params->fence);
		return -SYNX_INVALID;
	}

	if (params->flags & SYNX_IMPORT_DMA_FENCE)
		return synx_hwfence_import_fence(client, params);
	else if (params->flags & SYNX_IMPORT_SYNX_FENCE)
		return synx_hwfence_import_handle(client, params);

	HWFNC_ERR("invalid import flags:0x%x\n", params->flags);

	return -SYNX_INVALID;
}

static int synx_hwfence_import_arr(void *client, struct synx_import_arr_params *params)
{
	int i, ret;

	if (IS_ERR_OR_NULL(client) || IS_ERR_OR_NULL(params) || !params->num_fences) {
		HWFNC_ERR("invalid import arr client:0x%pK params:0x%pK num_fences:%u\n", client,
			params, IS_ERR_OR_NULL(params) ? -1 : params->num_fences);
		return -SYNX_INVALID;
	}

	for (i = 0; i < params->num_fences; i++) {
		ret = synx_hwfence_import_indv(client, &params->list[i]);
		if (ret) {
			HWFNC_ERR("importing fence[%u] 0x%pK failed ret:%d\n", i,
				params->list[i].fence, ret);
			return ret;
		}
	}

	return SYNX_SUCCESS;
}

int synx_hwfence_import(struct synx_session *session, struct synx_import_params *params)
{
	int ret;

	if (IS_ERR_OR_NULL(session) || !is_hw_fence_client(session->type)
			|| IS_ERR_OR_NULL(params)) {
		HWFNC_ERR("invalid session:0x%pK synx_id:%d params:0x%pK\n", session,
			IS_ERR_OR_NULL(session) ? -1 : session->type, params);
		return -SYNX_INVALID;
	}

	if (params->type == SYNX_IMPORT_ARR_PARAMS)
		ret = synx_hwfence_import_arr(session->client, &params->arr);
	else
		ret = synx_hwfence_import_indv(session->client, &params->indv);

	if (ret)
		HWFNC_ERR("synx_id:%d failed to import type:%s fences ret:%d\n", session->type,
			(params->type == SYNX_IMPORT_ARR_PARAMS) ? "arr" : "indv", ret);

	return ret;
}

int synx_hwfence_init_ops(struct synx_ops *hwfence_ops)
{
	if (IS_ERR_OR_NULL(hwfence_ops)) {
		HWFNC_ERR("invalid ops\n");
		return -SYNX_INVALID;
	}

	hwfence_ops->uninitialize = synx_hwfence_uninitialize;
	hwfence_ops->create = synx_hwfence_create;
	hwfence_ops->release = synx_hwfence_release;
	hwfence_ops->signal = synx_hwfence_signal;
	hwfence_ops->import = synx_hwfence_import;
	hwfence_ops->get_fence = synx_hwfence_get_fence;
	hwfence_ops->get_status = synx_hwfence_get_status;
	hwfence_ops->wait = synx_hwfence_wait;

	return SYNX_SUCCESS;
}
EXPORT_SYMBOL_GPL(synx_hwfence_init_ops);

int synx_hwfence_enable_resources(enum synx_client_id id, enum synx_resource_type resource,
	bool enable)
{
	int ret;

	if (!hw_fence_driver_enable)
		return -SYNX_INVALID;

	if (IS_ERR_OR_NULL(hw_fence_drv_data) || !hw_fence_drv_data->resources_ready) {
		HWFNC_ERR("hw fence driver not ready\n");
		return -SYNX_INVALID;
	}

	if (!is_hw_fence_client(id) || !(resource == SYNX_RESOURCE_SOCCP)) {
		HWFNC_ERR("enabling hw-fence resources for invalid client id:%d res:%d enable:%d\n",
			id, resource, enable);
		return -SYNX_INVALID;
	}

	if (!hw_fence_drv_data->has_soccp)
		return SYNX_SUCCESS;

	ret = hw_fence_utils_set_power_vote(hw_fence_drv_data, enable);
	if (ret)
		HWFNC_ERR("Failed to vote for SOCCP state:%d\n", enable);

	return hw_fence_interop_to_synx_status(ret);
}
EXPORT_SYMBOL_GPL(synx_hwfence_enable_resources);
