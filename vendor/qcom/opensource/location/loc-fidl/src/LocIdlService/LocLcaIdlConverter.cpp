/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.

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

#define LOG_TAG "LOC_IDL_LCA_CONV"
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "LocLcaIdlConverter.h"
#include "log_util.h"


#define IDL_MAX_GNSS_MEAS    176
static uint32_t posCount = 0;
static uint32_t measCount = 0;
/*************************************************************************

                            POSITION REPORT

/************************************************************************/

uint32_t parseIDLPosTechMask
(
    ::LocationTechnologyMask posTechMask
)
{
    uint32_t idlFlags = 0;
    if (LOCATION_TECHNOLOGY_GNSS_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_GNSS_BIT;
    }
    if (LOCATION_TECHNOLOGY_CELL_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_CELL_BIT;
    }
    if (LOCATION_TECHNOLOGY_WIFI_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_WIFI_BIT;
    }
    if (LOCATION_TECHNOLOGY_SENSORS_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_SENSORS_BIT;
    }
    if (LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_REF_LOC_BIT;
    }
    if (LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_INJECTED_COARSE_POS_BIT;
    }
    if (LOCATION_TECHNOLOGY_AFLT_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_AFLT_BIT;
    }
    if (LOCATION_TECHNOLOGY_HYBRID_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_HYBRID_BIT;
    }
    if (LOCATION_TECHNOLOGY_PPE_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_PPE_BIT;
    }
    if (LOCATION_TECHNOLOGY_VEH_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_VEH_BIT;
    }
    if (LOCATION_TECHNOLOGY_VIS_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_VIS_BIT;
    }
    if (LOCATION_TECHNOLOGY_PROPAGATED_BIT & posTechMask) {
        idlFlags |= LocIdlAPI::IDLLocationTechnologyMask::IDL_LOC_TECH_PROPAGATED_BIT;
    }

    return idlFlags;
}

LocIdlAPI::IDLLocation parseBasicLocationInfo
(
    const ::GnssLocation &basicLoc
)
{
    /* Fill Basic Location info */
    LocIdlAPI::IDLLocation idlLoc = {};
    memset(&idlLoc, 0, sizeof(idlLoc));
    uint32_t idlLocflags = 0;
    if (basicLoc.flags & LOCATION_HAS_TIMESTAMP_BIT) {
        idlLoc.setTimestamp(basicLoc.timestamp);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_TIMESTAMP_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_LAT_LONG_BIT) {
        idlLoc.setLatitude(basicLoc.latitude);
        idlLoc.setLongitude(basicLoc.longitude);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_LAT_LONG_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_ALTITUDE_BIT) {
        idlLoc.setAltitude(basicLoc.altitude);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_ALTITUDE_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_SPEED_BIT) {
        idlLoc.setSpeed(basicLoc.speed);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_SPEED_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_BEARING_BIT) {
        idlLoc.setBearing(basicLoc.bearing);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_BEARING_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_ACCURACY_BIT) {
        idlLoc.setHorizontalAccuracy(basicLoc.horizontalAccuracy);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_ACCURACY_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_VERTICAL_ACCURACY_BIT) {
        idlLoc.setVerticalAccuracy(basicLoc.verticalAccuracy);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_VERTICAL_ACCURACY_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_SPEED_ACCURACY_BIT) {
        idlLoc.setSpeedAccuracy(basicLoc.speedAccuracy);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_SPEED_ACCURACY_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_BEARING_ACCURACY_BIT) {
        idlLoc.setBearingAccuracy(basicLoc.bearingAccuracy);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_BEARING_ACCURACY_BIT;
    }

    idlLoc.setTechMask(::parseIDLPosTechMask(basicLoc.techMask));

    if (basicLoc.flags & LOCATION_HAS_ELAPSED_REAL_TIME_BIT) {
        idlLoc.setElapsedRealTimeNs(basicLoc.elapsedRealTimeNs);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_ELAPSED_REAL_TIME_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_ELAPSED_REAL_TIME_UNC_BIT) {
        idlLoc.setElapsedRealTimeUncNs(basicLoc.elapsedRealTimeUncNs);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_ELAPSED_REAL_TIME_UNC_BIT;
    }
    if (basicLoc.flags & LOCATION_HAS_TIME_UNC_BIT) {
        idlLoc.setTimeUncMs(basicLoc.timeUncMs);
        idlLocflags |= LocIdlAPI::IDLLocationFlagsMask::IDL_HAS_TIME_UNC_BIT;
    }
    idlLoc.setFlags(idlLocflags);
    return idlLoc;
}

uint32_t parseIDLLocReliability
(
    ::LocationReliability locReliability
)
{
    LocIdlAPI::IDLLocationReliability reliability =
            LocIdlAPI::IDLLocationReliability::IDL_LOC_RELIABILITY_NOT_SET;
    switch (locReliability) {
        case LocIdlAPI::IDLLocationReliability::IDL_LOC_RELIABILITY_VERY_LOW:
            reliability = LocIdlAPI::IDLLocationReliability::IDL_LOC_RELIABILITY_VERY_LOW;
            break;
        case LocIdlAPI::IDLLocationReliability::IDL_LOC_RELIABILITY_MEDIUM:
            reliability = LocIdlAPI::IDLLocationReliability::IDL_LOC_RELIABILITY_MEDIUM;
            break;
        case LocIdlAPI::IDLLocationReliability::IDL_LOC_RELIABILITY_HIGH:
            reliability = LocIdlAPI::IDLLocationReliability::IDL_LOC_RELIABILITY_HIGH;
            break;
        default:
            reliability = LocIdlAPI::IDLLocationReliability::IDL_LOC_RELIABILITY_NOT_SET;
            break;
    }
    return reliability;
}


LocIdlAPI::IDLLocationReportSvUsedInPosition parseIDLSvUsedInPosition
(
    ::GnssLocationSvUsedInPosition halSv
)
{

    LocIdlAPI::IDLLocationReportSvUsedInPosition idlSvUsed= {};
    idlSvUsed.setGpsSvUsedIdsMask(halSv.gpsSvUsedIdsMask);
    idlSvUsed.setGloSvUsedIdsMask(halSv.gloSvUsedIdsMask);
    idlSvUsed.setGalSvUsedIdsMask(halSv.galSvUsedIdsMask);
    idlSvUsed.setBdsSvUsedIdsMask(halSv.bdsSvUsedIdsMask);
    idlSvUsed.setQzssSvUsedIdsMask(halSv.qzssSvUsedIdsMask);
    idlSvUsed.setNavicSvUsedIdsMask(halSv.navicSvUsedIdsMask);

    return idlSvUsed;
}


uint32_t parseIDLNavSolutionMask
(
    ::GnssLocationNavSolutionMask navSolMask
)
{
    uint32_t idlNavSolMask = 0;
    if (LOCATION_SBAS_CORRECTION_IONO_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::IDL_SBAS_CORR_IONO;
    }
    if (LOCATION_SBAS_CORRECTION_FAST_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::IDL_SBAS_CORR_FAST;
    }
    if (LOCATION_SBAS_CORRECTION_LONG_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::IDL_SBAS_CORR_LONG;
    }
    if (LOCATION_SBAS_INTEGRITY_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::IDL_SBAS_INTEGRITY;
    }
    if (LOCATION_NAV_CORRECTION_DGNSS_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::IDL_NAV_CORR_DGNSS;
    }

    if (LOCATION_NAV_CORRECTION_RTK_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::IDL_NAV_CORR_RTK;
    }
    if (LOCATION_NAV_CORRECTION_PPP_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::IDL_NAV_CORR_PPP;
    }
    if (LOCATION_NAV_CORRECTION_RTK_FIXED_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::\
                         IDL_NAV_CORR_RTK_FIX;
    }
    if (LOCATION_NAV_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_BIT & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::\
                         IDL_NAV_CORR_ONLY_SBAS_CORR_SV_USED;
    }
    if (LOCATION_NAV_MMF_AIDED_POSITION & navSolMask) {
        idlNavSolMask |= LocIdlAPI::IDLLocationReportNavSolutionMask::\
                         LRNSM_NAV_CORR_MMF_AIDED;
    }
    return idlNavSolMask;
}


LocIdlAPI::IDLLocationReportPositionDynamics parseIDLBodyFrameData
(
    ::GnssLocationPositionDynamics bodyFrameData
)
{
    LocIdlAPI::IDLLocationReportPositionDynamics idlPosDynamics = {};
    memset(&idlPosDynamics, 0, sizeof(idlPosDynamics));

    uint32_t idlFlags = 0;
    if (LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_LONG_ACCEL;
        idlPosDynamics.setLongAccel(bodyFrameData.longAccel);
    }
    if (LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_LAT_ACCEL;
        idlPosDynamics.setLatAccel(bodyFrameData.latAccel);
    }
    if (LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_VERT_ACCEL;
        idlPosDynamics.setVertAccel(bodyFrameData.vertAccel);
    }
    if (LOCATION_NAV_DATA_HAS_YAW_RATE_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_YAW_RATE;
        idlPosDynamics.setYawRate(bodyFrameData.yawRate);
    }
    if (LOCATION_NAV_DATA_HAS_PITCH_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_PITCH;
        idlPosDynamics.setPitch(bodyFrameData.pitch);
    }
    if (LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_LONG_ACCEL_UNC;
        idlPosDynamics.setLongAccelUnc(bodyFrameData.longAccelUnc);
    }
    if (LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_LAT_ACCEL_UNC;
        idlPosDynamics.setLatAccelUnc(bodyFrameData.latAccelUnc);
    }
    if (LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_VERT_ACCEL_UNC;
        idlPosDynamics.setVertAccelUnc(bodyFrameData.vertAccelUnc);
    }
    if (LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_YAW_RATE_UNC;
        idlPosDynamics.setYawRateUnc(bodyFrameData.yawRateUnc);
    }
    if (LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_PITCH_UNC;
        idlPosDynamics.setPitchUnc(bodyFrameData.pitchUnc);
    }
    if (LOCATION_NAV_DATA_HAS_PITCH_RATE_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_PITCH_RATE;
        idlPosDynamics.setPitchRate(bodyFrameData.pitchRate);
    }
    if (LOCATION_NAV_DATA_HAS_PITCH_RATE_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_PITCH_RATE_UNC;
        idlPosDynamics.setPitchRateUnc(bodyFrameData.pitchRateUnc);
    }
    if (LOCATION_NAV_DATA_HAS_ROLL_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_ROLL;
        idlPosDynamics.setRoll(bodyFrameData.roll);
    }
    if (LOCATION_NAV_DATA_HAS_ROLL_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_ROLL_UNC;
        idlPosDynamics.setRollUnc(bodyFrameData.rollUnc);
    }
    if (LOCATION_NAV_DATA_HAS_ROLL_RATE_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_ROLL_RATE_UNC;
        idlPosDynamics.setRollRateUnc(bodyFrameData.rollRateUnc);
    }
    if (LOCATION_NAV_DATA_HAS_ROLL_RATE_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_ROLL_RATE;
        idlPosDynamics.setRollRate(bodyFrameData.rollRate);
    }
    if (LOCATION_NAV_DATA_HAS_YAW_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_YAW;
        idlPosDynamics.setYaw(bodyFrameData.yaw);
    }
    if (LOCATION_NAV_DATA_HAS_YAW_UNC_BIT & bodyFrameData.bodyFrameDataMask) {
        idlFlags |= LocIdlAPI::IDLLocationReportPosDataMask::IDL_NAV_DATA_YAW_UNC;
        idlPosDynamics.setYawUnc(bodyFrameData.yawUnc);
    }
    idlPosDynamics.setBodyFrameDataMask(idlFlags);
    return idlPosDynamics;
}

LocIdlAPI::IDLGnssSystemTimeStructType parseIDLGnssTime
(
    ::GnssSystemTimeStructType gnssTime
)
{
    LocIdlAPI::IDLGnssSystemTimeStructType idlGnssTime = {};
    memset(&idlGnssTime, 0, sizeof(idlGnssTime));
    uint32_t idlMask= 0;
    if (GNSS_SYSTEM_TIME_WEEK_VALID & gnssTime.validityMask) {
        idlMask |= LocIdlAPI::IDLGnssSystemTimeStructTypeFlags::IDL_SYS_TIME_WEEK_VALID;
        idlGnssTime.setSystemWeek(gnssTime.systemWeek);
    }
    if (GNSS_SYSTEM_TIME_WEEK_MS_VALID & gnssTime.validityMask) {
        idlMask |= LocIdlAPI::IDLGnssSystemTimeStructTypeFlags::IDL_SYS_TIME_WEEK_MS_VALID;
        idlGnssTime.setSystemMsec(gnssTime.systemMsec);
    }
    if (GNSS_SYSTEM_CLK_TIME_BIAS_VALID & gnssTime.validityMask) {
        idlMask |= LocIdlAPI::IDLGnssSystemTimeStructTypeFlags::\
                   IDL_SYS_CLK_TIME_BIAS_VALID;
        idlGnssTime.setSystemClkTimeBias(gnssTime.systemClkTimeBias);
    }
    if (GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID & gnssTime.validityMask) {
        idlMask |= LocIdlAPI::IDLGnssSystemTimeStructTypeFlags::\
                   IDL_SYS_CLK_TIME_BIAS_UNC_VALID;
        idlGnssTime.setSystemClkTimeUncMs(gnssTime.systemClkTimeUncMs);
    }
    if (GNSS_SYSTEM_REF_FCOUNT_VALID & gnssTime.validityMask) {
        idlMask |= LocIdlAPI::IDLGnssSystemTimeStructTypeFlags::IDL_SYS_REF_FCOUNT_VALID;
        idlGnssTime.setRefFCount(gnssTime.refFCount);
    }
    if (GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID & gnssTime.validityMask) {
        idlMask |= LocIdlAPI::IDLGnssSystemTimeStructTypeFlags::\
                   IDL_SYS_NUM_CLOCK_RESETS_VALID;
        idlGnssTime.setNumClockResets(gnssTime.numClockResets);
    }
    idlGnssTime.setValidityMask(idlMask);
    return idlGnssTime;
}


LocIdlAPI::IDLGnssGloTimeStructType parseIDLGloTime
(
    ::GnssGloTimeStructType gloTime
)
{
    LocIdlAPI::IDLGnssGloTimeStructType idlGloTime = {};
    memset(&idlGloTime, 0, sizeof(idlGloTime));
    uint32_t idlflags = 0;
    if (GNSS_CLO_DAYS_VALID & gloTime.validityMask) {
        idlflags |= LocIdlAPI::IDLGnssGloTimeStructTypeFlags::IDL_GLO_DAYS_VALID;
        idlGloTime.setGloDays(gloTime.gloDays);
    }
    if (GNSS_GLO_MSEC_VALID & gloTime.validityMask) {
        idlflags |= LocIdlAPI::IDLGnssGloTimeStructTypeFlags::IDL_GLO_MSEC_VALID;
        idlGloTime.setGloMsec(gloTime.gloMsec);
    }
    if (GNSS_GLO_CLK_TIME_BIAS_VALID & gloTime.validityMask) {
        idlflags |= LocIdlAPI::IDLGnssGloTimeStructTypeFlags::IDL_GLO_CLK_TIME_BIAS_VALID;
        idlGloTime.setGloClkTimeBias(gloTime.gloClkTimeBias);
    }
    if (GNSS_GLO_CLK_TIME_BIAS_UNC_VALID & gloTime.validityMask) {
        idlflags |= LocIdlAPI::IDLGnssGloTimeStructTypeFlags::\
                    IDL_GLO_CLK_TIME_BIAS_UNC_VALID;
        idlGloTime.setGloClkTimeUncMs(gloTime.gloClkTimeUncMs);
    }
    if (GNSS_GLO_REF_FCOUNT_VALID & gloTime.validityMask) {
        idlflags |= LocIdlAPI::IDLGnssGloTimeStructTypeFlags::IDL_GLO_REF_FCOUNT_VALID;
        idlGloTime.setRefFCount(gloTime.refFCount);
    }
    if (GNSS_GLO_NUM_CLOCK_RESETS_VALID & gloTime.validityMask) {
        idlflags |= LocIdlAPI::IDLGnssGloTimeStructTypeFlags::IDL_GLO_NUM_CLK_RESETS_VALID;
        idlGloTime.setNumClockResets(gloTime.numClockResets);
    }
    if (GNSS_GLO_FOUR_YEAR_VALID & gloTime.validityMask) {
        idlflags |= LocIdlAPI::IDLGnssGloTimeStructTypeFlags::IDL_GLO_FOUR_YEAR_VALID;
        idlGloTime.setGloFourYear(gloTime.gloFourYear);
    }
    idlGloTime.setValidityMask(idlflags);
    return idlGloTime;
}


LocIdlAPI::IDLGnssSystemTime parseGnssSystemTime(::GnssSystemTime gnssSystemTime) {
    LocIdlAPI::IDLGnssSystemTime idlSystemTime= {};
    memset(&idlSystemTime, 0, sizeof(idlSystemTime));
    switch (gnssSystemTime.gnssSystemTimeSrc) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            idlSystemTime.setGnssSystemTimeSrc(
                LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_GPS);
            idlSystemTime.setTimeUnion(parseIDLGnssTime(gnssSystemTime.u.gpsSystemTime));
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            idlSystemTime.setGnssSystemTimeSrc(
                LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_GAL);
            idlSystemTime.setTimeUnion(parseIDLGnssTime(gnssSystemTime.u.galSystemTime));
            break;
        case GNSS_LOC_SV_SYSTEM_SBAS:
            idlSystemTime.setGnssSystemTimeSrc(
                LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_SBAS);
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            idlSystemTime.setGnssSystemTimeSrc(
                LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_GLO);
            idlSystemTime.setTimeUnion(parseIDLGloTime(gnssSystemTime.u.gloSystemTime));
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            idlSystemTime.setGnssSystemTimeSrc(
                LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_BDS);
            idlSystemTime.setTimeUnion(parseIDLGnssTime(gnssSystemTime.u.bdsSystemTime));
            break;
         case GNSS_LOC_SV_SYSTEM_QZSS:
            idlSystemTime.setGnssSystemTimeSrc(
                LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_QZSS);
            idlSystemTime.setTimeUnion(parseIDLGnssTime(gnssSystemTime.u.qzssSystemTime));
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            idlSystemTime.setGnssSystemTimeSrc(
                LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_NAVIC);
            idlSystemTime.setTimeUnion(parseIDLGnssTime(gnssSystemTime.u.navicSystemTime));
            break;
    }
    return idlSystemTime;
}


uint32_t  parseIDLGnssConstellation
(
    ::Gnss_LocSvSystemEnumType constellation
)
{
    uint32_t idlConstellation = 0;
     switch (constellation) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            idlConstellation = LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_GPS;
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            idlConstellation = LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_GAL;
            break;
        case GNSS_LOC_SV_SYSTEM_SBAS:
            idlConstellation = LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_SBAS;
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            idlConstellation = LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_GLO;
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            idlConstellation = LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_BDS;
            break;
        case GNSS_LOC_SV_SYSTEM_QZSS:
            idlConstellation = LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_QZSS;
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            idlConstellation = LocIdlAPI::IDLGnssSvSystemEnumType::IDL_LOC_SV_SYSTEM_NAVIC;
            break;
    }
    return idlConstellation;
}

uint32_t parseIDLSignalType (
    ::GnssSignalTypeMask lcaSignalType
)
{
    uint32_t gnssSignalTypeMask = 0;

    if (lcaSignalType & GNSS_SIGNAL_GPS_L1CA_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GPS_L1CA_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_GPS_L1C_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GPS_L1C_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_GPS_L2_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GPS_L2_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_GPS_L5_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GPS_L5_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_GLONASS_G1_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GLONASS_G1_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_GLONASS_G2_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GLONASS_G2_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_GALILEO_E1_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GALILEO_E1_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_GALILEO_E5A_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GALILEO_E5A_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_GALILEO_E5B_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_GALILEO_E5B_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B1_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B1_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B2_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B2_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_QZSS_L1CA_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_QZSS_L1CA_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_QZSS_L1S_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_QZSS_L1S_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_QZSS_L2_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_QZSS_L2_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_QZSS_L5_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_QZSS_L5_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_SBAS_L1_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_SBAS_L1_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B1I_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B1I_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B1C_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B1C_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B2I_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B2I_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B2AI_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B2AI_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_NAVIC_L5_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_NAVIC_L5_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B2AQ_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B2AQ_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B2BI_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B2BI_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_BEIDOU_B2BQ_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::IDL_GNSS_SIGNAL_BEIDOU_B2BQ_BIT;
    }
    if (lcaSignalType & GNSS_SIGNAL_NAVIC_L1_BIT) {
        gnssSignalTypeMask |= LocIdlAPI::IDLGnssSignalTypeMask::GSTM_NAVIC_L1_BIT;
    }
    return gnssSignalTypeMask;
}

vector< LocIdlAPI::IDLGnssMeasUsageInfo > parseIDLMeasUsageInfo
(
    vector<::GnssMeasUsageInfo> measUsageInfo
)
{
    vector< LocIdlAPI::IDLGnssMeasUsageInfo > idlMeasUsed;
    for (int idx = 0; idx < measUsageInfo.size() && idx < IDL_MAX_GNSS_MEAS; idx++) {
        LocIdlAPI::IDLGnssMeasUsageInfo idlMeasInfo = {};
        idlMeasInfo.setGnssConstellation(
                (uint16_t)parseIDLGnssConstellation(measUsageInfo[idx].gnssConstellation));
        idlMeasInfo.setGnssSignalType(::parseIDLSignalType(measUsageInfo[idx].gnssSignalType));
        idlMeasInfo.setGnssSvId(measUsageInfo[idx].gnssSvId);

        idlMeasUsed.push_back(idlMeasInfo);
    }
    return idlMeasUsed;
}

uint64_t parseIDLCalibrationStatus(::DrCalibrationStatusMask statusMask) {
    uint64_t idlFlags = 0;
    if (DR_ROLL_CALIBRATION_NEEDED & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_ROLL_CALIB_NEEDED;
    }
    if (DR_PITCH_CALIBRATION_NEEDED & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_PITCH_CALIB_NEEDED;
    }
    if (DR_YAW_CALIBRATION_NEEDED & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_YAW_CALIB_NEEDED;
    }
    if (DR_ODO_CALIBRATION_NEEDED & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_ODO_CALIB_NEEDED;
    }
    if (DR_GYRO_CALIBRATION_NEEDED & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_GYRO_CALIB_NEEDED;
    }
    if (DR_TURN_CALIBRATION_LOW & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_TURN_CALIB_LOW;
    }
    if (DR_TURN_CALIBRATION_MEDIUM & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_TURN_CALIB_MED;
    }
    if (DR_TURN_CALIBRATION_HIGH & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_TURN_CALIB_HIGH;
    }
    if (DR_LINEAR_ACCEL_CALIBRATION_LOW & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_LINEAR_ACCEL_CALIB_LOW;
    }
    if (DR_LINEAR_ACCEL_CALIBRATION_MEDIUM & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_LINEAR_ACCEL_CALIB_MED;
    }
    if (DR_LINEAR_ACCEL_CALIBRATION_HIGH & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_LINEAR_ACCEL_CALIB_HIGH;
    }
    if (DR_LINEAR_MOTION_CALIBRATION_LOW & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_LINEAR_MOTION_CALIB_LOW;
    }
    if (DR_LINEAR_MOTION_CALIBRATION_MEDIUM & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_LINEAR_MOTION_CALIB_MED;
    }
    if (DR_LINEAR_MOTION_CALIBRATION_HIGH & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_LINEAR_MOTION_CALIB_HIGH;
    }
    if (DR_STATIC_CALIBRATION_LOW & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_STATIC_CALIB_LOW;
    }
    if (DR_STATIC_CALIBRATION_MEDIUM & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_STATIC_CALIB_MED;
    }
    if (DR_STATIC_CALIBRATION_HIGH & statusMask) {
        idlFlags |= LocIdlAPI::IDLDrCalibrationStatusMask::IDL_DR_STATIC_CALIB_HIGH;
    }
    return idlFlags;
}

uint32_t parseIDLEngMask
(
    ::PositioningEngineMask locOutputEngMask
)
{
    uint32_t idlEngMask = 0;
    if (STANDARD_POSITIONING_ENGINE & locOutputEngMask) {
        idlEngMask |= LocIdlAPI::IDLPositioningEngineMask::IDL_STANDARD_POSITIONING_ENGINE;
    }
    if (DEAD_RECKONING_ENGINE & locOutputEngMask) {
        idlEngMask |= LocIdlAPI::IDLPositioningEngineMask::IDL_DEAD_RECKONING_ENGINE;
    }
    if (PRECISE_POSITIONING_ENGINE & locOutputEngMask) {
        idlEngMask |= LocIdlAPI::IDLPositioningEngineMask::IDL_PRECISE_POSITIONING_ENGINE;
    }
    if (VP_POSITIONING_ENGINE & locOutputEngMask) {
        idlEngMask |= LocIdlAPI::IDLPositioningEngineMask::IDL_VP_POSITIONING_ENGINE;
    }
    return idlEngMask;
}


LocIdlAPI::IDLLLAInfo parseIDLLatLongAltInfo
(
    ::LLAInfo llaVRPBased
)
{
    LocIdlAPI::IDLLLAInfo llaInfo = {};

    llaInfo.setLatitude(llaVRPBased.latitude);
    llaInfo.setLongitude(llaVRPBased.longitude);
    llaInfo.setAltitude(llaVRPBased.altitude);
    return llaInfo;
}


uint64_t parseIDLDrSolStatusMask
(
    ::DrSolutionStatusMask drSolutionStatusMask
)
{
    uint64_t idlDrSolMask = 0;
    if (DR_SOLUTION_STATUS_VEHICLE_SENSOR_SPEED_INPUT_DETECTED & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_VEHICLE_SENSOR_SPEED_INPUT_DETECTED;
    }
    if (DR_SOLUTION_STATUS_VEHICLE_SENSOR_SPEED_INPUT_USED & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_VEHICLE_SENSOR_SPEED_INPUT_USED;
    }
    if (DR_SOLUTION_STATUS_WARNING_UNCALIBRATED & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_WARNING_UNCALIBRATED;
    }
    if (DR_SOLUTION_STATUS_WARNING_GNSS_QUALITY_INSUFFICIENT & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_WARNING_GNSS_QUALITY_INSUFFICIENT;
    }
    if (DR_SOLUTION_STATUS_WARNING_FERRY_DETECTED & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_WARNING_FERRY_DETECTED;
    }
    if (DR_SOLUTION_STATUS_ERROR_6DOF_SENSOR_UNAVAILABLE & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::
                IDL_DR_SOLUTION_STATUS_ERROR_6DOF_SENSOR_UNAVAILABLE;
    }
    if (DR_SOLUTION_STATUS_ERROR_VEHICLE_SPEED_UNAVAILABLE & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_ERROR_VEHICLE_SPEED_UNAVAILABLE;
    }
    if (DR_SOLUTION_STATUS_ERROR_GNSS_EPH_UNAVAILABLE & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_ERROR_GNSS_EPH_UNAVAILABLE;
    }
    if (DR_SOLUTION_STATUS_ERROR_GNSS_MEAS_UNAVAILABLE & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_ERROR_GNSS_MEAS_UNAVAILABLE;
    }
    if (DR_SOLUTION_STATUS_WARNING_INIT_POSITION_INVALID & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_WARNING_INIT_POSITION_INVALID;
    }
    if (DR_SOLUTION_STATUS_WARNING_POSITON_UNRELIABLE & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_WARNING_POSITON_UNRELIABLE;
    }
    if (DR_SOLUTION_STATUS_ERROR_GENERIC & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_ERROR_GENERIC;
    }
    if (DR_SOLUTION_STATUS_WARNING_SENSOR_TEMP_OUT_OF_RANGE & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_WARNING_SENSOR_TEMP_OUT_OF_RANGE;
    }
    if (DR_SOLUTION_STATUS_WARNING_USER_DYNAMICS_INSUFFICIENT & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_WARNING_USER_DYNAMICS_INSUFFICIENT;
    }
    if (DR_SOLUTION_STATUS_WARNING_FACTORY_DATA_INCONSISTENT & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                IDL_DR_SOLUTION_STATUS_WARNING_FACTORY_DATA_INCONSISTENT;
    }
    if (DR_SOLUTION_STATUS_WARNING_MMF_UNAVAILABLE & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                DSSM_WARNING_MMF_UNAVAILABLE;
    }
    if (DR_SOLUTION_STATUS_WARNING_MMF_NOT_USABLE  & drSolutionStatusMask) {
        idlDrSolMask |= LocIdlAPI::IDLDrSolutionStatusMask::\
                DSSM_WARNING_MMF_NOT_USABLE;
    }
    return idlDrSolMask;
}

LocIdlAPI::IDLLocationReport LocLcaIdlConverter::parseLocReport(const ::GnssLocation &lcaLoc)
{
    LocIdlAPI::IDLLocationReport idlLocReport = {};
    /* Fill Basic Location info */
    idlLocReport.setLocInfo(::parseBasicLocationInfo(lcaLoc));

    /* Fill Extended Location info */
    uint64_t locFlags = 0;
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT) {
        idlLocReport.setAltitudeMeanSeaLevel(lcaLoc.altitudeMeanSeaLevel);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::\
                    IDL_LOC_INFO_ALTITUDE_MEAN_SEA_LEVEL;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_DOP_BIT) {
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_DOP;
        idlLocReport.setPdop(lcaLoc.pdop);
        idlLocReport.setHdop(lcaLoc.hdop);
        idlLocReport.setVdop(lcaLoc.vdop);
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_EXT_DOP_BIT) {
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_EXT_DOP;
        idlLocReport.setGdop(lcaLoc.gdop);
        idlLocReport.setTdop(lcaLoc.tdop);
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT) {
        idlLocReport.setMagneticDeviation(lcaLoc.magneticDeviation);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_MAGNETIC_DEVIATION;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT) {
        idlLocReport.setHorReliability(::parseIDLLocReliability(lcaLoc.horReliability));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_HOR_RELIABILITY;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_VER_RELIABILITY_BIT) {
        idlLocReport.setVerReliability(::parseIDLLocReliability(lcaLoc.verReliability));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_VER_RELIABILITY;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT) {
        idlLocReport.setHorUncEllipseSemiMajor(lcaLoc.horUncEllipseSemiMajor);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::\
                    IDL_LOC_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT) {
        idlLocReport.setHorUncEllipseSemiMinor(lcaLoc.horUncEllipseSemiMinor);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::\
                    IDL_LOC_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT) {
        idlLocReport.setHorUncEllipseOrientAzimuth(lcaLoc.horUncEllipseOrientAzimuth);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::\
                    IDL_LOC_INFO_HOR_ACCURACY_ELIP_AZIMUTH;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT) {
        idlLocReport.setNorthStdDeviation(lcaLoc.northStdDeviation);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_NORTH_STD_DEV;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_EAST_STD_DEV_BIT) {
        idlLocReport.setEastStdDeviation(lcaLoc.eastStdDeviation);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_EAST_STD_DEV;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_NORTH_VEL_BIT) {
        idlLocReport.setNorthVelocity(lcaLoc.northVelocity);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_NORTH_VEL;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_EAST_VEL_BIT) {
        idlLocReport.setEastVelocity(lcaLoc.eastVelocity);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_EAST_VEL;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_UP_VEL_BIT) {
        idlLocReport.setUpVelocity(lcaLoc.upVelocity);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_UP_VEL;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT) {
        idlLocReport.setNorthVelocityStdDeviation(lcaLoc.northVelocityStdDeviation);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_NORTH_VEL_UNC;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT) {
        idlLocReport.setEastVelocityStdDeviation(lcaLoc.eastVelocityStdDeviation);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_EAST_VEL_UNC;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_UP_VEL_UNC_BIT) {
        idlLocReport.setUpVelocityStdDeviation(lcaLoc.upVelocityStdDeviation);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_UP_VEL_UNC;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT) {
        idlLocReport.setNumSvUsedInPosition(lcaLoc.numSvUsedInPosition);
        idlLocReport.setSvUsedInPosition(::parseIDLSvUsedInPosition(lcaLoc.svUsedInPosition));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_NUM_SV_USED_IN_POS;
    }

    if (lcaLoc.gnssInfoFlags & LCA_GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT) {
        idlLocReport.setMeasUsageInfo(::parseIDLMeasUsageInfo(lcaLoc.measUsageInfo));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_GNSS_SV_USED_DATA;
    }
    if (lcaLoc.gnssInfoFlags & LCA_GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT) {
        idlLocReport.setNavSolutionMask(::parseIDLNavSolutionMask(lcaLoc.navSolutionMask));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_NAV_SOLUTION_MASK_BIT;
    }
    if (lcaLoc.gnssInfoFlags & LCA_GNSS_LOCATION_INFO_POS_TECH_MASK_BIT) {
        idlLocReport.setPosTechMask(::parseIDLPosTechMask(lcaLoc.posTechMask));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LCA_GNSS_LOCATION_INFO_POS_TECH_MASK;
    }
    if (lcaLoc.gnssInfoFlags & LCA_GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT) {
        idlLocReport.setBodyFrameData(::parseIDLBodyFrameData(lcaLoc.bodyFrameData));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_POS_DYNAMICS_DATA;
    }

    idlLocReport.setGnssSystemTime(::parseGnssSystemTime(lcaLoc.gnssSystemTime));

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_LEAP_SECONDS_BIT) {
        idlLocReport.setLeapSeconds(lcaLoc.leapSeconds);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_LEAP_SECONDS;
    }

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_PERCENT_BIT) {
        idlLocReport.setCalibrationConfidencePercent(lcaLoc.calibrationConfidencePercent);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_CALIB_CONFIDENCE_PERCENT;
    }

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT) {
        idlLocReport.setCalibrationStatus(::parseIDLCalibrationStatus(lcaLoc.calibrationStatus));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_CALIB_STATUS;

    }

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT) {
        LocIdlAPI::IDLLocOutputEngineType idlEngType =
                LocIdlAPI::IDLLocOutputEngineType::IDL_LOC_OUTPUT_ENGINE_COUNT;
        switch (lcaLoc.locOutputEngType)  {
            case LOC_OUTPUT_ENGINE_FUSED:
                idlEngType =
                    LocIdlAPI::IDLLocOutputEngineType::IDL_LOC_OUTPUT_ENGINE_FUSED;
                break;
            case LOC_OUTPUT_ENGINE_SPE:
                idlEngType = LocIdlAPI::IDLLocOutputEngineType::IDL_LOC_OUTPUT_ENGINE_SPE;
                break;
            case LOC_OUTPUT_ENGINE_PPE:
                idlEngType = LocIdlAPI::IDLLocOutputEngineType::IDL_LOC_OUTPUT_ENGINE_PPE;
                break;
            case LOC_OUTPUT_ENGINE_VPE:
                idlEngType = LocIdlAPI::IDLLocOutputEngineType::IDL_LOC_OUTPUT_ENGINE_VPE;
                break;
            default:
                idlEngType =
                    LocIdlAPI::IDLLocOutputEngineType::IDL_LOC_OUTPUT_ENGINE_COUNT;
                break;
        }
        idlLocReport.setLocOutputEngType(idlEngType);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_OUTPUT_ENG_TYPE;
    }

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT) {
        idlLocReport.setLocOutputEngMask(::parseIDLEngMask(lcaLoc.locOutputEngMask));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_OUTPUT_ENG_MASK;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_CONFORMITY_INDEX_BIT) {
        idlLocReport.setConformityIndex(lcaLoc.conformityIndex);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_CONFORMITY_INDEX;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_LLA_VRP_BASED_BIT) {
        idlLocReport.setLlaVRPBased(::parseIDLLatLongAltInfo(lcaLoc.llaVRPBased));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_LLA_VRP_BASED;
    }

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_ENU_VELOCITY_VRP_BASED_BIT) {
        vector<float> idlEnuVelocityVRPBased;
        idlEnuVelocityVRPBased.push_back(lcaLoc.enuVelocityVRPBased[0]);
        idlEnuVelocityVRPBased.push_back(lcaLoc.enuVelocityVRPBased[1]);
        idlEnuVelocityVRPBased.push_back(lcaLoc.enuVelocityVRPBased[2]);
        idlLocReport.setEnuVelocityVRPBased(idlEnuVelocityVRPBased);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_ENU_VEL_VRP_BASED;
    }

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_DR_SOLUTION_STATUS_MASK_BIT) {
        idlLocReport.setDrSolutionStatusMask(::parseIDLDrSolStatusMask(
                lcaLoc.drSolutionStatusMask));
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_DR_SOL_STATUS_MASK;
    }

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_ALTITUDE_ASSUMED_BIT) {
        idlLocReport.setAltitudeAssumed(lcaLoc.altitudeAssumed);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_ALTITUDE_ASSUMED;
    }

    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_SESSION_STATUS_BIT) {
        uint32_t idlSessionStatus =
               LocIdlAPI::IDLLocSessionStatus::IDL_LOC_SESS_FAILURE;
        switch (lcaLoc.sessionStatus) {
            case LOC_SESS_SUCCESS:
                idlSessionStatus = LocIdlAPI::IDLLocSessionStatus::IDL_LOC_SESS_SUCCESS;
                break;
            case LOC_SESS_INTERMEDIATE:
                idlSessionStatus =\
                    LocIdlAPI::IDLLocSessionStatus::IDL_LOC_SESS_INTERMEDIATE;
                break;
            case LOC_SESS_FAILURE:
                idlSessionStatus = LocIdlAPI::IDLLocSessionStatus::IDL_LOC_SESS_FAILURE;
                break;
        }
        idlLocReport.setSessionStatus(idlSessionStatus);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_SESSION_STATUS;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_INTEGRITY_RISK_USED_BIT) {
        idlLocReport.setIntegrityRiskUsed(lcaLoc.integrityRiskUsed);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::\
                    IDL_LOC_INFO_INTEGRITY_RISK_USED;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_PROTECT_ALONG_TRACK_BIT) {
        idlLocReport.setProtectAlongTrack(lcaLoc.protectAlongTrack);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::\
                    IDL_LOC_INFO_PROTECT_ALONG_TRACK;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_PROTECT_CROSS_TRACK_BIT) {
        idlLocReport.setProtectCrossTrack(lcaLoc.protectCrossTrack);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::\
                    IDL_LOC_INFO_PROTECT_CROSS_TRACK;
    }
    if (lcaLoc.gnssInfoFlags & GNSS_LOCATION_INFO_PROTECT_VERTICAL_BIT ) {
        idlLocReport.setProtectVertical(lcaLoc.protectVertical);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_PROTECT_VERTICAL;
    }
    idlLocReport.setDgnssStationId(lcaLoc.dgnssStationId);
    locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_DGNSS_STATION_ID;

    if (lcaLoc.flags & LOCATION_HAS_GPTP_TIME_BIT) {
        idlLocReport.setElapsedgPTPTime(lcaLoc.elapsedgPTPTime);
        idlLocReport.setElapsedgPTPTimeUnc(lcaLoc.elapsedgPTPTimeUnc);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_GPTP_TIME_BIT;
        posCount++;
        uint64_t gptp_time_ns = 0;
        gptpGetCurPtpTime(&gptp_time_ns);
        int64_t latency = gptp_time_ns - lcaLoc.elapsedgPTPTime;
    }
    if (lcaLoc.gnssInfoFlags & LCA_GNSS_LOCATION_INFO_BASE_LINE_LENGTH_BIT) {
        idlLocReport.setBaseLineLength(lcaLoc.baseLineLength);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_BASE_LINE_LENGTH_BIT;
    }
    if (lcaLoc.gnssInfoFlags & LCA_GNSS_LOCATION_INFO_AGE_OF_CORRECTION_BIT) {
        idlLocReport.setAgeMsecOfCorrections(lcaLoc.ageMsecOfCorrections);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::IDL_LOC_INFO_AGE_OF_CORRECTION_BIT;
    }

    idlLocReport.setCurrReportingRate(0);

    if (lcaLoc.gnssInfoFlags & LCA_GNSS_LOCATION_INFO_LEAP_SECONDS_UNC_BIT) {
        idlLocReport.setLeapSecondsUnc(lcaLoc.leapSecondsUnc);
        locFlags |= LocIdlAPI::IDLLCALocationInfoFlagMask::LREFM_LEAP_SECONDS_UNC_BIT;
    }
    idlLocReport.setLocationInfoFlags(locFlags);
    LOC_LOGd("Position report %"PRIu64" ", lcaLoc.timestamp);

    return idlLocReport;
}

/*************************************************************************

                            SV-INFO REPORT

/************************************************************************/

LocIdlAPI::IDLGnssSvType  parseIDLSvType
(
    ::GnssSvType  svType
)
{
    LocIdlAPI::IDLGnssSvType idlSvType =  LocIdlAPI::IDLGnssSvType::IDL_GNSS_SV_TYPE_UNKNOWN;

    switch (svType) {
       case GNSS_SV_TYPE_GPS:
           idlSvType = LocIdlAPI::IDLGnssSvType::IDL_GNSS_SV_TYPE_GPS;
           break;
       case GNSS_SV_TYPE_SBAS:
           idlSvType = LocIdlAPI::IDLGnssSvType::IDL_GNSS_SV_TYPE_SBAS;
           break;
       case GNSS_SV_TYPE_GLONASS:
           idlSvType = LocIdlAPI::IDLGnssSvType::IDL_GNSS_SV_TYPE_GLONASS;
           break;
       case GNSS_SV_TYPE_QZSS:
           idlSvType = LocIdlAPI::IDLGnssSvType::IDL_GNSS_SV_TYPE_QZSS;
           break;
       case GNSS_SV_TYPE_GALILEO:
           idlSvType = LocIdlAPI::IDLGnssSvType::IDL_GNSS_SV_TYPE_GALILEO;
           break;
       case GNSS_SV_TYPE_NAVIC:
           idlSvType = LocIdlAPI::IDLGnssSvType::IDL_GNSS_SV_TYPE_NAVIC;
           break;
       case GNSS_SV_TYPE_BEIDOU:
           idlSvType = LocIdlAPI::IDLGnssSvType::IDL_GNSS_SV_TYPE_BEIDOU;
           break;
    }
    return idlSvType;
}

uint32_t  parseIDLSvOptionMask
(
    :: GnssSvOptionsMask optionMask
)
{
    uint32_t idlMask = 0;
    memset(&idlMask, 0, sizeof(idlMask));

    if (GNSS_SV_OPTIONS_HAS_EPHEMER_BIT & optionMask) {
        idlMask |= LocIdlAPI::IDLGnssSvOptionsMask::IDL_HAS_EPHEMER_BIT;
    }

    if (GNSS_SV_OPTIONS_HAS_ALMANAC_BIT & optionMask) {
        idlMask |= LocIdlAPI::IDLGnssSvOptionsMask::IDL_HAS_ALMANAC_BIT;
    }

    if (GNSS_SV_OPTIONS_USED_IN_FIX_BIT  & optionMask) {
        idlMask |= LocIdlAPI::IDLGnssSvOptionsMask::IDL_USED_IN_FIX_BIT ;
    }

    if (GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT & optionMask) {
        idlMask |= LocIdlAPI::IDLGnssSvOptionsMask::IDL_HAS_CARRIER_FREQ_BIT;
    }

    if (GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT & optionMask) {
        idlMask |= LocIdlAPI::IDLGnssSvOptionsMask::IDL_HAS_GNSS_SIGNAL_TYPE_BIT;
    }

    if (GNSS_SV_OPTIONS_HAS_BASEBAND_CARRIER_TO_NOISE_BIT & optionMask) {
        idlMask |= LocIdlAPI::IDLGnssSvOptionsMask::IDL_HAS_BASEBAND_CARRIER_TO_NOISE_BIT;
    }

    if (GNSS_SV_OPTIONS_HAS_ELEVATION_BIT & optionMask) {
        idlMask |= LocIdlAPI::IDLGnssSvOptionsMask::IDL_HAS_ELEVATION_BIT;
    }

    if (GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT & optionMask) {
        idlMask |= LocIdlAPI::IDLGnssSvOptionsMask::IDL_HAS_AZIMUTH_BIT;
    }
    return idlMask;
}

LocIdlAPI::IDLGnssSv LocLcaIdlConverter::parseSvReport
(
    const ::GnssSv& gnssSvs
)
{

    LocIdlAPI::IDLGnssSv idlGnssSv = {};
    memset(&idlGnssSv, 0, sizeof(idlGnssSv));

    idlGnssSv.setSvId(gnssSvs.svId);
    idlGnssSv.setType(::parseIDLSvType(gnssSvs.type));

    idlGnssSv.setCN0Dbhz(gnssSvs.cN0Dbhz);

    idlGnssSv.setElevation(gnssSvs.elevation);

    idlGnssSv.setAzimuth(gnssSvs.azimuth);

    idlGnssSv.setGnssSvOptionsMask(::parseIDLSvOptionMask(gnssSvs.gnssSvOptionsMask));

    idlGnssSv.setCarrierFrequencyHz(gnssSvs.carrierFrequencyHz);

    idlGnssSv.setGnssSignalTypeMask(::parseIDLSignalType(gnssSvs.gnssSignalTypeMask));

    idlGnssSv.setBasebandCarrierToNoiseDbHz(gnssSvs.basebandCarrierToNoiseDbHz);

    idlGnssSv.setGloFrequency(gnssSvs.gloFrequency);
    return idlGnssSv;
}

/*************************************************************************

                            MEASUREMENT REPORT

/************************************************************************/


uint32_t parseGnssClkFlags (
    ::GnssMeasurementsClockFlagsMask clkFlags
)
{
    uint32_t idlFlags = 0;
    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_LEAP_SECOND_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_TIME_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_FULL_BIAS_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_BIAS_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_BIAS_UNCERTAINTY_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_DRIFT_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_DRIFT_UNCERTAINTY_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_HW_CLOCK_DISCONTINUITY_COUNT_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_HW_CLOCK_DISCONTINUITY_COUNT_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_ELAPSED_REAL_TIME_BIT;
    }

    if (GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_BIT & clkFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsClockFlagsMask::\
                IDL_MEAS_CLK_FLAGS_ELAPSED_GPTP_TIME_BIT;
    }
    return idlFlags;
}

LocIdlAPI::IDLGnssMeasurementsClock parseIDLMeasClockInfo
(
    ::GnssMeasurementsClock gnssClock
) {

    LocIdlAPI::IDLGnssMeasurementsClock idlClkInfo= {};
    memset(&idlClkInfo, 0, sizeof(idlClkInfo));

    idlClkInfo.setFlags(::parseGnssClkFlags(gnssClock.flags));
    idlClkInfo.setLeapSecond(gnssClock.leapSecond);
    idlClkInfo.setTimeNs(gnssClock.timeNs);
    idlClkInfo.setTimeUncertaintyNs(gnssClock.timeUncertaintyNs);
    idlClkInfo.setFullBiasNs(gnssClock.fullBiasNs);
    idlClkInfo.setBiasNs(gnssClock.biasNs);
    idlClkInfo.setBiasUncertaintyNs(gnssClock.biasUncertaintyNs);
    idlClkInfo.setDriftNsps(gnssClock.driftNsps);
    idlClkInfo.setDriftUncertaintyNsps(gnssClock.driftUncertaintyNsps);
    idlClkInfo.setHwClockDiscontinuityCount(gnssClock.hwClockDiscontinuityCount);
    idlClkInfo.setElapsedRealTime(gnssClock.elapsedRealTime);
    idlClkInfo.setElapsedRealTimeUnc(gnssClock.elapsedRealTimeUnc);
    idlClkInfo.setElapsedgPTPTime(gnssClock.elapsedgPTPTime);
    idlClkInfo.setElapsedgPTPTimeUnc(gnssClock.elapsedgPTPTimeUnc);
    return idlClkInfo;
}

uint32_t parseIDLMeasFlags(
    ::GnssMeasurementsDataFlagsMask gnssFlags
) {

    uint32_t idlFlags = 0;
    if (GNSS_MEASUREMENTS_DATA_SV_ID_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::IDL_MEAS_DATA_SV_ID_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::IDL_MEAS_DATA_SV_TYPE_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_STATE_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::IDL_MEAS_DATA_STATE_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_RECEIVED_SV_TIME_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_RECVD_SV_TIME_UNC_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_CARRIER_TO_NOISE_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::IDL_MEAS_DATA_PR_RATE_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
               IDL_MEAS_DATA_PR_RATE_UNC_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
               IDL_MEAS_DATA_ADR_STATE_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_ADR_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::IDL_MEAS_DATA_ADR_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::IDL_MEAS_DATA_ADR_UNC_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_CARRIER_FREQ_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_CARRIER_CYCLES_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_CARRIER_PHASE_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_CARRIER_PHASE_UNC_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_MULTIPATH_IND_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_SIGNAL_TO_NOISE_RATIO;
    }

    if (GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_AUTO_GAIN_CTRL_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_FULL_ISB_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_FULL_ISB_UNC_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_CYCLE_SLIP_COUNT_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_GNSS_SIGNAL_TYPE_BIT;
    }

    if (GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT & gnssFlags) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsDataFlagsMask::\
                IDL_MEAS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT;
    }

    return idlFlags;
}

uint32_t parseIDLStateMask(
    ::GnssMeasurementsStateMask stateMask
) {

    uint32_t idlFlags = 0;
    if (GNSS_MEASUREMENTS_STATE_UNKNOWN_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_UNKNOWN;
    }

    if (GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_CODE_LOCK;
    }

    if (GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_BIT_SYNC;
    }

    if (GNSS_MEASUREMENTS_STATE_SUBFRAME_SYNC_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_SUBFRAME_SYNC;
    }

    if (GNSS_MEASUREMENTS_STATE_TOW_DECODED_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_TOW_DECODED;
    }

    if (GNSS_MEASUREMENTS_STATE_MSEC_AMBIGUOUS_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_MSEC_AMBIGUOUS;
    }

    if (GNSS_MEASUREMENTS_STATE_SYMBOL_SYNC_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_SYMBOL_SYNC;
    }

    if (GNSS_MEASUREMENTS_STATE_GLO_STRING_SYNC_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_GLO_STRING_SYNC;
    }

    if (GNSS_MEASUREMENTS_STATE_GLO_TOD_DECODED_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::\
                IDL_MEAS_STATE_GLO_TOD_DECODED;
    }

    if (GNSS_MEASUREMENTS_STATE_BDS_D2_BIT_SYNC_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::\
                IDL_MEAS_STATE_BDS_D2_BIT_SYNC;
    }

    if (GNSS_MEASUREMENTS_STATE_BDS_D2_SUBFRAME_SYNC_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::\
                IDL_MEAS_STATE_BDS_D2_SUBFRAME_SYNC;
    }

    if (GNSS_MEASUREMENTS_STATE_GAL_E1BC_CODE_LOCK_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_GAL_E1BC_CODE_LOCK;
    }

    if (GNSS_MEASUREMENTS_STATE_GAL_E1C_2ND_CODE_LOCK_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::\
               IDL_MEAS_STATE_GAL_E1C_2ND_CODE_LOCK;
    }

    if (GNSS_MEASUREMENTS_STATE_GAL_E1B_PAGE_SYNC_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::\
                IDL_MEAS_STATE_GAL_E1B_PAGE_SYNC;
    }

    if (GNSS_MEASUREMENTS_STATE_SBAS_SYNC_BIT  & stateMask) {
        idlFlags |= LocIdlAPI::IDLGnssMeasurementsStateMask::IDL_MEAS_STATE_SBAS_SYNC;
    }

    return idlFlags;

}

uint32_t parseIDLAdrStateMask(
    ::GnssMeasurementsAdrStateMask adrStatemask
) {

    uint32_t idlAdrMask = 0;
    if (GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_UNKNOWN & adrStatemask) {
        idlAdrMask |= LocIdlAPI::IDLGnssMeasurementsAdrStateMask::\
                    IDL_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_UNKNOWN;
    }

    if (GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT & adrStatemask) {
        idlAdrMask |= LocIdlAPI::IDLGnssMeasurementsAdrStateMask::\
                    IDL_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT;
    }

    if (GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT & adrStatemask) {
        idlAdrMask |= LocIdlAPI::IDLGnssMeasurementsAdrStateMask::\
                IDL_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT;
    }

    if (GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT & adrStatemask) {
        idlAdrMask |= LocIdlAPI::IDLGnssMeasurementsAdrStateMask::\
                IDL_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT;
    }

    if (GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT & adrStatemask) {
        idlAdrMask |= LocIdlAPI::IDLGnssMeasurementsAdrStateMask::\
                IDL_GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT;
    }

    return idlAdrMask;
}

uint32_t parseIDLMultiPathIndicator(
    ::GnssMeasurementsMultipathIndicator multipathIndicator
) {

    uint32_t idlMultiPathInd = LocIdlAPI::IDLGnssMeasurementsMultipathIndicator::\
            IDL_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN;
    switch (multipathIndicator) {
    case GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT:
        idlMultiPathInd = LocIdlAPI::IDLGnssMeasurementsMultipathIndicator::\
                IDL_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT;
        break;

        case GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT:
        idlMultiPathInd = LocIdlAPI::IDLGnssMeasurementsMultipathIndicator::\
                IDL_GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT;
        break;
    }

    return idlMultiPathInd;
}

vector<LocIdlAPI::IDLGnssMeasurementsData > parseIDLMeasData
(
    const vector<::GnssMeasurementsData>& gnssMeasData
) {
    vector<LocIdlAPI::IDLGnssMeasurementsData > idlMeasData;
    std::cout << "No of meas " << gnssMeasData.size() << std::endl;
    LOC_LOGd("Number of Meas %d", gnssMeasData.size());
    LocIdlAPI::IDLGnssMeasurementsData idlMeas = {};
    for (uint32_t idx = 0; idx < gnssMeasData.size(); idx++) {
        memset(&idlMeas, 0, sizeof(idlMeas));
        idlMeas.setFlags(::parseIDLMeasFlags(gnssMeasData[idx].flags));
        idlMeas.setSvId(gnssMeasData[idx].svId);
        idlMeas.setSvType(::parseIDLSvType(gnssMeasData[idx].svType));
        idlMeas.setTimeOffsetNs(gnssMeasData[idx].timeOffsetNs);
        idlMeas.setStateMask(::parseIDLStateMask(gnssMeasData[idx].stateMask));
        idlMeas.setReceivedSvTimeNs(gnssMeasData[idx].receivedSvTimeNs);
        idlMeas.setReceivedSvTimeSubNs(gnssMeasData[idx].receivedSvTimeSubNs);
        idlMeas.setReceivedSvTimeUncertaintyNs(gnssMeasData[idx].receivedSvTimeUncertaintyNs);
        idlMeas.setCarrierToNoiseDbHz(gnssMeasData[idx].carrierToNoiseDbHz);
        idlMeas.setPseudorangeRateMps(gnssMeasData[idx].pseudorangeRateMps);
        idlMeas.setPseudorangeRateUncertaintyMps(gnssMeasData[idx].pseudorangeRateUncertaintyMps);
        idlMeas.setAdrStateMask(::parseIDLAdrStateMask(gnssMeasData[idx].adrStateMask));
        idlMeas.setAdrMeters(gnssMeasData[idx].adrMeters);
        idlMeas.setAdrUncertaintyMeters(gnssMeasData[idx].adrUncertaintyMeters);
        idlMeas.setCarrierFrequencyHz(gnssMeasData[idx].carrierFrequencyHz);
        idlMeas.setCarrierCycles(gnssMeasData[idx].carrierCycles);
        idlMeas.setCarrierPhase(gnssMeasData[idx].carrierPhase);
        idlMeas.setCarrierPhaseUncertainty(gnssMeasData[idx].carrierPhaseUncertainty);
        idlMeas.setMultipathIndicator(::parseIDLMultiPathIndicator(
                gnssMeasData[idx].multipathIndicator));
        idlMeas.setSignalToNoiseRatioDb(gnssMeasData[idx].signalToNoiseRatioDb);
        idlMeas.setAgcLevelDb(gnssMeasData[idx].agcLevelDb);
        idlMeas.setBasebandCarrierToNoiseDbHz(gnssMeasData[idx].basebandCarrierToNoiseDbHz);
        idlMeas.setGnssSignalType(::parseIDLSignalType(gnssMeasData[idx].gnssSignalType));
        idlMeas.setFullInterSignalBiasNs(gnssMeasData[idx].fullInterSignalBiasNs);
        idlMeas.setFullInterSignalBiasUncertaintyNs(
                gnssMeasData[idx].fullInterSignalBiasUncertaintyNs);
        idlMeas.setCycleSlipCount(gnssMeasData[idx].cycleSlipCount);

        idlMeasData.push_back(idlMeas);
    }

    return idlMeasData;
}

LocIdlAPI::IDLGnssMeasurements LocLcaIdlConverter::parseMeasurements
(
    const ::GnssMeasurements& gnssMeas
) {

    LocIdlAPI::IDLGnssMeasurements idlGnssMeas = {};
    memset(&idlGnssMeas, 0, sizeof(idlGnssMeas));

    idlGnssMeas.setClock(::parseIDLMeasClockInfo(gnssMeas.clock));

    idlGnssMeas.setMeasurements(::parseIDLMeasData(gnssMeas.measurements));

    idlGnssMeas.setIsNHz(gnssMeas.isNhz);
    return idlGnssMeas;
}

uint32_t LocLcaIdlConverter::parseIDLDataMask
(
    const ::GnssDataMask& mask
) {

    uint32_t idlMask = 0;

    if (GNSS_DATA_JAMMER_IND_BIT & mask) {
        idlMask |= LocIdlAPI::IDLGnssDataMask::IDL_GNSS_DATA_JAMMER_IND_BIT;
    }

    if (GNSS_DATA_AGC_BIT & mask) {
        idlMask |= LocIdlAPI::IDLGnssDataMask::IDL_GNSS_DATA_AGC_BIT;
    }
    return idlMask;
}

LocIdlAPI::IDLGnssData LocLcaIdlConverter::parseGnssData
(
    const ::GnssData& gnssData
) {

    LocIdlAPI::IDLGnssData idlGnssdata = {};
    memset (&idlGnssdata, 0, sizeof(idlGnssdata));

    vector<uint32_t> dataMaskVal;
    vector<double> jammerIndVal;
    vector<double> agcVal;
    for (uint8_t idx = 0; idx < (LocIdlAPI::IDLGnssSignalTypes::\
            IDL_GNSS_MAX_NUMBER_OF_SIGNAL_TYPES - 1); idx++) {

         dataMaskVal.push_back(parseIDLDataMask(gnssData.gnssDataMask[idx]));
         jammerIndVal.push_back(gnssData.jammerInd[idx]);
         agcVal.push_back(gnssData.agc[idx]);
    }

    idlGnssdata.setGnssDataMask(dataMaskVal);
    idlGnssdata.setJammerInd(jammerIndVal);
    idlGnssdata.setAgc(agcVal);

    return idlGnssdata;
}

LocLcaIdlConverter::LocLcaIdlConverter() {

}

LocLcaIdlConverter::~LocLcaIdlConverter() {

}
