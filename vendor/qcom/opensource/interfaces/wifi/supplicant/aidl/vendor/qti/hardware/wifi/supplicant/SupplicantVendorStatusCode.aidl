/* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.wifi.supplicant;

/**
 * Enum values indicating the status of any supplicant operation.
 */
@VintfStability
enum SupplicantVendorStatusCode {
    /**
     * No errors.
     */
    SUCCESS,
    /**
     * Unknown failure occurred.
     */
    FAILURE_UNKNOWN,
    /**
     * One of the incoming args is invalid.
     */
    FAILURE_ARGS_INVALID,
    /**
     * |ISupplicantVendorStaIface| AIDL interface object is no longer valid.
     */
    FAILURE_IFACE_INVALID,
    FAILURE_UNSUPPORTED,
}
