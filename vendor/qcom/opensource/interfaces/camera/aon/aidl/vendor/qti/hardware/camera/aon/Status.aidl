/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

/**
 * Status codes returned directly by the AIDL method calls upon errors
 */
@VintfStability
@Backing(type="int")
enum Status {
    /**
     * Success
     */
    SUCCESS = 0,

    /**
     * Generic failure status
     */
    FAILED = 1,

    /**
     * NOT_SUPPORTED
     */
    NOT_SUPPORTED = 2,

    /**
     * Bad State
     */
    BAD_STATE = 3,

    /**
     * CB Pointer is invalid
     */
    INVALID_CALLBACK_PTR = 4,

    /**
     * AON Service Aborted
     */
    ABORT = 5,
}
