/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

/**
 * The information passed from the event of FaceDetectPro service type
 */
@VintfStability
parcelable FacePosType {
    /**
     * X coordinate of a position
     * It is relative to the frame dimension width (exposed in FaceInfoPro)
     * It can be negative (e.g. a facial part can be estimated to be outside of the
     * frame boundary)
     */
    int x;
    /**
     * Y coordinate of a position
     * It is relative to the frame dimension height (exposed in FaceInfoPro)
     * It can be negative (e.g. a facial part can be estimated to be outside of the
     * frame boundary)
     */
    int y;
}
