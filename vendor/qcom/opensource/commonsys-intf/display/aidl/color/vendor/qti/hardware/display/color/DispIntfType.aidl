/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.display.color;
/*
 * Display interface types
 */
@VintfStability
@Backing(type="int")
enum DispIntfType {
    DSI0 = 0,
    DSI1 = 1,
    HDMI = 2,
    MHL = 3,
    VIRTUAL = 4,
    MAX = 5,
}
