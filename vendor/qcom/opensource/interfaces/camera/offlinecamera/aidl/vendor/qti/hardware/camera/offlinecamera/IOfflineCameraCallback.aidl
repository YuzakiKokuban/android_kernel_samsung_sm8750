/*
 * {Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 * All rights reserved. SPDX-License-Identifier: BSD-3-Clause-Clear}
 */

package vendor.qti.hardware.camera.offlinecamera;

import vendor.qti.hardware.camera.offlinecamera.OfflineCaptureResult;
import vendor.qti.hardware.camera.offlinecamera.OfflineBufferRequest;
import vendor.qti.hardware.camera.offlinecamera.OfflineStreamBufferRet;
import android.hardware.camera.device.NotifyMsg;
import android.hardware.camera.device.BufferRequestStatus;
import android.hardware.camera.device.StreamBuffer;

/**
 * Callback methods for the offline camera service module to call into the Client.
 */
@VintfStability
interface IOfflineCameraCallback {
    /**
    * notify:
    *
    * service send messages back to this client
    *
    * @param msgs List of notification msgs to be processed by client
    */
    void notify(in NotifyMsg[] msgs);


    /**
    * processCaptureResult:
    *
    * service send result back to this client
    *
    * @param results to be processed by the client
    *
    */
    void processCaptureResult(in OfflineCaptureResult[] results);

    /**
    * requestStreamBuffers:
    *
    * callback for service to ask for buffers from this client
    *
    * @param bufReqs Buffers requested by the offline camera service
    * @param buffers the buffers returned to the offline camera service by this Client
    */
    BufferRequestStatus requestStreamBuffers(
       in OfflineBufferRequest[] bufReqs, out OfflineStreamBufferRet[] buffers);

    /**
    * returnStreamBuffers:
    *
    * callback for service to return  buffers to this client
    *
    * @param buffers The stream buffers returned to the offline camera Client
    */
    void returnStreamBuffers(in StreamBuffer[] buffers);

}
