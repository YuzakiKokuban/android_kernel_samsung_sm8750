/* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.wifi.supplicant;
import vendor.qti.hardware.wifi.supplicant.IVendorIfaceInfo;
import vendor.qti.hardware.wifi.supplicant.ISupplicantVendorStaIface;

/**
 * Interface exposed by the supplicant vendor AIDL service registered
 * with the service manager. This is the root level object for
 * any of the vendor supplicant interactions.
 */
@VintfStability
interface ISupplicantVendor {
    /**
     * Gets a AIDL interface object for the interface corresponding to iface
     * name which the supplicant already controls.
     *
     * @param ifaceInfo Combination of the iface type and name retrieved
     *        using |listVendorInterfaces|.
     * @return iface AIDL sta interface object representing the interface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantVendorStatusCode.FAILURE_UNKNOWN|
     */
    ISupplicantVendorStaIface getVendorInterface(in IVendorIfaceInfo ifaceInfo);

    /**
     * Retrieve a list of all the vendor interfaces controlled by the supplicant.
     *
     * The corresponding |ISupplicantStaIface| object for any interface can be
     * retrieved using |getVendorInterface| method.
     *
     * @return ifaces List of all interfaces controlled by the supplicant.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantVendorStatusCode.FAILURE_UNKNOWN|
     */
    IVendorIfaceInfo[] listVendorInterfaces();
}

