/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.servicetrackeraidl;

/**
 * structure to define service to client connection
 */
@VintfStability
parcelable ServiceConnection {
    String clientName;
    int clientPid;
    int count;
}
