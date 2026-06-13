/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.agm;

/**
 * Session Direction
 */
@VintfStability
@Backing(type="int")
enum Direction {
    Rx = 1,
    Tx,
}
