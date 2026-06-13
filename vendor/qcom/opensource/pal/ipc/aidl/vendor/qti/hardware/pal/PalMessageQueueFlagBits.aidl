/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

/**
 * The message queue flags used to synchronize reads and writes from
 * message queues used by PAL
 */
@VintfStability
@Backing(type="int")
enum PalMessageQueueFlagBits {
    NOT_EMPTY = 1 << 0,
    NOT_FULL = 1 << 1,
}
