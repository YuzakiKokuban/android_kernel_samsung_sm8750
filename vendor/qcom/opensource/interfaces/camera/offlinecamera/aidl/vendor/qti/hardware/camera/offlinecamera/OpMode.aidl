/*
 * {Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 * All rights reserved. SPDX-License-Identifier: BSD-3-Clause-Clear}
 */


package vendor.qti.hardware.camera.offlinecamera;


@VintfStability
@Backing(type="int")
enum OpMode {
    OFFLINEBAYER2YUV     = 0,

    OFFLINEYUV2JPEG      = 1,

    OFFLINEYUV2YUV       = 2,

    OFFLINEHWMF          = 3,

    OFFLINEQLL           = 4,

    OFFLINERAW2RAW       = 5,

    OFFLINERAW2JPEG      = 6,
}
