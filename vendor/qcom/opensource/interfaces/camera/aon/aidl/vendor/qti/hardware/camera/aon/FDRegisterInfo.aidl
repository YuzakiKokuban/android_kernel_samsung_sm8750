/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.DeliveryMode;

/**
 * The FD register information filled by client.
 * Only applicable when service type is FaceDetect or FaceDetectPro
 */
@VintfStability
parcelable FDRegisterInfo {
    /**
     * Indicate which FDEvtType the client is interested in.
     * If client need both FaceDetected & GazeDetected,
     * the fdEvtTypeMask should be assigned as 0x5(0x1|0x4)
     */
    int fdEvtTypeMask;
    /**
     * Index to the algoModes described in AONSensorCap which returned in GetAONSensorInfoList
     */
    int fdAlgoModeIdx;
    /**
     * To configure the delivery mode of the detection callback event
     * Since one AON sensor can only be configured one delivery mode at a time,
     * AON service will take the value of the latest registered client as the final configuration.
     */
    DeliveryMode deliveryMode;
    /**
     * Period in ms of detection callback event.
     * The value should be positive and only applicable when delivery mode is TimeBased.
     * Since one AON sensor can only be configured one delivery mode at a time,
     * AON service will take the value of the latest registered client as the final configuration.
     */
    int deliveryPeriodMs;
    /**
     * Number of times detections are performed per delivery
     * Applicable for both deliveryMode - MotionBased & TimeBased
     * If deliveryMode is MotionBased:
     * This is the number of detections that will be performed each
     * time a motion is detected.
     * If deliveryMode is TimeBased:
     * At the specified delivery periodicity, detection will be
     * performed consecutively at the native streaming rate of the image sensor.
     */
    int detectionPerDelivery;
}
