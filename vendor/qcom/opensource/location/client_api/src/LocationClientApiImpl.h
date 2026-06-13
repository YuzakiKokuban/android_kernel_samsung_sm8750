/* Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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

/*
Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LOCATIONCLIENTAPIIMPL_H
#define LOCATIONCLIENTAPIIMPL_H

#include <mutex>

#include <loc_pla.h>
#include <LocIpc.h>
#include <LocationDataTypes.h>
#include <ILocationAPI.h>
#include <MsgTask.h>
#include <LocationApiMsg.h>
#include <LocationApiPbMsgConv.h>
#include <LCAReportLoggerUtil.h>
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>
#include <chrono>

using namespace loc_util;

namespace location_client
{
enum LocationCallbackType {
    // Tracking callbacks type
    TRACKING_CBS = 0,
    // Batching callbacks type
    BATCHING_CBS = 1,
    // Geofence callbacks type
    GEOFENCE_CBS = 2
};

typedef std::function<void(
    uint32_t response
)> PingTestCb;


class GeofenceImpl: public std::enable_shared_from_this<GeofenceImpl> {
    uint32_t mId;
    Geofence mGeofence;
    static uint32_t nextId();
    static mutex mGfMutex;
public:
    GeofenceImpl(Geofence* geofence) : mId(nextId()), mGeofence(*geofence) {
    }
    void bindGeofence(Geofence* geofence) {
        geofence->mGeofenceImpl = shared_from_this();
    }
    inline uint32_t getClientId() { return mId; }
};

// utility for wait / notify
class Waitable {
    std::mutex mMutex;
    std::condition_variable mCond;
public:
    Waitable() = default;
    ~Waitable() = default;

    void wait(uint32_t ms) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCond.wait_for(lock, std::chrono::milliseconds(ms));
    }

    void notify() {
        mCond.notify_one();
    }
};

class IpcListener;

class LocationClientApiImpl : public ILocationAPI, public Waitable {
    friend IpcListener;
public:
    LocationClientApiImpl(capabilitiesCallback capabitiescb);
    virtual void destroy(locationApiDestroyCompleteCallback destroyCompleteCb=nullptr) override;

    // Tracking
    virtual void updateCallbacks(LocationCallbacks&) override;

    virtual uint32_t startTracking(const TrackingOptions&) override;

    virtual void stopTracking(uint32_t id) override;

    virtual void updateTrackingOptions(uint32_t id, const TrackingOptions&) override;

    //Batching
    virtual uint32_t startBatching(const BatchingOptions&) override;

    virtual void stopBatching(uint32_t id) override;

    virtual void updateBatchingOptions(uint32_t id, const BatchingOptions&) override;

    virtual void getBatchedLocations(uint32_t id, size_t count) override;

    //Geofence
    virtual uint32_t* addGeofences(size_t count, GeofenceOption*,
                                   GeofenceInfo*) override;

    virtual void removeGeofences(size_t count, uint32_t* ids) override;

    virtual void modifyGeofences(size_t count, uint32_t* ids,
                                 GeofenceOption* options) override;

    virtual void pauseGeofences(size_t count, uint32_t* ids) override;

    virtual void resumeGeofences(size_t count, uint32_t* ids) override;

    //GNSS
    virtual void gnssNiResponse(uint32_t id, GnssNiResponse response) override;

    virtual void getDebugReport(GnssDebugReport& reports) override;

    virtual uint32_t getAntennaInfo(AntennaInfoCallback* cb) override;

    // other interface
    void startPositionSession(const LocationCallbacks& callbacksOption,
                              const TrackingOptions& trackingOptions);

    void startBatchingSession(const LocationCallbacks& callbacksOption,
                              const BatchingOptions& batchOptions);

    void updateNetworkAvailability(bool available);
    void getGnssEnergyConsumed(gnssEnergyConsumedCallback gnssEnergyConsumedCb,
            responseCallback responseCb);
    void updateLocationSystemInfoListener(
            locationSystemInfoCallback locationSystemInfoCb,
            responseCallback responseCb);

    uint32_t startTrackingSync(const TrackingOptions&);
    uint32_t startBatchingSync(const BatchingOptions&);
    void updateCallbacksSync(LocationCallbacks& callbacks);

    void addGeofences(const LocationCallbacks& callbacksOption,
                      const std::vector<Geofence>& geofences);
    inline Geofence getMappedGeofence(uint32_t id) {
        return mGeofenceMap.at(id);
    }

    inline uint16_t getYearOfHw() {return mYearOfHw;}

    void getSingleTerrestrialPos(uint32_t timeoutMsec, TerrestrialTechMask techMask,
                                 float horQoS, trackingCallback terrestrialPositionCallback,
                                 responseCallback responseCallback);
    void getSinglePos(uint32_t timeoutMsec, float horQoS,
                      trackingCallback positionCb,
                      responseCallback responseCb);

    void pingTest(PingTestCb pingTestCallback);

    static LocationSystemInfo parseLocationSystemInfo(
            const::LocationSystemInfo &halSystemInfo);
    static LocationResponse parseLocationError(::LocationError error);
    static GnssMeasurements parseGnssMeasurements(const ::GnssMeasurementsNotification
            &halGnssMeasurements);
    static GnssData parseGnssData(const ::GnssDataNotification &halGnssData);
    static GnssSv parseGnssSv(const ::GnssSv &halGnssSv);
    static GnssLocation parseLocationInfo(const ::GnssLocationInfoNotification &halLocationInfo);
    static GnssSystemTime parseSystemTime(const ::GnssSystemTime &halSystemTime);
    static GnssGloTimeStructType parseGloTime(const ::GnssGloTimeStructType &halGloTime);
    static GnssSystemTimeStructType parseGnssTime(const ::GnssSystemTimeStructType &halGnssTime);
    static LocationReliability parseLocationReliability(
            const ::LocationReliability &halReliability);
    static GnssLocationPositionDynamics parseLocationPositionDynamics(
            const ::GnssLocationPositionDynamics &halPositionDynamics,
            const ::GnssLocationPositionDynamicsExt &halPositionDynamicsExt);
    static void parseGnssMeasUsageInfo(const ::GnssLocationInfoNotification &halLocationInfo,
            std::vector<GnssMeasUsageInfo>& clientMeasUsageInfo);
    static GnssSignalTypeMask parseGnssSignalType(
            const ::GnssSignalTypeMask &halGnssSignalTypeMask);
    static GnssLocationSvUsedInPosition parseLocationSvUsedInPosition(
            const ::GnssLocationSvUsedInPosition &halSv);
    static void parseLocation(const ::Location &halLocation, Location& location);
    static Location parseLocation(const ::Location &halLocation);
    static uint16_t parseYearOfHw(::LocationCapabilitiesMask mask);
    static LocationCapabilitiesMask parseCapabilitiesMask(::LocationCapabilitiesMask mask);
    static GnssMeasurementsDataFlagsMask parseMeasurementsDataMask(
            ::GnssMeasurementsDataFlagsMask in);
    static GnssEnergyConsumedInfo parseGnssConsumedInfo(::GnssEnergyConsumedInfo);
    static GnssDcReport parseDcReport(const::GnssDcReportInfo &halDcReport);
    static GeofenceBreachTypeMask parseGeofenceBreachType(GeofenceBreachType breachType);
    static GeofenceBreachType parseGeofenceBreachTypeMask(::GeofenceBreachTypeMask breachTypeMask);

    static void parseEphSrcAndAction(const ::GnssEphAction& halEphAction,
            GnssEphSource& ephSrc, GnssEphAction& ephAction);
    static GnssEphemeris parseGnssEphemerisInfo(const ::GnssSvEphemerisReport &ephInfo);
    static void parseCommanGnssEphemeris(const ::GnssEphCommon& halCommanEph,
            GnssEphCommonInfo& lcaCommanEph);
    static void parseGloEphemeris( const GlonassEphemerisResponse& halEph,
                std::vector<GlonassEphemeris>& lcaEphInfo);
    static void parseGalEphemeris( const GalileoEphemerisResponse& halEph,
                std::vector<GalileoEphemeris>& lcaEphInfo);
    static void parseGpsEphemeris(const GpsEphemerisResponse& halEph,
                std::vector<GpsQzssEphemeris>& lcaEphInfo);
    static void parseBdsEphemeris(const BdsEphemerisResponse& halEph,
                std::vector<BdsEphemeris>& lcaEphInfo);
    static void parseQzssEphemeris(const QzssEphemerisResponse& halEph,
                std::vector<QzssEphemeris>& lcaEphInfo);
    static void parseNavicEphemeris(const NavicEphemerisResponse& halEph,
                std::vector<NavicEphemeris>& lcaEphInfo);

    void logLocation(const Location &location,
                     LocReportTriggerType reportTriggerType);
    void logLocation(const GnssLocation &gnssLocation,
                     LocReportTriggerType reportTriggerType);
    void logGeofenceBreach(const GeofenceBreachNotification& breachNotif,
            const std::vector<Geofence> &geofences);

    LCAReportLoggerUtil & getLogger() {
        return mLogger;
    }

    void stopTrackingAndClearSubscriptions(uint32_t id);
    void clearSubscriptions(LocationCallbackType cbTypeToClear);
    void stopTrackingSync(bool clearSubscriptions);
    bool isInTracking() { return mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID; }
    bool isInBatching() { return mBatchingId != LOCATION_CLIENT_SESSION_ID_INVALID; }

private:
    ~LocationClientApiImpl();

    inline LocationCapabilitiesMask getCapabilities() {return mCapsMask;}
    void capabilitesCallback(ELocMsgID  msgId, const void* msgData);
    void updateTrackingOptionsSync(const TrackingOptions& option, bool clearSubscriptions);
    bool checkGeofenceMap(size_t count, uint32_t* ids);
    void addGeofenceMap(Geofence& geofence);
    void eraseGeofenceMap(size_t count, uint32_t* ids);
    bool isGeofenceMapEmpty();

    // convenient methods
    inline bool sendMessage(const uint8_t* data, uint32_t length) const {
        return (mIpcSender != nullptr) && LocIpc::send(*mIpcSender, data, length);
    }

    void invokePositionSessionResponseCb(LocationError errCode);
    void diagLogGnssLocation(const GnssLocation &gnssLocation);
    void processGetDebugRespCb(const LocAPIGetDebugRespMsg* pRespMsg);
    void processAntennaInfo(const LocAPIAntennaInfoMsg* pAntennaInfoMsg);

    // protobuf conversion util class
    LocationApiPbMsgConv mPbufMsgConv;

    // internal session parameter
    static uint32_t         mClientIdGenerator;
    static uint32_t         mClientIdIndex;
    static mutex            mMutex;
    uint32_t                mClientId;
    uint32_t                mSessionId;
    uint32_t                mBatchingId;
    bool                    mHalRegistered;
    // For client on different processor, socket name will start with
    // defined constant of SOCKET_TO_EXTERANL_AP_LOCATION_CLIENT_BASE.
    // For client on same processor, socket name will start with
    // SOCKET_LOC_CLIENT_DIR + LOC_CLIENT_NAME_PREFIX.
    char                       mSocketName[MAX_SOCKET_PATHNAME_LENGTH];
    // for client on a different processor, 0 is invalid
    uint32_t                   mInstanceId;
    LocationCallbacksMask      mCallbacksMask;
    LocationOptions            mLocationOptions;
    BatchingOptions            mBatchingOptions;
    LocationCapabilitiesMask   mCapsMask;
    //Year of HW information, 0 is invalid
    uint16_t                   mYearOfHw;
    bool                       mPositionSessionResponseCbPending;
    uint64_t                   mSessionStartBootTimestampNs;
    GnssDebugReport*           mpDebugReport;
    AntennaInfoCallback*       mpAntennaInfoCb;

    // callbacks
    LocationCallbacks       mLocationCbs;

    //TODO:: remove after replacing all calls with ILocationAPI callbacks
    capabilitiesCallback    mCapabilitiesCb;
    PingTestCb              mPingTestCb;

    gnssEnergyConsumedCallback    mGnssEnergyConsumedInfoCb;
    responseCallback              mGnssEnergyConsumedResponseCb;

    locationSystemInfoCallback    mLocationSysInfoCb;
    responseCallback              mLocationSysInfoResponseCb;

    // Terrestrial fix callback
    trackingCallback              mSingleTerrestrialPosCb;
    responseCallback              mSingleTerrestrialPosRespCb;

    // Single fix callback
    trackingCallback           mSinglePosCb;
    responseCallback           mSinglePosRespCb;

    MsgTask                    mMsgTask;

    LocIpc                     mIpc;
    shared_ptr<LocIpcSender>   mIpcSender;

    std::vector<uint32_t>      mLastAddedClientIds;
    // clientId --> Geofence object
    std::unordered_map<uint32_t, Geofence> mGeofenceMap;

    LCAReportLoggerUtil        mLogger;
};

} // namespace location_client

#endif /* LOCATIONCLIENTAPIIMPL_H */
