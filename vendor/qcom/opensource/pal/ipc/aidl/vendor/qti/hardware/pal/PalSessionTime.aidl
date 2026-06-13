/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalTimeus;

/**
 * Timestamp strucutre defintion used as argument for
 *  gettimestamp api
 */
@VintfStability
parcelable PalSessionTime {
    PalTimeus sessionTime;
    /**
     * Value of the current session time in microseconds
     */
    PalTimeus absoluteTime;
    /**
     * Value of the absolute time in microseconds
     */
    PalTimeus timestamp;
}
