/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.AONServiceType;
import vendor.qti.hardware.camera.aon.FDRegisterInfo;
import vendor.qti.hardware.camera.aon.QRRegisterInfo;
import vendor.qti.hardware.camera.aon.HDRegisterInfo;

/**
 * The register information filled by client
 */
@VintfStability
parcelable AONRegisterInfo {
    /**
     * Index of to the sensorInfoList returned in GetAONSensorInfoList
     * Indicate which aon sensor the client would like to register
     */
    int aonIdx;

    /**
     * Indicate which AONServiceType the client would like to register after knowing
     * the capability of each AON sensor by parsing the AONSensorCap in sensorInfoList
     */
    AONServiceType srvType;

    /**
     * The FD register information filled by client.
     * Only applicable when service type is FaceDetect or FaceDetectPro
     */
    FDRegisterInfo fdRegInfo;

    /**
     * The QRCode register information filled by client.
     * Only applicable when service type is QRCode
     */
    QRRegisterInfo qrRegInfo;

    /**
     * The HD register information filled by client.
     * Only applicable when service type is HandDetect
     */
    @nullable HDRegisterInfo hdRegInfo;
}
