/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.Direction;

@VintfStability
parcelable AifInfo {
    // AIF name
    String aifName;
    // direction Rx or Tx
    Direction direction;
}
