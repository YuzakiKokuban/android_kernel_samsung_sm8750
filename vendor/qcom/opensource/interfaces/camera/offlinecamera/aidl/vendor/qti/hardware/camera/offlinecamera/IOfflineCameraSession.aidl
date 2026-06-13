/*
 * {Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 * All rights reserved. SPDX-License-Identifier: BSD-3-Clause-Clear}
 */

package vendor.qti.hardware.camera.offlinecamera;

import vendor.qti.hardware.camera.offlinecamera.OpMode;
import vendor.qti.hardware.camera.offlinecamera.IOfflineCameraCallback;
import vendor.qti.hardware.camera.offlinecamera.OfflineSessionConfigureInfo;
import vendor.qti.hardware.camera.offlinecamera.OfflineRequestInfo;
import vendor.qti.hardware.camera.offlinecamera.OfflineFlushInfo;

@VintfStability
interface IOfflineCameraSession {

    /**
    * configureOfflineSession:
    *
    * send configure information to the offline session
    *
    *
    * @param sessionConfig  the info necessary for configuring the Offline Session
    *
    * A service specific error will be returned on the following conditions
    *     INTERNAL_ERROR:
    *         The offline session cannot be configured due to an internal
    *         error.
    *     ILLEGAL_ARGUMENT:
    *         sessionConfig is NULL
    *
    */
    void configureOfflineSession(in OfflineSessionConfigureInfo sessionConfig);

    /**
    * processOfflineRequest:
    *
    * send a list of capture requests to the Offline session.
    *
    * @param requests the request Info necessary for executing an Offline Request
    *
    * A service specific error will be returned on the following conditions
    *     INTERNAL_ERROR:
    *         The offline session cannot process request due to an internal
    *         error.
    *     ILLEGAL_ARGUMENT:
    *         If the input is malformed (the settings are empty when not
    *         allowed, the physical camera settings are invalid, there are 0
    *         output buffers, etc) and capture processing
    *         cannot start.
    *
    * @return  frameNumber populated by service, if multi JPEG requests in the same processOfflineRequest callback
    *          , then just return the frameNumber for the first jpeg request in the array
    *
    */
    int processOfflineRequest(in OfflineRequestInfo[] requests);

    /**
    * closeOfflineSession:
    *
    * Close an Offline Session
    *
    * A service specific error will be returned on the following conditions
    *     ILLEGAL_ARGUMENT:
    *         If sessionHandle is invalid.
    *
    */
    void closeOfflineSession();

    /**
    * OfflineSessionFlush:
    *
    * Flush an Offline Session
    *
    * @param flush the flush Info necessary for Flushing an OfflineSession
    *
    * A service specific error will be returned on the following conditions
    *     INTERNAL_ERROR:
    *         The offline session cannot flush due to an internal
    *         error.
    *     ILLEGAL_ARGUMENT:
    *         If sessionHandle is invalid.
    *
    */
    void OfflineSessionFlush(in OfflineFlushInfo flush);

    /**
    * OfflineSessionDump:
    *
    * Dump the Offline Session
    *
    * A service specific error will be returned on the following conditions
    *     INTERNAL_ERROR:
    *         The offline session cannot dump due to an internal
    *         error.
    *     ILLEGAL_ARGUMENT:
    *         If sessionHandle is invalid.
    *
    */
    void OfflineSessionDump();

}
