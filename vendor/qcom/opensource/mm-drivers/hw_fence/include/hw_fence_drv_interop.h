/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __HW_FENCE_INTEROP_H
#define __HW_FENCE_INTEROP_H

#include <synx_api.h>

extern struct hw_fence_driver_data *hw_fence_drv_data;
extern struct synx_hwfence_interops synx_interops;

/**
 * HW_FENCE_HANDLE_INDEX_MASK: Mask to extract table index from hw-fence handle
 */
#define HW_FENCE_HANDLE_INDEX_MASK GENMASK(16, 0)

/**
 * hw_fence_interop_to_synx_status() - Converts hw-fence status code to synx status code
 *
 * @param code  : hw-fence status code
 * @return synx status code corresponding to hw-fence status code
 */
int hw_fence_interop_to_synx_status(int hw_fence_status_code);

/**
 * hw_fence_interop_to_synx_signal_status() - Converts hw-fence flags and error to
 * synx signaling status
 *
 * @param flags  : hw-fence flags
 * @param error  : hw-fence error
 *
 * @return synx signaling status
 */
u32 hw_fence_interop_to_synx_signal_status(u32 flags, u32 error);

/**
 * hw_fence_interop_to_hw_fence_error() - Convert synx signaling status to hw-fence error
 *
 * @param status  : synx signaling status
 * @return hw-fence error
 */
u32 hw_fence_interop_to_hw_fence_error(u32 status);

/**
 * hw_fence_interop_create_fence_from_import() - Creates hw-fence if necessary during synx_import,
 * e.g. if there is no backing hw-fence for a synx fence.
 *
 * @param params  : pointer to import params
 * @return SYNX_SUCCESS upon success, -SYNX_INVALID if failed
 */
int hw_fence_interop_create_fence_from_import(struct synx_import_indv_params *params);

/**
 * hw_fence_interop_share_handle_status() - updates HW fence table with synx handle
 * (if not already signaled) and return hw-fence handle by populating params.new_h_synx
 * and returning signal status
 *
 * @param params  : pointer to import params
 * @param h_synx  : synx handle
 * @param signal_status: signalin status of fence
 *
 * @return SYNX_SUCCESS upon success, -SYNX_INVALID if failed
 */
int hw_fence_interop_share_handle_status(struct synx_import_indv_params *params, u32 h_synx,
	u32 *signal_status);

/**
 * hw_fence_interop_get_fence() – return the dma-fence associated with the given handle
 *
 * @param h_synx : hw-fence handle
 *
 * @return dma-fence associated with hw-fence handle. Null or error pointer in case of error.
 */
void *hw_fence_interop_get_fence(u32 h_synx);

#endif /* __HW_FENCE_INTEROP_H */
