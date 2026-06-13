/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

/**
 * The event type supported by QRCode.
 * These are bit values for client to assign to the qrEvtTypeMask in QRRegisterInfo during RegisterClient
 * and to check the qrEvtTypeMask in QREvtInfo when receiving the AONCallbackEvent.
 */
@VintfStability
@Backing(type="int")
enum QREvtType {
    /**
     * This indicates that QRCode detection was performed and a QRCode is detected.
     */
    QRCodeDetected = 1,

    /**
     * This indicates that QRCode detection was performed and a QRCode is not detected.
     */
    QRCodeNotDetected = 2,
}
