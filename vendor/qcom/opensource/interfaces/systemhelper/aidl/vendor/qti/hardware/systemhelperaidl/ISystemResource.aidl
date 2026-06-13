/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.systemhelperaidl;

import vendor.qti.hardware.systemhelperaidl.SystemResourceType;
import android.hardware.common.NativeHandle;

@VintfStability
interface ISystemResource {
    /**
     * Acquire a system resource
     *
     * @param resource resource to acquire
     * @return resourceId unique resource identifier for acquired resource
     *                    Note : resourceId stands invalid in case of API returned error
     */
    int acquire(in SystemResourceType resource);

    /**
     * Acquire a system graphics surface.
     *
     * @param width surface width
     * @param height surface height
     * @param out handle NativeHandle wrapper for IGraphicBufferProducer interface
     * @return resourceId unique resource identifier for acquired resource
     *                    Note : resourceId stands invalid in case of API returned error
     */
    int acquireSurface(in int width, in int height, out NativeHandle Handle);

    /**
     * Release an acquired resource
     *
     * Both resource and surface shall be released via this API.
     *
     * @param resourceId unique resource identifier to be released
     */
    void release(in int resourceId);
}