/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

@VintfStability
@Backing(type="int")
enum PalDrainType {
    /**
     * request notification when all accumlated data has be
     *  drained.
     */
    PAL_DRAIN,
    /**
     * request notification when drain completes shortly before all
     *  accumlated data of the current track has been played out
     */
    PAL_DRAIN_PARTIAL,
}
