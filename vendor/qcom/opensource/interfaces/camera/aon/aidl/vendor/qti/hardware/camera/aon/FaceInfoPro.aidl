/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.FaceInfoProPerFace;

/**
 * The information passed from the event of FaceDetectPro service type
 */
@VintfStability
parcelable FaceInfoPro {
    /**
     * Width of the frame dimension where the reported face ROI/Parts Coordinates will be based on
     */
    int frameDimWidth;

    /**
     * Height of the frame dimension where the reported face ROI/Parts Coordinates will be based on
     */
    int frameDimHeight;

    /**
     * A vector of the per face information
     */
    FaceInfoProPerFace[] perFace;
}
