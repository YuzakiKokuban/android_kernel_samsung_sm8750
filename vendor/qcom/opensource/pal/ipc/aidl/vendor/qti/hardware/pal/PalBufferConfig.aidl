/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

@VintfStability
parcelable PalBufferConfig {
    int bufCount;
    /**
     * < number of buffers
     */
    int bufSize;
    /**
     * < This would be the size of each buffer
     */
    int maxMetadataSize;
}
