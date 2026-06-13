/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.servicetrackeraidl;

import vendor.qti.hardware.servicetrackeraidl.ServiceConnection;

/**
 * Service information
 */
@VintfStability
parcelable ServiceRecord {
    String packageName;
    String processName;
    int pid;
    boolean serviceB;
    double lastActivity;
    ServiceConnection[] conn;
}
