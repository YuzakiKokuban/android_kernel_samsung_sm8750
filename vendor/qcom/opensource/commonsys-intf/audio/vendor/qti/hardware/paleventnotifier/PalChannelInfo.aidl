/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

/**
 * Audio channel info data structure
 */
@VintfStability
parcelable PalChannelInfo {
    char channels;
    byte[64] chMap;
}
