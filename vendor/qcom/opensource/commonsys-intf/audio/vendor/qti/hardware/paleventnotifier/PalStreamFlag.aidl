/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

/**
 * Available stream flags of an audio session
 */
@VintfStability
@Backing(type="int")
enum PalStreamFlag {
    PAL_STREAM_FLAG_TIMESTAMP = 0x1,
    PAL_STREAM_FLAG_NON_BLOCKING = 0x2,
    PAL_STREAM_FLAG_MMAP = 0x4,
    PAL_STREAM_FLAG_MMAP_NO_IRQ = 0x8,
    PAL_STREAM_FLAG_EXTERN_MEM = 0x10,
    PAL_STREAM_FLAG_SRCM_INBAND = 0x20,
    PAL_STREAM_FLAG_INVALID,
}
