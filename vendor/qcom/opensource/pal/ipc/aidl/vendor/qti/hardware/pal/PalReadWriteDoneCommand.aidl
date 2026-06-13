/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

/**
 * Commands that can be executed on the driver read/write done thread.
 */
@VintfStability
@Backing(type="int")
enum PalReadWriteDoneCommand {
    WRITE_READY,
    DRAIN_READY,
    PARTIAL_DRAIN_READY,
    READ_DONE,
    ERROR,
}
