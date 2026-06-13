/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

@VintfStability
@Backing(type="int")
enum AONServiceType {
    /**
     * Face Detect
     */
    FaceDetect = 0,

    /**
     * Face Detect Pro
     */
    FaceDetectPro = 1,

    /**
     * QR Code Detect
     */
    QRCode = 2,

    /**
     * Hand Detect
     */
    HandDetect = 3,

    /**
     * Vendor defined events start
     */
    VendorDefinedStart = 0x1000000,

    /**
     * Add vendor defined events here
     *
     *
     * Vendor defined event End
     */
    VendorDefinedEnd,
}
