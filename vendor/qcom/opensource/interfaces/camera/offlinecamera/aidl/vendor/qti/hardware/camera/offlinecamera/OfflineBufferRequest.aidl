/*
 * {Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 * All rights reserved. SPDX-License-Identifier: BSD-3-Clause-Clear}
 */


package vendor.qti.hardware.camera.offlinecamera;

import android.hardware.camera.device.Stream;

@VintfStability
parcelable OfflineBufferRequest {

    Stream              streamInfo;

    int                 numBuffersRequested;
}
