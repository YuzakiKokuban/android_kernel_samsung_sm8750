/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

@VintfStability
parcelable PalStreamInfo {
    long version;
    long size;
    long durationUs;
    boolean hasVideo;
    int txProxyType;
    int rxProxyType;
    boolean isStreaming;
    int loopbackType;
    int hapticsType;
}
