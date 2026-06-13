/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear 
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.AONServiceType;
import vendor.qti.hardware.camera.aon.FDEvtInfo;
import vendor.qti.hardware.camera.aon.FDProEvtInfo;
import vendor.qti.hardware.camera.aon.QREvtInfo;
import vendor.qti.hardware.camera.aon.HDEvtInfo;

/**
 * The event callback from AON service to client
 */
@VintfStability
parcelable AONCallbackEvent {
    /**
     * Service type of the callback event.
     */
    AONServiceType srvType;

    /**
     * The event information for FaceDetect.
     * Only applicable when srvType is FaceDetect.
     */
    FDEvtInfo fdEvtInfo;

    /**
     * The event information for FaceDetectPro.
     * Only applicable when srvType is FaceDetectPro.
     */
    FDProEvtInfo fdProEvtInfo;

    /**
     * The event information for QRCode.
     * Only applicable when srvType is QRCode.
     */
    QREvtInfo qrEvtInfo;

    /**
     * The event information for HandDetect.
     * Only applicable when srvType is HandDetect.
     */
    @nullable HDEvtInfo hdEvtInfo;
}
