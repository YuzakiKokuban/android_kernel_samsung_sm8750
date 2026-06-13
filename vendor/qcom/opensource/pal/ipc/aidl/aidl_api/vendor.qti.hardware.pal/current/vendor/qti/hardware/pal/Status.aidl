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

package vendor.qti.hardware.pal;
@Backing(type="int") @VintfStability
enum Status {
  UNKNOWN = (-1) /* -1 */,
  SUCCESS = 0,
  IO_ERROR,
  BUSY,
  NO_SPACE,
  INVALID_FD,
  ADVERTISE_ERROR,
  PROTOCOL_NOT_AVAILABLE,
  NOT_SUPPORTED,
  DOWN_WITH_SSR,
  NOW_INPROGRESS,
  ALREADY_INPROGRESS,
  CANCELLED,
  NOT_RECOVERABLE,
}
