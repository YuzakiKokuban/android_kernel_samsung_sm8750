/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

import vendor.qti.hardware.paleventnotifier.PalMediaConfig;
import vendor.qti.hardware.paleventnotifier.PalStreamDirection;
import vendor.qti.hardware.paleventnotifier.PalStreamFlag;
import vendor.qti.hardware.paleventnotifier.PalStreamInfo;
import vendor.qti.hardware.paleventnotifier.PalStreamType;

/**
 * < PAL stream attributes to be specified, used in pal_stream_open cmd
 */
@VintfStability
parcelable PalStreamAttributes {
    PalStreamType type;
    PalStreamInfo info;
    PalStreamFlag flags;
    PalStreamDirection direction;
    PalMediaConfig inMediaConfig;
    PalMediaConfig outMediaConfig;
}
