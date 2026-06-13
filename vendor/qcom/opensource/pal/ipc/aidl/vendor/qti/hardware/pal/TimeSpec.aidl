/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

/**
 * A substitute for POSIX timespec.
 */
@VintfStability
parcelable TimeSpec {
    long tvSec;
    long tvNSec;
}
