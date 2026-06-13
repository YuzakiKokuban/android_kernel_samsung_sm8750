/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.servicetrackeraidl;

import vendor.qti.hardware.servicetrackeraidl.ClientConnection;
import vendor.qti.hardware.servicetrackeraidl.ClientData;
import vendor.qti.hardware.servicetrackeraidl.ClientRecord;
import vendor.qti.hardware.servicetrackeraidl.ServiceConnection;
import vendor.qti.hardware.servicetrackeraidl.ServiceData;
import vendor.qti.hardware.servicetrackeraidl.ServiceRecord;
import vendor.qti.hardware.servicetrackeraidl.Status;

// Interface inherits from vendor.qti.hardware.servicetrackeraidl@1.0::IServicetracker but AIDL does not support interface inheritance (methods have been flattened).
@VintfStability
interface IServicetracker {
    /**
     * Collect and update  the information for the service which is just bound to
     * @param clientData
     * @param serviceData
     */
    oneway void bindService(in ServiceData serviceData, in ClientData clientData);

    /**
     * Update the service information when service specified by
     * @param serviceData has been destroyed
     */
    oneway void destroyService(in ServiceData serviceData);

    /**
     * Return the list of services associated with the client
     * specified by @param clientName
     */
    ClientConnection[] getClientConnections(in String clientName);


    /**
     * Return the pid of the process specified by @param processName
     */
    int getPid(in String processName);

    /**
     * Return the pid of the services listed in @param serviceList
     */
    int[]  getPids(in String[] serviceList);

    /**
     * Return Pids of all the running Android Services.
     */
    int[] getRunningServicePid();


    /**
     * generate the list of b services running in system
     */
    int  getServiceBCount(out ServiceRecord[] bServiceList);

    /**
     * Return list of clients associated with the service specified by
     * @param serviceName
     */
    ServiceConnection[] getServiceConnections(in String serviceName);

    /**
     * Return all the details related to Client specified by @param
     * clientName
     */
    ClientRecord getclientInfo(in String clientName);

    /**
     * Return all the details related to Service specified by @param
     * serviceName
     */
    ServiceRecord getserviceInfo(in String serviceName);

    /**
     * Return whether a service specified by @param serviceName is
     * B-Service or not
     */
    boolean isServiceB(in String serviceName);

    /**
     * Update the service and client information when process
     * specified by @param pid got killed
     */
    oneway void killProcess(in int pid);

    /**
     * Collect the information related to service that is just started
     * @param serviceData contains details of the service,@param clientData
     * contains the details of client which has started the service
     */
    oneway void startService(in ServiceData serviceData);

    /**
     * Update the bind information of sevice,client pair specified
     * by @param serviceData and @param clientData
     */
    oneway void unbindService(in ServiceData serviceData, in ClientData clientData);
}
