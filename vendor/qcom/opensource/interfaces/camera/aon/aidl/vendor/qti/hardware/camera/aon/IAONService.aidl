/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.camera.aon;

import vendor.qti.hardware.camera.aon.AONRegisterInfo;
import vendor.qti.hardware.camera.aon.AONSensorInfo;
import vendor.qti.hardware.camera.aon.AONServiceType;
import vendor.qti.hardware.camera.aon.IAONServiceCallback;
import vendor.qti.hardware.camera.aon.Status;

@VintfStability
interface IAONService {
    /**
     * Get a list of AONSensorInfo which describes supported AON sensors and corresponding capability information.
     *
     * @return   An output list of AONSensorInfo
     */
    AONSensorInfo[] GetAONSensorInfoList();

    /**
     * Register to an AON sensor for a specific AONServiceType
     *
     * @param  callback            Object passed by AIDL client which has callback function that got called for AON events
     * @param  regInfo             Register information filled by client
     *
     * @return clientHandle        AON Service return a valid client handle upon success return NULL in case of failure
     */
    long RegisterClient(in IAONServiceCallback callback, in AONRegisterInfo regInfo);

    /**
     * Unregisters for AON service events.
     *
     * @param   clientHandle       Valid client handle assigned during registerClient API
     *                             This client handle will be no longer valid after return
     * @return  status             Returns status defined in enum Status. Returns Status::SUCCESS in case of success
     *
     */
    Status UnregisterClient(in long clientHandle);
}
