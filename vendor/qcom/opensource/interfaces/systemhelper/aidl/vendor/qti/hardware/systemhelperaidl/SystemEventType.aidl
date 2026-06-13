/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.systemhelperaidl;

/** Types of Events **/

@VintfStability
@Backing(type="long")
enum SystemEventType {
    PHONE_STATE_RINGING  = 0x1,
    PHONE_STATE_OFF_HOOK = 0x2,
    PHONE_STATE_IDLE     = 0x4,
    ACTION_SCREEN_OFF    = 0x8,
    ACTION_SCREEN_ON     = 0x10,
    ACTION_SHUTDOWN      = 0x20,
    ACTION_USER_PRESENT  = 0x40,
    SYSTEM_EVENT_MAX     = (0x80 - 1),
}