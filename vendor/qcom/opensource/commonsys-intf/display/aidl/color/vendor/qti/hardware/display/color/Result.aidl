/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.color;

/*
 * Type enumerating various result codes returned from IDispColor methods
 */
@VintfStability
@Backing(type="int")
enum Result {
    OK,
    PERMISSION_DENIED = -1,
    NO_MEMORY = -12,
    BAD_VALUE = -22,
    INVALID_OPERATION = -38,
}
