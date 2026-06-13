/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

@VintfStability
parcelable PalTimeus {
    int valLsw;
    /**
     * Lower 32 bits of 64 bit time value in microseconds
     */
    int valMsw;
}
