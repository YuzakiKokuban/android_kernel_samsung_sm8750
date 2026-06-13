/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

/**
 * pal_mmap_buffer flags
 */
@VintfStability
@Backing(type="int")
enum PalMmapBufferFlags {
    PAL_MMMAP_BUFF_FLAGS_NONE = 0,
    /**
     * Only set this flag if applications can access the audio buffer memory
     * shared with the backend (usually DSP) _without_ security issue.
     *
     * Setting this flag also implies that Binder will allow passing the shared memory FD
     * to applications.
     *
     * That usually implies that the kernel will prevent any access to the
     * memory surrounding the audio buffer as it could lead to a security breach.
     *
     * For example, a "/dev/snd/" file descriptor generally is not shareable,
     * but an "anon_inode:dmabuffer" file descriptor is shareable.
     * See also Linux kernel's dma_buf.
     *
     */
    PAL_MMMAP_BUFF_FLAGS_APP_SHAREABLE = 1,
}
