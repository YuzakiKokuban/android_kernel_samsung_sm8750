/* Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation, nor the names of its
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
#ifndef LOCATIONAPISERVICE_H
#define LOCATIONAPISERVICE_H

#include <string>
#include <mutex>

#include <loc_pla.h>
#include <MsgTask.h>
#include <loc_cfg.h>
#include <LocIpc.h>
#include <LocTimer.h>
#include <location_interface.h>
#include <ILocationAPI.h>
#include <LocationApiMsg.h>

#include <LocHalDaemonClientHandler.h>
#include <unordered_map>

#undef LOG_TAG
#define LOG_TAG "LocSvc_HalDaemon"

typedef struct {
    uint32_t autoStartGnss;
    uint32_t gnssSessionTbfMs;
    uint32_t deleteAllBeforeAutoStart;
    uint32_t posEngineMask;
    uint32_t positionMode;
} configParamToRead;


/******************************************************************************
LocationApiService
******************************************************************************/

typedef struct {
    // this stores the client name and the command type that client requests
    // the info will be used to send back command response
    std::string clientName;
    ELocMsgID   configMsgId;
} ConfigReqClientData;

// periodic timer to perform maintenance work, e.g.: resource cleanup
// for location hal daemon
typedef std::unordered_map<std::string, shared_ptr<LocIpcSender>> ClientNameIpcSenderMap;
class MaintTimer : public LocTimer {
public:
    MaintTimer(LocationApiService* locationApiService) :
            mLocationApiService(locationApiService) {
    };

    ~MaintTimer()  = default;

public:
    void timeOutCallback() override;

private:
    LocationApiService* mLocationApiService;
};

enum SingleShotTimerType{
    SINGLE_SHOT_FIX_TIMER_TERRESTRIAL = 1,
    SINGLE_SHOT_FIX_TIMER_FUSED = 2,
};

class SingleFixTimer : public LocTimer {
public:

    SingleFixTimer(LocationApiService* locationApiService,
                   std::string& clientName,
                   SingleShotTimerType timerType) :
            mLocationApiService(locationApiService),
            mClientName(clientName), mTimerType(timerType) {
        LOC_LOGv("timer constructor called");
    }

    ~SingleFixTimer() {LOC_LOGv("timer destructed called");}

public:
    void timeOutCallback() override;

private:
    LocationApiService* mLocationApiService;
    const std::string   mClientName;
    SingleShotTimerType mTimerType;
};

// This keeps track of the client that requests single shot gtp or fused
// fix and the timer that will fire when the timeout value has reached
typedef std::unordered_map<std::string, SingleFixTimer>
        SingleFixClientTimeoutMap;

class SingleFixReqInfo {
public:
    float horQoS;
    SingleFixTimer* timeoutTimer;

    inline SingleFixReqInfo(float inQoS, SingleFixTimer* inTimer) :
            horQoS(inQoS), timeoutTimer(inTimer) {
    }

    inline SingleFixReqInfo(SingleFixReqInfo&& inReq) {
        horQoS = inReq.horQoS;
        timeoutTimer = inReq.timeoutTimer;
        inReq.timeoutTimer = nullptr;
    }
    inline ~SingleFixReqInfo() {
        if (timeoutTimer != nullptr) {
            delete timeoutTimer;
        }
        timeoutTimer = nullptr;
    }
};

typedef std::unordered_map<std::string, SingleFixReqInfo> SingleFixReqMap;

class LocationApiService
{
public:

    // singleton instance
    LocationApiService(const LocationApiService&) = delete;
    LocationApiService& operator = (const LocationApiService&) = delete;

    static LocationApiService* getInstance(
            const configParamToRead & configParamRead) {
        if (nullptr == mInstance) {
            mInstance = new LocationApiService(configParamRead);
        }
        return mInstance;
    }

    // APIs can be invoked by IPC
    void processClientMsg(const char* data, uint32_t length);

    // from IPC receiver
    void onListenerReady(bool externalApIpc);

    // other APIs
    void deleteClientbyName(const std::string name, bool forceRemove = true);
    void deleteEapClientByIds(int id1, int id2);

    // protobuf conversion util class
    LocationApiPbMsgConv mPbufMsgConv;

    static std::recursive_mutex mMutex;

    // Utility routine used by maintenance timer
    void performMaintenance();

    // Utility routine used by gtp fix timeout timer
    void gtpFixRequestTimeout(const std::string& clientName);

    // Utility routine used by fused fix timeout timer
    void singleFixRequestTimeout(const std::string& clientName);

    inline const MsgTask& getMsgTask() const {return mMsgTask;};

private:
    // APIs can be invoked to process client's IPC messgage
    void newClient(LocAPIClientRegisterReqMsg*);
    void deleteClient(LocAPIClientDeregisterReqMsg*);

    void startTracking(LocAPIStartTrackingReqMsg*);
    void stopTracking(LocAPIStopTrackingReqMsg*);

    void updateSubscription(LocAPIUpdateCallbacksReqMsg*);
    void updateTrackingOptions(LocAPIUpdateTrackingOptionsReqMsg*);
    void updateNetworkAvailability(bool availability);
    void getGnssEnergyConsumed(const char* clientSocketName);
    void getSingleTerrestrialPos(LocAPIGetSingleTerrestrialPosReqMsg*);
    void getSinglePos(LocAPIGetSinglePosReqMsg*);
    void stopTrackingSessionForSingleFixes();

    void startBatching(LocAPIStartBatchingReqMsg*);
    void stopBatching(LocAPIStopBatchingReqMsg*);
    void updateBatchingOptions(LocAPIUpdateBatchingOptionsReqMsg*);

    void addGeofences(LocAPIAddGeofencesReqMsg*);
    void removeGeofences(LocAPIRemoveGeofencesReqMsg*);
    void modifyGeofences(LocAPIModifyGeofencesReqMsg*);
    void pauseGeofences(LocAPIPauseGeofencesReqMsg*);
    void resumeGeofences(LocAPIResumeGeofencesReqMsg*);

    void pingTest(LocAPIPingTestReqMsg*);

    inline uint32_t gnssUpdateConfig(const GnssConfig& config) {
        uint32_t* sessionIds =  mLocationControlApi->gnssUpdateConfig(config);
        // in our usage, we only configure one setting at a time,
        // so we have only one sessionId
        uint32_t sessionId = 0;
        if (sessionIds) {
            sessionId = *sessionIds;
            delete [] sessionIds;
        }
        return sessionId;
    }

    // Location control API callback
    void onControlResponseCallback(LocationError err, uint32_t id);
    void onControlCollectiveResponseCallback(size_t count, LocationError *errs, uint32_t *ids);
    void onGnssConfigCallback(uint32_t sessionId, const GnssConfig& config);
    void onGnssEnergyConsumedCb(uint64_t totalEnergyConsumedSinceFirstBoot);
    void onGnssSignalTypesCb(const GnssCapabNotification& gnssCapabNotification);

    // Callbacks for location api used service GTP WWAN fix request
    void onCapabilitiesCallback(LocationCapabilitiesMask mask);
    void onResponseCb(LocationError err, uint32_t id);
    void onCollectiveResponseCallback(size_t count, LocationError *errs, uint32_t *ids);
    void onGtpWwanTrackingCallback(Location location);
    void onGnssLocationInfoCb(const GnssLocationInfoNotification& notification);

    // Location configuration API requests
    void configConstrainedTunc(
            const LocConfigConstrainedTuncReqMsg* pMsg);
    void configPositionAssistedClockEstimator(
            const LocConfigPositionAssistedClockEstimatorReqMsg* pMsg);
    void configConstellations(
            const LocConfigSvConstellationReqMsg* pMsg);
    void configConstellationSecondaryBand(
            const LocConfigConstellationSecondaryBandReqMsg* pMsg);
    void configAidingDataDeletion(
            LocConfigAidingDataDeletionReqMsg* pMsg);
    void configLeverArm(const LocConfigLeverArmReqMsg* pMsg);
    void configRobustLocation(const LocConfigRobustLocationReqMsg* pMsg);
    void configMinGpsWeek(const LocConfigMinGpsWeekReqMsg* pMsg);
    void configDeadReckoningEngineParams(const LocConfigDrEngineParamsReqMsg* pMsg);
    void configMinSvElevation(const LocConfigMinSvElevationReqMsg* pMsg);
    void configEngineRunState(const LocConfigEngineRunStateReqMsg* pMsg);
    void configUserConsentTerrestrialPositioning(
            LocConfigUserConsentTerrestrialPositioningReqMsg* pMsg);
    void configOutputNmeaTypes(const LocConfigOutputNmeaTypesReqMsg* pMsg);
    void configEngineIntegrityRisk(const LocConfigEngineIntegrityRiskReqMsg* pMsg);
    void injectLocation(const LocIntApiInjectLocationMsg* pMsg);
    void configXtraParams(const LocConfigXtraReqMsg* pMsg);
    void configMerkleTree(const LocConfigMerkleTreeReqMsg* pMsg);
    void configOsnmaEnablement(const LocConfigOsnmaEnablementReqMsg* pMsg);
    void registerGnssSignalTypesUpdate(const LocConfigRegisterGnssSignalTypesUpdateReqMsg* pReqMsg);
    void configMmfData(const LocInjectMmfDataReqMsg* pMsg);

    // Location configuration API get/read requests
    void getGnssConfig(const LocAPIMsgHeader* pReqMsg,
                       GnssConfigFlagsBits configFlag);
    void getConstellationSecondaryBandConfig(
            const LocConfigGetConstellationSecondaryBandConfigReqMsg* pReqMsg);
    void getDebugReport(const LocAPIGetDebugReqMsg* pReqMsg);
    void getAntennaInfo(const LocAPIGetAntennaInfoMsg* pMsg);
    void getXtraStatus(const LocConfigGetXtraStatusReqMsg* pReqMsg);
    void registerXtraStatusUpdate(
            const LocConfigRegisterXtraStatusUpdateReqMsg * pReqMsg);
    void deregisterXtraStatusUpdate(
            const LocConfigDeregisterXtraStatusUpdateReqMsg * pReqMsg);

    // Location configuration API util routines
    void addConfigRequestToMap(uint32_t sessionId,
                               const LocAPIMsgHeader* pMsg);

    LocationApiService(const configParamToRead & configParamRead);
    virtual ~LocationApiService();

    // private utilities
    inline LocHalDaemonClientHandler* getClient(const std::string& clientname) {
        // find client from property db
        auto client = mClients.find(clientname);
        if (client == std::end(mClients)) {
            LOC_LOGe("Failed to find client %s", clientname.c_str());
            return nullptr;
        }
        return client->second;
    }

    inline LocHalDaemonClientHandler* getClient(const char* socketName) {
        std::string clientname(socketName);
        return getClient(clientname);
    }

    inline const char* getClientNameByIds(int id1, int id2) {
        for (auto it = mClients.begin(); it != mClients.end(); ++it) {
            if (it->second->getServiceId() == id1 && it->second->getInstanceId() == id2) {
                return it->first.c_str();
            }
        }
        return nullptr;
    }

    GnssInterface* getGnssInterface();
    // OSFramework instance
    void createOSFrameworkInstance();
    void destroyOSFrameworkInstance();

    // singleton instance
    static LocationApiService *mInstance;

    // IPC interface
    LocIpc mIpc;
    unique_ptr<LocIpcRecver> mBlockingRecver;

    // Client propery database
    std::unordered_map<std::string, LocHalDaemonClientHandler*> mClients;
    std::unordered_map<uint32_t, ConfigReqClientData> mConfigReqs;

    // Location Control API interface
    uint32_t mLocationControlId;
    LocationControlCallbacks mControlCallabcks;
    ILocationControlAPI *mLocationControlApi;

    // Configration
    const uint32_t mAutoStartGnss;
    GnssSuplMode   mPositionMode;

    PowerStateType  mPowerState;

    // maintenance timer
    MaintTimer mMaintTimer;

    // msg task used by timers
    const MsgTask   mMsgTask;

    // Terrestrial service related APIs
    // Location api interface for single short wwan fix
    ILocationAPI* mGtpWwanSsLocationApi;
    LocationCallbacks mGtpWwanSsLocationApiCallbacks;
    trackingCallback mGtpWwanPosCallback;
    // -1: not set, 0: user not opt-in, 1: user opt in
    int mOptInTerrestrialService;
    // LIA clients that register for xtra status update
    SingleFixClientTimeoutMap mTerrestrialFixTimeoutMap;

    // Single fused fix related variables
    ILocationAPI* mSingleFixLocationApi;
    uint32_t     mSingleFixTrackingSessionId;
    LocationCallbacks mSingleFixLocationApiCallbacks;
    SingleFixReqMap   mSingleFixReqMap;
    Location          mSingleFixLastLocation;

    // LIA clients that register for xtra status update
    std::unordered_set<std::string> mClientsRegForXtraStatus;
    // Location api interface to register Gnss signal types callback
    ILocationAPI* mSignalTypesLocationApi;
    LocationCallbacks mSignalTypesLocationApiCallbacks;
};

#endif //LOCATIONAPISERVICE_H

