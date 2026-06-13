/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package vendor.qti.hardware.systemhelperaidl;
@Backing(type="long") @VintfStability
enum SystemEventType {
  PHONE_STATE_RINGING = 0x1,
  PHONE_STATE_OFF_HOOK = 0x2,
  PHONE_STATE_IDLE = 0x4,
  ACTION_SCREEN_OFF = 0x8,
  ACTION_SCREEN_ON = 0x10,
  ACTION_SHUTDOWN = 0x20,
  ACTION_USER_PRESENT = 0x40,
  SYSTEM_EVENT_MAX = (0x80 - 1) /* 127 */,
}
