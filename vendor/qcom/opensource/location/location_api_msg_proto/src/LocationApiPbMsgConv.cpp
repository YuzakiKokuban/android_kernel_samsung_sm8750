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

#define LOG_TAG "LocSvc_LocationApiPbMsgConv"

#include <inttypes.h>
#include <dirent.h>

#include <loc_pla.h>
#include <log_util.h>
#include <loc_misc_utils.h>
#include <gps_extended.h>
#include <LocIpc.h>
#include <LocTimer.h>
#include <loc_cfg.h>
#include <LocationDataTypes.h>

#include <LocationApiPbMsgConv.h>

using namespace loc_util;
using std::min;

// ********************
// LocationApiPbMsgConv
// ********************
LocationApiPbMsgConv::LocationApiPbMsgConv() {
    mPbDebugLogEnabled = false;
    mPbVerboseLogEnabled = false;
    // Logtag mechanism for Protobuf conv util log
    // Hidden configs to enable debug logs at runtime for printing many protobuf
    // conversion output (encode and decode). This will help for debugging. By
    // default the config is disabled, so these logs wont be printed.
    // LOCAPIPB_DEBUG_LOG - enable hidden debug logs
    // LOCAPIPB_VERBOSE_LOG - enable hidden verbose logs
    loc_param_s_type gpsConfParamTable[] = {
        {"LOCAPIPB_DEBUG_LOG",       &mPbDebugLogEnabled,      nullptr, 'n'},
        {"LOCAPIPB_VERBOSE_LOG",       &mPbVerboseLogEnabled,      nullptr, 'n'}
    };
    UTIL_READ_CONF(LOC_PATH_GPS_CONF, gpsConfParamTable);
    LOC_LOGd("LocApiPB logs - Debug log: %d, Verbose log:%d",
            mPbDebugLogEnabled, mPbVerboseLogEnabled);
    LOC_LOGi("LocationApiMsg version  Major:%d, Minor:%d",
            LOCAPI_MSG_VER_MAJOR, LOCAPI_MSG_VER_MINOR);
    LOC_LOGi("LocationApiDataTypes version  Major:%d, Minor:%d",
            LOCAPI_DATA_TYPES_VER_MAJOR, LOCAPI_DATA_TYPES_VER_MINOR);
}

// **** helper function for enum conversion from protobuf enums to normal format.
ELocMsgID LocationApiPbMsgConv::getEnumForPBELocMsgID(const PBELocMsgID &pbLocMsgId) const {
    ELocMsgID eLocMsgId = E_LOCAPI_UNDEFINED_MSG_ID;
    switch (pbLocMsgId) {
        case PB_E_LOCAPI_UNDEFINED_MSG_ID:
            eLocMsgId = E_LOCAPI_UNDEFINED_MSG_ID;
            break;
        case PB_E_LOCAPI_CLIENT_REGISTER_MSG_ID:
            eLocMsgId = E_LOCAPI_CLIENT_REGISTER_MSG_ID;
            break;
        case PB_E_LOCAPI_CLIENT_DEREGISTER_MSG_ID:
            eLocMsgId = E_LOCAPI_CLIENT_DEREGISTER_MSG_ID;
            break;
        case PB_E_LOCAPI_CAPABILILTIES_MSG_ID:
            eLocMsgId = E_LOCAPI_CAPABILILTIES_MSG_ID;
            break;
        case PB_E_LOCAPI_HAL_READY_MSG_ID:
            eLocMsgId = E_LOCAPI_HAL_READY_MSG_ID;
            break;
        case PB_E_LOCAPI_START_TRACKING_MSG_ID:
            eLocMsgId = E_LOCAPI_START_TRACKING_MSG_ID;
            break;
        case PB_E_LOCAPI_STOP_TRACKING_MSG_ID:
            eLocMsgId = E_LOCAPI_STOP_TRACKING_MSG_ID;
            break;
        case PB_E_LOCAPI_UPDATE_CALLBACKS_MSG_ID:
            eLocMsgId = E_LOCAPI_UPDATE_CALLBACKS_MSG_ID;
            break;
        case PB_E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID:
            eLocMsgId = E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID;
            break;
        case PB_E_LOCAPI_CONTROL_UPDATE_CONFIG_MSG_ID:
            eLocMsgId = E_LOCAPI_CONTROL_UPDATE_CONFIG_MSG_ID;
            break;
        case PB_E_LOCAPI_CONTROL_DELETE_AIDING_DATA_MSG_ID:
            eLocMsgId = E_LOCAPI_CONTROL_DELETE_AIDING_DATA_MSG_ID;
            break;
        case PB_E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID:
            eLocMsgId = E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID;
            break;
        case PB_E_LOCAPI_LOCATION_MSG_ID:
            eLocMsgId = E_LOCAPI_LOCATION_MSG_ID;
            break;
        case PB_E_LOCAPI_LOCATION_INFO_MSG_ID:
            eLocMsgId = E_LOCAPI_LOCATION_INFO_MSG_ID;
            break;
        case PB_E_LOCAPI_SATELLITE_VEHICLE_MSG_ID:
            eLocMsgId = E_LOCAPI_SATELLITE_VEHICLE_MSG_ID;
            break;
        case PB_E_LOCAPI_NMEA_MSG_ID:
            eLocMsgId = E_LOCAPI_NMEA_MSG_ID;
            break;
        case PB_E_LOCAPI_DATA_MSG_ID:
            eLocMsgId = E_LOCAPI_DATA_MSG_ID;
            break;
        case PB_E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID:
            eLocMsgId = E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID;
            break;
        case PB_E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID:
            eLocMsgId = E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID;
            break;
        case PB_E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID:
            eLocMsgId = E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID;
            break;
        case PB_E_LOCAPI_START_BATCHING_MSG_ID:
            eLocMsgId = E_LOCAPI_START_BATCHING_MSG_ID;
            break;
        case PB_E_LOCAPI_STOP_BATCHING_MSG_ID:
            eLocMsgId = E_LOCAPI_STOP_BATCHING_MSG_ID;
            break;
        case PB_E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID:
            eLocMsgId = E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID;
            break;
        case PB_E_LOCAPI_BATCHING_MSG_ID:
            eLocMsgId = E_LOCAPI_BATCHING_MSG_ID;
            break;
        case PB_E_LOCAPI_ADD_GEOFENCES_MSG_ID:
            eLocMsgId = E_LOCAPI_ADD_GEOFENCES_MSG_ID;
            break;
        case PB_E_LOCAPI_REMOVE_GEOFENCES_MSG_ID:
            eLocMsgId = E_LOCAPI_REMOVE_GEOFENCES_MSG_ID;
            break;
        case PB_E_LOCAPI_MODIFY_GEOFENCES_MSG_ID:
            eLocMsgId = E_LOCAPI_MODIFY_GEOFENCES_MSG_ID;
            break;
        case PB_E_LOCAPI_PAUSE_GEOFENCES_MSG_ID:
            eLocMsgId = E_LOCAPI_PAUSE_GEOFENCES_MSG_ID;
            break;
        case PB_E_LOCAPI_RESUME_GEOFENCES_MSG_ID:
            eLocMsgId = E_LOCAPI_RESUME_GEOFENCES_MSG_ID;
            break;
        case PB_E_LOCAPI_GEOFENCE_BREACH_MSG_ID:
            eLocMsgId = E_LOCAPI_GEOFENCE_BREACH_MSG_ID;
            break;
        case PB_E_LOCAPI_MEAS_MSG_ID:
            eLocMsgId = E_LOCAPI_MEAS_MSG_ID;
            break;
        case PB_E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID:
            eLocMsgId = E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID;
            break;
        case PB_E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID:
            eLocMsgId = E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID;
            break;
        case PB_E_LOCAPI_DC_REPORT_MSG_ID:
            eLocMsgId = E_LOCAPI_DC_REPORT_MSG_ID;
            break;
        case PB_E_LOCAPI_GET_SINGLE_POS_REQ_MSG_ID:
            eLocMsgId = E_LOCAPI_GET_SINGLE_POS_REQ_MSG_ID;
            break;
        case PB_E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID:
            eLocMsgId = E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID;
            break;
        case PB_E_LOCAPI_EPH_MSG_ID:
            eLocMsgId = E_LOCAPI_EPH_MSG_ID;
            break;
        case PB_E_LOCAPI_PINGTEST_MSG_ID:
            eLocMsgId = E_LOCAPI_PINGTEST_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_LEVER_ARM_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_LEVER_ARM_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID;
            break;
        case PB_E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID:
            eLocMsgId = E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID;
            break;
        case PB_E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID:
            eLocMsgId = E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID;
            break;
        case PB_E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID:
            eLocMsgId = E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID;
            break;
        case PB_E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID:
            eLocMsgId = E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID;
            break;
        case PB_E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID:
            eLocMsgId = E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID;
            break;
        case PB_E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID:
            eLocMsgId = E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID;
            break;
        case PB_E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID:
            eLocMsgId = E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID;
            break;
        case PB_E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID:
            eLocMsgId = E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID;
            break;
        case PB_E_LOCAPI_GET_DEBUG_REQ_MSG_ID:
            eLocMsgId = E_LOCAPI_GET_DEBUG_REQ_MSG_ID;
            break;
        case PB_E_LOCAPI_GET_DEBUG_RESP_MSG_ID:
            eLocMsgId = E_LOCAPI_GET_DEBUG_RESP_MSG_ID;
            break;
        case PB_E_INTAPI_INJECT_LOCATION_MSG_ID:
            eLocMsgId = E_INTAPI_INJECT_LOCATION_MSG_ID;
            break;
        case PB_E_LOCAPI_GET_ANTENNA_INFO_MSG_ID:
            eLocMsgId = E_LOCAPI_GET_ANTENNA_INFO_MSG_ID;
            break;
        case PB_E_LOCAPI_ANTENNA_INFO_MSG_ID:
            eLocMsgId = E_LOCAPI_ANTENNA_INFO_MSG_ID;
            break;
        case PB_E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID:
            eLocMsgId = E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID;
            break;
        case PB_E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID:
            eLocMsgId = E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID;
            break;
        case PB_E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
            eLocMsgId = E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID;
            break;
        case PB_E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
            eLocMsgId = E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID;
            break;
        case PB_E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID:
            eLocMsgId = E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID;
            break;
        case PB_E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID:
            eLocMsgId = E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID;
            break;
        case PB_E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID:
            eLocMsgId = E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbLocMsgId:%d, eLocMsgId:%d", pbLocMsgId, eLocMsgId);
    return eLocMsgId;
}

LocationError LocationApiPbMsgConv::getEnumForPBLocationError(
        const PBLocationError &pbLocErr) const {
    LocationError locErr = LOCATION_ERROR_GENERAL_FAILURE;
    if (PB_LOCATION_ERROR_SUCCESS == pbLocErr) {
        locErr = LOCATION_ERROR_SUCCESS;
    } else if (PB_LOCATION_ERROR_INVALID_PARAMETER == pbLocErr) {
        locErr = LOCATION_ERROR_INVALID_PARAMETER;
    } else if (PB_LOCATION_ERROR_NOT_SUPPORTED  == pbLocErr) {
        locErr = LOCATION_ERROR_NOT_SUPPORTED;
    } else if (PB_LOCATION_ERROR_TIMEOUT == pbLocErr) {
        locErr = LOCATION_ERROR_TIMEOUT;
    }
    return locErr;
}

ClientType LocationApiPbMsgConv::getEnumForPBClientType(const PBClientType &pbClntTyp) const {
    ClientType clntTyp = LOCATION_CLIENT_API;
    if (PB_LOCATION_INTEGRATION_API == pbClntTyp) {
        clntTyp = LOCATION_INTEGRATION_API;
    }
    LocApiPb_LOGv("LocApiPB: pbClntTyp:%d, clntTyp:%d", pbClntTyp, clntTyp);
    return clntTyp;
}

BatchingMode LocationApiPbMsgConv::getEnumForPBBatchingMode(
        const PBBatchingMode &pbBatchMode) const {
    BatchingMode batchMode = BATCHING_MODE_ROUTINE;
    switch (pbBatchMode) {
        case PB_BATCHING_MODE_TRIP:
            batchMode = BATCHING_MODE_TRIP;
            break;
        case PB_BATCHING_MODE_NO_AUTO_REPORT:
            batchMode = BATCHING_MODE_NO_AUTO_REPORT;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbBatchMode:%d, batchMode:%d", pbBatchMode, batchMode);
    return batchMode;
}

GnssSuplMode LocationApiPbMsgConv::getEnumForPBGnssSuplMode(
        const PBGnssSuplMode &pbGnssSuplMode) const {
    GnssSuplMode gnssSuplMode = GNSS_SUPL_MODE_STANDALONE;
    switch (pbGnssSuplMode) {
        case PB_GNSS_SUPL_MODE_MSB:
            gnssSuplMode = GNSS_SUPL_MODE_MSB;
            break;
        case PB_GNSS_SUPL_MODE_MSA:
            gnssSuplMode = GNSS_SUPL_MODE_MSA;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssSuplMode:%d, gnssSuplMode:%d", pbGnssSuplMode, gnssSuplMode);
    return gnssSuplMode;
}

FixQualityLevel LocationApiPbMsgConv::getEnumForPBFixQualityLevel(
    const PBFixQualityLevel &pbFixQualityLevel) const {
    FixQualityLevel qualityLevelAccepted = QUALITY_HIGH_ACCU_FIX_ONLY;
    switch (pbFixQualityLevel) {
        case PB_QUALITY_ANY_VALID_FIX:
            qualityLevelAccepted = QUALITY_ANY_VALID_FIX;
            break;
        case PB_QUALITY_ANY_OR_FAILED_FIX:
            qualityLevelAccepted = QUALITY_ANY_OR_FAILED_FIX;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbFixQualityLevel:%d, qualityLevelAccepted:%d", pbFixQualityLevel,
            qualityLevelAccepted);

    return qualityLevelAccepted;
}


BatchingStatus LocationApiPbMsgConv::getEnumForPBBatchingStatus(
        const PBBatchingStatus &pbBatchStat) const {
    BatchingStatus batchStat = BATCHING_STATUS_POSITION_UNAVAILABLE;
    switch (pbBatchStat) {
        case PB_BATCHING_STATUS_TRIP_COMPLETED:
            batchStat = BATCHING_STATUS_TRIP_COMPLETED;
            break;
        case PB_BATCHING_STATUS_POSITION_AVAILABE:
            batchStat = BATCHING_STATUS_POSITION_AVAILABE;
            break;
        case PB_BATCHING_STATUS_POSITION_UNAVAILABLE:
            batchStat = BATCHING_STATUS_POSITION_UNAVAILABLE;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbBatchStat:%d, batchStat:%d", pbBatchStat, batchStat);
    return batchStat;
}

GnssMeasurementsMultipathIndicator LocationApiPbMsgConv::getEnumForPBGnssMeasMultipathIndic(
        const PBGnssMeasurementsMultipathIndicator &pbGnssMeasMultipathIndic) const {
    GnssMeasurementsMultipathIndicator gnssMeasMultipathIndic =
            GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN;
    switch (pbGnssMeasMultipathIndic) {
        case PB_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN:
            gnssMeasMultipathIndic = GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN;
            break;
        case PB_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT:
            gnssMeasMultipathIndic = GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT;
            break;
        case PB_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT:
            gnssMeasMultipathIndic = GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssMeasMultipathIndic:%d, gnssMeasMultipathIndic:%d",
            pbGnssMeasMultipathIndic, gnssMeasMultipathIndic);
    return gnssMeasMultipathIndic;
}

LocationReliability LocationApiPbMsgConv::getEnumForPBLocReliability(
        const PBLocationReliability &pbLocReliability) const {
    LocationReliability locReliability = LOCATION_RELIABILITY_NOT_SET;
    switch (pbLocReliability) {
        case PB_LOCATION_RELIABILITY_NOT_SET:
            locReliability = LOCATION_RELIABILITY_NOT_SET;
            break;
        case PB_LOCATION_RELIABILITY_VERY_LOW:
            locReliability = LOCATION_RELIABILITY_VERY_LOW;
            break;
        case PB_LOCATION_RELIABILITY_LOW:
            locReliability = LOCATION_RELIABILITY_LOW;
            break;
        case PB_LOCATION_RELIABILITY_MEDIUM:
            locReliability = LOCATION_RELIABILITY_MEDIUM;
            break;
        case PB_LOCATION_RELIABILITY_HIGH:
            locReliability = LOCATION_RELIABILITY_HIGH;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbLocReliability:%d, locReliability:%d", pbLocReliability,
            locReliability);
    return locReliability;
}

Gnss_LocSvSystemEnumType LocationApiPbMsgConv::getEnumForPBGnssLocSvSystem(
            const PBLocApiGnss_LocSvSystemEnumType &pbGnssLocSvSysEnumType) const {
    Gnss_LocSvSystemEnumType gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_UNKNOWN;
    switch (pbGnssLocSvSysEnumType) {
        case PB_GNSS_LOC_SV_SYSTEM_INVALID:
            gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_UNKNOWN;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_GPS:
            gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_GPS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_GALILEO:
            gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_GALILEO;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_SBAS:
            gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_SBAS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_GLONASS:
            gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_GLONASS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_BDS:
            gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_BDS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_QZSS:
            gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_QZSS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_NAVIC:
            gnssLocSvSysEnumType = GNSS_LOC_SV_SYSTEM_NAVIC;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssLocSvSysEnumType:%d, gnssLocSvSysEnumType:%d",
            pbGnssLocSvSysEnumType, gnssLocSvSysEnumType);
    return gnssLocSvSysEnumType;
}

LocOutputEngineType LocationApiPbMsgConv::getEnumForPBLocOutputEngineType(
            const PBLocApiOutputEngineType &pbLocOpEngType) const {
    LocOutputEngineType locOpEngType = LOC_OUTPUT_ENGINE_FUSED;
    switch (pbLocOpEngType) {
        case PB_LOC_OUTPUT_ENGINE_FUSED:
            locOpEngType = LOC_OUTPUT_ENGINE_FUSED;
            break;
        case PB_LOC_OUTPUT_ENGINE_SPE:
            locOpEngType = LOC_OUTPUT_ENGINE_SPE;
            break;
        case PB_LOC_OUTPUT_ENGINE_PPE:
            locOpEngType = LOC_OUTPUT_ENGINE_PPE;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbLocOpEngType:%d, locOpEngType:%d",
            pbLocOpEngType, locOpEngType);
    return locOpEngType;
}

AgcStatus LocationApiPbMsgConv::getEnumForPBAgcStatus(const PBAgcStatus &pbAgcStatus) const {
    AgcStatus agcStatus = AGC_STATUS_UNKNOWN;
    switch (pbAgcStatus) {
        case PB_AGC_STATUS_NO_SATURATION:
            agcStatus = AGC_STATUS_NO_SATURATION;
            break;
        case PB_AGC_STATUS_FRONT_END_GAIN_MAXIMUM_SATURATION:
            agcStatus = AGC_STATUS_FRONT_END_GAIN_MAXIMUM_SATURATION;
            break;
        case PB_AGC_STATUS_FRONT_END_GAIN_MINIMUM_SATURATION:
            agcStatus = AGC_STATUS_FRONT_END_GAIN_MINIMUM_SATURATION;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbAgcStatus:%d, agcStatus:%d", pbAgcStatus, agcStatus);
    return agcStatus;
}
// HAL position mask from PB position engine mask
LocEngineRunState LocationApiPbMsgConv::getEnumForPBLocEngineRunState(
            const PBLocEngineRunState &pbLocEngRunState) const {
    LocEngineRunState locEngRunState = (LocEngineRunState) 0;

    if (pbLocEngRunState == PB_LOC_ENGINE_RUN_STATE_PAUSE) {
        locEngRunState = LOC_ENGINE_RUN_STATE_PAUSE;
    } else if (pbLocEngRunState == PB_LOC_ENGINE_RUN_STATE_RESUME) {
        locEngRunState = LOC_ENGINE_RUN_STATE_RESUME;
    } else if (pbLocEngRunState == PB_LOC_ENGINE_RUN_STATE_PAUSE_RETAIN) {
        locEngRunState = LOC_ENGINE_RUN_STATE_PAUSE_RETAIN;
    }

    LocApiPb_LOGv("LocApiPB: pbEngineRunState:%d, locEngRunState:%d",
            pbLocEngRunState, locEngRunState);
    return locEngRunState;
}

// HAL position mask to PB position engine mask
uint32_t LocationApiPbMsgConv::getPBEnumForLocEngineRunState(
        const LocEngineRunState& locEngineRunState) const {

    PBLocEngineRunState pbEngineRunState = PB_LOC_ENGINE_RUN_STATE_INVALID;;

    if (locEngineRunState == LOC_ENGINE_RUN_STATE_PAUSE) {
        pbEngineRunState = PB_LOC_ENGINE_RUN_STATE_PAUSE;
    } else if (locEngineRunState ==LOC_ENGINE_RUN_STATE_RESUME) {
        pbEngineRunState = PB_LOC_ENGINE_RUN_STATE_RESUME;
    } else if (locEngineRunState ==LOC_ENGINE_RUN_STATE_PAUSE_RETAIN) {
        pbEngineRunState = PB_LOC_ENGINE_RUN_STATE_PAUSE_RETAIN;
    }
    LocApiPb_LOGd("LocApiPB: locEngineRunState: %d, pbEngineRunState: %d",
                  locEngineRunState, pbEngineRunState);
    return pbEngineRunState;
}

// HAL TerrestrialTechMask from PB format
uint32_t LocationApiPbMsgConv::getTerrestrialTechMaskFromPB(
        const uint32_t &pbTerrestrialTechMask) const {
    uint32_t terrestrialTechMask = 0;

    if (pbTerrestrialTechMask & PB_TERRESTRIAL_TECH_GTP_WWAN) {
        terrestrialTechMask |= TERRESTRIAL_TECH_GTP_WWAN;
    }

    return terrestrialTechMask;
}

// PB TerrestrialTechMask from HAL format
uint32_t LocationApiPbMsgConv::getPBMaskForTerrestrialTechMask(
        const uint32_t& terrestrialTechMask) const {
    uint32_t pbTerrestrialTechMask = PB_TERRESTRIAL_TECH_INVALID;

    if (terrestrialTechMask & TERRESTRIAL_TECH_GTP_WWAN) {
        pbTerrestrialTechMask |= PB_TERRESTRIAL_TECH_GTP_WWAN;
    }

    return pbTerrestrialTechMask;
}

// HAL NMEA types mask from PB format
uint32_t LocationApiPbMsgConv::getNmeaTypesMaskFromPB(
        const uint32_t &pbNmeaTypesMask) const {

    uint32_t nmeaTypesMask = NMEA_TYPE_NONE;
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GGA) {
        nmeaTypesMask |= NMEA_TYPE_GGA;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_RMC) {
        nmeaTypesMask |= NMEA_TYPE_RMC;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GSA) {
        nmeaTypesMask |= NMEA_TYPE_GSA;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_VTG) {
        nmeaTypesMask |= NMEA_TYPE_VTG;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GNS) {
        nmeaTypesMask |= NMEA_TYPE_GNS;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_DTM) {
        nmeaTypesMask |= NMEA_TYPE_DTM;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GPGSV) {
        nmeaTypesMask |= NMEA_TYPE_GPGSV;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GLGSV) {
        nmeaTypesMask |= NMEA_TYPE_GLGSV;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GAGSV) {
        nmeaTypesMask |= NMEA_TYPE_GAGSV;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GQGSV) {
        nmeaTypesMask |= NMEA_TYPE_GQGSV;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GBGSV) {
        nmeaTypesMask |= NMEA_TYPE_GBGSV;
    }
    if (pbNmeaTypesMask & PB_NMEA_TYPE_GIGSV) {
        nmeaTypesMask |= NMEA_TYPE_GIGSV;
    }
    return nmeaTypesMask;
}

// PB NMEA types mask from HAL format
uint32_t LocationApiPbMsgConv::getPBMaskForNmeaTypesMask(
        const uint32_t& nmeaTypesMask) const {
    uint32_t pbNmeaTypesMask = PB_NMEA_TYPE_INVALID;

    if (nmeaTypesMask & NMEA_TYPE_GGA) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GGA;
    }
    if (nmeaTypesMask & NMEA_TYPE_RMC) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_RMC;
    }
    if (nmeaTypesMask & NMEA_TYPE_GSA) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GSA;
    }
    if (nmeaTypesMask & NMEA_TYPE_VTG) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_VTG;
    }
    if (nmeaTypesMask & NMEA_TYPE_GNS) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GNS;
    }
    if (nmeaTypesMask & NMEA_TYPE_DTM) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_DTM;
    }
    if (nmeaTypesMask & NMEA_TYPE_GPGSV) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GPGSV;
    }
    if (nmeaTypesMask & NMEA_TYPE_GLGSV) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GLGSV;
    }
    if (nmeaTypesMask & NMEA_TYPE_GAGSV) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GAGSV;
    }
    if (nmeaTypesMask & NMEA_TYPE_GQGSV) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GQGSV;
    }
    if (nmeaTypesMask & NMEA_TYPE_GBGSV) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GBGSV;
    }
    if (nmeaTypesMask & NMEA_TYPE_GIGSV) {
        pbNmeaTypesMask |= PB_NMEA_TYPE_GIGSV;
    }
    return pbNmeaTypesMask;
}

// PBLocApiGnss_LocSvSystemEnumType to GnssSvType
GnssSvType LocationApiPbMsgConv::getGnssSvTypeFromPBGnssLocSvSystemEnumType(
            const PBLocApiGnss_LocSvSystemEnumType &pbGnssLocSvSysEnum) const {
    GnssSvType gnssSvType = GNSS_SV_TYPE_UNKNOWN;
    switch (pbGnssLocSvSysEnum) {
        case PB_GNSS_LOC_SV_SYSTEM_GPS:
            gnssSvType = GNSS_SV_TYPE_GPS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_GALILEO:
            gnssSvType = GNSS_SV_TYPE_GALILEO;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_SBAS:
            gnssSvType = GNSS_SV_TYPE_SBAS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_GLONASS:
            gnssSvType = GNSS_SV_TYPE_GLONASS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_BDS:
            gnssSvType = GNSS_SV_TYPE_BEIDOU;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_QZSS:
            gnssSvType = GNSS_SV_TYPE_QZSS;
            break;
        case PB_GNSS_LOC_SV_SYSTEM_NAVIC:
            gnssSvType = GNSS_SV_TYPE_NAVIC;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssLocSvSysEnum:%d, gnssSvType:%d", pbGnssLocSvSysEnum,
            gnssSvType);
    return gnssSvType;
}

GnssDcReportType LocationApiPbMsgConv::getDcReportTypeFromPB(
        const PBGnssDcReportType& pbDcReportType) const {
    GnssDcReportType dcReportType = GNSS_DC_REPORT_TYPE_UNDEFINED;
    switch (pbDcReportType) {
    case PB_QZSS_JMA_DISASTER_PREVENTION_INFO:
        dcReportType = QZSS_JMA_DISASTER_PREVENTION_INFO;
        break;
    case PB_QZSS_NON_JMA_DISASTER_PREVENTION_INFO:
        dcReportType = QZSS_NON_JMA_DISASTER_PREVENTION_INFO;
        break;
    default:
        break;
    }
    return dcReportType;
}

PBGnssDcReportType LocationApiPbMsgConv::getPBEnumForDcReportType(
        const GnssDcReportType& dcReportType) const {
    PBGnssDcReportType pbDcReportType = PB_GNSS_DC_REPORT_TYPE_UNDEFINED;
    switch (dcReportType) {
    case QZSS_JMA_DISASTER_PREVENTION_INFO:
        pbDcReportType = PB_QZSS_JMA_DISASTER_PREVENTION_INFO;
        break;
    case QZSS_NON_JMA_DISASTER_PREVENTION_INFO:
        pbDcReportType = PB_QZSS_NON_JMA_DISASTER_PREVENTION_INFO;
        break;
    default:
        break;
    }
    return pbDcReportType;
}

// PBDebugLogLevel to DebugLogLevel
DebugLogLevel LocationApiPbMsgConv::getDebugLogLevelFromPB (
        const PBDebugLogLevel &pbLogLevel) const {
    DebugLogLevel logLevel = DEBUG_LOG_LEVEL_NONE;
    switch (pbLogLevel) {
    case PB_DEBUG_LOG_LEVEL_ERROR:
        logLevel = DEBUG_LOG_LEVEL_ERROR;
        break;
    case PB_DEBUG_LOG_LEVEL_WARNING:
        logLevel = DEBUG_LOG_LEVEL_WARNING;
        break;
    case PB_DEBUG_LOG_LEVEL_INFO:
        logLevel = DEBUG_LOG_LEVEL_INFO;
        break;
    case PB_DEBUG_LOG_LEVEL_DEBUG:
        logLevel = DEBUG_LOG_LEVEL_DEBUG;
        break;
    case PB_DEBUG_LOG_LEVEL_VERBOSE:
        logLevel = DEBUG_LOG_LEVEL_VERBOSE;
        break;
    default:
        break;
    }
    return logLevel;
}

// DebugLogLevel to PBDebugLogLevel
PBDebugLogLevel LocationApiPbMsgConv::getPBEnumForDebugLogLevel(
        const DebugLogLevel &logLevel) const {

    PBDebugLogLevel pbLogLevel = PB_DEBUG_LOG_LEVEL_NONE;
    switch (logLevel) {
    case DEBUG_LOG_LEVEL_ERROR:
        pbLogLevel = PB_DEBUG_LOG_LEVEL_ERROR;
        break;
    case DEBUG_LOG_LEVEL_WARNING:
        pbLogLevel = PB_DEBUG_LOG_LEVEL_WARNING;
        break;
    case DEBUG_LOG_LEVEL_INFO:
        pbLogLevel = PB_DEBUG_LOG_LEVEL_INFO;
        break;
    case DEBUG_LOG_LEVEL_DEBUG:
        pbLogLevel = PB_DEBUG_LOG_LEVEL_DEBUG;
        break;
    case DEBUG_LOG_LEVEL_VERBOSE:
        pbLogLevel = PB_DEBUG_LOG_LEVEL_VERBOSE;
        break;
    default:
        break;
    }
    return pbLogLevel;
}

XtraStatusUpdateType LocationApiPbMsgConv::getXtraStatusUpdateTypeFromPB(
        const PBXtraStatusUpdateType &pbUpdateType) const {
    XtraStatusUpdateType updateType = XTRA_STATUS_UPDATE_UNDEFINED;

    switch (pbUpdateType) {
    case PB_XTRA_STATUS_UPDATE_UPON_QUERY:
        updateType = XTRA_STATUS_UPDATE_UPON_QUERY;
        break;
    case PB_XTRA_STATUS_UPDATE_UPON_REGISTRATION:
        updateType = XTRA_STATUS_UPDATE_UPON_REGISTRATION;
        break;
    case PB_XTRA_STATUS_UPDATE_UPON_STATUS_CHANGE:
        updateType = XTRA_STATUS_UPDATE_UPON_STATUS_CHANGE;
        break;
    default:
        break;
    }

    return updateType;
}

XtraDataStatus LocationApiPbMsgConv::getXtraDataStatusFromPB(
        const PBXtraDataStatus &pbXtraDataStatus) const {
    XtraDataStatus status = XTRA_DATA_STATUS_UNKNOWN;
    switch (pbXtraDataStatus) {
    case PB_XTRA_DATA_STATUS_NOT_AVAIL:
        status = XTRA_DATA_STATUS_NOT_AVAIL;
        break;
    case PB_XTRA_DATA_STATUS_NOT_VALID:
        status = XTRA_DATA_STATUS_NOT_VALID;
        break;
    case PB_XTRA_DATA_STATUS_VALID:
        status = XTRA_DATA_STATUS_VALID;
        break;
    default:
        break;
    }
    return status;
}

PBXtraDataStatus LocationApiPbMsgConv::getPBEnumForXtraDataStatus(
        const XtraDataStatus &xtraDataStatus) const{
    PBXtraDataStatus pbStatus = PB_XTRA_DATA_STATUS_UNKNOWN;
    switch (xtraDataStatus) {
    case XTRA_DATA_STATUS_NOT_AVAIL:
        pbStatus = PB_XTRA_DATA_STATUS_NOT_AVAIL;
        break;
    case XTRA_DATA_STATUS_NOT_VALID:
        pbStatus = PB_XTRA_DATA_STATUS_NOT_VALID;
        break;
    case XTRA_DATA_STATUS_VALID:
        pbStatus = PB_XTRA_DATA_STATUS_VALID;
        break;
    default:
        break;
    }
    return pbStatus;
}

PBXtraStatusUpdateType LocationApiPbMsgConv::getPBEnumForXtraStatusUpdateType(
        const XtraStatusUpdateType &updateType) const {

    PBXtraStatusUpdateType pbUpdateType = PB_XTRA_STATUS_UPDATE_UNDEFINED;

    switch (updateType) {
    case XTRA_STATUS_UPDATE_UPON_QUERY:
        pbUpdateType = PB_XTRA_STATUS_UPDATE_UPON_QUERY;
        break;
    case XTRA_STATUS_UPDATE_UPON_REGISTRATION:
        pbUpdateType = PB_XTRA_STATUS_UPDATE_UPON_REGISTRATION;
        break;
    case XTRA_STATUS_UPDATE_UPON_STATUS_CHANGE:
        pbUpdateType = PB_XTRA_STATUS_UPDATE_UPON_STATUS_CHANGE;
        break;
    default:
        break;
    }

    return pbUpdateType;
}

// **** helper function for enum conversion to protobuf enums
PBELocMsgID LocationApiPbMsgConv::getPBEnumForELocMsgID(const ELocMsgID &eLocMsgId) const {
    PBELocMsgID pbLocMsgId = PB_E_LOCAPI_UNDEFINED_MSG_ID;
    switch (eLocMsgId) {
        case E_LOCAPI_CLIENT_REGISTER_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_CLIENT_REGISTER_MSG_ID;
            break;
        case E_LOCAPI_CLIENT_DEREGISTER_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_CLIENT_DEREGISTER_MSG_ID;
            break;
        case E_LOCAPI_CAPABILILTIES_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_CAPABILILTIES_MSG_ID;
            break;
        case E_LOCAPI_HAL_READY_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_HAL_READY_MSG_ID;
            break;
        case E_LOCAPI_START_TRACKING_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_START_TRACKING_MSG_ID;
            break;
        case E_LOCAPI_STOP_TRACKING_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_STOP_TRACKING_MSG_ID;
            break;
        case E_LOCAPI_UPDATE_CALLBACKS_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_UPDATE_CALLBACKS_MSG_ID;
            break;
        case E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID;
            break;
        case E_LOCAPI_CONTROL_UPDATE_CONFIG_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_CONTROL_UPDATE_CONFIG_MSG_ID;
            break;
        case E_LOCAPI_CONTROL_DELETE_AIDING_DATA_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_CONTROL_DELETE_AIDING_DATA_MSG_ID;
            break;
        case E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID;
            break;
        case E_LOCAPI_LOCATION_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_LOCATION_MSG_ID;
            break;
        case E_LOCAPI_LOCATION_INFO_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_LOCATION_INFO_MSG_ID;
            break;
        case E_LOCAPI_SATELLITE_VEHICLE_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_SATELLITE_VEHICLE_MSG_ID;
            break;
        case E_LOCAPI_NMEA_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_NMEA_MSG_ID;
            break;
        case E_LOCAPI_DATA_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_DATA_MSG_ID;
            break;
        case E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID;
            break;
        case E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID;
            break;
        case E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID;
            break;
        case E_LOCAPI_START_BATCHING_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_START_BATCHING_MSG_ID;
            break;
        case E_LOCAPI_STOP_BATCHING_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_STOP_BATCHING_MSG_ID;
            break;
        case E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID;
            break;
        case E_LOCAPI_BATCHING_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_BATCHING_MSG_ID;
            break;
        case E_LOCAPI_ADD_GEOFENCES_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_ADD_GEOFENCES_MSG_ID;
            break;
        case E_LOCAPI_REMOVE_GEOFENCES_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_REMOVE_GEOFENCES_MSG_ID;
            break;
        case E_LOCAPI_MODIFY_GEOFENCES_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_MODIFY_GEOFENCES_MSG_ID;
            break;
        case E_LOCAPI_PAUSE_GEOFENCES_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_PAUSE_GEOFENCES_MSG_ID;
            break;
        case E_LOCAPI_RESUME_GEOFENCES_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_RESUME_GEOFENCES_MSG_ID;
            break;
        case E_LOCAPI_GEOFENCE_BREACH_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GEOFENCE_BREACH_MSG_ID;
            break;
        case E_LOCAPI_MEAS_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_MEAS_MSG_ID;
            break;
        case E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID;
            break;
        case E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID;
            break;
        case E_LOCAPI_DC_REPORT_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_DC_REPORT_MSG_ID;
            break;
        case E_LOCAPI_GET_SINGLE_POS_REQ_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GET_SINGLE_POS_REQ_MSG_ID;
            break;
        case E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID;
            break;
        case E_LOCAPI_EPH_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_EPH_MSG_ID;
            break;
        case E_LOCAPI_PINGTEST_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_PINGTEST_MSG_ID;
            break;
        case E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID;
            break;
        case E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID;
            break;
        case E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID;
            break;
        case E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID;
            break;
        case E_INTAPI_CONFIG_LEVER_ARM_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_LEVER_ARM_MSG_ID;
            break;
        case E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID;
            break;
        case E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID;
            break;
        case E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID;
            break;
        case E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID;
            break;
        case E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID;
            break;
        case E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID;
            break;
        case E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID;
            break;
        case E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID;
            break;
        case E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID;
            break;
        case E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID;
            break;
        case E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID;
            break;
        case E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID;
            break;
        case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID;
            break;
        case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID;
            break;
        case E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID;
            break;
        case E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID;
            break;
        case E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID;
            break;
        case E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID;
            break;
        case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID;
            break;
        case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID;
            break;
        case E_LOCAPI_GET_DEBUG_REQ_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GET_DEBUG_REQ_MSG_ID;
            break;
        case E_LOCAPI_GET_DEBUG_RESP_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GET_DEBUG_RESP_MSG_ID;
            break;
        case E_INTAPI_INJECT_LOCATION_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_INJECT_LOCATION_MSG_ID;
            break;
        case E_LOCAPI_GET_ANTENNA_INFO_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_GET_ANTENNA_INFO_MSG_ID;
            break;
        case E_LOCAPI_ANTENNA_INFO_MSG_ID:
            pbLocMsgId = PB_E_LOCAPI_ANTENNA_INFO_MSG_ID;
            break;
        case E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID;
            break;
        case E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID;
            break;
        case E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID;
            break;
        case E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID;
            break;
        case E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID;
            break;
        case E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID;
            break;
        case E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID:
            pbLocMsgId = PB_E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: eLocMsgId:%d, pbLocMsgId:%d", eLocMsgId, pbLocMsgId);
    return pbLocMsgId;
}

PBClientType LocationApiPbMsgConv::getPBEnumForClientType(const ClientType &clientTyp) const {
    PBClientType pbClntTyp = PB_LOCATION_CLIENT_TYPE_INVALID;
    if (LOCATION_CLIENT_API == clientTyp) {
        pbClntTyp = PB_LOCATION_CLIENT_API;
    } else {
        // LOCATION_INTEGRATION_API
        pbClntTyp = PB_LOCATION_INTEGRATION_API;
    }
    return pbClntTyp;
}

PBBatchingMode LocationApiPbMsgConv::getPBEnumForBatchingMode(
        const BatchingMode &batchMode) const {
    PBBatchingMode pbBatchMode = PB_BATCHING_MODE_ROUTINE;
    switch (batchMode) {
        case BATCHING_MODE_ROUTINE:
            pbBatchMode = PB_BATCHING_MODE_ROUTINE;
            break;
        case BATCHING_MODE_TRIP:
            pbBatchMode = PB_BATCHING_MODE_TRIP;
            break;
        case BATCHING_MODE_NO_AUTO_REPORT:
            pbBatchMode = PB_BATCHING_MODE_NO_AUTO_REPORT;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: batchMode:%d, pbBatchMode:%d", batchMode, pbBatchMode);
    return pbBatchMode;
}

PBLocationError LocationApiPbMsgConv::getPBEnumForLocationError(
        const LocationError &locErr) const {
    PBLocationError pbLocErr = PB_LOCATION_ERROR_GENERAL_FAILURE;
    switch (locErr) {
        case LOCATION_ERROR_SUCCESS:
            pbLocErr = PB_LOCATION_ERROR_SUCCESS;
            break;
        case LOCATION_ERROR_INVALID_PARAMETER:
            pbLocErr = PB_LOCATION_ERROR_INVALID_PARAMETER;
            break;
        case LOCATION_ERROR_NOT_SUPPORTED:
            pbLocErr = PB_LOCATION_ERROR_NOT_SUPPORTED;
            break;
        case LOCATION_ERROR_TIMEOUT:
            pbLocErr = PB_LOCATION_ERROR_TIMEOUT;
            break;
        default:
            break;
    }
    return pbLocErr;
}

PBGnssSuplMode LocationApiPbMsgConv::getPBEnumForGnssSuplMode(
        const GnssSuplMode &gnssSuplMode) const {
    PBGnssSuplMode pbGnssSuplMode = PB_GNSS_SUPL_MODE_STANDALONE;
    switch (gnssSuplMode) {
        case GNSS_SUPL_MODE_STANDALONE:
            pbGnssSuplMode = PB_GNSS_SUPL_MODE_STANDALONE;
            break;
        case GNSS_SUPL_MODE_MSB:
            pbGnssSuplMode = PB_GNSS_SUPL_MODE_MSB;
            break;
        case GNSS_SUPL_MODE_MSA:
            pbGnssSuplMode = PB_GNSS_SUPL_MODE_MSA;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssSuplMode:%d, pbGnssSuplMode:%d", gnssSuplMode, pbGnssSuplMode);
    return pbGnssSuplMode;
}

PBFixQualityLevel LocationApiPbMsgConv::getPBEnumForFixQualityLevel(
        const FixQualityLevel &qualityLevel) const {
    PBFixQualityLevel pbQualityLevel = PB_QUALITY_HIGH_ACCU_FIX_ONLY;
    switch (qualityLevel) {
        case QUALITY_ANY_VALID_FIX:
            pbQualityLevel = PB_QUALITY_ANY_VALID_FIX;
            break;
        case QUALITY_ANY_OR_FAILED_FIX:
            pbQualityLevel = PB_QUALITY_ANY_OR_FAILED_FIX;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: qualityLevel:%d, pbQualityLevel:%d", qualityLevel, pbQualityLevel);
    return pbQualityLevel;
}

PBBatchingStatus LocationApiPbMsgConv::getPBEnumForBatchingStatus(
        const BatchingStatus &batchStatus) const {
    PBBatchingStatus pbBatchStatus = PB_BATCHING_STATUS_POSITION_UNAVAILABLE;
    switch (batchStatus) {
        case BATCHING_STATUS_TRIP_COMPLETED:
            pbBatchStatus = PB_BATCHING_STATUS_TRIP_COMPLETED;
            break;
        case BATCHING_STATUS_POSITION_AVAILABE:
            pbBatchStatus = PB_BATCHING_STATUS_POSITION_AVAILABE;
            break;
        case BATCHING_STATUS_POSITION_UNAVAILABLE:
            pbBatchStatus = PB_BATCHING_STATUS_POSITION_UNAVAILABLE;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: batchStatus:%d, pbGnssSuplMode:%d", batchStatus, pbBatchStatus);
    return pbBatchStatus;
}

PBLocationReliability LocationApiPbMsgConv::getPBEnumForLocationReliability(
        const LocationReliability &locReliab) const {
    PBLocationReliability pbLocReliab = PB_LOCATION_RELIABILITY_NOT_SET;
    switch (locReliab) {
        case LOCATION_RELIABILITY_NOT_SET:
            pbLocReliab = PB_LOCATION_RELIABILITY_NOT_SET;
            break;
        case LOCATION_RELIABILITY_VERY_LOW:
            pbLocReliab = PB_LOCATION_RELIABILITY_VERY_LOW;
            break;
        case LOCATION_RELIABILITY_LOW:
            pbLocReliab = PB_LOCATION_RELIABILITY_LOW;
            break;
        case LOCATION_RELIABILITY_MEDIUM:
            pbLocReliab = PB_LOCATION_RELIABILITY_MEDIUM;
            break;
        case LOCATION_RELIABILITY_HIGH:
            pbLocReliab = PB_LOCATION_RELIABILITY_HIGH;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: locReliab:%d, pbLocReliab:%d", locReliab, pbLocReliab);
    return pbLocReliab;
}

PBLocApiOutputEngineType LocationApiPbMsgConv::getPBEnumForLocOutputEngineType(
        const LocOutputEngineType &locOpEng) const {
    PBLocApiOutputEngineType pbLocOpEng = PB_LOC_OUTPUT_ENGINE_FUSED;
    switch (locOpEng) {
        case LOC_OUTPUT_ENGINE_FUSED:
            pbLocOpEng = PB_LOC_OUTPUT_ENGINE_FUSED;
            break;
        case LOC_OUTPUT_ENGINE_SPE:
            pbLocOpEng = PB_LOC_OUTPUT_ENGINE_SPE;
            break;
        case LOC_OUTPUT_ENGINE_PPE:
            pbLocOpEng = PB_LOC_OUTPUT_ENGINE_PPE;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: locOpEng:%d, pbLocOpEng:%d", locOpEng, pbLocOpEng);
    return pbLocOpEng;
}

PBGnssMeasurementsMultipathIndicator LocationApiPbMsgConv::getPBEnumForGnssMeasMultiPathIndic(
            const GnssMeasurementsMultipathIndicator &gnssMeasMultiPathIndic) const {
    PBGnssMeasurementsMultipathIndicator pbGnssMeasMultiPathIndic =
            PB_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN;
    switch (gnssMeasMultiPathIndic) {
        case GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN:
            pbGnssMeasMultiPathIndic = PB_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN;
            break;
        case GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT:
            pbGnssMeasMultiPathIndic = PB_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT;
            break;
        case GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT:
            pbGnssMeasMultiPathIndic = PB_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssMeasMultiPathIndic:%d, pbGnssMeasMultiPathIndic:%d",
            gnssMeasMultiPathIndic, pbGnssMeasMultiPathIndic);
    return pbGnssMeasMultiPathIndic;
}

PBLocApiGnss_LocSvSystemEnumType LocationApiPbMsgConv::getPBEnumForGnssLocSvSystem(
            const Gnss_LocSvSystemEnumType &gnssLocSvSysEnumType) const {
    PBLocApiGnss_LocSvSystemEnumType pbGnssLocSvSysEnumType = PB_GNSS_LOC_SV_SYSTEM_INVALID;
    switch (gnssLocSvSysEnumType) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            pbGnssLocSvSysEnumType = PB_GNSS_LOC_SV_SYSTEM_GPS;
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            pbGnssLocSvSysEnumType = PB_GNSS_LOC_SV_SYSTEM_GALILEO;
            break;
        case GNSS_LOC_SV_SYSTEM_SBAS:
            pbGnssLocSvSysEnumType = PB_GNSS_LOC_SV_SYSTEM_SBAS;
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            pbGnssLocSvSysEnumType = PB_GNSS_LOC_SV_SYSTEM_GLONASS;
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            pbGnssLocSvSysEnumType = PB_GNSS_LOC_SV_SYSTEM_BDS;
            break;
        case GNSS_LOC_SV_SYSTEM_QZSS:
            pbGnssLocSvSysEnumType = PB_GNSS_LOC_SV_SYSTEM_QZSS;
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            pbGnssLocSvSysEnumType = PB_GNSS_LOC_SV_SYSTEM_NAVIC;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssLocSvSysEnumType:%d, pbGnssLocSvSysEnumType:%d",
            gnssLocSvSysEnumType, pbGnssLocSvSysEnumType);
    return pbGnssLocSvSysEnumType;
}

PBAgcStatus LocationApiPbMsgConv::getPBEnumForAgcStatus(const AgcStatus &agcStatus) const {
     PBAgcStatus pbAgcStatus = PB_AGC_STATUS_UNKNOWN;
     switch (agcStatus) {
        case AGC_STATUS_NO_SATURATION:
            pbAgcStatus = PB_AGC_STATUS_NO_SATURATION;
            break;
        case AGC_STATUS_FRONT_END_GAIN_MAXIMUM_SATURATION:
            pbAgcStatus = PB_AGC_STATUS_FRONT_END_GAIN_MAXIMUM_SATURATION;
            break;
        case AGC_STATUS_FRONT_END_GAIN_MINIMUM_SATURATION:
            pbAgcStatus = PB_AGC_STATUS_FRONT_END_GAIN_MINIMUM_SATURATION;
            break;
        default:
            break;
     }
    LocApiPb_LOGv("LocApiPB: agcStatus:%x, pbAgcStatus:%x", agcStatus, pbAgcStatus);
    return pbAgcStatus;
}

// GnssSvType to PBLocApiGnss_LocSvSystemEnumType
PBLocApiGnss_LocSvSystemEnumType LocationApiPbMsgConv::getPBGnssLocSvSysEnumFromGnssSvType(
        const GnssSvType &gnssSvType) const {
    PBLocApiGnss_LocSvSystemEnumType pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_INVALID;
    switch (gnssSvType) {
        case GNSS_SV_TYPE_UNKNOWN:
            pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_INVALID;
            break;
        case GNSS_SV_TYPE_GPS:
            pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_GPS;
            break;
        case GNSS_SV_TYPE_SBAS:
            pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_SBAS;
            break;
        case GNSS_SV_TYPE_GLONASS:
            pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_GLONASS;
            break;
        case GNSS_SV_TYPE_QZSS:
            pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_QZSS;
            break;
        case GNSS_SV_TYPE_BEIDOU:
            pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_BDS;
            break;
        case GNSS_SV_TYPE_GALILEO:
            pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_GALILEO;
            break;
        case GNSS_SV_TYPE_NAVIC:
            pbGnssLocSvSysEnum = PB_GNSS_LOC_SV_SYSTEM_NAVIC;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssSvType:%d, pbGnssLocSvSysEnum:%d", gnssSvType,
            pbGnssLocSvSysEnum);
    return pbGnssLocSvSysEnum;
}

// **** helper function for mask conversion to protobuf masks
uint32_t LocationApiPbMsgConv::getPBMaskForLocationCallbacksMask(const uint32_t &locCbMask) const {
    uint32_t pbLocCbMask = 0;
    if (locCbMask & E_LOC_CB_TRACKING_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_TRACKING_BIT;
    }
    if (locCbMask & E_LOC_CB_GNSS_LOCATION_INFO_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GNSS_LOCATION_INFO_BIT;
    }
    if (locCbMask & E_LOC_CB_GNSS_SV_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GNSS_SV_BIT;
    }
    if (locCbMask & E_LOC_CB_GNSS_NMEA_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GNSS_NMEA_BIT;
    }
    if (locCbMask & E_LOC_CB_GNSS_DATA_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GNSS_DATA_BIT;
    }
    if (locCbMask & E_LOC_CB_SYSTEM_INFO_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_SYSTEM_INFO_BIT;
    }
    if (locCbMask & E_LOC_CB_BATCHING_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_BATCHING_BIT;
    }
    if (locCbMask & E_LOC_CB_BATCHING_STATUS_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_BATCHING_STATUS_BIT;
    }
    if (locCbMask & E_LOC_CB_GEOFENCE_BREACH_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GEOFENCE_BREACH_BIT;
    }
    if (locCbMask & E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT;
    }
    if (locCbMask & E_LOC_CB_SIMPLE_LOCATION_INFO_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_SIMPLE_LOCATION_INFO_BIT;
    }
    if (locCbMask & E_LOC_CB_GNSS_MEAS_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GNSS_MEAS_BIT;
    }
    if (locCbMask & E_LOC_CB_GNSS_NHZ_MEAS_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GNSS_NHZ_MEAS_BIT;
    }
    if (locCbMask & E_LOC_CB_GNSS_DC_REPORT_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GNSS_DC_REPORT_BIT;
    }
    if (locCbMask & E_LOC_CB_ENGINE_NMEA_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_ENGINE_NMEA_BIT;
    }
    if (locCbMask & E_LOC_CB_GNSS_EPH_BIT) {
        pbLocCbMask |= PB_E_LOC_CB_GNSS_EPH_BIT;
    }
    LocApiPb_LOGv("LocApiPB: locCbMask:%x, pbLocCbMask:%x", locCbMask, pbLocCbMask);
    return pbLocCbMask;
}

uint64_t LocationApiPbMsgConv::getPBMaskForLocationCapabilitiesMask(
        const uint64_t &locCapabMask) const {
    uint64_t pbLocCapabMask = 0;
    if (locCapabMask & LOCATION_CAPABILITIES_TIME_BASED_TRACKING_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_TIME_BASED_TRACKING_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_TIME_BASED_BATCHING_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_TIME_BASED_BATCHING_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_DISTANCE_BASED_TRACKING_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_DISTANCE_BASED_TRACKING_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_DISTANCE_BASED_BATCHING_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_DISTANCE_BASED_BATCHING_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_GEOFENCE_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_GEOFENCE_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_OUTDOOR_TRIP_BATCHING_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_OUTDOOR_TRIP_BATCHING_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_GNSS_MEASUREMENTS_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_CONSTELLATION_ENABLEMENT_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_CONSTELLATION_ENABLEMENT_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_CARRIER_PHASE_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_CARRIER_PHASE_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_SV_POLYNOMIAL_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_SV_POLYNOMIAL_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_GNSS_SINGLE_FREQUENCY) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_GNSS_SINGLE_FREQUENCY;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_GNSS_MULTI_FREQUENCY) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_GNSS_MULTI_FREQUENCY;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_VPE) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_VPE;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_BASIC) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_CV2X_LOCATION_BASIC;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_PREMIUM) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_CV2X_LOCATION_PREMIUM;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_PPE) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_PPE;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_QDR2) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_QDR2;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_QDR3) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_QDR3;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_DGNSS) {
        pbLocCapabMask |= PB_LOCATION_CAPS_QWES_DGNSS;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_ANTENNA_INFO) {
        pbLocCapabMask |= PB_LOCATION_CAPS_ANTENNA_INFO;
        LOC_LOGi("PB_LOCATION_CAPS_ANTENNA_INFO");
    }
    if (locCapabMask & LOCATION_CAPABILITIES_GNSS_MSB_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_GNSS_MSB_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_GNSS_MSA_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_GNSS_MSA_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_DEBUG_DATA_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_DEBUG_DATA_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_AGPM_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_AGPM_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_PRIVACY_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_PRIVACY_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_MEASUREMENTS_CORRECTION_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_MEASUREMENTS_CORRECTION_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_CONFORMITY_INDEX_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_CONFORMITY_INDEX_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_EDGNSS_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_EDGNSS_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_QWES_SV_EPHEMERIS_BIT) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_QWES_SV_EPHEMERIS_BIT;
    }
    if (locCapabMask & LOCATION_CAPABILITIES_NLOS_ML20) {
        pbLocCapabMask |= PB_LOCATION_CAPABILITIES_QWES_NLOS_ML20;
    }
    LOC_LOGi("LocApiPB: locCapabMask:0x%" PRIx64", pbLocCapabMask:0x%" PRIx64,
            locCapabMask, pbLocCapabMask);
    return pbLocCapabMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForPositioningEngineMask(
        const uint32_t &posEngMask) const {
    uint32_t pbPosEngMask = 0;
    if (posEngMask & STANDARD_POSITIONING_ENGINE) {
        pbPosEngMask |= PB_STANDARD_POSITIONING_ENGINE;
    }
    if (posEngMask & DEAD_RECKONING_ENGINE) {
        pbPosEngMask |= PB_DEAD_RECKONING_ENGINE;
    }
    if (posEngMask & PRECISE_POSITIONING_ENGINE) {
        pbPosEngMask |= PB_PRECISE_POSITIONING_ENGINE;
    }
    LocApiPb_LOGv("LocApiPB: posEngMask:%x, pbPosEngMask:%x", posEngMask, pbPosEngMask);
    return pbPosEngMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForLeverArmTypeMask(
        const uint32_t &leverArmTypeMask) const {
    uint32_t pbLeverArmTypeMask = 0;
    if (leverArmTypeMask & LEVER_ARM_TYPE_GNSS_TO_VRP_BIT) {
        pbLeverArmTypeMask |= PB_LEVER_ARM_TYPE_GNSS_TO_VRP_BIT;
    }
    if (leverArmTypeMask & LEVER_ARM_TYPE_DR_IMU_TO_GNSS_BIT) {
        pbLeverArmTypeMask |= PB_LEVER_ARM_TYPE_DR_IMU_TO_GNSS_BIT;
    }
    if (leverArmTypeMask & LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS_BIT) {
        pbLeverArmTypeMask |= PB_LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS_BIT;
    }
    LocApiPb_LOGv("LocApiPB: leverArmTypeMask:%x, pbLeverArmTypeMask:%x",
            leverArmTypeMask, pbLeverArmTypeMask);
    return pbLeverArmTypeMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForLocReqEngineTypeMask(
        const uint32_t &locReqEngTypeMask) const {
    uint32_t pbLocReqEngTypeMask = 0;
    if (locReqEngTypeMask & LOC_REQ_ENGINE_FUSED_BIT) {
        pbLocReqEngTypeMask |= PB_LOC_REQ_ENGINE_FUSED_BIT;
    }
    if (locReqEngTypeMask & LOC_REQ_ENGINE_SPE_BIT) {
        pbLocReqEngTypeMask |= PB_LOC_REQ_ENGINE_SPE_BIT;
    }
    if (locReqEngTypeMask & LOC_REQ_ENGINE_PPE_BIT) {
        pbLocReqEngTypeMask |= PB_LOC_REQ_ENGINE_PPE_BIT;
    }
    LocApiPb_LOGv("LocApiPB: locReqEngTypeMask:%x, pbLocReqEngTypeMask:%x",
            locReqEngTypeMask, pbLocReqEngTypeMask);
    return pbLocReqEngTypeMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssCfgRobustLocValidMask(
        const uint32_t &gnssCfgRobustLocValidMask) const {
    uint32_t pbGnssCfgRobustLocValidMask = 0;
    if (gnssCfgRobustLocValidMask & GNSS_CONFIG_ROBUST_LOCATION_ENABLED_VALID_BIT) {
        pbGnssCfgRobustLocValidMask |= PB_GNSS_CONFIG_ROBUST_LOCATION_ENABLED_VALID_BIT;
    }
    if (gnssCfgRobustLocValidMask & GNSS_CONFIG_ROBUST_LOCATION_ENABLED_FOR_E911_VALID_BIT) {
        pbGnssCfgRobustLocValidMask |= PB_GNSS_CONFIG_ROBUST_LOCATION_ENABLED_FOR_E911_VALID_BIT;
    }
    if (gnssCfgRobustLocValidMask & GNSS_CONFIG_ROBUST_LOCATION_VERSION_VALID_BIT) {
        pbGnssCfgRobustLocValidMask |= PB_GNSS_CONFIG_ROBUST_LOCATION_VERSION_VALID_BIT;
    }
    LocApiPb_LOGv("LocApiPB: gnssCfgRobustLocValidMask:%x, pbGnssCfgRobustLocValidMask:%x",
            gnssCfgRobustLocValidMask, pbGnssCfgRobustLocValidMask);
    return pbGnssCfgRobustLocValidMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForLocSysInfoMask(const uint32_t &locSysInfoMask) const {
    uint32_t pbLocSysInfoMask = 0;
    if (locSysInfoMask & LOCATION_SYS_INFO_LEAP_SECOND) {
        pbLocSysInfoMask |= PB_LOCATION_SYS_INFO_LEAP_SECOND;
    }
    LocApiPb_LOGv("LocApiPB: locSysInfoMask:%x, pbLocSysInfoMask:%x", locSysInfoMask,
            pbLocSysInfoMask);
    return pbLocSysInfoMask;
}

uint64_t LocationApiPbMsgConv::getPBMaskForGnssDataMask(const uint64_t &gnssDataMask) const {
    uint64_t pbGnssDataMask = 0;
    if (gnssDataMask & GNSS_LOC_DATA_JAMMER_IND_BIT) {
        pbGnssDataMask |= PB_GNSS_LOC_DATA_JAMMER_IND_BIT;
    }
    if (gnssDataMask & GNSS_LOC_DATA_AGC_BIT) {
        pbGnssDataMask |= PB_GNSS_LOC_DATA_AGC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: locSysInfoMask:%" PRIu64", pbGnssDataMask:%" PRIu64,
            gnssDataMask, pbGnssDataMask);
    return pbGnssDataMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssAidingDataSvMask(
        const uint32_t &gnssAidDataSvMask) const {
    uint32_t pbGnssAidDataSvMask = 0;
    if (gnssAidDataSvMask & GNSS_AIDING_DATA_SV_EPHEMERIS_BIT) {
        pbGnssAidDataSvMask |= PB_AIDING_DATA_SV_EPHEMERIS_BIT;
    }
    LocApiPb_LOGv("LocApiPB: gnssAidDataSvMask:%x, pbGnssAidDataSvMask:%x",
            gnssAidDataSvMask, pbGnssAidDataSvMask);
    return pbGnssAidDataSvMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForLeapSecondSysInfoMask(
        const uint32_t &leapSecSysInfoMask) const {
    uint32_t pbLeapSecSysInfoMask = 0;
    if (leapSecSysInfoMask & LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT) {
        pbLeapSecSysInfoMask |= PB_LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT;
    }
    if (leapSecSysInfoMask & LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT) {
        pbLeapSecSysInfoMask |= PB_LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT;
    }

    LocApiPb_LOGv("LocApiPB: leapSecSysInfoMask:%x, pbLeapSecSysInfoMask:%x",
            leapSecSysInfoMask, pbLeapSecSysInfoMask);
    return pbLeapSecSysInfoMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssSystemTimeStructTypeFlags(
        const uint32_t &gnssSysTimeStructTypeFlg) const {
    uint32_t pbGnssSysTimeStructTypeFlg = 0;
    if (gnssSysTimeStructTypeFlg & GNSS_SYSTEM_TIME_WEEK_VALID) {
        pbGnssSysTimeStructTypeFlg |= PB_GNSS_SYSTEM_TIME_WEEK_VALID;
    }
    if (gnssSysTimeStructTypeFlg & GNSS_SYSTEM_TIME_WEEK_MS_VALID) {
        pbGnssSysTimeStructTypeFlg |= PB_GNSS_SYSTEM_TIME_WEEK_MS_VALID;
    }
    if (gnssSysTimeStructTypeFlg & GNSS_SYSTEM_CLK_TIME_BIAS_VALID) {
        pbGnssSysTimeStructTypeFlg |= PB_GNSS_SYSTEM_CLK_TIME_BIAS_VALID;
    }
    if (gnssSysTimeStructTypeFlg & GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID) {
        pbGnssSysTimeStructTypeFlg |= PB_GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID;
    }
    if (gnssSysTimeStructTypeFlg & GNSS_SYSTEM_REF_FCOUNT_VALID) {
        pbGnssSysTimeStructTypeFlg |= PB_GNSS_SYSTEM_REF_FCOUNT_VALID;
    }
    if (gnssSysTimeStructTypeFlg & GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID) {
        pbGnssSysTimeStructTypeFlg |= PB_GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID;
    }
    LocApiPb_LOGv("LocApiPB: gnssSysTimeStructTypeFlg:%x, pbGnssSysTimeStructTypeFlg:%x",
            gnssSysTimeStructTypeFlg, pbGnssSysTimeStructTypeFlg);
    return pbGnssSysTimeStructTypeFlg;
}

uint32_t LocationApiPbMsgConv::getPBMaskForLocationFlagsMask(const uint32_t &locFlagsMask) const {
    uint32_t pbLocFlagsMask = 0;
    if (locFlagsMask & LOCATION_HAS_LAT_LONG_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_LAT_LONG_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_ALTITUDE_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_ALTITUDE_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_SPEED_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_SPEED_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_BEARING_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_BEARING_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_ACCURACY_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_ACCURACY_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_VERTICAL_ACCURACY_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_VERTICAL_ACCURACY_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_SPEED_ACCURACY_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_SPEED_ACCURACY_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_BEARING_ACCURACY_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_BEARING_ACCURACY_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_ELAPSED_REAL_TIME_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_ELAPSED_REAL_TIME_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_TIME_UNC_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_TIME_UNC_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_GPTP_TIME_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_GPTP_TIME_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_GPTP_TIME_UNC_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_GPTP_TIME_UNC_BIT;
    }
    if (locFlagsMask & LOCATION_HAS_SESSION_STATUS_BIT) {
        pbLocFlagsMask |= PB_LOCATION_HAS_SESSION_STATUS_BIT;
    }

    LocApiPb_LOGv("LocApiPB: locFlagsMask:%x, pbLocFlagsMask:%x", locFlagsMask, pbLocFlagsMask);
    return pbLocFlagsMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForLocationTechnologyMask(
        const uint32_t &locTechMask) const {
    uint32_t pbLocTechMask = 0;
    if (locTechMask & LOCATION_TECHNOLOGY_GNSS_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_GNSS_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_CELL_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_CELL_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_WIFI_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_WIFI_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_SENSORS_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_SENSORS_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_AFLT_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_AFLT_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_HYBRID_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_HYBRID_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_PPE_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_PPE_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_VEH_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_VEH_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_VIS_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_VIS_BIT;
    }
    if (locTechMask & LOCATION_TECHNOLOGY_PROPAGATED_BIT) {
        pbLocTechMask |= PB_LOCATION_TECHNOLOGY_PROPAGATED_BIT;
    }

    LocApiPb_LOGv("LocApiPB: locTechMask:%x, pbLocTechMask:%x", locTechMask, pbLocTechMask);
    return pbLocTechMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssLocationInfoFlagMask(
        const uint64_t &gnssLocInfoFlagMask) const {
    uint32_t pbGnssLocInfoFlagMask = 0;
    // (1ULL<<0) to (1ULL<<31)
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_DOP_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_DOP_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_VER_RELIABILITY_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_VER_RELIABILITY_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT;
    }
    // LDT_GNSS_LOCATION_INFO_SV_SOURCE_INFO_BIT field is deprecated.
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_EXT_DOP_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_EXT_DOP_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_EAST_STD_DEV_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_EAST_STD_DEV_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_NORTH_VEL_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_NORTH_VEL_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_EAST_VEL_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_EAST_VEL_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_UP_VEL_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_UP_VEL_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_UP_VEL_UNC_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_UP_VEL_UNC_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_LEAP_SECONDS_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_LEAP_SECONDS_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_PERCENT_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_CONFORMITY_INDEX_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_CONFORMITY_INDEX_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_LLA_VRP_BASED_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_LLA_VRP_BASED_BIT;
    }
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_ENU_VELOCITY_VRP_BASED_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_ENU_VELOCITY_VRP_BASED_BIT;
    }
    LocApiPb_LOGv("LocApiPB: gnssLocInfoFlagMask:%" PRIx64", pbGnssLocInfoFlagMask:%x",
            gnssLocInfoFlagMask, pbGnssLocInfoFlagMask);
    return pbGnssLocInfoFlagMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssLocationInfoExtFlagMask(
        const uint64_t &gnssLocInfoFlagMask) const {

    uint32_t pbGnssLocInfoFlagMask = 0;
    // (1ULL<<32) and onwards
    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_DR_SOLUTION_STATUS_MASK_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_DR_SOLUTION_STATUS_MASK_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_ALTITUDE_ASSUMED_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_ALTITUDE_ASSUMED_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_SESSION_STATUS_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_SESSION_STATUS_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_INTEGRITY_RISK_USED_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_INTEGRITY_RISK_USED_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_PROTECT_ALONG_TRACK_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_PROTECT_ALONG_TRACK_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_PROTECT_CROSS_TRACK_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_PROTECT_CROSS_TRACK_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_PROTECT_VERTICAL_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_PROTECT_VERTICAL_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_DGNSS_STATION_ID_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_DGNSS_STATION_ID_MASK_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_BASE_LINE_LENGTH_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_BASE_LINE_LENGTH_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_AGE_OF_CORRECTION_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_AGE_OF_CORRECTION_BIT;
    }

    if (gnssLocInfoFlagMask & LDT_GNSS_LOCATION_INFO_LEAP_SECONDS_UNC_BIT) {
        pbGnssLocInfoFlagMask |= PB_GNSS_LOCATION_INFO_LEAP_SECONDS_UNC_BIT;
    }

    return pbGnssLocInfoFlagMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssLocationNavSolutionMask(
        const uint32_t &gnssLocNavSolnMask) const {
    uint32_t pbGnssLocNavSolnMask = 0;
    if (gnssLocNavSolnMask & LOCATION_SBAS_CORRECTION_IONO_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_SBAS_CORRECTION_IONO_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_SBAS_CORRECTION_FAST_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_SBAS_CORRECTION_FAST_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_SBAS_CORRECTION_LONG_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_SBAS_CORRECTION_LONG_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_SBAS_INTEGRITY_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_SBAS_INTEGRITY_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_NAV_CORRECTION_DGNSS_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_NAV_CORRECTION_DGNSS_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_NAV_CORRECTION_RTK_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_NAV_CORRECTION_RTK_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_NAV_CORRECTION_PPP_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_NAV_CORRECTION_PPP_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_NAV_CORRECTION_RTK_FIXED_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_NAV_CORRECTION_RTK_FIXED_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_NAV_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_BIT) {
        pbGnssLocNavSolnMask |= PB_LOCATION_NAV_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_BIT;
    }
    if (gnssLocNavSolnMask & LOCATION_NAV_MMF_AIDED_POSITION) {
        pbGnssLocNavSolnMask |= PB_LOCATION_NAV_MMF_AIDED_POSITION;
    }

    LocApiPb_LOGv("LocApiPB: gnssLocNavSolnMask:%x, pbGnssLocNavSolnMask:%x",
            gnssLocNavSolnMask, pbGnssLocNavSolnMask);
    return pbGnssLocNavSolnMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForDrCalibrationStatusMask(
        const uint32_t &drCalibStatusMask) const {
    uint32_t pbDrCalibStatusMask = 0;
    if (drCalibStatusMask & DR_ROLL_CALIBRATION_NEEDED) {
        pbDrCalibStatusMask |= PB_DR_ROLL_CALIBRATION_NEEDED;
    }
    if (drCalibStatusMask & DR_PITCH_CALIBRATION_NEEDED) {
        pbDrCalibStatusMask |= PB_DR_PITCH_CALIBRATION_NEEDED;
    }
    if (drCalibStatusMask & DR_YAW_CALIBRATION_NEEDED) {
        pbDrCalibStatusMask |= PB_DR_YAW_CALIBRATION_NEEDED;
    }
    if (drCalibStatusMask & DR_ODO_CALIBRATION_NEEDED) {
        pbDrCalibStatusMask |= PB_DR_ODO_CALIBRATION_NEEDED;
    }
    if (drCalibStatusMask & DR_GYRO_CALIBRATION_NEEDED) {
        pbDrCalibStatusMask |= PB_DR_GYRO_CALIBRATION_NEEDED;
    }
    if (drCalibStatusMask & DR_TURN_CALIBRATION_LOW) {
        pbDrCalibStatusMask |= PB_DR_TURN_CALIBRATION_LOW;
    }
    if (drCalibStatusMask & DR_TURN_CALIBRATION_MEDIUM) {
        pbDrCalibStatusMask |= PB_DR_TURN_CALIBRATION_MEDIUM;
    }
    if (drCalibStatusMask & DR_TURN_CALIBRATION_HIGH) {
        pbDrCalibStatusMask |= PB_DR_TURN_CALIBRATION_HIGH;
    }
    if (drCalibStatusMask & DR_LINEAR_ACCEL_CALIBRATION_LOW) {
        pbDrCalibStatusMask |= PB_DR_LINEAR_ACCEL_CALIBRATION_LOW;
    }
    if (drCalibStatusMask & DR_LINEAR_ACCEL_CALIBRATION_MEDIUM) {
        pbDrCalibStatusMask |= PB_DR_LINEAR_ACCEL_CALIBRATION_MEDIUM;
    }
    if (drCalibStatusMask & DR_LINEAR_ACCEL_CALIBRATION_HIGH) {
        pbDrCalibStatusMask |= PB_DR_LINEAR_ACCEL_CALIBRATION_HIGH;
    }
    if (drCalibStatusMask & DR_LINEAR_MOTION_CALIBRATION_LOW) {
        pbDrCalibStatusMask |= PB_DR_LINEAR_MOTION_CALIBRATION_LOW;
    }
    if (drCalibStatusMask & DR_LINEAR_MOTION_CALIBRATION_MEDIUM) {
        pbDrCalibStatusMask |= PB_DR_LINEAR_MOTION_CALIBRATION_MEDIUM;
    }
    if (drCalibStatusMask & DR_LINEAR_MOTION_CALIBRATION_HIGH) {
        pbDrCalibStatusMask |= PB_DR_LINEAR_MOTION_CALIBRATION_HIGH;
    }
    if (drCalibStatusMask & DR_STATIC_CALIBRATION_LOW) {
        pbDrCalibStatusMask |= PB_DR_STATIC_CALIBRATION_LOW;
    }
    if (drCalibStatusMask & DR_STATIC_CALIBRATION_MEDIUM) {
        pbDrCalibStatusMask |= PB_DR_STATIC_CALIBRATION_MEDIUM;
    }
    if (drCalibStatusMask & DR_STATIC_CALIBRATION_HIGH) {
        pbDrCalibStatusMask |= PB_DR_STATIC_CALIBRATION_HIGH;
    }
    LocApiPb_LOGv("LocApiPB: drCalibStatusMask:%x, pbDrCalibStatusMask:%x",
            drCalibStatusMask, pbDrCalibStatusMask);
    return pbDrCalibStatusMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssMeasurementsDataFlagsMask(
        const uint32_t &gnssMeasDataFlagsMask) const {
    uint32_t pbGnssMeasDataFlagsMask = 0;
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_SV_ID_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_SV_ID_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_STATE_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_STATE_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_ADR_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_ADR_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT;
    }
    if (gnssMeasDataFlagsMask & GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT) {
        pbGnssMeasDataFlagsMask |= PB_GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT;
    }

    LocApiPb_LOGv("LocApiPB: gnssMeasDataFlagsMask:%x, pbGnssMeasDataFlagsMask:%x",
            gnssMeasDataFlagsMask, pbGnssMeasDataFlagsMask);
    return pbGnssMeasDataFlagsMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssMeasurementsStateMask(
        const uint32_t &gnssMeasStateMask) const {
    uint32_t pbGnssMeasStateMask = 0;
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_UNKNOWN_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_UNKNOWN_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_SUBFRAME_SYNC_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_SUBFRAME_SYNC_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_TOW_DECODED_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_TOW_DECODED_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_MSEC_AMBIGUOUS_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_MSEC_AMBIGUOUS_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_SYMBOL_SYNC_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_SYMBOL_SYNC_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_GLO_STRING_SYNC_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_GLO_STRING_SYNC_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_GLO_TOD_DECODED_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_GLO_TOD_DECODED_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_BDS_D2_BIT_SYNC_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_BDS_D2_BIT_SYNC_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_BDS_D2_SUBFRAME_SYNC_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_BDS_D2_SUBFRAME_SYNC_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_GAL_E1BC_CODE_LOCK_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_GAL_E1BC_CODE_LOCK_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_GAL_E1C_2ND_CODE_LOCK_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_GAL_E1C_2ND_CODE_LOCK_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_GAL_E1B_PAGE_SYNC_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_GAL_E1B_PAGE_SYNC_BIT;
    }
    if (gnssMeasStateMask & GNSS_MEASUREMENTS_STATE_SBAS_SYNC_BIT) {
        pbGnssMeasStateMask |= PB_GNSS_MEASUREMENTS_STATE_SBAS_SYNC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: gnssMeasStateMask:%x, pbGnssMeasStateMask:%x",
            gnssMeasStateMask, pbGnssMeasStateMask);
    return pbGnssMeasStateMask;
}


uint32_t LocationApiPbMsgConv::getPBMaskForGnssMeasurementsAdrStateMask(
        const uint32_t &gnssMeasAdrStateMask) const {
    uint32_t pbGnssMeasAdrStateMask = 0;
    if (gnssMeasAdrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_UNKNOWN) {
        pbGnssMeasAdrStateMask |= PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_UNKNOWN;
    }
    if (gnssMeasAdrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT) {
        pbGnssMeasAdrStateMask |= PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT;
    }
    if (gnssMeasAdrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT) {
        pbGnssMeasAdrStateMask |= PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT;
    }
    if (gnssMeasAdrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT) {
        pbGnssMeasAdrStateMask |= PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT;
    }
    if (gnssMeasAdrStateMask &
            GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT) {
        pbGnssMeasAdrStateMask |=
            PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT;
    }
    LocApiPb_LOGv("LocApiPB: gnssMeasAdrStateMask:%x, pbGnssMeasAdrStateMask:%x",
            gnssMeasAdrStateMask, pbGnssMeasAdrStateMask);
    return pbGnssMeasAdrStateMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssLocationPosDataMask(
        const uint32_t &gnssLocPosDataMask, const uint32_t &gnssLocPosDataMaskExt) const {
    uint32_t pbGnssLocPosDataMask = 0;
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_YAW_RATE_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_YAW_RATE_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_PITCH_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_PITCH_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT;
    }
    if (gnssLocPosDataMask & LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT;
    }
    if (gnssLocPosDataMaskExt & LOCATION_NAV_DATA_HAS_PITCH_RATE_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_PITCH_RATE_BIT;
    }
    if (gnssLocPosDataMaskExt & LOCATION_NAV_DATA_HAS_PITCH_RATE_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_PITCH_RATE_UNC_BIT;
    }
    if (gnssLocPosDataMaskExt & LOCATION_NAV_DATA_HAS_ROLL_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_ROLL_BIT;
    }
    if (gnssLocPosDataMaskExt & LOCATION_NAV_DATA_HAS_ROLL_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_ROLL_UNC_BIT;
    }
    if (gnssLocPosDataMaskExt & LOCATION_NAV_DATA_HAS_ROLL_RATE_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_ROLL_RATE_BIT;
    }
    if (gnssLocPosDataMaskExt & LOCATION_NAV_DATA_HAS_ROLL_RATE_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_ROLL_RATE_UNC_BIT;
    }
    if (gnssLocPosDataMaskExt & LOCATION_NAV_DATA_HAS_YAW_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_YAW_BIT;
    }
    if (gnssLocPosDataMaskExt & LOCATION_NAV_DATA_HAS_YAW_UNC_BIT) {
        pbGnssLocPosDataMask |= PB_LOCATION_NAV_DATA_HAS_YAW_UNC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: Mask - gnssLocPosData:%x, gnssLocPosDataExt:%x, pbGnssLocPosData:%x",
            gnssLocPosDataMask, gnssLocPosDataMaskExt, pbGnssLocPosDataMask);
    return pbGnssLocPosDataMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssMeasurementsClockFlagsMask(
        const uint32_t &gnssMeasClockFlagsMask) const {
    uint32_t pbGnssMeasClockFlagsMask = 0;
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_UNCERTAINTY_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_UNCERTAINTY_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_HW_CLOCK_DISCONTINUITY_COUNT_BIT) {
        pbGnssMeasClockFlagsMask |=
                PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_HW_CLOCK_DISCONTINUITY_COUNT_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_UNC_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_UNC_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_BIT;
    }
    if (gnssMeasClockFlagsMask & GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_UNC_BIT) {
        pbGnssMeasClockFlagsMask |= PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_UNC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: gnssMeasClockFlagsMask:%x, pbGnssMeasClockFlagsMask:%x",
            gnssMeasClockFlagsMask, pbGnssMeasClockFlagsMask);
    return pbGnssMeasClockFlagsMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssGloTimeStructTypeFlags(
        const uint32_t &gnssGloTimeStructTypeFlags) const {
    uint32_t pbGnssGloTimeStructTypeFlags = 0;
    if (gnssGloTimeStructTypeFlags & GNSS_CLO_DAYS_VALID) {
        pbGnssGloTimeStructTypeFlags |= PB_GNSS_CLO_DAYS_VALID;
    }
    if (gnssGloTimeStructTypeFlags & GNSS_GLO_MSEC_VALID) {
        pbGnssGloTimeStructTypeFlags |= PB_GNSS_GLO_MSEC_VALID;
    }
    if (gnssGloTimeStructTypeFlags & GNSS_GLO_CLK_TIME_BIAS_VALID) {
        pbGnssGloTimeStructTypeFlags |= PB_GNSS_GLO_CLK_TIME_BIAS_VALID;
    }
    if (gnssGloTimeStructTypeFlags & GNSS_GLO_CLK_TIME_BIAS_UNC_VALID) {
        pbGnssGloTimeStructTypeFlags |= PB_GNSS_GLO_CLK_TIME_BIAS_UNC_VALID;
    }
    if (gnssGloTimeStructTypeFlags & GNSS_GLO_REF_FCOUNT_VALID) {
        pbGnssGloTimeStructTypeFlags |= PB_GNSS_GLO_REF_FCOUNT_VALID;
    }
    if (gnssGloTimeStructTypeFlags & GNSS_GLO_NUM_CLOCK_RESETS_VALID) {
        pbGnssGloTimeStructTypeFlags |= PB_GNSS_GLO_NUM_CLOCK_RESETS_VALID;
    }
    if (gnssGloTimeStructTypeFlags & GNSS_GLO_FOUR_YEAR_VALID) {
        pbGnssGloTimeStructTypeFlags |= PB_GNSS_GLO_FOUR_YEAR_VALID;
    }
    LocApiPb_LOGv("LocApiPB: gnssGloTimeStructTypeFlags:%x, pbGnssGloTimeStructTypeFlags:%x",
            gnssGloTimeStructTypeFlags, pbGnssGloTimeStructTypeFlags);
    return pbGnssGloTimeStructTypeFlags;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssSvOptionsMask(const uint32_t &gnssSvOptMask) const {
    uint32_t pbGnssSvOptMask = 0;
    if (gnssSvOptMask & GNSS_SV_OPTIONS_HAS_EPHEMER_BIT) {
        pbGnssSvOptMask |= PB_GNSS_SV_OPTIONS_HAS_EPHEMER_BIT;
    }
    if (gnssSvOptMask & GNSS_SV_OPTIONS_HAS_ALMANAC_BIT) {
        pbGnssSvOptMask |= PB_GNSS_SV_OPTIONS_HAS_ALMANAC_BIT;
    }
    if (gnssSvOptMask & GNSS_SV_OPTIONS_USED_IN_FIX_BIT) {
        pbGnssSvOptMask |= PB_GNSS_SV_OPTIONS_USED_IN_FIX_BIT;
    }
    if (gnssSvOptMask & GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT) {
        pbGnssSvOptMask |= PB_GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT;
    }
    if (gnssSvOptMask & GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT) {
        pbGnssSvOptMask |= PB_GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT;
    }
    if (gnssSvOptMask & GNSS_SV_OPTIONS_HAS_BASEBAND_CARRIER_TO_NOISE_BIT) {
        pbGnssSvOptMask |= PB_GNSS_SV_OPTIONS_HAS_BASEBAND_CARRIER_TO_NOISE_BIT;
    }
    if (gnssSvOptMask & GNSS_SV_OPTIONS_HAS_ELEVATION_BIT) {
        pbGnssSvOptMask |= PB_GNSS_SV_OPTIONS_HAS_ELEVATION_BIT;
    }
    if (gnssSvOptMask & GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT) {
        pbGnssSvOptMask |= PB_GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT;
    }
    LocApiPb_LOGv("LocApiPB: gnssSvOptMask:%x, pbGnssSvOptMask:%x", gnssSvOptMask,
            pbGnssSvOptMask);
    return pbGnssSvOptMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGnssSignalTypeMask(
        const uint32_t &gnssSignalTypeMask) const {
    uint32_t pbGnssSignalTypeMask = 0;
    if (gnssSignalTypeMask & GNSS_SIGNAL_GPS_L1CA) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GPS_L1CA_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_GPS_L1C) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GPS_L1C_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_GPS_L2) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GPS_L2_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_GPS_L5) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GPS_L5_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_GLONASS_G1) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GLONASS_G1_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_GLONASS_G2) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GLONASS_G2_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_GALILEO_E1) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GALILEO_E1_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_GALILEO_E5A) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GALILEO_E5A_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_GALILEO_E5B) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_GALILEO_E5B_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B1) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B1_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B2) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B2_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_QZSS_L1CA) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_QZSS_L1CA_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_QZSS_L1S) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_QZSS_L1S_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_QZSS_L2) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_QZSS_L2_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_QZSS_L5) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_QZSS_L5_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_SBAS_L1) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_SBAS_L1_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B1I) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B1I_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B1C) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B1C_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B2I) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B2I_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B2AI) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B2AI_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_NAVIC_L5) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_NAVIC_L5_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B2AQ) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B2AQ_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B2BI) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B2BI_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_BEIDOU_B2BQ) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_BEIDOU_B2BQ_BIT;
    }
    if (gnssSignalTypeMask & GNSS_SIGNAL_NAVIC_L1) {
        pbGnssSignalTypeMask |= PB_GNSS_SIGNAL_NAVIC_L1_BIT;
    }

    LocApiPb_LOGv("LocApiPB: gnssSignalTypeMask:%x, pbGnssSignalTypeMask:%x",
            gnssSignalTypeMask, pbGnssSignalTypeMask);
    return pbGnssSignalTypeMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForGeofenceBreachTypeMask(
        const uint32_t &gfBreachTypeMask) const {
    uint32_t pbGfBreachTypeMask = 0;
    if (gfBreachTypeMask & GEOFENCE_BREACH_ENTER_BIT) {
        pbGfBreachTypeMask |= PB_GEOFENCE_BREACH_ENTER_BIT;
    }
    if (gfBreachTypeMask & GEOFENCE_BREACH_EXIT_BIT) {
        pbGfBreachTypeMask |= PB_GEOFENCE_BREACH_EXIT_BIT;
    }
    if (gfBreachTypeMask & GEOFENCE_BREACH_DWELL_IN_BIT) {
        pbGfBreachTypeMask |= PB_GEOFENCE_BREACH_DWELL_IN_BIT;
    }
    if (gfBreachTypeMask & GEOFENCE_BREACH_DWELL_OUT_BIT) {
        pbGfBreachTypeMask |= PB_GEOFENCE_BREACH_DWELL_OUT_BIT;
    }
    LocApiPb_LOGv("LocApiPB: gfBreachTypeMask:%x, pbGfBreachTypeMask:%x",
            gfBreachTypeMask, pbGfBreachTypeMask);
    return pbGfBreachTypeMask;
}

// DeadReckoningEngineConfigValidMask to PBDeadReckoningEngineConfigValidMask
uint64_t LocationApiPbMsgConv::getPBMaskForDeadReckoningEngineConfigValidMask(
            const uint64_t &drEngCfgValidMask) const {
    uint64_t pbDrEngCfgVldMask = 0;
    if (drEngCfgValidMask & BODY_TO_SENSOR_MOUNT_PARAMS_BIT) {
        pbDrEngCfgVldMask |= PB_BODY_TO_SENSOR_MOUNT_PARAMS_BIT;
    }
    if (drEngCfgValidMask & VEHICLE_SPEED_SCALE_FACTOR_BIT) {
        pbDrEngCfgVldMask |= PB_VEHICLE_SPEED_SCALE_FACTOR_BIT;
    }
    if (drEngCfgValidMask & VEHICLE_SPEED_SCALE_FACTOR_UNC_BIT) {
        pbDrEngCfgVldMask |= PB_VEHICLE_SPEED_SCALE_FACTOR_UNC_BIT;
    }
    if (drEngCfgValidMask & GYRO_SCALE_FACTOR_BIT) {
        pbDrEngCfgVldMask |= PB_GYRO_SCALE_FACTOR_BIT;
    }
    if (drEngCfgValidMask & GYRO_SCALE_FACTOR_UNC_BIT) {
        pbDrEngCfgVldMask |= PB_GYRO_SCALE_FACTOR_UNC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: drEngCfgValidMask:%" PRIu64", pbDrEngCfgVldMask:%" PRIu64,
            drEngCfgValidMask, pbDrEngCfgVldMask);
    return pbDrEngCfgVldMask;
}


uint32_t LocationApiPbMsgConv::getPBMaskForDrEngineAidingDataMask(
        const uint32_t &drEngAidDataMask) const {
    uint32_t pbDrEngAidDataMask = 0;
    if (drEngAidDataMask & DR_ENGINE_AIDING_DATA_CALIBRATION_BIT) {
        pbDrEngAidDataMask |= PB_DR_ENGINE_AIDING_DATA_CALIBRATION_BIT;
    }
    LocApiPb_LOGv("LocApiPB: drEngAidDataMask:%x, pbDrEngAidDataMask:%x",
            drEngAidDataMask, pbDrEngAidDataMask);
    return pbDrEngAidDataMask;
}

uint32_t LocationApiPbMsgConv::getPBMaskForDrSolutionStatusMask(
        const uint32_t &drSolnStatusMask) const {
    uint32_t pbDrSolnStatusMask = 0;
    if (drSolnStatusMask & VEHICLE_SENSOR_SPEED_INPUT_DETECTED) {
        pbDrSolnStatusMask |= PB_VEHICLE_SENSOR_SPEED_INPUT_DETECTED;
    }
    if (drSolnStatusMask & VEHICLE_SENSOR_SPEED_INPUT_USED) {
        pbDrSolnStatusMask |= PB_VEHICLE_SENSOR_SPEED_INPUT_USED;
    }
    if (drSolnStatusMask & DRE_WARNING_UNCALIBRATED) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_UNCALIBRATED;
    }
    if (drSolnStatusMask & DRE_WARNING_GNSS_QUALITY_INSUFFICIENT) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_GNSS_QUALITY_INSUFFICIENT;
    }
    if (drSolnStatusMask & DRE_WARNING_FERRY_DETECTED ) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_FERRY_DETECTED;
    }
    if (drSolnStatusMask & DRE_ERROR_6DOF_SENSOR_UNAVAILABLE) {
        pbDrSolnStatusMask |= PB_DRE_ERROR_6DOF_SENSOR_UNAVAILABLE;
    }
    if (drSolnStatusMask & DRE_ERROR_VEHICLE_SPEED_UNAVAILABLE) {
        pbDrSolnStatusMask |= PB_DRE_ERROR_VEHICLE_SPEED_UNAVAILABLE;
    }
    if (drSolnStatusMask & DRE_ERROR_GNSS_EPH_UNAVAILABLE ) {
        pbDrSolnStatusMask |= PB_DRE_ERROR_GNSS_EPH_UNAVAILABLE;
    }
    if (drSolnStatusMask & DRE_ERROR_GNSS_MEAS_UNAVAILABLE) {
        pbDrSolnStatusMask |= PB_DRE_ERROR_GNSS_MEAS_UNAVAILABLE;
    }
    if (drSolnStatusMask & DRE_WARNING_INIT_POSITION_INVALID) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_INIT_POSITION_INVALID;
    }
    if (drSolnStatusMask & DRE_WARNING_INIT_POSITION_UNRELIABLE) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_INIT_POSITION_UNRELIABLE;
    }
    if (drSolnStatusMask & DRE_WARNING_POSITON_UNRELIABLE) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_POSITON_UNRELIABLE;
    }
    if (drSolnStatusMask & DRE_ERROR_GENERIC) {
        pbDrSolnStatusMask |= PB_DRE_ERROR_GENERIC;
    }
    if (drSolnStatusMask & DRE_WARNING_SENSOR_TEMP_OUT_OF_RANGE) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_SENSOR_TEMP_OUT_OF_RANGE;
    }
    if (drSolnStatusMask & DRE_WARNING_USER_DYNAMICS_INSUFFICIENT) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_USER_DYNAMICS_INSUFFICIENT;
    }
    if (drSolnStatusMask & DRE_WARNING_FACTORY_DATA_INCONSISTENT) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_FACTORY_DATA_INCONSISTENT;
    }
    if (drSolnStatusMask & DRE_WARNING_MMF_UNAVAILABLE) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_MMF_UNAVAILABLE;
    }
    if (drSolnStatusMask & DRE_WARNING_MMF_NOT_USABLE) {
        pbDrSolnStatusMask |= PB_DRE_WARNING_MMF_NOT_USABLE;
    }

    LocApiPb_LOGv("LocApiPB: drSolnStatusMask:%x, pbDrSolnStatusMask:%x",
            drSolnStatusMask, pbDrSolnStatusMask);
    return pbDrSolnStatusMask;
}

// **** helper function for mask conversion from protobuf masks to normal rigid values
uint64_t LocationApiPbMsgConv::getLocationCapabilitiesMaskFromPB(
        const uint64_t &pbLocCapabMask) const {
    uint64_t locCapabMask = 0;
    if (pbLocCapabMask & PB_LOCATION_CAPS_TIME_BASED_TRACKING_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_TIME_BASED_TRACKING_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_TIME_BASED_BATCHING_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_TIME_BASED_BATCHING_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_DISTANCE_BASED_TRACKING_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_DISTANCE_BASED_TRACKING_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_DISTANCE_BASED_BATCHING_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_DISTANCE_BASED_BATCHING_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_GEOFENCE_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_GEOFENCE_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_OUTDOOR_TRIP_BATCHING_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_OUTDOOR_TRIP_BATCHING_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_GNSS_MEASUREMENTS_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_CONSTELLATION_ENABLEMENT_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_CONSTELLATION_ENABLEMENT_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_CARRIER_PHASE_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_CARRIER_PHASE_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_SV_POLYNOMIAL_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_SV_POLYNOMIAL_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_GNSS_SINGLE_FREQUENCY) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_GNSS_SINGLE_FREQUENCY;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_GNSS_MULTI_FREQUENCY) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_GNSS_MULTI_FREQUENCY;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_VPE) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_VPE;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_CV2X_LOCATION_BASIC) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_BASIC;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_CV2X_LOCATION_PREMIUM) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_PREMIUM;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_PPE) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_PPE;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_QDR2) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_QDR2;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_QDR3) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_QDR3;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_QWES_DGNSS) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_DGNSS;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPS_ANTENNA_INFO) {
        locCapabMask |= LOCATION_CAPABILITIES_ANTENNA_INFO;
        LOC_LOGi("LOCATION_CAPABILITIES_ANTENNA_INFO");
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_GNSS_MSB_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_GNSS_MSB_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_GNSS_MSA_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_GNSS_MSA_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_DEBUG_DATA_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_DEBUG_DATA_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_AGPM_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_AGPM_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_PRIVACY_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_PRIVACY_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_MEASUREMENTS_CORRECTION_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_MEASUREMENTS_CORRECTION_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_CONFORMITY_INDEX_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_CONFORMITY_INDEX_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_EDGNSS_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_EDGNSS_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_QWES_SV_EPHEMERIS_BIT) {
        locCapabMask |= LOCATION_CAPABILITIES_QWES_SV_EPHEMERIS_BIT;
    }
    if (pbLocCapabMask & PB_LOCATION_CAPABILITIES_QWES_NLOS_ML20) {
        locCapabMask |= LOCATION_CAPABILITIES_NLOS_ML20;
    }
    LOC_LOGi("LocApiPB: pbLocCapabMask:0x%" PRIx64", locCapabMask:0x%" PRIx64,
            pbLocCapabMask, locCapabMask);
    return locCapabMask;
}

uint32_t LocationApiPbMsgConv::getLocationCallbacksMaskFromPB(const uint32_t &pbLocCbMask) const {
    uint32_t locCbMask = 0;
    if (pbLocCbMask & PB_E_LOC_CB_TRACKING_BIT) {
        locCbMask |= E_LOC_CB_TRACKING_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GNSS_LOCATION_INFO_BIT) {
        locCbMask |= E_LOC_CB_GNSS_LOCATION_INFO_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GNSS_SV_BIT) {
        locCbMask |= E_LOC_CB_GNSS_SV_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GNSS_NMEA_BIT) {
        locCbMask |= E_LOC_CB_GNSS_NMEA_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GNSS_DATA_BIT) {
        locCbMask |= E_LOC_CB_GNSS_DATA_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_SYSTEM_INFO_BIT) {
        locCbMask |= E_LOC_CB_SYSTEM_INFO_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_BATCHING_BIT) {
        locCbMask |= E_LOC_CB_BATCHING_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_BATCHING_STATUS_BIT) {
        locCbMask |= E_LOC_CB_BATCHING_STATUS_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GEOFENCE_BREACH_BIT) {
        locCbMask |= E_LOC_CB_GEOFENCE_BREACH_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT) {
        locCbMask |= E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_SIMPLE_LOCATION_INFO_BIT) {
        locCbMask |= E_LOC_CB_SIMPLE_LOCATION_INFO_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GNSS_MEAS_BIT) {
        locCbMask |= E_LOC_CB_GNSS_MEAS_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GNSS_NHZ_MEAS_BIT) {
        locCbMask |= E_LOC_CB_GNSS_NHZ_MEAS_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GNSS_DC_REPORT_BIT) {
        locCbMask |= E_LOC_CB_GNSS_DC_REPORT_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_ENGINE_NMEA_BIT) {
        locCbMask |= E_LOC_CB_ENGINE_NMEA_BIT;
    }
    if (pbLocCbMask & PB_E_LOC_CB_GNSS_EPH_BIT) {
        locCbMask |= E_LOC_CB_GNSS_EPH_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbLocCbMask:%x, locCbMask:%x", pbLocCbMask, locCbMask);
    return locCbMask;
}

uint32_t LocationApiPbMsgConv::getLeverArmTypeMaskFromPB(const uint32_t &pbLeverTypeMask) const {
    uint32_t leverYypeMask = 0;
    if (pbLeverTypeMask & PB_LEVER_ARM_TYPE_GNSS_TO_VRP_BIT) {
        leverYypeMask |= LEVER_ARM_TYPE_GNSS_TO_VRP_BIT;
    }
    if (pbLeverTypeMask & PB_LEVER_ARM_TYPE_DR_IMU_TO_GNSS_BIT) {
        leverYypeMask |= LEVER_ARM_TYPE_DR_IMU_TO_GNSS_BIT;
    }
    if (pbLeverTypeMask & PB_LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS_BIT) {
        leverYypeMask |= LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbLeverTypeMask:%x, leverYypeMask:%x", pbLeverTypeMask,
            leverYypeMask);
    return leverYypeMask;
}

uint32_t LocationApiPbMsgConv::getEnumForPBPositioningEngineMask(
        const uint32_t &pbPosEngMask) const {
    uint32_t posEngMask = 0;
    if (pbPosEngMask & PB_STANDARD_POSITIONING_ENGINE) {
        posEngMask |= STANDARD_POSITIONING_ENGINE;
    }
    if (pbPosEngMask & PB_PRECISE_POSITIONING_ENGINE) {
        posEngMask |= PRECISE_POSITIONING_ENGINE;
    }
    if (pbPosEngMask & PB_DEAD_RECKONING_ENGINE) {
        posEngMask |= DEAD_RECKONING_ENGINE;
    }
    LocApiPb_LOGv("LocApiPB: pbPosEngMask:%x, posEngMask:%x", pbPosEngMask, posEngMask);
    return posEngMask;
}

uint64_t LocationApiPbMsgConv::getGnssSvTypesMaskFromPB(const uint64_t &pbGnssSvTypesMask) const {
    uint64_t gnssSvTypesMask = 0;
    if (pbGnssSvTypesMask & PB_GNSS_SV_TYPES_MASK_GLO_BIT) {
        gnssSvTypesMask |= GNSS_SV_TYPES_MASK_GLO_BIT;
    }
    if (pbGnssSvTypesMask & PB_GNSS_SV_TYPES_MASK_BDS_BIT) {
        gnssSvTypesMask |= GNSS_SV_TYPES_MASK_BDS_BIT;
    }
    if (pbGnssSvTypesMask & PB_GNSS_SV_TYPES_MASK_QZSS_BIT) {
        gnssSvTypesMask |= GNSS_SV_TYPES_MASK_QZSS_BIT;
    }
    if (pbGnssSvTypesMask & PB_GNSS_SV_TYPES_MASK_GAL_BIT) {
        gnssSvTypesMask |= GNSS_SV_TYPES_MASK_GAL_BIT;
    }
    if (pbGnssSvTypesMask & PB_GNSS_SV_TYPES_MASK_NAVIC_BIT) {
        gnssSvTypesMask |= GNSS_SV_TYPES_MASK_NAVIC_BIT;
    }
    if (pbGnssSvTypesMask & PB_GNSS_SV_TYPES_MASK_GPS_BIT) {
        gnssSvTypesMask |= GNSS_SV_TYPES_MASK_GPS_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssSvTypesMask:%" PRIu64", gnssSvTypesMask:%" PRIu64,
            pbGnssSvTypesMask, gnssSvTypesMask);
    return gnssSvTypesMask;
}

uint32_t LocationApiPbMsgConv::getGnssAidingDataSvMaskFromPB(
        const uint32_t &pbGnssAidDataSvMask) const {
    uint32_t gnssAidDataSvMask = 0;
    if (pbGnssAidDataSvMask & PB_AIDING_DATA_SV_EPHEMERIS_BIT) {
        gnssAidDataSvMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssAidDataSvMask:%x, gnssAidDataSvMask:%x",
            pbGnssAidDataSvMask, gnssAidDataSvMask);
    return gnssAidDataSvMask;
}

uint32_t LocationApiPbMsgConv::getLocReqEngineTypeMaskFromPB(
        const uint32_t &pbLocReqEngTypeMask) const {
    uint32_t locReqEngTypeMask = 0;
    if (pbLocReqEngTypeMask & PB_LOC_REQ_ENGINE_FUSED_BIT) {
        locReqEngTypeMask |= LOC_REQ_ENGINE_FUSED_BIT;
    }
    if (pbLocReqEngTypeMask & PB_LOC_REQ_ENGINE_SPE_BIT) {
        locReqEngTypeMask |= LOC_REQ_ENGINE_SPE_BIT;
    }
    if (pbLocReqEngTypeMask & PB_LOC_REQ_ENGINE_PPE_BIT) {
        locReqEngTypeMask |= LOC_REQ_ENGINE_PPE_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbLocReqEngTypeMask:%x, locReqEngTypeMask:%x",
            pbLocReqEngTypeMask, locReqEngTypeMask);
    return locReqEngTypeMask;
}

uint32_t LocationApiPbMsgConv::getLocationSystemInfoMaskFromPB(
        const uint32_t &pbLocSysInfoMask) const {
    uint32_t locSysInfoMask = PB_LOCATION_SYS_INFO_INVALID;
    if (pbLocSysInfoMask & PB_LOCATION_SYS_INFO_LEAP_SECOND) {
        locSysInfoMask |= LOCATION_SYS_INFO_LEAP_SECOND;
    }
    LocApiPb_LOGv("LocApiPB: pbLocSysInfoMask:%x, locSysInfoMask:%x", pbLocSysInfoMask,
        locSysInfoMask);
    return locSysInfoMask;
}

uint64_t LocationApiPbMsgConv::getGnssDataMaskFromPB(const uint64_t &pbGnssDataMask) const {
    uint64_t gnssDataMask = 0;
    if (pbGnssDataMask & PB_GNSS_LOC_DATA_JAMMER_IND_BIT) {
        gnssDataMask |= GNSS_LOC_DATA_JAMMER_IND_BIT;
    }
    if (pbGnssDataMask & PB_GNSS_LOC_DATA_AGC_BIT) {
        gnssDataMask |= GNSS_LOC_DATA_AGC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssDataMask:%" PRIu64", gnssDataMask:%" PRIu64,
            pbGnssDataMask, gnssDataMask);
    return gnssDataMask;
}

uint32_t LocationApiPbMsgConv::getLeapSecSysInfoMaskFromPB(
        const uint32_t &pbLeapSecSysInfoMask) const {
    uint32_t leapSecSysInfoMask = 0;
    if (pbLeapSecSysInfoMask & PB_LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT) {
        leapSecSysInfoMask |= LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT;
    }
    if (pbLeapSecSysInfoMask & PB_LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT) {
        leapSecSysInfoMask |= LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbLeapSecSysInfoMask:%x, leapSecSysInfoMask:%x",
            pbLeapSecSysInfoMask, leapSecSysInfoMask);
    return leapSecSysInfoMask;
}

uint32_t LocationApiPbMsgConv::getGnssSystemTimeStructTypeFlagsFromPB(
        const uint32_t &pbGnssSysTimeStrctType) const {
    uint32_t gnssSysTimeStrctTypeMsk = 0;
    if (pbGnssSysTimeStrctType & PB_GNSS_SYSTEM_TIME_WEEK_VALID) {
        gnssSysTimeStrctTypeMsk |= GNSS_SYSTEM_TIME_WEEK_VALID;
    }
    if (pbGnssSysTimeStrctType & PB_GNSS_SYSTEM_TIME_WEEK_MS_VALID) {
        gnssSysTimeStrctTypeMsk |= GNSS_SYSTEM_TIME_WEEK_MS_VALID;
    }
    if (pbGnssSysTimeStrctType & PB_GNSS_SYSTEM_CLK_TIME_BIAS_VALID) {
        gnssSysTimeStrctTypeMsk |= GNSS_SYSTEM_CLK_TIME_BIAS_VALID;
    }
    if (pbGnssSysTimeStrctType & PB_GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID) {
        gnssSysTimeStrctTypeMsk |= GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID;
    }
    if (pbGnssSysTimeStrctType & PB_GNSS_SYSTEM_REF_FCOUNT_VALID) {
        gnssSysTimeStrctTypeMsk |= GNSS_SYSTEM_REF_FCOUNT_VALID;
    }
    if (pbGnssSysTimeStrctType & PB_GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID) {
        gnssSysTimeStrctTypeMsk |= GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssSysTimeStrctType:%x, gnssSysTimeStrctTypeMsk:%x",
            pbGnssSysTimeStrctType, gnssSysTimeStrctTypeMsk);
    return gnssSysTimeStrctTypeMsk;
}

uint32_t LocationApiPbMsgConv::getGnssSignalTypeMaskFromPB(
        const uint32_t &pbGnssSignalTypeMask) const {
    uint32_t gnssSignalTypeMask = 0;
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GPS_L1CA_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GPS_L1CA;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GPS_L1C_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GPS_L1C;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GPS_L2_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GPS_L2;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GPS_L5_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GPS_L5;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GLONASS_G1_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GLONASS_G1;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GLONASS_G2_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GLONASS_G2;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GALILEO_E1_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GALILEO_E1;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GALILEO_E5A_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GALILEO_E5A;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_GALILEO_E5B_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GALILEO_E5B;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B1_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B1;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B2_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_QZSS_L1CA_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_QZSS_L1CA;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_QZSS_L1S_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_QZSS_L1S;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_QZSS_L2_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_QZSS_L2;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_QZSS_L5_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_QZSS_L5;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_SBAS_L1_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_SBAS_L1;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B1I_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B1I;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B1C_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B1C;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B2I_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2I;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B2AI_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2AI;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_NAVIC_L5_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_NAVIC_L5;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B2AQ_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2AQ;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B2BI_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2BI;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_BEIDOU_B2BQ_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2BQ;
    }
    if (pbGnssSignalTypeMask & PB_GNSS_SIGNAL_NAVIC_L1_BIT) {
        gnssSignalTypeMask |= GNSS_SIGNAL_NAVIC_L1;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssSignalTypeMask:%x, gnssSignalTypeMask:%x",
            pbGnssSignalTypeMask, gnssSignalTypeMask);
    return gnssSignalTypeMask;
}

uint32_t LocationApiPbMsgConv::getGnssSvOptionsMaskFromPB(
        const uint32_t &pbGnssSvOptMask) const {
    uint32_t gnssSvOptMask = 0;
    if (pbGnssSvOptMask & PB_GNSS_SV_OPTIONS_HAS_EPHEMER_BIT) {
        gnssSvOptMask |= GNSS_SV_OPTIONS_HAS_EPHEMER_BIT;
    }
    if (pbGnssSvOptMask & PB_GNSS_SV_OPTIONS_HAS_ALMANAC_BIT) {
        gnssSvOptMask |= GNSS_SV_OPTIONS_HAS_ALMANAC_BIT;
    }
    if (pbGnssSvOptMask & PB_GNSS_SV_OPTIONS_USED_IN_FIX_BIT) {
        gnssSvOptMask |= GNSS_SV_OPTIONS_USED_IN_FIX_BIT;
    }
    if (pbGnssSvOptMask & PB_GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT) {
        gnssSvOptMask |= GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT;
    }
    if (pbGnssSvOptMask & PB_GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT) {
        gnssSvOptMask |= GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT;
    }
    if (pbGnssSvOptMask & PB_GNSS_SV_OPTIONS_HAS_BASEBAND_CARRIER_TO_NOISE_BIT) {
        gnssSvOptMask |= GNSS_SV_OPTIONS_HAS_BASEBAND_CARRIER_TO_NOISE_BIT;
    }
    if (pbGnssSvOptMask & PB_GNSS_SV_OPTIONS_HAS_ELEVATION_BIT) {
        gnssSvOptMask |= GNSS_SV_OPTIONS_HAS_ELEVATION_BIT;
    }
    if (pbGnssSvOptMask & PB_GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT) {
        gnssSvOptMask |= GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssSvOptMask:%x, gnssSvOptMask:%x", pbGnssSvOptMask,
            gnssSvOptMask);
    return gnssSvOptMask;
}

uint32_t LocationApiPbMsgConv::getGfBreachTypeMaskFromPB(const uint32_t &pbGfBreackTypMask) const {
    uint32_t gfBreachTypMask = 0;
    if (pbGfBreackTypMask & PB_GEOFENCE_BREACH_ENTER_BIT) {
        gfBreachTypMask |= GEOFENCE_BREACH_ENTER_BIT;
    }
    if (pbGfBreackTypMask & PB_GEOFENCE_BREACH_EXIT_BIT) {
        gfBreachTypMask |= GEOFENCE_BREACH_EXIT_BIT;
    }
    if (pbGfBreackTypMask & PB_GEOFENCE_BREACH_DWELL_IN_BIT) {
        gfBreachTypMask |= GEOFENCE_BREACH_DWELL_IN_BIT;
    }
    if (pbGfBreackTypMask & PB_GEOFENCE_BREACH_DWELL_OUT_BIT) {
        gfBreachTypMask |= GEOFENCE_BREACH_DWELL_OUT_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGfBreackTypMask:%x, gfBreachTypMask:%x", pbGfBreackTypMask,
            gfBreachTypMask);
    return gfBreachTypMask;
}

uint32_t LocationApiPbMsgConv::getLocationFlagsMaskFromPB(const uint32_t &pbLocFlagsMask) const {
    uint32_t locFlagsMask = 0;
    if (pbLocFlagsMask & PB_LOCATION_HAS_LAT_LONG_BIT) {
        locFlagsMask |= LOCATION_HAS_LAT_LONG_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_ALTITUDE_BIT) {
        locFlagsMask |= LOCATION_HAS_ALTITUDE_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_SPEED_BIT) {
        locFlagsMask |= LOCATION_HAS_SPEED_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_BEARING_BIT) {
        locFlagsMask |= LOCATION_HAS_BEARING_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_ACCURACY_BIT) {
        locFlagsMask |= LOCATION_HAS_ACCURACY_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_VERTICAL_ACCURACY_BIT) {
        locFlagsMask |= LOCATION_HAS_VERTICAL_ACCURACY_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_SPEED_ACCURACY_BIT) {
        locFlagsMask |= LOCATION_HAS_SPEED_ACCURACY_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_BEARING_ACCURACY_BIT) {
        locFlagsMask |= LOCATION_HAS_BEARING_ACCURACY_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_ELAPSED_REAL_TIME_BIT) {
        locFlagsMask |= LOCATION_HAS_ELAPSED_REAL_TIME_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_TIME_UNC_BIT) {
        locFlagsMask |= LOCATION_HAS_TIME_UNC_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_GPTP_TIME_BIT) {
        locFlagsMask |= LOCATION_HAS_GPTP_TIME_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_GPTP_TIME_UNC_BIT) {
        locFlagsMask |= LOCATION_HAS_GPTP_TIME_UNC_BIT;
    }
    if (pbLocFlagsMask & PB_LOCATION_HAS_SESSION_STATUS_BIT) {
        locFlagsMask |= LOCATION_HAS_SESSION_STATUS_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbLocFlagsMask:%x, locFlagsMask:%x", pbLocFlagsMask, locFlagsMask);
    return locFlagsMask;
}

uint32_t LocationApiPbMsgConv::getLocationTechnologyMaskFromPB(
        const uint32_t &pbLocTechMask) const {
    uint32_t locTechMask = 0;
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_GNSS_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_GNSS_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_CELL_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_CELL_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_WIFI_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_WIFI_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_SENSORS_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_SENSORS_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_AFLT_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_AFLT_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_HYBRID_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_HYBRID_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_PPE_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_PPE_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_VEH_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_VEH_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_VIS_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_VIS_BIT;
    }
    if (pbLocTechMask & PB_LOCATION_TECHNOLOGY_PROPAGATED_BIT) {
        locTechMask |= LOCATION_TECHNOLOGY_PROPAGATED_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbLocTechMask:%x, locTechMask:%x", pbLocTechMask, locTechMask);
    return locTechMask;
}

uint32_t LocationApiPbMsgConv::getGnssMeasurementsClockFlagsMaskFromPB(
        const uint32_t &pbGnssMeasClockFlgMask) const {
    uint32_t gnssMeasClockFlgMask = 0;
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_UNCERTAINTY_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_UNCERTAINTY_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT;
    }
    if (pbGnssMeasClockFlgMask &
            PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_HW_CLOCK_DISCONTINUITY_COUNT_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_HW_CLOCK_DISCONTINUITY_COUNT_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_UNC_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_UNC_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_BIT;
    }
    if (pbGnssMeasClockFlgMask & PB_GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_UNC_BIT) {
        gnssMeasClockFlgMask |= GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_UNC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssMeasClockFlgMask:%x, gnssMeasClockFlgMask:%x",
            pbGnssMeasClockFlgMask, gnssMeasClockFlgMask);
    return gnssMeasClockFlgMask;
}

uint32_t LocationApiPbMsgConv::getGnssMeasurementsDataFlagsMaskFromPB(
        const uint32_t &pbGnssMeasDataFlgMask) const {
    uint32_t gnssMeasDataFlgMask = 0;
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_SV_ID_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_SV_ID_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_STATE_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_STATE_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_ADR_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_ADR_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT;
    }
    if (pbGnssMeasDataFlgMask & PB_GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT) {
        gnssMeasDataFlgMask |= GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssMeasDataFlgMask:%x, gnssMeasDataFlgMask:%x",
            pbGnssMeasDataFlgMask, gnssMeasDataFlgMask);
    return gnssMeasDataFlgMask;
}

uint32_t LocationApiPbMsgConv::getGnssMeasurementsAdrStateMaskFromPB(
        const uint32_t &pbGnssMeasAdrStateMask) const {
    uint32_t gnssMeasAdrStateMask = 0;
    if (pbGnssMeasAdrStateMask & PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT) {
        gnssMeasAdrStateMask |= GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT;
    }
    if (pbGnssMeasAdrStateMask & PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT) {
        gnssMeasAdrStateMask |= GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT;
    }
    if (pbGnssMeasAdrStateMask &
            PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT) {
        gnssMeasAdrStateMask |= GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT;
    }
    if (pbGnssMeasAdrStateMask &
            PB_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT) {
        gnssMeasAdrStateMask |=
            GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssMeasAdrStateMask:%x, gnssMeasAdrStateMask:%x",
            pbGnssMeasAdrStateMask, gnssMeasAdrStateMask);
    return gnssMeasAdrStateMask;
}

uint64_t LocationApiPbMsgConv::getGnssLocationInfoFlagMaskFromPB(
        const uint32_t &pbGnssLocInfoFlagMask,
        const uint32_t &pbGnssLocInfoExtFlagMask) const {

    uint64_t gnssLocInfoFlagMask = 0;
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_DOP_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_DOP_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_VER_RELIABILITY_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_VER_RELIABILITY_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_EXT_DOP_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_EXT_DOP_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_EAST_STD_DEV_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_EAST_STD_DEV_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_NORTH_VEL_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_NORTH_VEL_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_EAST_VEL_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_EAST_VEL_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_UP_VEL_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_UP_VEL_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_UP_VEL_UNC_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_UP_VEL_UNC_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_LEAP_SECONDS_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_LEAP_SECONDS_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_PERCENT_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_CONFORMITY_INDEX_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_CONFORMITY_INDEX_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_LLA_VRP_BASED_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_LLA_VRP_BASED_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_ENU_VELOCITY_VRP_BASED_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_ENU_VELOCITY_VRP_BASED_BIT;
    }
    if (pbGnssLocInfoFlagMask & PB_GNSS_LOCATION_INFO_DR_SOLUTION_STATUS_MASK_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_DR_SOLUTION_STATUS_MASK_BIT;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_ALTITUDE_ASSUMED_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_ALTITUDE_ASSUMED_BIT;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_SESSION_STATUS_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_SESSION_STATUS_BIT;
    }

    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_INTEGRITY_RISK_USED_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_INTEGRITY_RISK_USED_BIT;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_PROTECT_ALONG_TRACK_BIT ) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_PROTECT_ALONG_TRACK_BIT ;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_PROTECT_CROSS_TRACK_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_PROTECT_CROSS_TRACK_BIT;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_PROTECT_VERTICAL_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_PROTECT_VERTICAL_BIT;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_DGNSS_STATION_ID_MASK_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_DGNSS_STATION_ID_BIT;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_BASE_LINE_LENGTH_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_BASE_LINE_LENGTH_BIT;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_AGE_OF_CORRECTION_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_AGE_OF_CORRECTION_BIT;
    }
    if (pbGnssLocInfoExtFlagMask & PB_GNSS_LOCATION_INFO_LEAP_SECONDS_UNC_BIT) {
        gnssLocInfoFlagMask |= LDT_GNSS_LOCATION_INFO_LEAP_SECONDS_UNC_BIT;
    }

    LocApiPb_LOGv("LocApiPB: pbGnssLocInfoFlagMask:0x%x, pbGnssLocInfoExtFlagMask:0x%x, "
                  "gnssLocInfoFlagMask:0x%" PRIu64"", pbGnssLocInfoFlagMask,
                  pbGnssLocInfoExtFlagMask, gnssLocInfoFlagMask);

    return gnssLocInfoFlagMask;
}


uint32_t LocationApiPbMsgConv::getGnssLocationNavSolutionMaskFromPB(
        const uint32_t &pbGnssLocNavSoln) const {
    uint32_t gnssLocNavSoln = 0;
    if (pbGnssLocNavSoln & PB_LOCATION_SBAS_CORRECTION_IONO_BIT) {
        gnssLocNavSoln |= LOCATION_SBAS_CORRECTION_IONO_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_SBAS_CORRECTION_FAST_BIT) {
        gnssLocNavSoln |= LOCATION_SBAS_CORRECTION_FAST_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_SBAS_CORRECTION_LONG_BIT) {
        gnssLocNavSoln |= LOCATION_SBAS_CORRECTION_LONG_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_SBAS_INTEGRITY_BIT) {
        gnssLocNavSoln |= LOCATION_SBAS_INTEGRITY_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_NAV_CORRECTION_DGNSS_BIT) {
        gnssLocNavSoln |= LOCATION_NAV_CORRECTION_DGNSS_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_NAV_CORRECTION_RTK_BIT) {
        gnssLocNavSoln |= LOCATION_NAV_CORRECTION_RTK_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_NAV_CORRECTION_PPP_BIT) {
        gnssLocNavSoln |= LOCATION_NAV_CORRECTION_PPP_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_NAV_CORRECTION_RTK_FIXED_BIT) {
        gnssLocNavSoln |= LOCATION_NAV_CORRECTION_RTK_FIXED_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_NAV_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_BIT) {
        gnssLocNavSoln |= LOCATION_NAV_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_BIT;
    }
    if (pbGnssLocNavSoln & PB_LOCATION_NAV_MMF_AIDED_POSITION) {
        gnssLocNavSoln |= LOCATION_NAV_MMF_AIDED_POSITION;
    }

    LocApiPb_LOGv("LocApiPB: pbGnssLocNavSoln:%x, gnssLocNavSoln:%x", pbGnssLocNavSoln,
            gnssLocNavSoln);
    return gnssLocNavSoln;
}

uint32_t LocationApiPbMsgConv::getDrCalibrationStatusMaskFromPB(
        const uint32_t &pbDrCalibStatus) const {
    uint32_t drCalibStatus = 0;
    if (pbDrCalibStatus & PB_DR_ROLL_CALIBRATION_NEEDED) {
        drCalibStatus |= DR_ROLL_CALIBRATION_NEEDED;
    }
    if (pbDrCalibStatus & PB_DR_PITCH_CALIBRATION_NEEDED) {
        drCalibStatus |= DR_PITCH_CALIBRATION_NEEDED;
    }
    if (pbDrCalibStatus & PB_DR_YAW_CALIBRATION_NEEDED) {
        drCalibStatus |= DR_YAW_CALIBRATION_NEEDED;
    }
    if (pbDrCalibStatus & PB_DR_ODO_CALIBRATION_NEEDED) {
        drCalibStatus |= DR_ODO_CALIBRATION_NEEDED;
    }
    if (pbDrCalibStatus & PB_DR_GYRO_CALIBRATION_NEEDED) {
        drCalibStatus |= DR_GYRO_CALIBRATION_NEEDED;
    }
    if (pbDrCalibStatus & PB_DR_TURN_CALIBRATION_LOW) {
        drCalibStatus |= DR_TURN_CALIBRATION_LOW;
    }
    if (pbDrCalibStatus & PB_DR_TURN_CALIBRATION_MEDIUM) {
        drCalibStatus |= PB_DR_TURN_CALIBRATION_MEDIUM;
    }
    if (pbDrCalibStatus & PB_DR_TURN_CALIBRATION_HIGH) {
        drCalibStatus |= PB_DR_TURN_CALIBRATION_HIGH;
    }
    if (pbDrCalibStatus & PB_DR_LINEAR_ACCEL_CALIBRATION_LOW) {
        drCalibStatus |= DR_LINEAR_ACCEL_CALIBRATION_LOW;
    }
    if (pbDrCalibStatus & PB_DR_LINEAR_ACCEL_CALIBRATION_MEDIUM) {
        drCalibStatus |= PB_DR_LINEAR_ACCEL_CALIBRATION_MEDIUM;
    }
    if (pbDrCalibStatus & PB_DR_LINEAR_ACCEL_CALIBRATION_HIGH) {
        drCalibStatus |= PB_DR_LINEAR_ACCEL_CALIBRATION_HIGH;
    }
    if (pbDrCalibStatus & PB_DR_LINEAR_MOTION_CALIBRATION_LOW) {
        drCalibStatus |= DR_LINEAR_MOTION_CALIBRATION_LOW;
    }
    if (pbDrCalibStatus & PB_DR_LINEAR_MOTION_CALIBRATION_MEDIUM) {
        drCalibStatus |= PB_DR_LINEAR_MOTION_CALIBRATION_MEDIUM;
    }
    if (pbDrCalibStatus & PB_DR_LINEAR_MOTION_CALIBRATION_HIGH) {
        drCalibStatus |= PB_DR_LINEAR_MOTION_CALIBRATION_HIGH;
    }
    if (pbDrCalibStatus & PB_DR_STATIC_CALIBRATION_LOW) {
        drCalibStatus |= DR_STATIC_CALIBRATION_LOW;
    }
    if (pbDrCalibStatus & PB_DR_STATIC_CALIBRATION_MEDIUM) {
        drCalibStatus |= PB_DR_STATIC_CALIBRATION_MEDIUM;
    }
    if (pbDrCalibStatus & PB_DR_STATIC_CALIBRATION_HIGH) {
        drCalibStatus |= PB_DR_STATIC_CALIBRATION_HIGH;
    }

    LocApiPb_LOGv("LocApiPB: pbDrCalibStatus:%x, drCalibStatus:%x", pbDrCalibStatus,
            drCalibStatus);
    return drCalibStatus;
}

uint32_t LocationApiPbMsgConv::getGnssLocationPosDataMaskFromPB(
        const uint32_t &pbGnssLocPosDataMask) const {
    uint32_t gnssLocPosDataMask = 0;
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_YAW_RATE_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_YAW_RATE_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_PITCH_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_PITCH_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT) {
        gnssLocPosDataMask |= LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssLocPosDataMask:%x, gnssLocPosDataMask:%x",
            pbGnssLocPosDataMask, gnssLocPosDataMask);
    return gnssLocPosDataMask;
}

uint32_t LocationApiPbMsgConv::getGnssLocationPosDataMaskExtFromPB(
        const uint32_t &pbGnssLocPosDataMask) const {
    uint32_t gnssLocPosDataMaskExt = 0;
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_PITCH_RATE_BIT) {
        gnssLocPosDataMaskExt |= LOCATION_NAV_DATA_HAS_PITCH_RATE_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_PITCH_RATE_UNC_BIT) {
        gnssLocPosDataMaskExt |= LOCATION_NAV_DATA_HAS_PITCH_RATE_UNC_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_ROLL_BIT) {
        gnssLocPosDataMaskExt |= LOCATION_NAV_DATA_HAS_ROLL_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_ROLL_UNC_BIT) {
        gnssLocPosDataMaskExt |= LOCATION_NAV_DATA_HAS_ROLL_UNC_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_ROLL_RATE_BIT) {
        gnssLocPosDataMaskExt |= LOCATION_NAV_DATA_HAS_ROLL_RATE_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_ROLL_RATE_UNC_BIT) {
        gnssLocPosDataMaskExt |= LOCATION_NAV_DATA_HAS_ROLL_RATE_UNC_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_YAW_BIT) {
        gnssLocPosDataMaskExt |= LOCATION_NAV_DATA_HAS_YAW_BIT;
    }
    if (pbGnssLocPosDataMask & PB_LOCATION_NAV_DATA_HAS_YAW_UNC_BIT) {
        gnssLocPosDataMaskExt |= LOCATION_NAV_DATA_HAS_YAW_UNC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssLocPosDataMask:%x, gnssLocPosDataMaskExt:%x",
            pbGnssLocPosDataMask, gnssLocPosDataMaskExt);
    return gnssLocPosDataMaskExt;
}

uint32_t LocationApiPbMsgConv::getGnssGloTimeStructTypeFlagsFromPB(
        const uint32_t &pbGnssGloTimeStruct) const {
    uint32_t gnssGloTimeStruct = 0;
    if (pbGnssGloTimeStruct & PB_GNSS_CLO_DAYS_VALID) {
        gnssGloTimeStruct |= GNSS_CLO_DAYS_VALID;
    }
    if (pbGnssGloTimeStruct & PB_GNSS_GLO_MSEC_VALID) {
        gnssGloTimeStruct |= GNSS_GLO_MSEC_VALID;
    }
    if (pbGnssGloTimeStruct & PB_GNSS_GLO_CLK_TIME_BIAS_VALID) {
        gnssGloTimeStruct |= GNSS_GLO_CLK_TIME_BIAS_VALID;
    }
    if (pbGnssGloTimeStruct & PB_GNSS_GLO_CLK_TIME_BIAS_UNC_VALID) {
        gnssGloTimeStruct |= GNSS_GLO_CLK_TIME_BIAS_UNC_VALID;
    }
    if (pbGnssGloTimeStruct & PB_GNSS_GLO_REF_FCOUNT_VALID) {
        gnssGloTimeStruct |= GNSS_GLO_REF_FCOUNT_VALID;
    }
    if (pbGnssGloTimeStruct & PB_GNSS_GLO_NUM_CLOCK_RESETS_VALID) {
        gnssGloTimeStruct |= GNSS_GLO_NUM_CLOCK_RESETS_VALID;
    }
    if (pbGnssGloTimeStruct & PB_GNSS_GLO_FOUR_YEAR_VALID) {
        gnssGloTimeStruct |= GNSS_GLO_FOUR_YEAR_VALID;
    }
    LocApiPb_LOGv("LocApiPB: pbGnssGloTimeStruct:%x, gnssGloTimeStruct:%x",
            pbGnssGloTimeStruct, gnssGloTimeStruct);
    return gnssGloTimeStruct;
}

uint32_t LocationApiPbMsgConv::getGnssConfigRobustLocationValidMaskFromPB(
        const uint32_t &pbGnssCfgRobstLocValidMask) const {
    uint32_t gnssCfgRobstLocValidMask = 0;
    if (pbGnssCfgRobstLocValidMask & PB_GNSS_CONFIG_ROBUST_LOCATION_ENABLED_VALID_BIT) {
        gnssCfgRobstLocValidMask |= GNSS_CONFIG_ROBUST_LOCATION_ENABLED_VALID_BIT;
    }
    if (pbGnssCfgRobstLocValidMask & PB_GNSS_CONFIG_ROBUST_LOCATION_ENABLED_FOR_E911_VALID_BIT) {
        gnssCfgRobstLocValidMask |=  GNSS_CONFIG_ROBUST_LOCATION_ENABLED_FOR_E911_VALID_BIT;
    }
    if (pbGnssCfgRobstLocValidMask & PB_GNSS_CONFIG_ROBUST_LOCATION_VERSION_VALID_BIT) {
        gnssCfgRobstLocValidMask |= GNSS_CONFIG_ROBUST_LOCATION_VERSION_VALID_BIT;
    }

    LocApiPb_LOGv("LocApiPB: pbGnssCfgRobstLocValidMask:%x, gnssCfgRobstLocValidMask:%x",
            pbGnssCfgRobstLocValidMask, gnssCfgRobstLocValidMask);
    return gnssCfgRobstLocValidMask;
}

uint64_t LocationApiPbMsgConv::getDeadReckoningEngineConfigValidMaskFromPB(
        const uint64_t &pbDrEngCfgVldMask) const {
    uint64_t drEngCfgValidMask = 0;
    if (pbDrEngCfgVldMask & PB_BODY_TO_SENSOR_MOUNT_PARAMS_BIT) {
        drEngCfgValidMask |= BODY_TO_SENSOR_MOUNT_PARAMS_BIT;
    }
    if (pbDrEngCfgVldMask & PB_VEHICLE_SPEED_SCALE_FACTOR_BIT) {
        drEngCfgValidMask |= VEHICLE_SPEED_SCALE_FACTOR_BIT;
    }
    if (pbDrEngCfgVldMask & PB_VEHICLE_SPEED_SCALE_FACTOR_UNC_BIT) {
        drEngCfgValidMask |= VEHICLE_SPEED_SCALE_FACTOR_UNC_BIT;
    }
    if (pbDrEngCfgVldMask & PB_GYRO_SCALE_FACTOR_BIT) {
        drEngCfgValidMask |= GYRO_SCALE_FACTOR_BIT;
    }
    if (pbDrEngCfgVldMask & PB_GYRO_SCALE_FACTOR_UNC_BIT) {
        drEngCfgValidMask |= GYRO_SCALE_FACTOR_UNC_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbDrEngCfgVldMask:%" PRIu64", drEngCfgValidMask:%" PRIu64,
            pbDrEngCfgVldMask, drEngCfgValidMask);
    return drEngCfgValidMask;
}

uint32_t LocationApiPbMsgConv::getDrEngineAidingDataMaskFromPB(
        const uint32_t &pbDrEngAidDataMask) const {
    uint32_t drEngAidDataMask = 0;
    if (pbDrEngAidDataMask & PB_DR_ENGINE_AIDING_DATA_CALIBRATION_BIT) {
        drEngAidDataMask |= DR_ENGINE_AIDING_DATA_CALIBRATION_BIT;
    }
    LocApiPb_LOGv("LocApiPB: pbDrEngAidDataMask:%x, drEngAidDataMask:%x",
            pbDrEngAidDataMask, drEngAidDataMask);
    return drEngAidDataMask;
}

uint32_t LocationApiPbMsgConv::getDrSolutionStatusMaskFromPB(
        const uint32_t &pbDrSolnStatusMask) const {
    uint32_t drSolnStatusMask = 0;
    if (pbDrSolnStatusMask & PB_VEHICLE_SENSOR_SPEED_INPUT_DETECTED) {
        drSolnStatusMask |= VEHICLE_SENSOR_SPEED_INPUT_DETECTED;
    }
    if (pbDrSolnStatusMask & PB_VEHICLE_SENSOR_SPEED_INPUT_USED) {
        drSolnStatusMask |= VEHICLE_SENSOR_SPEED_INPUT_USED;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_UNCALIBRATED) {
        drSolnStatusMask |= DRE_WARNING_UNCALIBRATED;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_GNSS_QUALITY_INSUFFICIENT) {
        drSolnStatusMask |= DRE_WARNING_GNSS_QUALITY_INSUFFICIENT;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_FERRY_DETECTED) {
        drSolnStatusMask |= DRE_WARNING_FERRY_DETECTED;
    }
    if (pbDrSolnStatusMask & PB_DRE_ERROR_6DOF_SENSOR_UNAVAILABLE) {
        drSolnStatusMask |= DRE_ERROR_6DOF_SENSOR_UNAVAILABLE;
    }
    if (pbDrSolnStatusMask & PB_DRE_ERROR_VEHICLE_SPEED_UNAVAILABLE) {
        drSolnStatusMask |= DRE_ERROR_VEHICLE_SPEED_UNAVAILABLE;
    }
    if (pbDrSolnStatusMask & PB_DRE_ERROR_GNSS_EPH_UNAVAILABLE) {
        drSolnStatusMask |= DRE_ERROR_GNSS_EPH_UNAVAILABLE;
    }
    if (pbDrSolnStatusMask & PB_DRE_ERROR_GNSS_MEAS_UNAVAILABLE) {
        drSolnStatusMask |= DRE_ERROR_GNSS_MEAS_UNAVAILABLE;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_INIT_POSITION_INVALID) {
        drSolnStatusMask |= DRE_WARNING_INIT_POSITION_INVALID;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_INIT_POSITION_UNRELIABLE) {
        drSolnStatusMask |= DRE_WARNING_INIT_POSITION_UNRELIABLE;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_POSITON_UNRELIABLE) {
        drSolnStatusMask |= DRE_WARNING_POSITON_UNRELIABLE;
    }
    if (pbDrSolnStatusMask & PB_DRE_ERROR_GENERIC) {
        drSolnStatusMask |= DRE_ERROR_GENERIC;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_SENSOR_TEMP_OUT_OF_RANGE) {
        drSolnStatusMask |= DRE_WARNING_SENSOR_TEMP_OUT_OF_RANGE;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_USER_DYNAMICS_INSUFFICIENT) {
        drSolnStatusMask |= DRE_WARNING_USER_DYNAMICS_INSUFFICIENT;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_FACTORY_DATA_INCONSISTENT) {
        drSolnStatusMask |= DRE_WARNING_FACTORY_DATA_INCONSISTENT;
    }
    if (pbDrSolnStatusMask & PB_DRE_WARNING_MMF_UNAVAILABLE) {
        drSolnStatusMask |= DRE_WARNING_MMF_UNAVAILABLE;
    }
    if (pbDrSolnStatusMask  & PB_DRE_WARNING_MMF_NOT_USABLE) {
        drSolnStatusMask |= DRE_WARNING_MMF_NOT_USABLE;
    }

    LocApiPb_LOGv("LocApiPB: pbDrSolnStatusMask:%x, drSolnStatusMask:%x",
            pbDrSolnStatusMask, drSolnStatusMask);
    return drSolnStatusMask;
}

PBLocationSessionStatus LocationApiPbMsgConv::getPBEnumForLocSessionStatus(
        const loc_sess_status& status) const {
    PBLocationSessionStatus pbStatus = PB_LOCATION_SESS_FAILURE;

    if (status == LOC_SESS_SUCCESS) {
        pbStatus = PB_LOCATION_SESS_SUCCESS;
    } else if (status == LOC_SESS_INTERMEDIATE) {
        pbStatus = PB_LOCATION_SESS_INTERMEDIATE;
    }
    return pbStatus;
}

loc_sess_status LocationApiPbMsgConv::getLocSessionStatusFromPB(
        const PBLocationSessionStatus& pbStatus) const {

    loc_sess_status status = LOC_SESS_FAILURE;
    if (pbStatus == PB_LOCATION_SESS_SUCCESS) {
        status = LOC_SESS_SUCCESS;
    } else if (pbStatus == PB_LOCATION_SESS_INTERMEDIATE) {
        status = LOC_SESS_INTERMEDIATE;
    }
    return status;
}

// **** helper function for structure conversion to protobuf format
int LocationApiPbMsgConv::convertGnssSvTypeConfigToPB(const GnssSvTypeConfig &gnssSvTypeCfg,
        PBGnssSvTypeConfig *pbGnssSvTypeCfg) const {
    if (nullptr == pbGnssSvTypeCfg) {
        LOC_LOGe("pbGnssSvTypeCfg is NULL!, return");
        return 1;
    }
    // uint64 enabledSvTypesMask = 1;
    pbGnssSvTypeCfg->set_enabledsvtypesmask(gnssSvTypeCfg.enabledSvTypesMask);

    // uint64 blacklistedSvTypesMask = 2;
    pbGnssSvTypeCfg->set_blacklistedsvtypesmask(gnssSvTypeCfg.blacklistedSvTypesMask);

    LocApiPb_LOGd("LocApiPB: gnssSvTypeCfg - Enable Sv types: %" PRIu64 \
            " Blacklist Sv Types: %" PRIu64, gnssSvTypeCfg.enabledSvTypesMask,
            gnssSvTypeCfg.blacklistedSvTypesMask);
    return 0;
}

int LocationApiPbMsgConv::convertGnssSvIdConfigToPB(const GnssSvIdConfig &gnssSvIdCfg,
        PBGnssSvIdConfig *pbGnssSvIdCfg) const {
    if (nullptr == pbGnssSvIdCfg) {
        LOC_LOGe("pbGnssSvIdCfg is NULL!, return");
        return 1;
    }
    // uint64 gloBlacklistSvMask = 1;
    pbGnssSvIdCfg->set_globlacklistsvmask(gnssSvIdCfg.gloBlacklistSvMask);

    // uint64 bdsBlacklistSvMask = 2;
    pbGnssSvIdCfg->set_bdsblacklistsvmask(gnssSvIdCfg.bdsBlacklistSvMask);

    // uint64 qzssBlacklistSvMask = 3;
    pbGnssSvIdCfg->set_qzssblacklistsvmask(gnssSvIdCfg.qzssBlacklistSvMask);

    // uint64 galBlacklistSvMask = 4;
    pbGnssSvIdCfg->set_galblacklistsvmask(gnssSvIdCfg.galBlacklistSvMask);

    // uint64 sbasBlacklistSvMask = 5;
    pbGnssSvIdCfg->set_sbasblacklistsvmask(gnssSvIdCfg.sbasBlacklistSvMask);

    // uint64_t navicBlacklistSvMask = 6;
    pbGnssSvIdCfg->set_navicblacklistsvmask(gnssSvIdCfg.navicBlacklistSvMask);

    // uint64_t gpsBlacklistSvMask = 7;
    pbGnssSvIdCfg->set_gpsblacklistsvmask(gnssSvIdCfg.gpsBlacklistSvMask);

    LocApiPb_LOGd("LocApiPB: gnssSvIdCfg - Glo: %" PRIu64 ",Bds: %" PRIu64 ",Qzss: %" PRIu64 \
            ",Gal: %" PRIu64 ",Sbas: %" PRIu64",Nav: %" PRIu64", GPS: %" PRIu64 " ",
            gnssSvIdCfg.gloBlacklistSvMask,
            gnssSvIdCfg.bdsBlacklistSvMask, gnssSvIdCfg.qzssBlacklistSvMask,
            gnssSvIdCfg.galBlacklistSvMask, gnssSvIdCfg.sbasBlacklistSvMask,
            gnssSvIdCfg.navicBlacklistSvMask, gnssSvIdCfg.gpsBlacklistSvMask);
    return 0;
}

int LocationApiPbMsgConv::convertGnssAidingDataToPB(const GnssAidingData &gnssAidData,
        PBAidingData *pbGnssAidData) const {
    if (nullptr == pbGnssAidData) {
        LOC_LOGe("pbGnssAidData is NULL!, return");
        return 1;
    }

    //bool deleteAll  = 1;
    pbGnssAidData->set_deleteall(gnssAidData.deleteAll);

    // uint32 gnssAidingDataSvMask = 2; - bitwise OR of PBLocApiGnssAidingDataSvMask
    pbGnssAidData->set_gnssaidingdatasvmask(
        getPBMaskForGnssAidingDataSvMask(gnssAidData.sv.svMask));

    // uint32 dreAidingDataMask = 3;- PBDrEngineAidingDataMask
    pbGnssAidData->set_dreaidingdatamask(
        getPBMaskForDrEngineAidingDataMask(gnssAidData.dreAidingDataMask));

    // Masks from - PBLocApiPositioningEngineMask
    // uint32 posEngineMask = 4;
    pbGnssAidData->set_posenginemask(
            getPBMaskForPositioningEngineMask(gnssAidData.posEngineMask));

    LocApiPb_LOGd("LocApiPB: gnssAidData deleteAll:%d, svMask:%x, PosEngMask:%x",
            gnssAidData.deleteAll, gnssAidData.sv.svMask, gnssAidData.posEngineMask);
    return 0;
}

int LocationApiPbMsgConv::convertLeverArmConfigInfoToPB(const LeverArmConfigInfo &leverArmCfgInfo,
        PBLIALeverArmConfigInfo *pbLeverArmCfgInfo) const {
    if (nullptr == pbLeverArmCfgInfo) {
        LOC_LOGe("pbLeverArmCfgInfo is NULL!, return");
        return 1;
    }

    // uint32 leverArmValidMask = 1; - PBLIALeverArmTypeMask
    pbLeverArmCfgInfo->set_leverarmvalidmask(
            getPBMaskForLeverArmTypeMask(leverArmCfgInfo.leverArmValidMask));

    // PBLIALeverArmParams   gnssToVRP = 2;
    PBLIALeverArmParams* gnssToVRP = pbLeverArmCfgInfo->mutable_gnsstovrp();
    if (nullptr != gnssToVRP) {
        if (convertLeverArmParamsToPB(leverArmCfgInfo.gnssToVRP, gnssToVRP)) {
            LOC_LOGe("convertLeverArmParamsToPB failed");
            free(gnssToVRP);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_drimutognss failed");
        return 1;
    }

    // PBLIALeverArmParams   drImuToGnss = 3;
    PBLIALeverArmParams* drImuToGnss = pbLeverArmCfgInfo->mutable_drimutognss();
    if (nullptr != drImuToGnss) {
        if (convertLeverArmParamsToPB(leverArmCfgInfo.drImuToGnss, drImuToGnss)) {
            LOC_LOGe("convertLeverArmParamsToPB failed");
            free(drImuToGnss);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_drimutognss failed");
        return 1;
    }

    // PBLIALeverArmParams   veppImuToGnss = 4;
    PBLIALeverArmParams* veppImuToGnss = pbLeverArmCfgInfo->mutable_veppimutognss();
    if (nullptr != veppImuToGnss) {
        if (convertLeverArmParamsToPB(leverArmCfgInfo.veppImuToGnss, veppImuToGnss)) {
            LOC_LOGe("convertLeverArmParamsToPB failed");
            free(veppImuToGnss);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_veppimutognss failed");
        return 1;
    }

    LocApiPb_LOGd("LocApiPB: leverArmCfgInfo - leverArmValidMask: %x",
            leverArmCfgInfo.leverArmValidMask);
    return 0;
}

int LocationApiPbMsgConv::convertBodyToSensorMountParamsToPB(
        const BodyToSensorMountParams &bodyToSensorMntParams,
        PBLIABodyToSensorMountParams *pbBodyToSensorMntParams) const {
    if (nullptr == pbBodyToSensorMntParams) {
        LOC_LOGe("pbBodyToSensorMntParams is NULL!, return");
        return 1;
    }
    // float rollOffset = 1;
    pbBodyToSensorMntParams->set_rolloffset(bodyToSensorMntParams.rollOffset);
    // float yawOffset = 2;
    pbBodyToSensorMntParams->set_yawoffset(bodyToSensorMntParams.yawOffset);
    // float pitchOffset = 3;
    pbBodyToSensorMntParams->set_pitchoffset(bodyToSensorMntParams.pitchOffset);
    // float offsetUnc = 4;
    pbBodyToSensorMntParams->set_offsetunc(bodyToSensorMntParams.offsetUnc);
    LocApiPb_LOGd("LocApiPB: bodyToSensorMntParams: Offset - Roll:%f, Yaw:%f, Pitch:%f, Unc:%f",
            bodyToSensorMntParams.rollOffset, bodyToSensorMntParams.yawOffset,
            bodyToSensorMntParams.pitchOffset, bodyToSensorMntParams.offsetUnc);
    return 0;
}

int LocationApiPbMsgConv::convertDeadReckoningEngineConfigToPB(
        const DeadReckoningEngineConfig &drEngConfig,
        PBDeadReckoningEngineConfig *pbDrEngConfig) const {
    if (nullptr == pbDrEngConfig) {
        LOC_LOGe("pbDrEngConfig is NULL!, return");
        return 1;
    }
    // uint64    validMask = 1; - Bitwise OR mask of PBDeadReckoningEngineConfigValidMask
    pbDrEngConfig->set_validmask(
            getPBMaskForDeadReckoningEngineConfigValidMask(drEngConfig.validMask));

    // PBLIABodyToSensorMountParams bodyToSensorMountParams = 2;
    PBLIABodyToSensorMountParams* b2SMountParams = pbDrEngConfig->mutable_bodytosensormountparams();
    if (nullptr != b2SMountParams) {
        if (convertBodyToSensorMountParamsToPB(drEngConfig.bodyToSensorMountParams,
                b2SMountParams)) {
            LOC_LOGe("convertBodyToSensorMountParamsToPB failed");
            free(b2SMountParams);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_bodytosensormountparams failed");
        return 1;
    }

    // float vehicleSpeedScaleFactor = 3;
    pbDrEngConfig->set_vehiclespeedscalefactor(drEngConfig.vehicleSpeedScaleFactor);
    // float vehicleSpeedScaleFactorUnc = 4;
    pbDrEngConfig->set_vehiclespeedscalefactorunc(drEngConfig.vehicleSpeedScaleFactorUnc);
    // float gyroScaleFactor = 5;
    pbDrEngConfig->set_gyroscalefactor(drEngConfig.gyroScaleFactor);
    // float gyroScaleFactorUnc = 6;
    pbDrEngConfig->set_gyroscalefactorunc(drEngConfig.gyroScaleFactorUnc);

    LOC_LOGv("LocApiPB: drEngConfig - DrEngConfigValidMask:%"  PRIu64", VehSpeedScale: %f"
            " VehSpeedScaleUnc: %f, GyroScale: %f, GyroScaleUnc: %f", drEngConfig.validMask,
            drEngConfig.vehicleSpeedScaleFactor, drEngConfig.vehicleSpeedScaleFactorUnc,
            drEngConfig.gyroScaleFactor, drEngConfig.gyroScaleFactorUnc);
    return 0;
}

int LocationApiPbMsgConv::convertXtraConfigParamsToPB(
        const XtraConfigParams& xtraParams, PBXtraConfigParams* pbXtraParams) const {
    pbXtraParams->set_xtradownloadintervalminute(xtraParams.xtraDownloadIntervalMinute);
    pbXtraParams->set_xtradownloadtimeoutsec(xtraParams.xtraDownloadTimeoutSec);
    pbXtraParams->set_xtradownloadretryintervalminute(xtraParams.xtraDownloadRetryIntervalMinute);
    pbXtraParams->set_xtradownloadretryattempts(xtraParams.xtraDownloadRetryAttempts);
    pbXtraParams->set_xtracapath(xtraParams.xtraCaPath);
    for (int index = 0; index < xtraParams.xtraServerURLsCount; index++) {
        pbXtraParams->add_xtraserverurls(xtraParams.xtraServerURLs[index]);
        LOC_LOGv("add %s", xtraParams.xtraServerURLs[index]);
    }
    for (int index = 0; index < xtraParams.ntpServerURLsCount; index++) {
        pbXtraParams->add_ntpserverurls(xtraParams.ntpServerURLs[index]);
        LOC_LOGv("add %s", xtraParams.ntpServerURLs[index]);
    }

    pbXtraParams->set_ntskeserverurl(xtraParams.ntsKeServerURL);
    // conversion routine for debug level
    pbXtraParams->set_xtradaemondebugloglevel(
            getPBEnumForDebugLogLevel(xtraParams.xtraDaemonDebugLogLevel));

    pbXtraParams->set_xtraintegritydownloadenable(
            xtraParams.xtraIntegrityDownloadEnable);
    pbXtraParams->set_xtraintegritydownloadintervalminute(
            xtraParams.xtraIntegrityDownloadIntervalMinute);
    pbXtraParams->set_xtradaemondiagloggingstatus(
            xtraParams.xtraDaemonDiagLoggingStatus);

    return 0;
}

int LocationApiPbMsgConv::pbConvertToXtraConfig(const PBXtraConfigParams &pbXtraParams,
            XtraConfigParams& xtraParams) const {
    xtraParams.xtraDownloadIntervalMinute = pbXtraParams.xtradownloadintervalminute();
    xtraParams.xtraDownloadTimeoutSec = pbXtraParams.xtradownloadtimeoutsec();

    xtraParams.xtraDownloadRetryIntervalMinute =
            pbXtraParams.xtradownloadretryintervalminute();
    xtraParams.xtraDownloadRetryAttempts = pbXtraParams.xtradownloadretryattempts();

    strlcpy(xtraParams.xtraCaPath, pbXtraParams.xtracapath().c_str(),
            sizeof(xtraParams.xtraCaPath));

    for (int index = 0; index < pbXtraParams.xtraserverurls_size(); index++) {
        strlcpy(xtraParams.xtraServerURLs[index], pbXtraParams.xtraserverurls(index).c_str(),
                sizeof(xtraParams.xtraServerURLs[index]));
        LOC_LOGv("xtra server url: %d %s", index, xtraParams.xtraServerURLs[index]);
    }
    xtraParams.xtraServerURLsCount = pbXtraParams.xtraserverurls_size();

    for (int index = 0; index < pbXtraParams.ntpserverurls_size(); index++) {
        strlcpy(xtraParams.ntpServerURLs[index], pbXtraParams.ntpserverurls(index).c_str(),
                sizeof(xtraParams.ntpServerURLs[index]));
        LOC_LOGv("ntp server url: %d %s", index, xtraParams.ntpServerURLs[index]);
    }
    xtraParams.ntpServerURLsCount = pbXtraParams.ntpserverurls_size();
    strlcpy(xtraParams.ntsKeServerURL, pbXtraParams.ntskeserverurl().c_str(),
            sizeof(xtraParams.ntsKeServerURL));
    LOC_LOGv("nts ke server url: %s", xtraParams.ntsKeServerURL);

    xtraParams.xtraDaemonDebugLogLevel =
            getDebugLogLevelFromPB(pbXtraParams.xtradaemondebugloglevel());

    xtraParams.xtraIntegrityDownloadEnable = pbXtraParams.xtraintegritydownloadenable();
    xtraParams.xtraIntegrityDownloadIntervalMinute =
            pbXtraParams.xtraintegritydownloadintervalminute();
    xtraParams.xtraDaemonDiagLoggingStatus =
            pbXtraParams.xtradaemondiagloggingstatus();
    return 0;
}

int LocationApiPbMsgConv::convertXtraStatusToPB(
        const XtraStatus& xtraStatus, PBXtraStatus* pbXtraStatus) const {
    pbXtraStatus->set_featureenabled(xtraStatus.featureEnabled);
    if (xtraStatus.featureEnabled == true) {
        pbXtraStatus->set_xtradatastatus(getPBEnumForXtraDataStatus(xtraStatus.xtraDataStatus));
        pbXtraStatus->set_xtravalidforhours(xtraStatus.xtraValidForHours);
    }
    LOC_LOGv("pb xtra status %d %d %d", pbXtraStatus->featureenabled(),
             pbXtraStatus->xtradatastatus(), pbXtraStatus->xtravalidforhours());
    return 0;
}

int LocationApiPbMsgConv::pbConvertToXtraStatus(
        const PBXtraStatus &pbXtraStatus, XtraStatus& xtraStatus) const {
    xtraStatus.featureEnabled = pbXtraStatus.featureenabled();
    xtraStatus.xtraDataStatus = getXtraDataStatusFromPB(pbXtraStatus.xtradatastatus());
    xtraStatus.xtraValidForHours = pbXtraStatus.xtravalidforhours();
    return 0;
}

int LocationApiPbMsgConv::convertLocationOptionsToPB(const LocationOptions &locOpt,
        PBLocationOptions *pbLocOpt) const {
    if (nullptr == pbLocOpt) {
        LOC_LOGe("pbLocOpt is NULL!, return");
        return 1;
    }
    // uint32 minInterval = 1;
    pbLocOpt->set_mininterval(locOpt.minInterval);

    // uint32 minDistance = 2;
    pbLocOpt->set_mindistance(locOpt.minDistance);

    // PBGnssSuplMode mode = 3;
    pbLocOpt->set_mode(getPBEnumForGnssSuplMode(locOpt.mode));

    // uint32 locReqEngTypeMask = 4; - bitwise OR of PBLocReqEngineTypeMask
    pbLocOpt->set_locreqengtypemask(getPBMaskForLocReqEngineTypeMask(locOpt.locReqEngTypeMask));

    // PBFixQualityLevel = 5;
    pbLocOpt->set_qualitylevelaccepted(getPBEnumForFixQualityLevel(locOpt.qualityLevelAccepted));

    LocApiPb_LOGd("LocApiPB: locOpt - MinInterval: %u, MinDistance:%u, GnssSuplMode:%d, "\
            "LocReqEngineTypeMask:%x qualityLevelAccepted:%d",
            locOpt.minInterval, locOpt.minDistance, locOpt.mode,
            locOpt.locReqEngTypeMask, locOpt.qualityLevelAccepted);
    return 0;
}

int LocationApiPbMsgConv::convertGfAddedReqPayloadToPB(
        const GeofencesAddedReqPayload &gfAddReqPayload,
        PBGeofencesAddedReqPayload *pbGfAddReqPayload) const {
    if (nullptr == pbGfAddReqPayload) {
        LOC_LOGe("pbGfAddReqPayload is NULL!, return");
        return 1;
    }
    // repeated PBGeofencePayload gfPayload = 1;
    LOC_LOGv("LocApiPB: gfPayload count:%d", gfAddReqPayload.count);
    for (uint32_t i=0; i < gfAddReqPayload.count; i++) {
        PBGeofencePayload* gfPload = pbGfAddReqPayload->add_gfpayload();
        if (nullptr != gfPload) {
            // uint32 gfClientId = 1;
            gfPload->set_gfclientid(gfAddReqPayload.gfPayload[i].gfClientId);
            LocApiPb_LOGv("LocApiPB: gfPayload[%u] client Id:%u", i, gfAddReqPayload.count);

            // PBGeofenceOption gfOption = 2;
            PBGeofenceOption *pbGfOpt = gfPload->mutable_gfoption();
            if (nullptr != pbGfOpt) {
                if (convertGeofenceOptionToPB(gfAddReqPayload.gfPayload[i].gfOption, pbGfOpt)) {
                    LOC_LOGe("convertGeofenceOptionToPB failed");
                    free(pbGfOpt);
                    return 1;
                }
            } else {
                LOC_LOGe("mutable_gfoption failed");
                return 1;
            }

            // PBGeofenceInfo gfInfo = 3;
            PBGeofenceInfo* pbGfInfo = gfPload->mutable_gfinfo();
            if (nullptr != pbGfInfo) {
                if (convertGeofenceInfoToPB(gfAddReqPayload.gfPayload[i].gfInfo, pbGfInfo)) {
                    LOC_LOGe("convertGeofenceInfoToPB failed");
                    free(pbGfInfo);
                    return 1;
                }
            } else {
                LOC_LOGe("mutable_gfinfo failed");
                return 1;
            }
        } else {
            LOC_LOGe("add_gfpayload is NULL");
            return 1;
        }
    }

    return 0;
}

int LocationApiPbMsgConv::convertGfReqClientIdPayloadToPB(
        const GeofencesReqClientIdPayload &gfReqClntIdPayload,
        PBGeofencesReqClientIdPayload *pbGfReqClntIdPayload) const {
    if (nullptr == pbGfReqClntIdPayload) {
        LOC_LOGe("pbGfReqClntIdPayload is NULL!, return");
        return 1;
    }
    // repeated uint32 gfIds = 1;
    uint32_t gfCount = gfReqClntIdPayload.count;
    LocApiPb_LOGd("LocApiPB: gfReqClntIdPayload - count %u", gfCount);
    for (uint32_t i = 0; i < gfCount; i++) {
        LocApiPb_LOGv("LocApiPB: gfReqClntIdPayload gfIds[%u] - %u", i,
                gfReqClntIdPayload.gfIds[i]);
        pbGfReqClntIdPayload->add_gfids(gfReqClntIdPayload.gfIds[i]);
    }
    return 0;
}

int LocationApiPbMsgConv::convertCollectiveResPayloadToPB(
        const CollectiveResPayload &collctResPload,
        PBCollectiveResPayload *pbCollctResPload) const {
    if (nullptr == pbCollctResPload) {
        LOC_LOGe("pbCollctResPload is NULL!, return");
        return 1;
    }

    // repeated PBGeofenceResponse resp = 1;
    int count = collctResPload.resp.size();
    LocApiPb_LOGd("LocApiPB: collctResPload count:%d", count);
    for (int i=0; i < count; i++) {
        LocApiPb_LOGv("LocApiPB: collctResPload clientId:%u, error:%d",
                collctResPload.resp[i].clientId, collctResPload.resp[i].error);

        PBGeofenceResponse *gfResp = pbCollctResPload->add_resp();
        if (nullptr != gfResp) {
            // uint32 clientId = 1;
            gfResp->set_clientid(collctResPload.resp[i].clientId);
            // PBLocationError error = 2;
            gfResp->set_error(getPBEnumForLocationError(collctResPload.resp[i].error));
        } else {
            LOC_LOGe("add_resp is NULL");
            return 1;
        }
    }
    return 0;
}

int LocationApiPbMsgConv::convertGnssConfigRobustLocationToPB(
        const GnssConfigRobustLocation &gnssCfgRbstLoc,
        PBGnssConfigRobustLocation *pbGnssCfgRbstLoc) const {
    if (nullptr == pbGnssCfgRbstLoc) {
        LOC_LOGe("pbGnssCfgRbstLoc is NULL!, return");
        return 1;
    }

    // uint32 validMask = 1; - bitwise OR of PBGnssConfigRobustLocationValidMask
    pbGnssCfgRbstLoc->set_validmask(
        getPBMaskForGnssCfgRobustLocValidMask(gnssCfgRbstLoc.validMask));
    // bool enabled = 2;
    pbGnssCfgRbstLoc->set_enabled(gnssCfgRbstLoc.enabled);
    // bool enabledForE911 = 3;
    pbGnssCfgRbstLoc->set_enabledfore911(gnssCfgRbstLoc.enabledForE911);

    // PBGnssConfigRobustLocationVersion version = 4;
    PBGnssConfigRobustLocationVersion *cfRbstLocVer = pbGnssCfgRbstLoc->mutable_version();
    if (nullptr != cfRbstLocVer) {
        // uint32 major = 1;
        cfRbstLocVer->set_major(gnssCfgRbstLoc.version.major);
        // uint32 minor = 2;
        cfRbstLocVer->set_minor(gnssCfgRbstLoc.version.minor);
    } else {
        LOC_LOGe("cfRbstLocVer is NULL");
        return 1;
    }

    LOC_LOGv("LocApiPB: gnssCfgRbstLoc - CfgRobustLocMask:%x, Enabled:%d, EnabForE911:%d, "\
        "MajorVer:%d, MinorVer:%d", gnssCfgRbstLoc.validMask, gnssCfgRbstLoc.enabled,
        gnssCfgRbstLoc.enabledForE911, gnssCfgRbstLoc.version.major, gnssCfgRbstLoc.version.minor);
    return 0;
}

int LocationApiPbMsgConv::convertLocationToPB(const Location &location,
        PBLocation *pbLocation) const {
    if (nullptr == pbLocation) {
        LOC_LOGe("pbLocation is NULL!, return");
        return 1;
    }
    // uint32 flags = 1; - PBLocationFlagsMask
    pbLocation->set_flags(getPBMaskForLocationFlagsMask(location.flags));

    // uint64 timestamp = 2;
    pbLocation->set_timestamp(location.timestamp);

    // double latitude = 3;
    pbLocation->set_latitude(location.latitude);

    // double longitude = 4;
    pbLocation->set_longitude(location.longitude);

    // double altitude = 5;
    pbLocation->set_altitude(location.altitude);

    // float speed = 6;
    pbLocation->set_speed(location.speed);

    // float bearing = 7;
    pbLocation->set_bearing(location.bearing);

    // float horizontalAccuracy = 8;
    pbLocation->set_horizontalaccuracy(location.accuracy);

    // float verticalAccuracy = 9;
    pbLocation->set_verticalaccuracy(location.verticalAccuracy);

    // float speedAccuracy = 10;
    pbLocation->set_speedaccuracy(location.speedAccuracy);

    // float bearingAccuracy = 11;
    pbLocation->set_bearingaccuracy(location.bearingAccuracy);

    // uint32 techMask = 12; - PBLocationTechnologyMask
    pbLocation->set_techmask(getPBMaskForLocationTechnologyMask(location.techMask));

    // uint64 elapsedRealTime = 13;
    pbLocation->set_elapsedrealtime(location.elapsedRealTime);

    // uint64 elapsedRealTimeUnc = 14;
    pbLocation->set_elapsedrealtimeunc(location.elapsedRealTimeUnc);

    // float timeuncMs = 15;
    pbLocation->set_timeuncms(location.timeUncMs);

    // uint64 elapsedgPTPTime  = 16;
    pbLocation->set_elapsedgptptime(location.elapsedgPTPTime);
    // uint64 elapsedgPTPTimeUnc  = 17;
    pbLocation->set_elapsedgptptimeunc(location.elapsedgPTPTimeUnc);

    // LocSessionStatus sessionStatus = 18;
    pbLocation->set_sessionstatus(
            getPBEnumForLocSessionStatus(location.sessionStatus));

    LocApiPb_LOGd("LocApiPB: location - speed:%f, bear:%f, HorzAcc:%f, VertAcc:%f, SpeedAcc:%f, "
            "BearAcc:%f, time unc msec %f elapsedgPTPTime %" PRIu64" nsec, sessionStatus: %d",
            location.speed, location.bearing, location.accuracy, location.verticalAccuracy,
            location.speedAccuracy, location.bearingAccuracy, location.timeUncMs,
            location.elapsedgPTPTime, (int)location.sessionStatus);
    return 0;
}

int LocationApiPbMsgConv::convertLocAPIBatchingNotifMsgToPB(
        const LocAPIBatchNotification &locApiBatchNotifMsg,
        PBLocAPIBatchNotification *pbLocApiBatchNotifMsg) const {
    if (nullptr == pbLocApiBatchNotifMsg) {
        LOC_LOGe("pbLocApiBatchNotifMsg is NULL!, return");
        return 1;
    }
    int count = locApiBatchNotifMsg.location.size();
    LOC_LOGv("LocApiPB: locApiBatchNotifMsg - BatchStat: %d, Loc count:%d",
            locApiBatchNotifMsg.status, count);
    // PBBatchingStatus status = 1;
    pbLocApiBatchNotifMsg->set_status(
            getPBEnumForBatchingStatus(locApiBatchNotifMsg.status));
    // repeated PBLocation location = 2;
    for (int i=0; i < count; i++) {
        PBLocation* location = pbLocApiBatchNotifMsg->add_location();
        if (nullptr != location) {
            if (convertLocationToPB(locApiBatchNotifMsg.location[i], location)) {
                LOC_LOGe("convertLocationToPB failed");
                free(location);
                return 1;
            }
        } else {
            LOC_LOGe("add_location is NULL");
            return 1;
        }
    }
    return 0;
}

int LocationApiPbMsgConv::convertLocAPIGfBreachNotifToPB(
        const LocAPIGeofenceBreachNotification &locApiGfBreachNotif,
        PBLocAPIGeofenceBreachNotification *pbLocApiGfBreachNotif) const {
    if (nullptr == pbLocApiGfBreachNotif) {
        LOC_LOGe("pbLocApiGfBreachNotif is NULL!, return");
        return 1;
    }
    int gfBreachCnt = locApiGfBreachNotif.id.size();
    LOC_LOGv("LocApiPB: locApiGfBreachNotif - BreachTypMask: %x, timestamp: %" PRIu64 \
            "count:%d", locApiGfBreachNotif.type, locApiGfBreachNotif.timestamp, gfBreachCnt);

    // uint64 timestamp = 1;
    pbLocApiGfBreachNotif->set_timestamp(locApiGfBreachNotif.timestamp);

    // uint32 breachtype = 2; - PBGeofenceBreachTypeMask
    pbLocApiGfBreachNotif->set_breachtype(locApiGfBreachNotif.type);

    // PBLocation location = 3;
    PBLocation* location = pbLocApiGfBreachNotif->mutable_location();
    if (nullptr != location) {
        if (convertLocationToPB(locApiGfBreachNotif.location, location)) {
            LOC_LOGe("convertLocationToPB failed");
            free(location);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_locationnotification failed");
        return 1;
    }

    // repeated uint32 id = 4;
    for (int i = 0; i < gfBreachCnt; i++) {
        pbLocApiGfBreachNotif->add_id(locApiGfBreachNotif.id[i]);
    }
    return 0;
}

int LocationApiPbMsgConv::convertGnssLocInfoNotifToPB(
        const GnssLocationInfoNotification &gnssLocInfoNotif,
        PBGnssLocationInfoNotification *pbGnssLocInfoNotif) const {
    if (nullptr == pbGnssLocInfoNotif) {
        LOC_LOGe("pbGnssLocInfoNotif is NULL!, return");
        return 1;
    }
    // PBLocation location = 1;
    PBLocation* location = pbGnssLocInfoNotif->mutable_location();
    if (nullptr != location) {
        if (convertLocationToPB(gnssLocInfoNotif.location, location)) {
            LOC_LOGe("convertLocationToPB failed");
            free(location);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_location failed");
        return 1;
    }

    // uint32 flags = 2; - bitwise OR of PBGnssLocationInfoFlagMask for param validity
    pbGnssLocInfoNotif->set_flags(getPBMaskForGnssLocationInfoFlagMask(gnssLocInfoNotif.flags));

    // float altitudeMeanSeaLevel = 3;
    pbGnssLocInfoNotif->set_altitudemeansealevel(gnssLocInfoNotif.altitudeMeanSeaLevel);

    // float pdop = 4;
    // float hdop = 5;
    // float vdop = 6;
    // float gdop = 7;
    // float tdop = 8;
    // float magneticDeviation = 9;
    pbGnssLocInfoNotif->set_pdop(gnssLocInfoNotif.pdop);
    pbGnssLocInfoNotif->set_hdop(gnssLocInfoNotif.hdop);
    pbGnssLocInfoNotif->set_vdop(gnssLocInfoNotif.vdop);
    pbGnssLocInfoNotif->set_gdop(gnssLocInfoNotif.gdop);
    pbGnssLocInfoNotif->set_tdop(gnssLocInfoNotif.tdop);
    pbGnssLocInfoNotif->set_magneticdeviation(gnssLocInfoNotif.magneticDeviation);

    // PBLocationReliability horReliability = 10;
    // PBLocationReliability verReliability = 11;
    pbGnssLocInfoNotif->set_horreliability(
            getPBEnumForLocationReliability(gnssLocInfoNotif.horReliability));
    pbGnssLocInfoNotif->set_verreliability(
            getPBEnumForLocationReliability(gnssLocInfoNotif.verReliability));

    // float horUncEllipseSemiMajor = 12;
    // float horUncEllipseSemiMinor = 13;
    // float horUncEllipseOrientAzimuth = 14;
    pbGnssLocInfoNotif->set_horuncellipsesemimajor(gnssLocInfoNotif.horUncEllipseSemiMajor);
    pbGnssLocInfoNotif->set_horuncellipsesemiminor(gnssLocInfoNotif.horUncEllipseSemiMinor);
    pbGnssLocInfoNotif->set_horuncellipseorientazimuth(
            gnssLocInfoNotif.horUncEllipseOrientAzimuth);

    // float northStdDeviation = 15;
    // float eastStdDeviation = 16;
    // float northVelocity = 17;
    // float eastVelocity = 18;
    // float upVelocity = 19;
    // float northVelocityStdDeviation = 20;
    // float eastVelocityStdDeviation = 21;
    // float upVelocityStdDeviation = 22;
    pbGnssLocInfoNotif->set_northstddeviation(gnssLocInfoNotif.northStdDeviation);
    pbGnssLocInfoNotif->set_eaststddeviation(gnssLocInfoNotif.eastStdDeviation);
    pbGnssLocInfoNotif->set_northvelocity(gnssLocInfoNotif.northVelocity);
    pbGnssLocInfoNotif->set_eastvelocity(gnssLocInfoNotif.eastVelocity);
    pbGnssLocInfoNotif->set_upvelocity(gnssLocInfoNotif.upVelocity);
    pbGnssLocInfoNotif->set_northvelocitystddeviation(gnssLocInfoNotif.northVelocityStdDeviation);
    pbGnssLocInfoNotif->set_eastvelocitystddeviation(gnssLocInfoNotif.eastVelocityStdDeviation);
    pbGnssLocInfoNotif->set_upvelocitystddeviation(gnssLocInfoNotif.upVelocityStdDeviation);

    // uint32 numSvUsedInPosition = 23;
    pbGnssLocInfoNotif->set_numsvusedinposition(gnssLocInfoNotif.numSvUsedInPosition);

    // PBGnssLocationSvUsedInPosition svUsedInPosition = 24;
    PBGnssLocationSvUsedInPosition* gnssLocSvUsedInPos =
            pbGnssLocInfoNotif->mutable_svusedinposition();
    if (nullptr != gnssLocSvUsedInPos) {
        if (convertGnssLocSvUsedInPosToPB(gnssLocInfoNotif.svUsedInPosition, gnssLocSvUsedInPos)) {
            LOC_LOGe("convertLocationToPB failed");
            free(gnssLocSvUsedInPos);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_svusedinposition failed");
        return 1;
    }

    // uint32 navSolutionMask = 25;  - bitwise OR of PBGnssLocationNavSolutionMask
    pbGnssLocInfoNotif->set_navsolutionmask(
            getPBMaskForGnssLocationNavSolutionMask(gnssLocInfoNotif.navSolutionMask));

    // PBLocApiGnssLocationPositionDynamics bodyFrameData = 26;
    // GnssLocationPositionDynamicsExt bodyFrameDataExt - Additional Body Frame Dynamics.
    PBLocApiGnssLocationPositionDynamics* gnssLocPosDyn =
            pbGnssLocInfoNotif->mutable_bodyframedata();
    if (nullptr != gnssLocPosDyn) {
        if (convertGnssLocationPositionDynamicsToPB(gnssLocInfoNotif.bodyFrameData,
                gnssLocInfoNotif.bodyFrameDataExt, gnssLocPosDyn)) {
            LOC_LOGe("convertGnssLocationPositionDynamicsToPB failed");
            free(gnssLocPosDyn);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_bodyFrameData failed");
        return 1;
    }

    // PBLocApiGnssSystemTime gnssSystemTime = 27;
    PBLocApiGnssSystemTime* gnssSysTime = pbGnssLocInfoNotif->mutable_gnsssystemtime();
    if (nullptr != gnssSysTime) {
        if (convertGnssSystemTimeToPB(gnssLocInfoNotif.gnssSystemTime, gnssSysTime)) {
            LOC_LOGe("convertGnssSystemTimeToPB failed");
            free(gnssSysTime);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_gnsssystemtime failed");
        return 1;
    }

    // uint32 numOfMeasReceived = 28;
    pbGnssLocInfoNotif->set_numofmeasreceived(gnssLocInfoNotif.numOfMeasReceived);

    // repeated PBGnssMeasUsageInfo measUsageInfo = 29; (Max array len - GNSS_SV_MAX)
    uint8_t count = gnssLocInfoNotif.numOfMeasReceived;
    LOC_LOGv("LocApiPB: gnssLocInfoNotif numOfMeasReceived : %u", count);
    for (uint8_t iter = 0; iter < count; iter++) {
        PBGnssMeasUsageInfo *gnssMeasUsageInfo = pbGnssLocInfoNotif->add_measusageinfo();
        if (nullptr != gnssMeasUsageInfo) {
            if (convertGnssMeasUsageInfoToPB(gnssLocInfoNotif.measUsageInfo[iter],
                    gnssMeasUsageInfo)) {
                LOC_LOGe("convertGnssLocInfoNotifToPB failed");
                free(gnssMeasUsageInfo);
                return 1;
            }
        } else {
            LOC_LOGe("add_measusageinfo is NULL");
            return 1;
        }
    }

    // uint32 leapSeconds = 30;
    pbGnssLocInfoNotif->set_leapseconds(gnssLocInfoNotif.leapSeconds);

    // float timeUncMs = 31;
    pbGnssLocInfoNotif->set_timeuncms(gnssLocInfoNotif.location.timeUncMs);

    // uint32 calibrationConfidence = 32;
    pbGnssLocInfoNotif->set_calibrationconfidence(gnssLocInfoNotif.calibrationConfidence);

    // uint32 calibrationStatus = 33; - PBLocApiDrCalibrationStatusMask
    pbGnssLocInfoNotif->set_calibrationstatus(
            getPBMaskForDrCalibrationStatusMask(gnssLocInfoNotif.calibrationStatus));

    // PBLocApiOutputEngineType locOutputEngType = 34;
    pbGnssLocInfoNotif->set_locoutputengtype(
            getPBEnumForLocOutputEngineType(gnssLocInfoNotif.locOutputEngType));

    // uint32 locOutputEngMask = 35; - bitwise OR of PBLocApiPositioningEngineMask
    pbGnssLocInfoNotif->set_locoutputengmask(
            getPBMaskForPositioningEngineMask(gnssLocInfoNotif.locOutputEngMask));

    // float conformityIndex = 36;
    pbGnssLocInfoNotif->set_conformityindex(gnssLocInfoNotif.conformityIndex);

    // PBLLAInfo llaVRPBased = 37;
    PBLLAInfo* llaInfo = pbGnssLocInfoNotif->mutable_llavrpbased();
    if (nullptr != llaInfo) {
        if (convertLLAInfoToPB(gnssLocInfoNotif.llaVRPBased, llaInfo)) {
            LOC_LOGe("convertLLAInfoToPB failed");
            free(llaInfo);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_llavrpbased failed");
        return 1;
    }

    // repeated float enuVelocityVRPBased = 38; - Max array length 3
    for (int i = 0; i < 3; i++) {
        LOC_LOGv("LocApiPB: gnssLocInfoNotif - jammerInd: %lf",
                gnssLocInfoNotif.enuVelocityVRPBased[i]);
        pbGnssLocInfoNotif->add_enuvelocityvrpbased(gnssLocInfoNotif.enuVelocityVRPBased[i]);
    }

    // uint32 drSolutionStatusMask = 39; - PBDrSolutionStatusMask
    pbGnssLocInfoNotif->set_drsolutionstatusmask(getPBMaskForDrSolutionStatusMask(
            (uint32_t)gnssLocInfoNotif.drSolutionStatusMask));

    // uint32 extFlags = 40;
    // bitwise OR of PBGnssLocationInfoExtFlagMask for fields 41 and onwards
    pbGnssLocInfoNotif->set_extflags(
            getPBMaskForGnssLocationInfoExtFlagMask(gnssLocInfoNotif.flags));

    // bool altitudeAssumed = 41;
    pbGnssLocInfoNotif->set_altitudeassumed(gnssLocInfoNotif.altitudeAssumed);

    // bool sessionStatus = 42;
    pbGnssLocInfoNotif->set_sessionstatus(
            getPBEnumForLocSessionStatus(gnssLocInfoNotif.sessionStatus));

    // uint32 integrityRiskUsed = 43;
    pbGnssLocInfoNotif->set_integrityriskused(gnssLocInfoNotif.integrityRiskUsed);
    // float    protectAlongTrack = 44;
    pbGnssLocInfoNotif->set_protectalongtrack(gnssLocInfoNotif.protectAlongTrack);
    // float    protectCrossTrack = 45;
    pbGnssLocInfoNotif->set_protectcrosstrack(gnssLocInfoNotif.protectCrossTrack);
    // float    protectVertical = 46;
    pbGnssLocInfoNotif->set_protectvertical(gnssLocInfoNotif.protectVertical);
    // repeated uint32 dgnssStationId = 47;
    for (uint32_t iter = 0; iter < gnssLocInfoNotif.numOfDgnssStationId; iter++) {
        pbGnssLocInfoNotif->add_dgnssstationid(gnssLocInfoNotif.dgnssStationId[iter]);
    }
    // double baseLineLength = 48;
    pbGnssLocInfoNotif->set_baselinelength(gnssLocInfoNotif.baseLineLength);
    // uint64 ageMsecOfCorrections = 49;
    pbGnssLocInfoNotif->set_agemsecofcorrections(gnssLocInfoNotif.ageMsecOfCorrections);
    // uint32    leapSecondsUnc = 50;
    pbGnssLocInfoNotif->set_leapsecondsunc(gnssLocInfoNotif.leapSecondsUnc);

    LocApiPb_LOGv("LocApiPB: gnssLocInfoNotif - GLocInfoFlgMask:%" PRIu64", pdop:%f, hdop:%f, "
            "vdop:%f",
            gnssLocInfoNotif.flags, gnssLocInfoNotif.pdop, gnssLocInfoNotif.hdop,
            gnssLocInfoNotif.vdop);
    LocApiPb_LOGv("LocApiPB: gnssLocInfoNotif - HorReliab:%d, VerReliab:%d, HorUnc-SemiMajor:%f "
            "SemiMinor:%f",
            gnssLocInfoNotif.horReliability, gnssLocInfoNotif.verReliability,
            gnssLocInfoNotif.horUncEllipseSemiMajor, gnssLocInfoNotif.horUncEllipseSemiMinor);
    LOC_LOGv("LocApiPB: gnssLocInfoNotif - NavSolMask:%x, NumMeasRcvd:%u, "\
            "LocOpEngType:%d, PosEngMask:%x,  NumSvUsedInPos:%u",
            gnssLocInfoNotif.navSolutionMask, gnssLocInfoNotif.numOfMeasReceived,
            gnssLocInfoNotif.locOutputEngType, gnssLocInfoNotif.locOutputEngMask,
            gnssLocInfoNotif.numSvUsedInPosition);
    return 0;
}

int LocationApiPbMsgConv::convertLocSysInfoToPB(const LocationSystemInfo &locSysInfo,
        PBLocationSystemInfo *pbLocSysInfo) const {
    if (nullptr == pbLocSysInfo) {
        LOC_LOGe("pbLocSysInfo is NULL!, return");
        return 1;
    }
    LOC_LOGv("LocApiPB: locSysInfo - sysInfoMask: %x", locSysInfo.systemInfoMask);
    // uint32 systemInfoMask = 1; - bitwise OR of PBLocationSystemInfoMask
    pbLocSysInfo->set_systeminfomask(getPBMaskForLocSysInfoMask(locSysInfo.systemInfoMask));

    // PBLeapSecondSystemInfo  leapSecondSysInfo = 2;
    PBLeapSecondSystemInfo* leapSecSysInfo = pbLocSysInfo->mutable_leapsecondsysinfo();
    if (nullptr != leapSecSysInfo) {
        if (convertLeapSecondSystemInfoToPB(locSysInfo.leapSecondSysInfo, leapSecSysInfo)) {
            LOC_LOGe("convertLeapSecondSystemInfoToPB failed");
            free(leapSecSysInfo);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_leapsecondsysinfo failed");
        return 1;
    }
    return 0;
}

// Disaster and crisis report
int LocationApiPbMsgConv::convertGnssDcReportToPB(
        const GnssDcReportInfo &dcReportInfo,
        PBGnssDcReportInfo *pbDcReportInfo) const {

    if (nullptr == pbDcReportInfo) {
        LOC_LOGe("pbDcReportInfo is NULL!, return");
        return 1;
    }
    LOC_LOGv("LocApiPB: dc type %d, num bits %d, num bytes %zu",
             dcReportInfo.dcReportType, dcReportInfo.numValidBits,
             dcReportInfo.dcReportData.size());

    pbDcReportInfo->set_dcreporttype(getPBEnumForDcReportType(dcReportInfo.dcReportType));
    pbDcReportInfo->set_numvalidbits(dcReportInfo.numValidBits);
    // repeated uint32
    for (uint32_t i = 0; i < dcReportInfo.dcReportData.size(); i++) {
        pbDcReportInfo->add_dcreportdata((uint32_t)dcReportInfo.dcReportData[i]);
    }
    return 0;
}


int LocationApiPbMsgConv::pbConvertToDcReport(
        const PBGnssDcReportInfo & pbDcReportInfo, GnssDcReportInfo & dcReportInfo) const {

    dcReportInfo.dcReportType = getDcReportTypeFromPB(pbDcReportInfo.dcreporttype());
    dcReportInfo.numValidBits = pbDcReportInfo.numvalidbits();
    for (uint32_t i = 0; i < pbDcReportInfo.dcreportdata_size(); i++) {
        dcReportInfo.dcReportData.push_back((uint8_t) (pbDcReportInfo.dcreportdata(i)));
    }
    return 0;
}

int LocationApiPbMsgConv::convertGnssMeasNotifToPB(
        const GnssMeasurementsNotification &gnssMeasNotif,
        PBGnssMeasurementsNotification *pbGnssMeasNotif) const {
    if (nullptr == pbGnssMeasNotif) {
        LOC_LOGe("pbGnssMeasNotif is NULL!, return");
        return 1;
    }

    // repeated PBGnssMeasurementsData measurements = 1; Max array len - GNSS_MEASUREMENTS_MAX
    uint32_t count = gnssMeasNotif.count;
    LOC_LOGv("LocApiPB: gnssMeasNotif - MeasNotif count:%d, isNhz:%d", count, gnssMeasNotif.isNhz);
    for (int i=0; i < count; i++) {
        PBGnssMeasurementsData* gnssMeasData = pbGnssMeasNotif->add_measurements();
        if (nullptr != gnssMeasData) {
            if (convertGnssMeasDataToPB(gnssMeasNotif.measurements[i], gnssMeasData)) {
                LOC_LOGe("convertGnssMeasDataToPB failed");
                free(gnssMeasData);
                return 1;
            }
        } else {
            LOC_LOGe("add_measurements failed");
            return 1;
        }
    }

    // PBGnssMeasurementsClock clock = 2;
    PBGnssMeasurementsClock* gnssMeasClock = pbGnssMeasNotif->mutable_clock();
    if (nullptr != gnssMeasClock) {
        if (convertGnssMeasClockToPB(gnssMeasNotif.clock, gnssMeasClock)) {
            LOC_LOGe("convertGnssMeasClockToPB failed");
            free(gnssMeasClock);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_clock failed");
        return 1;
    }

    // bool isNhz = 3;
    pbGnssMeasNotif->set_isnhz(gnssMeasNotif.isNhz);
    // PBAgcStatus agcStatusL1 = 4;
    pbGnssMeasNotif->set_agcstatusl1(getPBEnumForAgcStatus(gnssMeasNotif.agcStatusL1));
    // PBAgcStatus agcStatusL2 = 5;
    pbGnssMeasNotif->set_agcstatusl2(getPBEnumForAgcStatus(gnssMeasNotif.agcStatusL2));
    // PBAgcStatus agcStatusL5 = 6;
    pbGnssMeasNotif->set_agcstatusl5(getPBEnumForAgcStatus(gnssMeasNotif.agcStatusL5));

    return 0;
}

int LocationApiPbMsgConv::convertGnssDataNotifToPB(const GnssDataNotification &gnssDataNotif,
        PBGnssDataNotification *pbGnssDataNotif) const {
    if (nullptr == pbGnssDataNotif) {
        LOC_LOGe("pbGnssDataNotif is NULL!, return");
        return 1;
    }
    // uint32 numberSignalTypes = 1
    pbGnssDataNotif->set_numbersignaltypes((uint32_t)GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES);

    int i=0;
    // bitwise OR of GnssDataMask. Max array len - GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES
    // repeated uint64  gnssDataMask = 2;
    // repeated double     jammerInd = 3;
    // repeated double     agc = 4;
    for (i = 0; i < GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES; i++) {
        LocApiPb_LOGd("LocApiPB: gnssDataNotif - jammerInd: %lf, agc: %lf, gnssDataMask: %" PRIu64,
                gnssDataNotif.jammerInd[i], gnssDataNotif.agc[i], gnssDataNotif.gnssDataMask[i]);
        pbGnssDataNotif->add_gnssdatamask(gnssDataNotif.gnssDataMask[i]);
        pbGnssDataNotif->add_jammerind(gnssDataNotif.jammerInd[i]);
        pbGnssDataNotif->add_agc(gnssDataNotif.agc[i]);
    }
    // PBAgcStatus agcStatusL1 = 5;
    pbGnssDataNotif->set_agcstatusl1(getPBEnumForAgcStatus(gnssDataNotif.agcStatusL1));
    // PBAgcStatus agcStatusL2 = 6;
    pbGnssDataNotif->set_agcstatusl2(getPBEnumForAgcStatus(gnssDataNotif.agcStatusL2));
    // PBAgcStatus agcStatusL5 = 7;
    pbGnssDataNotif->set_agcstatusl5(getPBEnumForAgcStatus(gnssDataNotif.agcStatusL5));
    return 0;
}

int LocationApiPbMsgConv::convertLocAPINmeaSerializedPayloadToPB(
        const LocAPINmeaSerializedPayload &locAPINmeaSerPload,
        PBLocAPINmeaSerializedPayload *pbLocAPINmeaSerPload) const {
    if (nullptr == pbLocAPINmeaSerPload) {
        LOC_LOGe("pbLocAPINmeaSerPload is NULL!, return");
        return 1;
    }
    // uint64 timestamp = 1;
    pbLocAPINmeaSerPload->set_timestamp(locAPINmeaSerPload.timestamp);
    // string nmea = 2;
    pbLocAPINmeaSerPload->set_nmea(locAPINmeaSerPload.nmea);
    // PBLocApiOutputEngineType = 3;
    pbLocAPINmeaSerPload->set_locoutputengtype(
            getPBEnumForLocOutputEngineType(locAPINmeaSerPload.locOutputEngType));

    LocApiPb_LOGv("LocApiPB: locAPINmeaSerPload - Timestamp: %" PRIu64 "locOutputEngType=%d",
            locAPINmeaSerPload.timestamp, locAPINmeaSerPload.locOutputEngType );
    return 0;
}

int LocationApiPbMsgConv::convertGnssSvNotifToPB(const GnssSvNotification &gnssSvNotif,
        PBLocApiGnssSvNotification *pbGnssSvNotif) const {
    if (nullptr == pbGnssSvNotif) {
        LOC_LOGe("pbGnssSvNotif is NULL!, return");
        return 1;
    }

    // bool gnssSignalTypeMaskValid = 1;
    pbGnssSvNotif->set_gnsssignaltypemaskvalid(gnssSvNotif.gnssSignalTypeMaskValid);

    // repeated PBLocApiGnssSv gnssSvs = 2; (max - GNSS_SV_MAX)
    uint32_t count = gnssSvNotif.count;
    LOC_LOGv("LocApiPB: gnssSvNotif - SvNotif count:%d", count);
    for (int i=0; i < count; i++) {
        PBLocApiGnssSv* gnssSv = pbGnssSvNotif->add_gnsssvs();
        if (nullptr != gnssSv) {
            if (convertGnssSvToPB(gnssSvNotif.gnssSvs[i], gnssSv)) {
                LOC_LOGe("convertGnssSvToPB failed");
                free(gnssSv);
                return 1;
            }
        } else {
            LOC_LOGe("add_gnsssvs failed");
            return 1;
        }
    }
    return 0;
}

int LocationApiPbMsgConv::convertLeverArmParamsToPB(const LeverArmParams &leverArmParams,
        PBLIALeverArmParams *pbLeverArmParams) const {
    if (nullptr == pbLeverArmParams) {
        LOC_LOGe("pbLeverArmParams is NULL!, return");
        return 1;
    }
    // float forwardOffsetMeters = 1;
    pbLeverArmParams->set_forwardoffsetmeters(leverArmParams.forwardOffsetMeters);

    // float sidewaysOffsetMeters = 2;
    pbLeverArmParams->set_sidewaysoffsetmeters(leverArmParams.sidewaysOffsetMeters);

    // float upOffsetMeters = 3;
    pbLeverArmParams->set_upoffsetmeters(leverArmParams.upOffsetMeters);

    LOC_LOGv("LocApiPB: leverArmParams: Offset - Fwd: %f, Side: %f, Up: %f",
            leverArmParams.forwardOffsetMeters, leverArmParams.sidewaysOffsetMeters,
            leverArmParams.upOffsetMeters);
    return 0;
}

int LocationApiPbMsgConv::convertLeapSecondSystemInfoToPB(
        const LeapSecondSystemInfo &leapSecSysInfo,
        PBLeapSecondSystemInfo *pbLeapSecSysInfo) const {
    if (nullptr == pbLeapSecSysInfo) {
        LOC_LOGe("pbLeapSecSysInfo is NULL!, return");
        return 1;
    }

    // uint32  leapSecondInfoMask = 1; - bitwise OR of PBLeapSecondSysInfoMask
    pbLeapSecSysInfo->set_leapsecondinfomask(
            getPBMaskForLeapSecondSysInfoMask(leapSecSysInfo.leapSecondInfoMask));

    // uint32  leapSecondCurrent = 2;
    pbLeapSecSysInfo->set_leapsecondcurrent(leapSecSysInfo.leapSecondCurrent);

    // PBLeapSecondChangeInfo  leapSecondChangeInfo = 3;
    PBLeapSecondChangeInfo* leapSecChngInfo = pbLeapSecSysInfo->mutable_leapsecondchangeinfo();
    if (nullptr != leapSecChngInfo) {
        if (convertLeapSecChgInfoToPB(leapSecSysInfo.leapSecondChangeInfo, leapSecChngInfo)) {
            LOC_LOGe("convertLeapSecChgInfoToPB failed");
            free(leapSecChngInfo);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_leapsecondchangeinfo failed");
        return 1;
    }

    LOC_LOGv("LocApiPB: leapSecSysInfo - LeapSecondInfoMask:%x, LeapSecCurr: %u",
            leapSecSysInfo.leapSecondInfoMask, leapSecSysInfo.leapSecondCurrent);
    return 0;
}

int LocationApiPbMsgConv::convertLeapSecChgInfoToPB(const LeapSecondChangeInfo &leapSecChngInfo,
        PBLeapSecondChangeInfo *pbLeapSecChngInfo) const {
    if (nullptr == pbLeapSecChngInfo) {
        LOC_LOGe("pbLeapSecChngInfo is NULL!, return");
        return 1;
    }
    // PBLocApiGnssSystemTimeStructType gpsTimestampLsChange = 1;
    PBLocApiGnssSystemTimeStructType* pLeapSecChngInfo =
            pbLeapSecChngInfo->mutable_gpstimestamplschange();
    if (nullptr != pLeapSecChngInfo) {
        if (convertGnssSystemTimeStructTypeToPB(leapSecChngInfo.gpsTimestampLsChange,
                pLeapSecChngInfo)) {
            LOC_LOGe("convertGnssSystemTimeStructTypeToPB failed");
            free(pLeapSecChngInfo);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_gpstimestamplschange failed");
        return 1;
    }

    // uint32 leapSecondsBeforeChange = 2;
    pbLeapSecChngInfo->set_leapsecondsbeforechange(leapSecChngInfo.leapSecondsBeforeChange);

    // uint32 leapSecondsAfterChange = 3;
    pbLeapSecChngInfo->set_leapsecondsafterchange(leapSecChngInfo.leapSecondsAfterChange);

    LocApiPb_LOGd("LocApiPB: leapSecChngInfo - LeapSecs Before: %u, After:%u",
            leapSecChngInfo.leapSecondsBeforeChange, leapSecChngInfo.leapSecondsAfterChange);
    return 0;
}

int LocationApiPbMsgConv::convertGnssSystemTimeStructTypeToPB(
        const GnssSystemTimeStructType &gnssSysTimeStructType,
        PBLocApiGnssSystemTimeStructType *pbGnssSysTimeStructType) const {
    if (nullptr == pbGnssSysTimeStructType) {
        LOC_LOGe("pbGnssSysTimeStructType is NULL!, return");
        return 1;
    }
    // uint32 validityMask = 1; - PBLocApiGnssSystemTimeStructTypeFlags
    pbGnssSysTimeStructType->set_validitymask(
            getPBMaskForGnssSystemTimeStructTypeFlags(gnssSysTimeStructType.validityMask));

    // uint32 systemWeek = 2;
    pbGnssSysTimeStructType->set_systemweek(gnssSysTimeStructType.systemWeek);

    // uint32 systemMsec = 3;
    pbGnssSysTimeStructType->set_systemmsec(gnssSysTimeStructType.systemMsec);

    // float systemClkTimeBiasMs = 4;
    pbGnssSysTimeStructType->set_systemclktimebiasms(gnssSysTimeStructType.systemClkTimeBias);

    // float systemClkTimeBiasUncMs = 5;
    pbGnssSysTimeStructType->set_systemclktimebiasuncms(gnssSysTimeStructType.systemClkTimeUncMs);

    // uint32 refFCount = 6;
    pbGnssSysTimeStructType->set_reffcount(gnssSysTimeStructType.refFCount);

    // uint32 numClockResets = 7;
    pbGnssSysTimeStructType->set_numclockresets(gnssSysTimeStructType.numClockResets);

    LocApiPb_LOGv("LocApiPB: gnssSysTimeStruct - ValidityMsk: %x, SysWeek: %u, SysMsec: %u, "\
            "SysClkTimeBias: %f", gnssSysTimeStructType.validityMask,
            gnssSysTimeStructType.systemWeek, gnssSysTimeStructType.systemMsec,
            gnssSysTimeStructType.systemClkTimeBias);
    LocApiPb_LOGv("LocApiPB: gnssSysTimeStruct - SysClkTimeUnc: %f, RefCnt: %u, NumClkReset:%u",
            gnssSysTimeStructType.systemClkTimeUncMs, gnssSysTimeStructType.refFCount,
            gnssSysTimeStructType.numClockResets);
    return 0;
}

int LocationApiPbMsgConv::convertGnssGloTimeStructTypeToPB(
        const GnssGloTimeStructType &gnssGloTime,
        PBLocApiGnssGloTimeStructType *pbgnssGloTime) const {
    if (nullptr == pbgnssGloTime) {
        LOC_LOGe("pbgnssGloTime is NULL!, return");
        return 1;
    }
    // uint32 validityMask = 1; - Bitwise OR of PBGnssGloTimeStructTypeFlags
    pbgnssGloTime->set_validitymask(
            getPBMaskForGnssGloTimeStructTypeFlags(gnssGloTime.validityMask));

    // uint32 gloFourYear = 2;
    pbgnssGloTime->set_glofouryear(gnssGloTime.gloFourYear);

    // uint32 gloDays = 3;
    pbgnssGloTime->set_glodays(gnssGloTime.gloDays);

    // uint32 gloMsec = 4;
    pbgnssGloTime->set_glomsec(gnssGloTime.gloMsec);

    // float gloClkTimeBias = 5;
    pbgnssGloTime->set_gloclktimebias(gnssGloTime.gloClkTimeBias);

    // float gloClkTimeUncMs = 6;
    pbgnssGloTime->set_gloclktimeuncms(gnssGloTime.gloClkTimeUncMs);

    // uint32  refFCount = 7;
    pbgnssGloTime->set_reffcount(gnssGloTime.refFCount);

    // uint32 numClockResets = 8;
    pbgnssGloTime->set_numclockresets(gnssGloTime.numClockResets);

    LOC_LOGv("LocApiPB: gnssGloTime - GloValidityMsk: %x, GloFourYear: %u, GloDays: %u, "\
            "GloMsec:%u", gnssGloTime.validityMask, gnssGloTime.gloFourYear, gnssGloTime.gloDays,
             gnssGloTime.gloMsec);
    LOC_LOGv("LocApiPB: gnssGloTime - GloClkTimeBias: %f, GloClkTimeUnc: %f, RefFCnt: %u, "\
            "NumClkReset:%u", gnssGloTime.gloClkTimeBias, gnssGloTime.gloClkTimeUncMs,
            gnssGloTime.refFCount, gnssGloTime.numClockResets);
    return 0;
}

int LocationApiPbMsgConv::convertGnssMeasDataToPB(const GnssMeasurementsData &gnssMeasData,
        PBGnssMeasurementsData *pbGnssMeasData) const {
    if (nullptr == pbGnssMeasData) {
        LOC_LOGe("pbGnssMeasData is NULL!, return");
        return 1;
    }
    // uint32 flags = 1; - bitwise OR of PBGnssMeasurementsDataFlagsMask
    pbGnssMeasData->set_flags(getPBMaskForGnssMeasurementsDataFlagsMask(gnssMeasData.flags));

    // int32 svId = 2;
    pbGnssMeasData->set_svid(gnssMeasData.svId);

    // Use Gnss_LocSvSystemEnumType instead of GnssSvType
    // PBLocApiGnss_LocSvSystemEnumType svType = 3;
    pbGnssMeasData->set_svtype(getPBGnssLocSvSysEnumFromGnssSvType(gnssMeasData.svType));

    // double timeOffsetNs = 4;
    pbGnssMeasData->set_timeoffsetns(gnssMeasData.timeOffsetNs);

    // uint32 stateMask = 5; - bitwise OR of PBGnssMeasurementsStateMask
    pbGnssMeasData->set_statemask(getPBMaskForGnssMeasurementsStateMask(gnssMeasData.stateMask));

    // int64 receivedSvTimeNs = 6;
    pbGnssMeasData->set_receivedsvtimens(gnssMeasData.receivedSvTimeNs);

    // int64 receivedSvTimeUncertaintyNs = 7;
    pbGnssMeasData->set_receivedsvtimeuncertaintyns(gnssMeasData.receivedSvTimeUncertaintyNs);

    // double carrierToNoiseDbHz = 8;
    pbGnssMeasData->set_carriertonoisedbhz(gnssMeasData.carrierToNoiseDbHz);

    // double pseudorangeRateMps = 9;
    pbGnssMeasData->set_pseudorangeratemps(gnssMeasData.pseudorangeRateMps);

    // double pseudorangeRateUncertaintyMps = 10;
    pbGnssMeasData->set_pseudorangerateuncertaintymps(gnssMeasData.pseudorangeRateUncertaintyMps);

    // uint32 adrStateMask = 11; - bitwise OR of PBGnssMeasurementsAdrStateMask
    pbGnssMeasData->set_adrstatemask(
            getPBMaskForGnssMeasurementsAdrStateMask(gnssMeasData.adrStateMask));

    // double adrMeters = 12;
    pbGnssMeasData->set_adrmeters(gnssMeasData.adrMeters);

    // double adrUncertaintyMeters = 13;
    pbGnssMeasData->set_adruncertaintymeters(gnssMeasData.adrUncertaintyMeters);

    // float carrierFrequencyHz = 14;
    pbGnssMeasData->set_carrierfrequencyhz(gnssMeasData.carrierFrequencyHz);

    // int64 carrierCycles = 15;
    pbGnssMeasData->set_carriercycles(gnssMeasData.carrierCycles);

    // double carrierPhase = 16;
    pbGnssMeasData->set_carrierphase(gnssMeasData.carrierPhase);

    // double carrierPhaseUncertainty = 17;
    pbGnssMeasData->set_carrierphaseuncertainty(gnssMeasData.carrierPhaseUncertainty);

    // PBGnssMeasurementsMultipathIndicator multipathIndicator = 18;
    pbGnssMeasData->set_multipathindicator(
            getPBEnumForGnssMeasMultiPathIndic(gnssMeasData.multipathIndicator));

    // double signalToNoiseRatioDb = 19;
    pbGnssMeasData->set_signaltonoiseratiodb(gnssMeasData.signalToNoiseRatioDb);

    // double agcLevelDb = 20;
    pbGnssMeasData->set_agcleveldb(gnssMeasData.agcLevelDb);

    // double basebandCarrierToNoiseDbHz = 21;
    pbGnssMeasData->set_basebandcarriertonoisedbhz(gnssMeasData.basebandCarrierToNoiseDbHz);

    // uint32 gnssSignalType = 22; - bitwise OR of  PBGnssSignalTypeMask
    pbGnssMeasData->set_gnsssignaltype(
            getPBMaskForGnssSignalTypeMask(gnssMeasData.gnssSignalType));

    // double fullInterSignalBiasNs = 23;
    pbGnssMeasData->set_fullintersignalbiasns(gnssMeasData.fullInterSignalBiasNs);

    // double fullInterSignalBiasUncertaintyNs = 24;
    pbGnssMeasData->set_fullintersignalbiasuncertaintyns(
            gnssMeasData.fullInterSignalBiasUncertaintyNs);

    // uint32 cycleSlipCount = 25;
    pbGnssMeasData->set_cycleslipcount(gnssMeasData.cycleSlipCount);

    // float receivedSvTimeSubNs = 26
    pbGnssMeasData->set_receivedsvtimesubns(gnssMeasData.receivedSvTimeSubNs);

    LOC_LOGv("LocApiPB: gnssMeasData - GnssMeasDataFlags:%x, Svid:%d, SvType:%d, StateMsk:%x, "\
            "RcvSvTime:%"  PRIu64", RcvSvTimeUnc:%" PRIu64", CNoDb:%lf", gnssMeasData.flags,
            gnssMeasData.svId, gnssMeasData.svType, gnssMeasData.stateMask,
            gnssMeasData.receivedSvTimeNs, gnssMeasData.receivedSvTimeUncertaintyNs,
            gnssMeasData.carrierToNoiseDbHz);

    LocApiPb_LOGv("LocApiPB: gnssMeasData - TimeOffset:%lf, PseuRngRt:%lf, PseuRngRtUnc:%lf,"\
            "AdrStateMask:%x, AdrMeters:%lf, AdrUncMeters:%lf, CarierFreq:%f, CarierCyc:%" PRIu64,
            gnssMeasData.timeOffsetNs, gnssMeasData.pseudorangeRateMps,
            gnssMeasData.pseudorangeRateUncertaintyMps, gnssMeasData.adrStateMask,
            gnssMeasData.adrMeters, gnssMeasData.adrUncertaintyMeters,
            gnssMeasData.carrierFrequencyHz, gnssMeasData.carrierCycles);

    LocApiPb_LOGv("LocApiPB: gnssMeasData - CarierPhase:%lf, CarierPhaseUnc:%lf, MultiPathInd:%d"\
            "CNoRatio:%lf, AgcLevel:%lf, BasebandCno:%lf", gnssMeasData.carrierPhase,
            gnssMeasData.carrierPhaseUncertainty, gnssMeasData.multipathIndicator,
            gnssMeasData.signalToNoiseRatioDb, gnssMeasData.agcLevelDb,
            gnssMeasData.basebandCarrierToNoiseDbHz);

    LocApiPb_LOGd("LocApiPB: gnssMeasData - GnssSignalType:%x, InterSigBiasNs:%lf, "\
            "InterSigBiasUncNs:%lf, cycleSlipCount:%d",
            gnssMeasData.gnssSignalType, gnssMeasData.fullInterSignalBiasNs,
            gnssMeasData.fullInterSignalBiasUncertaintyNs, gnssMeasData.cycleSlipCount);

    return 0;
}

int LocationApiPbMsgConv::convertGnssMeasClockToPB(const GnssMeasurementsClock &gnssMeasClock,
        PBGnssMeasurementsClock *pbGnssMeasClock) const {
    if (nullptr == pbGnssMeasClock) {
        LOC_LOGe("pbGnssMeasClock is NULL!, return");
        return 1;
    }
    // uint32 flags = 1; - bitwise OR of PBGnssMeasurementsClockFlagsMask
    pbGnssMeasClock->set_flags(getPBMaskForGnssMeasurementsClockFlagsMask(gnssMeasClock.flags));
    // int32 leapSecond = 2;
    pbGnssMeasClock->set_leapsecond(gnssMeasClock.leapSecond);
    // int64 timeNs = 3;
    pbGnssMeasClock->set_timens(gnssMeasClock.timeNs);
    // double timeUncertaintyNs = 4;
    pbGnssMeasClock->set_timeuncertaintyns(gnssMeasClock.timeUncertaintyNs);
    // int64 fullBiasNs = 5;
    pbGnssMeasClock->set_fullbiasns(gnssMeasClock.fullBiasNs);
    // double biasNs = 6;
    pbGnssMeasClock->set_biasns(gnssMeasClock.biasNs);
    // double biasUncertaintyNs = 7;
    pbGnssMeasClock->set_biasuncertaintyns(gnssMeasClock.biasUncertaintyNs);
    // double driftNsps = 8;
    pbGnssMeasClock->set_driftnsps(gnssMeasClock.driftNsps);
    // double driftUncertaintyNsps = 9;
    pbGnssMeasClock->set_driftuncertaintynsps(gnssMeasClock.driftUncertaintyNsps);
    // uint32 hwClockDiscontinuityCount= 10;
    pbGnssMeasClock->set_hwclockdiscontinuitycount(gnssMeasClock.hwClockDiscontinuityCount);
    // uint64 elapsedRealTime = 11;
    pbGnssMeasClock->set_elapsedrealtime(gnssMeasClock.elapsedRealTime);
    // uint64 elapsedRealTimeUnc = 12;
    pbGnssMeasClock->set_elapsedrealtimeunc(gnssMeasClock.elapsedRealTimeUnc);
    // uint64 elapsedgPTPTime = 13;
    pbGnssMeasClock->set_elapsedgptptime(gnssMeasClock.elapsedgPTPTime);
    // uint64 elapsedgPTPTimeUnc = 14;
    pbGnssMeasClock->set_elapsedgptptimeunc(gnssMeasClock.elapsedgPTPTimeUnc);

    LOC_LOGv("LocApiPB: gnssMeasClock - GnssMeasClockFlags:%x, leapSecond:%u, TimeNs:%" PRIu64\
        "TimeUnc:%lf FullBiasNs:%" PRIu64" BiasNs:%lf, BiasUncNs:%lf, DriftNs:%lf, DriftUncNs:%lf"
        "HwDiscCnt:%u, elapsedRealTime:%" PRIu64" elapsedRealTimeUnc: %" PRIu64\
        "elapsedgPTPTime:%" PRIu64" elapsedgPTPTimeUnc: %" PRIu64,
        gnssMeasClock.flags, gnssMeasClock.leapSecond, gnssMeasClock.timeNs,
        gnssMeasClock.timeUncertaintyNs, gnssMeasClock.fullBiasNs, gnssMeasClock.biasNs,
        gnssMeasClock.biasUncertaintyNs, gnssMeasClock.driftNsps,
        gnssMeasClock.driftUncertaintyNsps, gnssMeasClock.hwClockDiscontinuityCount,
        gnssMeasClock.elapsedRealTime, gnssMeasClock.elapsedRealTimeUnc,
        gnssMeasClock.elapsedgPTPTime, gnssMeasClock.elapsedgPTPTimeUnc);
    return 0;
}

int LocationApiPbMsgConv::convertGnssSvToPB(const GnssSv &gnssSv,
        PBLocApiGnssSv *pbGnssSv) const {
    if (nullptr == pbGnssSv) {
        LOC_LOGe("pbGnssSv is NULL!, return");
        return 1;
    }
    // uint32 svId = 1;
    pbGnssSv->set_svid(gnssSv.svId);

    // PBLocApiGnss_LocSvSystemEnumType type = 2; - Use Gnss_LocSvSystemEnumType
    // instead of GnssSvType
    pbGnssSv->set_type(getPBGnssLocSvSysEnumFromGnssSvType(gnssSv.type));

    // float cN0Dbhz = 3;
    pbGnssSv->set_cn0dbhz(gnssSv.cN0Dbhz);

    // float elevation = 4;
    pbGnssSv->set_elevation(gnssSv.elevation);

    // float azimuth = 5;
    pbGnssSv->set_azimuth(gnssSv.azimuth);

    // uint32 gnssSvOptionsMask = 6; - Bitwise OR of PBLocApiGnssSvOptionsMask
    pbGnssSv->set_gnsssvoptionsmask(getPBMaskForGnssSvOptionsMask(gnssSv.gnssSvOptionsMask));

    // float carrierFrequencyHz = 7;
    pbGnssSv->set_carrierfrequencyhz(gnssSv.carrierFrequencyHz);

    // uint32 gnssSignalTypeMask = 8;  - Bitwise OR of PBGnssSignalTypeMask
    pbGnssSv->set_gnsssignaltypemask(getPBMaskForGnssSignalTypeMask(gnssSv.gnssSignalTypeMask));

    // double basebandCarrierToNoiseDbHz = 9; // Baseband signal strength Db Hz.
    pbGnssSv->set_basebandcarriertonoisedbhz(gnssSv.basebandCarrierToNoiseDbHz);

    // uint32 gloFrequency = 10;
    pbGnssSv->set_glofrequency(gnssSv.gloFrequency);

    LocApiPb_LOGv("LocApiPB: gnssSv - SvId:%d, SvType:%d, CNo:%f, Elev:%f, Azi:%f, SvOptMask:%x, "\
            "CarrierFreq:%f, SignalTypeMask:%x, gloFrequency:%d",
            gnssSv.svId, gnssSv.type, gnssSv.cN0Dbhz, gnssSv.elevation,
            gnssSv.azimuth, gnssSv.gnssSvOptionsMask,
            gnssSv.carrierFrequencyHz, gnssSv.gnssSignalTypeMask, gnssSv.gloFrequency);
    return 0;
}

int LocationApiPbMsgConv::convertGnssLocSvUsedInPosToPB(
        const GnssLocationSvUsedInPosition &gnssLocSvUsedInPos,
        PBGnssLocationSvUsedInPosition *pbGnssLocSvUsedInPos) const {
    if (nullptr == pbGnssLocSvUsedInPos) {
        LOC_LOGe("pbGnssLocSvUsedInPos is NULL!, return");
        return 1;
    }
    // uint64 gpsSvUsedIdsMask = 1;
    pbGnssLocSvUsedInPos->set_gpssvusedidsmask(gnssLocSvUsedInPos.gpsSvUsedIdsMask);

    // uint64 gloSvUsedIdsMask = 2;
    pbGnssLocSvUsedInPos->set_glosvusedidsmask(gnssLocSvUsedInPos.gloSvUsedIdsMask);

    // uint64 galSvUsedIdsMask = 3;
    pbGnssLocSvUsedInPos->set_galsvusedidsmask(gnssLocSvUsedInPos.galSvUsedIdsMask);

    // uint64 bdsSvUsedIdsMask = 4;
    pbGnssLocSvUsedInPos->set_bdssvusedidsmask(gnssLocSvUsedInPos.bdsSvUsedIdsMask);

    // uint64 qzssSvUsedIdsMask = 5;
    pbGnssLocSvUsedInPos->set_qzsssvusedidsmask(gnssLocSvUsedInPos.qzssSvUsedIdsMask);

    // uint64 navicSvUsedIdsMask = 6;
    pbGnssLocSvUsedInPos->set_navicsvusedidsmask(gnssLocSvUsedInPos.navicSvUsedIdsMask);

    LOC_LOGv("LocApiPB: gnssLocSvUsedInPos - Gps:%" PRIu64", Glo:%" PRIu64", Gal:%" PRIu64\
            ", Bds:%" PRIu64", Qzss:%" PRIu64", Navic:%" PRIu64,
            gnssLocSvUsedInPos.gpsSvUsedIdsMask, gnssLocSvUsedInPos.gloSvUsedIdsMask,
            gnssLocSvUsedInPos.galSvUsedIdsMask, gnssLocSvUsedInPos.bdsSvUsedIdsMask,
            gnssLocSvUsedInPos.qzssSvUsedIdsMask, gnssLocSvUsedInPos.navicSvUsedIdsMask);
    return 0;
}

int LocationApiPbMsgConv::convertGnssSystemTimeToPB(const GnssSystemTime &gnssSysTime,
        PBLocApiGnssSystemTime *pbGnssSysTime) const {
    if (nullptr == pbGnssSysTime) {
        LOC_LOGe("pbGnssSysTime is NULL!, return");
        return 1;
    }
    // PBLocApiGnss_LocSvSystemEnumType gnssSystemTimeSrc = 1;
    pbGnssSysTime->set_gnsssystemtimesrc(
            getPBEnumForGnssLocSvSystem(gnssSysTime.gnssSystemTimeSrc));

    // PBLocApiSystemTimeStructUnion u = 2;
    PBLocApiSystemTimeStructUnion* sysTimeStructUnion = pbGnssSysTime->mutable_u();
    if (nullptr != sysTimeStructUnion) {
        if (convertSystemTimeStructUnionToPB(gnssSysTime.gnssSystemTimeSrc, gnssSysTime.u,
                sysTimeStructUnion)) {
            LOC_LOGe("convertSystemTimeStructUnionToPB failed");
            free(sysTimeStructUnion);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_u failed");
        return 1;
    }

    LocApiPb_LOGv("LocApiPB: gnssSysTime - SysTimeSrc:%d", gnssSysTime.gnssSystemTimeSrc);
    return 0;
}

int LocationApiPbMsgConv::convertGnssLocationPositionDynamicsToPB(
        const GnssLocationPositionDynamics &gnssLocPosDyn,
        const GnssLocationPositionDynamicsExt &gnssLocPosDynExt,
        PBLocApiGnssLocationPositionDynamics *pbGnssLocPosDyn) const {
    if (nullptr == pbGnssLocPosDyn) {
        LOC_LOGe("pbGnssLocPosDyn is NULL!, return");
        return 1;
    }
    // uint32  bodyFrameDataMask = 1; - Bitwise OR of PBLocApiGnssLocationPosDataMask
    pbGnssLocPosDyn->set_bodyframedatamask(
            getPBMaskForGnssLocationPosDataMask(
                gnssLocPosDyn.bodyFrameDataMask,
                gnssLocPosDynExt.bodyFrameDataMask));

    // float           longAccel = 2;
    // float           latAccel = 3;
    // float           vertAccel = 4;
    // float           longAccelUnc = 5;
    // float           latAccelUnc = 6;
    // float           vertAccelUnc = 7;
    pbGnssLocPosDyn->set_longaccel(gnssLocPosDyn.longAccel);
    pbGnssLocPosDyn->set_lataccel(gnssLocPosDyn.latAccel);
    pbGnssLocPosDyn->set_vertaccel(gnssLocPosDyn.vertAccel);
    pbGnssLocPosDyn->set_longaccelunc(gnssLocPosDyn.longAccelUnc);
    pbGnssLocPosDyn->set_lataccelunc(gnssLocPosDyn.latAccelUnc);
    pbGnssLocPosDyn->set_vertaccelunc(gnssLocPosDyn.vertAccelUnc);

    // float           pitch = 8;
    // float           pitchUnc = 9;
    // float           pitchRate = 10;
    // float           pitchRateUnc = 11;
    // float           roll = 12;
    // float           rollUnc = 13;
    // float           rollRate = 14;
    // float           rollRateUnc = 15;
    // float           yaw = 16;
    // float           yawUnc = 17;
    // float           yawRate = 18;
    // float           yawRateUnc = 19;
    pbGnssLocPosDyn->set_pitch(gnssLocPosDyn.pitch);
    pbGnssLocPosDyn->set_pitchunc(gnssLocPosDyn.pitchUnc);
    pbGnssLocPosDyn->set_pitchrate(gnssLocPosDynExt.pitchRate);
    pbGnssLocPosDyn->set_pitchrateunc(gnssLocPosDynExt.pitchRateUnc);
    pbGnssLocPosDyn->set_roll(gnssLocPosDynExt.roll);
    pbGnssLocPosDyn->set_rollunc(gnssLocPosDynExt.rollUnc);
    pbGnssLocPosDyn->set_rollrate(gnssLocPosDynExt.rollRate);
    pbGnssLocPosDyn->set_rollrateunc(gnssLocPosDynExt.rollRateUnc);
    pbGnssLocPosDyn->set_yaw(gnssLocPosDynExt.yaw);
    pbGnssLocPosDyn->set_yawunc(gnssLocPosDynExt.yawUnc);
    pbGnssLocPosDyn->set_yawrate(gnssLocPosDyn.yawRate);
    pbGnssLocPosDyn->set_yawrateunc(gnssLocPosDyn.yawRateUnc);

    LOC_LOGv("LocApiPB: gnssLocPosDyn - Mask:%x, MaskExt:%x, Accel-Long:%f Lat:%f Vert:%f",
            gnssLocPosDyn.bodyFrameDataMask, gnssLocPosDynExt.bodyFrameDataMask,
            gnssLocPosDyn.longAccel, gnssLocPosDyn.latAccel, gnssLocPosDyn.vertAccel);
    LocApiPb_LOGv("LocApiPB: Pitch:%f, PitchRate:%f, Roll:%f, RollRate:%f, Yaw:%f, YawRate:%f",
            gnssLocPosDyn.pitch, gnssLocPosDynExt.pitchRate, gnssLocPosDynExt.roll,
            gnssLocPosDynExt.rollRate, gnssLocPosDynExt.yaw, gnssLocPosDyn.yawRate);
    return 0;
}

int LocationApiPbMsgConv::convertLLAInfoToPB(const LLAInfo &llaInfo,
        PBLLAInfo *pbLlaInfo) const {
    if (nullptr == pbLlaInfo) {
        LOC_LOGe("pbLlaInfo is NULL!, return");
        return 1;
    }
    // double latitude = 1;
    pbLlaInfo->set_latitude(llaInfo.latitude);
    // double longitude = 2;
    pbLlaInfo->set_longitude(llaInfo.longitude);
    // float altitude = 3;
    pbLlaInfo->set_altitude(llaInfo.altitude);
    LOC_LOGv("LocApiPB: llaInfo - Lat:%lf, Lon:%lf, Alt:%f", llaInfo.latitude,
            llaInfo.longitude, llaInfo.altitude);
    return 0;
}

int LocationApiPbMsgConv::convertGnssMeasUsageInfoToPB(const GnssMeasUsageInfo &gnssMeasUsageInfo,
        PBGnssMeasUsageInfo *pbGnssMeasUsageInfo) const {
    if (nullptr == pbGnssMeasUsageInfo) {
        LOC_LOGe("pbGnssMeasUsageInfo is NULL!, return");
        return 1;
    }
    // PBLocApiGnss_LocSvSystemEnumType gnssConstellation = 1;
    pbGnssMeasUsageInfo->set_gnssconstellation(
            getPBEnumForGnssLocSvSystem(gnssMeasUsageInfo.gnssConstellation));

    // uint32 gnssSvId = 2;
    pbGnssMeasUsageInfo->set_gnsssvid(gnssMeasUsageInfo.gnssSvId);

    // uint32 gnssSignalType = 3; - bitwise OR of PBGnssSignalTypeMask
    pbGnssMeasUsageInfo->set_gnsssignaltype(
            getPBMaskForGnssSignalTypeMask(gnssMeasUsageInfo.gnssSignalType));

    LocApiPb_LOGv("LocApiPB: gnssMeasUsageInfo - Constl:%d, SvId:%d, SignalTypeMask:%x",
            gnssMeasUsageInfo.gnssConstellation, gnssMeasUsageInfo.gnssSvId,
            gnssMeasUsageInfo.gnssSignalType);
    return 0;
}

int LocationApiPbMsgConv::convertSystemTimeStructUnionToPB(
        const Gnss_LocSvSystemEnumType &gnssLocSvSysEnumType,
        const SystemTimeStructUnion &sysTimeStructUnion,
        PBLocApiSystemTimeStructUnion *pbSysTimeStructUnion) const {
    uint32_t retVal = 0;
    if (nullptr == pbSysTimeStructUnion) {
        LOC_LOGe("pbSysTimeStructUnion is NULL!, return");
        return 1;
    }

    switch (gnssLocSvSysEnumType) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            {
                // PBLocApiGnssSystemTimeStructType gpsSystemTime = 1;
                PBLocApiGnssSystemTimeStructType* gpsSysTime =
                        pbSysTimeStructUnion->mutable_gpssystemtime();
                if (nullptr != gpsSysTime) {
                    if (convertGnssSystemTimeStructTypeToPB(sysTimeStructUnion.gpsSystemTime,
                            gpsSysTime)) {
                        LOC_LOGe("convertGnssSystemTimeStructTypeToPB failed");
                        free(gpsSysTime);
                        retVal = 1;
                    }
                } else {
                    LOC_LOGe("mutable_gpssystemtime failed");
                    retVal = 1;
                }
            }
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            {
                // PBLocApiGnssSystemTimeStructType galSystemTime = 2;
                PBLocApiGnssSystemTimeStructType* galSysTime =
                        pbSysTimeStructUnion->mutable_galsystemtime();
                if (nullptr != galSysTime) {
                    if (convertGnssSystemTimeStructTypeToPB(sysTimeStructUnion.galSystemTime,
                            galSysTime)) {
                        LOC_LOGe("convertGnssSystemTimeStructTypeToPB failed");
                        free(galSysTime);
                        retVal = 1;
                    }
                } else {
                    LOC_LOGe("mutable_galsystemtime failed");
                    retVal = 1;
                }
            }
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            {
                // PBLocApiGnssSystemTimeStructType bdsSystemTime = 3;
                PBLocApiGnssSystemTimeStructType* bdsSysTime =
                        pbSysTimeStructUnion->mutable_bdssystemtime();
                if (nullptr != bdsSysTime) {
                    if (convertGnssSystemTimeStructTypeToPB(sysTimeStructUnion.bdsSystemTime,
                            bdsSysTime)) {
                        LOC_LOGe("convertGnssSystemTimeStructTypeToPB failed");
                        free(bdsSysTime);
                        retVal = 1;
                    }
                } else {
                    LOC_LOGe("mutable_bdssystemtime failed");
                    retVal = 1;
                }
            }
            break;
        case GNSS_LOC_SV_SYSTEM_QZSS:
            {
                // PBLocApiGnssSystemTimeStructType qzssSystemTime = 4;
                PBLocApiGnssSystemTimeStructType* qzssSysTime =
                        pbSysTimeStructUnion->mutable_qzsssystemtime();
                if (nullptr != qzssSysTime) {
                    if (convertGnssSystemTimeStructTypeToPB(sysTimeStructUnion.qzssSystemTime,
                            qzssSysTime)) {
                        LOC_LOGe("convertGnssSystemTimeStructTypeToPB failed");
                        free(qzssSysTime);
                        retVal = 1;
                    }
                } else {
                    LOC_LOGe("mutable_qzsssystemtime failed");
                    retVal = 1;
                }
            }
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            {
                // PBLocApiGnssGloTimeStructType gloSystemTime = 5;
                PBLocApiGnssGloTimeStructType* gloSysTime =
                        pbSysTimeStructUnion->mutable_glosystemtime();
                if (nullptr != gloSysTime) {
                    if (convertGnssGloTimeStructTypeToPB(sysTimeStructUnion.gloSystemTime,
                            gloSysTime)) {
                        LOC_LOGe("convertGnssGloTimeStructTypeToPB failed");
                        free(gloSysTime);
                        retVal = 1;
                    }
                } else {
                    LOC_LOGe("mutable_glosystemtime failed");
                    retVal = 1;
                }
            }
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            {
                // PBLocApiGnssSystemTimeStructType navicSystemTime = 6;
                PBLocApiGnssSystemTimeStructType* navicSysTime =
                        pbSysTimeStructUnion->mutable_navicsystemtime();
                if (nullptr != navicSysTime) {
                    if (convertGnssSystemTimeStructTypeToPB(sysTimeStructUnion.navicSystemTime,
                            navicSysTime)) {
                        LOC_LOGe("convertGnssSystemTimeStructTypeToPB failed");
                        free(navicSysTime);
                        retVal = 1;
                    }
                } else {
                    LOC_LOGe("mutable_navicsystemtime failed");
                    retVal = 1;
                }
            }
            break;
        default:
            retVal = 0;
            break;
    }
    LocApiPb_LOGv("LocApiPB: sysTimeStructUnion - gnssLocSvSysEnumType:%d, return %d",
            gnssLocSvSysEnumType, retVal);
    return retVal;
}

int LocationApiPbMsgConv::convertGeofenceInfoToPB(const GeofenceInfo &gfInfo,
        PBGeofenceInfo *pbGfInfo) const {
    if (nullptr == pbGfInfo) {
        LOC_LOGe("pbGfInfo is NULL!, return");
        return 1;
    }
    // double latitude = 1;
    pbGfInfo->set_latitude(gfInfo.latitude);
    // double longitude = 2;
    pbGfInfo->set_longitude(gfInfo.longitude);
    // double radius = 3;
    pbGfInfo->set_radius(gfInfo.radius);

    LOC_LOGv("LocApiPB: gfInfo - Lat:%lf, Lon:%lf, Rad:%lf", gfInfo.latitude, gfInfo.longitude,
            gfInfo.radius);
    return 0;
}

int LocationApiPbMsgConv::convertGeofenceOptionToPB(const GeofenceOption &gfOpt,
        PBGeofenceOption *pbGfOpt) const {
    if (nullptr == pbGfOpt) {
        LOC_LOGe("pbGfOpt is NULL!, return");
        return 1;
    }
    // bitwise OR of PBGeofenceBreachTypeMask
    // uint32 breachTypeMask = 1;
    pbGfOpt->set_breachtypemask(getPBMaskForGeofenceBreachTypeMask(gfOpt.breachTypeMask));
    // uint32 responsiveness = 2
    pbGfOpt->set_responsiveness(gfOpt.responsiveness);
    // uint32 dwellTime = 3;
    pbGfOpt->set_dwelltime(gfOpt.dwellTime);

    LOC_LOGv("LocApiPB: gfOpt - BreachTypeMask:%x Resp:%u, DwellTime:%u",
            gfOpt.breachTypeMask, gfOpt.responsiveness, gfOpt.dwellTime);
    return 0;
}

// **** helper function  to convert from protobuf struct to normal struct.
int LocationApiPbMsgConv::pbConvertToGnssConfigRobustLocation(
        const PBGnssConfigRobustLocation &pbGnssCfgRobLoc,
        GnssConfigRobustLocation &gnssCfgRobLoc) const {
    // uint32 validMask = 1; - bitwise OR of PBGnssConfigRobustLocationValidMask
    gnssCfgRobLoc.validMask = (GnssConfigRobustLocationValidMask)
            getGnssConfigRobustLocationValidMaskFromPB(pbGnssCfgRobLoc.validmask());

    // bool enabled = 2;
    gnssCfgRobLoc.enabled = pbGnssCfgRobLoc.enabled();

    // bool enabledForE911 = 3;
    gnssCfgRobLoc.enabledForE911 = pbGnssCfgRobLoc.enabledfore911();

    // PBGnssConfigRobustLocationVersion version = 4;
    // >> uint32 major = 1;
    // >> uint32 minor = 2;
    gnssCfgRobLoc.version.major = pbGnssCfgRobLoc.version().major();
    gnssCfgRobLoc.version.minor = pbGnssCfgRobLoc.version().minor();

    LOC_LOGv("LocApiPB: pbGnssCfgRobLoc - CfgRobustLocMask:%x, Enabled:%d, EnabForE911:%d, "\
           "MajorVer:%d, MinorVer:%d", gnssCfgRobLoc.validMask, gnssCfgRobLoc.enabled,
           gnssCfgRobLoc.enabledForE911, gnssCfgRobLoc.version.major, gnssCfgRobLoc.version.minor);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToCollectiveResPayload(
        const PBCollectiveResPayload &pbClctResPayload,
        CollectiveResPayload &clctResPayload) const {
    // repeated PBGeofenceResponse resp = 1;
    uint32_t count = pbClctResPayload.resp_size();
    LocApiPb_LOGd("LocApiPB: pbClctResPayload count:%" PRIu32"", count);
    for (int i=0; i < count; i++) {
        GeofenceResponse gfResp;
        gfResp.clientId = pbClctResPayload.resp(i).clientid();
        gfResp.error = getEnumForPBLocationError(pbClctResPayload.resp(i).error());
        clctResPayload.resp.push_back(gfResp);
        LocApiPb_LOGv("LocApiPB: pbClctResPayload clientId:%u, error:%d",
                gfResp.clientId, gfResp.error);
    }

    return 0;
}

int LocationApiPbMsgConv::pbConvertToLocation(const PBLocation &pbLoc, Location &loc) const {
    loc.size = sizeof(Location);

    // uint32 flags = 1; - Bitwise OR of PBLocationFlagsMask
    loc.flags = getLocationFlagsMaskFromPB(pbLoc.flags());

    // uint64 timestamp = 2;
    loc.timestamp = pbLoc.timestamp();

    // double latitude = 3;
    loc.latitude = pbLoc.latitude();

    // double longitude = 4;
    loc.longitude = pbLoc.longitude();

    // double altitude = 5;
    loc.altitude = pbLoc.altitude();

    // float speed = 6;
    loc.speed = pbLoc.speed();

    // float bearing = 7;
    loc.bearing = pbLoc.bearing();

    // float horizontalAccuracy = 8;
    loc.accuracy = pbLoc.horizontalaccuracy();

    // float verticalAccuracy = 9;
    loc.verticalAccuracy = pbLoc.verticalaccuracy();

    // float speedAccuracy = 10;
    loc.speedAccuracy = pbLoc.speedaccuracy();

    // float bearingAccuracy = 11;
    loc.bearingAccuracy = pbLoc.bearingaccuracy();

    // uint32 techMask = 12; - bitwise OR of PBLocationTechnologyMask
    loc.techMask = getLocationTechnologyMaskFromPB(pbLoc.techmask());

    // uint64 elapsedRealTime = 13;
    loc.elapsedRealTime = pbLoc.elapsedrealtime();

    // uint64 elapsedRealTimeUnc = 14;
    loc.elapsedRealTimeUnc = pbLoc.elapsedrealtimeunc();

    // float timeuncMs = 15;
    loc.timeUncMs = pbLoc.timeuncms();

    // uint64 elapsedgPTPTime  = 16;
    loc.elapsedgPTPTime = pbLoc.elapsedgptptime();
    // uint64 elapsedgPTPTimeUnc  = 17;
    loc.elapsedgPTPTimeUnc = pbLoc.elapsedgptptimeunc();

    // LocSessionStatus sessionStatus = 18;
    loc.sessionStatus = getLocSessionStatusFromPB(
            pbLoc.sessionstatus());

    LocApiPb_LOGd("LocApiPB: pbLoc - speed:%f, bearing:%f, HorzAcc:%f, VertAcc:%f, SpeedAcc:%f, "
            "BearAcc:%f, time unc ms %f, elapsedGPTPTime:%" PRIu64" , sessionStatus %d",
            loc.speed, loc.bearing, loc.accuracy, loc.verticalAccuracy, loc.speedAccuracy,
            loc.bearingAccuracy, loc.timeUncMs, loc.elapsedgPTPTime, (int)loc.sessionStatus);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLocAPIBatchNotification(
        const PBLocAPIBatchNotification &pbLocBatchNotif,
        LocAPIBatchNotification &locBatchNotif) const {
    // PBBatchingStatus status = 1;
    locBatchNotif.status = getEnumForPBBatchingStatus(pbLocBatchNotif.status());

    // repeated PBLocation location = 2;
    uint32_t count = pbLocBatchNotif.location_size();
    for (int i=0; i < count; i++) {
        Location batchLoc;
        pbConvertToLocation(pbLocBatchNotif.location(i), batchLoc);
        locBatchNotif.location.push_back(batchLoc);
    }

    LOC_LOGv("LocApiPB: pbLocBatchNotif - BatchStat: %d, Loc count:%u",
            locBatchNotif.status, count);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLocAPIGfBreachNotification(
            const PBLocAPIGeofenceBreachNotification &pbLocApiGfBreachNotif,
            LocAPIGeofenceBreachNotification &locApiGfBreachNotif) const {
    // uint64 timestamp = 1;
    locApiGfBreachNotif.timestamp = pbLocApiGfBreachNotif.timestamp();

    // uint32 breachtype = 2; - PBGeofenceBreachTypeMask
    locApiGfBreachNotif.type = getGfBreachTypeMaskFromPB(pbLocApiGfBreachNotif.breachtype());

    // PBLocation location = 3;
    pbConvertToLocation(pbLocApiGfBreachNotif.location(), locApiGfBreachNotif.location);

    // repeated uint32 id = 4;
    uint32_t gfBreachCnt = pbLocApiGfBreachNotif.id_size();
    LOC_LOGv("LocApiPB: gfBreachCnt: %" PRIu32"", gfBreachCnt);
    for (uint32_t i=0; i < gfBreachCnt; i++) {
        locApiGfBreachNotif.id.push_back(pbLocApiGfBreachNotif.id(i));
    }

    LOC_LOGv("LocApiPB: pbLocApiGfBreachNotif - BreachTypMask: %x, timestamp: %" PRIu64,
            locApiGfBreachNotif.type, locApiGfBreachNotif.timestamp);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssLocInfoNotif(
        const PBGnssLocationInfoNotification &pbGnssLocInfoNotif,
        GnssLocationInfoNotification &gnssLocInfoNotif) const {
    // PBLocation location = 1;
    pbConvertToLocation(pbGnssLocInfoNotif.location(), gnssLocInfoNotif.location);

    // uint64 flags = 2; -  bitwise OR of PBGnssLocationInfoFlagMask
    gnssLocInfoNotif.flags = getGnssLocationInfoFlagMaskFromPB(
            pbGnssLocInfoNotif.flags(), pbGnssLocInfoNotif.extflags());

    // float altitudeMeanSeaLevel = 3;
    gnssLocInfoNotif.altitudeMeanSeaLevel = pbGnssLocInfoNotif.altitudemeansealevel();

    // float pdop = 4;
    gnssLocInfoNotif.pdop = pbGnssLocInfoNotif.pdop();
    // float hdop = 5;
    gnssLocInfoNotif.hdop = pbGnssLocInfoNotif.hdop();
    // float vdop = 6;
    gnssLocInfoNotif.vdop = pbGnssLocInfoNotif.vdop();
    // float gdop = 7;
    gnssLocInfoNotif.gdop = pbGnssLocInfoNotif.gdop();
    // float tdop = 8;
    gnssLocInfoNotif.tdop = pbGnssLocInfoNotif.tdop();

    // float magneticDeviation = 9;
    gnssLocInfoNotif.magneticDeviation = pbGnssLocInfoNotif.magneticdeviation();

    // PBLocationReliability horReliability = 10;
    gnssLocInfoNotif.horReliability =
            getEnumForPBLocReliability(pbGnssLocInfoNotif.horreliability());
    // PBLocationReliability verReliability = 11;
    gnssLocInfoNotif.verReliability =
            getEnumForPBLocReliability(pbGnssLocInfoNotif.verreliability());

    // float horUncEllipseSemiMajor = 12;
    gnssLocInfoNotif.horUncEllipseSemiMajor = pbGnssLocInfoNotif.horuncellipsesemimajor();
    // float horUncEllipseSemiMinor = 13;
    gnssLocInfoNotif.horUncEllipseSemiMinor = pbGnssLocInfoNotif.horuncellipsesemiminor();
    // float horUncEllipseOrientAzimuth = 14;
    gnssLocInfoNotif.horUncEllipseOrientAzimuth = pbGnssLocInfoNotif.horuncellipseorientazimuth();

    // float northStdDeviation = 15;
    gnssLocInfoNotif.northStdDeviation = pbGnssLocInfoNotif.northstddeviation();
    // float eastStdDeviation = 16;
    gnssLocInfoNotif.eastStdDeviation = pbGnssLocInfoNotif.eaststddeviation();

    // float northVelocity = 17;
    gnssLocInfoNotif.northVelocity = pbGnssLocInfoNotif.northvelocity();
    // float eastVelocity = 18;
    gnssLocInfoNotif.eastVelocity = pbGnssLocInfoNotif.eastvelocity();
    // float upVelocity = 19;
    gnssLocInfoNotif.upVelocity = pbGnssLocInfoNotif.upvelocity();
    // float northVelocityStdDeviation = 20;
    gnssLocInfoNotif.northVelocityStdDeviation = pbGnssLocInfoNotif.northvelocitystddeviation();
    // float eastVelocityStdDeviation = 21;
    gnssLocInfoNotif.eastVelocityStdDeviation = pbGnssLocInfoNotif.eastvelocitystddeviation();
    // float upVelocityStdDeviation = 22;
    gnssLocInfoNotif.upVelocityStdDeviation = pbGnssLocInfoNotif.upvelocitystddeviation();

    // uint32 numSvUsedInPosition = 23;
    gnssLocInfoNotif.numSvUsedInPosition = pbGnssLocInfoNotif.numsvusedinposition();

    // PBGnssLocationSvUsedInPosition svUsedInPosition = 24;
    pbConvertToGnssLocationSvUsedInPosition(pbGnssLocInfoNotif.svusedinposition(),
            gnssLocInfoNotif.svUsedInPosition);

    // uint32 navSolutionMask = 25; - bitwise OR of PBGnssLocationNavSolutionMask
    gnssLocInfoNotif.navSolutionMask =
            getGnssLocationNavSolutionMaskFromPB(pbGnssLocInfoNotif.navsolutionmask());

    // PBLocApiGnssLocationPositionDynamics bodyFrameData = 26;
    // bodyFrameDataExt - Additional Body Frame Dynamics. Added to this too.
    pbConvertToGnssLocationPositionDynamics(pbGnssLocInfoNotif.bodyframedata(),
            gnssLocInfoNotif.bodyFrameData, gnssLocInfoNotif.bodyFrameDataExt);

    // PBLocApiGnssSystemTime gnssSystemTime = 27;
    pbConvertToGnssSystemTime(pbGnssLocInfoNotif.gnsssystemtime(),
            gnssLocInfoNotif.gnssSystemTime);

    // uint32 numOfMeasReceived = 28;
    gnssLocInfoNotif.numOfMeasReceived = min(pbGnssLocInfoNotif.numofmeasreceived(),
            (uint32_t)GNSS_SV_MAX);

    // repeated PBGnssMeasUsageInfo measUsageInfo = 29; Max array len - GNSS_SV_MAX
    for (uint32_t iter=0; iter < gnssLocInfoNotif.numOfMeasReceived; iter++) {
        pbConvertToGnssMeasUsageInfo(pbGnssLocInfoNotif.measusageinfo(iter),
                gnssLocInfoNotif.measUsageInfo[iter]);
    }

    // uint32 leapSeconds = 30;
    gnssLocInfoNotif.leapSeconds = pbGnssLocInfoNotif.leapseconds();

    // float timeUncMs = 31;
    gnssLocInfoNotif.location.timeUncMs = pbGnssLocInfoNotif.timeuncms();

    // uint32 calibrationConfidence = 32;
    gnssLocInfoNotif.calibrationConfidence = pbGnssLocInfoNotif.calibrationconfidence();

    // uint32 calibrationStatus = 33; -  PBLocApiDrCalibrationStatusMask
    gnssLocInfoNotif.calibrationStatus =
            getDrCalibrationStatusMaskFromPB(pbGnssLocInfoNotif.calibrationstatus());

    // PBLocApiOutputEngineType locOutputEngType = 34;
    gnssLocInfoNotif.locOutputEngType =
            getEnumForPBLocOutputEngineType(pbGnssLocInfoNotif.locoutputengtype());

    // uint32 locOutputEngMask = 35; - bitwise OR of PBLocApiPositioningEngineMask
    gnssLocInfoNotif.locOutputEngMask =
            getEnumForPBPositioningEngineMask(pbGnssLocInfoNotif.locoutputengmask());

    // float conformityIndex = 36;
    gnssLocInfoNotif.conformityIndex = pbGnssLocInfoNotif.conformityindex();

    // PBLLAInfo llaVRPBased = 37;
    pbConvertToLLAInfo(pbGnssLocInfoNotif.llavrpbased(), gnssLocInfoNotif.llaVRPBased);

    // repeated float enuVelocityVRPBased = 38; - Max array length 3
    for (int i=0; i < 3; i++) {
        gnssLocInfoNotif.enuVelocityVRPBased[i] = pbGnssLocInfoNotif.enuvelocityvrpbased(i);
        LocApiPb_LOGv("LocApiPB: enuVelocityVRPBased[%d]:%f", i,
                gnssLocInfoNotif.enuVelocityVRPBased[i]);
    }

    // uint32 drSolutionStatusMask = 39; - PBDrSolutionStatusMask
    gnssLocInfoNotif.drSolutionStatusMask =
            (DrSolutionStatusMask)getDrSolutionStatusMaskFromPB(
            pbGnssLocInfoNotif.drsolutionstatusmask());

    // bool altitudeAssumed = 41;
    gnssLocInfoNotif.altitudeAssumed= pbGnssLocInfoNotif.altitudeassumed();

    // bool sessionStatus = 42;
    gnssLocInfoNotif.sessionStatus = getLocSessionStatusFromPB(
            pbGnssLocInfoNotif.sessionstatus());

    // uint32 integrityRiskUsed = 43;
    gnssLocInfoNotif.integrityRiskUsed= pbGnssLocInfoNotif.integrityriskused();
    // float    protectAlongTrack = 44;
    gnssLocInfoNotif.protectAlongTrack= pbGnssLocInfoNotif.protectalongtrack();
    // float    protectCrossTrack = 45;
    gnssLocInfoNotif.protectCrossTrack = pbGnssLocInfoNotif.protectcrosstrack();
    // float    protectVertical = 46;
    gnssLocInfoNotif.protectVertical = pbGnssLocInfoNotif.protectvertical();

    // repeated uint32 dgnssStationId = 47;
    uint32_t cnt = (uint32_t) pbGnssLocInfoNotif.dgnssstationid_size();
    uint32_t i = 0;
    for (i = 0; i < cnt && i < DGNSS_STATION_ID_MAX ; i++) {
        gnssLocInfoNotif.dgnssStationId[i] = (uint16_t)pbGnssLocInfoNotif.dgnssstationid(i);
    }
    gnssLocInfoNotif.numOfDgnssStationId = i;
    // double baseLineLength = 48;
    gnssLocInfoNotif.baseLineLength = pbGnssLocInfoNotif.baselinelength();
    // uint64 ageMsecOfCorrections = 49;
    gnssLocInfoNotif.ageMsecOfCorrections = pbGnssLocInfoNotif.agemsecofcorrections();

   // uint32    leapSecondsUnc = 50;
   gnssLocInfoNotif.leapSecondsUnc = pbGnssLocInfoNotif.leapsecondsunc();

   LOC_LOGv("LocApiPB: pbGnssLocInfoNotif -GLocInfoFlgMask:0x%" PRIx64 ", pdop:%f, "
            "hdop:%f, vdop:%f",
            gnssLocInfoNotif.flags, gnssLocInfoNotif.pdop, gnssLocInfoNotif.hdop,
            gnssLocInfoNotif.vdop);
    LOC_LOGv("HorReliab:%d, VerReliab:%d, HorUncElps-SemiMajor:%f SemiMinor:%f, NumSvUsedInPos:%u",
            gnssLocInfoNotif.horReliability, gnssLocInfoNotif.verReliability,
            gnssLocInfoNotif.horUncEllipseSemiMajor, gnssLocInfoNotif.horUncEllipseSemiMinor,
            gnssLocInfoNotif.numSvUsedInPosition);
    LOC_LOGv("NavSolMask:%x, NumMeasRcvd:%u, LocOpEngType:%d, PosEngMask:%x",
            gnssLocInfoNotif.navSolutionMask, gnssLocInfoNotif.numOfMeasReceived,
            gnssLocInfoNotif.locOutputEngType, gnssLocInfoNotif.locOutputEngMask);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssSvNotif(const PBLocApiGnssSvNotification &pbGnssSvNotif,
            GnssSvNotification &gnssSvNotif) const {
    gnssSvNotif.size = sizeof(GnssSvNotification);

    // bool gnssSignalTypeMaskValid = 1;
    gnssSvNotif.gnssSignalTypeMaskValid = pbGnssSvNotif.gnsssignaltypemaskvalid();

    // information on a number of SVs (max - GNSS_SV_MAX)
    // repeated PBLocApiGnssSv gnssSvs = 2;
    gnssSvNotif.count = min(pbGnssSvNotif.gnsssvs_size(), (int)GNSS_SV_MAX);
    LOC_LOGv("LocApiPB: pbGnssSvNotif- num svs %d", gnssSvNotif.count);
    for (int i=0; i < gnssSvNotif.count; i++) {
        PBLocApiGnssSv pPbGnssSv = pbGnssSvNotif.gnsssvs(i);
        gnssSvNotif.gnssSvs[i].size = sizeof(GnssSv);
        // uint32 svId = 1;
        gnssSvNotif.gnssSvs[i].svId = pPbGnssSv.svid();

        // Use Gnss_LocSvSystemEnumType instead of GnssSvType
        // PBLocApiGnss_LocSvSystemEnumType type = 2;
        gnssSvNotif.gnssSvs[i].type =
                getGnssSvTypeFromPBGnssLocSvSystemEnumType(pPbGnssSv.type());

        // float cN0Dbhz = 3;
        gnssSvNotif.gnssSvs[i].cN0Dbhz = pPbGnssSv.cn0dbhz();

        // float elevation = 4;
        gnssSvNotif.gnssSvs[i].elevation = pPbGnssSv.elevation();

        // float azimuth = 5;
        gnssSvNotif.gnssSvs[i].azimuth = pPbGnssSv.azimuth();

        // Bitwise OR of PBLocApiGnssSvOptionsMask
        // uint32 gnssSvOptionsMask = 6;
        gnssSvNotif.gnssSvs[i].gnssSvOptionsMask =
                getGnssSvOptionsMaskFromPB(pPbGnssSv.gnsssvoptionsmask());

        // float carrierFrequencyHz = 7;
        gnssSvNotif.gnssSvs[i].carrierFrequencyHz = pPbGnssSv.carrierfrequencyhz();

        // uint32 gnssSignalTypeMask = 8; - PBGnssSignalTypeMask
        gnssSvNotif.gnssSvs[i].gnssSignalTypeMask =
                getGnssSignalTypeMaskFromPB(pPbGnssSv.gnsssignaltypemask());

        // double basebandCarrierToNoiseDbHz = 9; // Baseband signal strength Db Hz.
        gnssSvNotif.gnssSvs[i].basebandCarrierToNoiseDbHz = pPbGnssSv.basebandcarriertonoisedbhz();

        // uint32 gloFrequency = 10;
        gnssSvNotif.gnssSvs[i].gloFrequency = pPbGnssSv.glofrequency();
        LocApiPb_LOGv("LocApiPB: gnssSv[%d] - SvId:%d, CNo:%f, SvOptMask:%x, SignalTypeMask:%x, "\
                "gloFrequency:%d",
                i, gnssSvNotif.gnssSvs[i].svId, gnssSvNotif.gnssSvs[i].cN0Dbhz,
                gnssSvNotif.gnssSvs[i].gnssSvOptionsMask,
                gnssSvNotif.gnssSvs[i].gnssSignalTypeMask,
                gnssSvNotif.gnssSvs[i].gloFrequency);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLocAPINmeaSerializedPayload(
            const PBLocAPINmeaSerializedPayload &pbLocApiNmeaSerPayload,
            LocAPINmeaSerializedPayload &locApiNmeaSerPayload) const {
    // uint64 timestamp = 1;
    locApiNmeaSerPayload.timestamp = pbLocApiNmeaSerPayload.timestamp();
    // string nmea = 2;
    locApiNmeaSerPayload.nmea = pbLocApiNmeaSerPayload.nmea();
    //PBLocApiOutputEngineType logOutputEngineType = 3;
    locApiNmeaSerPayload.locOutputEngType =
            getEnumForPBLocOutputEngineType(pbLocApiNmeaSerPayload.locoutputengtype());
    LocApiPb_LOGv("LocApiPB: pbLocApiNmeaSerPayload %" PRIu64, locApiNmeaSerPayload.timestamp);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssDataNotification(
        const PBGnssDataNotification &pbGnssDataNotif,
        GnssDataNotification &gnssDataNotif) const {
    gnssDataNotif.size = sizeof(GnssDataNotification);

    // uint32 numberSignalTypes = 1
    uint32_t i = 0;
    uint32_t arrMaxLen = min(pbGnssDataNotif.numbersignaltypes(),
            (uint32_t)GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES);

    // repeated uint32  gnssDataMask = 2;
    // repeated double     jammerInd = 3;
    // repeated double     agc = 4;
    for (i = 0; i < arrMaxLen; i++) {
        gnssDataNotif.gnssDataMask[i] = getGnssDataMaskFromPB(pbGnssDataNotif.gnssdatamask(i));
        gnssDataNotif.jammerInd[i] = pbGnssDataNotif.jammerind(i);
        gnssDataNotif.agc[i] = pbGnssDataNotif.agc(i);
        LocApiPb_LOGd("LocApiPB: pbGnssDataNotif - jammerInd:%lf, agc:%lf, gnssDataMask: %" PRIu64,
                gnssDataNotif.jammerInd[i], gnssDataNotif.agc[i], gnssDataNotif.gnssDataMask[i]);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssMeasNotification(
        const PBGnssMeasurementsNotification &pbGnssMeasNotif,
        GnssMeasurementsNotification &gnssMeasNotif) const {
    gnssMeasNotif.size = sizeof(GnssMeasurementsNotification);
    // repeated PBGnssMeasurementsData measurements = 1; - Max array len - GNSS_MEASUREMENTS_MAX
    uint32_t count = pbGnssMeasNotif.measurements_size();
    gnssMeasNotif.count = count;
    for (int i=0; i < count; i++) {
        pbConvertToGnssMeasurementsData(pbGnssMeasNotif.measurements(i),
                gnssMeasNotif.measurements[i]);
    }

    // PBGnssMeasurementsClock clock = 2;
    pbConvertToGnssMeasurementsClock(pbGnssMeasNotif.clock(), gnssMeasNotif.clock);

    // bool isNhz = 3;
    gnssMeasNotif.isNhz = pbGnssMeasNotif.isnhz();
    // bool agcStatusL1 = 4;
    gnssMeasNotif.agcStatusL1 = getEnumForPBAgcStatus(pbGnssMeasNotif.agcstatusl1());
    // bool agcStatusL2 = 5;
    gnssMeasNotif.agcStatusL2 = getEnumForPBAgcStatus(pbGnssMeasNotif.agcstatusl2());
    // bool agcStatusL5 = 6;
    gnssMeasNotif.agcStatusL5 = getEnumForPBAgcStatus(pbGnssMeasNotif.agcstatusl5());

    LOC_LOGv("LocApiPB: pbGnssMeasNotif - count:%u, isNhz:%d", count, gnssMeasNotif.isNhz);

    return 0;
}

int LocationApiPbMsgConv::pbConvertToLocationSystemInfo(const PBLocationSystemInfo &pbLocSysInfo,
            LocationSystemInfo &locSysInfo) const {
    // bitwise OR of PBLocationSystemInfoMask
    // uint32 systemInfoMask = 1;
    locSysInfo.systemInfoMask = getLocationSystemInfoMaskFromPB(pbLocSysInfo.systeminfomask());

    // PBLeapSecondSystemInfo   leapSecondSysInfo = 2;
    pbConvertToLeapSecSysInfo(pbLocSysInfo.leapsecondsysinfo(), locSysInfo.leapSecondSysInfo);

    LOC_LOGv("LocApiPB: pbLocSysInfo - sysInfoMask: %x", locSysInfo.systemInfoMask);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLocationOptions(const PBLocationOptions &pbLocOpt,
        LocationOptions &locOpt) const {
    locOpt.size = sizeof(LocationOptions);
    // uint32 minInterval = 1;
    locOpt.minInterval = pbLocOpt.mininterval();

    // uint32 minDistance = 2;
    locOpt.minDistance = pbLocOpt.mindistance();

    // PBGnssSuplMode mode = 3;
    locOpt.mode = getEnumForPBGnssSuplMode(pbLocOpt.mode());

    // bitwise OR of PBLocReqEngineTypeMask
    // uint32 locReqEngTypeMask = 4;
    locOpt.locReqEngTypeMask = (LocReqEngineTypeMask)getLocReqEngineTypeMaskFromPB(
            pbLocOpt.locreqengtypemask());

    // PBQuailtyLevelAccepted = 5;
    locOpt.qualityLevelAccepted = getEnumForPBFixQualityLevel(pbLocOpt.qualitylevelaccepted());

    LocApiPb_LOGd("LocApiPB: pbLocOpt - MinInterval: %u, MinDistance:%u, GnssSuplMode:%d, "\
            "LocReqEngineTypeMask:%x qualityLevelAccepted: %d",
            locOpt.minInterval, locOpt.minDistance, locOpt.mode,
            locOpt.locReqEngTypeMask, locOpt.qualityLevelAccepted);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGfAddReqPayload(
        const PBGeofencesAddedReqPayload &pbGfAddReqPload,
        GeofencesAddedReqPayload &gfAddReqPload) const {
    // repeated PBGeofencePayload gfPayload = 1;
    uint32_t gfCount = pbGfAddReqPload.gfpayload_size();
    gfAddReqPload.count = gfCount;
    LOC_LOGv("LocApiPB: pbGfAddReqPload- count %d", gfCount);
    for (int i=0; i < gfCount; i++) {
        PBGeofencePayload pbGfPayload = pbGfAddReqPload.gfpayload(i);
        // uint32 gfClientId = 1;
        gfAddReqPload.gfPayload[i].gfClientId = pbGfPayload.gfclientid();
        LocApiPb_LOGv("LocApiPB: gfPayload[%d] gfClientId - %u", i,
                gfAddReqPload.gfPayload[i].gfClientId);
        // PBGeofenceOption gfOption = 2;
        pbConvertToGeofenceOption(pbGfPayload.gfoption(), gfAddReqPload.gfPayload[i].gfOption);
        // PBGeofenceInfo gfInfo = 3;
        pbConvertToGeofenceInfo(pbGfPayload.gfinfo(), gfAddReqPload.gfPayload[i].gfInfo);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGfReqClientIdPayload(
        const PBGeofencesReqClientIdPayload &pbGfReqClientIdPload,
        GeofencesReqClientIdPayload &gfReqClientIdPload) const {
    // repeated uint32 gfIds = 1;
    gfReqClientIdPload.count = pbGfReqClientIdPload.gfids_size();
    uint32_t gfCount = gfReqClientIdPload.count;
    LOC_LOGv("LocApiPB: gfReqClientIdPload count %u", gfCount);
    for (uint32_t i=0; i < gfCount; i++) {
        gfReqClientIdPload.gfIds[i] = pbGfReqClientIdPload.gfids(i);
        LocApiPb_LOGv("LocApiPB: gfReqClientIdPload gfIds[%u]: %u", i,
                gfReqClientIdPload.gfIds[i]);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssSvTypeConfig(const PBGnssSvTypeConfig &pbGnssSvTypeConfig,
        GnssSvTypeConfig &gnssSvTypeConfig) const {
    gnssSvTypeConfig.size = sizeof(GnssSvTypeConfig);
    // Enabled Constellations - bitwise OR of PBGnssSvTypesMask
    // uint64 enabledSvTypesMask = 1;
    gnssSvTypeConfig.enabledSvTypesMask = getGnssSvTypesMaskFromPB(
            pbGnssSvTypeConfig.enabledsvtypesmask());

    // Disabled Constellations - bitwise OR of PBGnssSvTypesMask
    // uint64 blacklistedSvTypesMask = 2;
    gnssSvTypeConfig.blacklistedSvTypesMask = getGnssSvTypesMaskFromPB(
            pbGnssSvTypeConfig.blacklistedsvtypesmask());

    LOC_LOGv("LocApiPB: pbGnssSvTypesMask - Enable Sv types: %" PRIu64 \
            " Blacklist Sv Types: %" PRIu64, gnssSvTypeConfig.enabledSvTypesMask,
            gnssSvTypeConfig.blacklistedSvTypesMask);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssSvIdConfig(const PBGnssSvIdConfig &pbGnssSvIdConfig,
        GnssSvIdConfig &gnssSvIdConfig) const {
    gnssSvIdConfig.size = sizeof(GnssSvIdConfig);
    // uint64 gloBlacklistSvMask = 1;
    gnssSvIdConfig.gloBlacklistSvMask = pbGnssSvIdConfig.globlacklistsvmask();

    // uint64 bdsBlacklistSvMask = 2;
    gnssSvIdConfig.bdsBlacklistSvMask = pbGnssSvIdConfig.bdsblacklistsvmask();

    // uint64 qzssBlacklistSvMask = 3;
    gnssSvIdConfig.qzssBlacklistSvMask = pbGnssSvIdConfig.qzssblacklistsvmask();

    // uint64 galBlacklistSvMask = 4;
    gnssSvIdConfig.galBlacklistSvMask = pbGnssSvIdConfig.galblacklistsvmask();

    // uint64 sbasBlacklistSvMask = 5;
    gnssSvIdConfig.sbasBlacklistSvMask = pbGnssSvIdConfig.sbasblacklistsvmask();

    // uint64_t navicBlacklistSvMask = 6;
    gnssSvIdConfig.navicBlacklistSvMask = pbGnssSvIdConfig.navicblacklistsvmask();

    // uint64_t gpsBlacklistSvMask = 7;
    gnssSvIdConfig.gpsBlacklistSvMask = pbGnssSvIdConfig.gpsblacklistsvmask();

    LOC_LOGv("LocApiPB: BlackListSvMask - Glo: %" PRIu64 ",Bds: %" PRIu64 ",Qzss: %" PRIu64 \
            ",Gal: %" PRIu64 ",Sbas: %" PRIu64",Nav: %" PRIu64 " GPS: %" PRIu64 " ",
            gnssSvIdConfig.gloBlacklistSvMask,
            gnssSvIdConfig.bdsBlacklistSvMask, gnssSvIdConfig.qzssBlacklistSvMask,
            gnssSvIdConfig.galBlacklistSvMask, gnssSvIdConfig.sbasBlacklistSvMask,
            gnssSvIdConfig.navicBlacklistSvMask, gnssSvIdConfig.gpsBlacklistSvMask);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssAidingData(const PBAidingData &pbGnssAidData,
        GnssAidingData &gnssAidData) const {
    // bool deleteAll  = 1;
    gnssAidData.deleteAll = pbGnssAidData.deleteall();

    // uint32 gnssAidingDataSvMask = 2; - bitwise OR of PBLocApiGnssAidingDataSvMask
    gnssAidData.sv.svMask = getGnssAidingDataSvMaskFromPB(pbGnssAidData.gnssaidingdatasvmask());
    // set sv type mask
    gnssAidData.sv.svTypeMask = GNSS_AIDING_DATA_SV_TYPE_MASK_ALL;

    // uint32 dreAidingDataMask = 3;- PBDrEngineAidingDataMask
    gnssAidData.dreAidingDataMask = getDrEngineAidingDataMaskFromPB(
            pbGnssAidData.dreaidingdatamask());

    // Masks from - PBLocApiPositioningEngineMask
    // uint32 posEngineMask = 4;
    gnssAidData.posEngineMask = getEnumForPBPositioningEngineMask(pbGnssAidData.posenginemask());

    LOC_LOGv("LocApiPB: pbGnssAidData deleteAll:%d, svMask: %x, PosEngMask:%x",
            gnssAidData.deleteAll, gnssAidData.sv.svMask, gnssAidData.posEngineMask);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLeverArmConfigInfo(
        const PBLIALeverArmConfigInfo &pbLeverArmCfgInfo,
        LeverArmConfigInfo &leverArmCfgInfo) const {
    // uint32 leverArmValidMask = 1;
    // PBLIALeverArmTypeMask
    leverArmCfgInfo.leverArmValidMask = getLeverArmTypeMaskFromPB(
            pbLeverArmCfgInfo.leverarmvalidmask());

    // PBLIALeverArmParams   gnssToVRP = 2;
    pbConvertToLeverArmParams(pbLeverArmCfgInfo.gnsstovrp(), leverArmCfgInfo.gnssToVRP);

    // PBLIALeverArmParams   drImuToGnss = 3;
    pbConvertToLeverArmParams(pbLeverArmCfgInfo.drimutognss(), leverArmCfgInfo.drImuToGnss);

    // PBLIALeverArmParams   veppImuToGnss = 4;
    pbConvertToLeverArmParams(pbLeverArmCfgInfo.veppimutognss(), leverArmCfgInfo.veppImuToGnss);

    LOC_LOGv("LocApiPB: pbLeverArmCfgInfo - leverArmValidMask: %x",
            leverArmCfgInfo.leverArmValidMask);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToBodyToSensorMountParams(
        const PBLIABodyToSensorMountParams &pbBody2SensorMntParam,
        BodyToSensorMountParams &body2SensorMntParam) const {
    // float rollOffset = 1;
    body2SensorMntParam.rollOffset = pbBody2SensorMntParam.rolloffset();
    // float yawOffset = 2;
    body2SensorMntParam.yawOffset = pbBody2SensorMntParam.yawoffset();
    // float pitchOffset = 3;
    body2SensorMntParam.pitchOffset = pbBody2SensorMntParam.pitchoffset();
    // float offsetUnc = 4;
    body2SensorMntParam.offsetUnc = pbBody2SensorMntParam.offsetunc();

    LOC_LOGv("LocApiPB: pbBody2SensorMntParam: Offset - Roll:%f, Yaw:%f, Pitch:%f, Unc:%f",
            body2SensorMntParam.rollOffset, body2SensorMntParam.yawOffset,
            body2SensorMntParam.pitchOffset, body2SensorMntParam.offsetUnc);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToDeadReckoningEngineConfig(
        const PBDeadReckoningEngineConfig &pbDrEngConfig,
        DeadReckoningEngineConfig &drEngConfig) const {
    // uint64    validMask = 1; - Bitwise OR mask of PBDeadReckoningEngineConfigValidMask
    drEngConfig.validMask = getDeadReckoningEngineConfigValidMaskFromPB(pbDrEngConfig.validmask());

    // PBLIABodyToSensorMountParams bodyToSensorMountParams = 2;
    pbConvertToBodyToSensorMountParams(pbDrEngConfig.bodytosensormountparams(),
            drEngConfig.bodyToSensorMountParams);

    // float vehicleSpeedScaleFactor = 3;
    drEngConfig.vehicleSpeedScaleFactor = pbDrEngConfig.vehiclespeedscalefactor();
    // float vehicleSpeedScaleFactorUnc = 4;
    drEngConfig.vehicleSpeedScaleFactorUnc = pbDrEngConfig.vehiclespeedscalefactorunc();
    // float gyroScaleFactor = 5;
    drEngConfig.gyroScaleFactor = pbDrEngConfig.gyroscalefactor();
    // float gyroScaleFactorUnc = 6;
    drEngConfig.gyroScaleFactorUnc = pbDrEngConfig.gyroscalefactorunc();

    LOC_LOGv("LocApiPB: pbDrEngConfig - DrEngConfigValidMask:%"  PRIu64", VehSpeedScale: %f"
            " VehSpeedScaleUnc: %f, GyroScale: %f, GyroScaleUnc: %f", drEngConfig.validMask,
            drEngConfig.vehicleSpeedScaleFactor, drEngConfig.vehicleSpeedScaleFactorUnc,
            drEngConfig.gyroScaleFactor, drEngConfig.gyroScaleFactorUnc);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLeverArmParams(const PBLIALeverArmParams &pbLeverArmParams,
            LeverArmParams &leverArmParams) const {
    // float forwardOffsetMeters = 1;
    leverArmParams.forwardOffsetMeters = pbLeverArmParams.forwardoffsetmeters();
    // float sidewaysOffsetMeters = 2;
    leverArmParams.sidewaysOffsetMeters = pbLeverArmParams.sidewaysoffsetmeters();
    // float upOffsetMeters = 3;
    leverArmParams.upOffsetMeters = pbLeverArmParams.upoffsetmeters();

    LOC_LOGv("LocApiPB: pbLeverArmParams: Offset - Fwd: %f, Side: %f, Up: %f",
            leverArmParams.forwardOffsetMeters, leverArmParams.sidewaysOffsetMeters,
            leverArmParams.upOffsetMeters);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLeapSecSysInfo(const PBLeapSecondSystemInfo &pbLeapSecSysInfo,
        LeapSecondSystemInfo &leapSecSysInfo) const {
    // bitwise OR of PBLeapSecondSysInfoMask
    // uint32  leapSecondInfoMask = 1;
    leapSecSysInfo.leapSecondInfoMask = getLeapSecSysInfoMaskFromPB(
            pbLeapSecSysInfo.leapsecondinfomask());

    // uint32  leapSecondCurrent = 2;
    leapSecSysInfo.leapSecondCurrent = pbLeapSecSysInfo.leapsecondcurrent();

    // PBLeapSecondChangeInfo  leapSecondChangeInfo = 3;
    pbConvertToLeapSecChgInfo(pbLeapSecSysInfo.leapsecondchangeinfo(),
            leapSecSysInfo.leapSecondChangeInfo);

    LOC_LOGv("LocApiPB: pbLeapSecSysInfo - LeapSecondInfoMask:%x, LeapSecCurr: %u",
            leapSecSysInfo.leapSecondInfoMask, leapSecSysInfo.leapSecondCurrent);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLeapSecChgInfo(const PBLeapSecondChangeInfo &pbLeapSecChgInfo,
            LeapSecondChangeInfo &leapSecChgInfo) const {
    // PBLocApiGnssSystemTimeStructType gpsTimestampLsChange = 1;
    pbConvertToGnssSystemTimeStructType(pbLeapSecChgInfo.gpstimestamplschange(),
            leapSecChgInfo.gpsTimestampLsChange);

    // uint32 leapSecondsBeforeChange = 2;
    leapSecChgInfo.leapSecondsBeforeChange = pbLeapSecChgInfo.leapsecondsbeforechange();

    // uint32 leapSecondsAfterChange = 3;
    leapSecChgInfo.leapSecondsAfterChange = pbLeapSecChgInfo.leapsecondsafterchange();

    LOC_LOGv("LocApiPB: pbLeapSecChgInfo - LeapSecs Before: %u, After:%u",
            leapSecChgInfo.leapSecondsBeforeChange, leapSecChgInfo.leapSecondsAfterChange);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssSystemTimeStructType(
        const PBLocApiGnssSystemTimeStructType &pbGnssSysTimeStrct,
        GnssSystemTimeStructType &gnssSysTimeStrct) const {
    // Validity mask for below fields PBLocApiGnssSystemTimeStructTypeFlags
    // uint32 validityMask = 1;
    gnssSysTimeStrct.validityMask = getGnssSystemTimeStructTypeFlagsFromPB(
            pbGnssSysTimeStrct.validitymask());

    // uint32 systemWeek = 2;
    gnssSysTimeStrct.systemWeek = pbGnssSysTimeStrct.systemweek();

    // uint32 systemMsec = 3;
    gnssSysTimeStrct.systemMsec = pbGnssSysTimeStrct.systemmsec();

    // float systemClkTimeBiasMs = 4;
    gnssSysTimeStrct.systemClkTimeBias = pbGnssSysTimeStrct.systemclktimebiasms();

    // float systemClkTimeBiasUncMs = 5;
    gnssSysTimeStrct.systemClkTimeUncMs = pbGnssSysTimeStrct.systemclktimebiasuncms();

    // uint32 refFCount = 6;
    gnssSysTimeStrct.refFCount = pbGnssSysTimeStrct.reffcount();

    // uint32 numClockResets = 7;
    gnssSysTimeStrct.numClockResets = pbGnssSysTimeStrct.numclockresets();

    LOC_LOGv("LocApiPB: pbGnssSysTimeStrct - ValidityMsk: %x, SysWeek: %u, SysMsec: %u, "\
            "SysClkTimeBias: %f", gnssSysTimeStrct.validityMask, gnssSysTimeStrct.systemWeek,
            gnssSysTimeStrct.systemMsec, gnssSysTimeStrct.systemClkTimeBias);
    LocApiPb_LOGv("LocApiPB: pbGnssSysTimeStrct - SysClkTimeUnc: %f, RefCnt: %u, NumClkReset:%u",
            gnssSysTimeStrct.systemClkTimeUncMs, gnssSysTimeStrct.refFCount,
            gnssSysTimeStrct.numClockResets);
    return 0;
}

// PBGnssMeasurementsData to GnssMeasurementsData
int LocationApiPbMsgConv::pbConvertToGnssMeasurementsData(
        const PBGnssMeasurementsData &pbGnssMeasData,
        GnssMeasurementsData &gnssMeasData) const {
    gnssMeasData.size = sizeof(GnssMeasurementsData);
    // uint32 flags = 1; - bitwise OR of PBGnssMeasurementsDataFlagsMask
    gnssMeasData.flags = getGnssMeasurementsDataFlagsMaskFromPB(pbGnssMeasData.flags());

    // int32 svId = 2;
    gnssMeasData.svId = pbGnssMeasData.svid();

    // Use Gnss_LocSvSystemEnumType instead of GnssSvType
    // PBLocApiGnss_LocSvSystemEnumType svType = 3;
    gnssMeasData.svType = getGnssSvTypeFromPBGnssLocSvSystemEnumType(pbGnssMeasData.svtype());

    // double timeOffsetNs = 4;
    gnssMeasData.timeOffsetNs = pbGnssMeasData.timeoffsetns();

    // uint32 stateMask = 5; - bitwise OR of PBGnssMeasurementsStateMask
    gnssMeasData.stateMask = pbGnssMeasData.statemask();

    // int64 receivedSvTimeNs = 6;
    gnssMeasData.receivedSvTimeNs = pbGnssMeasData.receivedsvtimens();

    // int64 receivedSvTimeUncertaintyNs = 7;
    gnssMeasData.receivedSvTimeUncertaintyNs = pbGnssMeasData.receivedsvtimeuncertaintyns();

    // double carrierToNoiseDbHz = 8;
    gnssMeasData.carrierToNoiseDbHz = pbGnssMeasData.carriertonoisedbhz();

    // double pseudorangeRateMps = 9;
    gnssMeasData.pseudorangeRateMps = pbGnssMeasData.pseudorangeratemps();

    // double pseudorangeRateUncertaintyMps = 10;
    gnssMeasData.pseudorangeRateUncertaintyMps = pbGnssMeasData.pseudorangerateuncertaintymps();

    // uint32 adrStateMask = 11; - bitwise OR of PBGnssMeasurementsAdrStateMask
    gnssMeasData.adrStateMask = getGnssMeasurementsAdrStateMaskFromPB(
            pbGnssMeasData.adrstatemask());

    // double adrMeters = 12;
    gnssMeasData.adrMeters = pbGnssMeasData.adrmeters();

    // double adrUncertaintyMeters = 13;
    gnssMeasData.adrUncertaintyMeters = pbGnssMeasData.adruncertaintymeters();

    // float carrierFrequencyHz = 14;
    gnssMeasData.carrierFrequencyHz = pbGnssMeasData.carrierfrequencyhz();

    // int64 carrierCycles = 15;
    gnssMeasData.carrierCycles = pbGnssMeasData.carriercycles();

    // double carrierPhase = 16;
    gnssMeasData.carrierPhase = pbGnssMeasData.carrierphase();

    // double carrierPhaseUncertainty = 17;
    gnssMeasData.carrierPhaseUncertainty = pbGnssMeasData.carrierphaseuncertainty();

    // PBGnssMeasurementsMultipathIndicator multipathIndicator = 18;
    gnssMeasData.multipathIndicator = getEnumForPBGnssMeasMultipathIndic(
            pbGnssMeasData.multipathindicator());

    // double signalToNoiseRatioDb = 19;
    gnssMeasData.signalToNoiseRatioDb = pbGnssMeasData.signaltonoiseratiodb();

    // double agcLevelDb = 20;
    gnssMeasData.agcLevelDb = pbGnssMeasData.agcleveldb();

    // double basebandCarrierToNoiseDbHz = 21;
    gnssMeasData.basebandCarrierToNoiseDbHz = pbGnssMeasData.basebandcarriertonoisedbhz();

    // uint32 gnssSignalType = 22; - bitwise OR of  PBGnssSignalTypeMask
    gnssMeasData.gnssSignalType =  getGnssSignalTypeMaskFromPB(pbGnssMeasData.gnsssignaltype());

    // double fullInterSignalBiasNs = 23;
    gnssMeasData.fullInterSignalBiasNs = pbGnssMeasData.fullintersignalbiasns();

    // double fullInterSignalBiasUncertaintyNs = 24;
    gnssMeasData.fullInterSignalBiasUncertaintyNs =
            pbGnssMeasData.fullintersignalbiasuncertaintyns();

    // uint32 cycleSlipCount = 25;
    gnssMeasData.cycleSlipCount = pbGnssMeasData.cycleslipcount();

    // int64 receivedSvTimeSubNs = 26;
    gnssMeasData.receivedSvTimeSubNs = pbGnssMeasData.receivedsvtimesubns();

    LOC_LOGv("LocApiPB: pbGnssMeasData - GnssMeasDataFlags:%x, Svid:%d, SvType:%d, StateMsk:%x, "\
            "RcvSvTime:%"  PRIu64", RcvSvTimeUnc:%" PRIu64", CNoDb:%lf", gnssMeasData.flags,
            gnssMeasData.svId, gnssMeasData.svType, gnssMeasData.stateMask,
            gnssMeasData.receivedSvTimeNs, gnssMeasData.receivedSvTimeUncertaintyNs,
            gnssMeasData.carrierToNoiseDbHz);

    LocApiPb_LOGv("LocApiPB: pbGnssMeasData - TimeOffset:%lf, PseuRngRt:%lf, PseuRngRtUnc:%lf,"\
            "AdrStateMask:%x, AdrMeters:%lf, AdrUncMeters:%lf, CarierFreq:%f, CarierCyc:%" PRIu64,
            gnssMeasData.timeOffsetNs, gnssMeasData.pseudorangeRateMps,
            gnssMeasData.pseudorangeRateUncertaintyMps, gnssMeasData.adrStateMask,
            gnssMeasData.adrMeters, gnssMeasData.adrUncertaintyMeters,
            gnssMeasData.carrierFrequencyHz, gnssMeasData.carrierCycles);

    LocApiPb_LOGv("LocApiPB: pbGnssMeasData- CarierPhase:%lf, CarierPhaseUnc:%lf, MultiPathInd:%d"\
            "CNoRatio:%lf, AgcLevel:%lf, BasebandCno:%lf", gnssMeasData.carrierPhase,
            gnssMeasData.carrierPhaseUncertainty, gnssMeasData.multipathIndicator,
            gnssMeasData.signalToNoiseRatioDb, gnssMeasData.agcLevelDb,
            gnssMeasData.basebandCarrierToNoiseDbHz);

    LocApiPb_LOGd("LocApiPB: gnssMeasData - GnssSignalType:%x, InterSigBiasNs:%lf, "\
            "InterSigBiasUncNs:%lf, cycleSlipCount:%d",
            gnssMeasData.gnssSignalType, gnssMeasData.fullInterSignalBiasNs,
            gnssMeasData.fullInterSignalBiasUncertaintyNs, gnssMeasData.cycleSlipCount);

    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssMeasurementsClock(
        const PBGnssMeasurementsClock &pbGnssMeasClock,
        GnssMeasurementsClock &gnssMeasClock) const {
    gnssMeasClock.size = sizeof(GnssMeasurementsClock);
    // uint32 flags = 1; - bitwise OR of PBGnssMeasurementsClockFlagsMask
    gnssMeasClock.flags = getGnssMeasurementsClockFlagsMaskFromPB(pbGnssMeasClock.flags());
    // int32 leapSecond = 2;
    gnssMeasClock.leapSecond = pbGnssMeasClock.leapsecond();
    // int64 timeNs = 3;
    gnssMeasClock.timeNs = pbGnssMeasClock.timens();
    // double timeUncertaintyNs = 4;
    gnssMeasClock.timeUncertaintyNs = pbGnssMeasClock.timeuncertaintyns();
    // int64 fullBiasNs = 5;
    gnssMeasClock.fullBiasNs = pbGnssMeasClock.fullbiasns();
    // double biasNs = 6;
    gnssMeasClock.biasNs = pbGnssMeasClock.biasns();
    // double biasUncertaintyNs = 7;
    gnssMeasClock.biasUncertaintyNs = pbGnssMeasClock.biasuncertaintyns();
    // double driftNsps = 8;
    gnssMeasClock.driftNsps = pbGnssMeasClock.driftnsps();
    // double driftUncertaintyNsps = 9;
    gnssMeasClock.driftUncertaintyNsps = pbGnssMeasClock.driftuncertaintynsps();
    // uint32 hwClockDiscontinuityCount= 10;
    gnssMeasClock.hwClockDiscontinuityCount = pbGnssMeasClock.hwclockdiscontinuitycount();
    // uint64 elapsedRealTime = 11;
    gnssMeasClock.elapsedRealTime = pbGnssMeasClock.elapsedrealtime();
    // uint64 elapsedRealTimeUnc = 12;
    gnssMeasClock.elapsedRealTimeUnc = pbGnssMeasClock.elapsedrealtimeunc();
    // uint64 elapsedgPTPTime = 13;
    gnssMeasClock.elapsedgPTPTime = pbGnssMeasClock.elapsedgptptime();
    // uint64 elapsedgPTPTimeUnc = 14;
    gnssMeasClock.elapsedgPTPTimeUnc = pbGnssMeasClock.elapsedgptptimeunc();

    LOC_LOGv("LocApiPB: pbGnssMeasClock - GnssMeasClockFlags:%x, leapSecond:%u, TimeNs:%" PRIu64\
        "TimeUnc:%lf FullBiasNs:%" PRIu64" BiasNs:%lf, BiasUncNs:%lf, DriftNs:%lf, DriftUncNs:%lf"
        "HwDiscCnt:%u, elapsedRealTime:%" PRIu64" elapsedRealTimeUnc:%" PRIu64\
        "elapsedgPTPTime:%" PRIu64" elapsedgPTPTimeUnc:%" PRIu64,
        gnssMeasClock.flags, gnssMeasClock.leapSecond, gnssMeasClock.timeNs,
        gnssMeasClock.timeUncertaintyNs, gnssMeasClock.fullBiasNs, gnssMeasClock.biasNs,
        gnssMeasClock.biasUncertaintyNs, gnssMeasClock.driftNsps,
        gnssMeasClock.driftUncertaintyNsps, gnssMeasClock.hwClockDiscontinuityCount,
        gnssMeasClock.elapsedRealTime, gnssMeasClock.elapsedRealTimeUnc,
        gnssMeasClock.elapsedgPTPTime, gnssMeasClock.elapsedgPTPTimeUnc);

    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssLocationSvUsedInPosition(
        const PBGnssLocationSvUsedInPosition &pbGnssLocSvUsedInPos,
        GnssLocationSvUsedInPosition &gnssLocSvUsedInPos) const {
    // uint64 gpsSvUsedIdsMask = 1;
    gnssLocSvUsedInPos.gpsSvUsedIdsMask = pbGnssLocSvUsedInPos.gpssvusedidsmask();
    // uint64 gloSvUsedIdsMask = 2;
    gnssLocSvUsedInPos.gloSvUsedIdsMask = pbGnssLocSvUsedInPos.glosvusedidsmask();
    // uint64 galSvUsedIdsMask = 3;
    gnssLocSvUsedInPos.galSvUsedIdsMask = pbGnssLocSvUsedInPos.galsvusedidsmask();
    // uint64 bdsSvUsedIdsMask = 4;
    gnssLocSvUsedInPos.bdsSvUsedIdsMask = pbGnssLocSvUsedInPos.bdssvusedidsmask();
    // uint64 qzssSvUsedIdsMask = 5;
    gnssLocSvUsedInPos.qzssSvUsedIdsMask = pbGnssLocSvUsedInPos.qzsssvusedidsmask();
    // uint64 navicSvUsedIdsMask = 6;
    gnssLocSvUsedInPos.navicSvUsedIdsMask = pbGnssLocSvUsedInPos.navicsvusedidsmask();
    LOC_LOGv("LocApiPB: pbGnssLocSvUsedInPos - Gps:%" PRIu64", Glo:%" PRIu64", Gal:%" PRIu64\
            ", Bds:%" PRIu64", Qzss:%" PRIu64", Navic:%" PRIu64,
            gnssLocSvUsedInPos.gpsSvUsedIdsMask, gnssLocSvUsedInPos.gloSvUsedIdsMask,
            gnssLocSvUsedInPos.galSvUsedIdsMask, gnssLocSvUsedInPos.bdsSvUsedIdsMask,
            gnssLocSvUsedInPos.qzssSvUsedIdsMask, gnssLocSvUsedInPos.navicSvUsedIdsMask);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToLLAInfo(const PBLLAInfo &pbLlaInfo, LLAInfo &llaInfo) const {
    // double latitude = 1;
    llaInfo.latitude = pbLlaInfo.latitude();

    // double longitude = 2;
    llaInfo.longitude = pbLlaInfo.longitude();

    // float altitude = 3;
    llaInfo.altitude = pbLlaInfo.altitude();

    LOC_LOGv("LocApiPB: pbLlaInfo - Lat:%lf, Lon:%lf, Alt:%f", llaInfo.latitude,
            llaInfo.longitude, llaInfo.altitude);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssLocationPositionDynamics(
        const PBLocApiGnssLocationPositionDynamics &pbGnssLocPosDyn,
        GnssLocationPositionDynamics &gnssLocPosDyn,
        GnssLocationPositionDynamicsExt &gnssLocPosDynExt) const {

    // uint32  bodyFrameDataMask = 1; - Bitwise OR of PBLocApiGnssLocationPosDataMask
    gnssLocPosDyn.bodyFrameDataMask =
            getGnssLocationPosDataMaskFromPB(pbGnssLocPosDyn.bodyframedatamask());
    gnssLocPosDynExt.bodyFrameDataMask =
            getGnssLocationPosDataMaskExtFromPB(pbGnssLocPosDyn.bodyframedatamask());

    // float           longAccel = 2;
    // float           latAccel = 3;
    // float           vertAccel = 4;
    // float           longAccelUnc = 5;
    // float           latAccelUnc = 6;
    // float           vertAccelUnc = 7;
    gnssLocPosDyn.longAccel = pbGnssLocPosDyn.longaccel();
    gnssLocPosDyn.latAccel = pbGnssLocPosDyn.lataccel();
    gnssLocPosDyn.vertAccel = pbGnssLocPosDyn.vertaccel();
    gnssLocPosDyn.longAccelUnc = pbGnssLocPosDyn.longaccelunc();
    gnssLocPosDyn.latAccelUnc = pbGnssLocPosDyn.lataccelunc();
    gnssLocPosDyn.vertAccelUnc = pbGnssLocPosDyn.vertaccelunc();

    // float           pitch = 8;
    // float           pitchUnc = 9;
    // float           pitchRate = 10;
    // float           pitchRateUnc = 11;
    gnssLocPosDyn.pitch = pbGnssLocPosDyn.pitch();
    gnssLocPosDyn.pitchUnc = pbGnssLocPosDyn.pitchunc();
    gnssLocPosDynExt.pitchRate = pbGnssLocPosDyn.pitchrate();
    gnssLocPosDynExt.pitchRateUnc = pbGnssLocPosDyn.pitchrateunc();

    // float           roll = 12;
    // float           rollUnc = 13;
    // float           rollRate = 14;
    // float           rollRateUnc = 15;
    gnssLocPosDynExt.roll = pbGnssLocPosDyn.roll();
    gnssLocPosDynExt.rollUnc = pbGnssLocPosDyn.rollunc();
    gnssLocPosDynExt.rollRate = pbGnssLocPosDyn.rollrate();
    gnssLocPosDynExt.rollRateUnc = pbGnssLocPosDyn.rollrateunc();

    // float           yaw = 16;
    // float           yawUnc = 17;
    // float           yawRate = 18;
    // float           yawRateUnc = 19;
    gnssLocPosDynExt.yaw = pbGnssLocPosDyn.yaw();
    gnssLocPosDynExt.yawUnc = pbGnssLocPosDyn.yawunc();
    gnssLocPosDyn.yawRate = pbGnssLocPosDyn.yawrate();
    gnssLocPosDyn.yawRateUnc = pbGnssLocPosDyn.yawrateunc();

    LOC_LOGv("LocApiPB: pbGnssLocPosDyn - Mask:%x, MaskExt:%x, Accel-Long:%f Lat:%f Vert:%f",
            gnssLocPosDyn.bodyFrameDataMask, gnssLocPosDynExt.bodyFrameDataMask,
            gnssLocPosDyn.longAccel, gnssLocPosDyn.latAccel, gnssLocPosDyn.vertAccel);
    LocApiPb_LOGv("LocApiPB: Pitch:%f, PitchRate:%f, Roll:%f, RollRate:%f, Yaw:%f, YawRate:%f",
            gnssLocPosDyn.pitch, gnssLocPosDynExt.pitchRate, gnssLocPosDynExt.roll,
            gnssLocPosDynExt.rollRate, gnssLocPosDynExt.yaw, gnssLocPosDyn.yawRate);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssSystemTime(const PBLocApiGnssSystemTime &pbGnssSysTime,
        GnssSystemTime &gnssSysTime) const {
    // PBLocApiGnss_LocSvSystemEnumType gnssSystemTimeSrc = 1;
    gnssSysTime.gnssSystemTimeSrc =
            getEnumForPBGnssLocSvSystem(pbGnssSysTime.gnsssystemtimesrc());
    // PBLocApiSystemTimeStructUnion u = 2;
    pbConvertToSystemTimeStructUnion(gnssSysTime.gnssSystemTimeSrc,
            pbGnssSysTime.u(), gnssSysTime.u);
    LOC_LOGv("LocApiPB: pbGnssSysTime - GnssLocSvSystem %d", gnssSysTime.gnssSystemTimeSrc);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssMeasUsageInfo(
        const PBGnssMeasUsageInfo &pbGnssMeasUsageInfo,
        GnssMeasUsageInfo &gnssMeasUsageInfo) const {
    // PBLocApiGnss_LocSvSystemEnumType gnssConstellation = 1;
    gnssMeasUsageInfo.gnssConstellation =
            getEnumForPBGnssLocSvSystem(pbGnssMeasUsageInfo.gnssconstellation());

    // uint32 gnssSvId = 2;
    gnssMeasUsageInfo.gnssSvId = pbGnssMeasUsageInfo.gnsssvid();

    // uint32 gnssSignalType = 3; - bitwise OR of PBGnssSignalTypeMask
    gnssMeasUsageInfo.gnssSignalType =
        getGnssSignalTypeMaskFromPB(pbGnssMeasUsageInfo.gnsssignaltype());

    LocApiPb_LOGv("LocApiPB: pbGnssMeasUsageInfo - Constl:%d, SvId:%d, SignalTypeMask:%x",
            gnssMeasUsageInfo.gnssConstellation, gnssMeasUsageInfo.gnssSvId,
            gnssMeasUsageInfo.gnssSignalType);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToSystemTimeStructUnion(
        const Gnss_LocSvSystemEnumType &gnssLocSvSysEnumType,
        const PBLocApiSystemTimeStructUnion &pbSysTimeStructUnion,
        SystemTimeStructUnion &sysTimeStructUnion) const {
    LOC_LOGv("LocApiPB: pbSysTimeStructUnion - gnssLocSvSysEnumType:%d", gnssLocSvSysEnumType);
    switch (gnssLocSvSysEnumType) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            // PBLocApiGnssSystemTimeStructType gpsSystemTime = 1;
            pbConvertToGnssSystemTimeStructType(pbSysTimeStructUnion.gpssystemtime(),
                    sysTimeStructUnion.gpsSystemTime);
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            // PBLocApiGnssSystemTimeStructType galSystemTime = 2;
            pbConvertToGnssSystemTimeStructType(pbSysTimeStructUnion.galsystemtime(),
                    sysTimeStructUnion.galSystemTime);
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            // PBLocApiGnssSystemTimeStructType bdsSystemTime = 3;
            pbConvertToGnssSystemTimeStructType(pbSysTimeStructUnion.bdssystemtime(),
                    sysTimeStructUnion.bdsSystemTime);
            break;
        case GNSS_LOC_SV_SYSTEM_QZSS:
            // PBLocApiGnssSystemTimeStructType qzssSystemTime = 4;
            pbConvertToGnssSystemTimeStructType(pbSysTimeStructUnion.qzsssystemtime(),
                    sysTimeStructUnion.qzssSystemTime);
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            // PBLocApiGnssGloTimeStructType gloSystemTime = 5;
            pbConvertToGnssGloTimeStructType(pbSysTimeStructUnion.glosystemtime(),
                    sysTimeStructUnion.gloSystemTime);
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            // PBLocApiGnssSystemTimeStructType navicSystemTime = 6;
            pbConvertToGnssSystemTimeStructType(pbSysTimeStructUnion.navicsystemtime(),
                    sysTimeStructUnion.navicSystemTime);
            break;
        default:
            break;
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssGloTimeStructType(
        const PBLocApiGnssGloTimeStructType &pbGnssGloTime,
        GnssGloTimeStructType &gnssGloTime) const {
    // uint32 validityMask = 1; - Bitwise OR of PBGnssGloTimeStructTypeFlags
    gnssGloTime.validityMask = getGnssGloTimeStructTypeFlagsFromPB(pbGnssGloTime.validitymask());

    // uint32 gloFourYear = 2;
    gnssGloTime.gloFourYear = pbGnssGloTime.glofouryear();

    // uint32 gloDays = 3;
    gnssGloTime.gloDays = pbGnssGloTime.glodays();

    // uint32 gloMsec = 4;
    gnssGloTime.gloMsec = pbGnssGloTime.glomsec();

    // float gloClkTimeBias = 5;
    gnssGloTime.gloClkTimeBias = pbGnssGloTime.gloclktimebias();

    // float gloClkTimeUncMs = 6;
    gnssGloTime.gloClkTimeUncMs = pbGnssGloTime.gloclktimeuncms();

    // uint32  refFCount = 7;
    gnssGloTime.refFCount = pbGnssGloTime.reffcount();

    // uint32 numClockResets = 8;
    gnssGloTime.numClockResets = pbGnssGloTime.numclockresets();

    LOC_LOGv("LocApiPB: pbGnssGloTime - GloValidityMsk: %x, GloFourYear: %u, GloDays: %u, "\
            "GloMsec:%u", gnssGloTime.validityMask, gnssGloTime.gloFourYear, gnssGloTime.gloDays,
             gnssGloTime.gloMsec);
    LocApiPb_LOGv("LocApiPB: pbGnssGloTime - GloClkTimeBias: %f, GloClkTimeUnc: %f, RefFCnt: %u, "\
            "NumClkReset:%u", gnssGloTime.gloClkTimeBias, gnssGloTime.gloClkTimeUncMs,
            gnssGloTime.refFCount, gnssGloTime.numClockResets);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGeofenceOption(const PBGeofenceOption &pbGfOpt,
        GeofenceOption &gfOpt) const {
    gfOpt.size = sizeof(GeofenceOption);
    // bitwise OR of PBGeofenceBreachTypeMask
    // uint32 breachTypeMask = 1;
    gfOpt.breachTypeMask = getGfBreachTypeMaskFromPB(pbGfOpt.breachtypemask());
    // uint32 responsiveness = 2;
    gfOpt.responsiveness = pbGfOpt.responsiveness();
    // uint32 dwellTime = 3;
    gfOpt.dwellTime = pbGfOpt.dwelltime();
    LOC_LOGv("LocApiPB: pbGfOpt - BreachTypeMask:%x Resp:%u, DwellTime:%u", gfOpt.breachTypeMask,
            gfOpt.responsiveness, gfOpt.dwellTime);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGeofenceInfo(const PBGeofenceInfo &pbGfInfo,
        GeofenceInfo &gfInfo) const {
    gfInfo.size = sizeof(GeofenceInfo);
    // double latitude = 1;
    gfInfo.latitude = pbGfInfo.latitude();
    // double longitude = 2;
    gfInfo.longitude = pbGfInfo.longitude();
    // double radius = 3;
    gfInfo.radius = pbGfInfo.radius();
    LOC_LOGv("LocApiPB: pbGfInfo - Lat:%lf, Lon:%lf, Rad:%lf", gfInfo.latitude, gfInfo.longitude,
            gfInfo.radius);
    return 0;
}

// GnssDebugReport to PBGnssDebugReport
int LocationApiPbMsgConv::convertTimespecToPB(const timespec& utcReported,
        PBTimespec* pbUtcReported ) const {
    if (nullptr == pbUtcReported) {
        LOC_LOGe("pbUtcReported is NULL!, return");
        return 1;
    }
    //second tv_sec = 1
    pbUtcReported->set_tv_sec(utcReported.tv_sec);
    //nanosecond tv_nsec = 2;
    pbUtcReported->set_tv_nsec(utcReported.tv_nsec);
    return 0;
}

int LocationApiPbMsgConv::convertGnssDebugLocationToPB(const GnssDebugLocation& debugLocation,
        PBGnssDebugLocation* pbDebugLocation) const {
    if (nullptr == pbDebugLocation) {
        LOC_LOGe("pbDebugLocation is NULL!, return");
        return 1;
    }
    //bool valid = 1;
    pbDebugLocation->set_valid(debugLocation.mValid);
    //PBLocation location = 2;
    PBLocation* location = pbDebugLocation->mutable_location();
    if (nullptr != location) {
        if (convertLocationToPB(debugLocation.mLocation, location)) {
            LOC_LOGe("convertLocationToPB failed");
            free(location);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_location failed");
        return 1;
    }
    //double verticalAccuracyMeters = 3;
    pbDebugLocation->set_verticalaccuracymeters(debugLocation.verticalAccuracyMeters);
    //double speedAccuracyMetersPerSecond = 4;
    pbDebugLocation->set_speedaccuracymeterspersecond(debugLocation.speedAccuracyMetersPerSecond);
    //double bearingAccuracyDegrees = 5;
    pbDebugLocation->set_bearingaccuracydegrees(debugLocation.bearingAccuracyDegrees);
    //PBTimespec utcReported = 6;
    PBTimespec* utcReported = pbDebugLocation->mutable_utcreported();
    if (nullptr != utcReported) {
        if (convertTimespecToPB(debugLocation.mUtcReported, utcReported)) {
            LOC_LOGe("convertTimespecToPB failed");
            free(utcReported);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_utcreported failed");
        return 1;
    }
    return 0;
}

int LocationApiPbMsgConv::convertGnssDebugTimeToPB(const GnssDebugTime& gnssDebugTime,
        PBGnssDebugTime* pbGnssDebugTime) const {
    if (nullptr == pbGnssDebugTime) {
        LOC_LOGe("pbGnssDebugTime is NULL!, return");
        return 1;
    }
    //bool valid = 1;
    pbGnssDebugTime->set_valid(gnssDebugTime.mValid);
    //int64 timeEstimate = 2;
    pbGnssDebugTime->set_timeestimate(gnssDebugTime.timeEstimate);
    //float timeUncertaintyNs = 3;
    pbGnssDebugTime->set_timeuncertaintyns(gnssDebugTime.timeUncertaintyNs);
    //float frequencyUncertaintyNsPerSec = 4;
    pbGnssDebugTime->set_frequencyuncertaintynspersec(
            gnssDebugTime.frequencyUncertaintyNsPerSec);
    return 0;
}

PBGnssEphemerisType LocationApiPbMsgConv::getPBEnumForGnssEphemerisType(
        const GnssEphemerisType& ephemerisType) const {
    PBGnssEphemerisType pbEphemerisType = PB_GNSS_EPH_TYPE_UNKNOWN;
    switch (ephemerisType) {
        case GNSS_EPH_TYPE_UNKNOWN:
            pbEphemerisType = PB_GNSS_EPH_TYPE_UNKNOWN;
            break;
        case GNSS_EPH_TYPE_EPHEMERIS:
            pbEphemerisType = PB_GNSS_EPH_TYPE_EPHEMERIS;
            break;
        case GNSS_EPH_TYPE_ALMANAC:
            pbEphemerisType = PB_GNSS_EPH_TYPE_ALMANAC;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssephemerisType:%d, pbGnssephemerisType:%d",
            ephemerisType, pbEphemerisType);
    return pbEphemerisType;
}

PBGnssEphemerisSource LocationApiPbMsgConv::getPBEnumForGnssEphemerisSource(
        const GnssEphemerisSource& ephemerisSource) const {
    PBGnssEphemerisSource pbEphemerisSource = PB_GNSS_EPH_SOURCE_UNKNOWN;
    switch (ephemerisSource) {
        case GNSS_EPH_SOURCE_UNKNOWN:
            pbEphemerisSource = PB_GNSS_EPH_SOURCE_UNKNOWN;
            break;
        case GNSS_EPH_SOURCE_DEMODULATED:
            pbEphemerisSource = PB_GNSS_EPH_SOURCE_DEMODULATED;
            break;
        case GNSS_EPH_SOURCE_SUPL_PROVIDED:
            pbEphemerisSource = PB_GNSS_EPH_SOURCE_SUPL_PROVIDED;
            break;
        case GNSS_EPH_SOURCE_OTHER_SERVER_PROVIDED:
            pbEphemerisSource = PB_GNSS_EPH_SOURCE_OTHER_SERVER_PROVIDED;
            break;
        case GNSS_EPH_SOURCE_LOCAL:
            pbEphemerisSource = PB_GNSS_EPH_SOURCE_LOCAL;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssephemerisSource:%d, pbGnssephemerisSource:%d",
            ephemerisSource, pbEphemerisSource);
    return pbEphemerisSource;
}

PBGnssEphemerisHealth LocationApiPbMsgConv::getPBEnumForGnssEphemerisHealth(
        const GnssEphemerisHealth& ephemerisHealth) const {
    PBGnssEphemerisHealth pbEphemerisHealth = PB_GNSS_EPH_HEALTH_UNKNOWN;
    switch (ephemerisHealth) {
        case GNSS_EPH_HEALTH_UNKNOWN:
            pbEphemerisHealth = PB_GNSS_EPH_HEALTH_UNKNOWN;
            break;
        case GNSS_EPH_HEALTH_GOOD:
            pbEphemerisHealth = PB_GNSS_EPH_HEALTH_GOOD;
            break;
        case GNSS_EPH_HEALTH_BAD:
            pbEphemerisHealth = PB_GNSS_EPH_HEALTH_BAD;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssephemerisHealth:%d, pbGnssephemerisHealth:%d",
            ephemerisHealth, pbEphemerisHealth);
    return pbEphemerisHealth;
}

int LocationApiPbMsgConv::convertGnssDebugSatelliteInfoToPB(
        const GnssDebugSatelliteInfo& satelliteInfo,
        PBGnssDebugSatelliteInfo* pbSatelliteInfo) const {
    if (nullptr == pbSatelliteInfo) {
        LOC_LOGe("pbSatelliteInfo is NULL!, return");
        return 1;
    }
    //uint32 svid = 1;
    pbSatelliteInfo->set_svid(satelliteInfo.svid);
    //PBLocApiGnss_LocSvSystemEnumType constellation = 2;
    pbSatelliteInfo->set_constellation(
            getPBGnssLocSvSysEnumFromGnssSvType(satelliteInfo.constellation));
    //PBGnssEphemerisType ephemerisType = 3;
    pbSatelliteInfo->set_ephemeristype(
            getPBEnumForGnssEphemerisType(satelliteInfo.mEphemerisType));
    //PBGnssEphemerisSource ephemerisSource = 4;
    pbSatelliteInfo->set_ephemerissource(
            getPBEnumForGnssEphemerisSource(satelliteInfo.mEphemerisSource));
    //PBGnssEphemerisHealth ephemerisHealth = 5;
    pbSatelliteInfo->set_ephemerishealth(
            getPBEnumForGnssEphemerisHealth(satelliteInfo.mEphemerisHealth));
    //float ephemerisAgeSeconds = 6;
    pbSatelliteInfo->set_ephemerisageseconds(satelliteInfo.ephemerisAgeSeconds);
    //bool serverPredictionIsAvailable = 7;
    pbSatelliteInfo->set_serverpredictionisavailable(satelliteInfo.serverPredictionIsAvailable);
    //float serverPredictionAgeSeconds = 8;
    pbSatelliteInfo->set_serverpredictionageseconds(satelliteInfo.serverPredictionAgeSeconds);
    return 0;
}

int LocationApiPbMsgConv::convertGnssDebugReportToPB(
        const GnssDebugReport& gnssDebugReport,
        PBGnssDebugReport* pbGnssDebugReport) const {
    if (nullptr == pbGnssDebugReport) {
        LOC_LOGe("pbGnssDebugReport is NULL!, return");
        return 1;
    }
    //pbGnssDebugReport-> location = 1;
    PBGnssDebugLocation* location = pbGnssDebugReport->mutable_location();
    if (nullptr != location) {
        if (convertGnssDebugLocationToPB(gnssDebugReport.mLocation, location)) {
            LOC_LOGe("convertGnssDebugLocation failed");
            free(location);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_location failed");
        return 1;
    }
    //PBGnssDebugTime time = 2;
    PBGnssDebugTime* time = pbGnssDebugReport->mutable_time();
    if (nullptr != time) {
        if (convertGnssDebugTimeToPB(gnssDebugReport.mTime, time)) {
            LOC_LOGe("convertGnssDebugTimeToPB failed");
            free(time);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_time failed");
        return 1;
    }
    //repeated PBGnssDebugSatelliteInfo satelliteInfo = 3;
    int count = gnssDebugReport.mSatelliteInfo.size();
    for (int i = 0; i < count; i++) {
        PBGnssDebugSatelliteInfo* pbSatelliteInfo = pbGnssDebugReport->add_satelliteinfo();
        if (nullptr != pbSatelliteInfo) {
            if (convertGnssDebugSatelliteInfoToPB(
                    gnssDebugReport.mSatelliteInfo[i], pbSatelliteInfo)) {
                LOC_LOGe("convertGnssDebugSatelliteInfoToPB failed");
                free(pbSatelliteInfo);
                return 1;
            }
        } else {
            LOC_LOGe("add_satelliteinfo failed");
            return 1;
        }
    }
    return 0;
}

// PBGnssDebugReport to GnssDebugReport
int LocationApiPbMsgConv::pbConvertToGnssTimespec(const PBTimespec& pbTimespec,
        timespec& timespec) const {
    timespec.tv_sec = pbTimespec.tv_sec();
    timespec.tv_nsec = pbTimespec.tv_nsec();
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssDebugTime(const PBGnssDebugTime& pbDebugTime,
        GnssDebugTime debugTime) const {
    debugTime.size = sizeof(GnssDebugTime);
    //bool valid = 1;
    debugTime.mValid = pbDebugTime.valid();
    //int64 timeEstimate = 2;
    debugTime.timeEstimate = pbDebugTime.timeestimate();
    //float timeUncertaintyNs = 3;
    debugTime.timeUncertaintyNs = pbDebugTime.timeuncertaintyns();
    //float frequencyUncertaintyNsPerSec = 4;
    debugTime.frequencyUncertaintyNsPerSec = pbDebugTime.frequencyuncertaintynspersec();
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssDebugLocation(
        const PBGnssDebugLocation& pbDebugLocation,
        GnssDebugLocation& debugLocation) const {
    debugLocation.size = sizeof(GnssDebugLocation);
    //bool valid = 1;
    debugLocation.mValid = pbDebugLocation.valid();
    //PBLocation location = 2;
    pbConvertToLocation(pbDebugLocation.location(), debugLocation.mLocation);
    //double verticalAccuracyMeters = 3;
    debugLocation.verticalAccuracyMeters = pbDebugLocation.verticalaccuracymeters();
    //double speedAccuracyMetersPerSecond = 4;
    debugLocation.speedAccuracyMetersPerSecond = pbDebugLocation.speedaccuracymeterspersecond();
    //double bearingAccuracyDegrees = 5;
    debugLocation.bearingAccuracyDegrees = pbDebugLocation.bearingaccuracydegrees();
    //PBTimespec utcReported = 6;
    pbConvertToGnssTimespec(pbDebugLocation.utcreported(), debugLocation.mUtcReported);
    return 0;
}

GnssEphemerisType LocationApiPbMsgConv::getEnumForPBGnssEphemerisType(
        const PBGnssEphemerisType& pbGnssEphemerisType) const {
    GnssEphemerisType ephemerisType = GNSS_EPH_TYPE_UNKNOWN;
    switch (pbGnssEphemerisType) {
        case PB_GNSS_EPH_TYPE_UNKNOWN:
            ephemerisType = GNSS_EPH_TYPE_UNKNOWN;
            break;
        case PB_GNSS_EPH_TYPE_EPHEMERIS:
            ephemerisType = GNSS_EPH_TYPE_EPHEMERIS;
            break;
        case PB_GNSS_EPH_TYPE_ALMANAC:
            ephemerisType = GNSS_EPH_TYPE_ALMANAC;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssephemerisType:%d, pbGnssephemerisType:%d",
            ephemerisType, pbGnssEphemerisType);
    return ephemerisType;
}

GnssEphemerisSource LocationApiPbMsgConv::getEnumForPBGnssEphemerisSource(
        const PBGnssEphemerisSource& pbGnssEphemerisSource) const {
    GnssEphemerisSource ephemerisSource = GNSS_EPH_SOURCE_UNKNOWN;
    switch (pbGnssEphemerisSource) {
        case PB_GNSS_EPH_SOURCE_UNKNOWN:
            ephemerisSource = GNSS_EPH_SOURCE_UNKNOWN;
            break;
        case PB_GNSS_EPH_SOURCE_DEMODULATED:
            ephemerisSource = GNSS_EPH_SOURCE_DEMODULATED;
            break;
        case PB_GNSS_EPH_SOURCE_SUPL_PROVIDED:
            ephemerisSource = GNSS_EPH_SOURCE_SUPL_PROVIDED;
            break;
        case PB_GNSS_EPH_SOURCE_OTHER_SERVER_PROVIDED:
            ephemerisSource = GNSS_EPH_SOURCE_OTHER_SERVER_PROVIDED;
            break;
        case PB_GNSS_EPH_SOURCE_LOCAL:
            ephemerisSource = GNSS_EPH_SOURCE_LOCAL;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssephemerisSource:%d, pbGnssephemerisSource:%d",
            ephemerisSource, pbGnssEphemerisSource);
    return ephemerisSource;
}

GnssEphemerisHealth LocationApiPbMsgConv::getEnumForPBGnssEphemerisHealth(
        const PBGnssEphemerisHealth& pbGnssEphemerisHealth) const {
    GnssEphemerisHealth ephemerisHealth = GNSS_EPH_HEALTH_UNKNOWN;
    switch (pbGnssEphemerisHealth) {
        case PB_GNSS_EPH_HEALTH_UNKNOWN:
            ephemerisHealth = GNSS_EPH_HEALTH_UNKNOWN;
            break;
        case PB_GNSS_EPH_HEALTH_GOOD:
            ephemerisHealth = GNSS_EPH_HEALTH_GOOD;
            break;
        case PB_GNSS_EPH_HEALTH_BAD:
            ephemerisHealth = GNSS_EPH_HEALTH_BAD;
            break;
        default:
            break;
    }
    LocApiPb_LOGv("LocApiPB: gnssephemerisHealth:%d, pbGnssephemerisHealth:%d",
            ephemerisHealth, pbGnssEphemerisHealth);
    return ephemerisHealth;
}

int LocationApiPbMsgConv::pbConvertToGnssDebugSatelliteInfo(
        const PBGnssDebugSatelliteInfo& pbSatelliteInfo,
        GnssDebugSatelliteInfo& satelliteInfo) const {
    satelliteInfo.size = sizeof(GnssDebugSatelliteInfo);
    //uint32 svid = 1;
    satelliteInfo.svid = pbSatelliteInfo.svid();
    //PBLocApiGnss_LocSvSystemEnumType constellation = 2;
    satelliteInfo.constellation = getGnssSvTypeFromPBGnssLocSvSystemEnumType(
            pbSatelliteInfo.constellation());
    //PBGnssEphemerisType ephemerisType = 3;
    satelliteInfo.mEphemerisType = getEnumForPBGnssEphemerisType(
            pbSatelliteInfo.ephemeristype());
    //PBGnssEphemerisSource ephemerisSource = 4;
    satelliteInfo.mEphemerisSource = getEnumForPBGnssEphemerisSource(
            pbSatelliteInfo.ephemerissource());
    //PBGnssEphemerisHealth ephemerisHealth = 5;
    satelliteInfo.mEphemerisHealth = getEnumForPBGnssEphemerisHealth(
            pbSatelliteInfo.ephemerishealth());
    //float ephemerisAgeSeconds = 6;
    satelliteInfo.ephemerisAgeSeconds = pbSatelliteInfo.ephemerisageseconds();
    //bool serverPredictionIsAvailable = 7;
    satelliteInfo.serverPredictionIsAvailable = pbSatelliteInfo.serverpredictionisavailable();
    //float serverPredictionAgeSeconds = 8;
    satelliteInfo.serverPredictionAgeSeconds = pbSatelliteInfo.serverpredictionageseconds();
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssDebugReport(
        const PBGnssDebugReport& pbGnssDebugReport,
        GnssDebugReport& gnssDebugReport) const {
    //PBGnssDebugLocation location = 1;
    pbConvertToGnssDebugLocation(pbGnssDebugReport.location(),
            gnssDebugReport.mLocation);
    //PBGnssDebugTime time = 2;
    pbConvertToGnssDebugTime(pbGnssDebugReport.time(), gnssDebugReport.mTime);
    //repeated PBGnssDebugSatelliteInfo satelliteInfo = 3;
    int count = pbGnssDebugReport.satelliteinfo_size();
    for (int i = 0; i < count; i++) {
        GnssDebugSatelliteInfo satelliteInfo;
        pbConvertToGnssDebugSatelliteInfo(pbGnssDebugReport.satelliteinfo(i), satelliteInfo);
        gnssDebugReport.mSatelliteInfo.push_back(satelliteInfo);
    }
    gnssDebugReport.size = sizeof(GnssDebugReport);
    return 0;
}

// AntennaInformation to PBAntennaInformation
int LocationApiPbMsgConv::convertAntennaInfoToPB(const AntennaInformation& antennaInfo,
        PBAntennaInformation* pbAntennaInfo) const {
    if (nullptr == pbAntennaInfo) {
        LOC_LOGe("pbAntennaInfo is NULL!, return");
        return 1;
    }

    //repeated PBGnssAntennaInformation antennaInfos = 1;
    int count = antennaInfo.antennaInfos.size();
    for (int i = 0; i < count; i++) {
        PBGnssAntennaInformation* pbGnssAntennaInfo = pbAntennaInfo->add_antennainfos();
        if (nullptr != pbGnssAntennaInfo) {
            if (convertGnssAntennaInformationToPB(
                    antennaInfo.antennaInfos[i], pbGnssAntennaInfo)) {
                LOC_LOGe("convertGnssAntennaInformationToPB failed");
                free(pbAntennaInfo);
                return 1;
            }
        } else {
            LOC_LOGe("add_antennainfos failed");
            return 1;
        }
    }
    return 0;
}

int LocationApiPbMsgConv::convertGnssAntennaInformationToPB(
        const GnssAntennaInformation& gnssAntennaInfo,
        PBGnssAntennaInformation* pbGnssAntennaInfo) const {
    if (nullptr == pbGnssAntennaInfo) {
        LOC_LOGe("pbGnssAntennaInfo is NULL!, return");
        return 1;
    }
    //double carrierFrequencyMHz = 1;
    pbGnssAntennaInfo->set_carrierfrequencymhz(gnssAntennaInfo.carrierFrequencyMHz);
    //PBGnssCoordinate phaseCenterOffsetCoordinateMillimeters = 2;
    PBGnssCoordinate* pbGnssCoordinate =
            pbGnssAntennaInfo->mutable_phasecenteroffsetcoordinatemillimeters();
    if (nullptr != pbGnssCoordinate) {
        if (convertGnssCoordinateToPB(
                gnssAntennaInfo.phaseCenterOffsetCoordinateMillimeters, pbGnssCoordinate)) {
            LOC_LOGe("convertGnssCoordinateToPB failed");
            free(pbGnssCoordinate);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_phasecenteroffsetcoordinatemillimeters failed");
    }

    //PB2DimensionDoubleVector phaseCenterVariationCorrectionMillimeters = 3;
    PB2DimensionDoubleVector* pPhaseCenterVariationCorrectionMillimeters =
        pbGnssAntennaInfo->mutable_phasecentervariationcorrectionmillimeters();
    if (nullptr != pPhaseCenterVariationCorrectionMillimeters) {
        if (convert2DimensionDoubleVectorToPB(
            gnssAntennaInfo.phaseCenterVariationCorrectionMillimeters,
            pPhaseCenterVariationCorrectionMillimeters)) {
            LOC_LOGe("convert phaseCenterVariationCorrectionMillimeters ToPB failed");
            free(pPhaseCenterVariationCorrectionMillimeters);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_phasecentervariationcorrectionmillimeters failed");
        return 1;
    }

    //PB2DimensionDoubleVector phaseCenterVariationCorrectionUncertaintyMillimeters = 4;
    PB2DimensionDoubleVector* pPhaseCenterVariationCorrectionUncertaintyMillimeters =
        pbGnssAntennaInfo->mutable_phasecentervariationcorrectionuncertaintymillimeters();
    if (nullptr != pPhaseCenterVariationCorrectionUncertaintyMillimeters) {
        if (convert2DimensionDoubleVectorToPB(
            gnssAntennaInfo.phaseCenterVariationCorrectionUncertaintyMillimeters,
            pPhaseCenterVariationCorrectionUncertaintyMillimeters)) {
            LOC_LOGe("convert phaseCenterVariationCorrectionUncertaintyMillimeters ToPB failed");
            free(pPhaseCenterVariationCorrectionUncertaintyMillimeters);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_phasecentervariationcorrectionuncertaintymillimeters failed");
        return 1;
    }

    //PB2DimensionDoubleVector signalGainCorrectionDbi = 5;
    PB2DimensionDoubleVector* pSignalGainCorrectionDbi =
        pbGnssAntennaInfo->mutable_signalgaincorrectiondbi();
    if (nullptr != pSignalGainCorrectionDbi) {
        if (convert2DimensionDoubleVectorToPB(
                gnssAntennaInfo.signalGainCorrectionDbi,
                pSignalGainCorrectionDbi)) {
            LOC_LOGe("convert signalGainCorrectionDbi ToPB failed");
            free(pSignalGainCorrectionDbi);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_signalgaincorrectiondbi failed");
        return 1;
    }

    //PB2DimensionDoubleVector signalGainCorrectionUncertaintyDbi = 6;
    PB2DimensionDoubleVector* pSignalGainCorrectionUncertaintyDbi =
        pbGnssAntennaInfo->mutable_signalgaincorrectionuncertaintydbi();
    if (nullptr != pSignalGainCorrectionUncertaintyDbi) {
        if (convert2DimensionDoubleVectorToPB(
                gnssAntennaInfo.signalGainCorrectionUncertaintyDbi,
                pSignalGainCorrectionUncertaintyDbi)) {
            LOC_LOGe("convert signalGainCorrectionUncertaintyDbi ToPB failed");
            free(pSignalGainCorrectionUncertaintyDbi);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_signalgaincorrectionuncertaintydbi failed");
    }
    return 0;
}

int LocationApiPbMsgConv::convertGnssCoordinateToPB(
        const GnssCoordinate& gnssCoordinate,
        PBGnssCoordinate* pbGnssCoordinate) const {
    if (nullptr == pbGnssCoordinate) {
        LOC_LOGe("pbGnssCoordinate is NULL!, return");
        return 1;
    }
    //double x = 1;
    pbGnssCoordinate->set_x(gnssCoordinate.x);
    //double xUncertainty = 2;
    pbGnssCoordinate->set_xuncertainty(gnssCoordinate.xUncertainty);
    //double y = 3;
    pbGnssCoordinate->set_y(gnssCoordinate.y);
    //double yUncertainty = 4;
    pbGnssCoordinate->set_yuncertainty(gnssCoordinate.yUncertainty);
    //double z = 5;
    pbGnssCoordinate->set_z(gnssCoordinate.z);
    //double zUncertainty = 6;
    pbGnssCoordinate->set_zuncertainty(gnssCoordinate.zUncertainty);
    return 0;
}

int LocationApiPbMsgConv::convert2DimensionDoubleVectorToPB(
        const std::vector<std::vector<double>>& doubleArrays,
        PB2DimensionDoubleVector* pbDoubleArrarys) const {
    if (nullptr == pbDoubleArrarys) {
        LOC_LOGe("pbDoubleArrarys is NULL!, return");
        return 1;
    }
    //uint32 row = 1;
    uint32_t row = doubleArrays.size();
    pbDoubleArrarys->set_row(row);

    //uint32 column = 2;
    uint32_t column = 0;
    //repeated double value = 3;
    for (uint32_t i = 0; i < row; i++) {
        column = doubleArrays[i].size();
        for (uint32_t j = 0; j < column; j++) {
            pbDoubleArrarys->add_value(doubleArrays[i][j]);
        }
    }
    pbDoubleArrarys->set_column(column);
    return 0;
}

// PBAntennaInformation to AntennaInformation
int LocationApiPbMsgConv::pbConvertToAntennaInfo(
        const PBAntennaInformation& pbAntennaInfo,
        AntennaInformation& antennaInfo) const {
    //repeated PBGnssAntennaInformation antennaInfos = 1;
    int count = pbAntennaInfo.antennainfos_size();
    for (int i = 0; i < count; i++) {
        GnssAntennaInformation gnssAntennaInfo;
        pbConvertToGnssAntennaInformaiton(pbAntennaInfo.antennainfos(i), gnssAntennaInfo);
        antennaInfo.antennaInfos.push_back(std::move(gnssAntennaInfo));
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssAntennaInformaiton(
        const PBGnssAntennaInformation& pbGnssAntennaInfo,
        GnssAntennaInformation& gnssAntennaInfo) const {
    gnssAntennaInfo.size = sizeof(GnssAntennaInformation);
    //double carrierFrequencyMHz = 1;
    gnssAntennaInfo.carrierFrequencyMHz = pbGnssAntennaInfo.carrierfrequencymhz();
    //PBGnssCoordinate phaseCenterOffsetCoordinateMillimeters = 2;
    pbConvertToGnssCoordinate(
        pbGnssAntennaInfo.phasecenteroffsetcoordinatemillimeters(),
        gnssAntennaInfo.phaseCenterOffsetCoordinateMillimeters);
    //PB2DimensionDoubleVector phaseCenterVariationCorrectionMillimeters = 3;
    pbConvertTo2DimensionDoubleVector(
        pbGnssAntennaInfo.phasecentervariationcorrectionmillimeters(),
        gnssAntennaInfo.phaseCenterVariationCorrectionMillimeters);
    //PB2DimensionDoubleVector phaseCenterVariationCorrectionUncertaintyMillimeters = 4;
    pbConvertTo2DimensionDoubleVector(
        pbGnssAntennaInfo.phasecentervariationcorrectionuncertaintymillimeters(),
        gnssAntennaInfo.phaseCenterVariationCorrectionUncertaintyMillimeters);
    //PB2DimensionDoubleVector signalGainCorrectionDbi = 5;
    pbConvertTo2DimensionDoubleVector(
        pbGnssAntennaInfo.signalgaincorrectiondbi(),
        gnssAntennaInfo.signalGainCorrectionDbi);
    //PB2DimensionDoubleVector signalGainCorrectionUncertaintyDbi = 6;
    pbConvertTo2DimensionDoubleVector(
        pbGnssAntennaInfo.signalgaincorrectionuncertaintydbi(),
        gnssAntennaInfo.signalGainCorrectionUncertaintyDbi);
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssCoordinate(
        const PBGnssCoordinate& pbGnssCoordinate,
        GnssCoordinate& gnssCoordinate) const {
    gnssCoordinate.size = sizeof(GnssCoordinate);
    //double x = 1;
    gnssCoordinate.x = pbGnssCoordinate.x();
    //double xUncertainty = 2;
    gnssCoordinate.xUncertainty = pbGnssCoordinate.xuncertainty();
    //double y = 3;
    gnssCoordinate.y = pbGnssCoordinate.y();
    //double yUncertainty = 4;
    gnssCoordinate.yUncertainty = pbGnssCoordinate.yuncertainty();
    //double z = 5;
    gnssCoordinate.z = pbGnssCoordinate.z();
    //double zUncertainty = 6;
    gnssCoordinate.zUncertainty = pbGnssCoordinate.zuncertainty();
    return 0;
}

int LocationApiPbMsgConv::pbConvertTo2DimensionDoubleVector(
        const PB2DimensionDoubleVector& pbDoubleArrays,
        std::vector<std::vector<double>>& doubleArrays) const {
    //uint32 row = 1;
    uint32_t row = pbDoubleArrays.row();
    //uint32 column = 2;
    uint32_t column = pbDoubleArrays.column();
    //repeated double value = 3;
    for (uint32_t i = 0; i < row; i++) {
        std::vector<double> dVector;
        dVector.clear();
        for (uint32_t j = 0; j < column; j++) {
            dVector.push_back(pbDoubleArrays.value(i * column + j));
        }
        doubleArrays.push_back(std::move(dVector));
    }
    return 0;
}

PBGnssEphAction LocationApiPbMsgConv::getPBEphAction (const GnssEphAction& ephAction) const {
    PBGnssEphAction pbEphAction = PB_GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02;
    switch (ephAction) {
        case GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02:
            pbEphAction = PB_GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02;
            break;
        case GNSS_EPH_ACTION_UPDATE_SRC_OTA_V02:
            pbEphAction = PB_GNSS_EPH_ACTION_UPDATE_SRC_OTA_V02;
            break;
        case GNSS_EPH_ACTION_UPDATE_SRC_NETWORK_V02:
            pbEphAction = PB_GNSS_EPH_ACTION_UPDATE_SRC_NETWORK_V02;
            break;
        case GNSS_EPH_ACTION_UPDATE_MAX_V02:
            pbEphAction = PB_GNSS_EPH_ACTION_UPDATE_MAX_V02;
            break;
        case GNSS_EPH_ACTION_DELETE_SRC_UNKNOWN_V02:
            pbEphAction = PB_GNSS_EPH_ACTION_DELETE_SRC_UNKNOWN_V02;
            break;
        case GNSS_EPH_ACTION_DELETE_SRC_NETWORK_V02:
            pbEphAction = PB_GNSS_EPH_ACTION_DELETE_SRC_NETWORK_V02;
            break;
        case GNSS_EPH_ACTION_DELETE_SRC_OTA_V02:
            pbEphAction = PB_GNSS_EPH_ACTION_DELETE_SRC_OTA_V02;
            break;
        case GNSS_EPH_ACTION_DELETE_MAX_V02:
            pbEphAction = PB_GNSS_EPH_ACTION_DELETE_MAX_V02;
            break;
    }
    return pbEphAction;
}

int LocationApiPbMsgConv::convertCommanEphToPB (
    const GnssEphCommon &commanEph,
    PBGnssEphCommon* pbCommanEph) const {

    if (pbCommanEph) {
        // uint32 gnssSvId = 1;
        pbCommanEph->set_gnsssvid(commanEph.gnssSvId);

        // PBGnssEphAction ephAction = 2;
        pbCommanEph->set_ephaction(getPBEphAction(commanEph.updateAction));

        // uint32 IODE = 3;
        pbCommanEph->set_iode(commanEph.IODE);

        // double aSqrt = 4;
        pbCommanEph->set_asqrt(commanEph.aSqrt);

        // double deltaN = 5;
        pbCommanEph->set_deltan(commanEph.deltaN);

        // double m0 = 6;
        pbCommanEph->set_m0(commanEph.m0);

        // double eccentricity = 7;
        pbCommanEph->set_eccentricity(commanEph.eccentricity);

        // double omega0 = 8;
        pbCommanEph->set_omega0(commanEph.omega0);

        // double i0 = 9;
        pbCommanEph->set_i0(commanEph.i0);

        // double omega = 10;
        pbCommanEph->set_omega(commanEph.omega);

        // double omegaDot = 11;
        pbCommanEph->set_omegadot(commanEph.omegaDot);

        // double iDot = 12;
        pbCommanEph->set_idot(commanEph.iDot);

        // double cUc = 13;
        pbCommanEph->set_cuc(commanEph.cUc);

        // double cUs = 14;
        pbCommanEph->set_cus(commanEph.cUs);

        // double cRc = 15;
        pbCommanEph->set_crc(commanEph.cRc);

        // double cRs = 16;
        pbCommanEph->set_crs(commanEph.cRs);

        // double cIc = 17;
        pbCommanEph->set_cic(commanEph.cIc);

        // double cIs = 18;
        pbCommanEph->set_cis(commanEph.cIs);

        // uint32 toe = 19;
        pbCommanEph->set_toe(commanEph.toe);

        // uint32 toc = 20;
        pbCommanEph->set_toc(commanEph.toc);

        // double af0 = 21;
        pbCommanEph->set_af0(commanEph.af0);

        // double af1 = 22;
        pbCommanEph->set_af1(commanEph.af1);

        // double af2 = 23;
        pbCommanEph->set_af2(commanEph.af2);
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertGpsEphDataToPB(
    const GpsEphemeris &halEphInfo,
    PBGpsEphemeris *pbEphInfo) const {

    if (pbEphInfo) {
        PBGnssEphCommon* pbcommanEph = pbEphInfo->mutable_commonephemerisdata();
        if (nullptr != pbcommanEph) {
            if (convertCommanEphToPB(halEphInfo.commonEphemerisData, pbcommanEph)) {
                LOC_LOGe(" convertCommanEphtoPB failed ");
                free(pbcommanEph);
            }
        } else {
            LOC_LOGe(" mutable_commonephemerisdata is nullptr ");
        }

        pbEphInfo->set_signalhealth(halEphInfo.signalHealth);
        pbEphInfo->set_urai(halEphInfo.URAI);
        pbEphInfo->set_codel2(halEphInfo.codeL2);
        pbEphInfo->set_dataflagl2p(halEphInfo.dataFlagL2P);
        pbEphInfo->set_tgd(halEphInfo.tgd);
        pbEphInfo->set_fitinterval(halEphInfo.fitInterval);
        pbEphInfo->set_iodc(halEphInfo.IODC);
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertGpsEphResponseToPB(
    const GpsEphemerisResponse  &halResp,
    PBGpsEphemerisResponse*  pbEphResp) const {

    if (pbEphResp) {
        for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
            PBGpsEphemeris *pbEph = pbEphResp->add_gpsephemerisdata();
            if (nullptr != pbEph) {
                if (convertGpsEphDataToPB(halResp.gpsEphemerisData[idx], pbEph)) {
                    LOC_LOGe(" Failed convertGpsEphData");
                    free(pbEph);
                    return 1;
                }
            } else {
                LOC_LOGe(" add_gpsephemerisdata is NULL ");
                return 1;
            }
        }
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertBdsEphDataToPB(
    const BdsEphemeris &halEphInfo,
    PBBdsEphemeris *pbEphInfo) const {

    if (pbEphInfo) {
        PBGnssEphCommon* pbcommanEph = pbEphInfo->mutable_commonephemerisdata();
        if (nullptr != pbcommanEph) {
            if (convertCommanEphToPB(halEphInfo.commonEphemerisData, pbcommanEph)) {
                LOC_LOGe(" convertCommanEphtoPB failed ");
                free(pbcommanEph);
            }
        } else {
            LOC_LOGe(" mutable_commonephemerisdata is nullptr ");
        }
        // uint32 svHealth = 2;
        pbEphInfo->set_svhealth(halEphInfo.svHealth);
        // uint32 AODC = 3;
        pbEphInfo->set_aodc(halEphInfo.AODC);
        // double tgd1 = 4;
        pbEphInfo->set_tgd1(halEphInfo.tgd1);
        //  double tgd2 = 5;
        pbEphInfo->set_tgd2(halEphInfo.tgd2);
        // uint32 URAI = 6;
        pbEphInfo->set_urai(halEphInfo.URAI);
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertBdsEphResponseToPB(
    const BdsEphemerisResponse  &halResp,
    PBBdsEphemerisResponse*  pbEphResp) const {

    if (pbEphResp) {
        for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
            PBBdsEphemeris *pbEph = pbEphResp->add_bdsephemerisdata();
            if (nullptr != pbEph) {
                if (convertBdsEphDataToPB(halResp.bdsEphemerisData[idx], pbEph)) {
                    LOC_LOGe(" Failed convertBdsEphData");
                    free(pbEph);
                    return 1;
                }
            } else {
                LOC_LOGe(" add_bdsephemerisdata is NULL ");
                return 1;
            }
        }
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertGlonassEphDataToPB(
    const GlonassEphemeris &halEphInfo,
    PBGlonassEphemeris *pbEphInfo) const {

    if (pbEphInfo) {
        // uint32 gnssSvId  = 1;
        pbEphInfo->set_gnsssvid(halEphInfo.gnssSvId);
        // PBGnssEphAction ephAction = 2;
        pbEphInfo->set_ephaction(getPBEphAction(halEphInfo.updateAction));
        // uint32 bnHealth = 3;
        pbEphInfo->set_bnhealth(halEphInfo.bnHealth);
        //  double bnHealth = 4;
        pbEphInfo->set_lnhealth(halEphInfo.lnHealth);
        // uint32 tb = 5;
        pbEphInfo->set_tb(halEphInfo.tb);
        // uint32 ft = 6;
        pbEphInfo->set_ft(halEphInfo.ft);
        // uint32 gloM = 7;
        pbEphInfo->set_glom(halEphInfo.gloM);
        // uint32 enAge = 8;
        pbEphInfo->set_enage(halEphInfo.enAge);
        //  uint32 gloFrequency = 9;
        pbEphInfo->set_glofrequency(halEphInfo.gloFrequency);
        // uint32 p1 = 10;
        pbEphInfo->set_p1(halEphInfo.p1);
        // uint32 p2 = 11;
        pbEphInfo->set_p2(halEphInfo.p2);
        // float deltaTau = 12;
        pbEphInfo->set_deltatau(halEphInfo.deltaTau);
        for (int i = 0; i < 3; i++) {
            // double position[3] = 13;
            pbEphInfo->add_position(halEphInfo.position[i]);
            // double velocity[3] = 14;
            pbEphInfo->add_velocity(halEphInfo.velocity[i]);
            // double acceleration[3] = 15;
            pbEphInfo->add_acceleration(halEphInfo.acceleration[i]);
        }
        //  float tauN = 16;
        pbEphInfo->set_taun(halEphInfo.tauN);
        // float gamma = 17;
        pbEphInfo->set_gamma(halEphInfo.gamma);
        // double toe = 18;
        pbEphInfo->set_toe(halEphInfo.toe);
        // uint32 nt = 19;
        pbEphInfo->set_nt(halEphInfo.nt);
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertGloEphResponseToPB(
    const GlonassEphemerisResponse  &halResp,
    PBGlonassEphemerisResponse*  pbEphResp) const {

    if (pbEphResp) {

        for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
            PBGlonassEphemeris *pbEph = pbEphResp->add_gloephemerisdata();
            if (nullptr != pbEph) {
                if (convertGlonassEphDataToPB(halResp.gloEphemerisData[idx], pbEph)) {
                    LOC_LOGe(" Failed convertGlonassEphDatatoPB ");
                    free(pbEph);
                    return 1;
                }
            } else {
                LOC_LOGe(" add_glonassephemerisdata is NULL ");
                return 1;
            }
        }
        return 0;
    } else {
        return 1;
    }
}

PBGalEphSignalSource LocationApiPbMsgConv::convertDataSignalSourceToPB(
    const GalEphSignalSource &ephSignal) const {

    PBGalEphSignalSource pbSignal = PB_GAL_EPH_SIGNAL_SRC_UNKNOWN;

    switch (ephSignal) {
        case GAL_EPH_SIGNAL_SRC_E1B_V02:
            pbSignal = PB_GAL_EPH_SIGNAL_SRC_E1B;
            break;
        case GAL_EPH_SIGNAL_SRC_E5A_V02:
            pbSignal = PB_GAL_EPH_SIGNAL_SRC_E5A;
            break;
        case GAL_EPH_SIGNAL_SRC_E5B_V02:
            pbSignal = PB_GAL_EPH_SIGNAL_SRC_E5B;
            break;
        default:
            pbSignal = PB_GAL_EPH_SIGNAL_SRC_UNKNOWN;
            break;
    }
    return pbSignal;
}

int LocationApiPbMsgConv::convertGalileoEphDataToPB(
    const GalileoEphemeris &halEphInfo,
    PBGalileoEphemeris *pbEphInfo) const {

    if (pbEphInfo) {
        PBGnssEphCommon* pbcommanEph = pbEphInfo->mutable_commonephemerisdata();
        if (nullptr != pbcommanEph) {
            if (convertCommanEphToPB(halEphInfo.commonEphemerisData, pbcommanEph)) {
                LOC_LOGe(" convertCommanEphtoPB failed ");
                free(pbcommanEph);
            }
        } else {
            LOC_LOGe(" mutable_commonephemerisdata is nullptr ");
        }
        //PBGalEphSignalSource dataSourceSignal = 2;
        pbEphInfo->set_datasourcesignal(convertDataSignalSourceToPB(halEphInfo.dataSourceSignal));
        // uint32 sisIndex = 3;
        pbEphInfo->set_sisindex(halEphInfo.sisIndex);
        // double bgdE1E5a = 4;
        pbEphInfo->set_bgde1e5a(halEphInfo.bgdE1E5a);
        //  double bgdE1E5b = 5;
        pbEphInfo->set_bgde1e5b(halEphInfo.bgdE1E5b);
        // uint32 svHealth = 6;
        pbEphInfo->set_svhealth(halEphInfo.svHealth);
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertGalEphResponseToPB(
    const GalileoEphemerisResponse  &halResp,
    PBGalileoEphemerisResponse*  pbEphResp) const {

    if (pbEphResp) {
        for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
            PBGalileoEphemeris *pbEph = pbEphResp->add_galephemerisdata();
            if (nullptr != pbEph) {
                if (convertGalileoEphDataToPB(halResp.galEphemerisData[idx], pbEph)) {
                    LOC_LOGe(" Failed convertGalileoEphDatatoPB ");
                    free(pbEph);
                    return 1;
                }
            } else {
                LOC_LOGe(" add_galileoephemerisdata is NULL ");
                return 1;
            }
        }
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertQzssEphResponseToPB(
    const QzssEphemerisResponse  &halResp,
    PBQzssEphemerisResponse*  pbEphResp) const {

    if (pbEphResp) {
        for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
            PBGpsEphemeris *pbEph = pbEphResp->add_qzssephemerisdata();
            if (nullptr != pbEph) {
                if (convertGpsEphDataToPB(halResp.qzssEphemerisData[idx], pbEph)) {
                    LOC_LOGe(" Failed convertGpsEphDatatoPB for QZSS ");
                    free(pbEph);
                    return 1;
                }
            } else {
                LOC_LOGe(" add_qzssephemerisdata is NULL ");
                return 1;
            }
        }
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertNavicEphDataToPB(
    const NavicEphemeris &halEphInfo,
    PBNavicEphemeris *pbEphInfo) const {

    if (pbEphInfo) {
        PBGnssEphCommon* pbcommanEph = pbEphInfo->mutable_commonephemerisdata();
        if (nullptr != pbcommanEph) {
            if (convertCommanEphToPB(halEphInfo.commonEphemerisData, pbcommanEph)) {
                LOC_LOGe(" convertCommanEphtoPB failed ");
                free(pbcommanEph);
            }
        } else {
            LOC_LOGe(" mutable_commonephemerisdata is nullptr ");
        }
        // uint32 weekNum = 2;
        pbEphInfo->set_weeknum(halEphInfo.weekNum);
        // uint32 iodec = 3;
        pbEphInfo->set_iodec(halEphInfo.iodec);
        // uint32 l5Health = 4;
        pbEphInfo->set_l5health(halEphInfo.l5Health);
        //  uint32 sHealth = 5;
        pbEphInfo->set_shealth(halEphInfo.sHealth);
        // double inclinationAngleRad = 6;
        pbEphInfo->set_inclinationanglerad(halEphInfo.inclinationAngleRad);
        // uint32 urai = 7;
        pbEphInfo->set_urai(halEphInfo.urai);
        // double  tgd = 8;
        pbEphInfo->set_tgd(halEphInfo.tgd);
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertNavicEphResponseToPB(
    const NavicEphemerisResponse  &halResp,
    PBNavicEphemerisResponse*  pbEphResp) const {

    if (pbEphResp) {
        for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
            PBNavicEphemeris *pbEph = pbEphResp->add_navicephemerisdata();
            if (nullptr != pbEph) {
                if (convertNavicEphDataToPB(halResp.navicEphemerisData[idx], pbEph)) {
                    LOC_LOGe(" Failed convertNavicEphDatatoPB ");
                    free(pbEph);
                    return 1;
                }
            } else {
                LOC_LOGe(" add_navicephemerisdata is NULL ");
                return 1;
            }
        }
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::convertGnssEphNotifToPB(
        const GnssSvEphemerisReport &gnssEphNotif,
        PBGnssEphemerisNotification *pbGnssEphNotif) const {

    if (nullptr == pbGnssEphNotif) {
        LOC_LOGe("pbGnssEphNotif is NULL!, return");
        return 1;
    }
    pbGnssEphNotif->set_gnssconstellation(
            getPBEnumForGnssLocSvSystem(gnssEphNotif.gnssConstellation));
    pbGnssEphNotif->set_issystemtimevalid(gnssEphNotif.isSystemTimeValid);

    PBLocApiGnssSystemTimeStructType*  sysTime = pbGnssEphNotif->mutable_systemtime();
    if (nullptr != sysTime) {
        if (convertGnssSystemTimeStructTypeToPB(gnssEphNotif.systemTime,
                sysTime)) {
            LOC_LOGe("convertGnssSystemTimeStructTypeToPB failed");
            free(sysTime);
            return 1;
        }
    } else {
        LOC_LOGe("mutable_systemtime failed");
        return 1;
    }
    PBEphInfoUnion *ephU = pbGnssEphNotif->mutable_ephunion();
    if (ephU) {
        switch (gnssEphNotif.gnssConstellation) {
            case GNSS_LOC_SV_SYSTEM_GPS: {
                PBGpsEphemerisResponse* ephResp = ephU->mutable_gpsephemeris();
                if (nullptr != ephResp) {
                    if (convertGpsEphResponseToPB(gnssEphNotif.ephInfo.gpsEphemeris, ephResp)) {
                        LOC_LOGe("Failed to convertGpsEphResponsetoPB");
                        free(ephResp);
                        return 1;
                    }
                } else {
                    LOC_LOGe("mutable_gpsephemeris failed");
                    return 1;
                }
                break;
            }
            case GNSS_LOC_SV_SYSTEM_GALILEO: {
                PBGalileoEphemerisResponse* ephRespGal = ephU->mutable_galileoephemeris();
                if (nullptr != ephRespGal) {
                    if (convertGalEphResponseToPB(gnssEphNotif.ephInfo.galileoEphemeris,
                            ephRespGal)) {
                        LOC_LOGe("Failed to convertGalEphResponsetoPB");
                        free(ephRespGal);
                        return 1;
                    }
                } else {
                    LOC_LOGe("mutable_galileoephemeris failed");
                    return 1;
                }
                break;
            }
            case GNSS_LOC_SV_SYSTEM_GLONASS: {
                PBGlonassEphemerisResponse* ephRespGlo = ephU->mutable_glonassephemeris();
                if (nullptr != ephRespGlo) {
                    if (convertGloEphResponseToPB(gnssEphNotif.ephInfo.glonassEphemeris,
                            ephRespGlo)) {
                        LOC_LOGe("Failed to convertGloEphResponsetoPB");
                        free(ephRespGlo);
                        return 1;
                    }
                } else {
                    LOC_LOGe("mutable_glonassephemeris failed");
                    return 1;
                }
                break;
            }
            case GNSS_LOC_SV_SYSTEM_BDS: {
                PBBdsEphemerisResponse* ephRespBds = ephU->mutable_bdsephemeris();
                if (nullptr != ephRespBds) {
                    if (convertBdsEphResponseToPB(gnssEphNotif.ephInfo.bdsEphemeris, ephRespBds)) {
                        LOC_LOGe("Failed to convertBdsEphResponsetoPB");
                        free(ephRespBds);
                        return 1;
                    }
                } else {
                    LOC_LOGe("mutable_bdsephemeris failed");
                    return 1;
                }
                break;
            }
            case GNSS_LOC_SV_SYSTEM_QZSS: {
                PBQzssEphemerisResponse* ephRespQzss = ephU->mutable_qzssephemeris();
                if (nullptr != ephRespQzss) {
                    if (convertQzssEphResponseToPB(gnssEphNotif.ephInfo.qzssEphemeris,
                             ephRespQzss)) {
                        LOC_LOGe("Failed to convertQzssEphResponsetoPB");
                        free(ephRespQzss);
                        return 1;
                    }
                } else {
                    LOC_LOGe("mutable_qzssephemeris failed");
                    return 1;
                }
                break;
            }
            case GNSS_LOC_SV_SYSTEM_NAVIC: {
                PBNavicEphemerisResponse* ephRespNavic = ephU->mutable_navicephemeris();
                if (nullptr != ephRespNavic) {
                    if (convertNavicEphResponseToPB(gnssEphNotif.ephInfo.navicEphemeris,
                            ephRespNavic)) {
                        LOC_LOGe("Failed to convertNavicEphResponsetoPB");
                        free(ephRespNavic);
                        return 1;
                    }
                } else {
                    LOC_LOGe("mutable_navicephemeris failed");
                    return 1;
                }
                break;
            }
        }
    } else {
        LOC_LOGe("mutable_ephunion failed");
        return 1;
    }
    return 0;
}

GnssEphAction LocationApiPbMsgConv::pbConvertToEphAction (
            const PBGnssEphAction& pbEphAction) const {
    GnssEphAction ephAction = GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02;
    switch (pbEphAction) {
        case PB_GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02:
            ephAction = GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02;
            break;
        case PB_GNSS_EPH_ACTION_UPDATE_SRC_OTA_V02:
            ephAction = GNSS_EPH_ACTION_UPDATE_SRC_OTA_V02;
            break;
        case PB_GNSS_EPH_ACTION_UPDATE_SRC_NETWORK_V02:
            ephAction = GNSS_EPH_ACTION_UPDATE_SRC_NETWORK_V02;
            break;
        case PB_GNSS_EPH_ACTION_UPDATE_MAX_V02:
            ephAction = GNSS_EPH_ACTION_UPDATE_MAX_V02;
            break;
        case PB_GNSS_EPH_ACTION_DELETE_SRC_UNKNOWN_V02:
            ephAction = GNSS_EPH_ACTION_DELETE_SRC_UNKNOWN_V02;
            break;
        case PB_GNSS_EPH_ACTION_DELETE_SRC_NETWORK_V02:
            ephAction = GNSS_EPH_ACTION_DELETE_SRC_NETWORK_V02;
            break;
        case PB_GNSS_EPH_ACTION_DELETE_SRC_OTA_V02:
            ephAction = GNSS_EPH_ACTION_DELETE_SRC_OTA_V02;
            break;
        case PB_GNSS_EPH_ACTION_DELETE_MAX_V02:
            ephAction = GNSS_EPH_ACTION_DELETE_MAX_V02;
            break;
    }
    return ephAction;
}

int LocationApiPbMsgConv::pbConvertToCommanEph (
    const PBGnssEphCommon &pbCommanEph,
    GnssEphCommon &commanEph) const {

    // uint32 gnssSvId = 1;
    commanEph.gnssSvId = pbCommanEph.gnsssvid();

    // PBGnssEphAction ephAction = 2;
    commanEph.updateAction = pbConvertToEphAction(pbCommanEph.ephaction());

    // uint32 IODE = 3;
    commanEph.IODE = pbCommanEph.iode();

    // double aSqrt = 4;
    commanEph.aSqrt = pbCommanEph.asqrt();

    // double deltaN = 5;
    commanEph.deltaN = pbCommanEph.deltan();

    // double m0 = 6;
    commanEph.m0 = pbCommanEph.m0();

    // double eccentricity = 7;
    commanEph.eccentricity = pbCommanEph.eccentricity();

    // double omega0 = 8;
    commanEph.omega0 = pbCommanEph.omega0();

    // double i0 = 9;
    commanEph.i0 = pbCommanEph.i0();

    // double omega = 10;
    commanEph.omega = pbCommanEph.omega();

    // double omegaDot = 11;
    commanEph.omegaDot = pbCommanEph.omegadot();

    // double iDot = 12;
    commanEph.iDot = pbCommanEph.idot();

    // double cUc = 13;
    commanEph.cUc = pbCommanEph.cuc();

    // double cUs = 14;
    commanEph.cUs = pbCommanEph.cus();

    // double cRc = 15;
    commanEph.cRc = pbCommanEph.crc();

    // double cRs = 16;
    commanEph.cRs = pbCommanEph.crs();

    // double cIc = 17;
    commanEph.cIc = pbCommanEph.cic();

    // double cIs = 18;
    commanEph.cIs = pbCommanEph.cis();

    // uint32 toe = 19;
    commanEph.toe = pbCommanEph.toe();

    // uint32 toc = 20;
    commanEph.toc = pbCommanEph.toc();

    // double af0 = 21;
    commanEph.af0 = pbCommanEph.af0();

    // double af1 = 22;
    commanEph.af1 = pbCommanEph.af1();

    // double af2 = 23;
    commanEph.af2 = pbCommanEph.af2();
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGpsEphData(
    const PBGpsEphemeris &pbEphInfo,
    GpsEphemeris &halEphInfo) const {

    pbConvertToCommanEph(pbEphInfo.commonephemerisdata(), halEphInfo.commonEphemerisData);
    halEphInfo.signalHealth = pbEphInfo.signalhealth();
    halEphInfo.URAI         = pbEphInfo.urai();
    halEphInfo.codeL2       = pbEphInfo.codel2();
    halEphInfo.dataFlagL2P  = pbEphInfo.dataflagl2p();
    halEphInfo.tgd          = pbEphInfo.tgd();
    halEphInfo.fitInterval  = pbEphInfo.fitinterval();
    halEphInfo.IODC         = pbEphInfo.iodc();

    return 0;
}

int LocationApiPbMsgConv::pbConvertToGpsEphResponse(
    const PBGpsEphemerisResponse  &pbEphResp,
    GpsEphemerisResponse  &halResp) const {

    halResp.numOfEphemeris = pbEphResp.gpsephemerisdata_size();

    for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
        pbConvertToGpsEphData(pbEphResp.gpsephemerisdata(idx),
                halResp.gpsEphemerisData[idx]);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToBdsEphData(
    const PBBdsEphemeris &pbEphInfo,
    BdsEphemeris &halEphInfo) const {

    pbConvertToCommanEph(pbEphInfo.commonephemerisdata(), halEphInfo.commonEphemerisData);
    // uint32 svHealth = 2;
    halEphInfo.svHealth = pbEphInfo.svhealth();
    // uint32 AODC = 3;
    halEphInfo.AODC = pbEphInfo.aodc();
    // double tgd1 = 4;
    halEphInfo.tgd1 = pbEphInfo.tgd1();
    //  double tgd2 = 5;
    halEphInfo.tgd2 = pbEphInfo.tgd2();
    // uint32 URAI = 6;
    halEphInfo.URAI = pbEphInfo.urai();
    return 0;

}

int LocationApiPbMsgConv::pbConvertToBdsEphResponse(
    const PBBdsEphemerisResponse &pbEphResp,
    BdsEphemerisResponse  &halResp) const {

    halResp.numOfEphemeris = pbEphResp.bdsephemerisdata_size();
    for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
        pbConvertToBdsEphData(pbEphResp.bdsephemerisdata(idx),
                halResp.bdsEphemerisData[idx]);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGlonassEphData(
    const  PBGlonassEphemeris &pbEphInfo,
    GlonassEphemeris &halEphInfo) const {

    // uint32 gnssSvId  = 1;
    halEphInfo.gnssSvId = pbEphInfo.gnsssvid();
    // PBGnssEphAction ephAction = 2;
    halEphInfo.updateAction = pbConvertToEphAction(pbEphInfo.ephaction());
    // uint32 bnHealth = 3;
    halEphInfo.bnHealth = pbEphInfo.bnhealth();
    //  double bnHealth = 4;
    halEphInfo.lnHealth = pbEphInfo.lnhealth();
    // uint32 tb = 5;
    halEphInfo.tb = pbEphInfo.tb();
    // uint32 ft = 6;
    halEphInfo.ft = pbEphInfo.ft();
    // uint32 gloM = 7;
    halEphInfo.gloM = pbEphInfo.glom();
    // uint32 enAge = 8;
    halEphInfo.enAge = pbEphInfo.enage();
    //  uint32 gloFrequency = 9;
    halEphInfo.gloFrequency = pbEphInfo.glofrequency();
    // uint32 p1 = 10;
    halEphInfo.p1 = pbEphInfo.p1();
    // uint32 p2 = 11;
    halEphInfo.p2 = pbEphInfo.p2();
    // float deltaTau = 12;
    halEphInfo.deltaTau = pbEphInfo.deltatau();

    for (int i = 0; i < 3; i++) {
        // double position[3] = 13;
        halEphInfo.position[i] = pbEphInfo.position(i);
        // double velocity[3] = 14;
        halEphInfo.velocity[i] = pbEphInfo.velocity(i);
        // double acceleration[3] = 15;
        halEphInfo.acceleration[i] = pbEphInfo.acceleration(i);
    }

    //  float tauN = 16;
    halEphInfo.tauN = pbEphInfo.taun();
    // float gamma = 17;
    halEphInfo.gamma = pbEphInfo.gamma();
    // double toe = 18;
    halEphInfo.toe = pbEphInfo.toe();
    // uint32 nt = 19;
    halEphInfo.nt = pbEphInfo.nt();
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGloEphResponse(
    const PBGlonassEphemerisResponse  &pbEphResp,
    GlonassEphemerisResponse  &halResp) const {

    halResp.numOfEphemeris = pbEphResp.gloephemerisdata_size();
    for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
        pbConvertToGlonassEphData(pbEphResp.gloephemerisdata(idx),
                halResp.gloEphemerisData[idx]);
    }
    return 0;
}

GalEphSignalSource LocationApiPbMsgConv::pbConvertToDataSourceSignal(
    const PBGalEphSignalSource &pbSignal) const {
    GalEphSignalSource ephSignal = GAL_EPH_SIGNAL_SRC_UNKNOWN_V02;

    switch (pbSignal) {
        case PB_GAL_EPH_SIGNAL_SRC_E1B:
            ephSignal = GAL_EPH_SIGNAL_SRC_E1B_V02;
            break;
        case PB_GAL_EPH_SIGNAL_SRC_E5A:
            ephSignal = GAL_EPH_SIGNAL_SRC_E5A_V02;
            break;
        case PB_GAL_EPH_SIGNAL_SRC_E5B:
            ephSignal = GAL_EPH_SIGNAL_SRC_E5B_V02;
            break;
        default:
            ephSignal = GAL_EPH_SIGNAL_SRC_UNKNOWN_V02;
            break;
    }
    return ephSignal;
}

int LocationApiPbMsgConv::pbConvertToGalileoEphData(
    const  PBGalileoEphemeris &pbEphInfo,
    GalileoEphemeris &halEphInfo) const {

    pbConvertToCommanEph(pbEphInfo.commonephemerisdata(), halEphInfo.commonEphemerisData);
    // PBGalEphSignalSource dataSourceSignal = 2;
    halEphInfo.dataSourceSignal = pbConvertToDataSourceSignal(pbEphInfo.datasourcesignal());
    // uint32 sisIndex = 3;
    halEphInfo.sisIndex = pbEphInfo.sisindex();
    // double bgdE1E5a = 4;
    halEphInfo.bgdE1E5a = pbEphInfo.bgde1e5a();
    //  double bgdE1E5b = 5;
    halEphInfo.bgdE1E5b = pbEphInfo.bgde1e5b();
    // uint32 svHealth = 6;
    halEphInfo.svHealth = pbEphInfo.svhealth();
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGalEphResponse(
    const PBGalileoEphemerisResponse  &pbEphResp,
    GalileoEphemerisResponse  &halResp) const {
    halResp.numOfEphemeris = pbEphResp.galephemerisdata_size();
    for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
        pbConvertToGalileoEphData(pbEphResp.galephemerisdata(idx),
                halResp.galEphemerisData[idx]);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToQzssEphResponse(
    const PBQzssEphemerisResponse  &pbEphResp,
    QzssEphemerisResponse  &halResp) const {

    halResp.numOfEphemeris = pbEphResp.qzssephemerisdata_size();
    for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
        pbConvertToGpsEphData(pbEphResp.qzssephemerisdata(idx),
                halResp.qzssEphemerisData[idx]);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToNavicEphData(
    const PBNavicEphemeris &pbEphInfo,
    NavicEphemeris &halEphInfo) const {
    pbConvertToCommanEph(pbEphInfo.commonephemerisdata(), halEphInfo.commonEphemerisData);
    // uint32 weekNum = 2;
    halEphInfo.weekNum = pbEphInfo.weeknum();
    // uint32 iodec = 3;
    halEphInfo.iodec = pbEphInfo.iodec();
    // uint32 l5Health = 4;
    halEphInfo.l5Health = pbEphInfo.l5health();
    //  uint32 sHealth = 5;
    halEphInfo.sHealth = pbEphInfo.shealth();
    // double inclinationAngleRad = 6;
    halEphInfo.inclinationAngleRad = pbEphInfo.inclinationanglerad();
    // uint32 urai = 7;
    halEphInfo.urai = pbEphInfo.urai();
    // double  tgd = 8;
    halEphInfo.tgd = pbEphInfo.tgd();
    return 0;
}

int LocationApiPbMsgConv::pbConvertToNavicEphResponse(
    const PBNavicEphemerisResponse  &pbEphResp,
    NavicEphemerisResponse  &halResp) const {

    halResp.numOfEphemeris = pbEphResp.navicephemerisdata_size();
    for (uint32_t idx = 0; idx < halResp.numOfEphemeris; idx++) {
        pbConvertToNavicEphData(pbEphResp.navicephemerisdata(idx),
                halResp.navicEphemerisData[idx]);
    }
    return 0;
}

int LocationApiPbMsgConv::pbConvertToSystemTime(
    const PBLocApiGnssSystemTimeStructType &pbSystemTime,
    GnssSystemTimeStructType &halSystemTime) const {

    halSystemTime.validityMask = getGnssSystemTimeStructTypeFlagsFromPB(
                                        pbSystemTime.validitymask());
    halSystemTime.systemWeek             = pbSystemTime.systemweek();
    halSystemTime.systemMsec             = pbSystemTime.systemmsec();
    halSystemTime.systemClkTimeBias      = pbSystemTime.systemclktimebiasms();
    halSystemTime.systemClkTimeUncMs     = pbSystemTime.systemclktimebiasuncms();
    halSystemTime.refFCount              = pbSystemTime.reffcount();
    halSystemTime.numClockResets         = pbSystemTime.numclockresets();
    return 0;
}

int LocationApiPbMsgConv::pbConvertToGnssEphNotif(
        const PBGnssEphemerisNotification &pbGnssEphNotif,
        GnssSvEphemerisReport &gnssEphNotif) const {

    gnssEphNotif.gnssConstellation =  getEnumForPBGnssLocSvSystem(
            pbGnssEphNotif.gnssconstellation());
    gnssEphNotif.isSystemTimeValid = pbGnssEphNotif.issystemtimevalid();

    pbConvertToSystemTime(pbGnssEphNotif.systemtime(),
            gnssEphNotif.systemTime);

    switch (gnssEphNotif.gnssConstellation) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            pbConvertToGpsEphResponse(pbGnssEphNotif.ephunion().gpsephemeris(),
                    gnssEphNotif.ephInfo.gpsEphemeris);
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            pbConvertToGalEphResponse(pbGnssEphNotif.ephunion().galileoephemeris(),
                    gnssEphNotif.ephInfo.galileoEphemeris);
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            pbConvertToGloEphResponse(pbGnssEphNotif.ephunion().glonassephemeris(),
                    gnssEphNotif.ephInfo.glonassEphemeris);
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            pbConvertToBdsEphResponse(pbGnssEphNotif.ephunion().bdsephemeris(),
                    gnssEphNotif.ephInfo.bdsEphemeris);
            break;
        case GNSS_LOC_SV_SYSTEM_QZSS:
            pbConvertToQzssEphResponse(pbGnssEphNotif.ephunion().qzssephemeris(),
                    gnssEphNotif.ephInfo.qzssEphemeris);
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            pbConvertToNavicEphResponse(pbGnssEphNotif.ephunion().navicephemeris(),
                    gnssEphNotif.ephInfo.navicEphemeris);
            break;
    }
    return 0;
}

int LocationApiPbMsgConv::convertGnssMmfDataToPB(const GnssMapMatchedData mmfData,
    PBGnssMapMatchedData* pbMmfData) const {

    if (pbMmfData) {
        //uint64 validityMask = 1;
        pbMmfData->set_validitymask(mmfData.validityMask);

        //uint64 utcTimestampMs = 2;
        pbMmfData->set_utctimestampms(mmfData.utcTimestampMs);

        // float  mapMatchedLatitudeDifference = 3;
        pbMmfData->set_mapmatchedlatitudedifference(mmfData.mapMatchedLatitudeDifference);

        // float  mapMatchedLongitudeDifference = 4;
        pbMmfData->set_mapmatchedlongitudedifference(mmfData.mapMatchedLongitudeDifference);

        // bool   isTunnel = 5;
        pbMmfData->set_istunnel(mmfData.isTunnel);

        // float  bearing = 6;
        pbMmfData->set_bearing(mmfData.bearing);

        // double altitude = 7;
        pbMmfData->set_altitude(mmfData.altitude);

        // float  horizontalAccuracy = 8;
        pbMmfData->set_horizontalaccuracy(mmfData.horizontalAccuracy);

        // float  altitudeAccuracy = 9;
        pbMmfData->set_altitudeaccuracy(mmfData.altitudeAccuracy);

        // float  bearingAccuracy = 10;
        pbMmfData->set_bearingaccuracy(mmfData.bearingAccuracy);
        return 0;
    } else {
        return 1;
    }
}

int LocationApiPbMsgConv::pbConvertToGnssMmfData(const PBGnssMapMatchedData& pbMmfData,
    GnssMapMatchedData& mmfData) const {

    //uint64 validityMask = 1;
    mmfData.validityMask = pbMmfData.validitymask();

    //uint64 utcTimestampMs = 2;
    mmfData.utcTimestampMs = pbMmfData.utctimestampms();

    // float  mapMatchedLatitudeDifference = 3;
    mmfData.mapMatchedLatitudeDifference = pbMmfData.mapmatchedlatitudedifference();


    // float  mapMatchedLongitudeDifference = 4;
    mmfData.mapMatchedLongitudeDifference = pbMmfData.mapmatchedlongitudedifference();

    // bool   isTunnel = 5;
    mmfData.isTunnel = pbMmfData.istunnel();

    // float  bearing = 6;
    mmfData.bearing = pbMmfData.bearing();

    // double altitude = 7;
    mmfData.altitude = pbMmfData.altitude();

    // float  horizontalAccuracy = 8;
    mmfData.horizontalAccuracy = pbMmfData.horizontalaccuracy();

    // float  altitudeAccuracy = 9;
    mmfData.altitudeAccuracy = pbMmfData.altitudeaccuracy();

    // float  bearingAccuracy = 10;
    mmfData.bearingAccuracy = pbMmfData.bearingaccuracy();
    return 0;
}
