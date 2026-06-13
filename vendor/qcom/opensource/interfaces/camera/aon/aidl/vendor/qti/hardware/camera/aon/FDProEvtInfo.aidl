/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.FaceInfoPro;

/**
 * The event information for FaceDetectPro AONServiceType
 */
@VintfStability
parcelable FDProEvtInfo {
    /**
     * Bit Mask to indicate the FDEvtTypes of this event.
     * If fdEvtTypeMask is 0x5(0x1|0x4), that means both
     * face & gaze are detected by AONService
     */
    int fdEvtTypeMask;
    /**
     * Face information of detected faces or gazes.
     * Only applicable when the BIT of FaceDetected or GazeDetected
     * in fdEvtTypeMask is being set by AONService.
     */
    FaceInfoPro faceInfo;
}
