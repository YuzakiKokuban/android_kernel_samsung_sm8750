/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.color;

import vendor.qti.hardware.display.color.PAConfigData;

@VintfStability
parcelable PAConfig {
    boolean valid;
    int flags;
    PAConfigData data;
}
