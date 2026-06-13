/* Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NLP_API_H
#define NLP_API_H

#include <dlfcn.h>
#include <stddef.h>
#include <stdbool.h>
#include <WiFiDBReceiver.h>
#include <WiFiDBProvider.h>
#include <AltitudeReceiver.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CURRENT_VERSION 4
/**
    API VERSION 1: Initial release
    API VERSION 2: Changed return value of connectToSystemStatus
                   from bool to const SystemRequester*
    API VERSION 3: Added clientData as an argument to all *connect calls.
                   Changed return value of connectToSystemStatus to SystemStatusListener*.
                   Added multiple-client support to connectToSystemStatus.
    API VERSION 4: Added passive location listener in SystemStatus.
                   Added AltitudeReceiver APIs.
*/

/** @brief
    Table of calls clients to implement for service to provide system level updates
*/
typedef struct {
    void (*onLocationOptInUpdate)(OptInStatus optInStatus, const void* clientData);
    void (*onNetworkStatusUpdate)(bool isConected, const NlpNetwork* networksAvailable,
            uint8_t networksAvailableCount, const void* clientData);
    void (*onLocationChange)(const NlpLocation* location, const void* clientData);
} SystemStatusListener;

/** @brief
    Table calls for client to make system level requests
    All calls must be made with listenerAsID registered to
    the service via connectToSystemStatus() call. Calls with
    unregistered listenerAsID would receirve false return.
*/
typedef struct {
    bool (*requestDataConnection)(const SystemStatusListener* listenerAsID);
    bool (*releaseDataConnection)(const SystemStatusListener* listenerAsID);
} SystemRequester;

typedef struct {
    /** @brief
        Provides an instance of SystemRequester object with
        the specified listener and clientData. The registered
        SystemStatusListener will get updates on changes, with
        clientData being passed back to the client. Support
        multiple clients identified by listener, pertaining to
        calls maked to SystemRequester calls.

        @param
        listener: instance of SystemStatusListener,
        implementing the required callback functions.
        Should be valid until disconnect function is called.

        @param
        clientData: opaque client data bundle, will be passed
        back to client with all listener callbacks.
    */
    const SystemRequester* (*connectToSystemStatus)(
            const SystemStatusListener* listener, const void* clientData);

    /** @brief
        Provides an instance of WiFiDBReceiver object with
        the specified listener and clientData. Only one valid
        registration at a given time.

        @param
        listener: instance of WiFiDBReceiverResponseListener,
        implementing the required callback functions.
        Should be valid until disconnect function is called.

        @param
        clientData: opaque client data bundle, will be passed
        back to client with all listener callbacks.

        @return WiFiDBReceiver*. Nullptr if listener is nullptr;
        or if one instance has already registered.
    */
    const WiFiDBReceiver* (*connectToWiFiDBReceiver)(
            const WiFiDBReceiverResponseListener* listener, const void* clientData);

    /** @brief
        Provides an instance of WiFiDBProvider object with
        the specified listener and clientData. Only one valid
        registration at a given time.

        @param
        listener: instance of WiFiDBProviderResponseListener,
        implementing the required callback functions.
        Should be valid until disconnect function is called.

        @param
        clientData: opaque client data bundle, will be passed
        back to client with all the callbacks.

        @return WiFiDBProvider*. False if listener is nullptr;
        or if one instance has already registered.
    */
    const WiFiDBProvider* (*connectToWiFiDBProvider)(
            const WiFiDBProviderResponseListener* listener,
            const void* clientData);

    /** @brief
        Provides an instance of AltitudeReceiver object with
        the specified priority listener.

        @param
        listener: instance of AltitudeReceiverResponseListener,
        implementing the required callback functions.
        Should not be freed until disconnect function is called.

        @param
        clientData: opaque client data bundle, will be passed
        back to client with all the callbacks.

        @return AltitudeReceiver
    */
    const AltitudeReceiver* (*connectToAltitudeReceiver)(
            const AltitudeReceiverResponseListener* listener, const void* clientData);

    /** @brief
        Disconnect the SystemStatusListener. Indicates that client process is not
        available for any reason. {listener, clientData} must match the pair given
        to the connectToSystemStatus call.

        @param
        listener: instance of SystemStatusListener, previously provided
        in the connectToSystemStatus call.

        @param
        clientData: opaque client data bundle, previously provided
        in the connectToSystemStatus call.
    */
    void (*disconnectFromSystemStatus)(const SystemStatusListener* listener);

    /** @brief
        Disconnect the WiFiDBReceiver associated with the provided listener.
        {listener, clientData} must match the pair given to the connectToSystemStatus call

        @param
        listener: instance of WiFiDBReceiverResponseListener, previously provided
        in the connectToWiFiDBReceiver call.

        @param
        clientData: opaque client data bundle, previously provided
        in the connectToWiFiDBReceiver call.
    */
    void (*disconnectFromWiFiDBReceiver)(const WiFiDBReceiverResponseListener* listener);

    /** @brief
        Disconnect the WiFiDBProvider associated with the provided listener.
        {listener, clientData} must match the pair given to the connectToSystemStatus call

        @param
        listener: instance of WiFiDBProviderResponseListener, previously provided
        in the connectToWiFiDBProvider call.

        @param
        clientData: opaque client data bundle, previously provided
        in the connectToWiFiDBProvider call.
    */
    void (*disconnectFromWiFiDBProvider)(const WiFiDBProviderResponseListener* listene);

    /** @brief
        Disconnect the AltitudeReceiver associated with the provided listener.

        @param
        listener: instance of AltitudeReceiverResponseListener, previously provided
        in the connectToAltitudeReceiver call.
    */
    void (*disconnectFromAltitudeReceiver)(
            const AltitudeReceiverResponseListener* listener);
} NLPApi;

/** @brief
    Provides a C pointer to an instance of NLPApi struct after dynamic linking to lobnlp_api.so.
*/
inline const NLPApi* linkGetNLPApi() {
    typedef const void* (getNLPApi)(uint32_t);

    getNLPApi* getter = nullptr;
    void *handle = dlopen("libnlp_client_api.so", RTLD_NOW);
    if (nullptr != handle) {
        getter = (getNLPApi*)dlsym(handle, "getNLPApiByVersion");
    }

    // increment this version number when aboe NLPApi changes
    return (const NLPApi*)(nullptr != getter) ? (NLPApi*)(*getter)(CURRENT_VERSION) : nullptr;
}

#ifdef __cplusplus
}
#endif

#endif /* NLP_API_H */
