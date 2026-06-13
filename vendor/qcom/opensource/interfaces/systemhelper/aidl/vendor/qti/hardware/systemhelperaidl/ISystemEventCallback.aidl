/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.systemhelperaidl;

@VintfStability
interface ISystemEventCallback {
    /**
     * Event notification.
     *
     * @param eventId event occurred (of type SystemEventType)
     */
    oneway void onEvent(in long eventId);
}