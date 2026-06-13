/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.AONServiceType;
import vendor.qti.hardware.camera.aon.FDAlgoMode;

/**
 * The AONServiceType and corresponding algo mode list supported by AON sensor
 */
@VintfStability
parcelable AONSensorCap {
    /**
     * Supported AONServiceType by AON sensor
     */
    AONServiceType srvType;

    /**
     * The supported fdAlgoMode list. Only applicable when srvType is FaceDetect or FaceDetectPro
     */
    FDAlgoMode[] fdAlgoModes;
}
