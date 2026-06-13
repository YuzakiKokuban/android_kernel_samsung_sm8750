/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
package vendor.qti.hardware.camera.aon;
@VintfStability
parcelable HandInfoPerHand {
    int confidence;      
    int width;
    int height;
    vendor.qti.hardware.camera.aon.HandPosType topLeftCorner;
    vendor.qti.hardware.camera.aon.HandPosType[] keyPointsList;
    vendor.qti.hardware.camera.aon.HDGestureInfoType gestureInfo;
}
