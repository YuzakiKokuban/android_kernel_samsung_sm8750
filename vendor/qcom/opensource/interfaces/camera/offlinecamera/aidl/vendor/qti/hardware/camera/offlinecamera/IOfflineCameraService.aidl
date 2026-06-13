/*
 * {Copyright (c) 2023 Qualcomm Innovation Center, Inc.
 * All rights reserved. SPDX-License-Identifier: BSD-3-Clause-Clear}
 */

package vendor.qti.hardware.camera.offlinecamera;

import vendor.qti.hardware.camera.offlinecamera.OpMode;
import vendor.qti.hardware.camera.offlinecamera.IOfflineCameraCallback;
import vendor.qti.hardware.camera.offlinecamera.IOfflineCameraSession;
import android.hardware.camera.device.CameraMetadata;

@VintfStability
interface IOfflineCameraService {

    /**
    * getSupportedOpModes:
    *
    * get all kind of supported offline snapshot opMode.
    *
    * @return The list of all the supported offline snapshotTypes
    *
    */
    OpMode[] getSupportedOpModes();


    /**
    * registerClient:
    *
    * register a client and callback.
    *
    * @param  clientID  clientID for record clientInfo, just used for callbackonly mode
    * @param  callback  Callback ops, help to return result.
    *
    * A service specific error will be returned on the following conditions
    *
    *     ILLEGAL_ARGUMENT:
    *         The callbacks handle is invalid (for example, it is null).
    *
    */
    void registerClient(in int clientID, in IOfflineCameraCallback callback);

    /**
    * unRegisterClient:
    *
    * unRegisterClient a client.
    *
    * @param  clientID  clientID for this client, just used for callbackonly mode
    * @param  callback  Callback ops, help to return result.
    *
    * A service specific error will be returned on the following conditions
    *     ILLEGAL_ARGUMENT:
    *         clientID not registerd before.
    *
    */
    void unRegisterClient(in int clientID);

    /**
    * openOfflineSeesion:
    *
    * Open a offline session and set the callback and then return the opened offline session handle.
    *
    * @param sessionName  give a session Name for this offline session
    * @param callback     session Callback ops, help to return result
    *
    * A service specific error will be returned on the following conditions
    *     INTERNAL_ERROR:
    *         The offline session cannot be opened due to an internal
    *         error.
    *     ILLEGAL_ARGUMENT:
    *         The callbacks handle or the SessiontName is invalid (for example, it is null).
    *
    * @return The interface to the newly-opened offline camera session
    *
    */
    IOfflineCameraSession openOfflineSeesion(in String sessionName, in IOfflineCameraCallback callback);

    /**
    * GetOfflineStaticCaps:
    *
    * get camera capabilities.
    *
    * @return camera metadata
    *
    */
    CameraMetadata GetOfflineStaticCaps();
}
