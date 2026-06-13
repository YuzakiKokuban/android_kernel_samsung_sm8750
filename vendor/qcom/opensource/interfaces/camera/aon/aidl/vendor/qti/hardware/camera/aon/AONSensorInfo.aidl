/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.AONSensorCap;
import vendor.qti.hardware.camera.aon.PositionType;

/**
 * The information of an AON sensor
 */
@VintfStability
parcelable AONSensorInfo {
    /**
     * Position type of AON sensor
     */
    PositionType position;
    /**
     * List of supported AONServiceType by AON sensor
     */
    AONSensorCap[] sensorCaps;
}
