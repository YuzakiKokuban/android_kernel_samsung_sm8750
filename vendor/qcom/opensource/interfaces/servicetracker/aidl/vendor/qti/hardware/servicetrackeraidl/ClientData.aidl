/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.servicetrackeraidl;

/**
 * Client information recieved by AMS
 */
@VintfStability
parcelable ClientData {
    String processName;
    int pid;
}
