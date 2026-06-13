/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

/**
 * Status codes to propagagte Linux error codes over AIDL.
 * Keep in sync with vendor/qcom/opensource/agm/service/src/utils.c
 * Few error codes are supported by binder_status.h by default.
 * For unsupported codes, create ServiceSpecificException and based on
 * exception extract the error code at client side.
 */
@VintfStability
@Backing(type="int")
enum Status {
    UNKNOWN = -1,
    SUCCESS = 0,
    IO_ERROR,
    BUSY,
    NO_SPACE,
    INVALID_FD,
    ADVERTISE_ERROR,
    PROTOCOL_NOT_AVAILABLE,
    NOT_SUPPORTED,
    DOWN_WITH_SSR,
    NOW_INPROGRESS,
    ALREADY_INPROGRESS,
    CANCELLED,
    NOT_RECOVERABLE,
}