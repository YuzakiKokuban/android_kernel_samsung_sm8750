/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

/**
 * Audio channel info data structure
 */
@VintfStability
parcelable PalChannelInfo {
    char channels;
    byte[64] chMap;
}
