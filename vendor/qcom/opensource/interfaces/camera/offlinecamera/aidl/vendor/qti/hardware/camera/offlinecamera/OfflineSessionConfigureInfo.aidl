/*
 * {Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 * All rights reserved. SPDX-License-Identifier: BSD-3-Clause-Clear}
 */



package vendor.qti.hardware.camera.offlinecamera;

import vendor.qti.hardware.camera.offlinecamera.OpMode;
import android.hardware.camera.device.Stream;
import android.hardware.camera.device.CameraMetadata;

@VintfStability
parcelable OfflineSessionConfigureInfo {
    String           cameraId;

    Stream[]         streams;

    /*not used in V2*/
    OpMode           mode;

    CameraMetadata   configureSetting;

    int              opModes;
}
