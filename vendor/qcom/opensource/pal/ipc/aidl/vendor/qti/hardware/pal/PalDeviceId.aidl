/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

@VintfStability
@Backing(type="int")
enum PalDeviceId {
    PAL_DEVICE_NONE = 1,
    /**
     * < for transcode usecases
     */
    PAL_DEVICE_OUT_EARPIECE,
    PAL_DEVICE_MAX,
}
