/* Copyright (c) 2020 The Linux Foundation. All rights reserved.
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

#ifndef LOCATION_API_PBMSGCONV_H
#define LOCATION_API_PBMSGCONV_H

#include <LocationApiMsg.h>

using namespace std;
using namespace loc_util;

#define LocApiPb_LOGd(fmt, ...) if (mPbDebugLogEnabled) { LOC_LOGd(fmt, ##__VA_ARGS__); }
#define LocApiPb_LOGv(fmt, ...) if (mPbVerboseLogEnabled) { LOC_LOGv(fmt, ##__VA_ARGS__); }

// LocationApiPbMsgConv - Class which handles conversion of rigid struct to protobuf
// messages and vice versa. This will be to serialize/deserialize message using protobuf to
// send across process or processor boundaries between different modules.
class LocationApiPbMsgConv {
public:
    LocationApiPbMsgConv();
    virtual ~LocationApiPbMsgConv() {}

    // STRUCTURE CONVERSION
    // ********************
    // **** helper function for structure conversion to protobuf format
    // GnssSvTypeConfig to PBGnssSvTypeConfig
    int convertGnssSvTypeConfigToPB(const GnssSvTypeConfig &gnssSvTypeCfg,
            PBGnssSvTypeConfig *pbGnssSvTypeCfg) const;
    // GnssSvIdConfig to PBGnssSvIdConfig
    int convertGnssSvIdConfigToPB(const GnssSvIdConfig &gnssSvIdCfg,
            PBGnssSvIdConfig *pbgnssSvIdCfg) const;
    // GnssAidingData to PBAidingData
    int convertGnssAidingDataToPB(const GnssAidingData &gnssAidData,
            PBAidingData *pbGnssAidData) const;
    // LeverArmConfigInfo to PBLIALeverArmConfigInfo
    int convertLeverArmConfigInfoToPB(const LeverArmConfigInfo &leverArmCfgInfo,
            PBLIALeverArmConfigInfo *pbLeverArmCfgInfo) const;
    // DeadReckoningEngineConfig to PBDeadReckoningEngineConfig
    int convertDeadReckoningEngineConfigToPB(const DeadReckoningEngineConfig &drEngConfig,
            PBDeadReckoningEngineConfig *pbDrEngConfig) const;
    // XtraConfigParams to PBXtraConfigParams
    int convertXtraConfigParamsToPB(const XtraConfigParams& xtraParams,
            PBXtraConfigParams* pbXtraParams) const;
    int convertXtraStatusToPB(const XtraStatus& xtraStatus, PBXtraStatus* pbXtraStatus) const;

    // LocationOptions to PBLocationOptions
    int convertLocationOptionsToPB(const LocationOptions &locOpt,
            PBLocationOptions *pbLocOpt) const;
    // GeofencesAddedReqPayload to PBGeofencesAddedReqPayload
    int convertGfAddedReqPayloadToPB(const GeofencesAddedReqPayload &gfAddReqPayload,
            PBGeofencesAddedReqPayload *pbGfAddReqPayload) const;
    // GeofencesReqClientIdPayload to PBGeofencesReqClientIdPayload
    int convertGfReqClientIdPayloadToPB(const GeofencesReqClientIdPayload &gfReqClntIdPayload,
            PBGeofencesReqClientIdPayload *pbgfReqClntIdPayload) const;
    // CollectiveResPayload to PBCollectiveResPayload
    int convertCollectiveResPayloadToPB(const CollectiveResPayload &collctResPload,
            PBCollectiveResPayload *pbCollctResPload) const;
    // LocAPIBatchNotification to PBLocAPIBatchNotification
    int convertLocAPIBatchingNotifMsgToPB(const LocAPIBatchNotification &locApiBatchNotifMsg,
            PBLocAPIBatchNotification *pbLocApiBatchNotifMsg) const;
    // LocAPIGeofenceBreachNotification to PBLocAPIGeofenceBreachNotification
    int convertLocAPIGfBreachNotifToPB(const LocAPIGeofenceBreachNotification &locApiGfBreachNotif,
            PBLocAPIGeofenceBreachNotification *pbLocApiGfBreachNotif) const;
    // GnssConfigRobustLocation to PBGnssConfigRobustLocation
    int convertGnssConfigRobustLocationToPB(const GnssConfigRobustLocation &gnssCfgRbstLoc,
            PBGnssConfigRobustLocation *pbGnssCfgRbstLoc) const;
    // Location to PBLocation
    int convertLocationToPB(const Location &location, PBLocation *pbLocation) const;
    // GnssLocationInfoNotification to PBGnssLocationInfoNotification
    int convertGnssLocInfoNotifToPB(const GnssLocationInfoNotification &gnssLocInfoNotif,
            PBGnssLocationInfoNotification *pbgnssLocInfoNotif) const;
    // GnssSvNotification to PBLocApiGnssSvNotification
    int convertGnssSvNotifToPB(const GnssSvNotification &gnssSvNotif,
            PBLocApiGnssSvNotification *pbGnssSvNotif) const;
    // LocAPINmeaSerializedPayload to PBLocAPINmeaSerializedPayload
    int convertLocAPINmeaSerializedPayloadToPB(
            const LocAPINmeaSerializedPayload &locAPINmeaSerPload,
            PBLocAPINmeaSerializedPayload *pbLocAPINmeaSerPload) const;
    // GnssDataNotification to PBGnssDataNotification
    int convertGnssDataNotifToPB(const GnssDataNotification &gnssDataNotif,
            PBGnssDataNotification *pbGnssDataNotif) const;
    // GnssMeasurementsNotification to PBGnssMeasurementsNotification
    int convertGnssMeasNotifToPB(const GnssMeasurementsNotification &gnssMeasNotif,
            PBGnssMeasurementsNotification *pbGnssMeasNotif) const;
    // LocationSystemInfo to PBLocationSystemInfo
    int convertLocSysInfoToPB(const LocationSystemInfo &locSysInfo,
            PBLocationSystemInfo *pbLocSysInfo) const;
    // GnssDebugReport to PBGnssDebugReport
    int convertGnssDebugReportToPB(const GnssDebugReport& gnssDebugReport,
            PBGnssDebugReport* pbGnssDebugReport) const;
    // GnssDcReportInfo to PBGnssDcReportInfo
    int convertGnssDcReportToPB(const GnssDcReportInfo &dcReportInfo,
            PBGnssDcReportInfo *pbDcReportInfo) const;
    // AntennaInformation to PBAntennaInformation
    int convertAntennaInfoToPB(const AntennaInformation& antennaInfo,
            PBAntennaInformation* pbAntennaInfo) const;

    // GnssMapMatchedData to PBGnssMapMatchedData
    int convertGnssMmfDataToPB(const GnssMapMatchedData mmfData,
           PBGnssMapMatchedData* pbMmfData) const;

    // Memory cleanup - Free up memory after PB conversion and serializing data
    inline void freeUpPBLocAPIStartTrackingReqMsg(PBLocAPIStartTrackingReqMsg &pbLocApiStartTrack)
            const {
        // PBLocationOptions locOptions = 1;
        pbLocApiStartTrack.clear_locoptions();
    }

    inline void freeUpPBLocAPIUpdateTrackingOptionsReqMsg(
            PBLocAPIUpdateTrackingOptionsReqMsg &pbLocApiUpdtTrackOpt) const {
        // PBLocationOptions locOptions = 1;
        pbLocApiUpdtTrackOpt.clear_locoptions();
    }

    inline void freeUpPBLocAPICollectiveRespMsg(PBLocAPICollectiveRespMsg &pbLocApiCollctvRspMsg)
            const {
        // PBCollectiveResPayload - repeated PBGeofenceResponse resp = 1;
        PBCollectiveResPayload cllctResPayld = pbLocApiCollctvRspMsg.collectiveres();
        cllctResPayld.clear_resp();

        // PBCollectiveResPayload collectiveRes = 1;
        pbLocApiCollctvRspMsg.clear_collectiveres();
    }

    void freeUpPBLocAPIAddGeofencesReqMsg(PBLocAPIAddGeofencesReqMsg &pbLocApiAddGfReqMsg)
            const {
        // PBGeofencePayload - PBGeofenceOption gfOption = 2;
        // PBGeofencePayload - PBGeofenceInfo gfInfo = 3;
        // PBGeofencesAddedReqPayload - repeated PBGeofencePayload gfPayload = 1;
        PBGeofencesAddedReqPayload gfAddReqPload = pbLocApiAddGfReqMsg.geofences();
        uint32_t gfCount = gfAddReqPload.gfpayload_size();
        for (uint32_t i=0; i < gfCount; i++) {
            PBGeofencePayload pbGfPayload = gfAddReqPload.gfpayload(i);
            pbGfPayload.clear_gfoption();
            pbGfPayload.clear_gfinfo();
        }
        gfAddReqPload.clear_gfpayload();

        // PBGeofencesAddedReqPayload geofences = 1;
        pbLocApiAddGfReqMsg.clear_geofences();
    }

    inline void freeUpPBLocAPIRemoveGeofencesReqMsg(PBLocAPIRemoveGeofencesReqMsg &pbLocApiRemGf)
            const {
        // PBGeofencesReqClientIdPayload gfClientIds = 1;
        pbLocApiRemGf.clear_gfclientids();
    }

    void freeUpPBLocAPIModifyGeofencesReqMsg(PBLocAPIModifyGeofencesReqMsg &pbLocApiModGf)
            const {
        // PBGeofencePayload - PBGeofenceOption gfOption = 2;
        // PBGeofencePayload - PBGeofenceInfo gfInfo = 3;
        // PBGeofencesAddedReqPayload - repeated PBGeofencePayload gfPayload = 1;
        PBGeofencesAddedReqPayload gfModReqPload = pbLocApiModGf.geofences();
        uint32_t gfCount = gfModReqPload.gfpayload_size();
        for (uint32_t i=0; i < gfCount; i++) {
            PBGeofencePayload pbGfPayload = gfModReqPload.gfpayload(i);
            pbGfPayload.clear_gfoption();
            pbGfPayload.clear_gfinfo();
        }
        gfModReqPload.clear_gfpayload();

        // PBGeofencesAddedReqPayload geofences = 1;
        pbLocApiModGf.clear_geofences();
    }

    inline void freeUpPBLocAPIPauseGeofencesReqMsg(PBLocAPIPauseGeofencesReqMsg &pbLocApiPauseGf)
            const {
        // PBGeofencesReqClientIdPayload gfClientIds = 1;
        pbLocApiPauseGf.clear_gfclientids();
    }
    inline void freeUpPBLocAPIResumeGeofencesReqMsg(PBLocAPIResumeGeofencesReqMsg
            &pbLocApiResumeGf) const {
        // PBGeofencesReqClientIdPayload gfClientIds = 1;
        pbLocApiResumeGf.clear_gfclientids();
    }

    inline void freeUpPBLocAPILocationIndMsg(PBLocAPILocationIndMsg &pbLocApiLocInd) const {
        // PBLocation locationNotification = 1;
        pbLocApiLocInd.clear_locationnotification();
    }

    inline void freeUpPBLocAPIBatchingIndMsg(PBLocAPIBatchingIndMsg &pbLocApiBatchInd) const {
        // PBLocAPIBatchNotification  - repeated PBLocation location = 2;
        PBLocAPIBatchNotification batchNotif = pbLocApiBatchInd.batchnotification();
        batchNotif.clear_location();

        // PBLocAPIBatchNotification batchNotification = 1;
        pbLocApiBatchInd.clear_batchnotification();
    }
    inline void freeUpPBLocAPIGeofenceBreachIndMsg(PBLocAPIGeofenceBreachIndMsg &pbLocApiGfBreach)
            const {
        // PBLocAPIGeofenceBreachNotification - PBLocation location = 3;
        PBLocAPIGeofenceBreachNotification gfBreachNotif = pbLocApiGfBreach.gfbreachnotification();
        gfBreachNotif.clear_location();

        // PBLocAPIGeofenceBreachNotification gfBreachNotification = 1;
        pbLocApiGfBreach.clear_gfbreachnotification();
    }

    void freeUpPBLocAPILocationInfoIndMsg(PBLocAPILocationInfoIndMsg &pbLocApiLocInfoInd)
            const {
        PBGnssLocationInfoNotification gnssLocInfoNotif =
                pbLocApiLocInfoInd.gnsslocationinfonotification();
        freeUpPBGnssLocationInfoNotification(gnssLocInfoNotif);

        // PBGnssLocationInfoNotification gnssLocationInfoNotification = 1;
        pbLocApiLocInfoInd.clear_gnsslocationinfonotification();
    }

    void freeUpPBLocAPIEngineLocationsInfoIndMsg(PBLocAPIEngineLocationsInfoIndMsg
            &pbLocApiEngLocInfo) const {
        uint32_t numEngLocInfo = pbLocApiEngLocInfo.enginelocationsinfo_size();
        for (uint32_t i=0; i < numEngLocInfo; i++) {
            PBGnssLocationInfoNotification gnssLocInfoNotif =
                    pbLocApiEngLocInfo.enginelocationsinfo(i);
            freeUpPBGnssLocationInfoNotification(gnssLocInfoNotif);
        }

        // repeated PBGnssLocationInfoNotification engineLocationsInfo = 1;
        pbLocApiEngLocInfo.clear_enginelocationsinfo();
    }

    void freeUpPBGnssLocationInfoNotification(PBGnssLocationInfoNotification &gnssLocInfoNotif)
            const {
        // PBLocApiSystemTimeStructUnion -
        //          - PBLocApiGnssSystemTimeStructType or
        //          - PBLocApiGnssGloTimeStructType
        // PBLocApiGnssSystemTime - PBLocApiSystemTimeStructUnion u = 2;
        // PBGnssLocationInfoNotification - PBLocation location = 1;
        // PBGnssLocationInfoNotification - PBGnssLocationSvUsedInPosition svUsedInPosition = 24;
        // PBGnssLocationInfoNotification - PBLocApiGnssLocationPositionDynamics bodyFrameData = 26;
        // PBGnssLocationInfoNotification - PBLocApiGnssSystemTime gnssSystemTime = 27;
        // PBGnssLocationInfoNotification - repeated PBGnssMeasUsageInfo measUsageInfo = 29;
        // PBGnssLocationInfoNotification - PBLLAInfo llaVRPBased = 37;
        // PBGnssLocationInfoNotification - repeated float enuVelocityVRPBased = 38;
        PBLocApiGnssSystemTime gnssSysTime = gnssLocInfoNotif.gnsssystemtime();
        PBLocApiSystemTimeStructUnion sysTimeStrctUnion = gnssSysTime.u();
        switch (gnssSysTime.gnsssystemtimesrc()) {
            case PB_GNSS_LOC_SV_SYSTEM_GPS:
                // PBLocApiGnssSystemTimeStructType gpsSystemTime = 1;
                sysTimeStrctUnion.clear_gpssystemtime();
                break;
            case PB_GNSS_LOC_SV_SYSTEM_GALILEO:
                // PBLocApiGnssSystemTimeStructType galSystemTime = 2;
                sysTimeStrctUnion.clear_galsystemtime();
                break;
            case PB_GNSS_LOC_SV_SYSTEM_BDS:
                // PBLocApiGnssSystemTimeStructType bdsSystemTime = 3;
                sysTimeStrctUnion.clear_bdssystemtime();
                break;
            case PB_GNSS_LOC_SV_SYSTEM_QZSS:
                // PBLocApiGnssSystemTimeStructType qzssSystemTime = 4;
                sysTimeStrctUnion.clear_qzsssystemtime();
                break;
            case PB_GNSS_LOC_SV_SYSTEM_GLONASS:
                // PBLocApiGnssGloTimeStructType gloSystemTime = 5;
                sysTimeStrctUnion.clear_glosystemtime();
                break;
            case PB_GNSS_LOC_SV_SYSTEM_NAVIC:
                // PBLocApiGnssSystemTimeStructType navicSystemTime = 6;
                sysTimeStrctUnion.clear_navicsystemtime();
                break;
            default:
                break;
        }
        gnssSysTime.clear_u();
        gnssLocInfoNotif.clear_location();
        gnssLocInfoNotif.clear_svusedinposition();
        gnssLocInfoNotif.clear_bodyframedata();
        gnssLocInfoNotif.clear_gnsssystemtime();
        gnssLocInfoNotif.clear_measusageinfo();
        gnssLocInfoNotif.clear_llavrpbased();
        gnssLocInfoNotif.clear_enuvelocityvrpbased();
        return;
    }

    inline void freeUpPBLocAPISatelliteVehicleIndMsg(PBLocAPISatelliteVehicleIndMsg
            &pbLocApiSatVehInd) const {
        // PBLocApiGnssSvNotification - repeated PBLocApiGnssSv gnssSvs = 2;
        PBLocApiGnssSvNotification pbGnssSvNotif = pbLocApiSatVehInd.gnsssvnotification();
        pbGnssSvNotif.clear_gnsssvs();

        // PBLocApiGnssSvNotification gnssSvNotification = 1;
        pbLocApiSatVehInd.clear_gnsssvnotification();
    }

    inline void freeUpPBLocAPINmeaIndMsg(PBLocAPINmeaIndMsg &pbLocApiNmeaInd) const {
        // PBLocAPINmeaSerializedPayload - string nmea = 2;
        PBLocAPINmeaSerializedPayload nmeaSerPayLoad = pbLocApiNmeaInd.gnssnmeanotification();
        nmeaSerPayLoad.clear_nmea();

        // PBLocAPINmeaSerializedPayload gnssNmeaNotification = 1;
        pbLocApiNmeaInd.clear_gnssnmeanotification();
    }

    inline void freeUpPBLocAPIDataIndMsg(PBLocAPIDataIndMsg &pbLocApiDataInd) const {
        // PBGnssDataNotification - repeated uint64  gnssDataMask = 2;
        // PBGnssDataNotification - repeated double     jammerInd = 3;
        // PBGnssDataNotification - repeated double     agc = 4;
        PBGnssDataNotification gnssDataNotif = pbLocApiDataInd.gnssdatanotification();
        gnssDataNotif.clear_gnssdatamask();
        gnssDataNotif.clear_jammerind();
        gnssDataNotif.clear_agc();

        // PBGnssDataNotification gnssDataNotification = 1;
        pbLocApiDataInd.clear_gnssdatanotification();
    }

    inline void freeUpPBLocAPIMeasIndMsg(PBLocAPIMeasIndMsg &pbLocApiMeasInd) const {
        // PBGnssMeasurementsNotification - repeated PBGnssMeasurementsData measurements = 1;
        // PBGnssMeasurementsNotification - PBGnssMeasurementsClock clock = 2;
        PBGnssMeasurementsNotification gnssMeasNotif =
                pbLocApiMeasInd.gnssmeasurementsnotification();
        gnssMeasNotif.clear_measurements();
        gnssMeasNotif.clear_clock();

        // PBGnssMeasurementsNotification gnssMeasurementsNotification = 1;
        pbLocApiMeasInd.clear_gnssmeasurementsnotification();
    }

    inline void freeUpPBLocInjectMmfDataReqMsg(PBLocInjectMmfDataReqMsg &pbLogMmfDataInd) const {
        pbLogMmfDataInd.clear_gnssmmfdata();
    }
    inline void freeUpPBLocAPILocationSystemInfoIndMsg(PBLocAPILocationSystemInfoIndMsg
            &pbLocApiLocSysInfoInd) const {
        // PBLeapSecondChangeInfo - PBLocApiGnssSystemTimeStructType gpsTimestampLsChange = 1;
        // PBLeapSecondSystemInfo- PBLeapSecondChangeInfo  leapSecondChangeInfo = 3;
        // PBLocationSystemInfo - PBLeapSecondSystemInfo   leapSecondSysInfo = 2;
        PBLocationSystemInfo pbLocSysInfo = pbLocApiLocSysInfoInd.locationsysteminfo();
        PBLeapSecondSystemInfo pbLeapSecSysInfo = pbLocSysInfo.leapsecondsysinfo();
        PBLeapSecondChangeInfo pbLeapSecChgInfo = pbLeapSecSysInfo.leapsecondchangeinfo();
        pbLeapSecChgInfo.clear_gpstimestamplschange();
        pbLeapSecSysInfo.clear_leapsecondchangeinfo();
        pbLocSysInfo.clear_leapsecondsysinfo();

        // PBLocationSystemInfo locationSystemInfo = 1;
        pbLocApiLocSysInfoInd.clear_locationsysteminfo();
    }

    inline void freeUpPBLocConfigSvConstellationReqMsg(PBLocConfigSvConstellationReqMsg
            &pbLocConfSvConst) const {
        // PBGnssSvTypeConfig mConstellationEnablementConfig = 1;
        pbLocConfSvConst.clear_mconstellationenablementconfig();
        // PBGnssSvIdConfig   mBlacklistSvConfig = 2;
        pbLocConfSvConst.clear_mblacklistsvconfig();
    }

    inline void freeUpPBLocConConstllSecBandReqMsg(PBLocConfigConstellationSecondaryBandReqMsg
            &pbLocCfgConstlSecBandReqMsg) const {
        // PBGnssSvTypeConfig mSecondaryBandConfig = 1;
        pbLocCfgConstlSecBandReqMsg.clear_msecondarybandconfig();
    }

    inline void freeUpPBLocConAidingDataDeletionReqMsg(PBLocConfigAidingDataDeletionReqMsg
            &pbLocConfAidDataDel) const {
        // PBAidingData mAidingData = 1;
        pbLocConfAidDataDel.clear_maidingdata();
    }

    inline void freeUpPBLocConfigLeverArmReqMsg(PBLocConfigLeverArmReqMsg &pbLocConfLeverArm)
            const {
        // PBLIALeverArmConfigInfo - PBLIALeverArmParams   gnssToVRP = 2;
        // PBLIALeverArmConfigInfo - PBLIALeverArmParams   drImuToGnss = 3;
        // PBLIALeverArmConfigInfo - PBLIALeverArmParams   veppImuToGnss = 4;
        PBLIALeverArmConfigInfo pbLeverArmCfInfo = pbLocConfLeverArm.mleverarmconfiginfo();
        pbLeverArmCfInfo.clear_gnsstovrp();
        pbLeverArmCfInfo.clear_drimutognss();
        pbLeverArmCfInfo.clear_veppimutognss();

        // PBLIALeverArmConfigInfo mLeverArmConfigInfo = 1;
        pbLocConfLeverArm.clear_mleverarmconfiginfo();
    }

    inline void freeUpPBLocConfDrEngineParamsReqMsg(PBLocConfigDrEngineParamsReqMsg
            &pbLocCfgDrEngParamReq) const {
        // PBDeadReckoningEngineConfig - PBLIABodyToSensorMountParams bodyToSensorMountParams = 2;
        PBDeadReckoningEngineConfig pbDrEngineCfg = pbLocCfgDrEngParamReq.mdreconfig();
        pbDrEngineCfg.clear_bodytosensormountparams();

        // PBDeadReckoningEngineConfig mDreConfig = 1;
        pbLocCfgDrEngParamReq.clear_mdreconfig();
    }

    inline void freeUpPBLocConGetRbstLocCfgRespMsg(PBLocConfigGetRobustLocationConfigRespMsg
            &pbLocConfGetRobustLocConfg) const {
        // PBGnssConfigRobustLocation - PBGnssConfigRobustLocationVersion version = 4;
        PBGnssConfigRobustLocation pbGnssCfgRbstLoc =
                pbLocConfGetRobustLocConfg.mrobustloationconfig();
        pbGnssCfgRbstLoc.clear_version();

        // PBGnssConfigRobustLocation mRobustLoationConfig = 1;
        pbLocConfGetRobustLocConfg.clear_mrobustloationconfig();
    }

    inline void freeUpPBLocConfGetConstllSecBandCfgRespMsg(
            PBLocConfigGetConstltnSecondaryBandConfigRespMsg &pbLocCfgGetConstlSecBandRespMsg)
            const {
        // PBGnssSvTypeConfig mSecondaryBandConfig = 1;
        pbLocCfgGetConstlSecBandRespMsg.clear_msecondarybandconfig();
    }

    inline void freeupPBLocAPIGetDebugRespMsg(
            PBLocAPIGetDebugRespMsg& pbLocAPIGetDebugRespMsg) const {
        // PBGnssDebugReport mDebugReport = 1;
        pbLocAPIGetDebugRespMsg.clear_mdebugreport();
    }

    inline void freeupPBAntennaInfoMsg(
            PBLocAPIAntennaInfoMsg& pbLocAPIAntennaInfoMsg) const {
        // PBAntennaInformation mAntennaInformation = 1;
        PBAntennaInformation antennaInformation = pbLocAPIAntennaInfoMsg.mantennainformation();
        antennaInformation.clear_antennainfos();
        pbLocAPIAntennaInfoMsg.clear_mantennainformation();
    }

    inline void freeUpPBLocConfigXtraReqMsg(PBLocConfigXtraReqMsg &pbLocConfMsg) const {
        pbLocConfMsg.clear_xtraparams();
    }

    inline void freeUpPBLocConfigGetXtraStatusRespMsg(
            PBLocConfigGetXtraStatusRespMsg &pbLocMsg) const {
        pbLocMsg.clear_mxtrastatus();
    }

    inline void freeUpPBLocAPIPingTestReqMsg(PBLocAPIPingTestReqMsg &pbLocApiPingTest) const {
        // repeated uint32 data = 2;
        pbLocApiPingTest.clear_data();
    }

    inline void freeUpPBLocAPIPingTestIndMsg(PBLocAPIPingTestIndMsg &pbLocApiPingTestIndMsg)
            const {
        // repeated uint32 data = 1;
        pbLocApiPingTestIndMsg.clear_data();
    }

    inline void freeUpPBLocAPIEphIndMsg(PBLocAPIEphIndMsg &pbLocApiEphInd) const {

        PBGnssEphemerisNotification gnssEphNotif =
                pbLocApiEphInd.gnssephemerisnotification();
        gnssEphNotif.clear_systemtime();

        PBEphInfoUnion ephUnion = gnssEphNotif.ephunion();
        switch (gnssEphNotif.gnssconstellation()) {
            case PB_GNSS_LOC_SV_SYSTEM_GPS:
            {
                PBGpsEphemerisResponse gpsResp =  ephUnion.gpsephemeris();
                for (uint32_t i = 0; i < gpsResp.gpsephemerisdata_size(); i++) {
                    PBGpsEphemeris gpsEph = gpsResp.gpsephemerisdata(i);
                    gpsEph.clear_commonephemerisdata();
                }
                gpsResp.clear_gpsephemerisdata();
                ephUnion.clear_gpsephemeris();
                break;
            }
            case PB_GNSS_LOC_SV_SYSTEM_GALILEO:
            {
                PBGalileoEphemerisResponse galResp =  ephUnion.galileoephemeris();
                for (uint32_t i = 0; i < galResp.galephemerisdata_size(); i++) {
                    PBGalileoEphemeris galEph = galResp.galephemerisdata(i);
                    galEph.clear_commonephemerisdata();
                }
                galResp.clear_galephemerisdata();
                ephUnion.clear_galileoephemeris();
                break;
            }
            case PB_GNSS_LOC_SV_SYSTEM_GLONASS:
            {
                PBGlonassEphemerisResponse gloResp =  ephUnion.glonassephemeris();
                gloResp.clear_gloephemerisdata();
                ephUnion.clear_glonassephemeris();
                break;
            }
            case PB_GNSS_LOC_SV_SYSTEM_BDS:
            {
                PBBdsEphemerisResponse bdsResp =  ephUnion.bdsephemeris();
                for (uint32_t i = 0; i < bdsResp.bdsephemerisdata_size(); i++) {
                    PBBdsEphemeris bdsEph = bdsResp.bdsephemerisdata(i);
                    bdsEph.clear_commonephemerisdata();
                }
                bdsResp.clear_bdsephemerisdata();
                ephUnion.clear_bdsephemeris();
                break;
            }
            case PB_GNSS_LOC_SV_SYSTEM_QZSS:
            {
                PBQzssEphemerisResponse qzssResp =  ephUnion.qzssephemeris();
                for (uint32_t i = 0; i < qzssResp.qzssephemerisdata_size(); i++) {
                    PBGpsEphemeris qzssEph = qzssResp.qzssephemerisdata(i);
                    qzssEph.clear_commonephemerisdata();
                }
                qzssResp.clear_qzssephemerisdata();
                ephUnion.clear_qzssephemeris();
                break;
            }
            case PB_GNSS_LOC_SV_SYSTEM_NAVIC:
            {
                PBNavicEphemerisResponse navicResp =  ephUnion.navicephemeris();
                for (uint32_t i = 0; i < navicResp.navicephemerisdata_size(); i++) {
                    PBNavicEphemeris navicEph = navicResp.navicephemerisdata(i);
                    navicEph.clear_commonephemerisdata();
                }
                navicResp.clear_navicephemerisdata();
                ephUnion.clear_navicephemeris();
                break;
            }
        }
        gnssEphNotif.clear_ephunion();
        // PBGnssMeasurementsNotification gnssMeasurementsNotification = 1;
        pbLocApiEphInd.clear_gnssephemerisnotification();
    }

    // **** helper function to convert from protobuf struct to normal struct.
    // PBCollectiveResPayload to CollectiveResPayload
    int pbConvertToCollectiveResPayload(const PBCollectiveResPayload &pbClctResPayload,
            CollectiveResPayload &clctResPayload) const;
    // PBLocationOptions to LocationOptions
    int pbConvertToLocationOptions(const PBLocationOptions &pbLocOpt,
            LocationOptions &locOpt) const;
    // PBGeofencesAddedReqPayload to GeofencesAddedReqPayload
    int pbConvertToGfAddReqPayload(const PBGeofencesAddedReqPayload &pbGfAddReqPload,
            GeofencesAddedReqPayload &gfAddReqPload) const;
    // PBGeofencesReqClientIdPayload to GeofencesReqClientIdPayload
    int pbConvertToGfReqClientIdPayload(const PBGeofencesReqClientIdPayload &pbGfReqClientIdPload,
            GeofencesReqClientIdPayload &gfReqClientIdPload) const;
    // PBLocAPIBatchNotification to LocAPIBatchNotification
    int pbConvertToLocAPIBatchNotification(const PBLocAPIBatchNotification &pbLocBatchNotif,
            LocAPIBatchNotification &locBatchNotif) const;
    // PBLocAPIGeofenceBreachNotification to LocAPIGeofenceBreachNotification
    int pbConvertToLocAPIGfBreachNotification(
            const PBLocAPIGeofenceBreachNotification &pbLocApiGfBreachNotif,
            LocAPIGeofenceBreachNotification &locApiGfBreachNotif) const;
    // PBGnssLocationInfoNotification to GnssLocationInfoNotification
    int pbConvertToGnssLocInfoNotif(const PBGnssLocationInfoNotification &pbGnssLocInfoNotif,
            GnssLocationInfoNotification &gnssLocInfoNotif) const;
    // PBLocApiGnssSvNotification to GnssSvNotification
    int pbConvertToGnssSvNotif(const PBLocApiGnssSvNotification &pbGnssSvNotif,
            GnssSvNotification &gnssSvNotif) const;
    // PBLocAPINmeaSerializedPayload to LocAPINmeaSerializedPayload
    int pbConvertToLocAPINmeaSerializedPayload(
            const PBLocAPINmeaSerializedPayload &pbLocApiNmeaSerPayload,
            LocAPINmeaSerializedPayload &locApiNmeaSerPayload) const;
    // PBGnssDataNotification to GnssDataNotification
    int pbConvertToGnssDataNotification(const PBGnssDataNotification &pbGnssDataNotif,
            GnssDataNotification &gnssDataNotif) const;
    // PBGnssMeasurementsNotification to GnssMeasurementsNotification
    int pbConvertToGnssMeasNotification(const PBGnssMeasurementsNotification &pbGnssMeasNotif,
            GnssMeasurementsNotification &gnssMeasNotif) const;
    // PBLocation to Location
    int pbConvertToLocation(const PBLocation &pbLoc, Location &loc) const;
    // PBLocationSystemInfo to LocationSystemInfo
    int pbConvertToLocationSystemInfo(const PBLocationSystemInfo &pbLocSysInfo,
            LocationSystemInfo &locSysInfo) const;
    // PBGnssSvTypeConfig to GnssSvTypeConfig
    int pbConvertToGnssSvTypeConfig(const PBGnssSvTypeConfig &pbGnssSvTypeConfig,
            GnssSvTypeConfig &gnssSvTypeConfig) const;
    // PBGnssSvIdConfig to GnssSvIdConfig
    int pbConvertToGnssSvIdConfig(const PBGnssSvIdConfig &pbGnssSvIdConfig,
            GnssSvIdConfig &gnssSvIdConfig) const;
    // PBAidingData to GnssAidingData
    int pbConvertToGnssAidingData(const PBAidingData &pbGnssAidData,
            GnssAidingData &gnssAidData) const;
    // PBLIALeverArmConfigInfo to LeverArmConfigInfo
    int pbConvertToLeverArmConfigInfo(const PBLIALeverArmConfigInfo &pbLeverArmCfgInfo,
            LeverArmConfigInfo &leverArmCfgInfo) const;
    // PBDeadReckoningEngineConfig to DeadReckoningEngineConfig
    int pbConvertToDeadReckoningEngineConfig(const PBDeadReckoningEngineConfig &pbDrEngConfig,
            DeadReckoningEngineConfig &drEngConfig) const;
    // PBGnssConfigRobustLocation to GnssConfigRobustLocation
    int pbConvertToGnssConfigRobustLocation(const PBGnssConfigRobustLocation &pbGnssCfgRobLoc,
            GnssConfigRobustLocation &gnssCfgRobLoc) const;
    // PBGnssDebugReport to GnssDebugReport
    int pbConvertToGnssDebugReport(const PBGnssDebugReport &pbGnssDebugReport,
            GnssDebugReport &gnssDebugReport) const;
    int pbConvertToDcReport(const PBGnssDcReportInfo & pbDcReportInfo,
                            GnssDcReportInfo & dcReporInfo) const;
    // PBAntennaInformation to AntennaInformation
    int pbConvertToAntennaInfo(const PBAntennaInformation& pbAntennaInfo,
            AntennaInformation& antennaInfo) const;
    int pbConvertToXtraConfig(const PBXtraConfigParams &pbXtraParams,
            XtraConfigParams& xtraParams) const;
    int pbConvertToXtraStatus(const PBXtraStatus &pbXtraStatus,
            XtraStatus& xtraStatus) const;
    int pbConvertToGnssMmfData(const PBGnssMapMatchedData& pbMmfData,
            GnssMapMatchedData& mmfData) const;

    // MASK CONVERSION
    // ***************
    // **** helper function for mask conversion to protobuf masks
    // LocationCallbacksMask to PBLocationCallbacksMask
    uint32_t getPBMaskForLocationCallbacksMask(const uint32_t &locCbMask) const;
    // LocationCapabilitiesMask to PBLocationCapabilitiesMask
    uint64_t getPBMaskForLocationCapabilitiesMask(const uint64_t &locCapabMask) const;

    // **** helper function for mask conversion from protobuf masks to normal rigid values
    // PBLocationCapabilitiesMask to LocationCapabilitiesMask
    uint64_t getLocationCapabilitiesMaskFromPB(const uint64_t &pbLocCapabMask) const;
    // PBLocationCallbacksMask to LocationCallbacksMask
    uint32_t getLocationCallbacksMaskFromPB(const uint32_t &pbLocCbMask) const;

    // ENUM CONVERSION
    // **** helper function for enum conversion from protobuf enums to normal format.
    ELocMsgID getEnumForPBELocMsgID(const PBELocMsgID &pbLocMsgId) const;
    ClientType getEnumForPBClientType(const PBClientType &pbClntTyp) const;
    LocationError getEnumForPBLocationError(const PBLocationError &pbLocErr) const;
    BatchingMode getEnumForPBBatchingMode(const PBBatchingMode &pbBatchMode) const;

    // PositioningEngineMask to/from PB PositioningEngineMask
    uint32_t getPBMaskForPositioningEngineMask(const uint32_t &posEngMask) const;
    uint32_t getEnumForPBPositioningEngineMask(const uint32_t &pbPosEngMask) const;

    // LocEngineRunState to/from PB LocEngineRunState
    LocEngineRunState getEnumForPBLocEngineRunState
            (const PBLocEngineRunState &pbLocEngRunState) const;
    uint32_t getPBEnumForLocEngineRunState(const LocEngineRunState& locEngineRunState) const;

    // TerrestrialTechMask to/from PB TerrestrialTechMask
    uint32_t getTerrestrialTechMaskFromPB(const uint32_t &pbTerrestrialTechMask) const;
    uint32_t getPBMaskForTerrestrialTechMask(const uint32_t& terrestrialTechMask) const;

    // NmeaTypesMask to/frmom PB NmeaTypesMask
    uint32_t getNmeaTypesMaskFromPB(const uint32_t &pbNmeaTypesMask) const;
    uint32_t getPBMaskForNmeaTypesMask(const uint32_t& nmeaTypesMask) const;

    // LocReqEngineTypeMask to PBLocReqEngineTypeMask
    uint32_t getPBMaskForLocReqEngineTypeMask(const uint32_t &locReqEngTypeMask) const;
    // PBLocReqEngineTypeMask to LocReqEngineTypeMask
    uint32_t getLocReqEngineTypeMaskFromPB(const uint32_t &pbLocReqEngTypeMask) const;

    // **** helper function for enum conversion from normal format to protobuf enums.
    PBBatchingMode getPBEnumForBatchingMode(const BatchingMode &batchMode) const;
    PBLocationError getPBEnumForLocationError(const LocationError &locErr) const;
    PBELocMsgID getPBEnumForELocMsgID(const ELocMsgID &eLocMsgId) const;
    PBClientType getPBEnumForClientType(const ClientType &clientTyp) const;
    GnssDcReportType getDcReportTypeFromPB(const PBGnssDcReportType& pbDcReportType) const;
    PBGnssDcReportType getPBEnumForDcReportType(const GnssDcReportType& dcReportType) const;

    DebugLogLevel getDebugLogLevelFromPB(const PBDebugLogLevel &pbLogLevel) const;
    PBDebugLogLevel getPBEnumForDebugLogLevel(const DebugLogLevel &logLevel) const;
    XtraStatusUpdateType getXtraStatusUpdateTypeFromPB(
            const PBXtraStatusUpdateType &pbXtraStatusUpdateType) const;
    PBXtraStatusUpdateType getPBEnumForXtraStatusUpdateType(
            const XtraStatusUpdateType &xtraStatusUpdateType) const;
    XtraDataStatus getXtraDataStatusFromPB(const PBXtraDataStatus &pbXtraDataStatus) const;
    PBXtraDataStatus getPBEnumForXtraDataStatus(const XtraDataStatus &xtraDataStatus) const;
    // GnssSignalTypeMask to PBGnssSignalTypeMask
    uint32_t getPBMaskForGnssSignalTypeMask(const uint32_t &gnssSignalTypeMask) const;
    int pbConvertToGnssEphNotif(const PBGnssEphemerisNotification &pbGnssEphNotif,
        GnssSvEphemerisReport &gnssEphNotif) const;
    int convertGnssEphNotifToPB(const GnssSvEphemerisReport &gnssEphNotif,
        PBGnssEphemerisNotification* pbGnssEphNotif) const;

private:
    bool mPbDebugLogEnabled;
    bool mPbVerboseLogEnabled;

    // RIGID TO PROTOBUF FORMAT
    // ************************
    // **** helper function for structure conversion to protobuf format
    // LeverArmParams to PBLIALeverArmParams
    int convertLeverArmParamsToPB(const LeverArmParams &leverArmParams,
            PBLIALeverArmParams *pbLeverArmParams) const;
    // LeapSecondSystemInfo to PBLeapSecondSystemInfo
    int convertLeapSecondSystemInfoToPB(const LeapSecondSystemInfo &leapSecSysInfo,
            PBLeapSecondSystemInfo *pbLeapSecSysInfo) const;
    // LeapSecondChangeInfo to PBLeapSecondChangeInfo
    int convertLeapSecChgInfoToPB(const LeapSecondChangeInfo &leapSecChngInfo,
            PBLeapSecondChangeInfo *pbLeapSecChngInfo) const;
    // GnssSystemTimeStructType to PBLocApiGnssSystemTimeStructType
    int convertGnssSystemTimeStructTypeToPB(const GnssSystemTimeStructType &gnssSysTimeStructType,
            PBLocApiGnssSystemTimeStructType *pbGnssSysTimeStructType) const;
    // GnssGloTimeStructType to PBLocApiGnssGloTimeStructType
    int convertGnssGloTimeStructTypeToPB(const GnssGloTimeStructType &gnssGloTime,
            PBLocApiGnssGloTimeStructType *pbgnssGloTime) const;
    // GnssMeasurementsData to PBGnssMeasurementsData
    int convertGnssMeasDataToPB(const GnssMeasurementsData &gnssMeasData,
            PBGnssMeasurementsData *pbGnssMeasData) const;
    // GnssMeasurementsClock to PBGnssMeasurementsClock
    int convertGnssMeasClockToPB(const GnssMeasurementsClock &gnssMeasClock,
            PBGnssMeasurementsClock *pbGnssMeasClock) const;
    // GnssSv to PBLocApiGnssSv
    int convertGnssSvToPB(const GnssSv &gnssSv, PBLocApiGnssSv *pbGnssSv) const;
    // GnssLocationSvUsedInPosition to PBGnssLocationSvUsedInPosition
    int convertGnssLocSvUsedInPosToPB(const GnssLocationSvUsedInPosition &gnssLocSvUsedInPos,
            PBGnssLocationSvUsedInPosition *pbGnssLocSvUsedInPos) const;
    // GnssSystemTime to PBLocApiGnssSystemTime
    int convertGnssSystemTimeToPB(const GnssSystemTime &gnssSysTime,
            PBLocApiGnssSystemTime *pbGnssSysTime) const;
    // GnssLocationPositionDynamics & GnssLocationPositionDynamicsExt to
    // PBLocApiGnssLocationPositionDynamics
    int convertGnssLocationPositionDynamicsToPB(
            const GnssLocationPositionDynamics &gnssLocPosDyn,
            const GnssLocationPositionDynamicsExt &gnssLocPosDynExt,
            PBLocApiGnssLocationPositionDynamics *pbGnssLocPosDyn) const;
    // LLAInfo to PBLLAInfo
    int convertLLAInfoToPB(const LLAInfo &llaInfo, PBLLAInfo *pbLlaInfo) const;
    // GnssMeasUsageInfo to PBGnssMeasUsageInfo
    int convertGnssMeasUsageInfoToPB(const GnssMeasUsageInfo &gnssMeasUsageInfo,
            PBGnssMeasUsageInfo *pbGnssMeasUsageInfo) const;
    // SystemTimeStructUnion to PBLocApiSystemTimeStructUnion
    int convertSystemTimeStructUnionToPB(const Gnss_LocSvSystemEnumType &gnssLocSvSysEnumType,
            const SystemTimeStructUnion &sysTimeStructUnion,
            PBLocApiSystemTimeStructUnion *pbSysTimeStructUnion) const;
    // GeofenceInfo to PBGeofenceInfo
    int convertGeofenceInfoToPB(const GeofenceInfo &gfInfo, PBGeofenceInfo *pbGfInfo) const;
    // GeofenceOption to PBGeofenceOption
    int convertGeofenceOptionToPB(const GeofenceOption &gfOpt, PBGeofenceOption *pbGfOpt) const;
    // BodyToSensorMountParams to PBLIABodyToSensorMountParams
    int convertBodyToSensorMountParamsToPB(const BodyToSensorMountParams &bodyToSensorMntParams,
            PBLIABodyToSensorMountParams *pbBodyToSensorMntParams) const;
    int convertGnssDebugLocationToPB(const GnssDebugLocation& debugLocation,
            PBGnssDebugLocation* pbDebugLocation) const;
    int convertTimespecToPB(const timespec& utcReported,
            PBTimespec* pbUtcReported) const;
    int convertGnssDebugTimeToPB(const GnssDebugTime& gnssDebugTime,
            PBGnssDebugTime* pbGnssDebugTime) const;
    int convertGnssDebugSatelliteInfoToPB(
            const GnssDebugSatelliteInfo& satelliteInfo,
            PBGnssDebugSatelliteInfo* pbSatelliteInfo) const;
    int convertGnssAntennaInformationToPB(
            const GnssAntennaInformation& gnssAntennaInfo,
            PBGnssAntennaInformation* pbGnssAntennaInfo) const;
    int convertGnssCoordinateToPB(
            const GnssCoordinate& gnssCoordinate,
            PBGnssCoordinate* pbGnssCoordinate) const;
    int convert2DimensionDoubleVectorToPB(
            const std::vector<std::vector<double>>& doubleArrays,
            PB2DimensionDoubleVector* pbDoubleArrarys) const;

    // HAL to PB EPH functions
    PBGnssEphAction getPBEphAction (const GnssEphAction& ephAction) const;
    PBGalEphSignalSource convertDataSignalSourceToPB(
            const GalEphSignalSource &ephSignal) const;
    int convertCommanEphToPB (const GnssEphCommon &commanEph,
            PBGnssEphCommon *pbCommanEph) const ;
    int convertGpsEphDataToPB(const GpsEphemeris &halEphInfo,
            PBGpsEphemeris *pbEphInfo) const;
    int convertGpsEphResponseToPB(const GpsEphemerisResponse  &halResp,
            PBGpsEphemerisResponse*  pbEphResp) const;
    int convertBdsEphDataToPB(const BdsEphemeris &halEphInfo,
            PBBdsEphemeris *pbEphInfo) const;
    int convertBdsEphResponseToPB(const BdsEphemerisResponse  &halResp,
            PBBdsEphemerisResponse*  pbEphResp) const;
    int convertGalileoEphDataToPB(const GalileoEphemeris &halEphInfo,
            PBGalileoEphemeris *pbEphInfo) const;
    int convertGlonassEphDataToPB(const GlonassEphemeris &halEphInfo,
            PBGlonassEphemeris *pbEphInfo) const;
    int convertGloEphResponseToPB(const GlonassEphemerisResponse  &halResp,
    PBGlonassEphemerisResponse*  pbEphResp) const;
    int convertGalEphResponseToPB(const GalileoEphemerisResponse  &halResp,
            PBGalileoEphemerisResponse*  pbEphResp) const;
    int convertQzssEphResponseToPB(const QzssEphemerisResponse  &halResp,
            PBQzssEphemerisResponse*  pbEphResp) const;
    int convertNavicEphDataToPB(const NavicEphemeris &halEphInfo,
            PBNavicEphemeris *pbEphInfo) const;
    int convertNavicEphResponseToPB(const NavicEphemerisResponse  &halResp,
            PBNavicEphemerisResponse*  pbEphResp) const;

    // **** helper function for mask conversion to protobuf masks
    // LeverArmTypeMask to PBLIALeverArmTypeMask
    uint32_t getPBMaskForLeverArmTypeMask(const uint32_t &leverArmTypeMask) const;
    // GnssConfigRobustLocationValidMask to PBGnssConfigRobustLocationValidMask
    uint32_t getPBMaskForGnssCfgRobustLocValidMask(
            const uint32_t &gnssCfgRobustLocValidMask) const;
    // LocationSystemInfoMask to PBLocationSystemInfoMask
    uint32_t getPBMaskForLocSysInfoMask(const uint32_t &locSysInfoMask) const;
    // GnssDataMask to PBGnssDataMask
    uint64_t getPBMaskForGnssDataMask(const uint64_t &gnssDataMask) const;
    // GnssAidingDataSvMask to PBLocApiGnssAidingDataSvMask
    uint32_t getPBMaskForGnssAidingDataSvMask(const uint32_t &gnssAidDataSvMask) const;
    // LeapSecondSysInfoMask to PBLeapSecondSysInfoMask
    uint32_t getPBMaskForLeapSecondSysInfoMask(const uint32_t &leapSecSysInfoMask) const;
    // GnssSystemTimeStructTypeFlags to PBGnssSystemTimeStructTypeFlag
    uint32_t getPBMaskForGnssSystemTimeStructTypeFlags(
            const uint32_t &gnssSysTimeStructTypeFlg) const;
    // LocationFlagsMask to PBLocationFlagsMask
    uint32_t getPBMaskForLocationFlagsMask(const uint32_t &locFlagsMask) const;
    // LocationTechnologyMask to PBLocationTechnologyMask
    uint32_t getPBMaskForLocationTechnologyMask(const uint32_t &locTechMask) const;
    // GnssLocationInfoFlagMask (lower 32 bits) to PBGnssLocationInfoFlagMask
    uint32_t getPBMaskForGnssLocationInfoFlagMask(const uint64_t &gnssLocInfoFlagMask) const;
    // GnssLocationInfoFlagMask (upper 32 bits) to PBGnssLocationInfoExtFlagMask
    uint32_t getPBMaskForGnssLocationInfoExtFlagMask(const uint64_t &gnssLocInfoFlagMask) const;
    // GnssLocationNavSolutionMask to PBGnssLocationNavSolutionMask
    uint32_t getPBMaskForGnssLocationNavSolutionMask(const uint32_t &gnssLocNavSolnMask) const;
    // DrCalibrationStatusMask to PBLocApiDrCalibrationStatusMask
    uint32_t getPBMaskForDrCalibrationStatusMask(const uint32_t &drCalibStatusMask) const;
    // GnssMeasurementsDataFlagsMask to PBGnssMeasurementsDataFlagsMask
    uint32_t getPBMaskForGnssMeasurementsDataFlagsMask(
            const uint32_t &gnssMeasDataFlagsMask) const;
    // GnssMeasurementsStateMask to PBGnssMeasurementsStateMask
    uint32_t getPBMaskForGnssMeasurementsStateMask(const uint32_t &gnssMeasStateMask) const;
    // GnssMeasurementsAdrStateMask to PBGnssMeasurementsAdrStateMask
    uint32_t getPBMaskForGnssMeasurementsAdrStateMask(const uint32_t &gnssMeasAdrStateMask) const;
    // GnssLocationPosDataMask/GnssLocationPosDataMaskExt to PBLocApiGnssLocationPosDataMask
    uint32_t getPBMaskForGnssLocationPosDataMask(const uint32_t &gnssLocPosDataMask,
            const uint32_t &gnssLocPosDataMaskExt) const;
    // GnssMeasurementsClockFlagsMask to PBGnssMeasurementsClockFlagsMask
    uint32_t getPBMaskForGnssMeasurementsClockFlagsMask(
            const uint32_t &gnssMeasClockFlagsMask) const;
    // GnssGloTimeStructTypeFlags to PBGnssGloTimeStructTypeFlags
    uint32_t getPBMaskForGnssGloTimeStructTypeFlags(
            const uint32_t &gnssGloTimeStructTypeFlags) const;
    // GnssSvOptionsMask to PBLocApiGnssSvOptionsMask
    uint32_t getPBMaskForGnssSvOptionsMask(const uint32_t &gnssSvOptMask) const;
    // GeofenceBreachTypeMask to PBGeofenceBreachTypeMask
    uint32_t getPBMaskForGeofenceBreachTypeMask(const uint32_t &gfBreachTypeMask) const;
    // DeadReckoningEngineConfigValidMask to PBDeadReckoningEngineConfigValidMask
    uint64_t getPBMaskForDeadReckoningEngineConfigValidMask(
            const uint64_t &drEngCfgValidMask) const;
    // DrEngineAidingDataMask to PBDrEngineAidingDataMask
    uint32_t getPBMaskForDrEngineAidingDataMask(const uint32_t &drEngAidDataMask) const;
    // DrSolutionStatusMask to PBDrSolutionStatusMask
    uint32_t getPBMaskForDrSolutionStatusMask(const uint32_t &drSolnStatusMask) const;

    // **** helper function for enum conversion to protobuf enums
    PBGnssSuplMode getPBEnumForGnssSuplMode(const GnssSuplMode &gnssSuplMode) const;
    PBFixQualityLevel getPBEnumForFixQualityLevel(const FixQualityLevel &qualityLevel) const;
    PBBatchingStatus getPBEnumForBatchingStatus(const BatchingStatus &batchStatus) const;
    PBLocationReliability getPBEnumForLocationReliability(
            const LocationReliability &locReliab) const;
    PBLocApiOutputEngineType getPBEnumForLocOutputEngineType(
            const LocOutputEngineType &locOpEng) const;
    PBGnssMeasurementsMultipathIndicator getPBEnumForGnssMeasMultiPathIndic(
            const GnssMeasurementsMultipathIndicator &gnssMeasMultiPathIndic) const;
    PBLocApiGnss_LocSvSystemEnumType getPBEnumForGnssLocSvSystem(
            const Gnss_LocSvSystemEnumType &gnssLocSvSysEnumType) const;
    PBAgcStatus getPBEnumForAgcStatus(const AgcStatus &agcStatus) const;
    // PBLocationSessionStatus from/to loc_sess_status
    PBLocationSessionStatus getPBEnumForLocSessionStatus(const loc_sess_status &status) const;
    PBGnssEphemerisType getPBEnumForGnssEphemerisType(
            const GnssEphemerisType& ephemerisType) const;
    PBGnssEphemerisSource getPBEnumForGnssEphemerisSource(
            const GnssEphemerisSource& ephemerisSource) const;
    PBGnssEphemerisHealth getPBEnumForGnssEphemerisHealth(
            const GnssEphemerisHealth& ephemerisHealth) const;


    // ** Special enum conversion
    // GnssSvType to PBLocApiGnss_LocSvSystemEnumType
    PBLocApiGnss_LocSvSystemEnumType getPBGnssLocSvSysEnumFromGnssSvType(
            const GnssSvType &gnssSvType) const;

    // PROTOBUF TO LOCAL FORMAT
    // ************************
    // **** helper function for mask conversion from protobuf masks to normal rigid values
    // PBLIALeverArmTypeMask to LeverArmTypeMask
    uint32_t getLeverArmTypeMaskFromPB(const uint32_t &pbLeverTypeMask) const;
    // PBGnssSvTypesMask to GnssSvTypesMask
    uint64_t getGnssSvTypesMaskFromPB(const uint64_t &pbGnssSvTypesMask) const;
    // PBLocApiGnssAidingDataSvMask to GnssAidingDataSvMask
    uint32_t getGnssAidingDataSvMaskFromPB(const uint32_t &pbGnssAidDataSvMask) const;
    // PBLocationSystemInfoMask to LocationSystemInfoMask
    uint32_t getLocationSystemInfoMaskFromPB(const uint32_t &pbLocSysInfoMask) const;
    // PBGnssDataMask to GnssDataMask
    uint64_t getGnssDataMaskFromPB(const uint64_t &pbGnssDataMask) const;
    // PBLeapSecondSysInfoMask to LeapSecondSysInfoMask
    uint32_t getLeapSecSysInfoMaskFromPB(const uint32_t &pbLeapSecSysInfoMask) const;
    // PBLocApiGnssSystemTimeStructTypeFlags to GnssSystemTimeStructTypeFlags
    uint32_t getGnssSystemTimeStructTypeFlagsFromPB(const uint32_t &pbGnssSysTimeStrctType) const;
    // PBGnssSignalTypeMask to GnssSignalTypeMask
    uint32_t getGnssSignalTypeMaskFromPB(const uint32_t &pbGnssSignalTypeMask) const;
    // PBLocApiGnssSvOptionsMask to GnssSvOptionsMask
    uint32_t getGnssSvOptionsMaskFromPB(const uint32_t &pbGnssSvOptMask) const;
    // PBGeofenceBreachTypeMask to GeofenceBreachTypeMask
    uint32_t getGfBreachTypeMaskFromPB(const uint32_t &pbGfBreackTypMask) const;
    // PBLocationFlagsMask to LocationFlagsMask
    uint32_t getLocationFlagsMaskFromPB(const uint32_t &pbLocFlagsMask) const;
    // PBLocationTechnologyMask to LocationTechnologyMask
    uint32_t getLocationTechnologyMaskFromPB(const uint32_t &pbLocTechMask) const;
    // PBGnssMeasurementsClockFlagsMask to GnssMeasurementsClockFlagsMask
    uint32_t getGnssMeasurementsClockFlagsMaskFromPB(const uint32_t &pbGnssMeasClockFlgMask) const;
    // PBGnssMeasurementsDataFlagsMask to GnssMeasurementsDataFlagsMask
    uint32_t getGnssMeasurementsDataFlagsMaskFromPB(const uint32_t &pbGnssMeasDataFlgMask) const;
    // PBGnssMeasurementsAdrStateMask to GnssMeasurementsAdrStateMask
    uint32_t getGnssMeasurementsAdrStateMaskFromPB(const uint32_t &pbGnssMeasAdrStateMask) const;
    // PBGnssLocationInfoFlagMask to GnssLocationInfoFlagMask
    uint64_t getGnssLocationInfoFlagMaskFromPB(const uint32_t &pbGnssLocInfoFlagMask,
                                               const uint32_t &pbGnssLocInfoExtFlagMask) const;
    // PBGnssLocationNavSolutionMask to GnssLocationNavSolutionMask
    uint32_t getGnssLocationNavSolutionMaskFromPB(const uint32_t &pbGnssLocNavSoln) const;
    // PBLocApiDrCalibrationStatusMask to DrCalibrationStatusMask
    uint32_t getDrCalibrationStatusMaskFromPB(const uint32_t &pbDrCalibStatus) const;
    // PBLocApiGnssLocationPosDataMask to GnssLocationPosDataMask
    uint32_t getGnssLocationPosDataMaskFromPB(const uint32_t &pbGnssLocPosDataMask) const;
    // PBLocApiGnssLocationPosDataMask to GnssLocationPosDataMaskExt
    uint32_t getGnssLocationPosDataMaskExtFromPB(const uint32_t &pbGnssLocPosDataMask) const;
    // PBGnssGloTimeStructTypeFlags to GnssGloTimeStructTypeFlags
    uint32_t getGnssGloTimeStructTypeFlagsFromPB(const uint32_t &pbGnssGloTimeStruct) const;
    // PBGnssConfigRobustLocationValidMask to GnssConfigRobustLocationValidMask
    uint32_t getGnssConfigRobustLocationValidMaskFromPB(
            const uint32_t &pbGnssCfgRobstLocValidMask) const;
    // PBDeadReckoningEngineConfigValidMask to DeadReckoningEngineConfigValidMask
    uint64_t getDeadReckoningEngineConfigValidMaskFromPB(const uint64_t &pbDrEngCfgVldMask) const;
    // PBDrEngineAidingDataMask to DrEngineAidingDataMask
    uint32_t getDrEngineAidingDataMaskFromPB(const uint32_t &pbDrEngAidDataMask) const;
    // PBDrSolutionStatusMask to DrSolutionStatusMask
    uint32_t getDrSolutionStatusMaskFromPB(const uint32_t &pbDrSolnStatusMask) const;

    // **** helper function for enum conversion from protobuf enums to normal format.
    GnssSuplMode getEnumForPBGnssSuplMode(const PBGnssSuplMode &pbGnssSuplMode) const;
    FixQualityLevel getEnumForPBFixQualityLevel(
                const PBFixQualityLevel &pbFixQualityLevel) const;
    BatchingStatus getEnumForPBBatchingStatus(const PBBatchingStatus &pbBatchStat) const;
    GnssMeasurementsMultipathIndicator getEnumForPBGnssMeasMultipathIndic(
            const PBGnssMeasurementsMultipathIndicator &pbGnssMeasMultipathIndic) const;
    LocationReliability getEnumForPBLocReliability(
            const PBLocationReliability &locReliability) const;
    Gnss_LocSvSystemEnumType getEnumForPBGnssLocSvSystem(
            const PBLocApiGnss_LocSvSystemEnumType &pbGnssLocSvSysEnumType) const;
    LocOutputEngineType getEnumForPBLocOutputEngineType(
            const PBLocApiOutputEngineType &pbLocOpEngType) const;
    loc_sess_status getLocSessionStatusFromPB(const PBLocationSessionStatus &pbStatus) const;
    GnssEphemerisType getEnumForPBGnssEphemerisType(
            const PBGnssEphemerisType& pbGnssEphemerisType) const;
    GnssEphemerisSource getEnumForPBGnssEphemerisSource(
            const PBGnssEphemerisSource& pbGnssEphemerisSource) const;
    GnssEphemerisHealth getEnumForPBGnssEphemerisHealth(
            const PBGnssEphemerisHealth& pbGnssEphemerisHealth) const;
    AgcStatus getEnumForPBAgcStatus(const PBAgcStatus &pbAgcStatus) const;

    // ** Special enum conversion
    // PBLocApiGnss_LocSvSystemEnumType to GnssSvType
    GnssSvType getGnssSvTypeFromPBGnssLocSvSystemEnumType(
            const PBLocApiGnss_LocSvSystemEnumType &pbGnssLocSvSysEnum) const;

    // **** helper function to convert from protobuf struct to normal struct.
    // PBLIALeverArmParams to LeverArmParams
    int pbConvertToLeverArmParams(const PBLIALeverArmParams &pbLeverArmParams,
            LeverArmParams &leverArmParams) const;
    // PBLeapSecondSystemInfo to LeapSecondSystemInfo
    int pbConvertToLeapSecSysInfo(const PBLeapSecondSystemInfo &pbLeapSecSysInfo,
            LeapSecondSystemInfo &leapSecSysInfo) const;
    // PBLeapSecondChangeInfo to LeapSecondChangeInfo
    int pbConvertToLeapSecChgInfo(const PBLeapSecondChangeInfo &pbLeapSecChgInfo,
            LeapSecondChangeInfo &leapSecChgInfo) const;
    // PBLocApiGnssSystemTimeStructType to GnssSystemTimeStructType
    int pbConvertToGnssSystemTimeStructType(
            const PBLocApiGnssSystemTimeStructType &pbGnssSysTimeStrct,
            GnssSystemTimeStructType &gnssSysTimeStrct) const;
    // PBGnssMeasurementsData to GnssMeasurementsData
    int pbConvertToGnssMeasurementsData(const PBGnssMeasurementsData &pbGnssMeasData,
            GnssMeasurementsData &gnssMeasData) const;
    // PBGnssMeasurementsClock to GnssMeasurementsClock
    int pbConvertToGnssMeasurementsClock(const PBGnssMeasurementsClock &pbGnssMeasClock,
            GnssMeasurementsClock &gnssMeasClock) const;
    // PBGnssLocationSvUsedInPosition to GnssLocationSvUsedInPosition
    int pbConvertToGnssLocationSvUsedInPosition(
            const PBGnssLocationSvUsedInPosition &pbGnssLocSvUsedInPos,
            GnssLocationSvUsedInPosition &gnssLocSvUsedInPos) const;
    // PBLLAInfo to LLAInfo
    int pbConvertToLLAInfo(const PBLLAInfo &pbLlaInfo, LLAInfo &llaInfo) const;
    // PBLocApiGnssLocationPositionDynamics to
    // GnssLocationPositionDynamics & GnssLocationPositionDynamicsExt
    int pbConvertToGnssLocationPositionDynamics(
            const PBLocApiGnssLocationPositionDynamics &pbGnssLocPosDyn,
            GnssLocationPositionDynamics &gnssLocPosDyn,
            GnssLocationPositionDynamicsExt &gnssLocPosDynExt) const;
    // PBLocApiGnssSystemTime to GnssSystemTime
    int pbConvertToGnssSystemTime(const PBLocApiGnssSystemTime &pbGnssSysTime,
            GnssSystemTime &gnssSysTime) const;
    // PBGnssMeasUsageInfo to GnssMeasUsageInfo
    int pbConvertToGnssMeasUsageInfo(const PBGnssMeasUsageInfo &pbGnssMeasUsageInfo,
            GnssMeasUsageInfo &gnssMeasUsageInfo) const;
    // PBLocApiSystemTimeStructUnion to SystemTimeStructUnion
    int pbConvertToSystemTimeStructUnion(const Gnss_LocSvSystemEnumType &gnssLocSvSysEnumType,
            const PBLocApiSystemTimeStructUnion &pbSysTimeStructUnion,
            SystemTimeStructUnion &sysTimeStructUnion) const;
    // PBLocApiGnssGloTimeStructType to GnssGloTimeStructType
    int pbConvertToGnssGloTimeStructType(const PBLocApiGnssGloTimeStructType &pbGnssGloTime,
            GnssGloTimeStructType &gnssGloTime) const;
    // PBGeofenceOption to GeofenceOption
    int pbConvertToGeofenceOption(const PBGeofenceOption &pbGfOpt, GeofenceOption &gfOpt) const;
    // PBGeofenceInfo to GeofenceInfo
    int pbConvertToGeofenceInfo(const PBGeofenceInfo &pbGfInfo, GeofenceInfo &gfInfo) const;
    // PBLIABodyToSensorMountParams to BodyToSensorMountParams
    int pbConvertToBodyToSensorMountParams(
            const PBLIABodyToSensorMountParams &pbBody2SensorMntParam,
            BodyToSensorMountParams &body2SensorMntParam) const;
    // Helper function of GnssDebugReport
    int pbConvertToGnssTimespec(const PBTimespec& pbTimespec,
            timespec& timespec) const;
    int pbConvertToGnssDebugTime(const PBGnssDebugTime &pbDebugTime,
            GnssDebugTime debugTime) const;
    int pbConvertToGnssDebugLocation(const PBGnssDebugLocation& pbDebugLocation,
            GnssDebugLocation& debugLocation) const;
    int pbConvertToGnssDebugSatelliteInfo(
            const PBGnssDebugSatelliteInfo &pbSatelliteInfo,
            GnssDebugSatelliteInfo &satelliteInfo) const;
    // Helper function of AntennaInfo
    int pbConvertToGnssAntennaInformaiton(
            const PBGnssAntennaInformation& pbGnssAntennaInfo,
            GnssAntennaInformation& gnssAntennaInfo) const;
    int pbConvertToGnssCoordinate(
            const PBGnssCoordinate& pbGnssCoordinate,
            GnssCoordinate& gnssCoordinate) const;
    int pbConvertTo2DimensionDoubleVector(
            const PB2DimensionDoubleVector& pbDoubleArrarys,
            std::vector<std::vector<double>>& doubleArrays) const;

    // To PB to HAL Eph report
    GnssEphAction pbConvertToEphAction (const PBGnssEphAction& pbEphAction) const;
    int pbConvertToCommanEph(const PBGnssEphCommon &pbCommanEph,
        GnssEphCommon &commanEph) const;
    int pbConvertToGpsEphResponse(const PBGpsEphemerisResponse  &pbEphResp,
        GpsEphemerisResponse  &halResp) const;
    int pbConvertToGpsEphData(const PBGpsEphemeris &pbEphInfo,
        GpsEphemeris &halEphInfo) const;
    int pbConvertToBdsEphData(const PBBdsEphemeris &pbEphInfo,
        BdsEphemeris &halEphInfo) const;
    int pbConvertToBdsEphResponse(const PBBdsEphemerisResponse &pbEphResp,
        BdsEphemerisResponse  &halResp) const;
    int pbConvertToGlonassEphData(const  PBGlonassEphemeris &pbEphInfo,
        GlonassEphemeris &halEphInfo) const;
    int pbConvertToGloEphResponse(const PBGlonassEphemerisResponse  &pbEphResp,
        GlonassEphemerisResponse  &halResp) const;
    int pbConvertToGalileoEphData(const  PBGalileoEphemeris &pbEphInfo,
        GalileoEphemeris &halEphInfo) const;
    int pbConvertToGalEphResponse(const PBGalileoEphemerisResponse  &pbEphResp,
        GalileoEphemerisResponse  &halResp) const;
    int pbConvertToQzssEphResponse(const PBQzssEphemerisResponse  &pbEphResp,
        QzssEphemerisResponse  &halResp) const;
    int pbConvertToNavicEphData(const PBNavicEphemeris &pbEphInfo,
        NavicEphemeris &halEphInfo) const;
    int pbConvertToNavicEphResponse(const PBNavicEphemerisResponse  &pbEphResp,
        NavicEphemerisResponse  &halResp) const;
    int pbConvertToSystemTime(const PBLocApiGnssSystemTimeStructType &pbSystemTime,
        GnssSystemTimeStructType &halSystemTime) const;
    GalEphSignalSource pbConvertToDataSourceSignal(
        const PBGalEphSignalSource &pbSignal) const;
};

#endif /* LOCATION_API_PBMSGCONV_H */
