/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.systemhelperaidl;

import vendor.qti.hardware.systemhelperaidl.ISystemEventCallback;
import vendor.qti.hardware.systemhelperaidl.SystemEventType;

@VintfStability
interface ISystemEvent {
    /**
     * Deregister for system event notification.
     *
     * @param eventIds event mask (of type SystemEventType) to deregister for
     * @param cb callback interface used for event registration
     * @return status of event de-registration request
     *                0 in case of success and negative in case of failure
     */
    int deRegisterEvent(in long eventIds, in ISystemEventCallback cb);

    /**
     * Register for system event notification.
     *
     * @param eventIds event mask (of type SystemEventType) to register for
     * @param cb callback interface for event notification
     * @return status of event registration request
     *                0 in case of success and negative in case of failure
     */
    int registerEvent(in long eventIds, in ISystemEventCallback cb);
}