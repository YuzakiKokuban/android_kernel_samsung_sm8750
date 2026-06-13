/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.color;

import vendor.qti.hardware.display.color.Range;
import vendor.qti.hardware.display.color.RangeFloat;

@VintfStability
parcelable PARange {
    int flags;
    Range hue;
    RangeFloat saturation;
    RangeFloat value;
    RangeFloat contrast;
    RangeFloat satThreshold;
}
