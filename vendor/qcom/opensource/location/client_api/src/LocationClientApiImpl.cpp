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

#define LOG_TAG "LocSvc_LocationClientApi"

#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <loc_cfg.h>
#include <LocationClientApiImpl.h>
#include <log_util.h>
#include <gps_extended_c.h>
#include <unistd.h>
#include <sstream>
#include <dlfcn.h>
#include <loc_misc_utils.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

static uint32_t gDebug = 0;
static uint32_t gSleepTime = 800000;

static const loc_param_s_type gConfigTable[] =
{
    {"DEBUG_LEVEL", &gDebug, NULL, 'n'},
    {"QRTRWATCHER_DELAY_MICROSECOND", &gSleepTime, NULL, 'n'}
};

namespace location_client {

static uint32_t gfIdGenerator = LOCATION_CLIENT_SESSION_ID_INVALID;
mutex GeofenceImpl::mGfMutex;
uint32_t GeofenceImpl::nextId() {
    lock_guard<mutex> lock(mGfMutex);
    uint32_t id = ++gfIdGenerator;
    if (0xFFFFFFFF == id) {
        id = 0;
        gfIdGenerator = 0;
    }
    return id;
}

/******************************************************************************
Utilities
******************************************************************************/
GnssMeasurementsDataFlagsMask LocationClientApiImpl::parseMeasurementsDataMask(
        ::GnssMeasurementsDataFlagsMask in) {
    uint32_t out = 0;
    LOC_LOGd("Hal GnssMeasurementsDataFlagsMask =0x%x ", in);

    if (::GNSS_MEASUREMENTS_DATA_SV_ID_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_SV_ID_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_STATE_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_STATE_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_ADR_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_ADR_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT;
    }
    if (::GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT & in) {
        out |= GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT;
    }
    LOC_LOGd("LCA GnssMeasurementsDataFlagsMask =0x%x ", out);
    return static_cast<GnssMeasurementsDataFlagsMask>(out);
}

LocationCapabilitiesMask LocationClientApiImpl::parseCapabilitiesMask(
        ::LocationCapabilitiesMask mask) {
    LocationCapabilitiesMask capsMask = 0;
    if (LOCATION_CAPABILITIES_TIME_BASED_TRACKING_BIT & mask) {
        capsMask |= LOCATION_CAPS_TIME_BASED_TRACKING_BIT;
    }
    if (LOCATION_CAPABILITIES_TIME_BASED_BATCHING_BIT & mask) {
        capsMask |=  LOCATION_CAPS_TIME_BASED_BATCHING_BIT;
    }
    if (LOCATION_CAPABILITIES_DISTANCE_BASED_TRACKING_BIT & mask) {
        capsMask |=  LOCATION_CAPS_DISTANCE_BASED_TRACKING_BIT;
    }
    if (LOCATION_CAPABILITIES_DISTANCE_BASED_BATCHING_BIT & mask) {
        capsMask |=  LOCATION_CAPS_DISTANCE_BASED_BATCHING_BIT;
    }
    if (LOCATION_CAPABILITIES_GEOFENCE_BIT & mask) {
        capsMask |=  LOCATION_CAPS_GEOFENCE_BIT;
    }
    if (LOCATION_CAPABILITIES_OUTDOOR_TRIP_BATCHING_BIT & mask) {
        capsMask |=  LOCATION_CAPS_OUTDOOR_TRIP_BATCHING_BIT;
    }
    if (LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT & mask) {
        capsMask |=  LOCATION_CAPS_GNSS_MEASUREMENTS_BIT;
    }
    if (LOCATION_CAPABILITIES_CONSTELLATION_ENABLEMENT_BIT & mask) {
        capsMask |=  LOCATION_CAPS_CONSTELLATION_ENABLEMENT_BIT;
    }
    if (LOCATION_CAPABILITIES_QWES_CARRIER_PHASE_BIT & mask) {
        capsMask |=  LOCATION_CAPS_CARRIER_PHASE_BIT;
    }
    if (LOCATION_CAPABILITIES_QWES_SV_POLYNOMIAL_BIT & mask) {
        capsMask |=  LOCATION_CAPS_SV_POLYNOMIAL_BIT;
    }
    if (LOCATION_CAPABILITIES_QWES_GNSS_SINGLE_FREQUENCY & mask) {
        capsMask |=  LOCATION_CAPS_QWES_GNSS_SINGLE_FREQUENCY;
    }
    if (LOCATION_CAPABILITIES_QWES_GNSS_MULTI_FREQUENCY & mask) {
        capsMask |=  LOCATION_CAPS_QWES_GNSS_MULTI_FREQUENCY;
    }
    if (LOCATION_CAPABILITIES_QWES_VPE & mask) {
        capsMask |=  LOCATION_CAPS_QWES_VPE;
    }
    if (LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_BASIC & mask) {
        capsMask |=  LOCATION_CAPS_QWES_CV2X_LOCATION_BASIC;
    }
    if (LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_PREMIUM & mask) {
        capsMask |=  LOCATION_CAPS_QWES_CV2X_LOCATION_PREMIUM;
    }
    if (LOCATION_CAPABILITIES_QWES_PPE & mask) {
        capsMask |=  LOCATION_CAPS_QWES_PPE;
    }
    if (LOCATION_CAPABILITIES_QWES_QDR2 & mask) {
        capsMask |=  LOCATION_CAPS_QWES_QDR2;
    }
    if (LOCATION_CAPABILITIES_QWES_QDR3 & mask) {
        capsMask |=  LOCATION_CAPS_QWES_QDR3;
    }
    if (LOCATION_CAPABILITIES_NLOS_ML20 & mask) {
        capsMask |=  LOCATION_CAPS_NLOS_ML20;
    }
    LOC_LOGd ("parseCapabilitiesMask LocCapabMask =0x%" PRIx64 " LCA mask 0x%" PRIx64,
            mask, capsMask);
    return capsMask;
}

uint16_t LocationClientApiImpl::parseYearOfHw(::LocationCapabilitiesMask mask) {
    uint16_t yearOfHw = 2015;

    if (LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT & mask) {
        yearOfHw++; // 2016
        if (LOCATION_CAPABILITIES_DEBUG_DATA_BIT & mask) {
            yearOfHw++; // 2017
            if (LOCATION_CAPABILITIES_CONSTELLATION_ENABLEMENT_BIT & mask ||
                LOCATION_CAPABILITIES_AGPM_BIT & mask) {
                yearOfHw++; // 2018
                if (LOCATION_CAPABILITIES_PRIVACY_BIT & mask) {
                    yearOfHw++; // 2019
                    if (LOCATION_CAPABILITIES_MEASUREMENTS_CORRECTION_BIT & mask) {
                        yearOfHw++; // 2020
                    }
                }
            }
        }
    }
    return yearOfHw;
}

void LocationClientApiImpl::parseLocation(const ::Location &halLocation, Location& location) {
    uint32_t flags = 0;

    location.sessionStatus = (LocSessionStatus)halLocation.sessionStatus;
    location.timestamp = halLocation.timestamp;
    location.timeUncMs = halLocation.timeUncMs;
    location.latitude = halLocation.latitude;
    location.longitude = halLocation.longitude;
    location.altitude = halLocation.altitude;
    location.speed = halLocation.speed;
    location.bearing = halLocation.bearing;
    location.horizontalAccuracy = halLocation.accuracy;
    location.verticalAccuracy = halLocation.verticalAccuracy;
    location.speedAccuracy = halLocation.speedAccuracy;
    location.bearingAccuracy = halLocation.bearingAccuracy;
#ifndef FEATURE_EXTERNAL_AP
    location.elapsedRealTimeNs = halLocation.elapsedRealTime;
    location.elapsedRealTimeUncNs = halLocation.elapsedRealTimeUnc;
#else
    location.elapsedRealTimeNs = 0;
    location.elapsedRealTimeUncNs = 0;
#endif

    if (0 != halLocation.timestamp) {
        flags |= LOCATION_HAS_TIMESTAMP_BIT;
    }
    if (::LOCATION_HAS_TIME_UNC_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_TIME_UNC_BIT;
    }
    if (::LOCATION_HAS_LAT_LONG_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_LAT_LONG_BIT;
    }
    if (::LOCATION_HAS_ALTITUDE_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_ALTITUDE_BIT;
    }
    if (::LOCATION_HAS_SPEED_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_SPEED_BIT;
    }
    if (::LOCATION_HAS_BEARING_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_BEARING_BIT;
    }
    if (::LOCATION_HAS_ACCURACY_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_ACCURACY_BIT;
    }
    if (::LOCATION_HAS_VERTICAL_ACCURACY_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_VERTICAL_ACCURACY_BIT;
    }
    if (::LOCATION_HAS_SPEED_ACCURACY_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_SPEED_ACCURACY_BIT;
    }
    if (::LOCATION_HAS_BEARING_ACCURACY_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_BEARING_ACCURACY_BIT;
    }
#ifndef FEATURE_EXTERNAL_AP
    if (::LOCATION_HAS_ELAPSED_REAL_TIME_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_ELAPSED_REAL_TIME_BIT;
        flags |= LOCATION_HAS_ELAPSED_REAL_TIME_UNC_BIT;
    }
#endif

    if (::LOCATION_HAS_GPTP_TIME_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_GPTP_TIME_BIT;
        location.elapsedgPTPTime  =  halLocation.elapsedgPTPTime;
    }

    if (::LOCATION_HAS_GPTP_TIME_UNC_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_GPTP_TIME_UNC_BIT;
        location.elapsedgPTPTimeUnc =  halLocation.elapsedgPTPTimeUnc;
    }

    if (::LOCATION_HAS_SESSION_STATUS_BIT & halLocation.flags) {
        flags |= LOCATION_HAS_SESSION_STATUS_BIT;
    }
    location.flags = (LocationFlagsMask)flags;

    flags = 0;
    if (::LOCATION_TECHNOLOGY_GNSS_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_GNSS_BIT;
    }
    if (::LOCATION_TECHNOLOGY_CELL_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_CELL_BIT;
    }
    if (::LOCATION_TECHNOLOGY_WIFI_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_WIFI_BIT;
    }
    if (::LOCATION_TECHNOLOGY_SENSORS_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_SENSORS_BIT;
    }
    if (::LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT;
    }
    if (::LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT;
    }
    if (::LOCATION_TECHNOLOGY_AFLT_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_AFLT_BIT;
    }
    if (::LOCATION_TECHNOLOGY_HYBRID_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_HYBRID_BIT;
    }
    if (::LOCATION_TECHNOLOGY_PPE_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_PPE_BIT;
    }
    if (::LOCATION_TECHNOLOGY_VEH_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_VEH_BIT;
    }
    if (::LOCATION_TECHNOLOGY_VIS_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_VIS_BIT;
    }
    if (::LOCATION_TECHNOLOGY_PROPAGATED_BIT & halLocation.techMask) {
        flags |= LOCATION_TECHNOLOGY_PROPAGATED_BIT;
    }
    location.techMask = (LocationTechnologyMask)flags;
}

Location LocationClientApiImpl::parseLocation(const ::Location &halLocation) {
    Location location = {};
    parseLocation(halLocation, location);
    return location;
}

GnssLocationSvUsedInPosition LocationClientApiImpl::parseLocationSvUsedInPosition(
        const ::GnssLocationSvUsedInPosition &halSv) {

    GnssLocationSvUsedInPosition clientSv = {};

    clientSv.gpsSvUsedIdsMask = halSv.gpsSvUsedIdsMask;
    clientSv.gloSvUsedIdsMask = halSv.gloSvUsedIdsMask;
    clientSv.galSvUsedIdsMask = halSv.galSvUsedIdsMask;
    clientSv.bdsSvUsedIdsMask = halSv.bdsSvUsedIdsMask;
    clientSv.qzssSvUsedIdsMask = halSv.qzssSvUsedIdsMask;
    clientSv.navicSvUsedIdsMask = halSv.navicSvUsedIdsMask;
    return clientSv;
}

GnssSignalTypeMask LocationClientApiImpl::parseGnssSignalType(
        const ::GnssSignalTypeMask &halGnssSignalTypeMask) {
    uint32_t gnssSignalTypeMask = 0;

    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GPS_L1CA) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GPS_L1CA_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GPS_L1C) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GPS_L1C_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GPS_L2) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GPS_L2_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GPS_L5) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GPS_L5_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GLONASS_G1) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GLONASS_G1_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GLONASS_G2) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GLONASS_G2_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GALILEO_E1) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GALILEO_E1_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GALILEO_E5A) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GALILEO_E5A_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_GALILEO_E5B) {
        gnssSignalTypeMask |= GNSS_SIGNAL_GALILEO_E5B_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B1I) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B1I_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B1C) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B1C_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2I) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2I_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2AI) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2AI_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_QZSS_L1CA) {
        gnssSignalTypeMask |= GNSS_SIGNAL_QZSS_L1CA_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_QZSS_L1S) {
        gnssSignalTypeMask |= GNSS_SIGNAL_QZSS_L1S_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_QZSS_L2) {
        gnssSignalTypeMask |= GNSS_SIGNAL_QZSS_L2_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_QZSS_L5) {
        gnssSignalTypeMask |= GNSS_SIGNAL_QZSS_L5_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_SBAS_L1) {
        gnssSignalTypeMask |= GNSS_SIGNAL_SBAS_L1_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_NAVIC_L5) {
        gnssSignalTypeMask |= GNSS_SIGNAL_NAVIC_L5_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2AQ) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2AQ_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B1) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B1;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2BI) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2BI_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2BQ) {
        gnssSignalTypeMask |= GNSS_SIGNAL_BEIDOU_B2BQ_BIT;
    }
    if (halGnssSignalTypeMask & ::GNSS_SIGNAL_NAVIC_L1) {
        gnssSignalTypeMask |= GNSS_SIGNAL_NAVIC_L1_BIT;
    }
    return (GnssSignalTypeMask)gnssSignalTypeMask;
}

void LocationClientApiImpl::parseGnssMeasUsageInfo(
        const ::GnssLocationInfoNotification &halLocationInfo,
        std::vector<GnssMeasUsageInfo>& clientMeasUsageInfo) {

    if (halLocationInfo.numOfMeasReceived) {
        for (int idx = 0; idx < halLocationInfo.numOfMeasReceived; idx++) {
            GnssMeasUsageInfo measUsageInfo = {};

            measUsageInfo.gnssSignalType = parseGnssSignalType(
                    halLocationInfo.measUsageInfo[idx].gnssSignalType);
            measUsageInfo.gnssConstellation = (Gnss_LocSvSystemEnumType)
                    halLocationInfo.measUsageInfo[idx].gnssConstellation;
            measUsageInfo.gnssSvId = halLocationInfo.measUsageInfo[idx].gnssSvId;
            clientMeasUsageInfo.push_back(measUsageInfo);
        }
    }
}

GnssLocationPositionDynamics LocationClientApiImpl::parseLocationPositionDynamics(
        const ::GnssLocationPositionDynamics &halPositionDynamics,
        const ::GnssLocationPositionDynamicsExt &halPositionDynamicsExt) {
    GnssLocationPositionDynamics positionDynamics = {};
    uint32_t bodyFrameDataMask = 0;

    if (::LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT;
        positionDynamics.longAccel = halPositionDynamics.longAccel;
    }
    if (::LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT;
        positionDynamics.latAccel = halPositionDynamics.latAccel;
    }
    if (::LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT;
        positionDynamics.vertAccel = halPositionDynamics.vertAccel;
    }
    if (::LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT;
        positionDynamics.longAccelUnc = halPositionDynamics.longAccelUnc;
    }
    if (::LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT;
        positionDynamics.latAccelUnc = halPositionDynamics.latAccelUnc;
    }
    if (::LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT;
        positionDynamics.vertAccelUnc = halPositionDynamics.vertAccelUnc;
    }

    if (::LOCATION_NAV_DATA_HAS_ROLL_BIT & halPositionDynamicsExt.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_ROLL_BIT;
        positionDynamics.roll = halPositionDynamicsExt.roll;
    }
    if (::LOCATION_NAV_DATA_HAS_ROLL_UNC_BIT & halPositionDynamicsExt.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_ROLL_UNC_BIT;
        positionDynamics.rollUnc = halPositionDynamicsExt.rollUnc;
    }
    if (::LOCATION_NAV_DATA_HAS_ROLL_RATE_BIT & halPositionDynamicsExt.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_ROLL_RATE_BIT;
        positionDynamics.rollRate = halPositionDynamicsExt.rollRate;
    }
    if (::LOCATION_NAV_DATA_HAS_ROLL_RATE_UNC_BIT & halPositionDynamicsExt.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_ROLL_RATE_UNC_BIT;
        positionDynamics.rollRateUnc = halPositionDynamicsExt.rollRateUnc;
    }

    if (::LOCATION_NAV_DATA_HAS_PITCH_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_PITCH_BIT;
        positionDynamics.pitch = halPositionDynamics.pitch;
    }
    if (::LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT;
        positionDynamics.pitchUnc = halPositionDynamics.pitchUnc;
    }
    if (::LOCATION_NAV_DATA_HAS_PITCH_RATE_BIT & halPositionDynamicsExt.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_PITCH_RATE_BIT;
        positionDynamics.pitchRate = halPositionDynamicsExt.pitchRate;
    }
    if (::LOCATION_NAV_DATA_HAS_PITCH_RATE_UNC_BIT & halPositionDynamicsExt.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_PITCH_RATE_UNC_BIT;
        positionDynamics.pitchRateUnc = halPositionDynamicsExt.pitchRateUnc;
    }

    if (::LOCATION_NAV_DATA_HAS_YAW_BIT & halPositionDynamicsExt.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_YAW_BIT;
        positionDynamics.yaw = halPositionDynamicsExt.yaw;
    }
    if (::LOCATION_NAV_DATA_HAS_YAW_UNC_BIT & halPositionDynamicsExt.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_YAW_UNC_BIT;
        positionDynamics.yawUnc = halPositionDynamicsExt.yawUnc;
    }
    if (::LOCATION_NAV_DATA_HAS_YAW_RATE_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_YAW_RATE_BIT;
        positionDynamics.yawRate = halPositionDynamics.yawRate;
    }
    if (::LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT & halPositionDynamics.bodyFrameDataMask) {
        bodyFrameDataMask |= LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT;
        positionDynamics.yawRateUnc = halPositionDynamics.yawRateUnc;
    }
    positionDynamics.bodyFrameDataMask = (GnssLocationPosDataMask)bodyFrameDataMask;

    return positionDynamics;
}

LocationReliability LocationClientApiImpl::parseLocationReliability(
        const ::LocationReliability &halReliability) {

    LocationReliability reliability;
    switch (halReliability) {
        case ::LOCATION_RELIABILITY_VERY_LOW:
            reliability = LOCATION_RELIABILITY_VERY_LOW;
            break;

        case ::LOCATION_RELIABILITY_LOW:
            reliability = LOCATION_RELIABILITY_LOW;
            break;

        case ::LOCATION_RELIABILITY_MEDIUM:
            reliability = LOCATION_RELIABILITY_MEDIUM;
            break;

        case ::LOCATION_RELIABILITY_HIGH:
            reliability = LOCATION_RELIABILITY_HIGH;
            break;

        default:
            reliability = LOCATION_RELIABILITY_NOT_SET;
            break;
    }

    return reliability;
}

static AgcStatus parseAgcStatus(const ::AgcStatus &halAgcStatus) {
    AgcStatus agcStatus = AGC_STATUS_UNKNOWN;
    switch (halAgcStatus) {
        case ::AGC_STATUS_NO_SATURATION:
            agcStatus = AGC_STATUS_NO_SATURATION;
            break;
        case ::AGC_STATUS_FRONT_END_GAIN_MAXIMUM_SATURATION:
            agcStatus = AGC_STATUS_FRONT_END_GAIN_MAXIMUM_SATURATION;
            break;
        case ::AGC_STATUS_FRONT_END_GAIN_MINIMUM_SATURATION:
            agcStatus = AGC_STATUS_FRONT_END_GAIN_MINIMUM_SATURATION;
            break;
        default:
            break;
    }
    return agcStatus;
}

GnssSystemTimeStructType LocationClientApiImpl::parseGnssTime(
        const ::GnssSystemTimeStructType &halGnssTime) {

    GnssSystemTimeStructType   gnssTime;
    memset(&gnssTime, 0, sizeof(gnssTime));
    uint32_t gnssTimeFlags = 0;

    if (::GNSS_SYSTEM_TIME_WEEK_VALID & halGnssTime.validityMask) {
        gnssTimeFlags |= GNSS_SYSTEM_TIME_WEEK_VALID;
        gnssTime.systemWeek = halGnssTime.systemWeek;
    }
    if (::GNSS_SYSTEM_TIME_WEEK_MS_VALID & halGnssTime.validityMask) {
        gnssTimeFlags |= GNSS_SYSTEM_TIME_WEEK_MS_VALID;
        gnssTime.systemMsec = halGnssTime.systemMsec;
    }
    if (::GNSS_SYSTEM_CLK_TIME_BIAS_VALID & halGnssTime.validityMask) {
        gnssTimeFlags |= GNSS_SYSTEM_CLK_TIME_BIAS_VALID;
        gnssTime.systemClkTimeBias = halGnssTime.systemClkTimeBias;
    }
    if (::GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID & halGnssTime.validityMask) {
        gnssTimeFlags |= GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID;
        gnssTime.systemClkTimeUncMs = halGnssTime.systemClkTimeUncMs;
    }
    if (::GNSS_SYSTEM_REF_FCOUNT_VALID & halGnssTime.validityMask) {
        gnssTimeFlags |= GNSS_SYSTEM_REF_FCOUNT_VALID;
        gnssTime.refFCount = halGnssTime.refFCount;
    }
    if (::GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID & halGnssTime.validityMask) {
        gnssTimeFlags |= GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID;
        gnssTime.numClockResets = halGnssTime.numClockResets;
    }

    gnssTime.validityMask = (GnssSystemTimeStructTypeFlags)gnssTimeFlags;

    return gnssTime;
}

GnssGloTimeStructType LocationClientApiImpl::parseGloTime(
        const ::GnssGloTimeStructType &halGloTime) {

    GnssGloTimeStructType   gloTime = {};
    uint32_t gloTimeFlags = 0;

    if (::GNSS_CLO_DAYS_VALID & halGloTime.validityMask) {
        gloTimeFlags |= GNSS_CLO_DAYS_VALID;
        gloTime.gloDays = halGloTime.gloDays;
    }
    if (::GNSS_GLO_MSEC_VALID  & halGloTime.validityMask) {
        gloTimeFlags |= GNSS_GLO_MSEC_VALID ;
        gloTime.gloMsec = halGloTime.gloMsec;
    }
    if (::GNSS_GLO_CLK_TIME_BIAS_VALID & halGloTime.validityMask) {
        gloTimeFlags |= GNSS_GLO_CLK_TIME_BIAS_VALID;
        gloTime.gloClkTimeBias = halGloTime.gloClkTimeBias;
    }
    if (::GNSS_GLO_CLK_TIME_BIAS_UNC_VALID & halGloTime.validityMask) {
        gloTimeFlags |= GNSS_GLO_CLK_TIME_BIAS_UNC_VALID;
        gloTime.gloClkTimeUncMs = halGloTime.gloClkTimeUncMs;
    }
    if (::GNSS_GLO_REF_FCOUNT_VALID & halGloTime.validityMask) {
        gloTimeFlags |= GNSS_GLO_REF_FCOUNT_VALID;
        gloTime.refFCount = halGloTime.refFCount;
    }
    if (::GNSS_GLO_NUM_CLOCK_RESETS_VALID & halGloTime.validityMask) {
        gloTimeFlags |= GNSS_GLO_NUM_CLOCK_RESETS_VALID;
        gloTime.numClockResets = halGloTime.numClockResets;
    }
    if (::GNSS_GLO_FOUR_YEAR_VALID & halGloTime.validityMask) {
        gloTimeFlags |= GNSS_GLO_FOUR_YEAR_VALID;
        gloTime.gloFourYear = halGloTime.gloFourYear;
    }

    gloTime.validityMask = (GnssGloTimeStructTypeFlags)gloTimeFlags;

    return gloTime;
}

GnssSystemTime LocationClientApiImpl::parseSystemTime(const ::GnssSystemTime &halSystemTime) {

    GnssSystemTime systemTime = {};

    switch (halSystemTime.gnssSystemTimeSrc) {
        case ::GNSS_LOC_SV_SYSTEM_GPS:
           systemTime.gnssSystemTimeSrc = GNSS_LOC_SV_SYSTEM_GPS;
           systemTime.u.gpsSystemTime = parseGnssTime(halSystemTime.u.gpsSystemTime);
           break;
        case ::GNSS_LOC_SV_SYSTEM_GALILEO:
           systemTime.gnssSystemTimeSrc = GNSS_LOC_SV_SYSTEM_GALILEO;
           systemTime.u.galSystemTime = parseGnssTime(halSystemTime.u.galSystemTime);
           break;
        case ::GNSS_LOC_SV_SYSTEM_SBAS:
           systemTime.gnssSystemTimeSrc = GNSS_LOC_SV_SYSTEM_SBAS;
           break;
        case ::GNSS_LOC_SV_SYSTEM_GLONASS:
           systemTime.gnssSystemTimeSrc = GNSS_LOC_SV_SYSTEM_GLONASS;
           systemTime.u.gloSystemTime = parseGloTime(halSystemTime.u.gloSystemTime);
           break;
        case ::GNSS_LOC_SV_SYSTEM_BDS:
           systemTime.gnssSystemTimeSrc = GNSS_LOC_SV_SYSTEM_BDS;
           systemTime.u.bdsSystemTime = parseGnssTime(halSystemTime.u.bdsSystemTime);
           break;
        case ::GNSS_LOC_SV_SYSTEM_QZSS:
           systemTime.gnssSystemTimeSrc = GNSS_LOC_SV_SYSTEM_QZSS;
           systemTime.u.qzssSystemTime = parseGnssTime(halSystemTime.u.qzssSystemTime);
           break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
           systemTime.gnssSystemTimeSrc = GNSS_LOC_SV_SYSTEM_NAVIC;
           systemTime.u.navicSystemTime = parseGnssTime(halSystemTime.u.navicSystemTime);
           break;
        default:
           break;
    }

    return systemTime;
}

GnssLocation LocationClientApiImpl::parseLocationInfo(
        const ::GnssLocationInfoNotification &halLocationInfo) {

    GnssLocation locationInfo = {};
    parseLocation(halLocationInfo.location, locationInfo);
    LCAGnssLocationInfoFlagMask flags = 0;

    if (LDT_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_DOP_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_DOP_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_VER_RELIABILITY_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_VER_RELIABILITY_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT;
    }
    flags |= LCA_GNSS_LOCATION_INFO_POS_TECH_MASK_BIT;
    if (LDT_GNSS_LOCATION_INFO_SV_SOURCE_INFO_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_SV_SOURCE_INFO_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_EXT_DOP_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_EXT_DOP_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_EAST_STD_DEV_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_EAST_STD_DEV_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_NORTH_VEL_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_NORTH_VEL_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_EAST_VEL_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_EAST_VEL_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_UP_VEL_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_UP_VEL_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_UP_VEL_UNC_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_UP_VEL_UNC_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_LEAP_SECONDS_BIT & halLocationInfo.flags) {
       flags |= LCA_GNSS_LOCATION_INFO_LEAP_SECONDS_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_PERCENT_BIT;
        locationInfo.calibrationConfidencePercent = halLocationInfo.calibrationConfidence;
    }
    if (LDT_GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT;
        locationInfo.calibrationStatus =
                (DrCalibrationStatusMask)halLocationInfo.calibrationStatus;
    }
    if (LDT_GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT;
    }
    if (LDT_GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_CONFORMITY_INDEX_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_CONFORMITY_INDEX_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_LLA_VRP_BASED_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_LLA_VRP_BASED_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_ENU_VELOCITY_VRP_BASED_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_ENU_VELOCITY_VRP_BASED_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_DR_SOLUTION_STATUS_MASK_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_DR_SOLUTION_STATUS_MASK_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_ALTITUDE_ASSUMED_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_ALTITUDE_ASSUMED_BIT;
    }

    //sessionStatus is set in parseLocation
    if (::LOCATION_HAS_SESSION_STATUS_BIT & halLocationInfo.location.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_SESSION_STATUS_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_INTEGRITY_RISK_USED_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_INTEGRITY_RISK_USED_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_PROTECT_ALONG_TRACK_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_PROTECT_ALONG_TRACK_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_PROTECT_CROSS_TRACK_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_PROTECT_CROSS_TRACK_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_PROTECT_VERTICAL_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_PROTECT_VERTICAL_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_DGNSS_STATION_ID_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_DGNSS_STATION_ID_BIT;
    }

    if (LDT_GNSS_LOCATION_INFO_BASE_LINE_LENGTH_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_BASE_LINE_LENGTH_BIT;
        locationInfo.baseLineLength = halLocationInfo.baseLineLength;
    }

    if (LDT_GNSS_LOCATION_INFO_AGE_OF_CORRECTION_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_AGE_OF_CORRECTION_BIT;
        locationInfo.ageMsecOfCorrections = halLocationInfo.ageMsecOfCorrections;
    }

    if (LDT_GNSS_LOCATION_INFO_LEAP_SECONDS_UNC_BIT & halLocationInfo.flags) {
        flags |= LCA_GNSS_LOCATION_INFO_LEAP_SECONDS_UNC_BIT;
    }

    locationInfo.gnssInfoFlags = flags;
    locationInfo.altitudeMeanSeaLevel = halLocationInfo.altitudeMeanSeaLevel;
    locationInfo.pdop = halLocationInfo.pdop;
    locationInfo.hdop = halLocationInfo.hdop;
    locationInfo.vdop = halLocationInfo.vdop;
    locationInfo.gdop = halLocationInfo.gdop;
    locationInfo.tdop = halLocationInfo.tdop;
    locationInfo.magneticDeviation = halLocationInfo.magneticDeviation;
    locationInfo.horReliability = parseLocationReliability(halLocationInfo.horReliability);
    locationInfo.verReliability = parseLocationReliability(halLocationInfo.verReliability);
    locationInfo.horUncEllipseSemiMajor = halLocationInfo.horUncEllipseSemiMajor;
    locationInfo.horUncEllipseSemiMinor = halLocationInfo.horUncEllipseSemiMinor;
    locationInfo.horUncEllipseOrientAzimuth = halLocationInfo.horUncEllipseOrientAzimuth;
    locationInfo.northStdDeviation = halLocationInfo.northStdDeviation;
    locationInfo.eastStdDeviation = halLocationInfo.eastStdDeviation;
    locationInfo.northVelocity = halLocationInfo.northVelocity;
    locationInfo.eastVelocity = halLocationInfo.eastVelocity;
    locationInfo.upVelocity = halLocationInfo.upVelocity;
    locationInfo.northVelocityStdDeviation = halLocationInfo.northVelocityStdDeviation;
    locationInfo.eastVelocityStdDeviation = halLocationInfo.eastVelocityStdDeviation;
    locationInfo.upVelocityStdDeviation = halLocationInfo.upVelocityStdDeviation;
    locationInfo.numSvUsedInPosition = halLocationInfo.numSvUsedInPosition;
    locationInfo.svUsedInPosition =
            parseLocationSvUsedInPosition(halLocationInfo.svUsedInPosition);
    locationInfo.locOutputEngType =
            (LocOutputEngineType)halLocationInfo.locOutputEngType;
    locationInfo.locOutputEngMask =
            (PositioningEngineMask)halLocationInfo.locOutputEngMask;
    locationInfo.conformityIndex = halLocationInfo.conformityIndex;
    locationInfo.llaVRPBased.latitude = halLocationInfo.llaVRPBased.latitude;
    locationInfo.llaVRPBased.longitude = halLocationInfo.llaVRPBased.longitude;
    locationInfo.llaVRPBased.altitude = halLocationInfo.llaVRPBased.altitude;
    // copy VRP-based north, east, up velocity
    locationInfo.enuVelocityVRPBased[0] = halLocationInfo.enuVelocityVRPBased[0];
    locationInfo.enuVelocityVRPBased[1] = halLocationInfo.enuVelocityVRPBased[1];
    locationInfo.enuVelocityVRPBased[2] = halLocationInfo.enuVelocityVRPBased[2];
    parseGnssMeasUsageInfo(halLocationInfo, locationInfo.measUsageInfo);
    locationInfo.drSolutionStatusMask = (DrSolutionStatusMask) halLocationInfo.drSolutionStatusMask;
    locationInfo.altitudeAssumed = halLocationInfo.altitudeAssumed;
    locationInfo.integrityRiskUsed =  halLocationInfo.integrityRiskUsed;
    locationInfo.protectAlongTrack =  halLocationInfo.protectAlongTrack;
    locationInfo.protectCrossTrack =  halLocationInfo.protectCrossTrack;
    locationInfo.protectVertical =  halLocationInfo.protectVertical;
    for (uint32_t i = 0; i < halLocationInfo.numOfDgnssStationId; i++) {
        locationInfo.dgnssStationId.push_back(halLocationInfo.dgnssStationId[i]);
    }

    flags = 0;
    if (::LOCATION_SBAS_CORRECTION_IONO_BIT & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_SBAS_CORRECTION_IONO_BIT;
    }
    if (::LOCATION_SBAS_CORRECTION_FAST_BIT & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_SBAS_CORRECTION_FAST_BIT;
    }
    if (::LOCATION_SBAS_CORRECTION_LONG_BIT & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_SBAS_CORRECTION_LONG_BIT;
    }
    if (::LOCATION_SBAS_INTEGRITY_BIT & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_SBAS_INTEGRITY_BIT;
    }
    if (::LOCATION_NAV_CORRECTION_DGNSS_BIT & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_NAV_CORRECTION_DGNSS_BIT;
    }
    if (::LOCATION_NAV_CORRECTION_RTK_BIT & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_NAV_CORRECTION_RTK_BIT;
    }
    if (::LOCATION_NAV_CORRECTION_RTK_FIXED_BIT & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_NAV_CORRECTION_RTK_FIXED_BIT;
    }
    if (::LOCATION_NAV_CORRECTION_PPP_BIT & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_NAV_CORRECTION_PPP_BIT;
    }
    if (::LOCATION_NAV_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_BIT &
            halLocationInfo.navSolutionMask) {
        flags |= LOCATION_NAV_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_BIT;
    }
    if (::LOCATION_NAV_MMF_AIDED_POSITION & halLocationInfo.navSolutionMask) {
        flags |= LOCATION_NAV_MMF_AIDED_POSITION;
    }
    locationInfo.navSolutionMask = (GnssLocationNavSolutionMask)flags;

    locationInfo.posTechMask = locationInfo.techMask;
    locationInfo.bodyFrameData = parseLocationPositionDynamics(
            halLocationInfo.bodyFrameData, halLocationInfo.bodyFrameDataExt);
    locationInfo.gnssSystemTime = parseSystemTime(halLocationInfo.gnssSystemTime);
    locationInfo.leapSeconds = halLocationInfo.leapSeconds;
    locationInfo.leapSecondsUnc = halLocationInfo.leapSecondsUnc;

    return locationInfo;
}

GnssSv LocationClientApiImpl::parseGnssSv(const ::GnssSv &halGnssSv) {
    GnssSv gnssSv = {};

    gnssSv.svId = halGnssSv.svId;
    switch (halGnssSv.type) {
        case ::GNSS_SV_TYPE_GPS:
            gnssSv.type = GNSS_SV_TYPE_GPS;
            break;

        case ::GNSS_SV_TYPE_SBAS:
            gnssSv.type = GNSS_SV_TYPE_SBAS;
            break;

        case ::GNSS_SV_TYPE_GLONASS:
            gnssSv.type = GNSS_SV_TYPE_GLONASS;
            if (isGloSlotUnknown(halGnssSv.svId)) { // OSN is not known, report FCN
                gnssSv.svId = halGnssSv.gloFrequency + 96;
            }
            break;

        case ::GNSS_SV_TYPE_QZSS:
            gnssSv.type = GNSS_SV_TYPE_QZSS;
            break;

        case ::GNSS_SV_TYPE_BEIDOU:
            gnssSv.type = GNSS_SV_TYPE_BEIDOU;
            break;

        case ::GNSS_SV_TYPE_GALILEO:
            gnssSv.type = GNSS_SV_TYPE_GALILEO;
            break;

        case ::GNSS_SV_TYPE_NAVIC:
            gnssSv.type = GNSS_SV_TYPE_NAVIC;
            break;

        default:
            gnssSv.type = GNSS_SV_TYPE_UNKNOWN;
            break;
    }
    gnssSv.cN0Dbhz = halGnssSv.cN0Dbhz;
    gnssSv.elevation = halGnssSv.elevation;
    gnssSv.azimuth = halGnssSv.azimuth;
    gnssSv.basebandCarrierToNoiseDbHz = halGnssSv.basebandCarrierToNoiseDbHz;

    uint32_t gnssSvOptionsMask = 0;
    if (::GNSS_SV_OPTIONS_HAS_EPHEMER_BIT & halGnssSv.gnssSvOptionsMask) {
        gnssSvOptionsMask |= GNSS_SV_OPTIONS_HAS_EPHEMER_BIT;
    }
    if (::GNSS_SV_OPTIONS_HAS_ALMANAC_BIT & halGnssSv.gnssSvOptionsMask) {
        gnssSvOptionsMask |= GNSS_SV_OPTIONS_HAS_ALMANAC_BIT;
    }
    if (::GNSS_SV_OPTIONS_USED_IN_FIX_BIT & halGnssSv.gnssSvOptionsMask) {
        gnssSvOptionsMask |= GNSS_SV_OPTIONS_USED_IN_FIX_BIT;
    }
    if (::GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT & halGnssSv.gnssSvOptionsMask) {
        gnssSvOptionsMask |= GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT;
    }
    if (::GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT & halGnssSv.gnssSvOptionsMask) {
        gnssSvOptionsMask |= GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT;
    }
    if (::GNSS_SV_OPTIONS_HAS_BASEBAND_CARRIER_TO_NOISE_BIT & halGnssSv.gnssSvOptionsMask) {
        gnssSvOptionsMask |= GNSS_SV_OPTIONS_HAS_BASEBAND_CARRIER_TO_NOISE_BIT;
    }
    if (::GNSS_SV_OPTIONS_HAS_ELEVATION_BIT & halGnssSv.gnssSvOptionsMask) {
        gnssSvOptionsMask |= GNSS_SV_OPTIONS_HAS_ELEVATION_BIT;
    }
    if (::GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT & halGnssSv.gnssSvOptionsMask) {
        gnssSvOptionsMask |= GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT;
    }
    gnssSv.gnssSvOptionsMask = (GnssSvOptionsMask)gnssSvOptionsMask;

    gnssSv.carrierFrequencyHz = halGnssSv.carrierFrequencyHz;
    gnssSv.gnssSignalTypeMask = parseGnssSignalType(halGnssSv.gnssSignalTypeMask);
    gnssSv.gloFrequency = halGnssSv.gloFrequency;
    gnssSv.basebandCarrierToNoiseDbHz = halGnssSv.basebandCarrierToNoiseDbHz;

    return gnssSv;
}

GnssData LocationClientApiImpl::parseGnssData(const ::GnssDataNotification &halGnssData) {

    GnssData gnssData = {};

    for (int sig = GNSS_LOC_SIGNAL_TYPE_GPS_L1CA;
         sig < GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES; sig++) {
        gnssData.gnssDataMask[sig] =
                (location_client::GnssDataMask) halGnssData.gnssDataMask[sig];
        gnssData.jammerInd[sig] = halGnssData.jammerInd[sig];
        gnssData.agc[sig] = halGnssData.agc[sig];
        if (0 != gnssData.gnssDataMask[sig]) {
            LOC_LOGv("gnssDataMask[%d]=0x%X", sig, gnssData.gnssDataMask[sig]);
            LOC_LOGv("jammerInd[%d]=%f", sig, gnssData.jammerInd[sig]);
            LOC_LOGv("agc[%d]=%f", sig, gnssData.agc[sig]);
        }
    }
    gnssData.agcStatusL1 = parseAgcStatus(halGnssData.agcStatusL1);
    gnssData.agcStatusL2 = parseAgcStatus(halGnssData.agcStatusL2);
    gnssData.agcStatusL5 = parseAgcStatus(halGnssData.agcStatusL5);
    return gnssData;
}

GnssMeasurements LocationClientApiImpl::parseGnssMeasurements(
        const ::GnssMeasurementsNotification &halGnssMeasurements) {
    GnssMeasurements gnssMeasurements = {};

    for (int meas = 0; meas < halGnssMeasurements.count; meas++) {
        GnssMeasurementsData measurement;

        measurement.flags =
                parseMeasurementsDataMask(halGnssMeasurements.measurements[meas].flags);
        measurement.svId = halGnssMeasurements.measurements[meas].svId;
        measurement.svType =
                (location_client::GnssSvType)halGnssMeasurements.measurements[meas].svType;
        if ((GNSS_SV_TYPE_GLONASS == measurement.svType) && isGloSlotUnknown(measurement.svId)) {
            // OSN is not known, report FCN
            measurement.svId = halGnssMeasurements.measurements[meas].gloFrequency + 96;
        }
        measurement.timeOffsetNs = halGnssMeasurements.measurements[meas].timeOffsetNs;
        measurement.stateMask = (GnssMeasurementsStateMask)
                halGnssMeasurements.measurements[meas].stateMask;
        measurement.receivedSvTimeNs = halGnssMeasurements.measurements[meas].receivedSvTimeNs;
        measurement.receivedSvTimeSubNs =
                halGnssMeasurements.measurements[meas].receivedSvTimeSubNs;
        measurement.receivedSvTimeUncertaintyNs =
                halGnssMeasurements.measurements[meas].receivedSvTimeUncertaintyNs;
        measurement.carrierToNoiseDbHz =
                halGnssMeasurements.measurements[meas].carrierToNoiseDbHz;
        measurement.pseudorangeRateMps =
                halGnssMeasurements.measurements[meas].pseudorangeRateMps;
        measurement.pseudorangeRateUncertaintyMps =
                halGnssMeasurements.measurements[meas].pseudorangeRateUncertaintyMps;
        measurement.adrStateMask = (GnssMeasurementsAdrStateMask)
                halGnssMeasurements.measurements[meas].adrStateMask;
        measurement.adrMeters = halGnssMeasurements.measurements[meas].adrMeters;
        measurement.adrUncertaintyMeters =
                halGnssMeasurements.measurements[meas].adrUncertaintyMeters;
        measurement.carrierFrequencyHz =
                halGnssMeasurements.measurements[meas].carrierFrequencyHz;
        measurement.carrierCycles = halGnssMeasurements.measurements[meas].carrierCycles;
        measurement.carrierPhase = halGnssMeasurements.measurements[meas].carrierPhase;
        measurement.carrierPhaseUncertainty =
                halGnssMeasurements.measurements[meas].carrierPhaseUncertainty;
        measurement.multipathIndicator = (location_client::GnssMeasurementsMultipathIndicator)
                halGnssMeasurements.measurements[meas].multipathIndicator;
        measurement.signalToNoiseRatioDb =
                halGnssMeasurements.measurements[meas].signalToNoiseRatioDb;
        measurement.agcLevelDb = halGnssMeasurements.measurements[meas].agcLevelDb;
        measurement.basebandCarrierToNoiseDbHz =
                halGnssMeasurements.measurements[meas].basebandCarrierToNoiseDbHz;
        measurement.gnssSignalType =
                parseGnssSignalType(halGnssMeasurements.measurements[meas].gnssSignalType);
        measurement.fullInterSignalBiasNs =
                halGnssMeasurements.measurements[meas].fullInterSignalBiasNs;
        measurement.fullInterSignalBiasUncertaintyNs =
                halGnssMeasurements.measurements[meas].fullInterSignalBiasUncertaintyNs;
        measurement.cycleSlipCount = halGnssMeasurements.measurements[meas].cycleSlipCount;

        measurement.basebandCarrierToNoiseDbHz =
               halGnssMeasurements.measurements[meas].basebandCarrierToNoiseDbHz;
        measurement.fullInterSignalBiasNs =
               halGnssMeasurements.measurements[meas].fullInterSignalBiasNs;
        measurement.fullInterSignalBiasUncertaintyNs =
               halGnssMeasurements.measurements[meas].fullInterSignalBiasUncertaintyNs;

        gnssMeasurements.measurements.push_back(measurement);
    }
    gnssMeasurements.clock.flags =
            (GnssMeasurementsClockFlagsMask) halGnssMeasurements.clock.flags;
    gnssMeasurements.clock.leapSecond = halGnssMeasurements.clock.leapSecond;
    gnssMeasurements.clock.timeNs = halGnssMeasurements.clock.timeNs;
    gnssMeasurements.clock.timeUncertaintyNs = halGnssMeasurements.clock.timeUncertaintyNs;
    gnssMeasurements.clock.fullBiasNs = halGnssMeasurements.clock.fullBiasNs;
    gnssMeasurements.clock.biasNs = halGnssMeasurements.clock.biasNs;
    gnssMeasurements.clock.biasUncertaintyNs = halGnssMeasurements.clock.biasUncertaintyNs;
    gnssMeasurements.clock.driftNsps = halGnssMeasurements.clock.driftNsps;
    gnssMeasurements.clock.driftUncertaintyNsps = halGnssMeasurements.clock.driftUncertaintyNsps;
    gnssMeasurements.clock.hwClockDiscontinuityCount =
            halGnssMeasurements.clock.hwClockDiscontinuityCount;
    gnssMeasurements.clock.elapsedRealTime = halGnssMeasurements.clock.elapsedRealTime;
    gnssMeasurements.clock.elapsedRealTimeUnc = halGnssMeasurements.clock.elapsedRealTimeUnc;
    gnssMeasurements.clock.elapsedgPTPTime = halGnssMeasurements.clock.elapsedgPTPTime;
    gnssMeasurements.clock.elapsedgPTPTimeUnc = halGnssMeasurements.clock.elapsedgPTPTimeUnc;
    gnssMeasurements.isNhz = halGnssMeasurements.isNhz;
    gnssMeasurements.agcStatusL1 = parseAgcStatus(halGnssMeasurements.agcStatusL1);
    gnssMeasurements.agcStatusL2 = parseAgcStatus(halGnssMeasurements.agcStatusL2);
    gnssMeasurements.agcStatusL5 = parseAgcStatus(halGnssMeasurements.agcStatusL5);

    return gnssMeasurements;
}

LocationResponse LocationClientApiImpl::parseLocationError(::LocationError error) {
    LocationResponse response;

    switch (error) {
        case ::LOCATION_ERROR_SUCCESS:
            response = LOCATION_RESPONSE_SUCCESS;
            break;
        case ::LOCATION_ERROR_NOT_SUPPORTED:
            response = LOCATION_RESPONSE_NOT_SUPPORTED;
            break;
        case ::LOCATION_ERROR_TIMEOUT:
            response = LOCATION_RESPONSE_TIMEOUT;
            break;
        case ::LOCATION_ERROR_ALREADY_STARTED:
            response = LOCATION_RESPONSE_REQUEST_ALREADY_IN_PROGRESS;
            break;
        case LOCATION_ERROR_SYSTEM_NOT_READY:
            response = LOCATION_RESPONSE_SYSTEM_NOT_READY;
            break;
        case LOCATION_ERROR_EXCLUSIVE_SESSION_IN_PROGRESS:
            response = LOCATION_RESPONSE_EXCLUSIVE_SESSION_IN_PROGRESS;
            break;
        case LOCATION_ERROR_INVALID_PARAMETER:
            response = LOCATION_RESPONSE_PARAM_INVALID;
            break;
        default:
            response = LOCATION_RESPONSE_UNKOWN_FAILURE;
            break;
    }

    return response;
}

LocationSystemInfo LocationClientApiImpl::parseLocationSystemInfo(
        const::LocationSystemInfo &halSystemInfo) {
    LocationSystemInfo systemInfo = {};

    systemInfo.systemInfoMask = (location_client::LocationSystemInfoMask)
            halSystemInfo.systemInfoMask;
    if (halSystemInfo.systemInfoMask & ::LOCATION_SYS_INFO_LEAP_SECOND) {
        systemInfo.leapSecondSysInfo.leapSecondInfoMask = (location_client::LeapSecondSysInfoMask)
                halSystemInfo.leapSecondSysInfo.leapSecondInfoMask;

        if (halSystemInfo.leapSecondSysInfo.leapSecondInfoMask &
                ::LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT) {
            LeapSecondChangeInfo &clientInfo =
                    systemInfo.leapSecondSysInfo.leapSecondChangeInfo;
            const::LeapSecondChangeInfo &halInfo =
                    halSystemInfo.leapSecondSysInfo.leapSecondChangeInfo;

            clientInfo.gpsTimestampLsChange = parseGnssTime(halInfo.gpsTimestampLsChange);
            clientInfo.leapSecondsBeforeChange = halInfo.leapSecondsBeforeChange;
            clientInfo.leapSecondsAfterChange = halInfo.leapSecondsAfterChange;
        }

        if (halSystemInfo.leapSecondSysInfo.leapSecondInfoMask &
            ::LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT) {
            systemInfo.leapSecondSysInfo.leapSecondCurrent =
                    halSystemInfo.leapSecondSysInfo.leapSecondCurrent;
        }
    }

    return systemInfo;
}

GnssEnergyConsumedInfo LocationClientApiImpl::parseGnssConsumedInfo(::GnssEnergyConsumedInfo in) {
    GnssEnergyConsumedInfo energyConsumed = {};
    uint32_t energyConsumedMask = 0;

    if (::ENERGY_CONSUMED_SINCE_FIRST_BOOT_BIT & in.flags) {
        energyConsumedMask |= ENERGY_CONSUMED_SINCE_FIRST_BOOT_BIT;
    }

    energyConsumed.flags = (GnssEnergyConsumedInfoMask)energyConsumedMask;
    energyConsumed.totalEnergyConsumedSinceFirstBoot = in.totalEnergyConsumedSinceFirstBoot;

    return energyConsumed;
}

GnssDcReport LocationClientApiImpl::parseDcReport(const::GnssDcReportInfo &halDcReport) {
    GnssDcReport dcReport = {};
    switch (halDcReport.dcReportType) {
        case ::QZSS_JMA_DISASTER_PREVENTION_INFO:
            dcReport.dcReportType = QZSS_JMA_DISASTER_PREVENTION_INFO;
            break;
        case ::QZSS_NON_JMA_DISASTER_PREVENTION_INFO:
            dcReport.dcReportType = QZSS_NON_JMA_DISASTER_PREVENTION_INFO;
            break;
        default:
            break;
    }
    dcReport.numValidBits = halDcReport.numValidBits;
    dcReport.dcReportData = std::move(halDcReport.dcReportData);
    return dcReport;
}

GeofenceBreachTypeMask LocationClientApiImpl::parseGeofenceBreachType(
        GeofenceBreachType breachType) {
    int mask = 0;
    switch (breachType) {
        case GEOFENCE_BREACH_ENTER:
            mask |= GEOFENCE_BREACH_ENTER_BIT;
            break;
        case GEOFENCE_BREACH_EXIT:
            mask |= GEOFENCE_BREACH_EXIT_BIT;
            break;
        case GEOFENCE_BREACH_DWELL_IN:
            mask |= GEOFENCE_BREACH_DWELL_IN_BIT;
            break;
        case GEOFENCE_BREACH_DWELL_OUT:
            mask |= GEOFENCE_BREACH_DWELL_OUT_BIT;
            break;
    }
    return (GeofenceBreachTypeMask)mask;
}

GeofenceBreachType LocationClientApiImpl::parseGeofenceBreachTypeMask(
        ::GeofenceBreachTypeMask breachTypeMask) {
    GeofenceBreachType breachType = (GeofenceBreachType)0;
    switch (breachTypeMask) {
        case ::GEOFENCE_BREACH_ENTER_BIT:
            breachType = GEOFENCE_BREACH_ENTER;
            break;
        case ::GEOFENCE_BREACH_EXIT_BIT:
            breachType = GEOFENCE_BREACH_EXIT;
            break;
        case ::GEOFENCE_BREACH_DWELL_IN_BIT:
            breachType = GEOFENCE_BREACH_DWELL_IN;
            break;
        case ::GEOFENCE_BREACH_DWELL_OUT_BIT:
            breachType = GEOFENCE_BREACH_DWELL_OUT;
            break;
    }
    return breachType;
}

void LocationClientApiImpl::logLocation(const Location &location,
                                        LocReportTriggerType reportTriggerType) {
    GnssLocation gnssLocation = {};
    gnssLocation.flags              = location.flags;
    gnssLocation.timestamp          = location.timestamp;
    gnssLocation.latitude           = location.latitude;
    gnssLocation.longitude          = location.longitude;
    gnssLocation.altitude           = location.altitude;
    gnssLocation.speed              = location.speed;
    gnssLocation.bearing            = location.bearing;
    gnssLocation.horizontalAccuracy = location.horizontalAccuracy;
    gnssLocation.verticalAccuracy   = location.verticalAccuracy;
    gnssLocation.speedAccuracy      = location.speedAccuracy;
    gnssLocation.bearingAccuracy    = location.bearingAccuracy;
    gnssLocation.techMask           = location.techMask;

    mLogger.log(gnssLocation,
                {mCapsMask, mSessionStartBootTimestampNs, reportTriggerType});
}

void LocationClientApiImpl::logLocation(const GnssLocation &gnssLocation,
                                        LocReportTriggerType reportTriggerType) {
    mLogger.log(gnssLocation,
                {mCapsMask, mSessionStartBootTimestampNs, reportTriggerType});
}

void LocationClientApiImpl::logGeofenceBreach(const GeofenceBreachNotification& breachNotif,
            const std::vector<Geofence> &geofences) {
    mLogger.log(breachNotif, geofences);
}

void LocationClientApiImpl::parseEphSrcAndAction(const ::GnssEphAction& halEphAction,
    GnssEphSource& ephSrc, GnssEphAction& ephAction) {

    ephSrc    = GNSS_EPH_SRC_UNKNOWN;
    ephAction = GNSS_EPH_ACTION_UNKNOWN;

    switch (halEphAction) {
        case GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02:
            ephSrc    = GNSS_EPH_SRC_UNKNOWN;
            ephAction = GNSS_EPH_ACTION_UNKNOWN;
            break;

        case GNSS_EPH_ACTION_UPDATE_SRC_OTA_V02:
            ephSrc    = GNSS_EPH_SRC_OTA;
            ephAction = GNSS_EPH_ACTION_UPDATE;
            break;

        case GNSS_EPH_ACTION_UPDATE_SRC_NETWORK_V02:
            ephSrc    = GNSS_EPH_SRC_UNKNOWN;
            ephAction = GNSS_EPH_ACTION_UNKNOWN;
            break;

        case GNSS_EPH_ACTION_UPDATE_MAX_V02:
            ephSrc    = GNSS_EPH_SRC_MAX;
            ephAction = GNSS_EPH_ACTION_MAX;
            break;

        case GNSS_EPH_ACTION_DELETE_SRC_UNKNOWN_V02:
            ephSrc    = GNSS_EPH_SRC_UNKNOWN;
            ephAction = GNSS_EPH_ACTION_UNKNOWN;
            break;

        case GNSS_EPH_ACTION_DELETE_SRC_NETWORK_V02:
            ephSrc    = GNSS_EPH_SRC_UNKNOWN;
            ephAction = GNSS_EPH_ACTION_UNKNOWN;
            break;

        case GNSS_EPH_ACTION_DELETE_SRC_OTA_V02:
            ephSrc    = GNSS_EPH_SRC_OTA;
            ephAction = GNSS_EPH_ACTION_DELETE;
            break;

        case GNSS_EPH_ACTION_DELETE_MAX_V02:
            ephSrc    = GNSS_EPH_SRC_UNKNOWN;
            ephAction = GNSS_EPH_ACTION_MAX;
            break;

        default:
            LOC_LOGe(" Source and Action on Ephemeris cannot be identified ");
            break;
    }
}

void LocationClientApiImpl::parseCommanGnssEphemeris(const ::GnssEphCommon& halCommanEph,
            GnssEphCommonInfo& lcaCommanEph)
{

    lcaCommanEph.gnssSvId     = halCommanEph.gnssSvId;
    parseEphSrcAndAction(halCommanEph.updateAction, lcaCommanEph.ephSource, lcaCommanEph.action);
    lcaCommanEph.IODE         = halCommanEph.IODE;
    lcaCommanEph.aSqrt        = halCommanEph.aSqrt;
    lcaCommanEph.deltaN       = halCommanEph.deltaN;
    lcaCommanEph.m0           = halCommanEph.m0;
    lcaCommanEph.eccentricity = halCommanEph.eccentricity;
    lcaCommanEph.omega0       = halCommanEph.omega0;
    lcaCommanEph.i0           = halCommanEph.i0;
    lcaCommanEph.omega        = halCommanEph.omega;
    lcaCommanEph.omegaDot     = halCommanEph.omegaDot;
    lcaCommanEph.iDot         = halCommanEph.iDot;
    lcaCommanEph.cUc          = halCommanEph.cUc;
    lcaCommanEph.cUs          = halCommanEph.cUs;
    lcaCommanEph.cRc          = halCommanEph.cRc;
    lcaCommanEph.cRs          = halCommanEph.cRs;
    lcaCommanEph.cIc          = halCommanEph.cIc;
    lcaCommanEph.cIs          = halCommanEph.cIs;
    lcaCommanEph.toe          = halCommanEph.toe;
    lcaCommanEph.toc          = halCommanEph.toc;
    lcaCommanEph.af0          = halCommanEph.af0;
    lcaCommanEph.af1          = halCommanEph.af1;
    lcaCommanEph.af2          = halCommanEph.af2;
}

void LocationClientApiImpl::parseGpsEphemeris(const GpsEphemerisResponse& halEph,
            std::vector<GpsQzssEphemeris>& lcaEphInfo) {

    for (int idx = 0; idx < halEph.numOfEphemeris; idx++) {
        GpsQzssEphemeris lcaEph = {};
        parseCommanGnssEphemeris(halEph.gpsEphemerisData[idx].commonEphemerisData,
                lcaEph.commonEphemerisData);

        lcaEph.signalHealth  = halEph.gpsEphemerisData[idx].signalHealth;
        lcaEph.URAI          = halEph.gpsEphemerisData[idx].URAI;
        lcaEph.codeL2        = halEph.gpsEphemerisData[idx].codeL2;
        lcaEph.dataFlagL2P   = halEph.gpsEphemerisData[idx].dataFlagL2P;
        lcaEph.fitInterval   = halEph.gpsEphemerisData[idx].fitInterval;
        lcaEph.IODC          = halEph.gpsEphemerisData[idx].IODC;
        lcaEph.tgd           = halEph.gpsEphemerisData[idx].tgd;
        lcaEphInfo.push_back(std::move(lcaEph));
    }
}

void LocationClientApiImpl::parseGalEphemeris(const GalileoEphemerisResponse& halEph,
            std::vector<GalileoEphemeris>& lcaEphInfo) {

    for (int idx = 0; idx < halEph.numOfEphemeris; idx++) {
        GalileoEphemeris lcaEph = {};
        parseCommanGnssEphemeris(halEph.galEphemerisData[idx].commonEphemerisData,
                lcaEph.commonEphemerisData);

        lcaEph.dataSourceSignal =
                (GalEphSignalSource)halEph.galEphemerisData[idx].dataSourceSignal;
        lcaEph.sisIndex = halEph.galEphemerisData[idx].sisIndex;
        lcaEph.bgdE1E5a = halEph.galEphemerisData[idx].bgdE1E5a;
        lcaEph.bgdE1E5b = halEph.galEphemerisData[idx].bgdE1E5b;
        lcaEph.svHealth = halEph.galEphemerisData[idx].svHealth;
        lcaEphInfo.push_back(std::move(lcaEph));
    }

}

void LocationClientApiImpl::parseGloEphemeris(const GlonassEphemerisResponse& halEph,
            std::vector<GlonassEphemeris>& lcaEphInfo) {

    for (int idx = 0; idx < halEph.numOfEphemeris; idx++) {
        GlonassEphemeris lcaEph = {};
        lcaEph.gnssSvId     = halEph.gloEphemerisData[idx].gnssSvId;
        parseEphSrcAndAction(halEph.gloEphemerisData[idx].updateAction,
                lcaEph.ephSource, lcaEph.action);
        lcaEph.bnHealth     = halEph.gloEphemerisData[idx].bnHealth;
        lcaEph.lnHealth     = halEph.gloEphemerisData[idx].lnHealth;
        lcaEph.tb           = halEph.gloEphemerisData[idx].tb;
        lcaEph.ft           = halEph.gloEphemerisData[idx].ft;
        lcaEph.gloM         = halEph.gloEphemerisData[idx].gloM;
        lcaEph.enAge        = halEph.gloEphemerisData[idx].enAge;
        lcaEph.gloFrequency = halEph.gloEphemerisData[idx].gloFrequency;
        lcaEph.p1           = halEph.gloEphemerisData[idx].p1;
        lcaEph.p2           = halEph.gloEphemerisData[idx].p2;
        lcaEph.deltaTau     = halEph.gloEphemerisData[idx].deltaTau;
        lcaEph.position[0]  = halEph.gloEphemerisData[idx].position[0];
        lcaEph.position[1]  = halEph.gloEphemerisData[idx].position[1];
        lcaEph.position[2]  = halEph.gloEphemerisData[idx].position[2];
        lcaEph.velocity[0]  = halEph.gloEphemerisData[idx].velocity[0];
        lcaEph.velocity[1]  = halEph.gloEphemerisData[idx].velocity[1];
        lcaEph.velocity[2]  = halEph.gloEphemerisData[idx].velocity[2];
        lcaEph.acceleration[0] = halEph.gloEphemerisData[idx].acceleration[0];
        lcaEph.acceleration[1] = halEph.gloEphemerisData[idx].acceleration[1];
        lcaEph.acceleration[2] = halEph.gloEphemerisData[idx].acceleration[2];
        lcaEph.tauN         = halEph.gloEphemerisData[idx].tauN;
        lcaEph.gamma        = halEph.gloEphemerisData[idx].gamma;
        lcaEph.toe          = halEph.gloEphemerisData[idx].toe;
        lcaEph.nt           = halEph.gloEphemerisData[idx].nt;
        lcaEphInfo.push_back(std::move(lcaEph));
    }
}

void LocationClientApiImpl::parseBdsEphemeris(const BdsEphemerisResponse& halEph,
            std::vector<BdsEphemeris>& lcaEphInfo) {

    for (int idx = 0; idx < halEph.numOfEphemeris; idx++) {
        BdsEphemeris lcaEph = {};
        parseCommanGnssEphemeris(halEph.bdsEphemerisData[idx].commonEphemerisData,
                lcaEph.commonEphemerisData);

        lcaEph.svHealth = halEph.bdsEphemerisData[idx].svHealth;
        lcaEph.AODC     = halEph.bdsEphemerisData[idx].AODC;
        lcaEph.tgd1     = halEph.bdsEphemerisData[idx].tgd1;
        lcaEph.tgd2     = halEph.bdsEphemerisData[idx].tgd2;
        lcaEph.URAI     = halEph.bdsEphemerisData[idx].URAI;
        lcaEphInfo.push_back(std::move(lcaEph));
    }
}

void LocationClientApiImpl::parseQzssEphemeris(const QzssEphemerisResponse& halEph,
                std::vector<QzssEphemeris>& lcaEphInfo) {

    for (int idx = 0; idx < halEph.numOfEphemeris; idx++) {
        QzssEphemeris lcaEph = {};
        parseCommanGnssEphemeris(halEph.qzssEphemerisData[idx].commonEphemerisData,
                lcaEph.qzssEphData.commonEphemerisData);

        lcaEph.qzssEphData.signalHealth  = halEph.qzssEphemerisData[idx].signalHealth;
        lcaEph.qzssEphData.URAI          = halEph.qzssEphemerisData[idx].URAI;
        lcaEph.qzssEphData.codeL2        = halEph.qzssEphemerisData[idx].codeL2;
        lcaEph.qzssEphData.dataFlagL2P   = halEph.qzssEphemerisData[idx].dataFlagL2P;
        lcaEph.qzssEphData.fitInterval   = halEph.qzssEphemerisData[idx].fitInterval;
        lcaEph.qzssEphData.IODC          = halEph.qzssEphemerisData[idx].IODC;
        lcaEph.qzssEphData.tgd           = halEph.qzssEphemerisData[idx].tgd;
        lcaEphInfo.push_back(std::move(lcaEph));
    }
}

void LocationClientApiImpl::parseNavicEphemeris(const NavicEphemerisResponse& halEph,
            std::vector<NavicEphemeris>& lcaEphInfo) {

    for (int idx = 0; idx < halEph.numOfEphemeris; idx++) {
        NavicEphemeris lcaEph = {};
        parseCommanGnssEphemeris(halEph.navicEphemerisData[idx].commonEphemerisData,
                lcaEph.commonEphemerisData);

        lcaEph.weekNum  = halEph.navicEphemerisData[idx].weekNum;
        lcaEph.iodec    = halEph.navicEphemerisData[idx].iodec;
        lcaEph.l5Health = halEph.navicEphemerisData[idx].l5Health;
        lcaEph.sHealth  = halEph.navicEphemerisData[idx].sHealth;
        lcaEph.inclinationAngleRad =
                halEph.navicEphemerisData[idx].inclinationAngleRad;
        lcaEph.urai     = halEph.navicEphemerisData[idx].urai;
        lcaEph.tgd      = halEph.navicEphemerisData[idx].tgd;
        lcaEphInfo.push_back(std::move(lcaEph));
    }
}
GnssEphemeris LocationClientApiImpl::parseGnssEphemerisInfo(
         const ::GnssSvEphemerisReport &halGnssEphemeris) {
    GnssEphemeris gnssEphInfo = {};
    switch (halGnssEphemeris.gnssConstellation) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            gnssEphInfo.gnssConstellation = GNSS_LOC_SV_SYSTEM_GPS;
            parseGpsEphemeris(halGnssEphemeris.ephInfo.gpsEphemeris,
                    gnssEphInfo.gpsEphemerisData);
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            gnssEphInfo.gnssConstellation = GNSS_LOC_SV_SYSTEM_GALILEO;
            parseGalEphemeris(halGnssEphemeris.ephInfo.galileoEphemeris,
                    gnssEphInfo.galEphemerisData);
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            gnssEphInfo.gnssConstellation = GNSS_LOC_SV_SYSTEM_GLONASS;
            parseGloEphemeris(halGnssEphemeris.ephInfo.glonassEphemeris,
                    gnssEphInfo.gloEphemerisData);
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            gnssEphInfo.gnssConstellation = GNSS_LOC_SV_SYSTEM_BDS;
            parseBdsEphemeris(halGnssEphemeris.ephInfo.bdsEphemeris,
                    gnssEphInfo.bdsEphemerisData);
            break;
        case GNSS_LOC_SV_SYSTEM_QZSS:
            gnssEphInfo.gnssConstellation = GNSS_LOC_SV_SYSTEM_QZSS;
            parseQzssEphemeris(halGnssEphemeris.ephInfo.qzssEphemeris,
                    gnssEphInfo.qzssEphemerisData);
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            gnssEphInfo.gnssConstellation = GNSS_LOC_SV_SYSTEM_NAVIC;
            parseNavicEphemeris(halGnssEphemeris.ephInfo.navicEphemeris,
                    gnssEphInfo.navicEphemerisData);
            break;
        default:
            LOC_LOGe("Unknown System Type for Ephemeris ");
            break;
   }
   if (halGnssEphemeris.isSystemTimeValid) {
       gnssEphInfo.isSystemTimeValid = 1;
       gnssEphInfo.systemTime = parseGnssTime(halGnssEphemeris.systemTime);
   }
   return gnssEphInfo;
}

/******************************************************************************
ILocIpcListener override
******************************************************************************/
class IpcListener : public ILocIpcListener {
    MsgTask& mMsgTask;
    LocationClientApiImpl& mApiImpl;
    const SockNode::Type mSockTpye;
public:
    inline IpcListener(LocationClientApiImpl& apiImpl, MsgTask& msgTask,
                       const SockNode::Type sockType) :
            mMsgTask(msgTask), mApiImpl(apiImpl), mSockTpye(sockType) {}
    virtual void onListenerReady() override;
    virtual void onReceive(const char* data, uint32_t length,
                           const LocIpcRecver* recver) override;
};

/******************************************************************************
LocIpcQrtrWatcher override
******************************************************************************/
class HalDaemonQrtrWatcher : public LocIpcQrtrWatcher {
    const weak_ptr<IpcListener> mIpcListener;
    const weak_ptr<LocIpcSender> mIpcSender;
    LocIpcQrtrWatcher::ServiceStatus mKnownStatus;
    LocationApiPbMsgConv mPbufMsgConv;
    MsgTask& mMsgTask;

public:
    inline HalDaemonQrtrWatcher(shared_ptr<IpcListener>& listener, shared_ptr<LocIpcSender>& sender,
                          LocationApiPbMsgConv& pbMsgConv, MsgTask& msgTask) :
            LocIpcQrtrWatcher({LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID}),
            mIpcListener(listener), mIpcSender(sender), mPbufMsgConv(pbMsgConv),
            mMsgTask(msgTask), mKnownStatus(LocIpcQrtrWatcher::ServiceStatus::DOWN) {
    }
    inline virtual void onServiceStatusChange(int serviceId, int instanceId,
                                              LocIpcQrtrWatcher::ServiceStatus status,
                                              uint32_t nodeId, uint32_t portId) {

        struct onHalServiceStatusChangeHandler : public LocMsg {
            onHalServiceStatusChangeHandler(HalDaemonQrtrWatcher& watcher,
                                            LocIpcQrtrWatcher::ServiceStatus status,
                                            uint32_t nodeId, uint32_t portId) :
                mWatcher(watcher), mStatus(status), mNodeId(nodeId), mPortId(portId) {}

            virtual ~onHalServiceStatusChangeHandler() {}
            void proc() const {
                LOC_LOGi("LocIpcQrtrWatcher:: HAL Daemon service status %d", (int)mStatus);
                if (LocIpcQrtrWatcher::ServiceStatus::UP == mStatus) {
                    auto sender = mWatcher.mIpcSender.lock();
                    if (nullptr != sender && sender->updateDestAddr(mNodeId, mPortId)) {
                        usleep(gSleepTime);
                        auto listener = mWatcher.mIpcListener.lock();
                        if (nullptr != listener) {
                            LocAPIHalReadyIndMsg msg(SERVICE_NAME, &mWatcher.mPbufMsgConv);
                            string pbStr;
                            if (msg.serializeToProtobuf(pbStr)) {
                                listener->onReceive(pbStr.c_str(), pbStr.size(), nullptr);
                            } else {
                                LOC_LOGe("LocAPIHalReadyIndMsg serializeToProtobuf failed");
                            }
                        }
                    }
                }
                mWatcher.mKnownStatus = mStatus;
            }

            HalDaemonQrtrWatcher& mWatcher;
            LocIpcQrtrWatcher::ServiceStatus mStatus;
            uint32_t mNodeId;
            uint32_t mPortId;
        };

        if (LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID == serviceId &&
            LOCATION_CLIENT_API_QSOCKET_HALDAEMON_INSTANCE_ID == instanceId) {
            mMsgTask.sendMsg(new (nothrow)
                     onHalServiceStatusChangeHandler(*this, status, nodeId, portId));
        }
    }
};

/******************************************************************************
LocationClientApiImpl
******************************************************************************/

uint32_t LocationClientApiImpl::mClientIdGenerator = LOCATION_CLIENT_SESSION_ID_INVALID;
uint32_t LocationClientApiImpl::mClientIdIndex = 0;
mutex LocationClientApiImpl::mMutex;

/******************************************************************************
LocationClientApiImpl - constructors
******************************************************************************/
LocationClientApiImpl::LocationClientApiImpl(capabilitiesCallback capabilitiescb) :
        mSessionId(LOCATION_CLIENT_SESSION_ID_INVALID),
        mBatchingId(LOCATION_CLIENT_SESSION_ID_INVALID),
        mHalRegistered(false),
        mCallbacksMask(0),
        mCapsMask((LocationCapabilitiesMask)0),
        mYearOfHw(0),
        mSessionStartBootTimestampNs(0),
        mLastAddedClientIds({}),
        mCapabilitiesCb(capabilitiescb),
        mPositionSessionResponseCbPending(false),
        mGnssEnergyConsumedInfoCb(nullptr),
        mGnssEnergyConsumedResponseCb(nullptr),
        mLocationSysInfoCb(nullptr),
        mLocationSysInfoResponseCb(nullptr),
        mSingleTerrestrialPosCb(nullptr),
        mSingleTerrestrialPosRespCb(nullptr),
        mSinglePosCb(nullptr),
        mSinglePosRespCb(nullptr),
        mPingTestCb(nullptr),
        mMsgTask("LcaMsgTask"),
        mLogger(),
        mpAntennaInfoCb(nullptr)
{
    // read configuration file
    UTIL_READ_CONF(LOC_PATH_GPS_CONF, gConfigTable);
    LOC_LOGd("gDebug=%u", gDebug);

    // get pid to generate sokect name
    uint32_t pid = (uint32_t)getpid();

#ifdef FEATURE_EXTERNAL_AP
    // The instance id is composed from pid and client id.
    // We support up to 32 unique client api within one process.
    // Each client id is tracked via a bit in mClientIdGenerator,
    // which is 4 bytes now.
    lock_guard<mutex> lock(mMutex);
    // find a bit in the mClientIdGenerator that is not yet used
    // and use that as client id
    // client id will be from 1 to 32, as client id will be used to
    // set session id and 0 is reserved for LOCATION_CLIENT_SESSION_ID_INVALID
    mClientIdIndex++;
    if (mClientIdIndex > 32) {
        mClientIdIndex = 1;
    }

    uint32_t loopCnt = 0;
    for (; loopCnt < sizeof(mClientIdGenerator) * 8; loopCnt++) {
        if ((mClientIdGenerator & (1UL << (mClientIdIndex-1))) == 0) {
            mClientIdGenerator |= (1UL << (mClientIdIndex-1));
            mClientId = mClientIdIndex;
            break;
        }
    }

    if (loopCnt >= sizeof(mClientIdGenerator) * 8) {
        LOC_LOGe("create Qsocket failed, already use up maximum of %d clients",
                 sizeof(mClientIdGenerator)*8);
        return;
    }

    SockNodeEap sock(LOCATION_CLIENT_API_QSOCKET_CLIENT_SERVICE_ID,
                     pid * 100 + mClientId);
    size_t pathNameLength = strlcpy(mSocketName, sock.getNodePathname().c_str(),
                                    sizeof(mSocketName));
    if (pathNameLength >= sizeof(mSocketName)) {
        LOC_LOGe("socket name length exceeds limit of %" PRIu32" bytes",
                (uint32_t)sizeof(mSocketName));
        return;
    }

    // establish an ipc sender to the hal daemon
    mIpcSender = LocIpc::getLocIpcQrtrSender(LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID,
                                     LOCATION_CLIENT_API_QSOCKET_HALDAEMON_INSTANCE_ID);
    if (mIpcSender == nullptr) {
        LOC_LOGe("create sender socket failed for service id: %d instance id: %d",
                 LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID,
                 LOCATION_CLIENT_API_QSOCKET_HALDAEMON_INSTANCE_ID);
        return;
    }
    shared_ptr<IpcListener> listener(make_shared<IpcListener>(*this, mMsgTask, SockNode::Eap));
    unique_ptr<LocIpcRecver> recver = LocIpc::getLocIpcQrtrRecver(listener,
            sock.getId1(), sock.getId2(),
            make_shared<HalDaemonQrtrWatcher>(listener, mIpcSender, mPbufMsgConv, mMsgTask));
#else
    // get clientId
    lock_guard<mutex> lock(mMutex);
    mClientId = ++mClientIdGenerator;
    // make sure client ID is not equal to LOCATION_CLIENT_SESSION_ID_INVALID,
    // as session id will be assigned to client ID
    if (LOCATION_CLIENT_SESSION_ID_INVALID == mClientId) {
        mClientId = ++mClientIdGenerator;
    }

    SockNodeLocal sock(LOCATION_CLIENT_API, pid, mClientId);
    size_t pathNameLength = (size_t) strlcpy(mSocketName, sock.getNodePathname().c_str(),
                                    sizeof(mSocketName));
    if (pathNameLength >= sizeof(mSocketName)) {
        LOC_LOGe("socket name length exceeds limit of %" PRIu32 " bytes",
                (uint32_t)sizeof(mSocketName));
        return;
    }

    LOC_LOGd("create sender socket %s", mSocketName);
    locUtilWaitForDir(SOCKET_LOC_CLIENT_DIR, "gps");

    // establish an udp ipc sender to the hal daemon
    mIpcSender = LocIpc::getLocIpcLocalSender(SOCKET_TO_LOCATION_HAL_DAEMON);
    if (nullptr == mIpcSender) {
        LOC_LOGe("create sender socket failed %s", SOCKET_TO_LOCATION_HAL_DAEMON);
        return;
    }

    unique_ptr<LocIpcRecver> recver = LocIpc::getLocIpcLocalRecver(
        make_shared<IpcListener>(*this, mMsgTask, SockNode::Local), mSocketName);
#endif

    LOC_LOGd("listen on socket: %s", mSocketName);
    mIpc.startNonBlockingListening(recver);
}

LocationClientApiImpl::~LocationClientApiImpl() {
}

void LocationClientApiImpl::destroy(locationApiDestroyCompleteCallback destroyCompleteCb) {

    struct DestroyReq : public LocMsg {
        DestroyReq(LocationClientApiImpl* apiImpl,
                locationApiDestroyCompleteCallback destroyCompleteCb) :
                mApiImpl(apiImpl),
                mDestroyCompleteCb(destroyCompleteCb) {}
        virtual ~DestroyReq() {}
        void proc() const {
            // deregister
            if (mApiImpl->mHalRegistered && (nullptr != mApiImpl->mIpcSender)) {
                string pbStr;
                LocAPIClientDeregisterReqMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                    LOC_LOGd(">>> DeregisterReq rc=%d", rc);
                    mApiImpl->mIpcSender = nullptr;
                } else {
                    LOC_LOGe("LocAPIClientDeregisterReqMsg serializeToProtobuf failed");
                }
            }

#ifdef FEATURE_EXTERNAL_AP
            {
                // get clientId
                lock_guard<mutex> lock(mMutex);
                mApiImpl->mClientIdGenerator &= ~(1UL << (mApiImpl->mClientId-1));
                LOC_LOGd("client id generarator 0x%x, id %d",
                         mApiImpl->mClientIdGenerator, mApiImpl->mClientId);
            }
#endif //FEATURE_EXTERNAL_AP
            if (mDestroyCompleteCb) {
                (mDestroyCompleteCb) ();
            }
            usleep(50000); //give 50ms for socket clean up

            delete mApiImpl;
        }
        LocationClientApiImpl* mApiImpl;
        locationApiDestroyCompleteCallback mDestroyCompleteCb;
    };

    mMsgTask.sendMsg(new (nothrow) DestroyReq(this, destroyCompleteCb));
    usleep(100000); //100ms for handling onReceive() messages
}

/******************************************************************************
LocationClientApiImpl - implementation
******************************************************************************/

void LocationClientApiImpl::updateCallbacks(LocationCallbacks& callbacks) {
    struct UpdateCallbacksReq : public LocMsg {
        UpdateCallbacksReq(LocationClientApiImpl* apiImpl, const LocationCallbacks& callbacks) :
                mApiImpl(apiImpl), mCallbacks(callbacks) {}
        virtual ~UpdateCallbacksReq() {}
        void proc() const {
            mApiImpl->updateCallbacksSync(const_cast<LocationCallbacks&>(mCallbacks));
        }

        LocationClientApiImpl* mApiImpl;
        LocationCallbacks mCallbacks;
    };
    mMsgTask.sendMsg(new (nothrow) UpdateCallbacksReq(this, callbacks));
}

void LocationClientApiImpl::updateCallbacksSync(LocationCallbacks& callbacks) {
    //convert callbacks to callBacksMask
    LocationCallbacksMask callBacksMask = 0;

    if (callbacks.responseCb) {
        mLocationCbs.responseCb = callbacks.responseCb;
    }
    if (callbacks.collectiveResponseCb) {
        mLocationCbs.collectiveResponseCb = callbacks.collectiveResponseCb;
    }
    if (callbacks.trackingCb) {
        callBacksMask |= E_LOC_CB_TRACKING_BIT;
        mLocationCbs.trackingCb = callbacks.trackingCb;
    }
    if (callbacks.gnssLocationInfoCb) {
        callBacksMask |= E_LOC_CB_GNSS_LOCATION_INFO_BIT;
        mLocationCbs.gnssLocationInfoCb = callbacks.gnssLocationInfoCb;
    }
    if (callbacks.engineLocationsInfoCb) {
        callBacksMask |= E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT;
        mLocationCbs.engineLocationsInfoCb = callbacks.engineLocationsInfoCb;
    }
    if (callbacks.gnssSvCb) {
        callBacksMask |= E_LOC_CB_GNSS_SV_BIT;
        mLocationCbs.gnssSvCb = callbacks.gnssSvCb;
    }
    if (callbacks.gnssNmeaCb) {
        callBacksMask |= E_LOC_CB_GNSS_NMEA_BIT;
        mLocationCbs.gnssNmeaCb = callbacks.gnssNmeaCb;
    }
    if (callbacks.gnssDataCb) {
        callBacksMask |= E_LOC_CB_GNSS_DATA_BIT;
        mLocationCbs.gnssDataCb = callbacks.gnssDataCb;
    }
    if (callbacks.gnssMeasurementsCb) {
        callBacksMask |= E_LOC_CB_GNSS_MEAS_BIT;
        mLocationCbs.gnssMeasurementsCb = callbacks.gnssMeasurementsCb;
    }
    if (callbacks.gnssNHzMeasurementsCb) {
        callBacksMask |= E_LOC_CB_GNSS_NHZ_MEAS_BIT;
        mLocationCbs.gnssNHzMeasurementsCb = callbacks.gnssNHzMeasurementsCb;
    }
    if (callbacks.gnssDcReportCb) {
        callBacksMask |= E_LOC_CB_GNSS_DC_REPORT_BIT;
        mLocationCbs.gnssDcReportCb = callbacks.gnssDcReportCb;
    }
    if (callbacks.engineNmeaCb) {
        callBacksMask |= E_LOC_CB_ENGINE_NMEA_BIT;
        mLocationCbs.engineNmeaCb = callbacks.engineNmeaCb;
    }
    // handle callbacks that are not related to a fix session
    if (mLocationSysInfoCb) {
        callBacksMask |= E_LOC_CB_SYSTEM_INFO_BIT;
    }
    if (callbacks.batchingCb) {
        callBacksMask |= E_LOC_CB_BATCHING_BIT;
        mLocationCbs.batchingCb = callbacks.batchingCb;
    }
    if (callbacks.batchingStatusCb) {
        callBacksMask |= E_LOC_CB_BATCHING_STATUS_BIT;
        mLocationCbs.batchingStatusCb = callbacks.batchingStatusCb;
    }
    if (callbacks.geofenceBreachCb) {
        callBacksMask |= E_LOC_CB_GEOFENCE_BREACH_BIT;
        mLocationCbs.geofenceBreachCb = callbacks.geofenceBreachCb;
    }
    if (callbacks.svEphemerisCb) {
        callBacksMask |= E_LOC_CB_GNSS_EPH_BIT;
        mLocationCbs.svEphemerisCb = callbacks.svEphemerisCb;
    }

    // Callbacks may get increamentally updated, hence OR with the existing
    // callback mask
    if ((mCallbacksMask & callBacksMask) != callBacksMask) {
        mCallbacksMask |= callBacksMask;
        if (mHalRegistered) {
            string pbStr;
            LocAPIUpdateCallbacksReqMsg msg(mSocketName, mCallbacksMask, &mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                bool rc = sendMessage(reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                                      pbStr.size());
                LOC_LOGd(">>> UpdateCallbacksReq callBacksMask=0x%x rc=%d",
                         mCallbacksMask, rc);
            } else {
                LOC_LOGe("LocAPIUpdateCallbacksReqMsg serializeToProtobuf failed");
            }
        }
    } else {
        LOC_LOGd("No updateCallbacks because same callBacksMask 0x%x", callBacksMask);
    }
}

uint32_t LocationClientApiImpl::startTracking(const TrackingOptions& option) {
    struct StartTrackingReq : public LocMsg {
        StartTrackingReq(LocationClientApiImpl* apiImpl, const TrackingOptions& option) :
                mApiImpl(apiImpl), mOptions(option) {}
        virtual ~StartTrackingReq() {}
        void proc() const {
            mApiImpl->startTrackingSync(mOptions);
        }

        LocationClientApiImpl* mApiImpl;
        TrackingOptions mOptions;
    };
    mMsgTask.sendMsg(new (nothrow) StartTrackingReq(this, option));
    return mClientId;
}

uint32_t LocationClientApiImpl::startTrackingSync(const TrackingOptions& option) {
    // check if option is updated
    bool isOptionUpdated = false;

    if ((mLocationOptions.minInterval != option.minInterval) ||
        (mLocationOptions.minDistance != option.minDistance) ||
        (mLocationOptions.locReqEngTypeMask != option.locReqEngTypeMask)) {
        isOptionUpdated = true;
    }

    if (!mHalRegistered) {
        mLocationOptions = option;
        // need to set session id so when hal is ready, the session can be resumed
        mSessionId = mClientId;
        LOC_LOGe(">>> startTracking - Not registered yet");
        return mSessionId;
    }

    if (LOCATION_CLIENT_SESSION_ID_INVALID == mSessionId) {
        mLocationOptions = option;
        //start a new tracking session
        mSessionId = mClientId;

        if ((0 != mLocationOptions.minInterval) ||
                (0 != mLocationOptions.minDistance)) {
            string pbStr;
            LocAPIStartTrackingReqMsg msg(mSocketName, mLocationOptions, &mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                bool rc = sendMessage(
                   reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                LOC_LOGd(">>> StartTrackingReq Interval=%d Distance=%d,"
                         " locReqEngTypeMask=0x%x rc=%d",
                         mLocationOptions.minInterval,
                         mLocationOptions.minDistance,
                         mLocationOptions.locReqEngTypeMask, rc);
            } else {
                LOC_LOGe("LocAPIStartTrackingReqMsg serializeToProtobuf failed");
            }
        }
    } else if (isOptionUpdated) {
        // update a tracking session, mLocationOptions
        // will be updated in updateTrackingOptionsSync
        updateTrackingOptionsSync(option, true);
    } else {
        LOC_LOGd(">>> StartTrackingReq - no change in option");
        invokePositionSessionResponseCb(LOCATION_ERROR_SUCCESS);
    }
    return mSessionId;
}

// updateTrackingOptions is called from Android HIDL clients and must be purely used
// to only update parameters of an ongoing session, and not start a new session.
void LocationClientApiImpl::updateTrackingOptions(uint32_t id, const TrackingOptions& options) {
    struct UpdateTrackingReq : public LocMsg {
        UpdateTrackingReq(LocationClientApiImpl* apiImpl, const TrackingOptions& options) :
                mApiImpl(apiImpl), mUpdatedOptions(options) {}
        virtual ~UpdateTrackingReq() {}
        void proc() const {
            // check if option is updated
            bool isOptionUpdated = false;

            if ((mApiImpl->mLocationOptions.minInterval != mUpdatedOptions.minInterval) ||
                (mApiImpl->mLocationOptions.minDistance != mUpdatedOptions.minDistance) ||
                (mApiImpl->mLocationOptions.locReqEngTypeMask !=
                        mUpdatedOptions.locReqEngTypeMask)) {
                isOptionUpdated = true;
            }

            if (!mApiImpl->mHalRegistered) {
                LOC_LOGe(">>> updateTrackingOptions - Not registered yet");
                return;
            }

            if (LOCATION_CLIENT_SESSION_ID_INVALID == mApiImpl->mSessionId) {
                LOC_LOGe(">>> updateTrackingOptions - No ongoing session in progress");
                return;
            }

            if (isOptionUpdated) {
                // update a tracking session, mLocationOptions
                // will be updated in updateTrackingOptionsSync
                mApiImpl->updateTrackingOptionsSync(const_cast<TrackingOptions&>(mUpdatedOptions),
                        false);
            } else {
                LOC_LOGd(">>> updateTrackingOptions - no change in option");
            }
        }
        LocationClientApiImpl* mApiImpl;
        TrackingOptions mUpdatedOptions;
    };
    mMsgTask.sendMsg(new (nothrow) UpdateTrackingReq(this, const_cast<TrackingOptions&>(options)));
}

void LocationClientApiImpl::startPositionSession(
        const LocationCallbacks& callbacksOption, const TrackingOptions& trackingOptions) {

    struct StartPositionSessionReqMsg : public LocMsg {
        StartPositionSessionReqMsg(LocationClientApiImpl* apiImpl,
                                   const LocationCallbacks& callbacksOption,
                                   const TrackingOptions& trackingOptions) :
                mApiImpl(apiImpl),
                mCallbacksOption(callbacksOption), mTrackingOptions(trackingOptions) {}
        virtual ~StartPositionSessionReqMsg() {}
        void proc() const {
            if (mApiImpl->mPositionSessionResponseCbPending) {
                mApiImpl->mLocationCbs.responseCb(::LOCATION_ERROR_ALREADY_STARTED, 0);
                return;
            }
            if (mApiImpl->isInBatching()) {
                mApiImpl->mLocationCbs.responseCb(
                        ::LOCATION_ERROR_EXCLUSIVE_SESSION_IN_PROGRESS, 0);
                return;
            }
            // set up the flag to indicate that responseCb is pending
            mApiImpl->mPositionSessionResponseCbPending = true;

            if (0 == mApiImpl->mSessionStartBootTimestampNs) {
                struct timespec ts;
                clock_gettime(CLOCK_BOOTTIME, &ts);
                mApiImpl->mSessionStartBootTimestampNs = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
            }

            mApiImpl->clearSubscriptions(TRACKING_CBS);
            mApiImpl->updateCallbacksSync(mCallbacksOption);
            mApiImpl->startTrackingSync(mTrackingOptions);
        }
        LocationClientApiImpl* mApiImpl;
        mutable LocationCallbacks mCallbacksOption;
        mutable TrackingOptions   mTrackingOptions;
    };

    mMsgTask.sendMsg(new (nothrow) StartPositionSessionReqMsg(
            this, callbacksOption, trackingOptions));
}

void LocationClientApiImpl::stopTracking(uint32_t) {

    struct StopTrackingReq : public LocMsg {
        StopTrackingReq(LocationClientApiImpl* apiImpl) : mApiImpl(apiImpl) {}
        virtual ~StopTrackingReq() {}
        void proc() const {
            mApiImpl->stopTrackingSync(false);
        }
        LocationClientApiImpl* mApiImpl;
    };
    mMsgTask.sendMsg(new (nothrow) StopTrackingReq(this));
}

void LocationClientApiImpl::stopTrackingAndClearSubscriptions(uint32_t) {

    struct StopTrackingReq : public LocMsg {
        StopTrackingReq(LocationClientApiImpl* apiImpl) : mApiImpl(apiImpl) {}
        virtual ~StopTrackingReq() {}
        void proc() const {
            mApiImpl->stopTrackingSync(true);
            mApiImpl->clearSubscriptions(TRACKING_CBS);
        }
        LocationClientApiImpl* mApiImpl;
    };
    mMsgTask.sendMsg(new (nothrow) StopTrackingReq(this));
}

void LocationClientApiImpl::stopTrackingSync(bool clearSubscriptions) {
    if (mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) {
        if (mHalRegistered &&
                ((mLocationOptions.minInterval != 0) ||
                    (mLocationOptions.minDistance != 0))) {
            string pbStr;
            LocAPIStopTrackingReqMsg msg(mSocketName, &mPbufMsgConv,
                    clearSubscriptions);
            if (msg.serializeToProtobuf(pbStr)) {
                bool rc = sendMessage(
                        reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                                pbStr.size());
                LOC_LOGd(">>> StopTrackingReq rc=%d\n", rc);
            } else {
                LOC_LOGe("LocAPIStopTrackingReqMsg serializeToProtobuf failed");
            }
        }
    }

    mLocationOptions.minInterval = 0;
    mLocationOptions.minDistance = 0;
    mSessionId = LOCATION_CLIENT_SESSION_ID_INVALID;
    mPositionSessionResponseCbPending = false;
    mSessionStartBootTimestampNs = 0;
}

void LocationClientApiImpl::clearSubscriptions(LocationCallbackType cbTypeToClear) {
    switch (cbTypeToClear) {
        case TRACKING_CBS:
            mCallbacksMask &= ~LOCATION_SESSON_ALL_INFO_MASK;

            mLocationCbs.trackingCb = nullptr;
            mLocationCbs.gnssLocationInfoCb = nullptr;
            mLocationCbs.gnssSvCb = nullptr;
            mLocationCbs.gnssNmeaCb = nullptr;
            mLocationCbs.gnssDataCb = nullptr;
            mLocationCbs.gnssMeasurementsCb = nullptr;
            mLocationCbs.gnssNHzMeasurementsCb = nullptr;
            mLocationCbs.engineLocationsInfoCb = nullptr;
            mLocationCbs.svEphemerisCb = nullptr;
            break;
        case BATCHING_CBS:
            mCallbacksMask &= ~LOCATION_BATCHING_SESSION_MASK;

            mLocationCbs.batchingCb = nullptr;
            mLocationCbs.batchingStatusCb = nullptr;
            break;
        case GEOFENCE_CBS:
            mCallbacksMask &= ~LOCATION_GEOFENCE_SESSION_MASK;

            mLocationCbs.geofenceBreachCb = nullptr;
            mLocationCbs.geofenceStatusCb = nullptr;
            break;
        default: return;
    }
}

void LocationClientApiImpl::updateTrackingOptionsSync(const TrackingOptions& option,
        bool clearSubscriptions) {

    LOC_LOGd(">>> updateTrackingOptionsSync,sessionId=%d, "
             "new Interval=%d Distance=%d, current Interval=%d Distance=%d",
             mSessionId, option.minInterval,
             option.minDistance, mLocationOptions.minInterval,
             mLocationOptions.minDistance);

    bool rc = true;
    string pbStr;
    // update option to passive listening where previous option
    // is not passive listening, in this case, we need to stop the session
    if (((0 == option.minInterval) && (0 == option.minDistance)) &&
            ((mLocationOptions.minInterval != 0) ||
             (mLocationOptions.minDistance != 0))) {
        LocAPIStopTrackingReqMsg msg(mSocketName, &mPbufMsgConv, clearSubscriptions);
        if (msg.serializeToProtobuf(pbStr)) {
            rc = sendMessage(reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                    pbStr.size());
        } else {
            LOC_LOGe("LocAPIStopTrackingReqMsg serializeToProtobuf failed");
        }
    } else if (((0 != option.minInterval) || (0 != option.minDistance)) &&
               ((0 == mLocationOptions.minInterval) &&
                (0 == mLocationOptions.minDistance))) {
        // update option from passive listening to none passive listening,
        // we need to start the session
        LocAPIStartTrackingReqMsg msg(mSocketName, option, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            rc = sendMessage(reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                    pbStr.size());
            LOC_LOGd(">>> start tracking Interval=%d Distance=%d",
                     option.minInterval, option.minDistance);
        } else {
            LOC_LOGe("LocAPIStartTrackingReqMsg serializeToProtobuf failed");
        }
    } else {
        LocAPIUpdateTrackingOptionsReqMsg msg(mSocketName, option, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                    pbStr.size());
            LOC_LOGd(">>> updateTrackingOptionsSync Interval=%d Distance=%d, reqTypeMask=0x%x "
                    "rc=%d",
                    option.minInterval, option.minDistance, option.locReqEngTypeMask, rc);
        } else {
            LOC_LOGe("LocAPIUpdateTrackingOptionsReqMsg serializeToProtobuf failed");
        }
    }

    mLocationOptions = option;
}

uint32_t LocationClientApiImpl::startBatching(const BatchingOptions& batchOptions) {
    struct StartBatchingReq : public LocMsg {
        StartBatchingReq(LocationClientApiImpl* apiImpl, const BatchingOptions& batchOptions) :
                mApiImpl(apiImpl), mBatchOptions(batchOptions) {}
        virtual ~StartBatchingReq() {}
        void proc() const {
            mApiImpl->startBatchingSync(mBatchOptions);
        }

        LocationClientApiImpl* mApiImpl;
        BatchingOptions mBatchOptions;
    };
    mMsgTask.sendMsg(new (nothrow) StartBatchingReq(this, batchOptions));
    return 0;
}

//Batching
uint32_t LocationClientApiImpl::startBatchingSync(const BatchingOptions& batchOptions) {
    if (!mHalRegistered) {
        mBatchingOptions = batchOptions;
        LOC_LOGe(">>> startBatching - Not registered yet");
        if (mLocationCbs.responseCb) {
            mLocationCbs.responseCb(::LOCATION_ERROR_SYSTEM_NOT_READY, 0);
        }
        return 0;
    }
    if (LOCATION_CLIENT_SESSION_ID_INVALID == mSessionId) {
        mBatchingOptions = batchOptions;
        //start a new batching session
        string pbStr;
        mBatchingId = mClientId;
        LocAPIStartBatchingReqMsg msg(mSocketName, mBatchingOptions.minInterval,
                                      mBatchingOptions.minDistance, mBatchingOptions.batchingMode,
                                      &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(
            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
            LOC_LOGd(">>> StartBatchingReq Interval=%d Distance=%d BatchingMode=%d rc=%d",
                     mBatchingOptions.minInterval, mBatchingOptions.minDistance,
                     mBatchingOptions.batchingMode, rc);
        } else {
            LOC_LOGe("LocAPIStartBatchingReqMsg serializeToProtobuf failed");
        }
    } else {
        updateBatchingOptions(mBatchingId, batchOptions);
    }
    return 0;
}

void LocationClientApiImpl::startBatchingSession(const LocationCallbacks& callbacksOption,
                                                 const BatchingOptions& batchingOptions) {
    struct StartBatchingSessionReqMsg : public LocMsg {
        StartBatchingSessionReqMsg(LocationClientApiImpl* apiImpl,
                                   const LocationCallbacks& callbacksOption,
                                   const BatchingOptions& batchingOptions) :
                mApiImpl(apiImpl), mCallbacksOption(callbacksOption),
                mBatchingOptions(batchingOptions) {}
        virtual ~StartBatchingSessionReqMsg() {}
        void proc() const {
            if (mApiImpl->mPositionSessionResponseCbPending) {
                mApiImpl->mLocationCbs.responseCb(::LOCATION_ERROR_ALREADY_STARTED, 0);
                return;
            }
            if (mApiImpl->isInTracking()) {
                mApiImpl->mLocationCbs.responseCb(
                        ::LOCATION_ERROR_EXCLUSIVE_SESSION_IN_PROGRESS, 0);
                return;
            }
            // set up the flag to indicate that responseCb is pending
            mApiImpl->mPositionSessionResponseCbPending = true;

            mApiImpl->updateCallbacksSync(mCallbacksOption);
            mApiImpl->startBatchingSync(mBatchingOptions);
        }
        LocationClientApiImpl* mApiImpl;
        mutable LocationCallbacks mCallbacksOption;
        mutable BatchingOptions   mBatchingOptions;
    };

    mMsgTask.sendMsg(new (nothrow) StartBatchingSessionReqMsg(
            this, callbacksOption, batchingOptions));
}

void LocationClientApiImpl::stopBatching(uint32_t id) {
    struct StopBatchingReq : public LocMsg {
        StopBatchingReq(LocationClientApiImpl *apiImpl) :
            mApiImpl(apiImpl) {}
        virtual ~StopBatchingReq() {}
        void proc() const {
            if (mApiImpl->mBatchingId == mApiImpl->mClientId) {
                string pbStr;
                mApiImpl->mBatchingOptions = {};
                LocAPIStopBatchingReqMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                    LOC_LOGd(">>> StopBatchingReq rc=%d", rc);
                } else {
                    LOC_LOGe("LocAPIStopBatchingReqMsg serializeToProtobuf failed");
                }
            }
            mApiImpl->mBatchingId = LOCATION_CLIENT_SESSION_ID_INVALID;
        }
        LocationClientApiImpl *mApiImpl;
    };
    mMsgTask.sendMsg(new (nothrow) StopBatchingReq(this));
}

void LocationClientApiImpl::updateBatchingOptions(uint32_t id,
        const BatchingOptions& batchOptions) {

    if ((mBatchingOptions.minInterval != batchOptions.minInterval) ||
            (mBatchingOptions.minDistance != batchOptions.minDistance) ||
            (mBatchingOptions.batchingMode != batchOptions.batchingMode)) {
        string pbStr;
        mBatchingOptions = batchOptions;
        LocAPIUpdateBatchingOptionsReqMsg msg(mSocketName, mBatchingOptions.minInterval,
                                              mBatchingOptions.minDistance,
                                              mBatchingOptions.batchingMode,
                                              &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(
                    reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
            LOC_LOGd(">>> StartBatchingReq Interval=%d Distance=%d BatchingMode=%d rc=%d",
                     mBatchingOptions.minInterval, mBatchingOptions.minDistance,
                     mBatchingOptions.batchingMode, rc);
        } else {
            LOC_LOGe("LocAPIUpdateBatchingOptionsReqMsg serializeToProtobuf failed");
        }
    } else {
        LOC_LOGd("No UpdateBatchingOptions because same Interval=%d Distance=%d, BatchingMode=%d",
                batchOptions.minInterval, batchOptions.minDistance, batchOptions.batchingMode);
        invokePositionSessionResponseCb(LOCATION_ERROR_SUCCESS);
    }
}

void LocationClientApiImpl::getBatchedLocations(uint32_t id, size_t count) {}

bool LocationClientApiImpl::checkGeofenceMap(size_t count, uint32_t* ids) {
    for (int i=0; i<count; ++i) {
        auto got = mGeofenceMap.find(ids[i]);
        if (got == mGeofenceMap.end()) {
            return false;
        }
    }
    return true;
}

void LocationClientApiImpl::addGeofenceMap(Geofence& geofence) {
    if (geofence.mGeofenceImpl != nullptr) {
        mGeofenceMap.insert(make_pair(geofence.mGeofenceImpl->getClientId(), geofence));
    }
}

void LocationClientApiImpl::eraseGeofenceMap(size_t count, uint32_t* ids) {
    for (int i=0; i<count; ++i) {
        mGeofenceMap.erase(ids[i]);
    }
}

bool LocationClientApiImpl::isGeofenceMapEmpty() {
    return mGeofenceMap.empty();
}

uint32_t* LocationClientApiImpl::addGeofences(size_t count, GeofenceOption* options,
        GeofenceInfo* infos) {
    if (!mHalRegistered) {
        LOC_LOGe(">>> addGeofences - Not registered yet");
        LocationError errs[count];
        for (int i=0; i < count; ++i) {
            errs[i] = LOCATION_ERROR_SYSTEM_NOT_READY;
        }
        mLocationCbs.collectiveResponseCb(count, errs, mLastAddedClientIds.data());
        return nullptr;
    }

    uint32_t gfCountUsed = std::min((size_t)MAX_GEOFENCE_ENTRY, count);
    //Add geofences, serialize geofence msg payload into ipc message payload
    GeofencesAddedReqPayload gfAddReqPayLoad;
    gfAddReqPayLoad.count = gfCountUsed;
    for (int i = 0; i < count; ++i) {
        gfAddReqPayLoad.gfPayload[i].gfClientId = mLastAddedClientIds[i];
        gfAddReqPayLoad.gfPayload[i].gfOption = options[i];
        gfAddReqPayLoad.gfPayload[i].gfInfo = infos[i];
    }

    string pbStr;
    LocAPIAddGeofencesReqMsg msg(mSocketName, gfAddReqPayLoad, &mPbufMsgConv);
    if (msg.serializeToProtobuf(pbStr)) {
        bool rc = sendMessage(reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                              pbStr.size());
        LOC_LOGd(">>> AddGeofencesReq count=%" PRIu32" rc=%d", gfCountUsed, rc);
    } else {
        LOC_LOGe("LocAPIAddGeofencesReqMsg serializeToProtobuf failed");
    }

    return nullptr;
}

void LocationClientApiImpl::addGeofences(const LocationCallbacks& callbacksOption,
                                         const std::vector<Geofence>& geofences) {
    struct AddGeofencesReq : public LocMsg {
        AddGeofencesReq(LocationClientApiImpl* apiImpl,
                        const LocationCallbacks& callbacksOption,
                        const std::vector<Geofence>& geofences):
                mApiImpl(apiImpl), mCallbacksOption(callbacksOption),
                mGeofences(std::move(geofences)) {}

        virtual ~AddGeofencesReq() {}
        void proc() const {
            if (mApiImpl->mPositionSessionResponseCbPending) {
                int cnt = mGeofences.size();
                LocationError* errs = new LocationError[cnt];
                if (nullptr == errs) {
                    LOC_LOGe("failed to create LocationErrors");
                    return;
                }
                uint32_t* ids = new uint32_t[cnt];
                if (nullptr == ids) {
                    LOC_LOGe("failed to create ClientIds");
                    delete [] errs;
                    return;
                }
                memset (ids, 0, cnt * sizeof(uint32_t));
                for (int i = 0; i < cnt; ++i) {
                    errs[i] = ::LOCATION_ERROR_ALREADY_STARTED;
                    if (mGeofences[i].mGeofenceImpl) {
                        ids[i] = mGeofences[i].mGeofenceImpl->getClientId();
                    }
                    LOC_LOGv("errs[%d]: %d, ids: %d", i, errs[i], ids[i]);
                    mApiImpl->addGeofenceMap(mGeofences[i]);
                }
                mApiImpl->mLocationCbs.collectiveResponseCb(cnt, errs, ids);
                delete[] ids;
                delete[] errs;
                return;
            }
            // set up the flag to indicate that responseCb is pending
            mApiImpl->mPositionSessionResponseCbPending = true;
            mApiImpl->updateCallbacksSync(mCallbacksOption);

            size_t count = mGeofences.size();
            mApiImpl->mLastAddedClientIds.clear();

            GeofenceOption* gfOptions = (GeofenceOption*)malloc(sizeof(GeofenceOption) * count);
            GeofenceInfo* gfInfos = (GeofenceInfo*)malloc(sizeof(GeofenceInfo) * count);
            if ((gfOptions != nullptr) && (gfInfos != nullptr)) {
                for (int i = 0; i < count; ++i) {
                    gfOptions[i].breachTypeMask = mGeofences[i].getBreachType();
                    gfOptions[i].responsiveness = mGeofences[i].getResponsiveness();
                    gfOptions[i].dwellTime = mGeofences[i].getDwellTime();
                    gfOptions[i].size = sizeof(gfOptions[i]);

                    gfInfos[i].latitude = mGeofences[i].getLatitude();
                    gfInfos[i].longitude = mGeofences[i].getLongitude();
                    gfInfos[i].radius = mGeofences[i].getRadius();
                    gfInfos[i].size = sizeof(gfInfos[i]);

                    uint32_t clientId = mGeofences[i].mGeofenceImpl->getClientId();
                    mApiImpl->mLastAddedClientIds.push_back(clientId);
                    mApiImpl->addGeofenceMap(mGeofences[i]);
                    LOC_LOGd("Geofence LastAddedClientId: %u", clientId);
                }
                mApiImpl->addGeofences(count, reinterpret_cast<GeofenceOption*>(gfOptions),
                                       reinterpret_cast<GeofenceInfo*>(gfInfos));
            } else {
                mApiImpl->invokePositionSessionResponseCb(LOCATION_ERROR_GENERAL_FAILURE);
            }
            if (gfOptions) {
                free(gfOptions);
            }
            if (gfInfos) {
                free(gfInfos);
            }
        }
        LocationClientApiImpl *mApiImpl;
        mutable LocationCallbacks mCallbacksOption;
        mutable std::vector<Geofence> mGeofences;
    };

    mMsgTask.sendMsg(new (nothrow) AddGeofencesReq(this, callbacksOption, geofences));
}

void LocationClientApiImpl::removeGeofences(size_t count, uint32_t* ids) {
    struct RemoveGeofencesReq : public LocMsg {
        RemoveGeofencesReq(LocationClientApiImpl* apiImpl, uint32_t count, uint32_t* gfIds) :
                mApiImpl(apiImpl), mGfCount(count), mGfIds(gfIds) {}
        virtual ~RemoveGeofencesReq() {}
        void proc() const {
            if (!mApiImpl->checkGeofenceMap(mGfCount, mGfIds)) {
                LOC_LOGe ("Wrong geofence IDs");
            } else  if (!mApiImpl->mHalRegistered) {
                LOC_LOGe(">>> removeGeofences - Not registered yet");
            } else if (mGfCount > 0) {
                uint32_t gfCountUsed = std::min((uint32_t)MAX_GEOFENCE_ENTRY, mGfCount);
                //Remove geofences
                GeofencesReqClientIdPayload gfRemReqPayLoad;
                gfRemReqPayLoad.count = gfCountUsed;
                memcpy(gfRemReqPayLoad.gfIds, mGfIds, sizeof(uint32_t) * gfCountUsed);

                string pbStr;
                LocAPIRemoveGeofencesReqMsg msg(mApiImpl->mSocketName, gfRemReqPayLoad,
                        &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                    LOC_LOGd(">>> RemoveGeofencesReq count=%" PRIu32" rc=%d", gfCountUsed, rc);
                } else {
                    LOC_LOGe("LocAPIRemoveGeofencesReqMsg serializeToProtobuf failed");
                }
            } else {
                LOC_LOGe("Invalid number of Gf count");
            }
            free(mGfIds);
        }
        LocationClientApiImpl *mApiImpl;
        uint32_t mGfCount;
        uint32_t* mGfIds;
    };
    mMsgTask.sendMsg(new (nothrow) RemoveGeofencesReq(this, count, ids));
}

void LocationClientApiImpl::modifyGeofences(
        size_t count, uint32_t* ids, GeofenceOption* options) {
    struct ModifyGeofencesReq : public LocMsg {
        ModifyGeofencesReq(LocationClientApiImpl* apiImpl, uint32_t count, uint32_t* gfIds,
                GeofenceOption* gfOptions) :
                mApiImpl(apiImpl), mGfCount(count), mGfIds(gfIds), mGfOptions(gfOptions) {}
        virtual ~ModifyGeofencesReq() {}
        void proc() const {
            if (!mApiImpl->checkGeofenceMap(mGfCount, mGfIds)) {
                LOC_LOGe ("Wrong geofence IDs");
            } else if (!mApiImpl->mHalRegistered) {
                LOC_LOGe(">>> modifyGeofences - Not registered yet");
            } else if (mGfCount > 0) {
                uint32_t gfCountUsed = std::min((uint32_t)MAX_GEOFENCE_ENTRY, mGfCount);
                //Modify geofences
                GeofencesAddedReqPayload gfModReqPayLoad;
                gfModReqPayLoad.count = gfCountUsed;
                for (int i=0; i < gfCountUsed; ++i) {
                    gfModReqPayLoad.gfPayload[i].gfClientId = mGfIds[i];
                    gfModReqPayLoad.gfPayload[i].gfOption = mGfOptions[i];
                    mApiImpl->mGeofenceMap.at(mGfIds[i]).setBreachType(
                            (GeofenceBreachTypeMask)mGfOptions[i].breachTypeMask);
                    mApiImpl->mGeofenceMap.at(mGfIds[i]).setResponsiveness(
                            mGfOptions[i].responsiveness);
                    mApiImpl->mGeofenceMap.at(mGfIds[i]).setDwellTime(mGfOptions[i].dwellTime);
                    LOC_LOGv(">>> updateGfOption, clientID: %d, %d %d %d", mGfIds[i],
                            mApiImpl->mGeofenceMap.at(mGfIds[i]).getBreachType(),
                            mApiImpl->mGeofenceMap.at(mGfIds[i]).getResponsiveness(),
                            mApiImpl->mGeofenceMap.at(mGfIds[i]).getDwellTime());
                }

                string pbStr;
                LocAPIModifyGeofencesReqMsg msg(mApiImpl->mSocketName, gfModReqPayLoad,
                        &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                    LOC_LOGd(">>> ModifyGeofencesReq count=%" PRIu32 " rc=%d", gfCountUsed, rc);
                } else {
                    LOC_LOGe("LocAPIModifyGeofencesReqMsg serializeToProtobuf failed");
                }
            } else {
                LOC_LOGe("Invalid number of Gf count");
            }
            free(mGfIds);
            free(mGfOptions);
        }
        LocationClientApiImpl *mApiImpl;
        uint32_t mGfCount;
        uint32_t* mGfIds;
        GeofenceOption* mGfOptions;
    };
    mMsgTask.sendMsg(new (nothrow) ModifyGeofencesReq(this, count, ids, options));
}

void LocationClientApiImpl::pauseGeofences(size_t count, uint32_t* ids) {
    struct PauseGeofencesReq : public LocMsg {
        PauseGeofencesReq(LocationClientApiImpl* apiImpl, uint32_t count, uint32_t* gfIds) :
                mApiImpl(apiImpl), mGfCount(count), mGfIds(gfIds) {}
        virtual ~PauseGeofencesReq() {}
        void proc() const {
            if (!mApiImpl->checkGeofenceMap(mGfCount, mGfIds)) {
                LOC_LOGe ("Wrong geofence IDs");
            } else if (!mApiImpl->mHalRegistered) {
                LOC_LOGe(">>> pauseGeofences - Not registered yet");
            } else if (mGfCount > 0) {
                uint32_t gfCountUsed = std::min((uint32_t)MAX_GEOFENCE_ENTRY, mGfCount);
                //Pause geofences
                GeofencesReqClientIdPayload gfPauseReqPayLoad;
                gfPauseReqPayLoad.count = gfCountUsed;
                memcpy(gfPauseReqPayLoad.gfIds, mGfIds, sizeof(uint32_t) * gfCountUsed);

                string pbStr;
                LocAPIPauseGeofencesReqMsg msg(mApiImpl->mSocketName, gfPauseReqPayLoad,
                        &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                    LOC_LOGd(">>> PauseGeofencesReq count=%" PRIu32" rc=%d", gfCountUsed, rc);
                } else {
                    LOC_LOGe("LocAPIPauseGeofencesReqMsg serializeToProtobuf failed");
                }
            } else {
                LOC_LOGe("Invalid number of Gf count");
            }
            free(mGfIds);
        }
        LocationClientApiImpl *mApiImpl;
        uint32_t mGfCount;
        uint32_t* mGfIds;
    };
    mMsgTask.sendMsg(new (nothrow) PauseGeofencesReq(this, count, ids));
}

void LocationClientApiImpl::resumeGeofences(size_t count, uint32_t* ids) {
    struct ResumeGeofencesReq : public LocMsg {
        ResumeGeofencesReq(LocationClientApiImpl* apiImpl, uint32_t count, uint32_t* gfIds) :
                mApiImpl(apiImpl), mGfCount(count), mGfIds(gfIds) {}
        virtual ~ResumeGeofencesReq() {}
        void proc() const {
            if (!mApiImpl->checkGeofenceMap(mGfCount, mGfIds)) {
                LOC_LOGe ("Wrong geofence IDs");
            } else if (!mApiImpl->mHalRegistered) {
                LOC_LOGe(">>> resumeGeofences - Not registered yet");
            } else if (mGfCount > 0) {
                uint32_t gfCountUsed = std::min((uint32_t)MAX_GEOFENCE_ENTRY, mGfCount);
                //Resume geofences
                GeofencesReqClientIdPayload gfResumeReqPayLoad;
                gfResumeReqPayLoad.count = gfCountUsed;
                memcpy(gfResumeReqPayLoad.gfIds, mGfIds, sizeof(uint32_t) * gfCountUsed);

                string pbStr;
                LocAPIResumeGeofencesReqMsg msg(mApiImpl->mSocketName, gfResumeReqPayLoad,
                        &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                    LOC_LOGd(">>> ResumeGeofencesReq count=%" PRIu32"rc=%d", gfCountUsed, rc);
                } else {
                    LOC_LOGe("LocAPIResumeGeofencesReqMsg serializeToProtobuf failed");
                }
            } else {
                LOC_LOGe("Invalid number of Gf count");
            }
            free(mGfIds);
        }
        LocationClientApiImpl *mApiImpl;
        uint32_t mGfCount;
        uint32_t* mGfIds;
    };
    mMsgTask.sendMsg(new (nothrow) ResumeGeofencesReq(this, count, ids));
}

void LocationClientApiImpl::updateNetworkAvailability(bool available) {

    struct UpdateNetworkAvailabilityReq : public LocMsg {
        UpdateNetworkAvailabilityReq(LocationClientApiImpl* apiImpl, bool available) :
                mApiImpl(apiImpl), mAvailable(available) {}
        virtual ~UpdateNetworkAvailabilityReq() {}
        void proc() const {
            string pbStr;
            LocAPIUpdateNetworkAvailabilityReqMsg msg(mApiImpl->mSocketName,
                                                      mAvailable,
                                                      &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                bool rc = mApiImpl->sendMessage(
                        reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                LOC_LOGd(">>> UpdateNetworkAvailabilityReq available=%d rc=%d", mAvailable, rc);
            } else {
                LOC_LOGe("LocAPIUpdateNetworkAvailabilityReqMsg serializeToProtobuf failed");
            }
        }
        LocationClientApiImpl* mApiImpl;
        const bool mAvailable;
    };
    mMsgTask.sendMsg(new (nothrow) UpdateNetworkAvailabilityReq(this, available));
}

void LocationClientApiImpl::getGnssEnergyConsumed(
        gnssEnergyConsumedCallback gnssEnergyConsumedCb,
        responseCallback responseCb) {

    struct GetGnssEnergyConsumedReq : public LocMsg {
        GetGnssEnergyConsumedReq(LocationClientApiImpl *apiImpl,
                                 gnssEnergyConsumedCallback gnssEnergyConsumedCb,
                                 responseCallback responseCb) :
        mApiImpl(apiImpl),
        mGnssEnergyConsumedCb(gnssEnergyConsumedCb),
        mResponseCb(responseCb) {}

        virtual ~GetGnssEnergyConsumedReq() {}
        void proc() const {
            mApiImpl->mGnssEnergyConsumedInfoCb = mGnssEnergyConsumedCb;
            mApiImpl->mGnssEnergyConsumedResponseCb = mResponseCb;

            // send msg to the hal daemon
            if (nullptr != mApiImpl->mGnssEnergyConsumedInfoCb) {
                string pbStr;
                LocAPIGetGnssEnergyConsumedReqMsg msg(mApiImpl->mSocketName,
                        &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                    if (mResponseCb) {
                        if (true == rc) {
                            mResponseCb(LOCATION_ERROR_SUCCESS, 0);
                        } else {
                            mResponseCb(LOCATION_ERROR_GENERAL_FAILURE, 0);
                        }
                    }
                } else {
                    LOC_LOGe("LocAPIGetGnssEnergyConsumedReqMsg serializeToProtobuf failed");
                }
            }
        }

        LocationClientApiImpl *mApiImpl;
        gnssEnergyConsumedCallback   mGnssEnergyConsumedCb;
        responseCallback             mResponseCb;
    };

    LOC_LOGd(">>> getGnssEnergyConsumed ");
    mMsgTask.sendMsg(new (nothrow)GetGnssEnergyConsumedReq(
            this, gnssEnergyConsumedCb, responseCb));
}

void LocationClientApiImpl::updateLocationSystemInfoListener(
        locationSystemInfoCallback locationSystemInfoCb,
        responseCallback responseCb) {

    struct UpdateLocationSystemInfoListenerReq : public LocMsg {
        UpdateLocationSystemInfoListenerReq(LocationClientApiImpl *apiImpl,
                                            locationSystemInfoCallback locationSystemInfoCb,
                                            responseCallback responseCb) :
        mApiImpl(apiImpl),
        mLocSysInfoCb(locationSystemInfoCb),
        mResponseCb(responseCb) {}

        virtual ~UpdateLocationSystemInfoListenerReq() {}
        void proc() const {
            bool needIpc = false;
            LocationCallbacksMask callbackMaskCopy = mApiImpl->mCallbacksMask;
            // send msg to the hal daemon if the registration changes
            if ((nullptr != mLocSysInfoCb) &&
                (nullptr == mApiImpl->mLocationSysInfoCb)) {
                // client registers for system info, set up the bit
                mApiImpl->mCallbacksMask |= E_LOC_CB_SYSTEM_INFO_BIT;
                needIpc = true;
            } else if ((nullptr == mLocSysInfoCb) &&
                       (nullptr != mApiImpl->mLocationSysInfoCb)) {
                // system info is no longer needed, clear the bit
                mApiImpl->mCallbacksMask &= ~E_LOC_CB_SYSTEM_INFO_BIT;
                needIpc = true;
            }

            // save the new callback
            mApiImpl->mLocationSysInfoCb = mLocSysInfoCb;
            mApiImpl->mLocationSysInfoResponseCb = mResponseCb;

            // inform hal daemon of updated callback only when changed
            if (true == needIpc) {
                if (mApiImpl->mHalRegistered) {
                    string pbStr;
                    LocAPIUpdateCallbacksReqMsg msg(mApiImpl->mSocketName,
                                                    mApiImpl->mCallbacksMask,
                                                    &mApiImpl->mPbufMsgConv);
                    if (msg.serializeToProtobuf(pbStr)) {
                        bool rc = mApiImpl->sendMessage(
                                reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                                        pbStr.size());
                        LOC_LOGd(">>> UpdateCallbacksReq new callBacksMask=0x%x, "
                                 "old mask =0x%x, rc=%d",
                                 mApiImpl->mCallbacksMask, callbackMaskCopy, rc);
                        if (mResponseCb) {
                            if (true == rc) {
                                mResponseCb(LOCATION_ERROR_SUCCESS, 0);
                            } else {
                                mResponseCb(LOCATION_ERROR_GENERAL_FAILURE, 0);
                            }
                        }
                    } else {
                        LOC_LOGe("LocAPIUpdateCallbacksReqMsg serializeToProtobuf failed");
                    }
                }
            } else {
                if (mResponseCb) {
                    mResponseCb(LOCATION_ERROR_SUCCESS, 0);
                }
                LOC_LOGd("No updateCallbacks because same callback");
            }
        }

        LocationClientApiImpl *mApiImpl;
        locationSystemInfoCallback   mLocSysInfoCb;
        responseCallback             mResponseCb;
    };

    LOC_LOGd(">>> updateLocationSystemInfoListener ");
    mMsgTask.sendMsg(new (nothrow)UpdateLocationSystemInfoListenerReq(
            this, locationSystemInfoCb, responseCb));
}

void LocationClientApiImpl::getSingleTerrestrialPos(uint32_t timeoutMsec,
        TerrestrialTechMask techMask,
        float horQoS, trackingCallback terrestrialPositionCb,
        responseCallback responseCb) {

    struct GetSingleTerrestrialPosReq : public LocMsg {
        GetSingleTerrestrialPosReq(LocationClientApiImpl *apiImpl,
                                        uint32_t timeoutMsec,
                                        TerrestrialTechMask techMask,
                                        float horQoS,
                                        trackingCallback terrestrialPositionCb,
                                        responseCallback responseCb) :
            mApiImpl(apiImpl), mTimeoutMsec(timeoutMsec), mTechMask(techMask),
            mHorQoS(horQoS), mSingleTerrestrialPosCb(terrestrialPositionCb),
            mResponseCb(responseCb) {}

        virtual ~GetSingleTerrestrialPosReq() {}
        void proc() const {
            do {
                if ((nullptr == mApiImpl->mSingleTerrestrialPosCb) &&
                        (nullptr == mSingleTerrestrialPosCb)) {
                    // pos cb can not be null if there is no pending request
                    if (mResponseCb) {
                        LOC_LOGe("No pending request to cancel");
                        mResponseCb(LOCATION_ERROR_INVALID_PARAMETER, 0);
                    }
                    break;
                }

                if (mApiImpl->mSingleTerrestrialPosCb != nullptr) {
                    LocationError errorCode = LOCATION_ERROR_ALREADY_STARTED;
                    if (nullptr == mSingleTerrestrialPosCb) {
                        // client wants to cancel the request
                        mApiImpl->mSingleTerrestrialPosCb = nullptr;
                        errorCode = LOCATION_ERROR_SUCCESS;
                    } // else: LOCATION_ERROR_ALREADY_STARTED

                    if (mResponseCb) {
                        // inform client of the response
                        mResponseCb(errorCode, 0);
                    }
                    break;
                }

                if (!mApiImpl->mHalRegistered) {
                    if (mResponseCb) {
                        mResponseCb(LOCATION_ERROR_SYSTEM_NOT_READY, 0);
                    }
                    break;
                }

                string pbStr;
                LocAPIGetSingleTerrestrialPosReqMsg msg(
                        mApiImpl->mSocketName, mTimeoutMsec, mTechMask, mHorQoS,
                        &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                                        pbStr.size());
                    if (rc) {
                        // request has been sent successfully to hal daemon,
                        // save the new callback
                        mApiImpl->mSingleTerrestrialPosCb = mSingleTerrestrialPosCb;
                        mApiImpl->mSingleTerrestrialPosRespCb = mResponseCb;
                    } else if (mResponseCb) {
                        // request failed to send to hal daemon
                        // inform client and the callback shall not be saved
                        mResponseCb(LOCATION_ERROR_GENERAL_FAILURE, 0);
                    }
                }
            } while (0);
        }

        LocationClientApiImpl *mApiImpl;
        uint32_t mTimeoutMsec;
        TerrestrialTechMask mTechMask;
        float mHorQoS;
        trackingCallback mSingleTerrestrialPosCb;
        responseCallback mResponseCb;
    };

    mMsgTask.sendMsg(new (nothrow)GetSingleTerrestrialPosReq(
            this, timeoutMsec, techMask, horQoS,
            terrestrialPositionCb, responseCb));
}

void LocationClientApiImpl::getDebugReport(GnssDebugReport& report) {

    struct GetDebugReportReq : public LocMsg {

        GetDebugReportReq(LocationClientApiImpl* apiImpl) :
            mApiImpl(apiImpl) {}
        virtual ~GetDebugReportReq() {}
        void proc() const {
            string pbStr;
            LocAPIGetDebugReqMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                bool rc = mApiImpl->sendMessage(
                    reinterpret_cast<uint8_t*>((uint8_t*)pbStr.c_str()),
                    pbStr.size());
                LOC_LOGd(">>> send LocAPIGetDebugReqMsg rc=%d", rc);
            } else {
                LOC_LOGe("LocAPIGetDebugReqMsg serializeToProtobuf failed");
            }
        }

        LocationClientApiImpl* mApiImpl;
    };

    mpDebugReport = &report;
    mMsgTask.sendMsg(new (nothrow) GetDebugReportReq(this));
    wait(500);  //500ms
}

void LocationClientApiImpl::processGetDebugRespCb(const LocAPIGetDebugRespMsg* pRespMsg) {
    *mpDebugReport = pRespMsg->mDebugReport;
    for (uint32_t i = 0; i < pRespMsg->mDebugReport.mSatelliteInfo.size(); i++) {
        mpDebugReport->mSatelliteInfo[i] = pRespMsg->mDebugReport.mSatelliteInfo[i];
    }
    notify(); //for the wait in getDebugReport
}

uint32_t LocationClientApiImpl::getAntennaInfo(AntennaInfoCallback* cb) {
    struct GetAntennaInfoMsg : public LocMsg {
        GetAntennaInfoMsg(LocationClientApiImpl* apiImpl) :
                mApiImpl(apiImpl) {}
        virtual ~GetAntennaInfoMsg() {}
        void proc() const {
            string pbStr;
            LocAPIGetAntennaInfoMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                bool rc = mApiImpl->sendMessage(
                    reinterpret_cast<uint8_t*>((uint8_t*)pbStr.c_str()),
                    pbStr.size());
                LOC_LOGd(">>> send LocAPIGetAntennaInfoMsg rc=%d", rc);
            }
            else {
                LOC_LOGe("LocAPIGetAntennaInfoMsg serializeToProtobuf failed");
            }
        }

        LocationClientApiImpl* mApiImpl;
    };

    if (!mHalRegistered) {
        LOC_LOGe("Not registered yet");
        return LOCATION_ERROR_GENERAL_FAILURE;
    }
    mpAntennaInfoCb = cb;
    mMsgTask.sendMsg(new (nothrow) GetAntennaInfoMsg(this));
    return LOCATION_ERROR_SUCCESS;
}

void LocationClientApiImpl::processAntennaInfo(
        const LocAPIAntennaInfoMsg* pAntennaInfoMsg) {
    if (mpAntennaInfoCb) {
        (*mpAntennaInfoCb)((std::vector<GnssAntennaInformation> &)
                (pAntennaInfoMsg->mAntennaInfo.antennaInfos));
    } else {
        LOC_LOGe("NULL mpAntennaInfoCb");
    }
}

void LocationClientApiImpl::getSinglePos(
        uint32_t timeoutMsec, float horQoS,
        trackingCallback positionCb, responseCallback responseCb) {

    struct GetSinglePosReq : public LocMsg {
        GetSinglePosReq(LocationClientApiImpl *apiImpl,
                        uint32_t timeoutMsec,
                        float horQoS,
                        trackingCallback positionCb,
                        responseCallback responseCb) :
            mApiImpl(apiImpl), mTimeoutMsec(timeoutMsec),
            mHorQoS(horQoS), mSinglePosCb(positionCb),
            mResponseCb(responseCb) {}

        virtual ~GetSinglePosReq() {}
        void proc() const {
            do {
                if ((mApiImpl->mSinglePosCb == nullptr) && (mSinglePosCb == nullptr)) {
                    // if there is no pending request, nothing to cancel
                    if (mResponseCb) {
                        mResponseCb(LOCATION_ERROR_SUCCESS, 0);
                    }
                    break;
                }

                if (mApiImpl->mSinglePosCb != nullptr){
                    if (mSinglePosCb != nullptr) {
                        if (mResponseCb) {
                            // inform client of the response
                            mResponseCb(LOCATION_ERROR_ALREADY_STARTED, 0);
                        }
                        break;
                    } else {
                        LOC_LOGe("cancel current request");
                        // always return success for cancelling current request
                        mResponseCb(LOCATION_ERROR_SUCCESS, 0);
                    }
                }

                if (!mApiImpl->mHalRegistered) {
                    if (mResponseCb) {
                        mResponseCb(LOCATION_ERROR_SYSTEM_NOT_READY, 0);
                    }
                    break;
                }

                string pbStr;
                LocAPIGetSinglePosReqMsg msg(
                        mApiImpl->mSocketName, mTimeoutMsec, mHorQoS,
                        &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                                        pbStr.size());
                    if (rc) {
                        // request has been sent successfully to hal daemon,
                        // save the new callback
                        mApiImpl->mSinglePosCb = mSinglePosCb;
                        mApiImpl->mSinglePosRespCb = mResponseCb;
                    } else if (mResponseCb) {
                        // request failed to send to hal daemon
                        // inform client and the callback shall not be saved
                        mResponseCb(LOCATION_ERROR_GENERAL_FAILURE, 0);
                    }
                }
            } while (0);
        }

        LocationClientApiImpl *mApiImpl;
        uint32_t mTimeoutMsec;
        float mHorQoS;
        trackingCallback mSinglePosCb;
        responseCallback mResponseCb;
    };

    mMsgTask.sendMsg(new (nothrow)GetSinglePosReq(
            this, timeoutMsec, horQoS, positionCb, responseCb));
}

/******************************************************************************
LocationClientApiImpl - LocIpc onReceive handler
******************************************************************************/
void LocationClientApiImpl::capabilitesCallback(ELocMsgID msgId, const void* msgData) {

    bool oldHalRegisterd = mHalRegistered;
    mHalRegistered = true;
    const LocAPICapabilitiesIndMsg* pCapabilitiesIndMsg =
            (LocAPICapabilitiesIndMsg*)(msgData);
    mCapsMask = parseCapabilitiesMask(pCapabilitiesIndMsg->capabilitiesMask);

    if (mCapabilitiesCb) {
        mCapabilitiesCb(pCapabilitiesIndMsg->capabilitiesMask);
    }

    mYearOfHw = parseYearOfHw(pCapabilitiesIndMsg->capabilitiesMask);

    // send updatecallback request
    if (0 != mCallbacksMask) {
        string pbStr;
        LocAPIUpdateCallbacksReqMsg msg(mSocketName, mCallbacksMask, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                    pbStr.size());
            LOC_LOGd(">>> UpdateCallbacksReq callBacksMask=0x%x rc=%d", mCallbacksMask, rc);
        } else {
            LOC_LOGe("LocAPIUpdateCallbacksReqMsg serializeToProtobuf failed");
        }
    }

    if (oldHalRegisterd == true) {
        LOC_LOGi("hal is not restarted, return");
        return;
    }

    LOC_LOGe(">>> session id %d, cap mask 0x%" PRIx64, mSessionId, mCapsMask);
    if (isInTracking())  {
        // force mSessionId to invalid so startTracking will start the sesssion
        // if hal deamon crashes and restarts in the middle of a session
        mSessionId = LOCATION_CLIENT_SESSION_ID_INVALID;
        TrackingOptions trackOption;
        trackOption.setLocationOptions(mLocationOptions);
        (void)startTrackingSync(trackOption);
    } else if (isInBatching()) {
        // force mBatchingId to invalid so startBatching will start the sesssion
        // if hal deamon crashes and restarts in the middle of a session
        mBatchingId = LOCATION_CLIENT_SESSION_ID_INVALID;
        BatchingOptions batchOption = mBatchingOptions;
        (void)startBatchingSync(batchOption);
    }

    if (mGeofenceMap.size() > 0) {
        size_t count = mGeofenceMap.size();
        GeofenceOption* gfOptions = (GeofenceOption*)malloc(sizeof(GeofenceOption) * count);
        GeofenceInfo* gfInfos = (GeofenceInfo*)malloc(sizeof(GeofenceInfo) * count);
        if ((gfOptions != nullptr) && (gfInfos != nullptr)) {
            int i = 0;
            for (auto it = mGeofenceMap.begin(); it != mGeofenceMap.end(); ++it) {
                gfOptions[i].breachTypeMask = it->second.getBreachType();
                gfOptions[i].responsiveness = it->second.getResponsiveness();
                gfOptions[i].dwellTime = it->second.getDwellTime();
                gfOptions[i].size = sizeof(gfOptions[i]);

                gfInfos[i].latitude = it->second.getLatitude();
                gfInfos[i].longitude = it->second.getLongitude();
                gfInfos[i].radius = it->second.getRadius();
                gfInfos[i].size = sizeof(gfInfos[i]);
                ++i;
            }
            addGeofences(count, gfOptions, gfInfos);
        }
        if (gfOptions) {
            free(gfOptions);
        }
        if (gfInfos) {
            free(gfInfos);
        }
    }

    // hal daemon restarts
    // inform client that gtp fix request fails and reset the variables
    if (mSingleTerrestrialPosRespCb) {
        mSingleTerrestrialPosRespCb(LOCATION_ERROR_SYSTEM_NOT_READY, 0);
    }
    mSingleTerrestrialPosCb = nullptr;
    mSingleTerrestrialPosRespCb = nullptr;

    // inform client that single shot fix request fails and reset the variables
    if (mSinglePosRespCb) {
        mSinglePosRespCb(LOCATION_ERROR_SYSTEM_NOT_READY, 0);
    }
    mSinglePosCb = nullptr;
    mSinglePosRespCb = nullptr;
}

void LocationClientApiImpl::pingTest(PingTestCb pingTestCallback) {

    mPingTestCb = pingTestCallback;

    struct PingTestReq : public LocMsg {
        PingTestReq(LocationClientApiImpl *apiImpl) : mApiImpl(apiImpl){}
        virtual ~PingTestReq() {}
        void proc() const {
            string pbStr;
            LocAPIPingTestReqMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendMessage(
                        reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
            } else {
                LOC_LOGe("LocAPIPingTestReqMsg serializeToProtobuf failed");
            }
        }
        LocationClientApiImpl *mApiImpl;
    };
    mMsgTask.sendMsg(new (nothrow) PingTestReq(this));
    return;
}

void LocationClientApiImpl::invokePositionSessionResponseCb(LocationError errCode) {
    if (mPositionSessionResponseCbPending) {
        if (nullptr != mLocationCbs.responseCb) {
            mLocationCbs.responseCb(errCode, 0);
        }
        mPositionSessionResponseCbPending = false;
    }
}

/******************************************************************************
LocationClientApiImpl -ILocIpcListener
******************************************************************************/
void IpcListener::onListenerReady() {
    struct ClientRegisterReq : public LocMsg {
        ClientRegisterReq(LocationClientApiImpl& apiImpl) : mApiImpl(apiImpl) {}
        void proc() const {
            string pbStr;
            LocAPIClientRegisterReqMsg msg(mApiImpl.mSocketName, LOCATION_CLIENT_API,
                    &mApiImpl.mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl.sendMessage(
                        reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
            } else {
                LOC_LOGe("LocAPIClientRegisterReqMsg serializeToProtobuf failed");
            }
        }
        LocationClientApiImpl& mApiImpl;
    };
    if (SockNode::Local == mSockTpye) {
        if (0 != chown(mApiImpl.mSocketName, getuid(), GID_LOCCLIENT)) {
            LOC_LOGe("chown to group locclient failed %s", strerror(errno));
        }
    }
    mMsgTask.sendMsg(new (nothrow) ClientRegisterReq(mApiImpl));
}

void IpcListener::onReceive(const char* data, uint32_t length,
                            const LocIpcRecver* recver) {
    struct OnReceiveHandler : public LocMsg {
        OnReceiveHandler(LocationClientApiImpl& apiImpl, IpcListener& listener,
                         const char* data, uint32_t length) :
                mApiImpl(apiImpl), mListener(listener), mMsgData(data, length) {}

        virtual ~OnReceiveHandler() {}
        void proc() const {
            // Protobuff Encoding enabled, so we need to convert the message from proto
            // encoded format to local structure
            PBLocAPIMsgHeader pbLocApiMsg;
            if (0 == pbLocApiMsg.ParseFromString(mMsgData)) {
                LOC_LOGe("Failed to parse pbLocApiMsg from input stream!! length: %" PRIu32,
                        (uint32_t) mMsgData.length());
                return;
            }

            ELocMsgID eLocMsgid = mApiImpl.mPbufMsgConv.getEnumForPBELocMsgID(pbLocApiMsg.msgid());
            string sockName = pbLocApiMsg.msocketname();
            uint32_t payloadSize = pbLocApiMsg.payloadsize();
            // pbLocApiMsg.payload() contains the payload data.

            LOC_LOGi(">-- onReceive Rcvd msg id: %d %s, sockname: %s, payload size: %d",
                    eLocMsgid, LocApiMsgString(eLocMsgid), sockName.c_str(), payloadSize);
            LocAPIMsgHeader locApiMsg(sockName.c_str(), eLocMsgid);

            // throw away message that does not come from location hal daemon
            if (false == locApiMsg.isValidServerMsg(payloadSize)) {
                return;
            }

            switch (locApiMsg.msgId) {
            case E_LOCAPI_CAPABILILTIES_MSG_ID:
            {
                LOC_LOGi("<<< capabilities indication for client: %s", mApiImpl.mSocketName);
                PBLocAPICapabilitiesIndMsg pbLocApiCapIndMsg;
                if (0 == pbLocApiCapIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse pbLocApiCapIndMsg from payload!!");
                    return;
                }
                LocAPICapabilitiesIndMsg msg(sockName.c_str(), pbLocApiCapIndMsg,
                        &mApiImpl.mPbufMsgConv);
                mApiImpl.capabilitesCallback(locApiMsg.msgId, (void*)&msg);
                break;
            }

            case E_LOCAPI_HAL_READY_MSG_ID:
            {
                LOC_LOGi("<<< HAL ready message for client: %s", mApiImpl.mSocketName);

                // location hal deamon has restarted, need to set this
                // flag to false to prevent messages to be sent to hal
                // before registeration completes
                mApiImpl.mHalRegistered = false;
                mListener.onListenerReady();
                break;
            }

            case E_LOCAPI_START_TRACKING_MSG_ID:
            case E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID:
            case E_LOCAPI_STOP_TRACKING_MSG_ID:
            case E_LOCAPI_START_BATCHING_MSG_ID:
            case E_LOCAPI_STOP_BATCHING_MSG_ID:
            case E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID:
            {
                LOC_LOGd("<<< response message %d\n", locApiMsg.msgId);
                PBLocAPIGenericRespMsg pbLocApiGenericRsp;
                if (0 == pbLocApiGenericRsp.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse pbLocApiGenericRsp from payload!!");
                    return;
                }
                if (locApiMsg.msgId != E_LOCAPI_STOP_TRACKING_MSG_ID) {
                    LocAPIGenericRespMsg respMsg(sockName.c_str(), eLocMsgid, pbLocApiGenericRsp,
                            &mApiImpl.mPbufMsgConv);
                    mApiImpl.invokePositionSessionResponseCb(respMsg.err);
                }
                break;
            }

            case E_LOCAPI_ADD_GEOFENCES_MSG_ID:
            case E_LOCAPI_REMOVE_GEOFENCES_MSG_ID:
            case E_LOCAPI_MODIFY_GEOFENCES_MSG_ID:
            case E_LOCAPI_PAUSE_GEOFENCES_MSG_ID:
            case E_LOCAPI_RESUME_GEOFENCES_MSG_ID:
            {
                LOC_LOGd("<<< collective response message, msgId = %d", locApiMsg.msgId);
                if (mApiImpl.mLocationCbs.collectiveResponseCb) {
                    PBLocAPICollectiveRespMsg pbLocApiCollctvRespMsg;
                    if (0 == pbLocApiCollctvRespMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiCollctvRespMsg from payload!!");
                        return;
                    }
                    LocAPICollectiveRespMsg msg(sockName.c_str(), eLocMsgid,
                            pbLocApiCollctvRespMsg, &mApiImpl.mPbufMsgConv);
                    const LocAPICollectiveRespMsg* pRespMsg = (LocAPICollectiveRespMsg*)(&msg);
                    int count = pRespMsg->collectiveRes.resp.size();
                    LOC_LOGd("CollectiveRes Pload count:%d", count);
                    LocationError* errs = new LocationError[count];
                    uint32_t* ids = new uint32_t[count];
                    if (errs != nullptr && ids != nullptr) {
                        for (int i=0; i < count; i++) {
                            ids[i] = pRespMsg->collectiveRes.resp[i].clientId;
                            errs[i] = pRespMsg->collectiveRes.resp[i].error;
                        }
                    }
                    if (mApiImpl.isGeofenceMapEmpty()) {
                        mApiImpl.clearSubscriptions(GEOFENCE_CBS);
                    }
                    mApiImpl.mLocationCbs.collectiveResponseCb(count, errs, ids);
                    for (int i=0; i < count; i++) {
                        if ((LOCATION_ERROR_SUCCESS !=
                                pRespMsg->collectiveRes.resp[i].error) ||
                                (E_LOCAPI_REMOVE_GEOFENCES_MSG_ID == locApiMsg.msgId)) {
                            mApiImpl.eraseGeofenceMap(1, const_cast<uint32_t*>(
                                    &(pRespMsg->collectiveRes.resp[i].clientId)));
                        }
                    }
                    if (ids) {
                        delete[] ids;
                    }
                    if (errs) {
                        delete[] errs;
                    }
                }
                if (mApiImpl.mPositionSessionResponseCbPending) {
                    mApiImpl.mPositionSessionResponseCbPending = false;
                }
                break;
            }

            // async indication messages
            case E_LOCAPI_LOCATION_MSG_ID:
            {
                LOC_LOGd("<<< message = simple location");
                PBLocAPILocationIndMsg pbLocApiLocIndMsg;
                if (0 == pbLocApiLocIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse pbLocApiLocIndMsg from payload!!");
                    return;
                }
                LocAPILocationIndMsg msg(sockName.c_str(), pbLocApiLocIndMsg,
                        &mApiImpl.mPbufMsgConv);
                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false) &&
                        (mApiImpl.mCallbacksMask & E_LOC_CB_TRACKING_BIT)) {
                    const LocAPILocationIndMsg* pLocationIndMsg = (LocAPILocationIndMsg*)(&msg);
                    if (mApiImpl.mLocationCbs.trackingCb) {
                        mApiImpl.mLocationCbs.trackingCb(pLocationIndMsg->locationNotification);
                    }
                }
                break;
            }

            case E_LOCAPI_BATCHING_MSG_ID:
            {
                LOC_LOGd("<<< message = batching");
                bool repStatusDone = false;

                if (mApiImpl.mCallbacksMask & E_LOC_CB_BATCHING_BIT) {
                    PBLocAPIBatchingIndMsg pbLocApiBatchIndMsg;
                    if (0 == pbLocApiBatchIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiBatchIndMsg from payload!!");
                        return;
                    }
                    LocAPIBatchingIndMsg msg(sockName.c_str(), pbLocApiBatchIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    const LocAPIBatchingIndMsg* pBatchingIndMsg =
                            (LocAPIBatchingIndMsg*)(&msg);
                    ::BatchingStatus batchStatus = pBatchingIndMsg->batchNotification.status;
                    if (BATCHING_STATUS_TRIP_COMPLETED == batchStatus) {
                        mApiImpl.stopBatching(0);
                        repStatusDone = true;
                    } else if (
                        (BATCHING_STATUS_POSITION_AVAILABE != batchStatus) &&
                        (BATCHING_STATUS_POSITION_UNAVAILABLE != batchStatus)) {
                        LOC_LOGe("invalid Batching Status!");
                        break;
                    } else if (BATCHING_STATUS_POSITION_AVAILABE == batchStatus) {
                        int batchCount = pBatchingIndMsg->batchNotification.location.size();
                        LOC_LOGd("Batch count : %d", batchCount);
                        for (int i=0; i < batchCount; i++) {
                            Location location = LocationClientApiImpl::parseLocation(
                                    pBatchingIndMsg->batchNotification.location[i]);
                            mApiImpl.logLocation(location,
                                    BATCHING_MODE_ROUTINE == pBatchingIndMsg->batchingMode ?
                                    LOC_REPORT_TRIGGER_ROUTINE_BATCHING_SESSION :
                                    LOC_REPORT_TRIGGER_TRIP_BATCHING_SESSION);
                        }
                    }

                    if (mApiImpl.mLocationCbs.batchingCb) {
                        // note:: batchingOpts is not used by LCA clients today.
                        BatchingOptions batchingOpts = {};
                        mApiImpl.mLocationCbs.batchingCb(
                                pBatchingIndMsg->batchNotification.location.size(),
                                (::Location *)pBatchingIndMsg->batchNotification.location.data(),
                                batchingOpts);
                    }

                    if ((repStatusDone) && (mApiImpl.mLocationCbs.batchingStatusCb)) {
                        BatchingStatusInfo statusInfo =
                                {sizeof(BatchingStatusInfo), BATCHING_STATUS_TRIP_COMPLETED};
                        std::list<uint32_t> listOfCompletedTrips;
                        mApiImpl.mLocationCbs.batchingStatusCb(statusInfo, listOfCompletedTrips);
                    }
                }
                break;
            }

            case E_LOCAPI_GEOFENCE_BREACH_MSG_ID:
            {
                LOC_LOGd("<<< message = geofence breach");
                if (mApiImpl.mCallbacksMask & E_LOC_CB_GEOFENCE_BREACH_BIT &&
                        mApiImpl.mLocationCbs.geofenceBreachCb) {
                    PBLocAPIGeofenceBreachIndMsg pbLocApiGfBreachIndMsg;
                    if (0 == pbLocApiGfBreachIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiGfBreachIndMsg from payload!!");
                        return;
                    }
                    LocAPIGeofenceBreachIndMsg msg(sockName.c_str(), pbLocApiGfBreachIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    const LocAPIGeofenceBreachIndMsg* pGfBreachIndMsg =
                        (LocAPIGeofenceBreachIndMsg*)(&msg);

                    GeofenceBreachNotification gfBrNotif;
                    gfBrNotif.size = sizeof(GeofenceBreachNotification);
                    gfBrNotif.count = pGfBreachIndMsg->gfBreachNotification.id.size();
                    gfBrNotif.timestamp = pGfBreachIndMsg->gfBreachNotification.timestamp;
                    gfBrNotif.location = pGfBreachIndMsg->gfBreachNotification.location;
                    gfBrNotif.type = LocationClientApiImpl::parseGeofenceBreachTypeMask(
                            pGfBreachIndMsg->gfBreachNotification.type);
                    gfBrNotif.ids = (uint32_t *)malloc(sizeof(uint32_t) * gfBrNotif.count);
                    std::vector<Geofence> geofences;
                    for (int i=0; i < gfBrNotif.count; i++) {
                        gfBrNotif.ids[i] = pGfBreachIndMsg->gfBreachNotification.id[i];
                        geofences.push_back(mApiImpl.getMappedGeofence(gfBrNotif.ids[i]));
                    }

                    if (mApiImpl.mLocationCbs.geofenceBreachCb) {
                        mApiImpl.mLocationCbs.geofenceBreachCb(gfBrNotif);
                    }
                    mApiImpl.logGeofenceBreach(gfBrNotif, geofences);
                    free(gfBrNotif.ids);
                }
                break;
            }

            case E_LOCAPI_LOCATION_INFO_MSG_ID:
            {
                LOC_LOGd("<<< message = location info");
                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false) &&
                        (mApiImpl.mCallbacksMask & E_LOC_CB_GNSS_LOCATION_INFO_BIT) &&
                        (mApiImpl.mLocationCbs.gnssLocationInfoCb)) {
                    PBLocAPILocationInfoIndMsg pbLocApiLocInfoIndMsg;
                    if (0 == pbLocApiLocInfoIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiLocInfoIndMsg from payload!!");
                        return;
                    }
                    LocAPILocationInfoIndMsg msg(sockName.c_str(), pbLocApiLocInfoIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    const LocAPILocationInfoIndMsg* pLocationInfoIndMsg =
                        (LocAPILocationInfoIndMsg*)(&msg);
                    mApiImpl.mLocationCbs.gnssLocationInfoCb(
                            pLocationInfoIndMsg->gnssLocationInfoNotification);
                }
                break;
            }
            case E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID:
            {
                LOC_LOGd("<<< message = engine location info\n");

                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false) &&
                        (mApiImpl.mCallbacksMask & E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT)) {
                    PBLocAPIEngineLocationsInfoIndMsg pbLocApiEngLocInfoIndMsg;
                    if (0 == pbLocApiEngLocInfoIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiEngLocInfoIndMsg from payload!!");
                        return;
                    }
                    LocAPIEngineLocationsInfoIndMsg msg(sockName.c_str(),
                            pbLocApiEngLocInfoIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    const LocAPIEngineLocationsInfoIndMsg* pEngLocationsInfoIndMsg =
                            (LocAPIEngineLocationsInfoIndMsg*)(&msg);

                    if (pEngLocationsInfoIndMsg->getMsgSize() != payloadSize) {
                        LOC_LOGw("payload size does not match for message with id: %d",
                                locApiMsg.msgId);
                    }
                    mApiImpl.mLocationCbs.engineLocationsInfoCb(
                            pEngLocationsInfoIndMsg->count,
                            (GnssLocationInfoNotification*)
                            pEngLocationsInfoIndMsg->engineLocationsInfo);
                }
                break;
            }

            case E_LOCAPI_SATELLITE_VEHICLE_MSG_ID:
            {
                LOC_LOGd("<<< message = sv");
                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false) &&
                        (mApiImpl.mCallbacksMask & E_LOC_CB_GNSS_SV_BIT) &&
                        (mApiImpl.mLocationCbs.gnssSvCb)) {
                    PBLocAPISatelliteVehicleIndMsg pbLocApiSatVehIndMsg;
                    if (0 == pbLocApiSatVehIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiSatVehIndMsg from payload!!");
                        return;
                    }
                    LocAPISatelliteVehicleIndMsg msg(sockName.c_str(), pbLocApiSatVehIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    const LocAPISatelliteVehicleIndMsg* pSvIndMsg =
                        (LocAPISatelliteVehicleIndMsg*)(&msg);
                    mApiImpl.mLocationCbs.gnssSvCb(pSvIndMsg->gnssSvNotification);
                }
                break;
            }

            case E_LOCAPI_NMEA_MSG_ID:
            {
                LOC_LOGd("<<< message = nmea");
                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false) &&
                        (mApiImpl.mCallbacksMask & (E_LOC_CB_GNSS_NMEA_BIT |
                        E_LOC_CB_ENGINE_NMEA_BIT)) &&
                        (mApiImpl.mLocationCbs.gnssNmeaCb || mApiImpl.mLocationCbs.engineNmeaCb)) {

                    PBLocAPINmeaIndMsg pbLocApiNmeaIndMsg;
                    if (0 == pbLocApiNmeaIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiNmeaIndMsg from payload!!");
                        return;
                    }
                    LocAPINmeaIndMsg msg(sockName.c_str(), pbLocApiNmeaIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    // nmea is variable length, can not be checked
                    const LocAPINmeaIndMsg* pNmeaIndMsg = (LocAPINmeaIndMsg*)(&msg);
                    ::GnssNmeaNotification nmeaNotif = {};
                    nmeaNotif.size = sizeof(GnssNmeaNotification);
                    nmeaNotif.timestamp = pNmeaIndMsg->gnssNmeaNotification.timestamp;
                    nmeaNotif.locOutputEngType =
                            pNmeaIndMsg->gnssNmeaNotification.locOutputEngType;
                    nmeaNotif.nmea = pNmeaIndMsg->gnssNmeaNotification.nmea.c_str();
                    nmeaNotif.length = pNmeaIndMsg->gnssNmeaNotification.nmea.length();
                    if (mApiImpl.mLocationCbs.gnssNmeaCb) {
                        mApiImpl.mLocationCbs.gnssNmeaCb(nmeaNotif);
                    }
                    if (mApiImpl.mLocationCbs.engineNmeaCb) {
                        mApiImpl.mLocationCbs.engineNmeaCb(nmeaNotif);
                    }

                }
                break;
            }

            case E_LOCAPI_DATA_MSG_ID:
            {
                LOC_LOGd("<<< message = data");
                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false) &&
                        (mApiImpl.mCallbacksMask & E_LOC_CB_GNSS_DATA_BIT) &&
                        (mApiImpl.mLocationCbs.gnssDataCb)) {
                    PBLocAPIDataIndMsg pbLocApiDataIndMsg;
                    if (0 == pbLocApiDataIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiDataIndMsg from payload!!");
                        return;
                    }
                    LocAPIDataIndMsg msg(sockName.c_str(), pbLocApiDataIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    mApiImpl.mLocationCbs.gnssDataCb(msg.gnssDataNotification);
                }
                break;
            }

            case E_LOCAPI_DC_REPORT_MSG_ID:
            {
                LOC_LOGd("<<< message = DC report");
                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false) &&
                        (mApiImpl.mCallbacksMask & E_LOC_CB_GNSS_DC_REPORT_BIT) &&
                        (mApiImpl.mLocationCbs.gnssDcReportCb)) {
                    PBLocAPIDcReportIndMsg pbMsg;
                    if (0 == pbMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse DC report from payload!!");
                        return;
                    }
                    LocAPIDcReportIndMsg msg(sockName.c_str(), pbMsg, &mApiImpl.mPbufMsgConv);
                    mApiImpl.mLocationCbs.gnssDcReportCb(msg.dcReportInfo);
                }
                break;
            }

            case E_LOCAPI_MEAS_MSG_ID:
            {
                LOC_LOGd("<<< message = measurements");
                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false)) {
                    PBLocAPIMeasIndMsg pbLocApiMeasIndMsg;
                    if (0 == pbLocApiMeasIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiMeasIndMsg from payload!!");
                        return;
                    }
                    LocAPIMeasIndMsg msg(sockName.c_str(), pbLocApiMeasIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    const LocAPIMeasIndMsg* pMeasIndMsg = (LocAPIMeasIndMsg*)(&msg);
                    if (pMeasIndMsg->gnssMeasurementsNotification.isNhz) {
                        if ((mApiImpl.mCallbacksMask & E_LOC_CB_GNSS_NHZ_MEAS_BIT) &&
                                (nullptr != mApiImpl.mLocationCbs.gnssNHzMeasurementsCb)) {
                            mApiImpl.mLocationCbs.gnssNHzMeasurementsCb(
                                    pMeasIndMsg->gnssMeasurementsNotification);
                        }
                    } else {
                        if ((mApiImpl.mCallbacksMask & E_LOC_CB_GNSS_MEAS_BIT) &&
                                (nullptr != mApiImpl.mLocationCbs.gnssMeasurementsCb)) {
                            mApiImpl.mLocationCbs.gnssMeasurementsCb(
                                    pMeasIndMsg->gnssMeasurementsNotification);
                        }
                    }
                }
                break;
            }

            case E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID:
            {
                LOC_LOGd("<<< message = GNSS power consumption\n");
                PBLocAPIGnssEnergyConsumedIndMsg pbLocApiGnssEnergyConsmdIndMsg;
                if (0 == pbLocApiGnssEnergyConsmdIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse pbLocApiGnssEnergyConsmdIndMsg from payload!!");
                    return;
                }
                LocAPIGnssEnergyConsumedIndMsg msg(sockName.c_str(),
                        pbLocApiGnssEnergyConsmdIndMsg,
                        &mApiImpl.mPbufMsgConv);
                LocAPIGnssEnergyConsumedIndMsg* pEnergyMsg =
                        (LocAPIGnssEnergyConsumedIndMsg*) &msg;
                uint64_t energyNumber = pEnergyMsg->totalGnssEnergyConsumedSinceFirstBoot;
                uint32_t flags = 0;
                if (energyNumber != 0xffffffffffffffff) {
                    flags = ENERGY_CONSUMED_SINCE_FIRST_BOOT_BIT;
                }
                ::GnssEnergyConsumedInfo energyConsumedInfo = {};
                energyConsumedInfo.flags =
                    (::GnssEnergyConsumedInfoMask) flags;
                energyConsumedInfo.totalEnergyConsumedSinceFirstBoot = energyNumber;
                if (0 == flags && mApiImpl.mGnssEnergyConsumedResponseCb) {
                    mApiImpl.mGnssEnergyConsumedResponseCb(
                        LOCATION_ERROR_ID_UNKNOWN, 0);
                } else if (mApiImpl.mGnssEnergyConsumedInfoCb){
                    mApiImpl.mGnssEnergyConsumedInfoCb(energyConsumedInfo);
                }
                break;
            }

            case E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID:
            {
                LOC_LOGd("<<< message = location system info");
                if (mApiImpl.mCallbacksMask & E_LOC_CB_SYSTEM_INFO_BIT) {
                    PBLocAPILocationSystemInfoIndMsg pbLocApiLocSysInfoIndMsg;
                    if (0 == pbLocApiLocSysInfoIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiLocSysInfoIndMsg from payload!!");
                        return;
                    }
                    LocAPILocationSystemInfoIndMsg msg(sockName.c_str(), pbLocApiLocSysInfoIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    const LocAPILocationSystemInfoIndMsg * pDataIndMsg =
                            (LocAPILocationSystemInfoIndMsg*)(&msg);
                    if (mApiImpl.mLocationSysInfoCb) {
                        mApiImpl.mLocationSysInfoCb(pDataIndMsg->locationSystemInfo);
                    }
                }
                break;
            }

            case E_LOCAPI_GET_DEBUG_RESP_MSG_ID:
            {
                PBLocAPIGetDebugRespMsg getDebugRespMsg;
                if (0 == getDebugRespMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse cfgGetDebugRespMsg from payload!!");
                    return;
                }
                LocAPIGetDebugRespMsg msg(sockName.c_str(),
                        getDebugRespMsg, &mApiImpl.mPbufMsgConv);
                mApiImpl.processGetDebugRespCb((LocAPIGetDebugRespMsg*)&msg);
                break;
            }

            case E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID:
            {
                LOC_LOGd("<<< message = terrestrial pos info");
                if (mApiImpl.mSingleTerrestrialPosCb) {
                    PBLocAPIGetSingleTerrestrialPosRespMsg pbMsg;
                    if (0 == pbMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse PBLocAPIGetSingleTerrestrialPosRespMsg!!");
                        return;
                    }

                    LocAPIGetSingleTerrestrialPosRespMsg msg(sockName.c_str(), pbMsg,
                                                             &mApiImpl.mPbufMsgConv);
                    if (mApiImpl.mSingleTerrestrialPosRespCb) {
                        mApiImpl.mSingleTerrestrialPosRespCb(msg.mErrorCode, 0);
                    }
                    if (::LOCATION_ERROR_SUCCESS == msg.mErrorCode) {
                        mApiImpl.mSingleTerrestrialPosCb(msg.mLocation);
                    }
                    // clean up variable to indicate that no request is pending
                    mApiImpl.mSingleTerrestrialPosRespCb = nullptr;
                    mApiImpl.mSingleTerrestrialPosCb = nullptr;
                }
                break;
            }

            case E_LOCAPI_ANTENNA_INFO_MSG_ID:
            {
                PBLocAPIAntennaInfoMsg antennaInfoMsg;
                if (0 == antennaInfoMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse PBLocAPIAntennaInfoMsg from payload!!");
                    return;
                }
                LocAPIAntennaInfoMsg msg(sockName.c_str(),
                    antennaInfoMsg, &mApiImpl.mPbufMsgConv);
                mApiImpl.processAntennaInfo((LocAPIAntennaInfoMsg*)&msg);
                break;
            }

            case E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID:
            {
                LOC_LOGd("<<< message = single fused pos info");
                if (mApiImpl.mSinglePosCb) {
                    PBLocAPIGetSinglePosRespMsg pbMsg;
                    if (0 == pbMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse PBLocAPIGetSinglePosRespMsg!!");
                        return;
                    }

                    LocAPIGetSinglePosRespMsg msg(sockName.c_str(), pbMsg,
                                                  &mApiImpl.mPbufMsgConv);
                    if (mApiImpl.mSinglePosRespCb) {
                        mApiImpl.mSinglePosRespCb(msg.mErrorCode, 0);
                    }
                    if (msg.mErrorCode == ::LOCATION_ERROR_SUCCESS ||
                            msg.mErrorCode == ::LOCATION_ERROR_TIMEOUT) {
                        mApiImpl.mSinglePosCb(msg.mLocation);
                    }
                    // clean up variable to indicate that no request is pending
                    mApiImpl.mSinglePosRespCb = nullptr;
                    mApiImpl.mSinglePosCb = nullptr;
                }
                break;
            }

            case E_LOCAPI_PINGTEST_MSG_ID:
            {
                LOC_LOGd("<<< ping message %d", locApiMsg.msgId);
                PBLocAPIPingTestIndMsg pbLocApiPingTestIndMsg;
                if (0 == pbLocApiPingTestIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse pbLocApiPingTestIndMsg from payload!!");
                    return;
                }
                LocAPIPingTestIndMsg msg(sockName.c_str(), pbLocApiPingTestIndMsg,
                        &mApiImpl.mPbufMsgConv);
                const LocAPIPingTestIndMsg* pIndMsg = (LocAPIPingTestIndMsg*)(&msg);
                if (mApiImpl.mPingTestCb) {
                    uint32_t response = pIndMsg->data[0];
                    mApiImpl.mPingTestCb(response);
                }
                break;
            }
            case E_LOCAPI_EPH_MSG_ID:
            {
                LOC_LOGd("<<< message = Ephemeris");
                if ((mApiImpl.mSessionId != LOCATION_CLIENT_SESSION_ID_INVALID) &&
                        (mApiImpl.mPositionSessionResponseCbPending == false)) {
                    PBLocAPIEphIndMsg pbLocApiEphIndMsg;
                    if (0 == pbLocApiEphIndMsg.ParseFromString(pbLocApiMsg.payload())) {
                        LOC_LOGe("Failed to parse pbLocApiEphIndMsg from payload!!");
                        return;
                    }
                    LocAPIEphIndMsg msg(sockName.c_str(), pbLocApiEphIndMsg,
                            &mApiImpl.mPbufMsgConv);
                    const LocAPIEphIndMsg* pEphIndMsg = (LocAPIEphIndMsg*)(&msg);
                    if ((mApiImpl.mCallbacksMask & E_LOC_CB_GNSS_EPH_BIT) &&
                            (nullptr != mApiImpl.mLocationCbs.svEphemerisCb)) {
                        mApiImpl.mLocationCbs.svEphemerisCb(
                                pEphIndMsg->gnssEphNotification);
                    }
                }
                break;
            }

            default:
            {
                LOC_LOGe("<<< unknown message %d\n", locApiMsg.msgId);
                break;
            }
            }
        }
        LocationClientApiImpl& mApiImpl;
        IpcListener& mListener;
        const string mMsgData;
    };
    mMsgTask.sendMsg(new (nothrow) OnReceiveHandler(mApiImpl, *this, data, length));
}

/******************************************************************************
LocationClientApiImpl - Not implemented overrides
******************************************************************************/

void LocationClientApiImpl::gnssNiResponse(uint32_t id, GnssNiResponse response) {
}


static ILocationAPI* gLocationClientApiImpl = nullptr;
static mutex gMutexForCreate;
extern "C" ILocationAPI* getLocationClientApiImpl(CapabilitiesCb capabitiescb)
{
    lock_guard<mutex> lock(gMutexForCreate);

    if (nullptr == gLocationClientApiImpl) {
        gLocationClientApiImpl = new LocationClientApiImpl(capabitiescb);
    }

    return gLocationClientApiImpl;
}

} // namespace location_client
