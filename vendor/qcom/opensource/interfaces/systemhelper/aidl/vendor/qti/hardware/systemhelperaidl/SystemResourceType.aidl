/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.systemhelperaidl;

@VintfStability
@Backing(type="long")
enum SystemResourceType {
    WAKE_LOCK = 1,
    ROTATION_LOCK = 2,
}