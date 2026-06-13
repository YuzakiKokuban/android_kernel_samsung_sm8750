/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

/**
 * The event information for QRCode AONServiceType
 */
@VintfStability
parcelable QREvtInfo {
    /**
     * Bit Mask to indicate the QREvtTypes of this event.
     */
    int qrEvtTypeMask;
}
