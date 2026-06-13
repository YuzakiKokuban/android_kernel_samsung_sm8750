/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

@VintfStability
@Backing(type="int")
enum PalReadWriteDoneResult {
    OK,
    NOT_INITIALIZED,
    INVALID_ARGUMENTS,
    INVALID_STATE,
    NOT_SUPPORTED,
}
