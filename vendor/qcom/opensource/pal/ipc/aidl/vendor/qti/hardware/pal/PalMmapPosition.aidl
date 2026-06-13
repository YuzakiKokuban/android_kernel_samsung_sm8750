/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

/**
 * Mmap buffer read/write position returned by GetMmapPosition.
 * note\ Used by streams opened in mmap mode.
 */
@VintfStability
parcelable PalMmapPosition {
    long timeNanoseconds;
    /**
     * < timestamp in ns, CLOCK_MONOTONIC
     */
    int positionFrames;
}
