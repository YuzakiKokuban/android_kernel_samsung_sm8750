/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

/**
 * Metadata flags
 */
@VintfStability
@Backing(type="int")
enum PalMetadataFlags {
    PAL_META_DATA_FLAGS_NONE = 0,
    PAL_META_DATA_VALID_TS,
    PAL_META_DATA_FLAGS_MAX,
}
