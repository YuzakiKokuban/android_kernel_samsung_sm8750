/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

@VintfStability
parcelable PalCallbackBufferInfo {
    long frameIndex;
    int sampleRate;
    int bitwidth;
    char channelCount;
}
