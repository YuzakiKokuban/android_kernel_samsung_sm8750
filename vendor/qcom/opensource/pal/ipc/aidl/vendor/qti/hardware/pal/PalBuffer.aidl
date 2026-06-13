/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalExternAllocBuffInfo;
import vendor.qti.hardware.pal.TimeSpec;

/**
 * PAL buffer structure used for reading/writing buffers from/to the stream
 */
@VintfStability
parcelable PalBuffer {
    byte[] buffer;
    int size;
    int offset;
    TimeSpec timeStamp;
    int flags;
    PalExternAllocBuffInfo allocInfo;
    long frameIndex;
}
