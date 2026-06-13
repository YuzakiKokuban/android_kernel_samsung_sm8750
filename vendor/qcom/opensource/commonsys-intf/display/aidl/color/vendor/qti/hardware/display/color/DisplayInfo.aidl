/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.color;

import vendor.qti.hardware.display.color.DispIntfType;

@VintfStability
parcelable DisplayInfo {
    int flags;
    int id;
    int width;
    int height;
    int status;
    DispIntfType intf;
    String name;
}
