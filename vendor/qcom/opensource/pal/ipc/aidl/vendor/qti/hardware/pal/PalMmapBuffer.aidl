/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalMmapBufferFlags;

/**
 * PAL buffer structure used for reading/writing buffers from/to the stream
 */
@VintfStability
parcelable PalMmapBuffer {
    long buffer;
    /**
     * < base address of mmap memory buffer,
     * for use by local proces only
     */
    int fd;
    /**
     * < fd for mmap memory buffer
     */
    int bufferSizeFrames;
    /**
     * < total buffer size in frames
     */
    int burstSizeFrames;
    /**
     * < transfer size granularity in frames
     */
    PalMmapBufferFlags flags;
}
