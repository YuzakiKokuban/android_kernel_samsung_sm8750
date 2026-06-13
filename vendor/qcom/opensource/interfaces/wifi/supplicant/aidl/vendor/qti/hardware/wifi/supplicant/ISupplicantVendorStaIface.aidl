/* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.wifi.supplicant;

/**
 * Vendor StaIface Interface
 */
@VintfStability
interface ISupplicantVendorStaIface {
    /**
     * run Driver Commands
     *
     * @param command Driver Command
     * @return status supplicant status/reply for driver command
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantVendorStatusCode.FAILURE_UNKNOWN|
     */
    String doDriverCmd(in String command);
}
