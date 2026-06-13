/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

import vendor.qti.hardware.paleventnotifier.PalAudioFmt;
import vendor.qti.hardware.paleventnotifier.PalChannelInfo;

/**
 * Media configuraiton
 */
@VintfStability
parcelable PalMediaConfig {
    int sampleRate;
    int bitwidth;
    PalChannelInfo chInfo;
    PalAudioFmt audioFormatId;
}
