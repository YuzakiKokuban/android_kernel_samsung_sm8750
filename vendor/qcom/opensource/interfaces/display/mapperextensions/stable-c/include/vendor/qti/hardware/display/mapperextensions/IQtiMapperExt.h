/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

#include <cutils/native_handle.h>

__BEGIN_DECLS

/**
 * IQtiMapperExt versioning
 *
 * IQtiMapperExt version 1 is HIDL
 * C-style IQtiMapperExt API starts at 2
 */
enum IQtiMapperExt_Version : uint32_t {
    IQTIMAPPEREXT_VERSION_2 = 2,
};

enum IQtiMapperExt_Error : int32_t {
    /**
     * No error.
     */
    IQTIMAPPEREXT_ERROR_NONE = 0,
    /**
     * Invalid BufferDescriptor.
     */
    IQTIMAPPEREXT_ERROR_BAD_DESCRIPTOR = 1,
    /**
     * Invalid buffer handle.
     */
    IQTIMAPPEREXT_ERROR_BAD_BUFFER = 2,
    /**
     * Invalid HardwareBufferDescription.
     */
    IQTIMAPPEREXT_ERROR_BAD_VALUE = 3,
    /**
     * Resource unavailable.
     */
    IQTIMAPPEREXT_ERROR_NO_RESOURCES = 5,
    /**
     * Permanent failure.
     */
    IQTIMAPPEREXT_ERROR_UNSUPPORTED = 7,
};

typedef struct IQtiMapperExtV2 {
    /**
     * Get view bit masks of all views included in this handle.
     */
    IQtiMapperExt_Error (*_Nonnull getMultiViewInfo)(buffer_handle_t _Nonnull buffer,
                                                     uint32_t* _Nonnull views);

    /**
     * Get view bit mask for the base view of current buffer
     */
    IQtiMapperExt_Error (*_Nonnull getBaseView)(buffer_handle_t _Nonnull buffer,
                                                uint32_t* _Nonnull view);

    /**
     * Imports a specific view buffer from a imported meta handle to create an imported buffer
     * specific for the view only.
     *
     * @param metaHandle Meta handle which has already been imported by importBuffer
     * @param view View of the meta buffer to import.
     * @param outBufferHandle The resulting imported buffer handle for the specific view
     * @return error Error status of the call, which may be
     *     - `NONE` upon success.
     *     - `BAD_BUFFER` if the raw handle is invalid.
     *     - `BAD_VALUE` if the view id is not valid in the meta handle.
     *     - `NO_RESOURCES` if the raw handle cannot be imported due to
     *       unavailability of resources.
     */
    IQtiMapperExt_Error (*_Nonnull importViewBuffer)(
            buffer_handle_t _Nonnull metaHandle, uint32_t view,
            buffer_handle_t _Nullable* _Nonnull outBufferHandle);
} IQtiMapperExtV2;

/**
 * Return value for IQtiMapperExt_loadIMapperExt
 *
 * Note: This struct's size is not fixed and callers must never store it by-value as a result.
 *       Only fields up to those covered by `version` are allowed to be accessed.
 */
typedef struct IQtiMapperExt {
    alignas(alignof(max_align_t)) IQtiMapperExt_Version version;
    IQtiMapperExtV2 v2;
} IQtiMapperExt;

/**
 * Loads the vendor-provided implementation of IQtiMapperExt
 * @return Error status of the call.
 *          - `NONE` upon success
 *          - `UNSUPPORTED` if no implementation is available
 */
IQtiMapperExt_Error IQtiMapperExt_loadIMapperExt(
        IQtiMapperExt* _Nullable* _Nonnull outImplementation);

__END_DECLS