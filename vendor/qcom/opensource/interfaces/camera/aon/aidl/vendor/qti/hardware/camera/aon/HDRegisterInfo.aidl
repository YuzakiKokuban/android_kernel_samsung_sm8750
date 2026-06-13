/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
package vendor.qti.hardware.camera.aon;
@VintfStability
parcelable HDRegisterInfo {
  int hdEvtTypeMask;
  vendor.qti.hardware.camera.aon.DeliveryMode deliveryMode;
  int deliveryPeriodMs;
  int detectionPerDelivery;
  boolean detectGesture;
}
