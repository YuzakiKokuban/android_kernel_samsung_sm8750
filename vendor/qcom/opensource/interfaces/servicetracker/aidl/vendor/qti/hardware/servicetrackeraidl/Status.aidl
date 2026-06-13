/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.servicetrackeraidl;

/**
 * Enum Values indicating the result of operation
 */
@VintfStability
@Backing(type="int")
enum Status {
    /**
     * No errors.
     */
    SUCCESS,
    /**
     * the component for which the query is made
     * is not yet available
     */
    ERROR_NOT_AVAILABLE,
    /**
     * the arguments passed are invalid
     */
    ERROR_INVALID_ARGS,
    /**
     * the information asked for the componenent
     * is not supported
     */
    ERROR_NOT_SUPPORTED,
    ERROR_UNKNOWN,
}
