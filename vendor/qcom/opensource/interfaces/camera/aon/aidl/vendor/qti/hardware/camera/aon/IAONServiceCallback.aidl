/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.AONCallbackEvent;

/**
 * AON Service callback interface.
 * This callback will be invoked when any client registers for AON Service
 * and specific AON event triggered by hardware/lower layers
 */
@VintfStability
interface IAONServiceCallback {
    void NotifyAONCallbackEvent(in long clientHandle, in AONCallbackEvent cbEvt);
}
