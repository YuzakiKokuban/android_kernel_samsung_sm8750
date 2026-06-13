/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

/**
 * Audio stream direction enumeration
 */
@VintfStability
@Backing(type="int")
enum PalStreamDirection {
    PAL_AUDIO_OUTPUT = 0x1,
    PAL_AUDIO_INPUT = 0x2,
    PAL_AUDIO_INPUT_OUTPUT = 0x3,
    PAL_AUDIO_INVALID = 0x4,
}
