/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.FDEngineType;

/**
 * The information of the FD algorithm mode
 */
@VintfStability
parcelable FDAlgoMode {
    /**
     * Image resolution used for algo processing in this algo mode.
     */
    int width;
    /**
     * Image resolution used for algo processing in this algo mode.
     */
    int height;
    /**
     * Indiates whether this FD algo mode is island-capable.
     */
    boolean isIslandModeCapable;
    /**
     * Indicates what the supporting engine is.
     */
    FDEngineType fdEngine;
    /**
     * A bit-mask indicating which FDEvtType are supported
     * in this FDAlgoMode. If both FaceDetected & GazeDetected are
     * supported, the supportedFDEvtTypeMask will be 0x5(0x1|0x4)
     */
    int supportedFDEvtTypeMask;
}
