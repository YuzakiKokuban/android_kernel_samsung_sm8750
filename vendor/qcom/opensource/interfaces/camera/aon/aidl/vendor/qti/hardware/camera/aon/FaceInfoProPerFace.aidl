/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.FacePosType;

/**
 * The information per face passed from the event of FaceDetectPro service type
 */
@VintfStability
parcelable FaceInfoProPerFace {
    /**
     * Indicates the roll (or rotate) angle of the face
     * Valid values: -180 through 179, where a positive value means the face is
     * rotated clockwise in-plane
     */
    int angleRoll;
    /**
     * Indicates the left/right direction of the face
     * Valid values: -180 through 179, where a positive value means the face is
     * facing right
     */
    int angleYaw;
    /**
     * Width of the face in the frame
     */
    int width;
    /**
     * height of the face in the frame
     */
    int height;
    /**
     * Position of the center of the face in the frame
     */
    FacePosType center;
    /**
     * Whether an eye gaze is detected
     * Only applicable when the bit of GazeDetected or GazeNotDetected
     * is set to 1 by client in RegisterClient.
     */
    boolean isGazeDetected;
}
