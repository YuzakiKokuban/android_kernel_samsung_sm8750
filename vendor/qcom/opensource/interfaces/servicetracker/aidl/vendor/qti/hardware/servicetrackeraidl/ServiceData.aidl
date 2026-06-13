/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.servicetrackeraidl;

/**
 * Service information recieved by AMS
 */
@VintfStability
parcelable ServiceData {
    String packageName;
    String processName;
    int pid;
    double lastActivity;
    boolean serviceB;
}
