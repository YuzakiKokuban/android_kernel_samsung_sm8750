/* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.wifi.supplicant;
import vendor.qti.hardware.wifi.supplicant.IVendorIfaceType;

/**
 * Structure describing the type and name of an vendor iface
 * controlled by the supplicant.
 */
@VintfStability
parcelable IVendorIfaceInfo {
    /**
     * Type of the interface.
     */
    IVendorIfaceType type;
    /**
     * Name of the interface, e.g., wlan0
     */
    String name;
}
