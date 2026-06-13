/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

/**
 * The delivery mode of event detection
 */
@VintfStability
@Backing(type="int")
enum DeliveryMode {
    /**
     * Detection is triggered to run based on motion detected by sensor.
     */
    MotionBased = 0,
    /**
     * Detection is triggered to run periodically according to
     * the value of deliveryPeriodMs set by client during RegisterClient.
     */
    TimeBased = 1,
}
