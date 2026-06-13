/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalCallbackBufferInfo;
import vendor.qti.hardware.pal.TimeSpec;

@VintfStability
parcelable PalCallbackBuffer {
    byte[] buffer;
    int size;
    TimeSpec timeStamp;
    int status;
    PalCallbackBufferInfo cbBufInfo;
}
