/* Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.

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
#ifndef SYNERGY_LOC_API_H
#define SYNERGY_LOC_API_H

#include <stdint.h>
#include <stdbool.h>
#include <LocApiBase.h>
#include <loc_sll_interface.h>
#include <vector>
#include <functional>


#define LOC_CLIENT_INVALID_HANDLE_VALUE (NULL)

using Resender = std::function<void()>;

using namespace std;
using namespace loc_core;


/* This class derives from the LocApiBase class.
   The members of this class are responsible for converting
   the Synergy Location API data structures into Loc Adapter data structures.
   This class also implements some of the virtual functions that
   handle the requests from loc engine. */
class SynergyLocApi : public LocApiBase {
protected:


private:
    bool mInSession;
    GnssPowerMode mPowerMode;
    bool mEngineOn;
    bool mMeasurementsStarted;
    std::vector<Resender> mResenders;
    bool mIsMasterRegistered;
    bool mMasterRegisterNotSupported;
    GnssSvMeasurementSet*  mSvMeasurementSet;
    LOC_API_ADAPTER_EVENT_MASK_T mSlMask;

    const SllInterfaceReq *sllReqIf;

    void registerEventMask(LOC_API_ADAPTER_EVENT_MASK_T adapterMask);


protected:
    virtual enum loc_api_adapter_err
        open(LOC_API_ADAPTER_EVENT_MASK_T mask);
    virtual enum loc_api_adapter_err
        close();

    SynergyLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask,
            ContextBase *context = NULL);
    virtual ~SynergyLocApi();

public:
    static LocApiBase* createSynergyLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                                     ContextBase* context);

    virtual void setPositionMode(const LocPosMode& mode);

    virtual void
        setTime(LocGpsUtcTime time, int64_t timeReference, int uncertainty);

    virtual int getNumSvUsed (uint64_t svUsedIdsMask, int totalSvCntInOneConstellation);

    virtual void
        injectPosition(const Location& location, bool onDemandCpi);

    virtual void
        injectPosition(const GnssLocationInfoNotification &locationInfo, bool onDemandCpi);

    virtual void
        deleteAidingData(const GnssAidingData& data, LocApiResponse *adapterResponse);

    virtual void
        informNiResponse(GnssNiResponse userResponse, const void* passThroughData);

    virtual LocationError
        setServerSync(const char* url, int len, LocServerType type);

    virtual LocationError
        setServerSync(unsigned int ip, int port, LocServerType type);

    virtual void
        atlOpenStatus(int handle, int is_succ, char* apn, uint32_t apnLen, AGpsBearerType bear,
                   LocAGpsType agpsType, LocApnTypeMask mask);
    virtual void atlCloseStatus(int handle, int is_succ);
    virtual LocationError setSUPLVersionSync(GnssConfigSuplVersion version);

    virtual enum loc_api_adapter_err setNMEATypesSync(uint32_t typesMask);

    virtual LocationError setLPPConfigSync(GnssConfigLppProfileMask profileMask);


    virtual enum loc_api_adapter_err
        setSensorPropertiesSync(bool gyroBiasVarianceRandomWalk_valid,
                            float gyroBiasVarianceRandomWalk,
                            bool accelBiasVarianceRandomWalk_valid,
                            float accelBiasVarianceRandomWalk,
                            bool angleBiasVarianceRandomWalk_valid,
                            float angleBiasVarianceRandomWalk,
                            bool rateBiasVarianceRandomWalk_valid,
                            float rateBiasVarianceRandomWalk,
                            bool velocityBiasVarianceRandomWalk_valid,
                            float velocityBiasVarianceRandomWalk);

    virtual enum loc_api_adapter_err
        setSensorPerfControlConfigSync(int controlMode, int accelSamplesPerBatch,
            int accelBatchesPerSec, int gyroSamplesPerBatch, int gyroBatchesPerSec,
            int accelSamplesPerBatchHigh, int accelBatchesPerSecHigh,
            int gyroSamplesPerBatchHigh, int gyroBatchesPerSecHigh, int algorithmConfig);
    virtual LocationError
        setAGLONASSProtocolSync(GnssConfigAGlonassPositionProtocolMask aGlonassProtocol);
    virtual LocationError setLPPeProtocolCpSync(GnssConfigLppeControlPlaneMask lppeCP);
    virtual LocationError setLPPeProtocolUpSync(GnssConfigLppeUserPlaneMask lppeUP);
    virtual void getWwanZppFix();
    virtual void getBestAvailableZppFix();
    virtual LocationError setGpsLockSync(GnssConfigGpsLock lock);
    virtual void setConstrainedTuncMode(bool enabled,
                                        float tuncConstraint,
                                        uint32_t energyBudget,
                                        LocApiResponse* adapterResponse=nullptr);
    virtual void setPositionAssistedClockEstimatorMode(bool enabled,
                                                       LocApiResponse* adapterResponse=nullptr);
    virtual void getGnssEnergyConsumed();
    virtual void requestForAidingData(GnssAidingDataSvMask svDataMask);

    virtual LocationError setXtraVersionCheckSync(uint32_t check);

    virtual void startTimeBasedTracking(const TrackingOptions& options,
             LocApiResponse* adapterResponse);
    virtual void stopTimeBasedTracking(LocApiResponse* adapterResponse);
    virtual void startDistanceBasedTracking(uint32_t sessionId, const LocationOptions& options,
             LocApiResponse* adapterResponse);
    virtual void stopDistanceBasedTracking(uint32_t sessionId,
             LocApiResponse* adapterResponse = nullptr);

    virtual GnssConfigSuplVersion convertSuplVersion(const uint32_t suplVersion);
    virtual GnssConfigLppeControlPlaneMask convertLppeCp(const uint32_t lppeControlPlaneMask);
    virtual GnssConfigLppeUserPlaneMask convertLppeUp(const uint32_t lppeUserPlaneMask);

    /* Requests for SV/Constellation Control */
    virtual LocationError setBlacklistSvSync(const GnssSvIdConfig& config);
    virtual void setBlacklistSv(const GnssSvIdConfig& config,
                                LocApiResponse* adapterResponse=nullptr);
    virtual void getBlacklistSv();
    virtual void setConstellationControl(const GnssSvTypeConfig& config,
                                         LocApiResponse *adapterResponse=nullptr);
    virtual void getConstellationControl();
    virtual void resetConstellationControl(LocApiResponse *adapterResponse=nullptr);
};

extern "C" LocApiBase* getLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                                 ContextBase *context);

#endif //SYNERGY_LOC_API_H
