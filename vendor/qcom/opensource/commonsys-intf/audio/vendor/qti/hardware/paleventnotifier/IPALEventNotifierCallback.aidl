/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

import vendor.qti.hardware.paleventnotifier.PalCallbackConfig;

@VintfStability
interface IPALEventNotifierCallback {
    oneway void onStart(in PalCallbackConfig Config);
    oneway void onStop(in PalCallbackConfig Config);
    oneway void onDeviceSwitch(in PalCallbackConfig Config);
}
