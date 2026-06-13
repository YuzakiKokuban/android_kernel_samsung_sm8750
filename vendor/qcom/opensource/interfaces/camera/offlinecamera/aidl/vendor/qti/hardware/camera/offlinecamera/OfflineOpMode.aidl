/*
 * {Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 * All rights reserved. SPDX-License-Identifier: BSD-3-Clause-Clear}
 */


package vendor.qti.hardware.camera.offlinecamera;


@VintfStability
@Backing(type="int")
enum OfflineOpMode {
    OfflineOpModeStart     = 0x8000,

    OpModeOfflineBayer2Yuv = 0x8001,

    OpModeOfflineYuv2Jpeg  = 0x8002,

    OpModeOfflineYuv2Yuv   = 0x8003,

    OpModeOfflineQLL       = 0x8004,

    OpModeOfflineHWMF      = 0x8005,

    OpModeOfflineRaw2Raw   = 0x8006,

    OpModeOfflineRaw2Jpeg  = 0x8007,
}
