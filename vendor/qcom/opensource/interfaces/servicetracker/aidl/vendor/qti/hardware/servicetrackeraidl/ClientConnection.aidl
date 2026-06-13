/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.servicetrackeraidl;

/**
 * structure to define client to service connection
 */
@VintfStability
parcelable ClientConnection {
    String serviceName;
    int servicePid;
    int count;
}
