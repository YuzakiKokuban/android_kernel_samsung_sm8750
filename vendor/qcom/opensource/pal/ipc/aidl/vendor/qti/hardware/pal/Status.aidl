/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

@Backing(type="int") @VintfStability
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
