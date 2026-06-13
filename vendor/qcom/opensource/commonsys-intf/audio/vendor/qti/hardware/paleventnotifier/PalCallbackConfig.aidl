/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

import vendor.qti.hardware.paleventnotifier.PalDeviceId;
import vendor.qti.hardware.paleventnotifier.PalStreamAttributes;

@VintfStability
parcelable PalCallbackConfig {
    int noOfPrevDevices;
    int noOfCurrentDevices;
    PalDeviceId[] prevDevices;
    PalDeviceId[] currentDevices;
    PalStreamAttributes streamAttributes;
}
