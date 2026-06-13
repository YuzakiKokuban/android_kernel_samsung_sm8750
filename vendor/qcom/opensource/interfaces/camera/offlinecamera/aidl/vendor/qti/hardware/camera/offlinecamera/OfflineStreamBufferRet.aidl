/*
 * {Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 * All rights reserved. SPDX-License-Identifier: BSD-3-Clause-Clear}
 */


package vendor.qti.hardware.camera.offlinecamera;

import android.hardware.camera.device.Stream;
import android.hardware.camera.device.StreamBuffersVal;

@VintfStability
parcelable OfflineStreamBufferRet {

    Stream              streamInfo;

    StreamBuffersVal    val;
}
