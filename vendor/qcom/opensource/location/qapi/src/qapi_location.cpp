/* Copyright (c) 2019-2020 The Linux Foundation. All rights reserved.
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

#define LOG_TAG "LocSvc_LocationQapi"

#include <loc_pla.h>
#include "log_util.h"
#include "loc_cfg.h"
#include "LocationClientApi.h"
#include "qapi_location.h"
#include <pthread.h>
#include <vector>
#include <string.h>
#include <LocTimer.h>

using namespace std;
using namespace loc_util;
using namespace location_client;
using Runnable = std::function<void()>;

class QapiTimer : public LocTimer {
public:
    QapiTimer() : LocTimer(), mStarted(false) {}
    inline ~QapiTimer() { stop(); }
    inline void set(const time_t waitTimeMs, const Runnable& runable) {
        mWaitTimeInMs = waitTimeMs;
        mRunnable = runable;
    }
    inline void start() {
        mStarted = true;
        LocTimer::start(mWaitTimeInMs, false);
    }
    inline void start(const time_t waitTimeSec) {
        mWaitTimeInMs = waitTimeSec * 1000;
        start();
    }
    inline void stop() {
        if (mStarted) {
            LocTimer::stop();
            mStarted = false;
        }
    }
    inline void restart() { stop(); start(); }
    inline void restart(const time_t waitTimeSec) { stop(); start(waitTimeSec); }
    inline bool isStarted() { return mStarted; }

private:
    // Override
    inline virtual void timeOutCallback() override {
        mStarted = false;
        mRunnable();
    }

private:
    time_t mWaitTimeInMs;
    Runnable mRunnable;
    bool mStarted;

};

static pthread_mutex_t qMutex;
static bool qMutexInitDone = false;
static LocationClientApi* pLocClientApi = nullptr;
static LocationClientApi* pLocPassiveClientApi = nullptr;
static QapiTimer mTimer;
qapi_Location_Callbacks_t   qLocationCallbacks;
static qapi_Location_t qCacheLocation;
static QapiTimer mPassiveTimer;

static qapi_Location_Error_t get_qapi_error(LocationResponse err)
{
    switch (err)
    {
    case LOCATION_RESPONSE_SUCCESS:         return QAPI_LOCATION_ERROR_SUCCESS;
    case LOCATION_RESPONSE_UNKOWN_FAILURE:  return QAPI_LOCATION_ERROR_GENERAL_FAILURE;
    case LOCATION_RESPONSE_NOT_SUPPORTED:   return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    default:                                return QAPI_LOCATION_ERROR_GENERAL_FAILURE;
    }
}

static int32_t copy_to_user_space_buffer(
    uint8_t* user_space_buffer,
    uint32_t user_space_buffer_size,
    uint8_t* data_ptr,
    uint32_t data_size,
    uint32_t offset) {

    if (offset >= user_space_buffer_size) {
        LOC_LOGe("Invalid offset %d, buffer size %d",
            offset, user_space_buffer_size);
        return -1;
    }

    memcpy((user_space_buffer + offset), data_ptr, data_size);

    return 0;
}

static void print_qLocation_array(qapi_Location_t* qLocationArray, uint32_t length) {

    for (uint32_t i = 0; i < length; i++) {
        LOC_LOGv("LAT: %d.%d LON: %d.%d ALT: %d.%d ALT_MSL: %d.%d",
            (int)qLocationArray[i].latitude,
            (abs((int)(qLocationArray[i].latitude * 100000))) % 100000,
            (int)qLocationArray[i].longitude,
            (abs((int)(qLocationArray[i].longitude * 100000))) % 100000,
            (int)qLocationArray[i].altitude,
            (abs((int)(qLocationArray[i].altitude * 100))) % 100,
            (int)qLocationArray[i].altitudeMeanSeaLevel,
            (abs((int)(qLocationArray[i].altitudeMeanSeaLevel * 100))) % 100);

        LOC_LOGv("SPEED: %d.%d BEAR: %d.%d TIME: 0x%x%x FLAGS: %u",
            (int)qLocationArray[i].speed,
            (abs((int)(qLocationArray[i].speed * 100))) % 100,
            (int)qLocationArray[i].bearing,
            (abs((int)(qLocationArray[i].bearing * 100))) % 100,
            (int)(qLocationArray[i].timestamp >> 32),
            (int)qLocationArray[i].timestamp, qLocationArray[i].flags);

        LOC_LOGv("ACC: %d.%d VERT_ACC: %d.%d SPEED_ACC: %d.%d BEAR_ACC: %d.%d",
            (int)qLocationArray[i].accuracy,
            (abs((int)(qLocationArray[i].accuracy * 100))) % 100,
            (int)qLocationArray[i].verticalAccuracy,
            (abs((int)(qLocationArray[i].verticalAccuracy * 100))) % 100,
            (int)qLocationArray[i].speedAccuracy,
            (abs((int)(qLocationArray[i].speedAccuracy * 100))) % 100,
            (int)qLocationArray[i].bearingAccuracy,
            (abs((int)(qLocationArray[i].bearingAccuracy * 100))) % 100);
    }
}

static void print_qGnssData(qapi_Gnss_Data_t* qGnssData) {

    LOC_LOGv("JAMMER_GPS: %u JAMMER_GLONASS: %u",
        qGnssData->jammerInd[QAPI_GNSS_SV_TYPE_GPS],
        qGnssData->jammerInd[QAPI_GNSS_SV_TYPE_GLONASS]);

    LOC_LOGv("JAMMER_BEIDOU: %u JAMMER_GALILEO: %u",
        qGnssData->jammerInd[QAPI_GNSS_SV_TYPE_BEIDOU],
        qGnssData->jammerInd[QAPI_GNSS_SV_TYPE_GALILEO]);
}

static void location_capabilities_callback(
    LocationCapabilitiesMask capsMask)
{
    LOC_LOGv("location_capabilities_callback!");

    qLocationCallbacks.capabilitiesCb(
        (qapi_Location_Capabilities_Mask_t)capsMask);
}

static void location_response_callback(
    LocationResponse err)
{
    qapi_Location_Error_t qErr;

    qErr = get_qapi_error(err);
    LOC_LOGv("location_response_callback!");

    // hard-code id to 0 for now
    uint32_t id = 0;
    qLocationCallbacks.responseCb(qErr, id);
}

static void location_collective_response_callback(
    vector<std::pair<Geofence, LocationResponse>>& responses)
{
    qapi_Location_Error_t* qLocationErrorArray = NULL;

    // hard-coded
    qLocationErrorArray = new qapi_Location_Error_t[5];
    if (nullptr == qLocationErrorArray) {
        LOC_LOGe("qLocationErrorArray is nullptr");
        return;
    }
    uint32_t* ids = new uint32_t[5];
    if (nullptr == ids) {
        LOC_LOGe("ids is nullptr");
        delete[] qLocationErrorArray;
        return;
    }

    for (int i = 0; i < 5; i++) {
        qLocationErrorArray[i] = QAPI_LOCATION_ERROR_SUCCESS;
    }

    LOC_LOGd("Invoking Collective Response Cb");
    qLocationCallbacks.collectiveResponseCb(
        5, qLocationErrorArray, ids);

    delete[] qLocationErrorArray;
    delete[] ids;
}

static void location_tracking_callback(
    Location location)
{
    qapi_Location_t qLocation = {};
    bool bIsSingleShot = false;

    // first check if location is valid
    if (0 == location.flags) {
        LOC_LOGd("Ignore invalid location");
        return;
    }

    if (mTimer.isStarted()) {
        LOC_LOGd("Timer was started, meaning this is singleshot");
        if (nullptr != pLocClientApi) {
            LOC_LOGd("Stop singleshot session");
            pLocClientApi->stopPositionSession();
        }
        LOC_LOGd("Stop singleshot timer");
        mTimer.stop();
        bIsSingleShot = true;
    }
    qLocation.size = sizeof(qapi_Location_t);
    qLocation.timestamp = location.timestamp;
    qLocation.latitude = location.latitude;
    qLocation.longitude = location.longitude;
    qLocation.altitude = location.altitude;
// TODO
//    qLocation.altitudeMeanSeaLevel = location.altitudeMeanSeaLevel;
    qLocation.speed = location.speed;
    qLocation.bearing = location.bearing;
    qLocation.accuracy = location.horizontalAccuracy;
    qLocation.flags = location.flags;
    qLocation.flags &= ~QAPI_LOCATION_HAS_ALTITUDE_MSL_BIT;
    qLocation.verticalAccuracy = location.verticalAccuracy;
    qLocation.speedAccuracy = location.speedAccuracy;
    qLocation.bearingAccuracy = location.bearingAccuracy;

    print_qLocation_array(&qLocation, 1);
    if (bIsSingleShot) {
        LOC_LOGd("Invoking Singleshot Callback");
        if (qLocationCallbacks.singleShotCb) {
            qLocationCallbacks.singleShotCb(qLocation, QAPI_LOCATION_ERROR_SUCCESS);
        } else {
            LOC_LOGe("No singleshot cb registered");
        }
    } else {
        LOC_LOGd("Invoking Tracking Callback");
        if (qLocationCallbacks.trackingCb) {
            qLocationCallbacks.trackingCb(qLocation);
        }
        else {
            LOC_LOGe("No tracking cb registered");
        }
    }
}

static void gnss_location_tracking_callback(
    GnssLocation gnsslocation)
{
    qapi_Location_t qLocation = {};
    bool bIsSingleShot = false;

    // first check if location is valid
    if (0 == gnsslocation.flags) {
        LOC_LOGd("Ignore invalid location");
        return;
    }

    if (mTimer.isStarted()) {
        LOC_LOGd("Timer was started, meaning this is singleshot");
        if (nullptr != pLocClientApi) {
            LOC_LOGd("Stop singleshot session");
            pLocClientApi->stopPositionSession();
        }
        LOC_LOGd("Stop singleshot timer");
        mTimer.stop();
        bIsSingleShot = true;
    }
    qLocation.size = sizeof(qapi_Location_t);
    qLocation.timestamp = gnsslocation.timestamp;
    qLocation.latitude = gnsslocation.latitude;
    qLocation.longitude = gnsslocation.longitude;
    qLocation.altitude = gnsslocation.altitude;
    qLocation.altitudeMeanSeaLevel = gnsslocation.altitudeMeanSeaLevel;
    qLocation.speed = gnsslocation.speed;
    qLocation.bearing = gnsslocation.bearing;
    qLocation.accuracy = gnsslocation.horizontalAccuracy;
    qLocation.flags = gnsslocation.flags;
    if (gnsslocation.gnssInfoFlags & GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT) {
        qLocation.flags |= QAPI_LOCATION_HAS_ALTITUDE_MSL_BIT;
    }
    qLocation.verticalAccuracy = gnsslocation.verticalAccuracy;
    qLocation.speedAccuracy = gnsslocation.speedAccuracy;
    qLocation.bearingAccuracy = gnsslocation.bearingAccuracy;

    print_qLocation_array(&qLocation, 1);
    if (bIsSingleShot) {
        LOC_LOGd("Invoking Singleshot Callback");
        if (qLocationCallbacks.singleShotCb) {
            qLocationCallbacks.singleShotCb(qLocation, QAPI_LOCATION_ERROR_SUCCESS);
        } else {
            LOC_LOGe("No singleshot cb registered");
        }
    } else {
        LOC_LOGd("Invoking Tracking Callback");
        if (qLocationCallbacks.trackingCb) {
            qLocationCallbacks.trackingCb(qLocation);
        }
        else {
            LOC_LOGe("No tracking cb registered");
        }
    }
}

static void loc_passive_capabilities_callback(
    LocationCapabilitiesMask capsMask)
{
    LOC_LOGv("loc_passive_capabilities_callback!");
}

static void loc_passive_tracking_callback(
    GnssLocation gnsslocation)
{
    qCacheLocation.size = sizeof(qapi_Location_t);
    qCacheLocation.timestamp = gnsslocation.timestamp;
    qCacheLocation.latitude = gnsslocation.latitude;
    qCacheLocation.longitude = gnsslocation.longitude;
    qCacheLocation.altitude = gnsslocation.altitude;
    qCacheLocation.altitudeMeanSeaLevel = gnsslocation.altitudeMeanSeaLevel;
    qCacheLocation.speed = gnsslocation.speed;
    qCacheLocation.bearing = gnsslocation.bearing;
    qCacheLocation.accuracy = gnsslocation.horizontalAccuracy;
    qCacheLocation.flags = gnsslocation.flags;
    if (gnsslocation.gnssInfoFlags & GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT) {
        qCacheLocation.flags |= QAPI_LOCATION_HAS_ALTITUDE_MSL_BIT;
    }
    qCacheLocation.flags |= QAPI_LOCATION_IS_BEST_AVAIL_POS_BIT;
    qCacheLocation.verticalAccuracy = gnsslocation.verticalAccuracy;
    qCacheLocation.speedAccuracy = gnsslocation.speedAccuracy;
    qCacheLocation.bearingAccuracy = gnsslocation.bearingAccuracy;

    print_qLocation_array(&qCacheLocation, 1);
}

static void loc_passive_response_callback(
    LocationResponse err)
{
    LOC_LOGv("loc_passive_response_callback!");
}

struct MyClientCallbacks {
    CapabilitiesCb capabilitycb;
    ResponseCb responsecb;
    CollectiveResponseCb collectivecb;
    LocationCb locationcb;
};

static MyClientCallbacks gLocationCallbacks = {
    location_capabilities_callback,
    location_response_callback,
    location_collective_response_callback,
    location_tracking_callback,
};

#define LOC_PATH_QAPI_CONF_STR      "/etc/qapi.conf"

const char LOC_PATH_QAPI_CONF[] = LOC_PATH_QAPI_CONF_STR;
static uint32_t gDebug = 0;
static uint32_t gSingleshotTimeout = 0;

static const loc_param_s_type gConfigTable[] =
{
    { "DEBUG_LEVEL", &gDebug, NULL, 'n' }
};

static const loc_param_s_type gQapiConfigTable[] =
{
    { "SINGLESHOT_TIMEOUT", &gSingleshotTimeout, NULL, 'n' }
};

/* Parameters we need in a configuration file specifically for QAPI:
    SINGLESHOT_TIMEOUT
    */

extern "C" {
    qapi_Location_Error_t qapi_Loc_Init(
        qapi_loc_client_id* pClientId,
        const qapi_Location_Callbacks_t* pCallbacks)
    {
        qapi_Location_Error_t retVal =
            QAPI_LOCATION_ERROR_SUCCESS;

        // read configuration file
        UTIL_READ_CONF(LOC_PATH_GPS_CONF, gConfigTable);
        LOC_LOGd("gDebug=%u LOC_PATH_GPS_CONF=%s", gDebug, LOC_PATH_GPS_CONF);

        UTIL_READ_CONF(LOC_PATH_QAPI_CONF, gQapiConfigTable);
        LOC_LOGd("gSingleshotTimeout=%u LOC_PATH_QAPI_CONF=%s", gSingleshotTimeout, LOC_PATH_QAPI_CONF);

        MyClientCallbacks locationCallbacks = gLocationCallbacks;

        LOC_LOGd("qapi_Loc_Init! pCallbacks %p"
            "cap %p res %p col %p trk %p",
            pCallbacks,
            pCallbacks->capabilitiesCb,
            pCallbacks->responseCb,
            pCallbacks->collectiveResponseCb,
            pCallbacks->trackingCb);

        LOC_LOGd("qapi_Loc_Init! pCallbacks %p"
            "bat %p gf %p ss %p gd %p",
            pCallbacks,
            pCallbacks->batchingCb,
            pCallbacks->geofenceBreachCb,
            pCallbacks->singleShotCb,
            pCallbacks->gnssDataCb);

        if (!qMutexInitDone)
        {
            pthread_mutex_init(&qMutex, NULL);
            qMutexInitDone = true;
        }

        /* Input sanity */
        if (NULL == pCallbacks->capabilitiesCb ||
            NULL == pCallbacks->responseCb ||
            NULL == pCallbacks->collectiveResponseCb)
        {
            return QAPI_LOCATION_ERROR_CALLBACK_MISSING;
        }
        /* Don't register callbacks not provided by client */
        if (NULL == pCallbacks->trackingCb) {
            locationCallbacks.locationcb = NULL;
        }
        pthread_mutex_lock(&qMutex);

        do
        {
            qLocationCallbacks = *pCallbacks;
            if (nullptr == pLocClientApi) {
                pLocClientApi = new LocationClientApi(
                    location_capabilities_callback);
                retVal = QAPI_LOCATION_ERROR_SUCCESS;
            } else {
                retVal = QAPI_LOCATION_ERROR_ALREADY_STARTED;
            }
            /* Start passive listener here in order to be able to satisfy
            qapi_Loc_Get_Best_Available_Position call */

            if (nullptr == pLocPassiveClientApi) {
                pLocPassiveClientApi = new LocationClientApi(
                    loc_passive_capabilities_callback);
                retVal = QAPI_LOCATION_ERROR_SUCCESS;
                bool ret;
                memset(&qCacheLocation, 0, sizeof(qCacheLocation));
                qCacheLocation.flags |= QAPI_LOCATION_IS_BEST_AVAIL_POS_BIT;

                GnssReportCbs gnnsReportCbs;
                memset(&gnnsReportCbs, 0, sizeof(gnnsReportCbs));
                gnnsReportCbs.gnssLocationCallback = loc_passive_tracking_callback;

                ret = pLocPassiveClientApi->startPositionSession(
                                    0,  // both 0 means passive listener
                                    gnnsReportCbs,
                                    loc_passive_response_callback);
                if (!ret) {
                    retVal = QAPI_LOCATION_ERROR_GENERAL_FAILURE;
                }
            } else {
                retVal = QAPI_LOCATION_ERROR_ALREADY_STARTED;
            }

            *pClientId = 0;
        } while (0);
        pthread_mutex_unlock(&qMutex);
        return retVal;
    }

    qapi_Location_Error_t qapi_Loc_Deinit(
        qapi_loc_client_id clientId)
    {
        qapi_Location_Error_t retVal =
            QAPI_LOCATION_ERROR_ID_UNKNOWN;

        LOC_LOGd("gDebug=%u LOC_PATH_GPS_CONF=%s", gDebug, LOC_PATH_GPS_CONF);
        LOC_LOGd("qapi_Loc_Deinit client %d", clientId);

        pthread_mutex_lock(&qMutex);
        if (nullptr != pLocClientApi) {
            delete pLocClientApi;
            pLocClientApi = nullptr;
            retVal = QAPI_LOCATION_ERROR_SUCCESS;
        }
        pthread_mutex_unlock(&qMutex);
        return retVal;
    }

    qapi_Location_Error_t qapi_Loc_Set_User_Buffer(
        qapi_loc_client_id clientId,
        uint8_t* pUserBuffer,
        size_t bufferSize)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Start_Tracking(
        qapi_loc_client_id clientId,
        const qapi_Location_Options_t* pOptions,
        uint32_t* pSessionId)
    {
        qapi_Location_Error_t retVal =
            QAPI_LOCATION_ERROR_SUCCESS;

        LOC_LOGd("qapi_Loc_Start_Tracking! pOptions=%p pSessionId=%p clientId=%d",
            pOptions, pSessionId, clientId);

        pthread_mutex_lock(&qMutex);
        do {
            if (NULL == pOptions)
            {
                LOC_LOGe("pOptions NULL");
                retVal = QAPI_LOCATION_ERROR_INVALID_PARAMETER;
                break;
            }

            LOC_LOGd("qapi_Loc_Start_Tracking! minInterval=%d minDistance=%d",
                pOptions->minInterval,
                pOptions->minDistance);

            if (nullptr != pLocClientApi) {
                bool ret;
                ret = pLocClientApi->startPositionSession(
                        pOptions->minInterval,
                        pOptions->minDistance,
                        location_tracking_callback,
                        location_response_callback);
                if (!ret) {
                    retVal = QAPI_LOCATION_ERROR_GENERAL_FAILURE;
                }
            } else {
                retVal = QAPI_LOCATION_ERROR_GENERAL_FAILURE;
            }
        } while (0);
        *pSessionId = 1;
        pthread_mutex_unlock(&qMutex);
        return retVal;
    }

    qapi_Location_Error_t qapi_Loc_Stop_Tracking(
        qapi_loc_client_id clientId,
        uint32_t sessionId)
    {
        qapi_Location_Error_t retVal =
            QAPI_LOCATION_ERROR_SUCCESS;

        LOC_LOGv("qapi_Loc_Stop_Tracking! clientId %d, sessionId %d",
            clientId, sessionId);

        pthread_mutex_lock(&qMutex);

        if (nullptr != pLocClientApi) {
            pLocClientApi->stopPositionSession();
        } else {
            retVal = QAPI_LOCATION_ERROR_GENERAL_FAILURE;
        }
        pthread_mutex_unlock(&qMutex);
        return retVal;
    }

    qapi_Location_Error_t qapi_Loc_Update_Tracking_Options(
        qapi_loc_client_id clientId,
        uint32_t sessionId,
        const qapi_Location_Options_t* pOptions)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Start_Batching(
        qapi_loc_client_id clientId,
        const qapi_Location_Options_t* pOptions,
        uint32_t* pSessionId)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Stop_Batching(
        qapi_loc_client_id clientId,
        uint32_t sessionId)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Update_Batching_Options(
        qapi_loc_client_id clientId,
        uint32_t sessionId,
        const qapi_Location_Options_t* pOptions)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Get_Batched_Locations(
        qapi_loc_client_id clientId,
        uint32_t sessionId,
        size_t count)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Add_Geofences(
        qapi_loc_client_id clientId,
        size_t count,
        const qapi_Geofence_Option_t* pOptions,
        const qapi_Geofence_Info_t* pInfo,
        uint32_t** pIdArray)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Remove_Geofences(
        qapi_loc_client_id clientId,
        size_t count,
        const uint32_t* pIDs)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Modify_Geofences(
        qapi_loc_client_id clientId,
        size_t count,
        const uint32_t* pIDs,
        const qapi_Geofence_Option_t* options)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Pause_Geofences(
        qapi_loc_client_id clientId,
        size_t count,
        const uint32_t* pIDs)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Resume_Geofences(
        qapi_loc_client_id clientId,
        size_t count,
        const uint32_t* pIDs)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Get_Single_Shot(
        qapi_loc_client_id clientId,
        qapi_Location_Power_Level_t powerLevel,
        uint32_t* pSessionId)
    {
        qapi_Location_Error_t retVal =
            QAPI_LOCATION_ERROR_SUCCESS;
        bool bFound = false;
        qapi_Location_Options_t options;

        LOC_LOGd("qapi_Loc_Get_Single_Shot! pSessionId=%p clientId=%d",
            pSessionId, clientId);

        pthread_mutex_lock(&qMutex);
        do {
            if (QAPI_LOCATION_POWER_HIGH != powerLevel &&
                QAPI_LOCATION_POWER_LOW != powerLevel)
            {
                LOC_LOGe("powerLevel is invalid");
                retVal = QAPI_LOCATION_ERROR_INVALID_PARAMETER;
                break;
            }

            LOC_LOGd("qapi_Loc_Get_Single_Shot! powerLevel=%d",
                     powerLevel);

            GnssReportCbs gnnsReportCbs;
            memset(&gnnsReportCbs, 0, sizeof(gnnsReportCbs));
            gnnsReportCbs.gnssLocationCallback = gnss_location_tracking_callback;

            if (nullptr != pLocClientApi) {
                bool ret;
                ret = pLocClientApi->startPositionSession(
                        1000,
                        gnnsReportCbs,
                        location_response_callback);
                if (!ret) {
                    retVal = QAPI_LOCATION_ERROR_GENERAL_FAILURE;
                } else {
                    Runnable timerRunnable = [pLocClientApi] {
                        pLocClientApi->stopPositionSession();
                        if (qLocationCallbacks.singleShotCb) {
                            qapi_Location_t qLocation;
                            qLocationCallbacks.singleShotCb(qLocation, QAPI_LOCATION_ERROR_TIMEOUT);
                        } else {
                            LOC_LOGe("No singleshot cb registered");
                        }
                    };
                    mTimer.set(gSingleshotTimeout, timerRunnable);
                    mTimer.start();
                }
            } else {
                retVal = QAPI_LOCATION_ERROR_GENERAL_FAILURE;
            }
        } while (0);
        *pSessionId = 1;
        pthread_mutex_unlock(&qMutex);
        return retVal;
    }

    qapi_Location_Error_t qapi_Loc_Cancel_Single_Shot(
        qapi_loc_client_id clientId,
        uint32_t sessionId)
    {
        qapi_Location_Error_t retVal =
            QAPI_LOCATION_ERROR_SUCCESS;

        LOC_LOGv("qapi_Loc_Cancel_Single_Shot! clientId %d, sessionId %d",
            clientId, sessionId);

        pthread_mutex_lock(&qMutex);
        if (mTimer.isStarted()) {
            LOC_LOGd("Timer was started, singleshot session in progress");
            if (nullptr != pLocClientApi) {
                LOC_LOGd("Stop singleshot session");
                pLocClientApi->stopPositionSession();
            }
            LOC_LOGd("Stop singleshot timer");
            mTimer.stop();
        } else {
            LOC_LOGd("No singleshot session in progress!");
        }
        pthread_mutex_unlock(&qMutex);
        return retVal;
    }

    qapi_Location_Error_t qapi_Loc_Start_Get_Gnss_Data(
        qapi_loc_client_id clientId,
        uint32_t* pSessionId)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Stop_Get_Gnss_Data(
        qapi_loc_client_id clientId,
        uint32_t sessionId)
    {
        return QAPI_LOCATION_ERROR_NOT_SUPPORTED;
    }

    qapi_Location_Error_t qapi_Loc_Get_Best_Available_Position(
        qapi_loc_client_id clientId,
        uint32_t* pSessionId)
    {
        qapi_Location_Error_t retVal = QAPI_LOCATION_ERROR_SUCCESS;

        LOC_LOGv("qapi_Loc_Get_Best_Available_Position! clientId %d", clientId);
        pthread_mutex_lock(&qMutex);
        do {
            if (nullptr != pLocClientApi) {
                Runnable timerRunnable = [] {
                    if (qLocationCallbacks.singleShotCb) {
                        qLocationCallbacks.singleShotCb(qCacheLocation, QAPI_LOCATION_ERROR_SUCCESS);
                    } else {
                        LOC_LOGe("No singleshot cb registered");
                    }
                };
                mPassiveTimer.set(500, timerRunnable);
                mPassiveTimer.start();

            } else {
                retVal = QAPI_LOCATION_ERROR_GENERAL_FAILURE;
            }
        } while (0);

        pthread_mutex_unlock(&qMutex);
        return retVal;
    }

} /* extern "C" */
