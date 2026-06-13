/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

/**
 * Position type of an AON sensor.
 */
@VintfStability
@Backing(type="int")
enum PositionType {
    /**
     * Rear main camera.
     */
    REAR = 0,
    /**
     * Front main camera.
     */
    FRONT = 1,
}
