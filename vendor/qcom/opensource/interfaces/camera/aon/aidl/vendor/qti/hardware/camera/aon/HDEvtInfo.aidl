/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
package vendor.qti.hardware.camera.aon;
@VintfStability
parcelable HDEvtInfo {
  int hdEvtTypeMask;
  int frameDimWidth;
  int frameDimHeight;
  vendor.qti.hardware.camera.aon.HandInfoPerHand[] handInfoPerHand;
}
