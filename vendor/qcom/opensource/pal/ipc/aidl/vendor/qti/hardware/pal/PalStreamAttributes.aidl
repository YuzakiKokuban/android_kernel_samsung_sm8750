/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalMediaConfig;
import vendor.qti.hardware.pal.PalStreamDirection;
import vendor.qti.hardware.pal.PalStreamFlag;
import vendor.qti.hardware.pal.PalStreamInfo;
import vendor.qti.hardware.pal.PalStreamType;

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
