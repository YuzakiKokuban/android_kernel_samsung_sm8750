/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalBuffer;

@VintfStability
parcelable PalReadReturnData {
    int ret;
    PalBuffer[] buffer;
}
