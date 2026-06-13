/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalDeviceCustomConfig;
import vendor.qti.hardware.pal.PalDeviceId;
import vendor.qti.hardware.pal.PalMediaConfig;
import vendor.qti.hardware.pal.PalUsbDeviceAddress;

@VintfStability
parcelable PalDevice {
    PalDeviceId id;
    PalMediaConfig config;
    PalUsbDeviceAddress address;
    String sndDevName;
    PalDeviceCustomConfig customConfig;
}
