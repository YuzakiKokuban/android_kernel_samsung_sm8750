/* Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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

#ifndef LOCATION_INTEGRATION_API_IMPL_H
#define LOCATION_INTEGRATION_API_IMPL_H

#include <mutex>

#include <loc_pla.h>
#include <LocIpc.h>
#include <ILocationAPI.h>
#include <LocationIntegrationApi.h>
#include <MsgTask.h>
#include <LocationApiMsg.h>
#include <LocationApiPbMsgConv.h>
#include <queue>
#include <unordered_map>

using namespace std;
using namespace loc_util;
using namespace location_integration;

namespace location_integration
{
typedef std::unordered_map<LocConfigTypeEnum, int32_t> LocConfigReqCntMap;
typedef std::unordered_map<PositioningEngineMask, LocEngineRunState> LocConfigEngRunStateMap;
typedef std::unordered_map<PositioningEngineMask, uint32_t> LocConfigEngIntegrityRiskMap;

struct TuncConfigInfo{
    bool     isValid;
    bool     enable;
    float    tuncThresholdMs; // need to be specified if enable is true
    uint32_t energyBudget;    // need to be specified if enable is true
};

struct PaceConfigInfo{
    bool isValid;
    bool enable;
};

struct SvConfigInfo{
    bool             isValid;
    GnssSvTypeConfig constellationEnablementConfig;
    GnssSvIdConfig   blacklistSvConfig;
    GnssSvTypeConfig secondaryBandConfig;
};

struct RobustLocationConfigInfo{
    bool isValid;
    bool enable;
    bool enableForE911;
};

struct DeadReckoningEngineConfigInfo{
    bool isValid;
    ::DeadReckoningEngineConfig dreConfig;
};

struct GtpUserConsentConfigInfo{
    bool isValid;
    bool userConsent;
};

struct NmeaConfigInfo{
    bool isValid;
    GnssNmeaTypesMask enabledNmeaTypes;
    GnssGeodeticDatumType nmeaDatumType;
    LocReqEngineTypeMask locReqEngMask;
};

struct ProtoMsgInfo{
    LocConfigTypeEnum configType;
    string protoStr;

    inline ProtoMsgInfo() :
            configType((LocConfigTypeEnum) 0), protoStr("") {}
    inline ProtoMsgInfo(LocConfigTypeEnum inType, string inStr) :
            configType(inType), protoStr(std::move(inStr)) {}
};

class IpcListener;

class LocationIntegrationApiImpl : public ILocationControlAPI {
    friend IpcListener;
public:
    LocationIntegrationApiImpl(LocIntegrationCbs& integrationCbs);

    virtual void destroy() override;

    // convenient methods
    inline bool sendMessage(const uint8_t* data, uint32_t length) const {
        return (mIpcSender != nullptr) && LocIpc::send(*mIpcSender, data, length);
    }

    // reset to defaut will apply to enable/disable SV constellation and
    // blacklist/unblacklist SVs
    virtual uint32_t configConstellations(
            const GnssSvTypeConfig& constellationEnablementConfig,
            const GnssSvIdConfig&   blacklistSvConfig) override;
    virtual uint32_t configConstellationSecondaryBand(
                                          const GnssSvTypeConfig& secondaryBandConfig) override;
    virtual uint32_t configConstrainedTimeUncertainty(
            bool enable, float tuncThreshold, uint32_t energyBudget) override;
    virtual uint32_t configPositionAssistedClockEstimator(bool enable) override;
    virtual uint32_t configLeverArm(const LeverArmConfigInfo& configInfo) override;
    virtual uint32_t configRobustLocation(bool enable, bool enableForE911) override;
    virtual uint32_t configDeadReckoningEngineParams(
            const ::DeadReckoningEngineConfig& dreConfig) override;
    virtual uint32_t* gnssUpdateConfig(const GnssConfig& config) override;
    virtual uint32_t gnssDeleteAidingData(const GnssAidingData& data) override;
    virtual uint32_t configMinGpsWeek(uint16_t minGpsWeek) override;
    virtual void odcpiInject(const ::Location& location) override;
    virtual uint32_t gnssInjectMmfData(const GnssMapMatchedData& mmfData) override;

    uint32_t getRobustLocationConfig();
    uint32_t getMinGpsWeek();

    uint32_t configMinSvElevation(uint8_t minSvElevation);
    uint32_t getMinSvElevation();

    uint32_t getConstellationSecondaryBandConfig();

    uint32_t configEngineRunState(PositioningEngineMask engType, LocEngineRunState engState);
    uint32_t configEngineIntegrityRisk(PositioningEngineMask engType, uint32_t integrityRisk);

    uint32_t setUserConsentForTerrestrialPositioning(bool userConsent);

    uint32_t configOutputNmeaTypes(GnssNmeaTypesMask enabledNmeaTypes,
                                   GnssGeodeticDatumType nmeaDatumType,
                                   LocReqEngineTypeMask locReqEngMask) override;

    uint32_t configXtraParams(bool enable, const ::XtraConfigParams& configParams);
    uint32_t getXtraStatus();
    uint32_t registerXtraStatusUpdate(bool registerUpdate);
    uint32_t configMerkleTree(const char * merkleTreeXml, int xmlSize);
    uint32_t configOsnmaEnablement(bool isEnabled);
    uint32_t registerGnssSignalTypesUpdate(bool registerUpdate);

private:
    ~LocationIntegrationApiImpl();
    bool integrationClientAllowed();
    bool sendConfigMsgToHalDaemon(LocConfigTypeEnum configType, const string& pbStr,
                                  bool invokeResponseCb = true);
    bool sendClientRegMsgToHalDaemon();
    void processHalReadyMsg();

    void addConfigReq(LocConfigTypeEnum configType);
    bool processQueuedReqs(); // return true indicates queue is note empty
    void flushConfigReqs();
    void processConfigRespCb(const LocAPIGenericRespMsg* pRespMsg);
    void processGetRobustLocationConfigRespCb(
            const LocConfigGetRobustLocationConfigRespMsg* pRespMsg);
    void processGetMinGpsWeekRespCb(const LocConfigGetMinGpsWeekRespMsg* pRespMsg);
    void processGetMinSvElevationRespCb(const LocConfigGetMinSvElevationRespMsg* pRespMsg);
    void processGetConstellationSecondaryBandConfigRespCb(
            const LocConfigGetConstellationSecondaryBandConfigRespMsg* pRespMsg);
    void processGetXtraStatusRespCb(
            const LocConfigGetXtraStatusRespMsg* pRespMsg);
    void processRegisterGnssSignalTypesRespCb(
            const LocConfigRegisterGnssSignalTypesUpdateRespMsg* msg);

    // protobuf conversion util class
    LocationApiPbMsgConv mPbufMsgConv;

    // internal session parameter
    static mutex             mMutex;
    static bool              mClientRunning; // allow singleton int client
    bool                     mHalRegistered;
    // For client on different processor, socket name will start with
    // defined constant of SOCKET_TO_EXTERANL_AP_LOCATION_CLIENT_BASE.
    // For client on same processor, socket name will start with
    // SOCKET_LOC_CLIENT_DIR + LOC_INTAPI_NAME_PREFIX.
    char                     mSocketName[MAX_SOCKET_PATHNAME_LENGTH];

    // cached configuration to be used when hal daemon crashes
    // and restarts
    TuncConfigInfo           mTuncConfigInfo;
    PaceConfigInfo           mPaceConfigInfo;
    SvConfigInfo             mSvConfigInfo;
    LeverArmConfigInfo       mLeverArmConfigInfo;
    RobustLocationConfigInfo mRobustLocationConfigInfo;
    DeadReckoningEngineConfigInfo mDreConfigInfo;
    LocConfigEngRunStateMap       mEngRunStateConfigMap;
    LocConfigEngIntegrityRiskMap  mEngIntegrityRiskConfigMap;
    GtpUserConsentConfigInfo      mGtpUserConsentConfigInfo;
    NmeaConfigInfo                mNmeaConfigInfo;
    bool                          mRegisterXtraUpdate;
    bool                          mXtraUpdateUponRegisterPending;
    LocConfigReqCntMap       mConfigReqCntMap;
    LocIntegrationCbs        mIntegrationCbs;
    std::queue<ProtoMsgInfo> mQueuedMsg; // queue of the requests before
                                         // communication with hal daemom
                                         // is established

    LocIpc                   mIpc;
    shared_ptr<LocIpcSender> mIpcSender;

    MsgTask                  mMsgTask;
};

} // namespace location_client

#endif /* LOCATION_INTEGRATION_API_IMPL_H */
